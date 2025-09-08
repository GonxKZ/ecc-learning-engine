#pragma once

#include "../core/types.hpp"
#include "../core/memory.hpp"
#include "../foundation/concepts.hpp"
#include <vector>
#include <span>
#include <memory>
#include <algorithm>
#include <type_traits>
#include <utility>
#include <cassert>
#include <bit>
#include <atomic>
#include <mutex>

/**
 * @file sparse_set.hpp
 * @brief Advanced sparse set implementation for archetype-based ECS registry
 * 
 * This file implements a high-performance sparse set with:
 * - O(1) insertion, deletion, and lookup operations
 * - Cache-friendly packed iteration over entities
 * - Thread-safe operations with minimal locking
 * - SIMD-optimized batch operations
 * - Memory-efficient sparse array growth strategy
 * - Support for versioned entities with generational indices
 * - Advanced iteration patterns and filtering
 * - Comprehensive debugging and validation support
 * 
 * Educational Notes:
 * - Sparse sets combine dense arrays (for iteration) with sparse arrays (for lookup)
 * - Dense array contains packed entities for cache-friendly iteration
 * - Sparse array maps entity IDs to dense indices for O(1) lookup
 * - Swap-and-pop deletion maintains packed layout without holes
 * - Generational indices prevent dangling entity references
 * - Thread safety is achieved through fine-grained locking
 * - Memory layout is optimized for modern CPU cache hierarchies
 */

namespace ecscope::registry {

using namespace ecscope::core;
using namespace ecscope::foundation;

/// @brief Configuration for sparse set behavior
struct SparseSetConfig {
    std::uint32_t initial_dense_capacity{1024};      ///< Initial dense array capacity
    std::uint32_t initial_sparse_capacity{2048};     ///< Initial sparse array capacity
    std::uint32_t sparse_growth_factor{2};           ///< Sparse array growth multiplier
    std::uint32_t dense_growth_factor{2};            ///< Dense array growth multiplier
    bool enable_thread_safety{true};                 ///< Enable thread-safe operations
    bool enable_debugging{false};                    ///< Enable debug assertions and validation
    bool enable_simd_optimization{true};             ///< Enable SIMD batch operations
    std::uint32_t simd_batch_size{16};               ///< SIMD batch processing size
    std::uint32_t prefetch_distance{8};              ///< Cache prefetch distance
};

/// @brief Advanced sparse set for entity-to-index mapping with O(1) operations
class AdvancedSparseSet {
public:
    using size_type = std::uint32_t;
    using entity_type = EntityHandle;
    using index_type = std::uint32_t;
    
    /// @brief Invalid index sentinel value
    static constexpr index_type INVALID_INDEX = std::numeric_limits<index_type>::max();
    
    explicit AdvancedSparseSet(const SparseSetConfig& config = {});
    
    /// @brief Non-copyable but movable for performance
    AdvancedSparseSet(const AdvancedSparseSet&) = delete;
    AdvancedSparseSet& operator=(const AdvancedSparseSet&) = delete;
    AdvancedSparseSet(AdvancedSparseSet&&) noexcept = default;
    AdvancedSparseSet& operator=(AdvancedSparseSet&&) noexcept = default;
    
    /// @brief Check if entity exists in the set
    /// @param entity Entity to check
    /// @return true if entity is present in the set
    bool contains(entity_type entity) const noexcept;
    
    /// @brief Get dense index for entity (O(1) lookup)
    /// @param entity Entity to look up
    /// @return Dense index, or INVALID_INDEX if not found
    index_type get_index(entity_type entity) const noexcept;
    
    /// @brief Insert entity and return dense index
    /// @param entity Entity to insert
    /// @return Dense index of inserted/existing entity
    /// @throws std::bad_alloc if memory allocation fails
    index_type insert(entity_type entity);
    
    /// @brief Remove entity from set
    /// @param entity Entity to remove
    /// @return true if entity was removed, false if not found
    bool remove(entity_type entity) noexcept;
    
