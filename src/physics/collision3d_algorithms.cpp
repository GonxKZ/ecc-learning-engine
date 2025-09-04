#include "collision3d_algorithms.hpp"
#include "core/log.hpp"
#include <algorithm>
#include <chrono>
#include <queue>

namespace ecscope::physics::collision3d {

//=============================================================================
// 3D SAT Implementation
//=============================================================================

namespace sat3d {

Projection3D project_sphere(const Sphere& sphere, const Vec3& axis) noexcept {
    f32 center_projection = sphere.center.dot(axis);
    return Projection3D{center_projection - sphere.radius, center_projection + sphere.radius};
}

Projection3D project_aabb(const AABB3D& aabb, const Vec3& axis) noexcept {
    Vec3 center = aabb.center();
    Vec3 half_size = aabb.half_size();
    
    f32 center_proj = center.dot(axis);
    f32 extent_proj = std::abs(axis.x * half_size.x) + 
                      std::abs(axis.y * half_size.y) + 
                      std::abs(axis.z * half_size.z);
    
    return Projection3D{center_proj - extent_proj, center_proj + extent_proj};
}

Projection3D project_obb(const OBB3D& obb, const Vec3& axis) noexcept {
    return obb.project_onto_axis(axis);
}

Projection3D project_capsule(const Capsule& capsule, const Vec3& axis) noexcept {
    // Project the line segment endpoints
    f32 proj_a = capsule.point_a.dot(axis);
    f32 proj_b = capsule.point_b.dot(axis);
    
    f32 min_proj = std::min(proj_a, proj_b) - capsule.radius;
    f32 max_proj = std::max(proj_a, proj_b) + capsule.radius;
    
    return Projection3D{min_proj, max_proj};
}

Projection3D project_convex_hull(const ConvexHull& hull, const Vec3& axis) noexcept {
    auto vertices = hull.get_vertices();
    if (vertices.empty()) {
        return Projection3D{0.0f, 0.0f};
    }
    
    f32 min_proj = vertices[0].dot(axis);
    f32 max_proj = min_proj;
    
    for (usize i = 1; i < vertices.size(); ++i) {
        f32 proj = vertices[i].dot(axis);
        min_proj = std::min(min_proj, proj);
        max_proj = std::max(max_proj, proj);
    }
    
    return Projection3D{min_proj, max_proj};
}

SATResult3D test_obb_vs_obb(const OBB3D& obb_a, const OBB3D& obb_b) noexcept {
    auto start_time = std::chrono::high_resolution_clock::now();
    
    SATResult3D result;
    result.min_overlap = std::numeric_limits<f32>::max();
    
    // Get separating axes to test
    std::vector<Vec3> axes = get_obb_separating_axes(obb_a, obb_b);
    
    for (usize i = 0; i < axes.size(); ++i) {
        const Vec3& axis = axes[i];
        
        SATResult3D::DebugStep step;
        step.axis_tested = axis;
        
        // Determine axis source for educational purposes
        if (i < 3) {
            step.axis_source = "Face A";
        } else if (i < 6) {
            step.axis_source = "Face B";
        } else {
            step.axis_source = "Edge Cross Product";
        }
        
        // Project both OBBs onto the axis
        step.projection_a = project_obb(obb_a, axis);
        step.projection_b = project_obb(obb_b, axis);
        step.overlap = step.projection_a.overlap_amount(step.projection_b);
        step.is_separating = !step.projection_a.overlaps(step.projection_b);
        
        if (step.is_separating) {
            result.is_separating = true;
            result.separating_axis = axis;
            result.separation_distance = step.projection_a.separation_distance(step.projection_b);
            result.early_exit_at_axis = static_cast<u32>(i + 1);
            
            step.explanation = "Separating axis found - objects do not intersect";
            result.debug_steps.push_back(step);
            break;
        } else {
            // Track minimum overlap for potential contact normal
            if (step.overlap < result.min_overlap) {
                result.min_overlap = step.overlap;
                result.min_overlap_axis = axis;
            }
            
            step.explanation = "Overlap found on this axis - continue testing";
            result.debug_steps.push_back(step);
        }
        
        result.total_axes_tested++;
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    result.total_computation_time_ns = std::chrono::duration<f64, std::nano>(end_time - start_time).count();
    
    return result;
}

SATResult3D test_convex_hull_vs_convex_hull(const ConvexHull& hull_a, const ConvexHull& hull_b) noexcept {
    auto start_time = std::chrono::high_resolution_clock::now();
    
    SATResult3D result;
    result.min_overlap = std::numeric_limits<f32>::max();
    
    // Get all potential separating axes
    std::vector<Vec3> axes = get_convex_hull_separating_axes(hull_a, hull_b);
    
    for (usize i = 0; i < axes.size(); ++i) {
        const Vec3& axis = axes[i];
        
        SATResult3D::DebugStep step;
        step.axis_tested = axis;
        
        // Project both hulls onto the axis
        step.projection_a = project_convex_hull(hull_a, axis);
        step.projection_b = project_convex_hull(hull_b, axis);
        step.overlap = step.projection_a.overlap_amount(step.projection_b);
        step.is_separating = !step.projection_a.overlaps(step.projection_b);
        
        if (step.is_separating) {
            result.is_separating = true;
            result.separating_axis = axis;
            result.separation_distance = step.projection_a.separation_distance(step.projection_b);
            result.early_exit_at_axis = static_cast<u32>(i + 1);
            
            step.explanation = "Separating axis found - convex hulls do not intersect";
            result.debug_steps.push_back(step);
            break;
        } else {
            if (step.overlap < result.min_overlap) {
                result.min_overlap = step.overlap;
                result.min_overlap_axis = axis;
            }
            
            step.explanation = "Overlap found - continue testing remaining axes";
            result.debug_steps.push_back(step);
        }
        
        result.total_axes_tested++;
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    result.total_computation_time_ns = std::chrono::duration<f64, std::nano>(end_time - start_time).count();
    
    return result;
}

std::vector<Vec3> get_obb_separating_axes(const OBB3D& obb_a, const OBB3D& obb_b) noexcept {
    std::vector<Vec3> axes;
    axes.reserve(15); // 3 + 3 + 9 axes for OBB-OBB
    
    // Face normals of OBB A
    axes.push_back(obb_a.get_axis_x());
    axes.push_back(obb_a.get_axis_y());
    axes.push_back(obb_a.get_axis_z());
    
    // Face normals of OBB B
    axes.push_back(obb_b.get_axis_x());
    axes.push_back(obb_b.get_axis_y());
    axes.push_back(obb_b.get_axis_z());
    
    // Cross products of edge pairs
    const Vec3 a_axes[3] = {obb_a.get_axis_x(), obb_a.get_axis_y(), obb_a.get_axis_z()};
    const Vec3 b_axes[3] = {obb_b.get_axis_x(), obb_b.get_axis_y(), obb_b.get_axis_z()};
    
    for (u32 i = 0; i < 3; ++i) {
        for (u32 j = 0; j < 3; ++j) {
            Vec3 cross_product = a_axes[i].cross(b_axes[j]);
            f32 length_sq = cross_product.length_squared();
            
            // Skip nearly parallel edges
            if (length_sq > constants::EPSILON * constants::EPSILON) {
                axes.push_back(cross_product / std::sqrt(length_sq));
            }
        }
    }
    
    return axes;
}

std::vector<Vec3> get_convex_hull_separating_axes(const ConvexHull& hull_a, const ConvexHull& hull_b) noexcept {
    std::vector<Vec3> axes;
    
    // Face normals of hull A
    auto faces_a = hull_a.get_faces();
    for (const auto& face : faces_a) {
        axes.push_back(face.normal);
    }
    
    // Face normals of hull B
    auto faces_b = hull_b.get_faces();
    for (const auto& face : faces_b) {
        axes.push_back(face.normal);
    }
    
    // Edge cross products (simplified - full implementation would enumerate all edge pairs)
    // For educational purposes, we'll use a subset of potential edge cross products
    auto vertices_a = hull_a.get_vertices();
    auto vertices_b = hull_b.get_vertices();
    
    // This is a simplified implementation - production code would be more thorough
    for (usize i = 0; i < std::min(vertices_a.size(), usize{8}); ++i) {
        for (usize j = 0; j < std::min(vertices_b.size(), usize{8}); ++j) {
            Vec3 edge_a = vertices_a[(i + 1) % vertices_a.size()] - vertices_a[i];
            Vec3 edge_b = vertices_b[(j + 1) % vertices_b.size()] - vertices_b[j];
            
            Vec3 cross_product = edge_a.cross(edge_b);
            f32 length_sq = cross_product.length_squared();
            
            if (length_sq > constants::EPSILON * constants::EPSILON) {
                axes.push_back(cross_product / std::sqrt(length_sq));
            }
        }
    }
    
    return axes;
}

} // namespace sat3d

//=============================================================================
// 3D GJK Implementation
//=============================================================================

namespace gjk3d {

Vec3 get_support_point_3d(const Sphere& shape, const Vec3& direction) noexcept {
    return shape.get_support_point(direction);
}

Vec3 get_support_point_3d(const AABB3D& shape, const Vec3& direction) noexcept {
    return shape.get_support_point(direction);
}

Vec3 get_support_point_3d(const OBB3D& shape, const Vec3& direction) noexcept {
    return shape.get_support_point(direction);
}

Vec3 get_support_point_3d(const Capsule& shape, const Vec3& direction) noexcept {
    return shape.get_support_point(direction);
}

Vec3 get_support_point_3d(const ConvexHull& shape, const Vec3& direction) noexcept {
    return shape.get_support_point(direction);
}

template<typename ShapeA, typename ShapeB>
GJKResult3D test_collision_3d(const ShapeA& a, const ShapeB& b) noexcept {
    auto start_time = std::chrono::high_resolution_clock::now();
    
    GJKResult3D result;
    Simplex3D& simplex = result.final_simplex;
    
    // Initial search direction (arbitrary)
    Vec3 direction = Vec3{1.0f, 0.0f, 0.0f};
    
    // Get first support point
    SupportPoint3D support = get_minkowski_support_3d(a, b, direction);
    simplex.add_point(support);
    
    // Search direction toward origin
    direction = -support.point;
    
    while (result.iterations_used < result.max_iterations) {
        result.iterations_used++;
        
        // Get new support point
        support = get_minkowski_support_3d(a, b, direction);
        
        // Check if we made progress toward the origin
        if (support.point.dot(direction) <= 0.0f) {
            // No progress made - shapes don't intersect
            result.is_intersecting = false;
            result.termination_reason = "No progress toward origin";
            break;
        }
        
        simplex.add_point(support);
        
        // Debug information
        GJKResult3D::DebugIteration debug_iter;
        debug_iter.simplex_state = simplex;
        debug_iter.search_direction = direction;
        debug_iter.new_support = support;
        debug_iter.distance_to_origin = direction.length();
        
        switch (simplex.count) {
            case 1: debug_iter.simplex_type = "Point"; break;
            case 2: debug_iter.simplex_type = "Line"; break;
            case 3: debug_iter.simplex_type = "Triangle"; break;
            case 4: debug_iter.simplex_type = "Tetrahedron"; break;
        }
        
        // Handle simplex and check for intersection
        if (handle_simplex_3d(simplex, direction)) {
            result.is_intersecting = true;
            result.termination_reason = "Origin enclosed in simplex";
            debug_iter.converged = true;
            debug_iter.explanation = "Tetrahedron encloses origin - shapes intersect";
            result.debug_iterations.push_back(debug_iter);
            break;
        }
        
        debug_iter.converged = false;
        debug_iter.explanation = "Simplex updated, continue search";
        result.debug_iterations.push_back(debug_iter);
        
        // Check for convergence (direction vector becoming very small)
        if (direction.length_squared() < constants::EPSILON * constants::EPSILON) {
            result.is_intersecting = false;
            result.termination_reason = "Convergence - shapes are touching";
            break;
        }
    }
    
    if (result.iterations_used >= result.max_iterations) {
        result.termination_reason = "Maximum iterations reached";
    }
    
    // If not intersecting, calculate closest points
    if (!result.is_intersecting) {
        auto closest_points = get_closest_points_from_simplex(simplex);
        result.closest_point_a = closest_points.first;
        result.closest_point_b = closest_points.second;
        result.distance = result.closest_point_a.distance_to(result.closest_point_b);
        result.separating_direction = (result.closest_point_b - result.closest_point_a).normalized();
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    result.total_computation_time_ns = std::chrono::duration<f64, std::nano>(end_time - start_time).count();
    
    return result;
}

bool handle_simplex_3d(Simplex3D& simplex, Vec3& direction) noexcept {
    switch (simplex.count) {
        case 1:
            return handle_point_simplex(simplex, direction);
        case 2:
            return handle_line_simplex(simplex, direction);
        case 3:
            return handle_triangle_simplex(simplex, direction);
        case 4:
            return handle_tetrahedron_simplex(simplex, direction);
        default:
            return false;
    }
}

bool handle_point_simplex(Simplex3D& simplex, Vec3& direction) noexcept {
    // With just one point, search toward the origin
    direction = -simplex[0].point;
    return false;
}

bool handle_line_simplex(Simplex3D& simplex, Vec3& direction) noexcept {
    const Vec3& a = simplex[1].point; // Most recent point
    const Vec3& b = simplex[0].point; // Previous point
    const Vec3 ab = b - a;
    const Vec3 ao = -a;
    
    if (ab.dot(ao) > 0.0f) {
        // Origin is closest to line segment
        direction = ab.cross(ao).cross(ab);
        
        // If cross product is zero, use perpendicular direction
        if (direction.length_squared() < constants::EPSILON) {
            // Create arbitrary perpendicular direction
            Vec3 arbitrary = Vec3::unit_x();
            if (std::abs(ab.dot(arbitrary)) > 0.9f) {
                arbitrary = Vec3::unit_y();
            }
            direction = ab.cross(arbitrary).normalized();
        }
    } else {
        // Origin is closest to point A
        simplex.remove_point(0);
        direction = ao;
    }
    
    return false;
}

bool handle_triangle_simplex(Simplex3D& simplex, Vec3& direction) noexcept {
    const Vec3& a = simplex[2].point; // Most recent point
    const Vec3& b = simplex[1].point;
    const Vec3& c = simplex[0].point;
    
    const Vec3 ab = b - a;
    const Vec3 ac = c - a;
    const Vec3 ao = -a;
    
    const Vec3 abc = ab.cross(ac); // Triangle normal
    
    // Check which side of the triangle the origin is on
    if (ab.cross(abc).dot(ao) > 0.0f) {
        // Origin is on the AB side
        if (ab.dot(ao) > 0.0f) {
            // Keep AB edge, remove C
            simplex.remove_point(0);
            direction = ab.cross(ao).cross(ab);
        } else {
            // Origin is closest to A
            simplex.count = 1; // Keep only A
            simplex[0] = simplex[2];
            direction = ao;
        }
    } else if (abc.cross(ac).dot(ao) > 0.0f) {
        // Origin is on the AC side
        if (ac.dot(ao) > 0.0f) {
            // Keep AC edge, remove B
            simplex.remove_point(1);
            direction = ac.cross(ao).cross(ac);
        } else {
            // Origin is closest to A
            simplex.count = 1;
            simplex[0] = simplex[2];
            direction = ao;
        }
    } else {
        // Origin is on the triangle
        if (abc.dot(ao) > 0.0f) {
            direction = abc;
        } else {
            direction = -abc;
        }
    }
    
    return false;
}

bool handle_tetrahedron_simplex(Simplex3D& simplex, Vec3& direction) noexcept {
    const Vec3& a = simplex[3].point; // Most recent point
    const Vec3& b = simplex[2].point;
    const Vec3& c = simplex[1].point;
    const Vec3& d = simplex[0].point;
    
    const Vec3 ab = b - a;
    const Vec3 ac = c - a;
    const Vec3 ad = d - a;
    const Vec3 ao = -a;
    
    // Check each face of the tetrahedron
    const Vec3 abc = ab.cross(ac);
    const Vec3 acd = ac.cross(ad);
    const Vec3 adb = ad.cross(ab);
    
    // Check if origin is outside face ABC
    if (abc.dot(ao) > 0.0f) {
        // Remove D, keep triangle ABC
        simplex.remove_point(0);
        direction = abc;
        return false;
    }
    
    // Check if origin is outside face ACD
    if (acd.dot(ao) > 0.0f) {
        // Remove B, keep triangle ACD
        simplex.remove_point(2);
        direction = acd;
        return false;
    }
    
    // Check if origin is outside face ADB
    if (adb.dot(ao) > 0.0f) {
        // Remove C, keep triangle ADB
        simplex.remove_point(1);
        direction = adb;
        return false;
    }
    
    // Origin is inside the tetrahedron
    return true;
}

std::pair<Vec3, Vec3> get_closest_points_from_simplex(const Simplex3D& simplex) noexcept {
    // This is a simplified implementation
    // Production code would use more sophisticated methods like EPA or iterative closest point
    
    if (simplex.count == 0) {
        return {Vec3::zero(), Vec3::zero()};
    }
    
    // For now, return the closest support points from the simplex
    const SupportPoint3D& closest = simplex[simplex.count - 1];
    return {closest.point_a, closest.point_b};
}

// Template instantiations
template GJKResult3D test_collision_3d<Sphere, Sphere>(const Sphere&, const Sphere&) noexcept;
template GJKResult3D test_collision_3d<AABB3D, AABB3D>(const AABB3D&, const AABB3D&) noexcept;
template GJKResult3D test_collision_3d<OBB3D, OBB3D>(const OBB3D&, const OBB3D&) noexcept;
template GJKResult3D test_collision_3d<ConvexHull, ConvexHull>(const ConvexHull&, const ConvexHull&) noexcept;

} // namespace gjk3d

//=============================================================================
// 3D Primitive Collision Detection Implementation
//=============================================================================

namespace primitives3d {

DistanceResult3D distance_sphere_to_sphere(const Sphere& a, const Sphere& b) noexcept {
    Vec3 center_diff = b.center - a.center;
    f32 center_distance = center_diff.length();
    f32 radii_sum = a.radius + b.radius;
    
    if (center_distance < constants::EPSILON) {
        // Spheres are at the same position
        return DistanceResult3D::overlapping(a.center, b.center, Vec3::unit_x(), radii_sum);
    }
    
    Vec3 normal = center_diff / center_distance;
    f32 distance = center_distance - radii_sum;
    
    Vec3 point_a = a.center + normal * a.radius;
    Vec3 point_b = b.center - normal * b.radius;
    
    if (distance <= 0.0f) {
        return DistanceResult3D::overlapping(point_a, point_b, normal, -distance);
    } else {
        return DistanceResult3D::separated(point_a, point_b, distance);
    }
}

DistanceResult3D distance_sphere_to_aabb(const Sphere& sphere, const AABB3D& aabb) noexcept {
    Vec3 closest_point = aabb.closest_point(sphere.center);
    Vec3 diff = sphere.center - closest_point;
    f32 distance_to_surface = diff.length();
    
    if (distance_to_surface < constants::EPSILON) {
        // Sphere center is inside AABB
        // Find closest face
        Vec3 center = aabb.center();
        Vec3 half_size = aabb.half_size();
        Vec3 local_pos = sphere.center - center;
        
        // Find the axis with minimum penetration
        f32 min_penetration = std::numeric_limits<f32>::max();
        Vec3 normal = Vec3::unit_x();
        
        // Check each axis
        for (u32 i = 0; i < 3; ++i) {
            f32 penetration = half_size[i] - std::abs(local_pos[i]);
            if (penetration < min_penetration) {
                min_penetration = penetration;
                normal = Vec3::zero();
                normal[i] = local_pos[i] > 0.0f ? 1.0f : -1.0f;
            }
        }
        
        Vec3 contact_point = sphere.center + normal * (min_penetration + sphere.radius);
        return DistanceResult3D::overlapping(contact_point, closest_point, normal, min_penetration + sphere.radius);
    }
    
    Vec3 normal = diff / distance_to_surface;
    f32 distance = distance_to_surface - sphere.radius;
    
    Vec3 point_on_sphere = sphere.center - normal * sphere.radius;
    
    if (distance <= 0.0f) {
        return DistanceResult3D::overlapping(point_on_sphere, closest_point, normal, -distance);
    } else {
        return DistanceResult3D::separated(point_on_sphere, closest_point, distance);
    }
}

DistanceResult3D distance_aabb_to_aabb(const AABB3D& a, const AABB3D& b) noexcept {
    // Check for separation on each axis
    f32 separation_x = std::max(a.min.x - b.max.x, b.min.x - a.max.x);
    f32 separation_y = std::max(a.min.y - b.max.y, b.min.y - a.max.y);
    f32 separation_z = std::max(a.min.z - b.max.z, b.min.z - a.max.z);
    
    if (separation_x > 0.0f || separation_y > 0.0f || separation_z > 0.0f) {
        // AABBs are separated
        f32 max_separation = std::max({separation_x, separation_y, separation_z});
        
        Vec3 normal = Vec3::zero();
        if (max_separation == separation_x) {
            normal.x = (a.center().x > b.center().x) ? 1.0f : -1.0f;
        } else if (max_separation == separation_y) {
            normal.y = (a.center().y > b.center().y) ? 1.0f : -1.0f;
        } else {
            normal.z = (a.center().z > b.center().z) ? 1.0f : -1.0f;
        }
        
        Vec3 point_a = a.center() - normal * (max_separation * 0.5f);
        Vec3 point_b = b.center() + normal * (max_separation * 0.5f);
        
        return DistanceResult3D::separated(point_a, point_b, max_separation);
    } else {
        // AABBs are overlapping
        f32 overlap_x = std::min(a.max.x - b.min.x, b.max.x - a.min.x);
        f32 overlap_y = std::min(a.max.y - b.min.y, b.max.y - a.min.y);
        f32 overlap_z = std::min(a.max.z - b.min.z, b.max.z - a.min.z);
        
        f32 min_overlap = std::min({overlap_x, overlap_y, overlap_z});
        
        Vec3 normal = Vec3::zero();
        if (min_overlap == overlap_x) {
            normal.x = (a.center().x > b.center().x) ? 1.0f : -1.0f;
        } else if (min_overlap == overlap_y) {
            normal.y = (a.center().y > b.center().y) ? 1.0f : -1.0f;
        } else {
            normal.z = (a.center().z > b.center().z) ? 1.0f : -1.0f;
        }
        
        Vec3 contact_center = (a.center() + b.center()) * 0.5f;
        Vec3 point_a = contact_center - normal * (min_overlap * 0.5f);
        Vec3 point_b = contact_center + normal * (min_overlap * 0.5f);
        
        return DistanceResult3D::overlapping(point_a, point_b, normal, min_overlap);
    }
}

bool point_in_sphere(const Vec3& point, const Sphere& sphere) noexcept {
    return sphere.contains(point);
}

bool point_in_aabb(const Vec3& point, const AABB3D& aabb) noexcept {
    return aabb.contains(point);
}

bool point_in_obb(const Vec3& point, const OBB3D& obb) noexcept {
    return obb.contains(point);
}

bool point_in_convex_hull(const Vec3& point, const ConvexHull& hull) noexcept {
    return hull.contains(point);
}

} // namespace primitives3d

//=============================================================================
// 3D Raycast Implementation
//=============================================================================

namespace raycast3d {

RaycastResult3D raycast_sphere(const Ray3D& ray, const Sphere& sphere) noexcept {
    Vec3 to_sphere = ray.origin - sphere.center;
    
    f32 a = ray.direction.dot(ray.direction); // Should be 1.0 for normalized direction
    f32 b = 2.0f * to_sphere.dot(ray.direction);
    f32 c = to_sphere.dot(to_sphere) - sphere.radius * sphere.radius;
    
    f32 discriminant = b * b - 4.0f * a * c;
    
    if (discriminant < 0.0f) {
        return RaycastResult3D::miss();
    }
    
    f32 sqrt_discriminant = std::sqrt(discriminant);
    f32 t1 = (-b - sqrt_discriminant) / (2.0f * a);
    f32 t2 = (-b + sqrt_discriminant) / (2.0f * a);
    
    f32 t = (t1 >= 0.0f) ? t1 : t2;
    
    if (t < 0.0f || t > ray.max_distance) {
        return RaycastResult3D::miss();
    }
    
    Vec3 hit_point = ray.point_at(t);
    Vec3 normal = (hit_point - sphere.center).normalized();
    
    return RaycastResult3D::hit_result(t, hit_point, normal, t / ray.max_distance);
}

RaycastResult3D raycast_aabb(const Ray3D& ray, const AABB3D& aabb) noexcept {
    // Slab method for AABB intersection
    Vec3 inv_dir = Vec3{
        std::abs(ray.direction.x) > constants::EPSILON ? 1.0f / ray.direction.x : std::numeric_limits<f32>::max(),
        std::abs(ray.direction.y) > constants::EPSILON ? 1.0f / ray.direction.y : std::numeric_limits<f32>::max(),
        std::abs(ray.direction.z) > constants::EPSILON ? 1.0f / ray.direction.z : std::numeric_limits<f32>::max()
    };
    
    f32 t_min = 0.0f;
    f32 t_max = ray.max_distance;
    Vec3 normal = Vec3::zero();
    
    for (u32 i = 0; i < 3; ++i) {
        f32 t1 = (aabb.min[i] - ray.origin[i]) * inv_dir[i];
        f32 t2 = (aabb.max[i] - ray.origin[i]) * inv_dir[i];
        
        if (t1 > t2) std::swap(t1, t2);
        
        if (t1 > t_min) {
            t_min = t1;
            normal = Vec3::zero();
            normal[i] = ray.direction[i] > 0.0f ? -1.0f : 1.0f;
        }
        
        if (t2 < t_max) {
            t_max = t2;
        }
        
        if (t_min > t_max) {
            return RaycastResult3D::miss();
        }
    }
    
    f32 t = (t_min >= 0.0f) ? t_min : t_max;
    
    if (t < 0.0f || t > ray.max_distance) {
        return RaycastResult3D::miss();
    }
    
    Vec3 hit_point = ray.point_at(t);
    return RaycastResult3D::hit_result(t, hit_point, normal, t / ray.max_distance);
}

} // namespace raycast3d

//=============================================================================
// Debug Implementation
//=============================================================================

namespace debug3d {

CollisionDebugInfo3D debug_collision_3d(const Sphere& a, const Sphere& b) noexcept {
    CollisionDebugInfo3D info;
    info.algorithm_used = "Sphere-Sphere Distance";
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    info.step_descriptions = {
        "Calculate vector between sphere centers",
        "Compute distance between centers", 
        "Compare with sum of radii",
        "Determine contact points and normal"
    };
    
    info.final_result = primitives3d::distance_sphere_to_sphere(a, b);
    
    auto end_time = std::chrono::high_resolution_clock::now();
    info.total_time_ns = std::chrono::duration<f64, std::nano>(end_time - start_time).count();
    
    return info;
}

AlgorithmExplanation3D explain_gjk_3d() noexcept {
    AlgorithmExplanation3D explanation;
    explanation.algorithm_name = "Gilbert-Johnson-Keerthi (GJK) 3D";
    explanation.mathematical_basis = 
        "GJK works on the Minkowski difference of two convex shapes. In 3D, it constructs "
        "a tetrahedron (4-simplex) that attempts to enclose the origin. If successful, "
        "the shapes are intersecting.";
    
    explanation.time_complexity = "O(k) where k is the number of iterations (typically < 32)";
    explanation.space_complexity = "O(1) - uses fixed-size simplex";
    
    explanation.key_concepts = {
        "Minkowski difference A - B",
        "Support functions for convex shapes", 
        "Simplex evolution (point → line → triangle → tetrahedron)",
        "Origin enclosure test"
    };
    
    explanation.advantages = {
        "Works with any convex shape having a support function",
        "No need to enumerate separating axes",
        "Provides closest points when shapes don't intersect",
        "Numerically stable"
    };
    
    explanation.disadvantages = {
        "More complex than specialized algorithms",
        "Requires EPA for penetration depth",
        "May require many iterations for nearly-touching objects"
    };
    
    explanation.when_to_use = "General-purpose convex collision detection, especially for complex shapes";
    explanation.comparison_to_2d = 
        "3D GJK uses tetrahedra instead of triangles, requiring more complex simplex handling. "
        "The number of possible simplex configurations increases significantly.";
    
    return explanation;
}

} // namespace debug3d

} // namespace ecscope::physics::collision3d