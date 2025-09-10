#include "ecscope/physics/physics_math.hpp"
#include <algorithm>
#include <numeric>
#include <chrono>
#include <cstring>

/**
 * @file physics/math.cpp
 * @brief Implementation of comprehensive 2D/3D Physics Mathematics Foundation
 * 
 * This implementation provides high-performance mathematical operations for 2D/3D physics
 * while maintaining production-ready performance and stability.
 */

namespace ecscope::physics {

//=============================================================================
// Extended Vector Mathematics Implementation
//=============================================================================

namespace vec2 {
    
    Vec2 slerp(const Vec2& a, const Vec2& b, f32 t) noexcept {
        // Spherical linear interpolation for 2D vectors
        // More complex than lerp but provides constant angular velocity
        
        // Normalize vectors for angle calculation
        Vec2 na = a.normalized();
        Vec2 nb = b.normalized();
        
        // Calculate angle between vectors
        f32 dot = std::clamp(na.dot(nb), -1.0f, 1.0f);
        f32 theta = std::acos(dot);
        
        // If vectors are nearly parallel, fall back to lerp
        if (std::abs(theta) < constants::EPSILON) {
            return lerp(a, b, t);
        }
        
        // Spherical interpolation formula
        f32 sin_theta = std::sin(theta);
        f32 factor_a = std::sin((1.0f - t) * theta) / sin_theta;
        f32 factor_b = std::sin(t * theta) / sin_theta;
        
        // Interpolate magnitudes separately
        f32 mag_a = a.length();
        f32 mag_b = b.length();
        f32 interpolated_mag = mag_a + t * (mag_b - mag_a);
        
        return (na * factor_a + nb * factor_b) * interpolated_mag;
    }
    
    f32 angle_between(const Vec2& a, const Vec2& b) noexcept {
        // Calculate angle between two vectors using atan2 for proper quadrant handling
        // This method is more numerically stable than acos(dot/(|a||b|))
        
        f32 cross_prod = cross(a, b);
        f32 dot_prod = a.dot(b);
        
        return std::atan2(cross_prod, dot_prod);
    }

#ifdef ECSCOPE_PHYSICS_USE_SSE2
    void dot_product_x2(const Vec2 a[2], const Vec2 b[2], f32 results[2]) noexcept {
        // Load vectors into SSE registers
        __m128 a_vec = _mm_setr_ps(a[0].x, a[0].y, a[1].x, a[1].y);
        __m128 b_vec = _mm_setr_ps(b[0].x, b[0].y, b[1].x, b[1].y);
        
        // Multiply components
        __m128 mul = _mm_mul_ps(a_vec, b_vec);
        
        // Horizontal add to compute dot products
        __m128 hadd1 = _mm_hadd_ps(mul, mul);
        
        // Store results
        _mm_storel_pi((__m64*)results, hadd1);
    }
    
