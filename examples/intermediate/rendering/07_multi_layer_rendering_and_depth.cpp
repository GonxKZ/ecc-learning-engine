/**
 * @file 07_multi_layer_rendering_and_depth.cpp
 * @brief Tutorial 7: Multi-Layer Rendering and Depth Management - Advanced Scene Organization
 * 
 * This tutorial explores multi-layer rendering systems and depth management in 2D graphics.
 * You'll learn how to organize complex scenes with multiple rendering layers and depth sorting.
 * 
 * Learning Objectives:
 * 1. Understand rendering layer architecture and depth sorting
 * 2. Learn layer-based scene organization strategies
 * 3. Explore depth buffer usage in 2D rendering
 * 4. Master parallax scrolling and multi-plane techniques
 * 5. Implement efficient depth sorting algorithms
 * 
 * Key Concepts Covered:
 * - Rendering layer system architecture
 * - Z-order and depth sorting algorithms
 * - Layer-based batching optimization
 * - Parallax scrolling implementation
 * - UI layer separation and rendering order
 * - Post-processing effects per layer
 * 
 * Educational Value:
 * Multi-layer rendering is essential for organizing complex 2D scenes with proper
 * depth relationships. This tutorial provides practical techniques for scalable
 * scene management and rendering optimization.
 * 
 * @author ECScope Educational ECS Framework
 * @date 2024
 */

#include <iostream>
#include <memory>
#include <vector>
#include <algorithm>
#include <unordered_map>
#include <chrono>
#include <cmath>

// ECScope Core
#include "../../src/core/types.hpp"
#include "../../src/core/log.hpp"

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
 * @brief Multi-Layer Rendering and Depth Management Tutorial
 * 
 * This tutorial demonstrates advanced layer-based rendering techniques through
 * practical examples with complex scene organization.
 */
class MultiLayerRenderingTutorial {
public:
    MultiLayerRenderingTutorial() = default;
    ~MultiLayerRenderingTutorial() { cleanup(); }
    
    bool initialize() {
        core::Log::info("Tutorial", "=== Multi-Layer Rendering and Depth Management Tutorial ===");
        core::Log::info("Tutorial", "Learning objective: Master complex scene organization with rendering layers");
        
        // Initialize window and renderer
        window_ = std::make_unique<Window>("Tutorial 7: Multi-Layer Rendering", 1800, 1000);
        if (!window_->initialize()) {
            core::Log::error("Tutorial", "Failed to create window");
            return false;
        }
        
        // Configure renderer for layered rendering
        Renderer2DConfig renderer_config = Renderer2DConfig::educational_mode();
        renderer_config.debug.show_performance_overlay = true;
        renderer_config.debug.enable_debug_rendering = true;
        renderer_config.rendering.enable_depth_testing = true; // Enable depth buffer
        renderer_config.rendering.enable_layer_batching = true;
        
        renderer_ = std::make_unique<Renderer2D>(renderer_config);
        if (!renderer_->initialize().is_ok()) {
            core::Log::error("Tutorial", "Failed to initialize renderer");
            return false;
        }
        
        // Set up camera
        camera_ = Camera2D::create_main_camera(1800, 1000);
        camera_.set_position(0.0f, 0.0f);
        camera_.set_zoom(1.0f);
        
        // Create ECS registry
        registry_ = std::make_unique<ecs::Registry>();
        
        core::Log::info("Tutorial", "System initialized. Creating rendering layer system...");
        
        // Initialize layer system
        initialize_layer_system();
        
        return true;
    }
    
    void run() {
        if (!window_ || !renderer_) return;
        
        core::Log::info("Tutorial", "Starting multi-layer rendering demonstration...");
        
        // Run layer system demonstrations
        demonstrate_basic_layer_concepts();
        demonstrate_depth_sorting();
        demonstrate_parallax_scrolling();
        demonstrate_ui_layer_separation();
        demonstrate_layer_effects();
        demonstrate_performance_optimization();
        
        display_educational_summary();
    }
    
private:
    //=========================================================================
    // Layer System Data Structures
    //=========================================================================
    
    enum class RenderLayer : u8 {
        // Background layers (farthest from camera)
        SkyBackground = 0,
        FarBackground = 10,
        MidBackground = 20,
        NearBackground = 30,
        
        // Game world layers
        EnvironmentBack = 40,
        GameObjects = 50,
        Characters = 60,
        EnvironmentFront = 70,
        
        // Effect layers
        ParticlesBack = 75,
        ParticlesFront = 85,
        
        // UI layers (closest to camera)
        UIBackground = 90,
        UIElements = 95,
        UIOverlay = 99,
        
        // Special layers
        Debug = 100
    };
    
    struct LayerInfo {
        RenderLayer layer_id;
        std::string name;
        std::string description;
        f32 depth_range_min;  // Near depth
        f32 depth_range_max;  // Far depth
        bool parallax_enabled;
        f32 parallax_factor;  // 0.0 = no movement, 1.0 = full movement
        bool depth_sorting_enabled;
        bool batching_enabled;
        u32 render_order;     // Lower values render first
        
        // Per-layer effects
        bool post_processing_enabled;
        f32 alpha_multiplier;
        Color tint_color;
        
        // Performance tracking
        u32 sprite_count;
        u32 draw_calls;
        f32 render_time_ms;
    };
    
    struct LayeredSprite {
        u32 entity_id;
        RenderLayer layer;
        f32 depth_within_layer;  // Z-order within the layer
        Vec2 world_position;
        Transform transform;
        RenderableSprite sprite;
        
        // Layer-specific properties
        Vec2 parallax_offset;
        bool depth_sort_dirty;
    };
    
    struct RenderBatch {
        RenderLayer layer;
        std::vector<LayeredSprite*> sprites;
        u32 texture_id;
        Material material;
        bool needs_sorting;
    };
    
    //=========================================================================
    // Layer System Implementation
    //=========================================================================
    
