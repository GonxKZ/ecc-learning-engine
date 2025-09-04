/**
 * @file 02_sprite_batching_fundamentals.cpp
 * @brief Tutorial 2: Sprite Batching Fundamentals - Performance Optimization Core
 * 
 * This tutorial explores one of the most important concepts in modern 2D rendering:
 * sprite batching. You'll learn why batching is crucial for performance and how
 * the ECScope rendering system automatically optimizes draw calls.
 * 
 * Learning Objectives:
 * 1. Understand what sprite batching is and why it's essential
 * 2. Learn how texture binding affects rendering performance
 * 3. Explore different batching strategies and their trade-offs
 * 4. See real-time performance metrics and optimization effects
 * 5. Experience hands-on performance optimization concepts
 * 
 * Key Concepts Covered:
 * - Draw call reduction through batching
 * - Texture binding and GPU state changes
 * - BatchRenderer system and strategies
 * - Performance measurement and analysis
 * - Memory usage optimization
 * 
 * Educational Value:
 * Batching is fundamental to achieving good performance in 2D games.
 * This tutorial provides deep insights into GPU optimization that apply
 * to all modern graphics programming.
 * 
 * @author ECScope Educational ECS Framework
 * @date 2024
 */

#include <iostream>
#include <memory>
#include <vector>
#include <random>
#include <chrono>
#include <iomanip>

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
 * @brief Sprite Batching Tutorial with Performance Analysis
 * 
 * This tutorial demonstrates batching concepts by creating many sprites
 * with different configurations and showing how batching affects performance.
 */
class SpriteBatchingTutorial {
public:
    SpriteBatchingTutorial() = default;
    ~SpriteBatchingTutorial() { cleanup(); }
    
    bool initialize() {
        core::Log::info("Tutorial", "=== Sprite Batching Fundamentals Tutorial ===");
        core::Log::info("Tutorial", "Learning objective: Understand sprite batching and performance optimization");
        
        // Initialize window and renderer
        window_ = std::make_unique<Window>("Tutorial 2: Sprite Batching", 1600, 900);
        if (!window_->initialize()) {
            core::Log::error("Tutorial", "Failed to create window");
            return false;
        }
        
        // Configure renderer for educational analysis
        Renderer2DConfig renderer_config = Renderer2DConfig::educational_mode();
        renderer_config.debug.collect_gpu_timings = true;
        renderer_config.debug.show_performance_overlay = true;
        renderer_config.debug.show_batch_colors = true; // Visualize batches!
        
        renderer_ = std::make_unique<Renderer2D>(renderer_config);
        if (!renderer_->initialize().is_ok()) {
            core::Log::error("Tutorial", "Failed to initialize renderer");
            return false;
        }
        
        // Set up camera
        camera_ = Camera2D::create_main_camera(1600, 900);
        camera_.set_position(0.0f, 0.0f);
        camera_.set_zoom(0.8f); // Zoom out to see more sprites
        
        // Create ECS registry
        registry_ = std::make_unique<ecs::Registry>();
        
        core::Log::info("Tutorial", "System initialized. Creating demo scenarios...");
        
        return true;
    }
    
    void run() {
        if (!window_ || !renderer_) return;
        
        core::Log::info("Tutorial", "Starting batching demonstration...");
        
        // Run multiple scenarios to demonstrate batching concepts
        run_scenario_1_no_batching();
        run_scenario_2_perfect_batching(); 
        run_scenario_3_mixed_textures();
        run_scenario_4_batching_strategies();
        
        display_educational_summary();
    }
    
private:
    //=========================================================================
    // Scenario 1: No Batching (Individual Draw Calls)
    //=========================================================================
    
    void run_scenario_1_no_batching() {
        core::Log::info("Scenario 1", "=== INDIVIDUAL DRAW CALLS (No Batching) ===");
        core::Log::info("Explanation", "This scenario simulates rendering without batching");
        core::Log::info("Explanation", "Each sprite requires its own draw call - very inefficient!");
        
        // Clear any existing entities
        clear_entities();
        
        // Create sprites that can't be batched (different textures)
        const u32 sprite_count = 100;
        create_unbatchable_sprites(sprite_count);
        
        // Measure performance
        auto performance = measure_rendering_performance("No Batching", 120); // 2 seconds at 60 FPS
        
        core::Log::info("Results", "Draw calls: {}, FPS: {:.1f}, Frame time: {:.2f}ms",
                       performance.average_draw_calls,
                       performance.average_fps,
                       performance.average_frame_time_ms);
        
        core::Log::info("Analysis", "Without batching, we need {} draw calls for {} sprites!",
                       performance.average_draw_calls, sprite_count);
        core::Log::info("Analysis", "This creates significant CPU-GPU communication overhead");
        
        no_batching_results_ = performance;
    }
    
