#pragma once

/**
 * @file physics/collision3d.hpp
 * @brief Comprehensive 3D Collision Detection Algorithms for ECScope Physics Engine
 * 
 * This header implements state-of-the-art 3D collision detection algorithms with emphasis
 * on educational clarity and performance optimization. It extends the 2D collision detection
 * foundation to handle the increased complexity of 3D space.
 * 
 * Key Features:
 * - Advanced 3D collision detection (SAT, GJK/EPA, MPR)
 * - Distance calculations between all 3D primitive pairs
 * - Contact manifold generation for complex 3D shapes
 * - Continuous Collision Detection (CCD) for fast-moving objects
 * - Broad-phase spatial partitioning optimized for 3D
 * - Educational step-by-step algorithm visualization
 * - Performance profiling and comparison tools
 * 
 * Educational Philosophy:
 * Each algorithm includes comprehensive mathematical explanations, complexity analysis,
 * and references to computer graphics and physics literature. Students can understand
 * the progression from 2D to 3D collision detection challenges.
 * 
 * Performance Considerations:
 * - Early exit optimizations for separated objects
 * - SIMD-optimized vector operations for 3D calculations
 * - Memory-efficient contact manifold generation
 * - Cache-friendly data access patterns
 * - Integration with job system for parallel broad-phase
 * 
 * 3D Specific Challenges:
 * - Increased computational complexity (O(n³) vs O(n²) for some algorithms)
 * - More complex shape representations (convex hulls, meshes)
 * - Higher-dimensional separating axis tests
 * - 3D contact manifold clipping and reduction
 * 
 * @author ECScope Educational ECS Framework - 3D Physics Extension
 * @date 2025
 */

#include "math3d.hpp"
#include "collision.hpp"  // Include 2D collision foundation
#include "core/types.hpp"
#include <optional>
#include <vector>
#include <array>
#include <span>

namespace ecscope::physics::collision3d {

// Import 3D math foundation
using namespace ecscope::physics::math3d;

// Import 2D collision foundation for comparison
using namespace ecscope::physics::collision;

//=============================================================================
// 3D Geometric Primitives
//=============================================================================

/**
 * @brief 3D Sphere primitive
 * 
 * Educational Context:
 * Spheres are the simplest 3D collision primitive, analogous to circles in 2D.
 * They have constant-time intersection tests and are used for broad-phase culling.
 */
struct Sphere {
    Vec3 center{0.0f, 0.0f, 0.0f};
    f32 radius{1.0f};
    
    constexpr Sphere() noexcept = default;
    constexpr Sphere(Vec3 c, f32 r) noexcept : center(c), radius(r) {}
    constexpr Sphere(f32 x, f32 y, f32 z, f32 r) noexcept : center(x, y, z), radius(r) {}
    
    // Geometric properties
    constexpr f32 volume() const noexcept {
        return (4.0f / 3.0f) * constants::PI_F * radius * radius * radius;
    }
    
    constexpr f32 surface_area() const noexcept {
        return 4.0f * constants::PI_F * radius * radius;
    }
    
    // Containment tests
    constexpr bool contains(const Vec3& point) const noexcept {
        return center.distance_squared_to(point) <= radius * radius;
    }
    
    bool contains(const Sphere& other) const noexcept {
        f32 distance = center.distance_to(other.center);
        return distance + other.radius <= radius;
    }
    
    // Bounding box
    struct AABB3D get_aabb() const noexcept;
    
    // Transform
    Sphere transformed(const Transform3D& transform) const noexcept {
        Vec3 world_center = transform.transform_point(center);
        f32 max_scale = std::max({transform.scale.x, transform.scale.y, transform.scale.z});
        return Sphere{world_center, radius * max_scale};
    }
    
    // Support function for GJK/EPA
    Vec3 get_support_point(const Vec3& direction) const noexcept {
        return center + direction.normalized() * radius;
    }
};

/**
 * @brief 3D Axis-Aligned Bounding Box (AABB3D)
 * 
 * Educational Context:
 * 3D AABB extends 2D AABB with a third dimension. Still very efficient for
 * broad-phase collision detection due to simple min/max comparisons.
 */
struct alignas(32) AABB3D {
    Vec3 min{-1.0f, -1.0f, -1.0f};  // Minimum corner
    Vec3 max{1.0f, 1.0f, 1.0f};     // Maximum corner
    
