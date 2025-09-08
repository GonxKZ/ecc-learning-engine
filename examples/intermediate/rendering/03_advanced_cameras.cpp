/**
 * @file examples/rendering_tutorials/03_advanced_cameras.cpp
 * @brief Tutorial 3: Advanced Camera Systems - ECScope Educational Graphics Programming
 * 
 * This tutorial explores advanced camera concepts and multi-camera rendering techniques.
 * Students will learn:
 * - Multiple camera management and viewport systems
 * - Camera projection and transformation matrices
 * - Screen-space to world-space coordinate conversion
 * - Camera following and smooth movement systems
 * - Viewport splitting and picture-in-picture rendering
 * - Camera culling and performance optimization
 * 
 * Educational Objectives:
 * - Master 2D camera mathematics and transformations
 * - Understand viewport and projection concepts
 * - Implement smooth camera movement and following
 * - Learn multi-camera rendering techniques
 * - Experience coordinate system transformations
 * 
 * Prerequisites: Completion of Tutorials 1-2, understanding of 2D transformations
 * 
 * @author ECScope Educational ECS Framework - Phase 7: Renderizado 2D
 * @date 2024
 */

#include <iostream>
#include <memory>
#include <vector>
#include <cmath>
#include <algorithm>

// ECScope Core Systems
#include "../../src/core/log.hpp"
#include "../../src/ecs/registry.hpp"
#include "../../src/ecs/components/transform.hpp"

// Rendering System
#include "../../src/renderer/renderer_2d.hpp"
#include "../../src/renderer/window.hpp"

// Platform Integration
#ifdef ECSCOPE_HAS_GRAPHICS
#include <SDL2/SDL.h>
#endif

using namespace ecscope;

/**
 * @brief Tutorial 3: Advanced Camera Systems Demonstration
 * 
 * This tutorial showcases multiple camera techniques including smooth following,
 * multi-viewport rendering, and coordinate system transformations.
 */
class AdvancedCamerasTutorial {
public:
    /**
     * @brief Camera movement modes for educational demonstration
     */
    enum class CameraMode {
        Manual,           // WASD manual control
        FollowTarget,     // Smooth following of target entity
        Orbital,          // Orbital movement around target
        Patrol,           // Patrol between waypoints
        Shake,            // Camera shake effects
        Split,            // Split-screen multiple cameras
        PictureInPicture  // Picture-in-picture rendering
    };
    
    /**
     * @brief Initialize the advanced camera tutorial
     */
    bool initialize() {
        std::cout << "\n=== ECScope Tutorial 3: Advanced Camera Systems ===\n";
        std::cout << "This tutorial explores sophisticated camera techniques for 2D games.\n\n";
        
        // Initialize core systems
        if (!initialize_graphics() || !initialize_ecs()) {
            return false;
        }
        
        // Create scene and cameras
        create_demo_world();
        create_cameras();
        
        // Initialize tutorial state
        reset_tutorial_state();
        
        std::cout << "\n🎉 Tutorial initialization complete!\n";
        print_controls();
        
        return true;
    }
    
    /**
     * @brief Main tutorial execution loop
     */
    void run() {
        std::cout << "\n=== Running Advanced Camera Tutorial ===\n\n";
        
        bool running = true;
        auto last_time = std::chrono::high_resolution_clock::now();
        
        while (running) {
            // Calculate timing
            auto current_time = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::microseconds>(current_time - last_time);
            f32 delta_time = duration.count() / 1000000.0f;
            last_time = current_time;
            
            // Handle input
            running = handle_input();
            
            // Update simulation
            update(delta_time);
            
            // Render with current camera configuration
            render();
            
            // Display info periodically
            frame_count_++;
            if (frame_count_ % 180 == 0) { // Every 3 seconds
                display_camera_info();
            }
        }
        
        std::cout << "\n✅ Advanced Camera Tutorial completed!\n";
        display_educational_summary();
    }

private:
    // Core systems
    std::unique_ptr<renderer::Window> window_;
    std::unique_ptr<ecs::Registry> registry_;
    std::unique_ptr<renderer::Renderer2D> renderer_;
    
    // Tutorial state
    CameraMode current_mode_{CameraMode::Manual};
    u32 frame_count_{0};
    f32 total_time_{0.0f};
    
