/**
 * @file examples/rendering_2d_demo.cpp
 * @brief Comprehensive 2D Rendering Demonstration for ECScope - Phase 7: Renderizado 2D
 * 
 * This comprehensive demo showcases the full capabilities of ECScope's 2D rendering system
 * while serving as an educational tool for graphics programming concepts. Features:
 * 
 * Educational Features:
 * - Interactive parameter adjustment with real-time feedback
 * - Performance visualization and bottleneck analysis
 * - Step-by-step execution mode for learning
 * - Comprehensive statistics and performance metrics
 * - Integration with ECScope systems (ECS, physics, memory)
 * 
 * Rendering Demonstrations:
 * - Sprite batching efficiency showcase
 * - Multiple camera systems and viewport management
 * - Advanced material and shader usage
 * - Debug rendering and wireframe visualization
 * - Real-time performance monitoring and analysis
 * 
 * Interactive Controls:
 * - WASD: Camera movement
 * - Mouse Wheel: Zoom in/out
 * - F1: Toggle debug overlay
 * - F2: Toggle performance overlay
 * - F3: Toggle wireframe mode
 * - F4: Step through render commands
 * - F5: Cycle batching strategies
 * - F6: Toggle batch visualization
 * - Space: Pause/resume animation
 * - R: Reset camera and parameters
 * 
 * @author ECScope Educational ECS Framework - Phase 7: Renderizado 2D
 * @date 2024
 */

#include <iostream>
#include <memory>
#include <vector>
#include <random>
#include <chrono>
#include <sstream>
#include <iomanip>

// ECScope Core Systems
#include "../src/core/log.hpp"
#include "../src/core/time.hpp"
#include "../src/ecs/registry.hpp"
#include "../src/ecs/components/transform.hpp"

// Rendering System
#include "../src/renderer/renderer_2d.hpp"
#include "../src/renderer/batch_renderer.hpp"
#include "../src/renderer/window.hpp"

// UI System for interactive controls
#include "../src/ui/overlay.hpp"
#include "../src/ui/panels/panel_stats.hpp"
#include "../src/ui/panels/panel_memory.hpp"
#include "../src/ui/panels/panel_rendering_debug.hpp"

// Physics integration for debug rendering
#ifdef ECSCOPE_HAS_PHYSICS
#include "../src/physics/world.hpp"
#include "../src/physics/components/physics_components.hpp"
#endif

// Platform and OpenGL
#ifdef ECSCOPE_HAS_GRAPHICS
#include <SDL2/SDL.h>
#include <GL/gl.h>
#endif

using namespace ecscope;

//=============================================================================
// Demo Configuration and Parameters
//=============================================================================

/**
 * @brief Demo Configuration Structure
 * 
 * Contains all configurable parameters for the rendering demo,
 * allowing real-time adjustment for educational exploration.
 */
struct DemoConfig {
    // Scene composition
    struct {
        u32 sprite_count{5000};             // Number of sprites to render
        f32 world_width{2000.0f};           // World dimensions
        f32 world_height{1500.0f};
        f32 sprite_size_min{16.0f};         // Sprite size range
        f32 sprite_size_max{64.0f};
        f32 animation_speed{1.0f};          // Animation speed multiplier
        bool enable_animation{true};        // Enable sprite animation
        bool enable_rotation{true};         // Enable sprite rotation
    } scene;
    
    // Rendering settings
    struct {
        renderer::BatchingStrategy batching_strategy{renderer::BatchingStrategy::AdaptiveHybrid};
        renderer::SortingCriteria::Primary sort_mode{renderer::SortingCriteria::Primary::ZOrder};
        bool enable_frustum_culling{true};
        bool enable_batch_visualization{false};
        u32 max_sprites_per_batch{1000};
        bool enable_vsync{true};
        bool enable_multisampling{false};
    } rendering;
    
    // Camera settings
    struct {
        f32 zoom{1.0f};                     // Camera zoom level
        f32 move_speed{500.0f};             // Camera movement speed
        f32 zoom_speed{0.1f};               // Zoom speed
        bool follow_cursor{false};          // Camera follows mouse
        bool auto_orbit{false};             // Auto-orbit camera
        f32 orbit_speed{0.5f};              // Orbit speed
    } camera;
    
    // Performance settings
    struct {
        bool show_performance_overlay{true};
        bool show_debug_overlay{false};
        bool show_memory_stats{true};
        bool collect_detailed_stats{true};
        bool enable_step_mode{false};
        u32 target_fps{60};
        bool vsync_adaptive{false};
    } performance;
    
    // Educational features
    struct {
        bool enable_tooltips{true};
        bool show_explanations{true};
        bool highlight_expensive_ops{false};
        f32 explanation_detail_level{1.0f}; // 0.5 = basic, 1.0 = normal, 2.0 = advanced
        bool auto_optimize{false};           // Auto-apply optimizations
    } educational;
};

//=============================================================================
// Demo State and Management
//=============================================================================

/**
 * @brief Main Demo Application State
 * 
 * Manages all aspects of the rendering demonstration including
 * scene generation, user interaction, and educational features.
 */
