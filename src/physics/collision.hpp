#pragma once

/**
 * @file physics/collision.hpp
 * @brief Collision Detection Algorithms for ECScope Phase 5: Física 2D
 * 
 * This header implements comprehensive collision detection algorithms for 2D physics
 * simulation with emphasis on educational clarity and performance optimization.
 * 
 * Key Features:
 * - Distance calculations between all primitive pairs
 * - Separating Axis Theorem (SAT) implementation with educational explanations
 * - GJK algorithm for advanced collision detection
 * - Raycast operations for all shape types
 * - Contact manifold generation for physics response
 * - Educational debugging and step-by-step visualization
 * 
 * Educational Philosophy:
 * Each algorithm includes detailed mathematical explanations, complexity analysis,
 * and references to physics/computer graphics literature. Students can trace
 * through each step of the collision detection process.
 * 
 * Performance Considerations:
 * - Early exit optimizations for separated objects
 * - SIMD-optimized vector operations where beneficial
 * - Memory-efficient contact manifold generation
 * - Cache-friendly data access patterns
 * 
 * @author ECScope Educational ECS Framework - Phase 5: Física 2D
 * @date 2024
 */

#include "math.hpp"
#include "../core/types.hpp"
#include <optional>
#include <vector>
#include <array>

namespace ecscope::physics::collision {

// Import math types
using namespace ecscope::physics::math;

//=============================================================================
// Contact Point and Manifold Structures
//=============================================================================

/**
 * @brief Single contact point between two colliding objects
 * 
 * Contains all information needed for collision response at a specific point.
 */
struct ContactPoint {
    Vec2 point;                    // Contact point in world space
    Vec2 normal;                   // Contact normal (from A to B)
    f32 penetration_depth;         // How deep objects are overlapping
    f32 normal_impulse;            // Accumulated normal impulse for warm starting
    f32 tangent_impulse;           // Accumulated tangent impulse for friction
    
    /** @brief Local coordinates on each shape for contact coherency */
    Vec2 local_point_a;            // Contact point in A's local space
    Vec2 local_point_b;            // Contact point in B's local space
    
    /** @brief Contact properties */
    f32 restitution;               // Combined restitution coefficient
    f32 friction;                  // Combined friction coefficient
    
    /** @brief Contact persistence tracking */
    u32 id;                        // Unique contact ID for tracking
    f32 lifetime;                  // How long this contact has existed
    bool is_new_contact;           // Whether this is a new contact this frame
    
    ContactPoint() noexcept 
        : penetration_depth(0.0f), normal_impulse(0.0f), tangent_impulse(0.0f)
        , restitution(0.0f), friction(0.0f), id(0), lifetime(0.0f), is_new_contact(true) {}
};

/**
 * @brief Contact manifold containing all contact points between two objects
 * 
 * Most collision pairs produce 1-4 contact points. The manifold manages
 * these points and their shared properties.
 */
struct ContactManifold {
    /** @brief Contact points (maximum 4 for polygon-polygon) */
    std::array<ContactPoint, constants::MAX_CONTACT_POINTS> points;
    u32 point_count{0};
    
    /** @brief Shared contact properties */
    Vec2 normal;                   // Average contact normal
    f32 restitution;               // Combined restitution
    f32 friction;                  // Combined friction
    
    /** @brief Object identification */
    u32 body_a_id;                 // First object ID
    u32 body_b_id;                 // Second object ID
    
    /** @brief Manifold properties */
    f32 total_impulse;             // Total impulse applied this frame
    f32 manifold_lifetime;         // How long these objects have been in contact
    bool is_sensor_contact;        // Whether this is a sensor/trigger contact
    
    ContactManifold() noexcept 
        : restitution(0.0f), friction(0.0f), body_a_id(0), body_b_id(0)
        , total_impulse(0.0f), manifold_lifetime(0.0f), is_sensor_contact(false) {}
    
    /** @brief Add contact point to manifold */
    void add_contact_point(const ContactPoint& point) {
        if (point_count < constants::MAX_CONTACT_POINTS) {
            points[point_count++] = point;
        }
    }
    
    /** @brief Clear all contact points */
    void clear() {
        point_count = 0;
        total_impulse = 0.0f;
    }
    
    /** @brief Get contact points as span */
    std::span<const ContactPoint> get_contact_points() const {
        return std::span<const ContactPoint>(points.data(), point_count);
    }
    
