/**
 * @file 05_texture_atlasing_and_optimization.cpp
 * @brief Tutorial 5: Texture Atlasing and Optimization - Advanced Performance Techniques
 * 
 * This tutorial explores texture atlasing, one of the most important optimization techniques
 * in 2D rendering. You'll learn how to combine multiple textures for better batching performance.
 * 
 * Learning Objectives:
 * 1. Understand texture atlasing concepts and benefits
 * 2. Learn UV coordinate mapping for atlas textures
 * 3. Explore different atlas packing algorithms
 * 4. Measure batching efficiency improvements
 * 5. Master texture memory optimization techniques
 * 
 * Key Concepts Covered:
 * - Texture atlas creation and management
 * - UV coordinate calculation and mapping
 * - Atlas packing algorithms (bin packing, shelf packing)
 * - Batching efficiency with texture atlases
 * - Memory usage optimization and compression
 * - Runtime atlas generation and updates
 * 
 * Educational Value:
 * Texture atlasing is crucial for achieving high performance in 2D games and applications.
 * This tutorial provides practical experience with texture optimization that directly
 * impacts rendering performance and memory usage.
 * 
 * @author ECScope Educational ECS Framework
 * @date 2024
 */

#include <iostream>
#include <memory>
#include <vector>
#include <algorithm>
#include <chrono>
#include <unordered_map>
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
#include "../../src/renderer/resources/texture.hpp"
#include "../../src/renderer/window.hpp"

using namespace ecscope;
using namespace ecscope::renderer;
using namespace ecscope::renderer::components;

/**
 * @brief Texture Atlasing and Optimization Tutorial
 * 
 * This tutorial demonstrates texture atlas techniques through practical examples
 * with performance measurements and visual comparisons.
 */
class TextureAtlasingTutorial {
public:
    TextureAtlasingTutorial() = default;
    ~TextureAtlasingTutorial() { cleanup(); }
    
    bool initialize() {
        core::Log::info("Tutorial", "=== Texture Atlasing and Optimization Tutorial ===");
        core::Log::info("Tutorial", "Learning objective: Master texture atlasing for optimal 2D rendering performance");
        
        // Initialize window and renderer
        window_ = std::make_unique<Window>("Tutorial 5: Texture Atlasing", 1600, 1000);
        if (!window_->initialize()) {
            core::Log::error("Tutorial", "Failed to create window");
            return false;
        }
        
        // Configure renderer for performance analysis
        Renderer2DConfig renderer_config = Renderer2DConfig::educational_mode();
        renderer_config.debug.show_performance_overlay = true;
        renderer_config.debug.show_batch_colors = true; // Visualize batching
        renderer_config.debug.collect_gpu_timings = true;
        
        renderer_ = std::make_unique<Renderer2D>(renderer_config);
        if (!renderer_->initialize().is_ok()) {
            core::Log::error("Tutorial", "Failed to initialize renderer");
            return false;
        }
        
        // Set up camera
        camera_ = Camera2D::create_main_camera(1600, 1000);
        camera_.set_position(0.0f, 0.0f);
        camera_.set_zoom(0.9f); // Show more sprites
        
        // Create ECS registry
        registry_ = std::make_unique<ecs::Registry>();
        
        core::Log::info("Tutorial", "System initialized. Creating texture atlas examples...");
        
        // Create example textures and atlases
        if (!create_example_textures()) {
            core::Log::error("Tutorial", "Failed to create example textures");
            return false;
        }
        
        return true;
    }
    
    void run() {
        if (!window_ || !renderer_) return;
        
        core::Log::info("Tutorial", "Starting texture atlasing demonstration...");
        
        // Run atlas demonstrations
        demonstrate_atlas_concepts();
        demonstrate_uv_coordinate_mapping();
        demonstrate_batching_comparison();
        demonstrate_atlas_packing_algorithms();
        demonstrate_memory_optimization();
        demonstrate_runtime_atlas_generation();
        
        display_educational_summary();
    }
    
private:
    //=========================================================================
    // Example Texture and Atlas Creation
    //=========================================================================
    
    bool create_example_textures() {
        core::Log::info("Textures", "Creating example textures and atlas configurations");
        
        // Create individual texture data (simulated)
        create_individual_textures();
        
        // Create texture atlases with different packing strategies
        create_texture_atlases();
        
        core::Log::info("Textures", "Created {} individual textures and {} atlas configurations",
                       individual_textures_.size(), texture_atlases_.size());
        return true;
    }
    
    void create_individual_textures() {
        // Simulate individual texture data for various game assets
        individual_textures_ = {
            {"player_idle", 64, 64, Color::blue()},
            {"player_walk1", 64, 64, Color::cyan()},
            {"player_walk2", 64, 64, Color::green()},
            {"player_jump", 64, 64, Color::yellow()},
            {"enemy_1", 48, 48, Color::red()},
            {"enemy_2", 48, 48, Color::magenta()},
            {"enemy_3", 48, 48, Color::red()},
            {"coin", 32, 32, Color::yellow()},
            {"gem", 32, 32, Color::cyan()},
            {"powerup", 32, 32, Color::green()},
            {"bullet", 16, 16, Color::white()},
            {"explosion1", 80, 80, Color::red()},
            {"explosion2", 80, 80, Color::yellow()},
            {"cloud1", 96, 48, Color::white()},
            {"cloud2", 128, 64, Color::white()},
            {"tree", 64, 128, Color::green()},
            {"rock1", 48, 48, Color{128, 128, 128, 255}},
            {"rock2", 56, 56, Color{128, 128, 128, 255}},
            {"grass1", 32, 16, Color::green()},
            {"grass2", 32, 16, Color::green()}
        };
        
        core::Log::info("Textures", "Individual texture summary:");
        usize total_memory = 0;
        for (const auto& tex : individual_textures_) {
            usize memory = tex.width * tex.height * 4; // RGBA bytes
            total_memory += memory;
            core::Log::info("Texture", "  {}: {}x{} = {} bytes", 
                           tex.name, tex.width, tex.height, memory);
        }
        core::Log::info("Memory", "Total individual texture memory: {} KB", total_memory / 1024);
    }
    
