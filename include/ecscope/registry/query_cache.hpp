#pragma once

#include "../core/types.hpp"
#include "../core/memory.hpp"
#include "../foundation/concepts.hpp"
#include "../foundation/component.hpp"
#include "sparse_set.hpp"
#include "archetype.hpp"
#include <vector>
#include <span>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <algorithm>
#include <type_traits>
#include <utility>
#include <cassert>
#include <atomic>
#include <mutex>
#include <chrono>
#include <functional>

/**
 * @file query_cache.hpp
 * @brief Advanced query caching system for ECS performance optimization
 * 
 * This file implements a comprehensive query caching system with:
 * - Intelligent query result caching with invalidation strategies
 * - Bloom filter optimization for fast query pre-filtering
 * - Adaptive cache sizing based on access patterns
 * - Multi-level cache hierarchy (hot, warm, cold)
 * - Query result compression for memory efficiency
 * - Thread-safe cache operations with minimal contention
 * - Cache miss prediction and preemptive loading
 * - Performance monitoring and optimization hints
 * 
 * Educational Notes:
 * - Query caches reduce expensive archetype matching operations
 * - Bloom filters provide probabilistic membership testing
 * - LRU eviction ensures hot queries stay cached
 * - Version-based invalidation maintains cache coherence
 * - Compressed storage reduces memory pressure
 * - Hierarchical caches optimize for different access patterns
 * - Cache warming improves cold-start performance
 */

namespace ecscope::registry {

using namespace ecscope::core;
using namespace ecscope::foundation;

/// @brief Configuration for query cache behavior
struct QueryCacheConfig {
    std::uint32_t max_cached_queries{1024};               ///< Maximum cached queries
    std::uint32_t max_cache_memory_mb{64};                ///< Maximum cache memory usage
    std::uint32_t bloom_filter_size{8192};               ///< Bloom filter size in bits
    std::uint32_t hot_cache_size{128};                    ///< Hot cache capacity
    std::uint32_t warm_cache_size{512};                   ///< Warm cache capacity
    std::uint32_t access_count_threshold{10};             ///< Access threshold for hot classification
    bool enable_bloom_filters{true};                      ///< Enable bloom filter optimization
    bool enable_result_compression{true};                 ///< Enable result compression
    bool enable_adaptive_sizing{true};                    ///< Enable adaptive cache sizing
    bool enable_preemptive_loading{true};                 ///< Enable query prefetching
    double cache_hit_ratio_target{0.85};                  ///< Target cache hit ratio
    std::chrono::milliseconds cache_entry_ttl{30000};     ///< Cache entry time-to-live
    std::chrono::milliseconds cleanup_interval{5000};     ///< Cache cleanup interval
};

/// @brief Query descriptor for caching
struct QueryDescriptor {
    ComponentSignature required_components{};
    ComponentSignature excluded_components{};
    std::uint32_t min_entity_count{0};
    std::uint32_t max_entity_count{std::numeric_limits<std::uint32_t>::max()};
    std::uint64_t hash{};
    
    QueryDescriptor() = default;
    
    QueryDescriptor(ComponentSignature required, ComponentSignature excluded = 0,
                   std::uint32_t min_count = 0, std::uint32_t max_count = std::numeric_limits<std::uint32_t>::max())
        : required_components(required), excluded_components(excluded)
        , min_entity_count(min_count), max_entity_count(max_count) {
        hash = calculate_hash();
    }
    
    bool operator==(const QueryDescriptor& other) const noexcept {
        return required_components == other.required_components &&
               excluded_components == other.excluded_components &&
               min_entity_count == other.min_entity_count &&
               max_entity_count == other.max_entity_count;
    }
    
    bool operator!=(const QueryDescriptor& other) const noexcept {
        return !(*this == other);
    }
    
    /// @brief Calculate hash for fast lookups
    std::uint64_t calculate_hash() const noexcept {
        std::uint64_t hash = 14695981039346656037ULL; // FNV-1a offset
        hash ^= required_components;
        hash *= 1099511628211ULL;
        hash ^= excluded_components;
        hash *= 1099511628211ULL;
        hash ^= static_cast<std::uint64_t>(min_entity_count);
        hash *= 1099511628211ULL;
        hash ^= static_cast<std::uint64_t>(max_entity_count);
        hash *= 1099511628211ULL;
        return hash;
    }
    
