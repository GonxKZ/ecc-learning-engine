/**
 * @file 03_camera_systems_and_viewports.cpp
 * @brief Tutorial 3: Camera Systems and Viewports - World to Screen Transformation
 * 
 * This tutorial explores 2D camera systems, coordinate transformations, and viewport management.
 * You'll learn how cameras work mathematically and practically in game engines.
 * 
 * Learning Objectives:
 * 1. Understand coordinate system transformations (world → screen)
 * 2. Learn camera properties: position, zoom, rotation, viewport
 * 3. Explore multiple camera systems and split-screen rendering
 * 4. Master coordinate conversion functions
 * 5. Implement camera following and smooth movement
 * 
 * Key Concepts Covered:
 * - World space vs Screen space coordinates
 * - View and projection matrices in 2D
 * - Viewport rectangles and scissor testing
 * - Camera movement, zoom, and rotation
 * - Multi-camera rendering (minimap, UI overlay)
 * - Camera constraints and boundaries
 * 
 * Educational Value:
 * Understanding cameras is fundamental to any graphics application.
 * This tutorial provides mathematical insights and practical implementation
 * techniques that apply to both 2D and 3D graphics programming.
 * 
 * @author ECScope Educational ECS Framework
 * @date 2024
 */

#include <iostream>
#include <memory>
#include <vector>
#include <cmath>
#include <iomanip>

// ECScope Core
#include "../../src/core/types.hpp"
#include "../../src/core/log.hpp"

// ECS System
#include "../../src/ecs/registry.hpp"
#include "../../src/ecs/components/transform.hpp"

// 2D Rendering System
#include "../../src/renderer/renderer_2d.hpp"
#include "../../src/renderer/components/render_components.hpp"
#include "../../src/renderer/window.hpp"

using namespace ecscope;
using namespace ecscope::renderer;
using namespace ecscope::renderer::components;

/**
 * @brief Camera Systems Tutorial with Interactive Demonstrations
 * 
 * This tutorial provides hands-on experience with camera systems,
 * showing coordinate transformations and viewport management in action.
 */
class CameraSystemsTutorial {
public:
    CameraSystemsTutorial() = default;
    ~CameraSystemsTutorial() { cleanup(); }
    
    bool initialize() {
        core::Log::info("Tutorial", "=== Camera Systems and Viewports Tutorial ===");
        core::Log::info("Tutorial", "Learning objective: Master 2D camera systems and coordinate transformations");
        
        // Initialize window (larger for multiple viewports)
        window_ = std::make_unique<Window>("Tutorial 3: Camera Systems", 1600, 1200);
        if (!window_->initialize()) {
            core::Log::error("Tutorial", "Failed to create window");
            return false;
        }
        
        // Initialize renderer with debug visualization
        Renderer2DConfig renderer_config = Renderer2DConfig::educational_mode();
        renderer_config.debug.enable_debug_rendering = true;
        renderer_config.debug.show_performance_overlay = false; // Focus on cameras
        
        renderer_ = std::make_unique<Renderer2D>(renderer_config);
        if (!renderer_->initialize().is_ok()) {
            core::Log::error("Tutorial", "Failed to initialize renderer");
            return false;
        }
        
        // Create ECS registry and demo world
        registry_ = std::make_unique<ecs::Registry>();
        create_demo_world();
        
        // Set up multiple cameras for demonstration
        setup_cameras();
        
        core::Log::info("Tutorial", "Camera systems initialized. Starting demonstrations...");
        return true;
    }
    
    void run() {
        if (!window_ || !renderer_) return;
        
        core::Log::info("Tutorial", "Starting camera systems demonstration...");
        
        // Run through different camera demonstrations
        demonstrate_basic_camera_properties();
        demonstrate_coordinate_transformations();
        demonstrate_viewport_management();
        demonstrate_camera_movement();
        demonstrate_multi_camera_rendering();
        
        display_educational_summary();
    }
    
private:
    //=========================================================================
    // Demo World Creation
    //=========================================================================
    
