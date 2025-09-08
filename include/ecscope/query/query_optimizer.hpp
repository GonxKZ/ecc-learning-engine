#pragma once

#include "../registry.hpp"
#include "../core/types.hpp"
#include "../core/log.hpp"
#include "query_engine.hpp"

#include <vector>
#include <unordered_map>
#include <memory>
#include <chrono>
#include <algorithm>
#include <numeric>
#include <type_traits>
#include <functional>
#include <string>
#include <sstream>

namespace ecscope::ecs::query {

// Forward declarations
template<typename... Components>
class QueryPredicate;

/**
 * @brief Query execution plan with optimization steps
 */
class QueryPlan {
public:
    enum class ExecutionStrategy {
        Sequential,           // Process archetypes sequentially
        Parallel,            // Process archetypes in parallel
        Indexed,             // Use component indices for fast lookup
        Spatial,             // Use spatial data structures
        Hybrid               // Combination of strategies
    };
    
    enum class FilterStrategy {
        EarlyFilter,         // Apply filters as early as possible
        LateFilter,          // Apply filters after all data is collected
        AdaptiveFilter       // Choose strategy based on selectivity
    };
    
    struct OptimizationStep {
        std::string name;
        std::string description;
        f64 estimated_cost_reduction;
        bool enabled;
        
        OptimizationStep(const std::string& n, const std::string& desc, f64 cost_reduction = 0.0)
            : name(n), description(desc), estimated_cost_reduction(cost_reduction), enabled(true) {}
    };
    
private:
    ExecutionStrategy execution_strategy_;
    FilterStrategy filter_strategy_;
    std::vector<OptimizationStep> optimization_steps_;
    
    // Cost estimates
    f64 estimated_entities_to_process_;
    f64 estimated_selectivity_;
    f64 estimated_execution_time_us_;
    
    // Component access pattern analysis
    std::unordered_map<std::string, f64> component_selectivity_;
    std::vector<std::string> optimal_filter_order_;
    
    // Spatial optimization parameters
    bool uses_spatial_queries_;
    std::string spatial_index_type_;
    
public:
    QueryPlan() 
        : execution_strategy_(ExecutionStrategy::Sequential)
        , filter_strategy_(FilterStrategy::EarlyFilter)
        , estimated_entities_to_process_(0.0)
        , estimated_selectivity_(1.0)
        , estimated_execution_time_us_(0.0)
        , uses_spatial_queries_(false) {}
    
    // Getters
    ExecutionStrategy execution_strategy() const { return execution_strategy_; }
    FilterStrategy filter_strategy() const { return filter_strategy_; }
    const std::vector<OptimizationStep>& optimization_steps() const { return optimization_steps_; }
    f64 estimated_entities_to_process() const { return estimated_entities_to_process_; }
    f64 estimated_selectivity() const { return estimated_selectivity_; }
    f64 estimated_execution_time_us() const { return estimated_execution_time_us_; }
    bool uses_spatial_queries() const { return uses_spatial_queries_; }
    const std::string& spatial_index_type() const { return spatial_index_type_; }
    
    // Setters
    void set_execution_strategy(ExecutionStrategy strategy) { execution_strategy_ = strategy; }
    void set_filter_strategy(FilterStrategy strategy) { filter_strategy_ = strategy; }
    void set_estimated_entities(f64 count) { estimated_entities_to_process_ = count; }
    void set_estimated_selectivity(f64 selectivity) { estimated_selectivity_ = selectivity; }
    void set_estimated_time(f64 time_us) { estimated_execution_time_us_ = time_us; }
    void set_spatial_optimization(bool enabled, const std::string& index_type = "rtree") {
        uses_spatial_queries_ = enabled;
        spatial_index_type_ = index_type;
    }
    
    void add_optimization_step(const OptimizationStep& step) {
        optimization_steps_.push_back(step);
    }
    
    void set_component_selectivity(const std::string& component, f64 selectivity) {
        component_selectivity_[component] = selectivity;
    }
    
    void set_optimal_filter_order(const std::vector<std::string>& order) {
        optimal_filter_order_ = order;
    }
    
    const std::vector<std::string>& optimal_filter_order() const {
        return optimal_filter_order_;
    }
    
