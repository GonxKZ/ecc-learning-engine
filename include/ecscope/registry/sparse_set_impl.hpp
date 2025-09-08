#pragma once

#include "sparse_set.hpp"
#include "../core/memory.hpp"
#include <algorithm>
#include <execution>

// Debug configuration
#ifndef ECSCOPE_DEBUG_ENABLED
#ifdef _DEBUG
#define ECSCOPE_DEBUG_ENABLED true
#else
#define ECSCOPE_DEBUG_ENABLED false
#endif
#endif

/**
 * @file sparse_set_impl.hpp
 * @brief Implementation details for advanced sparse set
 * 
 * This file contains the template implementations and inline functions
 * for the AdvancedSparseSet class to ensure optimal performance.
 */

namespace ecscope::registry {

// AdvancedSparseSet inline implementations

inline AdvancedSparseSet::AdvancedSparseSet(const SparseSetConfig& config)
    : config_(config) {
    dense_.reserve(config_.initial_dense_capacity);
    sparse_.resize(config_.initial_sparse_capacity, INVALID_INDEX);
}

inline bool AdvancedSparseSet::contains(entity_type entity) const noexcept {
    if (config_.enable_thread_safety) {
        std::lock_guard<std::mutex> lock(mutex_);
        return contains_impl(entity);
    }
    return contains_impl(entity);
}

inline auto AdvancedSparseSet::get_index(entity_type entity) const noexcept -> index_type {
    if (config_.enable_thread_safety) {
        std::lock_guard<std::mutex> lock(mutex_);
        return get_index_impl(entity);
    }
    return get_index_impl(entity);
}

inline auto AdvancedSparseSet::insert(entity_type entity) -> index_type {
    if (config_.enable_thread_safety) {
        std::lock_guard<std::mutex> lock(mutex_);
        return insert_impl(entity);
    }
    return insert_impl(entity);
}

inline bool AdvancedSparseSet::remove(entity_type entity) noexcept {
    if (config_.enable_thread_safety) {
        std::lock_guard<std::mutex> lock(mutex_);
        return remove_impl(entity);
    }
    return remove_impl(entity);
}

inline auto AdvancedSparseSet::get_entity(index_type index) const noexcept -> entity_type {
    assert(index < dense_.size());
    return dense_[index];
}

inline std::span<const AdvancedSparseSet::entity_type> AdvancedSparseSet::entities() const noexcept {
    if (config_.enable_thread_safety) {
        std::lock_guard<std::mutex> lock(mutex_);
        return std::span<const entity_type>(dense_);
    }
    return std::span<const entity_type>(dense_);
}

inline std::span<AdvancedSparseSet::entity_type> AdvancedSparseSet::entities() noexcept {
    if (config_.enable_thread_safety) {
        std::lock_guard<std::mutex> lock(mutex_);
        return std::span<entity_type>(dense_);
    }
    return std::span<entity_type>(dense_);
}

inline auto AdvancedSparseSet::size() const noexcept -> size_type {
    return static_cast<size_type>(dense_.size());
}

inline bool AdvancedSparseSet::empty() const noexcept {
    return dense_.empty();
}

inline auto AdvancedSparseSet::capacity() const noexcept -> size_type {
    return static_cast<size_type>(dense_.capacity());
}

inline void AdvancedSparseSet::clear() noexcept {
    if (config_.enable_thread_safety) {
        std::lock_guard<std::mutex> lock(mutex_);
        clear_impl();
    } else {
        clear_impl();
    }
}

inline void AdvancedSparseSet::reserve(size_type capacity) {
    if (config_.enable_thread_safety) {
        std::lock_guard<std::mutex> lock(mutex_);
        reserve_impl(capacity);
    } else {
        reserve_impl(capacity);
    }
}

inline void AdvancedSparseSet::shrink_to_fit() {
    if (config_.enable_thread_safety) {
        std::lock_guard<std::mutex> lock(mutex_);
        shrink_to_fit_impl();
    } else {
        shrink_to_fit_impl();
    }
}

inline void AdvancedSparseSet::swap_entities(index_type index1, index_type index2) noexcept {
    assert(index1 < dense_.size() && index2 < dense_.size());
    
    if (index1 == index2) return;
    
    const auto entity1 = dense_[index1];
    const auto entity2 = dense_[index2];
    
    // Swap entities in dense array
    dense_[index1] = entity2;
    dense_[index2] = entity1;
    
    // Update sparse array mappings
    if (entity1.id.value < sparse_.size()) {
        sparse_[entity1.id.value] = index2;
    }
    if (entity2.id.value < sparse_.size()) {
        sparse_[entity2.id.value] = index1;
    }
    
    version_.fetch_add(1, std::memory_order_relaxed);
}

// Private implementation methods

inline bool AdvancedSparseSet::contains_impl(entity_type entity) const noexcept {
    lookup_count_.fetch_add(1, std::memory_order_relaxed);
    
    const auto sparse_index = entity.id.value;
    if (sparse_index >= sparse_.size()) {
        return false;
    }
    
    const auto dense_index = sparse_[sparse_index];
    return dense_index < dense_.size() && 
           dense_[dense_index].id == entity.id && 
           dense_[dense_index].generation == entity.generation;
}

inline auto AdvancedSparseSet::get_index_impl(entity_type entity) const noexcept -> index_type {
    lookup_count_.fetch_add(1, std::memory_order_relaxed);
    
    const auto sparse_index = entity.id.value;
    if (sparse_index >= sparse_.size()) {
        return INVALID_INDEX;
    }
    
    const auto dense_index = sparse_[sparse_index];
    if (dense_index >= dense_.size() || 
        dense_[dense_index].id != entity.id || 
        dense_[dense_index].generation != entity.generation) {
        return INVALID_INDEX;
    }
    
    return dense_index;
}

inline auto AdvancedSparseSet::insert_impl(entity_type entity) -> index_type {
    insert_count_.fetch_add(1, std::memory_order_relaxed);
    
    const auto sparse_index = entity.id.value;
    
    // Ensure sparse array is large enough
    ensure_sparse_capacity(entity.id);
    
    // Check if already exists
    const auto existing_dense = sparse_[sparse_index];
    if (existing_dense < dense_.size() && 
        dense_[existing_dense].id == entity.id && 
        dense_[existing_dense].generation == entity.generation) {
        return existing_dense;
    }
    
    // Add to dense array
    if (dense_.size() >= dense_.capacity()) {
        grow_dense_array(dense_.capacity() * config_.dense_growth_factor);
    }
    
    const auto new_dense_index = static_cast<index_type>(dense_.size());
    dense_.push_back(entity);
    sparse_[sparse_index] = new_dense_index;
    
    version_.fetch_add(1, std::memory_order_relaxed);
    return new_dense_index;
}

inline bool AdvancedSparseSet::remove_impl(entity_type entity) noexcept {
    remove_count_.fetch_add(1, std::memory_order_relaxed);
    
    const auto sparse_index = entity.id.value;
    if (sparse_index >= sparse_.size()) {
        return false;
    }
    
    const auto dense_index = sparse_[sparse_index];
    if (dense_index >= dense_.size() || 
        dense_[dense_index].id != entity.id || 
        dense_[dense_index].generation != entity.generation) {
        return false;
    }
    
    // Swap with last element and pop (maintains packed layout)
    const auto last_index = dense_.size() - 1;
    if (dense_index != last_index) {
        const auto last_entity = dense_[last_index];
        dense_[dense_index] = last_entity;
        if (last_entity.id.value < sparse_.size()) {
            sparse_[last_entity.id.value] = dense_index;
        }
    }
    
    dense_.pop_back();
    sparse_[sparse_index] = INVALID_INDEX;
    
    version_.fetch_add(1, std::memory_order_relaxed);
    return true;
}

inline void AdvancedSparseSet::clear_impl() noexcept {
    dense_.clear();
    std::fill(sparse_.begin(), sparse_.end(), INVALID_INDEX);
    version_.fetch_add(1, std::memory_order_relaxed);
}

inline void AdvancedSparseSet::reserve_impl(size_type capacity) {
    if (capacity > dense_.capacity()) {
        dense_.reserve(capacity);
        
        // Also expand sparse array if needed
        const auto sparse_capacity = capacity * 2; // Heuristic: sparse is typically larger
        if (sparse_capacity > sparse_.size()) {
            sparse_.resize(sparse_capacity, INVALID_INDEX);
        }
    }
}

inline void AdvancedSparseSet::shrink_to_fit_impl() {
    dense_.shrink_to_fit();
    // Note: We don't shrink sparse array as it would invalidate entity mappings
}

inline void AdvancedSparseSet::ensure_sparse_capacity(EntityId entity_id) {
    if (entity_id.value >= sparse_.size()) {
        const auto new_capacity = std::max(
            static_cast<size_type>(entity_id.value + 1),
            sparse_.size() * config_.sparse_growth_factor
        );
        grow_sparse_array(new_capacity);
    }
}

inline void AdvancedSparseSet::grow_sparse_array(size_type new_capacity) {
    sparse_growth_count_.fetch_add(1, std::memory_order_relaxed);
    sparse_.resize(new_capacity, INVALID_INDEX);
}

inline void AdvancedSparseSet::grow_dense_array(size_type new_capacity) {
    dense_growth_count_.fetch_add(1, std::memory_order_relaxed);
    dense_.reserve(new_capacity);
}

inline bool AdvancedSparseSet::is_valid_entity(entity_type entity) const noexcept {
    return entity.is_valid();
}

inline auto AdvancedSparseSet::get_memory_stats() const noexcept -> MemoryStats {
    MemoryStats stats{};
    stats.dense_bytes = dense_.capacity() * sizeof(entity_type);
    stats.sparse_bytes = sparse_.capacity() * sizeof(index_type);
    stats.total_bytes = stats.dense_bytes + stats.sparse_bytes;
    stats.dense_capacity = dense_.capacity();
    stats.sparse_capacity = sparse_.capacity();
    stats.utilization = dense_.empty() ? 0.0 : 
                       static_cast<double>(dense_.size()) / dense_.capacity();
    return stats;
}

inline bool AdvancedSparseSet::validate_integrity() const noexcept {
    if constexpr (ECSCOPE_DEBUG_ENABLED) {
        // Check that all dense entities have correct sparse mappings
        for (size_type i = 0; i < dense_.size(); ++i) {
            const auto entity = dense_[i];
            if (entity.id.value >= sparse_.size()) {
                return false;
            }
            if (sparse_[entity.id.value] != i) {
                return false;
            }
        }
        
        // Check that all sparse entries either point to valid dense indices or are INVALID_INDEX
        for (size_type i = 0; i < sparse_.size(); ++i) {
            const auto dense_idx = sparse_[i];
            if (dense_idx != INVALID_INDEX) {
                if (dense_idx >= dense_.size()) {
                    return false;
                }
                if (dense_[dense_idx].id.value != i) {
                    return false;
                }
            }
        }
    }
    return true;
}

inline auto AdvancedSparseSet::get_performance_stats() const noexcept -> PerformanceStats {
    PerformanceStats stats{};
    stats.insert_count = insert_count_.load(std::memory_order_relaxed);
    stats.remove_count = remove_count_.load(std::memory_order_relaxed);
    stats.lookup_count = lookup_count_.load(std::memory_order_relaxed);
    stats.sparse_growth_count = sparse_growth_count_.load(std::memory_order_relaxed);
    stats.dense_growth_count = dense_growth_count_.load(std::memory_order_relaxed);
    
    // Calculate derived metrics
    const auto total_ops = stats.insert_count + stats.remove_count + stats.lookup_count;
    stats.cache_hit_ratio = total_ops > 0 ? 
        static_cast<double>(stats.lookup_count) / total_ops : 0.0;
    
    return stats;
}

inline void AdvancedSparseSet::reset_performance_stats() noexcept {
    insert_count_.store(0, std::memory_order_relaxed);
    remove_count_.store(0, std::memory_order_relaxed);
    lookup_count_.store(0, std::memory_order_relaxed);
    sparse_growth_count_.store(0, std::memory_order_relaxed);
    dense_growth_count_.store(0, std::memory_order_relaxed);
}

// BatchProcessor template implementations

template<typename Func>
void AdvancedSparseSet::BatchProcessor::for_each_with_index(Func&& func) const {
    const auto& entities = set_.dense_;
    
    if (set_.config_.enable_simd_optimization && entities.size() >= set_.config_.simd_batch_size) {
        // Process in SIMD-friendly batches
        const auto batch_size = set_.config_.simd_batch_size;
        const auto full_batches = entities.size() / batch_size;
        
        for (size_type batch = 0; batch < full_batches; ++batch) {
            const auto start_idx = batch * batch_size;
            
            // Prefetch next batch if available
            if (batch + 1 < full_batches) {
                const auto prefetch_idx = (batch + 1) * batch_size;
                core::memory::prefetch_read(&entities[prefetch_idx], 
                                          batch_size * sizeof(entity_type));
            }
            
            // Process current batch
            for (size_type i = 0; i < batch_size; ++i) {
                const auto idx = start_idx + i;
                func(entities[idx], static_cast<index_type>(idx));
            }
        }
        
        // Process remaining elements
        const auto remaining_start = full_batches * batch_size;
        for (size_type i = remaining_start; i < entities.size(); ++i) {
            func(entities[i], static_cast<index_type>(i));
        }
    } else {
        // Simple iteration for small sets or when SIMD is disabled
        for (size_type i = 0; i < entities.size(); ++i) {
            func(entities[i], static_cast<index_type>(i));
        }
    }
}

template<typename Func>
void AdvancedSparseSet::BatchProcessor::parallel_for_each(Func&& func, size_type batch_size) const {
    const auto& entities = set_.dense_;
    
    if (entities.size() < batch_size * 2) {
        // Too small for parallel processing, use sequential
        const auto span = std::span<const entity_type>(entities);
        func(span);
        return;
    }
    
    // In a full implementation, this would use a job system or std::execution
    // For now, we'll simulate with sequential batches
    const auto num_batches = (entities.size() + batch_size - 1) / batch_size;
    
    for (size_type batch = 0; batch < num_batches; ++batch) {
        const auto start = batch * batch_size;
        const auto end = std::min(start + batch_size, entities.size());
        const auto batch_span = std::span<const entity_type>(
            entities.data() + start, end - start);
        
        func(batch_span);
    }
}

// Thread-safe operation helpers

template<typename Func>
auto AdvancedSparseSet::execute_with_lock(Func&& func) const -> decltype(func()) {
    if (config_.enable_thread_safety) {
        std::lock_guard<std::mutex> lock(mutex_);
        return func();
    }
    return func();
}

template<typename Func>
auto AdvancedSparseSet::execute_with_lock(Func&& func) -> decltype(func()) {
    if (config_.enable_thread_safety) {
        std::lock_guard<std::mutex> lock(mutex_);
        return func();
    }
    return func();
}

// BatchProcessor implementations

inline std::vector<AdvancedSparseSet::index_type> 
AdvancedSparseSet::BatchProcessor::batch_insert(std::span<const entity_type> entities) {
    std::vector<index_type> results;
    results.reserve(entities.size());
    
    for (const auto entity : entities) {
        results.push_back(set_.insert(entity));
    }
    
    return results;
}

inline auto AdvancedSparseSet::BatchProcessor::batch_remove(std::span<const entity_type> entities) noexcept -> size_type {
    size_type removed_count = 0;
    
    for (const auto entity : entities) {
        if (set_.remove(entity)) {
            ++removed_count;
        }
    }
    
    return removed_count;
}

inline void AdvancedSparseSet::BatchProcessor::batch_contains(std::span<const entity_type> entities, 
                                                            std::span<bool> results) const noexcept {
    assert(entities.size() == results.size());
    
    if (set_.config_.enable_simd_optimization && entities.size() >= set_.config_.simd_batch_size) {
        // Use SIMD batch processing when available
        detail::SIMDBatchProcessor::batch_contains_simd(
            entities, set_.sparse_, set_.dense_, results);
    } else {
        // Fallback to scalar processing
        for (std::size_t i = 0; i < entities.size(); ++i) {
            results[i] = set_.contains(entities[i]);
        }
    }
}

inline void AdvancedSparseSet::BatchProcessor::batch_get_indices(std::span<const entity_type> entities,
                                                               std::span<index_type> results) const noexcept {
    assert(entities.size() == results.size());
    
    if (set_.config_.enable_simd_optimization && entities.size() >= set_.config_.simd_batch_size) {
        // Use SIMD batch processing when available
        detail::SIMDBatchProcessor::batch_lookup_simd(
            entities, set_.sparse_, set_.dense_, results);
    } else {
        // Fallback to scalar processing
        for (std::size_t i = 0; i < entities.size(); ++i) {
            results[i] = set_.get_index(entities[i]);
        }
    }
}

// SIMD detail implementations (placeholder for actual SIMD code)
namespace detail {

inline void SIMDBatchProcessor::batch_contains_simd(std::span<const EntityHandle> entities,
                                                  std::span<const std::uint32_t> sparse,
                                                  std::span<const EntityHandle> dense,
                                                  std::span<bool> results) noexcept {
    // Placeholder SIMD implementation - in production this would use intrinsics
    for (std::size_t i = 0; i < entities.size(); ++i) {
        const auto entity = entities[i];
        const auto sparse_index = entity.id.value;
        
        if (sparse_index < sparse.size()) {
            const auto dense_index = sparse[sparse_index];
            results[i] = dense_index < dense.size() && 
                        dense[dense_index].id == entity.id && 
                        dense[dense_index].generation == entity.generation;
        } else {
            results[i] = false;
        }
    }
}

inline void SIMDBatchProcessor::batch_lookup_simd(std::span<const EntityHandle> entities,
                                                std::span<const std::uint32_t> sparse,
                                                std::span<const EntityHandle> dense,
                                                std::span<std::uint32_t> results) noexcept {
    // Placeholder SIMD implementation - in production this would use intrinsics
    for (std::size_t i = 0; i < entities.size(); ++i) {
        const auto entity = entities[i];
        const auto sparse_index = entity.id.value;
        
        if (sparse_index < sparse.size()) {
            const auto dense_index = sparse[sparse_index];
            if (dense_index < dense.size() && 
                dense[dense_index].id == entity.id && 
                dense[dense_index].generation == entity.generation) {
                results[i] = dense_index;
            } else {
                results[i] = AdvancedSparseSet::INVALID_INDEX;
            }
        } else {
            results[i] = AdvancedSparseSet::INVALID_INDEX;
        }
    }
}

} // namespace detail

} // namespace ecscope::registry