    // Camera entities and data
    std::vector<ecs::EntityID> camera_entities_;
    ecs::EntityID active_camera_{ecs::INVALID_ENTITY_ID};
    ecs::EntityID target_entity_{ecs::INVALID_ENTITY_ID}; // Entity for camera to follow
    
    // Scene entities
    std::vector<ecs::EntityID> world_objects_;
    std::vector<ecs::EntityID> ui_elements_;
    
    // Camera movement parameters
    struct CameraParams {
        f32 move_speed{300.0f};
        f32 follow_speed{2.0f};
        f32 zoom_speed{1.0f};
        f32 shake_intensity{0.0f};
        f32 shake_duration{0.0f};
        f32 orbital_radius{200.0f};
        f32 orbital_speed{1.0f};
        
        // Waypoints for patrol mode
        std::vector<struct { f32 x, y; }> patrol_waypoints;
        u32 current_waypoint{0};
    } camera_params_;
    
    // Input state
    struct InputState {
        bool keys[256]{false};
        f32 mouse_x{0.0f}, mouse_y{0.0f};
        bool mouse_captured{false};
    } input_;
    
    /**
     * @brief Initialize graphics system
     */
    bool initialize_graphics() {
#ifdef ECSCOPE_HAS_GRAPHICS
        if (SDL_Init(SDL_INIT_VIDEO) < 0) {
            std::cerr << "SDL2 initialization failed: " << SDL_GetError() << "\n";
            return false;
        }
        
        window_ = std::make_unique<renderer::Window>();
        if (!window_->create(1200, 800, "ECScope Tutorial 3: Advanced Camera Systems")) {
            std::cerr << "Window creation failed\n";
            return false;
        }
        
        std::cout << "✅ Graphics system initialized\n";
        return true;
#else
        std::cerr << "Graphics support not compiled\n";
        return false;
#endif
    }
    
    /**
     * @brief Initialize ECS and renderer
     */
    bool initialize_ecs() {
        registry_ = std::make_unique<ecs::Registry>();
        
        auto config = renderer::Renderer2DConfig::educational_mode();
        config.rendering.enable_frustum_culling = true; // Important for camera systems
        config.debug.enable_debug_rendering = true;
        
        renderer_ = std::make_unique<renderer::Renderer2D>(config);
        auto result = renderer_->initialize();
        if (!result.is_ok()) {
            std::cerr << "Renderer initialization failed: " << result.error() << "\n";
            return false;
        }
        
        std::cout << "✅ ECS and renderer initialized\n";
        return true;
    }
    
    /**
     * @brief Create a diverse world for camera demonstration
     */
    void create_demo_world() {
        std::cout << "Creating demo world...\n";
        
        // Create a target entity for camera following
        target_entity_ = registry_->create_entity();
        auto& target_transform = registry_->add_component<ecs::components::Transform>(target_entity_);
        target_transform.position = {0.0f, 0.0f, 0.0f};
        target_transform.scale = {32.0f, 32.0f, 1.0f};
        
        auto& target_sprite = registry_->add_component<renderer::components::RenderableSprite>(target_entity_);
        target_sprite.color = {1.0f, 0.0f, 0.0f, 1.0f}; // Red target
        target_sprite.z_order = 1.0f; // Above other objects
        
        // Create a large world with various objects
        const u32 world_size = 2000;
        const u32 grid_spacing = 100;
        
        for (i32 x = -world_size; x <= world_size; x += grid_spacing) {
            for (i32 y = -world_size; y <= world_size; y += grid_spacing) {
                // Skip center area where target starts
                if (std::abs(x) < 200 && std::abs(y) < 200) continue;
                
                auto entity = registry_->create_entity();
                
                auto& transform = registry_->add_component<ecs::components::Transform>(entity);
                transform.position = {static_cast<f32>(x), static_cast<f32>(y), 0.0f};
                transform.scale = {24.0f, 24.0f, 1.0f};
                
                auto& sprite = registry_->add_component<renderer::components::RenderableSprite>(entity);
                // Color based on position for visual interest
                f32 color_factor_x = (x + world_size) / (2.0f * world_size);
                f32 color_factor_y = (y + world_size) / (2.0f * world_size);
                sprite.color = {color_factor_x, color_factor_y, 0.5f, 0.8f};
                sprite.z_order = 0.0f;
                
                world_objects_.push_back(entity);
            }
        }
        
        // Set up patrol waypoints
        camera_params_.patrol_waypoints = {
            {-500.0f, -500.0f},
            {500.0f, -500.0f},
            {500.0f, 500.0f},
            {-500.0f, 500.0f}
        };
        
        std::cout << "✅ Created world with " << world_objects_.size() << " objects\n";
        std::cout << "✅ Target entity created (red square)\n";
    }
    
