#include "ecscope/physics/narrow_phase.hpp"
#include <algorithm>
#include <cassert>

namespace ecscope::physics {

// Optimized GJK implementation with contact caching and warm starting
bool GJK::intersects_with_caching(const Shape& shape_a, const Transform3D& transform_a,
                                  const Shape& shape_b, const Transform3D& transform_b,
                                  GJKCache& cache, Simplex& out_simplex) {
    
    // Try warm starting with cached direction
    Vec3 direction = cache.last_direction;
    if (direction.length_squared() < PHYSICS_EPSILON) {
        direction = transform_b.position - transform_a.position;
        if (direction.length_squared() < PHYSICS_EPSILON) {
            direction = Vec3::unit_x();
        }
    }
    
    // Initialize simplex
    out_simplex.resize(0);
    
    // Get first support point
    SupportPoint support = get_support(shape_a, transform_a, shape_b, transform_b, direction);
    out_simplex.push_front(support);
    
    direction = support.point * -1;
    
    const int max_iterations = 32;
    for (int iterations = 0; iterations < max_iterations; ++iterations) {
        support = get_support(shape_a, transform_a, shape_b, transform_b, direction);
        
        if (support.point.dot(direction) <= 0) {
            // Cache the direction for next frame
            cache.last_direction = direction;
            cache.last_result = false;
            return false;  // No collision
        }
        
        out_simplex.push_front(support);
        
        if (out_simplex.contains_origin(direction)) {
            cache.last_direction = direction;
            cache.last_result = true;
            cache.last_simplex = out_simplex;
            return true;   // Collision detected
        }
    }
    
    cache.last_direction = direction;
    cache.last_result = false;
    return false;  // Max iterations reached, assume no collision
}

// Enhanced EPA with better contact point generation
ContactManifold EPA::get_contact_manifold_enhanced(const Shape& shape_a, const Transform3D& transform_a,
                                                  const Shape& shape_b, const Transform3D& transform_b,
                                                  const Simplex& simplex) {
    
    ContactManifold manifold(0, 0);  // IDs will be set by caller
    
    std::vector<SupportPoint> polytope;
    std::vector<EPAFace> faces;
    std::vector<EPAEdge> edges;
    
    // Initialize polytope with simplex
    for (size_t i = 0; i < simplex.size(); ++i) {
        polytope.push_back(simplex[i]);
    }
    
    // Build initial tetrahedron faces
    if (polytope.size() == 4) {
        add_face_enhanced(polytope, faces, 0, 1, 2);
        add_face_enhanced(polytope, faces, 0, 2, 3);
        add_face_enhanced(polytope, faces, 0, 3, 1);
        add_face_enhanced(polytope, faces, 1, 3, 2);
    }
    
    const int max_iterations = 64; // Increased for better accuracy
    const Real tolerance = 1e-6f;  // Tighter tolerance
    
    for (int iterations = 0; iterations < max_iterations; ++iterations) {
        // Find closest face
        auto closest_face_it = std::min_element(faces.begin(), faces.end(),
            [](const EPAFace& a, const EPAFace& b) {
                return a.distance < b.distance;
            });
        
        if (closest_face_it == faces.end()) break;
        
        Vec3 direction = closest_face_it->normal;
        SupportPoint new_support = get_support(shape_a, transform_a, shape_b, transform_b, direction);
        
        Real distance = new_support.point.dot(direction);
        
        // Check convergence
        if (distance - closest_face_it->distance < tolerance) {
            // Generate contact manifold with multiple contact points
            generate_contact_manifold(polytope, *closest_face_it, manifold);
            manifold.normal = closest_face_it->normal;
            break;
        }
        
        // Expand polytope
        size_t new_index = polytope.size();
        polytope.push_back(new_support);
        
        // Remove faces that can see the new point and collect boundary edges
        edges.clear();
        for (auto face_it = faces.begin(); face_it != faces.end();) {
            if (face_it->normal.dot(new_support.point - polytope[face_it->indices[0]].point) > PHYSICS_EPSILON) {
                // This face can see the new point, remove it and add its edges to boundary
                add_boundary_edge(edges, face_it->indices[0], face_it->indices[1]);
                add_boundary_edge(edges, face_it->indices[1], face_it->indices[2]);
                add_boundary_edge(edges, face_it->indices[2], face_it->indices[0]);
                face_it = faces.erase(face_it);
            } else {
                ++face_it;
            }
        }
        
        // Create new faces from boundary edges to new point
        for (const auto& edge : edges) {
            add_face_enhanced(polytope, faces, edge.a, edge.b, new_index);
        }
    }
    
    return manifold;
}

void EPA::add_face_enhanced(const std::vector<SupportPoint>& polytope, 
                           std::vector<EPAFace>& faces, 
                           size_t a, size_t b, size_t c) {
    Vec3 ab = polytope[b].point - polytope[a].point;
    Vec3 ac = polytope[c].point - polytope[a].point;
    Vec3 normal = ab.cross(ac).normalized();
    
    Real distance = normal.dot(polytope[a].point);
    if (distance < 0) {
        normal = normal * -1;
        distance = -distance;
        std::swap(b, c);
    }
    
    EPAFace face;
    face.indices[0] = a;
    face.indices[1] = b;
    face.indices[2] = c;
    face.normal = normal;
    face.distance = distance;
    
    faces.push_back(face);
}

void EPA::add_boundary_edge(std::vector<EPAEdge>& edges, size_t a, size_t b) {
    // Check if this edge already exists (reverse direction)
    for (auto it = edges.begin(); it != edges.end(); ++it) {
        if (it->a == b && it->b == a) {
            // Found matching edge in opposite direction, remove it
            edges.erase(it);
            return;
        }
    }
    
    // Add new boundary edge
    edges.emplace_back(a, b);
}

void EPA::generate_contact_manifold(const std::vector<SupportPoint>& polytope,
                                   const EPAFace& closest_face,
                                   ContactManifold& manifold) {
    // For now, generate a single contact point at the closest point on the face
    // In a full implementation, you would generate multiple contact points for stability
    
    Vec3 face_center = (polytope[closest_face.indices[0]].point +
                       polytope[closest_face.indices[1]].point +
                       polytope[closest_face.indices[2]].point) / 3.0f;
    
    ContactPoint contact;
    contact.normal = closest_face.normal;
    contact.penetration = closest_face.distance;
    
    // Project back to surface of each shape
    contact.world_position_a = face_center - contact.normal * (contact.penetration * 0.5f);
    contact.world_position_b = face_center + contact.normal * (contact.penetration * 0.5f);
    
    // Convert world positions to local coordinates (simplified)
    contact.position_a = contact.world_position_a;
    contact.position_b = contact.world_position_b;
    
    manifold.contacts.push_back(contact);
}

// Specialized collision detection for common shape pairs
bool test_sphere_sphere_optimized(const SphereShape& sphere_a, const Transform3D& transform_a,
                                 const SphereShape& sphere_b, const Transform3D& transform_b,
                                 ContactManifold& manifold) {
    Vec3 center_diff = transform_b.position - transform_a.position;
    Real distance_sq = center_diff.length_squared();
    Real combined_radius = sphere_a.radius + sphere_b.radius;
    Real combined_radius_sq = combined_radius * combined_radius;
    
    if (distance_sq >= combined_radius_sq) {
        return false; // No collision
    }
    
    Real distance = std::sqrt(distance_sq);
    
    ContactPoint contact;
    contact.penetration = combined_radius - distance;
    
    if (distance > PHYSICS_EPSILON) {
        contact.normal = center_diff / distance;
        contact.world_position_a = transform_a.position + contact.normal * sphere_a.radius;
        contact.world_position_b = transform_b.position - contact.normal * sphere_b.radius;
    } else {
        // Spheres are concentric
        contact.normal = Vec3::unit_x();
        contact.world_position_a = transform_a.position + contact.normal * sphere_a.radius;
        contact.world_position_b = transform_b.position - contact.normal * sphere_b.radius;
    }
    
    contact.position_a = contact.world_position_a;
    contact.position_b = contact.world_position_b;
    
    manifold.contacts.push_back(contact);
    manifold.normal = contact.normal;
    
    return true;
}

} // namespace ecscope::physics