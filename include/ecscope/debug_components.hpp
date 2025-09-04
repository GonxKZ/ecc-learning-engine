#pragma once

/**
 * @file physics/components/debug_components.hpp
 * @brief Physics Debug Visualization Components for ECScope - ECS Integration
 * 
 * This header provides comprehensive debug visualization components that integrate
 * seamlessly with ECScope's ECS system and modern 2D rendering pipeline. These
 * components enable real-time physics visualization with educational insights.
 * 
 * Key Features:
 * - ECS-native debug visualization components
 * - Integration with physics simulation and rendering systems
 * - Educational debug information and performance metrics
 * - Memory-efficient debug data structures
 * - Hierarchical debug visualization relationships
 * - Interactive debug parameter adjustment
 * 
 * Educational Demonstrations:
 * - Component-based debug visualization architecture
 * - Integration patterns between simulation, ECS, and rendering
 * - Memory layout optimization for debug data
 * - Component relationships and dependencies
 * - Performance impact of different debug visualization approaches
 * 
 * Advanced Features:
 * - Dynamic debug visualization levels
 * - Physics algorithm step visualization
 * - Memory allocation pattern tracking
 * - Performance profiling integration
 * - Interactive educational overlays
 * 
 * Technical Architecture:
 * - Full ECS Component compliance (trivially copyable, standard layout)
 * - Integration with existing physics components (RigidBody2D, Collider2D)
 * - Compatibility with rendering components (RenderableSprite, Material)
 * - Memory-aligned structures for optimal cache performance
 * - Educational debug information without runtime overhead in release builds
 * 
 * @author ECScope Educational ECS Framework - Physics Debug Components
 * @date 2024
 */

#include "../../ecs/component.hpp"
#include "../../renderer/components/render_components.hpp"
#include "../../memory/memory_tracker.hpp"
#include "../../core/types.hpp"
#include <array>
#include <bitset>
#include <chrono>

namespace ecscope::physics::debug {

// Import commonly used types
using namespace ecscope::ecs;
using namespace ecscope::renderer::components;

//=============================================================================
// Forward Declarations and Type Aliases  
//=============================================================================

struct Vec2 {
    f32 x{0.0f}, y{0.0f};
    constexpr Vec2() = default;
    constexpr Vec2(f32 x_, f32 y_) : x(x_), y(y_) {}
    
    Vec2 operator+(const Vec2& other) const { return {x + other.x, y + other.y}; }
    Vec2 operator-(const Vec2& other) const { return {x - other.x, y - other.y}; }
    Vec2 operator*(f32 scalar) const { return {x * scalar, y * scalar}; }
    f32 length() const { return std::sqrt(x * x + y * y); }
    Vec2 normalized() const { f32 len = length(); return len > 0 ? *this * (1.0f / len) : Vec2{}; }
};

//=============================================================================
// Core Physics Debug Components
//=============================================================================

/**
 * @brief Physics Debug Visualization Component
 * 
 * This component controls what physics debug information is visualized for an entity
 * and how it's rendered. It provides comprehensive control over debug visualization
 * while maintaining optimal performance for real-time physics simulation.
 * 
 * Educational Context:
 * This component demonstrates:
 * - Component-based debug visualization architecture
 * - Separation of concerns between simulation and visualization
 * - Memory-efficient debug data storage and access patterns
 * - Integration between physics, ECS, and rendering systems
 * - Performance optimization techniques for debug rendering
 * 
 * Performance Characteristics:
 * - Minimal memory footprint (64 bytes) for cache efficiency
 * - Hot debug data grouped together for optimal access patterns
 * - Cold debug data (configuration, statistics) placed separately
 * - Zero-overhead abstractions in release builds
 * - ECS-friendly data layout for efficient batch processing
 */
struct alignas(16) PhysicsDebugVisualization {
    //-------------------------------------------------------------------------
    // Visualization Control Flags
    //-------------------------------------------------------------------------
    
    /** 
     * @brief Debug visualization flags for different physics elements
     * 
     * These flags control which aspects of physics simulation are visualized.
     * Each flag can be toggled independently for fine-grained control.
     */
    union {
        u32 flags;
        struct {
            u32 show_collision_shape : 1;      ///< Show collision boundary visualization
            u32 show_velocity_vector : 1;      ///< Show velocity vector arrow
            u32 show_force_vectors : 1;        ///< Show applied forces as arrows
            u32 show_center_of_mass : 1;       ///< Show center of mass point
            u32 show_local_axes : 1;           ///< Show local coordinate axes
            u32 show_bounding_box : 1;         ///< Show axis-aligned bounding box
            u32 show_contact_points : 1;       ///< Show collision contact points
            u32 show_contact_normals : 1;      ///< Show collision normal vectors
            u32 show_impulse_vectors : 1;      ///< Show impulse application points
            u32 show_trajectory : 1;           ///< Show predicted trajectory path
            u32 show_energy_info : 1;          ///< Show kinetic/potential energy data
            u32 show_constraint_connections : 1; ///< Show constraint relationships
            u32 show_spatial_hash_cell : 1;    ///< Show spatial hash grid cell
            u32 show_debug_text : 1;           ///< Show textual debug information
            u32 show_performance_metrics : 1;  ///< Show performance overlay
            u32 show_memory_usage : 1;         ///< Show memory usage information
            u32 highlight_entity : 1;          ///< Highlight this entity specially
            u32 interactive_mode : 1;          ///< Enable interactive manipulation
            u32 step_by_step_mode : 1;         ///< Enable step-by-step visualization
            u32 educational_overlays : 1;      ///< Show educational information
            u32 reserved : 12;                 ///< Reserved for future use
        };
    } visualization_flags{0};
    
    //-------------------------------------------------------------------------
    // Debug Rendering Properties
    //-------------------------------------------------------------------------
    
    /** 
     * @brief Color scheme for debug visualization
     * 
     * Different physics properties are visualized with different colors
     * to help distinguish between various simulation aspects.
     */
    struct ColorScheme {
        Color collision_shape_color{Color::green()};       ///< Collision boundary color
        Color velocity_vector_color{Color::blue()};        ///< Velocity arrow color
        Color force_vector_color{Color::red()};            ///< Force arrow color
        Color center_of_mass_color{Color::yellow()};       ///< Center of mass point color
        Color bounding_box_color{Color::cyan()};           ///< AABB outline color
        Color contact_point_color{Color::magenta()};       ///< Contact point color
        Color contact_normal_color{Color::white()};        ///< Contact normal color
        Color trajectory_color{Color::transparent()};      ///< Trajectory path color
        Color highlight_color{Color::white()};             ///< Entity highlight color
        Color text_color{Color::white()};                  ///< Debug text color
        
