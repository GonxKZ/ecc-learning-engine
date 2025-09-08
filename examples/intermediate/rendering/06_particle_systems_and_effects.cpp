/**
 * @file 06_particle_systems_and_effects.cpp
 * @brief Tutorial 6: Particle Systems and Visual Effects - Dynamic Visual Content
 * 
 * This tutorial explores particle systems and dynamic visual effects in 2D rendering.
 * You'll learn how to create, manage, and optimize thousands of animated particles.
 * 
 * Learning Objectives:
 * 1. Understand particle system architecture and components
 * 2. Learn particle lifecycle management and update systems
 * 3. Explore different particle behaviors and physics simulation
 * 4. Master efficient rendering techniques for large particle counts
 * 5. Create various visual effects using particle systems
 * 
 * Key Concepts Covered:
 * - Particle system architecture and data structures
 * - Emitter systems and particle spawning patterns
 * - Particle physics: velocity, acceleration, forces
 * - Particle rendering optimizations and instancing
 * - Visual effects: fire, smoke, explosions, magic
 * - GPU-based particle systems and compute shaders
 * 
 * Educational Value:
 * Particle systems are essential for creating dynamic, engaging visual content
 * in games and applications. This tutorial provides both theoretical knowledge
 * and practical implementation techniques for high-performance particle rendering.
 * 
 * @author ECScope Educational ECS Framework
 * @date 2024
 */

#include <iostream>
#include <memory>
#include <vector>
#include <random>
#include <algorithm>
#include <chrono>
#include <cmath>

// ECScope Core
#include "../../src/core/types.hpp"
#include "../../src/core/log.hpp"
#include "../../src/core/time.hpp"

// ECS System
#include "../../src/ecs/registry.hpp"
#include "../../src/ecs/components/transform.hpp"

// 2D Rendering System
#include "../../src/renderer/renderer_2d.hpp"
#include "../../src/renderer/batch_renderer.hpp"
#include "../../src/renderer/components/render_components.hpp"
#include "../../src/renderer/window.hpp"

using namespace ecscope;
using namespace ecscope::renderer;
using namespace ecscope::renderer::components;

/**
 * @brief Particle Systems and Visual Effects Tutorial
 * 
 * This tutorial demonstrates particle system implementation through various
 * visual effects with performance analysis and optimization techniques.
 */
class ParticleSystemsTutorial {
public:
    ParticleSystemsTutorial() = default;
    ~ParticleSystemsTutorial() { cleanup(); }
    
    bool initialize() {
        core::Log::info("Tutorial", "=== Particle Systems and Visual Effects Tutorial ===");
        core::Log::info("Tutorial", "Learning objective: Master particle systems for dynamic visual effects");
        
        // Initialize window and renderer
        window_ = std::make_unique<Window>("Tutorial 6: Particle Systems", 1600, 1200);
        if (!window_->initialize()) {
            core::Log::error("Tutorial", "Failed to create window");
            return false;
        }
        
        // Configure renderer for particle rendering
        Renderer2DConfig renderer_config = Renderer2DConfig::performance_focused();
        renderer_config.debug.show_performance_overlay = true;
        renderer_config.debug.collect_gpu_timings = true;
        renderer_config.rendering.enable_instanced_rendering = true; // For particle optimization
        
        renderer_ = std::make_unique<Renderer2D>(renderer_config);
        if (!renderer_->initialize().is_ok()) {
            core::Log::error("Tutorial", "Failed to initialize renderer");
            return false;
        }
        
        // Set up camera
        camera_ = Camera2D::create_main_camera(1600, 1200);
        camera_.set_position(0.0f, 0.0f);
        camera_.set_zoom(1.0f);
        
        // Create ECS registry
        registry_ = std::make_unique<ecs::Registry>();
        
        // Initialize random number generation
        random_engine_.seed(std::chrono::high_resolution_clock::now().time_since_epoch().count());
        
        core::Log::info("Tutorial", "System initialized. Creating particle systems...");
        
        // Create particle system templates
        create_particle_system_templates();
        
        return true;
    }
    
    void run() {
        if (!window_ || !renderer_) return;
        
        core::Log::info("Tutorial", "Starting particle systems demonstration...");
        
        // Run particle system demonstrations
        demonstrate_basic_particle_concepts();
        demonstrate_emitter_systems();
        demonstrate_particle_physics();
        demonstrate_visual_effects();
        demonstrate_performance_optimization();
        demonstrate_gpu_particles();
        
        display_educational_summary();
    }
    
private:
    //=========================================================================
    // Particle System Data Structures
    //=========================================================================
    
    struct Particle {
        Vec2 position{0.0f, 0.0f};
        Vec2 velocity{0.0f, 0.0f};
        Vec2 acceleration{0.0f, 0.0f};
        
        Color color{255, 255, 255, 255};
        f32 scale{1.0f};
        f32 rotation{0.0f};
        
        f32 life_time{1.0f};        // Total lifetime
        f32 remaining_life{1.0f};   // Time remaining
        f32 age{0.0f};              // Age (0 to lifetime)
        
        u16 texture_id{0};
        bool is_active{false};
        
        // Animation properties
        f32 scale_start{1.0f};
        f32 scale_end{1.0f};
        Color color_start{255, 255, 255, 255};
        Color color_end{255, 255, 255, 0};
        f32 angular_velocity{0.0f};
    };
    
    struct ParticleEmitter {
        Vec2 position{0.0f, 0.0f};
        Vec2 direction{0.0f, -1.0f}; // Default upward
        f32 spread_angle{3.14159f / 6.0f}; // 30 degrees spread
        
        f32 emission_rate{10.0f};    // Particles per second
        f32 emission_timer{0.0f};
        
        // Particle properties
        f32 particle_speed{100.0f};
        f32 speed_variation{20.0f};
        f32 particle_lifetime{2.0f};
        f32 lifetime_variation{0.5f};
        
        Vec2 gravity{0.0f, 150.0f};  // Downward gravity
        f32 drag{0.1f};              // Air resistance
        
