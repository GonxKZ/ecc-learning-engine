#pragma once

/**
 * @file renderer/components/render_components.hpp
 * @brief 2D Rendering Components for ECScope Educational ECS Engine - Phase 7: Renderizado 2D
 * 
 * This header provides a comprehensive set of rendering components for 2D graphics with emphasis
 * on educational clarity while maintaining high performance. It includes:
 * 
 * Core Rendering Components:
 * - RenderableSprite: Sprite rendering with texture, UV mapping, and visual properties
 * - Camera2D: 2D camera with view/projection matrices, zoom, and viewport control
 * - Material: Shader and rendering state management for advanced graphics
 * - RenderInfo: Debug information and performance metrics for educational analysis
 * 
 * Advanced Rendering Components:
 * - AnimatedSprite: Frame-based sprite animation with timing control
 * - ParticleEmitter: 2D particle system for visual effects
 * - TileMap: Efficient tile-based rendering for backgrounds and levels
 * - RenderLayer: Z-ordering and layer management for complex scenes
 * 
 * Educational Philosophy:
 * Each component includes detailed educational documentation explaining the graphics
 * concepts, mathematical foundations, and performance implications. Components are
 * designed to be inspectable, debuggable, and easy to understand.
 * 
 * Performance Characteristics:
 * - SoA-friendly data layouts for efficient batching and rendering
 * - Memory-aligned structures for optimal GPU data transfer
 * - Integration with custom allocators for minimal memory overhead
 * - Zero-overhead abstractions where possible
 * - Educational debugging without performance impact in release builds
 * 
 * ECS Integration:
 * - Full compatibility with Component concept (trivially copyable, standard layout)
 * - Works seamlessly with existing Transform component hierarchy
 * - Integrates with memory tracking system for resource monitoring
 * - Supports component relationships for complex rendering setups
 * 
 * @author ECScope Educational ECS Framework - Phase 7: Renderizado 2D
 * @date 2024
 */

#include "../../ecs/component.hpp"
#include "../../ecs/components/transform.hpp"
#include "../../memory/memory_tracker.hpp"
#include "../../core/types.hpp"
#include <array>
#include <bitset>
#include <optional>

namespace ecscope::renderer::components {

// Import commonly used types
using namespace ecscope::ecs::components;

//=============================================================================
// Forward Declarations and Type Aliases
//=============================================================================

struct TextureHandle {
    u32 id{0};           // Texture resource ID (0 = invalid/default white texture)
    u16 width{1};        // Texture width in pixels
    u16 height{1};       // Texture height in pixels
    
    constexpr bool is_valid() const noexcept { return id != 0; }
    constexpr bool operator==(const TextureHandle& other) const noexcept { return id == other.id; }
    constexpr bool operator!=(const TextureHandle& other) const noexcept { return id != other.id; }
};

struct ShaderHandle {
    u32 id{0};           // Shader program ID (0 = invalid/default shader)
    
    constexpr bool is_valid() const noexcept { return id != 0; }
    constexpr bool operator==(const ShaderHandle& other) const noexcept { return id == other.id; }
    constexpr bool operator!=(const ShaderHandle& other) const noexcept { return id != other.id; }
};

//=============================================================================
// Color and UV Coordinate Types
//=============================================================================

/**
 * @brief RGBA Color with 8-bit precision per channel
 * 
 * Standard 32-bit color representation used throughout the rendering system.
 * Provides both floating-point [0,1] and integer [0,255] access patterns.
 * 
 * Educational Context:
 * - Demonstrates color space concepts and precision trade-offs
 * - Shows how graphics hardware typically expects color data
 * - Illustrates premultiplied alpha and blending considerations
 */
struct alignas(4) Color {
    union {
        struct {
            u8 r, g, b, a;
        };
        u32 rgba;
        std::array<u8, 4> channels;
    };
    
    // Constructors
    constexpr Color() noexcept : r(255), g(255), b(255), a(255) {} // Default white
    constexpr Color(u8 red, u8 green, u8 blue, u8 alpha = 255) noexcept 
        : r(red), g(green), b(blue), a(alpha) {}
    constexpr explicit Color(u32 color) noexcept : rgba(color) {}
    constexpr Color(f32 red, f32 green, f32 blue, f32 alpha = 1.0f) noexcept
        : r(static_cast<u8>(red * 255.0f))
        , g(static_cast<u8>(green * 255.0f))
        , b(static_cast<u8>(blue * 255.0f))
        , a(static_cast<u8>(alpha * 255.0f)) {}
    
    // Float accessors (0.0 - 1.0)
    constexpr f32 red_f() const noexcept { return r / 255.0f; }
    constexpr f32 green_f() const noexcept { return g / 255.0f; }
    constexpr f32 blue_f() const noexcept { return b / 255.0f; }
    constexpr f32 alpha_f() const noexcept { return a / 255.0f; }
    
    // Predefined colors for educational examples
    static constexpr Color white() noexcept { return Color{255, 255, 255, 255}; }
    static constexpr Color black() noexcept { return Color{0, 0, 0, 255}; }
    static constexpr Color red() noexcept { return Color{255, 0, 0, 255}; }
    static constexpr Color green() noexcept { return Color{0, 255, 0, 255}; }
    static constexpr Color blue() noexcept { return Color{0, 0, 255, 255}; }
    static constexpr Color transparent() noexcept { return Color{255, 255, 255, 0}; }
    static constexpr Color cyan() noexcept { return Color{0, 255, 255, 255}; }
    static constexpr Color magenta() noexcept { return Color{255, 0, 255, 255}; }
    static constexpr Color yellow() noexcept { return Color{255, 255, 0, 255}; }
    
    // Operations
    constexpr bool operator==(const Color& other) const noexcept { return rgba == other.rgba; }
    constexpr bool operator!=(const Color& other) const noexcept { return rgba != other.rgba; }
    
    // Color blending utilities
    Color lerp(const Color& other, f32 t) const noexcept {
        f32 inv_t = 1.0f - t;
        return Color{
            static_cast<u8>(r * inv_t + other.r * t),
            static_cast<u8>(g * inv_t + other.g * t),
            static_cast<u8>(b * inv_t + other.b * t),
            static_cast<u8>(a * inv_t + other.a * t)
        };
    }
    
    Color multiply(const Color& other) const noexcept {
        return Color{
            static_cast<u8>((r * other.r) / 255),
            static_cast<u8>((g * other.g) / 255),
            static_cast<u8>((b * other.b) / 255),
            static_cast<u8>((a * other.a) / 255)
        };
    }
};

/**
 * @brief UV Coordinate Rectangle for Texture Mapping
 * 
 * Defines a rectangular region within a texture using normalized coordinates [0,1].
 * Used for sprite sheets, texture atlases, and sub-texture rendering.
 * 
 * Educational Context:
 * - Demonstrates texture coordinate systems and UV mapping
 * - Shows how sprite sheets and texture atlases optimize memory usage
 * - Illustrates the relationship between texture pixels and UV coordinates
 */
struct alignas(16) UVRect {
    f32 u{0.0f};         // Left edge (normalized 0-1)
    f32 v{0.0f};         // Top edge (normalized 0-1)  
    f32 width{1.0f};     // Width (normalized 0-1)
    f32 height{1.0f};    // Height (normalized 0-1)
    
    constexpr UVRect() noexcept = default;
    constexpr UVRect(f32 u_pos, f32 v_pos, f32 w, f32 h) noexcept 
        : u(u_pos), v(v_pos), width(w), height(h) {}
    
    // Full texture (default)
    static constexpr UVRect full_texture() noexcept { return UVRect{0.0f, 0.0f, 1.0f, 1.0f}; }
    
    // Create from pixel coordinates in a texture
    static constexpr UVRect from_pixels(i32 x, i32 y, i32 w, i32 h, i32 tex_width, i32 tex_height) noexcept {
        return UVRect{
            static_cast<f32>(x) / tex_width,
            static_cast<f32>(y) / tex_height,
            static_cast<f32>(w) / tex_width,
            static_cast<f32>(h) / tex_height
        };
    }
    
    // Utility functions
    constexpr f32 right() const noexcept { return u + width; }
    constexpr f32 bottom() const noexcept { return v + height; }
    
    // Validation
    constexpr bool is_valid() const noexcept {
        return u >= 0.0f && v >= 0.0f && width > 0.0f && height > 0.0f &&
               u + width <= 1.0f && v + height <= 1.0f;
    }
};

//=============================================================================
// Core Rendering Components
//=============================================================================

/**
 * @brief Renderable Sprite Component
 * 
 * Core component for 2D sprite rendering. Contains all information needed to render
 * a textured quad including texture reference, UV coordinates, color modulation,
 * and rendering properties.
 * 
 * Educational Context:
 * This component demonstrates:
 * - Texture mapping and UV coordinate systems
 * - Color modulation and blending modes
 * - Z-ordering and depth testing in 2D
 * - Sprite sheet and texture atlas usage
 * - Performance optimization through batching-friendly design
 * 
 * Performance Design:
 * - Struct-of-Arrays friendly layout for efficient batching
 * - Minimal size (64 bytes) for cache efficiency
 * - Hot data (texture, color, UV) grouped together
 * - Cold data (flags, debug info) placed at end
 */
struct alignas(16) RenderableSprite {
    //-------------------------------------------------------------------------
    // Primary Rendering Data (hot path)
    //-------------------------------------------------------------------------
    
