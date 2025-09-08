#pragma once

#include "query_engine.hpp"
#include "query_operators.hpp"
#include "spatial_queries.hpp"
#include "../core/types.hpp"
#include "../core/log.hpp"

#include <type_traits>
#include <concepts>
#include <functional>
#include <string>
#include <vector>
#include <memory>
#include <sstream>

namespace ecscope::ecs::query {

// Forward declarations
template<typename... Components>
class QueryBuilder;

/**
 * @brief Type-safe query builder concept
 */
template<typename T>
concept QueryableComponent = Component<T>;

/**
 * @brief Fluent query builder for complex query composition
 * 
 * This builder provides a type-safe, chainable API for constructing complex
 * ECS queries with filters, sorting, spatial operations, and aggregations.
 * 
 * Example usage:
 * ```cpp
 * auto results = QueryBuilder<Transform, Velocity>{}
 *     .where([](const auto& transform, const auto& velocity) {
 *         return velocity.speed > 10.0f;
 *     })
 *     .within_radius({0, 0, 0}, 50.0f)
 *     .sort_by([](const auto& a, const auto& b) {
 *         return a.transform->position.y > b.transform->position.y;
 *     })
 *     .limit(100)
 *     .execute();
 * ```
 */
template<QueryableComponent... Components>
class QueryBuilder {
public:
    using EntityTuple = std::tuple<Entity, Components*...>;
    using ResultType = QueryResult<Components...>;
    using PredicateType = QueryPredicate<Components...>;
    
    // Predicate function types
    using ComponentPredicate = std::function<bool(Components*...)>;
    using EntityPredicate = std::function<bool(Entity, Components*...)>;
    using TuplePredicate = std::function<bool(const EntityTuple&)>;
    
    // Comparator function types
    using ComponentComparator = std::function<bool(const std::tuple<Components*...>&, 
                                                  const std::tuple<Components*...>&)>;
    using EntityComparator = std::function<bool(const EntityTuple&, const EntityTuple&)>;
    
private:
    QueryEngine* engine_;
    std::vector<PredicateType> predicates_;
    std::optional<EntityComparator> comparator_;
    std::optional<usize> limit_count_;
    std::optional<usize> offset_count_;
    std::string query_name_;
    bool enable_parallel_{true};
    bool enable_caching_{true};
    
    // Spatial query parameters
    std::optional<spatial::Region> spatial_region_;
    std::optional<std::pair<Vec3, f32>> radius_query_;
    std::optional<std::pair<Vec3, usize>> nearest_query_;
    
    // Aggregation parameters
    enum class AggregationType { None, Count, Sum, Average, Min, Max, Custom };
    AggregationType aggregation_type_{AggregationType::None};
    std::function<f64(const EntityTuple&)> aggregation_extractor_;
    std::function<f64(f64, f64)> custom_aggregator_;
    
public:
    /**
     * @brief Create query builder with engine reference
     */
    explicit QueryBuilder(QueryEngine* engine = nullptr) 
        : engine_(engine ? engine : &get_query_engine()) {
        
        // Set default query name based on component types
        std::ostringstream oss;
        oss << "Query<";
        ((oss << typeid(Components).name() << ","), ...);
        oss << ">";
        query_name_ = oss.str();
    }
    
    /**
     * @brief Add component-based predicate
     * 
     * @param predicate Function that takes component pointers and returns bool
     * @param description Human-readable description for debugging
     */
    QueryBuilder& where(ComponentPredicate predicate, 
                       const std::string& description = "") {
        auto tuple_predicate = [predicate = std::move(predicate)](const EntityTuple& tuple) -> bool {
            return std::apply([&predicate](Entity, Components*... components) {
                return predicate(components...);
            }, tuple);
        };
        
        predicates_.emplace_back(std::move(tuple_predicate), 
                               description.empty() ? "component_filter" : description);
        return *this;
    }
    
