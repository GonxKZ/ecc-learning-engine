#include "collision.hpp"
#include "../core/log.hpp"
#include <algorithm>
#include <cmath>
#include <chrono>

namespace ecscope::physics::collision {

//=============================================================================
// Basic Distance Functions Implementation
//=============================================================================

DistanceResult distance_circle_to_circle(const Circle& a, const Circle& b) noexcept {
    // Calculate vector between centers
    Vec2 center_to_center = b.center - a.center;
    f32 distance_between_centers = center_to_center.length();
    f32 combined_radii = a.radius + b.radius;
    
    DistanceResult result;
    result.distance = distance_between_centers - combined_radii;
    result.is_overlapping = result.distance < 0.0f;
    
    if (distance_between_centers > constants::EPSILON) {
        // Normalize direction vector
        Vec2 direction = center_to_center / distance_between_centers;
        
        // Calculate closest points on each circle
        result.point_a = a.center + direction * a.radius;
        result.point_b = b.center - direction * b.radius;
        result.normal = direction;
    } else {
        // Circles have identical centers - handle degenerate case
        result.point_a = a.center + Vec2{a.radius, 0.0f};
        result.point_b = b.center - Vec2{b.radius, 0.0f};
        result.normal = Vec2{1.0f, 0.0f};
    }
    
    return result;
}

DistanceResult distance_aabb_to_aabb(const AABB& a, const AABB& b) noexcept {
    DistanceResult result;
    
    // Calculate separations on each axis
    f32 x_separation = std::max(0.0f, std::max(a.min.x - b.max.x, b.min.x - a.max.x));
    f32 y_separation = std::max(0.0f, std::max(a.min.y - b.max.y, b.min.y - a.max.y));
    
    result.is_overlapping = (x_separation == 0.0f && y_separation == 0.0f);
    
    if (result.is_overlapping) {
        // AABBs are overlapping - calculate penetration
        f32 x_overlap = std::min(a.max.x - b.min.x, b.max.x - a.min.x);
        f32 y_overlap = std::min(a.max.y - b.min.y, b.max.y - a.min.y);
        
        if (x_overlap < y_overlap) {
            // Separate along X axis
            result.distance = -x_overlap;
            result.normal = (a.center().x < b.center().x) ? Vec2{-1.0f, 0.0f} : Vec2{1.0f, 0.0f};
            
            // Calculate contact points
            f32 contact_y = std::max(a.min.y, b.min.y) + 
                           std::min(a.max.y - a.min.y, b.max.y - b.min.y) * 0.5f;
            f32 contact_x = (result.normal.x > 0) ? a.min.x : a.max.x;
            
            result.point_a = Vec2{contact_x, contact_y};
            result.point_b = result.point_a - result.normal * result.distance;
        } else {
            // Separate along Y axis
            result.distance = -y_overlap;
            result.normal = (a.center().y < b.center().y) ? Vec2{0.0f, -1.0f} : Vec2{0.0f, 1.0f};
            
            // Calculate contact points
            f32 contact_x = std::max(a.min.x, b.min.x) + 
                           std::min(a.max.x - a.min.x, b.max.x - b.min.x) * 0.5f;
            f32 contact_y = (result.normal.y > 0) ? a.min.y : a.max.y;
            
            result.point_a = Vec2{contact_x, contact_y};
            result.point_b = result.point_a - result.normal * result.distance;
        }
    } else {
        // AABBs are separated
        result.distance = std::sqrt(x_separation * x_separation + y_separation * y_separation);
        
        // Find closest points
        Vec2 center_a = a.center();
        Vec2 center_b = b.center();
        
        result.point_a = Vec2{std::clamp(center_b.x, a.min.x, a.max.x),
                             std::clamp(center_b.y, a.min.y, a.max.y)};
        result.point_b = Vec2{std::clamp(center_a.x, b.min.x, b.max.x),
                             std::clamp(center_a.y, b.min.y, b.max.y)};
        
        if (result.distance > constants::EPSILON) {
            result.normal = (result.point_b - result.point_a) / result.distance;
        } else {
            result.normal = Vec2{1.0f, 0.0f};
        }
    }
    
    return result;
}

DistanceResult distance_circle_to_aabb(const Circle& circle, const AABB& aabb) noexcept {
    DistanceResult result;
    
    // Find closest point on AABB to circle center
    Vec2 closest_point = aabb.closest_point(circle.center);
    Vec2 circle_to_closest = closest_point - circle.center;
    f32 distance_to_closest = circle_to_closest.length();
    
    result.distance = distance_to_closest - circle.radius;
    result.is_overlapping = result.distance < 0.0f;
    
    if (distance_to_closest > constants::EPSILON) {
        Vec2 direction = circle_to_closest / distance_to_closest;
        result.normal = direction;
        result.point_a = circle.center + direction * circle.radius;
        result.point_b = closest_point;
    } else {
        // Circle center is exactly on AABB - handle degenerate case
        // Push circle away from AABB center
        Vec2 aabb_center = aabb.center();
        Vec2 push_direction = circle.center - aabb_center;
        
        if (push_direction.length_squared() < constants::EPSILON) {
            push_direction = Vec2{1.0f, 0.0f};  // Arbitrary direction
        } else {
            push_direction = push_direction.normalized();
        }
        
        result.normal = push_direction;
        result.point_a = circle.center + push_direction * circle.radius;
        result.point_b = closest_point;
    }
    
    return result;
}

DistanceResult distance_obb_to_obb(const OBB& a, const OBB& b) noexcept {
    // This is a complex implementation - using simplified SAT approach
    DistanceResult result;
    
    // Get the axes to test (2 from each OBB)
    const Vec2& a_axis_x = a.get_axis_x();
    const Vec2& a_axis_y = a.get_axis_y();
    const Vec2& b_axis_x = b.get_axis_x();
    const Vec2& b_axis_y = b.get_axis_y();
    
    std::array<Vec2, 4> test_axes = {a_axis_x, a_axis_y, b_axis_x, b_axis_y};
    
    f32 min_overlap = std::numeric_limits<f32>::max();
    Vec2 min_overlap_axis;
    bool is_separated = false;
    
    // Test each axis for separation
    for (const Vec2& axis : test_axes) {
        auto [min_a, max_a] = a.project_onto_axis(axis);
        auto [min_b, max_b] = b.project_onto_axis(axis);
        
        f32 separation = std::max(min_a - max_b, min_b - max_a);
        
        if (separation > 0.0f) {
            // Found separating axis
            is_separated = true;
            result.distance = separation;
            result.normal = (min_a > max_b) ? -axis : axis;
            result.is_overlapping = false;
            break;
        }
        
        // Track minimum overlap for penetration resolution
        f32 overlap = std::min(max_a, max_b) - std::max(min_a, min_b);
        if (overlap < min_overlap) {
            min_overlap = overlap;
            min_overlap_axis = axis;
        }
    }
    
    if (!is_separated) {
        // OBBs are overlapping
        result.is_overlapping = true;
        result.distance = -min_overlap;
        
        // Determine correct normal direction
        Vec2 center_to_center = b.center - a.center;
        if (center_to_center.dot(min_overlap_axis) < 0.0f) {
            min_overlap_axis = -min_overlap_axis;
        }
        result.normal = min_overlap_axis;
        
        // Calculate approximate contact points (simplified)
        result.point_a = a.center + result.normal * (min_overlap * 0.5f);
        result.point_b = result.point_a - result.normal * min_overlap;
    }
    
    // If separated, calculate closest points (simplified approximation)
    if (is_separated) {
        result.point_a = a.center;
        result.point_b = b.center;
    }
    
    return result;
}

DistanceResult distance_point_to_polygon(const Vec2& point, const Polygon& polygon) noexcept {
    DistanceResult result;
    
    if (polygon.vertex_count == 0) {
        result.distance = 0.0f;
        result.is_overlapping = false;
        return result;
    }
    
    bool inside = point_in_polygon(point, polygon);
    
    if (inside) {
        // Point is inside polygon - find distance to closest edge
        f32 min_distance = std::numeric_limits<f32>::max();
        Vec2 closest_point;
        Vec2 best_normal;
        
        for (u32 i = 0; i < polygon.vertex_count; ++i) {
            u32 next = (i + 1) % polygon.vertex_count;
            Vec2 edge_start = polygon.vertices[i];
            Vec2 edge_end = polygon.vertices[next];
            
            Vec2 edge_closest = closest_point_on_segment(point, edge_start, edge_end);
            f32 distance = vec2::distance(point, edge_closest);
            
            if (distance < min_distance) {
                min_distance = distance;
                closest_point = edge_closest;
                
                // Calculate outward normal
                Vec2 edge = edge_end - edge_start;
                best_normal = vec2::perpendicular(edge).normalized();
                
                // Ensure normal points outward
                Vec2 to_center = polygon.get_centroid() - edge_closest;
                if (best_normal.dot(to_center) > 0.0f) {
                    best_normal = -best_normal;
                }
            }
        }
        
        result.is_overlapping = true;
        result.distance = -min_distance;
        result.point_a = point;
        result.point_b = closest_point;
        result.normal = best_normal;
    } else {
        // Point is outside polygon - find closest edge
        f32 min_distance = std::numeric_limits<f32>::max();
        Vec2 closest_point;
        
        for (u32 i = 0; i < polygon.vertex_count; ++i) {
            u32 next = (i + 1) % polygon.vertex_count;
            Vec2 edge_start = polygon.vertices[i];
            Vec2 edge_end = polygon.vertices[next];
            
            Vec2 edge_closest = closest_point_on_segment(point, edge_start, edge_end);
            f32 distance = vec2::distance(point, edge_closest);
            
            if (distance < min_distance) {
                min_distance = distance;
                closest_point = edge_closest;
            }
        }
        
        result.is_overlapping = false;
        result.distance = min_distance;
        result.point_a = point;
        result.point_b = closest_point;
        result.normal = (point - closest_point).normalized();
    }
    
    return result;
}

//=============================================================================
// Utility Functions Implementation
//=============================================================================

Vec2 closest_point_on_segment(const Vec2& point, const Vec2& seg_start, const Vec2& seg_end) noexcept {
    Vec2 segment = seg_end - seg_start;
    Vec2 point_to_start = point - seg_start;
    
    f32 segment_length_squared = segment.length_squared();
    
    if (segment_length_squared < constants::EPSILON) {
        // Degenerate segment - both endpoints are the same
        return seg_start;
    }
    
    // Project point onto segment
    f32 t = point_to_start.dot(segment) / segment_length_squared;
    t = std::clamp(t, 0.0f, 1.0f);
    
    return seg_start + segment * t;
}

f32 distance_point_to_segment(const Vec2& point, const Vec2& seg_start, const Vec2& seg_end) noexcept {
    Vec2 closest = closest_point_on_segment(point, seg_start, seg_end);
    return vec2::distance(point, closest);
}

bool point_in_polygon(const Vec2& point, const Polygon& polygon) noexcept {
    if (polygon.vertex_count < 3) return false;
    
    // Use winding number algorithm for better numerical stability
    f32 winding_number = 0.0f;
    
    for (u32 i = 0; i < polygon.vertex_count; ++i) {
        u32 next = (i + 1) % polygon.vertex_count;
        Vec2 v1 = polygon.vertices[i] - point;
        Vec2 v2 = polygon.vertices[next] - point;
        
        // Calculate angle contribution
        f32 cross = vec2::cross(v1, v2);
        f32 dot = v1.dot(v2);
        f32 angle = std::atan2(cross, dot);
        winding_number += angle;
    }
    
    // Point is inside if total winding is approximately ±2π
    f32 total_rotation = std::abs(winding_number);
    return total_rotation > constants::PI_F;  // Greater than π means inside
}

bool point_in_polygon_crossing(const Vec2& point, const Polygon& polygon) noexcept {
    if (polygon.vertex_count < 3) return false;
    
    bool inside = false;
    
    for (u32 i = 0, j = polygon.vertex_count - 1; i < polygon.vertex_count; j = i++) {
        const Vec2& vi = polygon.vertices[i];
        const Vec2& vj = polygon.vertices[j];
        
        if (((vi.y > point.y) != (vj.y > point.y)) &&
            (point.x < (vj.x - vi.x) * (point.y - vi.y) / (vj.y - vi.y) + vi.x)) {
            inside = !inside;
        }
    }
    
    return inside;
}

BarycentricCoords calculate_barycentric_coords(const Vec2& point, 
                                             const Vec2& a, const Vec2& b, const Vec2& c) noexcept {
    Vec2 v0 = c - a;
    Vec2 v1 = b - a;
    Vec2 v2 = point - a;
    
    f32 dot00 = v0.dot(v0);
    f32 dot01 = v0.dot(v1);
    f32 dot02 = v0.dot(v2);
    f32 dot11 = v1.dot(v1);
    f32 dot12 = v1.dot(v2);
    
    f32 inv_denom = 1.0f / (dot00 * dot11 - dot01 * dot01);
    
    BarycentricCoords coords;
    coords.v = (dot11 * dot02 - dot01 * dot12) * inv_denom;
    coords.w = (dot00 * dot12 - dot01 * dot02) * inv_denom;
    coords.u = 1.0f - coords.v - coords.w;
    
    return coords;
}

//=============================================================================
// Raycast Implementation
//=============================================================================

RaycastResult raycast_circle(const Ray2D& ray, const Circle& circle) noexcept {
    Vec2 ray_to_circle = ray.origin - circle.center;
    
    // Quadratic equation coefficients: at² + bt + c = 0
    f32 a = ray.direction.dot(ray.direction);  // Should be 1 if direction is normalized
    f32 b = 2.0f * ray_to_circle.dot(ray.direction);
    f32 c = ray_to_circle.dot(ray_to_circle) - circle.radius * circle.radius;
    
    f32 discriminant = b * b - 4 * a * c;
    
    if (discriminant < 0.0f) {
        return RaycastResult::miss();  // No intersection
    }
    
    f32 sqrt_discriminant = std::sqrt(discriminant);
    f32 t1 = (-b - sqrt_discriminant) / (2 * a);
    f32 t2 = (-b + sqrt_discriminant) / (2 * a);
    
    // We want the closest positive intersection
    f32 t = (t1 >= 0.0f) ? t1 : t2;
    
    if (t < 0.0f || t > ray.max_distance) {
        return RaycastResult::miss();
    }
    
    Vec2 hit_point = ray.origin + ray.direction * t;
    Vec2 normal = (hit_point - circle.center).normalized();
    
    return RaycastResult::hit_result(t, hit_point, normal, t / ray.max_distance);
}

RaycastResult raycast_aabb(const Ray2D& ray, const AABB& aabb) noexcept {
    // Slab method - treat AABB as intersection of 2 slabs
    Vec2 inv_dir = Vec2{1.0f / ray.direction.x, 1.0f / ray.direction.y};
    
    // Calculate intersection times for each slab
    f32 t_min_x = (aabb.min.x - ray.origin.x) * inv_dir.x;
    f32 t_max_x = (aabb.max.x - ray.origin.x) * inv_dir.x;
    
    if (t_min_x > t_max_x) std::swap(t_min_x, t_max_x);
    
    f32 t_min_y = (aabb.min.y - ray.origin.y) * inv_dir.y;
    f32 t_max_y = (aabb.max.y - ray.origin.y) * inv_dir.y;
    
    if (t_min_y > t_max_y) std::swap(t_min_y, t_max_y);
    
    // Find intersection of the slabs
    f32 t_enter = std::max(t_min_x, t_min_y);
    f32 t_exit = std::min(t_max_x, t_max_y);
    
    // Check if ray intersects AABB
    if (t_enter > t_exit || t_exit < 0.0f || t_enter > ray.max_distance) {
        return RaycastResult::miss();
    }
    
    f32 t = (t_enter >= 0.0f) ? t_enter : t_exit;
    Vec2 hit_point = ray.origin + ray.direction * t;
    
    // Calculate normal based on which face was hit
    Vec2 normal;
    Vec2 center = aabb.center();
    Vec2 hit_relative = hit_point - center;
    Vec2 abs_hit = Vec2{std::abs(hit_relative.x), std::abs(hit_relative.y)};
    Vec2 half_size = aabb.half_size();
    
    if (abs_hit.x / half_size.x > abs_hit.y / half_size.y) {
        // Hit vertical face
        normal = Vec2{(hit_relative.x > 0) ? 1.0f : -1.0f, 0.0f};
    } else {
        // Hit horizontal face
        normal = Vec2{0.0f, (hit_relative.y > 0) ? 1.0f : -1.0f};
    }
    
    return RaycastResult::hit_result(t, hit_point, normal, t / ray.max_distance);
}

RaycastResult raycast_obb(const Ray2D& ray, const OBB& obb) noexcept {
    // Transform ray to OBB's local space
    Vec2 local_origin = obb.world_to_local(ray.origin);
    
    // Transform direction (no translation, only rotation)
    const Vec2& axis_x = obb.get_axis_x();
    const Vec2& axis_y = obb.get_axis_y();
    Vec2 local_direction = Vec2{ray.direction.dot(axis_x), ray.direction.dot(axis_y)};
    
    // Create local AABB and ray
    AABB local_aabb = AABB::from_center_size(Vec2::zero(), obb.half_extents * 2.0f);
    Ray2D local_ray{local_origin, local_direction, ray.max_distance};
    
    // Perform AABB raycast in local space
    RaycastResult local_result = raycast_aabb(local_ray, local_aabb);
    
    if (!local_result.hit) {
        return RaycastResult::miss();
    }
    
    // Transform result back to world space
    RaycastResult world_result = local_result;
    world_result.point = obb.local_to_world(local_result.point);
    
    // Transform normal back to world space
    world_result.normal = axis_x * local_result.normal.x + axis_y * local_result.normal.y;
    
    return world_result;
}

RaycastResult raycast_polygon(const Ray2D& ray, const Polygon& polygon) noexcept {
    if (polygon.vertex_count < 3) {
        return RaycastResult::miss();
    }
    
    f32 closest_t = std::numeric_limits<f32>::max();
    Vec2 closest_normal;
    bool hit_found = false;
    
    // Test ray against each edge of the polygon
    for (u32 i = 0; i < polygon.vertex_count; ++i) {
        u32 next = (i + 1) % polygon.vertex_count;
        Vec2 edge_start = polygon.vertices[i];
        Vec2 edge_end = polygon.vertices[next];
        
        // Ray-line intersection
        Vec2 edge_dir = edge_end - edge_start;
        Vec2 ray_to_edge = edge_start - ray.origin;
        
        f32 cross1 = vec2::cross(ray.direction, edge_dir);
        
        if (std::abs(cross1) < constants::EPSILON) {
            continue;  // Ray is parallel to edge
        }
        
        f32 t = vec2::cross(ray_to_edge, edge_dir) / cross1;
        f32 s = vec2::cross(ray_to_edge, ray.direction) / cross1;
        
        // Check if intersection is within ray and edge segment
        if (t >= 0.0f && t <= ray.max_distance && s >= 0.0f && s <= 1.0f) {
            if (t < closest_t) {
                closest_t = t;
                hit_found = true;
                
                // Calculate normal (perpendicular to edge, pointing outward)
                closest_normal = vec2::perpendicular(edge_dir).normalized();
                
                // Ensure normal points outward from polygon
                Vec2 polygon_center = polygon.get_centroid();
                Vec2 edge_center = (edge_start + edge_end) * 0.5f;
                Vec2 outward = edge_center - polygon_center;
                
                if (closest_normal.dot(outward) < 0.0f) {
                    closest_normal = -closest_normal;
                }
            }
        }
    }
    
    if (!hit_found) {
        return RaycastResult::miss();
    }
    
    Vec2 hit_point = ray.origin + ray.direction * closest_t;
    return RaycastResult::hit_result(closest_t, hit_point, closest_normal, closest_t / ray.max_distance);
}

//=============================================================================
// SAT Implementation
//=============================================================================

namespace sat {
    
