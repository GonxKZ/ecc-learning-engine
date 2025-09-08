#pragma once

#include "../core/types.hpp"
#include "../core/log.hpp"
#include "query_engine.hpp"

#include <unordered_map>
#include <list>
#include <memory>
#include <chrono>
#include <vector>
#include <bitset>
#include <functional>
#include <mutex>
#include <atomic>
#include <algorithm>
#include <type_traits>
#include <optional>

namespace ecscope::ecs::query {

// Forward declaration
template<typename... Components>
class QueryResult;

/**
 * @brief Bloom filter for fast negative lookups in query cache
 * 
 * Implements a space-efficient probabilistic data structure that can
 * quickly determine if a query has definitely not been cached, with
 * no false negatives but possible false positives.
 */
class BloomFilter {
private:
    static constexpr usize DEFAULT_SIZE = 8192;  // 8K bits = 1KB
    static constexpr usize NUM_HASH_FUNCTIONS = 3;
    
    std::vector<std::bitset<DEFAULT_SIZE / sizeof(u64)>> bit_arrays_;
    usize size_;
    usize hash_count_;
    std::atomic<usize> elements_added_{0};
    
    // Hash function implementations
    u64 hash1(const std::string& key) const {
        // FNV-1a hash
        constexpr u64 FNV_OFFSET_BASIS = 14695981039346656037ULL;
        constexpr u64 FNV_PRIME = 1099511628211ULL;
        
        u64 hash = FNV_OFFSET_BASIS;
        for (char c : key) {
            hash ^= static_cast<u64>(c);
            hash *= FNV_PRIME;
        }
        return hash;
    }
    
    u64 hash2(const std::string& key) const {
        // DJB2 hash
        u64 hash = 5381;
        for (char c : key) {
            hash = ((hash << 5) + hash) + static_cast<u64>(c);
        }
        return hash;
    }
    
    u64 hash3(const std::string& key) const {
        // SDBM hash
        u64 hash = 0;
        for (char c : key) {
            hash = static_cast<u64>(c) + (hash << 6) + (hash << 16) - hash;
        }
        return hash;
    }
    
    std::vector<u64> compute_hashes(const std::string& key) const {
        std::vector<u64> hashes;
        hashes.reserve(hash_count_);
        
        u64 h1 = hash1(key);
        u64 h2 = hash2(key);
        u64 h3 = hash3(key);
        
        hashes.push_back(h1 % size_);
        hashes.push_back(h2 % size_);
        hashes.push_back(h3 % size_);
        
        return hashes;
    }
    
public:
    explicit BloomFilter(usize estimated_elements = 10000, f64 false_positive_rate = 0.01)
        : hash_count_(NUM_HASH_FUNCTIONS) {
        
        // Calculate optimal size based on expected elements and false positive rate
        size_ = static_cast<usize>(-static_cast<f64>(estimated_elements) * 
                                  std::log(false_positive_rate) / 
                                  (std::log(2) * std::log(2)));
        
        // Ensure size is reasonable
        size_ = std::max(size_, usize(1024));
        size_ = std::min(size_, usize(1024 * 1024)); // Max 1MB
        
        // Initialize bit arrays
        usize num_arrays = (size_ + DEFAULT_SIZE - 1) / DEFAULT_SIZE;
        bit_arrays_.resize(num_arrays);
        
        LOG_DEBUG("BloomFilter initialized: size={}, hash_functions={}, arrays={}", 
                 size_, hash_count_, num_arrays);
    }
    
    /**
     * @brief Add key to bloom filter
     */
    void add(const std::string& key) {
        auto hashes = compute_hashes(key);
        
        for (u64 hash : hashes) {
            usize array_index = hash / DEFAULT_SIZE;
            usize bit_index = hash % DEFAULT_SIZE;
            
            if (array_index < bit_arrays_.size()) {
                bit_arrays_[array_index].set(bit_index);
            }
        }
        
        elements_added_++;
    }
    
