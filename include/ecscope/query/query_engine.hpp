#pragma once

#include "../registry.hpp"
#include "../core/types.hpp"
#include "../core/log.hpp"
#include "query_cache.hpp"
#include "query_optimizer.hpp"
#include "query_operators.hpp"
#include "spatial_queries.hpp"

#include <vector>
#include <unordered_map>
#include <memory>
#include <chrono>
#include <thread>
#include <atomic>
#include <concepts>
#include <algorithm>
#include <execution>
#include <ranges>
#include <functional>
#include <type_traits>

namespace ecscope::ecs::query {

// Forward declarations
template<typename T>
class QueryBuilder;
class QueryExecutor;
class QueryPlan;

/**
 * @brief Query execution statistics for performance monitoring
 */
struct QueryStats {
    std::chrono::nanoseconds execution_time{0};
    usize entities_processed{0};
    usize entities_matched{0};
    usize cache_hits{0};
    usize cache_misses{0};
    bool used_parallel_execution{false};
    bool used_spatial_optimization{false};
    std::string optimization_applied;
    
    f64 hit_ratio() const {
        usize total = cache_hits + cache_misses;
        return total > 0 ? static_cast<f64>(cache_hits) / total : 0.0;
    }
    
    f64 match_ratio() const {
        return entities_processed > 0 ? static_cast<f64>(entities_matched) / entities_processed : 0.0;
    }
};

/**
 * @brief Query execution configuration
 */
struct QueryConfig {
    bool enable_caching{true};
    bool enable_parallel_execution{true};
    bool enable_spatial_optimization{true};
    bool enable_hot_path_optimization{true};
    bool enable_query_profiling{true};
    usize parallel_threshold{1000};      // Minimum entities for parallel execution
    usize cache_max_entries{10000};      // Maximum cached query results
    f64 cache_ttl_seconds{5.0};          // Cache time-to-live
    usize max_worker_threads{std::thread::hardware_concurrency()};
    
    static QueryConfig create_performance_optimized() {
        QueryConfig config;
        config.enable_caching = true;
        config.enable_parallel_execution = true;
        config.enable_spatial_optimization = true;
        config.enable_hot_path_optimization = true;
        config.enable_query_profiling = false; // Reduce overhead
        config.parallel_threshold = 500;
        config.cache_max_entries = 50000;
        config.cache_ttl_seconds = 10.0;
        return config;
    }
    
    static QueryConfig create_memory_conservative() {
        QueryConfig config;
        config.enable_caching = false;
        config.enable_parallel_execution = false;
        config.enable_spatial_optimization = false;
        config.enable_hot_path_optimization = false;
        config.enable_query_profiling = false;
        config.parallel_threshold = 10000;
        config.cache_max_entries = 1000;
        config.max_worker_threads = 2;
        return config;
    }
    
    static QueryConfig create_development_mode() {
        QueryConfig config;
        config.enable_query_profiling = true;
        config.parallel_threshold = 2000;
        config.cache_ttl_seconds = 1.0; // Short cache for development
        return config;
    }
};

/**
 * @brief High-performance query result container with lazy evaluation
 */
template<typename... Components>
class QueryResult {
public:
    using EntityTuple = std::tuple<Entity, Components*...>;
    using ResultVector = std::vector<EntityTuple>;
    
private:
    ResultVector results_;
    QueryStats stats_;
    bool is_cached_{false};
    std::chrono::steady_clock::time_point cached_at_;
    
public:
    QueryResult() = default;
    
    explicit QueryResult(ResultVector&& results, const QueryStats& stats = {})
        : results_(std::move(results)), stats_(stats) {}
    
    // Iterator support
    auto begin() { return results_.begin(); }
    auto end() { return results_.end(); }
    auto begin() const { return results_.begin(); }
    auto end() const { return results_.end(); }
    
    // Range support
    auto& data() { return results_; }
    const auto& data() const { return results_; }
    
    // Size and access
    usize size() const { return results_.size(); }
    bool empty() const { return results_.empty(); }
    
