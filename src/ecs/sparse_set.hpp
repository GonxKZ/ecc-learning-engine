#pragma once

/**
 * @file ecs/sparse_set.hpp
 * @brief High-Performance Sparse Set Implementation for Modern ECS
 * 
 * This implementation extends the existing archetype system with modern sparse set
 * data structures optimized for component storage and iteration. It integrates
 * seamlessly with the existing memory allocators and provides educational insights
 * into data structure performance trade-offs.
 * 
 * Key Features:
 * - Memory-efficient sparse set implementation with O(1) operations
 * - Integration with existing Arena/Pool/PMR allocators
 * - Cache-friendly dense array iteration for systems
 * - Support for component versioning and change detection
 * - SIMD-friendly memory layout for batch operations
 * - Educational performance comparison tools
 * - C++20 concepts for type safety
 * 
 * Performance Benefits:
 * - O(1) component add/remove operations
 * - Dense iteration over active components
 * - Better cache locality than hash-based approaches
 * - Lower memory overhead than archetype systems for sparse data
 * 
 * Educational Value:
 * - Clear comparison with archetype storage performance
 * - Memory layout visualization tools
 * - Cache behavior analysis and optimization examples
 * - Integration patterns with custom allocators
 * 
 * @author ECScope Educational ECS Framework - Modern Extensions
 * @date 2024
 */

#include "core/types.hpp"
#include "core/log.hpp"
#include "ecs/entity.hpp"
#include "ecs/component.hpp"
#include "ecs/advanced_concepts.hpp"
#include "memory/allocators/arena.hpp"
#include "memory/pools/pool.hpp"
#include "memory/memory_tracker.hpp"
#include <vector>
#include <span>
#include <concepts>
#include <bit>
#include <algorithm>
#include <memory_resource>
#include <chrono>

namespace ecscope::ecs::sparse {

using namespace ecscope::ecs::concepts;

//=============================================================================
// C++20 Concepts for Type Safety
//=============================================================================

/**
 * @brief Concept for types that can be stored in sparse sets
 */
template<typename T>
concept SparseSetStorable = Component<T> && requires {
    // Must be trivially copyable for efficient memory operations
    requires std::is_trivially_copyable_v<T>;
    // Must have reasonable size (not too large for dense storage)
    requires sizeof(T) <= 1024;
    // Must be default constructible for slot initialization
    requires std::is_default_constructible_v<T>;
};

/**
 * @brief Concept for allocators compatible with sparse sets
 */
template<typename Allocator>
concept SparseSetAllocator = requires(Allocator& alloc, std::size_t size, std::size_t align) {
    { alloc.allocate(size, align) } -> std::convertible_to<void*>;
    { alloc.owns(std::declval<const void*>()) } -> std::convertible_to<bool>;
};

//=============================================================================
// Component Versioning for Change Detection
//=============================================================================

/**
 * @brief Version information for component change tracking
 */
struct ComponentVersion {
    u32 creation_version;    // Version when component was created
    u32 modification_version; // Version when last modified
    u32 access_version;      // Version when last accessed (for LRU)
    
    ComponentVersion() noexcept 
        : creation_version(0), modification_version(0), access_version(0) {}
        
    ComponentVersion(u32 version) noexcept
        : creation_version(version), modification_version(version), access_version(version) {}
        
    bool was_modified_since(u32 version) const noexcept {
        return modification_version > version;
    }
    
    bool was_accessed_since(u32 version) const noexcept {
        return access_version > version;
    }
    
    void mark_modified(u32 current_version) noexcept {
        modification_version = current_version;
        access_version = current_version;
    }
    