    void create_texture_atlases() {
        // Create different atlas configurations for comparison
        
        // Atlas 1: Simple shelf packing (power-of-2 size)
        TextureAtlas shelf_atlas;
        shelf_atlas.name = "Shelf Packing Atlas";
        shelf_atlas.description = "Simple shelf packing algorithm, 512x512";
        shelf_atlas.width = 512;
        shelf_atlas.height = 512;
        shelf_atlas.packing_algorithm = AtlasPackingAlgorithm::ShelfPacking;
        pack_atlas_shelf(shelf_atlas, individual_textures_);
        texture_atlases_["shelf"] = shelf_atlas;
        
        // Atlas 2: Bin packing (more efficient space usage)
        TextureAtlas bin_atlas;
        bin_atlas.name = "Bin Packing Atlas";
        bin_atlas.description = "Rectangle bin packing, 512x512";
        bin_atlas.width = 512;
        bin_atlas.height = 512;
        bin_atlas.packing_algorithm = AtlasPackingAlgorithm::BinPacking;
        pack_atlas_bin(bin_atlas, individual_textures_);
        texture_atlases_["bin"] = bin_atlas;
        
        // Atlas 3: Optimized size (minimal wasted space)
        TextureAtlas optimized_atlas;
        optimized_atlas.name = "Size Optimized Atlas";
        optimized_atlas.description = "Optimal size calculation with bin packing";
        optimized_atlas.packing_algorithm = AtlasPackingAlgorithm::OptimalSizeBinPacking;
        pack_atlas_optimal(optimized_atlas, individual_textures_);
        texture_atlases_["optimized"] = optimized_atlas;
        
        // Log atlas statistics
        for (const auto& [name, atlas] : texture_atlases_) {
            log_atlas_statistics(name, atlas);
        }
    }
    
    void pack_atlas_shelf(TextureAtlas& atlas, const std::vector<IndividualTexture>& textures) {
        // Simulate shelf packing algorithm
        core::Log::info("Packing", "Packing {} textures using shelf algorithm", textures.size());
        
        i32 current_x = 0, current_y = 0;
        i32 shelf_height = 0;
        
        for (const auto& tex : textures) {
            // Check if texture fits on current shelf
            if (current_x + tex.width > atlas.width) {
                // Move to next shelf
                current_x = 0;
                current_y += shelf_height;
                shelf_height = tex.height;
                
                // Check if we exceed atlas height
                if (current_y + tex.height > atlas.height) {
                    core::Log::warning("Packing", "Texture {} doesn't fit in shelf atlas", tex.name);
                    continue;
                }
            }
            
            // Place texture
            AtlasRegion region;
            region.texture_name = tex.name;
            region.x = current_x;
            region.y = current_y;
            region.width = tex.width;
            region.height = tex.height;
            region.uv_rect = UVRect{
                static_cast<f32>(current_x) / atlas.width,
                static_cast<f32>(current_y) / atlas.height,
                static_cast<f32>(tex.width) / atlas.width,
                static_cast<f32>(tex.height) / atlas.height
            };
            
            atlas.regions[tex.name] = region;
            
            current_x += tex.width;
            shelf_height = std::max(shelf_height, tex.height);
        }
        
        calculate_atlas_efficiency(atlas);
    }
    
    void pack_atlas_bin(TextureAtlas& atlas, const std::vector<IndividualTexture>& textures) {
        // Simulate rectangle bin packing algorithm (simplified)
        core::Log::info("Packing", "Packing {} textures using bin packing algorithm", textures.size());
        
        // Sort textures by area (largest first) for better packing
        std::vector<IndividualTexture> sorted_textures = textures;
        std::sort(sorted_textures.begin(), sorted_textures.end(),
                 [](const auto& a, const auto& b) {
                     return (a.width * a.height) > (b.width * b.height);
                 });
        
        std::vector<PackingRect> free_rects;
        free_rects.push_back({0, 0, atlas.width, atlas.height});
        
        for (const auto& tex : sorted_textures) {
            bool placed = false;
            
            for (auto it = free_rects.begin(); it != free_rects.end(); ++it) {
                if (it->width >= tex.width && it->height >= tex.height) {
                    // Place texture here
                    AtlasRegion region;
                    region.texture_name = tex.name;
                    region.x = it->x;
                    region.y = it->y;
                    region.width = tex.width;
                    region.height = tex.height;
                    region.uv_rect = UVRect{
                        static_cast<f32>(it->x) / atlas.width,
                        static_cast<f32>(it->y) / atlas.height,
                        static_cast<f32>(tex.width) / atlas.width,
                        static_cast<f32>(tex.height) / atlas.height
                    };
                    
                    atlas.regions[tex.name] = region;
                    
                    // Split remaining space
                    i32 remaining_width = it->width - tex.width;
                    i32 remaining_height = it->height - tex.height;
                    
                    // Remove used rectangle
                    PackingRect used_rect = *it;
                    free_rects.erase(it);
                    
                    // Add new free rectangles if there's remaining space
                    if (remaining_width > 0) {
                        free_rects.push_back({used_rect.x + tex.width, used_rect.y, 
                                            remaining_width, tex.height});
                    }
                    if (remaining_height > 0) {
                        free_rects.push_back({used_rect.x, used_rect.y + tex.height, 
                                            used_rect.width, remaining_height});
                    }
                    
                    placed = true;
                    break;
                }
            }
            
            if (!placed) {
                core::Log::warning("Packing", "Texture {} doesn't fit in bin packing atlas", tex.name);
            }
        }
        
        calculate_atlas_efficiency(atlas);
    }
    