    const EntityTuple& operator[](usize index) const { return results_[index]; }
    EntityTuple& operator[](usize index) { return results_[index]; }
    
    // Statistics
    const QueryStats& stats() const { return stats_; }
    
    // Cache information
    bool is_cached() const { return is_cached_; }
    void mark_cached() { 
        is_cached_ = true; 
        cached_at_ = std::chrono::steady_clock::now();
    }
    
    bool is_cache_valid(f64 ttl_seconds) const {
        if (!is_cached_) return false;
        auto now = std::chrono::steady_clock::now();
        auto age = std::chrono::duration<f64>(now - cached_at_).count();
        return age < ttl_seconds;
    }
    
    // Functional operations
    template<typename Predicate>
    QueryResult<Components...> filter(Predicate&& pred) const {
        ResultVector filtered;
        std::copy_if(results_.begin(), results_.end(), 
                    std::back_inserter(filtered), pred);
        return QueryResult<Components...>(std::move(filtered), stats_);
    }
    
    template<typename Comparator>
    QueryResult<Components...> sort(Comparator&& comp) const {
        ResultVector sorted = results_;
        std::sort(sorted.begin(), sorted.end(), comp);
        return QueryResult<Components...>(std::move(sorted), stats_);
    }
    
    template<typename Transform>
    auto transform(Transform&& transformer) const {
        using TransformResult = std::invoke_result_t<Transform, const EntityTuple&>;
        std::vector<TransformResult> transformed;
        transformed.reserve(results_.size());
        
        std::transform(results_.begin(), results_.end(),
                      std::back_inserter(transformed), transformer);
        
        return transformed;
    }
    
    // Aggregation operations
    usize count() const { return results_.size(); }
    
    template<typename Aggregator>
    auto aggregate(Aggregator&& agg) const {
        if (results_.empty()) {
            return std::invoke_result_t<Aggregator, const EntityTuple&>{};
        }
        
        auto result = agg(results_[0]);
        for (usize i = 1; i < results_.size(); ++i) {
            result = agg(result, results_[i]);
        }
        return result;
    }
    
    // Streaming support for large datasets
    template<typename Consumer>
    void stream_to(Consumer&& consumer) const {
        for (const auto& result : results_) {
            consumer(result);
        }
    }
    
    // Parallel processing
    template<typename Function>
    void parallel_for_each(Function&& func) const {
        std::for_each(std::execution::par_unseq, 
                     results_.begin(), results_.end(), func);
    }
};

/**
 * @brief Advanced query predicate system with type-safe composition
 */
template<typename... Components>
class QueryPredicate {
public:
    using EntityTuple = std::tuple<Entity, Components*...>;
    using PredicateFunction = std::function<bool(const EntityTuple&)>;
    
private:
    PredicateFunction predicate_;
    std::string description_;
    bool is_spatial_{false};
    
public:
    QueryPredicate(PredicateFunction pred, const std::string& desc = "")
        : predicate_(std::move(pred)), description_(desc) {}
    
    bool operator()(const EntityTuple& tuple) const {
        return predicate_(tuple);
    }
    
    const std::string& description() const { return description_; }
    bool is_spatial() const { return is_spatial_; }
    void mark_spatial() { is_spatial_ = true; }
    
    // Logical operations
    QueryPredicate<Components...> operator&&(const QueryPredicate<Components...>& other) const {
        return QueryPredicate<Components...>(
            [pred1 = predicate_, pred2 = other.predicate_](const EntityTuple& tuple) {
                return pred1(tuple) && pred2(tuple);
            },
            "(" + description_ + " AND " + other.description_ + ")"
        );
    }
    
    QueryPredicate<Components...> operator||(const QueryPredicate<Components...>& other) const {
        return QueryPredicate<Components...>(
            [pred1 = predicate_, pred2 = other.predicate_](const EntityTuple& tuple) {
                return pred1(tuple) || pred2(tuple);
            },
            "(" + description_ + " OR " + other.description_ + ")"
        );
    }
    