    /**
     * @brief Add entity-based predicate
     * 
     * @param predicate Function that takes entity and components, returns bool
     * @param description Human-readable description
     */
    QueryBuilder& where_entity(EntityPredicate predicate,
                              const std::string& description = "") {
        auto tuple_predicate = [predicate = std::move(predicate)](const EntityTuple& tuple) -> bool {
            return std::apply([&predicate](Entity entity, Components*... components) {
                return predicate(entity, components...);
            }, tuple);
        };
        
        predicates_.emplace_back(std::move(tuple_predicate),
                               description.empty() ? "entity_filter" : description);
        return *this;
    }
    
    /**
     * @brief Add tuple-based predicate for maximum flexibility
     */
    QueryBuilder& where_tuple(TuplePredicate predicate,
                             const std::string& description = "") {
        predicates_.emplace_back(std::move(predicate),
                               description.empty() ? "tuple_filter" : description);
        return *this;
    }
    
    /**
     * @brief Add spatial region filter (requires Transform/Position component)
     */
    template<typename T = void>
    QueryBuilder& within(const spatial::Region& region)
        requires (std::is_same_v<T, void> && 
                 (std::is_same_v<Components, Transform> || ... || 
                  std::is_same_v<Components, Position>))
    {
        spatial_region_ = region;
        
        auto spatial_predicate = [region](const EntityTuple& tuple) -> bool {
            return spatial::is_in_region(tuple, region);
        };
        
        PredicateType pred(spatial_predicate, "spatial:region");
        pred.mark_spatial();
        predicates_.emplace_back(std::move(pred));
        
        return *this;
    }
    
    /**
     * @brief Add radius-based spatial filter
     */
    template<typename T = void>
    QueryBuilder& within_radius(const Vec3& center, f32 radius)
        requires (std::is_same_v<T, void> && 
                 (std::is_same_v<Components, Transform> || ... || 
                  std::is_same_v<Components, Position>))
    {
        radius_query_ = {center, radius};
        
        auto radius_predicate = [center, radius_sq = radius * radius](const EntityTuple& tuple) -> bool {
            return spatial::distance_squared(tuple, center) <= radius_sq;
        };
        
        PredicateType pred(radius_predicate, "spatial:radius");
        pred.mark_spatial();
        predicates_.emplace_back(std::move(pred));
        
        return *this;
    }
    
    /**
     * @brief Component value-based filters with type safety
     */
    template<QueryableComponent T>
    QueryBuilder& where_component(std::function<bool(const T&)> predicate,
                                 const std::string& description = "")
        requires (std::is_same_v<T, Components> || ...)
    {
        auto component_predicate = [predicate = std::move(predicate)](const EntityTuple& tuple) -> bool {
            if constexpr ((std::is_same_v<T, Components> || ...)) {
                T* component = nullptr;
                ((component = std::is_same_v<T, Components> ? 
                  static_cast<T*>(std::get<Components*>(tuple)) : component), ...);
                
                return component && predicate(*component);
            }
            return false;
        };
        
        std::string desc = description.empty() ? 
            std::string("filter:") + typeid(T).name() : description;
        predicates_.emplace_back(component_predicate, desc);
        return *this;
    }
    
    /**
     * @brief Range-based filtering for numeric components
     */
    template<QueryableComponent T, typename ValueType>
    QueryBuilder& where_range(ValueType T::* member, ValueType min_val, ValueType max_val)
        requires (std::is_same_v<T, Components> || ...)
    {
        return where_component<T>([member, min_val, max_val](const T& component) {
            const ValueType& value = component.*member;
            return value >= min_val && value <= max_val;
        }, "range_filter");
    }
    
    /**
     * @brief Equality filter for component members
     */
    template<QueryableComponent T, typename ValueType>
    QueryBuilder& where_equals(ValueType T::* member, const ValueType& target_value)
        requires (std::is_same_v<T, Components> || ...)
    {
        return where_component<T>([member, target_value](const T& component) {
            return component.*member == target_value;
        }, "equality_filter");
    }
    
