#pragma once

#include "physics_math.hpp"
#include "collision_detection.hpp"
#include "rigid_body.hpp"
#include <vector>
#include <array>
#include <limits>

namespace ecscope::physics {

// Contact point information
struct ContactPoint {
    Vec3 position_a;    // Contact point on body A (local coordinates)
    Vec3 position_b;    // Contact point on body B (local coordinates)
    Vec3 world_position_a; // Contact point on body A (world coordinates)
    Vec3 world_position_b; // Contact point on body B (world coordinates)
    Vec3 normal;        // Contact normal (from A to B)
    Real penetration;   // Penetration depth
    Real normal_impulse = 0.0f;     // Accumulated normal impulse
    Real tangent_impulse = 0.0f;    // Accumulated tangent impulse
    Real bias_impulse = 0.0f;       // Bias impulse for position correction
    
    ContactPoint() = default;
    ContactPoint(const Vec3& pos_a, const Vec3& pos_b, const Vec3& normal_dir, Real depth)
        : position_a(pos_a), position_b(pos_b), normal(normal_dir), penetration(depth) {}
};

// Contact manifold for a pair of colliding bodies
struct ContactManifold {
    uint32_t body_a_id;
    uint32_t body_b_id;
    std::vector<ContactPoint> contacts;
    Vec3 normal;        // Primary contact normal
    Real friction;      // Combined friction coefficient
    Real restitution;   // Combined restitution coefficient
    
    ContactManifold(uint32_t a_id, uint32_t b_id) : body_a_id(a_id), body_b_id(b_id) {}
    
    void add_contact(const ContactPoint& contact) {
        // Limit maximum contacts per manifold (typically 4 for stability)
        if (contacts.size() >= 4) {
            // Replace the contact with least penetration
            auto min_it = std::min_element(contacts.begin(), contacts.end(),
                [](const ContactPoint& a, const ContactPoint& b) {
                    return a.penetration < b.penetration;
                });
            if (contact.penetration > min_it->penetration) {
                *min_it = contact;
            }
        } else {
            contacts.push_back(contact);
        }
    }
    
    bool is_valid() const {
        return !contacts.empty() && contacts[0].penetration > PHYSICS_EPSILON;
    }
};

// Support point for GJK algorithm
struct SupportPoint {
    Vec3 point;
    Vec3 support_a, support_b;
    
    SupportPoint() = default;
    SupportPoint(const Vec3& p, const Vec3& sa, const Vec3& sb) 
        : point(p), support_a(sa), support_b(sb) {}
};

// Simplex for GJK algorithm
class Simplex {
private:
    std::array<SupportPoint, 4> points;
    size_t count = 0;
    
public:
    void push_front(const SupportPoint& point) {
        for (size_t i = count; i > 0; --i) {
            points[i] = points[i - 1];
        }
        points[0] = point;
        count = std::min(count + 1, size_t(4));
    }
    
    SupportPoint& operator[](size_t index) { return points[index]; }
    const SupportPoint& operator[](size_t index) const { return points[index]; }
    
    size_t size() const { return count; }
    void resize(size_t new_size) { count = std::min(new_size, size_t(4)); }
    
    bool contains_origin(Vec3& direction) {
        switch (count) {
            case 2: return line_case(direction);
            case 3: return triangle_case(direction);
            case 4: return tetrahedron_case(direction);
            default: return false;
        }
    }
    
private:
    bool line_case(Vec3& direction) {
        Vec3 a = points[0].point;
        Vec3 b = points[1].point;
        Vec3 ab = b - a;
        Vec3 ao = Vec3::zero() - a;
        
        if (ab.dot(ao) > 0) {
            direction = ab.cross(ao).cross(ab);
        } else {
            points[1] = points[0];
            count = 1;
            direction = ao;
        }
        return false;
    }
    
    bool triangle_case(Vec3& direction) {
        Vec3 a = points[0].point;
        Vec3 b = points[1].point;
        Vec3 c = points[2].point;
        Vec3 ab = b - a;
        Vec3 ac = c - a;
        Vec3 ao = Vec3::zero() - a;
        Vec3 abc = ab.cross(ac);
        
        if (abc.cross(ac).dot(ao) > 0) {
            if (ac.dot(ao) > 0) {
                points[1] = points[0];
                count = 2;
                direction = ac.cross(ao).cross(ac);
            } else {
                return line_case(direction);
            }
        } else {
            if (ab.cross(abc).dot(ao) > 0) {
                return line_case(direction);
            } else {
                if (abc.dot(ao) > 0) {
                    direction = abc;
                } else {
                    std::swap(points[1], points[2]);
                    direction = abc * -1;
                }
            }
        }
        return false;
    }
    
