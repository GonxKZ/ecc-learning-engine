#pragma once

#include "physics_math.hpp"
#include "rigid_body.hpp"
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <functional>
#include <array>

namespace ecscope::physics {

// Forward declarations
struct ContactManifold;
class Shape;

// AABB (Axis-Aligned Bounding Box)
struct AABB2D {
    Vec2 min, max;
    
    AABB2D() : min(Vec2::zero()), max(Vec2::zero()) {}
    AABB2D(const Vec2& min_point, const Vec2& max_point) : min(min_point), max(max_point) {}
    
    bool overlaps(const AABB2D& other) const {
        return !(max.x < other.min.x || min.x > other.max.x || 
                 max.y < other.min.y || min.y > other.max.y);
    }
    
    bool contains(const Vec2& point) const {
        return point.x >= min.x && point.x <= max.x && 
               point.y >= min.y && point.y <= max.y;
    }
    
    Vec2 center() const { return (min + max) * 0.5f; }
    Vec2 extents() const { return (max - min) * 0.5f; }
    Real area() const { return (max.x - min.x) * (max.y - min.y); }
    
    AABB2D expanded(Real amount) const {
        Vec2 expansion(amount, amount);
        return AABB2D(min - expansion, max + expansion);
    }
    
    static AABB2D merge(const AABB2D& a, const AABB2D& b) {
        return AABB2D(
            Vec2(std::min(a.min.x, b.min.x), std::min(a.min.y, b.min.y)),
            Vec2(std::max(a.max.x, b.max.x), std::max(a.max.y, b.max.y))
        );
    }
};

struct AABB3D {
    Vec3 min, max;
    
    AABB3D() : min(Vec3::zero()), max(Vec3::zero()) {}
    AABB3D(const Vec3& min_point, const Vec3& max_point) : min(min_point), max(max_point) {}
    
    bool overlaps(const AABB3D& other) const {
        return !(max.x < other.min.x || min.x > other.max.x || 
                 max.y < other.min.y || min.y > other.max.y ||
                 max.z < other.min.z || min.z > other.max.z);
    }
    
    bool contains(const Vec3& point) const {
        return point.x >= min.x && point.x <= max.x && 
               point.y >= min.y && point.y <= max.y &&
               point.z >= min.z && point.z <= max.z;
    }
    
    Vec3 center() const { return (min + max) * 0.5f; }
    Vec3 extents() const { return (max - min) * 0.5f; }
    Real volume() const { return (max.x - min.x) * (max.y - min.y) * (max.z - min.z); }
    
    AABB3D expanded(Real amount) const {
        Vec3 expansion(amount, amount, amount);
        return AABB3D(min - expansion, max + expansion);
    }
    
    static AABB3D merge(const AABB3D& a, const AABB3D& b) {
        return AABB3D(
            Vec3(std::min(a.min.x, b.min.x), std::min(a.min.y, b.min.y), std::min(a.min.z, b.min.z)),
            Vec3(std::max(a.max.x, b.max.x), std::max(a.max.y, b.max.y), std::max(a.max.z, b.max.z))
        );
    }
};

// Collision shapes base class
class Shape {
public:
    enum Type {
        Circle, Box, Polygon, Sphere, BoxShape, ConvexMesh
    };
    
    Type type;
    
    explicit Shape(Type shape_type) : type(shape_type) {}
    virtual ~Shape() = default;
    
    virtual AABB2D get_aabb_2d(const Transform2D& transform) const = 0;
    virtual AABB3D get_aabb_3d(const Transform3D& transform) const = 0;
    virtual Vec2 get_support_point_2d(const Vec2& direction, const Transform2D& transform) const = 0;
    virtual Vec3 get_support_point_3d(const Vec3& direction, const Transform3D& transform) const = 0;
    virtual Real get_mass_factor() const = 0;
};

// 2D Shapes
class CircleShape : public Shape {
public:
    Real radius;
    
    explicit CircleShape(Real r) : Shape(Circle), radius(r) {}
    