    /**
     * @brief Null/existence checks
     */
    template<QueryableComponent T>
    QueryBuilder& where_exists() 
        requires (std::is_same_v<T, Components> || ...)
    {
        return where([](Components*... components) {
            if constexpr ((std::is_same_v<T, Components> || ...)) {
                T* target = nullptr;
                ((target = std::is_same_v<T, Components> ? 
                  static_cast<T*>(components) : target), ...);
                return target != nullptr;
            }
            return false;
        }, "exists_filter");
    }
    
    /**
     * @brief Sorting with component-based comparator
     */
    QueryBuilder& sort_by(ComponentComparator comparator) {
        comparator_ = [comparator = std::move(comparator)](const EntityTuple& a, const EntityTuple& b) -> bool {
            auto a_components = std::make_tuple(std::get<Components*>(a)...);
            auto b_components = std::make_tuple(std::get<Components*>(b)...);
            return comparator(a_components, b_components);
        };
        return *this;
    }
    
    /**
     * @brief Sorting with entity-based comparator
     */
    QueryBuilder& sort_by_entity(EntityComparator comparator) {
        comparator_ = std::move(comparator);
        return *this;
    }
    
    /**
     * @brief Sort by component member value
     */
    template<QueryableComponent T, typename ValueType>
    QueryBuilder& sort_by_member(ValueType T::* member, bool ascending = true)
        requires (std::is_same_v<T, Components> || ...)
    {
        return sort_by([member, ascending](const auto& a_comps, const auto& b_comps) -> bool {
            if constexpr ((std::is_same_v<T, Components> || ...)) {
                T* a_comp = nullptr;
                T* b_comp = nullptr;
                
                ((a_comp = std::is_same_v<T, Components> ? 
                  std::get<Components*>(a_comps) : a_comp), ...);
                ((b_comp = std::is_same_v<T, Components> ? 
                  std::get<Components*>(b_comps) : b_comp), ...);
                
                if (a_comp && b_comp) {
                    const ValueType& a_val = a_comp->*member;
                    const ValueType& b_val = b_comp->*member;
                    return ascending ? (a_val < b_val) : (a_val > b_val);
                }
            }
            return false;
        });
    }
    
    /**
     * @brief Limit result count
     */
    QueryBuilder& limit(usize count) {
        limit_count_ = count;
        return *this;
    }
    
    /**
     * @brief Skip first N results
     */
    QueryBuilder& offset(usize count) {
        offset_count_ = count;
        return *this;
    }
    
    /**
     * @brief Set query name for debugging/profiling
     */
    QueryBuilder& named(const std::string& name) {
        query_name_ = name;
        return *this;
    }
    
    /**
     * @brief Enable/disable parallel execution for this query
     */
    QueryBuilder& parallel(bool enable = true) {
        enable_parallel_ = enable;
        return *this;
    }
    
    /**
     * @brief Enable/disable caching for this query
     */
    QueryBuilder& cached(bool enable = true) {
        enable_caching_ = enable;
        return *this;
    }
    
    /**
     * @brief Find nearest entities to a point
     */
    template<typename T = void>
    QueryBuilder& nearest_to(const Vec3& point, usize count = 10)
        requires (std::is_same_v<T, void> && 
                 (std::is_same_v<Components, Transform> || ... || 
                  std::is_same_v<Components, Position>))
    {
        nearest_query_ = {point, count};
        
        // Sort by distance
        return sort_by_entity([point](const EntityTuple& a, const EntityTuple& b) {
            return spatial::distance_squared(a, point) < spatial::distance_squared(b, point);
        });
    }
    
    /**
     * @brief Count aggregation
     */
    QueryBuilder& count() {
        aggregation_type_ = AggregationType::Count;
        return *this;
    }
    
    /**
     * @brief Sum aggregation with value extractor
     */
    template<typename ExtractorFunc>
    QueryBuilder& sum(ExtractorFunc&& extractor) {
        aggregation_type_ = AggregationType::Sum;
        aggregation_extractor_ = std::forward<ExtractorFunc>(extractor);
        return *this;
    }
    
    /**
     * @brief Average aggregation
     */
    template<typename ExtractorFunc>
    QueryBuilder& average(ExtractorFunc&& extractor) {
        aggregation_type_ = AggregationType::Average;
        aggregation_extractor_ = std::forward<ExtractorFunc>(extractor);
        return *this;
    }
    
