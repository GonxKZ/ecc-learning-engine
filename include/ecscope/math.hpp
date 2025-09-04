#pragma once

/**
 * @file physics/math.hpp
 * @brief Comprehensive 2D Physics Mathematics Foundation for ECScope Educational ECS Engine
 * 
 * This header provides the mathematical foundation for 2D physics simulation with emphasis
 * on educational clarity while maintaining high performance. It includes:
 * 
 * - Advanced 2D vector mathematics with SIMD optimizations
 * - Geometric primitives (Circle, AABB, OBB, Polygon, Ray2D)
 * - Transform mathematics and matrix operations
 * - Collision detection mathematics
 * - Physics constants and utility functions
 * - Educational debugging and visualization helpers
 * 
 * Educational Philosophy:
 * This library is designed to teach physics programming concepts while delivering
 * production-ready performance. Each algorithm includes detailed comments explaining
 * the mathematical principles, with references to physics textbooks and papers.
 * 
 * Performance Characteristics:
 * - Header-only implementation for optimal inlining
 * - SIMD optimizations where beneficial (SSE/AVX)
 * - Cache-friendly data layouts
 * - Zero-overhead abstractions
 * - Compile-time computations where possible
 * 
 * Memory Integration:
 * - Full integration with ECScope's memory tracking system
 * - Custom allocator support for physics objects
 * - Memory pool optimizations for frequent allocations
 * 
 * @author ECScope Educational ECS Framework - Phase 5: Física 2D
 * @date 2024
 */

#include "core/types.hpp"
#include "ecs/components/transform.hpp"
#include "memory/memory_tracker.hpp"
#include <cmath>
#include <array>
#include <vector>
#include <optional>
#include <span>
#include <algorithm>
#include <immintrin.h>  // For SIMD intrinsics

// Enable SIMD optimizations if available and explicitly requested
// Note: Requires -msse3 compiler flag for _mm_hadd_ps
#if defined(__SSE3__) && defined(ECSCOPE_ENABLE_SIMD)
    #define ECSCOPE_PHYSICS_USE_SSE2
#endif

#if defined(__AVX__) && defined(ECSCOPE_ENABLE_SIMD)
    #define ECSCOPE_PHYSICS_USE_AVX
#endif

namespace ecscope::physics::math {

// Import common types and Vec2 from existing components
using Vec2 = ecs::components::Vec2;
using Transform = ecs::components::Transform;

//=============================================================================
// Physics Constants
//=============================================================================

namespace constants {
    // Mathematical constants with high precision
    constexpr f64 PI = 3.141592653589793238462643383279502884;
    constexpr f64 TWO_PI = 2.0 * PI;
    constexpr f64 HALF_PI = 0.5 * PI;
    constexpr f64 INV_PI = 1.0 / PI;
    constexpr f64 SQRT_2 = 1.4142135623730950488016887242096981;
    constexpr f64 INV_SQRT_2 = 1.0 / SQRT_2;
    
    // Single precision variants for performance
    constexpr f32 PI_F = static_cast<f32>(PI);
    constexpr f32 TWO_PI_F = static_cast<f32>(TWO_PI);
    constexpr f32 HALF_PI_F = static_cast<f32>(HALF_PI);
    constexpr f32 INV_PI_F = static_cast<f32>(INV_PI);
    constexpr f32 SQRT_2_F = static_cast<f32>(SQRT_2);
    constexpr f32 INV_SQRT_2_F = static_cast<f32>(INV_SQRT_2);
    
    // Physics constants
    constexpr f32 DEFAULT_GRAVITY = 9.81f;          // m/s² - Earth gravity
    constexpr f32 DEFAULT_GRAVITY_SCALE = 100.0f;   // Pixels per meter for visualization
    
    // Numerical precision constants
    constexpr f32 EPSILON = 1e-6f;                  // General floating point epsilon
    constexpr f32 COLLISION_EPSILON = 1e-4f;        // Collision detection epsilon
    constexpr f32 PENETRATION_SLOP = 0.01f;         // Allowed penetration for stability
    constexpr f32 LINEAR_SLOP = 0.005f;             // Linear position correction slop
    constexpr f32 ANGULAR_SLOP = 2.0f / 180.0f * PI_F; // Angular correction slop (2 degrees)
    
    // Performance tuning constants
    constexpr u32 MAX_POLYGON_VERTICES = 16;        // Maximum vertices for polygon shapes
    constexpr u32 MAX_CONTACT_POINTS = 4;           // Maximum contact points per collision
    constexpr u32 MAX_ITERATIONS = 20;              // Maximum solver iterations
    
    // Convert radians/degrees
    constexpr f32 DEG_TO_RAD = PI_F / 180.0f;
    constexpr f32 RAD_TO_DEG = 180.0f / PI_F;
}

//=============================================================================
// Advanced Vector Mathematics (extending existing Vec2)
//=============================================================================

/**
 * @brief Extended vector utilities complementing the existing Vec2 class
 * 
 * These functions extend the basic Vec2 functionality with physics-specific
 * operations while maintaining compatibility with the existing transform system.
 */
namespace vec2 {
    
    /**
     * @brief Cross product in 2D (returns scalar z-component)
     * 
     * The 2D cross product is the z-component of the 3D cross product when
     * treating 2D vectors as 3D vectors with z=0. This is crucial for:
     * - Determining rotation direction (positive = counter-clockwise)
     * - Computing torque and angular momentum
     * - Finding the signed area of triangles
     * 
     * Mathematical definition: a × b = a.x * b.y - a.y * b.x
     */
    constexpr f32 cross(const Vec2& a, const Vec2& b) noexcept {
        return a.x * b.y - a.y * b.x;
    }
    
    /**
     * @brief Cross product of vector and scalar (for rotational physics)
     * 
     * This operation is used when applying angular velocity to linear motion:
     * v_new = w × r, where w is angular velocity (scalar) and r is position vector
     */
    constexpr Vec2 cross(const Vec2& v, f32 s) noexcept {
        return Vec2{s * v.y, -s * v.x};
    }
    
    constexpr Vec2 cross(f32 s, const Vec2& v) noexcept {
        return Vec2{-s * v.y, s * v.x};
    }
    