    /** @brief Check if manifold has any contact points */
    bool has_contacts() const { return point_count > 0; }
};

//=============================================================================
// Distance and Intersection Results
//=============================================================================

/**
 * @brief Result of distance calculation between two shapes
 */
struct DistanceResult {
    f32 distance;                  // Distance between shapes (negative = penetration)
    Vec2 point_a;                  // Closest point on shape A
    Vec2 point_b;                  // Closest point on shape B
    Vec2 normal;                   // Normal from A to B
    bool is_overlapping;           // Whether shapes are overlapping
    
    /** @brief Educational debug information */
    struct DebugInfo {
        u32 iterations_used;       // Number of algorithm iterations
        f32 computation_time_ns;   // Time taken to compute
        std::string algorithm_used; // Which algorithm was used
        f32 precision_achieved;    // Final precision/error
    } debug_info;
    
    DistanceResult() noexcept 
        : distance(0.0f), is_overlapping(false) {}
    
    /** @brief Create overlapping result */
    static DistanceResult overlapping(const Vec2& point_a, const Vec2& point_b, 
                                    const Vec2& normal, f32 penetration) {
        DistanceResult result;
        result.distance = -penetration;
        result.point_a = point_a;
        result.point_b = point_b;
        result.normal = normal;
        result.is_overlapping = true;
        return result;
    }
    
    /** @brief Create separated result */
    static DistanceResult separated(const Vec2& point_a, const Vec2& point_b, f32 distance) {
        DistanceResult result;
        result.distance = distance;
        result.point_a = point_a;
        result.point_b = point_b;
        result.normal = (point_b - point_a).normalized();
        result.is_overlapping = false;
        return result;
    }
};

/**
 * @brief Result of raycast operation
 */
struct RaycastResult {
    bool hit;                      // Whether ray hit the shape
    f32 distance;                  // Distance along ray to hit point
    Vec2 point;                    // Hit point in world space
    Vec2 normal;                   // Surface normal at hit point
    f32 parameter;                 // Ray parameter t (0 to max_distance)
    
    /** @brief Additional raycast information */
    u32 shape_id;                  // ID of shape that was hit
    Vec2 local_point;              // Hit point in shape's local space
    bool is_backface_hit;          // Whether ray hit back face of shape
    
    RaycastResult() noexcept 
        : hit(false), distance(0.0f), parameter(0.0f)
        , shape_id(0), is_backface_hit(false) {}
    
    /** @brief Create hit result */
    static RaycastResult hit_result(f32 dist, const Vec2& point, const Vec2& normal, f32 param) {
        RaycastResult result;
        result.hit = true;
        result.distance = dist;
        result.point = point;
        result.normal = normal;
        result.parameter = param;
        return result;
    }
    