    void create_demo_world() {
        core::Log::info("World", "Creating demo world with grid and reference objects");
        
        // Create a grid of reference sprites for visual orientation
        for (i32 x = -10; x <= 10; x += 2) {
            for (i32 y = -10; y <= 10; y += 2) {
                u32 entity = registry_->create_entity();
                world_entities_.push_back(entity);
                
                Transform transform;
                transform.position = {x * 100.0f, y * 100.0f, 0.0f};
                transform.scale = {40.0f, 40.0f, 1.0f};
                registry_->add_component(entity, transform);
                
                RenderableSprite sprite;
                sprite.texture = TextureHandle{1, 32, 32};
                
                // Color code based on position
                if (x == 0 && y == 0) {
                    sprite.color_modulation = Color::red(); // Origin
                } else if (x == 0) {
                    sprite.color_modulation = Color::green(); // Y-axis
                } else if (y == 0) {
                    sprite.color_modulation = Color::blue(); // X-axis
                } else {
                    sprite.color_modulation = Color{128, 128, 128, 255}; // Grid points
                }
                
                sprite.z_order = 0.0f;
                registry_->add_component(entity, sprite);
            }
        }
        
        // Create a moving target for camera following demonstrations
        u32 target_entity = registry_->create_entity();
        target_entity_ = target_entity;
        world_entities_.push_back(target_entity);
        
        Transform target_transform;
        target_transform.position = {0.0f, 0.0f, 10.0f};
        target_transform.scale = {60.0f, 60.0f, 1.0f};
        registry_->add_component(target_entity, target_transform);
        
        RenderableSprite target_sprite;
        target_sprite.texture = TextureHandle{1, 32, 32};
        target_sprite.color_modulation = Color::yellow();
        target_sprite.z_order = 10.0f;
        registry_->add_component(target_entity, target_sprite);
        
        core::Log::info("World", "Created {}x{} grid with reference axes and moving target",
                       21, 21);
    }
    
    void setup_cameras() {
        core::Log::info("Cameras", "Setting up multiple cameras for demonstration");
        
        // Main camera (full screen)
        main_camera_ = Camera2D::create_main_camera(1600, 1200);
        main_camera_.set_position(0.0f, 0.0f);
        main_camera_.set_zoom(1.0f);
        
        // Minimap camera (small viewport in corner)
        minimap_camera_ = Camera2D::create_minimap_camera(1200, 50, 350, 200, 0.3f);
        minimap_camera_.set_position(0.0f, 0.0f);
        
        // UI camera (screen space)
        ui_camera_ = Camera2D::create_ui_camera(1600, 1200);
        
        // Zoomed camera (for detailed view)
        zoomed_camera_ = Camera2D::create_main_camera(800, 600);
        zoomed_camera_.viewport = {50, 300, 800, 600};
        zoomed_camera_.set_zoom(2.0f);
        
        core::Log::info("Cameras", "Created 4 cameras: main, minimap, UI, and zoomed");
        
        // Log camera properties for educational purposes
        log_camera_properties("Main Camera", main_camera_);
        log_camera_properties("Minimap Camera", minimap_camera_);
        log_camera_properties("UI Camera", ui_camera_);
        log_camera_properties("Zoomed Camera", zoomed_camera_);
    }
    
    void log_camera_properties(const char* name, const Camera2D& camera) {
        core::Log::info("Camera", "{}: Position({:.1f}, {:.1f}), Zoom: {:.2f}x",
                       name, camera.position.x, camera.position.y, camera.zoom);
        core::Log::info("Camera", "  Viewport: ({}, {}) {}x{}", 
                       camera.viewport.x, camera.viewport.y,
                       camera.viewport.width, camera.viewport.height);
        
        auto info = camera.get_camera_info();
        core::Log::info("Camera", "  World view: {:.1f}x{:.1f} units, {:.2f} pixels/unit",
                       info.world_width, info.world_height, info.pixels_per_unit);
    }
    