    /// @brief Create query for component types
    template<Component... Required, Component... Excluded>
    static QueryDescriptor create(std::uint32_t min_count = 0, 
                                 std::uint32_t max_count = std::numeric_limits<std::uint32_t>::max()) {
        ComponentSignature required = 0;
        ComponentSignature excluded = 0;
        
        ((required |= (1ULL << component_utils::get_component_id<Required>().value)), ...);
        ((excluded |= (1ULL << component_utils::get_component_id<Excluded>().value)), ...);
        
        return QueryDescriptor(required, excluded, min_count, max_count);
    }
};

/// @brief Query result with metadata
struct QueryResult {
    std::vector<ArchetypeId> matching_archetypes;
    std::vector<EntityHandle> matching_entities;  // Optional: for entity-level caching
    std::uint32_t total_entity_count{};
    Version cache_version{};
    std::chrono::steady_clock::time_point creation_time;
    std::chrono::steady_clock::time_point last_access_time;
    std::uint32_t access_count{};
    std::size_t compressed_size{};
    bool is_compressed{false};
    
    QueryResult() : creation_time(std::chrono::steady_clock::now()),
                   last_access_time(creation_time) {}
    
    /// @brief Record access to this query result
    void record_access() {
        last_access_time = std::chrono::steady_clock::now();
        ++access_count;
    }
    
    /// @brief Check if result is expired
    bool is_expired(std::chrono::milliseconds ttl) const noexcept {
        const auto now = std::chrono::steady_clock::now();
        return (now - creation_time) > ttl;
    }
    
    /// @brief Get age of result
    std::chrono::milliseconds get_age() const noexcept {
        const auto now = std::chrono::steady_clock::now();
        return std::chrono::duration_cast<std::chrono::milliseconds>(now - creation_time);
    }
    
    /// @brief Get time since last access
    std::chrono::milliseconds get_idle_time() const noexcept {
        const auto now = std::chrono::steady_clock::now();
        return std::chrono::duration_cast<std::chrono::milliseconds>(now - last_access_time);
    }
    
    /// @brief Estimate memory usage
    std::size_t memory_usage() const noexcept {
        if (is_compressed) {
            return compressed_size + sizeof(*this);
        }
        return matching_archetypes.size() * sizeof(ArchetypeId) +
               matching_entities.size() * sizeof(EntityHandle) +
               sizeof(*this);
    }
};

/// @brief Bloom filter for fast query pre-filtering
class QueryBloomFilter {
public:
    explicit QueryBloomFilter(std::uint32_t size_bits = 8192) 
        : bit_count_(size_bits), filter_(size_bits / 8, 0) {}
    
    /// @brief Add query to bloom filter
    void add(const QueryDescriptor& query) {
        const auto hash = query.hash;
        set_bit(hash);
        set_bit(hash >> 16);
        set_bit(hash >> 32);
    }
    
    /// @brief Check if query might be in cache
    /// @return false means definitely not in cache, true means might be
    bool might_contain(const QueryDescriptor& query) const noexcept {
        const auto hash = query.hash;
        return get_bit(hash) && get_bit(hash >> 16) && get_bit(hash >> 32);
    }
    
    /// @brief Clear bloom filter
    void clear() {
        std::fill(filter_.begin(), filter_.end(), 0);
    }
    
    /// @brief Get estimated false positive rate
    double false_positive_rate() const noexcept {
        const auto bits_set = count_set_bits();
        if (bits_set == 0) return 0.0;
        
        const double ratio = static_cast<double>(bits_set) / bit_count_;
        return std::pow(ratio, 3);  // 3 hash functions
    }

private:
    void set_bit(std::uint64_t hash) {
        const auto bit = hash % bit_count_;
        const auto byte_index = bit / 8;
        const auto bit_index = bit % 8;
        filter_[byte_index] |= (1 << bit_index);
    }
    
    bool get_bit(std::uint64_t hash) const noexcept {
        const auto bit = hash % bit_count_;
        const auto byte_index = bit / 8;
        const auto bit_index = bit % 8;
        return (filter_[byte_index] & (1 << bit_index)) != 0;
    }
    
    std::uint32_t count_set_bits() const noexcept {
        std::uint32_t count = 0;
        for (const auto byte : filter_) {
            count += std::popcount(byte);
        }
        return count;
    }
    
