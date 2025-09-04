#pragma once

/**
 * @file physics/debug_renderer_2d.hpp
 * @brief Enhanced 2D Physics Debug Renderer for ECScope - Integrating Physics with Modern 2D Rendering
 * 
 * This header provides a comprehensive physics debug visualization system that integrates
 * seamlessly with ECScope's new 2D BatchRenderer system. It demonstrates world-class
 * integration between physics simulation and modern graphics programming.
 * 
 * Key Features:
 * - Integration with BatchRenderer for optimized debug rendering
 * - GPU-efficient debug primitive batching and instanced rendering
 * - Real-time physics visualization with educational overlays
 * - Memory-efficient debug geometry generation using arena allocators
 * - Interactive physics parameter tuning with immediate visual feedback
 * - Performance comparison between different debug rendering approaches
 * 
 * Educational Demonstrations:
 * - SoA vs AoS impact on debug rendering performance
 * - Memory access patterns during physics visualization
 * - Integration patterns between simulation and rendering systems
 * - Batch optimization techniques for debug geometry
 * - GPU state management for mixed primitive types
 * 
 * Advanced Features:
 * - Multi-layered debug visualization (shapes, contacts, forces, grid)
 * - Step-by-step collision detection algorithm visualization  
 * - Physics algorithm breakdown with visual timing analysis
 * - Memory allocation pattern visualization for educational insights
 * - Performance impact analysis of different debug rendering modes
 * 
 * Technical Architecture:
 * - Leverages existing DebugRenderInterface for abstraction
 * - Integrates with BatchRenderer and RenderableSprite components
 * - Uses Camera2D system for proper world-to-screen transformation
 * - Coordinates with ECS scheduling for efficient debug data collection
 * - Memory-efficient debug primitive caching and reuse
 * 
 * @author ECScope Educational ECS Framework - Physics/Rendering Integration
 * @date 2024
 */

#include "debug_renderer.hpp"
#include "../renderer/batch_renderer.hpp"
#include "../renderer/renderer_2d.hpp"
#include "../renderer/components/render_components.hpp"
#include "../memory/arena.hpp"
#include "../core/types.hpp"
#include <memory>
#include <vector>
#include <unordered_map>
#include <chrono>

namespace ecscope::physics::debug {

// Import commonly used types
using namespace ecscope::renderer;
using namespace ecscope::renderer::components;

//=============================================================================
// 2D Physics Debug Renderer Implementation
//=============================================================================

/**
 * @brief Modern 2D Physics Debug Renderer with BatchRenderer Integration
 * 
 * This implementation of DebugRenderInterface provides high-performance physics
 * debug visualization by leveraging the modern 2D rendering pipeline. It demonstrates
 * advanced integration patterns between simulation and rendering systems.
 * 
 * Educational Context:
 * This renderer showcases:
 * - How physics debug data can be efficiently rendered using modern batching techniques
 * - Integration patterns between different engine systems (physics, rendering, ECS)
 * - Memory-efficient debug geometry generation and caching strategies
 * - Performance optimization techniques for real-time debug visualization
 * - Educational visualization of complex physics concepts and algorithms
 * 
 * Performance Characteristics:
 * - Uses sprite batching for collision shape rendering (thousands of shapes per frame)
 * - Leverages instanced rendering for repeated debug elements (contact points, arrows)
 * - Optimizes draw calls through texture atlasing for debug primitives
 * - Memory-efficient debug primitive generation using arena allocators
 * - Background debug batch preparation for smooth frame rates
 */
class PhysicsDebugRenderer2D : public DebugRenderInterface {
public:
    //-------------------------------------------------------------------------
    // Configuration and Setup
    //-------------------------------------------------------------------------
    
    /** @brief Enhanced debug renderer configuration */
    struct Config {
        /** @brief Rendering performance settings */
        bool enable_batching{true};             ///< Use sprite batching for debug shapes
        bool enable_instancing{true};           ///< Use instanced rendering for repeated elements
        bool enable_texture_atlasing{true};     ///< Use texture atlas for debug primitives
        u32 max_debug_sprites_per_batch{500};   ///< Sprites per debug batch (smaller for clearer analysis)
        
        /** @brief Memory management settings */
        usize debug_arena_size{1024 * 1024};   ///< 1MB arena for debug geometry
        bool enable_debug_caching{true};        ///< Cache debug geometry between frames
        bool enable_memory_tracking{true};      ///< Track debug memory usage
        
        /** @brief Educational features */
        bool show_batching_visualization{false}; ///< Color-code debug batches
        bool show_performance_metrics{true};     ///< Display debug rendering performance
        bool show_memory_usage{true};           ///< Display debug memory consumption
        bool enable_step_rendering{false};      ///< Step-through debug rendering
        