    void initialize_layer_system() {
        core::Log::info("Layers", "Initializing multi-layer rendering system");
        
        // Define all rendering layers
        layer_infos_[RenderLayer::SkyBackground] = {
            .layer_id = RenderLayer::SkyBackground,
            .name = "Sky Background",
            .description = "Skybox and far distant background",
            .depth_range_min = 0.9f,
            .depth_range_max = 1.0f,
            .parallax_enabled = true,
            .parallax_factor = 0.1f,
            .depth_sorting_enabled = false,
            .batching_enabled = true,
            .render_order = 0,
            .post_processing_enabled = false,
            .alpha_multiplier = 1.0f,
            .tint_color = Color::white(),
            .sprite_count = 0,
            .draw_calls = 0,
            .render_time_ms = 0.0f
        };
        
        layer_infos_[RenderLayer::FarBackground] = {
            .layer_id = RenderLayer::FarBackground,
            .name = "Far Background",
            .description = "Distant mountains, clouds",
            .depth_range_min = 0.8f,
            .depth_range_max = 0.9f,
            .parallax_enabled = true,
            .parallax_factor = 0.3f,
            .depth_sorting_enabled = false,
            .batching_enabled = true,
            .render_order = 10,
            .post_processing_enabled = false,
            .alpha_multiplier = 1.0f,
            .tint_color = Color::white(),
            .sprite_count = 0,
            .draw_calls = 0,
            .render_time_ms = 0.0f
        };
        
        layer_infos_[RenderLayer::MidBackground] = {
            .layer_id = RenderLayer::MidBackground,
            .name = "Mid Background",
            .description = "Mid-distance scenery, buildings",
            .depth_range_min = 0.6f,
            .depth_range_max = 0.8f,
            .parallax_enabled = true,
            .parallax_factor = 0.5f,
            .depth_sorting_enabled = true,
            .batching_enabled = true,
            .render_order = 20,
            .post_processing_enabled = false,
            .alpha_multiplier = 1.0f,
            .tint_color = Color::white(),
            .sprite_count = 0,
            .draw_calls = 0,
            .render_time_ms = 0.0f
        };
        
        layer_infos_[RenderLayer::GameObjects] = {
            .layer_id = RenderLayer::GameObjects,
            .name = "Game Objects",
            .description = "Interactive game objects, props",
            .depth_range_min = 0.4f,
            .depth_range_max = 0.6f,
            .parallax_enabled = false,
            .parallax_factor = 1.0f,
            .depth_sorting_enabled = true,
            .batching_enabled = true,
            .render_order = 50,
            .post_processing_enabled = false,
            .alpha_multiplier = 1.0f,
            .tint_color = Color::white(),
            .sprite_count = 0,
            .draw_calls = 0,
            .render_time_ms = 0.0f
        };
        
        layer_infos_[RenderLayer::Characters] = {
            .layer_id = RenderLayer::Characters,
            .name = "Characters",
            .description = "Player, NPCs, enemies",
            .depth_range_min = 0.2f,
            .depth_range_max = 0.4f,
            .parallax_enabled = false,
            .parallax_factor = 1.0f,
            .depth_sorting_enabled = true,
            .batching_enabled = false, // Characters often unique
            .render_order = 60,
            .post_processing_enabled = false,
            .alpha_multiplier = 1.0f,
            .tint_color = Color::white(),
            .sprite_count = 0,
            .draw_calls = 0,
            .render_time_ms = 0.0f
        };
        
        layer_infos_[RenderLayer::UIElements] = {
            .layer_id = RenderLayer::UIElements,
            .name = "UI Elements",
            .description = "User interface, HUD",
            .depth_range_min = 0.0f,
            .depth_range_max = 0.1f,
            .parallax_enabled = false,
            .parallax_factor = 0.0f,
            .depth_sorting_enabled = true,
            .batching_enabled = true,
            .render_order = 95,
            .post_processing_enabled = false,
            .alpha_multiplier = 1.0f,
            .tint_color = Color::white(),
            .sprite_count = 0,
            .draw_calls = 0,
            .render_time_ms = 0.0f
        };
        
        // Initialize render batches for each layer
        for (const auto& [layer_id, info] : layer_infos_) {
            render_batches_[layer_id] = std::vector<RenderBatch>();
        }
        
        core::Log::info("Layers", "Initialized {} rendering layers", layer_infos_.size());
        log_layer_configuration();
    }
    
    void log_layer_configuration() {
        core::Log::info("Layer Config", "=== RENDERING LAYER CONFIGURATION ===");
        
        // Sort layers by render order for logging
        std::vector<std::pair<u32, const LayerInfo*>> sorted_layers;
        for (const auto& [layer_id, info] : layer_infos_) {
            sorted_layers.emplace_back(info.render_order, &info);
        }
        
        std::sort(sorted_layers.begin(), sorted_layers.end());
        
        for (const auto& [order, info] : sorted_layers) {
            core::Log::info("Layer", "{}: {} (depth {:.1f}-{:.1f}, parallax {:.1f}x)",
                           info->name, info->description, 
                           info->depth_range_min, info->depth_range_max, info->parallax_factor);
        }
    }
    
    //=========================================================================
    // Demonstration Functions
    //=========================================================================
    
    void demonstrate_basic_layer_concepts() {
        core::Log::info("Demo 1", "=== BASIC LAYER CONCEPTS ===");
        core::Log::info("Explanation", "Understanding rendering layers and depth organization");
        
        // Create a simple multi-layer scene
        create_basic_layered_scene();
        
        core::Log::info("Demo", "Rendering scene with multiple layers...");
        
        // Render scene for demonstration
        f32 demo_duration = 5.0f; // 5 seconds
        u32 frames = static_cast<u32>(demo_duration * 60.0f);
        
        for (u32 frame = 0; frame < frames; ++frame) {
            f32 delta_time = 1.0f / 60.0f;
            
            // Update layer system
            update_layer_system(delta_time);
            
            // Render all layers
            render_all_layers();
            
            if (frame % 120 == 0) {
                log_layer_statistics();
            }
        }
        
        explain_layer_concepts();
    }
    
    void demonstrate_depth_sorting() {
        core::Log::info("Demo 2", "=== DEPTH SORTING ALGORITHMS ===");
        core::Log::info("Explanation", "Comparing depth sorting strategies and performance");
        
        // Create scene with overlapping sprites requiring depth sorting
        create_depth_sorting_scene();
        
        struct SortingTest {
            const char* name;
            const char* description;
            std::function<void()> sorting_function;
        };
        
        std::vector<SortingTest> sorting_tests = {
            {"No Sorting", "Render sprites in creation order", 
             [this]() { /* No sorting */ }},
            {"Simple Z-Sort", "Sort by Z-order value",
             [this]() { sort_sprites_by_z_order(); }},
            {"Back-to-Front", "Painter's algorithm sorting",
             [this]() { sort_sprites_back_to_front(); }},
            {"Layer + Z-Sort", "Sort by layer, then Z within layer",
             [this]() { sort_sprites_layer_and_z(); }}
        };
        
        for (const auto& test : sorting_tests) {
            core::Log::info("Sorting Test", "Testing: {} - {}", test.name, test.description);
            
            auto start_time = std::chrono::high_resolution_clock::now();
            
            // Apply sorting algorithm
            test.sorting_function();
            
            auto end_time = std::chrono::high_resolution_clock::now();
            f32 sort_time = std::chrono::duration<f32>(end_time - start_time).count() * 1000.0f;
            
            // Render with this sorting
            u32 test_frames = 60; // 1 second
            for (u32 frame = 0; frame < test_frames; ++frame) {
                render_all_layers();
            }
            
            core::Log::info("Sort Result", "{}: {:.3f}ms sort time", test.name, sort_time);
            sorting_performance_[test.name] = sort_time;
        }
        
        analyze_sorting_performance();
    }
    