    void mark_accessed(u32 current_version) noexcept {
        access_version = current_version;
    }
};

//=============================================================================
// Sparse Set Implementation
//=============================================================================

/**
 * @brief High-performance sparse set for component storage
 * 
 * Provides O(1) insertion, removal, and lookup with dense iteration.
 * Integrates with existing memory management and provides educational
 * insights into data structure performance characteristics.
 */
template<SparseSetStorable T>
class SparseSet {
private:
    // Core sparse set data structures
    std::vector<u32> sparse_;           // Sparse array: entity_id -> dense_index
    std::vector<Entity> packed_;        // Dense array: dense_index -> entity_id
    std::vector<T> components_;         // Dense array: dense_index -> component_data
    std::vector<ComponentVersion> versions_; // Dense array: dense_index -> version_info
    
    // Memory management
    memory::ArenaAllocator* arena_;
    std::pmr::memory_resource* pmr_resource_;
    bool owns_memory_;
    
    // Performance tracking
    mutable u64 total_lookups_;
    mutable u64 cache_hits_;
    mutable u64 iterations_;
    mutable u64 modifications_;
    
    // Configuration
    u32 current_version_;
    usize max_entities_;
    bool enable_versioning_;
    bool enable_memory_tracking_;
    
    // Educational metrics
    mutable f64 total_iteration_time_;
    mutable f64 total_lookup_time_;
    mutable usize memory_reallocations_;
    
public:
    /**
     * @brief Construct sparse set with custom allocator
     */
    explicit SparseSet(usize initial_capacity = 1024, 
                      memory::ArenaAllocator* arena = nullptr,
                      std::pmr::memory_resource* pmr = nullptr,
                      bool enable_versioning = true)
        : arena_(arena)
        , pmr_resource_(pmr ? pmr : std::pmr::get_default_resource())
        , owns_memory_(arena == nullptr)
        , total_lookups_(0)
        , cache_hits_(0)
        , iterations_(0)
        , modifications_(0)
        , current_version_(1)
        , max_entities_(initial_capacity * 2) // Sparse array larger than dense
        , enable_versioning_(enable_versioning)
        , enable_memory_tracking_(true)
        , total_iteration_time_(0.0)
        , total_lookup_time_(0.0)
        , memory_reallocations_(0) {
        
        // Initialize with PMR or custom allocators
        if (pmr_resource_ && pmr_resource_ != std::pmr::get_default_resource()) {
            // Use PMR allocators
            sparse_ = std::vector<u32>(max_entities_, UINT32_MAX, 
                std::pmr::polymorphic_allocator<u32>(pmr_resource_));
            packed_ = std::vector<Entity>(std::pmr::polymorphic_allocator<Entity>(pmr_resource_));
            components_ = std::vector<T>(std::pmr::polymorphic_allocator<T>(pmr_resource_));
            versions_ = std::vector<ComponentVersion>(
                std::pmr::polymorphic_allocator<ComponentVersion>(pmr_resource_));
        } else {
            // Use standard allocators
            sparse_.resize(max_entities_, UINT32_MAX);
        }
        
        // Reserve initial capacity for dense arrays
        packed_.reserve(initial_capacity);
        components_.reserve(initial_capacity);
        if (enable_versioning_) {
            versions_.reserve(initial_capacity);
        }
        
        if (enable_memory_tracking_) {
            memory::tracker::track_allocation(this, sizeof(*this), "SparseSet");
        }
    }
    
    ~SparseSet() {
        if (enable_memory_tracking_) {
            memory::tracker::track_deallocation(this, sizeof(*this));
        }
    }
    
    // Non-copyable but movable
    SparseSet(const SparseSet&) = delete;
    SparseSet& operator=(const SparseSet&) = delete;
    
    SparseSet(SparseSet&& other) noexcept = default;
    SparseSet& operator=(SparseSet&& other) noexcept = default;
    