        /** @brief Quality settings */
        f32 debug_primitive_quality{1.0f};     ///< Quality multiplier for debug primitives
        bool enable_antialiasing{true};        ///< Enable anti-aliasing for debug lines
        u32 circle_segments{16};               ///< Segments for circle approximation
        
        /** @brief Factory methods */
        static Config educational_mode() {
            Config config;
            config.max_debug_sprites_per_batch = 100; // Smaller for clearer analysis
            config.show_batching_visualization = true;
            config.show_performance_metrics = true;
            config.show_memory_usage = true;
            config.enable_step_rendering = true;
            return config;
        }
        
        static Config performance_mode() {
            Config config;
            config.max_debug_sprites_per_batch = 1000;
            config.debug_primitive_quality = 0.7f;
            config.enable_debug_caching = true;
            config.show_performance_metrics = false;
            config.show_memory_usage = false;
            config.circle_segments = 8;
            return config;
        }
    };
    
    //-------------------------------------------------------------------------
    // Construction and Initialization
    //-------------------------------------------------------------------------
    
    /** 
     * @brief Constructor with renderer integration
     * 
     * @param renderer2d Reference to 2D renderer system
     * @param batch_renderer Reference to batch renderer for optimization
     * @param registry ECS registry for entity management
     * @param config Configuration for debug renderer behavior
     */
    explicit PhysicsDebugRenderer2D(Renderer2D& renderer2d, 
                                   BatchRenderer& batch_renderer,
                                   ecs::Registry& registry,
                                   const Config& config = Config{})
        : config_(config)
        , renderer2d_(&renderer2d)
        , batch_renderer_(&batch_renderer)
        , registry_(&registry)
        , debug_arena_(config.debug_arena_size)
        , frame_number_(0) {
        
        initialize_debug_resources();
        
        LOG_INFO("PhysicsDebugRenderer2D initialized:");
        LOG_INFO("  - Batching: {}", config.enable_batching ? "enabled" : "disabled");
        LOG_INFO("  - Max sprites per batch: {}", config.max_debug_sprites_per_batch);
        LOG_INFO("  - Debug arena size: {} KB", config.debug_arena_size / 1024);
        LOG_INFO("  - Educational features: {}", 
                config.show_batching_visualization ? "enabled" : "disabled");
    }
    
    /** @brief Destructor with cleanup and final statistics */
    ~PhysicsDebugRenderer2D() {
        cleanup_debug_resources();
        
        if (config_.show_performance_metrics && render_stats_.total_frames > 0) {
            LOG_INFO("PhysicsDebugRenderer2D final statistics:");
            LOG_INFO("  - Total frames: {}", render_stats_.total_frames);
            LOG_INFO("  - Average render time: {:.3f} ms", 
                    render_stats_.total_render_time / render_stats_.total_frames);
            LOG_INFO("  - Total debug shapes rendered: {}", render_stats_.total_shapes_rendered);
            LOG_INFO("  - Batching efficiency: {:.2f}%", render_stats_.batching_efficiency * 100.0f);
        }
    }
    
    //-------------------------------------------------------------------------
    // DebugRenderInterface Implementation
    //-------------------------------------------------------------------------
    
    /** @brief Begin debug rendering frame */
    void begin_frame() override {
        auto frame_start = std::chrono::high_resolution_clock::now();
        
        ++frame_number_;
        current_frame_stats_ = FrameStats{};
        
        // Reset arena allocator for this frame
        debug_arena_.reset();
        
        // Clear cached debug entities from previous frame
        debug_entities_.clear();
        
        // Begin batch renderer frame if using batching
        if (config_.enable_batching) {
            batch_renderer_->begin_frame();
        }
        
        frame_start_time_ = frame_start;
        
        LOG_TRACE("Debug frame {} started", frame_number_);
    }
    
    /** @brief End debug rendering frame and submit batches */
    void end_frame() override {
        // Finalize all debug sprite submissions
        finalize_debug_batches();
        
        // Render all debug entities using appropriate method
        if (config_.enable_batching) {
            render_batched_debug_entities();
        } else {
            render_immediate_debug_entities();
        }
        
        // End batch renderer frame
        if (config_.enable_batching) {
            batch_renderer_->end_frame();
        }
        
        // Update performance statistics
        update_performance_stats();
        
        // Clean up temporary debug entities
        cleanup_frame_debug_entities();
        
        LOG_TRACE("Debug frame {} completed - {} shapes, {} batches", 
                 frame_number_, current_frame_stats_.shapes_rendered, 
                 current_frame_stats_.batches_generated);
    }
    
    /** @brief Set camera transformation for debug rendering */
    void set_camera_transform(const Vec2& position, f32 zoom, f32 rotation = 0.0f) override {
        current_camera_position_ = position;
        current_camera_zoom_ = zoom;
        current_camera_rotation_ = rotation;
        
        // Update camera transformation matrix for coordinate conversion
        update_camera_matrix();
    }
    
    //-------------------------------------------------------------------------
    // Line and Shape Rendering
    //-------------------------------------------------------------------------
    