class RenderingDemo {
public:
    //-------------------------------------------------------------------------
    // Construction and Initialization
    //-------------------------------------------------------------------------
    
    RenderingDemo() = default;
    ~RenderingDemo() = default;
    
    /**
     * @brief Initialize the rendering demo
     * @return True if initialization successful
     */
    bool initialize() {
        LOG_INFO("Initializing ECScope 2D Rendering Demo...");
        
        // Initialize window and OpenGL context
        if (!initialize_graphics()) {
            LOG_ERROR("Failed to initialize graphics system");
            return false;
        }
        
        // Initialize ECScope systems
        if (!initialize_ecscope()) {
            LOG_ERROR("Failed to initialize ECScope systems");
            return false;
        }
        
        // Create demo scene
        create_demo_scene();
        
        // Setup UI
        setup_user_interface();
        
        LOG_INFO("ECScope 2D Rendering Demo initialized successfully");
        return true;
    }
    
    /**
     * @brief Main demo execution loop
     */
    void run() {
        LOG_INFO("Starting ECScope 2D Rendering Demo");
        
        auto last_time = std::chrono::high_resolution_clock::now();
        bool running = true;
        
        while (running) {
            // Calculate frame timing
            auto current_time = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::microseconds>(current_time - last_time);
            f32 delta_time = duration.count() / 1000000.0f; // Convert to seconds
            last_time = current_time;
            
            // Handle input and events
            running = handle_events(delta_time);
            if (!running) break;
            
            // Update demo systems
            update(delta_time);
            
            // Render frame
            render();
            
            // Update UI
            render_ui();
            
            // Present frame
            present_frame();
            
            // Educational frame analysis
            analyze_frame_performance();
            
            frame_count_++;
        }
        
        LOG_INFO("ECScope 2D Rendering Demo shutting down");
    }

private:
    //-------------------------------------------------------------------------
    // Core Systems
    //-------------------------------------------------------------------------
    
    std::unique_ptr<renderer::Window> window_;
    std::unique_ptr<ecs::Registry> registry_;
    std::unique_ptr<renderer::Renderer2D> renderer_;
    std::unique_ptr<ui::Overlay> ui_overlay_;
    
    // Demo state
    DemoConfig config_;
    bool initialized_{false};
    u32 frame_count_{0};
    f32 demo_time_{0.0f};
    
    // Scene entities
    std::vector<ecs::EntityID> sprite_entities_;
    std::vector<ecs::EntityID> camera_entities_;
    ecs::EntityID main_camera_{ecs::INVALID_ENTITY_ID};
    
    // Performance tracking
    struct PerformanceMetrics {
        f32 frame_time_ms{0.0f};
        f32 update_time_ms{0.0f};
        f32 render_time_ms{0.0f};
        f32 ui_time_ms{0.0f};
        u32 draw_calls{0};
        u32 vertices_rendered{0};
        f32 batching_efficiency{0.0f};
        usize memory_usage{0};
        
        // Educational insights
        std::string bottleneck_analysis;
        std::vector<std::string> optimization_suggestions;
        char performance_grade{'A'};
    } current_metrics_;
    
    // Input state
    struct InputState {
        bool keys[256]{false};
        struct { f32 x, y; } mouse_pos{0.0f, 0.0f};
        struct { f32 x, y; } mouse_delta{0.0f, 0.0f};
        bool mouse_buttons[5]{false};
        f32 mouse_wheel_delta{0.0f};
    } input_;
    
    //-------------------------------------------------------------------------
    // Initialization Methods
    //-------------------------------------------------------------------------
    
    bool initialize_graphics() {
#ifdef ECSCOPE_HAS_GRAPHICS
        // Initialize SDL
        if (SDL_Init(SDL_INIT_VIDEO) < 0) {
            LOG_ERROR("Failed to initialize SDL: {}", SDL_GetError());
            return false;
        }
        
        // Create window
        window_ = std::make_unique<renderer::Window>();
        if (!window_->create(1920, 1080, "ECScope 2D Rendering Demo - Educational Graphics Programming")) {
            LOG_ERROR("Failed to create window");
            return false;
        }
        
        LOG_INFO("Graphics system initialized - Window: 1920x1080");
        return true;
#else
        LOG_ERROR("Graphics support not compiled - rebuild with ECSCOPE_ENABLE_GRAPHICS=ON");
        return false;
#endif
    }
    
    bool initialize_ecscope() {
        // Create ECS registry
        registry_ = std::make_unique<ecs::Registry>();
        LOG_INFO("ECS Registry initialized");
        
        // Create 2D renderer with educational configuration
        auto renderer_config = renderer::Renderer2DConfig::educational_mode();
        renderer_config.rendering.max_sprites_per_batch = config_.rendering.max_sprites_per_batch;
        renderer_config.debug.enable_debug_rendering = true;
        renderer_config.debug.show_performance_overlay = true;
        renderer_config.debug.collect_gpu_timings = true;
        
        renderer_ = std::make_unique<renderer::Renderer2D>(renderer_config);
        if (auto result = renderer_->initialize(); !result.is_ok()) {
            LOG_ERROR("Failed to initialize 2D renderer: {}", result.error());
            return false;
        }
        
        LOG_INFO("2D Renderer initialized with educational configuration");
        return true;
    }
    