    void add_vectors_x4(const Vec2 a[4], const Vec2 b[4], Vec2 results[4]) noexcept {
        // Process 4 vector additions simultaneously using SSE2
        for (usize i = 0; i < 4; i += 2) {
            __m128 a_vec = _mm_setr_ps(a[i].x, a[i].y, a[i+1].x, a[i+1].y);
            __m128 b_vec = _mm_setr_ps(b[i].x, b[i].y, b[i+1].x, b[i+1].y);
            __m128 result = _mm_add_ps(a_vec, b_vec);
            
            f32 temp[4];
            _mm_storeu_ps(temp, result);
            results[i] = Vec2{temp[0], temp[1]};
            results[i+1] = Vec2{temp[2], temp[3]};
        }
    }
#endif
    
} // namespace vec2

//=============================================================================
// Geometric Primitives Implementation
//=============================================================================

AABB Circle::get_aabb() const noexcept {
    return AABB{center - Vec2{radius, radius}, center + Vec2{radius, radius}};
}

Polygon Polygon::create_regular(Vec2 center, f32 radius, u32 sides) noexcept {
    sides = std::clamp(sides, 3u, MAX_VERTICES);
    
    Polygon result;
    result.vertex_count = sides;
    
    f32 angle_step = constants::TWO_PI_F / static_cast<f32>(sides);
    
    for (u32 i = 0; i < sides; ++i) {
        f32 angle = static_cast<f32>(i) * angle_step;
        result.vertices[i] = center + Vec2{
            radius * std::cos(angle),
            radius * std::sin(angle)
        };
    }
    
    result.properties_dirty = true;
    return result;
}

bool Polygon::contains(const Vec2& point) const noexcept {
    // Use winding number algorithm for robust point-in-polygon test
    return utils::point_in_polygon_winding(point, get_vertices());
}

bool Polygon::is_convex() const noexcept {
    if (vertex_count < 3) return false;
    
    bool sign_positive = false;
    bool sign_negative = false;
    
    for (u32 i = 0; i < vertex_count; ++i) {
        Vec2 v1 = vertices[i];
        Vec2 v2 = vertices[(i + 1) % vertex_count];
        Vec2 v3 = vertices[(i + 2) % vertex_count];
        
        // Calculate cross product of consecutive edges
        f32 cross_prod = vec2::cross(v2 - v1, v3 - v2);
        
        if (cross_prod > constants::EPSILON) {
            sign_positive = true;
        } else if (cross_prod < -constants::EPSILON) {
            sign_negative = true;
        }
        
        // If we have both positive and negative signs, polygon is concave
        if (sign_positive && sign_negative) {
            return false;
        }
    }
    
    return true;
}

bool Polygon::is_counter_clockwise() const noexcept {
    if (vertex_count < 3) return false;
    
    // Calculate signed area using shoelace formula
    f32 signed_area = 0.0f;
    for (u32 i = 0; i < vertex_count; ++i) {
        u32 next = (i + 1) % vertex_count;
        signed_area += (vertices[next].x - vertices[i].x) * (vertices[next].y + vertices[i].y);
    }
    
    return signed_area > 0.0f;  // Positive area means counter-clockwise
}

void Polygon::ensure_counter_clockwise() noexcept {
    if (!is_counter_clockwise()) {
        // Reverse vertex order
        std::reverse(vertices.begin(), vertices.begin() + vertex_count);
        properties_dirty = true;
    }
}

void Polygon::update_properties() const noexcept {
    if (vertex_count < 3) {
        centroid = Vec2::zero();
        area_cached = 0.0f;
        properties_dirty = false;
        return;
    }
    
    // Calculate centroid and area using shoelace formula
    f32 area_sum = 0.0f;
    Vec2 centroid_sum = Vec2::zero();
    
    for (u32 i = 0; i < vertex_count; ++i) {
        u32 next = (i + 1) % vertex_count;
        Vec2 v1 = vertices[i];
        Vec2 v2 = vertices[next];
        
        f32 cross = vec2::cross(v1, v2);
        area_sum += cross;
        centroid_sum += (v1 + v2) * cross;
    }
    
    area_cached = std::abs(area_sum) * 0.5f;
    
    if (area_cached > constants::EPSILON) {
        centroid = centroid_sum / (6.0f * area_sum);
    } else {
        // Fallback to arithmetic mean for degenerate polygons
        centroid = std::accumulate(vertices.begin(), vertices.begin() + vertex_count, Vec2::zero()) 
                  / static_cast<f32>(vertex_count);
    }
    
    properties_dirty = false;
}

//=============================================================================
// Collision and Distance Mathematics Implementation
//=============================================================================

namespace collision {
    
    f32 distance_point_to_line(const Vec2& point, const Vec2& line_start, const Vec2& line_end) noexcept {
        // Calculate perpendicular distance from point to infinite line
        Vec2 line_vec = line_end - line_start;
        f32 line_length_sq = line_vec.length_squared();
        
        if (line_length_sq < constants::EPSILON) {
            // Degenerate line case
            return vec2::distance(point, line_start);
        }
        
        Vec2 point_vec = point - line_start;
        f32 cross_prod = std::abs(vec2::cross(point_vec, line_vec));
        
        return cross_prod / std::sqrt(line_length_sq);
    }
    
    f32 distance_point_to_segment(const Vec2& point, const Vec2& seg_start, const Vec2& seg_end) noexcept {
        Vec2 segment = seg_end - seg_start;
        f32 segment_length_sq = segment.length_squared();
        
        if (segment_length_sq < constants::EPSILON) {
            // Degenerate segment case
            return vec2::distance(point, seg_start);
        }
        
        Vec2 to_point = point - seg_start;
        f32 projection_param = to_point.dot(segment) / segment_length_sq;
        
        if (projection_param <= 0.0f) {
            // Closest point is start of segment
            return vec2::distance(point, seg_start);
        } else if (projection_param >= 1.0f) {
            // Closest point is end of segment
            return vec2::distance(point, seg_end);
        } else {
            // Closest point is on the segment
            Vec2 closest = seg_start + segment * projection_param;
            return vec2::distance(point, closest);
        }
    }
    
    DistanceResult distance_circle_to_circle(const Circle& a, const Circle& b) noexcept {
        DistanceResult result;
        
        Vec2 center_diff = b.center - a.center;
        f32 center_distance = center_diff.length();
        f32 combined_radius = a.radius + b.radius;
        
        result.distance = center_distance - combined_radius;
        result.is_overlapping = result.distance < 0.0f;
        
        if (center_distance > constants::EPSILON) {
            // Normal from A to B
            result.normal = center_diff / center_distance;
            
            // Closest points on circle surfaces
            result.point_a = a.center + result.normal * a.radius;
            result.point_b = b.center - result.normal * b.radius;
        } else {
            // Circles are concentric - use arbitrary direction
            result.normal = Vec2{1.0f, 0.0f};
            result.point_a = a.center + result.normal * a.radius;
            result.point_b = b.center - result.normal * b.radius;
        }
        
        return result;
    }
    