        Color start_color{255, 255, 255, 255};
        Color end_color{255, 255, 255, 0};
        
        f32 start_scale{1.0f};
        f32 end_scale{0.0f};
        
        bool is_active{true};
        u32 max_particles{1000};
        u16 particle_texture{1};
        
        // Emitter shape
        enum class Shape { Point, Line, Circle, Rectangle } shape{Shape::Point};
        f32 shape_size{0.0f}; // Radius for circle, half-width for line/rect
    };
    
    struct ParticleSystem {
        std::string name;
        std::string description;
        std::vector<Particle> particles;
        ParticleEmitter emitter;
        
        // Performance metrics
        u32 active_particle_count{0};
        f32 update_time_ms{0.0f};
        f32 render_time_ms{0.0f};
    };
    
    //=========================================================================
    // Particle System Templates
    //=========================================================================
    
    void create_particle_system_templates() {
        core::Log::info("Templates", "Creating particle system effect templates");
        
        // Fire effect
        ParticleSystem fire_system;
        fire_system.name = "Fire Effect";
        fire_system.description = "Upward-flowing fire with color transition";
        setup_fire_system(fire_system);
        particle_systems_["fire"] = fire_system;
        
        // Smoke effect
        ParticleSystem smoke_system;
        smoke_system.name = "Smoke Effect";
        smoke_system.description = "Rising smoke with wind drift";
        setup_smoke_system(smoke_system);
        particle_systems_["smoke"] = smoke_system;
        
        // Explosion effect
        ParticleSystem explosion_system;
        explosion_system.name = "Explosion Effect";
        explosion_system.description = "Radial explosion with debris";
        setup_explosion_system(explosion_system);
        particle_systems_["explosion"] = explosion_system;
        
        // Magic sparkles
        ParticleSystem magic_system;
        magic_system.name = "Magic Sparkles";
        magic_system.description = "Sparkling magical effect with orbiting motion";
        setup_magic_system(magic_system);
        particle_systems_["magic"] = magic_system;
        
        // Rain effect
        ParticleSystem rain_system;
        rain_system.name = "Rain Effect";
        rain_system.description = "Falling rain drops with wind";
        setup_rain_system(rain_system);
        particle_systems_["rain"] = rain_system;
        
        // Fountain effect
        ParticleSystem fountain_system;
        fountain_system.name = "Fountain Effect";
        fountain_system.description = "Water fountain with gravity and splash";
        setup_fountain_system(fountain_system);
        particle_systems_["fountain"] = fountain_system;
        
        core::Log::info("Templates", "Created {} particle system templates", particle_systems_.size());
    }
    
    void setup_fire_system(ParticleSystem& system) {
        system.particles.resize(500);
        
        auto& emitter = system.emitter;
        emitter.position = {0.0f, 200.0f};
        emitter.direction = {0.0f, -1.0f}; // Upward
        emitter.spread_angle = 3.14159f / 4.0f; // 45 degrees
        emitter.emission_rate = 50.0f;
        emitter.particle_speed = 80.0f;
        emitter.speed_variation = 30.0f;
        emitter.particle_lifetime = 2.0f;
        emitter.lifetime_variation = 0.8f;
        emitter.gravity = {0.0f, -20.0f}; // Slight upward buoyancy
        emitter.drag = 0.2f;
        emitter.start_color = Color{255, 100, 20, 255}; // Orange-red
        emitter.end_color = Color{100, 0, 0, 0};        // Dark red to transparent
        emitter.start_scale = 0.5f;
        emitter.end_scale = 1.5f; // Fire grows as it rises
        emitter.shape = ParticleEmitter::Shape::Line;
        emitter.shape_size = 30.0f;
        
        core::Log::info("Fire", "Configured fire effect: upward flow with color transition");
    }
    
    void setup_smoke_system(ParticleSystem& system) {
        system.particles.resize(300);
        
        auto& emitter = system.emitter;
        emitter.position = {0.0f, 150.0f};
        emitter.direction = {0.2f, -1.0f}; // Slight wind drift
        emitter.spread_angle = 3.14159f / 6.0f; // 30 degrees
        emitter.emission_rate = 20.0f;
        emitter.particle_speed = 40.0f;
        emitter.speed_variation = 15.0f;
        emitter.particle_lifetime = 4.0f;
        emitter.lifetime_variation = 1.0f;
        emitter.gravity = {20.0f, -30.0f}; // Wind + slight upward
        emitter.drag = 0.3f;
        emitter.start_color = Color{200, 200, 200, 180}; // Light gray
        emitter.end_color = Color{150, 150, 150, 0};     // Darker gray to transparent
        emitter.start_scale = 0.8f;
        emitter.end_scale = 2.5f; // Smoke expands
        emitter.shape = ParticleEmitter::Shape::Circle;
        emitter.shape_size = 20.0f;
        
        core::Log::info("Smoke", "Configured smoke effect: rising with wind drift");
    }
    
    void setup_explosion_system(ParticleSystem& system) {
        system.particles.resize(800);
        
        auto& emitter = system.emitter;
        emitter.position = {0.0f, 0.0f};
        emitter.direction = {0.0f, 0.0f}; // Radial explosion
        emitter.spread_angle = 2.0f * 3.14159f; // Full circle
        emitter.emission_rate = 500.0f; // Burst emission
        emitter.particle_speed = 150.0f;
        emitter.speed_variation = 80.0f;
        emitter.particle_lifetime = 1.5f;
        emitter.lifetime_variation = 0.7f;
        emitter.gravity = {0.0f, 100.0f}; // Downward gravity
        emitter.drag = 0.4f; // High air resistance
        emitter.start_color = Color{255, 200, 100, 255}; // Bright yellow-orange
        emitter.end_color = Color{100, 50, 50, 0};       // Dark red to transparent
        emitter.start_scale = 1.0f;
        emitter.end_scale = 0.3f; // Debris shrinks
        emitter.shape = ParticleEmitter::Shape::Point;
        
        core::Log::info("Explosion", "Configured explosion effect: radial burst with gravity");
    }
    