    void pack_atlas_optimal(TextureAtlas& atlas, const std::vector<IndividualTexture>& textures) {
        // Calculate optimal atlas size based on texture content
        usize total_area = 0;
        i32 max_width = 0, max_height = 0;
        
        for (const auto& tex : textures) {
            total_area += tex.width * tex.height;
            max_width = std::max(max_width, tex.width);
            max_height = std::max(max_height, tex.height);
        }
        
        // Estimate optimal dimensions (square-ish, power of 2)
        i32 estimated_size = static_cast<i32>(std::sqrt(total_area * 1.2)); // 20% padding
        
        // Round up to next power of 2
        i32 optimal_size = 1;
        while (optimal_size < estimated_size) {
            optimal_size *= 2;
        }
        
        // Ensure it can fit the largest texture
        optimal_size = std::max(optimal_size, std::max(max_width, max_height));
        
        atlas.width = optimal_size;
        atlas.height = optimal_size;
        
        core::Log::info("Optimal", "Calculated optimal atlas size: {}x{} (total area: {}, efficiency estimate: {:.1f}%)",
                       atlas.width, atlas.height, total_area, 
                       (static_cast<f32>(total_area) / (atlas.width * atlas.height)) * 100.0f);
        
        // Use bin packing with optimal size
        pack_atlas_bin(atlas, textures);
    }
    
    void calculate_atlas_efficiency(TextureAtlas& atlas) {
        usize used_area = 0;
        for (const auto& [name, region] : atlas.regions) {
            used_area += region.width * region.height;
        }
        
        usize total_area = atlas.width * atlas.height;
        atlas.space_efficiency = static_cast<f32>(used_area) / static_cast<f32>(total_area);
        atlas.wasted_space_bytes = (total_area - used_area) * 4; // RGBA
        atlas.memory_usage_bytes = total_area * 4;
        
        core::Log::info("Efficiency", "Atlas {}: {:.1f}% space efficiency, {} KB wasted",
                       atlas.name, atlas.space_efficiency * 100.0f, atlas.wasted_space_bytes / 1024);
    }
    
    void log_atlas_statistics(const std::string& name, const TextureAtlas& atlas) {
        core::Log::info("Atlas", "=== {} Statistics ===", atlas.name);
        core::Log::info("Atlas", "  Size: {}x{} ({} KB)", atlas.width, atlas.height, 
                       atlas.memory_usage_bytes / 1024);
        core::Log::info("Atlas", "  Packed textures: {}/{}", atlas.regions.size(), individual_textures_.size());
        core::Log::info("Atlas", "  Space efficiency: {:.1f}%", atlas.space_efficiency * 100.0f);
        core::Log::info("Atlas", "  Wasted space: {} KB", atlas.wasted_space_bytes / 1024);
        core::Log::info("Atlas", "  Algorithm: {}", get_algorithm_name(atlas.packing_algorithm));
    }
    
    //=========================================================================
    // Demonstration Functions
    //=========================================================================
    
    void demonstrate_atlas_concepts() {
        core::Log::info("Demo 1", "=== TEXTURE ATLAS CONCEPTS ===");
        core::Log::info("Explanation", "Understanding how texture atlases improve batching efficiency");
        
        // Create demo scene with individual textures (worst case for batching)
        core::Log::info("Demo", "Creating scene with individual textures (poor batching)");
        create_individual_texture_scene();
        
        auto individual_performance = measure_rendering_performance("Individual Textures", 120);
        core::Log::info("Results", "Individual textures: {} FPS, {} draw calls", 
                       individual_performance.average_fps, individual_performance.average_draw_calls);
        
        // Create demo scene with atlas textures (optimal batching)
        core::Log::info("Demo", "Creating scene with atlas textures (optimal batching)");
        create_atlas_texture_scene();
        
        auto atlas_performance = measure_rendering_performance("Atlas Textures", 120);
        core::Log::info("Results", "Atlas textures: {} FPS, {} draw calls",
                       atlas_performance.average_fps, atlas_performance.average_draw_calls);
        
        // Compare results
        f32 fps_improvement = atlas_performance.average_fps / individual_performance.average_fps;
        f32 draw_call_reduction = static_cast<f32>(individual_performance.average_draw_calls) / 
                                 static_cast<f32>(atlas_performance.average_draw_calls);
        
        core::Log::info("Analysis", "Atlas improvement: {:.2f}x FPS, {:.1f}x fewer draw calls",
                       fps_improvement, draw_call_reduction);
        
        performance_comparisons_["individual_vs_atlas"] = {
            individual_performance, atlas_performance, fps_improvement, draw_call_reduction
        };
    }
    