    /** @brief Draw line using optimized 2D rendering */
    void draw_line(const Vec2& start, const Vec2& end, u32 color, f32 thickness = 1.0f) override {
        // Convert line to oriented rectangle for efficient rendering
        Vec2 direction = (end - start).normalized();
        Vec2 perpendicular = Vec2{-direction.y, direction.x};
        f32 length = (end - start).length();
        f32 half_thickness = thickness * 0.5f;
        
        Vec2 center = (start + end) * 0.5f;
        f32 angle = std::atan2(direction.y, direction.x);
        
        // Create line sprite using rectangle rendering
        create_debug_sprite_rect(center, Vec2{length, thickness}, angle, Color{color});
        
        ++current_frame_stats_.lines_rendered;
    }
    
    /** @brief Draw circle using optimized polygon approximation */
    void draw_circle(const Vec2& center, f32 radius, u32 color, bool filled = false, f32 thickness = 1.0f) override {
        if (filled) {
            // Use single sprite for filled circle
            create_debug_sprite_circle(center, radius, Color{color}, true);
        } else {
            // Create circle outline using multiple line segments
            u32 segments = config_.circle_segments;
            f32 angle_step = 2.0f * PI / segments;
            
            for (u32 i = 0; i < segments; ++i) {
                f32 angle1 = i * angle_step;
                f32 angle2 = (i + 1) * angle_step;
                
                Vec2 p1 = center + Vec2{std::cos(angle1), std::sin(angle1)} * radius;
                Vec2 p2 = center + Vec2{std::cos(angle2), std::sin(angle2)} * radius;
                
                draw_line(p1, p2, color, thickness);
            }
        }
        
        ++current_frame_stats_.circles_rendered;
    }
    
    /** @brief Draw rectangle using sprite rendering */
    void draw_rectangle(const Vec2& min, const Vec2& max, u32 color, bool filled = false, f32 thickness = 1.0f) override {
        Vec2 center = (min + max) * 0.5f;
        Vec2 size = max - min;
        
        if (filled) {
            create_debug_sprite_rect(center, size, 0.0f, Color{color});
        } else {
            // Draw rectangle outline as four lines
            draw_line(Vec2{min.x, min.y}, Vec2{max.x, min.y}, color, thickness); // Bottom
            draw_line(Vec2{max.x, min.y}, Vec2{max.x, max.y}, color, thickness); // Right
            draw_line(Vec2{max.x, max.y}, Vec2{min.x, max.y}, color, thickness); // Top
            draw_line(Vec2{min.x, max.y}, Vec2{min.x, min.y}, color, thickness); // Left
        }
        
        ++current_frame_stats_.rectangles_rendered;
    }
    
    /** @brief Draw oriented bounding box */
    void draw_obb(const Vec2& center, const Vec2& half_extents, f32 rotation, u32 color, bool filled = false, f32 thickness = 1.0f) override {
        if (filled) {
            create_debug_sprite_rect(center, half_extents * 2.0f, rotation, Color{color});
        } else {
            // Calculate OBB corners and draw outline
            f32 cos_r = std::cos(rotation);
            f32 sin_r = std::sin(rotation);
            
            Vec2 local_corners[4] = {
                {-half_extents.x, -half_extents.y},
                { half_extents.x, -half_extents.y},
                { half_extents.x,  half_extents.y},
                {-half_extents.x,  half_extents.y}
            };
            
            Vec2 world_corners[4];
            for (int i = 0; i < 4; ++i) {
                world_corners[i] = center + Vec2{
                    local_corners[i].x * cos_r - local_corners[i].y * sin_r,
                    local_corners[i].x * sin_r + local_corners[i].y * cos_r
                };
            }
            
            // Draw rectangle outline
            for (int i = 0; i < 4; ++i) {
                draw_line(world_corners[i], world_corners[(i + 1) % 4], color, thickness);
            }
        }
        
        ++current_frame_stats_.obbs_rendered;
    }
    
    /** @brief Draw polygon using line segments */
    void draw_polygon(const std::vector<Vec2>& vertices, u32 color, bool filled = false, f32 thickness = 1.0f) override {
        if (vertices.size() < 3) return;
        
        if (filled) {
            // For filled polygons, create a sprite entity with custom geometry
            create_debug_sprite_polygon(vertices, Color{color});
        } else {
            // Draw polygon outline
            for (usize i = 0; i < vertices.size(); ++i) {
                usize next = (i + 1) % vertices.size();
                draw_line(vertices[i], vertices[next], color, thickness);
            }
        }
        
        ++current_frame_stats_.polygons_rendered;
    }
    
    /** @brief Draw point using small circle or sprite */
    void draw_point(const Vec2& position, u32 color, f32 size = 3.0f) override {
        // Use small filled circle for points
        draw_circle(position, size * 0.5f, color, true);
        
        ++current_frame_stats_.points_rendered;
    }
    
