#include "ecscope/physics/collision_detection.hpp"
#include <algorithm>
#include <execution>
#include <unordered_set>

namespace ecscope::physics {

// High-performance spatial hash broad phase implementation optimized for 10,000+ objects
class HighPerformanceSpatialHash : public BroadPhaseCollisionDetection {
private:
    struct GridCell {
        std::vector<uint32_t> object_ids;
        std::vector<AABB3D> bounding_boxes;
        uint32_t version = 0;
        
        void clear() {
            object_ids.clear();
            bounding_boxes.clear();
            ++version;
        }
        
        void reserve(size_t capacity) {
            object_ids.reserve(capacity);
            bounding_boxes.reserve(capacity);
        }
    };
    
    // Memory-efficient sparse grid using unordered_map
    std::unordered_map<uint64_t, GridCell> grid;
    
    // Pre-allocated vectors for performance
    mutable std::vector<CollisionPair> collision_pairs;
    mutable std::unordered_set<uint64_t> seen_pairs;
    
    Real cell_size = 10.0f;
    Real inv_cell_size = 0.1f;
    
    // Performance tracking
    mutable size_t total_objects_inserted = 0;
    mutable size_t total_cells_used = 0;
    mutable size_t total_pairs_generated = 0;
    
public:
    HighPerformanceSpatialHash(Real initial_cell_size = 10.0f) 
        : cell_size(initial_cell_size), inv_cell_size(1.0f / initial_cell_size) {
        collision_pairs.reserve(1000);
        seen_pairs.reserve(2000);
    }
    
    void set_cell_size(Real size) override {
        cell_size = size;
        inv_cell_size = 1.0f / size;
    }
    
    void clear() override {
        for (auto& [key, cell] : grid) {
            cell.clear();
        }
        total_objects_inserted = 0;
        total_cells_used = grid.size();
    }
    
    // Optimized 3D hash function using bit manipulation
    uint64_t hash_position_3d(int x, int y, int z) const {
        // Use large prime numbers to reduce clustering
        return static_cast<uint64_t>(x) * 73856093ULL ^ 
               static_cast<uint64_t>(y) * 19349663ULL ^ 
               static_cast<uint64_t>(z) * 83492791ULL;
    }
    
    // 2D version for backward compatibility
    uint64_t hash_position_2d(int x, int y) const {
        return static_cast<uint64_t>(x) * 73856093ULL ^ 
               static_cast<uint64_t>(y) * 19349663ULL;
    }
    
    std::tuple<int, int, int> world_to_grid_3d(const Vec3& world_pos) const {
        return {
            static_cast<int>(std::floor(world_pos.x * inv_cell_size)),
            static_cast<int>(std::floor(world_pos.y * inv_cell_size)),
            static_cast<int>(std::floor(world_pos.z * inv_cell_size))
        };
    }
    
    void add_body_3d(const RigidBody3D& body, const Shape& shape) override {
        AABB3D aabb = shape.get_aabb_3d(body.transform);
        
        auto [min_x, min_y, min_z] = world_to_grid_3d(aabb.min);
        auto [max_x, max_y, max_z] = world_to_grid_3d(aabb.max);
        
        // Insert into all overlapping cells
        for (int x = min_x; x <= max_x; ++x) {
            for (int y = min_y; y <= max_y; ++y) {
                for (int z = min_z; z <= max_z; ++z) {
                    uint64_t cell_hash = hash_position_3d(x, y, z);
                    auto& cell = grid[cell_hash];
                    cell.object_ids.push_back(body.id);
                    cell.bounding_boxes.push_back(aabb);
                }
            }
        }
        
        ++total_objects_inserted;
    }
    