    /**
     * @brief Min/Max aggregations
     */
    template<typename ExtractorFunc>
    QueryBuilder& min_by(ExtractorFunc&& extractor) {
        aggregation_type_ = AggregationType::Min;
        aggregation_extractor_ = std::forward<ExtractorFunc>(extractor);
        return *this;
    }
    
    template<typename ExtractorFunc>
    QueryBuilder& max_by(ExtractorFunc&& extractor) {
        aggregation_type_ = AggregationType::Max;
        aggregation_extractor_ = std::forward<ExtractorFunc>(extractor);
        return *this;
    }
    
    /**
     * @brief Custom aggregation
     */
    template<typename ExtractorFunc, typename AggregatorFunc>
    QueryBuilder& aggregate(ExtractorFunc&& extractor, AggregatorFunc&& aggregator) {
        aggregation_type_ = AggregationType::Custom;
        aggregation_extractor_ = std::forward<ExtractorFunc>(extractor);
        custom_aggregator_ = std::forward<AggregatorFunc>(aggregator);
        return *this;
    }
    
    /**
     * @brief Execute query and return results
     */
    ResultType execute() {
        // Combine all predicates into a single predicate
        PredicateType combined_predicate = create_combined_predicate();
        
        // Update engine configuration for this query
        auto temp_config = engine_->get_config();
        temp_config.enable_parallel_execution = enable_parallel_;
        temp_config.enable_caching = enable_caching_;
        
        // Execute query
        ResultType result = engine_->query_with_predicate<Components...>(combined_predicate);
        
        // Apply post-processing
        result = apply_post_processing(std::move(result));
        
        return result;
    }
    
    /**
     * @brief Execute query asynchronously
     */
    std::future<ResultType> execute_async() {
        return std::async(std::launch::async, [this]() {
            return execute();
        });
    }
    
    /**
     * @brief Stream results to consumer (for large datasets)
     */
    template<typename Consumer>
    void stream_to(Consumer&& consumer) {
        PredicateType combined_predicate = create_combined_predicate();
        
        engine_->stream_query<Components...>(combined_predicate, [&](const EntityTuple& tuple) {
            consumer(tuple);
        });
    }
    
    /**
     * @brief Execute query and return only the count
     */
    usize count_only() {
        auto old_limit = limit_count_;
        
        // Optimize for counting - don't fetch unnecessary data
        limit_count_ = std::nullopt; // Remove limit for accurate count
        
        auto result = execute();
        usize count = result.count();
        
        // Restore original limit
        limit_count_ = old_limit;
        
        return count;
    }
    
    /**
     * @brief Check if any entities match the query
     */
    bool any() {
        auto old_limit = limit_count_;
        limit_count_ = 1; // Only need one result to check existence
        
        auto result = execute();
        bool has_any = !result.empty();
        
        limit_count_ = old_limit;
        return has_any;
    }
    
    /**
     * @brief Get the first matching entity (if any)
     */
    std::optional<EntityTuple> first() {
        auto old_limit = limit_count_;
        limit_count_ = 1;
        
        auto result = execute();
        
        limit_count_ = old_limit;
        
        if (result.empty()) {
            return std::nullopt;
        }
        
        return result[0];
    }
    