    /** @brief Create miss result */
    static RaycastResult miss() {
        return RaycastResult{};  // Default constructor creates miss
    }
};

//=============================================================================
// Primitive Distance Functions
//=============================================================================

/**
 * @brief Calculate distance between two circles
 * 
 * Educational Context:
 * Circle-circle collision is the simplest case in 2D physics.
 * Distance = |center_a - center_b| - radius_a - radius_b
 * If distance < 0, circles are overlapping.
 * 
 * Time Complexity: O(1)
 * Space Complexity: O(1)
 */
DistanceResult distance_circle_to_circle(const Circle& a, const Circle& b) noexcept;

/**
 * @brief Calculate distance between two AABBs
 * 
 * Educational Context:
 * AABB collision uses separating axis theorem with axis-aligned axes.
 * Very fast due to simple min/max comparisons.
 * 
 * Time Complexity: O(1)
 * Space Complexity: O(1)
 */
DistanceResult distance_aabb_to_aabb(const AABB& a, const AABB& b) noexcept;

/**
 * @brief Calculate distance between two OBBs
 * 
 * Educational Context:
 * OBB collision uses separating axis theorem with oriented axes.
 * More expensive than AABB but provides tighter bounds for rotated objects.
 * Tests 4 axes: 2 for each OBB.
 * 
 * Time Complexity: O(1) but with higher constant than AABB
 * Space Complexity: O(1)
 */
DistanceResult distance_obb_to_obb(const OBB& a, const OBB& b) noexcept;

/**
 * @brief Calculate distance between circle and AABB
 * 
 * Educational Context:
 * Hybrid algorithm that finds closest point on AABB to circle center,
 * then treats as circle-point distance calculation.
 * 
 * Time Complexity: O(1)
 * Space Complexity: O(1)
 */
DistanceResult distance_circle_to_aabb(const Circle& circle, const AABB& aabb) noexcept;

/**
 * @brief Calculate distance between circle and OBB
 */
DistanceResult distance_circle_to_obb(const Circle& circle, const OBB& obb) noexcept;

/**
 * @brief Calculate distance between point and polygon
 * 
 * Educational Context:
 * Uses winding number algorithm to determine if point is inside polygon,
 * then calculates distance to closest edge if outside.
 * 
 * Time Complexity: O(n) where n is number of vertices
 * Space Complexity: O(1)
 */
DistanceResult distance_point_to_polygon(const Vec2& point, const Polygon& polygon) noexcept;

/**
 * @brief Calculate distance between two polygons using SAT
 * 
 * Educational Context:
 * Separating Axis Theorem implementation for convex polygons.
 * Tests all edge normals of both polygons as potential separating axes.
 * 
 * Time Complexity: O(n + m) where n, m are vertex counts
 * Space Complexity: O(1)
 */
DistanceResult distance_polygon_to_polygon(const Polygon& a, const Polygon& b) noexcept;

//=============================================================================
// Contact Manifold Generation
//=============================================================================

/**
 * @brief Generate contact manifold between two circles
 * 
 * Circle-circle collision produces exactly one contact point.
 */
std::optional<ContactManifold> generate_contact_manifold(const Circle& a, const Circle& b,
                                                       const Transform2D& transform_a, 
                                                       const Transform2D& transform_b) noexcept;

/**
 * @brief Generate contact manifold between two AABBs
 * 
 * AABB-AABB collision can produce 1-4 contact points depending on overlap configuration.
 */
std::optional<ContactManifold> generate_contact_manifold(const AABB& a, const AABB& b,
                                                       const Transform2D& transform_a, 
                                                       const Transform2D& transform_b) noexcept;

/**
 * @brief Generate contact manifold between circle and AABB
 */
std::optional<ContactManifold> generate_contact_manifold(const Circle& circle, const AABB& aabb,
                                                       const Transform2D& transform_circle, 
                                                       const Transform2D& transform_aabb) noexcept;

/**
 * @brief Generate contact manifold between two polygons
 * 
 * Polygon-polygon collision uses clipping to generate up to 4 contact points.
 * This is the most complex case in 2D collision detection.
 */
std::optional<ContactManifold> generate_contact_manifold(const Polygon& a, const Polygon& b,
                                                       const Transform2D& transform_a, 
                                                       const Transform2D& transform_b) noexcept;

//=============================================================================
// Separating Axis Theorem (SAT) Implementation
//=============================================================================

/**
 * @brief SAT collision detection for educational purposes
 * 
 * This namespace provides step-by-step SAT implementation with detailed
 * educational information about how the algorithm works.
 */
namespace sat {
    
    /**
     * @brief Project shape onto axis and return min/max values
     */
    struct Projection {
        f32 min;
        f32 max;
        
        /** @brief Check if projections overlap */
        bool overlaps(const Projection& other) const noexcept {
            return !(max < other.min || other.max < min);
        }
        
        /** @brief Calculate overlap amount (positive = overlapping) */
        f32 overlap_amount(const Projection& other) const noexcept {
            return std::min(max, other.max) - std::max(min, other.min);
        }
    };
    
    /**
     * @brief Project AABB onto axis
     */
    Projection project_aabb(const AABB& aabb, const Vec2& axis) noexcept;
    
    /**
     * @brief Project OBB onto axis
     */
    Projection project_obb(const OBB& obb, const Vec2& axis) noexcept;
    
    /**
     * @brief Project circle onto axis
     */
    Projection project_circle(const Circle& circle, const Vec2& axis) noexcept;
    
    /**
     * @brief Project polygon onto axis
     */
    Projection project_polygon(const Polygon& polygon, const Vec2& axis) noexcept;
    
    /**
     * @brief SAT test with educational step tracking
     */
    struct SATResult {
        bool is_separating;
        Vec2 separating_axis;
        f32 separation_distance;
        f32 min_overlap;
        Vec2 min_overlap_axis;
        
        /** @brief Educational debug information */
        struct DebugStep {
            Vec2 axis_tested;
            Projection projection_a;
            Projection projection_b;
            f32 overlap;
            bool is_separating;
            std::string explanation;
        };
        std::vector<DebugStep> debug_steps;
        
        SATResult() : is_separating(false), separation_distance(0.0f), min_overlap(std::numeric_limits<f32>::max()) {}
    };
    
    /**
     * @brief Perform SAT test between two shapes with debug information
     */
    template<typename ShapeA, typename ShapeB>
    SATResult test_separation(const ShapeA& a, const ShapeB& b, 
                            const std::vector<Vec2>& test_axes) noexcept;
    
    /**
     * @brief Get all potential separating axes for two OBBs
     */
    std::vector<Vec2> get_obb_axes(const OBB& a, const OBB& b) noexcept;
    