        /** @brief Get color by debug element type */
        Color get_debug_color(u32 element_type) const {
            switch (element_type) {
                case 0: return collision_shape_color;
                case 1: return velocity_vector_color;
                case 2: return force_vector_color;
                case 3: return center_of_mass_color;
                case 4: return bounding_box_color;
                case 5: return contact_point_color;
                case 6: return contact_normal_color;
                case 7: return trajectory_color;
                case 8: return highlight_color;
                case 9: return text_color;
                default: return Color::white();
            }
        }
    } color_scheme;
    
    /** 
     * @brief Debug rendering scale factors
     * 
     * These values control the visual scale of debug elements relative
     * to the physics simulation scale.
     */
    struct ScaleFactors {
        f32 velocity_scale{2.0f};        ///< Scale factor for velocity vectors
        f32 force_scale{0.5f};           ///< Scale factor for force vectors
        f32 contact_point_size{3.0f};    ///< Size of contact point markers
        f32 text_size{12.0f};            ///< Size of debug text
        f32 line_thickness{2.0f};        ///< Thickness of debug lines
        f32 arrow_head_size{4.0f};       ///< Size of arrow heads
        f32 highlight_thickness{3.0f};   ///< Thickness of highlight outlines
        f32 axis_length{20.0f};          ///< Length of coordinate axes
        
        /** @brief Apply global scale multiplier */
        void apply_global_scale(f32 global_scale) {
            velocity_scale *= global_scale;
            force_scale *= global_scale;
            contact_point_size *= global_scale;
            text_size *= global_scale;
            line_thickness *= global_scale;
            arrow_head_size *= global_scale;
            highlight_thickness *= global_scale;
            axis_length *= global_scale;
        }
    } scale_factors;
    
    /** 
     * @brief Debug visualization layer and depth
     * 
     * Controls rendering order and layer visibility for debug elements.
     */
    struct LayerInfo {
        u8 debug_layer{10};              ///< Debug rendering layer (higher = on top)
        f32 base_z_order{100.0f};        ///< Base Z-order for debug elements
        f32 z_order_step{0.1f};          ///< Z-order increment between elements
        bool render_behind_objects{false}; ///< Render debug info behind main objects
        f32 transparency{0.8f};          ///< Debug element transparency
        
        /** @brief Get Z-order for specific debug element */
        f32 get_z_order(u32 element_index) const {
            return render_behind_objects ? 
                (base_z_order - z_order_step * element_index) :
                (base_z_order + z_order_step * element_index);
        }
    } layer_info;
    
    //-------------------------------------------------------------------------
    // Interactive Debug Features
    //-------------------------------------------------------------------------
    
    /** 
     * @brief Interactive debug manipulation settings
     * 
     * Controls how users can interact with debug visualization elements
     * for educational purposes.
     */
    struct InteractionSettings {
        bool allow_drag_entity{true};        ///< Allow dragging entity with mouse
        bool allow_force_application{true};  ///< Allow applying forces via mouse
        bool allow_parameter_tuning{true};   ///< Allow real-time parameter adjustment
        bool show_interaction_hints{true};   ///< Show UI hints for interaction
        f32 drag_force_multiplier{10.0f};   ///< Force multiplier for dragging
        f32 click_radius{15.0f};             ///< Pixel radius for click detection
        
        /** @brief Check if point is within interaction radius */
        bool is_within_interaction_radius(const Vec2& point, const Vec2& target) const {
            Vec2 diff = point - target;
            return diff.length() <= click_radius;
        }
    } interaction_settings;
    
    //-------------------------------------------------------------------------
    // Educational Debug Information
    //-------------------------------------------------------------------------
    
    /** 
     * @brief Educational debug information display
     * 
     * Controls what educational information is shown alongside the visualization.
     */
    struct EducationalInfo {
        bool show_physics_equations{false}; ///< Show relevant physics equations
        bool show_numerical_values{true};   ///< Show numerical data (velocity, force, etc.)
        bool show_algorithm_steps{false};   ///< Show step-by-step algorithm breakdown
        bool show_performance_impact{false}; ///< Show performance cost of debug visualization
        bool show_memory_usage{false};      ///< Show memory usage of debug data
        bool show_optimization_hints{false}; ///< Show optimization suggestions
        
        /** @brief Text format for numerical display */
        enum class NumberFormat : u8 {
            Short = 0,      ///< "1.23"
            Scientific,     ///< "1.23e+2"
            Engineering,    ///< "123.0"
            Percentage      ///< "123%"
        } number_format{NumberFormat::Short};
        
        u8 decimal_precision{2};             ///< Decimal places for numerical display
    } educational_info;
    
    //-------------------------------------------------------------------------
    // Performance and Statistics
    //-------------------------------------------------------------------------
    
    /** 
     * @brief Debug rendering performance tracking
     * 
     * Tracks the performance impact of debug visualization for educational analysis.
     */
    mutable struct DebugPerformance {
        u32 frames_visualized{0};           ///< Number of frames this entity was visualized
        f32 total_render_time{0.0f};        ///< Total time spent rendering debug info (ms)
        f32 average_render_time{0.0f};      ///< Average render time per frame (ms)
        f32 peak_render_time{0.0f};         ///< Peak render time recorded (ms)
        u32 debug_elements_rendered{0};     ///< Number of debug elements currently rendered
        u32 batch_breaks_caused{0};         ///< Times debug rendering broke batching
        
        /** @brief Update performance statistics */
        void update_stats(f32 frame_render_time, u32 elements_count) {
            ++frames_visualized;
            total_render_time += frame_render_time;
            average_render_time = total_render_time / frames_visualized;
            peak_render_time = std::max(peak_render_time, frame_render_time);
            debug_elements_rendered = elements_count;
        }
        
        /** @brief Get performance efficiency score (0-1, higher is better) */
        f32 get_efficiency_score() const {
            if (debug_elements_rendered == 0) return 1.0f;
            f32 base_cost = debug_elements_rendered * 0.1f; // 0.1ms per element baseline
            return std::min(1.0f, base_cost / std::max(0.001f, average_render_time));
        }
    } debug_performance;
    
    //-------------------------------------------------------------------------
    // Debug Data Cache
    //-------------------------------------------------------------------------
    
    /** 
     * @brief Cached debug data for performance optimization
     * 
     * Stores frequently accessed debug data to avoid recalculation each frame.
     */
    mutable struct DebugDataCache {
        // Cached vectors
        Vec2 cached_velocity{0.0f, 0.0f};
        Vec2 cached_position{0.0f, 0.0f};
        Vec2 cached_center_of_mass{0.0f, 0.0f};
        
        // Cached forces (up to 4 most significant forces)
        static constexpr usize MAX_CACHED_FORCES = 4;
        std::array<Vec2, MAX_CACHED_FORCES> cached_forces{};
        u8 cached_force_count{0};
        
        // Cached contact data
        static constexpr usize MAX_CACHED_CONTACTS = 8;
        struct ContactCache {
            Vec2 point{0.0f, 0.0f};
            Vec2 normal{0.0f, 0.0f};
            f32 depth{0.0f};
            f32 impulse_magnitude{0.0f};
        };
        std::array<ContactCache, MAX_CACHED_CONTACTS> cached_contacts{};
        u8 cached_contact_count{0};
        
        // Cache validity flags
        bool velocity_cache_valid{false};
        bool position_cache_valid{false};
        bool forces_cache_valid{false};
        bool contacts_cache_valid{false};
        u32 cache_frame_number{0}; ///< Frame number when cache was last updated
        
        /** @brief Clear all cached data */
        void clear_cache() {
            velocity_cache_valid = false;
            position_cache_valid = false;
            forces_cache_valid = false;
            contacts_cache_valid = false;
            cached_force_count = 0;
            cached_contact_count = 0;
        }
        
        /** @brief Check if cache is valid for current frame */
        bool is_cache_valid(u32 current_frame) const {
            return cache_frame_number == current_frame;
        }
    } debug_cache;
    