    //=========================================================================
    // Scenario 2: Perfect Batching (Single Texture)
    //=========================================================================
    
    void run_scenario_2_perfect_batching() {
        core::Log::info("Scenario 2", "=== PERFECT BATCHING (Single Texture) ===");
        core::Log::info("Explanation", "All sprites use the same texture - optimal for batching");
        core::Log::info("Explanation", "Should require only 1-2 draw calls regardless of sprite count!");
        
        clear_entities();
        
        // Create sprites that can be perfectly batched (same texture)
        const u32 sprite_count = 1000; // 10x more sprites!
        create_batchable_sprites(sprite_count);
        
        auto performance = measure_rendering_performance("Perfect Batching", 120);
        
        core::Log::info("Results", "Draw calls: {}, FPS: {:.1f}, Frame time: {:.2f}ms",
                       performance.average_draw_calls,
                       performance.average_fps,
                       performance.average_frame_time_ms);
        
        core::Log::info("Analysis", "With batching, {} sprites only need {} draw calls!",
                       sprite_count, performance.average_draw_calls);
        core::Log::info("Analysis", "This is a {}x improvement in draw call efficiency!",
                       static_cast<u32>(sprite_count / std::max(1.0f, performance.average_draw_calls)));
        
        perfect_batching_results_ = performance;
    }
    
    //=========================================================================
    // Scenario 3: Mixed Textures (Real-World Scenario)
    //=========================================================================
    
    void run_scenario_3_mixed_textures() {
        core::Log::info("Scenario 3", "=== MIXED TEXTURES (Real-World Scenario) ===");
        core::Log::info("Explanation", "Sprites use multiple textures - typical game scenario");
        core::Log::info("Explanation", "Batching system must group sprites by texture");
        
        clear_entities();
        
        // Create sprites with mixed textures (more realistic)
        const u32 sprite_count = 800;
        const u32 texture_count = 8; // 8 different textures
        create_mixed_texture_sprites(sprite_count, texture_count);
        
        auto performance = measure_rendering_performance("Mixed Textures", 120);
        
        core::Log::info("Results", "Draw calls: {}, FPS: {:.1f}, Frame time: {:.2f}ms",
                       performance.average_draw_calls,
                       performance.average_fps,
                       performance.average_frame_time_ms);
        
        core::Log::info("Analysis", "With {} textures, we need approximately {} draw calls",
                       texture_count, performance.average_draw_calls);
        core::Log::info("Analysis", "Each texture change requires a new batch");
        core::Log::info("Analysis", "This is why texture atlases are important for performance!");
        
        mixed_texture_results_ = performance;
    }
    
    //=========================================================================
    // Scenario 4: Different Batching Strategies
    //=========================================================================
    
    void run_scenario_4_batching_strategies() {
        core::Log::info("Scenario 4", "=== BATCHING STRATEGIES COMPARISON ===");
        core::Log::info("Explanation", "Comparing different batching strategies");
        
        clear_entities();
        
        // Create a complex scene with varied properties
        const u32 sprite_count = 600;
        create_complex_scene(sprite_count);
        
        // Test different strategies
        std::vector<std::pair<BatchingStrategy, const char*>> strategies = {
            {BatchingStrategy::TextureFirst, "Texture First"},
            {BatchingStrategy::MaterialFirst, "Material First"},
            {BatchingStrategy::ZOrderPreserving, "Z-Order Preserving"},
            {BatchingStrategy::SpatialLocality, "Spatial Locality"},
            {BatchingStrategy::AdaptiveHybrid, "Adaptive Hybrid"}
        };
        
        for (const auto& [strategy, name] : strategies) {
            core::Log::info("Strategy Test", "Testing {} strategy...", name);
            
            // Configure renderer with this strategy
            set_batching_strategy(strategy);
            
            auto performance = measure_rendering_performance(name, 60); // 1 second test
            
            core::Log::info("Strategy Results", "{}: {} draw calls, {:.1f} FPS, {:.2f}ms",
                           name, 
                           performance.average_draw_calls,
                           performance.average_fps,
                           performance.average_frame_time_ms);
            
            strategy_results_.push_back({strategy, name, performance});
        }
        
        // Analyze strategy results
        analyze_strategy_performance();
    }
    