    /**
     * @brief Perpendicular vector (90-degree counter-clockwise rotation)
     * 
     * Essential for:
     * - Computing surface normals from edge vectors
     * - Converting velocity to force directions
     * - Implementing 2D rotations without trigonometry
     */
    constexpr Vec2 perpendicular(const Vec2& v) noexcept {
        return Vec2{-v.y, v.x};
    }
    
    /**
     * @brief Right-hand perpendicular (90-degree clockwise rotation)
     */
    constexpr Vec2 perpendicular_cw(const Vec2& v) noexcept {
        return Vec2{v.y, -v.x};
    }
    
    /**
     * @brief Triple product: (a × b) × c
     * 
     * Used in collision detection for finding support points and in
     * constraint resolution for computing relative motion.
     */
    constexpr Vec2 triple_product(const Vec2& a, const Vec2& b, const Vec2& c) noexcept {
        f32 dot = a.x * c.x + a.y * c.y;
        return Vec2{b.x * dot - a.x * (b.x * c.x + b.y * c.y),
                    b.y * dot - a.y * (b.x * c.x + b.y * c.y)};
    }
    
    /**
     * @brief Safe vector normalization with fallback
     * 
     * Returns a normalized vector, or the fallback if the vector is too small.
     * This prevents division by zero in physics calculations.
     */
    inline Vec2 safe_normalize(const Vec2& v, const Vec2& fallback = Vec2{1.0f, 0.0f}) noexcept {
        f32 length_sq = v.length_squared();
        if (length_sq > constants::EPSILON * constants::EPSILON) {
            return v / std::sqrt(length_sq);
        }
        return fallback;
    }
    
    /**
     * @brief Linear interpolation between two vectors
     * 
     * Essential for smooth physics integration and animation.
     * t = 0.0 returns a, t = 1.0 returns b
     */
    constexpr Vec2 lerp(const Vec2& a, const Vec2& b, f32 t) noexcept {
        return Vec2{a.x + t * (b.x - a.x), a.y + t * (b.y - a.y)};
    }
    
    /**
     * @brief Spherical linear interpolation (slerp) for vectors
     * 
     * Provides constant angular velocity interpolation, useful for smooth rotations
     * and maintaining consistent physics behavior during interpolation.
     */
    Vec2 slerp(const Vec2& a, const Vec2& b, f32 t) noexcept;
    
    /**
     * @brief Project vector a onto vector b
     * 
     * Returns the component of vector a that lies in the direction of vector b.
     * Used extensively in collision response for separating normal and tangential components.
     */
    inline Vec2 project(const Vec2& a, const Vec2& b) noexcept {
        f32 b_length_sq = b.length_squared();
        if (b_length_sq < constants::EPSILON) {
            return Vec2::zero();
        }
        return b * (a.dot(b) / b_length_sq);
    }
    
    /**
     * @brief Reject vector a from vector b (orthogonal component)
     * 
     * Returns the component of vector a that is perpendicular to vector b.
     * Useful for computing tangential forces and friction.
     */
    inline Vec2 reject(const Vec2& a, const Vec2& b) noexcept {
        return a - project(a, b);
    }
    
    /**
     * @brief Reflect vector across a normal
     * 
     * Computes perfect elastic reflection: r = v - 2(v·n)n
     * Used for bouncing objects off surfaces.
     */
    constexpr Vec2 reflect(const Vec2& v, const Vec2& normal) noexcept {
        return v - normal * (2.0f * v.dot(normal));
    }
    
    /**
     * @brief Clamp vector magnitude to maximum length
     * 
     * Prevents velocities or forces from becoming unreasonably large,
     * maintaining numerical stability in physics simulation.
     */
    inline Vec2 clamp_magnitude(const Vec2& v, f32 max_length) noexcept {
        f32 length_sq = v.length_squared();
        if (length_sq > max_length * max_length) {
            return v * (max_length / std::sqrt(length_sq));
        }
        return v;
    }
    
    /**
     * @brief Distance squared between two points
     * 
     * Avoids expensive square root when only comparing distances.
     */
    constexpr f32 distance_squared(const Vec2& a, const Vec2& b) noexcept {
        Vec2 diff = a - b;
        return diff.length_squared();
    }
    
    /**
     * @brief Distance between two points
     */
    inline f32 distance(const Vec2& a, const Vec2& b) noexcept {
        return std::sqrt(distance_squared(a, b));
    }
    
    /**
     * @brief Check if two vectors are approximately equal
     * 
     * Uses epsilon comparison for robust floating-point equality testing.
     */
    constexpr bool approximately_equal(const Vec2& a, const Vec2& b, f32 epsilon = constants::EPSILON) noexcept {
        return std::abs(a.x - b.x) <= epsilon && std::abs(a.y - b.y) <= epsilon;
    }
    
    /**
     * @brief Angle between two vectors in radians
     * 
     * Returns the angle between vectors using atan2 for proper quadrant handling.
     * Range: [-π, π]
     */
    f32 angle_between(const Vec2& a, const Vec2& b) noexcept;
    
    /**
     * @brief Create vector from angle and magnitude
     * 
     * Useful for converting polar coordinates to Cartesian.
     */
    constexpr Vec2 from_angle(f32 angle, f32 magnitude = 1.0f) noexcept {
        return Vec2{magnitude * std::cos(angle), magnitude * std::sin(angle)};
    }
    
    /**
     * @brief Get angle of vector in radians
     * 
     * Returns the angle of the vector from the positive x-axis.
     * Range: [-π, π]
     */
    inline f32 angle(const Vec2& v) noexcept {
        return std::atan2(v.y, v.x);
    }

#ifdef ECSCOPE_PHYSICS_USE_SSE2
    /**
     * @brief SIMD-optimized dot product for multiple vector pairs
     * 
     * Computes dot products for 2 vector pairs simultaneously using SSE2.
     * Useful for batch collision detection operations.
     */
    void dot_product_x2(const Vec2 a[2], const Vec2 b[2], f32 results[2]) noexcept;
    
    /**
     * @brief SIMD-optimized vector addition for multiple pairs
     */
    void add_vectors_x4(const Vec2 a[4], const Vec2 b[4], Vec2 results[4]) noexcept;
#endif
}

//=============================================================================
// 2x2 Matrix for 2D Transformations
//=============================================================================

/**
 * @brief 2x2 Matrix for 2D rotations and transformations
 * 
 * Represents a 2x2 matrix in column-major order for compatibility with graphics APIs.
 * Used for efficient rotation operations without trigonometric function calls.
 * 
 * Matrix layout:
 * | m00  m01 |   | col0.x  col1.x |
 * | m10  m11 | = | col0.y  col1.y |
 * 
 * Educational Note:
 * 2x2 matrices are sufficient for 2D rotations, scaling, and shearing.
 * For translation, we use separate position vectors (Transform class).
 */
struct alignas(16) Matrix2 {
    Vec2 col0;  // First column
    Vec2 col1;  // Second column
    