    bool tetrahedron_case(Vec3& direction) {
        Vec3 a = points[0].point;
        Vec3 b = points[1].point;
        Vec3 c = points[2].point;
        Vec3 d = points[3].point;
        Vec3 ab = b - a;
        Vec3 ac = c - a;
        Vec3 ad = d - a;
        Vec3 ao = Vec3::zero() - a;
        Vec3 abc = ab.cross(ac);
        Vec3 acd = ac.cross(ad);
        Vec3 adb = ad.cross(ab);
        
        if (abc.dot(ao) > 0) {
            points[3] = points[0];
            count = 3;
            return triangle_case(direction);
        }
        
        if (acd.dot(ao) > 0) {
            points[1] = points[2];
            points[2] = points[3];
            points[3] = points[0];
            count = 3;
            return triangle_case(direction);
        }
        
        if (adb.dot(ao) > 0) {
            points[2] = points[1];
            points[1] = points[3];
            points[3] = points[0];
            count = 3;
            return triangle_case(direction);
        }
        
        return true;
    }
};

// Edge for EPA algorithm
struct EPAEdge {
    size_t a, b;
    Vec3 normal;
    Real distance;
    
    EPAEdge(size_t idx_a, size_t idx_b, const Vec3& norm, Real dist)
        : a(idx_a), b(idx_b), normal(norm), distance(dist) {}
};

// GJK (Gilbert-Johnson-Keerthi) algorithm for collision detection
class GJK {
public:
    static bool intersects(const Shape& shape_a, const Transform3D& transform_a,
                          const Shape& shape_b, const Transform3D& transform_b,
                          Simplex& out_simplex) {
        
        // Initial search direction
        Vec3 direction = transform_b.position - transform_a.position;
        if (direction.length_squared() < PHYSICS_EPSILON) {
            direction = Vec3::unit_x();
        }
        
        // Get first support point
        SupportPoint support = get_support(shape_a, transform_a, shape_b, transform_b, direction);
        out_simplex.push_front(support);
        
        direction = support.point * -1;
        
        const int max_iterations = 32;
        for (int iterations = 0; iterations < max_iterations; ++iterations) {
            support = get_support(shape_a, transform_a, shape_b, transform_b, direction);
            
            if (support.point.dot(direction) <= 0) {
                return false;  // No collision
            }
            
            out_simplex.push_front(support);
            
            if (out_simplex.contains_origin(direction)) {
                return true;   // Collision detected
            }
        }
        
        return false;  // Max iterations reached, assume no collision
    }
    
    // 2D version of GJK
    static bool intersects_2d(const Shape& shape_a, const Transform2D& transform_a,
                             const Shape& shape_b, const Transform2D& transform_b) {
        
        Vec2 direction = transform_b.position - transform_a.position;
        if (direction.length_squared() < PHYSICS_EPSILON) {
            direction = Vec2::unit_x();
        }
        
        std::vector<Vec2> simplex;
        
        // Get first support point
        Vec2 support_a = shape_a.get_support_point_2d(direction, transform_a);
        Vec2 support_b = shape_b.get_support_point_2d(direction * -1, transform_b);
        Vec2 support = support_a - support_b;
        simplex.push_back(support);
        
        direction = support * -1;
        
        const int max_iterations = 32;
        for (int iterations = 0; iterations < max_iterations; ++iterations) {
            support_a = shape_a.get_support_point_2d(direction, transform_a);
            support_b = shape_b.get_support_point_2d(direction * -1, transform_b);
            support = support_a - support_b;
            
            if (support.dot(direction) <= 0) {
                return false;  // No collision
            }
            
            simplex.insert(simplex.begin(), support);
            
            if (contains_origin_2d(simplex, direction)) {
                return true;   // Collision detected
            }
        }
        
        return false;
    }
    
private:
    static SupportPoint get_support(const Shape& shape_a, const Transform3D& transform_a,
                                   const Shape& shape_b, const Transform3D& transform_b,
                                   const Vec3& direction) {
        Vec3 support_a = shape_a.get_support_point_3d(direction, transform_a);
        Vec3 support_b = shape_b.get_support_point_3d(direction * -1, transform_b);
        return SupportPoint(support_a - support_b, support_a, support_b);
    }
    
