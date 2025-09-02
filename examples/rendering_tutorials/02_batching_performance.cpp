/**
 * @file examples/rendering_tutorials/02_batching_performance.cpp
 * @brief Tutorial 2: Understanding Sprite Batching and Performance - ECScope Educational Graphics Programming
 * 
 * This tutorial demonstrates the importance and impact of sprite batching on rendering performance.
 * Students will learn:
 * - What sprite batching is and why it matters
 * - How different batching strategies affect performance
 * - The relationship between draw calls and performance
 * - How to analyze and optimize batching efficiency
 * - Visual comparison of batched vs unbatched rendering
 * 
 * Educational Objectives:
 * - Understand the GPU performance bottlenecks
 * - Learn about draw call optimization
 * - Experience the dramatic impact of batching
 * - Analyze performance metrics in real-time
 * - Compare different batching strategies
 * 
 * Prerequisites: Completion of Tutorial 1, basic understanding of rendering pipeline
 * 
 * @author ECScope Educational ECS Framework - Phase 7: Renderizado 2D
 * @date 2024
 */

#include <iostream>
#include <memory>
#include <chrono>
#include <random>
#include <iomanip>

// ECScope Core Systems
#include "../../src/core/log.hpp"
#include "../../src/ecs/registry.hpp"
#include "../../src/ecs/components/transform.hpp"

// Rendering System
#include "../../src/renderer/renderer_2d.hpp"
#include "../../src/renderer/batch_renderer.hpp"
#include "../../src/renderer/window.hpp"

// Platform Integration
#ifdef ECSCOPE_HAS_GRAPHICS
#include <SDL2/SDL.h>
#endif

using namespace ecscope;

/**
 * @brief Tutorial 2: Sprite Batching Performance Demonstration
 * 
 * This tutorial shows the dramatic performance difference between batched
 * and unbatched sprite rendering, with real-time metrics and analysis.
 */
class BatchingPerformanceTutorial {
public:
    /**
     * @brief Initialize the batching performance tutorial
     */
    bool initialize() {
        std::cout << "\n=== ECScope Tutorial 2: Sprite Batching Performance ===\n";
        std::cout << "This tutorial demonstrates the critical importance of sprite batching.\n\n";
        
        // Initialize graphics and ECS
        if (!initialize_graphics() || !initialize_ecs()) {
            return false;
        }
        
        // Create test scenarios
        create_camera();
        create_test_sprites();
        
        // Initialize performance tracking
        reset_performance_metrics();
        
        std::cout << "\n🎉 Tutorial initialization complete!\n";
        print_controls();
        
        return true;
    }
    
    /**
     * @brief Main tutorial execution loop
     */
    void run() {
        std::cout << "\n=== Running Batching Performance Tutorial ===\n\n";
        
        bool running = true;
        auto last_frame_time = std::chrono::high_resolution_clock::now();
        
        while (running) {
            // Calculate frame timing
            auto current_time = std::chrono::high_resolution_clock::now();
            auto frame_duration = std::chrono::duration_cast<std::chrono::microseconds>(current_time - last_frame_time);
            f32 delta_time = frame_duration.count() / 1000000.0f;
            last_frame_time = current_time;
            
            // Handle input
            running = handle_input();
            
            // Update simulation
            update(delta_time);
            
            // Render with performance tracking
            auto render_start = std::chrono::high_resolution_clock::now();
            render();
            auto render_end = std::chrono::high_resolution_clock::now();
            
            // Update performance metrics
            auto render_duration = std::chrono::duration_cast<std::chrono::microseconds>(render_end - render_start);
            update_performance_metrics(delta_time, render_duration.count() / 1000.0f);
            
            // Display performance info periodically
            if (frame_count_ % 60 == 0) { // Every second at 60 FPS
                display_performance_analysis();
            }
            
            frame_count_++;
        }
        
        std::cout << "\n✅ Batching Performance Tutorial completed!\n";
        display_final_analysis();
    }

private:
    // Core systems
    std::unique_ptr<renderer::Window> window_;
    std::unique_ptr<ecs::Registry> registry_;
    std::unique_ptr<renderer::Renderer2D> renderer_;
    