    /**
     * @brief Create multiple cameras for demonstration
     */
    void create_cameras() {
        std::cout << "Creating camera systems...\n";
        
        // Main camera
        auto main_camera = registry_->create_entity();
        
        auto& main_transform = registry_->add_component<ecs::components::Transform>(main_camera);
        main_transform.position = {0.0f, 0.0f, 0.0f};
        
        auto& main_camera_comp = registry_->add_component<renderer::components::Camera2D>(main_camera);
        main_camera_comp.position = {0.0f, 0.0f};
        main_camera_comp.zoom = 1.0f;
        main_camera_comp.viewport_width = 1200.0f;
        main_camera_comp.viewport_height = 800.0f;
        
        camera_entities_.push_back(main_camera);
        active_camera_ = main_camera;
        
        // Secondary camera for split-screen and picture-in-picture
        auto secondary_camera = registry_->create_entity();
        
        auto& sec_transform = registry_->add_component<ecs::components::Transform>(secondary_camera);
        sec_transform.position = {500.0f, 500.0f, 0.0f};
        
        auto& sec_camera_comp = registry_->add_component<renderer::components::Camera2D>(secondary_camera);
        sec_camera_comp.position = {500.0f, 500.0f};
        sec_camera_comp.zoom = 2.0f; // More zoomed in
        sec_camera_comp.viewport_width = 400.0f;  // Smaller viewport
        sec_camera_comp.viewport_height = 300.0f;
        
        camera_entities_.push_back(secondary_camera);
        
        std::cout << "✅ Created " << camera_entities_.size() << " cameras\n";
    }
    
    /**
     * @brief Reset tutorial state for clean demonstration
     */
    void reset_tutorial_state() {
        total_time_ = 0.0f;
        frame_count_ = 0;
        camera_params_.shake_intensity = 0.0f;
        camera_params_.shake_duration = 0.0f;
        camera_params_.current_waypoint = 0;
        
        // Reset main camera position
        auto& transform = registry_->get_component<ecs::components::Transform>(active_camera_);
        transform.position = {0.0f, 0.0f, 0.0f};
        
        auto& camera = registry_->get_component<renderer::components::Camera2D>(active_camera_);
        camera.position = {0.0f, 0.0f};
        camera.zoom = 1.0f;
    }
    
    /**
     * @brief Handle input for camera control and mode switching
     */
    bool handle_input() {
#ifdef ECSCOPE_HAS_GRAPHICS
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_QUIT:
                    return false;
                    
                case SDL_KEYDOWN:
                    handle_key_down(event.key.keysym.sym);
                    break;
                    
                case SDL_KEYUP:
                    handle_key_up(event.key.keysym.sym);
                    break;
                    
                case SDL_MOUSEMOTION:
                    input_.mouse_x = event.motion.x;
                    input_.mouse_y = event.motion.y;
                    break;
                    
                case SDL_MOUSEWHEEL:
                    handle_mouse_wheel(event.wheel.y);
                    break;
            }
        }
        return true;
#else
        return false;
