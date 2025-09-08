/**
 * @file examples/rendering_tutorials/01_basic_sprite_rendering.cpp
 * @brief Tutorial 1: Basic Sprite Rendering - ECScope Educational Graphics Programming
 * 
 * This tutorial introduces the fundamentals of 2D sprite rendering using ECScope.
 * Students will learn:
 * - Basic rendering system initialization
 * - Creating and configuring cameras
 * - Entity creation with transform and sprite components
 * - Simple render loop implementation
 * - Understanding the rendering pipeline
 * 
 * Educational Objectives:
 * - Understand the ECS approach to rendering
 * - Learn basic 2D graphics concepts
 * - Familiarize with ECScope rendering API
 * - Grasp the separation of concerns in rendering
 * 
 * Prerequisites: Basic C++ knowledge, understanding of ECScope ECS basics
 * 
 * @author ECScope Educational ECS Framework - Phase 7: Renderizado 2D
 * @date 2024
 */

#include <iostream>
#include <memory>

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
 * @brief Tutorial 1: Basic Sprite Rendering Class
 * 
 * This class demonstrates the minimum setup required to render sprites
 * using ECScope's 2D rendering system.
 */
class BasicSpriteRenderingTutorial {
public:
    /**
     * @brief Initialize the tutorial
     * 
     * Sets up the window, renderer, ECS registry, and creates basic entities.
     * This is the foundation that all rendering applications need.
     */
    bool initialize() {
        std::cout << "\n=== ECScope Tutorial 1: Basic Sprite Rendering ===\n";
        std::cout << "This tutorial demonstrates the fundamentals of 2D sprite rendering.\n\n";
        
        // Step 1: Initialize the graphics system
        std::cout << "Step 1: Initializing graphics system...\n";
        if (!initialize_graphics()) {
            std::cerr << "❌ Failed to initialize graphics system\n";
            return false;
        }
        std::cout << "✅ Graphics system initialized\n";
        
        // Step 2: Create ECS registry for entity management
        std::cout << "\nStep 2: Creating ECS registry...\n";
        registry_ = std::make_unique<ecs::Registry>();
        std::cout << "✅ ECS registry created\n";
        
        // Step 3: Initialize 2D renderer
        std::cout << "\nStep 3: Initializing 2D renderer...\n";
        if (!initialize_renderer()) {
            std::cerr << "❌ Failed to initialize 2D renderer\n";
            return false;
        }
        std::cout << "✅ 2D renderer initialized\n";
        
        // Step 4: Create camera entity
        std::cout << "\nStep 4: Creating camera entity...\n";
        create_camera();
        std::cout << "✅ Camera entity created\n";
        
        // Step 5: Create sprite entities
        std::cout << "\nStep 5: Creating sprite entities...\n";
        create_sprites();
        std::cout << "✅ Sprite entities created\n";
        
        std::cout << "\n🎉 Tutorial initialization complete! Press SPACE to continue...\n";
        return true;
    }
    
    /**
     * @brief Main tutorial execution loop
     * 
     * Demonstrates a basic game loop with input handling, updating, and rendering.
     * This is the pattern used in most real-time graphics applications.
     */
    void run() {
        std::cout << "\n=== Running Basic Sprite Rendering Tutorial ===\n";
        std::cout << "Controls: SPACE = Exit tutorial\n\n";
        
        bool running = true;
        u32 frame_count = 0;
        
        while (running) {
            // Handle input events
            running = handle_input();
            
            // Update simulation (empty for this basic tutorial)
            update();
            
            // Render the frame
            render();
            
            // Educational: Show progress every 60 frames (roughly 1 second at 60 FPS)
            frame_count++;
            if (frame_count % 60 == 0) {
                std::cout << "Frame " << frame_count << " rendered successfully\n";
            }
        }
        
        std::cout << "\n✅ Tutorial completed! Total frames rendered: " << frame_count << "\n";
    }
    
    /**
     * @brief Clean up resources
     */
    ~BasicSpriteRenderingTutorial() {
        std::cout << "🧹 Cleaning up tutorial resources...\n";
    }

private:
    // Core systems
    std::unique_ptr<renderer::Window> window_;
    std::unique_ptr<ecs::Registry> registry_;
    std::unique_ptr<renderer::Renderer2D> renderer_;
    
    // Scene entities
    ecs::EntityID camera_entity_{ecs::INVALID_ENTITY_ID};
    std::vector<ecs::EntityID> sprite_entities_;
    