    // Scene data
    ecs::EntityID camera_entity_{ecs::INVALID_ENTITY_ID};
    std::vector<ecs::EntityID> sprite_entities_;
    
    // Tutorial state
    enum class BatchingMode {
        Optimal,      // Best batching strategy
        Suboptimal,   // Poor batching (many texture switches)
        Disabled      // No batching (individual draw calls)
    };
    
    BatchingMode current_mode_{BatchingMode::Optimal};
    u32 sprite_count_{1000};
    bool show_debug_visualization_{false};
    bool animate_sprites_{true};
    f32 animation_time_{0.0f};
    
    // Performance tracking
    u32 frame_count_{0};
    struct PerformanceData {
        f32 frame_time_ms{0.0f};
        f32 render_time_ms{0.0f};
        u32 draw_calls{0};
        u32 vertices_rendered{0};
        f32 batching_efficiency{0.0f};
        usize gpu_memory_used{0};
        
        // Running averages (for stability)
        f32 avg_frame_time{0.0f};
        f32 avg_render_time{0.0f};
        f32 avg_draw_calls{0.0f};
        f32 avg_batching_efficiency{0.0f};
    } perf_data_;
    
    /**
     * @brief Initialize graphics window and context
     */
    bool initialize_graphics() {
#ifdef ECSCOPE_HAS_GRAPHICS
        if (SDL_Init(SDL_INIT_VIDEO) < 0) {
            std::cerr << "SDL2 initialization failed: " << SDL_GetError() << "\n";
            return false;
        }
        
        window_ = std::make_unique<renderer::Window>();
        if (!window_->create(1200, 800, "ECScope Tutorial 2: Sprite Batching Performance")) {
            std::cerr << "Window creation failed\n";
            return false;
        }
        
        std::cout << "✅ Graphics system initialized (1200x800 window)\n";
        return true;
#else
        std::cerr << "Graphics support not compiled\n";
        return false;
#endif
    }
    