    //-------------------------------------------------------------------------
    // Constructors and Factory Methods
    //-------------------------------------------------------------------------
    
    constexpr PhysicsDebugVisualization() noexcept = default;
    
    /** @brief Create basic debug visualization */
    static PhysicsDebugVisualization create_basic() {
        PhysicsDebugVisualization debug;
        debug.visualization_flags.show_collision_shape = 1;
        debug.visualization_flags.show_velocity_vector = 1;
        debug.visualization_flags.show_center_of_mass = 1;
        return debug;
    }
    
    /** @brief Create comprehensive debug visualization for education */
    static PhysicsDebugVisualization create_educational() {
        PhysicsDebugVisualization debug;
        debug.visualization_flags.flags = 0xFFFFF; // Enable most features
        debug.educational_info.show_physics_equations = true;
        debug.educational_info.show_numerical_values = true;
        debug.educational_info.show_algorithm_steps = true;
        debug.educational_info.show_performance_impact = true;
        return debug;
    }
    
    /** @brief Create minimal debug visualization for performance */
    static PhysicsDebugVisualization create_minimal() {
        PhysicsDebugVisualization debug;
        debug.visualization_flags.show_collision_shape = 1;
        debug.scale_factors.line_thickness = 1.0f;
        debug.layer_info.transparency = 0.5f;
        return debug;
    }
    
    /** @brief Create interactive debug visualization */
    static PhysicsDebugVisualization create_interactive() {
        PhysicsDebugVisualization debug = create_basic();
        debug.visualization_flags.interactive_mode = 1;
        debug.interaction_settings.allow_drag_entity = true;
        debug.interaction_settings.allow_force_application = true;
        debug.interaction_settings.show_interaction_hints = true;
        return debug;
    }
    
    //-------------------------------------------------------------------------
    // Debug Control Interface
    //-------------------------------------------------------------------------
    
    /** @brief Enable specific debug visualization */
    void enable_visualization(u32 flag_mask) {
        visualization_flags.flags |= flag_mask;
        debug_cache.clear_cache(); // Invalidate cache when visualization changes
    }
    
    /** @brief Disable specific debug visualization */
    void disable_visualization(u32 flag_mask) {
        visualization_flags.flags &= ~flag_mask;
        debug_cache.clear_cache();
    }
    
    /** @brief Check if specific visualization is enabled */
    bool is_visualization_enabled(u32 flag_mask) const {
        return (visualization_flags.flags & flag_mask) != 0;
    }
    
    /** @brief Set debug color scheme */
    void set_color_scheme(const ColorScheme& new_scheme) {
        color_scheme = new_scheme;
    }
    
    /** @brief Apply global scale to all debug elements */
    void apply_global_scale(f32 scale) {
        scale_factors.apply_global_scale(scale);
    }
    
    /** @brief Set debug layer and rendering order */
    void set_debug_layer(u8 layer, f32 z_order, bool behind_objects = false) {
        layer_info.debug_layer = layer;
        layer_info.base_z_order = z_order;
        layer_info.render_behind_objects = behind_objects;
    }
    
    /** @brief Enable/disable educational features */
    void set_educational_mode(bool enabled) {
        educational_info.show_physics_equations = enabled;
        educational_info.show_numerical_values = enabled;
        educational_info.show_algorithm_steps = enabled;
        educational_info.show_performance_impact = enabled;
        educational_info.show_optimization_hints = enabled;
    }
    
    /** @brief Reset performance statistics */
    void reset_performance_stats() {
        debug_performance = DebugPerformance{};
    }
    
    //-------------------------------------------------------------------------
    // Utility Functions
    //-------------------------------------------------------------------------
    
    /** @brief Get estimated memory usage of debug data */
    usize get_debug_memory_usage() const {
        usize base_size = sizeof(PhysicsDebugVisualization);
        usize cache_size = sizeof(DebugDataCache);
        usize performance_size = sizeof(DebugPerformance);
        return base_size + cache_size + performance_size;
    }
    
    /** @brief Get debug complexity score (higher = more expensive to render) */
    f32 get_debug_complexity() const {
        f32 complexity = 0.0f;
        
        // Count enabled visualization flags
        u32 enabled_flags = __builtin_popcount(visualization_flags.flags);
        complexity += enabled_flags * 0.1f;
        
        // Add complexity for interactive features
        if (visualization_flags.interactive_mode) complexity += 0.5f;
        if (educational_info.show_algorithm_steps) complexity += 0.3f;
        if (educational_info.show_performance_impact) complexity += 0.2f;
        
        // Factor in transparency (alpha blending cost)
        if (layer_info.transparency < 1.0f) complexity += 0.2f;
        
        return complexity;
    }
    
    /** @brief Validate debug visualization configuration */
    bool is_valid() const {
        return scale_factors.velocity_scale > 0.0f &&
               scale_factors.force_scale > 0.0f &&
               scale_factors.contact_point_size > 0.0f &&
               scale_factors.text_size > 0.0f &&
               scale_factors.line_thickness > 0.0f &&
               layer_info.transparency >= 0.0f &&
               layer_info.transparency <= 1.0f;
    }
    
    /** @brief Get debug visualization summary for educational display */
    struct DebugSummary {
        u32 enabled_visualizations;
        f32 complexity_score;
        f32 performance_impact;
        const char* recommended_level;
        bool has_educational_features;
        bool has_interactive_features;
        usize memory_usage_bytes;
    };
    
    DebugSummary get_debug_summary() const {
        DebugSummary summary{};
        summary.enabled_visualizations = __builtin_popcount(visualization_flags.flags);
        summary.complexity_score = get_debug_complexity();
        summary.performance_impact = debug_performance.average_render_time;
        summary.has_educational_features = educational_info.show_physics_equations ||
                                          educational_info.show_algorithm_steps;
        summary.has_interactive_features = visualization_flags.interactive_mode;
        summary.memory_usage_bytes = get_debug_memory_usage();
        
        // Determine recommended level
        if (summary.complexity_score < 0.5f) {
            summary.recommended_level = "Basic";
        } else if (summary.complexity_score < 1.0f) {
            summary.recommended_level = "Intermediate";
        } else {
            summary.recommended_level = "Advanced";
        }
        
        return summary;
    }
};

/**
 * @brief Physics Debug Shape Component
 * 
 * This component stores debug geometry data for efficient rendering of physics
 * collision shapes, contact points, and other debug primitives. It provides
 * optimized storage for debug geometry generation and caching.
 * 
 * Educational Context:
 * This component demonstrates:
 * - Separation between debug data generation and rendering
 * - Memory-efficient storage of debug geometry
 * - Cache-friendly data access patterns for debug rendering
 * - Integration with modern 2D rendering pipeline
 * - Performance optimization through geometry caching
 */
struct alignas(32) PhysicsDebugShape {
    //-------------------------------------------------------------------------
    // Debug Geometry Data
    //-------------------------------------------------------------------------
    