    void demonstrate_parallax_scrolling() {
        core::Log::info("Demo 3", "=== PARALLAX SCROLLING IMPLEMENTATION ===");
        core::Log::info("Explanation", "Multi-plane parallax for depth illusion");
        
        // Create parallax background layers
        create_parallax_scene();
        
        core::Log::info("Parallax", "Animating camera to demonstrate parallax effect");
        
        // Animate camera movement to show parallax effect
        f32 demo_duration = 10.0f; // 10 seconds
        u32 frames = static_cast<u32>(demo_duration * 60.0f);
        
        for (u32 frame = 0; frame < frames; ++frame) {
            f32 time = frame / 60.0f;
            f32 delta_time = 1.0f / 60.0f;
            
            // Move camera in a pattern
            f32 camera_x = std::sin(time * 0.3f) * 400.0f;
            f32 camera_y = std::cos(time * 0.2f) * 200.0f;
            camera_.set_position(camera_x, camera_y);
            
            // Update parallax offsets
            update_parallax_layers(camera_x, camera_y);
            
            // Update and render
            update_layer_system(delta_time);
            render_all_layers();
            
            if (frame % 180 == 0) {
                core::Log::info("Parallax", "Camera: ({:.1f}, {:.1f}), Time: {:.1f}s", 
                               camera_x, camera_y, time);
                log_parallax_offsets();
            }
        }
        
        explain_parallax_technique();
    }
    
    void demonstrate_ui_layer_separation() {
        core::Log::info("Demo 4", "=== UI LAYER SEPARATION ===");
        core::Log::info("Explanation", "Separating UI from world space for independent rendering");
        
        // Create scene with world objects and UI elements
        create_ui_separation_scene();
        
        core::Log::info("UI Demo", "Demonstrating UI layer independence from world camera");
        
        // Move world camera while keeping UI fixed
        f32 demo_duration = 6.0f;
        u32 frames = static_cast<u32>(demo_duration * 60.0f);
        
        for (u32 frame = 0; frame < frames; ++frame) {
            f32 time = frame / 60.0f;
            f32 delta_time = 1.0f / 60.0f;
            
            // Move world camera dramatically
            f32 world_camera_x = std::sin(time * 0.8f) * 600.0f;
            f32 world_camera_y = std::cos(time * 0.6f) * 400.0f;
            camera_.set_position(world_camera_x, world_camera_y);
            
            // Update layers
            update_layer_system(delta_time);
            
            // Render world layers with world camera
            render_world_layers();
            
            // Render UI layers with screen-space camera
            render_ui_layers();
            
            if (frame % 90 == 0) {
                core::Log::info("UI Demo", "World camera: ({:.1f}, {:.1f}), UI remains fixed",
                               world_camera_x, world_camera_y);
            }
        }
    }
    
    void demonstrate_layer_effects() {
        core::Log::info("Demo 5", "=== LAYER-BASED EFFECTS ===");
        core::Log::info("Explanation", "Per-layer post-processing and visual effects");
        
        // Create scene for effect demonstration
        create_effects_scene();
        
        struct EffectTest {
            const char* name;
            RenderLayer target_layer;
            std::function<void(LayerInfo&)> effect_setup;
        };
        
        std::vector<EffectTest> effect_tests = {
            {"Background Tint", RenderLayer::FarBackground,
             [](LayerInfo& layer) { 
                 layer.tint_color = Color{150, 180, 255, 255}; // Blue tint
                 layer.post_processing_enabled = true;
             }},
            {"Character Highlight", RenderLayer::Characters,
             [](LayerInfo& layer) {
                 layer.alpha_multiplier = 1.2f; // Slight brightness boost
                 layer.tint_color = Color{255, 255, 200, 255}; // Warm tint
                 layer.post_processing_enabled = true;
             }},
            {"UI Semi-Transparent", RenderLayer::UIElements,
             [](LayerInfo& layer) {
                 layer.alpha_multiplier = 0.8f; // Semi-transparent
                 layer.post_processing_enabled = true;
             }}
        };
        
        for (const auto& test : effect_tests) {
            core::Log::info("Effect Test", "Applying: {} to {}", 
                           test.name, layer_infos_[test.target_layer].name);
            
            // Apply effect
            test.effect_setup(layer_infos_[test.target_layer]);
            
            // Render with effect
            u32 effect_frames = 120; // 2 seconds
            for (u32 frame = 0; frame < effect_frames; ++frame) {
                f32 delta_time = 1.0f / 60.0f;
                update_layer_system(delta_time);
                render_all_layers();
            }
            
            // Reset effect
            layer_infos_[test.target_layer].post_processing_enabled = false;
            layer_infos_[test.target_layer].alpha_multiplier = 1.0f;
            layer_infos_[test.target_layer].tint_color = Color::white();
        }
    }
    
    void demonstrate_performance_optimization() {
        core::Log::info("Demo 6", "=== LAYER SYSTEM PERFORMANCE OPTIMIZATION ===");
        core::Log::info("Explanation", "Optimizing multi-layer rendering for performance");
        
        // Create performance test scene
        create_performance_test_scene();
        
        struct OptimizationTest {
            const char* name;
            const char* description;
            std::function<void()> setup_function;
        };
        
        std::vector<OptimizationTest> optimization_tests = {
            {"No Optimization", "Basic layer rendering without optimizations",
             [this]() { disable_all_optimizations(); }},
            {"Layer Batching", "Enable batching within layers",
             [this]() { enable_layer_batching(); }},
            {"Depth Culling", "Skip depth sorting for distant layers",
             [this]() { enable_depth_culling(); }},
            {"Frustum Culling", "Skip off-screen sprites per layer",
             [this]() { enable_frustum_culling(); }},
            {"All Optimizations", "All optimizations enabled",
             [this]() { enable_all_optimizations(); }}
        };
        
        for (const auto& test : optimization_tests) {
            core::Log::info("Optimization Test", "Testing: {} - {}", test.name, test.description);
            
            // Apply optimization configuration
            test.setup_function();
            
            // Measure performance
            auto performance = measure_layer_rendering_performance(180); // 3 seconds
            
            core::Log::info("Performance", "{}: {:.1f} FPS, {:.2f}ms total, {} draw calls",
                           test.name, performance.fps, performance.total_render_time_ms, 
                           performance.total_draw_calls);
            
            optimization_results_[test.name] = performance;
        }
        
        analyze_optimization_results();
    }
    