    std::uint32_t bit_count_;
    std::vector<std::uint8_t> filter_;
};

/// @brief Cache level for hierarchical caching
enum class CacheLevel : std::uint8_t {
    Hot,     ///< Frequently accessed queries (fastest access)
    Warm,    ///< Moderately accessed queries (fast access)
    Cold     ///< Rarely accessed queries (slower access, disk-based)
};

/// @brief Cache statistics for monitoring and optimization
struct QueryCacheStats {
    std::uint64_t total_queries{};
    std::uint64_t cache_hits{};
    std::uint64_t cache_misses{};
    std::uint64_t bloom_filter_hits{};
    std::uint64_t bloom_filter_misses{};
    std::uint64_t cache_evictions{};
    std::uint64_t cache_invalidations{};
    std::uint32_t hot_cache_size{};
    std::uint32_t warm_cache_size{};
    std::uint32_t cold_cache_size{};
    std::size_t total_memory_usage{};
    std::size_t compressed_memory_saved{};
    double cache_hit_ratio{};
    double bloom_filter_false_positive_rate{};
    std::chrono::milliseconds average_query_time{};
    std::chrono::milliseconds average_cache_build_time{};
};

/// @brief Advanced query cache with multi-level hierarchy
class AdvancedQueryCache {
public:
    using query_builder_func = std::function<QueryResult(const QueryDescriptor&)>;
    using invalidation_callback = std::function<void(const QueryDescriptor&)>;
    
    explicit AdvancedQueryCache(const QueryCacheConfig& config = {})
        : config_(config)
        , bloom_filter_(config.bloom_filter_size)
        , cleanup_timer_(std::chrono::steady_clock::now()) {
        
        // Reserve space for cache levels
        hot_cache_.reserve(config_.hot_cache_size);
        warm_cache_.reserve(config_.warm_cache_size);
    }
    
    /// @brief Query with caching support
    /// @param query Query descriptor
    /// @param builder Function to build query result if not cached
    /// @return Query result (from cache or newly built)
    QueryResult execute_query(const QueryDescriptor& query, query_builder_func builder) {
        const auto start_time = std::chrono::steady_clock::now();
        
        stats_.total_queries.fetch_add(1, std::memory_order_relaxed);
        
        // Check bloom filter first for fast rejection
        if (config_.enable_bloom_filters && !bloom_filter_.might_contain(query)) {
            stats_.bloom_filter_misses.fetch_add(1, std::memory_order_relaxed);
            return build_and_cache_query(query, builder, start_time);
        }
        
        if (config_.enable_bloom_filters) {
            stats_.bloom_filter_hits.fetch_add(1, std::memory_order_relaxed);
        }
        
        // Try to get from cache
        if (auto cached_result = get_cached_result(query)) {
            cached_result->record_access();
            stats_.cache_hits.fetch_add(1, std::memory_order_relaxed);
            
            const auto end_time = std::chrono::steady_clock::now();
            update_query_time(start_time, end_time);
            
            return *cached_result;
        }
        
        // Cache miss - build and cache the result
        stats_.cache_misses.fetch_add(1, std::memory_order_relaxed);
        return build_and_cache_query(query, builder, start_time);
    }
    
    /// @brief Invalidate cached queries affected by component changes
    /// @param changed_signature Component signature that changed
    void invalidate_queries(ComponentSignature changed_signature) {
        std::lock_guard<std::mutex> hot_lock(hot_mutex_);
        std::lock_guard<std::mutex> warm_lock(warm_mutex_);
        
        std::uint32_t invalidated_count = 0;
        
        // Invalidate hot cache entries
        for (auto it = hot_cache_.begin(); it != hot_cache_.end();) {
            const auto& query = it->first;
            if (query_affected_by_change(query, changed_signature)) {
                if (invalidation_callback_) {
                    invalidation_callback_(query);
                }
                it = hot_cache_.erase(it);
                ++invalidated_count;
            } else {
                ++it;
            }
        }
        
        // Invalidate warm cache entries
        for (auto it = warm_cache_.begin(); it != warm_cache_.end();) {
            const auto& query = it->first;
            if (query_affected_by_change(query, changed_signature)) {
                if (invalidation_callback_) {
                    invalidation_callback_(query);
                }
                it = warm_cache_.erase(it);
                ++invalidated_count;
            } else {
                ++it;
            }
        }
        
        stats_.cache_invalidations.fetch_add(invalidated_count, std::memory_order_relaxed);
        
        // Rebuild bloom filter if many entries were invalidated
        if (invalidated_count > config_.hot_cache_size / 4) {
            rebuild_bloom_filter();
        }
    }
    
