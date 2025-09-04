#pragma once

/**
 * @file physics/collision3d_algorithms.hpp
 * @brief Advanced 3D Collision Detection Algorithms (SAT, GJK/EPA, MPR)
 * 
 * This header implements state-of-the-art 3D collision detection algorithms with emphasis
 * on educational clarity and performance. It provides comprehensive implementations of:
 * 
 * - 3D Separating Axis Theorem (SAT) for convex polyhedra
 * - Gilbert-Johnson-Keerthi (GJK) algorithm for general convex shapes
 * - Expanding Polytope Algorithm (EPA) for penetration depth calculation
 * - Minkowski Portal Refinement (MPR) as alternative to GJK
 * - Specialized algorithms for primitive pairs (sphere-sphere, box-box, etc.)
 * - Educational step-by-step visualization and debugging
 * 
 * Educational Philosophy:
 * Each algorithm includes detailed mathematical explanations, complexity analysis,
 * visualization support, and comparisons with 2D counterparts to help students
 * understand the increased complexity of 3D collision detection.
 * 
 * Performance Considerations:
 * - Early termination optimizations
 * - SIMD-optimized vector operations
 * - Cache-friendly memory access patterns
 * - Hierarchical collision detection integration
 * 
 * @author ECScope Educational ECS Framework - 3D Collision Algorithms
 * @date 2025
 */

#include "collision3d.hpp"
#include "math3d.hpp"
#include "simd_math3d.hpp"
#include <vector>
#include <array>
#include <optional>
#include <chrono>

namespace ecscope::physics::collision3d {

//=============================================================================
// 3D Separating Axis Theorem (SAT) Implementation
//=============================================================================

/**
 * @brief 3D SAT implementation for educational purposes
 * 
 * Educational Context:
 * The Separating Axis Theorem in 3D states that two convex objects are separated
 * if and only if there exists a separating axis (a direction) such that the
 * projections of the objects onto this axis do not overlap.
 * 
 * For two convex polyhedra, the potential separating axes are:
 * 1. Face normals of object A
 * 2. Face normals of object B  
 * 3. Cross products of edges from A with edges from B
 * 
 * The number of axes to test can be significant: for two boxes, we test 15 axes
 * (3 + 3 + 9), while for arbitrary convex hulls it can be much larger.
 */
namespace sat3d {
    
    /**
     * @brief 3D projection of a shape onto an axis
     */
    struct Projection3D {
        f32 min;
        f32 max;
        
        /** @brief Check if projections overlap */
        bool overlaps(const Projection3D& other) const noexcept {
            return !(max < other.min || other.max < min);
        }
        
        /** @brief Calculate overlap amount (positive = overlapping) */
        f32 overlap_amount(const Projection3D& other) const noexcept {
            return std::min(max, other.max) - std::max(min, other.min);
        }
        
        /** @brief Get separation distance (negative = overlapping) */
        f32 separation_distance(const Projection3D& other) const noexcept {
            if (overlaps(other)) {
                return -overlap_amount(other);
            }
            return std::max(min - other.max, other.min - max);
        }
    };
    
    // Shape projection functions
    Projection3D project_sphere(const Sphere& sphere, const Vec3& axis) noexcept;
    Projection3D project_aabb(const AABB3D& aabb, const Vec3& axis) noexcept;
    Projection3D project_obb(const OBB3D& obb, const Vec3& axis) noexcept;
    Projection3D project_capsule(const Capsule& capsule, const Vec3& axis) noexcept;
    Projection3D project_convex_hull(const ConvexHull& hull, const Vec3& axis) noexcept;
    
    /**
     * @brief 3D SAT test result with educational information
     */
    struct SATResult3D {
        bool is_separating;
        Vec3 separating_axis;
        f32 separation_distance;
        f32 min_overlap;
        Vec3 min_overlap_axis;
        
        /** @brief Educational debug information */
        struct DebugStep {
            Vec3 axis_tested;
            Projection3D projection_a;
            Projection3D projection_b;
            f32 overlap;
            bool is_separating;
            std::string explanation;
            std::string axis_source;  // "Face A", "Face B", "Edge Cross Product"
        };
        std::vector<DebugStep> debug_steps;
        