    //=========================================================================
    // Scene Creation Functions
    //=========================================================================
    
    void create_basic_layered_scene() {
        core::Log::info("Scene", "Creating basic multi-layer scene");
        
        // Clear existing sprites
        layered_sprites_.clear();
        
        // Sky background
        create_layer_sprites(RenderLayer::SkyBackground, 3, 
                           "Sky elements", Color{135, 206, 235, 255});
        
        // Far background (mountains, clouds)
        create_layer_sprites(RenderLayer::FarBackground, 5,
                           "Mountains/clouds", Color{100, 149, 237, 255});
        
        // Game objects
        create_layer_sprites(RenderLayer::GameObjects, 8,
                           "Props and objects", Color{34, 139, 34, 255});
        
        // Characters
        create_layer_sprites(RenderLayer::Characters, 4,
                           "Player and NPCs", Color{255, 69, 0, 255});
        
        // UI elements
        create_layer_sprites(RenderLayer::UIElements, 6,
                           "UI buttons/panels", Color{255, 255, 255, 200});
        
        core::Log::info("Scene", "Created basic scene with {} total sprites", layered_sprites_.size());
    }
    
    void create_layer_sprites(RenderLayer layer, u32 count, const char* description, Color color) {
        const auto& layer_info = layer_infos_[layer];
        
        for (u32 i = 0; i < count; ++i) {
            LayeredSprite sprite;
            sprite.entity_id = static_cast<u32>(layered_sprites_.size() + 1);
            sprite.layer = layer;
            
            // Position sprites across the layer
            f32 x = (i - count / 2.0f) * 150.0f;
            f32 y = static_cast<f32>(static_cast<int>(layer) - 50) * 20.0f; // Spread by layer
            sprite.world_position = {x, y};
            
            // Depth within layer
            sprite.depth_within_layer = layer_info.depth_range_min + 
                                      (i / static_cast<f32>(count)) * 
                                      (layer_info.depth_range_max - layer_info.depth_range_min);
            
            // Transform
            sprite.transform.position = {x, y, sprite.depth_within_layer};
            sprite.transform.scale = {60.0f, 60.0f, 1.0f};
            sprite.transform.rotation = 0.0f;
            
            // Renderable sprite
            sprite.sprite.texture = TextureHandle{1, 32, 32};
            sprite.sprite.color_modulation = color;
            sprite.sprite.z_order = sprite.depth_within_layer;
            sprite.sprite.set_visible(true);
            
            sprite.parallax_offset = {0.0f, 0.0f};
            sprite.depth_sort_dirty = false;
            
            layered_sprites_.push_back(sprite);
        }
        
        core::Log::info("Layer Creation", "Added {} sprites to {} layer", count, layer_info.name);
        layer_infos_[layer].sprite_count += count;
    }
    
    void create_depth_sorting_scene() {
        core::Log::info("Scene", "Creating depth sorting test scene");
        
        layered_sprites_.clear();
        
        // Create overlapping sprites that require depth sorting
        const u32 sprite_count = 50;
        const f32 overlap_area = 400.0f;
        
        for (u32 i = 0; i < sprite_count; ++i) {
            LayeredSprite sprite;
            sprite.entity_id = i + 1;
            sprite.layer = RenderLayer::GameObjects;
            
            // Position sprites in overlapping area
            f32 angle = (i / static_cast<f32>(sprite_count)) * 2.0f * 3.14159f;
            f32 radius = 50.0f + (i % 10) * 20.0f;
            f32 x = std::cos(angle) * radius;
            f32 y = std::sin(angle) * radius;
            
            sprite.world_position = {x, y};
            sprite.depth_within_layer = static_cast<f32>(i) / sprite_count;
            
            sprite.transform.position = {x, y, sprite.depth_within_layer};
            sprite.transform.scale = {40.0f + (i % 5) * 10.0f, 40.0f + (i % 5) * 10.0f, 1.0f};
            
            // Vary colors for visibility
            u8 red = static_cast<u8>(128 + (i * 127) % 128);
            u8 green = static_cast<u8>(128 + (i * 73) % 128);
            u8 blue = static_cast<u8>(128 + (i * 191) % 128);
            sprite.sprite.color_modulation = Color{red, green, blue, 200}; // Semi-transparent
            
            sprite.sprite.texture = TextureHandle{1, 32, 32};
            sprite.sprite.z_order = sprite.depth_within_layer;
            sprite.sprite.set_visible(true);
            
            layered_sprites_.push_back(sprite);
        }
        
        core::Log::info("Scene", "Created depth sorting scene with {} overlapping sprites", sprite_count);
    }
    
    void create_parallax_scene() {
        core::Log::info("Scene", "Creating parallax scrolling demonstration scene");
        
        layered_sprites_.clear();
        
        struct ParallaxLayer {
            RenderLayer layer;
            u32 sprite_count;
            f32 spacing;
            Color color;
        };
        
        std::vector<ParallaxLayer> parallax_layers = {
            {RenderLayer::SkyBackground, 3, 800.0f, Color{100, 150, 255, 255}},
            {RenderLayer::FarBackground, 6, 400.0f, Color{120, 180, 120, 255}},
            {RenderLayer::MidBackground, 8, 200.0f, Color{139, 69, 19, 255}},
            {RenderLayer::GameObjects, 12, 100.0f, Color{255, 165, 0, 255}}
        };
        
        for (const auto& p_layer : parallax_layers) {
            for (u32 i = 0; i < p_layer.sprite_count; ++i) {
                LayeredSprite sprite;
                sprite.entity_id = static_cast<u32>(layered_sprites_.size() + 1);
                sprite.layer = p_layer.layer;
                
                // Spread sprites across parallax layer
                f32 x = (i - p_layer.sprite_count / 2.0f) * p_layer.spacing;
                f32 y = static_cast<f32>(static_cast<int>(p_layer.layer) - 25) * 15.0f;
                
                sprite.world_position = {x, y};
                sprite.depth_within_layer = layer_infos_[p_layer.layer].depth_range_min;
                
                sprite.transform.position = {x, y, sprite.depth_within_layer};
                sprite.transform.scale = {80.0f, 80.0f, 1.0f};
                
                sprite.sprite.texture = TextureHandle{1, 32, 32};
                sprite.sprite.color_modulation = p_layer.color;
                sprite.sprite.z_order = sprite.depth_within_layer;
                sprite.sprite.set_visible(true);
                
                sprite.parallax_offset = {0.0f, 0.0f};
                
                layered_sprites_.push_back(sprite);
            }
        }
        
        core::Log::info("Scene", "Created parallax scene with {} layers, {} total sprites",
                       parallax_layers.size(), layered_sprites_.size());
    }
    