    /** 
     * @brief Debug shape type enumeration
     */
    enum class ShapeType : u8 {
        None = 0,
        Circle,
        Rectangle,  
        OrientedBox,
        Polygon,
        Line,
        Point,
        Arrow,
        Text
    };
    
    /** 
     * @brief Primary debug shape definition
     */
    ShapeType primary_shape_type{ShapeType::None};
    
    /** 
     * @brief Shape geometry data (using union for memory efficiency)
     */
    union GeometryData {
        // Circle shape data
        struct {
            Vec2 center;
            f32 radius;
            f32 _padding1;
        } circle;
        
        // Rectangle shape data
        struct {
            Vec2 min;
            Vec2 max;
        } rectangle;
        
        // Oriented box shape data
        struct {
            Vec2 center;
            Vec2 half_extents;
            f32 rotation;
            f32 _padding2;
        } oriented_box;
        
        // Line shape data
        struct {
            Vec2 start;
            Vec2 end;
        } line;
        
        // Point shape data
        struct {
            Vec2 position;
            f32 size;
            f32 _padding3;
        } point;
        
        // Arrow shape data
        struct {
            Vec2 start;
            Vec2 end;
            f32 head_size;
            f32 thickness;
        } arrow;
        
        GeometryData() { std::memset(this, 0, sizeof(GeometryData)); }
    } geometry;
    
    /** 
     * @brief Rendering properties for debug shape
     */
    struct RenderProperties {
        Color color{Color::white()};
        f32 thickness{1.0f};
        f32 alpha{1.0f};
        bool filled{false};
        bool visible{true};
        u8 layer{10};
        f32 z_order{100.0f};
    } render_props;
    
    //-------------------------------------------------------------------------
    // Multi-Shape Support
    //-------------------------------------------------------------------------
    
    /** 
     * @brief Additional debug shapes (for complex debug visualization)
     */
    static constexpr usize MAX_ADDITIONAL_SHAPES = 4;
    struct AdditionalShape {
        ShapeType shape_type{ShapeType::None};
        GeometryData geometry{};
        RenderProperties render_props{};
        bool active{false};
    };
    
    std::array<AdditionalShape, MAX_ADDITIONAL_SHAPES> additional_shapes{};
    u8 additional_shape_count{0};
    
    //-------------------------------------------------------------------------
    // Polygon Support
    //-------------------------------------------------------------------------
    
    /** 
     * @brief Polygon vertex data (for complex collision shapes)
     */
    static constexpr usize MAX_POLYGON_VERTICES = 8;
    std::array<Vec2, MAX_POLYGON_VERTICES> polygon_vertices{};
    u8 polygon_vertex_count{0};
    
    //-------------------------------------------------------------------------
    // Text Debug Support
    //-------------------------------------------------------------------------
    
    /** 
     * @brief Debug text information
     */
    struct DebugText {
        static constexpr usize MAX_TEXT_LENGTH = 32;
        char text[MAX_TEXT_LENGTH]{};
        Vec2 position{0.0f, 0.0f};
        f32 size{12.0f};
        Color color{Color::white()};
        bool screen_space{false}; ///< True for screen-space, false for world-space
        
        /** @brief Set debug text */
        void set_text(const char* new_text) {
            strncpy(text, new_text, MAX_TEXT_LENGTH - 1);
            text[MAX_TEXT_LENGTH - 1] = '\0';
        }
        
        /** @brief Check if text is valid */
        bool is_valid() const {
            return text[0] != '\0';
        }
    } debug_text;
    
    //-------------------------------------------------------------------------
    // Performance and Caching
    //-------------------------------------------------------------------------
    
    /** 
     * @brief Geometry generation cache
     */
    mutable struct GeometryCache {
        bool geometry_dirty{true};          ///< Whether geometry needs regeneration
        u32 last_update_frame{0};          ///< Frame when geometry was last updated
        u32 vertex_count{0};               ///< Number of vertices generated
        u32 index_count{0};                ///< Number of indices generated
        f32 generation_time_ms{0.0f};      ///< Time spent generating geometry
        usize memory_used{0};              ///< Memory used by generated geometry
        
        /** @brief Mark geometry as dirty */
        void mark_dirty() {
            geometry_dirty = true;
        }
        
        /** @brief Update cache statistics */
        void update_cache_stats(u32 frame, u32 vertices, u32 indices, f32 time_ms, usize memory) {
            geometry_dirty = false;
            last_update_frame = frame;
            vertex_count = vertices;
            index_count = indices;
            generation_time_ms = time_ms;
            memory_used = memory;
        }
    } geometry_cache;
    
    //-------------------------------------------------------------------------
    // Constructors and Factory Methods
    //-------------------------------------------------------------------------
    
    constexpr PhysicsDebugShape() noexcept = default;
    
    /** @brief Create debug circle shape */
    static PhysicsDebugShape create_circle(const Vec2& center, f32 radius, const Color& color, bool filled = false) {
        PhysicsDebugShape shape;
        shape.primary_shape_type = ShapeType::Circle;
        shape.geometry.circle.center = center;
        shape.geometry.circle.radius = radius;
        shape.render_props.color = color;
        shape.render_props.filled = filled;
        shape.render_props.visible = true;
        return shape;
    }
    
    /** @brief Create debug rectangle shape */
    static PhysicsDebugShape create_rectangle(const Vec2& min, const Vec2& max, const Color& color, bool filled = false) {
        PhysicsDebugShape shape;
        shape.primary_shape_type = ShapeType::Rectangle;
        shape.geometry.rectangle.min = min;
        shape.geometry.rectangle.max = max;
        shape.render_props.color = color;
        shape.render_props.filled = filled;
        shape.render_props.visible = true;
        return shape;
    }
    
    /** @brief Create debug oriented box shape */
    static PhysicsDebugShape create_oriented_box(const Vec2& center, const Vec2& half_extents, f32 rotation, const Color& color, bool filled = false) {
        PhysicsDebugShape shape;
        shape.primary_shape_type = ShapeType::OrientedBox;
        shape.geometry.oriented_box.center = center;
        shape.geometry.oriented_box.half_extents = half_extents;
        shape.geometry.oriented_box.rotation = rotation;
        shape.render_props.color = color;
        shape.render_props.filled = filled;
        shape.render_props.visible = true;
        return shape;
    }
    