    /** 
     * @brief Texture handle for sprite rendering
     * 
     * References a texture resource in the texture manager. A value of 0
     * indicates no texture (will use default white texture).
     * 
     * Educational Note:
     * Modern graphics uses texture handles/IDs rather than direct pointers
     * to enable GPU-side resource management and reduce CPU-GPU synchronization.
     */
    TextureHandle texture{};
    
    /** 
     * @brief UV rectangle defining texture region to render
     * 
     * Specifies which part of the texture to use for this sprite.
     * Enables sprite sheets, texture atlases, and animation frames.
     * 
     * Default: Full texture (0,0,1,1)
     * 
     * Educational Context:
     * UV coordinates are normalized (0-1) texture coordinates:
     * - (0,0) = top-left corner of texture
     * - (1,1) = bottom-right corner of texture
     * - Values outside [0,1] can be used for texture wrapping
     */
    UVRect uv_rect{UVRect::full_texture()};
    
    /** 
     * @brief Color modulation (tint) applied to sprite
     * 
     * Multiplied with texture colors during rendering. Allows for:
     * - Tinting sprites different colors
     * - Fading sprites in/out (alpha modulation)
     * - Creating visual effects without multiple textures
     * 
     * Default: White (no modulation)
     * 
     * Mathematical Operation: final_color = texture_color * modulation_color
     */
    Color color_modulation{Color::white()};
    
    //-------------------------------------------------------------------------
    // Rendering Properties
    //-------------------------------------------------------------------------
    
    /** 
     * @brief Z-order for depth sorting
     * 
     * Determines rendering order in 2D. Higher values render on top.
     * Used by the renderer to sort sprites before drawing.
     * 
     * Range: Typically 0-1000, but can be any float value
     * - Background elements: 0-100
     * - Game objects: 100-500
     * - UI elements: 500-1000
     * - Debug overlays: 1000+
     * 
     * Educational Note:
     * Z-ordering in 2D is crucial for proper visual layering.
     * Unlike 3D depth buffers, 2D requires explicit sorting.
     */
    f32 z_order{0.0f};
    
    /** 
     * @brief Size multiplier relative to texture dimensions
     * 
     * Scales the sprite from its base texture size. Allows the same
     * texture to be rendered at different sizes efficiently.
     * 
     * Default: (1,1) = render at texture's native size
     * Values: (0.5,0.5) = half size, (2,2) = double size
     * 
     * Educational Context:
     * Scaling sprites in shaders is more efficient than creating
     * multiple texture sizes, but be aware of filtering artifacts.
     */
    struct {
        f32 x{1.0f};
        f32 y{1.0f};
    } size_multiplier;
    
    /** 
     * @brief Pivot point for rotation and scaling
     * 
     * Normalized coordinates (0-1) within the sprite rectangle.
     * Determines the center point for transformations.
     * 
     * Common values:
     * - (0.5, 0.5) = center (default for most sprites)
     * - (0.0, 0.0) = top-left corner
     * - (0.5, 1.0) = bottom-center (good for characters)
     * 
     * Educational Note:
     * Pivot points are crucial for natural-looking rotations and scaling.
     * Characters often rotate around their bottom-center point.
     */
    struct {
        f32 x{0.5f};
        f32 y{0.5f};
    } pivot;
    
    //-------------------------------------------------------------------------
    // Rendering State and Behavior
    //-------------------------------------------------------------------------
    
    /** 
     * @brief Blending mode for transparency and effects
     * 
     * Determines how this sprite's colors combine with existing pixels.
     * 
     * Educational Context:
     * Different blend modes create different visual effects:
     * - Alpha: Standard transparency (most common)
     * - Additive: Colors add together (good for lights, effects)
     * - Multiply: Colors multiply (good for shadows, overlays)
     * - Screen: Opposite of multiply (brightening effect)
     */
    enum class BlendMode : u8 {
        Alpha = 0,      ///< Standard alpha blending (Src*SrcAlpha + Dst*(1-SrcAlpha))
        Additive,       ///< Additive blending (Src + Dst) - brightens
        Multiply,       ///< Multiplicative blending (Src * Dst) - darkens
        Screen,         ///< Screen blending (1 - (1-Src)*(1-Dst)) - brightens
        Premultiplied   ///< Premultiplied alpha (Src + Dst*(1-SrcAlpha))
    } blend_mode{BlendMode::Alpha};
    
    /** 
     * @brief Texture filtering mode
     * 
     * How pixels are interpolated when texture is scaled.
     * 
     * Educational Context:
     * - Nearest: Sharp, pixelated look (good for pixel art)
     * - Linear: Smooth, blurred look (good for realistic graphics)
     * - Choice affects both performance and visual style
     */
    enum class FilterMode : u8 {
        Nearest = 0,    ///< Nearest neighbor (pixelated, fast)
        Linear          ///< Bilinear filtering (smooth, slightly slower)
    } filter_mode{FilterMode::Linear};
    
    /** 
     * @brief Sprite behavior flags
     */
    union {
        u32 flags;
        struct {
            u32 visible : 1;                ///< Whether sprite should be rendered
            u32 flip_horizontal : 1;        ///< Horizontally flip texture coordinates
            u32 flip_vertical : 1;          ///< Vertically flip texture coordinates
            u32 world_space_ui : 1;         ///< Render in world space vs screen space
            u32 depth_test_enabled : 1;     ///< Use depth testing (for 2.5D effects)
            u32 depth_write_enabled : 1;    ///< Write to depth buffer
            u32 cull_when_offscreen : 1;    ///< Skip rendering when outside view
            u32 receive_shadows : 1;        ///< Affected by shadow rendering
            u32 cast_shadows : 1;           ///< Casts shadows on other objects
            u32 affected_by_lighting : 1;   ///< Affected by 2D lighting system
            u32 high_quality_filtering : 1; ///< Use anisotropic filtering
            u32 reserved : 21;              ///< Reserved for future use
        };
    } render_flags{.flags = 1}; // Default: visible only
    
    //-------------------------------------------------------------------------
    // Performance and Debug Information
    //-------------------------------------------------------------------------
    
    /** 
     * @brief Rendering performance metrics
     * 
     * Educational data for understanding rendering costs and optimization.
     */
    mutable struct {
        u32 times_rendered{0};          ///< How many frames this sprite was drawn
        u32 batch_breaks_caused{0};     ///< How many times this sprite broke batching
        f32 last_render_time{0.0f};     ///< GPU time for last render (if available)
        u32 texture_cache_misses{0};    ///< Texture binding cache misses
        u16 current_batch_id{0};        ///< Which batch this sprite is currently in
        u16 vertices_generated{4};      ///< Number of vertices (typically 4 for quads)
    } performance_info;
    
    /** 
     * @brief Debug visualization information
     */
    mutable struct {
        Color debug_tint{Color::white()}; ///< Debug overlay color
        bool show_bounds{false};          ///< Show sprite bounding box
        bool show_pivot{false};           ///< Show pivot point
        bool show_uv_coords{false};       ///< Show UV coordinate overlay
        f32 debug_alpha{1.0f};            ///< Debug overlay transparency
    } debug_info;
    
    //-------------------------------------------------------------------------
    // Constructors and Factory Methods
    //-------------------------------------------------------------------------
    
    constexpr RenderableSprite() noexcept = default;
    
    constexpr RenderableSprite(TextureHandle tex) noexcept 
        : texture(tex) {}
    
    constexpr RenderableSprite(TextureHandle tex, const UVRect& uv) noexcept 
        : texture(tex), uv_rect(uv) {}
    
    constexpr RenderableSprite(TextureHandle tex, const UVRect& uv, Color color) noexcept 
        : texture(tex), uv_rect(uv), color_modulation(color) {}
    
    // Factory methods for common sprite types
    static RenderableSprite create_textured_quad(TextureHandle texture, f32 z_order = 0.0f) noexcept {
        RenderableSprite sprite{texture};
        sprite.z_order = z_order;
        return sprite;
    }
    
    static RenderableSprite create_colored_quad(Color color, f32 z_order = 0.0f) noexcept {
        RenderableSprite sprite{TextureHandle{}}; // Use default white texture
        sprite.color_modulation = color;
        sprite.z_order = z_order;
        return sprite;
    }
    
    static RenderableSprite create_sprite_from_atlas(TextureHandle atlas, i32 x, i32 y, i32 w, i32 h, 
                                                   i32 atlas_width, i32 atlas_height, f32 z_order = 0.0f) noexcept {
        RenderableSprite sprite{atlas};
        sprite.uv_rect = UVRect::from_pixels(x, y, w, h, atlas_width, atlas_height);
        sprite.z_order = z_order;
        return sprite;
    }
    
    //-------------------------------------------------------------------------
    // Sprite Manipulation Interface
    //-------------------------------------------------------------------------
    
    /** @brief Set sprite visibility */
    void set_visible(bool visible) noexcept {
        render_flags.visible = visible ? 1 : 0;
    }
    