    /**
     * @brief Generate human-readable plan description
     */
    std::string describe() const {
        std::ostringstream oss;
        oss << "=== Query Execution Plan ===\n";
        
        oss << "Execution Strategy: ";
        switch (execution_strategy_) {
            case ExecutionStrategy::Sequential: oss << "Sequential"; break;
            case ExecutionStrategy::Parallel: oss << "Parallel"; break;
            case ExecutionStrategy::Indexed: oss << "Indexed"; break;
            case ExecutionStrategy::Spatial: oss << "Spatial"; break;
            case ExecutionStrategy::Hybrid: oss << "Hybrid"; break;
        }
        oss << "\n";
        
        oss << "Filter Strategy: ";
        switch (filter_strategy_) {
            case FilterStrategy::EarlyFilter: oss << "Early Filtering"; break;
            case FilterStrategy::LateFilter: oss << "Late Filtering"; break;
            case FilterStrategy::AdaptiveFilter: oss << "Adaptive Filtering"; break;
        }
        oss << "\n";
        
        oss << "Estimated Entities: " << static_cast<usize>(estimated_entities_to_process_) << "\n";
        oss << "Estimated Selectivity: " << (estimated_selectivity_ * 100.0) << "%\n";
        oss << "Estimated Time: " << estimated_execution_time_us_ << " µs\n";
        
        if (uses_spatial_queries_) {
            oss << "Spatial Index: " << spatial_index_type_ << "\n";
        }
        
        if (!optimal_filter_order_.empty()) {
            oss << "Filter Order: ";
            for (usize i = 0; i < optimal_filter_order_.size(); ++i) {
                if (i > 0) oss << " -> ";
                oss << optimal_filter_order_[i];
            }
            oss << "\n";
        }
        
        oss << "\nOptimizations Applied:\n";
        for (const auto& step : optimization_steps_) {
            oss << "  " << (step.enabled ? "✓" : "✗") << " " << step.name;
            if (step.estimated_cost_reduction > 0.0) {
                oss << " (-" << (step.estimated_cost_reduction * 100.0) << "% cost)";
            }
            oss << "\n    " << step.description << "\n";
        }
        
        return oss.str();
    }
};

/**
 * @brief Advanced query optimizer with cost-based optimization
 */
class QueryOptimizer {
private:
    // Performance history for learning
    struct QueryPerformanceHistory {
        std::string query_signature;
        std::vector<f64> execution_times_us;
        std::vector<usize> entity_counts;
        f64 average_time_per_entity;
        usize sample_count;
        
        void add_sample(f64 time_us, usize entity_count) {
            execution_times_us.push_back(time_us);
            entity_counts.push_back(entity_count);
            
            if (execution_times_us.size() > 100) {
                execution_times_us.erase(execution_times_us.begin());
                entity_counts.erase(entity_counts.begin());
            }
            
            update_averages();
        }
        
        void update_averages() {
            if (execution_times_us.empty()) return;
            
            f64 total_time = std::accumulate(execution_times_us.begin(), execution_times_us.end(), 0.0);
            usize total_entities = std::accumulate(entity_counts.begin(), entity_counts.end(), 0u);
            
            sample_count = execution_times_us.size();
            average_time_per_entity = total_entities > 0 ? total_time / total_entities : 0.0;
        }
    };
    
    std::unordered_map<std::string, QueryPerformanceHistory> performance_history_;
    std::unordered_map<std::string, f64> component_selectivity_cache_;
    
    // Optimization thresholds
    static constexpr f64 PARALLEL_THRESHOLD_ENTITIES = 1000.0;
    static constexpr f64 SPATIAL_THRESHOLD_ENTITIES = 500.0;
    static constexpr f64 HIGH_SELECTIVITY_THRESHOLD = 0.1; // 10% or less
    static constexpr f64 LOW_SELECTIVITY_THRESHOLD = 0.8;  // 80% or more
    
public:
    QueryOptimizer() {
        LOG_INFO("QueryOptimizer initialized with cost-based optimization");
    }
    