    /** @brief Create debug line shape */
    static PhysicsDebugShape create_line(const Vec2& start, const Vec2& end, const Color& color, f32 thickness = 1.0f) {
        PhysicsDebugShape shape;
        shape.primary_shape_type = ShapeType::Line;
        shape.geometry.line.start = start;
        shape.geometry.line.end = end;
        shape.render_props.color = color;
        shape.render_props.thickness = thickness;
        shape.render_props.visible = true;
        return shape;
    }
    
    /** @brief Create debug arrow shape */
    static PhysicsDebugShape create_arrow(const Vec2& start, const Vec2& end, const Color& color, f32 head_size = 3.0f, f32 thickness = 1.0f) {
        PhysicsDebugShape shape;
        shape.primary_shape_type = ShapeType::Arrow;
        shape.geometry.arrow.start = start;
        shape.geometry.arrow.end = end;
        shape.geometry.arrow.head_size = head_size;
        shape.geometry.arrow.thickness = thickness;
        shape.render_props.color = color;
        shape.render_props.visible = true;
        return shape;
    }
    
    /** @brief Create debug point shape */
    static PhysicsDebugShape create_point(const Vec2& position, const Color& color, f32 size = 3.0f) {
        PhysicsDebugShape shape;
        shape.primary_shape_type = ShapeType::Point;
        shape.geometry.point.position = position;
        shape.geometry.point.size = size;
        shape.render_props.color = color;
        shape.render_props.filled = true;
        shape.render_props.visible = true;
        return shape;
    }
    
    /** @brief Create debug polygon shape */
    static PhysicsDebugShape create_polygon(const std::vector<Vec2>& vertices, const Color& color, bool filled = false) {
        PhysicsDebugShape shape;
        shape.primary_shape_type = ShapeType::Polygon;
        shape.render_props.color = color;
        shape.render_props.filled = filled;
        shape.render_props.visible = true;
        
        // Copy vertices (up to maximum)
        usize vertex_count = std::min(vertices.size(), static_cast<usize>(MAX_POLYGON_VERTICES));
        for (usize i = 0; i < vertex_count; ++i) {
            shape.polygon_vertices[i] = vertices[i];
        }
        shape.polygon_vertex_count = static_cast<u8>(vertex_count);
        
        return shape;
    }
    
    /** @brief Create debug text */
    static PhysicsDebugShape create_text(const Vec2& position, const char* text, const Color& color, f32 size = 12.0f, bool screen_space = false) {
        PhysicsDebugShape shape;
        shape.primary_shape_type = ShapeType::Text;
        shape.debug_text.position = position;
        shape.debug_text.set_text(text);
        shape.debug_text.color = color;
        shape.debug_text.size = size;
        shape.debug_text.screen_space = screen_space;
        shape.render_props.visible = true;
        return shape;
    }
    
    //-------------------------------------------------------------------------
    // Shape Manipulation Interface
    //-------------------------------------------------------------------------
    
    /** @brief Add additional debug shape */
    bool add_additional_shape(ShapeType type, const GeometryData& geom, const RenderProperties& props) {
        if (additional_shape_count < MAX_ADDITIONAL_SHAPES) {
            auto& shape = additional_shapes[additional_shape_count];
            shape.shape_type = type;
            shape.geometry = geom;
            shape.render_props = props;
            shape.active = true;
            ++additional_shape_count;
            geometry_cache.mark_dirty();
            return true;
        }
        return false;
    }
    
    /** @brief Remove additional shapes */
    void clear_additional_shapes() {
        additional_shape_count = 0;
        for (auto& shape : additional_shapes) {
            shape.active = false;
        }
        geometry_cache.mark_dirty();
    }
    
    /** @brief Update primary shape geometry */
    void update_geometry(const GeometryData& new_geometry) {
        geometry = new_geometry;
        geometry_cache.mark_dirty();
    }
    
    /** @brief Update rendering properties */
    void update_render_properties(const RenderProperties& props) {
        render_props = props;
        // Render properties don't affect geometry, so no need to mark cache dirty
    }
    
    /** @brief Set visibility */
    void set_visible(bool visible) {
        render_props.visible = visible;
    }
    
    /** @brief Set color */
    void set_color(const Color& color) {
        render_props.color = color;
    }
    
    /** @brief Set alpha transparency */
    void set_alpha(f32 alpha) {
        render_props.alpha = std::clamp(alpha, 0.0f, 1.0f);
    }
    
    //-------------------------------------------------------------------------
    // Utility Functions
    //-------------------------------------------------------------------------
    
    /** @brief Check if shape has valid geometry */
    bool is_valid() const {
        if (primary_shape_type == ShapeType::None) return false;
        
        switch (primary_shape_type) {
            case ShapeType::Circle:
                return geometry.circle.radius > 0.0f;
            case ShapeType::Rectangle:
                return geometry.rectangle.max.x > geometry.rectangle.min.x &&
                       geometry.rectangle.max.y > geometry.rectangle.min.y;
            case ShapeType::OrientedBox:
                return geometry.oriented_box.half_extents.x > 0.0f &&
                       geometry.oriented_box.half_extents.y > 0.0f;
            case ShapeType::Line:
                return geometry.line.start.x != geometry.line.end.x ||
                       geometry.line.start.y != geometry.line.end.y;
            case ShapeType::Point:
                return geometry.point.size > 0.0f;
            case ShapeType::Arrow:
                return (geometry.arrow.start.x != geometry.arrow.end.x ||
                        geometry.arrow.start.y != geometry.arrow.end.y) &&
                       geometry.arrow.head_size > 0.0f;
            case ShapeType::Polygon:
                return polygon_vertex_count >= 3;
            case ShapeType::Text:
                return debug_text.is_valid();
            default:
                return false;
        }
    }
    
    /** @brief Get estimated rendering complexity */
    f32 get_render_complexity() const {
        f32 complexity = 1.0f; // Base complexity
        
        switch (primary_shape_type) {
            case ShapeType::Circle:
                complexity += render_props.filled ? 2.0f : 1.0f;
                break;
            case ShapeType::Rectangle:
                complexity += render_props.filled ? 1.5f : 1.0f;
                break;
            case ShapeType::OrientedBox:
                complexity += render_props.filled ? 2.0f : 1.5f;
                break;
            case ShapeType::Polygon:
                complexity += polygon_vertex_count * (render_props.filled ? 0.5f : 0.2f);
                break;
            case ShapeType::Line:
                complexity += 0.5f;
                break;
            case ShapeType::Point:
                complexity += 0.5f;
                break;
            case ShapeType::Arrow:
                complexity += 1.5f; // Line + arrowhead
                break;
            case ShapeType::Text:
                complexity += debug_text.size / 12.0f; // Scale by text size
                break;
            default:
                break;
        }
        
        // Add complexity for additional shapes
        complexity += additional_shape_count * 0.5f;
        
        // Add complexity for transparency
        if (render_props.alpha < 1.0f) {
            complexity *= 1.2f;
        }
        
        return complexity;
    }
    
