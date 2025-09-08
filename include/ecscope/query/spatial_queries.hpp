#pragma once

#include "../core/types.hpp"
#include "../core/log.hpp"

#include <vector>
#include <memory>
#include <algorithm>
#include <cmath>
#include <limits>
#include <functional>
#include <unordered_map>
#include <tuple>
#include <type_traits>

namespace ecscope::ecs::query::spatial {

/**
 * @brief 3D Vector for spatial calculations
 */
struct Vec3 {
    f32 x, y, z;
    
    Vec3() : x(0.0f), y(0.0f), z(0.0f) {}
    Vec3(f32 x_, f32 y_, f32 z_) : x(x_), y(y_), z(z_) {}
    
    Vec3 operator+(const Vec3& other) const {
        return Vec3(x + other.x, y + other.y, z + other.z);
    }
    
    Vec3 operator-(const Vec3& other) const {
        return Vec3(x - other.x, y - other.y, z - other.z);
    }
    
    Vec3 operator*(f32 scalar) const {
        return Vec3(x * scalar, y * scalar, z * scalar);
    }
    
    f32 dot(const Vec3& other) const {
        return x * other.x + y * other.y + z * other.z;
    }
    
    f32 length_squared() const {
        return x * x + y * y + z * z;
    }
    
    f32 length() const {
        return std::sqrt(length_squared());
    }
    
    Vec3 normalized() const {
        f32 len = length();
        return len > 0.0f ? (*this * (1.0f / len)) : Vec3();
    }
};

/**
 * @brief Axis-Aligned Bounding Box
 */
struct AABB {
    Vec3 min, max;
    
    AABB() : min(Vec3()), max(Vec3()) {}
    AABB(const Vec3& min_, const Vec3& max_) : min(min_), max(max_) {}
    
    bool contains(const Vec3& point) const {
        return point.x >= min.x && point.x <= max.x &&
               point.y >= min.y && point.y <= max.y &&
               point.z >= min.z && point.z <= max.z;
    }
    
    bool intersects(const AABB& other) const {
        return !(other.min.x > max.x || other.max.x < min.x ||
                 other.min.y > max.y || other.max.y < min.y ||
                 other.min.z > max.z || other.max.z < min.z);
    }
    
    Vec3 center() const {
        return Vec3((min.x + max.x) * 0.5f, (min.y + max.y) * 0.5f, (min.z + max.z) * 0.5f);
    }
    
    Vec3 size() const {
        return max - min;
    }
    
    f32 volume() const {
        Vec3 s = size();
        return s.x * s.y * s.z;
    }
    
    void expand(const Vec3& point) {
        min.x = std::min(min.x, point.x);
        min.y = std::min(min.y, point.y);
        min.z = std::min(min.z, point.z);
        max.x = std::max(max.x, point.x);
        max.y = std::max(max.y, point.y);
        max.z = std::max(max.z, point.z);
    }
    
    void expand(const AABB& other) {
        expand(other.min);
        expand(other.max);
    }
};

/**
 * @brief Sphere for spatial queries
 */
struct Sphere {
    Vec3 center;
    f32 radius;
    
    Sphere() : center(Vec3()), radius(0.0f) {}
    Sphere(const Vec3& center_, f32 radius_) : center(center_), radius(radius_) {}
    
    bool contains(const Vec3& point) const {
        return (point - center).length_squared() <= radius * radius;
    }
    
    bool intersects(const AABB& box) const {
        Vec3 closest_point;
        closest_point.x = std::max(box.min.x, std::min(center.x, box.max.x));
        closest_point.y = std::max(box.min.y, std::min(center.y, box.max.y));
        closest_point.z = std::max(box.min.z, std::min(center.z, box.max.z));
        
        return (closest_point - center).length_squared() <= radius * radius;
    }
};

/**
 * @brief Spatial region types
 */
enum class RegionType {
    Box,        // Axis-aligned bounding box
    Sphere,     // Spherical region
    Cylinder,   // Cylindrical region
    Custom      // Custom region with predicate
};

/**
 * @brief Unified spatial region
 */
class Region {
private:
    RegionType type_;
    AABB box_;
    Sphere sphere_;
    std::function<bool(const Vec3&)> custom_predicate_;
    
public:
    explicit Region(const AABB& box) : type_(RegionType::Box), box_(box) {}
    explicit Region(const Sphere& sphere) : type_(RegionType::Sphere), sphere_(sphere) {}
    explicit Region(std::function<bool(const Vec3&)> predicate) 
        : type_(RegionType::Custom), custom_predicate_(std::move(predicate)) {}
    
    static Region box(const Vec3& min, const Vec3& max) {
        return Region(AABB(min, max));
    }
    