    /**
     * @brief Create optimized execution plan for query
     */
    template<typename... Components>
    QueryPlan create_plan(const Registry& registry, 
                         const QueryPredicate<Components...>& predicate) {
        QueryPlan plan;
        
        // Analyze query characteristics
        std::string query_signature = generate_query_signature<Components...>(predicate);
        f64 estimated_entities = estimate_matching_entities<Components...>(registry);
        f64 estimated_selectivity = estimate_selectivity<Components...>(registry, predicate);
        
        plan.set_estimated_entities(estimated_entities);
        plan.set_estimated_selectivity(estimated_selectivity);
        
        // Choose execution strategy
        QueryPlan::ExecutionStrategy strategy = choose_execution_strategy(
            estimated_entities, estimated_selectivity, predicate.is_spatial());
        plan.set_execution_strategy(strategy);
        
        // Choose filter strategy
        QueryPlan::FilterStrategy filter_strategy = choose_filter_strategy(estimated_selectivity);
        plan.set_filter_strategy(filter_strategy);
        
        // Apply optimizations
        apply_component_order_optimization<Components...>(plan, registry);
        apply_parallel_optimization(plan, estimated_entities);
        apply_spatial_optimization(plan, predicate.is_spatial());
        apply_early_termination_optimization(plan, estimated_selectivity);
        apply_memory_layout_optimization<Components...>(plan);
        
        // Estimate final execution time
        f64 estimated_time = estimate_execution_time(plan, query_signature);
        plan.set_estimated_time(estimated_time);
        
        LOG_DEBUG("Created optimized plan for query: {} entities, {:.2f}% selectivity", 
                 static_cast<usize>(estimated_entities), estimated_selectivity * 100.0);
        
        return plan;
    }
    
    /**
     * @brief Record query execution performance for learning
     */
    void record_performance(const std::string& query_signature,
                          f64 execution_time_us,
                          usize actual_entity_count) {
        auto& history = performance_history_[query_signature];
        history.query_signature = query_signature;
        history.add_sample(execution_time_us, actual_entity_count);
        
        LOG_DEBUG("Recorded performance: query={}, time={:.2f}µs, entities={}", 
                 query_signature, execution_time_us, actual_entity_count);
    }
    
    /**
     * @brief Get optimization statistics
     */
    struct OptimizationStats {
        usize queries_optimized;
        usize queries_with_history;
        f64 average_speedup_factor;
        std::unordered_map<std::string, f64> component_selectivities;
        std::vector<std::pair<std::string, f64>> top_slow_queries;
    };
    
    OptimizationStats get_statistics() const {
        OptimizationStats stats;
        stats.queries_optimized = performance_history_.size();
        stats.queries_with_history = 0;
        stats.component_selectivities = component_selectivity_cache_;
        
        f64 total_speedup = 0.0;
        usize speedup_samples = 0;
        
        for (const auto& [signature, history] : performance_history_) {
            if (history.sample_count > 5) {
                stats.queries_with_history++;
                
                // Calculate speedup based on time per entity improvement
                if (history.execution_times_us.size() >= 2) {
                    f64 early_avg = std::accumulate(history.execution_times_us.begin(), 
                                                  history.execution_times_us.begin() + 
                                                  std::min(5ul, history.execution_times_us.size()), 0.0) / 
                                  std::min(5ul, history.execution_times_us.size());
                    
                    f64 recent_avg = std::accumulate(history.execution_times_us.end() - 
                                                   std::min(5ul, history.execution_times_us.size()), 
                                                   history.execution_times_us.end(), 0.0) / 
                                   std::min(5ul, history.execution_times_us.size());
                    
                    if (recent_avg > 0) {
                        total_speedup += early_avg / recent_avg;
                        speedup_samples++;
                    }
                }
            }
            
            // Add to slow queries if applicable
            if (history.average_time_per_entity > 10.0) { // More than 10µs per entity
                stats.top_slow_queries.emplace_back(signature, history.average_time_per_entity);
            }
        }
        
        stats.average_speedup_factor = speedup_samples > 0 ? total_speedup / speedup_samples : 1.0;
        
        // Sort slow queries by time
        std::sort(stats.top_slow_queries.begin(), stats.top_slow_queries.end(),
                 [](const auto& a, const auto& b) { return a.second > b.second; });
        
        if (stats.top_slow_queries.size() > 10) {
            stats.top_slow_queries.resize(10);
        }
        
        return stats;
    }
    