    // Constructors
    constexpr Matrix2() noexcept : col0{1.0f, 0.0f}, col1{0.0f, 1.0f} {}
    
    constexpr Matrix2(const Vec2& c0, const Vec2& c1) noexcept : col0(c0), col1(c1) {}
    
    constexpr Matrix2(f32 m00, f32 m01, f32 m10, f32 m11) noexcept 
        : col0{m00, m10}, col1{m01, m11} {}
    
    // Element access
    f32& operator()(usize row, usize col) noexcept {
        if (col == 0) return (row == 0) ? col0.x : col0.y;
        else return (row == 0) ? col1.x : col1.y;
    }
    
    f32 operator()(usize row, usize col) const noexcept {
        if (col == 0) return (row == 0) ? col0.x : col0.y;
        else return (row == 0) ? col1.x : col1.y;
    }
    
    // Matrix operations
    constexpr Matrix2 operator+(const Matrix2& other) const noexcept {
        return Matrix2{col0 + other.col0, col1 + other.col1};
    }
    
    constexpr Matrix2 operator-(const Matrix2& other) const noexcept {
        return Matrix2{col0 - other.col0, col1 - other.col1};
    }
    
    constexpr Matrix2 operator*(f32 scalar) const noexcept {
        return Matrix2{col0 * scalar, col1 * scalar};
    }
    
    // Matrix multiplication
    constexpr Matrix2 operator*(const Matrix2& other) const noexcept {
        return Matrix2{
            Vec2{col0.x * other.col0.x + col1.x * other.col0.y,
                 col0.y * other.col0.x + col1.y * other.col0.y},
            Vec2{col0.x * other.col1.x + col1.x * other.col1.y,
                 col0.y * other.col1.x + col1.y * other.col1.y}
        };
    }
    
    // Matrix-vector multiplication
    constexpr Vec2 operator*(const Vec2& v) const noexcept {
        return Vec2{col0.x * v.x + col1.x * v.y,
                    col0.y * v.x + col1.y * v.y};
    }
    
    // Matrix properties
    constexpr f32 determinant() const noexcept {
        return col0.x * col1.y - col1.x * col0.y;
    }
    
    constexpr Matrix2 transpose() const noexcept {
        return Matrix2{Vec2{col0.x, col1.x}, Vec2{col0.y, col1.y}};
    }
    
    Matrix2 inverse() const noexcept {
        f32 det = determinant();
        if (std::abs(det) < constants::EPSILON) {
            return identity();  // Return identity if matrix is singular
        }
        f32 inv_det = 1.0f / det;
        return Matrix2{Vec2{col1.y * inv_det, -col0.y * inv_det},
                       Vec2{-col1.x * inv_det, col0.x * inv_det}};
    }
    
    // Factory methods
    static constexpr Matrix2 identity() noexcept {
        return Matrix2{};
    }
    
    static constexpr Matrix2 zero() noexcept {
        return Matrix2{Vec2::zero(), Vec2::zero()};
    }
    
    static Matrix2 rotation(f32 angle) noexcept {
        f32 cos_a = std::cos(angle);
        f32 sin_a = std::sin(angle);
        return Matrix2{Vec2{cos_a, sin_a}, Vec2{-sin_a, cos_a}};
    }
    
    static constexpr Matrix2 scale(f32 sx, f32 sy) noexcept {
        return Matrix2{Vec2{sx, 0.0f}, Vec2{0.0f, sy}};
    }
    
    static constexpr Matrix2 scale(const Vec2& s) noexcept {
        return scale(s.x, s.y);
    }
};

//=============================================================================
// Enhanced Transform2D for Physics
//=============================================================================

/**
 * @brief Enhanced 2D transform for physics simulation
 * 
 * Extends the basic Transform with physics-specific functionality:
 * - Pre-computed transformation matrices for performance
 * - Hierarchical transforms with parent-child relationships
 * - Physics integration helpers
 * - Bounds calculation utilities
 */
struct alignas(32) Transform2D {
    Vec2 position{0.0f, 0.0f};      // World position
    f32 rotation{0.0f};             // Rotation in radians
    Vec2 scale{1.0f, 1.0f};         // Scale factors
    
    // Cached transformation matrices (updated when transform changes)
    mutable Matrix2 rotation_matrix;    // Cached rotation matrix
    mutable bool matrix_dirty{true};    // Whether matrices need updating
    
    // Default constructor
    constexpr Transform2D() noexcept = default;
    
    // Constructors
    constexpr Transform2D(Vec2 pos, f32 rot = 0.0f, Vec2 scl = Vec2::one()) noexcept
        : position(pos), rotation(rot), scale(scl), matrix_dirty(true) {}
    
    constexpr Transform2D(f32 x, f32 y, f32 rot = 0.0f, f32 uniform_scale = 1.0f) noexcept
        : position(x, y), rotation(rot), scale(uniform_scale, uniform_scale), matrix_dirty(true) {}
    
    // Convert from basic Transform
    explicit constexpr Transform2D(const Transform& t) noexcept
        : position(t.position), rotation(t.rotation), scale(t.scale), matrix_dirty(true) {}
    
    // Convert to basic Transform
    constexpr Transform to_basic() const noexcept {
        return Transform{position, rotation, scale};
    }
    
    // Matrix access (updates cache if dirty)
    const Matrix2& get_rotation_matrix() const noexcept {
        if (matrix_dirty) {
            update_matrices();
        }
        return rotation_matrix;
    }
    
    // Transform operations (mark matrices as dirty)
    void set_position(const Vec2& pos) noexcept {
        position = pos;
    }
    
    void set_rotation(f32 rot) noexcept {
        rotation = rot;
        matrix_dirty = true;
    }
    
    void set_scale(const Vec2& scl) noexcept {
        scale = scl;
        matrix_dirty = true;
    }
    