    DistanceResult distance_aabb_to_aabb(const AABB& a, const AABB& b) noexcept {
        DistanceResult result;
        
        // Calculate overlap on each axis
        f32 x_overlap = std::min(a.max.x, b.max.x) - std::max(a.min.x, b.min.x);
        f32 y_overlap = std::min(a.max.y, b.max.y) - std::max(a.min.y, b.min.y);
        
        result.is_overlapping = (x_overlap > 0.0f && y_overlap > 0.0f);
        
        if (result.is_overlapping) {
            // Boxes are overlapping - find minimum separation
            if (x_overlap < y_overlap) {
                // Separate along X axis
                result.distance = -x_overlap;
                result.normal = (a.center().x < b.center().x) ? Vec2{-1.0f, 0.0f} : Vec2{1.0f, 0.0f};
            } else {
                // Separate along Y axis
                result.distance = -y_overlap;
                result.normal = (a.center().y < b.center().y) ? Vec2{0.0f, -1.0f} : Vec2{0.0f, 1.0f};
            }
        } else {
            // Boxes are separated - find closest points
            Vec2 a_center = a.center();
            Vec2 b_center = b.center();
            
            Vec2 closest_a = a.closest_point(b_center);
            Vec2 closest_b = b.closest_point(a_center);
            
            Vec2 separation = closest_b - closest_a;
            result.distance = separation.length();
            
            if (result.distance > constants::EPSILON) {
                result.normal = separation / result.distance;
            } else {
                result.normal = Vec2{1.0f, 0.0f};
            }
            
            result.point_a = closest_a;
            result.point_b = closest_b;
        }
        
        return result;
    }
    
    DistanceResult distance_obb_to_obb(const OBB& a, const OBB& b) noexcept {
        // OBB to OBB distance using Separating Axis Theorem (SAT)
        DistanceResult result;
        
        // Get the axes to test (4 axes total: 2 from each OBB)
        std::array<Vec2, 4> axes = {
            a.get_axis_x(), a.get_axis_y(),
            b.get_axis_x(), b.get_axis_y()
        };
        
        f32 min_overlap = std::numeric_limits<f32>::max();
        Vec2 separation_axis;
        bool is_overlapping = true;
        
        for (const Vec2& axis : axes) {
            // Project both OBBs onto this axis
            auto [a_min, a_max] = a.project_onto_axis(axis);
            auto [b_min, b_max] = b.project_onto_axis(axis);
            
            // Calculate overlap
            f32 overlap = std::min(a_max, b_max) - std::max(a_min, b_min);
            
            if (overlap <= 0.0f) {
                // Separating axis found - boxes don't overlap
                is_overlapping = false;
                f32 separation = -overlap;
                
                if (separation < min_overlap) {
                    min_overlap = separation;
                    separation_axis = axis;
                }
            } else {
                // Overlapping on this axis
                if (overlap < min_overlap) {
                    min_overlap = overlap;
                    separation_axis = axis;
                }
            }
        }
        
        result.is_overlapping = is_overlapping;
        
        if (is_overlapping) {
            result.distance = -min_overlap;
            result.normal = separation_axis;
            
            // Ensure normal points from A to B
            Vec2 center_diff = b.center - a.center;
            if (center_diff.dot(result.normal) < 0.0f) {
                result.normal = result.normal * -1.0f;
            }
        } else {
            result.distance = min_overlap;
            result.normal = separation_axis;
            
            // Calculate closest points (simplified)
            result.point_a = a.center;
            result.point_b = b.center;
        }
        
        return result;
    }
    
    DistanceResult distance_circle_to_aabb(const Circle& circle, const AABB& aabb) noexcept {
        DistanceResult result;
        
        // Find closest point on AABB to circle center
        Vec2 closest_point = aabb.closest_point(circle.center);
        Vec2 center_to_closest = closest_point - circle.center;
        f32 distance_to_closest = center_to_closest.length();
        
        result.distance = distance_to_closest - circle.radius;
        result.is_overlapping = result.distance < 0.0f;
        
        if (distance_to_closest > constants::EPSILON) {
            result.normal = center_to_closest / distance_to_closest;
            result.point_a = circle.center + result.normal * circle.radius;
            result.point_b = closest_point;
        } else {
            // Circle center is inside/on AABB - find minimum separation
            Vec2 aabb_center = aabb.center();
            Vec2 center_diff = circle.center - aabb_center;
            Vec2 aabb_half_size = aabb.half_size();
            
            // Find axis with minimum penetration
            f32 x_penetration = aabb_half_size.x - std::abs(center_diff.x);
            f32 y_penetration = aabb_half_size.y - std::abs(center_diff.y);
            
            if (x_penetration < y_penetration) {
                result.normal = Vec2{(center_diff.x > 0) ? 1.0f : -1.0f, 0.0f};
            } else {
                result.normal = Vec2{0.0f, (center_diff.y > 0) ? 1.0f : -1.0f};
            }
            
            result.point_a = circle.center + result.normal * circle.radius;
            result.point_b = aabb.closest_point(result.point_a);
        }
        
        return result;
    }
    
    Vec2 closest_point_on_line(const Vec2& point, const Vec2& line_start, const Vec2& line_end) noexcept {
        Vec2 line_vec = line_end - line_start;
        f32 line_length_sq = line_vec.length_squared();
        
        if (line_length_sq < constants::EPSILON) {
            return line_start;
        }
        
        Vec2 point_vec = point - line_start;
        f32 projection_param = point_vec.dot(line_vec) / line_length_sq;
        
        return line_start + line_vec * projection_param;
    }
    