    static Region sphere(const Vec3& center, f32 radius) {
        return Region(Sphere(center, radius));
    }
    
    static Region cylinder(const Vec3& center, f32 radius, f32 height) {
        return Region([center, radius, height](const Vec3& point) -> bool {
            Vec3 diff = point - center;
            f32 horizontal_dist_sq = diff.x * diff.x + diff.z * diff.z;
            return horizontal_dist_sq <= radius * radius && 
                   std::abs(diff.y) <= height * 0.5f;
        });
    }
    
    bool contains(const Vec3& point) const {
        switch (type_) {
            case RegionType::Box:
                return box_.contains(point);
            case RegionType::Sphere:
                return sphere_.contains(point);
            case RegionType::Custom:
                return custom_predicate_ ? custom_predicate_(point) : false;
            default:
                return false;
        }
    }
    
    AABB bounding_box() const {
        switch (type_) {
            case RegionType::Box:
                return box_;
            case RegionType::Sphere:
                return AABB(sphere_.center - Vec3(sphere_.radius, sphere_.radius, sphere_.radius),
                           sphere_.center + Vec3(sphere_.radius, sphere_.radius, sphere_.radius));
            case RegionType::Custom:
                // For custom regions, we can't determine bounds automatically
                return AABB(Vec3(-1000, -1000, -1000), Vec3(1000, 1000, 1000));
            default:
                return AABB();
        }
    }
    
    RegionType type() const { return type_; }
};

/**
 * @brief Spatial index node for R-tree implementation
 */
template<typename T>
struct SpatialNode {
    AABB bounds;
    std::vector<std::unique_ptr<SpatialNode<T>>> children;
    std::vector<std::pair<AABB, T>> entries; // Leaf entries
    bool is_leaf;
    
    SpatialNode() : is_leaf(true) {}
    
    void add_entry(const AABB& box, const T& data) {
        entries.emplace_back(box, data);
        bounds.expand(box);
    }
    
    void add_child(std::unique_ptr<SpatialNode<T>> child) {
        bounds.expand(child->bounds);
        children.push_back(std::move(child));
        is_leaf = false;
    }
};

/**
 * @brief R-tree spatial index for efficient spatial queries
 */
template<typename T>
class RTree {
private:
    static constexpr usize MAX_ENTRIES = 16;
    static constexpr usize MIN_ENTRIES = MAX_ENTRIES / 2;
    
    std::unique_ptr<SpatialNode<T>> root_;
    usize size_;
    
    std::unique_ptr<SpatialNode<T>> choose_leaf(SpatialNode<T>* node, const AABB& box) {
        if (node->is_leaf) {
            return nullptr; // Return nullptr, we'll handle this differently
        }
        
        // Choose child with minimum area enlargement
        SpatialNode<T>* best_child = nullptr;
        f32 best_enlargement = std::numeric_limits<f32>::max();
        
        for (auto& child : node->children) {
            AABB enlarged = child->bounds;
            enlarged.expand(box);
            
            f32 enlargement = enlarged.volume() - child->bounds.volume();
            if (enlargement < best_enlargement) {
                best_enlargement = enlargement;
                best_child = child.get();
            }
        }
        
        return choose_leaf(best_child, box);
    }
    
    void split_node(SpatialNode<T>* node) {
        if (node->entries.size() <= MAX_ENTRIES) return;
        
        // Simple split implementation - in practice, you'd use more sophisticated algorithms
        auto new_node = std::make_unique<SpatialNode<T>>();
        
        usize mid = node->entries.size() / 2;
        new_node->entries.assign(node->entries.begin() + mid, node->entries.end());
        node->entries.resize(mid);
        
        // Recalculate bounds
        node->bounds = AABB();
        for (const auto& entry : node->entries) {
            node->bounds.expand(entry.first);
        }
        
        new_node->bounds = AABB();
        for (const auto& entry : new_node->entries) {
            new_node->bounds.expand(entry.first);
        }
        
        // This is a simplified split - a real implementation would handle parent updates
    }
    
    void query_recursive(SpatialNode<T>* node, const Region& region, 
                        std::vector<T>& results) const {
        if (!node) return;
        
        // Check if node bounds intersect with query region
        if (!region.contains(node->bounds.center()) && 
            !region.bounding_box().intersects(node->bounds)) {
            return;
        }
        
        if (node->is_leaf) {
            // Check each entry
            for (const auto& entry : node->entries) {
                if (region.contains(entry.first.center())) {
                    results.push_back(entry.second);
                }
            }
        } else {
            // Recurse into children
            for (const auto& child : node->children) {
                query_recursive(child.get(), region, results);
            }
        }
    }
    
public:
    RTree() : root_(std::make_unique<SpatialNode<T>>()), size_(0) {}
    