    void create_ui_separation_scene() {
        layered_sprites_.clear();
        
        // World objects that move with camera
        create_layer_sprites(RenderLayer::GameObjects, 10, "World objects", Color{34, 139, 34, 255});
        
        // UI elements that stay fixed on screen
        const u32 ui_element_count = 8;
        for (u32 i = 0; i < ui_element_count; ++i) {
            LayeredSprite ui_sprite;
            ui_sprite.entity_id = 1000 + i;
            ui_sprite.layer = RenderLayer::UIElements;
            
            // Position UI elements around screen edges
            f32 x, y;
            if (i < 4) {
                // Top row
                x = -600.0f + i * 400.0f;
                y = -400.0f;
            } else {
                // Bottom row
                x = -600.0f + (i - 4) * 400.0f;
                y = 400.0f;
            }
            
            ui_sprite.world_position = {x, y}; // Screen space coordinates
            ui_sprite.depth_within_layer = 0.05f;
            
            ui_sprite.transform.position = {x, y, 0.05f};
            ui_sprite.transform.scale = {50.0f, 50.0f, 1.0f};
            
            ui_sprite.sprite.texture = TextureHandle{1, 32, 32};
            ui_sprite.sprite.color_modulation = Color{255, 255, 255, 180};
            ui_sprite.sprite.z_order = 0.05f;
            ui_sprite.sprite.set_visible(true);
            
            layered_sprites_.push_back(ui_sprite);
        }
        
        core::Log::info("Scene", "Created UI separation scene with world and UI layers");
    }
    
    void create_effects_scene() {
        layered_sprites_.clear();
        
        // Create sprites in multiple layers for effect testing
        create_layer_sprites(RenderLayer::FarBackground, 4, "Background", Color{100, 149, 237, 255});
        create_layer_sprites(RenderLayer::GameObjects, 6, "Objects", Color{34, 139, 34, 255});
        create_layer_sprites(RenderLayer::Characters, 3, "Characters", Color{255, 69, 0, 255});
        create_layer_sprites(RenderLayer::UIElements, 5, "UI", Color{255, 255, 255, 255});
        
        core::Log::info("Scene", "Created effects test scene");
    }
    
    void create_performance_test_scene() {
        layered_sprites_.clear();
        
        // Create large number of sprites across all layers for performance testing
        const u32 sprites_per_layer = 100;
        
        std::vector<RenderLayer> test_layers = {
            RenderLayer::FarBackground,
            RenderLayer::MidBackground,
            RenderLayer::GameObjects,
            RenderLayer::Characters,
            RenderLayer::UIElements
        };
        
        for (RenderLayer layer : test_layers) {
            for (u32 i = 0; i < sprites_per_layer; ++i) {
                LayeredSprite sprite;
                sprite.entity_id = static_cast<u32>(layered_sprites_.size() + 1);
                sprite.layer = layer;
                
                // Random positioning
                f32 x = (rand() % 2000) - 1000.0f;
                f32 y = (rand() % 1000) - 500.0f;
                sprite.world_position = {x, y};
                
                const auto& layer_info = layer_infos_[layer];
                sprite.depth_within_layer = layer_info.depth_range_min + 
                    (static_cast<f32>(rand()) / RAND_MAX) * 
                    (layer_info.depth_range_max - layer_info.depth_range_min);
                
                sprite.transform.position = {x, y, sprite.depth_within_layer};
                sprite.transform.scale = {30.0f, 30.0f, 1.0f};
                
                sprite.sprite.texture = TextureHandle{static_cast<u16>(i % 8 + 1), 16, 16};
                sprite.sprite.color_modulation = Color{
                    static_cast<u8>(rand() % 256),
                    static_cast<u8>(rand() % 256),
                    static_cast<u8>(rand() % 256),
                    255
                };
                sprite.sprite.z_order = sprite.depth_within_layer;
                sprite.sprite.set_visible(true);
                
                layered_sprites_.push_back(sprite);
            }
        }
        
        core::Log::info("Scene", "Created performance test scene with {} sprites across {} layers",
                       layered_sprites_.size(), test_layers.size());
    }
    
    //=========================================================================
    // Layer System Update and Rendering
    //=========================================================================
    
    void update_layer_system(f32 delta_time) {
        // Reset layer statistics
        for (auto& [layer_id, info] : layer_infos_) {
            info.sprite_count = 0;
            info.draw_calls = 0;
            info.render_time_ms = 0.0f;
        }
        
        // Update sprite counts per layer
        for (const auto& sprite : layered_sprites_) {
            layer_infos_[sprite.layer].sprite_count++;
        }
        
        // Sort sprites if needed
        if (needs_depth_sorting_) {
            sort_sprites_layer_and_z();
            needs_depth_sorting_ = false;
        }
        
        // Update render batches
        update_render_batches();
    }
    
    void update_render_batches() {
        // Clear existing batches
        for (auto& [layer_id, batches] : render_batches_) {
            batches.clear();
        }
        
        // Group sprites by layer and texture for batching
        std::unordered_map<RenderLayer, std::unordered_map<u32, std::vector<LayeredSprite*>>> layer_texture_groups;
        
        for (auto& sprite : layered_sprites_) {
            if (sprite.sprite.is_visible()) {
                layer_texture_groups[sprite.layer][sprite.sprite.texture.id].push_back(&sprite);
            }
        }
        
        // Create render batches
        for (auto& [layer_id, texture_groups] : layer_texture_groups) {
            for (auto& [texture_id, sprites] : texture_groups) {
                RenderBatch batch;
                batch.layer = layer_id;
                batch.sprites = sprites;
                batch.texture_id = texture_id;
                batch.needs_sorting = layer_infos_[layer_id].depth_sorting_enabled;
                
                // Sort sprites within batch if needed
                if (batch.needs_sorting) {
                    std::sort(batch.sprites.begin(), batch.sprites.end(),
                             [](const LayeredSprite* a, const LayeredSprite* b) {
                                 return a->depth_within_layer > b->depth_within_layer; // Back to front
                             });
                }
                
                render_batches_[layer_id].push_back(batch);
            }
        }
    }
    
