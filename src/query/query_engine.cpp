#include "../../include/ecscope/query/query_engine.hpp"
#include "../../include/ecscope/query/query_cache.hpp"
#include "../../include/ecscope/query/query_optimizer.hpp"
#include "../../include/ecscope/registry.hpp"

namespace ecscope::ecs::query {

// Global query engine instance management
namespace {
    std::unique_ptr<QueryEngine> global_query_engine_;
    std::mutex global_engine_mutex_;
}

QueryEngine& get_query_engine() {
    std::lock_guard<std::mutex> lock(global_engine_mutex_);
    
    if (!global_query_engine_) {
        // Create default query engine with registry
        auto& registry = ecscope::ecs::get_registry();
        global_query_engine_ = std::make_unique<QueryEngine>(&registry);
        LOG_INFO("Created default global QueryEngine");
    }
    
    return *global_query_engine_;
}

void set_query_engine(std::unique_ptr<QueryEngine> engine) {
    std::lock_guard<std::mutex> lock(global_engine_mutex_);
    global_query_engine_ = std::move(engine);
    LOG_INFO("Set custom global QueryEngine");
}

// QueryEngine implementation details

std::vector<Archetype*> QueryEngine::get_matching_archetypes(const ComponentSignature& required) const {
    std::vector<Archetype*> matching;
    
    // Access the registry's archetypes - this is a simplified approach
    // In the actual implementation, you'd need proper access to Registry internals
    auto archetype_stats = registry_->get_archetype_stats();
    
    for (const auto& [signature, entity_count] : archetype_stats) {
        if (signature.is_superset_of(required) && entity_count > 0) {
            // Find the actual archetype pointer
            // This would require proper integration with Registry
            // For now, we'll create a placeholder
        }
    }
    
    return matching;
}

usize QueryEngine::estimate_total_entities(const std::vector<Archetype*>& archetypes) const {
    usize total = 0;
    for (const auto* archetype : archetypes) {
        if (archetype) {
            total += archetype->entity_count();
        }
    }
    return total;
}

void QueryEngine::initialize_thread_pool() {
    // Thread pool initialization is handled in the advanced module
    LOG_DEBUG("Thread pool initialization requested");
}

void QueryEngine::update_query_frequency(const std::string& query_hash) const {
    query_frequency_[query_hash]++;
    
    if (config_.enable_hot_path_optimization && 
        query_frequency_[query_hash] >= 100) { // Hot query threshold
        hot_queries_.insert(query_hash);
        LOG_DEBUG("Query marked as hot: {}", query_hash);
    }
}

} // namespace ecscope::ecs::query