    /** @brief Check if sprite is visible */
    bool is_visible() const noexcept {
        return render_flags.visible != 0;
    }
    
    /** @brief Set sprite color with optional alpha */
    void set_color(Color color) noexcept {
        color_modulation = color;
    }
    
    /** @brief Set sprite alpha (transparency) while preserving RGB */
    void set_alpha(f32 alpha) noexcept {
        color_modulation.a = static_cast<u8>(alpha * 255.0f);
    }
    
    /** @brief Get sprite alpha as float [0,1] */
    f32 get_alpha() const noexcept {
        return color_modulation.alpha_f();
    }
    
    /** @brief Set texture flip flags */
    void set_flip(bool horizontal, bool vertical) noexcept {
        render_flags.flip_horizontal = horizontal ? 1 : 0;
        render_flags.flip_vertical = vertical ? 1 : 0;
    }
    
    /** @brief Set size multiplier (scale sprite) */
    void set_size(f32 width_scale, f32 height_scale) noexcept {
        size_multiplier.x = width_scale;
        size_multiplier.y = height_scale;
    }
    
    /** @brief Set pivot point for transformations */
    void set_pivot(f32 pivot_x, f32 pivot_y) noexcept {
        pivot.x = pivot_x;
        pivot.y = pivot_y;
    }
    
    /** @brief Set Z-order for depth sorting */
    void set_z_order(f32 new_z_order) noexcept {
        z_order = new_z_order;
    }
    
    //-------------------------------------------------------------------------
    // Utility Functions
    //-------------------------------------------------------------------------
    
    /** @brief Calculate world-space size based on texture and multiplier */
    struct Size2D {
        f32 width, height;
    };
    
    Size2D calculate_world_size() const noexcept {
        f32 base_width = static_cast<f32>(texture.width) * uv_rect.width;
        f32 base_height = static_cast<f32>(texture.height) * uv_rect.height;
        return {base_width * size_multiplier.x, base_height * size_multiplier.y};
    }
    
    /** @brief Check if sprite uses transparency */
    bool uses_transparency() const noexcept {
        return color_modulation.a < 255 || blend_mode != BlendMode::Alpha;
    }
    
    /** @brief Estimate rendering cost (for educational analysis) */
    f32 estimate_render_cost() const noexcept {
        f32 base_cost = 1.0f;
        
        // Transparency increases cost
        if (uses_transparency()) base_cost *= 1.5f;
        
        // Special blend modes increase cost
        if (blend_mode != BlendMode::Alpha) base_cost *= 1.2f;
        
        // High quality filtering increases cost
        if (render_flags.high_quality_filtering) base_cost *= 1.3f;
        
        // Shadows increase cost significantly
        if (render_flags.cast_shadows || render_flags.receive_shadows) base_cost *= 2.0f;
        
        return base_cost;
    }
    
    /** @brief Validate sprite configuration */
    bool is_valid() const noexcept {
        return uv_rect.is_valid() &&
               pivot.x >= 0.0f && pivot.x <= 1.0f &&
               pivot.y >= 0.0f && pivot.y <= 1.0f &&
               size_multiplier.x > 0.0f && size_multiplier.y > 0.0f;
    }
    
    /** @brief Get detailed sprite information for educational display */
    struct SpriteInfo {
        bool has_texture;
        f32 world_width, world_height;
        f32 texture_memory_mb;
        u32 estimated_triangles;
        f32 screen_coverage_percent;
        const char* blend_mode_name;
        const char* filter_mode_name;
    };
    
    SpriteInfo get_sprite_info() const noexcept;
};

/**
 * @brief 2D Camera Component
 * 
 * Defines a 2D camera with position, rotation, zoom, and viewport management.
 * Handles view and projection matrix generation for 2D rendering.
 * 
 * Educational Context:
 * This component demonstrates:
 * - 2D camera mathematics and coordinate systems
 * - View and projection matrix construction
 * - Viewport management and screen coordinate mapping
 * - Zoom and pan functionality implementation
 * - Frustum culling for performance optimization
 */
struct alignas(32) Camera2D {
    //-------------------------------------------------------------------------
    // Camera Transform Properties
    //-------------------------------------------------------------------------
    
    /** 
     * @brief Camera position in world coordinates
     * 
     * The camera looks at this point in the world. All rendering is relative
     * to this position.
     * 
     * Educational Note:
     * In 2D, the camera position is typically the center of the view.
     * Moving the camera right moves the world left in screen space.
     */
    struct {
        f32 x{0.0f};
        f32 y{0.0f};
    } position;
    
    /** 
     * @brief Camera rotation in radians
     * 
     * Rotation around the camera's center (Z-axis in 3D terms).
     * Positive rotation is counter-clockwise.
     * 
     * Range: Typically -π to π, but can be any value
     * Common values: 0 = no rotation, π/2 = 90° CCW, π = 180°
     */
    f32 rotation{0.0f};
    
    /** 
     * @brief Camera zoom factor
     * 
     * Multiplier for the camera's view scale. Higher zoom = closer view.
     * 
     * Values:
     * - 1.0 = default zoom level
     * - 2.0 = 2x zoom (objects appear twice as large)
     * - 0.5 = 0.5x zoom (objects appear half size, see more area)
     * 
     * Educational Context:
     * Zoom is implemented by scaling the view matrix, which is more
     * efficient than moving the camera forward/backward in 3D.
     */
    f32 zoom{1.0f};
    
    //-------------------------------------------------------------------------
    // Viewport and Projection Properties
    //-------------------------------------------------------------------------
    
    /** 
     * @brief Viewport rectangle in screen coordinates
     * 
     * Defines the screen region this camera renders to.
     * Allows for multiple cameras (split-screen, mini-maps, etc.).
     * 
     * Coordinates: (0,0) = top-left of screen
     * Default: Full screen viewport
     */
    struct ViewportRect {
        i32 x{0};           // Left edge in pixels
        i32 y{0};           // Top edge in pixels  
        i32 width{1920};    // Width in pixels
        i32 height{1080};   // Height in pixels
        
        f32 aspect_ratio() const noexcept {
            return height > 0 ? static_cast<f32>(width) / height : 1.0f;
        }
    } viewport;
    
    /** 
     * @brief Projection type for 2D rendering
     * 
     * Determines how world coordinates map to screen coordinates.
     * 
     * Educational Context:
     * - Orthographic: Parallel projection, no perspective distortion
     * - Perspective: Vanishing points, objects further away appear smaller
     * - Screen: Direct pixel-to-pixel mapping for UI elements
     */
    enum class ProjectionType : u8 {
        Orthographic = 0,   ///< Parallel projection (standard for 2D)
        Perspective,        ///< Perspective projection (for 2.5D effects)
        Screen             ///< Screen-space projection (for UI)
    } projection_type{ProjectionType::Orthographic};
    
    /** 
     * @brief Orthographic projection bounds
     * 
     * Defines the world-space rectangle visible by the camera.
     * Calculated automatically based on viewport and zoom.
     */
    struct OrthoBounds {
        f32 left{-960.0f};
        f32 right{960.0f};
        f32 bottom{-540.0f};
        f32 top{540.0f};
        f32 near{-1.0f};    // Near clipping plane
        f32 far{1.0f};      // Far clipping plane
    } ortho_bounds;
    
    //-------------------------------------------------------------------------
    // Camera Behavior and Constraints
    //-------------------------------------------------------------------------
    
    /** 
     * @brief Zoom constraints
     */
    struct ZoomLimits {
        f32 min_zoom{0.1f};     // Minimum zoom level (furthest out)
        f32 max_zoom{10.0f};    // Maximum zoom level (closest in)
        f32 zoom_speed{1.0f};   // Speed multiplier for zoom changes
    } zoom_limits;
    
    /** 
     * @brief Movement constraints (optional world bounds)
     */
    struct MovementLimits {
        bool constrain_movement{false};  // Whether to apply constraints
        f32 min_x{-1000.0f};            // Left world boundary
        f32 max_x{1000.0f};             // Right world boundary  
        f32 min_y{-1000.0f};            // Bottom world boundary
        f32 max_y{1000.0f};             // Top world boundary
    } movement_limits;
    
    /** 
     * @brief Camera behavior flags
     */
    union {
        u32 flags;
        struct {
            u32 active : 1;                 ///< Whether camera is active for rendering
            u32 follow_target : 1;          ///< Whether camera follows a target entity
            u32 smooth_follow : 1;          ///< Use smooth interpolation when following
            u32 constrain_to_bounds : 1;    ///< Apply movement constraints
            u32 auto_resize_viewport : 1;   ///< Automatically resize viewport with window
            u32 clear_before_render : 1;    ///< Clear render target before rendering
            u32 render_debug_info : 1;      ///< Show camera debug information
            u32 frustum_culling : 1;        ///< Enable frustum culling optimization
            u32 pixel_perfect : 1;          ///< Snap positions to pixel boundaries
            u32 reserved : 23;
        };
    } camera_flags{.flags = 0x31}; // Active, clear_before_render, frustum_culling
    
    //-------------------------------------------------------------------------
    // Target Following System
    //-------------------------------------------------------------------------
    
    /** 
     * @brief Target entity to follow (0 = no target)
     */
    u32 follow_target_entity{0};
    