    // Point transformations
    Vec2 transform_point(const Vec2& local_point) const noexcept {
        const Matrix2& rot_matrix = get_rotation_matrix();
        Vec2 scaled = Vec2{local_point.x * scale.x, local_point.y * scale.y};
        return rot_matrix * scaled + position;
    }
    
    Vec2 transform_direction(const Vec2& local_direction) const noexcept {
        const Matrix2& rot_matrix = get_rotation_matrix();
        Vec2 scaled = Vec2{local_direction.x * scale.x, local_direction.y * scale.y};
        return rot_matrix * scaled;
    }
    
    Vec2 inverse_transform_point(const Vec2& world_point) const noexcept {
        const Matrix2& rot_matrix = get_rotation_matrix();
        Matrix2 inv_rot = rot_matrix.inverse();
        Vec2 translated = world_point - position;
        Vec2 rotated = inv_rot * translated;
        return Vec2{rotated.x / scale.x, rotated.y / scale.y};
    }
    
    // Direction vectors
    Vec2 right() const noexcept {
        return get_rotation_matrix().col0;
    }
    
    Vec2 up() const noexcept {
        return get_rotation_matrix().col1;
    }
    
    // Combine transforms (useful for hierarchical transforms)
    Transform2D operator*(const Transform2D& child) const noexcept {
        Vec2 child_world_pos = transform_point(child.position);
        f32 child_world_rot = rotation + child.rotation;
        Vec2 child_world_scale = Vec2{scale.x * child.scale.x, scale.y * child.scale.y};
        return Transform2D{child_world_pos, child_world_rot, child_world_scale};
    }
    
    // Interpolation for smooth physics integration
    static Transform2D lerp(const Transform2D& a, const Transform2D& b, f32 t) noexcept {
        Vec2 pos = vec2::lerp(a.position, b.position, t);
        f32 rot = a.rotation + t * (b.rotation - a.rotation);
        Vec2 scl = vec2::lerp(a.scale, b.scale, t);
        return Transform2D{pos, rot, scl};
    }
    
private:
    void update_matrices() const noexcept {
        rotation_matrix = Matrix2::rotation(rotation);
        matrix_dirty = false;
    }
};

//=============================================================================
// Geometric Primitives
//=============================================================================

/**
 * @brief 2D Circle primitive
 * 
 * Represents a circle with center and radius. Most efficient shape for
 * collision detection and physics simulation.
 */
struct Circle {
    Vec2 center{0.0f, 0.0f};
    f32 radius{1.0f};
    
    constexpr Circle() noexcept = default;
    constexpr Circle(Vec2 c, f32 r) noexcept : center(c), radius(r) {}
    constexpr Circle(f32 x, f32 y, f32 r) noexcept : center(x, y), radius(r) {}
    
    // Geometric properties
    constexpr f32 area() const noexcept {
        return constants::PI_F * radius * radius;
    }
    
    constexpr f32 circumference() const noexcept {
        return constants::TWO_PI_F * radius;
    }
    
    // Containment tests
    constexpr bool contains(const Vec2& point) const noexcept {
        return vec2::distance_squared(center, point) <= radius * radius;
    }
    
    inline bool contains(const Circle& other) const noexcept {
        f32 distance = vec2::distance(center, other.center);
        return distance + other.radius <= radius;
    }
    
    // Bounding box
    struct AABB get_aabb() const noexcept;
    
    // Expansion/scaling
    constexpr Circle expanded(f32 amount) const noexcept {
        return Circle{center, radius + amount};
    }
    
    constexpr Circle scaled(f32 factor) const noexcept {
        return Circle{center, radius * factor};
    }
    
    // Transform
    Circle transformed(const Transform2D& transform) const noexcept {
        Vec2 world_center = transform.transform_point(center);
        f32 max_scale = std::max(transform.scale.x, transform.scale.y);
        return Circle{world_center, radius * max_scale};
    }
};

/**
 * @brief Axis-Aligned Bounding Box (AABB)
 * 
 * Rectangle aligned with coordinate axes. Very efficient for broad-phase
 * collision detection and spatial partitioning.
 */
struct alignas(16) AABB {
    Vec2 min{-1.0f, -1.0f};  // Bottom-left corner
    Vec2 max{1.0f, 1.0f};    // Top-right corner
    
    constexpr AABB() noexcept = default;
    constexpr AABB(Vec2 minimum, Vec2 maximum) noexcept : min(minimum), max(maximum) {}
    
    constexpr AABB(f32 min_x, f32 min_y, f32 max_x, f32 max_y) noexcept 
        : min(min_x, min_y), max(max_x, max_y) {}
    
    // Factory methods
    static constexpr AABB from_center_size(Vec2 center, Vec2 size) noexcept {
        Vec2 half_size = size * 0.5f;
        return AABB{center - half_size, center + half_size};
    }
    
    static constexpr AABB from_points(const Vec2& a, const Vec2& b) noexcept {
        return AABB{Vec2{std::min(a.x, b.x), std::min(a.y, b.y)},
                    Vec2{std::max(a.x, b.x), std::max(a.y, b.y)}};
    }
    
    // Properties
    constexpr Vec2 center() const noexcept {
        return (min + max) * 0.5f;
    }
    
    constexpr Vec2 size() const noexcept {
        return max - min;
    }
    
    constexpr Vec2 half_size() const noexcept {
        return (max - min) * 0.5f;
    }
    
    constexpr f32 width() const noexcept {
        return max.x - min.x;
    }
    
    constexpr f32 height() const noexcept {
        return max.y - min.y;
    }
    
    constexpr f32 area() const noexcept {
        return width() * height();
    }
    
    constexpr f32 perimeter() const noexcept {
        return 2.0f * (width() + height());
    }
    
    // Validation
    constexpr bool is_valid() const noexcept {
        return min.x <= max.x && min.y <= max.y;
    }
    
    // Containment tests
    constexpr bool contains(const Vec2& point) const noexcept {
        return point.x >= min.x && point.x <= max.x &&
               point.y >= min.y && point.y <= max.y;
    }
    
    constexpr bool contains(const AABB& other) const noexcept {
        return other.min.x >= min.x && other.max.x <= max.x &&
               other.min.y >= min.y && other.max.y <= max.y;
    }
    
    // Intersection tests
    constexpr bool intersects(const AABB& other) const noexcept {
        return !(other.min.x > max.x || other.max.x < min.x ||
                 other.min.y > max.y || other.max.y < min.y);
    }
    