    void demonstrate_uv_coordinate_mapping() {
        core::Log::info("Demo 2", "=== UV COORDINATE MAPPING ===");
        core::Log::info("Explanation", "How atlas regions map to UV coordinates for sprites");
        
        // Show UV calculation examples
        const auto& atlas = texture_atlases_.at("bin");
        
        core::Log::info("UV Mapping", "Atlas size: {}x{}", atlas.width, atlas.height);
        
        for (const auto& [name, region] : atlas.regions) {
            core::Log::info("UV Example", "Texture '{}' at ({}, {}), size {}x{}",
                           name, region.x, region.y, region.width, region.height);
            core::Log::info("UV Coords", "  UV rect: ({:.3f}, {:.3f}) to ({:.3f}, {:.3f})",
                           region.uv_rect.u, region.uv_rect.v,
                           region.uv_rect.u + region.uv_rect.width,
                           region.uv_rect.v + region.uv_rect.height);
            
            // Show pixel to UV conversion
            f32 center_u = region.uv_rect.u + region.uv_rect.width * 0.5f;
            f32 center_v = region.uv_rect.v + region.uv_rect.height * 0.5f;
            core::Log::info("UV Center", "  Center UV: ({:.3f}, {:.3f})", center_u, center_v);
        }
        
        // Demonstrate UV calculation formula
        core::Log::info("Formula", "UV calculation: u = pixel_x / atlas_width, v = pixel_y / atlas_height");
        core::Log::info("Formula", "Width/Height: uv_width = pixel_width / atlas_width");
    }
    
    void demonstrate_batching_comparison() {
        core::Log::info("Demo 3", "=== BATCHING EFFICIENCY COMPARISON ===");
        core::Log::info("Explanation", "Measuring batching improvements with different atlas strategies");
        
        struct BatchingTest {
            std::string name;
            std::string description;
            std::function<void()> setup;
        };
        
        std::vector<BatchingTest> tests = {
            {"No Atlas", "Each sprite uses different texture", 
             [this]() { create_no_atlas_scene(); }},
            {"Shelf Atlas", "Simple shelf packing algorithm",
             [this]() { create_atlas_scene("shelf"); }},
            {"Bin Atlas", "Rectangle bin packing algorithm",
             [this]() { create_atlas_scene("bin"); }},
            {"Optimal Atlas", "Size-optimized bin packing",
             [this]() { create_atlas_scene("optimized"); }}
        };
        
        for (const auto& test : tests) {
            core::Log::info("Batching Test", "Testing: {} - {}", test.name, test.description);
            
            test.setup();
            auto performance = measure_rendering_performance(test.name, 90); // 1.5 seconds
            
            core::Log::info("Results", "{}: {:.1f} FPS, {} draw calls, {:.2f}ms frame time",
                           test.name, performance.average_fps, 
                           performance.average_draw_calls, performance.average_frame_time_ms);
            
            batching_test_results_[test.name] = performance;
        }
        
        analyze_batching_results();
    }
    
    void demonstrate_atlas_packing_algorithms() {
        core::Log::info("Demo 4", "=== ATLAS PACKING ALGORITHMS COMPARISON ===");
        core::Log::info("Explanation", "Comparing different packing strategies for space efficiency");
        
        // Compare atlas efficiency
        core::Log::info("Packing Comparison", "Space efficiency analysis:");
        
        f32 best_efficiency = 0.0f;
        std::string best_algorithm;
        
        for (const auto& [name, atlas] : texture_atlases_) {
            f32 efficiency = atlas.space_efficiency * 100.0f;
            core::Log::info("Efficiency", "{}: {:.1f}% space usage, {} KB memory, {} KB wasted",
                           atlas.name, efficiency, 
                           atlas.memory_usage_bytes / 1024,
                           atlas.wasted_space_bytes / 1024);
            
            if (efficiency > best_efficiency) {
                best_efficiency = efficiency;
                best_algorithm = atlas.name;
            }
        }
        
        core::Log::info("Winner", "Most efficient packing: {} ({:.1f}% efficiency)",
                       best_algorithm, best_efficiency);
        
        // Demonstrate packing algorithm characteristics
        explain_packing_algorithms();
    }
    
    void demonstrate_memory_optimization() {
        core::Log::info("Demo 5", "=== MEMORY OPTIMIZATION TECHNIQUES ===");
        core::Log::info("Explanation", "Advanced techniques for reducing texture memory usage");
        
        // Calculate memory usage comparison
        usize individual_memory = 0;
        for (const auto& tex : individual_textures_) {
            individual_memory += tex.width * tex.height * 4; // RGBA
        }
        
        core::Log::info("Memory", "Individual textures total: {} KB", individual_memory / 1024);
        
        for (const auto& [name, atlas] : texture_atlases_) {
            f32 memory_ratio = static_cast<f32>(atlas.memory_usage_bytes) / static_cast<f32>(individual_memory);
            f32 savings_percent = (1.0f - memory_ratio) * 100.0f;
            
            core::Log::info("Memory", "{}: {} KB ({:.1f}% of individual, {:.1f}% savings)",
                           atlas.name, atlas.memory_usage_bytes / 1024, 
                           memory_ratio * 100.0f, savings_percent);
        }
        
        // Demonstrate additional optimization techniques
        demonstrate_compression_techniques();
        demonstrate_mipmapping_considerations();
    }
    