    /**
     * @brief Get all potential separating axes for two polygons
     */
    std::vector<Vec2> get_polygon_axes(const Polygon& a, const Polygon& b) noexcept;
}

//=============================================================================
// GJK Algorithm Implementation
//=============================================================================

/**
 * @brief Gilbert-Johnson-Keerthi (GJK) algorithm for advanced collision detection
 * 
 * Educational Context:
 * GJK is an advanced algorithm that works on the Minkowski difference of two shapes.
 * It's more complex than SAT but handles more general cases and provides distance information.
 * 
 * The algorithm constructs a simplex (triangle in 2D) that encloses the origin
 * in the Minkowski difference space.
 */
namespace gjk {
    
    /**
     * @brief Support function result
     */
    struct SupportPoint {
        Vec2 point;                // Support point in Minkowski difference
        Vec2 point_a;              // Support point on shape A
        Vec2 point_b;              // Support point on shape B
    };
    
    /**
     * @brief 2D simplex for GJK algorithm
     */
    struct Simplex {
        std::array<SupportPoint, 3> points;
        u32 count{0};
        
        void add_point(const SupportPoint& point) {
            if (count < 3) {
                points[count++] = point;
            }
        }
        
        void clear() { count = 0; }
        
        SupportPoint& operator[](u32 index) { return points[index]; }
        const SupportPoint& operator[](u32 index) const { return points[index]; }
    };
    
    /**
     * @brief GJK algorithm result
     */
    struct GJKResult {
        bool is_colliding;
        Simplex final_simplex;
        u32 iterations_used;
        
        /** @brief If not colliding, closest points */
        Vec2 closest_point_a;
        Vec2 closest_point_b;
        f32 distance;
        
        /** @brief Educational debug information */
        struct DebugIteration {
            Simplex simplex_state;
            Vec2 search_direction;
            SupportPoint new_support;
            std::string explanation;
        };
        std::vector<DebugIteration> debug_iterations;
        
        GJKResult() : is_colliding(false), iterations_used(0), distance(0.0f) {}
    };
    
    /**
     * @brief Get support point for shape in given direction
     */
    Vec2 get_support_point(const Circle& shape, const Vec2& direction) noexcept;
    Vec2 get_support_point(const AABB& shape, const Vec2& direction) noexcept;
    Vec2 get_support_point(const OBB& shape, const Vec2& direction) noexcept;
    Vec2 get_support_point(const Polygon& shape, const Vec2& direction) noexcept;
    
    /**
     * @brief Get support point in Minkowski difference A - B
     */
    template<typename ShapeA, typename ShapeB>
    SupportPoint get_minkowski_support(const ShapeA& a, const ShapeB& b, const Vec2& direction) noexcept;
    
    /**
     * @brief Perform GJK collision test
     */
    template<typename ShapeA, typename ShapeB>
    GJKResult test_collision(const ShapeA& a, const ShapeB& b) noexcept;
    
    /**
     * @brief Handle simplex evolution (core GJK logic)
     */
    bool handle_simplex(Simplex& simplex, Vec2& direction) noexcept;
    
    /**
     * @brief Calculate distance using GJK (when shapes don't intersect)
     */
    template<typename ShapeA, typename ShapeB>
    DistanceResult calculate_distance(const ShapeA& a, const ShapeB& b) noexcept;
}

//=============================================================================
// Raycast Operations
//=============================================================================

/**
 * @brief Raycast against circle
 * 
 * Educational Context:
 * Ray-circle intersection uses quadratic formula.
 * Parametric ray: P(t) = origin + t * direction
 * Circle equation: |P - center|² = radius²
 * Substitute and solve for t.
 * 
 * Time Complexity: O(1)
 */
RaycastResult raycast_circle(const Ray2D& ray, const Circle& circle) noexcept;

/**
 * @brief Raycast against AABB
 * 
 * Educational Context:
 * Uses slab method - treat AABB as intersection of 2 slabs.
 * Calculate entry/exit times for each slab, take intersection.
 * 
 * Time Complexity: O(1)
 */
RaycastResult raycast_aabb(const Ray2D& ray, const AABB& aabb) noexcept;

/**
 * @brief Raycast against OBB
 * 
 * Educational Context:
 * Transform ray to OBB's local space, then use AABB raycast.
 * More expensive than AABB due to coordinate transformation.
 * 
 * Time Complexity: O(1)
 */
RaycastResult raycast_obb(const Ray2D& ray, const OBB& obb) noexcept;

/**
 * @brief Raycast against convex polygon
 * 
 * Educational Context:
 * Test ray against each edge of polygon.
 * Use cross product to determine intersection.
 * 
 * Time Complexity: O(n) where n is number of edges
 */
RaycastResult raycast_polygon(const Ray2D& ray, const Polygon& polygon) noexcept;

//=============================================================================
// Utility Functions
//=============================================================================

/**
 * @brief Find closest point on line segment to given point
 */
Vec2 closest_point_on_segment(const Vec2& point, const Vec2& seg_start, const Vec2& seg_end) noexcept;

/**
 * @brief Calculate distance from point to line segment
 */
f32 distance_point_to_segment(const Vec2& point, const Vec2& seg_start, const Vec2& seg_end) noexcept;

/**
 * @brief Check if point is inside convex polygon using winding number
 */
bool point_in_polygon(const Vec2& point, const Polygon& polygon) noexcept;

/**
 * @brief Check if point is inside polygon using crossing number (ray casting)
 */
bool point_in_polygon_crossing(const Vec2& point, const Polygon& polygon) noexcept;

/**
 * @brief Calculate barycentric coordinates of point relative to triangle
 */
struct BarycentricCoords {
    f32 u, v, w;
    