    // Closest point
    constexpr Vec2 closest_point(const Vec2& point) const noexcept {
        return Vec2{std::clamp(point.x, min.x, max.x),
                    std::clamp(point.y, min.y, max.y)};
    }
    
    // Union and intersection
    constexpr AABB union_with(const AABB& other) const noexcept {
        return AABB{Vec2{std::min(min.x, other.min.x), std::min(min.y, other.min.y)},
                    Vec2{std::max(max.x, other.max.x), std::max(max.y, other.max.y)}};
    }
    
    std::optional<AABB> intersection_with(const AABB& other) const noexcept {
        AABB result{Vec2{std::max(min.x, other.min.x), std::max(min.y, other.min.y)},
                    Vec2{std::min(max.x, other.max.x), std::min(max.y, other.max.y)}};
        return result.is_valid() ? std::optional<AABB>{result} : std::nullopt;
    }
    
    // Expansion
    constexpr AABB expanded(f32 amount) const noexcept {
        return AABB{min - Vec2{amount, amount}, max + Vec2{amount, amount}};
    }
    
    constexpr AABB expanded(const Vec2& amount) const noexcept {
        return AABB{min - amount, max + amount};
    }
    
    // Get corner points
    constexpr Vec2 corner(u32 index) const noexcept {
        // 0: bottom-left, 1: bottom-right, 2: top-right, 3: top-left
        switch (index & 3) {
            case 0: return min;
            case 1: return Vec2{max.x, min.y};
            case 2: return max;
            case 3: return Vec2{min.x, max.y};
            default: return min;
        }
    }
    
    std::array<Vec2, 4> get_corners() const noexcept {
        return {{min, {max.x, min.y}, max, {min.x, max.y}}};
    }
};

/**
 * @brief Oriented Bounding Box (OBB)
 * 
 * Rectangle that can be rotated. More expensive than AABB but provides
 * tighter bounds for rotated objects.
 */
struct alignas(32) OBB {
    Vec2 center{0.0f, 0.0f};        // Center point
    Vec2 half_extents{1.0f, 1.0f};  // Half-width and half-height
    f32 rotation{0.0f};             // Rotation in radians
    
    // Cached axes (updated when rotation changes)
    mutable Vec2 axis_x{1.0f, 0.0f};
    mutable Vec2 axis_y{0.0f, 1.0f};
    mutable bool axes_dirty{true};
    
    constexpr OBB() noexcept = default;
    
    constexpr OBB(Vec2 c, Vec2 extents, f32 rot = 0.0f) noexcept
        : center(c), half_extents(extents), rotation(rot), axes_dirty(true) {}
    
    // Factory methods
    static constexpr OBB from_aabb(const AABB& aabb, f32 rot = 0.0f) noexcept {
        return OBB{aabb.center(), aabb.half_size(), rot};
    }
    
    static constexpr OBB from_transform(const Transform2D& transform, Vec2 local_extents) noexcept {
        Vec2 world_extents = Vec2{local_extents.x * transform.scale.x, 
                                  local_extents.y * transform.scale.y};
        return OBB{transform.position, world_extents, transform.rotation};
    }
    
    // Axis access (updates cache if dirty)
    const Vec2& get_axis_x() const noexcept {
        if (axes_dirty) update_axes();
        return axis_x;
    }
    
    const Vec2& get_axis_y() const noexcept {
        if (axes_dirty) update_axes();
        return axis_y;
    }
    
    // Properties
    constexpr Vec2 size() const noexcept {
        return half_extents * 2.0f;
    }
    
    constexpr f32 area() const noexcept {
        return 4.0f * half_extents.x * half_extents.y;
    }
    
    // Get corner points in world space
    std::array<Vec2, 4> get_corners() const noexcept {
        const Vec2& ax = get_axis_x();
        const Vec2& ay = get_axis_y();
        Vec2 x_extent = ax * half_extents.x;
        Vec2 y_extent = ay * half_extents.y;
        
        return {{
            center - x_extent - y_extent,  // Bottom-left
            center + x_extent - y_extent,  // Bottom-right  
            center + x_extent + y_extent,  // Top-right
            center - x_extent + y_extent   // Top-left
        }};
    }
    
    // Transform point from world to local coordinates
    Vec2 world_to_local(const Vec2& world_point) const noexcept {
        const Vec2& ax = get_axis_x();
        const Vec2& ay = get_axis_y();
        Vec2 delta = world_point - center;
        return Vec2{delta.dot(ax), delta.dot(ay)};
    }
    
    // Transform point from local to world coordinates
    Vec2 local_to_world(const Vec2& local_point) const noexcept {
        const Vec2& ax = get_axis_x();
        const Vec2& ay = get_axis_y();
        return center + ax * local_point.x + ay * local_point.y;
    }
    
    // Containment test
    bool contains(const Vec2& point) const noexcept {
        Vec2 local = world_to_local(point);
        return std::abs(local.x) <= half_extents.x && std::abs(local.y) <= half_extents.y;
    }
    
    // Get AABB (loose bounds)
    AABB get_aabb() const noexcept {
        auto corners = get_corners();
        Vec2 min = corners[0];
        Vec2 max = corners[0];
        
        for (usize i = 1; i < 4; ++i) {
            min.x = std::min(min.x, corners[i].x);
            min.y = std::min(min.y, corners[i].y);
            max.x = std::max(max.x, corners[i].x);
            max.y = std::max(max.y, corners[i].y);
        }
        
        return AABB{min, max};
    }
    
    // Project OBB onto an axis (returns min and max projection values)
    std::pair<f32, f32> project_onto_axis(const Vec2& axis) const noexcept {
        const Vec2& ax = get_axis_x();
        const Vec2& ay = get_axis_y();
        
        f32 center_proj = center.dot(axis);
        f32 extent_proj = std::abs(ax.dot(axis) * half_extents.x) + 
                          std::abs(ay.dot(axis) * half_extents.y);
        
        return {center_proj - extent_proj, center_proj + extent_proj};
    }
    
private:
    void update_axes() const noexcept {
        f32 cos_r = std::cos(rotation);
        f32 sin_r = std::sin(rotation);
        axis_x = Vec2{cos_r, sin_r};
        axis_y = Vec2{-sin_r, cos_r};
        axes_dirty = false;
    }
};

/**
 * @brief Convex polygon primitive
 * 
 * Represents a convex polygon with up to MAX_POLYGON_VERTICES vertices.
 * Vertices must be specified in counter-clockwise order.
 */
struct alignas(64) Polygon {
    static constexpr u32 MAX_VERTICES = constants::MAX_POLYGON_VERTICES;
    