    /**
     * @brief Initialize the graphics window and OpenGL context
     * 
     * Educational Note: Graphics applications need a window and rendering context.
     * SDL2 provides cross-platform window management and OpenGL context creation.
     */
    bool initialize_graphics() {
#ifdef ECSCOPE_HAS_GRAPHICS
        // Initialize SDL2 video subsystem
        if (SDL_Init(SDL_INIT_VIDEO) < 0) {
            std::cerr << "SDL2 initialization failed: " << SDL_GetError() << "\n";
            return false;
        }
        
        // Create window with OpenGL context
        window_ = std::make_unique<renderer::Window>();
        bool success = window_->create(800, 600, "ECScope Tutorial 1: Basic Sprite Rendering");
        
        if (!success) {
            std::cerr << "Window creation failed\n";
            return false;
        }
        
        return true;
#else
        std::cerr << "Graphics support not compiled. Please rebuild with ECSCOPE_ENABLE_GRAPHICS=ON\n";
        return false;
#endif
    }
    
    /**
     * @brief Initialize the 2D renderer with basic configuration
     * 
     * Educational Note: The renderer needs to be configured and initialized
     * before it can be used. We use a simple configuration for this tutorial.
     */
    bool initialize_renderer() {
        // Create a basic renderer configuration
        // Educational: Start with simple settings for learning
        renderer::Renderer2DConfig config;
        config.rendering.max_sprites_per_batch = 100;  // Small batch size for clarity
        config.rendering.enable_frustum_culling = false; // Disabled for simplicity
        config.debug.enable_debug_rendering = true;    // Enable debug features
        config.debug.collect_gpu_timings = false;      // Disabled for performance
        
        renderer_ = std::make_unique<renderer::Renderer2D>(config);
        
        // Initialize the renderer with OpenGL
        auto result = renderer_->initialize();
        if (!result.is_ok()) {
            std::cerr << "Renderer initialization failed: " << result.error() << "\n";
            return false;
        }
        
        return true;
    }
    
    /**
     * @brief Create a camera entity for viewing the scene
     * 
     * Educational Note: Cameras define what part of the world is visible.
     * In 2D graphics, cameras typically handle translation, zoom, and viewport.
     */
    void create_camera() {
        // Create camera entity using ECS
        camera_entity_ = registry_->create_entity();
        
        // Add Transform component for camera position
        auto& camera_transform = registry_->add_component<ecs::components::Transform>(camera_entity_);
        camera_transform.position = {0.0f, 0.0f, 0.0f}; // Center of the world
        
        // Add Camera2D component for rendering
        auto& camera = registry_->add_component<renderer::components::Camera2D>(camera_entity_);
        camera.position = {0.0f, 0.0f};        // World position
        camera.zoom = 1.0f;                    // No zoom (1:1 scale)
        camera.viewport_width = 800.0f;        // Match window width
        camera.viewport_height = 600.0f;       // Match window height
        
        std::cout << "   📷 Camera positioned at (0, 0) with 1.0x zoom\n";
    }
    
    /**
     * @brief Create sprite entities to render
     * 
     * Educational Note: Each sprite is an entity with Transform and RenderableSprite components.
     * This separation allows for flexible composition and easy modification.
     */
    void create_sprites() {
        // Create a few sprites at different positions
        const struct SpriteData {
            f32 x, y;
            f32 size;
            renderer::components::Color color;
            const char* description;
        } sprites[] = {
            {-200.0f, 100.0f, 64.0f, {1.0f, 0.0f, 0.0f, 1.0f}, "Red sprite (left)"},
            {0.0f, 0.0f, 80.0f, {0.0f, 1.0f, 0.0f, 1.0f}, "Green sprite (center)"},
            {200.0f, -100.0f, 48.0f, {0.0f, 0.0f, 1.0f, 1.0f}, "Blue sprite (right)"},
            {-100.0f, -150.0f, 32.0f, {1.0f, 1.0f, 0.0f, 1.0f}, "Yellow sprite (bottom-left)"},
            {100.0f, 150.0f, 56.0f, {1.0f, 0.0f, 1.0f, 1.0f}, "Magenta sprite (top-right)"}
        };
        
        for (const auto& sprite_data : sprites) {
            // Create entity
            auto entity = registry_->create_entity();
            
            // Add Transform component
            auto& transform = registry_->add_component<ecs::components::Transform>(entity);
            transform.position = {sprite_data.x, sprite_data.y, 0.0f};
            transform.scale = {sprite_data.size, sprite_data.size, 1.0f};
            transform.rotation = {0.0f, 0.0f, 0.0f}; // No rotation
            
            // Add RenderableSprite component
            auto& sprite = registry_->add_component<renderer::components::RenderableSprite>(entity);
            sprite.texture_id = renderer::INVALID_TEXTURE_ID; // Use default white texture
            sprite.color = sprite_data.color;
            sprite.z_order = 0.0f; // All sprites at same depth
            sprite.blend_mode = renderer::components::RenderableSprite::BlendMode::Alpha;
            
            sprite_entities_.push_back(entity);
            
            std::cout << "   🟦 Created " << sprite_data.description 
                      << " at (" << sprite_data.x << ", " << sprite_data.y << ")\n";
        }
        
        std::cout << "   📊 Total sprites created: " << sprite_entities_.size() << "\n";
    }
    
