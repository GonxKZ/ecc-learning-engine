#pragma once

/**
 * @file query.hpp
 * @brief Advanced ECS Query System for ECScope Educational Engine
 * 
 * This comprehensive query system provides sophisticated entity filtering and iteration
 * capabilities with educational focus on memory-efficient patterns and performance analysis.
 * It builds on the existing arena allocator and archetype system to demonstrate advanced
 * ECS query techniques.
 * 
 * Key Educational Features:
 * - Multi-archetype query execution with memory-efficient iteration
 * - Query result caching with smart invalidation patterns
 * - Complex filter combinations (ALL, ANY, NOT, WITH, WITHOUT)
 * - Compile-time query optimization and template-based builders
 * - Query performance profiling and visualization
 * - Memory access pattern analysis for educational insights
 * - Cache-friendly archetype traversal optimization
 * 
 * Advanced Query Types:
 * - Simple queries: Query<Transform, Velocity>
 * - Complex filters: Query<ALL<Transform, Velocity>, NOT<Disabled>>
 * - Optional components: Query<Transform, OPTIONAL<Velocity>>
 * - Tag queries: Query<WITH<Player>, WITHOUT<Enemy>>
 * - Relationship queries: Query<CHILDREN<Node>, PARENT<Node>>
 * 
 * Performance Features:
 * - Query result caching with automatic invalidation
 * - Chunk-based iteration for cache efficiency
 * - SIMD-friendly data layout access
 * - Parallel query execution support
 * - Query cost estimation and optimization
 * 
 * @author ECScope Educational ECS Framework
 * @date 2024
 */

#include "core/types.hpp"
#include "core/log.hpp"
#include "component.hpp"
#include "signature.hpp"
#include "archetype.hpp"
#include "memory/arena.hpp"
#include "memory/memory_tracker.hpp"
#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <memory>
#include <functional>
#include <type_traits>
#include <tuple>
#include <span>
#include <optional>
#include <chrono>
#include <concepts>

namespace ecscope::ecs {

// Forward declarations
class Registry;
class QueryCache;

/**
 * @brief Query filter types for complex query composition
 */
namespace filters {
    
    // ALL<Components...> - Entity must have ALL specified components
    template<Component... Components>
    struct ALL {
        static constexpr usize component_count = sizeof...(Components);
        using component_types = std::tuple<Components...>;
        
        static ComponentSignature to_signature() noexcept {
            return make_signature<Components...>();
        }
        
        static constexpr bool matches_all = true;
        static constexpr bool matches_any = false;
        static constexpr bool matches_not = false;
    };
    
    // ANY<Components...> - Entity must have AT LEAST ONE of specified components
    template<Component... Components>
    struct ANY {
        static constexpr usize component_count = sizeof...(Components);
        using component_types = std::tuple<Components...>;
        
        static constexpr bool matches_all = false;
        static constexpr bool matches_any = true;
        static constexpr bool matches_not = false;
    };
    
    // NOT<Components...> - Entity must NOT have any of specified components
    template<Component... Components>
    struct NOT {
        static constexpr usize component_count = sizeof...(Components);
        using component_types = std::tuple<Components...>;
        
        static ComponentSignature to_signature() noexcept {
            return make_signature<Components...>();
        }
        
        static constexpr bool matches_all = false;
        static constexpr bool matches_any = false;
        static constexpr bool matches_not = true;
    };
    
    // OPTIONAL<Component> - Component may or may not be present
    template<Component T>
    struct OPTIONAL {
        using component_type = T;
        static constexpr bool is_optional = true;
    };
    
    // WITH<Component> - Alias for component that must be present (same as including directly)
    template<Component T>
    struct WITH {
        using component_type = T;
        static constexpr bool is_required = true;
    };
    
    // WITHOUT<Component> - Component must not be present (alias for NOT<Component>)
    template<Component T>
    using WITHOUT = NOT<T>;
    
    // CHANGED<Component> - Component was modified since last query (requires change detection)
    template<Component T>
    struct CHANGED {
        using component_type = T;
        static constexpr bool is_change_filter = true;
    };
    
    // ADDED<Component> - Component was added since last query
    template<Component T>
    struct ADDED {
        using component_type = T;
        static constexpr bool is_addition_filter = true;
    };
    