        f64 total_computation_time_ns;
        u32 total_axes_tested;
        u32 early_exit_at_axis;  // Which axis caused early exit (if any)
        
        SATResult3D() : is_separating(false), separation_distance(0.0f), 
                       min_overlap(std::numeric_limits<f32>::max()), 
                       total_computation_time_ns(0.0), total_axes_tested(0), early_exit_at_axis(0) {}
    };
    
    /**
     * @brief Perform 3D SAT test between two OBBs
     * 
     * Educational Context:
     * OBB-OBB collision is the classic 3D SAT test case. We test:
     * - 3 face normals of OBB A
     * - 3 face normals of OBB B
     * - 9 cross products of edge pairs (3x3)
     * 
     * Time Complexity: O(1) but with high constant factor (15 axis tests)
     * Space Complexity: O(1)
     */
    SATResult3D test_obb_vs_obb(const OBB3D& obb_a, const OBB3D& obb_b) noexcept;
    
    /**
     * @brief Perform 3D SAT test between two convex hulls
     * 
     * Educational Context:
     * This is the most general SAT case. We test:
     * - All face normals of hull A
     * - All face normals of hull B
     * - All cross products of edge pairs
     * 
     * Time Complexity: O(n*m + e_a*e_b) where n,m are face counts and e_a,e_b are edge counts
     * Space Complexity: O(e_a*e_b) for storing edge cross products
     */
    SATResult3D test_convex_hull_vs_convex_hull(const ConvexHull& hull_a, const ConvexHull& hull_b) noexcept;
    
    /**
     * @brief Get all potential separating axes for two OBBs
     */
    std::vector<Vec3> get_obb_separating_axes(const OBB3D& obb_a, const OBB3D& obb_b) noexcept;
    
    /**
     * @brief Get all potential separating axes for two convex hulls
     */
    std::vector<Vec3> get_convex_hull_separating_axes(const ConvexHull& hull_a, const ConvexHull& hull_b) noexcept;
    
    /**
     * @brief Optimized SAT test with early exit and axis caching
     */
    SATResult3D test_optimized_sat(const ConvexHull& hull_a, const ConvexHull& hull_b,
                                  const std::vector<Vec3>& cached_axes = {}) noexcept;
}

//=============================================================================
// 3D Gilbert-Johnson-Keerthi (GJK) Algorithm Implementation
//=============================================================================

/**
 * @brief 3D GJK algorithm for general convex collision detection
 * 
 * Educational Context:
 * GJK is a powerful algorithm that works on the Minkowski difference of two shapes.
 * In 3D, the algorithm constructs a tetrahedron (4-simplex) that attempts to
 * enclose the origin in the Minkowski difference space. If successful, the shapes
 * are intersecting.
 * 
 * Key Advantages over SAT:
 * - Works with any convex shape that has a support function
 * - No need to enumerate all potential separating axes
 * - Naturally provides closest points when shapes don't intersect
 * 
 * Key Challenges in 3D:
 * - More complex simplex handling (tetrahedron vs triangle in 2D)
 * - Multiple simplex evolution cases
 * - Numerical precision issues in degenerate cases
 */
namespace gjk3d {
    
    /**
     * @brief 3D Support point in Minkowski difference
     */
    struct SupportPoint3D {
        Vec3 point;                // Support point in Minkowski difference (A - B)
        Vec3 point_a;              // Support point on shape A
        Vec3 point_b;              // Support point on shape B
    };
    
    /**
     * @brief 3D Simplex for GJK algorithm (up to 4 points in 3D)
     */
    struct Simplex3D {
        std::array<SupportPoint3D, 4> points;
        u32 count{0};
        
        void add_point(const SupportPoint3D& point) {
            if (count < 4) {
                points[count++] = point;
            }
        }
        
        void clear() { count = 0; }
        
