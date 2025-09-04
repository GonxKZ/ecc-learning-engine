#include "collision3d.hpp"
#include "collision3d_algorithms.hpp"
#include <algorithm>
#include <cmath>

namespace ecscope::physics::collision3d {

//=============================================================================
// 3D Primitive Implementation
//=============================================================================

AABB3D Sphere::get_aabb() const noexcept {
    Vec3 radius_vec{radius, radius, radius};
    return AABB3D{center - radius_vec, center + radius_vec};
}

AABB3D AABB3D::from_points(std::span<const Vec3> points) noexcept {
    if (points.empty()) {
        return AABB3D{Vec3::zero(), Vec3::zero()};
    }
    
    Vec3 min_point = points[0];
    Vec3 max_point = points[0];
    
    for (usize i = 1; i < points.size(); ++i) {
        min_point.x = std::min(min_point.x, points[i].x);
        min_point.y = std::min(min_point.y, points[i].y);
        min_point.z = std::min(min_point.z, points[i].z);
        max_point.x = std::max(max_point.x, points[i].x);
        max_point.y = std::max(max_point.y, points[i].y);
        max_point.z = std::max(max_point.z, points[i].z);
    }
    
    return AABB3D{min_point, max_point};
}

//=============================================================================
// ConvexHull Implementation
//=============================================================================

ConvexHull::ConvexHull(std::span<const Vec3> verts) noexcept {
    set_vertices(verts);
}

void ConvexHull::set_vertices(std::span<const Vec3> verts) noexcept {
    vertex_count = std::min(static_cast<u32>(verts.size()), MAX_VERTICES);
    std::copy_n(verts.begin(), vertex_count, vertices.begin());
    properties_dirty = true;
    build_convex_hull();
}

void ConvexHull::add_vertex(const Vec3& vertex) noexcept {
    if (vertex_count < MAX_VERTICES) {
        vertices[vertex_count++] = vertex;
        properties_dirty = true;
        build_convex_hull();
    }
}

void ConvexHull::clear() noexcept {
    vertex_count = 0;
    face_count = 0;
    properties_dirty = true;
}

bool ConvexHull::contains(const Vec3& point) const noexcept {
    // Point is inside if it's on the correct side of all faces
    for (u32 i = 0; i < face_count; ++i) {
        const Face& face = faces[i];
        f32 distance_to_plane = (point - vertices[face.vertex_indices[0]]).dot(face.normal);
        if (distance_to_plane > constants::EPSILON) {
            return false; // Point is on the outside of this face
        }
    }
    return true;
}

f32 ConvexHull::distance_to_surface(const Vec3& point) const noexcept {
    if (contains(point)) {
        return 0.0f; // Point is inside
    }
    
    f32 min_distance = std::numeric_limits<f32>::max();
    
    // Find minimum distance to all faces
    for (u32 i = 0; i < face_count; ++i) {
        const Face& face = faces[i];
        const Vec3& v0 = vertices[face.vertex_indices[0]];
        
        // Distance from point to plane
        f32 distance_to_plane = std::abs((point - v0).dot(face.normal));
        min_distance = std::min(min_distance, distance_to_plane);
    }
    
    return min_distance;
}

Vec3 ConvexHull::closest_point_on_surface(const Vec3& point) const noexcept {
    if (contains(point)) {
        return point; // Point is inside
    }
    
    Vec3 closest_point = point;
    f32 min_distance_sq = std::numeric_limits<f32>::max();
    
    // Check distance to all faces
    for (u32 i = 0; i < face_count; ++i) {
        const Face& face = faces[i];
        const Vec3& v0 = vertices[face.vertex_indices[0]];
        const Vec3& v1 = vertices[face.vertex_indices[1]];
        const Vec3& v2 = vertices[face.vertex_indices[2]];
        
        // Project point onto face plane
        Vec3 to_point = point - v0;
        f32 distance_to_plane = to_point.dot(face.normal);
        Vec3 projected = point - face.normal * distance_to_plane;
        
        // Check if projected point is inside triangle (simplified)
        // For a complete implementation, we'd use barycentric coordinates
        f32 distance_sq = point.distance_squared_to(projected);
        if (distance_sq < min_distance_sq) {
            min_distance_sq = distance_sq;
            closest_point = projected;
        }
    }
    
    return closest_point;
}

ConvexHull ConvexHull::transformed(const Transform3D& transform) const noexcept {
    ConvexHull result;
    result.vertex_count = vertex_count;
    result.face_count = face_count;
    
    // Transform vertices
    for (u32 i = 0; i < vertex_count; ++i) {
        result.vertices[i] = transform.transform_point(vertices[i]);
    }
    
    // Transform face normals
    const Matrix3& rotation_matrix = transform.get_rotation_matrix();
    for (u32 i = 0; i < face_count; ++i) {
        result.faces[i] = faces[i]; // Copy indices
        result.faces[i].normal = rotation_matrix * faces[i].normal;
        
        // Recompute distance to origin for transformed face
        const Vec3& v0 = result.vertices[faces[i].vertex_indices[0]];
        result.faces[i].distance_to_origin = v0.dot(result.faces[i].normal);
    }
    
    result.properties_dirty = true;
    return result;
}

void ConvexHull::update_properties() const noexcept {
    if (vertex_count == 0) {
        centroid = Vec3::zero();
        volume_cached = 0.0f;
        properties_dirty = false;
        return;
    }
    
    // Calculate centroid
    centroid = Vec3::zero();
    for (u32 i = 0; i < vertex_count; ++i) {
        centroid += vertices[i];
    }
    centroid /= static_cast<f32>(vertex_count);
    
    // Calculate volume using divergence theorem (simplified)
    volume_cached = 0.0f;
    for (u32 i = 0; i < face_count; ++i) {
        const Face& face = faces[i];
        const Vec3& v0 = vertices[face.vertex_indices[0]];
        // Volume contribution: (1/3) * face_area * distance_to_origin
        // This is a simplified calculation
        volume_cached += std::abs(face.distance_to_origin) / 3.0f;
    }
    
    properties_dirty = false;
}

void ConvexHull::build_convex_hull() noexcept {
    if (vertex_count < 4) {
        face_count = 0;
        return; // Need at least 4 vertices for a 3D convex hull
    }
    
    // For educational purposes, we'll implement a simplified convex hull algorithm
    // In a production system, you'd use more sophisticated algorithms like QuickHull
    
    // This is a placeholder implementation
    // A full implementation would:
    // 1. Find initial tetrahedron
    // 2. Add remaining vertices one by one
    // 3. Update hull by removing visible faces and adding new faces
    
    face_count = 0; // Placeholder
    compute_face_normals();
}

void ConvexHull::compute_face_normals() noexcept {
    for (u32 i = 0; i < face_count; ++i) {
        Face& face = faces[i];
        const Vec3& v0 = vertices[face.vertex_indices[0]];
        const Vec3& v1 = vertices[face.vertex_indices[1]];
        const Vec3& v2 = vertices[face.vertex_indices[2]];
        
        // Calculate normal using cross product
        Vec3 edge1 = v1 - v0;
        Vec3 edge2 = v2 - v0;
        face.normal = edge1.cross(edge2).normalized();
        
        // Calculate distance to origin
        face.distance_to_origin = v0.dot(face.normal);
    }
}

//=============================================================================
// Contact Manifold 3D Implementation
//=============================================================================

void ContactManifold3D::reduce_contact_points() noexcept {
    if (point_count <= 4) {
        return; // No need to reduce
    }
    
    // Keep the 4 contact points that best represent the contact area
    // This is a simplified implementation - production code would use
    // more sophisticated contact reduction algorithms
    
    // Find the points with maximum separation
    std::array<ContactPoint3D, 4> reduced_points;
    u32 reduced_count = 0;
    
    // Start with the point with maximum penetration
    u32 deepest_index = 0;
    for (u32 i = 1; i < point_count; ++i) {
        if (points[i].penetration_depth > points[deepest_index].penetration_depth) {
            deepest_index = i;
        }
    }
    reduced_points[reduced_count++] = points[deepest_index];
    
    // Add up to 3 more points that are furthest from existing points
    for (u32 target = 1; target < 4 && reduced_count < point_count; ++target) {
        f32 max_distance = 0.0f;
        u32 best_index = 0;
        
        for (u32 i = 0; i < point_count; ++i) {
            // Skip if this point is already selected
            bool already_selected = false;
            for (u32 j = 0; j < reduced_count; ++j) {
                if (points[i].point.distance_squared_to(reduced_points[j].point) < constants::EPSILON) {
                    already_selected = true;
                    break;
                }
            }
            
            if (already_selected) continue;
            
            // Find minimum distance to already selected points
            f32 min_distance_to_selected = std::numeric_limits<f32>::max();
            for (u32 j = 0; j < reduced_count; ++j) {
                f32 dist = points[i].point.distance_to(reduced_points[j].point);
                min_distance_to_selected = std::min(min_distance_to_selected, dist);
            }
            
            if (min_distance_to_selected > max_distance) {
                max_distance = min_distance_to_selected;
                best_index = i;
            }
        }
        
        if (max_distance > constants::EPSILON) {
            reduced_points[reduced_count++] = points[best_index];
        }
    }
    
    // Copy reduced points back
    for (u32 i = 0; i < reduced_count; ++i) {
        points[i] = reduced_points[i];
    }
    point_count = reduced_count;
}

} // namespace ecscope::physics::collision3d