    /** 
     * @brief Follow behavior parameters
     */
    struct FollowParams {
        f32 follow_speed{5.0f};         // Speed of camera interpolation
        f32 look_ahead_distance{50.0f}; // Distance to look ahead in movement direction
        f32 dead_zone_radius{10.0f};    // Radius where camera doesn't move
        struct {
            f32 x{0.0f};                // Offset from target position
            f32 y{0.0f};
        } offset;
    } follow_params;
    
    //-------------------------------------------------------------------------
    // Cached Matrix Data (Performance Optimization)
    //-------------------------------------------------------------------------
    
    /** 
     * @brief Cached transformation matrices
     * 
     * Pre-calculated matrices updated when camera properties change.
     * Avoids recalculating matrices every frame for performance.
     */
    mutable struct MatrixCache {
        // View matrix (world-to-camera transform)
        struct Matrix3x3 {
            f32 m[9];
            
            Matrix3x3() noexcept {
                // Identity matrix
                m[0] = 1.0f; m[1] = 0.0f; m[2] = 0.0f;
                m[3] = 0.0f; m[4] = 1.0f; m[5] = 0.0f;
                m[6] = 0.0f; m[7] = 0.0f; m[8] = 1.0f;
            }
        } view_matrix;
        
        // Projection matrix (camera-to-screen transform)
        Matrix3x3 projection_matrix;
        
        // Combined view-projection matrix
        Matrix3x3 view_projection_matrix;
        
        // Inverse matrices for screen-to-world conversion
        Matrix3x3 inverse_view_matrix;
        Matrix3x3 inverse_projection_matrix;
        Matrix3x3 inverse_view_projection_matrix;
        
        // Cache validity flags
        bool matrices_dirty{true};          // Need to recalculate matrices
        u32 last_update_frame{0};           // Frame when matrices were last updated
        f32 last_aspect_ratio{1.0f};        // Last calculated aspect ratio
    } matrix_cache;
    
    //-------------------------------------------------------------------------
    // Performance and Debug Information
    //-------------------------------------------------------------------------
    
    /** 
     * @brief Rendering statistics for educational analysis
     */
    mutable struct RenderStats {
        u32 objects_rendered{0};        // Objects rendered by this camera
        u32 objects_culled{0};          // Objects culled by frustum
        f32 culling_efficiency{0.0f};   // Percentage of objects culled
        f32 render_time{0.0f};          // Time spent rendering this camera
        u32 draw_calls{0};              // Number of draw calls issued
        u32 vertices_rendered{0};       // Total vertices rendered
    } render_stats;
    
    /** 
     * @brief Debug visualization settings
     */
    struct DebugSettings {
        Color frustum_color{Color::cyan()};     // Color for frustum bounds
        Color target_color{Color::red()};       // Color for follow target
        Color bounds_color{Color::yellow()};    // Color for movement bounds
        bool show_frustum{false};               // Show camera frustum
        bool show_target{false};                // Show follow target
        bool show_bounds{false};                // Show movement constraints
        bool show_grid{false};                  // Show world grid
        f32 grid_spacing{100.0f};               // Grid line spacing
    } debug_settings;
    
    //-------------------------------------------------------------------------
    // Constructor and Factory Methods
    //-------------------------------------------------------------------------
    
    constexpr Camera2D() noexcept {
        update_ortho_bounds();
    }
    
    Camera2D(f32 x, f32 y, f32 zoom_level = 1.0f) noexcept 
        : position{x, y}, zoom(zoom_level) {
        update_ortho_bounds();
    }
    
    // Factory methods for common camera setups
    static Camera2D create_main_camera(i32 screen_width, i32 screen_height) noexcept {
        Camera2D camera;
        camera.viewport = {0, 0, screen_width, screen_height};
        camera.camera_flags.active = 1;
        camera.update_ortho_bounds();
        return camera;
    }
    
    static Camera2D create_ui_camera(i32 screen_width, i32 screen_height) noexcept {
        Camera2D camera;
        camera.viewport = {0, 0, screen_width, screen_height};
        camera.projection_type = ProjectionType::Screen;
        camera.camera_flags.active = 1;
        camera.camera_flags.pixel_perfect = 1;
        camera.update_ortho_bounds();
        return camera;
    }
    
    static Camera2D create_minimap_camera(i32 x, i32 y, i32 width, i32 height, f32 zoom_level) noexcept {
        Camera2D camera;
        camera.viewport = {x, y, width, height};
        camera.zoom = zoom_level;
        camera.camera_flags.active = 1;
        camera.camera_flags.clear_before_render = 0; // Don't clear for overlay
        camera.update_ortho_bounds();
        return camera;
    }
    
    //-------------------------------------------------------------------------
    // Camera Control Interface
    //-------------------------------------------------------------------------
    
    /** @brief Set camera position */
    void set_position(f32 x, f32 y) noexcept {
        position.x = x;
        position.y = y;
        apply_movement_constraints();
        mark_matrices_dirty();
    }
    
    /** @brief Move camera by offset */
    void move(f32 dx, f32 dy) noexcept {
        set_position(position.x + dx, position.y + dy);
    }
    
    /** @brief Set camera zoom with clamping */
    void set_zoom(f32 new_zoom) noexcept {
        zoom = std::clamp(new_zoom, zoom_limits.min_zoom, zoom_limits.max_zoom);
        update_ortho_bounds();
        mark_matrices_dirty();
    }
    
    /** @brief Zoom by factor (multiply current zoom) */
    void zoom_by(f32 factor) noexcept {
        set_zoom(zoom * factor);
    }
    
    /** @brief Set camera rotation */
    void set_rotation(f32 angle_radians) noexcept {
        rotation = angle_radians;
        mark_matrices_dirty();
    }
    
    /** @brief Rotate camera by angle */
    void rotate(f32 angle_radians) noexcept {
        set_rotation(rotation + angle_radians);
    }
    
    /** @brief Set viewport size and update projection */
    void set_viewport(i32 x, i32 y, i32 width, i32 height) noexcept {
        viewport = {x, y, width, height};
        update_ortho_bounds();
        mark_matrices_dirty();
    }
    
    /** @brief Set follow target entity */
    void set_follow_target(u32 entity_id, bool smooth = true) noexcept {
        follow_target_entity = entity_id;
        camera_flags.follow_target = (entity_id != 0) ? 1 : 0;
        camera_flags.smooth_follow = smooth ? 1 : 0;
    }
    
    /** @brief Clear follow target */
    void clear_follow_target() noexcept {
        follow_target_entity = 0;
        camera_flags.follow_target = 0;
    }
    
    //-------------------------------------------------------------------------
    // Coordinate Conversion
    //-------------------------------------------------------------------------
    
    /** @brief Convert world coordinates to screen coordinates */
    struct Point2D {
        f32 x, y;
    };
    
    Point2D world_to_screen(f32 world_x, f32 world_y) const noexcept {
        update_matrices_if_dirty();
        
        // Apply view-projection transformation
        const auto& vp = matrix_cache.view_projection_matrix;
        f32 screen_x = vp.m[0] * world_x + vp.m[1] * world_y + vp.m[2];
        f32 screen_y = vp.m[3] * world_x + vp.m[4] * world_y + vp.m[5];
        
        // Convert from NDC [-1,1] to screen coordinates
        return {
            (screen_x + 1.0f) * 0.5f * viewport.width + viewport.x,
            (1.0f - screen_y) * 0.5f * viewport.height + viewport.y
        };
    }
    
    /** @brief Convert screen coordinates to world coordinates */
    Point2D screen_to_world(f32 screen_x, f32 screen_y) const noexcept {
        update_matrices_if_dirty();
        
        // Convert screen coordinates to NDC [-1,1]
        f32 ndc_x = ((screen_x - viewport.x) / viewport.width) * 2.0f - 1.0f;
        f32 ndc_y = 1.0f - ((screen_y - viewport.y) / viewport.height) * 2.0f;
        
        // Apply inverse view-projection transformation
        const auto& ivp = matrix_cache.inverse_view_projection_matrix;
        return {
            ivp.m[0] * ndc_x + ivp.m[1] * ndc_y + ivp.m[2],
            ivp.m[3] * ndc_x + ivp.m[4] * ndc_y + ivp.m[5]
        };
    }
    
    //-------------------------------------------------------------------------
    // Frustum Culling and Visibility Testing
    //-------------------------------------------------------------------------
    
    /** @brief Get camera frustum bounds in world coordinates */
    struct FrustumBounds {
        f32 left, right, bottom, top;
    };
    
    FrustumBounds get_frustum_bounds() const noexcept {
        f32 half_width = (ortho_bounds.right - ortho_bounds.left) * 0.5f;
        f32 half_height = (ortho_bounds.top - ortho_bounds.bottom) * 0.5f;
        
        return {
            position.x - half_width,
            position.x + half_width,
            position.y - half_height,
            position.y + half_height
        };
    }
    
    /** @brief Test if point is visible by camera */
    bool is_point_visible(f32 world_x, f32 world_y) const noexcept {
        if (!camera_flags.frustum_culling) return true;
        
        auto bounds = get_frustum_bounds();
        return world_x >= bounds.left && world_x <= bounds.right &&
               world_y >= bounds.bottom && world_y <= bounds.top;
    }
    