    /**
     * @brief Check if key might be in the set
     * @return false = definitely not in set, true = might be in set
     */
    bool might_contain(const std::string& key) const {
        auto hashes = compute_hashes(key);
        
        for (u64 hash : hashes) {
            usize array_index = hash / DEFAULT_SIZE;
            usize bit_index = hash % DEFAULT_SIZE;
            
            if (array_index >= bit_arrays_.size()) {
                return false;
            }
            
            if (!bit_arrays_[array_index].test(bit_index)) {
                return false;
            }
        }
        
        return true;
    }
    
    /**
     * @brief Clear all entries
     */
    void clear() {
        for (auto& array : bit_arrays_) {
            array.reset();
        }
        elements_added_ = 0;
    }
    
    /**
     * @brief Get statistics
     */
    struct Statistics {
        usize elements_added;
        usize size_bits;
        f64 estimated_false_positive_rate;
        f64 fill_ratio;
    };
    
    Statistics get_statistics() const {
        Statistics stats;
        stats.elements_added = elements_added_.load();
        stats.size_bits = size_;
        
        // Count set bits
        usize set_bits = 0;
        for (const auto& array : bit_arrays_) {
            set_bits += array.count();
        }
        
        stats.fill_ratio = static_cast<f64>(set_bits) / size_;
        
        // Estimate false positive rate based on fill ratio
        stats.estimated_false_positive_rate = 
            std::pow(stats.fill_ratio, static_cast<f64>(hash_count_));
        
        return stats;
    }
};

/**
 * @brief Cache entry with metadata and expiration
 */
struct CacheEntry {
    std::shared_ptr<void> data;
    std::chrono::steady_clock::time_point created_at;
    std::chrono::steady_clock::time_point last_accessed;
    f64 ttl_seconds;
    usize access_count;
    std::string query_hash;
    usize data_size;
    
    CacheEntry(std::shared_ptr<void> d, f64 ttl, const std::string& hash, usize size)
        : data(std::move(d))
        , created_at(std::chrono::steady_clock::now())
        , last_accessed(created_at)
        , ttl_seconds(ttl)
        , access_count(1)
        , query_hash(hash)
        , data_size(size) {}
    
    bool is_expired() const {
        auto now = std::chrono::steady_clock::now();
        auto age = std::chrono::duration<f64>(now - created_at).count();
        return age > ttl_seconds;
    }
    
    void touch() {
        last_accessed = std::chrono::steady_clock::now();
        access_count++;
    }
    
    f64 age_seconds() const {
        auto now = std::chrono::steady_clock::now();
        return std::chrono::duration<f64>(now - created_at).count();
    }
};

/**
 * @brief LRU (Least Recently Used) cache implementation for query results
 */
class LRUCache {
private:
    using CacheList = std::list<std::string>;
    using CacheMap = std::unordered_map<std::string, 
                                       std::pair<CacheEntry, CacheList::iterator>>;
    
    CacheMap cache_;
    CacheList lru_list_;
    usize max_entries_;
    mutable std::mutex cache_mutex_;
    
    // Statistics
    mutable std::atomic<usize> hits_{0};
    mutable std::atomic<usize> misses_{0};
    mutable std::atomic<usize> evictions_{0};
    mutable std::atomic<usize> expirations_{0};
    std::atomic<usize> total_memory_usage_{0};
    
    void move_to_front(CacheMap::iterator& cache_it) {
        // Move to front of LRU list
        lru_list_.erase(cache_it->second.second);
        lru_list_.push_front(cache_it->first);
        cache_it->second.second = lru_list_.begin();
    }
    
    void evict_lru() {
        if (lru_list_.empty()) return;
        
        std::string lru_key = lru_list_.back();
        auto cache_it = cache_.find(lru_key);
        
        if (cache_it != cache_.end()) {
            total_memory_usage_ -= cache_it->second.first.data_size;
            cache_.erase(cache_it);
            evictions_++;
        }
        
        lru_list_.pop_back();
    }
    