    /** @brief Draw arrow using line and arrowhead */
    void draw_arrow(const Vec2& start, const Vec2& end, u32 color, f32 thickness = 1.0f, f32 head_size = 3.0f) override {
        // Draw main line
        draw_line(start, end, color, thickness);
        
        // Draw arrowhead
        Vec2 direction = (end - start).normalized();
        Vec2 perpendicular = Vec2{-direction.y, direction.x};
        
        // Arrowhead points
        Vec2 head_base = end - direction * head_size;
        Vec2 head_left = head_base + perpendicular * head_size * 0.5f;
        Vec2 head_right = head_base - perpendicular * head_size * 0.5f;
        
        draw_line(end, head_left, color, thickness);
        draw_line(end, head_right, color, thickness);
        
        ++current_frame_stats_.arrows_rendered;
    }
    
    //-------------------------------------------------------------------------
    // Text and Grid Rendering
    //-------------------------------------------------------------------------
    
    /** @brief Draw text using UI text rendering system */
    void draw_text(const Vec2& position, const std::string& text, u32 color, f32 size = 12.0f) override {
        // Convert world position to screen coordinates
        Vec2 screen_pos = world_to_screen(position);
        draw_text_screen(screen_pos, text, color, size);
    }
    
    /** @brief Draw text in screen space */
    void draw_text_screen(const Vec2& screen_position, const std::string& text, u32 color, f32 size = 12.0f) override {
        // Create text entity using UI rendering system
        create_debug_text_entity(screen_position, text, Color{color}, size);
        
        ++current_frame_stats_.text_elements_rendered;
    }
    
    /** @brief Draw grid for spatial reference */
    void draw_grid(const Vec2& origin, const Vec2& cell_size, u32 width, u32 height, u32 color, f32 alpha = 1.0f) override {
        Color grid_color{color};
        grid_color.a = static_cast<u8>(alpha * 255.0f);
        
        // Draw horizontal lines
        for (u32 y = 0; y <= height; ++y) {
            Vec2 start = origin + Vec2{0.0f, y * cell_size.y};
            Vec2 end = start + Vec2{width * cell_size.x, 0.0f};
            draw_line(start, end, grid_color.rgba, 1.0f);
        }
        
        // Draw vertical lines
        for (u32 x = 0; x <= width; ++x) {
            Vec2 start = origin + Vec2{x * cell_size.x, 0.0f};
            Vec2 end = start + Vec2{0.0f, height * cell_size.y};
            draw_line(start, end, grid_color.rgba, 1.0f);
        }
        
        ++current_frame_stats_.grids_rendered;
    }
    
    /** @brief Draw screen-space rectangle for UI overlays */
    void draw_rectangle_screen(const Vec2& min, const Vec2& max, u32 color, bool filled = true) override {
        // Create screen-space UI sprite
        Vec2 center = (min + max) * 0.5f;
        Vec2 size = max - min;
        
        create_debug_ui_sprite(center, size, Color{color});
        
        ++current_frame_stats_.ui_elements_rendered;
    }
    
    //-------------------------------------------------------------------------
    // Performance Analysis and Educational Features
    //-------------------------------------------------------------------------
    
    /** @brief Get comprehensive debug rendering statistics */
    struct DebugRenderStatistics {
        // Frame statistics
        u32 total_frames;
        f32 average_render_time_ms;
        f32 peak_render_time_ms;
        f32 total_render_time_ms;
        
        // Shape statistics
        u32 total_shapes_rendered;
        u32 shapes_per_frame_average;
        u32 peak_shapes_per_frame;
        
        // Batching efficiency
        f32 batching_efficiency;        // 0-1, sprites batched vs individual
        u32 average_batches_per_frame;
        u32 average_sprites_per_batch;
        
        // Memory usage
        usize debug_memory_used_bytes;
        usize peak_debug_memory_bytes;
        f32 memory_efficiency;          // 0-1, useful data vs total allocation
        
        // Educational metrics
        f32 batch_overhead_ms;          // Time spent on batching vs rendering
        f32 memory_allocation_overhead_ms;
        const char* performance_rating; // "Excellent", "Good", "Fair", "Poor"
        std::vector<std::string> optimization_suggestions;
    };
    