    //=========================================================================
    // Camera Demonstrations
    //=========================================================================
    
    void demonstrate_basic_camera_properties() {
        core::Log::info("Demo 1", "=== BASIC CAMERA PROPERTIES ===");
        core::Log::info("Explanation", "Understanding camera position, zoom, and rotation effects");
        
        std::vector<std::pair<const char*, std::function<void()>>> tests = {
            {"Default Position (0,0)", [this]() { 
                main_camera_.set_position(0.0f, 0.0f);
                main_camera_.set_zoom(1.0f);
                main_camera_.set_rotation(0.0f);
            }},
            {"Moved Right (+200, 0)", [this]() { 
                main_camera_.set_position(200.0f, 0.0f);
            }},
            {"Moved Up (200, +200)", [this]() { 
                main_camera_.set_position(200.0f, 200.0f);
            }},
            {"Zoomed In 2x", [this]() { 
                main_camera_.set_zoom(2.0f);
            }},
            {"Zoomed Out 0.5x", [this]() { 
                main_camera_.set_zoom(0.5f);
            }},
            {"Rotated 45 degrees", [this]() { 
                main_camera_.set_rotation(3.14159f / 4.0f);
                main_camera_.set_zoom(1.0f);
            }},
        };
        
        for (const auto& [description, setup] : tests) {
            core::Log::info("Test", "Demonstrating: {}", description);
            setup();
            
            // Render several frames to show the effect
            render_demonstration_frames(30, description);
            
            // Show coordinate transformation examples
            demonstrate_coordinate_conversion();
        }
    }
    
    void demonstrate_coordinate_transformations() {
        core::Log::info("Demo 2", "=== COORDINATE TRANSFORMATIONS ===");
        core::Log::info("Explanation", "Converting between world space and screen space coordinates");
        
        // Test various world positions and show their screen coordinates
        std::vector<std::pair<f32, f32>> world_points = {
            {0.0f, 0.0f},     // Origin
            {100.0f, 0.0f},   // Right
            {0.0f, 100.0f},   // Up
            {-200.0f, -150.0f}, // Bottom-left
            {300.0f, 200.0f}    // Top-right
        };
        
        core::Log::info("Transformation", "World → Screen coordinate conversion:");
        for (const auto& [world_x, world_y] : world_points) {
            auto screen_pos = main_camera_.world_to_screen(world_x, world_y);
            core::Log::info("Coordinate", "World({:.1f}, {:.1f}) → Screen({:.1f}, {:.1f})",
                           world_x, world_y, screen_pos.x, screen_pos.y);
        }
        
        core::Log::info("Transformation", "Screen → World coordinate conversion:");
        std::vector<std::pair<f32, f32>> screen_points = {
            {800.0f, 600.0f},   // Screen center
            {0.0f, 0.0f},       // Top-left
            {1600.0f, 1200.0f}, // Bottom-right
            {400.0f, 300.0f},   // Quarter point
        };
        
        for (const auto& [screen_x, screen_y] : screen_points) {
            auto world_pos = main_camera_.screen_to_world(screen_x, screen_y);
            core::Log::info("Coordinate", "Screen({:.1f}, {:.1f}) → World({:.1f}, {:.1f})",
                           screen_x, screen_y, world_pos.x, world_pos.y);
        }
    }
    