    /**
     * @brief Clear optimization history
     */
    void clear_history() {
        performance_history_.clear();
        component_selectivity_cache_.clear();
        LOG_INFO("QueryOptimizer history cleared");
    }
    
private:
    /**
     * @brief Generate unique signature for query type
     */
    template<typename... Components>
    std::string generate_query_signature(const QueryPredicate<Components...>& predicate) const {
        std::ostringstream oss;
        oss << "query<";
        ((oss << typeid(Components).name() << ","), ...);
        oss << ">";
        
        if (!predicate.description().empty()) {
            oss << "_" << predicate.description();
        }
        
        if (predicate.is_spatial()) {
            oss << "_spatial";
        }
        
        return oss.str();
    }
    
    /**
     * @brief Estimate number of entities that will match the component requirements
     */
    template<typename... Components>
    f64 estimate_matching_entities(const Registry& registry) const {
        // This is a simplified estimation
        // In a full implementation, you'd analyze archetype statistics
        
        ComponentSignature required = make_signature<Components...>();
        
        // Estimate based on total entities and component commonality
        usize total_entities = registry.active_entities();
        f64 estimated_coverage = 1.0;
        
        // Each component reduces coverage (simplified model)
        constexpr usize component_count = sizeof...(Components);
        if (component_count > 0) {
            // Assume each component covers 70% of entities on average
            estimated_coverage = std::pow(0.7, static_cast<f64>(component_count));
        }
        
        return static_cast<f64>(total_entities) * estimated_coverage;
    }
    
    /**
     * @brief Estimate selectivity of predicate filters
     */
    template<typename... Components>
    f64 estimate_selectivity(const Registry& registry, 
                            const QueryPredicate<Components...>& predicate) const {
        // Look up cached selectivity if available
        std::string pred_desc = predicate.description();
        auto cache_it = component_selectivity_cache_.find(pred_desc);
        if (cache_it != component_selectivity_cache_.end()) {
            return cache_it->second;
        }
        
        // Estimate based on predicate type
        f64 estimated_selectivity = 1.0;
        
        if (pred_desc.find("range") != std::string::npos) {
            estimated_selectivity = 0.3; // Range queries typically select 30%
        } else if (pred_desc.find("equality") != std::string::npos) {
            estimated_selectivity = 0.1; // Equality queries typically select 10%
        } else if (pred_desc.find("spatial") != std::string::npos) {
            estimated_selectivity = 0.2; // Spatial queries typically select 20%
        } else if (pred_desc == "match_all") {
            estimated_selectivity = 1.0;
        } else {
            estimated_selectivity = 0.5; // Default assumption
        }
        
        return estimated_selectivity;
    }
    
    /**
     * @brief Choose optimal execution strategy
     */
    QueryPlan::ExecutionStrategy choose_execution_strategy(f64 estimated_entities, 
                                                          f64 estimated_selectivity,
                                                          bool is_spatial) const {
        if (is_spatial && estimated_entities > SPATIAL_THRESHOLD_ENTITIES) {
            return QueryPlan::ExecutionStrategy::Spatial;
        }
        
        if (estimated_entities > PARALLEL_THRESHOLD_ENTITIES) {
            return QueryPlan::ExecutionStrategy::Parallel;
        }
        
        if (estimated_selectivity < HIGH_SELECTIVITY_THRESHOLD) {
            return QueryPlan::ExecutionStrategy::Indexed;
        }
        
        if (estimated_entities > 100 && is_spatial) {
            return QueryPlan::ExecutionStrategy::Hybrid;
        }
        
        return QueryPlan::ExecutionStrategy::Sequential;
    }
    
    /**
     * @brief Choose optimal filter strategy
     */
    QueryPlan::FilterStrategy choose_filter_strategy(f64 estimated_selectivity) const {
        if (estimated_selectivity < HIGH_SELECTIVITY_THRESHOLD) {
            return QueryPlan::FilterStrategy::EarlyFilter;
        }
        
        if (estimated_selectivity > LOW_SELECTIVITY_THRESHOLD) {
            return QueryPlan::FilterStrategy::LateFilter;
        }
        
        return QueryPlan::FilterStrategy::AdaptiveFilter;
    }
    