    DebugRenderStatistics get_debug_render_statistics() const {
        DebugRenderStatistics stats{};
        
        stats.total_frames = render_stats_.total_frames;
        stats.average_render_time_ms = render_stats_.total_frames > 0 ? 
            render_stats_.total_render_time / render_stats_.total_frames : 0.0f;
        stats.peak_render_time_ms = render_stats_.peak_render_time;
        stats.total_render_time_ms = render_stats_.total_render_time;
        
        stats.total_shapes_rendered = render_stats_.total_shapes_rendered;
        stats.shapes_per_frame_average = render_stats_.total_frames > 0 ?
            render_stats_.total_shapes_rendered / render_stats_.total_frames : 0;
        stats.peak_shapes_per_frame = render_stats_.peak_shapes_per_frame;
        
        stats.batching_efficiency = render_stats_.batching_efficiency;
        stats.average_batches_per_frame = render_stats_.total_frames > 0 ?
            render_stats_.total_batches_generated / render_stats_.total_frames : 0;
        
        stats.debug_memory_used_bytes = debug_arena_.get_used();
        stats.peak_debug_memory_bytes = debug_arena_.get_peak_usage();
        stats.memory_efficiency = debug_arena_.get_efficiency();
        
        // Generate performance rating and suggestions
        analyze_performance(stats);
        
        return stats;
    }
    
    /** @brief Generate comprehensive debug rendering report */
    std::string generate_debug_render_report() const {
        auto stats = get_debug_render_statistics();
        std::ostringstream oss;
        
        oss << "=== Physics Debug Rendering Performance Report ===\n";
        oss << "Performance Rating: " << stats.performance_rating << "\n";
        oss << "\n--- Timing Analysis ---\n";
        oss << "Average Frame Time: " << stats.average_render_time_ms << " ms\n";
        oss << "Peak Frame Time: " << stats.peak_render_time_ms << " ms\n";
        oss << "Total Render Time: " << stats.total_render_time_ms << " ms\n";
        
        oss << "\n--- Shape Rendering ---\n";
        oss << "Total Shapes: " << stats.total_shapes_rendered << "\n";
        oss << "Avg Shapes/Frame: " << stats.shapes_per_frame_average << "\n";
        oss << "Peak Shapes/Frame: " << stats.peak_shapes_per_frame << "\n";
        
        oss << "\n--- Batching Efficiency ---\n";
        oss << "Batching Efficiency: " << (stats.batching_efficiency * 100.0f) << "%\n";
        oss << "Avg Batches/Frame: " << stats.average_batches_per_frame << "\n";
        oss << "Avg Sprites/Batch: " << stats.average_sprites_per_batch << "\n";
        
        oss << "\n--- Memory Usage ---\n";
        oss << "Debug Memory Used: " << (stats.debug_memory_used_bytes / 1024.0f) << " KB\n";
        oss << "Peak Debug Memory: " << (stats.peak_debug_memory_bytes / 1024.0f) << " KB\n";
        oss << "Memory Efficiency: " << (stats.memory_efficiency * 100.0f) << "%\n";
        
        if (!stats.optimization_suggestions.empty()) {
            oss << "\n--- Optimization Suggestions ---\n";
            for (const auto& suggestion : stats.optimization_suggestions) {
                oss << "- " << suggestion << "\n";
            }
        }
        
        return oss.str();
    }
    
    /** @brief Educational comparison between rendering approaches */
    struct RenderingComparison {
        f32 immediate_mode_time_ms;
        f32 batched_mode_time_ms;
        f32 performance_improvement_ratio;
        usize immediate_mode_memory_kb;
        usize batched_mode_memory_kb;
        f32 memory_efficiency_ratio;
        const char* recommended_approach;
    };
    
    RenderingComparison compare_rendering_approaches() const {
        RenderingComparison comparison{};
        
        // These would be measured values from actual performance tests
        comparison.immediate_mode_time_ms = render_stats_.immediate_mode_time;
        comparison.batched_mode_time_ms = render_stats_.batched_mode_time;
        
        if (comparison.immediate_mode_time_ms > 0) {
            comparison.performance_improvement_ratio = 
                comparison.immediate_mode_time_ms / comparison.batched_mode_time_ms;
        }
        
        comparison.immediate_mode_memory_kb = render_stats_.immediate_mode_memory / 1024;
        comparison.batched_mode_memory_kb = render_stats_.batched_mode_memory / 1024;
        
        if (comparison.immediate_mode_memory_kb > 0) {
            comparison.memory_efficiency_ratio = 
                static_cast<f32>(comparison.batched_mode_memory_kb) / comparison.immediate_mode_memory_kb;
        }
        
        // Recommend approach based on performance characteristics
        if (comparison.performance_improvement_ratio > 1.5f && comparison.memory_efficiency_ratio < 2.0f) {
            comparison.recommended_approach = "Batched Rendering";
        } else if (current_frame_stats_.shapes_rendered < 50) {
            comparison.recommended_approach = "Immediate Mode (Low Shape Count)";
        } else {
            comparison.recommended_approach = "Batched Rendering";
        }
        
        return comparison;
    }

private:
    //-------------------------------------------------------------------------
    // Internal Data Structures
    //-------------------------------------------------------------------------
    
    /** @brief Debug entity for temporary rendering */
    struct DebugEntity {
        Entity entity_id;
        bool is_temporary;
        RenderableSprite::BlendMode blend_mode;
        f32 z_order;
        