    static bool contains_origin_2d(std::vector<Vec2>& simplex, Vec2& direction) {
        if (simplex.size() == 2) {
            Vec2 a = simplex[0];
            Vec2 b = simplex[1];
            Vec2 ab = b - a;
            Vec2 ao = a * -1;
            
            if (ab.dot(ao) > 0) {
                direction = Vec2(-ab.y, ab.x);
                if (direction.dot(ao) < 0) {
                    direction = direction * -1;
                }
            } else {
                simplex = {a};
                direction = ao;
            }
            return false;
        }
        
        if (simplex.size() == 3) {
            Vec2 a = simplex[0];
            Vec2 b = simplex[1];
            Vec2 c = simplex[2];
            Vec2 ab = b - a;
            Vec2 ac = c - a;
            Vec2 ao = a * -1;
            
            Vec2 ab_perp(-ab.y, ab.x);
            if (ab_perp.dot(ac) < 0) ab_perp = ab_perp * -1;
            
            Vec2 ac_perp(-ac.y, ac.x);
            if (ac_perp.dot(ab) < 0) ac_perp = ac_perp * -1;
            
            if (ab_perp.dot(ao) > 0) {
                simplex = {a, b};
                direction = ab_perp;
                return false;
            }
            
            if (ac_perp.dot(ao) > 0) {
                simplex = {a, c};
                direction = ac_perp;
                return false;
            }
            
            return true;
        }
        
        return false;
    }
};

// EPA (Expanding Polytope Algorithm) for contact generation
class EPA {
public:
    static ContactManifold get_contact_manifold(const Shape& shape_a, const Transform3D& transform_a,
                                               const Shape& shape_b, const Transform3D& transform_b,
                                               const Simplex& simplex) {
        
        ContactManifold manifold(0, 0);  // IDs will be set by caller
        
        std::vector<SupportPoint> polytope;
        std::vector<EPAEdge> edges;
        
        // Initialize polytope with simplex
        for (size_t i = 0; i < simplex.size(); ++i) {
            polytope.push_back(simplex[i]);
        }
        
        // Add initial faces for tetrahedron
        if (polytope.size() == 4) {
            add_face(polytope, edges, 0, 1, 2);
            add_face(polytope, edges, 0, 2, 3);
            add_face(polytope, edges, 0, 3, 1);
            add_face(polytope, edges, 1, 3, 2);
        }
        
        const int max_iterations = 32;
        const Real tolerance = 1e-4f;
        
        for (int iterations = 0; iterations < max_iterations; ++iterations) {
            // Find closest edge
            auto closest_edge = std::min_element(edges.begin(), edges.end(),
                [](const EPAEdge& a, const EPAEdge& b) {
                    return a.distance < b.distance;
                });
            
            if (closest_edge == edges.end()) break;
            
            Vec3 direction = closest_edge->normal;
            SupportPoint new_support = get_support(shape_a, transform_a, shape_b, transform_b, direction);
            
            Real distance = new_support.point.dot(direction);
            
            // Check if we've reached the surface
            if (distance - closest_edge->distance < tolerance) {
                // Generate contact point
                ContactPoint contact;
                contact.normal = closest_edge->normal;
                contact.penetration = distance;
                
                // Barycentric coordinates for contact point
                Vec3 closest_point = closest_edge->normal * closest_edge->distance;
                contact.world_position_a = transform_a.position + closest_point;
                contact.world_position_b = transform_b.position + closest_point - contact.normal * contact.penetration;
                
                manifold.add_contact(contact);
                manifold.normal = contact.normal;
                break;
            }
            
            // Expand polytope
            size_t new_index = polytope.size();
            polytope.push_back(new_support);
            
            // Remove edges that can see the new point
            std::vector<EPAEdge> new_edges;
            for (const auto& edge : edges) {
                Vec3 to_new_point = new_support.point - polytope[edge.a].point;
                if (edge.normal.dot(to_new_point) <= 0) {
                    new_edges.push_back(edge);
                } else {
                    // Add new faces
                    add_face_from_edge(polytope, new_edges, edge, new_index);
                }
            }
            edges = std::move(new_edges);
        }
        
        return manifold;
    }
    
private:
    static SupportPoint get_support(const Shape& shape_a, const Transform3D& transform_a,
                                   const Shape& shape_b, const Transform3D& transform_b,
                                   const Vec3& direction) {
        Vec3 support_a = shape_a.get_support_point_3d(direction, transform_a);
        Vec3 support_b = shape_b.get_support_point_3d(direction * -1, transform_b);
        return SupportPoint(support_a - support_b, support_a, support_b);
    }
    