    AABB2D get_aabb_2d(const Transform2D& transform) const override {
        Vec2 r_vec(radius, radius);
        return AABB2D(transform.position - r_vec, transform.position + r_vec);
    }
    
    AABB3D get_aabb_3d(const Transform3D& transform) const override {
        Vec3 r_vec(radius, radius, 0);
        return AABB3D(transform.position - r_vec, transform.position + r_vec);
    }
    
    Vec2 get_support_point_2d(const Vec2& direction, const Transform2D& transform) const override {
        Vec2 normalized = direction.normalized();
        return transform.position + normalized * radius;
    }
    
    Vec3 get_support_point_3d(const Vec3& direction, const Transform3D& transform) const override {
        Vec3 dir_2d(direction.x, direction.y, 0);
        Vec3 normalized = dir_2d.normalized();
        return transform.position + normalized * radius;
    }
    
    Real get_mass_factor() const override {
        return PI * radius * radius;
    }
};

class BoxShape2D : public Shape {
public:
    Vec2 half_extents;
    
    explicit BoxShape2D(const Vec2& extents) : Shape(Box), half_extents(extents) {}
    
    AABB2D get_aabb_2d(const Transform2D& transform) const override {
        // For rotated boxes, we need to calculate the actual extents
        Vec2 corners[4] = {
            Vec2(-half_extents.x, -half_extents.y),
            Vec2( half_extents.x, -half_extents.y),
            Vec2( half_extents.x,  half_extents.y),
            Vec2(-half_extents.x,  half_extents.y)
        };
        
        Vec2 min_point = transform.transform_point(corners[0]);
        Vec2 max_point = min_point;
        
        for (int i = 1; i < 4; ++i) {
            Vec2 corner = transform.transform_point(corners[i]);
            min_point.x = std::min(min_point.x, corner.x);
            min_point.y = std::min(min_point.y, corner.y);
            max_point.x = std::max(max_point.x, corner.x);
            max_point.y = std::max(max_point.y, corner.y);
        }
        
        return AABB2D(min_point, max_point);
    }
    
    AABB3D get_aabb_3d(const Transform3D& transform) const override {
        Vec3 extents_3d(half_extents.x, half_extents.y, 0);
        return AABB3D(transform.position - extents_3d, transform.position + extents_3d);
    }
    
    Vec2 get_support_point_2d(const Vec2& direction, const Transform2D& transform) const override {
        Vec2 local_dir = Vec2(
            direction.x * std::cos(-transform.rotation) - direction.y * std::sin(-transform.rotation),
            direction.x * std::sin(-transform.rotation) + direction.y * std::cos(-transform.rotation)
        );
        
        Vec2 support(
            local_dir.x >= 0 ? half_extents.x : -half_extents.x,
            local_dir.y >= 0 ? half_extents.y : -half_extents.y
        );
        
        return transform.transform_point(support);
    }
    
    Vec3 get_support_point_3d(const Vec3& direction, const Transform3D& transform) const override {
        Vec2 dir_2d(direction.x, direction.y);
        return Vec3(get_support_point_2d(dir_2d, Transform2D(Vec2(transform.position.x, transform.position.y), 0)));
    }
    
    Real get_mass_factor() const override {
        return 4.0f * half_extents.x * half_extents.y;
    }
};

// 3D Shapes
class SphereShape : public Shape {
public:
    Real radius;
    
    explicit SphereShape(Real r) : Shape(Sphere), radius(r) {}
    
    AABB2D get_aabb_2d(const Transform2D& transform) const override {
        Vec2 r_vec(radius, radius);
        return AABB2D(transform.position - r_vec, transform.position + r_vec);
    }
    
    AABB3D get_aabb_3d(const Transform3D& transform) const override {
        Vec3 r_vec(radius, radius, radius);
        return AABB3D(transform.position - r_vec, transform.position + r_vec);
    }
    
    Vec2 get_support_point_2d(const Vec2& direction, const Transform2D& transform) const override {
        Vec2 normalized = direction.normalized();
        return transform.position + normalized * radius;
    }
    