#endif
    }
    
    /**
     * @brief Handle key press events
     */
    void handle_key_down(u32 key) {
        if (key < 256) {
            input_.keys[key] = true;
        }
        
        switch (key) {
            case SDLK_ESCAPE:
            case SDLK_q:
                // Exit handled in main loop
                break;
                
            case SDLK_1:
                change_camera_mode(CameraMode::Manual);
                break;
            case SDLK_2:
                change_camera_mode(CameraMode::FollowTarget);
                break;
            case SDLK_3:
                change_camera_mode(CameraMode::Orbital);
                break;
            case SDLK_4:
                change_camera_mode(CameraMode::Patrol);
                break;
            case SDLK_5:
                change_camera_mode(CameraMode::Shake);
                break;
            case SDLK_6:
                change_camera_mode(CameraMode::Split);
                break;
            case SDLK_7:
                change_camera_mode(CameraMode::PictureInPicture);
                break;
                
            case SDLK_r:
                reset_tutorial_state();
                break;
                
            case SDLK_c:
                center_camera_on_target();
                break;
                
            case SDLK_SPACE:
                trigger_camera_shake();
                break;
        }
    }
    
    /**
     * @brief Handle key release events
     */
    void handle_key_up(u32 key) {
        if (key < 256) {
            input_.keys[key] = false;
        }
    }
    
    /**
     * @brief Handle mouse wheel for zoom control
     */
    void handle_mouse_wheel(i32 wheel_y) {
        auto& camera = registry_->get_component<renderer::components::Camera2D>(active_camera_);
        f32 zoom_factor = 1.0f + (wheel_y * 0.1f);
        camera.zoom = std::clamp(camera.zoom * zoom_factor, 0.1f, 5.0f);
        
        std::cout << "🔍 Camera zoom: " << std::fixed << std::setprecision(2) << camera.zoom << "x\n";
    }
    
    /**
     * @brief Update camera systems and target movement
     */
    void update(f32 delta_time) {
        total_time_ += delta_time;
        
        // Update target entity movement (for camera following demo)
        update_target_movement(delta_time);
        
        // Update camera based on current mode
        switch (current_mode_) {
            case CameraMode::Manual:
                update_manual_camera(delta_time);
                break;
            case CameraMode::FollowTarget:
                update_follow_camera(delta_time);
                break;
            case CameraMode::Orbital:
                update_orbital_camera(delta_time);
                break;
            case CameraMode::Patrol:
                update_patrol_camera(delta_time);
                break;
            case CameraMode::Shake:
                update_shake_camera(delta_time);
                break;
            case CameraMode::Split:
            case CameraMode::PictureInPicture:
                update_multi_camera(delta_time);
                break;
        }
        
        // Update camera shake if active
        if (camera_params_.shake_duration > 0.0f) {
            update_camera_shake(delta_time);
        }
    }
    
    /**
     * @brief Move the target entity for camera following demonstration
     */
    void update_target_movement(f32 delta_time) {
        auto& target_transform = registry_->get_component<ecs::components::Transform>(target_entity_);
        
        // Circular movement pattern
        f32 radius = 400.0f;
        f32 speed = 0.5f;
        target_transform.position.x = std::cos(total_time_ * speed) * radius;
        target_transform.position.y = std::sin(total_time_ * speed) * radius;
    }
    
    /**
     * @brief Update manual camera control (WASD movement)
     */
    void update_manual_camera(f32 delta_time) {
        auto& camera_transform = registry_->get_component<ecs::components::Transform>(active_camera_);
        auto& camera = registry_->get_component<renderer::components::Camera2D>(active_camera_);
        
        f32 speed = camera_params_.move_speed * delta_time / camera.zoom; // Zoom-adjusted speed
        
        if (input_.keys['w'] || input_.keys['W']) camera_transform.position.y += speed;
        if (input_.keys['s'] || input_.keys['S']) camera_transform.position.y -= speed;
        if (input_.keys['a'] || input_.keys['A']) camera_transform.position.x -= speed;
        if (input_.keys['d'] || input_.keys['D']) camera_transform.position.x += speed;
        
        // Update camera component position
        camera.position = {camera_transform.position.x, camera_transform.position.y};
    }
    
    /**
     * @brief Update smooth camera following
     */
    void update_follow_camera(f32 delta_time) {
        const auto& target_transform = registry_->get_component<ecs::components::Transform>(target_entity_);
        auto& camera_transform = registry_->get_component<ecs::components::Transform>(active_camera_);
        auto& camera = registry_->get_component<renderer::components::Camera2D>(active_camera_);
        
        // Smooth interpolation towards target
        f32 follow_speed = camera_params_.follow_speed * delta_time;
        camera_transform.position.x += (target_transform.position.x - camera_transform.position.x) * follow_speed;
        camera_transform.position.y += (target_transform.position.y - camera_transform.position.y) * follow_speed;
        
        camera.position = {camera_transform.position.x, camera_transform.position.y};
    }
    
    /**
     * @brief Update orbital camera movement
     */
    void update_orbital_camera(f32 delta_time) {
        const auto& target_transform = registry_->get_component<ecs::components::Transform>(target_entity_);
        auto& camera_transform = registry_->get_component<ecs::components::Transform>(active_camera_);
        auto& camera = registry_->get_component<renderer::components::Camera2D>(active_camera_);
        
        f32 angle = total_time_ * camera_params_.orbital_speed;
        camera_transform.position.x = target_transform.position.x + std::cos(angle) * camera_params_.orbital_radius;
        camera_transform.position.y = target_transform.position.y + std::sin(angle) * camera_params_.orbital_radius;
        
        camera.position = {camera_transform.position.x, camera_transform.position.y};
    }
    
    /**
     * @brief Update patrol camera movement
     */
    void update_patrol_camera(f32 delta_time) {
        auto& camera_transform = registry_->get_component<ecs::components::Transform>(active_camera_);
        auto& camera = registry_->get_component<renderer::components::Camera2D>(active_camera_);
        
        if (camera_params_.patrol_waypoints.empty()) return;
        
        auto& target_waypoint = camera_params_.patrol_waypoints[camera_params_.current_waypoint];
        f32 target_x = target_waypoint.x;
        f32 target_y = target_waypoint.y;
        
        // Move towards current waypoint
        f32 dx = target_x - camera_transform.position.x;
        f32 dy = target_y - camera_transform.position.y;
        f32 distance = std::sqrt(dx * dx + dy * dy);
        
        if (distance < 50.0f) { // Close enough, move to next waypoint
            camera_params_.current_waypoint = (camera_params_.current_waypoint + 1) % camera_params_.patrol_waypoints.size();
        } else {
            // Move towards waypoint
            f32 speed = camera_params_.move_speed * delta_time;
            camera_transform.position.x += (dx / distance) * speed;
            camera_transform.position.y += (dy / distance) * speed;
        }
        
        camera.position = {camera_transform.position.x, camera_transform.position.y};
    }
    
    /**
     * @brief Update camera shake effects
     */
    void update_shake_camera(f32 delta_time) {
        if (camera_params_.shake_duration <= 0.0f) {
            trigger_camera_shake(); // Continuous shake in shake mode
        }
        update_camera_shake(delta_time);
    }
    
    /**
     * @brief Update multi-camera systems
     */
    void update_multi_camera(f32 delta_time) {
        // For split screen and picture-in-picture, we update both cameras
        // Main camera follows target
        update_follow_camera(delta_time);
        
        // Secondary camera orbits around a different point
        if (camera_entities_.size() > 1) {
            auto& sec_transform = registry_->get_component<ecs::components::Transform>(camera_entities_[1]);
            auto& sec_camera = registry_->get_component<renderer::components::Camera2D>(camera_entities_[1]);
            
            f32 angle = total_time_ * 1.5f; // Faster orbital movement
            sec_transform.position.x = std::cos(angle) * 600.0f;
            sec_transform.position.y = std::sin(angle) * 600.0f;
            
            sec_camera.position = {sec_transform.position.x, sec_transform.position.y};
        }
    }
    
    /**
     * @brief Apply camera shake effects
     */
    void update_camera_shake(f32 delta_time) {
        camera_params_.shake_duration -= delta_time;
        
        if (camera_params_.shake_duration > 0.0f) {
            auto& camera_transform = registry_->get_component<ecs::components::Transform>(active_camera_);
            auto& camera = registry_->get_component<renderer::components::Camera2D>(active_camera_);
            
            // Random offset based on shake intensity
            f32 offset_x = ((rand() % 200 - 100) / 100.0f) * camera_params_.shake_intensity;
            f32 offset_y = ((rand() % 200 - 100) / 100.0f) * camera_params_.shake_intensity;
            
            // Apply shake to camera position (this would normally be added to the base position)
            camera.position.x = camera_transform.position.x + offset_x;
            camera.position.y = camera_transform.position.y + offset_y;
        }
    }
    
    /**
     * @brief Render the scene with current camera configuration
     */
    void render() {
        renderer_->begin_frame();
        
        switch (current_mode_) {
            case CameraMode::Split:
                render_split_screen();
                break;
                
            case CameraMode::PictureInPicture:
                render_picture_in_picture();
                break;
                
            default:
                render_single_camera();
                break;
        }
        
        renderer_->end_frame();
        
        if (window_) {
            window_->swap_buffers();
        }
    }
    
    /**
     * @brief Render with single active camera
     */
    void render_single_camera() {
        const auto& camera = registry_->get_component<renderer::components::Camera2D>(active_camera_);
        renderer_->set_active_camera(camera);
        renderer_->render_entities(*registry_);
        
        // Draw coordinate system for reference
        draw_coordinate_system();
    }
    
    /**
     * @brief Render split-screen view
     */
    void render_split_screen() {
        // This is a simplified version - real implementation would use viewports
        // Render with main camera (left half)
        const auto& main_camera = registry_->get_component<renderer::components::Camera2D>(active_camera_);
        renderer_->set_active_camera(main_camera);
        renderer_->render_entities(*registry_);
        
        // Educational note: In a full implementation, you would:
        // 1. Set viewport to left half of screen
        // 2. Render with first camera
        // 3. Set viewport to right half of screen  
        // 4. Render with second camera
        
        std::cout << "📺 Split-screen rendering (simplified demonstration)\n";
    }
    
    /**
     * @brief Render picture-in-picture view
     */
    void render_picture_in_picture() {
        // Main camera renders full screen
        const auto& main_camera = registry_->get_component<renderer::components::Camera2D>(active_camera_);
        renderer_->set_active_camera(main_camera);
        renderer_->render_entities(*registry_);
        
        // Educational note: Picture-in-picture would involve:
        // 1. Render main scene to full framebuffer
        // 2. Set smaller viewport for inset
        // 3. Render secondary camera view
        // 4. Composite or render to separate texture
        
        std::cout << "📺 Picture-in-picture rendering (simplified demonstration)\n";
    }
    
    /**
     * @brief Draw coordinate system and reference markers
     */
    void draw_coordinate_system() {
        // Draw origin marker
        renderer_->draw_debug_line(-50.0f, 0.0f, 50.0f, 0.0f, {1.0f, 0.0f, 0.0f, 1.0f}, 2.0f); // Red X-axis
        renderer_->draw_debug_line(0.0f, -50.0f, 0.0f, 50.0f, {0.0f, 1.0f, 0.0f, 1.0f}, 2.0f); // Green Y-axis
        
        // Draw grid lines
        const i32 grid_size = 2000;
        const i32 grid_spacing = 200;
        
        for (i32 x = -grid_size; x <= grid_size; x += grid_spacing) {
            if (x == 0) continue; // Skip origin lines
            renderer_->draw_debug_line(x, -grid_size, x, grid_size, {0.3f, 0.3f, 0.3f, 0.5f}, 1.0f);
        }
        
        for (i32 y = -grid_size; y <= grid_size; y += grid_spacing) {
            if (y == 0) continue; // Skip origin lines
            renderer_->draw_debug_line(-grid_size, y, grid_size, y, {0.3f, 0.3f, 0.3f, 0.5f}, 1.0f);
        }
    }
    
    /**
     * @brief Change camera mode and provide educational explanation
     */
    void change_camera_mode(CameraMode new_mode) {
        current_mode_ = new_mode;
        reset_tutorial_state();
        
        const char* mode_names[] = {
            "Manual Control",
            "Follow Target", 
            "Orbital Movement",
            "Patrol Path",
            "Camera Shake",
            "Split Screen",
            "Picture-in-Picture"
        };
        
        std::cout << "\n🎥 Camera Mode: " << mode_names[static_cast<int>(new_mode)] << "\n";
        
        // Educational explanations
        switch (new_mode) {
            case CameraMode::Manual:
                std::cout << "   📚 Use WASD to manually control camera position\n";
                std::cout << "   💡 Speed adjusts automatically based on zoom level\n";
                break;
                
            case CameraMode::FollowTarget:
                std::cout << "   📚 Camera smoothly follows the red target entity\n";
                std::cout << "   💡 Uses interpolation for smooth movement, avoids jittering\n";
                break;
                
            case CameraMode::Orbital:
                std::cout << "   📚 Camera orbits around the target in a circular pattern\n";
                std::cout << "   💡 Useful for showcasing objects or creating dynamic views\n";
                break;
                
            case CameraMode::Patrol:
                std::cout << "   📚 Camera moves between predefined waypoints\n";
                std::cout << "   💡 Common for cutscenes or automated camera movements\n";
                break;
                
            case CameraMode::Shake:
                std::cout << "   📚 Adds camera shake effects for impact and drama\n";
                std::cout << "   💡 Press SPACE to trigger shake effects\n";
                break;
                
            case CameraMode::Split:
                std::cout << "   📚 Demonstrates split-screen multi-camera rendering\n";
                std::cout << "   💡 Each viewport can have different camera settings\n";
                break;
                
            case CameraMode::PictureInPicture:
                std::cout << "   📚 Shows picture-in-picture rendering technique\n";
                std::cout << "   💡 Useful for minimap or security camera views\n";
                break;
        }
    }
    
    /**
     * @brief Center camera on target entity
     */
    void center_camera_on_target() {
        const auto& target_transform = registry_->get_component<ecs::components::Transform>(target_entity_);
        auto& camera_transform = registry_->get_component<ecs::components::Transform>(active_camera_);
        auto& camera = registry_->get_component<renderer::components::Camera2D>(active_camera_);
        
        camera_transform.position = target_transform.position;
        camera.position = {target_transform.position.x, target_transform.position.y};
        
        std::cout << "📍 Camera centered on target at (" 
                  << target_transform.position.x << ", " << target_transform.position.y << ")\n";
    }
    
    /**
     * @brief Trigger camera shake effect
     */
    void trigger_camera_shake() {
        camera_params_.shake_intensity = 15.0f;
        camera_params_.shake_duration = 0.5f;
        std::cout << "💥 Camera shake triggered!\n";
    }
    
    /**
     * @brief Display current camera information
     */
    void display_camera_info() {
        const auto& camera_transform = registry_->get_component<ecs::components::Transform>(active_camera_);
        const auto& camera = registry_->get_component<renderer::components::Camera2D>(active_camera_);
        const auto& target_transform = registry_->get_component<ecs::components::Transform>(target_entity_);
        
        std::cout << "\n" << std::string(50, '-') << "\n";
        std::cout << "CAMERA INFORMATION\n";
        std::cout << std::string(50, '-') << "\n";
        std::cout << std::fixed << std::setprecision(1);
        std::cout << "Camera Position: (" << camera.position.x << ", " << camera.position.y << ")\n";
        std::cout << "Camera Zoom:     " << camera.zoom << "x\n";
        std::cout << "Target Position: (" << target_transform.position.x << ", " << target_transform.position.y << ")\n";
        
        // Calculate distance to target
        f32 dx = camera.position.x - target_transform.position.x;
        f32 dy = camera.position.y - target_transform.position.y;
        f32 distance = std::sqrt(dx * dx + dy * dy);
        std::cout << "Distance to Target: " << distance << " units\n";
        
        // Educational insights about coordinate systems
        std::cout << "\n💡 Coordinate System Notes:\n";
        std::cout << "• World coordinates are independent of camera\n";
        std::cout << "• Camera zoom affects visible world area\n";
        std::cout << "• Screen coordinates are derived from world + camera\n";
    }
    
    /**
     * @brief Display educational summary
     */
    void display_educational_summary() {
        std::cout << "\n" << std::string(70, '=') << "\n";
        std::cout << "ADVANCED CAMERAS TUTORIAL - EDUCATIONAL SUMMARY\n";
        std::cout << std::string(70, '=') << "\n";
        
        std::cout << "\n📚 Key Camera Concepts Learned:\n\n";
        
        std::cout << "1. CAMERA TRANSFORMATIONS\n";
        std::cout << "   • Cameras have position, zoom, and viewport properties\n";
        std::cout << "   • World-to-screen coordinate transformation\n";
        std::cout << "   • Zoom affects both rendering scale and movement speed\n\n";
        
        std::cout << "2. SMOOTH CAMERA MOVEMENT\n";
        std::cout << "   • Linear interpolation (lerp) for smooth following\n";
        std::cout << "   • Adjustable follow speed for different feels\n";
        std::cout << "   • Avoiding jittery movement with proper smoothing\n\n";
        
        std::cout << "3. ADVANCED CAMERA TECHNIQUES\n";
        std::cout << "   • Orbital movement for dynamic perspectives\n";
        std::cout << "   • Patrol systems for automated movement\n";
        std::cout << "   • Camera shake for impact and feedback\n\n";
        
        std::cout << "4. MULTI-CAMERA SYSTEMS\n";
        std::cout << "   • Viewport management for split-screen rendering\n";
        std::cout << "   • Picture-in-picture for minimap/security views\n";
        std::cout << "   • Independent camera properties per viewport\n\n";
        
        std::cout << "5. PERFORMANCE CONSIDERATIONS\n";
        std::cout << "   • Frustum culling reduces rendering overhead\n";
        std::cout << "   • Camera-based LOD systems for optimization\n";
        std::cout << "   • Efficient coordinate transformations\n\n";
        
        std::cout << "💡 Professional Tips:\n";
        std::cout << "• Always consider the player's comfort and readability\n";
        std::cout << "• Use easing functions for more natural movement\n";
        std::cout << "• Test camera systems with different content densities\n";
        std::cout << "• Implement camera bounds to prevent showing empty areas\n";
        std::cout << "• Consider accessibility - avoid excessive shaking/movement\n\n";
        
        std::cout << "🎓 Congratulations! You've mastered advanced 2D camera systems.\n";
        std::cout << "Next: Explore Tutorial 4 for advanced lighting and effects.\n";
    }
    
    /**
     * @brief Print control instructions
     */
    void print_controls() {
        std::cout << "\n" << std::string(55, '-') << "\n";
        std::cout << "INTERACTIVE CONTROLS:\n";
        std::cout << std::string(55, '-') << "\n";
        std::cout << "1-7        - Switch camera modes\n";
        std::cout << "WASD       - Manual camera movement (mode 1)\n";
        std::cout << "Mouse Wheel- Zoom in/out\n";
        std::cout << "C          - Center camera on target\n";
        std::cout << "SPACE      - Trigger camera shake\n";
        std::cout << "R          - Reset camera and state\n";
        std::cout << "Q/ESC      - Exit tutorial\n";
        std::cout << std::string(55, '-') << "\n";
        std::cout << "Camera Modes:\n";
        std::cout << "1 = Manual     2 = Follow     3 = Orbital    4 = Patrol\n";
        std::cout << "5 = Shake      6 = Split      7 = Picture-in-Picture\n";
        std::cout << std::string(55, '-') << "\n";
    }
};