        SupportPoint3D& operator[](u32 index) { return points[index]; }
        const SupportPoint3D& operator[](u32 index) const { return points[index]; }
        
        // Get the most recently added point
        const SupportPoint3D& last() const { return points[count - 1]; }
        
        // Remove a point and shift others
        void remove_point(u32 index) {
            if (index < count) {
                for (u32 i = index; i < count - 1; ++i) {
                    points[i] = points[i + 1];
                }
                count--;
            }
        }
    };
    
    /**
     * @brief 3D GJK algorithm result
     */
    struct GJKResult3D {
        bool is_intersecting;
        Simplex3D final_simplex;
        u32 iterations_used;
        u32 max_iterations{32};      // Reasonable limit for 3D GJK
        
        /** @brief If not intersecting, closest points and distance */
        Vec3 closest_point_a;
        Vec3 closest_point_b;
        f32 distance;
        Vec3 separating_direction;
        
        /** @brief Educational debug information */
        struct DebugIteration {
            Simplex3D simplex_state;
            Vec3 search_direction;
            SupportPoint3D new_support;
            std::string simplex_type;    // "Point", "Line", "Triangle", "Tetrahedron"
            std::string evolution_type;  // How simplex was evolved
            std::string explanation;
            bool converged;
            f32 distance_to_origin;
        };
        std::vector<DebugIteration> debug_iterations;
        
        f64 total_computation_time_ns;
        std::string termination_reason;
        
        GJKResult3D() : is_intersecting(false), iterations_used(0), distance(0.0f),
                       total_computation_time_ns(0.0) {}
    };
    
    /**
     * @brief Get support point for 3D shapes in given direction
     */
    Vec3 get_support_point_3d(const Sphere& shape, const Vec3& direction) noexcept;
    Vec3 get_support_point_3d(const AABB3D& shape, const Vec3& direction) noexcept;
    Vec3 get_support_point_3d(const OBB3D& shape, const Vec3& direction) noexcept;
    Vec3 get_support_point_3d(const Capsule& shape, const Vec3& direction) noexcept;
    Vec3 get_support_point_3d(const ConvexHull& shape, const Vec3& direction) noexcept;
    
    /**
     * @brief Get support point in Minkowski difference A - B
     */
    template<typename ShapeA, typename ShapeB>
    SupportPoint3D get_minkowski_support_3d(const ShapeA& a, const ShapeB& b, const Vec3& direction) noexcept {
        Vec3 support_a = get_support_point_3d(a, direction);
        Vec3 support_b = get_support_point_3d(b, -direction);
        return SupportPoint3D{support_a - support_b, support_a, support_b};
    }
    
    /**
     * @brief Perform 3D GJK collision test
     */
    template<typename ShapeA, typename ShapeB>
    GJKResult3D test_collision_3d(const ShapeA& a, const ShapeB& b) noexcept;
    
    /**
     * @brief Handle 3D simplex evolution (core GJK logic)
     * 
     * Educational Context:
     * This is the heart of the GJK algorithm. In 3D, we have four cases:
     * 1. Point simplex (1 vertex): continue search
     * 2. Line simplex (2 vertices): project origin onto line
     * 3. Triangle simplex (3 vertices): check if origin projects inside triangle
     * 4. Tetrahedron simplex (4 vertices): check if origin is inside tetrahedron
     */
    bool handle_simplex_3d(Simplex3D& simplex, Vec3& direction) noexcept;
    
    // Individual simplex handling functions
    bool handle_point_simplex(Simplex3D& simplex, Vec3& direction) noexcept;
    bool handle_line_simplex(Simplex3D& simplex, Vec3& direction) noexcept;
    bool handle_triangle_simplex(Simplex3D& simplex, Vec3& direction) noexcept;
    bool handle_tetrahedron_simplex(Simplex3D& simplex, Vec3& direction) noexcept;
    
    /**
     * @brief Calculate distance using GJK when shapes don't intersect
     */
    template<typename ShapeA, typename ShapeB>
    DistanceResult3D calculate_distance_gjk_3d(const ShapeA& a, const ShapeB& b) noexcept;
    