    Vec2 closest_point_on_segment(const Vec2& point, const Vec2& seg_start, const Vec2& seg_end) noexcept {
        Vec2 segment = seg_end - seg_start;
        f32 segment_length_sq = segment.length_squared();
        
        if (segment_length_sq < constants::EPSILON) {
            return seg_start;
        }
        
        Vec2 to_point = point - seg_start;
        f32 projection_param = std::clamp(to_point.dot(segment) / segment_length_sq, 0.0f, 1.0f);
        
        return seg_start + segment * projection_param;
    }
    
    Vec2 closest_point_on_circle(const Vec2& point, const Circle& circle) noexcept {
        Vec2 to_point = point - circle.center;
        f32 distance = to_point.length();
        
        if (distance < constants::EPSILON) {
            // Point is at circle center - return arbitrary point on circumference
            return circle.center + Vec2{circle.radius, 0.0f};
        }
        
        return circle.center + (to_point / distance) * circle.radius;
    }
    
    bool intersects_circle_circle(const Circle& a, const Circle& b) noexcept {
        f32 center_distance_sq = vec2::distance_squared(a.center, b.center);
        f32 combined_radius = a.radius + b.radius;
        return center_distance_sq <= combined_radius * combined_radius;
    }
    
    bool intersects_aabb_aabb(const AABB& a, const AABB& b) noexcept {
        return !(b.min.x > a.max.x || b.max.x < a.min.x ||
                 b.min.y > a.max.y || b.max.y < a.min.y);
    }
    
    RaycastResult raycast_circle(const Ray2D& ray, const Circle& circle) noexcept {
        RaycastResult result;
        
        Vec2 to_center = circle.center - ray.origin;
        f32 projection = to_center.dot(ray.direction);
        
        // If projection is negative, ray points away from circle
        if (projection < 0.0f) {
            return result;  // No hit
        }
        
        // Find closest point on ray to circle center
        Vec2 closest_point = ray.origin + ray.direction * projection;
        f32 distance_to_center_sq = vec2::distance_squared(closest_point, circle.center);
        f32 radius_sq = circle.radius * circle.radius;
        
        // Check if ray passes close enough to circle
        if (distance_to_center_sq > radius_sq) {
            return result;  // No hit
        }
        
        // Calculate hit distance
        f32 chord_half_length = std::sqrt(radius_sq - distance_to_center_sq);
        f32 hit_distance = projection - chord_half_length;
        
        // Check if hit is within ray bounds
        if (hit_distance < 0.0f || hit_distance > ray.max_distance) {
            return result;  // No hit within bounds
        }
        
        // Calculate hit point and normal
        result.hit = true;
        result.distance = hit_distance;
        result.parameter = hit_distance / ray.max_distance;
        result.point = ray.origin + ray.direction * hit_distance;
        result.normal = (result.point - circle.center).normalized();
        
        return result;
    }
    
    RaycastResult raycast_aabb(const Ray2D& ray, const AABB& aabb) noexcept {
        RaycastResult result;
        
        // Calculate intersection parameters for each axis
        Vec2 inv_direction = Vec2{1.0f / ray.direction.x, 1.0f / ray.direction.y};
        
        f32 t1 = (aabb.min.x - ray.origin.x) * inv_direction.x;
        f32 t2 = (aabb.max.x - ray.origin.x) * inv_direction.x;
        f32 t3 = (aabb.min.y - ray.origin.y) * inv_direction.y;
        f32 t4 = (aabb.max.y - ray.origin.y) * inv_direction.y;
        
        f32 t_min_x = std::min(t1, t2);
        f32 t_max_x = std::max(t1, t2);
        f32 t_min_y = std::min(t3, t4);
        f32 t_max_y = std::max(t3, t4);
        
        f32 t_enter = std::max(t_min_x, t_min_y);
        f32 t_exit = std::min(t_max_x, t_max_y);
        
        // Check for intersection
        if (t_enter > t_exit || t_exit < 0.0f || t_enter > ray.max_distance) {
            return result;  // No hit
        }
        
        f32 hit_t = (t_enter > 0.0f) ? t_enter : t_exit;
        
        result.hit = true;
        result.distance = hit_t;
        result.parameter = hit_t / ray.max_distance;
        result.point = ray.origin + ray.direction * hit_t;
        
        // Calculate normal based on which face was hit
        Vec2 center = aabb.center();
        Vec2 hit_local = result.point - center;
        Vec2 half_size = aabb.half_size();
        
        // Find the axis with the largest relative distance
        f32 x_ratio = std::abs(hit_local.x / half_size.x);
        f32 y_ratio = std::abs(hit_local.y / half_size.y);
        
        if (x_ratio > y_ratio) {
            result.normal = Vec2{(hit_local.x > 0) ? 1.0f : -1.0f, 0.0f};
        } else {
            result.normal = Vec2{0.0f, (hit_local.y > 0) ? 1.0f : -1.0f};
        }
        
        return result;
    }
    