    Projection project_aabb(const AABB& aabb, const Vec2& axis) noexcept {
        Vec2 center = aabb.center();
        Vec2 half_size = aabb.half_size();
        
        f32 center_projection = center.dot(axis);
        f32 extent_projection = std::abs(axis.x * half_size.x) + std::abs(axis.y * half_size.y);
        
        return {center_projection - extent_projection, center_projection + extent_projection};
    }
    
    Projection project_circle(const Circle& circle, const Vec2& axis) noexcept {
        f32 center_projection = circle.center.dot(axis);
        return {center_projection - circle.radius, center_projection + circle.radius};
    }
    
    Projection project_polygon(const Polygon& polygon, const Vec2& axis) noexcept {
        if (polygon.vertex_count == 0) {
            return {0.0f, 0.0f};
        }
        
        f32 min_proj = polygon.vertices[0].dot(axis);
        f32 max_proj = min_proj;
        
        for (u32 i = 1; i < polygon.vertex_count; ++i) {
            f32 projection = polygon.vertices[i].dot(axis);
            min_proj = std::min(min_proj, projection);
            max_proj = std::max(max_proj, projection);
        }
        
        return {min_proj, max_proj};
    }
    
    std::vector<Vec2> get_polygon_axes(const Polygon& a, const Polygon& b) noexcept {
        std::vector<Vec2> axes;
        axes.reserve(a.vertex_count + b.vertex_count);
        
        // Add normals from polygon A
        for (u32 i = 0; i < a.vertex_count; ++i) {
            u32 next = (i + 1) % a.vertex_count;
            Vec2 edge = a.vertices[next] - a.vertices[i];
            Vec2 normal = vec2::perpendicular(edge).normalized();
            axes.push_back(normal);
        }
        
        // Add normals from polygon B
        for (u32 i = 0; i < b.vertex_count; ++i) {
            u32 next = (i + 1) % b.vertex_count;
            Vec2 edge = b.vertices[next] - b.vertices[i];
            Vec2 normal = vec2::perpendicular(edge).normalized();
            axes.push_back(normal);
        }
        
        return axes;
    }
    
} // namespace sat

//=============================================================================
// Debug and Educational Functions
//=============================================================================

namespace debug {
    