    /** @brief Get memory usage estimate */
    usize get_memory_usage() const {
        usize base_size = sizeof(PhysicsDebugShape);
        usize polygon_size = polygon_vertex_count * sizeof(Vec2);
        usize additional_size = additional_shape_count * sizeof(AdditionalShape);
        return base_size + polygon_size + additional_size + geometry_cache.memory_used;
    }
    
    /** @brief Get total shape count (primary + additional) */
    u32 get_total_shape_count() const {
        return 1 + additional_shape_count;
    }
};

/**
 * @brief Physics Debug Statistics Component
 * 
 * This component tracks comprehensive statistics about physics debug visualization
 * performance and provides educational insights into the rendering pipeline.
 * 
 * Educational Context:
 * This component demonstrates:
 * - Performance monitoring and analysis in real-time systems
 * - Statistical analysis of rendering performance
 * - Educational metrics for understanding system behavior
 * - Memory usage tracking and optimization analysis
 * - Integration between profiling and educational visualization
 */
struct alignas(32) PhysicsDebugStats {
    //-------------------------------------------------------------------------
    // Real-time Performance Metrics
    //-------------------------------------------------------------------------
    
    /** 
     * @brief Frame-by-frame performance tracking
     */
    struct FrameMetrics {
        f32 debug_render_time_ms{0.0f};     ///< Time spent rendering debug info this frame
        f32 debug_update_time_ms{0.0f};     ///< Time spent updating debug data this frame
        u32 debug_shapes_rendered{0};       ///< Number of debug shapes rendered this frame
        u32 debug_draw_calls{0};            ///< Number of draw calls used for debug rendering
        u32 debug_vertices_generated{0};    ///< Number of vertices generated for debug rendering
        u32 debug_batches_created{0};       ///< Number of batches created for debug rendering
        f32 batching_efficiency{1.0f};      ///< Debug batching efficiency (0-1)
        
        /** @brief Reset frame metrics */
        void reset() {
            debug_render_time_ms = 0.0f;
            debug_update_time_ms = 0.0f;
            debug_shapes_rendered = 0;
            debug_draw_calls = 0;
            debug_vertices_generated = 0;
            debug_batches_created = 0;
            batching_efficiency = 1.0f;
        }
        
        /** @brief Update batching efficiency calculation */
        void update_batching_efficiency() {
            if (debug_shapes_rendered > 0 && debug_batches_created > 0) {
                f32 ideal_batches = 1.0f; // Ideally all shapes in one batch
                batching_efficiency = ideal_batches / debug_batches_created;
            }
        }
    } current_frame;
    
    //-------------------------------------------------------------------------
    // Historical Performance Data
    //-------------------------------------------------------------------------
    
    /** 
     * @brief Accumulated performance statistics
     */
    struct AccumulatedStats {
        u32 total_frames_with_debug{0};     ///< Total frames that had debug rendering
        f32 total_debug_time_ms{0.0f};      ///< Total time spent on debug rendering
        f32 average_debug_time_ms{0.0f};    ///< Average debug render time per frame
        f32 peak_debug_time_ms{0.0f};       ///< Peak debug render time recorded
        
        u64 total_shapes_rendered{0};       ///< Total debug shapes rendered over time
        u64 total_vertices_generated{0};    ///< Total debug vertices generated
        u64 total_draw_calls{0};            ///< Total draw calls for debug rendering
        
        f32 average_batching_efficiency{1.0f}; ///< Average batching efficiency over time
        f32 worst_batching_efficiency{1.0f};   ///< Worst batching efficiency recorded
        
        /** @brief Update accumulated statistics */
        void update(const FrameMetrics& frame) {
            ++total_frames_with_debug;
            total_debug_time_ms += frame.debug_render_time_ms;
            average_debug_time_ms = total_debug_time_ms / total_frames_with_debug;
            peak_debug_time_ms = std::max(peak_debug_time_ms, frame.debug_render_time_ms);
            
            total_shapes_rendered += frame.debug_shapes_rendered;
            total_vertices_generated += frame.debug_vertices_generated;
            total_draw_calls += frame.debug_draw_calls;
            
            // Update batching efficiency averages
            f32 frame_weight = 1.0f / total_frames_with_debug;
            average_batching_efficiency = (average_batching_efficiency * (1.0f - frame_weight)) + 
                                        (frame.batching_efficiency * frame_weight);
            worst_batching_efficiency = std::min(worst_batching_efficiency, frame.batching_efficiency);
        }
    } accumulated_stats;
    
    //-------------------------------------------------------------------------
    // Memory Usage Tracking
    //-------------------------------------------------------------------------
    
    /** 
     * @brief Debug memory usage statistics
     */
    struct MemoryStats {
        usize debug_geometry_memory{0};     ///< Memory used for debug geometry data
        usize debug_texture_memory{0};      ///< Memory used for debug textures
        usize debug_vertex_memory{0};       ///< Memory used for debug vertex buffers
        usize debug_index_memory{0};        ///< Memory used for debug index buffers
        usize debug_component_memory{0};    ///< Memory used for debug components
        usize total_debug_memory{0};        ///< Total memory used for debug rendering
        
        usize peak_debug_memory{0};         ///< Peak memory usage recorded
        f32 memory_efficiency{1.0f};        ///< Memory usage efficiency (useful/total)
        u32 memory_allocations{0};          ///< Number of memory allocations this frame
        
        /** @brief Update memory statistics */
        void update() {
            total_debug_memory = debug_geometry_memory + debug_texture_memory + 
                               debug_vertex_memory + debug_index_memory + debug_component_memory;
            peak_debug_memory = std::max(peak_debug_memory, total_debug_memory);
            
            // Calculate memory efficiency (geometry + vertex data vs total)
            usize useful_memory = debug_geometry_memory + debug_vertex_memory;
            memory_efficiency = total_debug_memory > 0 ? 
                static_cast<f32>(useful_memory) / total_debug_memory : 1.0f;
        }
        
        /** @brief Get memory usage in human-readable format */
        struct MemoryReport {
            f32 total_mb;
            f32 geometry_mb;
            f32 vertex_mb;
            f32 efficiency_percentage;
            const char* efficiency_rating;
        };
        
        MemoryReport get_memory_report() const {
            MemoryReport report;
            report.total_mb = total_debug_memory / (1024.0f * 1024.0f);
            report.geometry_mb = debug_geometry_memory / (1024.0f * 1024.0f);
            report.vertex_mb = debug_vertex_memory / (1024.0f * 1024.0f);
            report.efficiency_percentage = memory_efficiency * 100.0f;
            
            if (memory_efficiency > 0.8f) report.efficiency_rating = "Excellent";
            else if (memory_efficiency > 0.6f) report.efficiency_rating = "Good";
            else if (memory_efficiency > 0.4f) report.efficiency_rating = "Fair";
            else report.efficiency_rating = "Poor";
            
            return report;
        }
    } memory_stats;
    