    /**
     * @brief Get closest points on shapes from final GJK simplex
     */
    std::pair<Vec3, Vec3> get_closest_points_from_simplex(const Simplex3D& simplex) noexcept;
}

//=============================================================================
// 3D Expanding Polytope Algorithm (EPA) Implementation
//=============================================================================

/**
 * @brief 3D EPA algorithm for penetration depth calculation
 * 
 * Educational Context:
 * EPA is used when GJK determines that two shapes are intersecting, but we need
 * to know by how much. EPA expands the final GJK simplex (tetrahedron) toward
 * the origin until it finds the closest face to the origin, which gives us
 * the penetration depth and contact normal.
 * 
 * Key Concepts:
 * - Start with GJK's final tetrahedron
 * - Iteratively add support points in the direction of closest face normal
 * - Maintain convex polytope structure
 * - Terminate when expansion distance is below threshold
 */
namespace epa3d {
    
    /**
     * @brief EPA face (triangle) in the expanding polytope
     */
    struct EPAFace {
        std::array<u32, 3> vertex_indices; // Indices into vertex list
        Vec3 normal;                        // Outward normal
        f32 distance_to_origin;             // Distance from origin to face
        
        EPAFace() = default;
        EPAFace(u32 v0, u32 v1, u32 v2) : vertex_indices{v0, v1, v2} {}
        
        bool operator<(const EPAFace& other) const {
            return distance_to_origin < other.distance_to_origin;
        }
    };
    
    /**
     * @brief EPA edge for polytope maintenance
     */
    struct EPAEdge {
        u32 vertex_indices[2];
        
        EPAEdge(u32 v0, u32 v1) : vertex_indices{v0, v1} {}
        
        bool operator==(const EPAEdge& other) const {
            return (vertex_indices[0] == other.vertex_indices[0] && 
                    vertex_indices[1] == other.vertex_indices[1]) ||
                   (vertex_indices[0] == other.vertex_indices[1] && 
                    vertex_indices[1] == other.vertex_indices[0]);
        }
    };
    
    /**
     * @brief 3D EPA algorithm result
     */
    struct EPAResult3D {
        bool success;
        Vec3 penetration_normal;     // Normal from A to B
        f32 penetration_depth;       // Penetration distance
        Vec3 contact_point_a;        // Contact point on shape A
        Vec3 contact_point_b;        // Contact point on shape B
        
        /** @brief Educational debug information */
        struct DebugIteration {
            std::vector<EPAFace> polytope_faces;
            std::vector<SupportPoint3D> polytope_vertices;
            EPAFace closest_face;
            Vec3 search_direction;
            SupportPoint3D new_support;
            f32 expansion_distance;
            std::string explanation;
        };
        std::vector<DebugIteration> debug_iterations;
        
        u32 iterations_used;
        u32 max_iterations{64};      // Reasonable limit for EPA
        f64 total_computation_time_ns;
        std::string termination_reason;
        
        EPAResult3D() : success(false), penetration_depth(0.0f), iterations_used(0),
                       total_computation_time_ns(0.0) {}
    };
    
    /**
     * @brief Run EPA algorithm starting from GJK's final simplex
     */
    template<typename ShapeA, typename ShapeB>
    EPAResult3D calculate_penetration_epa_3d(const ShapeA& a, const ShapeB& b, 
                                            const gjk3d::Simplex3D& initial_simplex) noexcept;
    
    /**
     * @brief Initialize polytope from GJK tetrahedron
     */
    void initialize_polytope_from_simplex(const gjk3d::Simplex3D& simplex,
                                        std::vector<gjk3d::SupportPoint3D>& vertices,
                                        std::vector<EPAFace>& faces) noexcept;
    
    /**
     * @brief Find closest face to origin in polytope
     */
    EPAFace find_closest_face(const std::vector<gjk3d::SupportPoint3D>& vertices,
                             const std::vector<EPAFace>& faces) noexcept;
    