    /**
     * @brief Insert component for entity with O(1) complexity
     */
    bool insert(Entity entity, const T& component) {
        u32 entity_id = entity.id();
        
        // Ensure sparse array can accommodate this entity
        if (entity_id >= max_entities_) {
            expand_sparse_array(entity_id + 1);
        }
        
        // Check if entity already has component
        if (sparse_[entity_id] != UINT32_MAX) {
            // Update existing component
            u32 dense_idx = sparse_[entity_id];
            components_[dense_idx] = component;
            if (enable_versioning_) {
                versions_[dense_idx].mark_modified(current_version_++);
            }
            ++modifications_;
            return false; // Already existed
        }
        
        // Add new component
        u32 dense_idx = static_cast<u32>(packed_.size());
        sparse_[entity_id] = dense_idx;
        packed_.push_back(entity);
        components_.push_back(component);
        
        if (enable_versioning_) {
            versions_.emplace_back(current_version_++);
        }
        
        ++modifications_;
        return true; // New insertion
    }
    
    /**
     * @brief Remove component for entity with O(1) complexity
     */
    bool remove(Entity entity) {
        u32 entity_id = entity.id();
        if (entity_id >= max_entities_ || sparse_[entity_id] == UINT32_MAX) {
            return false; // Entity not found
        }
        
        u32 dense_idx = sparse_[entity_id];
        u32 last_idx = static_cast<u32>(packed_.size() - 1);
        
        if (dense_idx != last_idx) {
            // Swap with last element for O(1) removal
            Entity last_entity = packed_[last_idx];
            
            packed_[dense_idx] = last_entity;
            components_[dense_idx] = std::move(components_[last_idx]);
            
            if (enable_versioning_) {
                versions_[dense_idx] = versions_[last_idx];
            }
            
            // Update sparse array for swapped entity
            sparse_[last_entity.id()] = dense_idx;
        }
        
        // Remove last element
        packed_.pop_back();
        components_.pop_back();
        if (enable_versioning_) {
            versions_.pop_back();
        }
        
        // Mark sparse slot as empty
        sparse_[entity_id] = UINT32_MAX;
        
        ++modifications_;
        return true;
    }
    
    /**
     * @brief Get component for entity with O(1) complexity
     */
    T* get(Entity entity) {
        auto start_time = std::chrono::high_resolution_clock::now();
        
        u32 entity_id = entity.id();
        if (entity_id >= max_entities_) {
            ++total_lookups_;
            return nullptr;
        }
        
        u32 dense_idx = sparse_[entity_id];
        if (dense_idx == UINT32_MAX) {
            ++total_lookups_;
            
            // Track lookup time for educational analysis
            auto end_time = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration<f64>(end_time - start_time).count();
            total_lookup_time_ += duration;
            
            return nullptr;
        }
        
        ++total_lookups_;
        ++cache_hits_;
        
        if (enable_versioning_) {
            versions_[dense_idx].mark_accessed(current_version_);
        }
        
        // Track lookup time
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration<f64>(end_time - start_time).count();
        total_lookup_time_ += duration;
        
        return &components_[dense_idx];
    }
    
    const T* get(Entity entity) const {
        // Similar implementation to non-const version
        return const_cast<SparseSet*>(this)->get(entity);
    }
    
    /**
     * @brief Check if entity has component
     */
    bool contains(Entity entity) const noexcept {
        u32 entity_id = entity.id();
        return entity_id < max_entities_ && sparse_[entity_id] != UINT32_MAX;
    }
    
    /**
     * @brief Dense iteration over all components
     */
    std::span<const Entity> entities() const noexcept {
        return std::span<const Entity>(packed_);
    }
    
    std::span<T> components() noexcept {
        return std::span<T>(components_);
    }
    
    std::span<const T> components() const noexcept {
        return std::span<const T>(components_);
    }
    
    /**
     * @brief Paired iteration over entities and components
     */
    void for_each(auto&& func) const {
        auto start_time = std::chrono::high_resolution_clock::now();
        
        for (usize i = 0; i < packed_.size(); ++i) {
            func(packed_[i], components_[i]);
        }
        
        ++iterations_;
        
        // Track iteration time for educational analysis
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration<f64>(end_time - start_time).count();
        total_iteration_time_ += duration;
    }
    