    void demonstrate_runtime_atlas_generation() {
        core::Log::info("Demo 6", "=== RUNTIME ATLAS GENERATION ===");
        core::Log::info("Explanation", "Dynamic texture atlas creation and updates");
        
        // Simulate runtime atlas generation
        RuntimeAtlas runtime_atlas;
        runtime_atlas.name = "Runtime Generated";
        runtime_atlas.max_width = 1024;
        runtime_atlas.max_height = 1024;
        runtime_atlas.current_width = 256;
        runtime_atlas.current_height = 256;
        
        core::Log::info("Runtime", "Starting with {}x{} atlas, max size {}x{}",
                       runtime_atlas.current_width, runtime_atlas.current_height,
                       runtime_atlas.max_width, runtime_atlas.max_height);
        
        // Simulate adding textures dynamically
        std::vector<std::string> dynamic_textures = {
            "ui_button", "ui_panel", "particle_spark", "particle_smoke", "font_glyph_a"
        };
        
        for (const auto& tex_name : dynamic_textures) {
            if (try_add_to_runtime_atlas(runtime_atlas, tex_name, 48, 48)) {
                core::Log::info("Runtime", "Added {} to atlas at ({}, {})",
                               tex_name, runtime_atlas.regions[tex_name].x, 
                               runtime_atlas.regions[tex_name].y);
            } else {
                core::Log::info("Runtime", "Atlas resize required for {}", tex_name);
                resize_runtime_atlas(runtime_atlas);
                
                if (try_add_to_runtime_atlas(runtime_atlas, tex_name, 48, 48)) {
                    core::Log::info("Runtime", "Added {} after resize", tex_name);
                }
            }
        }
        
        core::Log::info("Runtime", "Final atlas size: {}x{}, {} textures",
                       runtime_atlas.current_width, runtime_atlas.current_height,
                       runtime_atlas.regions.size());
    }
    
    //=========================================================================
    // Scene Creation Functions
    //=========================================================================
    
    void create_individual_texture_scene() {
        clear_entities();
        
        const u32 sprite_count = 200;
        const u32 texture_count = individual_textures_.size();
        
        for (u32 i = 0; i < sprite_count; ++i) {
            u32 entity = registry_->create_entity();
            sprite_entities_.push_back(entity);
            
            // Position in grid
            f32 x = (i % 20 - 10) * 60.0f;
            f32 y = (i / 20 - 5) * 60.0f;
            
            Transform transform;
            transform.position = {x, y, static_cast<f32>(i % 10)};
            transform.scale = {40.0f, 40.0f, 1.0f};
            registry_->add_component(entity, transform);
            
            // Each sprite uses different texture (worst case for batching)
            const auto& tex = individual_textures_[i % texture_count];
            RenderableSprite sprite;
            sprite.texture = TextureHandle{i % texture_count + 1, static_cast<u16>(tex.width), 
                                          static_cast<u16>(tex.height)};
            sprite.color_modulation = tex.color;
            sprite.z_order = static_cast<f32>(i % 10);
            registry_->add_component(entity, sprite);
        }
        
        core::Log::info("Scene", "Created individual texture scene: {} sprites, {} different textures",
                       sprite_count, texture_count);
    }
    
    void create_atlas_texture_scene() {
        clear_entities();
        
        const u32 sprite_count = 200;
        const auto& atlas = texture_atlases_.at("bin");
        
        for (u32 i = 0; i < sprite_count; ++i) {
            u32 entity = registry_->create_entity();
            sprite_entities_.push_back(entity);
            
            // Position in grid
            f32 x = (i % 20 - 10) * 60.0f;
            f32 y = (i / 20 - 5) * 60.0f;
            
            Transform transform;
            transform.position = {x, y, static_cast<f32>(i % 10)};
            transform.scale = {40.0f, 40.0f, 1.0f};
            registry_->add_component(entity, transform);
            
            // All sprites use the same atlas texture (optimal batching)
            RenderableSprite sprite;
            sprite.texture = TextureHandle{1000, static_cast<u16>(atlas.width), 
                                          static_cast<u16>(atlas.height)}; // Atlas texture ID
            
            // Get random atlas region for variety
            auto region_it = atlas.regions.begin();
            std::advance(region_it, i % atlas.regions.size());
            sprite.uv_rect = region_it->second.uv_rect;
            
            sprite.color_modulation = Color::white();
            sprite.z_order = static_cast<f32>(i % 10);
            registry_->add_component(entity, sprite);
        }
        
        core::Log::info("Scene", "Created atlas texture scene: {} sprites, 1 atlas texture", sprite_count);
    }
    
    void create_no_atlas_scene() {
        // Same as individual texture scene but with emphasis on no batching
        create_individual_texture_scene();
    }
    
    void create_atlas_scene(const std::string& atlas_name) {
        clear_entities();
        
        const auto& atlas = texture_atlases_.at(atlas_name);
        const u32 sprite_count = 150;
        
        for (u32 i = 0; i < sprite_count; ++i) {
            u32 entity = registry_->create_entity();
            sprite_entities_.push_back(entity);
            
            f32 x = (i % 15 - 7) * 70.0f;
            f32 y = (i / 15 - 5) * 70.0f;
            
            Transform transform;
            transform.position = {x, y, static_cast<f32>(i % 5)};
            transform.scale = {50.0f, 50.0f, 1.0f};
            registry_->add_component(entity, transform);
            
            RenderableSprite sprite;
            sprite.texture = TextureHandle{2000 + static_cast<u32>(atlas_name.length()), 
                                          static_cast<u16>(atlas.width), 
                                          static_cast<u16>(atlas.height)};
            
            // Use atlas regions
            auto region_it = atlas.regions.begin();
            std::advance(region_it, i % atlas.regions.size());
            sprite.uv_rect = region_it->second.uv_rect;
            
            sprite.color_modulation = Color::white();
            sprite.z_order = static_cast<f32>(i % 5);
            registry_->add_component(entity, sprite);
        }
        
        core::Log::info("Scene", "Created {} atlas scene: {} sprites", atlas_name, sprite_count);
    }
    