    void create_demo_scene() {
        LOG_INFO("Creating demo scene with {} sprites", config_.scene.sprite_count);
        
        // Create main camera entity
        main_camera_ = registry_->create_entity();
        
        // Add camera component
        auto& camera = registry_->add_component<renderer::components::Camera2D>(main_camera_);
        camera.position = {0.0f, 0.0f};
        camera.zoom = config_.camera.zoom;
        camera.viewport_width = 1920.0f;
        camera.viewport_height = 1080.0f;
        
        // Add transform for camera movement
        auto& camera_transform = registry_->add_component<ecs::components::Transform>(main_camera_);
        camera_transform.position = {0.0f, 0.0f, 0.0f};
        
        camera_entities_.push_back(main_camera_);
        
        // Create sprite entities
        sprite_entities_.reserve(config_.scene.sprite_count);
        
        // Random generation for diverse scene
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<f32> pos_x(-config_.scene.world_width/2, config_.scene.world_width/2);
        std::uniform_real_distribution<f32> pos_y(-config_.scene.world_height/2, config_.scene.world_height/2);
        std::uniform_real_distribution<f32> size(config_.scene.sprite_size_min, config_.scene.sprite_size_max);
        std::uniform_real_distribution<f32> rotation(0.0f, 6.28318f); // 2π radians
        std::uniform_real_distribution<f32> color_component(0.3f, 1.0f);
        std::uniform_real_distribution<f32> z_order(-10.0f, 10.0f);
        std::uniform_int_distribution<u32> texture_id(0, 7); // Assume 8 demo textures
        
        for (u32 i = 0; i < config_.scene.sprite_count; ++i) {
            auto entity = registry_->create_entity();
            
            // Transform component
            auto& transform = registry_->add_component<ecs::components::Transform>(entity);
            transform.position = {pos_x(gen), pos_y(gen), z_order(gen)};
            transform.rotation = {0.0f, 0.0f, rotation(gen)};
            f32 sprite_size = size(gen);
            transform.scale = {sprite_size, sprite_size, 1.0f};
            
            // Renderable sprite component
            auto& sprite = registry_->add_component<renderer::components::RenderableSprite>(entity);
            sprite.texture_id = static_cast<renderer::TextureID>(texture_id(gen));
            sprite.color = renderer::components::Color{
                color_component(gen), color_component(gen), color_component(gen), 1.0f
            };
            sprite.z_order = transform.position.z;
            
            // Add some variety in blending modes for educational purposes
            if (i % 10 == 0) {
                sprite.blend_mode = renderer::components::RenderableSprite::BlendMode::Additive;
            } else if (i % 15 == 0) {
                sprite.blend_mode = renderer::components::RenderableSprite::BlendMode::Multiply;
            }
            
            // Animation component for movement
            if (config_.scene.enable_animation) {
                struct AnimationData {
                    f32 speed{std::uniform_real_distribution<f32>(10.0f, 100.0f)(gen)};
                    f32 direction{rotation(gen)};
                    f32 rotation_speed{std::uniform_real_distribution<f32>(-2.0f, 2.0f)(gen)};
                    f32 original_x{transform.position.x};
                    f32 original_y{transform.position.y};
                    f32 orbit_radius{std::uniform_real_distribution<f32>(50.0f, 200.0f)(gen)};
                };
                
                // Store animation data in a custom component (simplified for demo)
                // In a real system, this would be a proper component
            }
            
            sprite_entities_.push_back(entity);
        }
        
        LOG_INFO("Created {} sprite entities with diverse properties", sprite_entities_.size());
        
        // Create some demo textures (placeholder IDs)
        create_demo_textures();
    }
    
    void create_demo_textures() {
        // In a real implementation, this would load actual texture assets
        // For the demo, we'll register placeholder texture IDs
        auto& texture_manager = renderer_->get_texture_manager();
        
        // Educational note: Different texture types for batching analysis
        const char* texture_names[] = {
            "demo_sprite_01.png",  // Common sprite
            "demo_sprite_02.png",  // Common sprite
            "demo_particle.png",   // Small particle texture
            "demo_large.png",      // Large sprite texture
            "demo_ui_element.png", // UI element
            "demo_background.png", // Background tile
            "demo_effect.png",     // Effect texture
            "demo_debug.png"       // Debug visualization
        };
        
        for (u32 i = 0; i < 8; ++i) {
            // In real implementation: texture_manager.load_texture(texture_names[i]);
            LOG_DEBUG("Registered demo texture {}: {}", i, texture_names[i]);
        }
    }
    
    void setup_user_interface() {
        // Initialize UI overlay
        ui_overlay_ = std::make_unique<ui::Overlay>();
        ui_overlay_->initialize();
        
        // Add educational panels
        setup_educational_panels();
        
        LOG_INFO("User interface initialized with educational panels");
    }
    