    void setup_magic_system(ParticleSystem& system) {
        system.particles.resize(200);
        
        auto& emitter = system.emitter;
        emitter.position = {0.0f, 0.0f};
        emitter.direction = {0.0f, -1.0f};
        emitter.spread_angle = 2.0f * 3.14159f; // All directions
        emitter.emission_rate = 30.0f;
        emitter.particle_speed = 60.0f;
        emitter.speed_variation = 25.0f;
        emitter.particle_lifetime = 3.0f;
        emitter.lifetime_variation = 1.0f;
        emitter.gravity = {0.0f, 0.0f}; // No gravity
        emitter.drag = 0.1f;
        emitter.start_color = Color{100, 200, 255, 255}; // Bright cyan
        emitter.end_color = Color{200, 100, 255, 0};     // Purple to transparent
        emitter.start_scale = 0.3f;
        emitter.end_scale = 0.8f; // Sparkles grow slightly
        emitter.shape = ParticleEmitter::Shape::Circle;
        emitter.shape_size = 50.0f;
        
        core::Log::info("Magic", "Configured magic sparkles: orbiting particles with color shift");
    }
    
    void setup_rain_system(ParticleSystem& system) {
        system.particles.resize(1000);
        
        auto& emitter = system.emitter;
        emitter.position = {0.0f, -400.0f}; // Above screen
        emitter.direction = {0.1f, 1.0f}; // Slight angle downward
        emitter.spread_angle = 3.14159f / 12.0f; // 15 degrees
        emitter.emission_rate = 100.0f;
        emitter.particle_speed = 200.0f;
        emitter.speed_variation = 50.0f;
        emitter.particle_lifetime = 4.0f;
        emitter.lifetime_variation = 1.0f;
        emitter.gravity = {10.0f, 200.0f}; // Gravity + wind
        emitter.drag = 0.05f; // Low air resistance
        emitter.start_color = Color{150, 200, 255, 200}; // Light blue
        emitter.end_color = Color{100, 150, 200, 50};    // Darker blue, more transparent
        emitter.start_scale = 0.2f;
        emitter.end_scale = 0.1f; // Rain drops shrink slightly
        emitter.shape = ParticleEmitter::Shape::Line;
        emitter.shape_size = 400.0f; // Wide rain line
        
        core::Log::info("Rain", "Configured rain effect: falling drops with wind");
    }
    
    void setup_fountain_system(ParticleSystem& system) {
        system.particles.resize(400);
        
        auto& emitter = system.emitter;
        emitter.position = {0.0f, 100.0f};
        emitter.direction = {0.0f, -1.0f}; // Upward
        emitter.spread_angle = 3.14159f / 3.0f; // 60 degrees
        emitter.emission_rate = 60.0f;
        emitter.particle_speed = 120.0f;
        emitter.speed_variation = 40.0f;
        emitter.particle_lifetime = 3.0f;
        emitter.lifetime_variation = 0.5f;
        emitter.gravity = {0.0f, 150.0f}; // Strong downward gravity
        emitter.drag = 0.15f;
        emitter.start_color = Color{150, 200, 255, 220}; // Light blue water
        emitter.end_color = Color{100, 150, 200, 100};   // Darker, more transparent
        emitter.start_scale = 0.4f;
        emitter.end_scale = 0.6f; // Water drops spread
        emitter.shape = ParticleEmitter::Shape::Circle;
        emitter.shape_size = 15.0f;
        
        core::Log::info("Fountain", "Configured fountain effect: water arc with gravity");
    }
    
    //=========================================================================
    // Demonstration Functions
    //=========================================================================
    
    void demonstrate_basic_particle_concepts() {
        core::Log::info("Demo 1", "=== BASIC PARTICLE CONCEPTS ===");
        core::Log::info("Explanation", "Understanding particle lifecycle and basic physics simulation");
        
        // Start with a simple fire effect
        auto& fire_system = particle_systems_["fire"];
        fire_system.emitter.position = {0.0f, 200.0f};
        fire_system.emitter.is_active = true;
        
        core::Log::info("Demo", "Demonstrating fire particle system");
        core::Log::info("Physics", "Particles have: position, velocity, acceleration, lifetime");
        core::Log::info("Properties", "Fire: upward velocity, color transition, scale growth");
        
        // Run fire simulation
        f32 demo_duration = 8.0f; // 8 seconds
        u32 frames = static_cast<u32>(demo_duration * 60.0f);
        
        for (u32 frame = 0; frame < frames; ++frame) {
            f32 delta_time = 1.0f / 60.0f;
            
            update_particle_system(fire_system, delta_time);
            render_particle_system(fire_system);
            
            if (frame % 120 == 0) {
                core::Log::info("Update", "Active particles: {}, Update time: {:.3f}ms",
                               fire_system.active_particle_count, fire_system.update_time_ms);
            }
        }
        
        explain_particle_lifecycle();
    }
    
    void demonstrate_emitter_systems() {
        core::Log::info("Demo 2", "=== EMITTER SYSTEMS AND SPAWNING PATTERNS ===");
        core::Log::info("Explanation", "Different emitter shapes and particle spawning strategies");
        
        struct EmitterDemo {
            std::string name;
            std::string system_key;
            Vec2 position;
            f32 duration;
        };
        
        std::vector<EmitterDemo> demos = {
            {"Point Emitter (Explosion)", "explosion", {-300.0f, 0.0f}, 3.0f},
            {"Line Emitter (Fire)", "fire", {-100.0f, 200.0f}, 4.0f},
            {"Circle Emitter (Magic)", "magic", {100.0f, 0.0f}, 5.0f},
            {"Rain Emitter (Area)", "rain", {300.0f, -400.0f}, 4.0f}
        };
        
        for (const auto& demo : demos) {
            core::Log::info("Emitter Demo", "Demonstrating: {}", demo.name);
            
            auto& system = particle_systems_[demo.system_key];
            system.emitter.position = demo.position;
            system.emitter.is_active = true;
            
            // Clear existing particles
            for (auto& particle : system.particles) {
                particle.is_active = false;
            }
            
            u32 frames = static_cast<u32>(demo.duration * 60.0f);
            for (u32 frame = 0; frame < frames; ++frame) {
                f32 delta_time = 1.0f / 60.0f;
                
                update_particle_system(system, delta_time);
                render_particle_system(system);
                
                if (frame % 60 == 0) {
                    core::Log::info("Emitter", "{}: {} active particles, emission rate: {:.1f}/sec",
                                   demo.name, system.active_particle_count, system.emitter.emission_rate);
                }
            }
            
            system.emitter.is_active = false;
        }
        
        explain_emitter_shapes();
    }
    