    bool is_inside() const { return u >= 0.0f && v >= 0.0f && w >= 0.0f; }
};

BarycentricCoords calculate_barycentric_coords(const Vec2& point, 
                                             const Vec2& a, const Vec2& b, const Vec2& c) noexcept;

//=============================================================================
// Educational Debug and Visualization
//=============================================================================

/**
 * @brief Educational collision detection with step-by-step breakdown
 */
namespace debug {
    
    /**
     * @brief Detailed collision detection with educational output
     */
    struct CollisionDebugInfo {
        std::string algorithm_used;
        std::vector<std::string> step_descriptions;
        std::vector<f64> step_timings;
        DistanceResult final_result;
        f64 total_time_ns;
        
        /** @brief Visualization data for educational rendering */
        struct VisualizationData {
            std::vector<Vec2> test_axes;
            std::vector<std::pair<f32, f32>> projections_a;
            std::vector<std::pair<f32, f32>> projections_b;
            std::vector<Vec2> support_points;
            std::vector<Vec2> closest_points;
        } visualization;
    };
    
    /**
     * @brief Perform collision detection with full debug information
     */
    CollisionDebugInfo debug_collision_detection(const Circle& a, const Circle& b) noexcept;
    CollisionDebugInfo debug_collision_detection(const AABB& a, const AABB& b) noexcept;
    CollisionDebugInfo debug_collision_detection(const OBB& a, const OBB& b) noexcept;
    CollisionDebugInfo debug_collision_detection(const Polygon& a, const Polygon& b) noexcept;
    
    /**
     * @brief Generate educational explanation of algorithm
     */
    struct AlgorithmExplanation {
        std::string algorithm_name;
        std::string mathematical_basis;
        std::string time_complexity;
        std::string space_complexity;
        std::vector<std::string> key_concepts;
        std::vector<std::string> common_optimizations;
        std::string when_to_use;
    };
    
    AlgorithmExplanation explain_sat_algorithm() noexcept;
    AlgorithmExplanation explain_gjk_algorithm() noexcept;
    AlgorithmExplanation explain_circle_collision() noexcept;
    
    /**
     * @brief Performance comparison between algorithms
     */
    struct PerformanceComparison {
        std::string test_name;
        std::map<std::string, f64> algorithm_times;
        std::map<std::string, u32> algorithm_iterations;
        std::string fastest_algorithm;
        std::string most_accurate_algorithm;
        std::vector<std::string> recommendations;
    };
    
    PerformanceComparison compare_collision_algorithms(const CollisionShape& a, const CollisionShape& b) noexcept;
}

//=============================================================================
// Template Implementation Details
//=============================================================================

namespace detail {
    /**
     * @brief SFINAE helpers for shape type detection
     */
    template<typename T>
    struct is_circle : std::false_type {};
    
    template<>
    struct is_circle<Circle> : std::true_type {};
    
    template<typename T>
    struct is_aabb : std::false_type {};
    
    template<>
    struct is_aabb<AABB> : std::true_type {};
    
    template<typename T>
    struct is_polygon : std::false_type {};
    
    template<>
    struct is_polygon<Polygon> : std::true_type {};
    
    /**
     * @brief Generic collision dispatcher
     */
    template<typename ShapeA, typename ShapeB>
    DistanceResult dispatch_collision(const ShapeA& a, const ShapeB& b) noexcept;
}

} // namespace ecscope::physics::collision