    /**
     * @brief Expand polytope by adding new vertex
     */
    void expand_polytope(std::vector<gjk3d::SupportPoint3D>& vertices,
                        std::vector<EPAFace>& faces,
                        const gjk3d::SupportPoint3D& new_vertex,
                        const EPAFace& closest_face) noexcept;
    
    /**
     * @brief Calculate face normal and distance to origin
     */
    void calculate_face_properties(EPAFace& face,
                                 const std::vector<gjk3d::SupportPoint3D>& vertices) noexcept;
}

//=============================================================================
// Specialized 3D Primitive-Primitive Collision Detection
//=============================================================================

/**
 * @brief Optimized algorithms for specific 3D primitive pairs
 * 
 * Educational Context:
 * While GJK/EPA can handle any convex shapes, specialized algorithms for
 * common primitive pairs are much faster. These serve as both performance
 * optimizations and educational examples of geometric reasoning.
 */
namespace primitives3d {
    
    // Sphere-sphere collision (simplest 3D case)
    DistanceResult3D distance_sphere_to_sphere(const Sphere& a, const Sphere& b) noexcept;
    
    // Sphere-AABB collision  
    DistanceResult3D distance_sphere_to_aabb(const Sphere& sphere, const AABB3D& aabb) noexcept;
    
    // Sphere-OBB collision
    DistanceResult3D distance_sphere_to_obb(const Sphere& sphere, const OBB3D& obb) noexcept;
    
    // AABB-AABB collision (3D extension of 2D algorithm)
    DistanceResult3D distance_aabb_to_aabb(const AABB3D& a, const AABB3D& b) noexcept;
    
    // OBB-OBB collision using SAT
    DistanceResult3D distance_obb_to_obb(const OBB3D& a, const OBB3D& b) noexcept;
    
    // Capsule-capsule collision (complex but important for character physics)
    DistanceResult3D distance_capsule_to_capsule(const Capsule& a, const Capsule& b) noexcept;
    
    // Point-in-shape tests
    bool point_in_sphere(const Vec3& point, const Sphere& sphere) noexcept;
    bool point_in_aabb(const Vec3& point, const AABB3D& aabb) noexcept;
    bool point_in_obb(const Vec3& point, const OBB3D& obb) noexcept;
    bool point_in_capsule(const Vec3& point, const Capsule& capsule) noexcept;
    bool point_in_convex_hull(const Vec3& point, const ConvexHull& hull) noexcept;
}

//=============================================================================
// 3D Raycast Operations
//=============================================================================

/**
 * @brief 3D raycasting algorithms for all primitive types
 */
namespace raycast3d {
    
    /**
     * @brief Raycast against 3D sphere
     * 
     * Educational Context:
     * Ray-sphere intersection uses the quadratic formula, similar to 2D
     * ray-circle intersection but in 3D space.
     * 
     * Time Complexity: O(1)
     */
    RaycastResult3D raycast_sphere(const Ray3D& ray, const Sphere& sphere) noexcept;
    
    /**
     * @brief Raycast against 3D AABB
     * 
     * Educational Context:
     * Uses the slab method - treat AABB as intersection of 3 slabs.
     * Calculate entry/exit times for each slab, take intersection.
     * 
     * Time Complexity: O(1)
     */
    RaycastResult3D raycast_aabb(const Ray3D& ray, const AABB3D& aabb) noexcept;
    
    /**
     * @brief Raycast against 3D OBB
     * 
     * Educational Context:
     * Transform ray to OBB's local space, then use AABB raycast.
     * More expensive than AABB due to coordinate transformation.
     * 
     * Time Complexity: O(1)
     */
    RaycastResult3D raycast_obb(const Ray3D& ray, const OBB3D& obb) noexcept;
    
    /**
     * @brief Raycast against 3D capsule
     * 
     * Educational Context:
     * Capsule raycast involves ray-cylinder and ray-sphere intersections.
     * More complex than basic primitives but important for character physics.
     * 
     * Time Complexity: O(1)
     */
    RaycastResult3D raycast_capsule(const Ray3D& ray, const Capsule& capsule) noexcept;
    