    void cleanup_expired() {
        auto now = std::chrono::steady_clock::now();
        std::vector<std::string> expired_keys;
        
        for (const auto& [key, entry_pair] : cache_) {
            if (entry_pair.first.is_expired()) {
                expired_keys.push_back(key);
            }
        }
        
        for (const auto& key : expired_keys) {
            auto cache_it = cache_.find(key);
            if (cache_it != cache_.end()) {
                total_memory_usage_ -= cache_it->second.first.data_size;
                lru_list_.erase(cache_it->second.second);
                cache_.erase(cache_it);
                expirations_++;
            }
        }
    }
    
public:
    explicit LRUCache(usize max_entries = 10000) 
        : max_entries_(max_entries) {
        LOG_DEBUG("LRUCache initialized with capacity: {}", max_entries);
    }
    
    template<typename T>
    void put(const std::string& key, std::shared_ptr<T> value, 
             f64 ttl_seconds, usize estimated_size = sizeof(T)) {
        std::lock_guard<std::mutex> lock(cache_mutex_);
        
        // Remove existing entry if present
        auto existing = cache_.find(key);
        if (existing != cache_.end()) {
            total_memory_usage_ -= existing->second.first.data_size;
            lru_list_.erase(existing->second.second);
            cache_.erase(existing);
        }
        
        // Evict expired entries periodically
        if (cache_.size() % 100 == 0) {
            cleanup_expired();
        }
        
        // Evict LRU entries if at capacity
        while (cache_.size() >= max_entries_) {
            evict_lru();
        }
        
        // Add new entry
        lru_list_.push_front(key);
        CacheEntry entry(std::static_pointer_cast<void>(value), 
                        ttl_seconds, key, estimated_size);
        
        cache_[key] = std::make_pair(std::move(entry), lru_list_.begin());
        total_memory_usage_ += estimated_size;
    }
    
    template<typename T>
    std::shared_ptr<T> get(const std::string& key) {
        std::lock_guard<std::mutex> lock(cache_mutex_);
        
        auto cache_it = cache_.find(key);
        if (cache_it == cache_.end()) {
            misses_++;
            return nullptr;
        }
        
        // Check expiration
        if (cache_it->second.first.is_expired()) {
            total_memory_usage_ -= cache_it->second.first.data_size;
            lru_list_.erase(cache_it->second.second);
            cache_.erase(cache_it);
            expirations_++;
            misses_++;
            return nullptr;
        }
        
        // Update access info and move to front
        cache_it->second.first.touch();
        move_to_front(cache_it);
        hits_++;
        
        return std::static_pointer_cast<T>(cache_it->second.first.data);
    }
    
    void remove(const std::string& key) {
        std::lock_guard<std::mutex> lock(cache_mutex_);
        
        auto cache_it = cache_.find(key);
        if (cache_it != cache_.end()) {
            total_memory_usage_ -= cache_it->second.first.data_size;
            lru_list_.erase(cache_it->second.second);
            cache_.erase(cache_it);
        }
    }
    
    void clear() {
        std::lock_guard<std::mutex> lock(cache_mutex_);
        cache_.clear();
        lru_list_.clear();
        total_memory_usage_ = 0;
    }
    
    struct Statistics {
        usize hits;
        usize misses;
        usize evictions;
        usize expirations;
        usize entries;
        usize memory_usage_bytes;
        f64 hit_ratio;
    };
    
    Statistics get_statistics() const {
        std::lock_guard<std::mutex> lock(cache_mutex_);
        
        Statistics stats;
        stats.hits = hits_.load();
        stats.misses = misses_.load();
        stats.evictions = evictions_.load();
        stats.expirations = expirations_.load();
        stats.entries = cache_.size();
        stats.memory_usage_bytes = total_memory_usage_.load();
        
        usize total_accesses = stats.hits + stats.misses;
        stats.hit_ratio = total_accesses > 0 ? 
            static_cast<f64>(stats.hits) / total_accesses : 0.0;
        
        return stats;
    }
};

/**
 * @brief Hierarchical query cache with intelligent invalidation
 */
class QueryCache {
private:
    std::unique_ptr<LRUCache> primary_cache_;
    std::unique_ptr<BloomFilter> bloom_filter_;
    