    void demonstrate_viewport_management() {
        core::Log::info("Demo 3", "=== VIEWPORT MANAGEMENT ===");
        core::Log::info("Explanation", "Multiple viewports rendering different views of the same world");
        
        // Configure cameras for split-screen demonstration
        Camera2D left_camera = Camera2D::create_main_camera(800, 1200);
        left_camera.viewport = {0, 0, 800, 1200};
        left_camera.set_position(-200.0f, 0.0f);
        left_camera.set_zoom(1.5f);
        
        Camera2D right_camera = Camera2D::create_main_camera(800, 1200);
        right_camera.viewport = {800, 0, 800, 1200};
        right_camera.set_position(200.0f, 0.0f);
        right_camera.set_zoom(0.8f);
        
        core::Log::info("Viewport", "Left camera: viewport(0,0,800,1200), position(-200,0), zoom=1.5x");
        core::Log::info("Viewport", "Right camera: viewport(800,0,800,1200), position(200,0), zoom=0.8x");
        
        // Render split-screen view
        for (u32 frame = 0; frame < 60; ++frame) {
            renderer_->begin_frame();
            
            // Render left viewport
            renderer_->begin_camera(left_camera);
            renderer_->render_entities(*registry_);
            draw_debug_info("LEFT VIEW");
            renderer_->end_camera();
            
            // Render right viewport
            renderer_->begin_camera(right_camera);
            renderer_->render_entities(*registry_);
            draw_debug_info("RIGHT VIEW");
            renderer_->end_camera();
            
            renderer_->end_frame();
            window_->swap_buffers();
            window_->poll_events();
        }
        
        core::Log::info("Demo", "Split-screen demonstration completed");
    }
    
    void demonstrate_camera_movement() {
        core::Log::info("Demo 4", "=== CAMERA MOVEMENT AND FOLLOWING ===");
        core::Log::info("Explanation", "Smooth camera movement and target following");
        
        // Reset to main camera
        main_camera_.set_position(0.0f, 0.0f);
        main_camera_.set_zoom(1.0f);
        main_camera_.set_rotation(0.0f);
        
        // Animate target movement
        f32 time = 0.0f;
        const f32 duration = 5.0f; // 5 seconds
        const u32 total_frames = static_cast<u32>(duration * 60.0f); // 60 FPS
        
        for (u32 frame = 0; frame < total_frames; ++frame) {
            time = frame / 60.0f;
            
            // Move target in a circular path
            f32 target_x = std::cos(time * 0.5f) * 300.0f;
            f32 target_y = std::sin(time * 0.5f) * 200.0f;
            
            if (auto* transform = registry_->get_component<Transform>(target_entity_)) {
                transform->position.x = target_x;
                transform->position.y = target_y;
            }
            
            // Smooth camera following with interpolation
            f32 follow_speed = 2.0f;
            f32 current_x = main_camera_.position.x;
            f32 current_y = main_camera_.position.y;
            
            f32 new_x = current_x + (target_x - current_x) * follow_speed * (1.0f / 60.0f);
            f32 new_y = current_y + (target_y - current_y) * follow_speed * (1.0f / 60.0f);
            
            main_camera_.set_position(new_x, new_y);
            
            // Render frame
            renderer_->begin_frame();
            renderer_->set_active_camera(main_camera_);
            renderer_->render_entities(*registry_);
            
            // Draw camera info
            draw_camera_debug_info();
            
            renderer_->end_frame();
            window_->swap_buffers();
            window_->poll_events();
            
            // Log progress periodically
            if (frame % 60 == 0) {
                core::Log::info("Following", "Camera position: ({:.1f}, {:.1f}), Target: ({:.1f}, {:.1f})",
                               new_x, new_y, target_x, target_y);
            }
        }
        
        core::Log::info("Demo", "Camera following demonstration completed");
    }
    