    /** @brief Test if rectangle is visible by camera */
    bool is_rect_visible(f32 x, f32 y, f32 width, f32 height) const noexcept {
        if (!camera_flags.frustum_culling) return true;
        
        auto bounds = get_frustum_bounds();
        return !(x + width < bounds.left || x > bounds.right ||
                 y + height < bounds.bottom || y > bounds.top);
    }
    
    //-------------------------------------------------------------------------
    // Matrix Access (for renderer)
    //-------------------------------------------------------------------------
    
    /** @brief Get view matrix (updates cache if needed) */
    const f32* get_view_matrix() const noexcept {
        update_matrices_if_dirty();
        return matrix_cache.view_matrix.m;
    }
    
    /** @brief Get projection matrix (updates cache if needed) */
    const f32* get_projection_matrix() const noexcept {
        update_matrices_if_dirty();
        return matrix_cache.projection_matrix.m;
    }
    
    /** @brief Get combined view-projection matrix (updates cache if needed) */
    const f32* get_view_projection_matrix() const noexcept {
        update_matrices_if_dirty();
        return matrix_cache.view_projection_matrix.m;
    }
    
    //-------------------------------------------------------------------------
    // Utility Functions
    //-------------------------------------------------------------------------
    
    /** @brief Calculate world units per pixel at current zoom */
    f32 get_world_units_per_pixel() const noexcept {
        return (ortho_bounds.right - ortho_bounds.left) / viewport.width;
    }
    
    /** @brief Calculate pixels per world unit at current zoom */
    f32 get_pixels_per_world_unit() const noexcept {
        return viewport.width / (ortho_bounds.right - ortho_bounds.left);
    }
    
    /** @brief Get camera information for educational display */
    struct CameraInfo {
        f32 world_width, world_height;      // World space dimensions visible
        f32 pixels_per_unit;                // Pixel density
        f32 frustum_area;                   // Total visible area
        const char* projection_type_name;   // Human-readable projection type
        bool is_following_target;           // Whether actively following
        f32 effective_zoom;                 // Calculated zoom including constraints
    };
    
    CameraInfo get_camera_info() const noexcept;
    
    /** @brief Validate camera configuration */
    bool is_valid() const noexcept {
        return viewport.width > 0 && viewport.height > 0 &&
               zoom > 0.0f && zoom >= zoom_limits.min_zoom && zoom <= zoom_limits.max_zoom &&
               ortho_bounds.right > ortho_bounds.left &&
               ortho_bounds.top > ortho_bounds.bottom;
    }
    
private:
    /** @brief Update orthographic projection bounds based on viewport and zoom */
    void update_ortho_bounds() noexcept {
        f32 aspect = viewport.aspect_ratio();
        f32 half_height = 540.0f / zoom; // Base height, adjust as needed
        f32 half_width = half_height * aspect;
        
        ortho_bounds.left = -half_width;
        ortho_bounds.right = half_width;
        ortho_bounds.bottom = -half_height;
        ortho_bounds.top = half_height;
    }
    
    /** @brief Apply movement constraints if enabled */
    void apply_movement_constraints() noexcept {
        if (!movement_limits.constrain_movement) return;
        
        position.x = std::clamp(position.x, movement_limits.min_x, movement_limits.max_x);
        position.y = std::clamp(position.y, movement_limits.min_y, movement_limits.max_y);
    }
    
    /** @brief Mark matrices as needing recalculation */
    void mark_matrices_dirty() const noexcept {
        matrix_cache.matrices_dirty = true;
    }
    
    /** @brief Update cached matrices if they're dirty */
    void update_matrices_if_dirty() const noexcept {
        if (matrix_cache.matrices_dirty) {
            calculate_matrices();
            matrix_cache.matrices_dirty = false;
        }
    }
    
    /** @brief Calculate all transformation matrices */
    void calculate_matrices() const noexcept;
};

//=============================================================================
// Advanced Rendering Components
//=============================================================================

/**
 * @brief Material Component for Advanced Rendering
 * 
 * Defines rendering material properties including shaders, uniforms,
 * and render states. Enables advanced graphics techniques and effects.
 * 
 * Educational Context:
 * This component demonstrates:
 * - Shader programming concepts and material systems
 * - Uniform buffer management and GPU resource binding
 * - Render state management and graphics pipeline control
 * - Material property organization for complex rendering
 */
struct alignas(32) Material {
    //-------------------------------------------------------------------------
    // Shader Program
    //-------------------------------------------------------------------------
    
    /** 
     * @brief Shader program handle
     * 
     * References a compiled shader program containing vertex and fragment shaders.
     * Default (ID 0) uses the engine's built-in sprite shader.
     */
    ShaderHandle shader{};
    
    /** 
     * @brief Shader uniform values
     * 
     * Storage for shader uniform variables. These values are uploaded to the GPU
     * when the material is bound for rendering.
     * 
     * Educational Note:
     * Uniforms are global variables in shaders that remain constant for all
     * vertices/pixels in a draw call. They're perfect for material properties.
     */
    static constexpr usize MAX_UNIFORMS = 16;
    struct UniformValue {
        enum class Type : u8 {
            None = 0,
            Float, Float2, Float3, Float4,
            Int, Int2, Int3, Int4,
            Matrix3, Matrix4,
            Texture2D
        };
        
        Type type{Type::None};
        alignas(16) union {
            f32 float_value;
            f32 float2_value[2];
            f32 float3_value[3];
            f32 float4_value[4];
            i32 int_value;
            i32 int2_value[2];
            i32 int3_value[3];
            i32 int4_value[4];
            f32 matrix3_value[9];
            f32 matrix4_value[16];
            TextureHandle texture_value;
        } data{};
        
        constexpr UniformValue() noexcept = default;
        constexpr explicit UniformValue(f32 value) noexcept : type(Type::Float) { data.float_value = value; }
        constexpr UniformValue(f32 x, f32 y) noexcept : type(Type::Float2) { 
            data.float2_value[0] = x; data.float2_value[1] = y; 
        }
        constexpr UniformValue(f32 x, f32 y, f32 z) noexcept : type(Type::Float3) { 
            data.float3_value[0] = x; data.float3_value[1] = y; data.float3_value[2] = z; 
        }
        constexpr UniformValue(f32 x, f32 y, f32 z, f32 w) noexcept : type(Type::Float4) { 
            data.float4_value[0] = x; data.float4_value[1] = y; data.float4_value[2] = z; data.float4_value[3] = w; 
        }
        constexpr explicit UniformValue(TextureHandle texture) noexcept : type(Type::Texture2D) { 
            data.texture_value = texture; 
        }
    };
    
    std::array<UniformValue, MAX_UNIFORMS> uniforms{};
    u8 uniform_count{0};
    
    //-------------------------------------------------------------------------
    // Render State
    //-------------------------------------------------------------------------
    
    /** 
     * @brief Blend state for transparency and compositing
     */
    struct BlendState {
        enum class Factor : u8 {
            Zero = 0, One, SrcColor, OneMinusSrcColor, DstColor, OneMinusDstColor,
            SrcAlpha, OneMinusSrcAlpha, DstAlpha, OneMinusDstAlpha, ConstantColor, OneMinusConstantColor
        };
        
        bool blend_enabled{true};
        Factor src_color_factor{Factor::SrcAlpha};
        Factor dst_color_factor{Factor::OneMinusSrcAlpha};
        Factor src_alpha_factor{Factor::One};
        Factor dst_alpha_factor{Factor::OneMinusSrcAlpha};
        
        enum class Equation : u8 {
            Add = 0, Subtract, ReverseSubtract, Min, Max
        } blend_equation{Equation::Add};
        
        Color constant_color{Color::white()};
    } blend_state;
    
    /** 
     * @brief Depth and stencil state
     */
    struct DepthState {
        bool depth_test_enabled{false};
        bool depth_write_enabled{true};
        
        enum class CompareFunc : u8 {
            Never = 0, Less, Equal, LessEqual, Greater, NotEqual, GreaterEqual, Always
        } depth_compare{CompareFunc::Less};
        
        f32 depth_bias{0.0f};
        f32 depth_bias_clamp{0.0f};
        f32 slope_scaled_depth_bias{0.0f};
    } depth_state;
    
    /** 
     * @brief Rasterization state
     */
    struct RasterState {
        enum class CullMode : u8 {
            None = 0, Front, Back
        } cull_mode{CullMode::Back};
        
        enum class FillMode : u8 {
            Solid = 0, Wireframe, Point
        } fill_mode{FillMode::Solid};
        
        bool scissor_test_enabled{false};
        struct ScissorRect {
            i32 x{0}, y{0}, width{1920}, height{1080};
        } scissor_rect;
    } raster_state;
    
    //-------------------------------------------------------------------------
    // Material Properties and Flags
    //-------------------------------------------------------------------------
    