    void insert(const AABB& bounds, const T& data) {
        if (root_->is_leaf && root_->entries.size() < MAX_ENTRIES) {
            root_->add_entry(bounds, data);
        } else {
            // For simplicity, we'll just add to root for now
            // A full implementation would handle tree growth and splits properly
            root_->add_entry(bounds, data);
            if (root_->entries.size() > MAX_ENTRIES) {
                split_node(root_.get());
            }
        }
        size_++;
    }
    
    void remove(const T& data) {
        // Simplified removal - find and remove the entry
        auto it = std::find_if(root_->entries.begin(), root_->entries.end(),
            [&data](const auto& entry) { return entry.second == data; });
        
        if (it != root_->entries.end()) {
            root_->entries.erase(it);
            size_--;
            
            // Recalculate bounds
            root_->bounds = AABB();
            for (const auto& entry : root_->entries) {
                root_->bounds.expand(entry.first);
            }
        }
    }
    
    std::vector<T> query(const Region& region) const {
        std::vector<T> results;
        query_recursive(root_.get(), region, results);
        return results;
    }
    
    usize size() const { return size_; }
    bool empty() const { return size_ == 0; }
    
    void clear() {
        root_ = std::make_unique<SpatialNode<T>>();
        size_ = 0;
    }
};

/**
 * @brief Spatial hash grid for fast spatial queries
 */
template<typename T>
class SpatialHashGrid {
private:
    f32 cell_size_;
    std::unordered_map<u64, std::vector<std::pair<Vec3, T>>> grid_;
    
    u64 hash_position(const Vec3& pos) const {
        i32 x = static_cast<i32>(std::floor(pos.x / cell_size_));
        i32 y = static_cast<i32>(std::floor(pos.y / cell_size_));
        i32 z = static_cast<i32>(std::floor(pos.z / cell_size_));
        
        // Simple hash combination
        return static_cast<u64>(x) ^ (static_cast<u64>(y) << 16) ^ (static_cast<u64>(z) << 32);
    }
    
    std::vector<u64> get_cell_hashes_in_region(const Region& region) const {
        std::vector<u64> hashes;
        AABB bounds = region.bounding_box();
        
        i32 min_x = static_cast<i32>(std::floor(bounds.min.x / cell_size_));
        i32 max_x = static_cast<i32>(std::ceil(bounds.max.x / cell_size_));
        i32 min_y = static_cast<i32>(std::floor(bounds.min.y / cell_size_));
        i32 max_y = static_cast<i32>(std::ceil(bounds.max.y / cell_size_));
        i32 min_z = static_cast<i32>(std::floor(bounds.min.z / cell_size_));
        i32 max_z = static_cast<i32>(std::ceil(bounds.max.z / cell_size_));
        
        for (i32 x = min_x; x <= max_x; ++x) {
            for (i32 y = min_y; y <= max_y; ++y) {
                for (i32 z = min_z; z <= max_z; ++z) {
                    u64 hash = static_cast<u64>(x) ^ (static_cast<u64>(y) << 16) ^ (static_cast<u64>(z) << 32);
                    hashes.push_back(hash);
                }
            }
        }
        
        return hashes;
    }
    
public:
    explicit SpatialHashGrid(f32 cell_size = 10.0f) : cell_size_(cell_size) {}
    
    void insert(const Vec3& position, const T& data) {
        u64 hash = hash_position(position);
        grid_[hash].emplace_back(position, data);
    }
    
    void remove(const Vec3& position, const T& data) {
        u64 hash = hash_position(position);
        auto& cell = grid_[hash];
        
        auto it = std::find_if(cell.begin(), cell.end(),
            [&data](const auto& entry) { return entry.second == data; });
        
        if (it != cell.end()) {
            cell.erase(it);
            if (cell.empty()) {
                grid_.erase(hash);
            }
        }
    }
    
    std::vector<T> query(const Region& region) const {
        std::vector<T> results;
        auto cell_hashes = get_cell_hashes_in_region(region);
        
        for (u64 hash : cell_hashes) {
            auto it = grid_.find(hash);
            if (it != grid_.end()) {
                for (const auto& entry : it->second) {
                    if (region.contains(entry.first)) {
                        results.push_back(entry.second);
                    }
                }
            }
        }
        
        return results;
    }
    
    std::vector<T> query_radius(const Vec3& center, f32 radius) const {
        Region sphere_region = Region::sphere(center, radius);
        return query(sphere_region);
    }
    
    void clear() {
        grid_.clear();
    }
    
    usize size() const {
        usize total = 0;
        for (const auto& cell : grid_) {
            total += cell.second.size();
        }
        return total;
    }
    
    f32 cell_size() const { return cell_size_; }
    