    /**
     * @brief Execute query and apply aggregation
     */
    template<typename ResultType = f64>
    std::optional<ResultType> execute_aggregation() {
        if (aggregation_type_ == AggregationType::None) {
            return std::nullopt;
        }
        
        auto result = execute();
        if (result.empty()) {
            return std::nullopt;
        }
        
        switch (aggregation_type_) {
            case AggregationType::Count:
                return static_cast<ResultType>(result.count());
                
            case AggregationType::Sum: {
                f64 sum = 0.0;
                for (const auto& tuple : result) {
                    sum += aggregation_extractor_(tuple);
                }
                return static_cast<ResultType>(sum);
            }
            
            case AggregationType::Average: {
                f64 sum = 0.0;
                for (const auto& tuple : result) {
                    sum += aggregation_extractor_(tuple);
                }
                return static_cast<ResultType>(sum / result.count());
            }
            
            case AggregationType::Min: {
                f64 min_val = aggregation_extractor_(result[0]);
                for (usize i = 1; i < result.count(); ++i) {
                    min_val = std::min(min_val, aggregation_extractor_(result[i]));
                }
                return static_cast<ResultType>(min_val);
            }
            
            case AggregationType::Max: {
                f64 max_val = aggregation_extractor_(result[0]);
                for (usize i = 1; i < result.count(); ++i) {
                    max_val = std::max(max_val, aggregation_extractor_(result[i]));
                }
                return static_cast<ResultType>(max_val);
            }
            
            case AggregationType::Custom: {
                f64 aggregate_val = aggregation_extractor_(result[0]);
                for (usize i = 1; i < result.count(); ++i) {
                    aggregate_val = custom_aggregator_(aggregate_val, 
                                                     aggregation_extractor_(result[i]));
                }
                return static_cast<ResultType>(aggregate_val);
            }
        }
        
        return std::nullopt;
    }
    
    /**
     * @brief Get query description for debugging
     */
    std::string describe() const {
        std::ostringstream oss;
        oss << "QueryBuilder<";
        ((oss << typeid(Components).name() << ","), ...);
        oss << "> '" << query_name_ << "':\n";
        
        oss << "  Predicates: " << predicates_.size() << "\n";
        for (usize i = 0; i < predicates_.size(); ++i) {
            oss << "    " << i << ": " << predicates_[i].description() << "\n";
        }
        
        if (comparator_) {
            oss << "  Sorting: enabled\n";
        }
        
        if (limit_count_) {
            oss << "  Limit: " << *limit_count_ << "\n";
        }
        
        if (offset_count_) {
            oss << "  Offset: " << *offset_count_ << "\n";
        }
        
        oss << "  Parallel: " << (enable_parallel_ ? "enabled" : "disabled") << "\n";
        oss << "  Caching: " << (enable_caching_ ? "enabled" : "disabled") << "\n";
        
        return oss.str();
    }
    
private:
    /**
     * @brief Combine all predicates into a single predicate
     */
    PredicateType create_combined_predicate() {
        if (predicates_.empty()) {
            return PredicateType([](const EntityTuple&) { return true; }, "match_all");
        }
        
        if (predicates_.size() == 1) {
            return predicates_[0];
        }
        
        // Combine predicates with AND logic
        auto combined_func = [predicates = predicates_](const EntityTuple& tuple) -> bool {
            return std::all_of(predicates.begin(), predicates.end(),
                [&tuple](const auto& predicate) {
                    return predicate(tuple);
                });
        };
        
        std::ostringstream desc;
        desc << "combined(";
        for (usize i = 0; i < predicates_.size(); ++i) {
            if (i > 0) desc << " AND ";
            desc << predicates_[i].description();
        }
        desc << ")";
        
        return PredicateType(combined_func, desc.str());
    }
    
    /**
     * @brief Apply post-processing operations (sorting, limiting, etc.)
     */
    ResultType apply_post_processing(ResultType result) {
        // Apply sorting
        if (comparator_) {
            result = result.sort(*comparator_);
        }
        
        // Apply offset
        if (offset_count_ && *offset_count_ < result.size()) {
            auto& data = result.data();
            data.erase(data.begin(), data.begin() + *offset_count_);
        }
        
        // Apply limit
        if (limit_count_ && *limit_count_ < result.size()) {
            result.data().resize(*limit_count_);
        }
        
        return result;
    }
};

/**
 * @brief Convenience function to create a query builder
 */
template<QueryableComponent... Components>
QueryBuilder<Components...> query() {
    return QueryBuilder<Components...>{};
}

/**
 * @brief Convenience function with explicit engine
 */
template<QueryableComponent... Components>
QueryBuilder<Components...> query(QueryEngine& engine) {
    return QueryBuilder<Components...>{&engine};
}

} // namespace ecscope::ecs::query