    /// @brief Get entity by dense index
    /// @param index Dense index (must be valid)
    /// @return Entity at the given index
    entity_type get_entity(index_type index) const noexcept;
    
    /// @brief Get all entities in dense order
    /// @return Span of all entities for cache-friendly iteration
    std::span<const entity_type> entities() const noexcept;
    
    /// @brief Get mutable entities span
    /// @return Mutable span of entities (use with caution)
    std::span<entity_type> entities() noexcept;
    
    /// @brief Get number of entities in the set
    /// @return Current entity count
    size_type size() const noexcept;
    
    /// @brief Check if the set is empty
    /// @return true if no entities are present
    bool empty() const noexcept;
    
    /// @brief Get current dense array capacity
    /// @return Dense array capacity
    size_type capacity() const noexcept;
    
    /// @brief Clear all entities from the set
    void clear() noexcept;
    
    /// @brief Reserve capacity for entities
    /// @param capacity New capacity to reserve
    void reserve(size_type capacity);
    
    /// @brief Shrink storage to fit current size
    void shrink_to_fit();
    
    /// @brief Swap entities at two dense indices
    /// @param index1 First index
    /// @param index2 Second index
    void swap_entities(index_type index1, index_type index2) noexcept;
    
    /// @brief Get memory usage statistics
    struct MemoryStats {
        std::size_t dense_bytes{};
        std::size_t sparse_bytes{};
        std::size_t total_bytes{};
        std::size_t dense_capacity{};
        std::size_t sparse_capacity{};
        double utilization{};
    };
    
    MemoryStats get_memory_stats() const noexcept;
    
    /// @brief Validate internal consistency (debug builds only)
    /// @return true if set is internally consistent
    bool validate_integrity() const noexcept;
    
    /// @brief Advanced iteration support
    class Iterator {
    public:
        using iterator_category = std::random_access_iterator_tag;
        using difference_type = std::ptrdiff_t;
        using value_type = entity_type;
        using pointer = const entity_type*;
        using reference = const entity_type&;
        
        Iterator() = default;
        explicit Iterator(const entity_type* ptr) noexcept : ptr_(ptr) {}
        
        reference operator*() const noexcept { return *ptr_; }
        pointer operator->() const noexcept { return ptr_; }
        reference operator[](difference_type n) const noexcept { return ptr_[n]; }
        
        Iterator& operator++() noexcept { ++ptr_; return *this; }
        Iterator operator++(int) noexcept { auto tmp = *this; ++ptr_; return tmp; }
        Iterator& operator--() noexcept { --ptr_; return *this; }
        Iterator operator--(int) noexcept { auto tmp = *this; --ptr_; return tmp; }
        
        Iterator& operator+=(difference_type n) noexcept { ptr_ += n; return *this; }
        Iterator& operator-=(difference_type n) noexcept { ptr_ -= n; return *this; }
        Iterator operator+(difference_type n) const noexcept { return Iterator(ptr_ + n); }
        Iterator operator-(difference_type n) const noexcept { return Iterator(ptr_ - n); }
        difference_type operator-(const Iterator& other) const noexcept { return ptr_ - other.ptr_; }
        
        bool operator==(const Iterator& other) const noexcept { return ptr_ == other.ptr_; }
        bool operator!=(const Iterator& other) const noexcept { return ptr_ != other.ptr_; }
        bool operator<(const Iterator& other) const noexcept { return ptr_ < other.ptr_; }
        bool operator<=(const Iterator& other) const noexcept { return ptr_ <= other.ptr_; }
        bool operator>(const Iterator& other) const noexcept { return ptr_ > other.ptr_; }
        bool operator>=(const Iterator& other) const noexcept { return ptr_ >= other.ptr_; }

    private:
        const entity_type* ptr_{nullptr};
    };
    
    /// @brief Iterator support for range-based loops and STL algorithms
    Iterator begin() const noexcept { return Iterator(dense_.data()); }
    Iterator end() const noexcept { return Iterator(dense_.data() + dense_.size()); }
    
