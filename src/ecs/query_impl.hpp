#pragma once

/**
 * @file query_impl.hpp
 * @brief Implementation details for the Advanced ECS Query System
 * 
 * This file contains the template implementations and specialized code for
 * the query system. It demonstrates advanced template metaprogramming
 * techniques for compile-time query optimization.
 */

#include "query.hpp"
#include "registry.hpp"
#include <algorithm>
#include <execution>
#include <numeric>
#include <cmath>

namespace ecscope::ecs {

// Template specializations for component type extraction
template<SimpleComponent T>
struct extract_component_types<T> {
    using type = std::tuple<T>;
};

template<Component... Components>
struct extract_component_types<filters::ALL<Components...>> {
    using type = std::tuple<Components...>;
};

template<Component T>
struct extract_component_types<filters::OPTIONAL<T>> {
    using type = std::tuple<T>;
};

template<Component T>
struct extract_component_types<filters::WITH<T>> {
    using type = std::tuple<T>;
};

// Recursive template to combine multiple filter types
template<typename First, typename... Rest>
struct extract_component_types<First, Rest...> {
    using first_type = typename extract_component_types<First>::type;
    using rest_type = typename extract_component_types<Rest...>::type;
    using type = decltype(std::tuple_cat(first_type{}, rest_type{}));
};

// QueryIterator implementation
template<typename... ComponentTypes>
QueryIterator<ComponentTypes...>::QueryIterator(const Registry& registry,
                                               std::vector<Archetype*> archetypes,
                                               usize chunk_size,
                                               bool enable_prefetching)
    : registry_(&registry)
    , matching_archetypes_(std::move(archetypes))
    , current_archetype_idx_(0)
    , current_entity_idx_(0)
    , chunk_size_(chunk_size)
    , enable_prefetching_(enable_prefetching)
    , current_entity_(Entity::invalid())
    , is_valid_(false) {
    
    // Find first valid entity
    find_next_valid_entity();
}

template<typename... ComponentTypes>
void QueryIterator<ComponentTypes...>::advance() {
    if (!is_valid_) return;
    
    ++current_entity_idx_;
    find_next_valid_entity();
}

template<typename... ComponentTypes>
void QueryIterator<ComponentTypes...>::find_next_valid_entity() {
    is_valid_ = false;
    
    while (current_archetype_idx_ < matching_archetypes_.size()) {
        Archetype* archetype = matching_archetypes_[current_archetype_idx_];
        const auto& entities = archetype->entities();
        
        if (current_entity_idx_ < entities.size()) {
            // Found valid entity
            current_entity_ = entities[current_entity_idx_];
            update_current_components();
            is_valid_ = true;
            
            // Prefetch next chunk if enabled
            if (enable_prefetching_) {
                prefetch_next_chunk();
            }
            
            return;
        }
        
        // Move to next archetype
        ++current_archetype_idx_;
        current_entity_idx_ = 0;
    }
}

template<typename... ComponentTypes>
void QueryIterator<ComponentTypes...>::update_current_components() {
    if (current_archetype_idx_ >= matching_archetypes_.size()) return;
    
    Archetype* archetype = matching_archetypes_[current_archetype_idx_];
    
    // Get component pointers for current entity
    current_components_ = std::make_tuple(
        archetype->get_component<ComponentTypes>(current_entity_)...
    );
}

template<typename... ComponentTypes>
void QueryIterator<ComponentTypes...>::prefetch_next_chunk() {
    // Prefetch memory for upcoming entities to improve cache performance
    if (current_archetype_idx_ >= matching_archetypes_.size()) return;
    
    Archetype* archetype = matching_archetypes_[current_archetype_idx_];
    const auto& entities = archetype->entities();
    
    usize prefetch_end = std::min(current_entity_idx_ + chunk_size_, entities.size());
    
    // Prefetch component data for upcoming entities
    for (usize i = current_entity_idx_ + 1; i < prefetch_end; ++i) {
        Entity entity = entities[i];
        
        // Prefetch each component type
        ((void)archetype->get_component<ComponentTypes>(entity), ...);
    }
}

template<typename... ComponentTypes>
auto QueryIterator<ComponentTypes...>::next_chunk() -> std::optional<Chunk> {
    if (!is_valid_) return std::nullopt;
    
    Chunk chunk;
    chunk.entities.reserve(chunk_size_);
    
    // Initialize component arrays
    (std::get<std::vector<ComponentTypes*>>(chunk.component_arrays).reserve(chunk_size_), ...);
    
    usize entities_in_chunk = 0;
    
    while (is_valid_ && entities_in_chunk < chunk_size_) {
        // Add current entity to chunk
        chunk.entities.push_back(current_entity_);
        
        // Add component pointers to arrays
        (std::get<std::vector<ComponentTypes*>>(chunk.component_arrays)
            .push_back(std::get<ComponentTypes*>(current_components_)), ...);
        
        ++entities_in_chunk;
        advance();
    }
    
    chunk.count = entities_in_chunk;
    return chunk;
}

template<typename... ComponentTypes>
usize QueryIterator<ComponentTypes...>::entities_processed() const noexcept {
    usize total = 0;
    for (usize i = 0; i < current_archetype_idx_; ++i) {
        total += matching_archetypes_[i]->entity_count();
    }
    total += current_entity_idx_;
    return total;
}

template<typename... ComponentTypes>
usize QueryIterator<ComponentTypes...>::archetypes_processed() const noexcept {
    return current_archetype_idx_;
}

// Query implementation
template<typename... Filters>
Query<Filters...>::Query(const Registry& registry, QueryCache* cache, const std::string& name)
    : registry_(&registry)
    , cache_(cache)
    , query_name_(name)
    , query_hash_(0)
    , config_(QueryConfig{}) {
    
    compute_signatures();
    compute_hash();
    
    if (config_.enable_statistics) {
        stats_.query_name = query_name_;
        stats_.query_hash = query_hash_;
    }
}

template<typename... Filters>
void Query<Filters...>::compute_signatures() {
    // Process each filter type to build appropriate signatures
    (process_filter<Filters>(), ...);
}

template<typename... Filters>
template<typename Filter>
void Query<Filters...>::process_filter() {
    if constexpr (SimpleComponent<Filter>) {
        // Simple component requirement
        required_signature_.set<Filter>();
    }
    else if constexpr (AllFilter<Filter>) {
        // ALL filter - add all components to required
        auto signature = Filter::to_signature();
        required_signature_ |= signature;
    }
    else if constexpr (NotFilter<Filter>) {
        // NOT filter - add to forbidden
        auto signature = Filter::to_signature();
        forbidden_signature_ |= signature;
    }
    else if constexpr (AnyFilter<Filter>) {
        // ANY filter - add to any signatures list
        auto signature = Filter::to_signature();
        any_signatures_.push_back(signature);
    }
    // Optional filters don't affect signatures directly
}

template<typename... Filters>
void Query<Filters...>::compute_hash() {
    // Compute hash based on signature combination
    std::hash<ComponentSignature> hasher;
    query_hash_ = hasher(required_signature_);
    
    // Include forbidden signature in hash
    query_hash_ ^= hasher(forbidden_signature_) << 1;
    
    // Include any signatures
    for (const auto& signature : any_signatures_) {
        query_hash_ ^= hasher(signature) << 2;
    }
    
    // Include query name hash
    query_hash_ ^= std::hash<std::string>{}(query_name_) << 3;
}

template<typename... Filters>
std::vector<Entity> Query<Filters...>::entities() const {
    auto start_time = std::chrono::high_resolution_clock::now();
    
    std::vector<Entity> result;
    usize archetypes_matched = 0;
    usize total_archetypes = 0;
    bool cache_hit = false;
    
    // Try to use cached result if caching is enabled
    if (config_.enable_caching && cache_) {
        if (auto cached = cache_->get_cached_result(query_hash_)) {
            result.reserve(cached->size());
            for (const auto& entry : *cached) {
                result.push_back(entry.entity);
            }
            cache_hit = true;
            
            auto end_time = std::chrono::high_resolution_clock::now();
            f64 execution_time = std::chrono::duration<f64>(end_time - start_time).count();
            
            if (config_.enable_statistics) {
                record_execution(execution_time, result.size(), archetypes_matched, cache_hit);
            }
            
            return result;
        }
    }
    
    // Execute query
    auto matching_archetypes = find_matching_archetypes();
    archetypes_matched = matching_archetypes.size();
    
    // Estimate result size and reserve capacity
    usize estimated_size = 0;
    for (const auto* archetype : matching_archetypes) {
        estimated_size += archetype->entity_count();
    }
    result.reserve(estimated_size);
    
    // Collect entities from matching archetypes
    std::unordered_set<usize> archetype_ids;
    std::vector<QueryResultEntry> cache_entries;
    
    for (const auto* archetype : matching_archetypes) {
        const auto& entities = archetype->entities();
        
        for (Entity entity : entities) {
            result.push_back(entity);
            
            // Prepare cache entry if caching enabled
            if (config_.enable_caching && cache_) {
                QueryResultEntry entry;
                entry.entity = entity;
                entry.archetype_version = 0; // TODO: Track archetype versions
                cache_entries.push_back(entry);
                
                // Track archetype dependency
                // archetype_ids.insert(get_archetype_id(archetype));
            }
        }
    }
    
    // Cache the result
    if (config_.enable_caching && cache_ && !cache_entries.empty()) {
        cache_->cache_result(query_hash_, required_signature_, cache_entries, archetype_ids);
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    f64 execution_time = std::chrono::duration<f64>(end_time - start_time).count();
    
    if (config_.enable_statistics) {
        record_execution(execution_time, result.size(), archetypes_matched, cache_hit);
    }
    
    return result;
}

template<typename... Filters>
std::vector<Archetype*> Query<Filters...>::find_matching_archetypes() const {
    std::vector<Archetype*> matching;
    
    // Get all archetypes from registry
    auto archetype_stats = registry_->get_archetype_stats();
    
    for (const auto& [signature, entity_count] : archetype_stats) {
        if (archetype_matches_signature(signature)) {
            // TODO: Get actual archetype pointer from registry
            // This is a simplified version - real implementation would get archetype pointer
            // matching.push_back(get_archetype_by_signature(signature));
        }
    }
    
    return matching;
}

template<typename... Filters>
bool Query<Filters...>::archetype_matches_signature(const ComponentSignature& signature) const {
    // Check required components
    if (!signature.is_superset_of(required_signature_)) {
        return false;
    }
    
    // Check forbidden components
    if (signature.intersects(forbidden_signature_)) {
        return false;
    }
    
    // Check ANY requirements
    for (const auto& any_sig : any_signatures_) {
        if (!signature.intersects(any_sig)) {
            return false; // Must have at least one component from ANY group
        }
    }
    
    return true;
}

template<typename... Filters>
template<typename Func>
void Query<Filters...>::for_each(Func&& func) const {
    static_assert(std::is_invocable_v<Func, Entity, ComponentTypes*...>, 
                 "Function must accept Entity and component pointers");
    
    auto iterator = iter();
    while (iterator.is_valid()) {
        auto components = iterator.components();
        std::apply([&](auto*... comp_ptrs) {
            func(iterator.entity(), comp_ptrs...);
        }, components);
        iterator.advance();
    }
}

template<typename... Filters>
template<typename Func>
void Query<Filters...>::for_each_chunk(Func&& func) const {
    auto iterator = iter();
    
    while (iterator.is_valid()) {
        if (auto chunk = iterator.next_chunk()) {
            func(*chunk);
        }
    }
}

template<typename... Filters>
template<typename Func>
void Query<Filters...>::for_each_parallel(Func&& func) const {
    if (!config_.enable_parallel_execution) {
        for_each(std::forward<Func>(func));
        return;
    }
    
    auto entities_list = entities();
    if (entities_list.size() < config_.parallel_threshold) {
        for_each(std::forward<Func>(func));
        return;
    }
    
    // Parallel execution using C++17 parallel algorithms
    std::for_each(std::execution::par_unseq, entities_list.begin(), entities_list.end(),
                  [this, &func](Entity entity) {
                      // Get components for this entity
                      auto components = std::make_tuple(
                          registry_->get_component<ComponentTypes>(entity)...
                      );
                      
                      // Check if all required components exist
                      bool all_valid = (std::get<ComponentTypes*>(components) && ...);
                      if (all_valid) {
                          std::apply([&](auto*... comp_ptrs) {
                              func(entity, comp_ptrs...);
                          }, components);
                      }
                  });
}

template<typename... Filters>
Entity Query<Filters...>::first() const {
    auto iterator = iter();
    return iterator.is_valid() ? iterator.entity() : Entity::invalid();
}

template<typename... Filters>
Entity Query<Filters...>::single() const {
    auto result = try_single();
    if (!result) {
        LOG_ERROR("Query::single() called but no entities match the query");
        throw std::runtime_error("No entities match single() query");
    }
    return *result;
}

template<typename... Filters>
std::optional<Entity> Query<Filters...>::try_single() const {
    auto iterator = iter();
    if (!iterator.is_valid()) {
        return std::nullopt;
    }
    
    Entity result = iterator.entity();
    iterator.advance();
    
    if (iterator.is_valid()) {
        LOG_ERROR("Query::single() called but multiple entities match the query");
        return std::nullopt;
    }
    
    return result;
}

template<typename... Filters>
usize Query<Filters...>::count() const {
    if (config_.enable_caching && cache_) {
        if (auto cached = cache_->get_cached_result(query_hash_)) {
            return cached->size();
        }
    }
    
    usize total = 0;
    auto iterator = iter();
    while (iterator.is_valid()) {
        ++total;
        iterator.advance();
    }
    
    return total;
}

template<typename... Filters>
bool Query<Filters...>::empty() const {
    auto iterator = iter();
    return !iterator.is_valid();
}

template<typename... Filters>
std::string Query<Filters...>::get_description() const {
    std::ostringstream desc;
    desc << "Query '" << query_name_ << "' [";
    
    // Add required components
    if (!required_signature_.empty()) {
        desc << "REQUIRE: " << required_signature_.to_string();
    }
    
    // Add forbidden components
    if (!forbidden_signature_.empty()) {
        if (!required_signature_.empty()) desc << ", ";
        desc << "FORBID: " << forbidden_signature_.to_string();
    }
    
    // Add ANY groups
    if (!any_signatures_.empty()) {
        if (!required_signature_.empty() || !forbidden_signature_.empty()) desc << ", ";
        desc << "ANY: " << any_signatures_.size() << " groups";
    }
    
    desc << "]";
    return desc.str();
}

template<typename... Filters>
f64 Query<Filters...>::estimate_selectivity() const {
    // Simple selectivity estimation based on signature complexity
    f64 selectivity = 1.0;
    
    // More required components = lower selectivity
    selectivity *= std::pow(0.8, required_signature_.count());
    
    // Forbidden components increase selectivity slightly
    selectivity *= (1.0 + 0.1 * forbidden_signature_.count());
    
    // ANY requirements reduce selectivity
    selectivity *= std::pow(0.9, any_signatures_.size());
    
    return std::clamp(selectivity, 0.01, 1.0);
}

template<typename... Filters>
usize Query<Filters...>::estimate_result_count() const {
    auto total_entities = registry_->active_entities();
    return static_cast<usize>(total_entities * estimate_selectivity());
}

template<typename... Filters>
f64 Query<Filters...>::estimate_execution_time() const {
    if (stats_.total_executions > 0) {
        return stats_.average_execution_time;
    }
    
    // Base estimation on entity count and query complexity
    auto estimated_entities = estimate_result_count();
    f64 base_time = estimated_entities * 0.000001; // 1 microsecond per entity (rough estimate)
    
    // Adjust for query complexity
    f64 complexity_factor = 1.0 + (required_signature_.count() * 0.1) + (any_signatures_.size() * 0.2);
    
    return base_time * complexity_factor;
}

template<typename... Filters>
void Query<Filters...>::record_execution(f64 execution_time, usize entities_processed,
                                        usize archetypes_matched, bool cache_hit) const {
    if (!config_.enable_statistics) return;
    
    auto& stats = const_cast<QueryStats&>(stats_);
    
    stats.total_executions++;
    stats.total_execution_time += execution_time;
    stats.total_entities_processed += entities_processed;
    stats.archetypes_matched = archetypes_matched;
    
    if (cache_hit) {
        stats.cache_hits++;
    } else {
        stats.cache_misses++;
    }
    
    // Update min/max times
    if (execution_time < stats.min_execution_time) {
        stats.min_execution_time = execution_time;
    }
    if (execution_time > stats.max_execution_time) {
        stats.max_execution_time = execution_time;
    }
    
    if (entities_processed > stats.max_entities_per_query) {
        stats.max_entities_per_query = entities_processed;
    }
    
    // Update averages
    stats.update_averages();
}

} // namespace ecscope::ecs