    std::array<Vec2, MAX_VERTICES> vertices;
    u32 vertex_count{0};
    
    // Cached properties (updated when vertices change)
    mutable Vec2 centroid{0.0f, 0.0f};
    mutable f32 area_cached{0.0f};
    mutable bool properties_dirty{true};
    
    constexpr Polygon() noexcept = default;
    
    // Initialize from vertex list
    explicit Polygon(std::span<const Vec2> verts) noexcept {
        set_vertices(verts);
    }
    
    // Factory methods
    static Polygon create_box(Vec2 center, Vec2 size) noexcept {
        Vec2 half_size = size * 0.5f;
        return Polygon{std::array<Vec2, 4>{{
            center + Vec2{-half_size.x, -half_size.y},  // Bottom-left
            center + Vec2{half_size.x, -half_size.y},   // Bottom-right
            center + Vec2{half_size.x, half_size.y},    // Top-right
            center + Vec2{-half_size.x, half_size.y}    // Top-left
        }}};
    }
    
    static Polygon create_regular(Vec2 center, f32 radius, u32 sides) noexcept;
    
    // Vertex management
    void set_vertices(std::span<const Vec2> verts) noexcept {
        vertex_count = std::min(static_cast<u32>(verts.size()), MAX_VERTICES);
        std::copy_n(verts.begin(), vertex_count, vertices.begin());
        properties_dirty = true;
    }
    
    void add_vertex(const Vec2& vertex) noexcept {
        if (vertex_count < MAX_VERTICES) {
            vertices[vertex_count++] = vertex;
            properties_dirty = true;
        }
    }
    
    void clear() noexcept {
        vertex_count = 0;
        properties_dirty = true;
    }
    
    // Access
    std::span<const Vec2> get_vertices() const noexcept {
        return std::span<const Vec2>{vertices.data(), vertex_count};
    }
    
    constexpr const Vec2& operator[](u32 index) const noexcept {
        return vertices[index % vertex_count];  // Wrap around
    }
    
    // Properties (cached)
    const Vec2& get_centroid() const noexcept {
        if (properties_dirty) update_properties();
        return centroid;
    }
    
    f32 get_area() const noexcept {
        if (properties_dirty) update_properties();
        return area_cached;
    }
    
    // Geometric queries
    bool contains(const Vec2& point) const noexcept;
    bool is_convex() const noexcept;
    bool is_counter_clockwise() const noexcept;
    void ensure_counter_clockwise() noexcept;
    
    // Get edge vector (from vertex i to vertex i+1)
    Vec2 get_edge(u32 index) const noexcept {
        return vertices[(index + 1) % vertex_count] - vertices[index];
    }
    
    // Get edge normal (outward pointing)
    Vec2 get_edge_normal(u32 index) const noexcept {
        Vec2 edge = get_edge(index);
        return vec2::perpendicular(edge).normalized();
    }
    
    // Transform polygon
    Polygon transformed(const Transform2D& transform) const noexcept {
        Polygon result;
        result.vertex_count = vertex_count;
        for (u32 i = 0; i < vertex_count; ++i) {
            result.vertices[i] = transform.transform_point(vertices[i]);
        }
        result.properties_dirty = true;
        return result;
    }
    
    // Get AABB
    AABB get_aabb() const noexcept {
        if (vertex_count == 0) return AABB{};
        
        Vec2 min = vertices[0];
        Vec2 max = vertices[0];
        
        for (u32 i = 1; i < vertex_count; ++i) {
            min.x = std::min(min.x, vertices[i].x);
            min.y = std::min(min.y, vertices[i].y);
            max.x = std::max(max.x, vertices[i].x);
            max.y = std::max(max.y, vertices[i].y);
        }
        
        return AABB{min, max};
    }
    
    // Project polygon onto an axis
    std::pair<f32, f32> project_onto_axis(const Vec2& axis) const noexcept {
        if (vertex_count == 0) return {0.0f, 0.0f};
        
        f32 min_proj = vertices[0].dot(axis);
        f32 max_proj = min_proj;
        
        for (u32 i = 1; i < vertex_count; ++i) {
            f32 proj = vertices[i].dot(axis);
            min_proj = std::min(min_proj, proj);
            max_proj = std::max(max_proj, proj);
        }
        
        return {min_proj, max_proj};
    }
    
    // Get support point in given direction (used in collision detection)
    Vec2 get_support_point(const Vec2& direction) const noexcept {
        if (vertex_count == 0) return Vec2::zero();
        
        u32 best_index = 0;
        f32 best_projection = vertices[0].dot(direction);
        
        for (u32 i = 1; i < vertex_count; ++i) {
            f32 projection = vertices[i].dot(direction);
            if (projection > best_projection) {
                best_projection = projection;
                best_index = i;
            }
        }
        
        return vertices[best_index];
    }
    
private:
    void update_properties() const noexcept;
};

/**
 * @brief 2D Ray for raycasting operations
 * 
 * Represents a ray with origin and direction. Used for:
 * - Line-of-sight calculations
 * - Mouse picking
 * - Projectile trajectory queries
 * - Collision queries
 */
struct Ray2D {
    Vec2 origin{0.0f, 0.0f};        // Ray starting point
    Vec2 direction{1.0f, 0.0f};     // Ray direction (should be normalized)
    f32 max_distance{1000.0f};      // Maximum ray distance
    
    constexpr Ray2D() noexcept = default;
    
    constexpr Ray2D(Vec2 orig, Vec2 dir, f32 max_dist = 1000.0f) noexcept
        : origin(orig), direction(dir), max_distance(max_dist) {}
    
    // Factory methods
    static inline Ray2D from_to(Vec2 start, Vec2 end) noexcept {
        Vec2 dir = end - start;
        f32 dist = dir.length();
        return Ray2D{start, dir / dist, dist};
    }
    
    static constexpr Ray2D from_angle(Vec2 orig, f32 angle, f32 max_dist = 1000.0f) noexcept {
        return Ray2D{orig, vec2::from_angle(angle), max_dist};
    }
    