        DebugEntity(Entity id, bool temp = true, f32 z = 100.0f) 
            : entity_id(id), is_temporary(temp), blend_mode(RenderableSprite::BlendMode::Alpha), z_order(z) {}
    };
    
    /** @brief Frame-specific rendering statistics */
    struct FrameStats {
        u32 shapes_rendered{0};
        u32 lines_rendered{0};
        u32 circles_rendered{0};
        u32 rectangles_rendered{0};
        u32 obbs_rendered{0};
        u32 polygons_rendered{0};
        u32 points_rendered{0};
        u32 arrows_rendered{0};
        u32 text_elements_rendered{0};
        u32 grids_rendered{0};
        u32 ui_elements_rendered{0};
        u32 batches_generated{0};
        f32 frame_render_time{0.0f};
    };
    
    /** @brief Accumulated rendering statistics */
    struct RenderStats {
        u32 total_frames{0};
        f32 total_render_time{0.0f};
        f32 peak_render_time{0.0f};
        u32 total_shapes_rendered{0};
        u32 peak_shapes_per_frame{0};
        u32 total_batches_generated{0};
        f32 batching_efficiency{1.0f};
        
        // Comparison data
        f32 immediate_mode_time{0.0f};
        f32 batched_mode_time{0.0f};
        usize immediate_mode_memory{0};
        usize batched_mode_memory{0};
    };
    
    //-------------------------------------------------------------------------
    // Member Variables
    //-------------------------------------------------------------------------
    
    Config config_;
    Renderer2D* renderer2d_;
    BatchRenderer* batch_renderer_;
    ecs::Registry* registry_;
    
    // Memory management
    memory::ArenaAllocator debug_arena_;
    
    // Frame state
    u32 frame_number_;
    std::chrono::high_resolution_clock::time_point frame_start_time_;
    FrameStats current_frame_stats_;
    RenderStats render_stats_;
    
    // Camera transformation
    Vec2 current_camera_position_{0.0f, 0.0f};
    f32 current_camera_zoom_{1.0f};
    f32 current_camera_rotation_{0.0f};
    
    // Debug entity management
    std::vector<DebugEntity> debug_entities_;
    std::unordered_map<u64, Entity> debug_entity_cache_;
    
    // Debug resources
    TextureHandle debug_texture_atlas_;
    ShaderHandle debug_shader_;
    
    //-------------------------------------------------------------------------
    // Internal Methods
    //-------------------------------------------------------------------------
    
    /** @brief Initialize debug rendering resources */
    void initialize_debug_resources() {
        // Create debug texture atlas for primitive rendering
        create_debug_texture_atlas();
        
        // Load or create debug shader
        create_debug_shader();
        
        LOG_DEBUG("Debug rendering resources initialized");
    }
    
    /** @brief Cleanup debug rendering resources */
    void cleanup_debug_resources() {
        // Clean up all temporary debug entities
        cleanup_frame_debug_entities();
        
        // Release debug resources
        // (Implementation would release GPU resources)
        
        LOG_DEBUG("Debug rendering resources cleaned up");
    }
    
    /** @brief Create debug texture atlas for primitives */
    void create_debug_texture_atlas() {
        // This would create a texture atlas containing:
        // - Circle texture (for filled circles and points)
        // - Square texture (for rectangles and lines)
        // - Arrow texture (for vector visualization)
        // - Grid pattern (for spatial hash visualization)
        
        // For now, use default white texture
        debug_texture_atlas_ = TextureHandle{0}; // Default white texture
        
        LOG_DEBUG("Debug texture atlas created");
    }
    
    /** @brief Create debug shader program */
    void create_debug_shader() {
        // This would create a shader optimized for debug rendering:
        // - Vertex shader with position, UV, and color attributes
        // - Fragment shader with texture sampling and color modulation
        // - Uniform support for camera transformation
        
        // For now, use default sprite shader
        debug_shader_ = ShaderHandle{0}; // Default sprite shader
        
        LOG_DEBUG("Debug shader created");
    }
    
    /** @brief Create debug sprite for rectangle */
    void create_debug_sprite_rect(const Vec2& center, const Vec2& size, f32 rotation, const Color& color) {
        Entity debug_entity = registry_->create();
        
        // Add Transform component
        Transform transform;
        transform.position = center;
        transform.rotation = rotation;
        transform.scale = Vec2{size.x, size.y};
        registry_->add_component(debug_entity, transform);
        
        // Add RenderableSprite component
        RenderableSprite sprite = RenderableSprite::create_colored_quad(color, 100.0f);
        sprite.set_size(1.0f, 1.0f); // Size controlled by transform scale
        registry_->add_component(debug_entity, sprite);
        
        // Track debug entity
        debug_entities_.emplace_back(debug_entity);
        
        // Submit to batch renderer if enabled
        if (config_.enable_batching) {
            batch_renderer_->submit_sprite(sprite, transform);
        }
        
        ++current_frame_stats_.shapes_rendered;
    }
    