    constexpr AABB3D() noexcept = default;
    constexpr AABB3D(Vec3 minimum, Vec3 maximum) noexcept : min(minimum), max(maximum) {}
    
    constexpr AABB3D(f32 min_x, f32 min_y, f32 min_z, f32 max_x, f32 max_y, f32 max_z) noexcept 
        : min(min_x, min_y, min_z), max(max_x, max_y, max_z) {}
    
    // Factory methods
    static constexpr AABB3D from_center_size(Vec3 center, Vec3 size) noexcept {
        Vec3 half_size = size * 0.5f;
        return AABB3D{center - half_size, center + half_size};
    }
    
    static constexpr AABB3D from_points(const Vec3& a, const Vec3& b) noexcept {
        return AABB3D{
            Vec3{std::min(a.x, b.x), std::min(a.y, b.y), std::min(a.z, b.z)},
            Vec3{std::max(a.x, b.x), std::max(a.y, b.y), std::max(a.z, b.z)}
        };
    }
    
    static AABB3D from_points(std::span<const Vec3> points) noexcept;
    
    // Properties
    constexpr Vec3 center() const noexcept {
        return (min + max) * 0.5f;
    }
    
    constexpr Vec3 size() const noexcept {
        return max - min;
    }
    
    constexpr Vec3 half_size() const noexcept {
        return (max - min) * 0.5f;
    }
    
    constexpr f32 width() const noexcept { return max.x - min.x; }
    constexpr f32 height() const noexcept { return max.y - min.y; }
    constexpr f32 depth() const noexcept { return max.z - min.z; }
    
    constexpr f32 volume() const noexcept {
        return width() * height() * depth();
    }
    
    constexpr f32 surface_area() const noexcept {
        f32 w = width(), h = height(), d = depth();
        return 2.0f * (w * h + w * d + h * d);
    }
    
    // Validation
    constexpr bool is_valid() const noexcept {
        return min.x <= max.x && min.y <= max.y && min.z <= max.z;
    }
    
    // Containment tests
    constexpr bool contains(const Vec3& point) const noexcept {
        return point.x >= min.x && point.x <= max.x &&
               point.y >= min.y && point.y <= max.y &&
               point.z >= min.z && point.z <= max.z;
    }
    
    constexpr bool contains(const AABB3D& other) const noexcept {
        return other.min.x >= min.x && other.max.x <= max.x &&
               other.min.y >= min.y && other.max.y <= max.y &&
               other.min.z >= min.z && other.max.z <= max.z;
    }
    
    // Intersection test
    constexpr bool intersects(const AABB3D& other) const noexcept {
        return !(other.min.x > max.x || other.max.x < min.x ||
                 other.min.y > max.y || other.max.y < min.y ||
                 other.min.z > max.z || other.max.z < min.z);
    }
    
    // Closest point
    constexpr Vec3 closest_point(const Vec3& point) const noexcept {
        return Vec3{
            std::clamp(point.x, min.x, max.x),
            std::clamp(point.y, min.y, max.y),
            std::clamp(point.z, min.z, max.z)
        };
    }
    
    // Union and intersection
    constexpr AABB3D union_with(const AABB3D& other) const noexcept {
        return AABB3D{
            Vec3{std::min(min.x, other.min.x), std::min(min.y, other.min.y), std::min(min.z, other.min.z)},
            Vec3{std::max(max.x, other.max.x), std::max(max.y, other.max.y), std::max(max.z, other.max.z)}
        };
    }
    
    std::optional<AABB3D> intersection_with(const AABB3D& other) const noexcept {
        AABB3D result{
            Vec3{std::max(min.x, other.min.x), std::max(min.y, other.min.y), std::max(min.z, other.min.z)},
            Vec3{std::min(max.x, other.max.x), std::min(max.y, other.max.y), std::min(max.z, other.max.z)}
        };
        return result.is_valid() ? std::optional<AABB3D>{result} : std::nullopt;
    }
    
    // Expansion
    constexpr AABB3D expanded(f32 amount) const noexcept {
        Vec3 expansion{amount, amount, amount};
        return AABB3D{min - expansion, max + expansion};
    }
    
    constexpr AABB3D expanded(const Vec3& amount) const noexcept {
        return AABB3D{min - amount, max + amount};
    }
    
    // Get corner points (8 corners for 3D box)
    constexpr Vec3 corner(u32 index) const noexcept {
        // Corner indexing: binary representation determines min/max for each axis
        return Vec3{
            (index & 1) ? max.x : min.x,
            (index & 2) ? max.y : min.y,
            (index & 4) ? max.z : min.z
        };
    }
    