    void set_cell_size(f32 new_size) {
        if (new_size != cell_size_) {
            // Would need to rehash all entries
            cell_size_ = new_size;
            // For simplicity, we'll just clear the grid
            clear();
        }
    }
};

/**
 * @brief Spatial index interface
 */
template<typename T>
class SpatialIndex {
public:
    virtual ~SpatialIndex() = default;
    virtual void insert(const Vec3& position, const T& data) = 0;
    virtual void remove(const Vec3& position, const T& data) = 0;
    virtual std::vector<T> query(const Region& region) const = 0;
    virtual std::vector<T> query_radius(const Vec3& center, f32 radius) const = 0;
    virtual void clear() = 0;
    virtual usize size() const = 0;
};

/**
 * @brief Hash grid spatial index implementation
 */
template<typename T>
class HashGridSpatialIndex : public SpatialIndex<T> {
private:
    SpatialHashGrid<T> grid_;
    
public:
    explicit HashGridSpatialIndex(f32 cell_size = 10.0f) : grid_(cell_size) {}
    
    void insert(const Vec3& position, const T& data) override {
        grid_.insert(position, data);
    }
    
    void remove(const Vec3& position, const T& data) override {
        grid_.remove(position, data);
    }
    
    std::vector<T> query(const Region& region) const override {
        return grid_.query(region);
    }
    
    std::vector<T> query_radius(const Vec3& center, f32 radius) const override {
        return grid_.query_radius(center, radius);
    }
    
    void clear() override {
        grid_.clear();
    }
    
    usize size() const override {
        return grid_.size();
    }
    
    f32 cell_size() const { return grid_.cell_size(); }
    void set_cell_size(f32 size) { grid_.set_cell_size(size); }
};

/**
 * @brief R-tree spatial index implementation
 */
template<typename T>
class RTreeSpatialIndex : public SpatialIndex<T> {
private:
    RTree<std::pair<Vec3, T>> rtree_;
    
public:
    void insert(const Vec3& position, const T& data) override {
        AABB point_box(position, position);
        rtree_.insert(point_box, std::make_pair(position, data));
    }
    
    void remove(const Vec3& position, const T& data) override {
        rtree_.remove(std::make_pair(position, data));
    }
    
    std::vector<T> query(const Region& region) const override {
        auto entries = rtree_.query(region);
        std::vector<T> results;
        results.reserve(entries.size());
        
        for (const auto& entry : entries) {
            if (region.contains(entry.first)) {
                results.push_back(entry.second);
            }
        }
        
        return results;
    }
    
    std::vector<T> query_radius(const Vec3& center, f32 radius) const override {
        Region sphere_region = Region::sphere(center, radius);
        return query(sphere_region);
    }
    
    void clear() override {
        rtree_.clear();
    }
    
    usize size() const override {
        return rtree_.size();
    }
};

/**
 * @brief Utility functions for spatial operations
 */
template<typename EntityTuple>
Vec3 extract_position(const EntityTuple& tuple) {
    // This would need to be specialized for actual component types
    // For now, return a default position
    return Vec3(0, 0, 0);
}

template<typename EntityTuple>
f32 distance_squared(const EntityTuple& tuple, const Vec3& point) {
    Vec3 pos = extract_position(tuple);
    return (pos - point).length_squared();
}

template<typename EntityTuple>
bool is_in_region(const EntityTuple& tuple, const Region& region) {
    Vec3 pos = extract_position(tuple);
    return region.contains(pos);
}

std::string to_string(const Region& region) {
    switch (region.type()) {
        case RegionType::Box: return "box";
        case RegionType::Sphere: return "sphere";
        case RegionType::Cylinder: return "cylinder";
        case RegionType::Custom: return "custom";
        default: return "unknown";
    }
}

/**
 * @brief Nearest neighbor search utilities
 */
template<typename T>
struct NearestNeighborResult {
    T data;
    f32 distance_squared;
    
    bool operator<(const NearestNeighborResult& other) const {
        return distance_squared < other.distance_squared;
    }
};

template<typename T>
std::vector<NearestNeighborResult<T>> find_k_nearest(
    const std::vector<std::pair<Vec3, T>>& points,
    const Vec3& query_point,
    usize k) {
    
    std::vector<NearestNeighborResult<T>> candidates;
    candidates.reserve(points.size());
    
    for (const auto& point_data : points) {
        f32 dist_sq = (point_data.first - query_point).length_squared();
        candidates.push_back({point_data.second, dist_sq});
    }
    
    // Partial sort to get k nearest
    if (k < candidates.size()) {
        std::partial_sort(candidates.begin(), candidates.begin() + k, candidates.end());
        candidates.resize(k);
    } else {
        std::sort(candidates.begin(), candidates.end());
    }
    
    return candidates;
}

} // namespace ecscope::ecs::query::spatial