    //-------------------------------------------------------------------------
    // Educational Analysis Data
    //-------------------------------------------------------------------------
    
    /** 
     * @brief Educational insights and analysis
     */
    struct EducationalAnalysis {
        // Performance classification
        enum class PerformanceRating : u8 {
            Excellent = 0, Good, Fair, Poor, Critical
        } performance_rating{PerformanceRating::Good};
        
        // Bottleneck identification
        enum class PrimaryBottleneck : u8 {
            None = 0, CPU_Rendering, GPU_Overdraw, Memory_Bandwidth, 
            Draw_Calls, Geometry_Generation, Cache_Misses
        } primary_bottleneck{PrimaryBottleneck::None};
        
        // Optimization suggestions
        static constexpr usize MAX_SUGGESTIONS = 6;
        std::array<const char*, MAX_SUGGESTIONS> optimization_suggestions{};
        u8 suggestion_count{0};
        
        // Efficiency scores (0-1, higher is better)
        f32 rendering_efficiency{1.0f};
        f32 memory_efficiency{1.0f};
        f32 batching_efficiency{1.0f};
        f32 overall_efficiency{1.0f};
        
        /** @brief Add optimization suggestion */
        void add_suggestion(const char* suggestion) {
            if (suggestion_count < MAX_SUGGESTIONS) {
                optimization_suggestions[suggestion_count++] = suggestion;
            }
        }
        
        /** @brief Clear all suggestions */
        void clear_suggestions() {
            suggestion_count = 0;
        }
        
        /** @brief Calculate overall efficiency score */
        void calculate_overall_efficiency() {
            overall_efficiency = (rendering_efficiency + memory_efficiency + batching_efficiency) / 3.0f;
        }
        
        /** @brief Update performance rating based on efficiency */
        void update_performance_rating() {
            if (overall_efficiency > 0.9f) performance_rating = PerformanceRating::Excellent;
            else if (overall_efficiency > 0.75f) performance_rating = PerformanceRating::Good;
            else if (overall_efficiency > 0.6f) performance_rating = PerformanceRating::Fair;
            else if (overall_efficiency > 0.4f) performance_rating = PerformanceRating::Poor;
            else performance_rating = PerformanceRating::Critical;
        }
    } educational_analysis;
    
    //-------------------------------------------------------------------------
    // Comparison and Benchmarking
    //-------------------------------------------------------------------------
    
    /** 
     * @brief Performance comparison data for educational analysis
     */
    struct ComparisonData {
        f32 immediate_mode_time_ms{0.0f};   ///< Time for immediate mode rendering
        f32 batched_mode_time_ms{0.0f};     ///< Time for batched rendering
        f32 instanced_mode_time_ms{0.0f};   ///< Time for instanced rendering
        
        f32 no_debug_baseline_ms{0.0f};     ///< Baseline performance without debug rendering
        f32 debug_overhead_percentage{0.0f}; ///< Debug rendering overhead as percentage
        
        /** @brief Calculate performance improvements */
        struct ImprovementRatios {
            f32 batched_vs_immediate;
            f32 instanced_vs_immediate;
            f32 instanced_vs_batched;
            f32 debug_overhead_factor;
        };
        
        ImprovementRatios calculate_improvements() const {
            ImprovementRatios ratios{};
            
            if (immediate_mode_time_ms > 0.0f) {
                ratios.batched_vs_immediate = immediate_mode_time_ms / 
                    std::max(0.001f, batched_mode_time_ms);
                ratios.instanced_vs_immediate = immediate_mode_time_ms / 
                    std::max(0.001f, instanced_mode_time_ms);
            }
            
            if (batched_mode_time_ms > 0.0f) {
                ratios.instanced_vs_batched = batched_mode_time_ms / 
                    std::max(0.001f, instanced_mode_time_ms);
            }
            
            if (no_debug_baseline_ms > 0.0f) {
                f32 best_debug_time = std::min({immediate_mode_time_ms, batched_mode_time_ms, instanced_mode_time_ms});
                ratios.debug_overhead_factor = best_debug_time / no_debug_baseline_ms;
            }
            
            return ratios;
        }
    } comparison_data;
    
    //-------------------------------------------------------------------------
    // Constructors and Interface
    //-------------------------------------------------------------------------
    
    constexpr PhysicsDebugStats() noexcept = default;
    
    /** @brief Update statistics for current frame */
    void update_frame_stats(f32 render_time, f32 update_time, u32 shapes, u32 draw_calls, u32 vertices, u32 batches) {
        current_frame.debug_render_time_ms = render_time;
        current_frame.debug_update_time_ms = update_time;
        current_frame.debug_shapes_rendered = shapes;
        current_frame.debug_draw_calls = draw_calls;
        current_frame.debug_vertices_generated = vertices;
        current_frame.debug_batches_created = batches;
        current_frame.update_batching_efficiency();
        
        // Update accumulated statistics
        accumulated_stats.update(current_frame);
        
        // Update memory statistics
        memory_stats.update();
        
        // Update educational analysis
        update_educational_analysis();
    }
    
    /** @brief Reset all statistics */
    void reset_stats() {
        current_frame.reset();
        accumulated_stats = AccumulatedStats{};
        memory_stats = MemoryStats{};
        educational_analysis = EducationalAnalysis{};
        comparison_data = ComparisonData{};
    }
    
    /** @brief Generate comprehensive statistics report */
    std::string generate_statistics_report() const {
        std::ostringstream oss;
        
        oss << "=== Physics Debug Rendering Statistics ===\n\n";
        
        // Current frame statistics
        oss << "--- Current Frame ---\n";
        oss << "Render Time: " << current_frame.debug_render_time_ms << " ms\n";
        oss << "Shapes Rendered: " << current_frame.debug_shapes_rendered << "\n";
        oss << "Draw Calls: " << current_frame.debug_draw_calls << "\n";
        oss << "Batching Efficiency: " << (current_frame.batching_efficiency * 100.0f) << "%\n\n";
        
        // Accumulated statistics
        oss << "--- Historical Performance ---\n";
        oss << "Average Render Time: " << accumulated_stats.average_debug_time_ms << " ms\n";
        oss << "Peak Render Time: " << accumulated_stats.peak_debug_time_ms << " ms\n";
        oss << "Total Frames: " << accumulated_stats.total_frames_with_debug << "\n";
        oss << "Total Shapes: " << accumulated_stats.total_shapes_rendered << "\n\n";
        
        // Memory statistics
        auto memory_report = memory_stats.get_memory_report();
        oss << "--- Memory Usage ---\n";
        oss << "Total Memory: " << memory_report.total_mb << " MB\n";
        oss << "Geometry Memory: " << memory_report.geometry_mb << " MB\n";
        oss << "Vertex Memory: " << memory_report.vertex_mb << " MB\n";
        oss << "Efficiency: " << memory_report.efficiency_rating << 
               " (" << memory_report.efficiency_percentage << "%)\n\n";
        
        // Educational analysis
        oss << "--- Educational Analysis ---\n";
        oss << "Overall Performance Rating: " << get_performance_rating_string() << "\n";
        oss << "Primary Bottleneck: " << get_bottleneck_string() << "\n";
        oss << "Overall Efficiency: " << (educational_analysis.overall_efficiency * 100.0f) << "%\n";
        
        if (educational_analysis.suggestion_count > 0) {
            oss << "\n--- Optimization Suggestions ---\n";
            for (u8 i = 0; i < educational_analysis.suggestion_count; ++i) {
                oss << "- " << educational_analysis.optimization_suggestions[i] << "\n";
            }
        }
        
        return oss.str();
    }
    