    /** 
     * @brief Material behavior flags
     */
    union {
        u32 flags;
        struct {
            u32 transparent : 1;            ///< Material uses transparency
            u32 two_sided : 1;              ///< Disable backface culling
            u32 cast_shadows : 1;           ///< Material casts shadows
            u32 receive_shadows : 1;        ///< Material receives shadows
            u32 unlit : 1;                  ///< Material is unaffected by lighting
            u32 emissive : 1;               ///< Material emits light
            u32 alpha_test : 1;             ///< Use alpha testing instead of blending
            u32 depth_bias : 1;             ///< Apply depth bias for decals
            u32 high_quality : 1;           ///< Use high-quality rendering path
            u32 animated : 1;               ///< Material has animated properties
            u32 instanced : 1;              ///< Material supports instanced rendering
            u32 reserved : 21;
        };
    } material_flags{0};
    
    /** 
     * @brief Material sorting key for render ordering
     * 
     * Used by the renderer to sort materials for optimal rendering performance.
     * Lower values render first.
     */
    u16 sort_key{1000};
    
    /** 
     * @brief Alpha test threshold (if alpha testing is enabled)
     */
    f32 alpha_threshold{0.5f};
    
    //-------------------------------------------------------------------------
    // Performance and Debug Information
    //-------------------------------------------------------------------------
    
    /** 
     * @brief Material performance metrics
     */
    mutable struct MaterialPerf {
        u32 times_bound{0};                 ///< How many times material was bound
        u32 shader_switches{0};             ///< Number of shader program changes
        u32 uniform_updates{0};             ///< Number of uniform buffer updates
        f32 total_gpu_time{0.0f};           ///< Total GPU rendering time
        u32 draw_calls_with_material{0};    ///< Draw calls using this material
        u32 vertices_rendered{0};           ///< Vertices rendered with this material
    } performance;
    
    //-------------------------------------------------------------------------
    // Constructor and Factory Methods
    //-------------------------------------------------------------------------
    
    constexpr Material() noexcept = default;
    
    explicit Material(ShaderHandle shader_handle) noexcept : shader(shader_handle) {}
    
    // Factory methods for common material types
    static Material create_sprite_material() noexcept {
        Material material;
        material.blend_state.blend_enabled = true;
        material.material_flags.transparent = 1;
        material.sort_key = 1000; // Standard transparent sorting
        return material;
    }
    
    static Material create_ui_material() noexcept {
        Material material;
        material.blend_state.blend_enabled = true;
        material.depth_state.depth_test_enabled = false;
        material.material_flags.transparent = 1;
        material.material_flags.unlit = 1;
        material.sort_key = 2000; // UI renders on top
        return material;
    }
    
    static Material create_opaque_material() noexcept {
        Material material;
        material.blend_state.blend_enabled = false;
        material.depth_state.depth_test_enabled = true;
        material.sort_key = 500; // Opaque renders before transparent
        return material;
    }
    
    //-------------------------------------------------------------------------
    // Uniform Management Interface
    //-------------------------------------------------------------------------
    
    /** @brief Set float uniform value */
    void set_uniform(u8 index, f32 value) noexcept {
        if (index < MAX_UNIFORMS) {
            uniforms[index] = UniformValue{value};
            uniform_count = std::max(uniform_count, static_cast<u8>(index + 1));
        }
    }
    
    /** @brief Set float2 uniform value */
    void set_uniform(u8 index, f32 x, f32 y) noexcept {
        if (index < MAX_UNIFORMS) {
            uniforms[index] = UniformValue{x, y};
            uniform_count = std::max(uniform_count, static_cast<u8>(index + 1));
        }
    }
    
    /** @brief Set float3 uniform value */
    void set_uniform(u8 index, f32 x, f32 y, f32 z) noexcept {
        if (index < MAX_UNIFORMS) {
            uniforms[index] = UniformValue{x, y, z};
            uniform_count = std::max(uniform_count, static_cast<u8>(index + 1));
        }
    }
    
    /** @brief Set float4 uniform value */
    void set_uniform(u8 index, f32 x, f32 y, f32 z, f32 w) noexcept {
        if (index < MAX_UNIFORMS) {
            uniforms[index] = UniformValue{x, y, z, w};
            uniform_count = std::max(uniform_count, static_cast<u8>(index + 1));
        }
    }
    
    /** @brief Set color uniform value */
    void set_uniform(u8 index, const Color& color) noexcept {
        set_uniform(index, color.red_f(), color.green_f(), color.blue_f(), color.alpha_f());
    }
    
    /** @brief Set texture uniform value */
    void set_uniform(u8 index, TextureHandle texture) noexcept {
        if (index < MAX_UNIFORMS) {
            uniforms[index] = UniformValue{texture};
            uniform_count = std::max(uniform_count, static_cast<u8>(index + 1));
        }
    }
    
    /** @brief Clear all uniforms */
    void clear_uniforms() noexcept {
        uniform_count = 0;
        for (auto& uniform : uniforms) {
            uniform = UniformValue{};
        }
    }
    
    //-------------------------------------------------------------------------
    // Material State Management
    //-------------------------------------------------------------------------
    
    /** @brief Set material transparency */
    void set_transparent(bool transparent) noexcept {
        material_flags.transparent = transparent ? 1 : 0;
        blend_state.blend_enabled = transparent;
        
        // Adjust sort key for proper rendering order
        if (transparent && sort_key < 1000) {
            sort_key += 1000;
        } else if (!transparent && sort_key >= 1000) {
            sort_key -= 1000;
        }
    }
    
    /** @brief Set alpha testing parameters */
    void set_alpha_test(bool enabled, f32 threshold = 0.5f) noexcept {
        material_flags.alpha_test = enabled ? 1 : 0;
        alpha_threshold = threshold;
        
        // Alpha test materials can be treated as opaque for sorting
        if (enabled) {
            set_transparent(false);
        }
    }
    
    /** @brief Configure for 2D sprite rendering */
    void configure_for_sprites() noexcept {
        blend_state.blend_enabled = true;
        blend_state.src_color_factor = BlendState::Factor::SrcAlpha;
        blend_state.dst_color_factor = BlendState::Factor::OneMinusSrcAlpha;
        depth_state.depth_test_enabled = false;
        raster_state.cull_mode = RasterState::CullMode::None;
        material_flags.transparent = 1;
        material_flags.unlit = 1;
        sort_key = 1000;
    }
    
    /** @brief Configure for UI rendering */
    void configure_for_ui() noexcept {
        configure_for_sprites();
        depth_state.depth_test_enabled = false;
        depth_state.depth_write_enabled = false;
        sort_key = 2000; // UI renders on top
    }
    
    //-------------------------------------------------------------------------
    // Utility Functions
    //-------------------------------------------------------------------------
    
    /** @brief Check if material requires depth sorting */
    bool requires_depth_sorting() const noexcept {
        return material_flags.transparent && !material_flags.alpha_test;
    }
    
    /** @brief Estimate rendering cost for educational analysis */
    f32 estimate_render_cost() const noexcept {
        f32 base_cost = 1.0f;
        
        // Transparency increases cost
        if (material_flags.transparent) base_cost *= 1.5f;
        
        // Alpha testing adds cost
        if (material_flags.alpha_test) base_cost *= 1.2f;
        
        // Depth testing adds cost
        if (depth_state.depth_test_enabled) base_cost *= 1.1f;
        
        // High quality rendering increases cost
        if (material_flags.high_quality) base_cost *= 1.8f;
        
        // Custom shaders add cost
        if (shader.is_valid()) base_cost *= 1.3f;
        
        // Multiple uniforms add cost
        base_cost += uniform_count * 0.1f;
        
        return base_cost;
    }
    
    /** @brief Get material description for debugging */
    const char* get_description() const noexcept {
        if (material_flags.unlit && material_flags.transparent) return "Transparent Unlit";
        if (material_flags.unlit) return "Opaque Unlit";
        if (material_flags.transparent) return "Transparent Lit";
        if (material_flags.emissive) return "Emissive";
        return "Standard";
    }
    
    /** @brief Validate material configuration */
    bool is_valid() const noexcept {
        return alpha_threshold >= 0.0f && alpha_threshold <= 1.0f &&
               uniform_count <= MAX_UNIFORMS &&
               sort_key < 10000; // Reasonable sort key range
    }
    
    /** @brief Get material information for educational display */
    struct MaterialInfo {
        const char* material_type;
        bool uses_custom_shader;
        u32 uniform_count;
        f32 estimated_cost;
        bool requires_sorting;
        const char* blend_mode_description;
    };
    
    MaterialInfo get_material_info() const noexcept;
};

/**
 * @brief Render Information Component for Debug and Performance Analysis
 * 
 * Provides comprehensive debug information and performance metrics for
 * rendered entities. Essential for educational analysis and optimization.
 * 
 * Educational Context:
 * This component demonstrates:
 * - Performance profiling and metrics collection in graphics
 * - Debug visualization techniques for rendering systems
 * - Memory usage tracking for graphics resources
 * - Frame timing and bottleneck identification
 */
struct alignas(32) RenderInfo {
    //-------------------------------------------------------------------------
    // Rendering Performance Metrics
    //-------------------------------------------------------------------------
    
    /** 
     * @brief Per-frame rendering statistics
     */
    struct FrameStats {
        u32 frames_rendered{0};             ///< Total frames this entity was rendered
        u32 frames_culled{0};               ///< Frames when entity was culled
        f32 average_render_time{0.0f};      ///< Average GPU time per frame
        f32 worst_render_time{0.0f};        ///< Worst GPU time recorded
        f32 total_render_time{0.0f};        ///< Total GPU time spent rendering
        