    /**
     * @brief Handle input events
     * 
     * Educational Note: Input handling is crucial for interactive applications.
     * We keep it simple for this tutorial - just exit on SPACE key.
     */
    bool handle_input() {
#ifdef ECSCOPE_HAS_GRAPHICS
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_QUIT:
                    std::cout << "🚪 Window close requested\n";
                    return false;
                    
                case SDL_KEYDOWN:
                    if (event.key.keysym.sym == SDLK_SPACE) {
                        std::cout << "⌨️ Space key pressed - exiting tutorial\n";
                        return false;
                    }
                    break;
            }
        }
        return true;
#else
        return false;
#endif
    }
    
    /**
     * @brief Update simulation state
     * 
     * Educational Note: The update phase is where game logic runs.
     * For this basic tutorial, we don't need to update anything.
     */
    void update() {
        // No updates needed for this static scene tutorial
        // In more complex applications, this would update:
        // - Entity positions and rotations
        // - Physics simulation
        // - Game logic
        // - Animation systems
    }
    
    /**
     * @brief Render the current frame
     * 
     * Educational Note: This demonstrates the standard rendering pipeline:
     * 1. Begin frame
     * 2. Set camera
     * 3. Submit render commands
     * 4. End frame (executes commands)
     * 5. Present to screen
     */
    void render() {
        // Step 1: Begin the rendering frame
        // This prepares the renderer for new commands
        renderer_->begin_frame();
        
        // Step 2: Set the active camera
        // The camera determines what part of the world is visible
        const auto& camera = registry_->get_component<renderer::components::Camera2D>(camera_entity_);
        renderer_->set_active_camera(camera);
        
        // Step 3: Render all entities with sprite components
        // The renderer automatically finds entities with Transform + RenderableSprite
        renderer_->render_entities(*registry_);
        
        // Step 4: End the frame
        // This processes all render commands and draws to the back buffer
        renderer_->end_frame();
        
        // Step 5: Present the frame to the screen
        // Swap buffers to show the rendered image
        if (window_) {
            window_->swap_buffers();
        }
    }
};

/**
 * @brief Tutorial entry point
 * 
 * Educational Note: This shows the typical structure of a graphics application:
 * 1. Initialize systems
 * 2. Run main loop
 * 3. Clean up (automatic with RAII)
 */
int main() {
    // Initialize logging for educational feedback
    core::Log::initialize(core::LogLevel::INFO);
    
    std::cout << R"(
    ╔══════════════════════════════════════════════════════════╗
    ║            ECScope 2D Rendering Tutorial 1              ║
    ║                 Basic Sprite Rendering                  ║
    ╠══════════════════════════════════════════════════════════╣
    ║  This tutorial teaches the fundamentals of 2D graphics  ║
    ║  programming using ECScope's rendering system.          ║
    ║                                                          ║
    ║  You will learn:                                         ║
    ║  • How to initialize a graphics window                   ║
    ║  • Setting up the 2D renderer                          ║
    ║  • Creating cameras for viewing                         ║
    ║  • Making entities with sprites                         ║
    ║  • Basic render loop implementation                     ║
    ╚══════════════════════════════════════════════════════════╝
    )";
    
    try {
        BasicSpriteRenderingTutorial tutorial;
        
        if (!tutorial.initialize()) {
            std::cerr << "\n❌ Tutorial initialization failed!\n";
            return -1;
        }
        
        tutorial.run();
        
        std::cout << "\n🎓 Congratulations! You've completed Tutorial 1.\n";
        std::cout << "Next: Try Tutorial 2 to learn about sprite animation and interaction.\n";
        
    } catch (const std::exception& e) {
        std::cerr << "\n💥 Tutorial crashed: " << e.what() << "\n";
        return -1;
    }
    
    return 0;
}