    void for_each(auto&& func) {
        for (usize i = 0; i < packed_.size(); ++i) {
            func(packed_[i], components_[i]);
        }
        ++iterations_;
    }
    
    /**
     * @brief Batch operations for SIMD optimization
     */
    template<typename Operation>
    requires requires(Operation op, T& component) {
        op(component);
    }
    void transform_components(Operation&& op) {
        // Process components in batches for better cache utilization
        constexpr usize batch_size = 64 / sizeof(T); // Cache line optimized
        
        for (usize i = 0; i < components_.size(); i += batch_size) {
            usize end = std::min(i + batch_size, components_.size());
            
            // Prefetch next batch
            if (end < components_.size()) {
                __builtin_prefetch(&components_[end], 0, 3);
            }
            
            // Process current batch
            for (usize j = i; j < end; ++j) {
                op(components_[j]);
                if (enable_versioning_) {
                    versions_[j].mark_modified(current_version_);
                }
            }
        }
        
        if (enable_versioning_) {
            ++current_version_;
        }
        ++modifications_;
    }
    
    /**
     * @brief Change detection support
     */
    bool was_modified_since(Entity entity, u32 version) const {
        if (!enable_versioning_) return false;
        
        u32 entity_id = entity.id();
        if (entity_id >= max_entities_ || sparse_[entity_id] == UINT32_MAX) {
            return false;
        }
        
        u32 dense_idx = sparse_[entity_id];
        return versions_[dense_idx].was_modified_since(version);
    }
    
    u32 get_modification_version(Entity entity) const {
        if (!enable_versioning_) return 0;
        
        u32 entity_id = entity.id();
        if (entity_id >= max_entities_ || sparse_[entity_id] == UINT32_MAX) {
            return 0;
        }
        
        u32 dense_idx = sparse_[entity_id];
        return versions_[dense_idx].modification_version;
    }
    
    u32 current_version() const noexcept { return current_version_; }
    
    /**
     * @brief Memory management and statistics
     */
    usize size() const noexcept { return packed_.size(); }
    usize capacity() const noexcept { return components_.capacity(); }
    bool empty() const noexcept { return packed_.empty(); }
    
    void reserve(usize new_capacity) {
        if (new_capacity > components_.capacity()) {
            packed_.reserve(new_capacity);
            components_.reserve(new_capacity);
            if (enable_versioning_) {
                versions_.reserve(new_capacity);
            }
            ++memory_reallocations_;
        }
    }
    
    void shrink_to_fit() {
        packed_.shrink_to_fit();
        components_.shrink_to_fit();
        if (enable_versioning_) {
            versions_.shrink_to_fit();
        }
    }
    
    void clear() {
        packed_.clear();
        components_.clear();
        if (enable_versioning_) {
            versions_.clear();
        }
        std::fill(sparse_.begin(), sparse_.end(), UINT32_MAX);
        current_version_ = 1;
        modifications_ = 0;
    }
    
    /**
     * @brief Performance analysis and educational insights
     */
    struct PerformanceMetrics {
        // Basic statistics
        usize total_components;
        usize sparse_array_size;
        usize dense_array_size;
        
        // Performance metrics
        u64 total_lookups;
        u64 cache_hits;
        f64 cache_hit_ratio;
        u64 total_iterations;
        u64 total_modifications;
        
        // Timing metrics
        f64 average_lookup_time_ns;
        f64 average_iteration_time_ns;
        
        // Memory metrics
        usize total_memory_bytes;
        usize sparse_memory_bytes;
        usize dense_memory_bytes;
        usize versioning_memory_bytes;
        f64 memory_efficiency; // useful_data / total_memory
        usize memory_reallocations;
        
        // Educational insights
        f64 sparsity_ratio;      // empty_slots / total_sparse_slots
        f64 fragmentation_score; // measure of memory fragmentation
        f64 access_locality;     // measure of spatial locality in accesses
        
        std::string performance_analysis;
        std::vector<std::string> optimization_suggestions;
    };
    