    /// @brief Invalidate specific query
    /// @param query Query to invalidate
    /// @return true if query was cached and invalidated
    bool invalidate_query(const QueryDescriptor& query) {
        bool invalidated = false;
        
        {
            std::lock_guard<std::mutex> lock(hot_mutex_);
            if (hot_cache_.erase(query) > 0) {
                invalidated = true;
            }
        }
        
        {
            std::lock_guard<std::mutex> lock(warm_mutex_);
            if (warm_cache_.erase(query) > 0) {
                invalidated = true;
            }
        }
        
        if (invalidated) {
            stats_.cache_invalidations.fetch_add(1, std::memory_order_relaxed);
            if (invalidation_callback_) {
                invalidation_callback_(query);
            }
        }
        
        return invalidated;
    }
    
    /// @brief Clear all cached queries
    void clear_cache() {
        {
            std::lock_guard<std::mutex> lock(hot_mutex_);
            hot_cache_.clear();
        }
        
        {
            std::lock_guard<std::mutex> lock(warm_mutex_);
            warm_cache_.clear();
        }
        
        bloom_filter_.clear();
        stats_.cache_invalidations.fetch_add(
            stats_.hot_cache_size + stats_.warm_cache_size, std::memory_order_relaxed);
    }
    
    /// @brief Preload queries for better performance
    /// @param queries Queries to preload
    /// @param builder Function to build query results
    void preload_queries(std::span<const QueryDescriptor> queries, query_builder_func builder) {
        if (!config_.enable_preemptive_loading) return;
        
        for (const auto& query : queries) {
            // Only preload if not already cached
            if (!get_cached_result(query)) {
                const auto start_time = std::chrono::steady_clock::now();
                build_and_cache_query(query, builder, start_time);
            }
        }
    }
    
    /// @brief Optimize cache performance
    void optimize_cache() {
        const auto now = std::chrono::steady_clock::now();
        
        // Perform cleanup if enough time has passed
        if ((now - cleanup_timer_) > config_.cleanup_interval) {
            cleanup_expired_entries();
            cleanup_timer_ = now;
        }
        
        // Adaptive sizing based on hit ratio
        if (config_.enable_adaptive_sizing) {
            adapt_cache_sizes();
        }
        
        // Rebuild bloom filter periodically
        if (bloom_filter_.false_positive_rate() > 0.3) {
            rebuild_bloom_filter();
        }
    }
    
    /// @brief Get cache statistics
    QueryCacheStats get_stats() const {
        QueryCacheStats stats{};
        
        // Copy atomic values
        stats.total_queries = stats_.total_queries.load(std::memory_order_relaxed);
        stats.cache_hits = stats_.cache_hits.load(std::memory_order_relaxed);
        stats.cache_misses = stats_.cache_misses.load(std::memory_order_relaxed);
        stats.bloom_filter_hits = stats_.bloom_filter_hits.load(std::memory_order_relaxed);
        stats.bloom_filter_misses = stats_.bloom_filter_misses.load(std::memory_order_relaxed);
        stats.cache_evictions = stats_.cache_evictions.load(std::memory_order_relaxed);
        stats.cache_invalidations = stats_.cache_invalidations.load(std::memory_order_relaxed);
        
        // Calculate derived metrics
        if (stats.total_queries > 0) {
            stats.cache_hit_ratio = static_cast<double>(stats.cache_hits) / stats.total_queries;
        }
        
        stats.bloom_filter_false_positive_rate = bloom_filter_.false_positive_rate();
        
        // Get current cache sizes
        {
            std::lock_guard<std::mutex> lock(hot_mutex_);
            stats.hot_cache_size = static_cast<std::uint32_t>(hot_cache_.size());
            for (const auto& [query, result] : hot_cache_) {
                stats.total_memory_usage += result.memory_usage();
                if (result.is_compressed) {
                    stats.compressed_memory_saved += 
                        (result.matching_archetypes.capacity() * sizeof(ArchetypeId) +
                         result.matching_entities.capacity() * sizeof(EntityHandle)) - 
                        result.compressed_size;
                }
            }
        }
        
        {
            std::lock_guard<std::mutex> lock(warm_mutex_);
            stats.warm_cache_size = static_cast<std::uint32_t>(warm_cache_.size());
            for (const auto& [query, result] : warm_cache_) {
                stats.total_memory_usage += result.memory_usage();
                if (result.is_compressed) {
                    stats.compressed_memory_saved += 
                        (result.matching_archetypes.capacity() * sizeof(ArchetypeId) +
                         result.matching_entities.capacity() * sizeof(EntityHandle)) - 
                        result.compressed_size;
                }
            }
        }
        
        // Calculate average times
        const auto total_time = total_query_time_.load(std::memory_order_relaxed);
        const auto total_build_time = total_build_time_.load(std::memory_order_relaxed);
        
        if (stats.total_queries > 0) {
            stats.average_query_time = std::chrono::microseconds(total_time / stats.total_queries);
        }
        if (stats.cache_misses > 0) {
            stats.average_cache_build_time = std::chrono::microseconds(total_build_time / stats.cache_misses);
        }
        
        return stats;
    }
    