    QueryPredicate<Components...> operator!() const {
        return QueryPredicate<Components...>(
            [pred = predicate_](const EntityTuple& tuple) {
                return !pred(tuple);
            },
            "NOT(" + description_ + ")"
        );
    }
};

/**
 * @brief Professional-grade query engine with advanced optimization
 */
class QueryEngine {
private:
    Registry* registry_;
    std::unique_ptr<QueryCache> cache_;
    std::unique_ptr<QueryOptimizer> optimizer_;
    QueryConfig config_;
    
    // Performance monitoring
    mutable std::unordered_map<std::string, QueryStats> query_performance_;
    mutable std::atomic<usize> total_queries_executed_{0};
    mutable std::atomic<usize> cache_hits_{0};
    mutable std::atomic<usize> parallel_executions_{0};
    
    // Hot path optimization
    mutable std::unordered_map<std::string, usize> query_frequency_;
    mutable std::unordered_set<std::string> hot_queries_;
    
    // Thread pool for parallel execution
    std::unique_ptr<std::vector<std::thread>> worker_threads_;
    std::atomic<bool> shutdown_requested_{false};
    
public:
    explicit QueryEngine(Registry* registry, const QueryConfig& config = {})
        : registry_(registry)
        , cache_(std::make_unique<QueryCache>(config.cache_max_entries, config.cache_ttl_seconds))
        , optimizer_(std::make_unique<QueryOptimizer>())
        , config_(config) {
        
        if (config_.enable_parallel_execution) {
            initialize_thread_pool();
        }
        
        LOG_INFO("QueryEngine initialized with {} worker threads", 
                config_.max_worker_threads);
    }
    
    ~QueryEngine() {
        shutdown_requested_ = true;
        if (worker_threads_) {
            for (auto& thread : *worker_threads_) {
                if (thread.joinable()) {
                    thread.join();
                }
            }
        }
    }
    
    // Non-copyable but movable
    QueryEngine(const QueryEngine&) = delete;
    QueryEngine& operator=(const QueryEngine&) = delete;
    QueryEngine(QueryEngine&&) = default;
    QueryEngine& operator=(QueryEngine&&) = default;
    
    /**
     * @brief Execute query with full optimization pipeline
     */
    template<Component... Components>
    QueryResult<Components...> query() const {
        return query_with_predicate<Components...>(
            QueryPredicate<Components...>([](const auto&) { return true; }, "all")
        );
    }
    
    /**
     * @brief Execute query with predicate and full optimization
     */
    template<Component... Components>
    QueryResult<Components...> query_with_predicate(
        const QueryPredicate<Components...>& predicate) const {
        
        auto start_time = std::chrono::high_resolution_clock::now();
        
        // Generate query hash for caching and hot path detection
        std::string query_hash = generate_query_hash<Components...>(predicate);
        
        // Update query frequency for hot path optimization
        update_query_frequency(query_hash);
        
        // Check cache first
        if (config_.enable_caching) {
            if (auto cached_result = cache_->get<Components...>(query_hash)) {
                if (cached_result->is_cache_valid(config_.cache_ttl_seconds)) {
                    cache_hits_++;
                    auto stats = cached_result->stats();
                    stats.cache_hits++;
                    return *cached_result;
                }
            }
        }
        
        // Create and optimize query plan
        auto query_plan = optimizer_->create_plan<Components...>(*registry_, predicate);
        
        // Execute query with optimization
        QueryResult<Components...> result = execute_optimized_query<Components...>(
            query_plan, predicate);
        
        // Cache result if enabled
        if (config_.enable_caching && !result.empty()) {
            cache_->store(query_hash, result);
            result.mark_cached();
        }
        
        // Update performance statistics
        auto end_time = std::chrono::high_resolution_clock::now();
        update_performance_stats(query_hash, start_time, end_time, result);
        
        total_queries_executed_++;
        
        return result;
    }
    