    bool sat_intersect(const Polygon& a, const Polygon& b) noexcept {
        // Separating Axis Theorem implementation for polygon intersection
        
        auto test_separation = [](const Polygon& poly_a, const Polygon& poly_b) -> bool {
            auto vertices_a = poly_a.get_vertices();
            
            for (usize i = 0; i < vertices_a.size(); ++i) {
                // Get edge normal as separating axis
                Vec2 edge = poly_a.get_edge(static_cast<u32>(i));
                Vec2 axis = vec2::perpendicular(edge).normalized();
                
                // Project both polygons onto axis
                auto [a_min, a_max] = poly_a.project_onto_axis(axis);
                auto [b_min, b_max] = poly_b.project_onto_axis(axis);
                
                // Check for separation
                if (a_max < b_min || b_max < a_min) {
                    return false;  // Separating axis found
                }
            }
            return true;  // No separating axis found
        };
        
        // Test both polygon's edge normals as potential separating axes
        return test_separation(a, b) && test_separation(b, a);
    }
    
} // namespace collision

//=============================================================================
// Physics Utility Functions Implementation
//=============================================================================

namespace utils {
    
    f32 moment_of_inertia_circle(f32 mass, f32 radius) noexcept {
        // I = (1/2) * m * r²
        return 0.5f * mass * radius * radius;
    }
    
    f32 moment_of_inertia_box(f32 mass, f32 width, f32 height) noexcept {
        // I = (1/12) * m * (w² + h²)
        return (1.0f / 12.0f) * mass * (width * width + height * height);
    }
    
    f32 moment_of_inertia_polygon(f32 mass, const Polygon& polygon) noexcept {
        // Calculate moment of inertia for arbitrary convex polygon
        auto vertices = polygon.get_vertices();
        if (vertices.size() < 3) return 0.0f;
        
        f32 numerator = 0.0f;
        f32 denominator = 0.0f;
        
        for (usize i = 0; i < vertices.size(); ++i) {
            usize next = (i + 1) % vertices.size();
            
            Vec2 v1 = vertices[i];
            Vec2 v2 = vertices[next];
            
            f32 cross_prod = vec2::cross(v1, v2);
            f32 dot_sum = v1.dot(v1) + v1.dot(v2) + v2.dot(v2);
            
            numerator += cross_prod * dot_sum;
            denominator += cross_prod;
        }
        
        if (std::abs(denominator) < constants::EPSILON) return 0.0f;
        
        return (mass / 6.0f) * numerator / denominator;
    }
    
    Vec2 center_of_mass_points(std::span<const Vec2> points, std::span<const f32> masses) noexcept {
        if (points.empty() || masses.empty() || points.size() != masses.size()) {
            return Vec2::zero();
        }
        
        Vec2 weighted_sum = Vec2::zero();
        f32 total_mass = 0.0f;
        
        for (usize i = 0; i < points.size(); ++i) {
            weighted_sum += points[i] * masses[i];
            total_mass += masses[i];
        }
        
        return (total_mass > constants::EPSILON) ? weighted_sum / total_mass : Vec2::zero();
    }
    
    Vec2 center_of_mass_polygon(const Polygon& polygon) noexcept {
        return polygon.get_centroid();
    }
    
    f32 smooth_step(f32 t) noexcept {
        t = std::clamp(t, 0.0f, 1.0f);
        return t * t * (3.0f - 2.0f * t);
    }
    
    f32 smoother_step(f32 t) noexcept {
        t = std::clamp(t, 0.0f, 1.0f);
        return t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f);
    }
    
    f32 ease_in_quad(f32 t) noexcept {
        return t * t;
    }
    
    f32 ease_out_quad(f32 t) noexcept {
        return 1.0f - (1.0f - t) * (1.0f - t);
    }
    
    f32 ease_in_out_quad(f32 t) noexcept {
        return (t < 0.5f) ? 2.0f * t * t : 1.0f - 2.0f * (1.0f - t) * (1.0f - t);
    }
    
    SpringForce calculate_spring_force(f32 current_length, f32 rest_length, 
                                     f32 spring_constant, f32 damping_ratio,
                                     f32 velocity) noexcept {
        SpringForce result;
        
        f32 displacement = current_length - rest_length;
        
        // Hooke's law: F = -k * x
        result.force = -spring_constant * displacement;
        
        // Damping force: F = -c * v
        result.damping_force = -damping_ratio * velocity;
        
        return result;
    }
    
    Vec2 integrate_velocity_verlet(Vec2 position, Vec2 velocity, Vec2 acceleration, f32 dt) noexcept {
        // Velocity-Verlet integration for better numerical stability
        // x(t+dt) = x(t) + v(t)*dt + 0.5*a(t)*dt²
        return position + velocity * dt + acceleration * (0.5f * dt * dt);
    }
    
    f32 calculate_polygon_area(std::span<const Vec2> vertices) noexcept {
        if (vertices.size() < 3) return 0.0f;
        
        f32 area = 0.0f;
        for (usize i = 0; i < vertices.size(); ++i) {
            usize next = (i + 1) % vertices.size();
            area += vec2::cross(vertices[i], vertices[next]);
        }
        
        return std::abs(area) * 0.5f;
    }
    