    void demonstrate_particle_physics() {
        core::Log::info("Demo 3", "=== PARTICLE PHYSICS SIMULATION ===");
        core::Log::info("Explanation", "Forces, gravity, drag, and realistic particle motion");
        
        // Create physics comparison demo
        auto& fountain = particle_systems_["fountain"];
        fountain.emitter.position = {0.0f, 100.0f};
        
        struct PhysicsTest {
            const char* name;
            Vec2 gravity;
            f32 drag;
            f32 duration;
        };
        
        std::vector<PhysicsTest> physics_tests = {
            {"No Physics", {0.0f, 0.0f}, 0.0f, 3.0f},
            {"Gravity Only", {0.0f, 150.0f}, 0.0f, 3.0f},
            {"Gravity + Drag", {0.0f, 150.0f}, 0.2f, 3.0f},
            {"Wind + Gravity", {50.0f, 150.0f}, 0.15f, 4.0f}
        };
        
        for (const auto& test : physics_tests) {
            core::Log::info("Physics Test", "Testing: {}", test.name);
            
            // Configure physics
            fountain.emitter.gravity = test.gravity;
            fountain.emitter.drag = test.drag;
            fountain.emitter.is_active = true;
            
            // Clear existing particles
            for (auto& particle : fountain.particles) {
                particle.is_active = false;
            }
            
            u32 frames = static_cast<u32>(test.duration * 60.0f);
            for (u32 frame = 0; frame < frames; ++frame) {
                f32 delta_time = 1.0f / 60.0f;
                
                update_particle_system(fountain, delta_time);
                render_particle_system(fountain);
                
                if (frame % 90 == 0) {
                    core::Log::info("Physics", "{}: gravity({:.1f}, {:.1f}), drag: {:.2f}",
                                   test.name, test.gravity.x, test.gravity.y, test.drag);
                }
            }
            
            fountain.emitter.is_active = false;
        }
        
        explain_particle_physics();
    }
    
    void demonstrate_visual_effects() {
        core::Log::info("Demo 4", "=== VISUAL EFFECTS SHOWCASE ===");
        core::Log::info("Explanation", "Complex effects combining multiple particle systems");
        
        // Create multi-system effects
        struct EffectDemo {
            const char* name;
            std::vector<std::string> systems;
            f32 duration;
        };
        
        std::vector<EffectDemo> effect_demos = {
            {"Campfire", {"fire", "smoke"}, 6.0f},
            {"Magical Explosion", {"explosion", "magic"}, 4.0f},
            {"Stormy Weather", {"rain"}, 5.0f}
        };
        
        for (const auto& effect : effect_demos) {
            core::Log::info("Effect Demo", "Creating: {}", effect.name);
            
            // Position systems for combined effect
            if (effect.name == std::string("Campfire")) {
                particle_systems_["fire"].emitter.position = {0.0f, 150.0f};
                particle_systems_["smoke"].emitter.position = {0.0f, 100.0f};
            } else if (effect.name == std::string("Magical Explosion")) {
                particle_systems_["explosion"].emitter.position = {0.0f, 0.0f};
                particle_systems_["magic"].emitter.position = {0.0f, 0.0f};
            }
            
            // Activate systems
            for (const auto& system_name : effect.systems) {
                particle_systems_[system_name].emitter.is_active = true;
                
                // Clear existing particles
                for (auto& particle : particle_systems_[system_name].particles) {
                    particle.is_active = false;
                }
            }
            
            u32 frames = static_cast<u32>(effect.duration * 60.0f);
            for (u32 frame = 0; frame < frames; ++frame) {
                f32 delta_time = 1.0f / 60.0f;
                
                // Update all systems
                u32 total_particles = 0;
                for (const auto& system_name : effect.systems) {
                    auto& system = particle_systems_[system_name];
                    update_particle_system(system, delta_time);
                    total_particles += system.active_particle_count;
                }
                
                // Render all systems
                renderer_->begin_frame();
                renderer_->set_active_camera(camera_);
                
                for (const auto& system_name : effect.systems) {
                    render_particle_system(particle_systems_[system_name]);
                }
                
                renderer_->end_frame();
                window_->swap_buffers();
                window_->poll_events();
                
                if (frame % 90 == 0) {
                    core::Log::info("Effect", "{}: {} total active particles", 
                                   effect.name, total_particles);
                }
            }
            
            // Deactivate systems
            for (const auto& system_name : effect.systems) {
                particle_systems_[system_name].emitter.is_active = false;
            }
        }
    }
    