    std::array<Vec3, 8> get_corners() const noexcept {
        return {{
            min,                           // 000
            {max.x, min.y, min.z},        // 001
            {min.x, max.y, min.z},        // 010
            {max.x, max.y, min.z},        // 011
            {min.x, min.y, max.z},        // 100
            {max.x, min.y, max.z},        // 101
            {min.x, max.y, max.z},        // 110
            max                            // 111
        }};
    }
    
    // Support function for GJK/EPA
    Vec3 get_support_point(const Vec3& direction) const noexcept {
        return Vec3{
            direction.x > 0.0f ? max.x : min.x,
            direction.y > 0.0f ? max.y : min.y,
            direction.z > 0.0f ? max.z : min.z
        };
    }
};

/**
 * @brief 3D Oriented Bounding Box (OBB3D)
 * 
 * Educational Context:
 * 3D OBB extends 2D OBB with a third axis. Provides tighter bounds than AABB
 * for rotated objects but requires more complex intersection tests.
 */
struct alignas(64) OBB3D {
    Vec3 center{0.0f, 0.0f, 0.0f};      // Center point
    Vec3 half_extents{1.0f, 1.0f, 1.0f}; // Half-extents along local axes
    Matrix3 orientation;                  // Orientation matrix (local to world)
    
    // Cached axes (columns of orientation matrix)
    mutable bool axes_dirty{true};
    
    constexpr OBB3D() noexcept = default;
    
    constexpr OBB3D(Vec3 c, Vec3 extents, const Matrix3& orient = Matrix3::identity()) noexcept
        : center(c), half_extents(extents), orientation(orient), axes_dirty(true) {}
    
    // Factory methods
    static constexpr OBB3D from_aabb(const AABB3D& aabb, const Matrix3& orient = Matrix3::identity()) noexcept {
        return OBB3D{aabb.center(), aabb.half_size(), orient};
    }
    
    static constexpr OBB3D from_transform(const Transform3D& transform, Vec3 local_extents) noexcept {
        Vec3 world_extents = Vec3{
            local_extents.x * transform.scale.x, 
            local_extents.y * transform.scale.y,
            local_extents.z * transform.scale.z
        };
        return OBB3D{transform.position, world_extents, transform.get_rotation_matrix()};
    }
    
    // Axis access
    const Vec3& get_axis_x() const noexcept { return orientation.col0; }
    const Vec3& get_axis_y() const noexcept { return orientation.col1; }
    const Vec3& get_axis_z() const noexcept { return orientation.col2; }
    
    // Properties
    constexpr Vec3 size() const noexcept {
        return half_extents * 2.0f;
    }
    
    constexpr f32 volume() const noexcept {
        return 8.0f * half_extents.x * half_extents.y * half_extents.z;
    }
    
    f32 surface_area() const noexcept {
        const Vec3 size_vec = size();
        return 2.0f * (size_vec.x * size_vec.y + size_vec.x * size_vec.z + size_vec.y * size_vec.z);
    }
    
    // Get corner points in world space (8 corners)
    std::array<Vec3, 8> get_corners() const noexcept {
        const Vec3& ax = get_axis_x();
        const Vec3& ay = get_axis_y();
        const Vec3& az = get_axis_z();
        
        const Vec3 x_extent = ax * half_extents.x;
        const Vec3 y_extent = ay * half_extents.y;
        const Vec3 z_extent = az * half_extents.z;
        
        return {{
            center - x_extent - y_extent - z_extent,  // ---
            center + x_extent - y_extent - z_extent,  // +--
            center - x_extent + y_extent - z_extent,  // -+-
            center + x_extent + y_extent - z_extent,  // ++-
            center - x_extent - y_extent + z_extent,  // --+
            center + x_extent - y_extent + z_extent,  // +-+
            center - x_extent + y_extent + z_extent,  // -++
            center + x_extent + y_extent + z_extent   // +++
        }};
    }
    
    // Transform point from world to local coordinates
    Vec3 world_to_local(const Vec3& world_point) const noexcept {
        Vec3 delta = world_point - center;
        return Vec3{
            delta.dot(get_axis_x()),
            delta.dot(get_axis_y()),
            delta.dot(get_axis_z())
        };
    }
    