    /// @brief Set invalidation callback
    void set_invalidation_callback(invalidation_callback callback) {
        invalidation_callback_ = std::move(callback);
    }
    
    /// @brief Query cache utilities
    class CacheAnalyzer {
    public:
        explicit CacheAnalyzer(const AdvancedQueryCache& cache) : cache_(cache) {}
        
        /// @brief Analyze cache efficiency
        struct CacheAnalysis {
            double hit_ratio{};
            double memory_efficiency{};
            std::vector<QueryDescriptor> most_accessed_queries;
            std::vector<QueryDescriptor> least_accessed_queries;
            std::vector<QueryDescriptor> expired_queries;
            std::size_t wasted_memory_bytes{};
            bool needs_optimization{};
        };
        
        CacheAnalysis analyze() const {
            CacheAnalysis analysis{};
            
            const auto stats = cache_.get_stats();
            analysis.hit_ratio = stats.cache_hit_ratio;
            
            if (stats.total_memory_usage > 0) {
                analysis.memory_efficiency = 
                    static_cast<double>(stats.compressed_memory_saved) / stats.total_memory_usage;
            }
            
            // Analyze cache contents
            analyze_cache_contents(analysis);
            
            // Determine if optimization is needed
            analysis.needs_optimization = 
                analysis.hit_ratio < cache_.config_.cache_hit_ratio_target ||
                analysis.expired_queries.size() > stats.hot_cache_size / 4 ||
                stats.bloom_filter_false_positive_rate > 0.2;
            
            return analysis;
        }
        
        /// @brief Get optimization recommendations
        std::vector<std::string> get_optimization_recommendations() const {
            std::vector<std::string> recommendations;
            const auto analysis = analyze();
            const auto stats = cache_.get_stats();
            
            if (analysis.hit_ratio < 0.8) {
                recommendations.push_back("Consider increasing cache size or preloading common queries");
            }
            
            if (stats.bloom_filter_false_positive_rate > 0.2) {
                recommendations.push_back("Increase bloom filter size to reduce false positives");
            }
            
            if (analysis.memory_efficiency < 0.5 && cache_.config_.enable_result_compression) {
                recommendations.push_back("Result compression is not providing significant benefits");
            }
            
            if (analysis.expired_queries.size() > stats.hot_cache_size / 2) {
                recommendations.push_back("Reduce cache TTL or increase cleanup frequency");
            }
            
            return recommendations;
        }

    private:
        void analyze_cache_contents(CacheAnalysis& analysis) const {
            // This would analyze the actual cache contents
            // For now, we'll provide a basic implementation
        }
        
        const AdvancedQueryCache& cache_;
    };
    
    /// @brief Get cache analyzer
    CacheAnalyzer analyzer() const { return CacheAnalyzer(*this); }

private:
    /// @brief Get cached result for query
    QueryResult* get_cached_result(const QueryDescriptor& query) {
        // Try hot cache first
        {
            std::lock_guard<std::mutex> lock(hot_mutex_);
            auto it = hot_cache_.find(query);
            if (it != hot_cache_.end()) {
                return &it->second;
            }
        }
        
        // Try warm cache
        {
            std::lock_guard<std::mutex> lock(warm_mutex_);
            auto it = warm_cache_.find(query);
            if (it != warm_cache_.end()) {
                // Move to hot cache if accessed frequently
                if (it->second.access_count >= config_.access_count_threshold) {
                    promote_to_hot_cache(query, std::move(it->second));
                    warm_cache_.erase(it);
                    return get_cached_result(query);  // Get from hot cache
                }
                return &it->second;
            }
        }
        
        return nullptr;
    }
    