    /**
     * @brief Initialize ECS registry and renderer
     */
    bool initialize_ecs() {
        registry_ = std::make_unique<ecs::Registry>();
        
        // Start with optimal batching configuration
        auto config = renderer::Renderer2DConfig::educational_mode();
        config.rendering.max_sprites_per_batch = 1000;
        config.debug.enable_batch_visualization = false;
        config.debug.collect_detailed_stats = true;
        
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
     * @brief Create camera for viewing the scene
     */
    void create_camera() {
        camera_entity_ = registry_->create_entity();
        
        auto& transform = registry_->add_component<ecs::components::Transform>(camera_entity_);
        transform.position = {0.0f, 0.0f, 0.0f};
        
        auto& camera = registry_->add_component<renderer::components::Camera2D>(camera_entity_);
        camera.position = {0.0f, 0.0f};
        camera.zoom = 0.5f; // Zoomed out to see more sprites
        camera.viewport_width = 1200.0f;
        camera.viewport_height = 800.0f;
        
        std::cout << "✅ Camera created with 0.5x zoom\n";
    }
    
    /**
     * @brief Create test sprites for batching demonstration
     */
    void create_test_sprites() {
        std::cout << "Creating " << sprite_count_ << " test sprites...\n";
        
        sprite_entities_.clear();
        sprite_entities_.reserve(sprite_count_);
        
        std::random_device rd;
        std::mt19937 gen(42); // Fixed seed for reproducible results
        std::uniform_real_distribution<f32> pos_dist(-800.0f, 800.0f);
        std::uniform_real_distribution<f32> size_dist(16.0f, 48.0f);
        std::uniform_real_distribution<f32> color_dist(0.4f, 1.0f);
        
        for (u32 i = 0; i < sprite_count_; ++i) {
            auto entity = registry_->create_entity();
            
            // Transform component
            auto& transform = registry_->add_component<ecs::components::Transform>(entity);
            transform.position = {pos_dist(gen), pos_dist(gen), 0.0f};
            f32 size = size_dist(gen);
            transform.scale = {size, size, 1.0f};
            transform.rotation = {0.0f, 0.0f, 0.0f};
            
            // RenderableSprite component with texture assignment based on batching mode
            auto& sprite = registry_->add_component<renderer::components::RenderableSprite>(entity);
            
            // Assign textures based on current batching strategy
            assign_texture_for_batching_mode(sprite, i);
            
            sprite.color = {color_dist(gen), color_dist(gen), color_dist(gen), 1.0f};
            sprite.z_order = 0.0f;
            sprite.blend_mode = renderer::components::RenderableSprite::BlendMode::Alpha;
            
            sprite_entities_.push_back(entity);
        }
        
        std::cout << "✅ Created " << sprite_entities_.size() << " sprites\n";
        update_batching_configuration();
    }
    
    /**
     * @brief Assign texture IDs based on current batching mode
     */
    void assign_texture_for_batching_mode(renderer::components::RenderableSprite& sprite, u32 sprite_index) {
        switch (current_mode_) {
            case BatchingMode::Optimal:
                // Use only a few textures to maximize batching
                sprite.texture_id = static_cast<renderer::TextureID>(sprite_index % 4);
                break;
                
            case BatchingMode::Suboptimal:
                // Use many different textures to break batches frequently
                sprite.texture_id = static_cast<renderer::TextureID>(sprite_index % 16);
                break;
                
            case BatchingMode::Disabled:
                // Each sprite gets a unique texture ID to force individual draw calls
                sprite.texture_id = static_cast<renderer::TextureID>(sprite_index % 32);
                break;
        }
    }
    
    /**
     * @brief Update renderer configuration based on batching mode
     */
    void update_batching_configuration() {
        auto config = renderer_->get_config();
        
        switch (current_mode_) {
            case BatchingMode::Optimal:
                config.rendering.max_sprites_per_batch = 1000;
                config.debug.enable_batch_visualization = show_debug_visualization_;
                break;
                
            case BatchingMode::Suboptimal:
                config.rendering.max_sprites_per_batch = 200;
                config.debug.enable_batch_visualization = show_debug_visualization_;
                break;
                
            case BatchingMode::Disabled:
                config.rendering.max_sprites_per_batch = 1; // Force individual draw calls
                config.debug.enable_batch_visualization = false;
                break;
        }
        
        renderer_->update_config(config);
        
        // Update sprite texture assignments
        for (u32 i = 0; i < sprite_entities_.size(); ++i) {
            auto entity = sprite_entities_[i];
            auto& sprite = registry_->get_component<renderer::components::RenderableSprite>(entity);
            assign_texture_for_batching_mode(sprite, i);
        }
    }
    
    /**
     * @brief Handle user input for controlling the demonstration
     */
    bool handle_input() {
#ifdef ECSCOPE_HAS_GRAPHICS
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_QUIT:
                    return false;
                    
                case SDL_KEYDOWN:
                    switch (event.key.keysym.sym) {
                        case SDLK_ESCAPE:
                        case SDLK_q:
                            return false;
                            
                        case SDLK_1:
                            change_batching_mode(BatchingMode::Optimal);
                            break;
                            
                        case SDLK_2:
                            change_batching_mode(BatchingMode::Suboptimal);
                            break;
                            
                        case SDLK_3:
                            change_batching_mode(BatchingMode::Disabled);
                            break;
                            
                        case SDLK_v:
                            toggle_debug_visualization();
                            break;
                            
                        case SDLK_a:
                            toggle_animation();
                            break;
                            
                        case SDLK_PLUS:
                        case SDLK_EQUALS:
                            change_sprite_count(sprite_count_ + 500);
                            break;
                            
                        case SDLK_MINUS:
                            change_sprite_count(sprite_count_ > 500 ? sprite_count_ - 500 : 100);
                            break;
                            
                        case SDLK_r:
                            reset_performance_metrics();
                            break;
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
     * @brief Update animation and other dynamic elements
     */
    void update(f32 delta_time) {
        if (animate_sprites_) {
            animation_time_ += delta_time;
            
            // Gentle movement animation to keep things visually interesting
            for (u32 i = 0; i < sprite_entities_.size(); ++i) {
                auto entity = sprite_entities_[i];
                auto& transform = registry_->get_component<ecs::components::Transform>(entity);
                
                // Small orbital movement
                f32 phase = (i % 100) / 100.0f * 6.28318f;
                f32 radius = 20.0f;
                f32 speed = 0.5f;
                
                transform.position.x += std::cos(animation_time_ * speed + phase) * radius * delta_time * 0.1f;
                transform.position.y += std::sin(animation_time_ * speed + phase) * radius * delta_time * 0.1f;
            }
        }
    }
    
    /**
     * @brief Render the scene with performance tracking
     */
    void render() {
        // Begin frame
        renderer_->begin_frame();
        
        // Set camera
        const auto& camera = registry_->get_component<renderer::components::Camera2D>(camera_entity_);
        renderer_->set_active_camera(camera);
        
        // Render all sprites
        renderer_->render_entities(*registry_);
        
        // End frame
        renderer_->end_frame();
        
        // Present to screen
        if (window_) {
            window_->swap_buffers();
        }
    }
    
    /**
     * @brief Change the current batching mode
     */
    void change_batching_mode(BatchingMode new_mode) {
        current_mode_ = new_mode;
        
        const char* mode_names[] = {"Optimal Batching", "Suboptimal Batching", "Batching Disabled"};
        std::cout << "\n🔄 Switched to: " << mode_names[static_cast<int>(new_mode)] << "\n";
        
        update_batching_configuration();
        reset_performance_metrics();
        
        // Educational explanation
        switch (new_mode) {
            case BatchingMode::Optimal:
                std::cout << "   📚 This mode uses few textures to maximize sprite batching efficiency.\n";
                std::cout << "   💡 Expect high performance with few draw calls.\n";
                break;
                
            case BatchingMode::Suboptimal:
                std::cout << "   📚 This mode uses many textures, breaking batches frequently.\n";
                std::cout << "   ⚠️  Expect moderate performance with more draw calls.\n";
                break;
                
            case BatchingMode::Disabled:
                std::cout << "   📚 This mode renders each sprite individually (no batching).\n";
                std::cout << "   🐌 Expect poor performance with many draw calls.\n";
                break;
        }
    }
    
    /**
     * @brief Toggle debug visualization
     */
    void toggle_debug_visualization() {
        show_debug_visualization_ = !show_debug_visualization_;
        std::cout << "\n👁️  Debug visualization: " << (show_debug_visualization_ ? "ON" : "OFF") << "\n";
        
        if (show_debug_visualization_) {
            std::cout << "   📚 Different batch colors will be shown to visualize batching.\n";
        }
        
        update_batching_configuration();
    }
    
    /**
     * @brief Toggle sprite animation
     */
    void toggle_animation() {
        animate_sprites_ = !animate_sprites_;
        std::cout << "\n🎬 Animation: " << (animate_sprites_ ? "ON" : "OFF") << "\n";
        
        if (!animate_sprites_) {
            std::cout << "   📚 Static scene - better for analyzing pure batching performance.\n";
        }
    }
    
    /**
     * @brief Change the number of sprites
     */
    void change_sprite_count(u32 new_count) {
        sprite_count_ = std::clamp(new_count, 100u, 5000u);
        std::cout << "\n🔢 Sprite count changed to: " << sprite_count_ << "\n";
        
        create_test_sprites();
        reset_performance_metrics();
    }
    
    /**
     * @brief Reset performance metrics for clean measurement
     */
    void reset_performance_metrics() {
        perf_data_ = {};
        frame_count_ = 0;
        std::cout << "📊 Performance metrics reset\n";
    }
    
    /**
     * @brief Update performance metrics with current frame data
     */
    void update_performance_metrics(f32 frame_time, f32 render_time) {
        // Get renderer statistics
        const auto& stats = renderer_->get_statistics();
        
        // Update current frame data
        perf_data_.frame_time_ms = frame_time * 1000.0f;
        perf_data_.render_time_ms = render_time;
        perf_data_.draw_calls = stats.gpu_stats.draw_calls;
        perf_data_.vertices_rendered = stats.gpu_stats.vertices_rendered;
        perf_data_.batching_efficiency = stats.gpu_stats.batching_efficiency;
        perf_data_.gpu_memory_used = stats.gpu_stats.total_gpu_memory;
        
        // Update running averages (exponential smoothing)
        f32 alpha = 0.05f; // Smoothing factor
        perf_data_.avg_frame_time = alpha * perf_data_.frame_time_ms + (1.0f - alpha) * perf_data_.avg_frame_time;
        perf_data_.avg_render_time = alpha * perf_data_.render_time_ms + (1.0f - alpha) * perf_data_.avg_render_time;
        perf_data_.avg_draw_calls = alpha * perf_data_.draw_calls + (1.0f - alpha) * perf_data_.avg_draw_calls;
        perf_data_.avg_batching_efficiency = alpha * perf_data_.batching_efficiency + (1.0f - alpha) * perf_data_.avg_batching_efficiency;
    }
    
    /**
     * @brief Display real-time performance analysis
     */
    void display_performance_analysis() {
        const char* mode_names[] = {"OPTIMAL", "SUBOPTIMAL", "DISABLED"};
        
        std::cout << "\n" << std::string(60, '=') << "\n";
        std::cout << "PERFORMANCE ANALYSIS - " << mode_names[static_cast<int>(current_mode_)] << " BATCHING\n";
        std::cout << std::string(60, '=') << "\n";
        
        std::cout << std::fixed << std::setprecision(2);
        std::cout << "Sprites:          " << sprite_count_ << "\n";
        std::cout << "FPS:              " << (perf_data_.avg_frame_time > 0 ? 1000.0f / perf_data_.avg_frame_time : 0) << "\n";
        std::cout << "Frame Time:       " << perf_data_.avg_frame_time << " ms\n";
        std::cout << "Render Time:      " << perf_data_.avg_render_time << " ms\n";
        std::cout << "Draw Calls:       " << static_cast<u32>(perf_data_.avg_draw_calls) << "\n";
        std::cout << "Vertices:         " << perf_data_.vertices_rendered << "\n";
        std::cout << "Batching Eff:     " << (perf_data_.avg_batching_efficiency * 100.0f) << "%\n";
        std::cout << "GPU Memory:       " << (perf_data_.gpu_memory_used / 1024) << " KB\n";
        
        // Performance rating
        f32 fps = perf_data_.avg_frame_time > 0 ? 1000.0f / perf_data_.avg_frame_time : 0;
        std::cout << "Performance:      ";
        if (fps >= 58.0f) std::cout << "EXCELLENT 🟢\n";
        else if (fps >= 45.0f) std::cout << "GOOD 🟡\n";
        else if (fps >= 30.0f) std::cout << "FAIR 🟠\n";
        else std::cout << "POOR 🔴\n";
        
        // Educational insights
        std::cout << "\nInsights:\n";
        switch (current_mode_) {
            case BatchingMode::Optimal:
                std::cout << "• Minimal draw calls maximize performance\n";
                std::cout << "• High batching efficiency reduces GPU overhead\n";
                break;
                
            case BatchingMode::Suboptimal:
                std::cout << "• Texture switches break batches, increasing draw calls\n";
                std::cout << "• Performance impact depends on GPU and driver\n";
                break;
                
            case BatchingMode::Disabled:
                std::cout << "• Each sprite = one draw call = maximum overhead\n";
                std::cout << "• This shows why batching is critical for performance\n";
                break;
        }
    }
    
    /**
     * @brief Display final performance analysis and educational summary
     */
    void display_final_analysis() {
        std::cout << "\n" << std::string(70, '=') << "\n";
        std::cout << "FINAL ANALYSIS - SPRITE BATCHING PERFORMANCE TUTORIAL\n";
        std::cout << std::string(70, '=') << "\n";
        
        std::cout << "\n📚 Key Learning Points:\n";
        std::cout << "1. DRAW CALLS ARE EXPENSIVE\n";
        std::cout << "   - Each draw call has CPU and GPU overhead\n";
        std::cout << "   - Reducing draw calls dramatically improves performance\n\n";
        
        std::cout << "2. BATCHING GROUPS SIMILAR OPERATIONS\n";
        std::cout << "   - Sprites with same texture/material can be batched\n";
        std::cout << "   - Fewer state changes = better performance\n\n";
        
        std::cout << "3. TEXTURE MANAGEMENT IS CRUCIAL\n";
        std::cout << "   - Using texture atlases improves batching\n";
        std::cout << "   - Frequent texture switches break batches\n\n";
        
        std::cout << "4. PERFORMANCE SCALES WITH COMPLEXITY\n";
        std::cout << "   - More sprites = more potential for optimization\n";
        std::cout << "   - Good batching becomes critical in complex scenes\n\n";
        
        std::cout << "💡 Optimization Recommendations:\n";
        std::cout << "• Use texture atlases to reduce texture count\n";
        std::cout << "• Sort sprites by texture/material before rendering\n";
        std::cout << "• Monitor draw calls and batching efficiency\n";
        std::cout << "• Profile on target hardware for accurate results\n\n";
        
        std::cout << "🎓 Congratulations! You now understand sprite batching fundamentals.\n";
        std::cout << "Next: Try Tutorial 3 to learn about advanced camera systems.\n";
    }
    
    /**
     * @brief Print control instructions
     */
    void print_controls() {
        std::cout << "\n" << std::string(50, '-') << "\n";
        std::cout << "INTERACTIVE CONTROLS:\n";
        std::cout << std::string(50, '-') << "\n";
        std::cout << "1, 2, 3    - Switch batching modes\n";
        std::cout << "V          - Toggle debug visualization\n";
        std::cout << "A          - Toggle animation\n";
        std::cout << "+/-        - Increase/decrease sprite count\n";
        std::cout << "R          - Reset performance metrics\n";
        std::cout << "Q/ESC      - Exit tutorial\n";
        std::cout << std::string(50, '-') << "\n";
    }
};

/**
 * @brief Tutorial entry point
 */
int main() {
    core::Log::initialize(core::LogLevel::INFO);
    
    std::cout << R"(
    ╔════════════════════════════════════════════════════════════╗
    ║            ECScope 2D Rendering Tutorial 2                ║
    ║              Sprite Batching Performance                   ║
    ╠════════════════════════════════════════════════════════════╣
    ║  This tutorial demonstrates the critical importance of     ║
    ║  sprite batching for 2D rendering performance.            ║
    ║                                                            ║
    ║  You will learn:                                           ║
    ║  • Why batching matters for GPU performance                ║
    ║  • How texture management affects batching                 ║
    ║  • The relationship between draw calls and FPS            ║
    ║  • Real-time performance analysis and optimization        ║
    ║  • Visual debugging of batching efficiency                ║
    ╚════════════════════════════════════════════════════════════╝
    )";
    
    try {
        BatchingPerformanceTutorial tutorial;
        
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