    // Transform point from local to world coordinates
    Vec3 local_to_world(const Vec3& local_point) const noexcept {
        return center + orientation * local_point;
    }
    
    // Containment test
    bool contains(const Vec3& point) const noexcept {
        Vec3 local = world_to_local(point);
        return std::abs(local.x) <= half_extents.x && 
               std::abs(local.y) <= half_extents.y && 
               std::abs(local.z) <= half_extents.z;
    }
    
    // Get AABB (loose bounds)
    AABB3D get_aabb() const noexcept {
        auto corners = get_corners();
        Vec3 min_corner = corners[0];
        Vec3 max_corner = corners[0];
        
        for (usize i = 1; i < 8; ++i) {
            min_corner.x = std::min(min_corner.x, corners[i].x);
            min_corner.y = std::min(min_corner.y, corners[i].y);
            min_corner.z = std::min(min_corner.z, corners[i].z);
            max_corner.x = std::max(max_corner.x, corners[i].x);
            max_corner.y = std::max(max_corner.y, corners[i].y);
            max_corner.z = std::max(max_corner.z, corners[i].z);
        }
        
        return AABB3D{min_corner, max_corner};
    }
    
    // Project OBB onto an axis (returns min and max projection values)
    std::pair<f32, f32> project_onto_axis(const Vec3& axis) const noexcept {
        const Vec3& ax = get_axis_x();
        const Vec3& ay = get_axis_y();
        const Vec3& az = get_axis_z();
        
        f32 center_proj = center.dot(axis);
        f32 extent_proj = std::abs(ax.dot(axis) * half_extents.x) + 
                          std::abs(ay.dot(axis) * half_extents.y) +
                          std::abs(az.dot(axis) * half_extents.z);
        
        return {center_proj - extent_proj, center_proj + extent_proj};
    }
    
    // Support function for GJK/EPA
    Vec3 get_support_point(const Vec3& direction) const noexcept {
        const Vec3& ax = get_axis_x();
        const Vec3& ay = get_axis_y();
        const Vec3& az = get_axis_z();
        
        Vec3 support = center;
        support += ax * (direction.dot(ax) > 0.0f ? half_extents.x : -half_extents.x);
        support += ay * (direction.dot(ay) > 0.0f ? half_extents.y : -half_extents.y);
        support += az * (direction.dot(az) > 0.0f ? half_extents.z : -half_extents.z);
        
        return support;
    }
};

/**
 * @brief 3D Capsule primitive (sphere-swept line segment)
 * 
 * Educational Context:
 * Capsules are commonly used in 3D physics for character controllers and
 * cylindrical objects. They consist of a line segment with spherical caps.
 */
struct Capsule {
    Vec3 point_a{0.0f, -1.0f, 0.0f};   // First endpoint
    Vec3 point_b{0.0f, 1.0f, 0.0f};    // Second endpoint
    f32 radius{0.5f};                   // Radius of spherical caps
    
    constexpr Capsule() noexcept = default;
    constexpr Capsule(Vec3 a, Vec3 b, f32 r) noexcept : point_a(a), point_b(b), radius(r) {}
    
    // Properties
    f32 height() const noexcept {
        return point_a.distance_to(point_b);
    }
    
    Vec3 center() const noexcept {
        return (point_a + point_b) * 0.5f;
    }
    
    Vec3 axis() const noexcept {
        return (point_b - point_a).normalized();
    }
    
    f32 volume() const noexcept {
        f32 h = height();
        f32 cylinder_volume = constants::PI_F * radius * radius * h;
        f32 sphere_volume = (4.0f / 3.0f) * constants::PI_F * radius * radius * radius;
        return cylinder_volume + sphere_volume;
    }
    
    f32 surface_area() const noexcept {
        f32 h = height();
        f32 cylinder_area = constants::TWO_PI_F * radius * h;
        f32 sphere_area = 4.0f * constants::PI_F * radius * radius;
        return cylinder_area + sphere_area;
    }
    
    // Get closest point on the line segment to a given point
    Vec3 closest_point_on_segment(const Vec3& point) const noexcept {
        Vec3 ab = point_b - point_a;
        Vec3 ap = point - point_a;
        f32 ab_length_sq = ab.length_squared();
        
        if (ab_length_sq < constants::EPSILON) {
            return point_a; // Degenerate capsule
        }
        
        f32 t = std::clamp(ap.dot(ab) / ab_length_sq, 0.0f, 1.0f);
        return point_a + ab * t;
    }
    