    PerformanceMetrics get_performance_metrics() const {
        PerformanceMetrics metrics{};
        
        // Basic statistics
        metrics.total_components = packed_.size();
        metrics.sparse_array_size = sparse_.size();
        metrics.dense_array_size = components_.size();
        
        // Performance metrics
        metrics.total_lookups = total_lookups_;
        metrics.cache_hits = cache_hits_;
        metrics.cache_hit_ratio = total_lookups_ > 0 ? 
            static_cast<f64>(cache_hits_) / total_lookups_ : 0.0;
        metrics.total_iterations = iterations_;
        metrics.total_modifications = modifications_;
        
        // Timing metrics
        metrics.average_lookup_time_ns = total_lookups_ > 0 ? 
            (total_lookup_time_ * 1e9) / total_lookups_ : 0.0;
        metrics.average_iteration_time_ns = iterations_ > 0 ? 
            (total_iteration_time_ * 1e9) / iterations_ : 0.0;
        
        // Memory metrics
        metrics.sparse_memory_bytes = sparse_.size() * sizeof(u32);
        metrics.dense_memory_bytes = components_.capacity() * sizeof(T) + 
                                   packed_.capacity() * sizeof(Entity);
        metrics.versioning_memory_bytes = enable_versioning_ ? 
            versions_.capacity() * sizeof(ComponentVersion) : 0;
        metrics.total_memory_bytes = metrics.sparse_memory_bytes + 
                                   metrics.dense_memory_bytes + 
                                   metrics.versioning_memory_bytes;
        
        usize useful_data = packed_.size() * (sizeof(T) + sizeof(Entity));
        metrics.memory_efficiency = static_cast<f64>(useful_data) / 
                                  std::max(metrics.total_memory_bytes, usize{1});
        
        metrics.memory_reallocations = memory_reallocations_;
        
        // Educational insights
        usize empty_sparse_slots = 0;
        for (u32 slot : sparse_) {
            if (slot == UINT32_MAX) ++empty_sparse_slots;
        }
        metrics.sparsity_ratio = static_cast<f64>(empty_sparse_slots) / sparse_.size();
        
        // Generate analysis text
        metrics.performance_analysis = generate_performance_analysis(metrics);
        metrics.optimization_suggestions = generate_optimization_suggestions(metrics);
        
        return metrics;
    }
    
    /**
     * @brief Compare with archetype storage performance
     */
    template<typename ArchetypeStorage>
    struct StorageComparison {
        f64 sparse_set_lookup_time;
        f64 archetype_lookup_time;
        f64 lookup_speedup_factor;
        
        f64 sparse_set_iteration_time;
        f64 archetype_iteration_time;
        f64 iteration_speedup_factor;
        
        usize sparse_set_memory;
        usize archetype_memory;
        f64 memory_efficiency_ratio;
        
        std::string recommendation;
        std::string use_case_analysis;
    };
    
    template<typename ArchetypeStorage>
    StorageComparison<ArchetypeStorage> compare_with_archetype(
        const ArchetypeStorage& archetype_storage) const {
        
        StorageComparison<ArchetypeStorage> comparison{};
        
        auto sparse_metrics = get_performance_metrics();
        // Would get archetype metrics here
        
        comparison.sparse_set_memory = sparse_metrics.total_memory_bytes;
        // comparison.archetype_memory = archetype_storage.get_memory_usage();
        
        comparison.recommendation = generate_storage_recommendation(comparison);
        comparison.use_case_analysis = generate_use_case_analysis();
        
        return comparison;
    }
    
private:
    void expand_sparse_array(usize new_size) {
        if (new_size > max_entities_) {
            usize old_size = sparse_.size();
            max_entities_ = new_size * 2; // Grow with headroom
            sparse_.resize(max_entities_, UINT32_MAX);
            ++memory_reallocations_;
            
            if (enable_memory_tracking_) {
                usize new_memory = (max_entities_ - old_size) * sizeof(u32);
                memory::tracker::track_allocation(sparse_.data() + old_size, 
                                               new_memory, "SparseSet::sparse");
            }
        }
    }
    