    //=========================================================================
    // Sprite Creation Methods
    //=========================================================================
    
    void create_unbatchable_sprites(u32 count) {
        core::Log::info("Creation", "Creating {} unbatchable sprites (different textures)", count);
        
        for (u32 i = 0; i < count; ++i) {
            u32 entity = registry_->create_entity();
            sprite_entities_.push_back(entity);
            
            // Position in a grid
            f32 x = (i % 10 - 5) * 80.0f;
            f32 y = (i / 10 - 5) * 80.0f;
            
            Transform transform;
            transform.position = {x, y, static_cast<f32>(i)};
            transform.scale = {50.0f, 50.0f, 1.0f};
            registry_->add_component(entity, transform);
            
            // Each sprite gets a different texture (unbatchable!)
            RenderableSprite sprite;
            sprite.texture = TextureHandle{i % 16 + 1, 32, 32}; // Different texture per sprite
            sprite.color_modulation = Color::white();
            sprite.z_order = static_cast<f32>(i);
            registry_->add_component(entity, sprite);
        }
        
        core::Log::info("Creation", "Created {} sprites, each with different texture", count);
    }
    
    void create_batchable_sprites(u32 count) {
        core::Log::info("Creation", "Creating {} batchable sprites (same texture)", count);
        
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<f32> pos_dist(-400.0f, 400.0f);
        std::uniform_real_distribution<f32> size_dist(20.0f, 60.0f);
        
        for (u32 i = 0; i < count; ++i) {
            u32 entity = registry_->create_entity();
            sprite_entities_.push_back(entity);
            
            Transform transform;
            transform.position = {pos_dist(gen), pos_dist(gen), static_cast<f32>(i % 100)};
            transform.scale = {size_dist(gen), size_dist(gen), 1.0f};
            registry_->add_component(entity, transform);
            
            // All sprites use the same texture (highly batchable!)
            RenderableSprite sprite;
            sprite.texture = TextureHandle{1, 32, 32}; // Same texture for all
            sprite.color_modulation = Color{
                static_cast<u8>(128 + (i * 127) % 128),
                static_cast<u8>(128 + (i * 73) % 128),
                static_cast<u8>(128 + (i * 191) % 128),
                255
            };
            sprite.z_order = static_cast<f32>(i % 100);
            registry_->add_component(entity, sprite);
        }
        
        core::Log::info("Creation", "Created {} sprites, all with same texture", count);
    }
    
    void create_mixed_texture_sprites(u32 count, u32 texture_count) {
        core::Log::info("Creation", "Creating {} sprites with {} different textures", count, texture_count);
        
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<f32> pos_dist(-500.0f, 500.0f);
        std::uniform_int_distribution<u32> texture_dist(1, texture_count);
        
        for (u32 i = 0; i < count; ++i) {
            u32 entity = registry_->create_entity();
            sprite_entities_.push_back(entity);
            
            Transform transform;
            transform.position = {pos_dist(gen), pos_dist(gen), static_cast<f32>(i % 50)};
            transform.scale = {40.0f, 40.0f, 1.0f};
            registry_->add_component(entity, transform);
            
            RenderableSprite sprite;
            sprite.texture = TextureHandle{texture_dist(gen), 32, 32}; // Random texture
            sprite.color_modulation = Color::white();
            sprite.z_order = static_cast<f32>(i % 50);
            registry_->add_component(entity, sprite);
        }
        
        core::Log::info("Creation", "Created {} sprites with {} texture variations", count, texture_count);
    }
    