    static void add_face(const std::vector<SupportPoint>& polytope, 
                        std::vector<EPAEdge>& edges, 
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
        
        edges.emplace_back(a, b, normal, distance);
        edges.emplace_back(b, c, normal, distance);
        edges.emplace_back(c, a, normal, distance);
    }
    
    static void add_face_from_edge(const std::vector<SupportPoint>& polytope,
                                  std::vector<EPAEdge>& edges,
                                  const EPAEdge& edge, size_t new_point) {
        Vec3 ab = polytope[edge.b].point - polytope[edge.a].point;
        Vec3 ac = polytope[new_point].point - polytope[edge.a].point;
        Vec3 normal = ab.cross(ac).normalized();
        
        Real distance = normal.dot(polytope[edge.a].point);
        if (distance < 0) {
            normal = normal * -1;
            distance = -distance;
        }
        
        edges.emplace_back(edge.a, edge.b, normal, distance);
        edges.emplace_back(edge.b, new_point, normal, distance);
        edges.emplace_back(new_point, edge.a, normal, distance);
    }
};

// Narrow phase collision detection system
class NarrowPhaseCollisionDetection {
public:
    struct CollisionInfo {
        bool is_colliding;
        ContactManifold manifold;
        
        CollisionInfo(bool colliding = false) : is_colliding(colliding), manifold(0, 0) {}
    };
    
    // Test collision between two 2D bodies
    static CollisionInfo test_collision_2d(const RigidBody2D& body_a, const Shape& shape_a,
                                          const RigidBody2D& body_b, const Shape& shape_b) {
        CollisionInfo info;
        info.manifold.body_a_id = body_a.id;
        info.manifold.body_b_id = body_b.id;
        
        // Use GJK for collision detection
        info.is_colliding = GJK::intersects_2d(shape_a, body_a.transform, shape_b, body_b.transform);
        
        if (info.is_colliding) {
            // Generate contact manifold for 2D (simplified)
            generate_contact_manifold_2d(body_a, shape_a, body_b, shape_b, info.manifold);
        }
        
        return info;
    }
    
    // Test collision between two 3D bodies
    static CollisionInfo test_collision_3d(const RigidBody3D& body_a, const Shape& shape_a,
                                          const RigidBody3D& body_b, const Shape& shape_b) {
        CollisionInfo info;
        info.manifold.body_a_id = body_a.id;
        info.manifold.body_b_id = body_b.id;
        
        Simplex simplex;
        info.is_colliding = GJK::intersects(shape_a, body_a.transform, shape_b, body_b.transform, simplex);
        
        if (info.is_colliding) {
            // Use EPA to generate detailed contact information
            info.manifold = EPA::get_contact_manifold(shape_a, body_a.transform, shape_b, body_b.transform, simplex);
            info.manifold.body_a_id = body_a.id;
            info.manifold.body_b_id = body_b.id;
            
            // Combine material properties
            info.manifold.friction = std::sqrt(body_a.material.friction * body_b.material.friction);
            info.manifold.restitution = std::max(body_a.material.restitution, body_b.material.restitution);
        }
        
        return info;
    }
    
private:
    // Generate contact manifold for 2D shapes (simplified implementation)
    static void generate_contact_manifold_2d(const RigidBody2D& body_a, const Shape& shape_a,
                                            const RigidBody2D& body_b, const Shape& shape_b,
                                            ContactManifold& manifold) {
        // This is a simplified 2D contact generation
        // In a full implementation, you'd have specific algorithms for each shape type combination
        
        Vec2 direction = body_b.transform.position - body_a.transform.position;
        if (direction.length_squared() < PHYSICS_EPSILON) {
            direction = Vec2::unit_x();
        }
        direction = direction.normalized();
        
        Vec2 support_a = shape_a.get_support_point_2d(direction, body_a.transform);
        Vec2 support_b = shape_b.get_support_point_2d(direction * -1, body_b.transform);
        
        Real penetration = (support_a - support_b).length();
        
        ContactPoint contact;
        contact.world_position_a = Vec3(support_a.x, support_a.y, 0);
        contact.world_position_b = Vec3(support_b.x, support_b.y, 0);
        contact.normal = Vec3(direction.x, direction.y, 0);
        contact.penetration = penetration;
        
        manifold.add_contact(contact);
        manifold.normal = contact.normal;
        manifold.friction = std::sqrt(body_a.material.friction * body_b.material.friction);
        manifold.restitution = std::max(body_a.material.restitution, body_b.material.restitution);
    }
};

} // namespace ecscope::physics