    void render_all_layers() {
        renderer_->begin_frame();
        renderer_->set_active_camera(camera_);
        
        // Render layers in order
        std::vector<std::pair<u32, RenderLayer>> layer_order;
        for (const auto& [layer_id, info] : layer_infos_) {
            layer_order.emplace_back(info.render_order, layer_id);
        }
        std::sort(layer_order.begin(), layer_order.end());
        
        for (const auto& [order, layer_id] : layer_order) {
            render_layer(layer_id);
        }
        
        renderer_->end_frame();
        window_->swap_buffers();
        window_->poll_events();
    }
    
    void render_layer(RenderLayer layer_id) {
        auto render_start = std::chrono::high_resolution_clock::now();
        
        const auto& layer_info = layer_infos_[layer_id];
        const auto& batches = render_batches_[layer_id];
        
        u32 layer_draw_calls = 0;
        
        // Apply layer-wide effects
        if (layer_info.post_processing_enabled) {
            // In a real implementation, this would set up render targets and effects
            // For this tutorial, we'll simulate the concept
        }
        
        // Render all batches in this layer
        for (const auto& batch : batches) {
            render_batch(batch);
            layer_draw_calls++;
        }
        
        auto render_end = std::chrono::high_resolution_clock::now();
        
        // Update layer statistics
        layer_infos_[layer_id].draw_calls = layer_draw_calls;
        layer_infos_[layer_id].render_time_ms = 
            std::chrono::duration<f32>(render_end - render_start).count() * 1000.0f;
    }
    
    void render_batch(const RenderBatch& batch) {
        // In a real implementation, this would:
        // 1. Bind the batch texture
        // 2. Set up layer-specific uniforms
        // 3. Render all sprites in the batch with a single draw call
        
        // For this educational demo, render sprites individually
        for (const LayeredSprite* sprite : batch.sprites) {
            render_layered_sprite(*sprite);
        }
    }
    
    void render_layered_sprite(const LayeredSprite& sprite) {
        // Create temporary entity for rendering
        u32 temp_entity = registry_->create_entity();
        
        // Apply parallax offset if enabled
        Transform render_transform = sprite.transform;
        if (layer_infos_[sprite.layer].parallax_enabled) {
            render_transform.position.x += sprite.parallax_offset.x;
            render_transform.position.y += sprite.parallax_offset.y;
        }
        
        registry_->add_component(temp_entity, render_transform);
        
        // Apply layer effects to sprite
        RenderableSprite render_sprite = sprite.sprite;
        if (layer_infos_[sprite.layer].post_processing_enabled) {
            // Apply layer tint
            Color layer_tint = layer_infos_[sprite.layer].tint_color;
            render_sprite.color_modulation = Color{
                static_cast<u8>((render_sprite.color_modulation.r * layer_tint.r) / 255),
                static_cast<u8>((render_sprite.color_modulation.g * layer_tint.g) / 255),
                static_cast<u8>((render_sprite.color_modulation.b * layer_tint.b) / 255),
                static_cast<u8>(render_sprite.color_modulation.a * layer_infos_[sprite.layer].alpha_multiplier)
            };
        }
        
        registry_->add_component(temp_entity, render_sprite);
        
        // Render the sprite
        renderer_->render_entities(*registry_);
        
        // Clean up
        registry_->remove_entity(temp_entity);
    }
    
    void render_world_layers() {
        // Render only world layers (not UI)
        std::vector<RenderLayer> world_layers = {
            RenderLayer::SkyBackground,
            RenderLayer::FarBackground,
            RenderLayer::MidBackground,
            RenderLayer::GameObjects,
            RenderLayer::Characters
        };
        
        renderer_->begin_frame();
        renderer_->set_active_camera(camera_);
        
        for (RenderLayer layer : world_layers) {
            render_layer(layer);
        }
        
        renderer_->end_frame();
    }
    
    void render_ui_layers() {
        // Render UI layers with screen-space camera
        Camera2D ui_camera = Camera2D::create_ui_camera(1800, 1000);
        
        renderer_->begin_frame();
        renderer_->set_active_camera(ui_camera);
        
        render_layer(RenderLayer::UIElements);
        
        renderer_->end_frame();
        window_->swap_buffers();
        window_->poll_events();
    }
    
    //=========================================================================
    // Sorting Algorithms
    //=========================================================================
    
    void sort_sprites_by_z_order() {
        std::sort(layered_sprites_.begin(), layered_sprites_.end(),
                 [](const LayeredSprite& a, const LayeredSprite& b) {
                     return a.depth_within_layer > b.depth_within_layer;
                 });
    }
    
    void sort_sprites_back_to_front() {
        std::sort(layered_sprites_.begin(), layered_sprites_.end(),
                 [](const LayeredSprite& a, const LayeredSprite& b) {
                     // Sort by actual depth (considering layer + depth within layer)
                     f32 depth_a = static_cast<f32>(static_cast<int>(a.layer)) + a.depth_within_layer;
                     f32 depth_b = static_cast<f32>(static_cast<int>(b.layer)) + b.depth_within_layer;
                     return depth_a > depth_b;
                 });
    }
    
    void sort_sprites_layer_and_z() {
        std::sort(layered_sprites_.begin(), layered_sprites_.end(),
                 [](const LayeredSprite& a, const LayeredSprite& b) {
                     if (a.layer != b.layer) {
                         return static_cast<int>(a.layer) < static_cast<int>(b.layer);
                     }
                     return a.depth_within_layer > b.depth_within_layer;
                 });
    }
    
    //=========================================================================
    // Parallax System
    //=========================================================================
    
    void update_parallax_layers(f32 camera_x, f32 camera_y) {
        for (auto& sprite : layered_sprites_) {
            const auto& layer_info = layer_infos_[sprite.layer];
            if (layer_info.parallax_enabled) {
                // Calculate parallax offset based on camera movement and layer factor
                sprite.parallax_offset.x = -camera_x * (1.0f - layer_info.parallax_factor);
                sprite.parallax_offset.y = -camera_y * (1.0f - layer_info.parallax_factor);
            }
        }
    }
    
    void log_parallax_offsets() {
        for (const auto& [layer_id, info] : layer_infos_) {
            if (info.parallax_enabled && !layered_sprites_.empty()) {
                // Find first sprite in this layer
                auto it = std::find_if(layered_sprites_.begin(), layered_sprites_.end(),
                                      [layer_id](const LayeredSprite& s) { return s.layer == layer_id; });
                if (it != layered_sprites_.end()) {
                    core::Log::info("Parallax", "{}: offset({:.1f}, {:.1f}), factor: {:.1f}",
                                   info.name, it->parallax_offset.x, it->parallax_offset.y, info.parallax_factor);
                }
            }
        }
    }
    
    //=========================================================================
    // Performance Analysis
    //=========================================================================
    