    /**
     * @brief Optimize component access order for cache efficiency
     */
    template<typename... Components>
    void apply_component_order_optimization(QueryPlan& plan, const Registry& registry) {
        std::vector<std::string> component_names = {typeid(Components).name()...};
        
        // Sort by estimated access frequency (most common first)
        // This is a simplified implementation
        std::sort(component_names.begin(), component_names.end());
        
        plan.set_optimal_filter_order(component_names);
        
        plan.add_optimization_step(QueryPlan::OptimizationStep{
            "component_order",
            "Reordered component access for better cache locality",
            0.15 // 15% estimated improvement
        });
    }
    
    /**
     * @brief Apply parallel execution optimization
     */
    void apply_parallel_optimization(QueryPlan& plan, f64 estimated_entities) {
        if (estimated_entities > PARALLEL_THRESHOLD_ENTITIES) {
            plan.add_optimization_step(QueryPlan::OptimizationStep{
                "parallel_execution",
                "Enable parallel processing across multiple threads",
                0.6 // 60% estimated improvement with parallelization
            });
        }
    }
    
    /**
     * @brief Apply spatial query optimization
     */
    void apply_spatial_optimization(QueryPlan& plan, bool is_spatial) {
        if (is_spatial) {
            plan.set_spatial_optimization(true, "rtree");
            
            plan.add_optimization_step(QueryPlan::OptimizationStep{
                "spatial_index",
                "Use R-tree spatial index for efficient spatial queries",
                0.8 // 80% estimated improvement for spatial queries
            });
        }
    }
    
    /**
     * @brief Apply early termination optimization
     */
    void apply_early_termination_optimization(QueryPlan& plan, f64 estimated_selectivity) {
        if (estimated_selectivity < HIGH_SELECTIVITY_THRESHOLD) {
            plan.add_optimization_step(QueryPlan::OptimizationStep{
                "early_termination",
                "Apply filters early to reduce processing overhead",
                0.4 // 40% estimated improvement for highly selective queries
            });
        }
    }
    
    /**
     * @brief Optimize memory layout and access patterns
     */
    template<typename... Components>
    void apply_memory_layout_optimization(QueryPlan& plan) {
        plan.add_optimization_step(QueryPlan::OptimizationStep{
            "memory_layout",
            "Optimize memory access patterns for cache efficiency",
            0.2 // 20% estimated improvement from better memory access
        });
    }
    
    /**
     * @brief Estimate execution time based on plan and historical data
     */
    f64 estimate_execution_time(const QueryPlan& plan, const std::string& query_signature) {
        // Check historical performance
        auto history_it = performance_history_.find(query_signature);
        if (history_it != performance_history_.end() && 
            history_it->second.sample_count > 3) {
            
            f64 base_time = history_it->second.average_time_per_entity * plan.estimated_entities_to_process();
            
            // Apply optimization factors
            f64 total_optimization_factor = 1.0;
            for (const auto& step : plan.optimization_steps()) {
                if (step.enabled) {
                    total_optimization_factor *= (1.0 - step.estimated_cost_reduction);
                }
            }
            
            return base_time * total_optimization_factor;
        }
        
        // Fallback estimation based on entity count and selectivity
        f64 base_cost_per_entity = 1.0; // 1 microsecond per entity base cost
        
        switch (plan.execution_strategy()) {
            case QueryPlan::ExecutionStrategy::Parallel:
                base_cost_per_entity *= 0.3; // Assume 70% reduction with parallelization
                break;
            case QueryPlan::ExecutionStrategy::Spatial:
                base_cost_per_entity *= 0.2; // Assume 80% reduction with spatial indexing
                break;
            case QueryPlan::ExecutionStrategy::Indexed:
                base_cost_per_entity *= 0.5; // Assume 50% reduction with indexing
                break;
            case QueryPlan::ExecutionStrategy::Hybrid:
                base_cost_per_entity *= 0.25; // Best of both worlds
                break;
            default:
                break;
        }
        
        return base_cost_per_entity * plan.estimated_entities_to_process();
    }
};

} // namespace ecscope::ecs::query