    void demonstrate_performance_optimization() {
        core::Log::info("Demo 5", "=== PERFORMANCE OPTIMIZATION TECHNIQUES ===");
        core::Log::info("Explanation", "Optimizing particle systems for high particle counts");
        
        // Test different particle counts and optimization techniques
        std::vector<u32> particle_counts = {100, 500, 1000, 2000, 5000};
        
        for (u32 count : particle_counts) {
            core::Log::info("Performance Test", "Testing {} particles", count);
            
            auto& test_system = particle_systems_["magic"];
            test_system.particles.resize(count);
            test_system.emitter.max_particles = count;
            test_system.emitter.emission_rate = static_cast<f32>(count) * 0.5f; // Fill quickly
            test_system.emitter.position = {0.0f, 0.0f};
            test_system.emitter.is_active = true;
            
            // Measure performance over time
            auto start_time = std::chrono::high_resolution_clock::now();
            f32 total_update_time = 0.0f;
            f32 total_render_time = 0.0f;
            u32 test_frames = 300; // 5 seconds
            
            for (u32 frame = 0; frame < test_frames; ++frame) {
                f32 delta_time = 1.0f / 60.0f;
                
                auto update_start = std::chrono::high_resolution_clock::now();
                update_particle_system(test_system, delta_time);
                auto update_end = std::chrono::high_resolution_clock::now();
                
                auto render_start = std::chrono::high_resolution_clock::now();
                render_particle_system(test_system);
                auto render_end = std::chrono::high_resolution_clock::now();
                
                total_update_time += std::chrono::duration<f32>(update_end - update_start).count();
                total_render_time += std::chrono::duration<f32>(render_end - render_start).count();
            }
            
            auto end_time = std::chrono::high_resolution_clock::now();
            f32 total_time = std::chrono::duration<f32>(end_time - start_time).count();
            f32 avg_fps = test_frames / total_time;
            f32 avg_update_ms = (total_update_time / test_frames) * 1000.0f;
            f32 avg_render_ms = (total_render_time / test_frames) * 1000.0f;
            
            core::Log::info("Performance", "{} particles: {:.1f} FPS, {:.3f}ms update, {:.3f}ms render",
                           count, avg_fps, avg_update_ms, avg_render_ms);
            
            performance_results_[count] = {avg_fps, avg_update_ms, avg_render_ms};
            
            test_system.emitter.is_active = false;
        }
        
        analyze_performance_results();
        explain_optimization_techniques();
    }
    
    void demonstrate_gpu_particles() {
        core::Log::info("Demo 6", "=== GPU-BASED PARTICLE SYSTEMS ===");
        core::Log::info("Explanation", "Advanced GPU compute shader particle simulation (simulated)");
        
        // Note: This would use compute shaders in a real implementation
        // For this tutorial, we'll simulate the concepts
        
        core::Log::info("GPU Particles", "Benefits of GPU-based particle systems:");
        core::Log::info("GPU Benefits", "- Massive parallelization (thousands of cores)");
        core::Log::info("GPU Benefits", "- Reduced CPU-GPU data transfer");
        core::Log::info("GPU Benefits", "- Hardware-accelerated physics calculations");
        core::Log::info("GPU Benefits", "- Instanced rendering for optimal draw calls");
        
        // Simulate GPU particle system performance
        struct GPUParticleTest {
            u32 particle_count;
            f32 simulated_fps;
            f32 simulated_update_time;
        };
        
        std::vector<GPUParticleTest> gpu_tests = {
            {10000, 58.0f, 0.5f},
            {50000, 55.0f, 1.2f},
            {100000, 52.0f, 2.1f},
            {500000, 45.0f, 4.8f}
        };
        
        core::Log::info("GPU Performance", "Simulated GPU particle system performance:");
        for (const auto& test : gpu_tests) {
            core::Log::info("GPU Test", "{} particles: {:.1f} FPS, {:.2f}ms update",
                           test.particle_count, test.simulated_fps, test.simulated_update_time);
        }
        
        explain_gpu_particle_architecture();
    }
    
    //=========================================================================
    // Particle System Update and Rendering
    //=========================================================================
    
    void update_particle_system(ParticleSystem& system, f32 delta_time) {
        auto update_start = std::chrono::high_resolution_clock::now();
        
        // Update emitter
        update_emitter(system.emitter, system.particles, delta_time);
        
        // Update particles
        system.active_particle_count = 0;
        for (auto& particle : system.particles) {
            if (particle.is_active) {
                update_particle(particle, system.emitter, delta_time);
                if (particle.is_active) {
                    system.active_particle_count++;
                }
            }
        }
        
        auto update_end = std::chrono::high_resolution_clock::now();
        system.update_time_ms = std::chrono::duration<f32>(update_end - update_start).count() * 1000.0f;
    }
    
    void update_emitter(ParticleEmitter& emitter, std::vector<Particle>& particles, f32 delta_time) {
        if (!emitter.is_active) return;
        
        // Update emission timer
        emitter.emission_timer += delta_time;
        
        // Calculate particles to emit this frame
        f32 emission_interval = 1.0f / emitter.emission_rate;
        u32 particles_to_emit = 0;
        
        while (emitter.emission_timer >= emission_interval) {
            particles_to_emit++;
            emitter.emission_timer -= emission_interval;
        }
        
        // Emit particles
        for (u32 i = 0; i < particles_to_emit; ++i) {
            emit_particle(emitter, particles);
        }
    }
    
    void emit_particle(const ParticleEmitter& emitter, std::vector<Particle>& particles) {
        // Find inactive particle
        Particle* particle = nullptr;
        for (auto& p : particles) {
            if (!p.is_active) {
                particle = &p;
                break;
            }
        }
        
        if (!particle) return; // No available particles
        
        // Initialize particle
        particle->is_active = true;
        
        // Position based on emitter shape
        particle->position = get_emission_position(emitter);
        
        // Velocity based on direction and spread
        Vec2 direction = get_emission_direction(emitter);
        f32 speed = emitter.particle_speed + 
                   (random_float(-1.0f, 1.0f) * emitter.speed_variation);
        particle->velocity = {direction.x * speed, direction.y * speed};
        
        // Initialize other properties
        particle->acceleration = {0.0f, 0.0f};
        particle->life_time = emitter.particle_lifetime + 
                             (random_float(-1.0f, 1.0f) * emitter.lifetime_variation);
        particle->remaining_life = particle->life_time;
        particle->age = 0.0f;
        
        particle->color_start = emitter.start_color;
        particle->color_end = emitter.end_color;
        particle->color = particle->color_start;
        
        particle->scale_start = emitter.start_scale;
        particle->scale_end = emitter.end_scale;
        particle->scale = particle->scale_start;
        
        particle->rotation = random_float(0.0f, 2.0f * 3.14159f);
        particle->angular_velocity = random_float(-2.0f, 2.0f);
        
        particle->texture_id = emitter.particle_texture;
    }
    