    // REMOVED<Component> - Component was removed since last query
    template<Component T>
    struct REMOVED {
        using component_type = T;
        static constexpr bool is_removal_filter = true;
    };
}

/**
 * @brief Query filter concepts for template validation
 */
template<typename T>
concept QueryFilter = requires {
    T::component_count;
} || requires {
    typename T::component_type;
};

template<typename T>
concept SimpleComponent = Component<T> && !QueryFilter<T>;

template<typename T>
concept AllFilter = requires {
    T::matches_all;
    requires T::matches_all == true;
};

template<typename T>
concept AnyFilter = requires {
    T::matches_any;
    requires T::matches_any == true;
};

template<typename T>
concept NotFilter = requires {
    T::matches_not;
    requires T::matches_not == true;
};

template<typename T>
concept OptionalFilter = requires {
    T::is_optional;
    requires T::is_optional == true;
};

/**
 * @brief Query execution statistics for educational insights
 */
struct QueryStats {
    // Query identification
    std::string query_name;             // Human-readable query name
    u64 query_hash;                    // Hash of query signature for tracking
    
    // Performance metrics
    u64 total_executions;              // Total times this query was executed
    f64 total_execution_time;          // Total time spent executing
    f64 average_execution_time;        // Average execution time
    f64 min_execution_time;            // Fastest execution
    f64 max_execution_time;            // Slowest execution
    
    // Result metrics
    u64 total_entities_processed;      // Total entities processed across all executions
    u64 average_entities_per_query;    // Average entities per execution
    u64 max_entities_per_query;        // Maximum entities in single execution
    
    // Memory access patterns
    u64 cache_hits;                    // Cache hits from cached results
    u64 cache_misses;                  // Cache misses requiring full execution
    f64 cache_hit_ratio;              // Hit ratio for educational analysis
    
    // Archetype analysis
    usize archetypes_matched;          // Number of archetypes matched
    usize archetypes_total;            // Total archetypes examined
    f64 archetype_selectivity;         // Percentage of archetypes matched
    
    // Memory efficiency
    usize memory_accessed;             // Total memory accessed during queries
    usize memory_wasted;               // Memory accessed but not used
    f64 memory_efficiency;             // Efficiency ratio
    
    QueryStats() noexcept { reset(); }
    
    void reset() noexcept {
        query_name.clear();
        query_hash = 0;
        total_executions = 0;
        total_execution_time = 0.0;
        average_execution_time = 0.0;
        min_execution_time = std::numeric_limits<f64>::max();
        max_execution_time = 0.0;
        total_entities_processed = 0;
        average_entities_per_query = 0;
        max_entities_per_query = 0;
        cache_hits = 0;
        cache_misses = 0;
        cache_hit_ratio = 0.0;
        archetypes_matched = 0;
        archetypes_total = 0;
        archetype_selectivity = 0.0;
        memory_accessed = 0;
        memory_wasted = 0;
        memory_efficiency = 0.0;
    }
    
    void update_averages() noexcept {
        if (total_executions > 0) {
            average_execution_time = total_execution_time / total_executions;
            average_entities_per_query = total_entities_processed / total_executions;
            cache_hit_ratio = static_cast<f64>(cache_hits) / (cache_hits + cache_misses);
            archetype_selectivity = archetypes_total > 0 ? 
                static_cast<f64>(archetypes_matched) / archetypes_total : 0.0;
            memory_efficiency = memory_accessed > 0 ? 
                1.0 - (static_cast<f64>(memory_wasted) / memory_accessed) : 1.0;
        }
    }
};

/**
 * @brief Query result entry for cached results
 */
struct QueryResultEntry {
    Entity entity;                      // Entity ID
    void* component_data[8];           // Component data pointers (up to 8 components)
    u8 component_count;                // Number of components in this entry
    u32 archetype_version;             // Archetype version for invalidation
    
    QueryResultEntry() noexcept 
        : entity(Entity::invalid()), component_count(0), archetype_version(0) {
        std::fill(std::begin(component_data), std::end(component_data), nullptr);
    }
};

/**
 * @brief Cached query results with smart invalidation
 */
struct CachedQueryResult {
    std::vector<QueryResultEntry> entries;     // Cached result entries
    ComponentSignature query_signature;        // Original query signature
    std::unordered_set<usize> archetype_ids;  // Archetype IDs this query depends on
    f64 creation_time;                         // When cache was created
    f64 last_access_time;                      // When cache was last used
    u32 access_count;                          // Number of times accessed
    bool is_valid;                             // Whether cache is still valid
    