    f32 calculate_triangle_area(const Vec2& a, const Vec2& b, const Vec2& c) noexcept {
        // Using cross product: Area = 0.5 * |cross(AB, AC)|
        Vec2 ab = b - a;
        Vec2 ac = c - a;
        return std::abs(vec2::cross(ab, ac)) * 0.5f;
    }
    
    bool point_in_polygon_winding(const Vec2& point, std::span<const Vec2> vertices) noexcept {
        // Winding number algorithm - more robust than crossing number
        if (vertices.size() < 3) return false;
        
        i32 winding_number = 0;
        
        for (usize i = 0; i < vertices.size(); ++i) {
            usize next = (i + 1) % vertices.size();
            
            Vec2 v1 = vertices[i];
            Vec2 v2 = vertices[next];
            
            if (v1.y <= point.y) {
                if (v2.y > point.y) {  // Upward crossing
                    f32 cross = vec2::cross(v2 - v1, point - v1);
                    if (cross > 0.0f) {
                        winding_number++;
                    }
                }
            } else {
                if (v2.y <= point.y) {  // Downward crossing
                    f32 cross = vec2::cross(v2 - v1, point - v1);
                    if (cross < 0.0f) {
                        winding_number--;
                    }
                }
            }
        }
        
        return winding_number != 0;
    }
    
    bool point_in_polygon_crossing(const Vec2& point, std::span<const Vec2> vertices) noexcept {
        // Ray casting algorithm - count intersections with edges
        if (vertices.size() < 3) return false;
        
        bool inside = false;
        
        for (usize i = 0; i < vertices.size(); ++i) {
            usize next = (i + 1) % vertices.size();
            
            Vec2 v1 = vertices[i];
            Vec2 v2 = vertices[next];
            
            // Check if point is between vertex y-coordinates
            if (((v1.y > point.y) != (v2.y > point.y))) {
                // Calculate x-coordinate of intersection
                f32 x_intersect = (v2.x - v1.x) * (point.y - v1.y) / (v2.y - v1.y) + v1.x;
                
                if (point.x < x_intersect) {
                    inside = !inside;
                }
            }
        }
        
        return inside;
    }
    
    std::vector<Vec2> convex_hull(std::span<const Vec2> points) noexcept {
        if (points.size() < 3) {
            return std::vector<Vec2>(points.begin(), points.end());
        }
        
        // Convert to vector for sorting
        std::vector<Vec2> sorted_points(points.begin(), points.end());
        
        // Sort points lexicographically (x first, then y)
        std::sort(sorted_points.begin(), sorted_points.end(), [](const Vec2& a, const Vec2& b) {
            return a.x < b.x || (a.x == b.x && a.y < b.y);
        });
        
        // Graham scan algorithm
        std::vector<Vec2> hull;
        
        // Build lower hull
        for (const Vec2& point : sorted_points) {
            while (hull.size() >= 2) {
                f32 cross = vec2::cross(hull[hull.size()-1] - hull[hull.size()-2], 
                                       point - hull[hull.size()-2]);
                if (cross <= 0.0f) {
                    hull.pop_back();
                } else {
                    break;
                }
            }
            hull.push_back(point);
        }
        
        // Build upper hull
        usize lower_size = hull.size();
        for (auto it = sorted_points.rbegin() + 1; it != sorted_points.rend(); ++it) {
            while (hull.size() > lower_size) {
                f32 cross = vec2::cross(hull[hull.size()-1] - hull[hull.size()-2], 
                                       *it - hull[hull.size()-2]);
                if (cross <= 0.0f) {
                    hull.pop_back();
                } else {
                    break;
                }
            }
            hull.push_back(*it);
        }
        
        // Remove last point as it's the same as the first
        if (hull.size() > 1) {
            hull.pop_back();
        }
        
        return hull;
    }
    
    AABB smallest_enclosing_aabb(std::span<const Vec2> points) noexcept {
        if (points.empty()) return AABB{};
        
        Vec2 min = points[0];
        Vec2 max = points[0];
        
        for (const Vec2& point : points) {
            min.x = std::min(min.x, point.x);
            min.y = std::min(min.y, point.y);
            max.x = std::max(max.x, point.x);
            max.y = std::max(max.y, point.y);
        }
        
        return AABB{min, max};
    }
    
} // namespace utils

//=============================================================================
// Educational Debug Utilities Implementation
//=============================================================================

namespace debug {
    
    CollisionDebugInfo debug_collision_detection(const Circle& a, const Circle& b) noexcept {
        using namespace std::chrono;
        auto start_time = high_resolution_clock::now();
        
        CollisionDebugInfo info;
        
        // Step 1: Calculate center distance
        Vec2 center_diff = b.center - a.center;
        f32 center_distance = center_diff.length();
        
        info.steps.push_back({
            "Calculate distance between circle centers",
            a.center, b.center, Vec2::zero(), center_distance, true
        });
        
        // Step 2: Calculate combined radius
        f32 combined_radius = a.radius + b.radius;
        
        info.steps.push_back({
            "Combined radius calculation",
            Vec2::zero(), Vec2::zero(), Vec2::zero(), combined_radius, true
        });
        
        // Step 3: Compare distances
        f32 separation = center_distance - combined_radius;
        bool overlapping = separation < 0.0f;
        
        info.steps.push_back({
            overlapping ? "Circles are overlapping" : "Circles are separated",
            Vec2::zero(), Vec2::zero(), Vec2::zero(), separation, true
        });
        
        // Final result
        info.final_result = collision::distance_circle_to_circle(a, b);
        
        auto end_time = high_resolution_clock::now();
        auto duration = duration_cast<microseconds>(end_time - start_time);
        info.computation_time_ms = duration.count() / 1000.0;
        
        return info;
    }
    