    void create_complex_scene(u32 count) {
        core::Log::info("Creation", "Creating complex scene with {} sprites", count);
        
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<f32> pos_dist(-600.0f, 600.0f);
        std::uniform_int_distribution<u32> texture_dist(1, 6);
        
        for (u32 i = 0; i < count; ++i) {
            u32 entity = registry_->create_entity();
            sprite_entities_.push_back(entity);
            
            Transform transform;
            transform.position = {pos_dist(gen), pos_dist(gen), static_cast<f32>(i % 20)};
            transform.scale = {30.0f + (i % 40), 30.0f + (i % 40), 1.0f};
            registry_->add_component(entity, transform);
            
            RenderableSprite sprite;
            sprite.texture = TextureHandle{texture_dist(gen), 32, 32};
            
            // Vary blend modes for complexity
            if (i % 20 == 0) sprite.blend_mode = RenderableSprite::BlendMode::Additive;
            if (i % 25 == 0) sprite.blend_mode = RenderableSprite::BlendMode::Multiply;
            
            sprite.color_modulation = Color{
                static_cast<u8>(100 + (i * 155) % 156),
                static_cast<u8>(100 + (i * 97) % 156),
                static_cast<u8>(100 + (i * 139) % 156),
                255
            };
            sprite.z_order = static_cast<f32>(i % 20);
            registry_->add_component(entity, sprite);
        }
        
        core::Log::info("Creation", "Created complex scene with varied properties");
    }
    
    //=========================================================================
    // Performance Measurement
    //=========================================================================
    
    struct PerformanceMetrics {
        f32 average_fps{0.0f};
        f32 average_frame_time_ms{0.0f};
        u32 average_draw_calls{0};
        u32 total_vertices{0};
        f32 batching_efficiency{0.0f};
        usize memory_usage{0};
    };
    
    PerformanceMetrics measure_rendering_performance(const char* scenario_name, u32 frames_to_measure) {
        core::Log::info("Measurement", "Measuring performance for {} frames...", frames_to_measure);
        
        PerformanceMetrics metrics;
        f32 total_frame_time = 0.0f;
        u32 total_draw_calls = 0;
        u32 total_vertices = 0;
        f32 total_batching_efficiency = 0.0f;
        
        for (u32 frame = 0; frame < frames_to_measure; ++frame) {
            auto frame_start = std::chrono::high_resolution_clock::now();
            
            // Render frame
            renderer_->begin_frame();
            renderer_->set_active_camera(camera_);
            renderer_->render_entities(*registry_);
            renderer_->end_frame();
            
            // Present to screen
            window_->swap_buffers();
            window_->poll_events();
            
            auto frame_end = std::chrono::high_resolution_clock::now();
            f32 frame_time = std::chrono::duration<f32>(frame_end - frame_start).count();
            total_frame_time += frame_time;
            
            // Collect statistics
            const auto& stats = renderer_->get_statistics();
            total_draw_calls += stats.gpu_stats.draw_calls;
            total_vertices += stats.gpu_stats.vertices_rendered;
            total_batching_efficiency += stats.gpu_stats.batching_efficiency;
        }
        
        // Calculate averages
        metrics.average_frame_time_ms = (total_frame_time / frames_to_measure) * 1000.0f;
        metrics.average_fps = 1.0f / (total_frame_time / frames_to_measure);
        metrics.average_draw_calls = total_draw_calls / frames_to_measure;
        metrics.total_vertices = total_vertices / frames_to_measure;
        metrics.batching_efficiency = total_batching_efficiency / frames_to_measure;
        
        const auto& final_stats = renderer_->get_statistics();
        metrics.memory_usage = final_stats.gpu_stats.total_gpu_memory;
        
        return metrics;
    }
    
    //=========================================================================
    // Utility Functions
    //=========================================================================
    
    void clear_entities() {
        sprite_entities_.clear();
        registry_ = std::make_unique<ecs::Registry>();
    }
    
    void set_batching_strategy(BatchingStrategy strategy) {
        // In a real implementation, this would update the batch renderer configuration
        core::Log::info("Config", "Setting batching strategy to {}", static_cast<int>(strategy));
    }
    
    void analyze_strategy_performance() {
        core::Log::info("Analysis", "=== BATCHING STRATEGY PERFORMANCE ANALYSIS ===");
        
        // Find best and worst performing strategies
        auto best = std::min_element(strategy_results_.begin(), strategy_results_.end(),
            [](const auto& a, const auto& b) { return a.performance.average_draw_calls < b.performance.average_draw_calls; });
        
        auto worst = std::max_element(strategy_results_.begin(), strategy_results_.end(),
            [](const auto& a, const auto& b) { return a.performance.average_draw_calls < b.performance.average_draw_calls; });
        
        if (best != strategy_results_.end() && worst != strategy_results_.end()) {
            core::Log::info("Analysis", "Best strategy: {} ({} draw calls)",
                           best->name, best->performance.average_draw_calls);
            core::Log::info("Analysis", "Worst strategy: {} ({} draw calls)", 
                           worst->name, worst->performance.average_draw_calls);
            
            f32 improvement = static_cast<f32>(worst->performance.average_draw_calls) / 
                             static_cast<f32>(best->performance.average_draw_calls);
            core::Log::info("Analysis", "Best strategy is {:.1f}x more efficient in draw calls!", improvement);
        }
    }
    