    void setup_educational_panels() {
        // Performance analysis panel
        auto performance_panel = std::make_unique<ui::panels::PanelStats>();
        ui_overlay_->add_panel("Performance Analysis", std::move(performance_panel));
        
        // Memory usage panel  
        auto memory_panel = std::make_unique<ui::panels::PanelMemory>();
        ui_overlay_->add_panel("Memory Usage", std::move(memory_panel));
        
        // Rendering debug panel
        auto rendering_panel = std::make_unique<ui::panels::PanelRenderingDebug>();
        ui_overlay_->add_panel("Rendering Debug", std::move(rendering_panel));
        
        LOG_INFO("Educational UI panels configured");
    }
    
    //-------------------------------------------------------------------------
    // Update and Animation Systems
    //-------------------------------------------------------------------------
    
    void update(f32 delta_time) {
        auto start_time = std::chrono::high_resolution_clock::now();
        
        demo_time_ += delta_time;
        
        // Update camera
        update_camera(delta_time);
        
        // Update sprite animations
        if (config_.scene.enable_animation && !config_.performance.enable_step_mode) {
            update_sprite_animations(delta_time);
        }
        
        // Update renderer systems
        renderer_->update(delta_time);
        
        // Educational: Calculate update timing
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
        current_metrics_.update_time_ms = duration.count() / 1000.0f;
    }
    
    void update_camera(f32 delta_time) {
        if (!registry_->has_component<ecs::components::Transform>(main_camera_)) return;
        
        auto& transform = registry_->get_component<ecs::components::Transform>(main_camera_);
        auto& camera = registry_->get_component<renderer::components::Camera2D>(main_camera_);
        
        // Handle camera movement (WASD)
        f32 move_speed = config_.camera.move_speed * delta_time;
        if (input_.keys['w'] || input_.keys['W']) transform.position.y += move_speed;
        if (input_.keys['s'] || input_.keys['S']) transform.position.y -= move_speed;
        if (input_.keys['a'] || input_.keys['A']) transform.position.x -= move_speed;
        if (input_.keys['d'] || input_.keys['D']) transform.position.x += move_speed;
        
        // Handle zoom (mouse wheel)
        if (std::abs(input_.mouse_wheel_delta) > 0.01f) {
            camera.zoom += input_.mouse_wheel_delta * config_.camera.zoom_speed;
            camera.zoom = std::clamp(camera.zoom, 0.1f, 10.0f);
            input_.mouse_wheel_delta = 0.0f; // Reset wheel delta
        }
        
        // Auto-orbit mode
        if (config_.camera.auto_orbit) {
            f32 orbit_angle = demo_time_ * config_.camera.orbit_speed;
            transform.position.x = std::cos(orbit_angle) * 300.0f;
            transform.position.y = std::sin(orbit_angle) * 300.0f;
        }
        
        // Follow cursor mode
        if (config_.camera.follow_cursor) {
            // Convert mouse position to world coordinates
            // This is a simplified version - real implementation would use proper projection
            f32 world_x = (input_.mouse_pos.x - 960.0f) * 2.0f;
            f32 world_y = (540.0f - input_.mouse_pos.y) * 2.0f;
            transform.position.x = world_x;
            transform.position.y = world_y;
        }
        
        // Update camera position
        camera.position = {transform.position.x, transform.position.y};
    }
    
    void update_sprite_animations(f32 delta_time) {
        // Simplified animation system for demo purposes
        // In practice, this would be a proper ECS system
        
        f32 time_factor = config_.scene.animation_speed * delta_time;
        
        for (auto entity : sprite_entities_) {
            if (!registry_->has_component<ecs::components::Transform>(entity)) continue;
            
            auto& transform = registry_->get_component<ecs::components::Transform>(entity);
            
            // Simple orbital movement around original position
            u32 entity_id = static_cast<u32>(entity);
            f32 phase = (entity_id % 1000) / 1000.0f * 6.28318f; // Unique phase per sprite
            f32 animation_time = demo_time_ + phase;
            
            // Orbital movement
            f32 orbit_radius = 20.0f + (entity_id % 50);
            f32 orbit_speed = 0.5f + (entity_id % 100) / 200.0f;
            
            transform.position.x += std::cos(animation_time * orbit_speed) * orbit_radius * time_factor * 0.01f;
            transform.position.y += std::sin(animation_time * orbit_speed) * orbit_radius * time_factor * 0.01f;
            
            // Rotation animation
            if (config_.scene.enable_rotation) {
                transform.rotation.z += ((entity_id % 4) - 2) * time_factor;
            }
            
            // Update sprite Z-order for depth sorting demonstration
            if (registry_->has_component<renderer::components::RenderableSprite>(entity)) {
                auto& sprite = registry_->get_component<renderer::components::RenderableSprite>(entity);
                sprite.z_order = transform.position.z;
            }
        }
    }
    
    //-------------------------------------------------------------------------
    // Rendering System
    //-------------------------------------------------------------------------
    