    // Point along ray at parameter t
    constexpr Vec2 point_at(f32 t) const noexcept {
        return origin + direction * t;
    }
    
    // Point at maximum distance
    constexpr Vec2 end_point() const noexcept {
        return point_at(max_distance);
    }
    
    // Ensure direction is normalized
    void normalize_direction() noexcept {
        direction = direction.normalized();
    }
    
    // Check if point is on ray (within epsilon)
    bool contains_point(const Vec2& point, f32 epsilon = constants::EPSILON) const noexcept {
        Vec2 to_point = point - origin;
        f32 t = to_point.dot(direction);
        
        if (t < 0.0f || t > max_distance) return false;
        
        Vec2 projected = origin + direction * t;
        return vec2::distance(projected, point) <= epsilon;
    }
};

//=============================================================================
// Distance and Collision Mathematics
//=============================================================================

/**
 * @brief Collision and distance calculation utilities
 * 
 * This namespace provides mathematical functions for:
 * - Distance calculations between primitives
 * - Closest point queries
 * - Intersection tests
 * - Normal vector computations
 */
namespace collision {
    
    /**
     * @brief Result of a distance query between two shapes
     */
    struct DistanceResult {
        f32 distance;           // Distance between shapes (negative if overlapping)
        Vec2 point_a;          // Closest point on shape A
        Vec2 point_b;          // Closest point on shape B
        Vec2 normal;           // Normal vector from A to B
        bool is_overlapping;   // Whether shapes are overlapping
        
        constexpr DistanceResult() noexcept 
            : distance(0.0f), is_overlapping(false) {}
    };
    
    /**
     * @brief Result of a raycast query
     */
    struct RaycastResult {
        bool hit;              // Whether ray hit the shape
        f32 distance;          // Distance to hit point
        Vec2 point;            // Hit point in world space
        Vec2 normal;           // Surface normal at hit point
        f32 parameter;         // Ray parameter (0 to 1 for finite rays)
        
        constexpr RaycastResult() noexcept 
            : hit(false), distance(0.0f), parameter(0.0f) {}
    };
    
    // Distance calculations
    f32 distance_point_to_line(const Vec2& point, const Vec2& line_start, const Vec2& line_end) noexcept;
    f32 distance_point_to_segment(const Vec2& point, const Vec2& seg_start, const Vec2& seg_end) noexcept;
    DistanceResult distance_circle_to_circle(const Circle& a, const Circle& b) noexcept;
    DistanceResult distance_aabb_to_aabb(const AABB& a, const AABB& b) noexcept;
    DistanceResult distance_obb_to_obb(const OBB& a, const OBB& b) noexcept;
    DistanceResult distance_circle_to_aabb(const Circle& circle, const AABB& aabb) noexcept;
    DistanceResult distance_point_to_polygon(const Vec2& point, const Polygon& polygon) noexcept;
    
    // Closest point calculations
    Vec2 closest_point_on_line(const Vec2& point, const Vec2& line_start, const Vec2& line_end) noexcept;
    Vec2 closest_point_on_segment(const Vec2& point, const Vec2& seg_start, const Vec2& seg_end) noexcept;
    Vec2 closest_point_on_circle(const Vec2& point, const Circle& circle) noexcept;
    Vec2 closest_point_on_aabb(const Vec2& point, const AABB& aabb) noexcept;
    Vec2 closest_point_on_obb(const Vec2& point, const OBB& obb) noexcept;
    Vec2 closest_point_on_polygon(const Vec2& point, const Polygon& polygon) noexcept;
    
    // Intersection tests (boolean)
    bool intersects_circle_circle(const Circle& a, const Circle& b) noexcept;
    bool intersects_aabb_aabb(const AABB& a, const AABB& b) noexcept;
    bool intersects_obb_obb(const OBB& a, const OBB& b) noexcept;
    bool intersects_circle_aabb(const Circle& circle, const AABB& aabb) noexcept;
    bool intersects_circle_obb(const Circle& circle, const OBB& obb) noexcept;
    bool intersects_polygon_polygon(const Polygon& a, const Polygon& b) noexcept;
    
    // Line/segment intersection
    bool intersects_line_line(const Vec2& a1, const Vec2& a2, const Vec2& b1, const Vec2& b2) noexcept;
    std::optional<Vec2> intersection_line_line(const Vec2& a1, const Vec2& a2, const Vec2& b1, const Vec2& b2) noexcept;
    std::optional<Vec2> intersection_segment_segment(const Vec2& a1, const Vec2& a2, const Vec2& b1, const Vec2& b2) noexcept;
    
    // Raycasting
    RaycastResult raycast_circle(const Ray2D& ray, const Circle& circle) noexcept;
    RaycastResult raycast_aabb(const Ray2D& ray, const AABB& aabb) noexcept;
    RaycastResult raycast_obb(const Ray2D& ray, const OBB& obb) noexcept;
    RaycastResult raycast_polygon(const Ray2D& ray, const Polygon& polygon) noexcept;
    
    // Advanced collision detection
    bool gjk_intersect(const Polygon& a, const Polygon& b) noexcept;
    DistanceResult gjk_distance(const Polygon& a, const Polygon& b) noexcept;
    
    // Separating Axis Theorem (SAT) for polygons
    bool sat_intersect(const Polygon& a, const Polygon& b) noexcept;
    DistanceResult sat_distance(const Polygon& a, const Polygon& b) noexcept;
}

//=============================================================================
// Physics Utility Functions
//=============================================================================

/**
 * @brief Physics utility functions and helpers
 */
namespace utils {
    
    /**
     * @brief Calculate moment of inertia for various shapes
     * 
     * Moment of inertia is crucial for rotational dynamics.
     * These functions calculate the moment of inertia about the center of mass.
     */
    f32 moment_of_inertia_circle(f32 mass, f32 radius) noexcept;
    f32 moment_of_inertia_box(f32 mass, f32 width, f32 height) noexcept;
    f32 moment_of_inertia_polygon(f32 mass, const Polygon& polygon) noexcept;
    
    /**
     * @brief Calculate center of mass for compound shapes
     */
    Vec2 center_of_mass_points(std::span<const Vec2> points, std::span<const f32> masses) noexcept;
    Vec2 center_of_mass_polygon(const Polygon& polygon) noexcept;
    