    /** @brief Get current performance rating as string */
    const char* get_performance_rating_string() const {
        switch (educational_analysis.performance_rating) {
            case EducationalAnalysis::PerformanceRating::Excellent: return "Excellent";
            case EducationalAnalysis::PerformanceRating::Good: return "Good";
            case EducationalAnalysis::PerformanceRating::Fair: return "Fair";
            case EducationalAnalysis::PerformanceRating::Poor: return "Poor";
            case EducationalAnalysis::PerformanceRating::Critical: return "Critical";
            default: return "Unknown";
        }
    }
    
    /** @brief Get primary bottleneck as string */
    const char* get_bottleneck_string() const {
        switch (educational_analysis.primary_bottleneck) {
            case EducationalAnalysis::PrimaryBottleneck::None: return "None";
            case EducationalAnalysis::PrimaryBottleneck::CPU_Rendering: return "CPU Rendering";
            case EducationalAnalysis::PrimaryBottleneck::GPU_Overdraw: return "GPU Overdraw";
            case EducationalAnalysis::PrimaryBottleneck::Memory_Bandwidth: return "Memory Bandwidth";
            case EducationalAnalysis::PrimaryBottleneck::Draw_Calls: return "Draw Calls";
            case EducationalAnalysis::PrimaryBottleneck::Geometry_Generation: return "Geometry Generation";
            case EducationalAnalysis::PrimaryBottleneck::Cache_Misses: return "Cache Misses";
            default: return "Unknown";
        }
    }

private:
    /** @brief Update educational analysis based on current statistics */
    void update_educational_analysis() {
        educational_analysis.clear_suggestions();
        
        // Calculate efficiency scores
        educational_analysis.rendering_efficiency = calculate_rendering_efficiency();
        educational_analysis.memory_efficiency = memory_stats.memory_efficiency;
        educational_analysis.batching_efficiency = current_frame.batching_efficiency;
        educational_analysis.calculate_overall_efficiency();
        educational_analysis.update_performance_rating();
        
        // Identify primary bottleneck
        identify_primary_bottleneck();
        
        // Generate optimization suggestions
        generate_optimization_suggestions();
    }
    
    /** @brief Calculate rendering efficiency score */
    f32 calculate_rendering_efficiency() const {
        if (current_frame.debug_shapes_rendered == 0) return 1.0f;
        
        // Ideal time per shape (rough estimate)
        f32 ideal_time_per_shape = 0.1f; // 0.1ms per shape baseline
        f32 ideal_total_time = current_frame.debug_shapes_rendered * ideal_time_per_shape;
        
        return std::min(1.0f, ideal_total_time / std::max(0.001f, current_frame.debug_render_time_ms));
    }
    
    /** @brief Identify the primary performance bottleneck */
    void identify_primary_bottleneck() {
        // Simple heuristic-based bottleneck identification
        if (current_frame.debug_render_time_ms > 5.0f) {
            if (current_frame.batching_efficiency < 0.5f) {
                educational_analysis.primary_bottleneck = EducationalAnalysis::PrimaryBottleneck::Draw_Calls;
            } else if (memory_stats.memory_efficiency < 0.6f) {
                educational_analysis.primary_bottleneck = EducationalAnalysis::PrimaryBottleneck::Memory_Bandwidth;
            } else if (current_frame.debug_shapes_rendered > 1000) {
                educational_analysis.primary_bottleneck = EducationalAnalysis::PrimaryBottleneck::GPU_Overdraw;
            } else {
                educational_analysis.primary_bottleneck = EducationalAnalysis::PrimaryBottleneck::CPU_Rendering;
            }
        } else {
            educational_analysis.primary_bottleneck = EducationalAnalysis::PrimaryBottleneck::None;
        }
    }
    
    /** @brief Generate optimization suggestions based on analysis */
    void generate_optimization_suggestions() {
        if (current_frame.batching_efficiency < 0.7f) {
            educational_analysis.add_suggestion("Improve batching by grouping similar debug shapes");
            educational_analysis.add_suggestion("Consider using texture atlasing for debug primitives");
        }
        
        if (current_frame.debug_render_time_ms > 3.0f) {
            educational_analysis.add_suggestion("Reduce debug visualization complexity");
            educational_analysis.add_suggestion("Enable frustum culling for debug shapes");
        }
        
        if (memory_stats.memory_efficiency < 0.6f) {
            educational_analysis.add_suggestion("Optimize debug data structures for memory efficiency");
            educational_analysis.add_suggestion("Use object pooling for debug geometry");
        }
        
        if (current_frame.debug_draw_calls > current_frame.debug_batches_created * 2) {
            educational_analysis.add_suggestion("Optimize debug rendering pipeline to reduce draw calls");
        }
        
        if (current_frame.debug_shapes_rendered > 500) {
            educational_analysis.add_suggestion("Implement level-of-detail for distant debug elements");
            educational_analysis.add_suggestion("Consider selective debug visualization based on importance");
        }
    }
};

//=============================================================================
// Component Validation and Static Assertions
//=============================================================================

// Ensure all components satisfy the ECS Component concept
static_assert(ecs::Component<PhysicsDebugVisualization>, "PhysicsDebugVisualization must satisfy Component concept");
static_assert(ecs::Component<PhysicsDebugShape>, "PhysicsDebugShape must satisfy Component concept");
static_assert(ecs::Component<PhysicsDebugStats>, "PhysicsDebugStats must satisfy Component concept");

// Verify memory alignment for optimal performance
static_assert(alignof(PhysicsDebugVisualization) >= 16, "PhysicsDebugVisualization should be 16-byte aligned");
static_assert(alignof(PhysicsDebugShape) >= 32, "PhysicsDebugShape should be 32-byte aligned");
static_assert(alignof(PhysicsDebugStats) >= 32, "PhysicsDebugStats should be 32-byte aligned");

// Verify reasonable component sizes for cache efficiency
static_assert(sizeof(PhysicsDebugVisualization) <= 512, "PhysicsDebugVisualization should fit in reasonable cache lines");
static_assert(sizeof(PhysicsDebugShape) <= 1024, "PhysicsDebugShape should be reasonably sized for batching");

} // namespace ecscope::physics::debug