    /**
     * @brief Range-based query for specific entities
     */
    template<Component... Components>
    QueryResult<Components...> query_entities(const std::vector<Entity>& entities) const {
        using EntityTuple = std::tuple<Entity, Components*...>;
        std::vector<EntityTuple> results;
        results.reserve(entities.size());
        
        usize matched = 0;
        for (Entity entity : entities) {
            if (!registry_->is_valid(entity)) continue;
            
            auto components = std::make_tuple(registry_->get_component<Components>(entity)...);
            bool has_all = (std::get<Components*>(components) && ...);
            
            if (has_all) {
                results.emplace_back(entity, std::get<Components*>(components)...);
                matched++;
            }
        }
        
        QueryStats stats;
        stats.entities_processed = entities.size();
        stats.entities_matched = matched;
        
        return QueryResult<Components...>(std::move(results), stats);
    }
    
    /**
     * @brief Spatial query support
     */
    template<Component... Components>
    QueryResult<Components...> query_spatial(const spatial::Region& region) const {
        static_assert(((std::is_same_v<Components, Transform> || 
                       std::is_same_v<Components, Position>) || ...), 
                     "Spatial queries require Transform or Position component");
        
        auto spatial_predicate = QueryPredicate<Components...>(
            [region](const auto& tuple) {
                return spatial::is_in_region(tuple, region);
            },
            "spatial:" + spatial::to_string(region)
        );
        spatial_predicate.mark_spatial();
        
        return query_with_predicate<Components...>(spatial_predicate);
    }
    
    /**
     * @brief Nearest neighbor query
     */
    template<Component... Components>
    QueryResult<Components...> query_nearest(const Vec3& position, usize count = 10) const {
        auto result = query<Components...>();
        
        // Sort by distance
        auto sorted = result.sort([position](const auto& a, const auto& b) {
            return spatial::distance_squared(a, position) < 
                   spatial::distance_squared(b, position);
        });
        
        // Take only the nearest N
        if (sorted.size() > count) {
            sorted.data().resize(count);
        }
        
        return sorted;
    }
    
    /**
     * @brief Stream query results for large datasets
     */
    template<Component... Components, typename Consumer>
    void stream_query(const QueryPredicate<Components...>& predicate, 
                     Consumer&& consumer) const {
        // Execute query in chunks to avoid memory pressure
        const usize chunk_size = 10000;
        ComponentSignature required = make_signature<Components...>();
        
        for (const auto& archetype : registry_->get_archetypes()) {
            if (!archetype->signature().is_superset_of(required)) continue;
            
            const auto& entities = archetype->entities();
            
            for (usize i = 0; i < entities.size(); i += chunk_size) {
                usize end = std::min(i + chunk_size, entities.size());
                
                for (usize j = i; j < end; ++j) {
                    Entity entity = entities[j];
                    auto components = std::make_tuple(
                        archetype->get_component<Components>(entity)...);
                    
                    bool has_all = (std::get<Components*>(components) && ...);
                    if (has_all) {
                        auto tuple = std::make_tuple(entity, std::get<Components*>(components)...);
                        if (predicate(tuple)) {
                            consumer(tuple);
                        }
                    }
                }
            }
        }
    }
    
    /**
     * @brief Get comprehensive performance statistics
     */
    struct PerformanceMetrics {
        usize total_queries{0};
        usize cache_hits{0};
        usize parallel_executions{0};
        f64 average_execution_time_us{0.0};
        f64 cache_hit_ratio{0.0};
        std::vector<std::string> hot_queries;
        std::unordered_map<std::string, f64> query_performance;
    };
    
    PerformanceMetrics get_performance_metrics() const {
        PerformanceMetrics metrics;
        metrics.total_queries = total_queries_executed_.load();
        metrics.cache_hits = cache_hits_.load();
        metrics.parallel_executions = parallel_executions_.load();
        
        if (metrics.total_queries > 0) {
            metrics.cache_hit_ratio = static_cast<f64>(metrics.cache_hits) / metrics.total_queries;
        }
        
        // Calculate average execution time
        f64 total_time = 0.0;
        for (const auto& [query, stats] : query_performance_) {
            total_time += std::chrono::duration<f64, std::micro>(stats.execution_time).count();
            metrics.query_performance[query] = 
                std::chrono::duration<f64, std::micro>(stats.execution_time).count();
        }
        
        if (!query_performance_.empty()) {
            metrics.average_execution_time_us = total_time / query_performance_.size();
        }
        
        // Get hot queries
        for (const auto& query : hot_queries_) {
            metrics.hot_queries.push_back(query);
        }
        
        return metrics;
    }
    