    // Distance from point to capsule surface
    f32 distance_to_surface(const Vec3& point) const noexcept {
        Vec3 closest_on_segment = closest_point_on_segment(point);
        return std::max(0.0f, closest_on_segment.distance_to(point) - radius);
    }
    
    // Support function for GJK/EPA
    Vec3 get_support_point(const Vec3& direction) const noexcept {
        Vec3 line_center = center();
        Vec3 line_axis = axis();
        f32 line_half_height = height() * 0.5f;
        
        // Choose the endpoint that's furthest in the direction
        Vec3 endpoint = (direction.dot(line_axis) > 0.0f) ? point_b : point_a;
        
        // Add spherical cap contribution
        Vec3 dir_normalized = direction.normalized();
        return endpoint + dir_normalized * radius;
    }
    
    // Get AABB
    AABB3D get_aabb() const noexcept {
        Vec3 min_point = Vec3{
            std::min(point_a.x, point_b.x) - radius,
            std::min(point_a.y, point_b.y) - radius,
            std::min(point_a.z, point_b.z) - radius
        };
        Vec3 max_point = Vec3{
            std::max(point_a.x, point_b.x) + radius,
            std::max(point_a.y, point_b.y) + radius,
            std::max(point_a.z, point_b.z) + radius
        };
        return AABB3D{min_point, max_point};
    }
};

/**
 * @brief 3D Convex Hull primitive
 * 
 * Educational Context:
 * Convex hulls represent arbitrary convex shapes in 3D. They're the most
 * general convex primitive and are used for complex collision shapes.
 */
struct ConvexHull {
    static constexpr u32 MAX_VERTICES = 64;   // Reasonable limit for real-time physics
    static constexpr u32 MAX_FACES = 128;     // Theoretical max for 64 vertices
    static constexpr u32 MAX_EDGES = 192;     // Theoretical max edges
    
    // Vertex data
    std::array<Vec3, MAX_VERTICES> vertices;
    u32 vertex_count{0};
    
    // Face data (each face stores vertex indices in CCW order)
    struct Face {
        std::array<u32, 3> vertex_indices; // Triangle face (3 vertices)
        Vec3 normal;                        // Outward-facing normal
        f32 distance_to_origin;             // Distance to origin along normal
        
        Face() = default;
        Face(u32 v0, u32 v1, u32 v2, const Vec3& norm, f32 dist) 
            : vertex_indices{v0, v1, v2}, normal(norm), distance_to_origin(dist) {}
    };
    
    std::array<Face, MAX_FACES> faces;
    u32 face_count{0};
    
    // Cached properties
    mutable Vec3 centroid{0.0f, 0.0f, 0.0f};
    mutable f32 volume_cached{0.0f};
    mutable bool properties_dirty{true};
    
    constexpr ConvexHull() noexcept = default;
    
    // Initialize from vertex list (will compute convex hull)
    explicit ConvexHull(std::span<const Vec3> verts) noexcept;
    
    // Vertex management
    void set_vertices(std::span<const Vec3> verts) noexcept;
    void add_vertex(const Vec3& vertex) noexcept;
    void clear() noexcept;
    
    // Access
    std::span<const Vec3> get_vertices() const noexcept {
        return std::span<const Vec3>{vertices.data(), vertex_count};
    }
    
    std::span<const Face> get_faces() const noexcept {
        return std::span<const Face>{faces.data(), face_count};
    }
    
    constexpr const Vec3& operator[](u32 index) const noexcept {
        return vertices[index];
    }
    
    // Properties (cached)
    const Vec3& get_centroid() const noexcept {
        if (properties_dirty) update_properties();
        return centroid;
    }
    
    f32 get_volume() const noexcept {
        if (properties_dirty) update_properties();
        return volume_cached;
    }
    
    // Geometric queries
    bool contains(const Vec3& point) const noexcept;
    f32 distance_to_surface(const Vec3& point) const noexcept;
    Vec3 closest_point_on_surface(const Vec3& point) const noexcept;
    