    //=========================================================================
    // Performance Measurement and Analysis
    //=========================================================================
    
    struct PerformanceMetrics {
        f32 average_fps{0.0f};
        f32 average_frame_time_ms{0.0f};
        u32 average_draw_calls{0};
        f32 batching_efficiency{0.0f};
        usize memory_usage{0};
    };
    
    PerformanceMetrics measure_rendering_performance(const std::string& test_name, u32 frames) {
        PerformanceMetrics metrics;
        f32 total_frame_time = 0.0f;
        u32 total_draw_calls = 0;
        f32 total_efficiency = 0.0f;
        
        for (u32 frame = 0; frame < frames; ++frame) {
            auto frame_start = std::chrono::high_resolution_clock::now();
            
            renderer_->begin_frame();
            renderer_->set_active_camera(camera_);
            renderer_->render_entities(*registry_);
            renderer_->end_frame();
            
            window_->swap_buffers();
            window_->poll_events();
            
            auto frame_end = std::chrono::high_resolution_clock::now();
            f32 frame_time = std::chrono::duration<f32>(frame_end - frame_start).count();
            total_frame_time += frame_time;
            
            const auto& stats = renderer_->get_statistics();
            total_draw_calls += stats.gpu_stats.draw_calls;
            total_efficiency += stats.gpu_stats.batching_efficiency;
        }
        
        metrics.average_frame_time_ms = (total_frame_time / frames) * 1000.0f;
        metrics.average_fps = 1.0f / (total_frame_time / frames);
        metrics.average_draw_calls = total_draw_calls / frames;
        metrics.batching_efficiency = total_efficiency / frames;
        
        const auto& final_stats = renderer_->get_statistics();
        metrics.memory_usage = final_stats.gpu_stats.total_gpu_memory;
        
        return metrics;
    }
    
    void analyze_batching_results() {
        core::Log::info("Analysis", "=== BATCHING EFFICIENCY ANALYSIS ===");
        
        if (batching_test_results_.count("No Atlas") && batching_test_results_.count("Optimal Atlas")) {
            const auto& no_atlas = batching_test_results_["No Atlas"];
            const auto& optimal = batching_test_results_["Optimal Atlas"];
            
            f32 fps_improvement = optimal.average_fps / no_atlas.average_fps;
            f32 draw_call_reduction = static_cast<f32>(no_atlas.average_draw_calls) / 
                                     static_cast<f32>(optimal.average_draw_calls);
            
            core::Log::info("Improvement", "Atlas vs No Atlas:");
            core::Log::info("Improvement", "  FPS: {:.1f} → {:.1f} ({:.2f}x improvement)",
                           no_atlas.average_fps, optimal.average_fps, fps_improvement);
            core::Log::info("Improvement", "  Draw calls: {} → {} ({:.1f}x reduction)",
                           no_atlas.average_draw_calls, optimal.average_draw_calls, draw_call_reduction);
            core::Log::info("Improvement", "  Batching efficiency: {:.1f}% → {:.1f}%",
                           no_atlas.batching_efficiency * 100.0f, optimal.batching_efficiency * 100.0f);
        }
        
        // Find best performing atlas strategy
        f32 best_fps = 0.0f;
        std::string best_strategy;
        
        for (const auto& [name, metrics] : batching_test_results_) {
            if (name != "No Atlas" && metrics.average_fps > best_fps) {
                best_fps = metrics.average_fps;
                best_strategy = name;
            }
        }
        
        core::Log::info("Best Strategy", "{} achieved highest performance: {:.1f} FPS", 
                       best_strategy, best_fps);
    }
    
    //=========================================================================
    // Support Functions
    //=========================================================================
    
    void clear_entities() {
        sprite_entities_.clear();
        registry_ = std::make_unique<ecs::Registry>();
    }
    
    void explain_packing_algorithms() {
        core::Log::info("Education", "=== ATLAS PACKING ALGORITHMS ===");
        
        core::Log::info("Shelf Packing", "Simple algorithm that packs textures in horizontal shelves");
        core::Log::info("Shelf Packing", "  Pros: Simple, fast, predictable memory access");
        core::Log::info("Shelf Packing", "  Cons: Can waste space with different texture heights");
        
        core::Log::info("Bin Packing", "Treats atlas as 2D bin packing problem");
        core::Log::info("Bin Packing", "  Pros: Better space efficiency, handles varied sizes well");
        core::Log::info("Bin Packing", "  Cons: More complex, can fragment space");
        
        core::Log::info("Optimal Sizing", "Calculates minimal atlas size before packing");
        core::Log::info("Optimal Sizing", "  Pros: Minimizes memory usage, reduces waste");
        core::Log::info("Optimal Sizing", "  Cons: Requires analysis pass, may need power-of-2 adjustment");
    }
    