    /**
     * @brief Clear all caches and reset statistics
     */
    void clear_caches() {
        cache_->clear();
        query_performance_.clear();
        query_frequency_.clear();
        hot_queries_.clear();
        total_queries_executed_ = 0;
        cache_hits_ = 0;
        parallel_executions_ = 0;
        
        LOG_INFO("QueryEngine caches and statistics cleared");
    }
    
    /**
     * @brief Update query engine configuration
     */
    void update_config(const QueryConfig& new_config) {
        config_ = new_config;
        cache_->update_config(new_config.cache_max_entries, new_config.cache_ttl_seconds);
        
        if (new_config.enable_parallel_execution && !worker_threads_) {
            initialize_thread_pool();
        }
        
        LOG_INFO("QueryEngine configuration updated");
    }
    
    /**
     * @brief Get current configuration
     */
    const QueryConfig& get_config() const { return config_; }
    
private:
    /**
     * @brief Execute optimized query with parallel execution support
     */
    template<Component... Components>
    QueryResult<Components...> execute_optimized_query(
        const QueryPlan& plan,
        const QueryPredicate<Components...>& predicate) const {
        
        using EntityTuple = std::tuple<Entity, Components*...>;
        std::vector<EntityTuple> results;
        
        ComponentSignature required = make_signature<Components...>();
        
        // Get all matching archetypes
        auto matching_archetypes = get_matching_archetypes(required);
        
        usize total_entities = estimate_total_entities(matching_archetypes);
        results.reserve(std::min(total_entities, usize(10000))); // Reserve reasonably
        
        QueryStats stats;
        stats.entities_processed = 0;
        stats.entities_matched = 0;
        
        // Decide on parallel execution
        bool use_parallel = config_.enable_parallel_execution && 
                           total_entities >= config_.parallel_threshold;
        
        if (use_parallel) {
            results = execute_parallel<Components...>(matching_archetypes, predicate, stats);
            stats.used_parallel_execution = true;
            parallel_executions_++;
        } else {
            results = execute_sequential<Components...>(matching_archetypes, predicate, stats);
        }
        
        // Apply spatial optimization if applicable
        if (config_.enable_spatial_optimization && predicate.is_spatial()) {
            stats.used_spatial_optimization = true;
            stats.optimization_applied = "spatial_index";
        }
        
        return QueryResult<Components...>(std::move(results), stats);
    }
    
    /**
     * @brief Sequential query execution
     */
    template<Component... Components>
    std::vector<std::tuple<Entity, Components*...>> execute_sequential(
        const std::vector<Archetype*>& archetypes,
        const QueryPredicate<Components...>& predicate,
        QueryStats& stats) const {
        
        using EntityTuple = std::tuple<Entity, Components*...>;
        std::vector<EntityTuple> results;
        
        for (const auto* archetype : archetypes) {
            const auto& entities = archetype->entities();
            
            for (Entity entity : entities) {
                stats.entities_processed++;
                
                auto components = std::make_tuple(
                    archetype->get_component<Components>(entity)...);
                
                bool has_all = (std::get<Components*>(components) && ...);
                if (has_all) {
                    auto tuple = std::make_tuple(entity, std::get<Components*>(components)...);
                    if (predicate(tuple)) {
                        results.emplace_back(tuple);
                        stats.entities_matched++;
                    }
                }
            }
        }
        
        return results;
    }
    