    // Support function for GJK/EPA (essential for convex hulls)
    Vec3 get_support_point(const Vec3& direction) const noexcept {
        if (vertex_count == 0) return Vec3::zero();
        
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
    
    // Get AABB
    AABB3D get_aabb() const noexcept {
        if (vertex_count == 0) return AABB3D{};
        
        Vec3 min_point = vertices[0];
        Vec3 max_point = vertices[0];
        
        for (u32 i = 1; i < vertex_count; ++i) {
            min_point.x = std::min(min_point.x, vertices[i].x);
            min_point.y = std::min(min_point.y, vertices[i].y);
            min_point.z = std::min(min_point.z, vertices[i].z);
            max_point.x = std::max(max_point.x, vertices[i].x);
            max_point.y = std::max(max_point.y, vertices[i].y);
            max_point.z = std::max(max_point.z, vertices[i].z);
        }
        
        return AABB3D{min_point, max_point};
    }
    
    // Transform convex hull
    ConvexHull transformed(const Transform3D& transform) const noexcept;
    
private:
    void update_properties() const noexcept;
    void build_convex_hull() noexcept;  // Compute convex hull from vertices
    void compute_face_normals() noexcept;
};

/**
 * @brief 3D Ray for raycasting operations
 */
struct Ray3D {
    Vec3 origin{0.0f, 0.0f, 0.0f};      // Ray starting point
    Vec3 direction{0.0f, 0.0f, -1.0f};  // Ray direction (should be normalized)
    f32 max_distance{1000.0f};          // Maximum ray distance
    
    constexpr Ray3D() noexcept = default;
    
    constexpr Ray3D(Vec3 orig, Vec3 dir, f32 max_dist = 1000.0f) noexcept
        : origin(orig), direction(dir), max_distance(max_dist) {}
    
    // Factory methods
    static Ray3D from_to(Vec3 start, Vec3 end) noexcept {
        Vec3 dir = end - start;
        f32 dist = dir.length();
        return Ray3D{start, dir / dist, dist};
    }
    
    // Point along ray at parameter t
    constexpr Vec3 point_at(f32 t) const noexcept {
        return origin + direction * t;
    }
    
    // Point at maximum distance
    constexpr Vec3 end_point() const noexcept {
        return point_at(max_distance);
    }
    
    // Ensure direction is normalized
    void normalize_direction() noexcept {
        direction = direction.normalized();
    }
};

//=============================================================================
// 3D Distance and Collision Results
//=============================================================================

/**
 * @brief Result of 3D distance calculation between shapes
 */
struct DistanceResult3D {
    f32 distance;                  // Distance between shapes (negative = penetration)
    Vec3 point_a;                  // Closest point on shape A
    Vec3 point_b;                  // Closest point on shape B
    Vec3 normal;                   // Normal from A to B
    bool is_overlapping;           // Whether shapes are overlapping
    
    /** @brief Educational debug information */
    struct DebugInfo {
        u32 iterations_used;       // Number of algorithm iterations
        f64 computation_time_ns;   // Time taken to compute
        std::string algorithm_used; // Which algorithm was used
        f32 precision_achieved;    // Final precision/error
        std::vector<Vec3> intermediate_points; // For GJK visualization
    } debug_info;
    
    DistanceResult3D() noexcept 
        : distance(0.0f), is_overlapping(false) {}
    
    // Factory methods
    static DistanceResult3D overlapping(const Vec3& point_a, const Vec3& point_b, 
                                      const Vec3& normal, f32 penetration) {
        DistanceResult3D result;
        result.distance = -penetration;
        result.point_a = point_a;
        result.point_b = point_b;
        result.normal = normal;
        result.is_overlapping = true;
        return result;
    }
    
    static DistanceResult3D separated(const Vec3& point_a, const Vec3& point_b, f32 distance) {
        DistanceResult3D result;
        result.distance = distance;
        result.point_a = point_a;
        result.point_b = point_b;
        result.normal = (point_b - point_a).normalized();
        result.is_overlapping = false;
        return result;
    }
};

/**
 * @brief Result of 3D raycast operation
 */
struct RaycastResult3D {
    bool hit;                      // Whether ray hit the shape
    f32 distance;                  // Distance along ray to hit point
    Vec3 point;                    // Hit point in world space
    Vec3 normal;                   // Surface normal at hit point
    f32 parameter;                 // Ray parameter t (0 to max_distance)
    
    /** @brief Additional raycast information */
    u32 shape_id;                  // ID of shape that was hit
    Vec3 local_point;              // Hit point in shape's local space
    bool is_backface_hit;          // Whether ray hit back face of shape
    
    RaycastResult3D() noexcept 
        : hit(false), distance(0.0f), parameter(0.0f)
        , shape_id(0), is_backface_hit(false) {}
    