    Vec3 get_support_point_3d(const Vec3& direction, const Transform3D& transform) const override {
        Vec3 normalized = direction.normalized();
        return transform.position + normalized * radius;
    }
    
    Real get_mass_factor() const override {
        return (4.0f / 3.0f) * PI * radius * radius * radius;
    }
};

class BoxShape3D : public Shape {
public:
    Vec3 half_extents;
    
    explicit BoxShape3D(const Vec3& extents) : Shape(BoxShape), half_extents(extents) {}
    
    AABB2D get_aabb_2d(const Transform2D& transform) const override {
        Vec2 extents_2d(half_extents.x, half_extents.y);
        return AABB2D(transform.position - extents_2d, transform.position + extents_2d);
    }
    
    AABB3D get_aabb_3d(const Transform3D& transform) const override {
        // For rotated boxes, calculate actual extents
        Vec3 corners[8];
        int idx = 0;
        for (int x = -1; x <= 1; x += 2) {
            for (int y = -1; y <= 1; y += 2) {
                for (int z = -1; z <= 1; z += 2) {
                    corners[idx++] = Vec3(x * half_extents.x, y * half_extents.y, z * half_extents.z);
                }
            }
        }
        
        Vec3 min_point = transform.transform_point(corners[0]);
        Vec3 max_point = min_point;
        
        for (int i = 1; i < 8; ++i) {
            Vec3 corner = transform.transform_point(corners[i]);
            min_point.x = std::min(min_point.x, corner.x);
            min_point.y = std::min(min_point.y, corner.y);
            min_point.z = std::min(min_point.z, corner.z);
            max_point.x = std::max(max_point.x, corner.x);
            max_point.y = std::max(max_point.y, corner.y);
            max_point.z = std::max(max_point.z, corner.z);
        }
        
        return AABB3D(min_point, max_point);
    }
    
    Vec2 get_support_point_2d(const Vec2& direction, const Transform2D& transform) const override {
        return transform.position; // Simplified for 2D projection
    }
    
    Vec3 get_support_point_3d(const Vec3& direction, const Transform3D& transform) const override {
        // Transform direction to local space
        Vec3 local_dir = transform.rotation.normalized();
        // This is simplified - full implementation would inverse transform the direction
        
        Vec3 support(
            direction.x >= 0 ? half_extents.x : -half_extents.x,
            direction.y >= 0 ? half_extents.y : -half_extents.y,
            direction.z >= 0 ? half_extents.z : -half_extents.z
        );
        
        return transform.transform_point(support);
    }
    
    Real get_mass_factor() const override {
        return 8.0f * half_extents.x * half_extents.y * half_extents.z;
    }
};

// Collision pair for broad phase
struct CollisionPair {
    uint32_t id_a, id_b;
    
    CollisionPair(uint32_t a, uint32_t b) : id_a(std::min(a, b)), id_b(std::max(a, b)) {}
    
    bool operator==(const CollisionPair& other) const {
        return id_a == other.id_a && id_b == other.id_b;
    }
};

// Hash function for collision pairs
struct CollisionPairHash {
    size_t operator()(const CollisionPair& pair) const {
        return std::hash<uint64_t>{}((uint64_t(pair.id_a) << 32) | pair.id_b);
    }
};

// High-performance spatial hash for broad phase collision detection
template<typename T>
class SpatialHash {
public:
    struct Entry {
        uint32_t id;
        T aabb;
        void* user_data;
    };
    
private:
    Real cell_size;
    std::unordered_map<uint64_t, std::vector<Entry>> grid;
    
    uint64_t hash_position(int x, int y) const {
        // Use a good hash function for spatial coordinates
        const uint64_t p1 = 73856093;
        const uint64_t p2 = 19349663;
        return (uint64_t(x) * p1) ^ (uint64_t(y) * p2);
    }
    
    uint64_t hash_position_3d(int x, int y, int z) const {
        const uint64_t p1 = 73856093;
        const uint64_t p2 = 19349663;
        const uint64_t p3 = 83492791;
        return (uint64_t(x) * p1) ^ (uint64_t(y) * p2) ^ (uint64_t(z) * p3);
    }
    
public:
    explicit SpatialHash(Real cell_sz = 10.0f) : cell_size(cell_sz) {}
    