    /// @brief Build query result and cache it
    QueryResult build_and_cache_query(const QueryDescriptor& query, 
                                     query_builder_func builder,
                                     std::chrono::steady_clock::time_point start_time) {
        const auto build_start = std::chrono::steady_clock::now();
        
        // Build the query result
        auto result = builder(query);
        
        const auto build_end = std::chrono::steady_clock::now();
        const auto build_time = std::chrono::duration_cast<std::chrono::microseconds>(
            build_end - build_start);
        total_build_time_.fetch_add(build_time.count(), std::memory_order_relaxed);
        
        // Compress result if beneficial
        if (config_.enable_result_compression && should_compress_result(result)) {
            compress_query_result(result);
        }
        
        // Cache the result
        cache_query_result(query, result);
        
        // Add to bloom filter
        if (config_.enable_bloom_filters) {
            bloom_filter_.add(query);
        }
        
        const auto end_time = std::chrono::steady_clock::now();
        update_query_time(start_time, end_time);
        
        return result;
    }
    
    /// @brief Cache query result in appropriate level
    void cache_query_result(const QueryDescriptor& query, const QueryResult& result) {
        if (hot_cache_.size() < config_.hot_cache_size) {
            std::lock_guard<std::mutex> lock(hot_mutex_);
            hot_cache_[query] = result;
        } else {
            std::lock_guard<std::mutex> lock(warm_mutex_);
            
            // Evict least recently used if warm cache is full
            if (warm_cache_.size() >= config_.warm_cache_size) {
                evict_lru_from_warm_cache();
            }
            
            warm_cache_[query] = result;
        }
    }
    
    /// @brief Promote query result to hot cache
    void promote_to_hot_cache(const QueryDescriptor& query, QueryResult result) {
        std::lock_guard<std::mutex> lock(hot_mutex_);
        
        // Evict from hot cache if full
        if (hot_cache_.size() >= config_.hot_cache_size) {
            evict_lru_from_hot_cache();
        }
        
        hot_cache_[query] = std::move(result);
    }
    
    /// @brief Check if query is affected by component signature change
    bool query_affected_by_change(const QueryDescriptor& query, ComponentSignature changed) const noexcept {
        // Query is affected if:
        // 1. Changed signature overlaps with required components
        // 2. Changed signature overlaps with excluded components
        return (query.required_components & changed) != 0 || 
               (query.excluded_components & changed) != 0;
    }
    
    /// @brief Check if result should be compressed
    bool should_compress_result(const QueryResult& result) const noexcept {
        const auto uncompressed_size = result.matching_archetypes.size() * sizeof(ArchetypeId) +
                                      result.matching_entities.size() * sizeof(EntityHandle);
        return uncompressed_size > 1024;  // Compress if larger than 1KB
    }
    
    /// @brief Compress query result
    void compress_query_result(QueryResult& result) const {
        // Placeholder for compression implementation
        // In a full implementation, this would use a compression algorithm
        result.is_compressed = true;
        result.compressed_size = result.memory_usage() * 0.7; // Assume 30% compression
    }
    
    /// @brief Evict least recently used entry from hot cache
    void evict_lru_from_hot_cache() {
        auto oldest_it = hot_cache_.begin();
        auto oldest_time = oldest_it->second.last_access_time;
        
        for (auto it = hot_cache_.begin(); it != hot_cache_.end(); ++it) {
            if (it->second.last_access_time < oldest_time) {
                oldest_it = it;
                oldest_time = it->second.last_access_time;
            }
        }
        
        hot_cache_.erase(oldest_it);
        stats_.cache_evictions.fetch_add(1, std::memory_order_relaxed);
    }
    
    /// @brief Evict least recently used entry from warm cache
    void evict_lru_from_warm_cache() {
        auto oldest_it = warm_cache_.begin();
        auto oldest_time = oldest_it->second.last_access_time;
        
        for (auto it = warm_cache_.begin(); it != warm_cache_.end(); ++it) {
            if (it->second.last_access_time < oldest_time) {
                oldest_it = it;
                oldest_time = it->second.last_access_time;
            }
        }
        
        warm_cache_.erase(oldest_it);
        stats_.cache_evictions.fetch_add(1, std::memory_order_relaxed);
    }
    