    /// @brief Reverse iterator support
    using reverse_iterator = std::reverse_iterator<Iterator>;
    reverse_iterator rbegin() const noexcept { return reverse_iterator(end()); }
    reverse_iterator rend() const noexcept { return reverse_iterator(begin()); }
    
    /// @brief Batch operations for performance
    class BatchProcessor {
    public:
        explicit BatchProcessor(AdvancedSparseSet& set) : set_(set) {}
        
        /// @brief Insert multiple entities efficiently
        /// @param entities Span of entities to insert
        /// @return Vector of dense indices for inserted entities
        std::vector<index_type> batch_insert(std::span<const entity_type> entities);
        
        /// @brief Remove multiple entities efficiently
        /// @param entities Span of entities to remove
        /// @return Number of entities actually removed
        size_type batch_remove(std::span<const entity_type> entities) noexcept;
        
        /// @brief Check existence of multiple entities
        /// @param entities Entities to check
        /// @param results Output span for results (must be same size as entities)
        void batch_contains(std::span<const entity_type> entities, 
                           std::span<bool> results) const noexcept;
        
        /// @brief Get indices for multiple entities
        /// @param entities Entities to look up
        /// @param results Output span for indices (must be same size as entities)
        void batch_get_indices(std::span<const entity_type> entities,
                              std::span<index_type> results) const noexcept;
        
        /// @brief Process all entities with function (cache-optimized)
        /// @tparam Func Function type: (entity_type, index_type) -> void
        /// @param func Function to call for each entity
        template<typename Func>
        void for_each_with_index(Func&& func) const;
        
        /// @brief Process entities in parallel batches
        /// @tparam Func Function type: (std::span<const entity_type>) -> void
        /// @param func Function to process each batch
        /// @param batch_size Size of each processing batch
        template<typename Func>
        void parallel_for_each(Func&& func, size_type batch_size = 64) const;

    private:
        AdvancedSparseSet& set_;
    };
    
    /// @brief Get batch processor for efficient bulk operations
    BatchProcessor batch() { return BatchProcessor(*this); }
    
    /// @brief Performance metrics and diagnostics
    struct PerformanceStats {
        std::uint64_t insert_count{};
        std::uint64_t remove_count{};
        std::uint64_t lookup_count{};
        std::uint64_t sparse_growth_count{};
        std::uint64_t dense_growth_count{};
        std::uint64_t collision_count{};
        double average_lookup_time_ns{};
        double cache_hit_ratio{};
    };
    
    PerformanceStats get_performance_stats() const noexcept;
    void reset_performance_stats() noexcept;

private:
    /// @brief Ensure sparse array can accommodate entity ID
    void ensure_sparse_capacity(EntityId entity_id);
    
    /// @brief Grow sparse array by growth factor
    void grow_sparse_array(size_type new_capacity);
    
    /// @brief Grow dense array by growth factor
    void grow_dense_array(size_type new_capacity);
    
    /// @brief Validate entity handle generation
    bool is_valid_entity(entity_type entity) const noexcept;
    
    /// @brief Thread-safe operations helpers
    template<typename Func>
    auto execute_with_lock(Func&& func) const -> decltype(func());
    
    template<typename Func>
    auto execute_with_lock(Func&& func) -> decltype(func());
    
    SparseSetConfig config_;
    
    /// @brief Dense array for packed entity storage (cache-friendly iteration)
    std::vector<entity_type> dense_;
    
    /// @brief Sparse array for O(1) entity-to-index mapping
    std::vector<index_type> sparse_;
    
    /// @brief Thread safety (when enabled)
    mutable std::mutex mutex_;
    
    /// @brief Performance tracking (atomic for thread-safe access)
    mutable std::atomic<std::uint64_t> insert_count_{0};
    mutable std::atomic<std::uint64_t> remove_count_{0};
    mutable std::atomic<std::uint64_t> lookup_count_{0};
    mutable std::atomic<std::uint64_t> sparse_growth_count_{0};
    mutable std::atomic<std::uint64_t> dense_growth_count_{0};
    