    // Factory methods
    static RaycastResult3D hit_result(f32 dist, const Vec3& point, const Vec3& normal, f32 param) {
        RaycastResult3D result;
        result.hit = true;
        result.distance = dist;
        result.point = point;
        result.normal = normal;
        result.parameter = param;
        return result;
    }
    
    static RaycastResult3D miss() {
        return RaycastResult3D{};
    }
};

//=============================================================================
// 3D Contact Manifold Generation
//=============================================================================

/**
 * @brief 3D Contact point between two colliding objects
 */
struct ContactPoint3D {
    Vec3 point;                    // Contact point in world space
    Vec3 normal;                   // Contact normal (from A to B)
    f32 penetration_depth;         // How deep objects are overlapping
    f32 normal_impulse;            // Accumulated normal impulse
    f32 tangent1_impulse;          // Accumulated tangent impulse (friction direction 1)
    f32 tangent2_impulse;          // Accumulated tangent impulse (friction direction 2)
    
    /** @brief Local coordinates on each shape */
    Vec3 local_point_a;            // Contact point in A's local space
    Vec3 local_point_b;            // Contact point in B's local space
    
    /** @brief Contact properties */
    f32 restitution;               // Combined restitution coefficient
    f32 friction;                  // Combined friction coefficient
    
    /** @brief Contact persistence tracking */
    u32 id;                        // Unique contact ID for tracking
    f32 lifetime;                  // How long this contact has existed
    bool is_new_contact;           // Whether this is a new contact this frame
    
    ContactPoint3D() noexcept 
        : penetration_depth(0.0f), normal_impulse(0.0f)
        , tangent1_impulse(0.0f), tangent2_impulse(0.0f)
        , restitution(0.0f), friction(0.0f), id(0), lifetime(0.0f), is_new_contact(true) {}
};

/**
 * @brief 3D Contact manifold containing all contact points between two objects
 * 
 * Educational Context:
 * 3D contact manifolds are more complex than 2D due to the possibility of
 * face-face contacts that can generate multiple contact points arranged
 * in complex patterns.
 */
struct ContactManifold3D {
    /** @brief Contact points (up to 8 for face-face contact in 3D) */
    static constexpr u32 MAX_CONTACT_POINTS_3D = 8;
    std::array<ContactPoint3D, MAX_CONTACT_POINTS_3D> points;
    u32 point_count{0};
    
    /** @brief Shared contact properties */
    Vec3 normal;                   // Average contact normal
    f32 restitution;               // Combined restitution
    f32 friction;                  // Combined friction
    
    /** @brief Object identification */
    u32 body_a_id;                 // First object ID
    u32 body_b_id;                 // Second object ID
    
    /** @brief Manifold properties */
    f32 total_impulse;             // Total impulse applied this frame
    f32 manifold_lifetime;         // How long these objects have been in contact
    bool is_sensor_contact;        // Whether this is a sensor/trigger contact
    
    ContactManifold3D() noexcept 
        : restitution(0.0f), friction(0.0f), body_a_id(0), body_b_id(0)
        , total_impulse(0.0f), manifold_lifetime(0.0f), is_sensor_contact(false) {}
    
    /** @brief Add contact point to manifold */
    void add_contact_point(const ContactPoint3D& point) {
        if (point_count < MAX_CONTACT_POINTS_3D) {
            points[point_count++] = point;
        }
    }
    
    /** @brief Clear all contact points */
    void clear() {
        point_count = 0;
        total_impulse = 0.0f;
    }
    
    /** @brief Get contact points as span */
    std::span<const ContactPoint3D> get_contact_points() const {
        return std::span<const ContactPoint3D>(points.data(), point_count);
    }
    
    /** @brief Check if manifold has any contact points */
    bool has_contacts() const { return point_count > 0; }
    
    /** @brief Reduce contact points to essential set (for performance) */
    void reduce_contact_points() noexcept;
};

//=============================================================================
// Forward Declarations for Template Implementations
//=============================================================================

template<typename ShapeA, typename ShapeB>
DistanceResult3D calculate_distance_3d(const ShapeA& a, const ShapeB& b) noexcept;

template<typename ShapeA, typename ShapeB>
std::optional<ContactManifold3D> generate_contact_manifold_3d(
    const ShapeA& a, const ShapeB& b,
    const Transform3D& transform_a, 
    const Transform3D& transform_b) noexcept;

} // namespace ecscope::physics::collision3d