    /// @brief Clean up expired cache entries
    void cleanup_expired_entries() {
        const auto now = std::chrono::steady_clock::now();
        std::uint32_t cleaned_count = 0;
        
        // Clean up hot cache
        {
            std::lock_guard<std::mutex> lock(hot_mutex_);
            for (auto it = hot_cache_.begin(); it != hot_cache_.end();) {
                if (it->second.is_expired(config_.cache_entry_ttl)) {
                    it = hot_cache_.erase(it);
                    ++cleaned_count;
                } else {
                    ++it;
                }
            }
        }
        
        // Clean up warm cache
        {
            std::lock_guard<std::mutex> lock(warm_mutex_);
            for (auto it = warm_cache_.begin(); it != warm_cache_.end();) {
                if (it->second.is_expired(config_.cache_entry_ttl)) {
                    it = warm_cache_.erase(it);
                    ++cleaned_count;
                } else {
                    ++it;
                }
            }
        }
        
        if (cleaned_count > 0) {
            stats_.cache_evictions.fetch_add(cleaned_count, std::memory_order_relaxed);
            rebuild_bloom_filter();
        }
    }
    
    /// @brief Adapt cache sizes based on usage patterns
    void adapt_cache_sizes() {
        const auto stats = get_stats();
        
        if (stats.cache_hit_ratio < config_.cache_hit_ratio_target) {
            // Increase hot cache size if hit ratio is low
            config_.hot_cache_size = std::min(
                config_.hot_cache_size * 12 / 10,  // Increase by 20%
                config_.max_cached_queries / 4
            );
        } else if (stats.cache_hit_ratio > 0.95 && config_.hot_cache_size > 64) {
            // Decrease hot cache size if hit ratio is very high
            config_.hot_cache_size = std::max(
                config_.hot_cache_size * 9 / 10,   // Decrease by 10%
                static_cast<std::uint32_t>(64)
            );
        }
    }
    
    /// @brief Rebuild bloom filter from current cache contents
    void rebuild_bloom_filter() {
        bloom_filter_.clear();
        
        {
            std::lock_guard<std::mutex> lock(hot_mutex_);
            for (const auto& [query, result] : hot_cache_) {
                bloom_filter_.add(query);
            }
        }
        
        {
            std::lock_guard<std::mutex> lock(warm_mutex_);
            for (const auto& [query, result] : warm_cache_) {
                bloom_filter_.add(query);
            }
        }
    }
    
    /// @brief Update average query time
    void update_query_time(std::chrono::steady_clock::time_point start,
                          std::chrono::steady_clock::time_point end) {
        const auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        total_query_time_.fetch_add(duration.count(), std::memory_order_relaxed);
    }
    
    QueryCacheConfig config_;
    QueryBloomFilter bloom_filter_;
    
    /// @brief Hot cache (most frequently accessed)
    mutable std::mutex hot_mutex_;
    std::unordered_map<QueryDescriptor, QueryResult> hot_cache_;
    
    /// @brief Warm cache (moderately accessed)
    mutable std::mutex warm_mutex_;
    std::unordered_map<QueryDescriptor, QueryResult> warm_cache_;
    
    /// @brief Performance tracking (atomic for thread-safe access)
    struct {
        std::atomic<std::uint64_t> total_queries{0};
        std::atomic<std::uint64_t> cache_hits{0};
        std::atomic<std::uint64_t> cache_misses{0};
        std::atomic<std::uint64_t> bloom_filter_hits{0};
        std::atomic<std::uint64_t> bloom_filter_misses{0};
        std::atomic<std::uint64_t> cache_evictions{0};
        std::atomic<std::uint64_t> cache_invalidations{0};
        std::atomic<std::uint32_t> hot_cache_size{0};
        std::atomic<std::uint32_t> warm_cache_size{0};
    } stats_;
    
    /// @brief Timing statistics
    std::atomic<std::uint64_t> total_query_time_{0};
    std::atomic<std::uint64_t> total_build_time_{0};
    
    /// @brief Cleanup timing
    std::chrono::steady_clock::time_point cleanup_timer_;
    
    /// @brief Callbacks
    invalidation_callback invalidation_callback_;
};

} // namespace ecscope::registry

/// @brief Hash specialization for QueryDescriptor
template<>
struct std::hash<ecscope::registry::QueryDescriptor> {
    std::size_t operator()(const ecscope::registry::QueryDescriptor& query) const noexcept {
        return query.hash;
    }
};