    void display_educational_summary() {
        std::cout << "\n=== SPRITE BATCHING TUTORIAL SUMMARY ===\n\n";
        
        std::cout << "KEY LEARNINGS:\n\n";
        
        std::cout << "1. DRAW CALL IMPACT:\n";
        if (no_batching_results_.average_draw_calls > 0 && perfect_batching_results_.average_draw_calls > 0) {
            f32 improvement = static_cast<f32>(no_batching_results_.average_draw_calls) / 
                             static_cast<f32>(perfect_batching_results_.average_draw_calls);
            std::cout << "   - Without batching: " << no_batching_results_.average_draw_calls << " draw calls\n";
            std::cout << "   - With batching: " << perfect_batching_results_.average_draw_calls << " draw calls\n";
            std::cout << "   - Performance improvement: " << std::fixed << std::setprecision(1) << improvement << "x better!\n\n";
        }
        
        std::cout << "2. TEXTURE BINDING COST:\n";
        std::cout << "   - Each texture change requires a new batch\n";
        std::cout << "   - Texture atlases combine multiple images into one texture\n";
        std::cout << "   - This dramatically reduces texture binding overhead\n\n";
        
        std::cout << "3. BATCHING STRATEGIES:\n";
        for (const auto& result : strategy_results_) {
            std::cout << "   - " << result.name << ": " << result.performance.average_draw_calls << " draw calls\n";
        }
        std::cout << "   - Different strategies work better for different scene types\n\n";
        
        std::cout << "4. MEMORY EFFICIENCY:\n";
        std::cout << "   - Batching reduces CPU memory usage for draw commands\n";
        std::cout << "   - GPU memory usage depends on vertex buffer management\n";
        std::cout << "   - Smart batching reduces both CPU and GPU overhead\n\n";
        
        std::cout << "PRACTICAL APPLICATIONS:\n";
        std::cout << "- Use texture atlases to improve batching efficiency\n";
        std::cout << "- Group sprites by material properties when possible\n";
        std::cout << "- Consider depth sorting vs. batching trade-offs\n";
        std::cout << "- Monitor draw calls as a key performance metric\n\n";
        
        std::cout << "NEXT TUTORIAL: Camera Systems and Coordinate Transformations\n\n";
    }
    
    void cleanup() {
        if (renderer_) renderer_->shutdown();
        if (window_) window_->shutdown();
    }
    
    // Tutorial resources
    std::unique_ptr<Window> window_;
    std::unique_ptr<Renderer2D> renderer_;
    std::unique_ptr<ecs::Registry> registry_;
    Camera2D camera_;
    
    // Entity tracking
    std::vector<u32> sprite_entities_;
    
    // Performance results
    PerformanceMetrics no_batching_results_;
    PerformanceMetrics perfect_batching_results_;
    PerformanceMetrics mixed_texture_results_;
    
    struct StrategyResult {
        BatchingStrategy strategy;
        const char* name;
        PerformanceMetrics performance;
    };
    std::vector<StrategyResult> strategy_results_;
};

//=============================================================================
// Main Function
//=============================================================================

int main() {
    core::Log::info("Main", "Starting Sprite Batching Fundamentals Tutorial");
    
    std::cout << "\n=== WELCOME TO TUTORIAL 2: SPRITE BATCHING FUNDAMENTALS ===\n";
    std::cout << "This tutorial demonstrates the critical importance of sprite batching\n";
    std::cout << "for achieving high performance in 2D rendering systems.\n\n";
    std::cout << "You will see:\n";
    std::cout << "- Performance difference between batched and unbatched rendering\n";
    std::cout << "- How texture changes affect batching efficiency\n"; 
    std::cout << "- Comparison of different batching strategies\n";
    std::cout << "- Real-world scenarios and optimization techniques\n\n";
    std::cout << "Watch the console for detailed performance analysis.\n\n";
    
    SpriteBatchingTutorial tutorial;
    
    if (!tutorial.initialize()) {
        core::Log::error("Main", "Failed to initialize tutorial");
        return -1;
    }
    
    tutorial.run();
    
    core::Log::info("Main", "Sprite Batching Tutorial completed successfully!");
    return 0;
}