    CachedQueryResult() noexcept
        : creation_time(0.0), last_access_time(0.0), access_count(0), is_valid(false) {}
};

/**
 * @brief Query configuration for performance tuning
 */
struct QueryConfig {
    bool enable_caching;                // Enable query result caching
    bool enable_parallel_execution;     // Enable parallel query execution
    bool enable_prefetching;           // Enable memory prefetching
    bool enable_statistics;            // Collect query statistics
    
    // Caching parameters
    usize max_cached_results;          // Maximum number of cached query results
    f64 cache_timeout;                 // Cache timeout in seconds
    usize max_entries_per_cache;       // Maximum entries per cached result
    
    // Performance parameters
    usize chunk_size;                  // Chunk size for iteration
    usize parallel_threshold;          // Minimum entities for parallel execution
    usize prefetch_distance;           // Memory prefetch distance
    
    // Memory management
    usize arena_size;                  // Arena size for temporary allocations
    bool use_pool_allocator;           // Use pool allocator for query objects
    
    QueryConfig() noexcept
        : enable_caching(true)
        , enable_parallel_execution(false)
        , enable_prefetching(true)
        , enable_statistics(true)
        , max_cached_results(1000)
        , cache_timeout(5.0)
        , max_entries_per_cache(10000)
        , chunk_size(256)
        , parallel_threshold(1000)
        , prefetch_distance(64)
        , arena_size(1024 * 1024)  // 1MB
        , use_pool_allocator(true) {}
};

/**
 * @brief Advanced query cache system with smart invalidation
 */
class QueryCache {
private:
    // Cache storage
    std::unordered_map<u64, std::unique_ptr<CachedQueryResult>> cached_results_;
    mutable std::shared_mutex cache_mutex_;
    
    // Arena allocator for temporary query data
    std::unique_ptr<memory::ArenaAllocator> query_arena_;
    
    // Configuration
    QueryConfig config_;
    
    // Statistics
    std::atomic<u64> cache_hits_{0};
    std::atomic<u64> cache_misses_{0};
    std::atomic<u64> cache_invalidations_{0};
    
    // Invalidation tracking
    std::unordered_map<usize, std::vector<u64>> archetype_to_queries_;
    mutable std::mutex invalidation_mutex_;
    
public:
    explicit QueryCache(const QueryConfig& config);
    ~QueryCache();
    
    // Cache operations
    std::optional<std::span<const QueryResultEntry>> get_cached_result(u64 query_hash);
    void cache_result(u64 query_hash, const ComponentSignature& signature,
                     std::span<const QueryResultEntry> entries,
                     const std::unordered_set<usize>& archetype_ids);
    
    // Invalidation
    void invalidate_archetype(usize archetype_id);
    void invalidate_query(u64 query_hash);
    void invalidate_all();
    
    // Statistics
    f64 get_hit_ratio() const noexcept;
    u64 get_cached_query_count() const noexcept;
    usize get_memory_usage() const noexcept;
    
    // Configuration
    void set_config(const QueryConfig& config);
    const QueryConfig& get_config() const noexcept { return config_; }
    
    // Maintenance
    void cleanup_expired_caches();
    void compact_memory();
    
private:
    bool is_cache_valid(const CachedQueryResult& cache) const noexcept;
    void remove_expired_caches();
};

/**
 * @brief Query iterator for efficient archetype traversal
 * 
 * Provides cache-friendly iteration over query results with support for
 * chunked processing and memory prefetching.
 */
template<typename... ComponentTypes>
class QueryIterator {
private:
    const Registry* registry_;
    std::vector<Archetype*> matching_archetypes_;
    usize current_archetype_idx_;
    usize current_entity_idx_;
    usize chunk_size_;
    bool enable_prefetching_;
    
    // Current iteration state
    std::tuple<ComponentTypes*...> current_components_;
    Entity current_entity_;
    bool is_valid_;
    
public:
    explicit QueryIterator(const Registry& registry,
                          std::vector<Archetype*> archetypes,
                          usize chunk_size = 256,
                          bool enable_prefetching = true);
    
    // Iterator interface
    bool is_valid() const noexcept { return is_valid_; }
    void advance();
    
    // Access current data
    Entity entity() const noexcept { return current_entity_; }
    std::tuple<ComponentTypes*...> components() const noexcept { return current_components_; }
    
    // Convenience accessors for single components
    template<Component T>
    T* get() const noexcept {
        static_assert((std::is_same_v<T, ComponentTypes> || ...), "Component type not in query");
        return std::get<T*>(current_components_);
    }
    