        // Batching efficiency
        u32 times_batched{0};               ///< Times included in sprite batch
        u32 times_drawn_individually{0};    ///< Times required individual draw call
        u32 batch_breaks_caused{0};         ///< Times this entity broke batching
        f32 batching_efficiency{1.0f};      ///< Ratio of batched to individual draws
    } frame_stats;
    
    /** 
     * @brief Memory usage information
     */
    struct MemoryStats {
        usize vertex_buffer_memory{0};      ///< Bytes used for vertex data
        usize index_buffer_memory{0};       ///< Bytes used for index data
        usize texture_memory{0};            ///< Bytes used for textures
        usize uniform_buffer_memory{0};     ///< Bytes used for uniform data
        usize total_gpu_memory{0};          ///< Total GPU memory for this entity
        
        // Memory efficiency metrics
        f32 memory_per_vertex{0.0f};        ///< Average memory per vertex
        f32 memory_utilization{1.0f};       ///< How efficiently memory is used
        u32 memory_allocations{0};          ///< Number of GPU memory allocations
    } memory_stats;
    
    /** 
     * @brief Rendering quality and accuracy metrics
     */
    struct QualityStats {
        f32 pixel_coverage{0.0f};           ///< Percentage of screen covered
        f32 overdraw_factor{1.0f};          ///< How much this entity causes overdraw
        u32 texture_cache_hits{0};          ///< Texture cache hits
        u32 texture_cache_misses{0};        ///< Texture cache misses
        f32 mipmap_level_used{0.0f};        ///< Average mipmap level accessed
        
        // Visual quality metrics
        f32 texture_filtering_quality{1.0f}; ///< Quality of texture filtering
        bool has_visual_artifacts{false};    ///< Whether visual issues detected
        f32 color_accuracy{1.0f};            ///< Color reproduction accuracy
    } quality_stats;
    
    //-------------------------------------------------------------------------
    // Debug Visualization Settings
    //-------------------------------------------------------------------------
    
    /** 
     * @brief Debug rendering flags and settings
     */
    struct DebugSettings {
        union {
            u32 flags;
            struct {
                u32 show_bounds : 1;         ///< Show entity bounding box
                u32 show_pivot : 1;          ///< Show rotation/scale pivot point
                u32 show_origin : 1;         ///< Show world origin
                u32 show_normals : 1;        ///< Show surface normals (if applicable)
                u32 show_wireframe : 1;      ///< Render in wireframe mode
                u32 show_overdraw : 1;       ///< Highlight overdraw areas
                u32 show_texture_coords : 1; ///< Visualize UV coordinates
                u32 show_vertex_colors : 1;  ///< Show vertex color debug info
                u32 show_depth_info : 1;     ///< Show depth buffer information
                u32 show_performance : 1;    ///< Show performance overlay
                u32 show_memory_usage : 1;   ///< Show memory usage overlay
                u32 reserved : 21;
            };
        } debug_flags{0};
        
        // Debug visualization colors
        Color bounds_color{Color::cyan()};
        Color pivot_color{Color::red()};
        Color origin_color{Color::green()};
        Color wireframe_color{Color::yellow()};
        Color overdraw_color{Color::magenta()};
        
        // Debug overlay transparency
        f32 debug_alpha{0.7f};
        
        // Performance display settings
        bool show_frame_time{true};
        bool show_memory_usage{true};
        bool show_batching_info{true};
    } debug_settings;
    
    //-------------------------------------------------------------------------
    // Render State History (for analysis)
    //-------------------------------------------------------------------------
    
    /** 
     * @brief Historical data for trend analysis
     */
    static constexpr usize HISTORY_SIZE = 60; // 1 second at 60 FPS
    struct HistoryData {
        std::array<f32, HISTORY_SIZE> render_times{};
        std::array<f32, HISTORY_SIZE> pixel_coverage{};
        std::array<u32, HISTORY_SIZE> draw_calls{};
        std::array<usize, HISTORY_SIZE> memory_usage{};
        
        u32 current_index{0};
        bool history_full{false};
        
        void add_sample(f32 render_time, f32 coverage, u32 draws, usize memory) noexcept {
            render_times[current_index] = render_time;
            pixel_coverage[current_index] = coverage;
            draw_calls[current_index] = draws;
            memory_usage[current_index] = memory;
            
            current_index = (current_index + 1) % HISTORY_SIZE;
            if (current_index == 0) history_full = true;
        }
        
        f32 get_average_render_time() const noexcept {
            usize count = history_full ? HISTORY_SIZE : current_index;
            if (count == 0) return 0.0f;
            
            f32 sum = 0.0f;
            for (usize i = 0; i < count; ++i) {
                sum += render_times[i];
            }
            return sum / count;
        }
        
        f32 get_max_render_time() const noexcept {
            usize count = history_full ? HISTORY_SIZE : current_index;
            if (count == 0) return 0.0f;
            
            f32 max_time = render_times[0];
            for (usize i = 1; i < count; ++i) {
                if (render_times[i] > max_time) max_time = render_times[i];
            }
            return max_time;
        }
    };
    
    mutable HistoryData history;
    
    //-------------------------------------------------------------------------
    // Educational Analysis Data
    //-------------------------------------------------------------------------
    
    /** 
     * @brief Comprehensive analysis for educational purposes
     */
    struct EducationalAnalysis {
        // Performance classification
        enum class PerformanceRating : u8 {
            Excellent = 0, Good, Fair, Poor, Critical
        } performance_rating{PerformanceRating::Good};
        
        // Optimization suggestions
        static constexpr usize MAX_SUGGESTIONS = 8;
        std::array<const char*, MAX_SUGGESTIONS> optimization_suggestions{};
        u8 suggestion_count{0};
        
        // Resource efficiency scores (0-1)
        f32 memory_efficiency{1.0f};
        f32 batching_efficiency{1.0f};
        f32 cache_efficiency{1.0f};
        f32 overdraw_efficiency{1.0f};
        
        // Educational insights
        const char* primary_bottleneck{"None"};
        const char* recommended_action{"No action needed"};
        f32 estimated_performance_impact{0.0f}; // Cost relative to simple sprite
        
        void add_suggestion(const char* suggestion) noexcept {
            if (suggestion_count < MAX_SUGGESTIONS) {
                optimization_suggestions[suggestion_count++] = suggestion;
            }
        }
        
        void clear_suggestions() noexcept {
            suggestion_count = 0;
        }
    };
    
    mutable EducationalAnalysis analysis;
    
    //-------------------------------------------------------------------------
    // Constructor and Initialization
    //-------------------------------------------------------------------------
    
    constexpr RenderInfo() noexcept = default;
    
    //-------------------------------------------------------------------------
    // Performance Tracking Interface
    //-------------------------------------------------------------------------
    
    /** @brief Record frame rendering data */
    void record_frame_render(f32 render_time, bool was_culled, bool was_batched) const noexcept {
        if (was_culled) {
            frame_stats.frames_culled++;
        } else {
            frame_stats.frames_rendered++;
            frame_stats.total_render_time += render_time;
            frame_stats.average_render_time = frame_stats.total_render_time / 
                std::max(1u, frame_stats.frames_rendered);
            
            if (render_time > frame_stats.worst_render_time) {
                frame_stats.worst_render_time = render_time;
            }
            
            if (was_batched) {
                frame_stats.times_batched++;
            } else {
                frame_stats.times_drawn_individually++;
            }
            
            // Update batching efficiency
            u32 total_draws = frame_stats.times_batched + frame_stats.times_drawn_individually;
            frame_stats.batching_efficiency = total_draws > 0 ? 
                static_cast<f32>(frame_stats.times_batched) / total_draws : 1.0f;
        }
    }
    
    /** @brief Record memory usage update */
    void record_memory_usage(usize vertex_mem, usize index_mem, usize texture_mem, usize uniform_mem) const noexcept {
        memory_stats.vertex_buffer_memory = vertex_mem;
        memory_stats.index_buffer_memory = index_mem;
        memory_stats.texture_memory = texture_mem;
        memory_stats.uniform_buffer_memory = uniform_mem;
        memory_stats.total_gpu_memory = vertex_mem + index_mem + texture_mem + uniform_mem;
        
        // Calculate memory per vertex if we have vertex data
        if (vertex_mem > 0 && frame_stats.frames_rendered > 0) {
            memory_stats.memory_per_vertex = static_cast<f32>(vertex_mem) / 4.0f; // Assume quad (4 vertices)
        }
    }
    
    /** @brief Record quality metrics */
    void record_quality_metrics(f32 coverage, f32 overdraw, u32 cache_hits, u32 cache_misses) const noexcept {
        quality_stats.pixel_coverage = coverage;
        quality_stats.overdraw_factor = overdraw;
        quality_stats.texture_cache_hits += cache_hits;
        quality_stats.texture_cache_misses += cache_misses;
        
        // Update history
        history.add_sample(frame_stats.average_render_time, coverage, 1, memory_stats.total_gpu_memory);
    }
    