    Vec2 get_emission_position(const ParticleEmitter& emitter) {
        switch (emitter.shape) {
            case ParticleEmitter::Shape::Point:
                return emitter.position;
                
            case ParticleEmitter::Shape::Line:
                return {
                    emitter.position.x + random_float(-emitter.shape_size, emitter.shape_size),
                    emitter.position.y
                };
                
            case ParticleEmitter::Shape::Circle: {
                f32 angle = random_float(0.0f, 2.0f * 3.14159f);
                f32 radius = random_float(0.0f, emitter.shape_size);
                return {
                    emitter.position.x + std::cos(angle) * radius,
                    emitter.position.y + std::sin(angle) * radius
                };
            }
            
            case ParticleEmitter::Shape::Rectangle:
                return {
                    emitter.position.x + random_float(-emitter.shape_size, emitter.shape_size),
                    emitter.position.y + random_float(-emitter.shape_size, emitter.shape_size)
                };
        }
        
        return emitter.position;
    }
    
    Vec2 get_emission_direction(const ParticleEmitter& emitter) {
        if (emitter.spread_angle >= 2.0f * 3.14159f) {
            // Full circle - random direction
            f32 angle = random_float(0.0f, 2.0f * 3.14159f);
            return {std::cos(angle), std::sin(angle)};
        }
        
        // Direction with spread
        f32 base_angle = std::atan2(emitter.direction.y, emitter.direction.x);
        f32 spread = random_float(-emitter.spread_angle * 0.5f, emitter.spread_angle * 0.5f);
        f32 final_angle = base_angle + spread;
        
        return {std::cos(final_angle), std::sin(final_angle)};
    }
    
    void update_particle(Particle& particle, const ParticleEmitter& emitter, f32 delta_time) {
        // Update lifetime
        particle.remaining_life -= delta_time;
        particle.age += delta_time;
        
        if (particle.remaining_life <= 0.0f) {
            particle.is_active = false;
            return;
        }
        
        // Calculate life progress (0 to 1)
        f32 life_progress = particle.age / particle.life_time;
        
        // Apply forces
        particle.acceleration = emitter.gravity;
        
        // Apply drag
        Vec2 drag_force = {
            -particle.velocity.x * emitter.drag,
            -particle.velocity.y * emitter.drag
        };
        particle.acceleration.x += drag_force.x;
        particle.acceleration.y += drag_force.y;
        
        // Update velocity and position
        particle.velocity.x += particle.acceleration.x * delta_time;
        particle.velocity.y += particle.acceleration.y * delta_time;
        
        particle.position.x += particle.velocity.x * delta_time;
        particle.position.y += particle.velocity.y * delta_time;
        
        // Update visual properties based on age
        particle.scale = lerp(particle.scale_start, particle.scale_end, life_progress);
        particle.color = lerp_color(particle.color_start, particle.color_end, life_progress);
        
        // Update rotation
        particle.rotation += particle.angular_velocity * delta_time;
    }
    
    void render_particle_system(const ParticleSystem& system) {
        auto render_start = std::chrono::high_resolution_clock::now();
        
        renderer_->begin_frame();
        renderer_->set_active_camera(camera_);
        
        // Render all active particles
        for (const auto& particle : system.particles) {
            if (particle.is_active) {
                render_particle(particle);
            }
        }
        
        // Draw emitter debug visualization
        if (renderer_->is_debug_rendering_enabled()) {
            render_emitter_debug(system.emitter);
        }
        
        renderer_->end_frame();
        window_->swap_buffers();
        window_->poll_events();
        
        auto render_end = std::chrono::high_resolution_clock::now();
        // Note: In real implementation, we'd update the system's render_time_ms here
    }
    
    void render_particle(const Particle& particle) {
        // In a real implementation, this would use instanced rendering or sprite batching
        // For this educational demo, we'll use the existing sprite rendering system
        
        u32 temp_entity = registry_->create_entity();
        
        Transform transform;
        transform.position = {particle.position.x, particle.position.y, 0.0f};
        transform.scale = {particle.scale * 20.0f, particle.scale * 20.0f, 1.0f}; // Base size
        transform.rotation = particle.rotation;
        registry_->add_component(temp_entity, transform);
        
        RenderableSprite sprite;
        sprite.texture = TextureHandle{particle.texture_id, 16, 16};
        sprite.color_modulation = particle.color;
        sprite.z_order = 10.0f; // Particles on top
        registry_->add_component(temp_entity, sprite);
        
        // Render this particle
        renderer_->render_entities(*registry_);
        
        // Clean up temporary entity
        registry_->remove_entity(temp_entity);
    }
    
    void render_emitter_debug(const ParticleEmitter& emitter) {
        // Draw emitter position
        renderer_->draw_debug_circle(emitter.position.x, emitter.position.y, 5.0f, Color::red(), 8);
        
        // Draw emitter shape
        switch (emitter.shape) {
            case ParticleEmitter::Shape::Line:
                renderer_->draw_debug_line(
                    emitter.position.x - emitter.shape_size, emitter.position.y,
                    emitter.position.x + emitter.shape_size, emitter.position.y,
                    Color::yellow(), 2.0f
                );
                break;
                
            case ParticleEmitter::Shape::Circle:
                renderer_->draw_debug_circle(emitter.position.x, emitter.position.y,
                                           emitter.shape_size, Color::yellow(), 16);
                break;
                
            case ParticleEmitter::Shape::Rectangle:
                renderer_->draw_debug_box(
                    emitter.position.x - emitter.shape_size,
                    emitter.position.y - emitter.shape_size,
                    emitter.shape_size * 2.0f,
                    emitter.shape_size * 2.0f,
                    Color::yellow(), 2.0f
                );
                break;
                
            default:
                break;
        }
        
        // Draw direction indicator
        Vec2 dir_end = {
            emitter.position.x + emitter.direction.x * 30.0f,
            emitter.position.y + emitter.direction.y * 30.0f
        };
        renderer_->draw_debug_line(emitter.position.x, emitter.position.y,
                                  dir_end.x, dir_end.y, Color::green(), 3.0f);
    }
    