    void demonstrate_multi_camera_rendering() {
        core::Log::info("Demo 5", "=== MULTI-CAMERA RENDERING ===");
        core::Log::info("Explanation", "Rendering with main view, minimap, and UI overlay");
        
        // Set up cameras for multi-view rendering
        main_camera_.set_position(0.0f, 0.0f);
        main_camera_.set_zoom(1.2f);
        
        minimap_camera_.set_position(0.0f, 0.0f);
        minimap_camera_.set_zoom(0.2f); // Show much more of the world
        
        for (u32 frame = 0; frame < 120; ++frame) { // 2 seconds
            f32 time = frame / 60.0f;
            
            // Animate main camera
            main_camera_.set_position(std::sin(time) * 100.0f, std::cos(time) * 80.0f);
            
            renderer_->begin_frame();
            
            // Render main view
            renderer_->begin_camera(main_camera_);
            renderer_->render_entities(*registry_);
            draw_debug_info("MAIN VIEW");
            renderer_->end_camera();
            
            // Render minimap
            renderer_->begin_camera(minimap_camera_);
            renderer_->render_entities(*registry_);
            
            // Draw main camera bounds on minimap
            auto frustum = main_camera_.get_frustum_bounds();
            renderer_->draw_debug_box(frustum.left, frustum.bottom,
                                     frustum.right - frustum.left,
                                     frustum.top - frustum.bottom,
                                     Color::red(), 3.0f);
            
            draw_debug_info("MINIMAP");
            renderer_->end_camera();
            
            // Render UI overlay
            renderer_->begin_camera(ui_camera_);
            draw_ui_overlay();
            renderer_->end_camera();
            
            renderer_->end_frame();
            window_->swap_buffers();
            window_->poll_events();
            
            if (frame % 30 == 0) {
                core::Log::info("MultiCamera", "Frame {}: Main({:.1f}, {:.1f}), Minimap zoom: {:.2f}x",
                               frame, main_camera_.position.x, main_camera_.position.y, minimap_camera_.zoom);
            }
        }
        
        core::Log::info("Demo", "Multi-camera rendering demonstration completed");
    }
    
    //=========================================================================
    // Utility Functions
    //=========================================================================
    
    void render_demonstration_frames(u32 frame_count, const char* description) {
        for (u32 i = 0; i < frame_count; ++i) {
            renderer_->begin_frame();
            renderer_->set_active_camera(main_camera_);
            renderer_->render_entities(*registry_);
            
            // Draw camera info
            draw_camera_debug_info();
            
            renderer_->end_frame();
            window_->swap_buffers();
            window_->poll_events();
        }
    }
    
    void demonstrate_coordinate_conversion() {
        // Pick a few world points and show their screen coordinates
        std::vector<std::pair<f32, f32>> test_points = {
            {0.0f, 0.0f}, {200.0f, 0.0f}, {0.0f, 200.0f}
        };
        
        for (const auto& [wx, wy] : test_points) {
            auto screen_pos = main_camera_.world_to_screen(wx, wy);
            core::Log::info("Coordinate", "World({:.0f},{:.0f}) → Screen({:.0f},{:.0f})", 
                           wx, wy, screen_pos.x, screen_pos.y);
        }
    }
    
    void draw_camera_debug_info() {
        // Draw camera frustum bounds
        auto frustum = main_camera_.get_frustum_bounds();
        renderer_->draw_debug_box(frustum.left, frustum.bottom,
                                 frustum.right - frustum.left,
                                 frustum.top - frustum.bottom,
                                 Color::cyan(), 2.0f);
        
        // Draw camera position
        renderer_->draw_debug_circle(main_camera_.position.x, main_camera_.position.y,
                                    20.0f, Color::red(), 16);
        
        // Draw coordinate axes at camera position
        f32 axis_length = 100.0f / main_camera_.zoom;
        renderer_->draw_debug_line(main_camera_.position.x - axis_length, main_camera_.position.y,
                                  main_camera_.position.x + axis_length, main_camera_.position.y,
                                  Color::red(), 2.0f);
        renderer_->draw_debug_line(main_camera_.position.x, main_camera_.position.y - axis_length,
                                  main_camera_.position.x, main_camera_.position.y + axis_length,
                                  Color::green(), 2.0f);
    }
    
    void draw_debug_info(const char* view_name) {
        // This would typically render text
        // For this demo, we'll just draw a colored border
        // Implementation would use a proper UI/text rendering system
    }
    
