/**
 * @file query.cpp
 * @brief Implementation of the Advanced ECS Query System
 * 
 * Contains the runtime implementations for query caching, dynamic queries,
 * and query management. This demonstrates advanced ECS query patterns and
 * memory-efficient query result caching.
 */

#include "query.hpp"
#include "registry.hpp"
#include "core/time.hpp"
#include <algorithm>
#include <fstream>
#include <sstream>
#include <iomanip>

namespace ecscope::ecs {

namespace {
    // Global query manager instance
    std::unique_ptr<QueryManager> g_query_manager;
    std::once_flag g_query_manager_init_flag;
    
    void initialize_query_manager() {
        g_query_manager = std::make_unique<QueryManager>();
    }
}

// QueryCache implementation
QueryCache::QueryCache(const QueryConfig& config)
    : config_(config) {
    // Initialize arena allocator for query temporary data
    query_arena_ = std::make_unique<memory::ArenaAllocator>(
        config_.arena_size,
        "QueryCache_Arena",
        true  // Enable tracking
    );
    
    LOG_INFO("QueryCache initialized with {} KB arena", config_.arena_size / 1024);
}

QueryCache::~QueryCache() {
    LOG_INFO("QueryCache destroyed - {} cached queries, {:.2f}% hit ratio",
             cached_results_.size(), get_hit_ratio() * 100.0);
}

std::optional<std::span<const QueryResultEntry>> QueryCache::get_cached_result(u64 query_hash) {
    std::shared_lock lock(cache_mutex_);
    
    auto it = cached_results_.find(query_hash);
    if (it == cached_results_.end()) {
        cache_misses_.fetch_add(1, std::memory_order_relaxed);
        return std::nullopt;
    }
    
    auto& cached_result = *it->second;
    
    // Check if cache is still valid
    if (!is_cache_valid(cached_result)) {
        lock.unlock();
        invalidate_query(query_hash);
        cache_misses_.fetch_add(1, std::memory_order_relaxed);
        return std::nullopt;
    }
    
    // Update access statistics
    cached_result.last_access_time = core::get_time_seconds();
    cached_result.access_count++;
    
    cache_hits_.fetch_add(1, std::memory_order_relaxed);
    return std::span<const QueryResultEntry>(cached_result.entries.data(), cached_result.entries.size());
}

void QueryCache::cache_result(u64 query_hash, const ComponentSignature& signature,
                              std::span<const QueryResultEntry> entries,
                              const std::unordered_set<usize>& archetype_ids) {
    if (!config_.enable_caching || entries.size() > config_.max_entries_per_cache) {
        return;
    }
    
    std::unique_lock lock(cache_mutex_);
    
    // Check if we need to evict old caches
    if (cached_results_.size() >= config_.max_cached_results) {
        remove_expired_caches();
        
        // If still too many, remove least recently used
        if (cached_results_.size() >= config_.max_cached_results) {
            auto oldest_it = std::min_element(cached_results_.begin(), cached_results_.end(),
                [](const auto& a, const auto& b) {
                    return a.second->last_access_time < b.second->last_access_time;
                });
            
            if (oldest_it != cached_results_.end()) {
                cached_results_.erase(oldest_it);
            }
        }
    }
    
    // Create new cached result
    auto cached_result = std::make_unique<CachedQueryResult>();
    cached_result->entries.assign(entries.begin(), entries.end());
    cached_result->query_signature = signature;
    cached_result->archetype_ids = archetype_ids;
    cached_result->creation_time = core::get_time_seconds();
    cached_result->last_access_time = cached_result->creation_time;
    cached_result->access_count = 0;
    cached_result->is_valid = true;
    
    cached_results_[query_hash] = std::move(cached_result);
    
    // Register archetype dependencies for invalidation
    {
        std::lock_guard inv_lock(invalidation_mutex_);
        for (usize archetype_id : archetype_ids) {
            archetype_to_queries_[archetype_id].push_back(query_hash);
        }
    }
}

void QueryCache::invalidate_archetype(usize archetype_id) {
    std::lock_guard inv_lock(invalidation_mutex_);
    
    auto it = archetype_to_queries_.find(archetype_id);
    if (it == archetype_to_queries_.end()) {
        return;
    }
    
    // Invalidate all queries that depend on this archetype
    for (u64 query_hash : it->second) {
        invalidate_query(query_hash);
    }
    
    archetype_to_queries_.erase(it);
    cache_invalidations_.fetch_add(it->second.size(), std::memory_order_relaxed);
}

void QueryCache::invalidate_query(u64 query_hash) {
    std::unique_lock lock(cache_mutex_);
    
    auto it = cached_results_.find(query_hash);
    if (it != cached_results_.end()) {
        it->second->is_valid = false;
        cached_results_.erase(it);
    }
}

void QueryCache::invalidate_all() {
    std::unique_lock lock(cache_mutex_);
    std::lock_guard inv_lock(invalidation_mutex_);
    
    usize count = cached_results_.size();
    cached_results_.clear();
    archetype_to_queries_.clear();
    cache_invalidations_.fetch_add(count, std::memory_order_relaxed);
    
    LOG_INFO("Invalidated all {} cached query results", count);
}

f64 QueryCache::get_hit_ratio() const noexcept {
    u64 hits = cache_hits_.load(std::memory_order_relaxed);
    u64 misses = cache_misses_.load(std::memory_order_relaxed);
    u64 total = hits + misses;
    
    return total > 0 ? static_cast<f64>(hits) / total : 0.0;
}

u64 QueryCache::get_cached_query_count() const noexcept {
    std::shared_lock lock(cache_mutex_);
    return cached_results_.size();
}

usize QueryCache::get_memory_usage() const noexcept {
    std::shared_lock lock(cache_mutex_);
    
    usize total = sizeof(*this);
    total += query_arena_->used_size();
    
    for (const auto& [hash, result] : cached_results_) {
        total += sizeof(*result);
        total += result->entries.size() * sizeof(QueryResultEntry);
        total += result->archetype_ids.size() * sizeof(usize);
    }
    
    return total;
}

void QueryCache::set_config(const QueryConfig& config) {
    std::unique_lock lock(cache_mutex_);
    config_ = config;
    
    // If caching disabled, clear all caches
    if (!config_.enable_caching) {
        cached_results_.clear();
        std::lock_guard inv_lock(invalidation_mutex_);
        archetype_to_queries_.clear();
    }
}

void QueryCache::cleanup_expired_caches() {
    std::unique_lock lock(cache_mutex_);
    remove_expired_caches();
}

void QueryCache::compact_memory() {
    query_arena_->reset();  // Reset arena for fresh allocation
}

bool QueryCache::is_cache_valid(const CachedQueryResult& cache) const noexcept {
    if (!cache.is_valid) {
        return false;
    }
    
    f64 current_time = core::get_time_seconds();
    if (current_time - cache.creation_time > config_.cache_timeout) {
        return false;
    }
    
    return true;
}

void QueryCache::remove_expired_caches() {
    f64 current_time = core::get_time_seconds();
    
    auto it = cached_results_.begin();
    while (it != cached_results_.end()) {
        if (current_time - it->second->creation_time > config_.cache_timeout) {
            it = cached_results_.erase(it);
        } else {
            ++it;
        }
    }
}

// DynamicQuery implementation
DynamicQuery::DynamicQuery(const Registry& registry, QueryCache* cache)
    : registry_(&registry)
    , cache_(cache)
    , query_name_("DynamicQuery")
    , config_(QueryConfig{}) {
}

DynamicQuery& DynamicQuery::require_component(core::ComponentID component_id) {
    required_components_.set(component_id);
    return *this;
}

DynamicQuery& DynamicQuery::forbid_component(core::ComponentID component_id) {
    forbidden_components_.set(component_id);
    return *this;
}

DynamicQuery& DynamicQuery::any_components(const std::vector<core::ComponentID>& component_ids) {
    ComponentSignature any_sig;
    for (auto id : component_ids) {
        any_sig.set(id);
    }
    any_component_groups_.push_back(any_sig);
    return *this;
}

DynamicQuery& DynamicQuery::optional_component(core::ComponentID component_id) {
    optional_components_.push_back(ComponentSignature{});
    optional_components_.back().set(component_id);
    return *this;
}

DynamicQuery& DynamicQuery::named(const std::string& name) {
    query_name_ = name;
    return *this;
}

DynamicQuery& DynamicQuery::with_config(const QueryConfig& config) {
    config_ = config;
    return *this;
}

std::vector<Entity> DynamicQuery::entities() const {
    auto start_time = std::chrono::high_resolution_clock::now();
    
    std::vector<Entity> result;
    auto all_entities = registry_->get_all_entities();
    
    result.reserve(all_entities.size());
    
    for (Entity entity : all_entities) {
        bool matches = true;
        
        // TODO: Implement entity signature checking
        // This would require getting the entity's archetype and checking its signature
        // For now, this is a placeholder implementation
        
        if (matches) {
            result.push_back(entity);
        }
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    f64 execution_time = std::chrono::duration<f64>(end_time - start_time).count();
    
    if (config_.enable_statistics) {
        stats_.total_executions++;
        stats_.total_execution_time += execution_time;
        stats_.total_entities_processed += result.size();
        stats_.update_averages();
    }
    
    return result;
}

usize DynamicQuery::count() const {
    return entities().size();  // Not efficient, but simple for this implementation
}

void DynamicQuery::for_each(std::function<void(Entity, void**)> func) const {
    auto entity_list = entities();
    
    // Allocate component pointer array
    const usize max_components = 8;  // Maximum components we can handle
    void* component_ptrs[max_components];
    
    for (Entity entity : entity_list) {
        // TODO: Gather component pointers for this entity
        // This would require introspecting the entity's archetype
        
        func(entity, component_ptrs);
    }
}

std::string DynamicQuery::get_description() const {
    std::ostringstream desc;
    desc << "DynamicQuery '" << query_name_ << "' [";
    
    if (!required_components_.empty()) {
        desc << "REQUIRE: " << required_components_.count() << " components";
    }
    
    if (!forbidden_components_.empty()) {
        if (!required_components_.empty()) desc << ", ";
        desc << "FORBID: " << forbidden_components_.count() << " components";
    }
    
    if (!any_component_groups_.empty()) {
        if (!required_components_.empty() || !forbidden_components_.empty()) desc << ", ";
        desc << "ANY: " << any_component_groups_.size() << " groups";
    }
    
    desc << "]";
    return desc.str();
}

// QueryManager implementation
QueryManager::QueryManager()
    : cache_(std::make_unique<QueryCache>(QueryConfig{})) {
    LOG_INFO("QueryManager initialized");
}

QueryManager::~QueryManager() {
    LOG_INFO("QueryManager shutdown - {} total queries executed, {:.2f}ms average time",
             total_queries_executed_.load(), get_average_query_time() * 1000.0);
}

void QueryManager::register_query(QueryStats* stats) {
    std::unique_lock lock(queries_mutex_);
    tracked_queries_.push_back(stats);
}

void QueryManager::unregister_query(QueryStats* stats) {
    std::unique_lock lock(queries_mutex_);
    auto it = std::find(tracked_queries_.begin(), tracked_queries_.end(), stats);
    if (it != tracked_queries_.end()) {
        tracked_queries_.erase(it);
    }
}

std::vector<QueryStats> QueryManager::get_all_query_stats() const {
    std::shared_lock lock(queries_mutex_);
    
    std::vector<QueryStats> stats;
    stats.reserve(tracked_queries_.size());
    
    for (const QueryStats* query_stats : tracked_queries_) {
        stats.push_back(*query_stats);
    }
    
    return stats;
}

QueryStats QueryManager::get_combined_stats() const {
    std::shared_lock lock(queries_mutex_);
    
    QueryStats combined;
    combined.query_name = "Combined_All_Queries";
    
    for (const QueryStats* stats : tracked_queries_) {
        combined.total_executions += stats->total_executions;
        combined.total_execution_time += stats->total_execution_time;
        combined.total_entities_processed += stats->total_entities_processed;
        combined.cache_hits += stats->cache_hits;
        combined.cache_misses += stats->cache_misses;
        combined.archetypes_matched += stats->archetypes_matched;
        combined.archetypes_total += stats->archetypes_total;
        combined.memory_accessed += stats->memory_accessed;
        combined.memory_wasted += stats->memory_wasted;
        
        if (stats->min_execution_time < combined.min_execution_time) {
            combined.min_execution_time = stats->min_execution_time;
        }
        if (stats->max_execution_time > combined.max_execution_time) {
            combined.max_execution_time = stats->max_execution_time;
        }
        if (stats->max_entities_per_query > combined.max_entities_per_query) {
            combined.max_entities_per_query = stats->max_entities_per_query;
        }
    }
    
    combined.update_averages();
    return combined;
}

std::vector<QueryStats> QueryManager::get_slowest_queries(usize count) const {
    auto all_stats = get_all_query_stats();
    
    // Sort by average execution time (descending)
    std::sort(all_stats.begin(), all_stats.end(),
              [](const QueryStats& a, const QueryStats& b) {
                  return a.average_execution_time > b.average_execution_time;
              });
    
    if (all_stats.size() > count) {
        all_stats.resize(count);
    }
    
    return all_stats;
}

std::vector<QueryStats> QueryManager::get_most_frequent_queries(usize count) const {
    auto all_stats = get_all_query_stats();
    
    // Sort by total executions (descending)
    std::sort(all_stats.begin(), all_stats.end(),
              [](const QueryStats& a, const QueryStats& b) {
                  return a.total_executions > b.total_executions;
              });
    
    if (all_stats.size() > count) {
        all_stats.resize(count);
    }
    
    return all_stats;
}

f64 QueryManager::get_average_query_time() const {
    u64 total_queries = total_queries_executed_.load(std::memory_order_relaxed);
    f64 total_time = total_query_time_.load(std::memory_order_relaxed);
    
    return total_queries > 0 ? total_time / total_queries : 0.0;
}

usize QueryManager::get_total_memory_usage() const {
    return cache_->get_memory_usage();
}

void QueryManager::set_global_config(const QueryConfig& config) {
    cache_->set_config(config);
}

void QueryManager::optimize_caches() {
    cache_->cleanup_expired_caches();
    cache_->compact_memory();
}

void QueryManager::reset_all_statistics() {
    std::unique_lock lock(queries_mutex_);
    
    for (QueryStats* stats : tracked_queries_) {
        stats->reset();
    }
    
    total_queries_executed_.store(0, std::memory_order_relaxed);
    total_query_time_.store(0.0, std::memory_order_relaxed);
}

// Global query manager access
QueryManager& get_query_manager() {
    std::call_once(g_query_manager_init_flag, initialize_query_manager);
    return *g_query_manager;
}

} // namespace ecscope::ecs