    /**
     * @brief Raycast against convex hull
     * 
     * Educational Context:
     * Test ray against each face of the convex hull.
     * For complex hulls, this can be expensive.
     * 
     * Time Complexity: O(n) where n is number of faces
     */
    RaycastResult3D raycast_convex_hull(const Ray3D& ray, const ConvexHull& hull) noexcept;
}

//=============================================================================
// Educational Debug and Visualization Support
//=============================================================================

/**
 * @brief Educational collision detection with comprehensive debugging
 */
namespace debug3d {
    
    /**
     * @brief Comprehensive 3D collision debugging information
     */
    struct CollisionDebugInfo3D {
        std::string algorithm_used;
        std::vector<std::string> step_descriptions;
        std::vector<f64> step_timings;
        DistanceResult3D final_result;
        f64 total_time_ns;
        
        /** @brief 3D visualization data */
        struct VisualizationData3D {
            std::vector<Vec3> test_axes;
            std::vector<std::pair<f32, f32>> projections_a;
            std::vector<std::pair<f32, f32>> projections_b;
            std::vector<Vec3> support_points;
            std::vector<Vec3> closest_points;
            std::vector<std::vector<Vec3>> simplex_evolution; // For GJK visualization
            std::vector<std::vector<Vec3>> polytope_faces;    // For EPA visualization
        } visualization;
        
        // Performance comparison data
        struct PerformanceComparison {
            f64 sat_time_ns;
            f64 gjk_time_ns;
            f64 specialized_time_ns;
            std::string fastest_algorithm;
            f32 accuracy_comparison;
        } performance;
    };
    
    /**
     * @brief Debug collision detection with full step breakdown
     */
    CollisionDebugInfo3D debug_collision_3d(const Sphere& a, const Sphere& b) noexcept;
    CollisionDebugInfo3D debug_collision_3d(const AABB3D& a, const AABB3D& b) noexcept;
    CollisionDebugInfo3D debug_collision_3d(const OBB3D& a, const OBB3D& b) noexcept;
    CollisionDebugInfo3D debug_collision_3d(const ConvexHull& a, const ConvexHull& b) noexcept;
    
    /**
     * @brief Compare different collision detection algorithms
     */
    struct AlgorithmComparison3D {
        std::string test_case_description;
        std::map<std::string, f64> algorithm_times;
        std::map<std::string, f32> algorithm_accuracy;
        std::map<std::string, u32> algorithm_iterations;
        std::string recommended_algorithm;
        std::vector<std::string> educational_insights;
    };
    
    AlgorithmComparison3D compare_collision_algorithms_3d(const ConvexHull& a, const ConvexHull& b) noexcept;
    
    /**
     * @brief Educational explanations of 3D collision algorithms
     */
    struct AlgorithmExplanation3D {
        std::string algorithm_name;
        std::string mathematical_basis;
        std::string time_complexity;
        std::string space_complexity;
        std::vector<std::string> key_concepts;
        std::vector<std::string> advantages;
        std::vector<std::string> disadvantages;
        std::string when_to_use;
        std::string comparison_to_2d;
    };
    
    AlgorithmExplanation3D explain_sat_3d() noexcept;
    AlgorithmExplanation3D explain_gjk_3d() noexcept;
    AlgorithmExplanation3D explain_epa_3d() noexcept;
    
    /**
     * @brief Generate educational visualization data for rendering
     */
    struct RenderingData3D {
        std::vector<Vec3> axes_to_draw;
        std::vector<std::pair<Vec3, Vec3>> projection_lines;
        std::vector<Vec3> support_points;
        std::vector<std::array<Vec3, 3>> simplex_triangles;
        std::vector<std::array<Vec3, 4>> tetrahedra;
        std::vector<Vec3> contact_points;
        std::vector<std::pair<Vec3, Vec3>> contact_normals;
    };
    
    RenderingData3D generate_visualization_data_3d(const CollisionDebugInfo3D& debug_info) noexcept;
}

} // namespace ecscope::physics::collision3d