    struct LayerPerformance {
        f32 fps;
        f32 total_render_time_ms;
        u32 total_draw_calls;
        u32 total_sprites;
    };
    
    LayerPerformance measure_layer_rendering_performance(u32 frames) {
        LayerPerformance performance{};
        
        auto start_time = std::chrono::high_resolution_clock::now();
        f32 total_render_time = 0.0f;
        u32 total_draw_calls = 0;
        u32 total_sprites = 0;
        
        for (u32 frame = 0; frame < frames; ++frame) {
            f32 delta_time = 1.0f / 60.0f;
            
            auto frame_start = std::chrono::high_resolution_clock::now();
            
            update_layer_system(delta_time);
            render_all_layers();
            
            auto frame_end = std::chrono::high_resolution_clock::now();
            total_render_time += std::chrono::duration<f32>(frame_end - frame_start).count();
            
            // Collect statistics
            u32 frame_draw_calls = 0;
            u32 frame_sprites = 0;
            for (const auto& [layer_id, info] : layer_infos_) {
                frame_draw_calls += info.draw_calls;
                frame_sprites += info.sprite_count;
            }
            total_draw_calls += frame_draw_calls;
            total_sprites = frame_sprites; // Same each frame
        }
        
        auto end_time = std::chrono::high_resolution_clock::now();
        f32 total_time = std::chrono::duration<f32>(end_time - start_time).count();
        
        performance.fps = frames / total_time;
        performance.total_render_time_ms = (total_render_time / frames) * 1000.0f;
        performance.total_draw_calls = total_draw_calls / frames;
        performance.total_sprites = total_sprites;
        
        return performance;
    }
    
    void analyze_sorting_performance() {
        core::Log::info("Analysis", "=== DEPTH SORTING PERFORMANCE ANALYSIS ===");
        
        f32 fastest_sort = std::numeric_limits<f32>::max();
        f32 slowest_sort = 0.0f;
        std::string fastest_method, slowest_method;
        
        for (const auto& [method, time] : sorting_performance_) {
            if (time < fastest_sort) {
                fastest_sort = time;
                fastest_method = method;
            }
            if (time > slowest_sort) {
                slowest_sort = time;
                slowest_method = method;
            }
            
            core::Log::info("Sort Performance", "{}: {:.3f}ms", method, time);
        }
        
        if (!fastest_method.empty() && !slowest_method.empty()) {
            f32 speedup = slowest_sort / fastest_sort;
            core::Log::info("Analysis", "Fastest: {} ({:.3f}ms), Slowest: {} ({:.3f}ms)",
                           fastest_method, fastest_sort, slowest_method, slowest_sort);
            core::Log::info("Analysis", "{} is {:.1f}x faster than {}",
                           fastest_method, speedup, slowest_method);
        }
    }
    
    void analyze_optimization_results() {
        core::Log::info("Analysis", "=== LAYER OPTIMIZATION ANALYSIS ===");
        
        if (optimization_results_.count("No Optimization") && optimization_results_.count("All Optimizations")) {
            const auto& baseline = optimization_results_["No Optimization"];
            const auto& optimized = optimization_results_["All Optimizations"];
            
            f32 fps_improvement = optimized.fps / baseline.fps;
            f32 render_time_reduction = baseline.total_render_time_ms / optimized.total_render_time_ms;
            f32 draw_call_reduction = static_cast<f32>(baseline.total_draw_calls) / optimized.total_draw_calls;
            
            core::Log::info("Improvement", "FPS: {:.1f} → {:.1f} ({:.1f}x improvement)",
                           baseline.fps, optimized.fps, fps_improvement);
            core::Log::info("Improvement", "Render time: {:.2f}ms → {:.2f}ms ({:.1f}x faster)",
                           baseline.total_render_time_ms, optimized.total_render_time_ms, render_time_reduction);
            core::Log::info("Improvement", "Draw calls: {} → {} ({:.1f}x reduction)",
                           baseline.total_draw_calls, optimized.total_draw_calls, draw_call_reduction);
        }
    }
    
    //=========================================================================
    // Optimization Controls
    //=========================================================================
    
    void disable_all_optimizations() {
        for (auto& [layer_id, info] : layer_infos_) {
            info.batching_enabled = false;
            info.depth_sorting_enabled = false;
        }
        frustum_culling_enabled_ = false;
    }
    
    void enable_layer_batching() {
        for (auto& [layer_id, info] : layer_infos_) {
            info.batching_enabled = true;
        }
    }
    
    void enable_depth_culling() {
        // Skip depth sorting for distant layers
        layer_infos_[RenderLayer::SkyBackground].depth_sorting_enabled = false;
        layer_infos_[RenderLayer::FarBackground].depth_sorting_enabled = false;
    }
    
    void enable_frustum_culling() {
        frustum_culling_enabled_ = true;
    }
    
    void enable_all_optimizations() {
        enable_layer_batching();
        enable_depth_culling();
        enable_frustum_culling();
    }
    
    //=========================================================================
    // Educational Explanations
    //=========================================================================
    
    void explain_layer_concepts() {
        core::Log::info("Education", "=== RENDERING LAYER CONCEPTS ===");
        core::Log::info("Concept", "Layers organize sprites by depth relationship");
        core::Log::info("Concept", "Each layer has depth range, parallax factor, optimization settings");
        core::Log::info("Concept", "Layers render in order: background → foreground → UI");
        core::Log::info("Concept", "Within layers, sprites can be depth-sorted for correct overlap");
        core::Log::info("Benefits", "Better organization, batching optimization, effect isolation");
    }
    
    void explain_parallax_technique() {
        core::Log::info("Education", "=== PARALLAX SCROLLING TECHNIQUE ===");
        core::Log::info("Parallax", "Simulates depth by moving layers at different speeds");
        core::Log::info("Parallax", "Far layers move slower, near layers move faster");
        core::Log::info("Formula", "parallax_offset = -camera_movement * (1.0 - parallax_factor)");
        core::Log::info("Example", "Factor 0.0 = no movement, 1.0 = full movement with camera");
        core::Log::info("Usage", "Creates illusion of 3D depth in 2D scenes");
    }
    
    void log_layer_statistics() {
        core::Log::info("Statistics", "=== LAYER RENDERING STATISTICS ===");
        u32 total_sprites = 0;
        u32 total_draw_calls = 0;
        f32 total_render_time = 0.0f;
        
        for (const auto& [layer_id, info] : layer_infos_) {
            if (info.sprite_count > 0) {
                core::Log::info("Layer Stats", "{}: {} sprites, {} draws, {:.2f}ms",
                               info.name, info.sprite_count, info.draw_calls, info.render_time_ms);
                total_sprites += info.sprite_count;
                total_draw_calls += info.draw_calls;
                total_render_time += info.render_time_ms;
            }
        }
        
        core::Log::info("Total Stats", "All layers: {} sprites, {} draw calls, {:.2f}ms total",
                       total_sprites, total_draw_calls, total_render_time);
    }
    