    void demonstrate_compression_techniques() {
        core::Log::info("Compression", "=== TEXTURE COMPRESSION TECHNIQUES ===");
        
        // Simulate compression analysis
        for (const auto& [name, atlas] : texture_atlases_) {
            usize uncompressed_size = atlas.memory_usage_bytes;
            
            // Simulate different compression ratios
            usize dxt1_size = uncompressed_size / 8;    // DXT1: 8:1 compression
            usize dxt5_size = uncompressed_size / 4;    // DXT5: 4:1 compression
            usize bc7_size = uncompressed_size / 4;     // BC7: 4:1 compression (higher quality)
            
            core::Log::info("Compression", "{} atlas compression options:", atlas.name);
            core::Log::info("Option", "  Uncompressed: {} KB", uncompressed_size / 1024);
            core::Log::info("Option", "  DXT1: {} KB ({:.1f}% size, no alpha)",
                           dxt1_size / 1024, (static_cast<f32>(dxt1_size) / uncompressed_size) * 100.0f);
            core::Log::info("Option", "  DXT5: {} KB ({:.1f}% size, with alpha)",
                           dxt5_size / 1024, (static_cast<f32>(dxt5_size) / uncompressed_size) * 100.0f);
            core::Log::info("Option", "  BC7: {} KB ({:.1f}% size, high quality)",
                           bc7_size / 1024, (static_cast<f32>(bc7_size) / uncompressed_size) * 100.0f);
        }
    }
    
    void demonstrate_mipmapping_considerations() {
        core::Log::info("Mipmapping", "=== MIPMAPPING WITH TEXTURE ATLASES ===");
        
        core::Log::info("Challenge", "Atlas mipmapping is complex due to UV bleeding");
        core::Log::info("Challenge", "Adjacent textures in atlas can bleed into each other at lower mip levels");
        
        core::Log::info("Solution", "Padding between atlas regions prevents bleeding");
        core::Log::info("Solution", "Typical padding: 1-2 pixels for each mip level");
        core::Log::info("Solution", "Alternative: Generate separate mipmaps for atlas regions");
        
        // Calculate padding requirements
        for (const auto& [name, atlas] : texture_atlases_) {
            i32 mip_levels = static_cast<i32>(std::log2(std::max(atlas.width, atlas.height))) + 1;
            i32 padding_needed = 1 << (mip_levels - 1); // Padding for deepest mip
            
            core::Log::info("Mipmap", "{}: {} mip levels, {} pixel padding needed",
                           atlas.name, mip_levels, padding_needed);
        }
    }
    
    bool try_add_to_runtime_atlas(RuntimeAtlas& atlas, const std::string& name, i32 width, i32 height) {
        // Simplified runtime packing - just check if it fits
        // In real implementation, would use proper dynamic packing algorithm
        
        i32 next_x = 0;
        i32 next_y = 0;
        
        // Find next available position (simplified)
        for (const auto& [tex_name, region] : atlas.regions) {
            next_x = std::max(next_x, region.x + region.width);
            next_y = std::max(next_y, region.y + region.height);
        }
        
        if (next_x + width <= atlas.current_width && next_y + height <= atlas.current_height) {
            AtlasRegion region;
            region.texture_name = name;
            region.x = next_x;
            region.y = next_y;
            region.width = width;
            region.height = height;
            region.uv_rect = UVRect{
                static_cast<f32>(next_x) / atlas.current_width,
                static_cast<f32>(next_y) / atlas.current_height,
                static_cast<f32>(width) / atlas.current_width,
                static_cast<f32>(height) / atlas.current_height
            };
            
            atlas.regions[name] = region;
            return true;
        }
        
        return false;
    }
    
    void resize_runtime_atlas(RuntimeAtlas& atlas) {
        i32 new_width = std::min(atlas.current_width * 2, atlas.max_width);
        i32 new_height = std::min(atlas.current_height * 2, atlas.max_height);
        
        core::Log::info("Runtime", "Resizing atlas from {}x{} to {}x{}",
                       atlas.current_width, atlas.current_height, new_width, new_height);
        
        // Recalculate UV coordinates for existing regions
        for (auto& [name, region] : atlas.regions) {
            region.uv_rect = UVRect{
                static_cast<f32>(region.x) / new_width,
                static_cast<f32>(region.y) / new_height,
                static_cast<f32>(region.width) / new_width,
                static_cast<f32>(region.height) / new_height
            };
        }
        
        atlas.current_width = new_width;
        atlas.current_height = new_height;
    }
    
    const char* get_algorithm_name(AtlasPackingAlgorithm algorithm) {
        switch (algorithm) {
            case AtlasPackingAlgorithm::ShelfPacking: return "Shelf Packing";
            case AtlasPackingAlgorithm::BinPacking: return "Bin Packing";
            case AtlasPackingAlgorithm::OptimalSizeBinPacking: return "Optimal Size Bin Packing";
            default: return "Unknown";
        }
    }
    