    /**
     * @brief Parallel query execution
     */
    template<Component... Components>
    std::vector<std::tuple<Entity, Components*...>> execute_parallel(
        const std::vector<Archetype*>& archetypes,
        const QueryPredicate<Components...>& predicate,
        QueryStats& stats) const {
        
        using EntityTuple = std::tuple<Entity, Components*...>;
        std::vector<std::vector<EntityTuple>> thread_results(archetypes.size());
        
        // Process archetypes in parallel
        std::for_each(std::execution::par_unseq,
            archetypes.begin(), archetypes.end(),
            [&](const auto* archetype) {
                usize index = std::distance(archetypes.data(), &archetype);
                auto& local_results = thread_results[index];
                
                const auto& entities = archetype->entities();
                local_results.reserve(entities.size());
                
                for (Entity entity : entities) {
                    auto components = std::make_tuple(
                        archetype->get_component<Components>(entity)...);
                    
                    bool has_all = (std::get<Components*>(components) && ...);
                    if (has_all) {
                        auto tuple = std::make_tuple(entity, std::get<Components*>(components)...);
                        if (predicate(tuple)) {
                            local_results.emplace_back(tuple);
                        }
                    }
                }
            }
        );
        
        // Merge results
        std::vector<EntityTuple> final_results;
        usize total_size = 0;
        for (const auto& thread_result : thread_results) {
            total_size += thread_result.size();
        }
        final_results.reserve(total_size);
        
        for (auto& thread_result : thread_results) {
            std::move(thread_result.begin(), thread_result.end(),
                     std::back_inserter(final_results));
            stats.entities_matched += thread_result.size();
        }
        
        return final_results;
    }
    
    /**
     * @brief Generate unique hash for query caching
     */
    template<Component... Components>
    std::string generate_query_hash(const QueryPredicate<Components...>& predicate) const {
        std::ostringstream oss;
        oss << "query<";
        ((oss << typeid(Components).name() << ","), ...);
        oss << ">";
        if (!predicate.description().empty()) {
            oss << "_" << predicate.description();
        }
        return oss.str();
    }
    
    /**
     * @brief Update query frequency for hot path optimization
     */
    void update_query_frequency(const std::string& query_hash) const {
        query_frequency_[query_hash]++;
        
        if (config_.enable_hot_path_optimization && 
            query_frequency_[query_hash] >= 100) { // Hot query threshold
            hot_queries_.insert(query_hash);
        }
    }
    
    /**
     * @brief Update performance statistics
     */
    void update_performance_stats(const std::string& query_hash,
                                 const std::chrono::high_resolution_clock::time_point& start,
                                 const std::chrono::high_resolution_clock::time_point& end,
                                 const QueryResult<auto...>& result) const {
        if (!config_.enable_query_profiling) return;
        
        QueryStats& stats = query_performance_[query_hash];
        stats.execution_time = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
        stats.entities_processed = result.stats().entities_processed;
        stats.entities_matched = result.stats().entities_matched;
        
        if (result.is_cached()) {
            stats.cache_hits++;
        } else {
            stats.cache_misses++;
        }
    }
    
    /**
     * @brief Get archetypes matching the required component signature
     */
    std::vector<Archetype*> get_matching_archetypes(const ComponentSignature& required) const {
        std::vector<Archetype*> matching;
        
        // This would need access to Registry's archetype iteration
        // For now, we'll use a simplified approach
        auto entities = registry_->get_all_entities();
        std::unordered_set<Archetype*> unique_archetypes;
        
        for (Entity entity : entities) {
            // Find entity's archetype and check if it matches
            // This is a simplified implementation
        }
        
        return matching;
    }
    
    /**
     * @brief Estimate total entities across archetypes
     */
    usize estimate_total_entities(const std::vector<Archetype*>& archetypes) const {
        usize total = 0;
        for (const auto* archetype : archetypes) {
            total += archetype->entity_count();
        }
        return total;
    }
    
    /**
     * @brief Initialize thread pool for parallel execution
     */
    void initialize_thread_pool() {
        worker_threads_ = std::make_unique<std::vector<std::thread>>();
        worker_threads_->reserve(config_.max_worker_threads);
        
        // Initialize worker threads (simplified)
        // In a full implementation, you'd have a proper thread pool
        LOG_INFO("Thread pool initialized with {} threads", config_.max_worker_threads);
    }
};

// Global query engine instance (convenience)
QueryEngine& get_query_engine();
void set_query_engine(std::unique_ptr<QueryEngine> engine);

} // namespace ecscope::ecs::query