/**
 * @brief Tutorial entry point
 */
int main() {
    core::Log::initialize(core::LogLevel::INFO);
    
    std::cout << R"(
    ╔══════════════════════════════════════════════════════════════╗
    ║            ECScope 2D Rendering Tutorial 3                  ║
    ║                Advanced Camera Systems                       ║
    ╠══════════════════════════════════════════════════════════════╣
    ║  This tutorial explores sophisticated camera techniques      ║
    ║  essential for professional 2D game development.            ║
    ║                                                              ║
    ║  You will master:                                            ║
    ║  • Multiple camera management and control                    ║
    ║  • Smooth following and interpolated movement               ║
    ║  • Advanced camera effects and cinematics                   ║
    ║  • Multi-viewport and split-screen rendering               ║
    ║  • Coordinate system transformations                        ║
    ║  • Performance optimization with culling                    ║
    ╚══════════════════════════════════════════════════════════════╝
    )";
    
    try {
        AdvancedCamerasTutorial tutorial;
        
        if (!tutorial.initialize()) {
            std::cerr << "\n❌ Tutorial initialization failed!\n";
            return -1;
        }
        
        tutorial.run();
        
    } catch (const std::exception& e) {
        std::cerr << "\n💥 Tutorial crashed: " << e.what() << "\n";
        return -1;
    }
    
    return 0;
}