    /** @brief Update educational analysis */
    void update_analysis() const noexcept {
        analysis.clear_suggestions();
        
        // Analyze batching efficiency
        if (frame_stats.batching_efficiency < 0.5f) {
            analysis.performance_rating = EducationalAnalysis::PerformanceRating::Poor;
            analysis.add_suggestion("Consider texture atlasing to improve batching");
            analysis.primary_bottleneck = "Poor batching efficiency";
        }
        
        // Analyze overdraw
        if (quality_stats.overdraw_factor > 3.0f) {
            analysis.add_suggestion("Reduce overdraw by sorting sprites front-to-back");
        }
        
        // Analyze memory usage
        if (memory_stats.total_gpu_memory > 1024 * 1024) { // > 1MB
            analysis.add_suggestion("Consider texture compression to reduce memory usage");
        }
        
        // Analyze cache efficiency
        u32 total_cache_accesses = quality_stats.texture_cache_hits + quality_stats.texture_cache_misses;
        if (total_cache_accesses > 0) {
            f32 cache_hit_ratio = static_cast<f32>(quality_stats.texture_cache_hits) / total_cache_accesses;
            analysis.cache_efficiency = cache_hit_ratio;
            
            if (cache_hit_ratio < 0.8f) {
                analysis.add_suggestion("Improve texture cache locality by batching similar textures");
            }
        }
        
        // Calculate overall performance impact
        analysis.estimated_performance_impact = estimate_performance_cost();
    }
    
    //-------------------------------------------------------------------------
    // Debug Visualization Interface
    //-------------------------------------------------------------------------
    
    /** @brief Enable specific debug visualization */
    void enable_debug(u32 debug_flag_mask) noexcept {
        debug_settings.debug_flags.flags |= debug_flag_mask;
    }
    
    /** @brief Disable specific debug visualization */
    void disable_debug(u32 debug_flag_mask) noexcept {
        debug_settings.debug_flags.flags &= ~debug_flag_mask;
    }
    
    /** @brief Check if debug feature is enabled */
    bool is_debug_enabled(u32 debug_flag_mask) const noexcept {
        return (debug_settings.debug_flags.flags & debug_flag_mask) != 0;
    }
    
    /** @brief Enable comprehensive debug mode */
    void enable_full_debug() noexcept {
        debug_settings.debug_flags.flags = 0xFFFFFFFF;
    }
    
    /** @brief Disable all debug visualization */
    void disable_all_debug() noexcept {
        debug_settings.debug_flags.flags = 0;
    }
    
    //-------------------------------------------------------------------------
    // Educational Reporting
    //-------------------------------------------------------------------------
    
    /** @brief Get comprehensive performance report */
    struct PerformanceReport {
        f32 average_ms_per_frame;
        f32 worst_ms_per_frame;
        f32 memory_mb_used;
        f32 batching_efficiency_percent;
        f32 cache_hit_rate_percent;
        f32 overdraw_factor;
        const char* performance_grade; // "A", "B", "C", "D", "F"
        const char* optimization_priority; // "High", "Medium", "Low"
        u32 suggestion_count;
        const char* const* suggestions;
    };
    
    PerformanceReport get_performance_report() const noexcept {
        update_analysis();
        
        return {
            .average_ms_per_frame = frame_stats.average_render_time * 1000.0f,
            .worst_ms_per_frame = frame_stats.worst_render_time * 1000.0f,
            .memory_mb_used = memory_stats.total_gpu_memory / (1024.0f * 1024.0f),
            .batching_efficiency_percent = frame_stats.batching_efficiency * 100.0f,
            .cache_hit_rate_percent = analysis.cache_efficiency * 100.0f,
            .overdraw_factor = quality_stats.overdraw_factor,
            .performance_grade = get_performance_grade(),
            .optimization_priority = get_optimization_priority(),
            .suggestion_count = analysis.suggestion_count,
            .suggestions = analysis.optimization_suggestions.data()
        };
    }
    
    /** @brief Reset all statistics */
    void reset_stats() noexcept {
        frame_stats = {};
        memory_stats = {};
        quality_stats = {};
        history = {};
        analysis = {};
    }
    
    /** @brief Get render statistics summary */
    struct StatsSummary {
        u32 total_frames;
        f32 average_fps_impact;
        f32 total_memory_mb;
        f32 efficiency_score; // 0-1, higher is better
    };
    
    StatsSummary get_stats_summary() const noexcept {
        f32 fps_impact = frame_stats.average_render_time * 60.0f; // Assume 60 FPS target
        f32 efficiency = (analysis.memory_efficiency + analysis.batching_efficiency + 
                         analysis.cache_efficiency + analysis.overdraw_efficiency) / 4.0f;
        
        return {
            .total_frames = frame_stats.frames_rendered + frame_stats.frames_culled,
            .average_fps_impact = fps_impact,
            .total_memory_mb = memory_stats.total_gpu_memory / (1024.0f * 1024.0f),
            .efficiency_score = efficiency
        };
    }
    
private:
    /** @brief Estimate performance cost relative to simple sprite */
    f32 estimate_performance_cost() const noexcept {
        f32 cost = 1.0f;
        
        // Poor batching increases cost significantly
        cost *= (2.0f - frame_stats.batching_efficiency);
        
        // Overdraw increases cost
        cost *= quality_stats.overdraw_factor;
        
        // Cache misses increase cost
        if (analysis.cache_efficiency < 1.0f) {
            cost *= (1.0f + (1.0f - analysis.cache_efficiency));
        }
        
        return cost;
    }
    
    /** @brief Get letter grade for performance */
    const char* get_performance_grade() const noexcept {
        f32 cost = analysis.estimated_performance_impact;
        if (cost <= 1.2f) return "A";
        if (cost <= 1.5f) return "B";
        if (cost <= 2.0f) return "C";
        if (cost <= 3.0f) return "D";
        return "F";
    }
    
    /** @brief Get optimization priority level */
    const char* get_optimization_priority() const noexcept {
        if (analysis.performance_rating >= EducationalAnalysis::PerformanceRating::Poor) return "High";
        if (analysis.performance_rating >= EducationalAnalysis::PerformanceRating::Fair) return "Medium";
        return "Low";
    }
};

//=============================================================================
// Component Validation and Static Assertions
//=============================================================================

// Ensure all components satisfy the ECS Component concept
static_assert(ecs::Component<Color>, "Color must satisfy Component concept");
static_assert(ecs::Component<UVRect>, "UVRect must satisfy Component concept");
static_assert(ecs::Component<RenderableSprite>, "RenderableSprite must satisfy Component concept");
static_assert(ecs::Component<Camera2D>, "Camera2D must satisfy Component concept");
static_assert(ecs::Component<Material>, "Material must satisfy Component concept");
static_assert(ecs::Component<RenderInfo>, "RenderInfo must satisfy Component concept");

// Verify memory alignment for optimal performance
static_assert(alignof(Color) >= 4, "Color should be 4-byte aligned");
static_assert(alignof(UVRect) >= 16, "UVRect should be 16-byte aligned");
static_assert(alignof(RenderableSprite) >= 16, "RenderableSprite should be 16-byte aligned");
static_assert(alignof(Camera2D) >= 32, "Camera2D should be 32-byte aligned");
static_assert(alignof(Material) >= 32, "Material should be 32-byte aligned");
static_assert(alignof(RenderInfo) >= 32, "RenderInfo should be 32-byte aligned");

// Verify reasonable component sizes for cache efficiency
static_assert(sizeof(Color) == 4, "Color should be exactly 4 bytes");
static_assert(sizeof(UVRect) == 16, "UVRect should be exactly 16 bytes");
static_assert(sizeof(RenderableSprite) <= 256, "RenderableSprite should fit in 256 bytes");
static_assert(sizeof(Camera2D) <= 512, "Camera2D should fit in 512 bytes");

//=============================================================================
// Utility Functions and Component Relationships
//=============================================================================

namespace utils {
    /**
     * @brief Create a complete renderable entity with reasonable defaults
     */
    struct RenderableEntityDesc {
        TextureHandle texture{};
        UVRect uv_rect = UVRect::full_texture();
        Color color = Color::white();
        f32 z_order = 0.0f;
        bool enable_debug = false;
        Material::BlendState::Factor blend_mode = Material::BlendState::Factor::SrcAlpha;
    };
    
    /**
     * @brief Helper to create rendering components for an entity
     */
    struct RenderingComponents {
        RenderableSprite sprite;
        Material material;
        std::optional<RenderInfo> debug_info;
    };
    
    RenderingComponents create_renderable_entity(const RenderableEntityDesc& desc) noexcept;
    
    /**
     * @brief Validate rendering component consistency
     */
    bool validate_rendering_components(const RenderableSprite* sprite, 
                                     const Material* material,
                                     const Camera2D* camera) noexcept;
    
    /**
     * @brief Calculate screen space bounds for a rendered entity
     */
    struct ScreenBounds {
        f32 left, right, top, bottom;
        f32 width() const noexcept { return right - left; }
        f32 height() const noexcept { return bottom - top; }
    };
    
    ScreenBounds calculate_screen_bounds(const Transform& transform,
                                       const RenderableSprite& sprite,
                                       const Camera2D& camera) noexcept;
}

} // namespace ecscope::renderer::components