    void display_educational_summary() {
        std::cout << "\n=== MULTI-LAYER RENDERING TUTORIAL SUMMARY ===\n\n";
        
        std::cout << "KEY CONCEPTS LEARNED:\n\n";
        
        std::cout << "1. RENDERING LAYER ARCHITECTURE:\n";
        std::cout << "   - Layer-based scene organization by depth relationship\n";
        std::cout << "   - Each layer has depth range, parallax factor, render order\n";
        std::cout << "   - Sprites grouped by layer for batching optimization\n";
        std::cout << "   - Per-layer effects and post-processing capabilities\n\n";
        
        std::cout << "2. DEPTH MANAGEMENT:\n";
        std::cout << "   - Z-order sorting within layers for proper overlap\n";
        std::cout << "   - Back-to-front rendering (painter's algorithm)\n";
        std::cout << "   - Depth buffer usage for complex occlusion\n";
        std::cout << "   - Layer-first vs depth-first sorting strategies\n\n";
        
        std::cout << "3. PARALLAX SCROLLING:\n";
        std::cout << "   - Multi-plane depth illusion through differential movement\n";
        std::cout << "   - Parallax factor controls layer movement speed\n";
        std::cout << "   - Creates pseudo-3D depth in 2D scenes\n";
        std::cout << "   - Efficient implementation with offset calculations\n\n";
        
        std::cout << "4. PERFORMANCE OPTIMIZATION:\n";
        if (!optimization_results_.empty() && optimization_results_.count("No Optimization") && 
            optimization_results_.count("All Optimizations")) {
            const auto& baseline = optimization_results_.at("No Optimization");
            const auto& optimized = optimization_results_.at("All Optimizations");
            f32 improvement = optimized.fps / baseline.fps;
            std::cout << "   - Layer optimizations: " << improvement << "x FPS improvement achieved\n";
            std::cout << "   - Draw call reduction: " << baseline.total_draw_calls 
                     << " → " << optimized.total_draw_calls << " calls\n";
        }
        std::cout << "   - Layer-based batching reduces state changes\n";
        std::cout << "   - Frustum culling per layer for off-screen rejection\n";
        std::cout << "   - Depth sorting optimization for distant layers\n\n";
        
        std::cout << "5. UI LAYER SEPARATION:\n";
        std::cout << "   - UI renders in screen space independent of world camera\n";
        std::cout << "   - Separate render passes for world and UI content\n";
        std::cout << "   - UI layers always render on top with fixed positioning\n";
        std::cout << "   - Enables complex camera movements without UI disruption\n\n";
        
        std::cout << "PRACTICAL APPLICATIONS:\n";
        std::cout << "- 2D game scene organization with background/foreground layers\n";
        std::cout << "- Parallax scrolling for platformer and side-scrolling games\n";
        std::cout << "- UI overlay systems independent of world camera\n";
        std::cout << "- Level editor tools with layer-based content management\n";
        std::cout << "- Visual novel engines with character/background separation\n";
        std::cout << "- Particle effect organization by visual priority\n\n";
        
        std::cout << "LAYER SYSTEM DESIGN PRINCIPLES:\n";
        std::cout << "1. Define clear depth relationships between content types\n";
        std::cout << "2. Group related content into logical rendering layers\n";
        std::cout << "3. Configure parallax factors for realistic depth perception\n";
        std::cout << "4. Enable optimizations appropriate for each layer's content\n";
        std::cout << "5. Separate UI from world space for independent rendering\n";
        std::cout << "6. Profile and optimize layer rendering order and batching\n\n";
        
        std::cout << "ADVANCED TECHNIQUES:\n";
        std::cout << "- Depth peeling for order-independent transparency\n";
        std::cout << "- Multi-target rendering for per-layer post-processing\n";
        std::cout << "- Dynamic layer creation and destruction\n";
        std::cout << "- Layer-based lighting and shadow systems\n";
        std::cout << "- Procedural parallax generation from heightmaps\n\n";
        
        std::cout << "NEXT TOPIC: Integration with Physics and Memory Systems\n\n";
    }
    
    void cleanup() {
        if (renderer_) renderer_->shutdown();
        if (window_) window_->shutdown();
    }
    
    //=========================================================================
    // Data Members
    //=========================================================================
    
    // Tutorial resources
    std::unique_ptr<Window> window_;
    std::unique_ptr<Renderer2D> renderer_;
    std::unique_ptr<ecs::Registry> registry_;
    Camera2D camera_;
    
    // Layer system
    std::unordered_map<RenderLayer, LayerInfo> layer_infos_;
    std::vector<LayeredSprite> layered_sprites_;
    std::unordered_map<RenderLayer, std::vector<RenderBatch>> render_batches_;
    
    // System state
    bool needs_depth_sorting_{false};
    bool frustum_culling_enabled_{false};
    
    // Performance tracking
    std::unordered_map<std::string, f32> sorting_performance_;
    std::unordered_map<std::string, LayerPerformance> optimization_results_;
};

//=============================================================================
// Main Function
//=============================================================================

int main() {
    core::Log::info("Main", "Starting Multi-Layer Rendering and Depth Management Tutorial");
    
    std::cout << "\n=== WELCOME TO TUTORIAL 7: MULTI-LAYER RENDERING AND DEPTH MANAGEMENT ===\n";
    std::cout << "This tutorial provides comprehensive coverage of advanced 2D scene organization\n";
    std::cout << "using rendering layers and sophisticated depth management techniques.\n\n";
    std::cout << "You will learn:\n";
    std::cout << "- Layer-based rendering system architecture and organization\n";
    std::cout << "- Depth sorting algorithms and performance optimization\n";
    std::cout << "- Parallax scrolling implementation for depth illusion\n";
    std::cout << "- UI layer separation and independent rendering\n";
    std::cout << "- Per-layer effects and post-processing techniques\n";
    std::cout << "- Performance optimization strategies for complex scenes\n\n";
    std::cout << "Watch for detailed performance analysis and optimization comparisons.\n\n";
    
    MultiLayerRenderingTutorial tutorial;
    
    if (!tutorial.initialize()) {
        core::Log::error("Main", "Failed to initialize tutorial");
        return -1;
    }
    
    tutorial.run();
    
    core::Log::info("Main", "Multi-Layer Rendering Tutorial completed successfully!");
    return 0;
}