    void render() {
        auto start_time = std::chrono::high_resolution_clock::now();
        
        // Begin frame
        renderer_->begin_frame();
        
        // Set active camera
        if (registry_->has_component<renderer::components::Camera2D>(main_camera_)) {
            const auto& camera = registry_->get_component<renderer::components::Camera2D>(main_camera_);
            renderer_->set_active_camera(camera);
        }
        
        // Render all entities with sprite components
        renderer_->render_entities(*registry_);
        
        // Debug rendering
        if (config_.performance.show_debug_overlay) {
            render_debug_information();
        }
        
        // Educational: Render batch visualization
        if (config_.rendering.enable_batch_visualization) {
            render_batch_visualization();
        }
        
        // End frame and execute render commands
        renderer_->end_frame();
        
        // Calculate render timing
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
        current_metrics_.render_time_ms = duration.count() / 1000.0f;
        
        // Update rendering metrics
        const auto& stats = renderer_->get_statistics();
        current_metrics_.draw_calls = stats.gpu_stats.draw_calls;
        current_metrics_.vertices_rendered = stats.gpu_stats.vertices_rendered;
        current_metrics_.batching_efficiency = stats.gpu_stats.batching_efficiency;
    }
    
    void render_debug_information() {
        // Render sprite bounding boxes
        for (auto entity : sprite_entities_) {
            if (!registry_->has_component<ecs::components::Transform>(entity) ||
                !registry_->has_component<renderer::components::RenderableSprite>(entity)) continue;
            
            const auto& transform = registry_->get_component<ecs::components::Transform>(entity);
            
            // Simple bounding box (in a real system, this would be calculated properly)
            f32 half_width = transform.scale.x * 0.5f;
            f32 half_height = transform.scale.y * 0.5f;
            
            // Draw bounding box
            renderer_->draw_debug_box(
                transform.position.x - half_width,
                transform.position.y - half_height,
                transform.scale.x,
                transform.scale.y,
                renderer::components::Color::cyan(),
                1.0f
            );
        }
        
        // Render camera frustum
        if (registry_->has_component<renderer::components::Camera2D>(main_camera_)) {
            const auto& camera = registry_->get_component<renderer::components::Camera2D>(main_camera_);
            const auto& camera_transform = registry_->get_component<ecs::components::Transform>(main_camera_);
            
            f32 width = camera.viewport_width / camera.zoom;
            f32 height = camera.viewport_height / camera.zoom;
            
            renderer_->draw_debug_box(
                camera_transform.position.x - width * 0.5f,
                camera_transform.position.y - height * 0.5f,
                width, height,
                renderer::components::Color::yellow(),
                2.0f
            );
        }
    }
    
    void render_batch_visualization() {
        // This would render color-coded overlays showing which sprites are batched together
        // Implementation would depend on batch renderer debug information
        
        const auto& batch_renderer = renderer_->get_batch_renderer();
        const auto& batches = batch_renderer.get_batches();
        
        // Color-code different batches
        std::vector<renderer::components::Color> batch_colors = {
            {1.0f, 0.0f, 0.0f, 0.3f}, // Red
            {0.0f, 1.0f, 0.0f, 0.3f}, // Green
            {0.0f, 0.0f, 1.0f, 0.3f}, // Blue
            {1.0f, 1.0f, 0.0f, 0.3f}, // Yellow
            {1.0f, 0.0f, 1.0f, 0.3f}, // Magenta
            {0.0f, 1.0f, 1.0f, 0.3f}, // Cyan
        };
        
        for (usize i = 0; i < batches.size() && i < batch_colors.size(); ++i) {
            // Render batch boundary or overlay
            // This is a simplified visualization
        }
    }
    
    void render_ui() {
        auto start_time = std::chrono::high_resolution_clock::now();
        
        // Render performance overlay
        if (config_.performance.show_performance_overlay) {
            render_performance_overlay();
        }
        
        // Render educational explanations
        if (config_.educational.show_explanations) {
            render_educational_overlay();
        }
        
        // Render control hints
        render_control_hints();
        
        // Main UI overlay
        if (ui_overlay_) {
            ui_overlay_->render();
        }
        
        // Calculate UI timing
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
        current_metrics_.ui_time_ms = duration.count() / 1000.0f;
    }
    