    void add_body_2d(const RigidBody2D& body, const Shape& shape) override {
        AABB2D aabb = shape.get_aabb_2d(body.transform);
        
        int min_x = static_cast<int>(std::floor(aabb.min.x * inv_cell_size));
        int min_y = static_cast<int>(std::floor(aabb.min.y * inv_cell_size));
        int max_x = static_cast<int>(std::floor(aabb.max.x * inv_cell_size));
        int max_y = static_cast<int>(std::floor(aabb.max.y * inv_cell_size));
        
        for (int x = min_x; x <= max_x; ++x) {
            for (int y = min_y; y <= max_y; ++y) {
                uint64_t cell_hash = hash_position_2d(x, y);
                auto& cell = grid[cell_hash];
                cell.object_ids.push_back(body.id);
                // Convert 2D AABB to 3D for uniform storage
                AABB3D aabb_3d;
                aabb_3d.min = Vec3(aabb.min.x, aabb.min.y, 0);
                aabb_3d.max = Vec3(aabb.max.x, aabb.max.y, 0);
                cell.bounding_boxes.push_back(aabb_3d);
            }
        }
        
        ++total_objects_inserted;
    }
    
    void find_collision_pairs_3d() override {
        collision_pairs.clear();
        seen_pairs.clear();
        total_pairs_generated = 0;
        
        // Use parallel execution for large grids
        if (grid.size() > 100 && total_objects_inserted > 1000) {
            // Parallel processing for large grids
            std::vector<std::pair<uint64_t, GridCell*>> grid_entries;
            grid_entries.reserve(grid.size());
            for (auto& [key, cell] : grid) {
                if (cell.object_ids.size() >= 2) {
                    grid_entries.emplace_back(key, &cell);
                }
            }
            
            std::mutex pairs_mutex;
            std::for_each(std::execution::par, grid_entries.begin(), grid_entries.end(),
                [this, &pairs_mutex](const auto& entry) {
                    const auto& cell = *entry.second;
                    std::vector<CollisionPair> local_pairs;
                    
                    // Generate pairs within this cell
                    for (size_t i = 0; i < cell.object_ids.size(); ++i) {
                        for (size_t j = i + 1; j < cell.object_ids.size(); ++j) {
                            uint32_t id_a = cell.object_ids[i];
                            uint32_t id_b = cell.object_ids[j];
                            
                            // Basic AABB overlap test
                            const AABB3D& aabb_a = cell.bounding_boxes[i];
                            const AABB3D& aabb_b = cell.bounding_boxes[j];
                            
                            if (aabb_overlap_3d(aabb_a, aabb_b)) {
                                local_pairs.emplace_back(id_a, id_b);
                            }
                        }
                    }
                    
                    // Thread-safe merge
                    std::lock_guard<std::mutex> lock(pairs_mutex);
                    for (const auto& pair : local_pairs) {
                        uint64_t pair_key = make_pair_key(pair.id_a, pair.id_b);
                        if (seen_pairs.find(pair_key) == seen_pairs.end()) {
                            seen_pairs.insert(pair_key);
                            collision_pairs.push_back(pair);
                        }
                    }
                });
        } else {
            // Sequential processing for smaller grids
            for (const auto& [cell_hash, cell] : grid) {
                if (cell.object_ids.size() < 2) continue;
                
                // Generate pairs within this cell
                for (size_t i = 0; i < cell.object_ids.size(); ++i) {
                    for (size_t j = i + 1; j < cell.object_ids.size(); ++j) {
                        uint32_t id_a = cell.object_ids[i];
                        uint32_t id_b = cell.object_ids[j];
                        
                        // Basic AABB overlap test
                        const AABB3D& aabb_a = cell.bounding_boxes[i];
                        const AABB3D& aabb_b = cell.bounding_boxes[j];
                        
                        if (aabb_overlap_3d(aabb_a, aabb_b)) {
                            uint64_t pair_key = make_pair_key(id_a, id_b);
                            if (seen_pairs.find(pair_key) == seen_pairs.end()) {
                                seen_pairs.insert(pair_key);
                                collision_pairs.emplace_back(id_a, id_b);
                            }
                        }
                    }
                }
            }
        }
        
        total_pairs_generated = collision_pairs.size();
    }
    