    /// @brief Version tracking for change detection
    std::atomic<Version> version_{constants::INITIAL_VERSION};
};

/// @brief Specialized sparse set for component storage integration
template<Component T>
class ComponentSparseSet : public AdvancedSparseSet {
public:
    using component_type = T;
    using base_type = AdvancedSparseSet;
    
    explicit ComponentSparseSet(const SparseSetConfig& config = {})
        : base_type(config), component_id_(component_utils::get_component_id<T>()) {}
    
    /// @brief Get component ID for this sparse set
    ComponentId component_id() const noexcept { return component_id_; }
    
    /// @brief Get component type information
    const ComponentTypeInfo& type_info() const noexcept {
        static const auto info = ComponentTypeInfo::create<T>(component_id_, typeid(T).name());
        return info;
    }
    
    /// @brief Validate component-specific constraints
    bool validate_component_constraints(EntityHandle entity) const noexcept {
        // Check component dependencies if registry is available
        return true; // Basic implementation
    }

private:
    ComponentId component_id_;
};

/// @brief Utility functions for sparse set operations
namespace sparse_set_utils {

/// @brief Create sparse set with optimal configuration for component type
template<Component T>
SparseSetConfig optimal_config_for_component() {
    SparseSetConfig config{};
    
    // Adjust initial capacities based on component size
    const auto component_size = sizeof(T);
    if (component_size <= 16) {
        config.initial_dense_capacity = 2048;  // Small components
    } else if (component_size <= 64) {
        config.initial_dense_capacity = 1024;  // Medium components
    } else {
        config.initial_dense_capacity = 512;   // Large components
    }
    
    config.initial_sparse_capacity = config.initial_dense_capacity * 2;
    
    // Enable SIMD for arithmetic types
    config.enable_simd_optimization = std::is_arithmetic_v<T>;
    
    return config;
}

/// @brief Calculate memory overhead ratio
inline double calculate_memory_overhead(const AdvancedSparseSet& set) {
    const auto stats = set.get_memory_stats();
    if (stats.total_bytes == 0) return 0.0;
    
    const auto used_bytes = set.size() * (sizeof(EntityHandle) + sizeof(std::uint32_t));
    return static_cast<double>(stats.total_bytes - used_bytes) / stats.total_bytes;
}

/// @brief Optimize sparse set performance
inline void optimize_for_access_pattern(AdvancedSparseSet& set, 
                                       std::span<const EntityHandle> typical_entities) {
    // Pre-populate with typical access patterns to reduce future allocations
    for (const auto entity : typical_entities) {
        set.insert(entity);
    }
    
    // Reserve additional space for growth
    set.reserve(set.size() + typical_entities.size() / 2);
}

} // namespace sparse_set_utils

// Implementation details
namespace detail {

/// @brief SIMD-optimized batch operations (when available)
class SIMDBatchProcessor {
public:
    /// @brief Batch contains check using SIMD
    static void batch_contains_simd(std::span<const EntityHandle> entities,
                                   std::span<const std::uint32_t> sparse,
                                   std::span<const EntityHandle> dense,
                                   std::span<bool> results) noexcept;
    
    /// @brief Batch index lookup using SIMD
    static void batch_lookup_simd(std::span<const EntityHandle> entities,
                                 std::span<const std::uint32_t> sparse,
                                 std::span<const EntityHandle> dense,
                                 std::span<std::uint32_t> results) noexcept;
};

/// @brief Memory layout optimizer for cache efficiency
class MemoryLayoutOptimizer {
public:
    /// @brief Calculate optimal sparse array capacity
    static std::uint32_t calculate_optimal_sparse_capacity(std::uint32_t max_entity_id,
                                                          std::uint32_t dense_capacity);
    
    /// @brief Analyze access patterns for optimization
    static void analyze_access_patterns(const AdvancedSparseSet& set,
                                       std::span<const EntityHandle> recent_accesses);
};

} // namespace detail

} // namespace ecscope::registry

/// @brief Template implementation
#include "sparse_set_impl.hpp"