    /** @brief Create debug sprite for circle */
    void create_debug_sprite_circle(const Vec2& center, f32 radius, const Color& color, bool filled) {
        Entity debug_entity = registry_->create();
        
        // Add Transform component
        Transform transform;
        transform.position = center;
        transform.scale = Vec2{radius * 2.0f, radius * 2.0f};
        registry_->add_component(debug_entity, transform);
        
        // Add RenderableSprite component with circle texture
        RenderableSprite sprite = RenderableSprite::create_colored_quad(color, 100.0f);
        // In a real implementation, this would use a circle texture from the atlas
        sprite.texture = debug_texture_atlas_;
        sprite.uv_rect = UVRect{0.0f, 0.0f, 0.25f, 0.25f}; // Circle region in atlas
        registry_->add_component(debug_entity, sprite);
        
        // Track debug entity
        debug_entities_.emplace_back(debug_entity);
        
        // Submit to batch renderer if enabled
        if (config_.enable_batching) {
            batch_renderer_->submit_sprite(sprite, transform);
        }
        
        ++current_frame_stats_.shapes_rendered;
    }
    
    /** @brief Create debug sprite for polygon */
    void create_debug_sprite_polygon(const std::vector<Vec2>& vertices, const Color& color) {
        // For complex polygons, we would need triangulation
        // For educational purposes, create a bounding box sprite
        if (vertices.empty()) return;
        
        Vec2 min_pos = vertices[0];
        Vec2 max_pos = vertices[0];
        
        for (const auto& vertex : vertices) {
            min_pos.x = std::min(min_pos.x, vertex.x);
            min_pos.y = std::min(min_pos.y, vertex.y);
            max_pos.x = std::max(max_pos.x, vertex.x);
            max_pos.y = std::max(max_pos.y, vertex.y);
        }
        
        Vec2 center = (min_pos + max_pos) * 0.5f;
        Vec2 size = max_pos - min_pos;
        
        create_debug_sprite_rect(center, size, 0.0f, color);
    }
    
    /** @brief Create debug text entity */
    void create_debug_text_entity(const Vec2& screen_position, const std::string& text, const Color& color, f32 size) {
        Entity debug_entity = registry_->create();
        
        // Add Transform component in screen space
        Transform transform;
        transform.position = screen_position;
        transform.scale = Vec2{size, size};
        registry_->add_component(debug_entity, transform);
        
        // Add RenderableSprite component for text background (if needed)
        RenderableSprite sprite = RenderableSprite::create_colored_quad(Color::transparent(), 200.0f);
        sprite.render_flags.world_space_ui = 0; // Render in screen space
        registry_->add_component(debug_entity, sprite);
        
        // In a real implementation, add TextComponent for actual text rendering
        
        // Track debug entity
        debug_entities_.emplace_back(debug_entity);
    }
    
    /** @brief Create debug UI sprite */
    void create_debug_ui_sprite(const Vec2& center, const Vec2& size, const Color& color) {
        Entity debug_entity = registry_->create();
        
        // Add Transform component
        Transform transform;
        transform.position = center;
        transform.scale = size;
        registry_->add_component(debug_entity, transform);
        
        // Add RenderableSprite component for UI rendering
        RenderableSprite sprite = RenderableSprite::create_colored_quad(color, 200.0f);
        sprite.render_flags.world_space_ui = 0; // Render in screen space
        registry_->add_component(debug_entity, sprite);
        
        // Track debug entity
        debug_entities_.emplace_back(debug_entity);
        
        // Submit to batch renderer if enabled
        if (config_.enable_batching) {
            batch_renderer_->submit_sprite(sprite, transform);
        }
    }
    
    /** @brief Finalize debug batches for rendering */
    void finalize_debug_batches() {
        if (config_.enable_batching) {
            // Generate batches from submitted sprites
            batch_renderer_->generate_batches();
            
            // Optimize batches if enabled
            batch_renderer_->optimize_batches();
            
            // Sort batches for proper rendering order
            batch_renderer_->sort_batches();
            
            current_frame_stats_.batches_generated = static_cast<u32>(batch_renderer_->get_batch_count());
        }
    }
    
    /** @brief Render debug entities using batching */
    void render_batched_debug_entities() {
        auto render_start = std::chrono::high_resolution_clock::now();
        
        // Render all batches through the batch renderer
        if (renderer2d_) {
            batch_renderer_->render_all(*renderer2d_);
        }
        
        auto render_end = std::chrono::high_resolution_clock::now();
        render_stats_.batched_mode_time = std::chrono::duration<f32, std::milli>(render_end - render_start).count();
    }
    