    void display_educational_summary() {
        std::cout << "\n=== TEXTURE ATLASING TUTORIAL SUMMARY ===\n\n";
        
        std::cout << "KEY CONCEPTS LEARNED:\n\n";
        
        std::cout << "1. TEXTURE ATLAS BENEFITS:\n";
        std::cout << "   - Reduces draw calls by enabling sprite batching\n";
        std::cout << "   - Minimizes texture binding state changes\n";
        std::cout << "   - Improves GPU cache utilization\n";
        std::cout << "   - Reduces memory fragmentation\n\n";
        
        std::cout << "2. PACKING ALGORITHMS:\n";
        std::cout << "   - Shelf Packing: Simple horizontal strips, wastes vertical space\n";
        std::cout << "   - Bin Packing: 2D rectangle packing, better space efficiency\n";
        std::cout << "   - Optimal Sizing: Calculates minimal atlas dimensions\n\n";
        
        std::cout << "3. UV COORDINATE MAPPING:\n";
        std::cout << "   - Atlas regions map to normalized UV coordinates (0-1)\n";
        std::cout << "   - UV = pixel_position / atlas_dimensions\n";
        std::cout << "   - Sprites use sub-rectangles of the atlas texture\n\n";
        
        std::cout << "4. PERFORMANCE IMPACT:\n";
        if (!performance_comparisons_.empty()) {
            const auto& comparison = performance_comparisons_.begin()->second;
            std::cout << "   - Atlas vs Individual: " << comparison.fps_improvement << "x FPS improvement\n";
            std::cout << "   - Draw call reduction: " << comparison.draw_call_improvement << "x fewer calls\n";
        }
        std::cout << "   - Memory usage varies by packing efficiency\n";
        std::cout << "   - Best performance achieved with single atlas texture\n\n";
        
        std::cout << "5. OPTIMIZATION TECHNIQUES:\n";
        std::cout << "   - Texture compression (DXT1/5, BC7) reduces memory 4-8x\n";
        std::cout << "   - Padding prevents mipmap bleeding between regions\n";
        std::cout << "   - Power-of-2 dimensions improve GPU compatibility\n";
        std::cout << "   - Runtime atlases enable dynamic content loading\n\n";
        
        std::cout << "PRACTICAL APPLICATIONS:\n";
        std::cout << "- Game sprite sheets and animation frames\n";
        std::cout << "- UI element collections (buttons, panels, icons)\n";
        std::cout << "- Font glyph atlases for text rendering\n";
        std::cout << "- Particle effect texture collections\n";
        std::cout << "- Tile sets for 2D tile-based games\n\n";
        
        std::cout << "ATLAS CREATION WORKFLOW:\n";
        std::cout << "1. Collect all textures that will be used together\n";
        std::cout << "2. Choose appropriate packing algorithm for content type\n";
        std::cout << "3. Calculate optimal atlas size with padding considerations\n";
        std::cout << "4. Pack textures and generate UV coordinate mapping\n";
        std::cout << "5. Apply compression appropriate for content and platform\n";
        std::cout << "6. Update sprite assets to use atlas UV coordinates\n\n";
        
        std::cout << "NEXT TUTORIAL: Particle Systems and Visual Effects\n\n";
    }
    
    void cleanup() {
        if (renderer_) renderer_->shutdown();
        if (window_) window_->shutdown();
    }
    
    //=========================================================================
    // Data Structures
    //=========================================================================
    
    enum class AtlasPackingAlgorithm {
        ShelfPacking,
        BinPacking,
        OptimalSizeBinPacking
    };
    
    struct IndividualTexture {
        std::string name;
        i32 width, height;
        Color color; // Simulated texture color for demo
    };
    
    struct AtlasRegion {
        std::string texture_name;
        i32 x, y, width, height;
        UVRect uv_rect;
    };
    
    struct TextureAtlas {
        std::string name;
        std::string description;
        i32 width, height;
        AtlasPackingAlgorithm packing_algorithm;
        std::unordered_map<std::string, AtlasRegion> regions;
        
        // Statistics
        f32 space_efficiency{0.0f};
        usize wasted_space_bytes{0};
        usize memory_usage_bytes{0};
    };
    
    struct PackingRect {
        i32 x, y, width, height;
    };
    
    struct RuntimeAtlas {
        std::string name;
        i32 current_width, current_height;
        i32 max_width, max_height;
        std::unordered_map<std::string, AtlasRegion> regions;
    };
    
    struct PerformanceComparison {
        PerformanceMetrics individual_performance;
        PerformanceMetrics atlas_performance;
        f32 fps_improvement;
        f32 draw_call_improvement;
    };
    
    // Tutorial resources
    std::unique_ptr<Window> window_;
    std::unique_ptr<Renderer2D> renderer_;
    std::unique_ptr<ecs::Registry> registry_;
    Camera2D camera_;
    
    // Demo entities
    std::vector<u32> sprite_entities_;
    
    // Texture data
    std::vector<IndividualTexture> individual_textures_;
    std::unordered_map<std::string, TextureAtlas> texture_atlases_;
    
    // Performance tracking
    std::unordered_map<std::string, PerformanceMetrics> batching_test_results_;
    std::unordered_map<std::string, PerformanceComparison> performance_comparisons_;
};

//=============================================================================
// Main Function
//=============================================================================

int main() {
    core::Log::info("Main", "Starting Texture Atlasing and Optimization Tutorial");
    
    std::cout << "\n=== WELCOME TO TUTORIAL 5: TEXTURE ATLASING AND OPTIMIZATION ===\n";
    std::cout << "This tutorial provides comprehensive coverage of texture atlasing techniques\n";
    std::cout << "and optimization strategies for high-performance 2D rendering.\n\n";
    std::cout << "You will learn:\n";
    std::cout << "- Texture atlas concepts and batching benefits\n";
    std::cout << "- UV coordinate mapping and atlas region management\n";
    std::cout << "- Atlas packing algorithms and space efficiency\n";
    std::cout << "- Performance measurement and optimization techniques\n";
    std::cout << "- Memory usage analysis and compression strategies\n";
    std::cout << "- Runtime atlas generation for dynamic content\n\n";
    std::cout << "Watch for detailed performance comparisons and practical optimization tips.\n\n";
    
    TextureAtlasingTutorial tutorial;
    
    if (!tutorial.initialize()) {
        core::Log::error("Main", "Failed to initialize tutorial");
        return -1;
    }
    
    tutorial.run();
    
    core::Log::info("Main", "Texture Atlasing Tutorial completed successfully!");
    return 0;
}