    // Chunk processing
    struct Chunk {
        std::vector<Entity> entities;
        std::tuple<std::vector<ComponentTypes*>...> component_arrays;
        usize count;
    };
    
    std::optional<Chunk> next_chunk();
    
    // Statistics
    usize entities_processed() const noexcept;
    usize archetypes_processed() const noexcept;
    
private:
    void find_next_valid_entity();
    void prefetch_next_chunk();
    void update_current_components();
};

/**
 * @brief Main query interface with fluent API and compile-time optimization
 * 
 * Provides a comprehensive query system with support for complex filters,
 * caching, and performance analysis. Designed for educational demonstration
 * of advanced ECS query techniques.
 */
template<typename... Filters>
class Query {
private:
    const Registry* registry_;
    QueryCache* cache_;
    ComponentSignature required_signature_;
    ComponentSignature forbidden_signature_;
    std::vector<ComponentSignature> any_signatures_;
    std::string query_name_;
    u64 query_hash_;
    mutable QueryStats stats_;
    
    // Configuration
    QueryConfig config_;
    
    // Type analysis
    static constexpr usize filter_count = sizeof...(Filters);
    using filter_tuple = std::tuple<Filters...>;
    
    // Extract required components (simple components and ALL filters)
    template<typename Filter>
    static constexpr bool is_required_component() {
        return SimpleComponent<Filter> || AllFilter<Filter>;
    }
    
    // Extract optional components
    template<typename Filter>
    static constexpr bool is_optional_component() {
        return OptionalFilter<Filter>;
    }
    
    // Extract forbidden components (NOT filters)
    template<typename Filter>
    static constexpr bool is_forbidden_component() {
        return NotFilter<Filter>;
    }
    
    // Extract ANY components
    template<typename Filter>
    static constexpr bool is_any_component() {
        return AnyFilter<Filter>;
    }
    
public:
    explicit Query(const Registry& registry, QueryCache* cache = nullptr,
                  const std::string& name = "Unnamed Query");
    
    // Fluent API for query building
    Query& named(const std::string& name) { 
        query_name_ = name; 
        compute_hash();
        return *this; 
    }
    
    Query& with_config(const QueryConfig& config) { 
        config_ = config; 
        return *this; 
    }
    
    Query& enable_caching(bool enable = true) { 
        config_.enable_caching = enable; 
        return *this; 
    }
    
    Query& enable_parallel(bool enable = true) { 
        config_.enable_parallel_execution = enable; 
        return *this; 
    }
    
    Query& chunk_size(usize size) { 
        config_.chunk_size = size; 
        return *this; 
    }
    
    // Query execution
    std::vector<Entity> entities() const;
    usize count() const;
    bool empty() const;
    
    // Iteration interface
    auto iter() const {
        return create_iterator<Filters...>();
    }
    
    // Functional iteration
    template<typename Func>
    void for_each(Func&& func) const;
    
    template<typename Func>
    void for_each_chunk(Func&& func) const;
    
    template<typename Func>
    void for_each_parallel(Func&& func) const;
    
    // Single entity queries
    Entity first() const;
    Entity single() const;  // Throws if not exactly one match
    std::optional<Entity> try_single() const;
    
    // Statistics and analysis
    const QueryStats& statistics() const { return stats_; }
    void reset_statistics() { stats_.reset(); }
    
    // Query introspection
    ComponentSignature get_required_signature() const { return required_signature_; }
    ComponentSignature get_forbidden_signature() const { return forbidden_signature_; }
    const std::vector<ComponentSignature>& get_any_signatures() const { return any_signatures_; }
    std::string get_description() const;
    
    // Performance analysis
    f64 estimate_selectivity() const;
    usize estimate_result_count() const;
    f64 estimate_execution_time() const;
    
private:
    void compute_signatures();
    void compute_hash();
    
    std::vector<Archetype*> find_matching_archetypes() const;
    bool archetype_matches(const Archetype& archetype) const;
    
    // Iterator creation (compile-time type deduction)
    template<typename... FilterTypes>
    auto create_iterator() const {
        using ComponentTuple = typename extract_component_types<FilterTypes...>::type;
        return create_iterator_impl(ComponentTuple{});
    }
    
    template<typename... ComponentTypes>
    QueryIterator<ComponentTypes...> create_iterator_impl(std::tuple<ComponentTypes...>) const {
        auto archetypes = find_matching_archetypes();
        return QueryIterator<ComponentTypes...>(*registry_, std::move(archetypes),
                                               config_.chunk_size, config_.enable_prefetching);
    }
    