    void clear() {
        grid.clear();
    }
    
    void insert(uint32_t id, const T& aabb, void* user_data = nullptr) {
        if constexpr (std::is_same_v<T, AABB2D>) {
            int min_x = static_cast<int>(std::floor(aabb.min.x / cell_size));
            int min_y = static_cast<int>(std::floor(aabb.min.y / cell_size));
            int max_x = static_cast<int>(std::floor(aabb.max.x / cell_size));
            int max_y = static_cast<int>(std::floor(aabb.max.y / cell_size));
            
            for (int x = min_x; x <= max_x; ++x) {
                for (int y = min_y; y <= max_y; ++y) {
                    uint64_t hash = hash_position(x, y);
                    grid[hash].emplace_back(Entry{id, aabb, user_data});
                }
            }
        } else {
            int min_x = static_cast<int>(std::floor(aabb.min.x / cell_size));
            int min_y = static_cast<int>(std::floor(aabb.min.y / cell_size));
            int min_z = static_cast<int>(std::floor(aabb.min.z / cell_size));
            int max_x = static_cast<int>(std::floor(aabb.max.x / cell_size));
            int max_y = static_cast<int>(std::floor(aabb.max.y / cell_size));
            int max_z = static_cast<int>(std::floor(aabb.max.z / cell_size));
            
            for (int x = min_x; x <= max_x; ++x) {
                for (int y = min_y; y <= max_y; ++y) {
                    for (int z = min_z; z <= max_z; ++z) {
                        uint64_t hash = hash_position_3d(x, y, z);
                        grid[hash].emplace_back(Entry{id, aabb, user_data});
                    }
                }
            }
        }
    }
    
    std::vector<CollisionPair> find_collision_pairs() const {
        std::unordered_set<CollisionPair, CollisionPairHash> unique_pairs;
        
        for (const auto& cell : grid) {
            const auto& entries = cell.second;
            
            // Check all pairs within this cell
            for (size_t i = 0; i < entries.size(); ++i) {
                for (size_t j = i + 1; j < entries.size(); ++j) {
                    if (entries[i].aabb.overlaps(entries[j].aabb)) {
                        unique_pairs.emplace(entries[i].id, entries[j].id);
                    }
                }
            }
        }
        
        return std::vector<CollisionPair>(unique_pairs.begin(), unique_pairs.end());
    }
    
    std::vector<Entry> query(const T& query_aabb) const {
        std::vector<Entry> results;
        std::unordered_set<uint32_t> seen_ids;
        
        if constexpr (std::is_same_v<T, AABB2D>) {
            int min_x = static_cast<int>(std::floor(query_aabb.min.x / cell_size));
            int min_y = static_cast<int>(std::floor(query_aabb.min.y / cell_size));
            int max_x = static_cast<int>(std::floor(query_aabb.max.x / cell_size));
            int max_y = static_cast<int>(std::floor(query_aabb.max.y / cell_size));
            
            for (int x = min_x; x <= max_x; ++x) {
                for (int y = min_y; y <= max_y; ++y) {
                    uint64_t hash = hash_position(x, y);
                    auto it = grid.find(hash);
                    if (it != grid.end()) {
                        for (const auto& entry : it->second) {
                            if (seen_ids.find(entry.id) == seen_ids.end() &&
                                query_aabb.overlaps(entry.aabb)) {
                                results.push_back(entry);
                                seen_ids.insert(entry.id);
                            }
                        }
                    }
                }
            }
        } else {
            // 3D implementation similar to 2D
            int min_x = static_cast<int>(std::floor(query_aabb.min.x / cell_size));
            int min_y = static_cast<int>(std::floor(query_aabb.min.y / cell_size));
            int min_z = static_cast<int>(std::floor(query_aabb.min.z / cell_size));
            int max_x = static_cast<int>(std::floor(query_aabb.max.x / cell_size));
            int max_y = static_cast<int>(std::floor(query_aabb.max.y / cell_size));
            int max_z = static_cast<int>(std::floor(query_aabb.max.z / cell_size));
            
            for (int x = min_x; x <= max_x; ++x) {
                for (int y = min_y; y <= max_y; ++y) {
                    for (int z = min_z; z <= max_z; ++z) {
                        uint64_t hash = hash_position_3d(x, y, z);
                        auto it = grid.find(hash);
                        if (it != grid.end()) {
                            for (const auto& entry : it->second) {
                                if (seen_ids.find(entry.id) == seen_ids.end() &&
                                    query_aabb.overlaps(entry.aabb)) {
                                    results.push_back(entry);
                                    seen_ids.insert(entry.id);
                                }
                            }
                        }
                    }
                }
            }
        }
        
        return results;
    }
    