    void render_performance_overlay() {
        // This would render an ImGui-style performance overlay
        // Showing FPS, frame time, render statistics, etc.
        
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(2);
        oss << "ECScope 2D Rendering Demo - Performance Analysis\n";
        oss << "================================================\n";
        oss << "Frame: " << frame_count_ << "\n";
        oss << "FPS: " << (current_metrics_.frame_time_ms > 0 ? 1000.0f / current_metrics_.frame_time_ms : 0) << "\n";
        oss << "Frame Time: " << current_metrics_.frame_time_ms << " ms\n";
        oss << "  Update: " << current_metrics_.update_time_ms << " ms\n";
        oss << "  Render: " << current_metrics_.render_time_ms << " ms\n";
        oss << "  UI: " << current_metrics_.ui_time_ms << " ms\n";
        oss << "\nRendering Statistics:\n";
        oss << "  Draw Calls: " << current_metrics_.draw_calls << "\n";
        oss << "  Vertices: " << current_metrics_.vertices_rendered << "\n";
        oss << "  Batching Efficiency: " << (current_metrics_.batching_efficiency * 100.0f) << "%\n";
        oss << "  Memory Usage: " << (current_metrics_.memory_usage / 1024) << " KB\n";
        oss << "\nPerformance Grade: " << current_metrics_.performance_grade << "\n";
        
        if (!current_metrics_.bottleneck_analysis.empty()) {
            oss << "Bottleneck: " << current_metrics_.bottleneck_analysis << "\n";
        }
        
        // In a real implementation, this would be rendered with ImGui or similar
        std::cout << "\r" << oss.str() << std::flush;
    }
    
    void render_educational_overlay() {
        // Educational explanations and tips
        if (config_.educational.enable_tooltips) {
            // Render context-sensitive educational information
            // This could explain what's happening in the current frame,
            // why certain optimizations are being applied, etc.
        }
    }
    
    void render_control_hints() {
        // Display control instructions
        static const char* controls = R"(
Controls:
  WASD: Move Camera    Mouse Wheel: Zoom    Space: Pause/Resume
  F1: Debug Overlay    F2: Performance      F3: Wireframe Mode
  F4: Step Mode        F5: Batching Mode    F6: Batch Visualization
  R: Reset Camera      ESC: Exit Demo
)";
        
        // This would be rendered as overlay text in a real implementation
    }
    
    void present_frame() {
        // Present the completed frame
        if (window_) {
            window_->swap_buffers();
        }
        
        // Handle VSync
        if (config_.rendering.enable_vsync) {
            // VSync handling would be implemented here
        }
    }
    
    //-------------------------------------------------------------------------
    // Event Handling and Input
    //-------------------------------------------------------------------------
    
    bool handle_events(f32 delta_time) {
#ifdef ECSCOPE_HAS_GRAPHICS
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_QUIT:
                    return false;
                    
                case SDL_KEYDOWN:
                    return handle_key_down(event.key.keysym.sym);
                    
                case SDL_KEYUP:
                    handle_key_up(event.key.keysym.sym);
                    break;
                    
                case SDL_MOUSEMOTION:
                    handle_mouse_motion(event.motion.x, event.motion.y, 
                                      event.motion.xrel, event.motion.yrel);
                    break;
                    
                case SDL_MOUSEWHEEL:
                    handle_mouse_wheel(event.wheel.y);
                    break;
                    
                case SDL_MOUSEBUTTONDOWN:
                case SDL_MOUSEBUTTONUP:
                    handle_mouse_button(event.button.button, event.button.state == SDL_PRESSED);
                    break;
                    
                case SDL_WINDOWEVENT:
                    if (event.window.event == SDL_WINDOWEVENT_RESIZED) {
                        handle_window_resize(event.window.data1, event.window.data2);
                    }
                    break;
            }
        }
        return true;
#else
        return false;