    // Cache configuration
    usize max_entries_;
    f64 default_ttl_seconds_;
    
    // Invalidation tracking
    std::unordered_map<std::string, std::vector<std::string>> dependency_map_;
    std::atomic<u64> registry_version_{0}; // Tracks ECS state changes
    std::unordered_map<std::string, u64> query_versions_;
    
    // Performance monitoring
    mutable std::atomic<usize> bloom_hits_{0};
    mutable std::atomic<usize> bloom_misses_{0};
    
    std::string generate_dependency_key(const std::string& component_type) const {
        return "comp:" + component_type;
    }
    
    void add_dependency(const std::string& query_hash, const std::string& dependency) {
        dependency_map_[dependency].push_back(query_hash);
    }
    
public:
    explicit QueryCache(usize max_entries = 10000, f64 default_ttl = 5.0)
        : primary_cache_(std::make_unique<LRUCache>(max_entries))
        , bloom_filter_(std::make_unique<BloomFilter>(max_entries * 2))
        , max_entries_(max_entries)
        , default_ttl_seconds_(default_ttl) {
        
        LOG_INFO("QueryCache initialized: max_entries={}, default_ttl={}s", 
                max_entries, default_ttl);
    }
    
    /**
     * @brief Store query result in cache
     */
    template<typename... Components>
    void store(const std::string& query_hash, 
               const QueryResult<Components...>& result,
               f64 ttl_seconds = 0.0) {
        
        if (ttl_seconds <= 0.0) {
            ttl_seconds = default_ttl_seconds_;
        }
        
        // Estimate memory usage
        usize estimated_size = sizeof(QueryResult<Components...>) + 
                              result.size() * sizeof(typename QueryResult<Components...>::EntityTuple);
        
        // Store in primary cache
        auto result_copy = std::make_shared<QueryResult<Components...>>(result);
        primary_cache_->put(query_hash, result_copy, ttl_seconds, estimated_size);
        
        // Add to bloom filter
        bloom_filter_->add(query_hash);
        
        // Track dependencies for invalidation
        (add_dependency(query_hash, generate_dependency_key(typeid(Components).name())), ...);
        
        // Record query version
        query_versions_[query_hash] = registry_version_.load();
        
        LOG_DEBUG("Cached query result: hash={}, size={}, ttl={}s", 
                 query_hash, result.size(), ttl_seconds);
    }
    
    /**
     * @brief Get query result from cache
     */
    template<typename... Components>
    std::optional<QueryResult<Components...>> get(const std::string& query_hash) const {
        // Quick bloom filter check
        if (!bloom_filter_->might_contain(query_hash)) {
            bloom_misses_++;
            return std::nullopt;
        }
        bloom_hits_++;
        
        // Check version compatibility
        auto version_it = query_versions_.find(query_hash);
        if (version_it != query_versions_.end() && 
            version_it->second != registry_version_.load()) {
            // Query is outdated, remove from cache
            primary_cache_->remove(query_hash);
            return std::nullopt;
        }
        
        // Get from primary cache
        auto cached_result = primary_cache_->get<QueryResult<Components...>>(query_hash);
        if (cached_result) {
            LOG_DEBUG("Cache hit: {}", query_hash);
            return *cached_result;
        }
        
        LOG_DEBUG("Cache miss: {}", query_hash);
        return std::nullopt;
    }
    
    /**
     * @brief Invalidate queries that depend on specific components
     */
    template<typename... Components>
    void invalidate_component_queries() {
        std::vector<std::string> to_invalidate;
        
        ((to_invalidate.insert(to_invalidate.end(),
                              dependency_map_[generate_dependency_key(typeid(Components).name())].begin(),
                              dependency_map_[generate_dependency_key(typeid(Components).name())].end())), ...);
        
        for (const auto& query_hash : to_invalidate) {
            primary_cache_->remove(query_hash);
            query_versions_.erase(query_hash);
        }
        
        // Clear dependencies
        ((dependency_map_[generate_dependency_key(typeid(Components).name())].clear()), ...);
        
        LOG_DEBUG("Invalidated {} queries due to component changes", to_invalidate.size());
    }
    