    VisualizationData visualize_collision(const Circle& a, const Circle& b) noexcept {
        VisualizationData data;
        data.title = "Circle-Circle Collision Visualization";
        data.description = "Visualization of distance calculation between two circles";
        
        // Draw circles
        constexpr u32 CIRCLE_SEGMENTS = 32;
        constexpr u32 COLOR_A = 0xFF0000FF;  // Red
        constexpr u32 COLOR_B = 0x0000FFFF;  // Blue
        
        for (u32 i = 0; i < CIRCLE_SEGMENTS; ++i) {
            f32 angle1 = (static_cast<f32>(i) / CIRCLE_SEGMENTS) * constants::TWO_PI_F;
            f32 angle2 = (static_cast<f32>(i + 1) / CIRCLE_SEGMENTS) * constants::TWO_PI_F;
            
            Vec2 p1_a = a.center + Vec2{a.radius * std::cos(angle1), a.radius * std::sin(angle1)};
            Vec2 p2_a = a.center + Vec2{a.radius * std::cos(angle2), a.radius * std::sin(angle2)};
            
            Vec2 p1_b = b.center + Vec2{b.radius * std::cos(angle1), b.radius * std::sin(angle1)};
            Vec2 p2_b = b.center + Vec2{b.radius * std::cos(angle2), b.radius * std::sin(angle2)};
            
            data.lines.push_back({p1_a, p2_a, COLOR_A, 2.0f, false});
            data.lines.push_back({p1_b, p2_b, COLOR_B, 2.0f, false});
        }
        
        // Draw center connection
        data.lines.push_back({a.center, b.center, 0x00FF00FF, 1.0f, true});  // Green dashed
        
        // Mark centers
        data.points.push_back({a.center, COLOR_A, 4.0f, "Center A"});
        data.points.push_back({b.center, COLOR_B, 4.0f, "Center B"});
        
        // Calculate and visualize closest points
        auto result = collision::distance_circle_to_circle(a, b);
        data.points.push_back({result.point_a, 0xFFFF00FF, 3.0f, "Closest A"});  // Yellow
        data.points.push_back({result.point_b, 0xFF00FFFF, 3.0f, "Closest B"});  // Magenta
        
        return data;
    }
    
    VisualizationData visualize_raycast(const Ray2D& ray, const Circle& target) noexcept {
        VisualizationData data;
        data.title = "Ray-Circle Intersection Visualization";
        data.description = "Visualization of ray casting against a circle";
        
        // Draw ray
        Vec2 ray_end = ray.origin + ray.direction * ray.max_distance;
        data.lines.push_back({ray.origin, ray_end, 0xFF0000FF, 2.0f, false});  // Red
        
        // Draw circle
        constexpr u32 CIRCLE_SEGMENTS = 32;
        for (u32 i = 0; i < CIRCLE_SEGMENTS; ++i) {
            f32 angle1 = (static_cast<f32>(i) / CIRCLE_SEGMENTS) * constants::TWO_PI_F;
            f32 angle2 = (static_cast<f32>(i + 1) / CIRCLE_SEGMENTS) * constants::TWO_PI_F;
            
            Vec2 p1 = target.center + Vec2{target.radius * std::cos(angle1), target.radius * std::sin(angle1)};
            Vec2 p2 = target.center + Vec2{target.radius * std::cos(angle2), target.radius * std::sin(angle2)};
            
            data.lines.push_back({p1, p2, 0x0000FFFF, 2.0f, false});  // Blue
        }
        
        // Mark key points
        data.points.push_back({ray.origin, 0xFF0000FF, 4.0f, "Ray Origin"});
        data.points.push_back({target.center, 0x0000FFFF, 4.0f, "Circle Center"});
        
        // Perform raycast and visualize result
        auto result = collision::raycast_circle(ray, target);
        if (result.hit) {
            data.points.push_back({result.point, 0x00FF00FF, 5.0f, "Hit Point"});  // Green
            
            // Draw normal at hit point
            Vec2 normal_end = result.point + result.normal * 20.0f;
            data.lines.push_back({result.point, normal_end, 0x00FF00FF, 1.5f, false});  // Green
        }
        
        return data;
    }
    