    CollisionDebugInfo debug_collision_detection(const Circle& a, const Circle& b) noexcept {
        auto start_time = std::chrono::high_resolution_clock::now();
        
        CollisionDebugInfo info;
        info.algorithm_used = "Circle-Circle Distance";
        
        info.step_descriptions.push_back("Calculate vector between circle centers");
        info.step_descriptions.push_back("Calculate distance between centers");
        info.step_descriptions.push_back("Compare with sum of radii");
        info.step_descriptions.push_back("Calculate closest points and normal");
        
        // Perform the actual collision detection
        info.final_result = distance_circle_to_circle(a, b);
        
        auto end_time = std::chrono::high_resolution_clock::now();
        info.total_time_ns = std::chrono::duration<f64, std::nano>(end_time - start_time).count();
        
        // Fill visualization data
        info.visualization.closest_points.push_back(info.final_result.point_a);
        info.visualization.closest_points.push_back(info.final_result.point_b);
        
        return info;
    }
    
    CollisionDebugInfo debug_collision_detection(const AABB& a, const AABB& b) noexcept {
        auto start_time = std::chrono::high_resolution_clock::now();
        
        CollisionDebugInfo info;
        info.algorithm_used = "AABB-AABB Separation Test";
        
        info.step_descriptions.push_back("Calculate X-axis separation");
        info.step_descriptions.push_back("Calculate Y-axis separation");
        info.step_descriptions.push_back("Check for overlap on both axes");
        info.step_descriptions.push_back("Calculate penetration or separation distance");
        
        info.final_result = distance_aabb_to_aabb(a, b);
        
        auto end_time = std::chrono::high_resolution_clock::now();
        info.total_time_ns = std::chrono::duration<f64, std::nano>(end_time - start_time).count();
        
        // Fill visualization data
        info.visualization.test_axes.push_back(Vec2{1.0f, 0.0f});  // X axis
        info.visualization.test_axes.push_back(Vec2{0.0f, 1.0f});  // Y axis
        
        return info;
    }
    