    void draw_ui_overlay() {
        // Draw UI elements in screen space
        // This would typically include HUD elements, menus, etc.
        // For this demo, we'll draw simple UI indicators
        
        // Draw performance info background (simplified)
        u32 ui_entity = registry_->create_entity();
        
        Transform ui_transform;
        ui_transform.position = {100.0f, 100.0f, 100.0f}; // Screen space
        ui_transform.scale = {200.0f, 80.0f, 1.0f};
        registry_->add_component(ui_entity, ui_transform);
        
        RenderableSprite ui_sprite;
        ui_sprite.texture = TextureHandle{1, 1, 1};
        ui_sprite.color_modulation = Color{0, 0, 0, 128}; // Semi-transparent
        ui_sprite.z_order = 100.0f;
        registry_->add_component(ui_entity, ui_sprite);
        
        renderer_->render_entities(*registry_);
        
        // Clean up temporary UI entity
        registry_->remove_entity(ui_entity);
    }
    
    void display_educational_summary() {
        std::cout << "\n=== CAMERA SYSTEMS TUTORIAL SUMMARY ===\n\n";
        
        std::cout << "KEY CONCEPTS LEARNED:\n\n";
        
        std::cout << "1. COORDINATE SYSTEMS:\n";
        std::cout << "   - World Space: Where game objects exist (units can be anything)\n";
        std::cout << "   - Screen Space: Pixel coordinates on display (0,0 at top-left)\n";
        std::cout << "   - Camera transforms between these coordinate systems\n\n";
        
        std::cout << "2. CAMERA PROPERTIES:\n";
        std::cout << "   - Position: Where the camera looks in world space\n";
        std::cout << "   - Zoom: How much of the world fits on screen\n";
        std::cout << "   - Rotation: Camera orientation (rarely used in 2D)\n";
        std::cout << "   - Viewport: Rectangle on screen where camera renders\n\n";
        
        std::cout << "3. TRANSFORMATION MATHEMATICS:\n";
        std::cout << "   - View Matrix: Transforms world coordinates to camera space\n";
        std::cout << "   - Projection Matrix: Transforms camera space to screen space\n";
        std::cout << "   - Combined: World → Camera → Screen transformation pipeline\n\n";
        
        std::cout << "4. VIEWPORT MANAGEMENT:\n";
        std::cout << "   - Multiple cameras can render to different screen regions\n";
        std::cout << "   - Useful for split-screen, minimaps, UI overlays\n";
        std::cout << "   - Each viewport can have different zoom and position\n\n";
        
        std::cout << "5. CAMERA MOVEMENT TECHNIQUES:\n";
        std::cout << "   - Direct positioning: Instant camera movement\n";
        std::cout << "   - Interpolated following: Smooth camera movement\n";
        std::cout << "   - Constrained movement: Keeping camera within bounds\n";
        std::cout << "   - Predictive following: Looking ahead in movement direction\n\n";
        
        std::cout << "PRACTICAL APPLICATIONS:\n";
        std::cout << "- Implement smooth camera following for player characters\n";
        std::cout << "- Create picture-in-picture effects with multiple cameras\n";
        std::cout << "- Build UI systems using screen-space cameras\n";
        std::cout << "- Design minimap systems with overview cameras\n";
        std::cout << "- Convert between mouse coordinates and world positions\n\n";
        
        std::cout << "PERFORMANCE CONSIDERATIONS:\n";
        std::cout << "- Camera matrix calculations can be cached between frames\n";
        std::cout << "- Frustum culling uses camera bounds to skip invisible objects\n";
        std::cout << "- Multiple cameras multiply rendering cost per camera\n";
        std::cout << "- Viewport changes require GPU state changes\n\n";
        
        std::cout << "NEXT TUTORIAL: Advanced Materials and Shader Effects\n\n";
    }
    