    /**
     * @brief Angle utilities
     */
    constexpr f32 normalize_angle(f32 angle) noexcept {
        while (angle > constants::PI_F) angle -= constants::TWO_PI_F;
        while (angle < -constants::PI_F) angle += constants::TWO_PI_F;
        return angle;
    }
    
    constexpr f32 angle_difference(f32 a, f32 b) noexcept {
        return normalize_angle(a - b);
    }
    
    constexpr f32 degrees_to_radians(f32 degrees) noexcept {
        return degrees * constants::DEG_TO_RAD;
    }
    
    constexpr f32 radians_to_degrees(f32 radians) noexcept {
        return radians * constants::RAD_TO_DEG;
    }
    
    /**
     * @brief Interpolation functions for physics
     */
    f32 smooth_step(f32 t) noexcept;
    f32 smoother_step(f32 t) noexcept;
    f32 ease_in_quad(f32 t) noexcept;
    f32 ease_out_quad(f32 t) noexcept;
    f32 ease_in_out_quad(f32 t) noexcept;
    
    /**
     * @brief Spring dynamics
     */
    struct SpringForce {
        f32 force;
        f32 damping_force;
    };
    
    SpringForce calculate_spring_force(f32 current_length, f32 rest_length, 
                                     f32 spring_constant, f32 damping_ratio,
                                     f32 velocity) noexcept;
    
    /**
     * @brief Numerical integration utilities
     */
    Vec2 integrate_velocity_verlet(Vec2 position, Vec2 velocity, Vec2 acceleration, f32 dt) noexcept;
    Vec2 integrate_runge_kutta_4(Vec2 position, Vec2 velocity, 
                                 std::function<Vec2(Vec2, Vec2, f32)> acceleration_func, 
                                 f32 dt, f32 time) noexcept;
    
    /**
     * @brief Area and volume calculations
     */
    f32 calculate_polygon_area(std::span<const Vec2> vertices) noexcept;
    f32 calculate_triangle_area(const Vec2& a, const Vec2& b, const Vec2& c) noexcept;
    
    /**
     * @brief Convex hull generation (Graham scan algorithm)
     */
    std::vector<Vec2> convex_hull(std::span<const Vec2> points) noexcept;
    
    /**
     * @brief Point-in-polygon tests
     */
    bool point_in_polygon_winding(const Vec2& point, std::span<const Vec2> vertices) noexcept;
    bool point_in_polygon_crossing(const Vec2& point, std::span<const Vec2> vertices) noexcept;
    
    /**
     * @brief Triangulation utilities (for complex polygons)
     */
    std::vector<std::array<u32, 3>> triangulate_polygon(const Polygon& polygon) noexcept;
    
    /**
     * @brief Bounding volume utilities
     */
    Circle smallest_enclosing_circle(std::span<const Vec2> points) noexcept;
    AABB smallest_enclosing_aabb(std::span<const Vec2> points) noexcept;
    OBB smallest_enclosing_obb(std::span<const Vec2> points) noexcept;
}

//=============================================================================
// Educational Debug Utilities
//=============================================================================

/**
 * @brief Educational debugging and visualization helpers
 * 
 * These utilities help students understand physics mathematics by providing
 * detailed information about calculations and intermediate results.
 */
namespace debug {
    
    /**
     * @brief Step-by-step collision detection breakdown
     */
    struct CollisionDebugInfo {
        struct Step {
            std::string description;
            Vec2 point_a;
            Vec2 point_b;
            Vec2 normal;
            f32 distance;
            bool significant;  // Whether this step is significant for learning
        };
        
        std::vector<Step> steps;
        collision::DistanceResult final_result;
        f64 computation_time_ms;
    };
    
    CollisionDebugInfo debug_collision_detection(const Circle& a, const Circle& b) noexcept;
    CollisionDebugInfo debug_collision_detection(const AABB& a, const AABB& b) noexcept;
    CollisionDebugInfo debug_collision_detection(const OBB& a, const OBB& b) noexcept;
    
    /**
     * @brief Visualization data structures
     */
    struct VisualizationLine {
        Vec2 start;
        Vec2 end;
        u32 color;      // RGBA color
        f32 thickness;
        bool dashed;
    };
    
    struct VisualizationPoint {
        Vec2 position;
        u32 color;
        f32 size;
        std::string label;
    };
    
    struct VisualizationData {
        std::vector<VisualizationLine> lines;
        std::vector<VisualizationPoint> points;
        std::string title;
        std::string description;
    };
    
    // Generate visualization data for educational purposes
    VisualizationData visualize_collision(const Circle& a, const Circle& b) noexcept;
    VisualizationData visualize_collision(const AABB& a, const AABB& b) noexcept;
    VisualizationData visualize_raycast(const Ray2D& ray, const Circle& target) noexcept;
    VisualizationData visualize_polygon_properties(const Polygon& polygon) noexcept;
    
    /**
     * @brief Performance analysis integration
     */
    struct PerformanceMetrics {
        f64 computation_time_ns;
        usize memory_allocated;
        u32 cache_misses_estimate;
        u32 floating_point_operations;
        std::string algorithm_complexity;
    };
    
    PerformanceMetrics analyze_performance(const std::string& operation_name,
                                         std::function<void()> operation) noexcept;
    
    /**
     * @brief Educational explanations
     */
    struct MathExplanation {
        std::string concept_name;
        std::string formula;
        std::string intuitive_explanation;
        std::vector<std::string> applications;
        std::vector<std::string> common_mistakes;
        std::string complexity_analysis;
    };
    
    MathExplanation explain_cross_product() noexcept;
    MathExplanation explain_dot_product() noexcept;
    MathExplanation explain_vector_projection() noexcept;
    MathExplanation explain_sat_algorithm() noexcept;
    MathExplanation explain_gjk_algorithm() noexcept;
    
    /**
     * @brief Unit test helpers for educational verification
     */
    bool verify_vector_operations() noexcept;
    bool verify_matrix_operations() noexcept;
    bool verify_collision_detection() noexcept;
    bool verify_geometric_properties() noexcept;
    
    /**
     * @brief Memory usage analysis
     */
    struct MemoryAnalysis {
        usize shape_memory_usage[5];  // Circle, AABB, OBB, Polygon, Ray2D
        usize cache_line_efficiency;
        usize alignment_waste;
        std::string recommendations;
    };
    
    MemoryAnalysis analyze_memory_usage() noexcept;
}

} // namespace ecscope::physics::math