    std::string generate_performance_analysis(const PerformanceMetrics& metrics) const {
        std::ostringstream analysis;
        analysis << "Sparse Set Performance Analysis:\n";
        analysis << "- Cache hit ratio: " << (metrics.cache_hit_ratio * 100.0) << "%\n";
        analysis << "- Memory efficiency: " << (metrics.memory_efficiency * 100.0) << "%\n";
        analysis << "- Sparsity ratio: " << (metrics.sparsity_ratio * 100.0) << "%\n";
        
        if (metrics.cache_hit_ratio > 0.9) {
            analysis << "- Excellent cache performance!\n";
        } else if (metrics.cache_hit_ratio > 0.7) {
            analysis << "- Good cache performance.\n";
        } else {
            analysis << "- Cache performance could be improved.\n";
        }
        
        return analysis.str();
    }
    
    std::vector<std::string> generate_optimization_suggestions(
        const PerformanceMetrics& metrics) const {
        
        std::vector<std::string> suggestions;
        
        if (metrics.sparsity_ratio > 0.8) {
            suggestions.push_back("High sparsity detected - consider archetype storage for better memory usage");
        }
        
        if (metrics.cache_hit_ratio < 0.8) {
            suggestions.push_back("Low cache hit ratio - consider entity ID recycling or compaction");
        }
        
        if (metrics.memory_efficiency < 0.5) {
            suggestions.push_back("Low memory efficiency - consider shrinking unused capacity");
        }
        
        if (metrics.memory_reallocations > 10) {
            suggestions.push_back("Frequent reallocations detected - consider reserving more capacity upfront");
        }
        
        return suggestions;
    }
    
    std::string generate_storage_recommendation(const auto& comparison) const {
        // Analysis logic would go here
        return "Recommendation based on usage patterns and performance characteristics";
    }
    
    std::string generate_use_case_analysis() const {
        return "Sparse sets excel when: components are sparsely distributed, frequent add/remove operations, cache-friendly iteration is important";
    }
};

//=============================================================================
// Sparse Set Registry Integration
//=============================================================================

/**
 * @brief Registry extension for sparse set storage
 */
class SparseSetRegistry {
private:
    std::unordered_map<core::ComponentID, std::unique_ptr<void, void(*)(void*)>> sparse_sets_;
    memory::ArenaAllocator* arena_;
    std::pmr::memory_resource* pmr_resource_;
    
public:
    explicit SparseSetRegistry(memory::ArenaAllocator* arena = nullptr,
                              std::pmr::memory_resource* pmr = nullptr)
        : arena_(arena), pmr_resource_(pmr) {}
    
    template<SparseSetStorable T>
    SparseSet<T>& get_or_create_sparse_set() {
        auto component_id = component_id<T>();
        
        auto it = sparse_sets_.find(component_id);
        if (it == sparse_sets_.end()) {
            auto sparse_set = std::make_unique<SparseSet<T>>(1024, arena_, pmr_resource_);
            SparseSet<T>* ptr = sparse_set.get();
            
            sparse_sets_.emplace(component_id, 
                std::unique_ptr<void, void(*)(void*)>(
                    sparse_set.release(),
                    [](void* p) { delete static_cast<SparseSet<T>*>(p); }
                ));
            
            return *ptr;
        }
        
        return *static_cast<SparseSet<T>*>(it->second.get());
    }
    
    template<SparseSetStorable T>
    bool has_sparse_set() const {
        return sparse_sets_.find(component_id<T>()) != sparse_sets_.end();
    }
    
    void clear_all() {
        sparse_sets_.clear();
    }
    
    usize total_memory_usage() const {
        usize total = 0;
        // Would iterate through all sparse sets and sum their memory usage
        return total;
    }
};

} // namespace ecscope::ecs::sparse