#endif
    }
    
    bool handle_key_down(u32 key) {
        // Update input state
        if (key < 256) {
            input_.keys[key] = true;
        }
        
        // Handle function keys and special commands
        switch (key) {
            case SDLK_ESCAPE:
                return false; // Exit demo
                
            case SDLK_F1:
                config_.performance.show_debug_overlay = !config_.performance.show_debug_overlay;
                LOG_INFO("Debug overlay: {}", config_.performance.show_debug_overlay ? "ON" : "OFF");
                break;
                
            case SDLK_F2:
                config_.performance.show_performance_overlay = !config_.performance.show_performance_overlay;
                LOG_INFO("Performance overlay: {}", config_.performance.show_performance_overlay ? "ON" : "OFF");
                break;
                
            case SDLK_F3:
                // Toggle wireframe mode
                toggle_wireframe_mode();
                break;
                
            case SDLK_F4:
                config_.performance.enable_step_mode = !config_.performance.enable_step_mode;
                renderer_->set_step_through_mode(config_.performance.enable_step_mode);
                LOG_INFO("Step-through mode: {}", config_.performance.enable_step_mode ? "ON" : "OFF");
                break;
                
            case SDLK_F5:
                cycle_batching_strategy();
                break;
                
            case SDLK_F6:
                config_.rendering.enable_batch_visualization = !config_.rendering.enable_batch_visualization;
                LOG_INFO("Batch visualization: {}", config_.rendering.enable_batch_visualization ? "ON" : "OFF");
                break;
                
            case SDLK_SPACE:
                config_.scene.enable_animation = !config_.scene.enable_animation;
                LOG_INFO("Animation: {}", config_.scene.enable_animation ? "ON" : "OFF");
                break;
                
            case SDLK_r:
                reset_camera_and_parameters();
                break;
                
            case SDLK_n:
                // Step to next render command (if in step mode)
                if (config_.performance.enable_step_mode) {
                    renderer_->step_to_next_command();
                }
                break;
        }
        
        return true;
    }
    
    void handle_key_up(u32 key) {
        if (key < 256) {
            input_.keys[key] = false;
        }
    }
    
    void handle_mouse_motion(i32 x, i32 y, i32 rel_x, i32 rel_y) {
        input_.mouse_pos.x = static_cast<f32>(x);
        input_.mouse_pos.y = static_cast<f32>(y);
        input_.mouse_delta.x = static_cast<f32>(rel_x);
        input_.mouse_delta.y = static_cast<f32>(rel_y);
    }
    
    void handle_mouse_wheel(i32 wheel_y) {
        input_.mouse_wheel_delta = static_cast<f32>(wheel_y);
    }
    
    void handle_mouse_button(u8 button, bool pressed) {
        if (button < 5) {
            input_.mouse_buttons[button] = pressed;
        }
    }
    
    void handle_window_resize(i32 width, i32 height) {
        if (renderer_) {
            renderer_->handle_window_resize(static_cast<u32>(width), static_cast<u32>(height));
        }
        
        // Update camera viewport
        if (registry_->has_component<renderer::components::Camera2D>(main_camera_)) {
            auto& camera = registry_->get_component<renderer::components::Camera2D>(main_camera_);
            camera.viewport_width = static_cast<f32>(width);
            camera.viewport_height = static_cast<f32>(height);
        }
        
        LOG_INFO("Window resized to {}x{}", width, height);
    }
    
    //-------------------------------------------------------------------------
    // Interactive Controls and Configuration
    //-------------------------------------------------------------------------
    
    void toggle_wireframe_mode() {
        auto config = renderer_->get_config();
        config.debug.enable_wireframe_mode = !config.debug.enable_wireframe_mode;
        renderer_->update_config(config);
        LOG_INFO("Wireframe mode: {}", config.debug.enable_wireframe_mode ? "ON" : "OFF");
    }
    
    void cycle_batching_strategy() {
        using Strategy = renderer::BatchingStrategy;
        
        // Cycle through available strategies
        static const std::array<Strategy, 5> strategies = {
            Strategy::TextureFirst,
            Strategy::MaterialFirst,
            Strategy::ZOrderPreserving,
            Strategy::SpatialLocality,
            Strategy::AdaptiveHybrid
        };
        
        static const std::array<const char*, 5> strategy_names = {
            "Texture First",
            "Material First", 
            "Z-Order Preserving",
            "Spatial Locality",
            "Adaptive Hybrid"
        };
        
        // Find current strategy and advance to next
        usize current_index = 0;
        for (usize i = 0; i < strategies.size(); ++i) {
            if (strategies[i] == config_.rendering.batching_strategy) {
                current_index = i;
                break;
            }
        }
        
        current_index = (current_index + 1) % strategies.size();
        config_.rendering.batching_strategy = strategies[current_index];
        
        // Update renderer configuration
        auto renderer_config = renderer_->get_config();
        // Apply new batching strategy to renderer config
        renderer_->update_config(renderer_config);
        
        LOG_INFO("Batching strategy changed to: {}", strategy_names[current_index]);
        
        // Educational: Explain the strategy
        print_batching_strategy_explanation(strategies[current_index]);
    }
    
    void print_batching_strategy_explanation(renderer::BatchingStrategy strategy) {
        using Strategy = renderer::BatchingStrategy;
        
        const char* explanation = "";
        
        switch (strategy) {
            case Strategy::TextureFirst:
                explanation = "Groups sprites by texture first - minimizes texture binding changes";
                break;
            case Strategy::MaterialFirst:
                explanation = "Groups sprites by material properties - reduces expensive state changes";
                break;
            case Strategy::ZOrderPreserving:
                explanation = "Maintains depth order for correct transparency - balances performance and correctness";
                break;
            case Strategy::SpatialLocality:
                explanation = "Groups nearby sprites together - optimizes vertex cache and culling";
                break;
            case Strategy::AdaptiveHybrid:
                explanation = "Dynamically chooses optimal strategy based on scene characteristics";
                break;
        }
        
        LOG_INFO("Strategy explanation: {}", explanation);
    }
    
    void reset_camera_and_parameters() {
        // Reset camera position and zoom
        if (registry_->has_component<ecs::components::Transform>(main_camera_)) {
            auto& transform = registry_->get_component<ecs::components::Transform>(main_camera_);
            transform.position = {0.0f, 0.0f, 0.0f};
        }
        
        if (registry_->has_component<renderer::components::Camera2D>(main_camera_)) {
            auto& camera = registry_->get_component<renderer::components::Camera2D>(main_camera_);
            camera.zoom = 1.0f;
            camera.position = {0.0f, 0.0f};
        }
        
        // Reset configuration to defaults
        config_ = DemoConfig{};
        
        LOG_INFO("Camera and parameters reset to defaults");
    }
    
    //-------------------------------------------------------------------------
    // Performance Analysis and Educational Features
    //-------------------------------------------------------------------------
    
    void analyze_frame_performance() {
        // Collect comprehensive performance data
        current_metrics_.frame_time_ms = current_metrics_.update_time_ms + 
                                       current_metrics_.render_time_ms + 
                                       current_metrics_.ui_time_ms;
        
        // Get memory usage
        if (renderer_) {
            auto memory_usage = renderer_->get_memory_usage();
            current_metrics_.memory_usage = memory_usage.total;
        }
        
        // Performance analysis
        analyze_performance_bottlenecks();
        
        // Generate optimization suggestions
        generate_optimization_suggestions();
        
        // Calculate performance grade
        calculate_performance_grade();
    }
    
    void analyze_performance_bottlenecks() {
        // Simple heuristic-based bottleneck analysis
        f32 total_time = current_metrics_.frame_time_ms;
        
        if (total_time < 16.67f) { // 60 FPS
            current_metrics_.bottleneck_analysis = "No significant bottlenecks";
        } else if (current_metrics_.render_time_ms > total_time * 0.7f) {
            if (current_metrics_.draw_calls > 1000) {
                current_metrics_.bottleneck_analysis = "GPU bound - Too many draw calls";
            } else if (current_metrics_.batching_efficiency < 0.5f) {
                current_metrics_.bottleneck_analysis = "GPU bound - Poor batching efficiency";
            } else {
                current_metrics_.bottleneck_analysis = "GPU bound - Fill rate or complexity";
            }
        } else if (current_metrics_.update_time_ms > total_time * 0.5f) {
            current_metrics_.bottleneck_analysis = "CPU bound - Update systems";
        } else {
            current_metrics_.bottleneck_analysis = "Balanced - Minor optimizations possible";
        }
    }
    
    void generate_optimization_suggestions() {
        current_metrics_.optimization_suggestions.clear();
        
        // Analyze rendering metrics
        if (current_metrics_.draw_calls > 500) {
            current_metrics_.optimization_suggestions.push_back(
                "Consider increasing max sprites per batch to reduce draw calls"
            );
        }
        
        if (current_metrics_.batching_efficiency < 0.6f) {
            current_metrics_.optimization_suggestions.push_back(
                "Improve batching by sorting sprites by texture or material"
            );
        }
        
        if (current_metrics_.memory_usage > 100 * 1024 * 1024) { // 100MB
            current_metrics_.optimization_suggestions.push_back(
                "High memory usage - consider texture atlasing or LOD systems"
            );
        }
        
        if (current_metrics_.frame_time_ms > 33.33f) { // Below 30 FPS
            current_metrics_.optimization_suggestions.push_back(
                "Low frame rate - enable frustum culling or reduce sprite count"
            );
        }
    }
    
    void calculate_performance_grade() {
        f32 fps = current_metrics_.frame_time_ms > 0 ? 1000.0f / current_metrics_.frame_time_ms : 0;
        
        if (fps >= 58.0f && current_metrics_.batching_efficiency > 0.8f) {
            current_metrics_.performance_grade = 'A';
        } else if (fps >= 45.0f && current_metrics_.batching_efficiency > 0.6f) {
            current_metrics_.performance_grade = 'B';
        } else if (fps >= 30.0f && current_metrics_.batching_efficiency > 0.4f) {
            current_metrics_.performance_grade = 'C';
        } else if (fps >= 20.0f) {
            current_metrics_.performance_grade = 'D';
        } else {
            current_metrics_.performance_grade = 'F';
        }
    }
};