    MathExplanation explain_cross_product() noexcept {
        MathExplanation explanation;
        explanation.concept_name = "2D Cross Product";
        explanation.formula = "a × b = a.x * b.y - a.y * b.x";
        explanation.intuitive_explanation = 
            "The 2D cross product gives the z-component of the 3D cross product. "
            "It represents the signed area of the parallelogram formed by the two vectors. "
            "Positive values indicate counter-clockwise rotation from a to b, "
            "negative values indicate clockwise rotation.";
        
        explanation.applications = {
            "Determining rotation direction",
            "Computing torque and angular momentum", 
            "Finding signed area of triangles",
            "Collision detection and response",
            "Checking if point is left/right of line"
        };
        
        explanation.common_mistakes = {
            "Forgetting that 2D cross product returns scalar, not vector",
            "Not understanding the geometric meaning of sign",
            "Confusing cross product with dot product",
            "Not normalizing vectors when only direction matters"
        };
        
        explanation.complexity_analysis = "O(1) - constant time operation with 3 arithmetic operations";
        
        return explanation;
    }
    
    MathExplanation explain_sat_algorithm() noexcept {
        MathExplanation explanation;
        explanation.concept_name = "Separating Axis Theorem (SAT)";
        explanation.formula = "If ∃ axis such that projA ∩ projB = ∅, then A ∩ B = ∅";
        explanation.intuitive_explanation = 
            "SAT states that two convex shapes don't intersect if there exists a line "
            "onto which their projections don't overlap. For polygons, we only need to "
            "test the normals of each edge as potential separating axes. If all projections "
            "overlap, the shapes intersect.";
        
        explanation.applications = {
            "Polygon-polygon collision detection",
            "Fast broad-phase collision culling",
            "Separating overlapping objects",
            "Computing minimum translation vector"
        };
        
        explanation.common_mistakes = {
            "Testing too many axes (only edge normals needed)",
            "Not handling degenerate cases properly",
            "Incorrect projection calculations",
            "Forgetting to test both polygon's edge normals"
        };
        
        explanation.complexity_analysis = "O(n + m) where n, m are vertex counts";
        
        return explanation;
    }
    
    bool verify_vector_operations() noexcept {
        bool all_passed = true;
        
        // Test vector addition
        Vec2 a{1.0f, 2.0f};
        Vec2 b{3.0f, 4.0f};
        Vec2 sum = a + b;
        if (!vec2::approximately_equal(sum, Vec2{4.0f, 6.0f})) {
            // Vector addition test failed
            all_passed = false;
        }
        
        // Test cross product
        f32 cross = vec2::cross(Vec2{1.0f, 0.0f}, Vec2{0.0f, 1.0f});
        if (std::abs(cross - 1.0f) > constants::EPSILON) {
            // Cross product test failed
            all_passed = false;
        }
        
        // Test normalization
        Vec2 v{3.0f, 4.0f};
        Vec2 normalized = v.normalized();
        if (std::abs(normalized.length() - 1.0f) > constants::EPSILON) {
            // Vector normalization test failed
            all_passed = false;
        }
        
        if (all_passed) {
            // All vector operation tests passed
        }
        
        return all_passed;
    }
    
    bool verify_collision_detection() noexcept {
        bool all_passed = true;
        
        // Test circle-circle intersection
        Circle c1{{0.0f, 0.0f}, 1.0f};
        Circle c2{{1.5f, 0.0f}, 1.0f};
        
        bool should_intersect = collision::intersects_circle_circle(c1, c2);
        if (!should_intersect) {
            // Circle intersection test failed - should intersect
            all_passed = false;
        }
        
        Circle c3{{3.0f, 0.0f}, 1.0f};
        bool should_not_intersect = collision::intersects_circle_circle(c1, c3);
        if (should_not_intersect) {
            // Circle intersection test failed - should not intersect
            all_passed = false;
        }
        
        // Test AABB intersection
        AABB box1{{0.0f, 0.0f}, {2.0f, 2.0f}};
        AABB box2{{1.0f, 1.0f}, {3.0f, 3.0f}};
        
        bool boxes_intersect = collision::intersects_aabb_aabb(box1, box2);
        if (!boxes_intersect) {
            // AABB intersection test failed
            all_passed = false;
        }
        
        if (all_passed) {
            // All collision detection tests passed
        }
        
        return all_passed;
    }
    
    MemoryAnalysis analyze_memory_usage() noexcept {
        MemoryAnalysis analysis;
        
        analysis.shape_memory_usage[0] = sizeof(Circle);
        analysis.shape_memory_usage[1] = sizeof(AABB);
        analysis.shape_memory_usage[2] = sizeof(OBB);
        analysis.shape_memory_usage[3] = sizeof(Polygon);
        analysis.shape_memory_usage[4] = sizeof(Ray2D);
        
        // Calculate cache line efficiency
        analysis.cache_line_efficiency = 
            (sizeof(Vec2) * 2) % ecscope::core::CACHE_LINE_SIZE;  // Two Vec2s per cache line
        
        // Estimate alignment waste
        analysis.alignment_waste = 
            alignof(Transform2D) - sizeof(Transform2D) % alignof(Transform2D);
        
        analysis.recommendations = 
            "Consider packing small shapes into arrays for better cache locality. "
            "Use SOA (Structure of Arrays) for bulk operations on many objects.";
        
        return analysis;
    }
    
} // namespace debug

} // namespace ecscope::physics::math