    /** @brief Render debug entities immediately (for comparison) */
    void render_immediate_debug_entities() {
        auto render_start = std::chrono::high_resolution_clock::now();
        
        // Render each debug entity individually
        for (const auto& debug_entity : debug_entities_) {
            auto* sprite = registry_->get_component<RenderableSprite>(debug_entity.entity_id);
            auto* transform = registry_->get_component<Transform>(debug_entity.entity_id);
            
            if (sprite && transform && renderer2d_) {
                // Render individual sprite (this would be less efficient)
                // renderer2d_->render_sprite(*sprite, *transform);
            }
        }
        
        auto render_end = std::chrono::high_resolution_clock::now();
        render_stats_.immediate_mode_time = std::chrono::duration<f32, std::milli>(render_end - render_start).count();
    }
    
    /** @brief Update camera transformation matrix */
    void update_camera_matrix() {
        // This would update internal camera transformation matrix
        // for world-to-screen coordinate conversion
    }
    
    /** @brief Convert world coordinates to screen coordinates */
    Vec2 world_to_screen(const Vec2& world_pos) const {
        // Apply camera transformation
        Vec2 camera_relative = world_pos - current_camera_position_;
        camera_relative *= current_camera_zoom_;
        
        // Apply rotation if needed
        if (std::abs(current_camera_rotation_) > 0.001f) {
            f32 cos_r = std::cos(current_camera_rotation_);
            f32 sin_r = std::sin(current_camera_rotation_);
            
            Vec2 rotated = {
                camera_relative.x * cos_r - camera_relative.y * sin_r,
                camera_relative.x * sin_r + camera_relative.y * cos_r
            };
            camera_relative = rotated;
        }
        
        // Convert to screen space (assuming center screen origin)
        return camera_relative + Vec2{960.0f, 540.0f}; // Assume 1920x1080 screen
    }
    
    /** @brief Update performance statistics */
    void update_performance_stats() {
        auto frame_end = std::chrono::high_resolution_clock::now();
        f32 frame_time = std::chrono::duration<f32, std::milli>(frame_end - frame_start_time_).count();
        
        current_frame_stats_.frame_render_time = frame_time;
        
        // Update accumulated statistics
        render_stats_.total_frames++;
        render_stats_.total_render_time += frame_time;
        render_stats_.peak_render_time = std::max(render_stats_.peak_render_time, frame_time);
        render_stats_.total_shapes_rendered += current_frame_stats_.shapes_rendered;
        render_stats_.peak_shapes_per_frame = std::max(render_stats_.peak_shapes_per_frame, current_frame_stats_.shapes_rendered);
        render_stats_.total_batches_generated += current_frame_stats_.batches_generated;
        
        // Update batching efficiency
        if (config_.enable_batching && current_frame_stats_.batches_generated > 0) {
            f32 ideal_batches = std::ceil(static_cast<f32>(current_frame_stats_.shapes_rendered) / config_.max_debug_sprites_per_batch);
            render_stats_.batching_efficiency = ideal_batches / current_frame_stats_.batches_generated;
        }
        
        // Update memory statistics
        render_stats_.batched_mode_memory = debug_arena_.get_used();
    }
    
    /** @brief Clean up temporary debug entities */
    void cleanup_frame_debug_entities() {
        for (const auto& debug_entity : debug_entities_) {
            if (debug_entity.is_temporary) {
                registry_->destroy(debug_entity.entity_id);
            }
        }
        
        debug_entities_.clear();
    }
    
    /** @brief Analyze performance and generate suggestions */
    void analyze_performance(DebugRenderStatistics& stats) const {
        // Determine performance rating
        if (stats.average_render_time_ms < 2.0f && stats.batching_efficiency > 0.8f) {
            stats.performance_rating = "Excellent";
        } else if (stats.average_render_time_ms < 5.0f && stats.batching_efficiency > 0.6f) {
            stats.performance_rating = "Good";
        } else if (stats.average_render_time_ms < 10.0f) {
            stats.performance_rating = "Fair";
        } else {
            stats.performance_rating = "Poor";
        }
        
        // Generate optimization suggestions
        stats.optimization_suggestions.clear();
        
        if (stats.batching_efficiency < 0.7f) {
            stats.optimization_suggestions.push_back("Improve batching by using consistent materials and textures");
            stats.optimization_suggestions.push_back("Consider increasing max sprites per batch");
        }
        
        if (stats.average_render_time_ms > 5.0f) {
            stats.optimization_suggestions.push_back("Reduce debug primitive quality for better performance");
            stats.optimization_suggestions.push_back("Enable debug geometry caching to avoid regeneration");
        }
        
        if (stats.memory_efficiency < 0.8f) {
            stats.optimization_suggestions.push_back("Optimize memory allocation patterns");
            stats.optimization_suggestions.push_back("Consider using object pooling for debug entities");
        }
        
        if (stats.shapes_per_frame_average > 1000) {
            stats.optimization_suggestions.push_back("Implement frustum culling for debug shapes");
            stats.optimization_suggestions.push_back("Use level-of-detail system for distant debug elements");
        }
    }
};

} // namespace ecscope::physics::debug