    /**
     * @brief Notify cache of ECS state changes
     */
    void on_registry_changed() {
        registry_version_++;
        LOG_DEBUG("Registry version updated to: {}", registry_version_.load());
    }
    
    /**
     * @brief Clear all cached data
     */
    void clear() {
        primary_cache_->clear();
        bloom_filter_->clear();
        dependency_map_.clear();
        query_versions_.clear();
        registry_version_ = 0;
        bloom_hits_ = 0;
        bloom_misses_ = 0;
        
        LOG_INFO("QueryCache cleared");
    }
    
    /**
     * @brief Update cache configuration
     */
    void update_config(usize max_entries, f64 default_ttl) {
        max_entries_ = max_entries;
        default_ttl_seconds_ = default_ttl;
        
        // Note: Changing max_entries requires recreating the cache
        // For simplicity, we'll just clear it
        if (max_entries != max_entries_) {
            clear();
            primary_cache_ = std::make_unique<LRUCache>(max_entries);
            bloom_filter_ = std::make_unique<BloomFilter>(max_entries * 2);
        }
        
        LOG_INFO("QueryCache configuration updated: max_entries={}, ttl={}s", 
                max_entries, default_ttl);
    }
    
    /**
     * @brief Get comprehensive cache statistics
     */
    struct CacheStatistics {
        LRUCache::Statistics lru_stats;
        BloomFilter::Statistics bloom_stats;
        usize bloom_hits;
        usize bloom_misses;
        f64 bloom_efficiency;
        usize dependency_count;
        u64 registry_version;
    };
    
    CacheStatistics get_statistics() const {
        CacheStatistics stats;
        stats.lru_stats = primary_cache_->get_statistics();
        stats.bloom_stats = bloom_filter_->get_statistics();
        stats.bloom_hits = bloom_hits_.load();
        stats.bloom_misses = bloom_misses_.load();
        
        usize total_bloom_checks = stats.bloom_hits + stats.bloom_misses;
        stats.bloom_efficiency = total_bloom_checks > 0 ? 
            static_cast<f64>(stats.bloom_misses) / total_bloom_checks : 0.0;
        
        stats.dependency_count = dependency_map_.size();
        stats.registry_version = registry_version_.load();
        
        return stats;
    }
    
    /**
     * @brief Generate detailed cache report
     */
    std::string generate_report() const {
        auto stats = get_statistics();
        
        std::ostringstream oss;
        oss << "=== Query Cache Report ===\n";
        
        // LRU Cache stats
        oss << "Primary Cache:\n";
        oss << "  Entries: " << stats.lru_stats.entries << "/" << max_entries_ << "\n";
        oss << "  Hit Ratio: " << (stats.lru_stats.hit_ratio * 100.0) << "%\n";
        oss << "  Memory Usage: " << (stats.lru_stats.memory_usage_bytes / 1024) << " KB\n";
        oss << "  Evictions: " << stats.lru_stats.evictions << "\n";
        oss << "  Expirations: " << stats.lru_stats.expirations << "\n";
        
        // Bloom filter stats
        oss << "\nBloom Filter:\n";
        oss << "  Elements: " << stats.bloom_stats.elements_added << "\n";
        oss << "  Fill Ratio: " << (stats.bloom_stats.fill_ratio * 100.0) << "%\n";
        oss << "  Est. False Positive Rate: " << (stats.bloom_stats.estimated_false_positive_rate * 100.0) << "%\n";
        oss << "  Efficiency: " << (stats.bloom_efficiency * 100.0) << "% negative lookups avoided\n";
        
        // General stats
        oss << "\nGeneral:\n";
        oss << "  Dependencies: " << stats.dependency_count << "\n";
        oss << "  Registry Version: " << stats.registry_version << "\n";
        
        return oss.str();
    }
};

} // namespace ecscope::ecs::query