    size_t get_memory_usage() const {
        size_t total_size = grid.size() * (sizeof(uint64_t) + sizeof(std::vector<Entry>));
        for (const auto& cell : grid) {
            total_size += cell.second.capacity() * sizeof(Entry);
        }
        return total_size;
    }
    
    void set_cell_size(Real new_cell_size) {
        cell_size = new_cell_size;
        // Note: This would invalidate current data, so call clear() after
    }
};

// Broad phase collision detection system
class BroadPhaseCollisionDetection {
private:
    SpatialHash<AABB2D> spatial_hash_2d;
    SpatialHash<AABB3D> spatial_hash_3d;
    std::vector<CollisionPair> current_pairs;
    
    // Performance tracking
    size_t total_objects = 0;
    size_t total_pairs_generated = 0;
    Real last_update_time = 0.0f;
    
public:
    explicit BroadPhaseCollisionDetection(Real cell_size = 10.0f) 
        : spatial_hash_2d(cell_size), spatial_hash_3d(cell_size) {}
    
    void clear() {
        spatial_hash_2d.clear();
        spatial_hash_3d.clear();
        current_pairs.clear();
        total_objects = 0;
    }
    
    void add_body_2d(const RigidBody2D& body, const Shape& shape) {
        AABB2D aabb = shape.get_aabb_2d(body.transform);
        // Expand AABB slightly for continuous collision detection
        aabb = aabb.expanded(0.1f);
        spatial_hash_2d.insert(body.id, aabb, const_cast<RigidBody2D*>(&body));
        ++total_objects;
    }
    
    void add_body_3d(const RigidBody3D& body, const Shape& shape) {
        AABB3D aabb = shape.get_aabb_3d(body.transform);
        aabb = aabb.expanded(0.1f);
        spatial_hash_3d.insert(body.id, aabb, const_cast<RigidBody3D*>(&body));
        ++total_objects;
    }
    
    const std::vector<CollisionPair>& find_collision_pairs_2d() {
        current_pairs = spatial_hash_2d.find_collision_pairs();
        total_pairs_generated = current_pairs.size();
        return current_pairs;
    }
    
    const std::vector<CollisionPair>& find_collision_pairs_3d() {
        current_pairs = spatial_hash_3d.find_collision_pairs();
        total_pairs_generated = current_pairs.size();
        return current_pairs;
    }
    
    // Performance metrics
    Real get_efficiency_ratio() const {
        if (total_objects < 2) return 1.0f;
        size_t max_possible_pairs = total_objects * (total_objects - 1) / 2;
        return Real(total_pairs_generated) / Real(max_possible_pairs);
    }
    
    size_t get_memory_usage() const {
        return spatial_hash_2d.get_memory_usage() + spatial_hash_3d.get_memory_usage();
    }
    
    void set_cell_size(Real cell_size) {
        spatial_hash_2d.set_cell_size(cell_size);
        spatial_hash_3d.set_cell_size(cell_size);
    }
    
    // Debug info
    struct Stats {
        size_t total_objects;
        size_t total_pairs;
        Real efficiency_ratio;
        size_t memory_usage_bytes;
    };
    
    Stats get_stats() const {
        return {
            total_objects,
            total_pairs_generated,
            get_efficiency_ratio(),
            get_memory_usage()
        };
    }
};

} // namespace ecscope::physics