    //=========================================================================
    // Educational Explanations
    //=========================================================================
    
    void explain_particle_lifecycle() {
        core::Log::info("Education", "=== PARTICLE LIFECYCLE ===");
        core::Log::info("Lifecycle", "1. Emission: Particle created at emitter with initial properties");
        core::Log::info("Lifecycle", "2. Update: Position, velocity, and visual properties updated each frame");
        core::Log::info("Lifecycle", "3. Physics: Forces (gravity, drag) applied to velocity");
        core::Log::info("Lifecycle", "4. Animation: Color, scale, rotation interpolated over lifetime");
        core::Log::info("Lifecycle", "5. Death: Particle deactivated when lifetime expires");
        core::Log::info("Lifecycle", "6. Recycling: Inactive particles reused for new emissions");
    }
    
    void explain_emitter_shapes() {
        core::Log::info("Education", "=== EMITTER SHAPES ===");
        core::Log::info("Point", "Point emitter: All particles spawn from single location");
        core::Log::info("Line", "Line emitter: Particles spawn along line segment (fire, laser)");
        core::Log::info("Circle", "Circle emitter: Particles spawn within circular area (explosion)");
        core::Log::info("Rectangle", "Rectangle emitter: Particles spawn in rectangular region (rain)");
        core::Log::info("Usage", "Shape choice affects visual distribution and effect realism");
    }
    
    void explain_particle_physics() {
        core::Log::info("Education", "=== PARTICLE PHYSICS ===");
        core::Log::info("Forces", "Gravity: Constant downward acceleration (9.8 m/s² realistic)");
        core::Log::info("Forces", "Drag: Air resistance proportional to velocity");
        core::Log::info("Forces", "Custom forces: Wind, magnetic, orbital, turbulence");
        core::Log::info("Integration", "Euler integration: velocity += acceleration * dt");
        core::Log::info("Integration", "Position update: position += velocity * dt");
        core::Log::info("Optimization", "Simple physics suitable for visual effects, not simulation");
    }
    
    void explain_optimization_techniques() {
        core::Log::info("Education", "=== PERFORMANCE OPTIMIZATION ===");
        core::Log::info("Memory", "Object pooling: Reuse particle objects instead of allocating");
        core::Log::info("Memory", "Structure of Arrays (SoA): Better cache performance for updates");
        core::Log::info("Rendering", "Instanced rendering: Single draw call for all particles");
        core::Log::info("Rendering", "Texture atlasing: Pack particle textures for fewer bindings");
        core::Log::info("Culling", "Frustum culling: Don't update/render off-screen particles");
        core::Log::info("LOD", "Level of Detail: Reduce particle count at distance");
        core::Log::info("Threading", "Multi-threading: Update particles on worker threads");
    }
    
    void explain_gpu_particle_architecture() {
        core::Log::info("Education", "=== GPU PARTICLE ARCHITECTURE ===");
        core::Log::info("Compute", "Compute shaders: Massively parallel particle updates");
        core::Log::info("Storage", "Buffer objects: Store particle data in GPU memory");
        core::Log::info("Pipeline", "1. Dispatch compute shader for particle update");
        core::Log::info("Pipeline", "2. Memory barrier to ensure compute completion");
        core::Log::info("Pipeline", "3. Instanced rendering using updated particle data");
        core::Log::info("Benefits", "Eliminates CPU-GPU transfer bottleneck");
        core::Log::info("Benefits", "Enables millions of particles at 60 FPS");
        core::Log::info("Complexity", "Requires advanced graphics programming knowledge");
    }
    
    void analyze_performance_results() {
        core::Log::info("Analysis", "=== PARTICLE PERFORMANCE ANALYSIS ===");
        
        if (performance_results_.empty()) return;
        
        f32 baseline_fps = performance_results_[100].fps;
        
        for (const auto& [count, result] : performance_results_) {
            f32 fps_ratio = result.fps / baseline_fps;
            f32 particles_per_ms = static_cast<f32>(count) / result.update_ms;
            
            core::Log::info("Performance", "{} particles: {:.1f}% baseline FPS, {:.0f} particles/ms",
                           count, fps_ratio * 100.0f, particles_per_ms);
        }
        
        // Identify performance characteristics
        auto worst_result = std::min_element(performance_results_.begin(), performance_results_.end(),
                                           [](const auto& a, const auto& b) { return a.second.fps < b.second.fps; });
        
        if (worst_result != performance_results_.end()) {
            core::Log::info("Analysis", "Performance drops significantly at {} particles ({:.1f} FPS)",
                           worst_result->first, worst_result->second.fps);
        }
    }
    