//=============================================================================
// Main Entry Point
//=============================================================================

/**
 * @brief Main entry point for the ECScope 2D Rendering Demo
 * 
 * Initializes and runs the comprehensive rendering demonstration,
 * providing an educational exploration of modern 2D graphics programming.
 */
int main(int argc, char* argv[]) {
    // Initialize logging system
    core::Log::initialize(core::LogLevel::INFO);
    LOG_INFO("Starting ECScope 2D Rendering Educational Demo");
    
    // Parse command line arguments for demo configuration
    bool educational_mode = true;
    bool performance_mode = false;
    
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--performance") {
            performance_mode = true;
            educational_mode = false;
        } else if (arg == "--educational") {
            educational_mode = true;
            performance_mode = false;
        } else if (arg == "--help" || arg == "-h") {
            std::cout << "ECScope 2D Rendering Demo\n";
            std::cout << "Usage: " << argv[0] << " [options]\n";
            std::cout << "Options:\n";
            std::cout << "  --educational    Enable comprehensive educational features (default)\n";
            std::cout << "  --performance    Focus on performance demonstration\n";
            std::cout << "  --help, -h       Show this help message\n";
            return 0;
        }
    }
    
    try {
        // Create and initialize demo
        RenderingDemo demo;
        
        if (!demo.initialize()) {
            LOG_ERROR("Failed to initialize rendering demo");
            return -1;
        }
        
        LOG_INFO("Demo mode: {}", educational_mode ? "Educational" : "Performance");
        
        // Run the demo
        demo.run();
        
    } catch (const std::exception& e) {
        LOG_ERROR("Demo crashed with exception: {}", e.what());
        return -1;
    }
    
    LOG_INFO("ECScope 2D Rendering Demo completed successfully");
    return 0;
}