    // Type extraction helpers
    template<typename... FilterTypes>
    struct extract_component_types {
        using type = std::tuple<>; // Default empty tuple, specialized below
    };
    
    void record_execution(f64 execution_time, usize entities_processed,
                         usize archetypes_matched, bool cache_hit) const;
};

/**
 * @brief Query builder for dynamic query construction
 * 
 * Provides a runtime query builder when compile-time query construction
 * is not feasible. Less efficient than compile-time queries but more flexible.
 */
class DynamicQuery {
private:
    const Registry* registry_;
    QueryCache* cache_;
    
    ComponentSignature required_components_;
    ComponentSignature forbidden_components_;
    std::vector<ComponentSignature> any_component_groups_;
    std::vector<ComponentSignature> optional_components_;
    
    std::string query_name_;
    QueryConfig config_;
    mutable QueryStats stats_;
    
public:
    explicit DynamicQuery(const Registry& registry, QueryCache* cache = nullptr);
    
    // Dynamic query building
    DynamicQuery& require_component(core::ComponentID component_id);
    DynamicQuery& forbid_component(core::ComponentID component_id);
    DynamicQuery& any_components(const std::vector<core::ComponentID>& component_ids);
    DynamicQuery& optional_component(core::ComponentID component_id);
    
    template<Component T>
    DynamicQuery& require() {
        return require_component(component_id<T>());
    }
    
    template<Component T>
    DynamicQuery& forbid() {
        return forbid_component(component_id<T>());
    }
    
    template<Component... Components>
    DynamicQuery& any() {
        std::vector<core::ComponentID> ids = {component_id<Components>()...};
        return any_components(ids);
    }
    
    // Configuration
    DynamicQuery& named(const std::string& name);
    DynamicQuery& with_config(const QueryConfig& config);
    
    // Execution
    std::vector<Entity> entities() const;
    usize count() const;
    void for_each(std::function<void(Entity, void**)> func) const;
    
    // Statistics
    const QueryStats& statistics() const { return stats_; }
    std::string get_description() const;
};

// Common query aliases for convenience
using TransformQuery = Query<class Transform>;
using MovementQuery = Query<class Transform, class Velocity>;
using RenderableQuery = Query<class Transform, class Renderable>;
using PlayerQuery = Query<filters::WITH<class Player>>;
using EnemyQuery = Query<filters::WITH<class Enemy>, filters::WITHOUT<class Player>>;
using DamagedQuery = Query<class Health, filters::CHANGED<class Health>>;

// Query factory functions
template<Component... Components>
auto make_query(const Registry& registry, QueryCache* cache = nullptr) {
    return Query<Components...>(registry, cache);
}

template<Component... Required, Component... Forbidden>
auto make_query_with_without(const Registry& registry, QueryCache* cache = nullptr) {
    return Query<filters::ALL<Required...>, filters::NOT<Forbidden...>>(registry, cache);
}

template<Component... Any>
auto make_any_query(const Registry& registry, QueryCache* cache = nullptr) {
    return Query<filters::ANY<Any...>>(registry, cache);
}

/**
 * @brief Global query statistics and management
 */
class QueryManager {
private:
    std::unique_ptr<QueryCache> cache_;
    std::vector<QueryStats*> tracked_queries_;
    mutable std::shared_mutex queries_mutex_;
    
    // Global statistics
    std::atomic<u64> total_queries_executed_{0};
    std::atomic<f64> total_query_time_{0.0};
    
public:
    QueryManager();
    ~QueryManager();
    
    // Cache management
    QueryCache& cache() { return *cache_; }
    const QueryCache& cache() const { return *cache_; }
    
    // Query tracking
    void register_query(QueryStats* stats);
    void unregister_query(QueryStats* stats);
    
    // Global statistics
    std::vector<QueryStats> get_all_query_stats() const;
    QueryStats get_combined_stats() const;
    
    // Performance analysis
    std::vector<QueryStats> get_slowest_queries(usize count = 10) const;
    std::vector<QueryStats> get_most_frequent_queries(usize count = 10) const;
    f64 get_average_query_time() const;
    
    // Memory usage
    usize get_total_memory_usage() const;
    
    // Configuration
    void set_global_config(const QueryConfig& config);
    void optimize_caches();
    void reset_all_statistics();
};

// Global query manager instance
QueryManager& get_query_manager();

} // namespace ecscope::ecs

// Include implementation details
#include "query_impl.hpp"