    void find_collision_pairs_2d() override {
        collision_pairs.clear();
        seen_pairs.clear();
        total_pairs_generated = 0;
        
        for (const auto& [cell_hash, cell] : grid) {
            if (cell.object_ids.size() < 2) continue;
            
            // Generate pairs within this cell
            for (size_t i = 0; i < cell.object_ids.size(); ++i) {
                for (size_t j = i + 1; j < cell.object_ids.size(); ++j) {
                    uint32_t id_a = cell.object_ids[i];
                    uint32_t id_b = cell.object_ids[j];
                    
                    // Basic AABB overlap test (2D)
                    const AABB3D& aabb_a = cell.bounding_boxes[i];
                    const AABB3D& aabb_b = cell.bounding_boxes[j];
                    
                    if (aabb_overlap_2d(aabb_a, aabb_b)) {
                        uint64_t pair_key = make_pair_key(id_a, id_b);
                        if (seen_pairs.find(pair_key) == seen_pairs.end()) {
                            seen_pairs.insert(pair_key);
                            collision_pairs.emplace_back(id_a, id_b);
                        }
                    }
                }
            }
        }
        
        total_pairs_generated = collision_pairs.size();
    }
    
    const std::vector<CollisionPair>& get_collision_pairs() const override {
        return collision_pairs;
    }
    
    BroadPhaseStats get_stats() const override {
        BroadPhaseStats stats;
        stats.total_objects = total_objects_inserted;
        stats.total_cells = grid.size();
        stats.total_pairs = total_pairs_generated;
        stats.efficiency_ratio = total_objects_inserted > 0 ? 
            static_cast<Real>(total_pairs_generated) / (total_objects_inserted * total_objects_inserted * 0.5f) : 0.0f;
        
        // Calculate memory usage
        size_t memory_usage = sizeof(*this);
        for (const auto& [key, cell] : grid) {
            memory_usage += sizeof(key) + sizeof(cell);
            memory_usage += cell.object_ids.capacity() * sizeof(uint32_t);
            memory_usage += cell.bounding_boxes.capacity() * sizeof(AABB3D);
        }
        memory_usage += collision_pairs.capacity() * sizeof(CollisionPair);
        memory_usage += seen_pairs.bucket_count() * sizeof(uint64_t);
        
        stats.memory_usage_bytes = memory_usage;
        
        return stats;
    }
    
    Real get_efficiency_ratio() const override {
        return get_stats().efficiency_ratio;
    }
    
private:
    uint64_t make_pair_key(uint32_t a, uint32_t b) const {
        if (a > b) std::swap(a, b);
        return (static_cast<uint64_t>(a) << 32) | b;
    }
    
    bool aabb_overlap_3d(const AABB3D& a, const AABB3D& b) const {
        return !(a.max.x < b.min.x || a.min.x > b.max.x ||
                 a.max.y < b.min.y || a.min.y > b.max.y ||
                 a.max.z < b.min.z || a.min.z > b.max.z);
    }
    
    bool aabb_overlap_2d(const AABB3D& a, const AABB3D& b) const {
        return !(a.max.x < b.min.x || a.min.x > b.max.x ||
                 a.max.y < b.min.y || a.min.y > b.max.y);
    }
};

// Factory function for creating optimized broad phase implementations
std::unique_ptr<BroadPhaseCollisionDetection> create_optimal_broad_phase(size_t expected_object_count, Real world_size) {
    // Calculate optimal cell size based on expected object density
    Real optimal_cell_size = std::sqrt(world_size * world_size / expected_object_count) * 2.0f;
    optimal_cell_size = std::max(1.0f, std::min(optimal_cell_size, 50.0f)); // Clamp to reasonable bounds
    
    return std::make_unique<HighPerformanceSpatialHash>(optimal_cell_size);
}

} // namespace ecscope::physics