    void display_educational_summary() {
        std::cout << "\n=== PARTICLE SYSTEMS TUTORIAL SUMMARY ===\n\n";
        
        std::cout << "KEY CONCEPTS LEARNED:\n\n";
        
        std::cout << "1. PARTICLE SYSTEM ARCHITECTURE:\n";
        std::cout << "   - Particles: Individual elements with position, velocity, lifetime\n";
        std::cout << "   - Emitters: Spawn particles with configurable properties\n";
        std::cout << "   - Physics: Forces, gravity, drag affect particle motion\n";
        std::cout << "   - Lifecycle: Birth → Update → Animation → Death → Recycling\n\n";
        
        std::cout << "2. EMITTER SYSTEMS:\n";
        std::cout << "   - Shape-based emission: Point, Line, Circle, Rectangle\n";
        std::cout << "   - Emission rate control: Particles per second timing\n";
        std::cout << "   - Property variation: Speed, lifetime, color randomization\n";
        std::cout << "   - Directional control: Spread angle and base direction\n\n";
        
        std::cout << "3. PARTICLE PHYSICS:\n";
        std::cout << "   - Force integration: Gravity, drag, custom forces\n";
        std::cout << "   - Motion simulation: Velocity and position updates\n";
        std::cout << "   - Realistic behavior: Ballistic trajectories, air resistance\n";
        std::cout << "   - Simple integration: Euler method for visual effects\n\n";
        
        std::cout << "4. VISUAL EFFECTS TECHNIQUES:\n";
        std::cout << "   - Fire: Upward flow with color transition and scale growth\n";
        std::cout << "   - Smoke: Rising motion with wind drift and expansion\n";
        std::cout << "   - Explosions: Radial burst with gravity and debris\n";
        std::cout << "   - Magic: Orbital motion with color cycling\n";
        std::cout << "   - Weather: Rain, snow with environmental forces\n\n";
        
        std::cout << "5. PERFORMANCE OPTIMIZATION:\n";
        if (!performance_results_.empty()) {
            auto best = performance_results_.begin();
            auto worst = std::prev(performance_results_.end());
            f32 performance_ratio = worst->second.fps / best->second.fps;
            std::cout << "   - Particle count impact: " << best->first << " particles (" << best->second.fps 
                     << " FPS) vs " << worst->first << " particles (" << worst->second.fps << " FPS)\n";
            std::cout << "   - Performance scaling: " << (performance_ratio * 100.0f) << "% efficiency at high counts\n";
        }
        std::cout << "   - Object pooling prevents memory allocation overhead\n";
        std::cout << "   - Instanced rendering reduces draw call count\n";
        std::cout << "   - GPU compute shaders enable massive particle counts\n\n";
        
        std::cout << "PRACTICAL APPLICATIONS:\n";
        std::cout << "- Game visual effects: Explosions, fire, smoke, magic spells\n";
        std::cout << "- Environmental effects: Weather, atmospheric particles\n";
        std::cout << "- UI enhancements: Button sparkles, loading animations\n";
        std::cout << "- Scientific visualization: Fluid simulation, data particles\n";
        std::cout << "- Abstract art: Generative visual compositions\n\n";
        
        std::cout << "PARTICLE SYSTEM DESIGN WORKFLOW:\n";
        std::cout << "1. Define effect requirements: Visual goal, performance target\n";
        std::cout << "2. Choose emitter configuration: Shape, rate, particle properties\n";
        std::cout << "3. Implement physics simulation: Forces, integration method\n";
        std::cout << "4. Create visual animation: Color, scale, rotation over time\n";
        std::cout << "5. Optimize rendering: Batching, instancing, culling\n";
        std::cout << "6. Profile and tune: Adjust parameters for performance/quality balance\n\n";
        
        std::cout << "ADVANCED TECHNIQUES:\n";
        std::cout << "- GPU compute shaders for massive particle simulation\n";
        std::cout << "- Signed distance fields for complex collision detection\n";
        std::cout << "- Fluid simulation integration for realistic liquid effects\n";
        std::cout << "- Procedural texture generation for variety without memory cost\n";
        std::cout << "- Multi-threaded updates for CPU-based particle systems\n\n";
        
        std::cout << "NEXT TUTORIAL: Multi-Layer Rendering and Depth Management\n\n";
    }
    
    //=========================================================================
    // Utility Functions
    //=========================================================================
    
    f32 random_float(f32 min, f32 max) {
        std::uniform_real_distribution<f32> dist(min, max);
        return dist(random_engine_);
    }
    
    f32 lerp(f32 a, f32 b, f32 t) {
        return a + (b - a) * t;
    }
    
    Color lerp_color(const Color& a, const Color& b, f32 t) {
        return Color{
            static_cast<u8>(lerp(static_cast<f32>(a.r), static_cast<f32>(b.r), t)),
            static_cast<u8>(lerp(static_cast<f32>(a.g), static_cast<f32>(b.g), t)),
            static_cast<u8>(lerp(static_cast<f32>(a.b), static_cast<f32>(b.b), t)),
            static_cast<u8>(lerp(static_cast<f32>(a.a), static_cast<f32>(b.a), t))
        };
    }
    
    void cleanup() {
        if (renderer_) renderer_->shutdown();
        if (window_) window_->shutdown();
    }
    
    //=========================================================================
    // Data Members
    //=========================================================================
    
    struct PerformanceResult {
        f32 fps;
        f32 update_ms;
        f32 render_ms;
    };
    
    // Tutorial resources
    std::unique_ptr<Window> window_;
    std::unique_ptr<Renderer2D> renderer_;
    std::unique_ptr<ecs::Registry> registry_;
    Camera2D camera_;
    
    // Particle systems
    std::unordered_map<std::string, ParticleSystem> particle_systems_;
    
    // Random number generation
    std::mt19937 random_engine_;
    
    // Performance tracking
    std::map<u32, PerformanceResult> performance_results_;
};

//=============================================================================
// Main Function
//=============================================================================

int main() {
    core::Log::info("Main", "Starting Particle Systems and Visual Effects Tutorial");
    
    std::cout << "\n=== WELCOME TO TUTORIAL 6: PARTICLE SYSTEMS AND VISUAL EFFECTS ===\n";
    std::cout << "This tutorial provides comprehensive coverage of particle system design\n";
    std::cout << "and implementation for creating dynamic visual effects.\n\n";
    std::cout << "You will learn:\n";
    std::cout << "- Particle system architecture and component design\n";
    std::cout << "- Emitter systems and particle spawning strategies\n";
    std::cout << "- Physics simulation: forces, gravity, drag, integration\n";
    std::cout << "- Visual effects creation: fire, smoke, explosions, magic\n";
    std::cout << "- Performance optimization for high particle counts\n";
    std::cout << "- GPU-based particle systems and advanced techniques\n\n";
    std::cout << "Watch for detailed physics explanations and performance analysis.\n\n";
    
    ParticleSystemsTutorial tutorial;
    
    if (!tutorial.initialize()) {
        core::Log::error("Main", "Failed to initialize tutorial");
        return -1;
    }
    
    tutorial.run();
    
    core::Log::info("Main", "Particle Systems Tutorial completed successfully!");
    return 0;
}