    AlgorithmExplanation explain_sat_algorithm() noexcept {
        AlgorithmExplanation explanation;
        
        explanation.algorithm_name = "Separating Axis Theorem (SAT)";
        explanation.mathematical_basis = "Two convex shapes are separated if and only if there exists a line such that when both shapes are projected onto that line, the projections do not overlap.";
        explanation.time_complexity = "O(n + m) where n and m are the number of edges";
        explanation.space_complexity = "O(1) additional space";
        
        explanation.key_concepts = {
            "Convex shapes only - concave shapes need decomposition",
            "Test all edge normals as potential separating axes",
            "Project both shapes onto each axis",
            "If any axis shows separation, shapes don't intersect",
            "Minimum overlap axis gives collision normal and penetration"
        };
        
        explanation.common_optimizations = {
            "Early exit on first separating axis found",
            "Cache previous frame's separating axis for coherent contacts",
            "Use SIMD for multiple projections simultaneously",
            "Pre-compute and cache edge normals"
        };
        
        explanation.when_to_use = "Best for polygon-polygon collision. More expensive than specialized algorithms for simple shapes like circles and axis-aligned boxes.";
        
        return explanation;
    }
    
    AlgorithmExplanation explain_gjk_algorithm() noexcept {
        AlgorithmExplanation explanation;
        
        explanation.algorithm_name = "Gilbert-Johnson-Keerthi (GJK)";
        explanation.mathematical_basis = "Works in Minkowski difference space. Two shapes intersect if and only if the origin is contained in their Minkowski difference.";
        explanation.time_complexity = "O(1) iterations typically, O(n) worst case";
        explanation.space_complexity = "O(1) - only stores current simplex";
        
        explanation.key_concepts = {
            "Minkowski difference: A ⊕ (-B) = {a - b | a ∈ A, b ∈ B}",
            "Support function: finds furthest point in given direction",
            "Simplex evolution: iteratively builds simplex around origin",
            "Works with any convex shape that has support function",
            "Can provide distance information for separated shapes"
        };
        
        explanation.common_optimizations = {
            "Warm starting with previous frame's simplex",
            "EPA (Expanding Polytope Algorithm) for penetration depth",
            "Cached support points for common directions",
            "Specialized support functions for primitive shapes"
        };
        
        explanation.when_to_use = "Most general collision detection algorithm. Ideal when you need one algorithm for all convex shape pairs. Overkill for simple cases like circle-circle.";
        
        return explanation;
    }
    
} // namespace debug

} // namespace ecscope::physics::collision