    void cleanup() {
        if (renderer_) renderer_->shutdown();
        if (window_) window_->shutdown();
    }
    
    // Tutorial resources
    std::unique_ptr<Window> window_;
    std::unique_ptr<Renderer2D> renderer_;
    std::unique_ptr<ecs::Registry> registry_;
    
    // Cameras for demonstration
    Camera2D main_camera_;
    Camera2D minimap_camera_;
    Camera2D ui_camera_;
    Camera2D zoomed_camera_;
    
    // Demo world entities
    std::vector<u32> world_entities_;
    u32 target_entity_{0};
};

//=============================================================================
// Educational Coordinate System Explanation
//=============================================================================

void explain_coordinate_systems() {
    std::cout << "\n=== COORDINATE SYSTEMS IN DEPTH ===\n\n";
    
    std::cout << "WORLD SPACE:\n";
    std::cout << "- The coordinate system where your game logic operates\n";
    std::cout << "- Units can represent meters, pixels, tiles, etc.\n";
    std::cout << "- Origin (0,0) is wherever you define it to be\n";
    std::cout << "- Y-axis can point up (math convention) or down (screen convention)\n\n";
    
    std::cout << "SCREEN SPACE:\n";
    std::cout << "- Pixel coordinates on the physical display\n";
    std::cout << "- Origin (0,0) is typically at top-left corner\n";
    std::cout << "- X increases rightward, Y increases downward\n";
    std::cout << "- Ranges from (0,0) to (screen_width-1, screen_height-1)\n\n";
    
    std::cout << "CAMERA TRANSFORMATION:\n";
    std::cout << "- View Matrix: Applies camera position, rotation, zoom\n";
    std::cout << "- Projection Matrix: Maps camera space to screen space\n";
    std::cout << "- Combined: WorldPos → CameraSpace → ScreenSpace\n\n";
    
    std::cout << "PRACTICAL EXAMPLE:\n";
    std::cout << "- Game object at world position (100, 50)\n";
    std::cout << "- Camera at position (50, 25) with 2x zoom\n";
    std::cout << "- Screen center at (400, 300)\n";
    std::cout << "- Result: Object appears at screen position (500, 350)\n\n";
    
    std::cout << "MATHEMATICS:\n";
    std::cout << "- camera_x = (world_x - camera.position.x) * camera.zoom\n";
    std::cout << "- camera_y = (world_y - camera.position.y) * camera.zoom\n";
    std::cout << "- screen_x = camera_x + screen_center.x\n";
    std::cout << "- screen_y = camera_y + screen_center.y\n\n";
}

//=============================================================================
// Main Function
//=============================================================================

int main() {
    core::Log::info("Main", "Starting Camera Systems and Viewports Tutorial");
    
    std::cout << "\n=== WELCOME TO TUTORIAL 3: CAMERA SYSTEMS AND VIEWPORTS ===\n";
    std::cout << "This tutorial provides comprehensive coverage of 2D camera systems,\n";
    std::cout << "coordinate transformations, and viewport management techniques.\n\n";
    std::cout << "You will experience:\n";
    std::cout << "- Camera position, zoom, and rotation effects\n";
    std::cout << "- Coordinate system transformations (world ↔ screen)\n";
    std::cout << "- Multiple viewport rendering (split-screen, minimap)\n";
    std::cout << "- Smooth camera movement and target following\n";
    std::cout << "- Multi-camera rendering pipelines\n\n";
    std::cout << "Watch for mathematical explanations and practical examples.\n\n";
    
    CameraSystemsTutorial tutorial;
    
    if (!tutorial.initialize()) {
        core::Log::error("Main", "Failed to initialize tutorial");
        return -1;
    }
    
    tutorial.run();
    
    // Show additional coordinate system explanation
    explain_coordinate_systems();
    
    core::Log::info("Main", "Camera Systems Tutorial completed successfully!");
    return 0;
}