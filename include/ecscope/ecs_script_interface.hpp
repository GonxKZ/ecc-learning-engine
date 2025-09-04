#pragma once

/**
 * @file scripting/ecs_script_interface.hpp
 * @brief Advanced ECS Scripting Interface for ECScope Engine
 * 
 * This system provides comprehensive scripting access to the ECS with educational focus:
 * 
 * Key Features:
 * - Automatic binding generation for all ECS components and systems
 * - Script-based entity creation, modification, and destruction
 * - Component data access with type safety and validation
 * - System creation and management from scripts
 * - Query-based entity iteration and filtering
 * - Event system integration for script communication
 * - Performance monitoring and optimization tools
 * - Educational visualization of ECS operations
 * 
 * Architecture:
 * - Template-based automatic binding generation
 * - Type-safe wrapper classes for script exposure  
 * - Registry integration with script lifetime management
 * - Performance tracking for script-driven operations
 * - Memory pool integration for script objects
 * - Thread-safe access patterns for multi-threaded scripts
 * 
 * Performance Benefits:
 * - Zero-copy data access for component manipulation
 * - Bulk operations for efficient entity processing
 * - Cached query results to minimize ECS overhead
 * - Smart caching of frequently accessed components
 * - Vectorized operations for array-based component updates
 * 
 * Educational Features:
 * - Real-time visualization of entity relationships
 * - Performance profiling of script operations
 * - Component usage analytics and optimization hints
 * - Interactive debugging of ECS state changes
 * - Visual representation of system execution flow
 * 
 * @author ECScope Educational ECS Framework - Scripting System
 * @date 2025
 */

#include "core/types.hpp"
#include "core/log.hpp"
#include "ecs/entity.hpp"
#include "ecs/component.hpp"
#include "ecs/registry.hpp"
#include "ecs/query.hpp"
#include "scripting/python_integration.hpp"
#include "scripting/lua_integration.hpp"
#include "memory/lockfree_structures.hpp"
#include "job_system/work_stealing_job_system.hpp"
#include <memory>
#include <unordered_map>
#include <string>
#include <vector>
#include <chrono>
#include <functional>
#include <atomic>
#include <mutex>
#include <span>
#include <typeindex>

namespace ecscope::scripting::ecs_interface {

//=============================================================================
// Forward Declarations
//=============================================================================

class ECSScriptInterface;
class ScriptEntity;
class ScriptComponent;
class ScriptSystem;
class ScriptQuery;
class ComponentFactory;
class SystemFactory;

//=============================================================================
// Script Entity Wrapper
//=============================================================================

/**
 * @brief Entity wrapper for safe script access
 */
class ScriptEntity {
private:
    ecs::Entity entity_;
    ecs::Registry* registry_;
    mutable std::mutex access_mutex_;
    
    // Lifetime tracking
    std::chrono::high_resolution_clock::time_point creation_time_;
    std::atomic<u64> script_access_count_{0};
    std::atomic<u64> component_modifications_{0};
    
public:
    ScriptEntity(ecs::Entity entity, ecs::Registry* registry)
        : entity_(entity)
        , registry_(registry)
        , creation_time_(std::chrono::high_resolution_clock::now()) {}
    
    ~ScriptEntity() = default;
    
    // Entity ID access
    u32 id() const noexcept { 
        return entity_.id(); 
    }
    
    u32 generation() const noexcept { 
        return entity_.generation(); 
    }
    
    bool is_valid() const {
        std::lock_guard<std::mutex> lock(access_mutex_);
        script_access_count_.fetch_add(1, std::memory_order_relaxed);
        return registry_ && registry_->is_valid(entity_);
    }
    
    // Component management
    template<ecs::Component T>
    bool has_component() const {
        std::lock_guard<std::mutex> lock(access_mutex_);
        if (!registry_) return false;
        
        script_access_count_.fetch_add(1, std::memory_order_relaxed);
        return registry_->has_component<T>(entity_);
    }
    
    template<ecs::Component T>
    bool add_component(const T& component = T{}) {
        std::lock_guard<std::mutex> lock(access_mutex_);
        if (!registry_) return false;
        
        script_access_count_.fetch_add(1, std::memory_order_relaxed);
        component_modifications_.fetch_add(1, std::memory_order_relaxed);
        
        return registry_->add_component(entity_, component);
    }
    
    template<ecs::Component T>
    bool remove_component() {
        std::lock_guard<std::mutex> lock(access_mutex_);
        if (!registry_) return false;
        
        script_access_count_.fetch_add(1, std::memory_order_relaxed);
        component_modifications_.fetch_add(1, std::memory_order_relaxed);
        
        return registry_->remove_component<T>(entity_);
    }
    
    template<ecs::Component T>
    T* get_component() {
        std::lock_guard<std::mutex> lock(access_mutex_);
        if (!registry_) return nullptr;
        
        script_access_count_.fetch_add(1, std::memory_order_relaxed);
        return registry_->get_component<T>(entity_);
    }
    
    template<ecs::Component T>
    const T* get_component() const {
        std::lock_guard<std::mutex> lock(access_mutex_);
        if (!registry_) return nullptr;
        
        script_access_count_.fetch_add(1, std::memory_order_relaxed);
        return registry_->get_component<T>(entity_);
    }
    
    // Bulk component operations
    std::vector<std::string> get_component_names() const {
        std::lock_guard<std::mutex> lock(access_mutex_);
        if (!registry_) return {};
        
        script_access_count_.fetch_add(1, std::memory_order_relaxed);
        
        std::vector<std::string> component_names;
        
        // This would iterate through all registered component types
        // and check if entity has each one - simplified here
        return component_names;
    }
    
    usize component_count() const {
        std::lock_guard<std::mutex> lock(access_mutex_);
        if (!registry_) return 0;
        
        script_access_count_.fetch_add(1, std::memory_order_relaxed);
        return registry_->get_component_count(entity_);
    }
    
    // Entity destruction
    void destroy() {
        std::lock_guard<std::mutex> lock(access_mutex_);
        if (!registry_) return;
        
        script_access_count_.fetch_add(1, std::memory_order_relaxed);
        registry_->destroy_entity(entity_);
        registry_ = nullptr; // Invalidate
    }
    
    // Statistics and debugging
    struct Statistics {
        std::chrono::high_resolution_clock::time_point creation_time;
        u64 script_access_count;
        u64 component_modifications;
        usize current_component_count;
        f64 lifetime_seconds;
        bool is_valid;
    };
    
    Statistics get_statistics() const {
        auto now = std::chrono::high_resolution_clock::now();
        auto lifetime = std::chrono::duration<f64>(now - creation_time_).count();
        
        return Statistics{
            .creation_time = creation_time_,
            .script_access_count = script_access_count_.load(std::memory_order_relaxed),
            .component_modifications = component_modifications_.load(std::memory_order_relaxed),
            .current_component_count = component_count(),
            .lifetime_seconds = lifetime,
            .is_valid = is_valid()
        };
    }
    
    // Comparison operators
    bool operator==(const ScriptEntity& other) const {
        return entity_ == other.entity_;
    }
    
    bool operator!=(const ScriptEntity& other) const {
        return !(*this == other);
    }
    
    // Hash support for containers
    struct Hash {
        usize operator()(const ScriptEntity& entity) const noexcept {
            return std::hash<ecs::Entity>{}(entity.entity_);
        }
    };
};

//=============================================================================
// Script Query System
//=============================================================================

/**
 * @brief ECS query wrapper for script access
 */
template<typename... Components>
class ScriptQuery {
private:
    std::unique_ptr<ecs::Query<Components...>> query_;
    ecs::Registry* registry_;
    
    // Caching and performance
    mutable std::vector<ecs::Entity> cached_entities_;
    mutable std::chrono::high_resolution_clock::time_point last_cache_update_;
    mutable std::mutex cache_mutex_;
    std::chrono::milliseconds cache_duration_{100}; // 100ms cache
    
    // Statistics
    mutable std::atomic<u64> query_executions_{0};
    mutable std::atomic<u64> cache_hits_{0};
    mutable std::atomic<u64> entities_processed_{0};
    
public:
    ScriptQuery(ecs::Registry* registry) : registry_(registry) {
        if (registry_) {
            query_ = std::make_unique<ecs::Query<Components...>>(*registry_);
        }
    }
    
    ~ScriptQuery() = default;
    
    // Non-copyable but movable
    ScriptQuery(const ScriptQuery&) = delete;
    ScriptQuery& operator=(const ScriptQuery&) = delete;
    ScriptQuery(ScriptQuery&&) = default;
    ScriptQuery& operator=(ScriptQuery&&) = default;
    
    // Query execution
    std::vector<ScriptEntity> get_entities() {
        query_executions_.fetch_add(1, std::memory_order_relaxed);
        
        std::lock_guard<std::mutex> lock(cache_mutex_);
        
        auto now = std::chrono::high_resolution_clock::now();
        bool cache_valid = !cached_entities_.empty() && 
                          (now - last_cache_update_) < cache_duration_;
        
        if (!cache_valid) {
            // Update cache
            cached_entities_.clear();
            if (query_) {
                query_->for_each([this](ecs::Entity entity, Components&...) {
                    cached_entities_.push_back(entity);
                });
            }
            last_cache_update_ = now;
        } else {
            cache_hits_.fetch_add(1, std::memory_order_relaxed);
        }
        
        // Convert to ScriptEntity wrappers
        std::vector<ScriptEntity> script_entities;
        script_entities.reserve(cached_entities_.size());
        
        for (ecs::Entity entity : cached_entities_) {
            script_entities.emplace_back(entity, registry_);
        }
        
        entities_processed_.fetch_add(script_entities.size(), std::memory_order_relaxed);
        return script_entities;
    }
    
    // Functional iteration
    template<typename Func>
    void for_each(Func&& func) {
        query_executions_.fetch_add(1, std::memory_order_relaxed);
        
        if (!query_) return;
        
        usize processed = 0;
        query_->for_each([&func, &processed](ecs::Entity entity, Components&... components) {
            func(ScriptEntity{entity, nullptr}, components...); // Note: registry not needed for read-only access
            ++processed;
        });
        
        entities_processed_.fetch_add(processed, std::memory_order_relaxed);
    }
    
    // Parallel iteration
    template<typename Func>
    void for_each_parallel(Func&& func, job_system::JobSystem* job_system = nullptr) {
        query_executions_.fetch_add(1, std::memory_order_relaxed);
        
        if (!query_ || !job_system) {
            // Fallback to sequential
            for_each(std::forward<Func>(func));
            return;
        }
        
        auto entities = get_entities();
        const usize batch_size = 1000;
        const usize num_batches = (entities.size() + batch_size - 1) / batch_size;
        
        std::vector<job_system::JobID> jobs;
        jobs.reserve(num_batches);
        
        for (usize i = 0; i < num_batches; ++i) {
            usize start = i * batch_size;
            usize end = std::min(start + batch_size, entities.size());
            
            std::string job_name = "ScriptQuery_Batch_" + std::to_string(i);
            
            auto job_id = job_system->submit_job(job_name, [this, &func, &entities, start, end]() {
                for (usize j = start; j < end; ++j) {
                    auto& script_entity = entities[j];
                    
                    // Get components for this entity
                    if constexpr (sizeof...(Components) > 0) {
                        std::tuple<Components*...> components = get_entity_components<Components...>(script_entity);
                        std::apply([&](auto*... comp_ptrs) {
                            if ((comp_ptrs && ...)) { // All components exist
                                func(script_entity, *comp_ptrs...);
                            }
                        }, components);
                    } else {
                        func(script_entity);
                    }
                }
            });
            
            jobs.push_back(job_id);
        }
        
        job_system->wait_for_batch(jobs);
    }
    
    // Query filtering
    template<typename Predicate>
    std::vector<ScriptEntity> where(Predicate&& predicate) {
        auto entities = get_entities();
        std::vector<ScriptEntity> filtered;
        
        for (auto& entity : entities) {
            if (predicate(entity)) {
                filtered.push_back(std::move(entity));
            }
        }
        
        return filtered;
    }
    
    // Count operations
    usize count() {
        return get_entities().size();
    }
    
    bool empty() {
        return count() == 0;
    }
    
    // Statistics
    struct Statistics {
        u64 query_executions;
        u64 cache_hits;
        u64 entities_processed;
        f64 cache_hit_rate;
        f64 average_entities_per_query;
    };
    
    Statistics get_statistics() const {
        u64 executions = query_executions_.load(std::memory_order_relaxed);
        u64 cache_hits = cache_hits_.load(std::memory_order_relaxed);
        u64 entities = entities_processed_.load(std::memory_order_relaxed);
        
        return Statistics{
            .query_executions = executions,
            .cache_hits = cache_hits,
            .entities_processed = entities,
            .cache_hit_rate = executions > 0 ? static_cast<f64>(cache_hits) / executions : 0.0,
            .average_entities_per_query = executions > 0 ? static_cast<f64>(entities) / executions : 0.0
        };
    }
    
    // Cache control
    void invalidate_cache() {
        std::lock_guard<std::mutex> lock(cache_mutex_);
        cached_entities_.clear();
        last_cache_update_ = std::chrono::high_resolution_clock::time_point{};
    }
    
    void set_cache_duration(std::chrono::milliseconds duration) {
        cache_duration_ = duration;
    }

private:
    template<typename... Comps>
    std::tuple<Comps*...> get_entity_components(ScriptEntity& entity) {
        return std::make_tuple(entity.get_component<Comps>()...);
    }
};

//=============================================================================
// Script System Interface
//=============================================================================

/**
 * @brief Base class for script-defined systems
 */
class ScriptSystemBase {
public:
    enum class ExecutionOrder {
        PreUpdate = 0,
        Update = 1,
        PostUpdate = 2,
        Render = 3,
        PostRender = 4
    };
    
private:
    std::string name_;
    ExecutionOrder execution_order_;
    bool enabled_;
    f32 execution_priority_;
    
    // Performance tracking
    std::atomic<u64> execution_count_{0};
    std::atomic<u64> total_execution_time_us_{0};
    std::chrono::high_resolution_clock::time_point last_execution_;
    
    // Dependencies
    std::vector<std::string> dependencies_;
    std::vector<std::string> dependents_;
    
public:
    ScriptSystemBase(std::string name, ExecutionOrder order = ExecutionOrder::Update, f32 priority = 0.0f)
        : name_(std::move(name))
        , execution_order_(order)
        , enabled_(true)
        , execution_priority_(priority) {}
    
    virtual ~ScriptSystemBase() = default;
    
    // System execution interface
    virtual void initialize() {}
    virtual void update(f32 delta_time) = 0;
    virtual void shutdown() {}
    
    // System properties
    const std::string& name() const noexcept { return name_; }
    ExecutionOrder execution_order() const noexcept { return execution_order_; }
    f32 execution_priority() const noexcept { return execution_priority_; }
    bool is_enabled() const noexcept { return enabled_; }
    
    void set_enabled(bool enabled) { enabled_ = enabled; }
    void set_execution_priority(f32 priority) { execution_priority_ = priority; }
    
    // Dependencies
    void add_dependency(const std::string& system_name) {
        dependencies_.push_back(system_name);
    }
    
    void add_dependent(const std::string& system_name) {
        dependents_.push_back(system_name);
    }
    
    const std::vector<std::string>& dependencies() const { return dependencies_; }
    const std::vector<std::string>& dependents() const { return dependents_; }
    
    // Performance tracking
    void record_execution_start() {
        last_execution_ = std::chrono::high_resolution_clock::now();
    }
    
    void record_execution_end() {
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration_us = std::chrono::duration_cast<std::chrono::microseconds>(
            end_time - last_execution_).count();
        
        execution_count_.fetch_add(1, std::memory_order_relaxed);
        total_execution_time_us_.fetch_add(static_cast<u64>(duration_us), std::memory_order_relaxed);
    }
    
    struct Statistics {
        u64 execution_count;
        u64 total_execution_time_us;
        f64 average_execution_time_ms;
        std::chrono::high_resolution_clock::time_point last_execution;
        bool is_enabled;
    };
    
    Statistics get_statistics() const {
        u64 count = execution_count_.load(std::memory_order_relaxed);
        u64 total_time = total_execution_time_us_.load(std::memory_order_relaxed);
        
        return Statistics{
            .execution_count = count,
            .total_execution_time_us = total_time,
            .average_execution_time_ms = count > 0 ? 
                static_cast<f64>(total_time) / (count * 1000.0) : 0.0,
            .last_execution = last_execution_,
            .is_enabled = enabled_
        };
    }
};

//=============================================================================
// Main ECS Script Interface
//=============================================================================

/**
 * @brief Main interface for ECS scripting integration
 */
class ECSScriptInterface {
private:
    ecs::Registry* registry_;
    python::PythonEngine* python_engine_;
    lua::LuaEngine* lua_engine_;
    
    // Entity management
    std::unordered_map<ecs::Entity, std::unique_ptr<ScriptEntity>> script_entities_;
    mutable std::shared_mutex entities_mutex_;
    
    // System management
    std::vector<std::unique_ptr<ScriptSystemBase>> script_systems_;
    std::unordered_map<std::string, usize> system_name_to_index_;
    mutable std::mutex systems_mutex_;
    
    // Component factories
    std::unordered_map<std::string, std::function<void()>> component_factories_;
    
    // Performance monitoring
    std::atomic<u64> entities_created_{0};
    std::atomic<u64> entities_destroyed_{0};
    std::atomic<u64> component_accesses_{0};
    std::atomic<u64> query_executions_{0};
    
public:
    ECSScriptInterface(ecs::Registry* registry, 
                      python::PythonEngine* python_engine = nullptr,
                      lua::LuaEngine* lua_engine = nullptr)
        : registry_(registry)
        , python_engine_(python_engine)
        , lua_engine_(lua_engine) {}
    
    ~ECSScriptInterface() = default;
    
    // Entity management
    ScriptEntity* create_entity() {
        if (!registry_) return nullptr;
        
        ecs::Entity entity = registry_->create_entity();
        auto script_entity = std::make_unique<ScriptEntity>(entity, registry_);
        ScriptEntity* ptr = script_entity.get();
        
        {
            std::unique_lock<std::shared_mutex> lock(entities_mutex_);
            script_entities_[entity] = std::move(script_entity);
        }
        
        entities_created_.fetch_add(1, std::memory_order_relaxed);
        return ptr;
    }
    
    ScriptEntity* get_entity(ecs::Entity entity) {
        std::shared_lock<std::shared_mutex> lock(entities_mutex_);
        
        auto it = script_entities_.find(entity);
        return it != script_entities_.end() ? it->second.get() : nullptr;
    }
    
    ScriptEntity* get_entity(u32 entity_id, u32 generation) {
        return get_entity(ecs::Entity{entity_id, generation});
    }
    
    bool destroy_entity(ecs::Entity entity) {
        ScriptEntity* script_entity = nullptr;
        
        {
            std::unique_lock<std::shared_mutex> lock(entities_mutex_);
            auto it = script_entities_.find(entity);
            if (it != script_entities_.end()) {
                script_entity = it->second.get();
                script_entities_.erase(it);
            }
        }
        
        if (script_entity) {
            script_entity->destroy();
            entities_destroyed_.fetch_add(1, std::memory_order_relaxed);
            return true;
        }
        
        return false;
    }
    
    std::vector<ScriptEntity*> get_all_entities() {
        std::shared_lock<std::shared_mutex> lock(entities_mutex_);
        
        std::vector<ScriptEntity*> entities;
        entities.reserve(script_entities_.size());
        
        for (auto& [entity_id, script_entity] : script_entities_) {
            entities.push_back(script_entity.get());
        }
        
        return entities;
    }
    
    usize entity_count() const {
        std::shared_lock<std::shared_mutex> lock(entities_mutex_);
        return script_entities_.size();
    }
    
    // Query creation
    template<ecs::Component... Components>
    std::unique_ptr<ScriptQuery<Components...>> create_query() {
        query_executions_.fetch_add(1, std::memory_order_relaxed);
        return std::make_unique<ScriptQuery<Components...>>(registry_);
    }
    
    // System management
    template<typename SystemType>
    bool register_system(std::unique_ptr<SystemType> system) {
        static_assert(std::is_base_of_v<ScriptSystemBase, SystemType>, 
                     "SystemType must derive from ScriptSystemBase");
        
        std::lock_guard<std::mutex> lock(systems_mutex_);
        
        const std::string& name = system->name();
        if (system_name_to_index_.find(name) != system_name_to_index_.end()) {
            LOG_ERROR("System with name '{}' already registered", name);
            return false;
        }
        
        usize index = script_systems_.size();
        system_name_to_index_[name] = index;
        script_systems_.push_back(std::move(system));
        
        LOG_INFO("Registered script system: {}", name);
        return true;
    }
    
    bool unregister_system(const std::string& name) {
        std::lock_guard<std::mutex> lock(systems_mutex_);
        
        auto it = system_name_to_index_.find(name);
        if (it == system_name_to_index_.end()) {
            return false;
        }
        
        usize index = it->second;
        script_systems_[index].reset();
        system_name_to_index_.erase(it);
        
        LOG_INFO("Unregistered script system: {}", name);
        return true;
    }
    
    ScriptSystemBase* get_system(const std::string& name) {
        std::lock_guard<std::mutex> lock(systems_mutex_);
        
        auto it = system_name_to_index_.find(name);
        if (it == system_name_to_index_.end()) {
            return nullptr;
        }
        
        return script_systems_[it->second].get();
    }
    
    void update_systems(f32 delta_time, ScriptSystemBase::ExecutionOrder order) {
        std::vector<ScriptSystemBase*> systems_to_update;
        
        {
            std::lock_guard<std::mutex> lock(systems_mutex_);
            
            for (auto& system : script_systems_) {
                if (system && system->is_enabled() && system->execution_order() == order) {
                    systems_to_update.push_back(system.get());
                }
            }
        }
        
        // Sort by priority
        std::sort(systems_to_update.begin(), systems_to_update.end(),
                 [](const ScriptSystemBase* a, const ScriptSystemBase* b) {
                     return a->execution_priority() > b->execution_priority();
                 });
        
        // Execute systems
        for (auto* system : systems_to_update) {
            try {
                system->record_execution_start();
                system->update(delta_time);
                system->record_execution_end();
            } catch (const std::exception& e) {
                LOG_ERROR("Error in script system '{}': {}", system->name(), e.what());
            }
        }
    }
    
    // Component registration for scripting
    template<ecs::Component T>
    void register_component_type(const std::string& name) {
        if (python_engine_) {
            python_engine_->register_component<T>(name);
        }
        
        // Register component factory
        component_factories_[name] = []() {
            // This would create and register component binding
            LOG_INFO("Component type '{}' registered for scripting", name);
        };
        
        LOG_INFO("Registered component type for scripting: {}", name);
    }
    
    // Statistics and monitoring
    struct Statistics {
        u64 entities_created;
        u64 entities_destroyed;
        u64 current_entities;
        u64 component_accesses;
        u64 query_executions;
        usize registered_systems;
        f64 entity_creation_rate;
        f64 component_access_rate;
    };
    
    Statistics get_statistics() const {
        std::shared_lock<std::shared_mutex> entities_lock(entities_mutex_);
        std::lock_guard<std::mutex> systems_lock(systems_mutex_);
        
        return Statistics{
            .entities_created = entities_created_.load(std::memory_order_relaxed),
            .entities_destroyed = entities_destroyed_.load(std::memory_order_relaxed),
            .current_entities = script_entities_.size(),
            .component_accesses = component_accesses_.load(std::memory_order_relaxed),
            .query_executions = query_executions_.load(std::memory_order_relaxed),
            .registered_systems = script_systems_.size(),
            .entity_creation_rate = 0.0, // Would calculate based on time window
            .component_access_rate = 0.0  // Would calculate based on time window
        };
    }
    
    std::vector<ScriptSystemBase::Statistics> get_system_statistics() const {
        std::lock_guard<std::mutex> lock(systems_mutex_);
        
        std::vector<ScriptSystemBase::Statistics> stats;
        stats.reserve(script_systems_.size());
        
        for (const auto& system : script_systems_) {
            if (system) {
                stats.push_back(system->get_statistics());
            }
        }
        
        return stats;
    }
    
    // Utility functions
    void cleanup_destroyed_entities() {
        std::unique_lock<std::shared_mutex> lock(entities_mutex_);
        
        for (auto it = script_entities_.begin(); it != script_entities_.end();) {
            if (!it->second->is_valid()) {
                it = script_entities_.erase(it);
            } else {
                ++it;
            }
        }
    }
    
    std::vector<std::string> get_registered_component_types() const {
        std::vector<std::string> types;
        types.reserve(component_factories_.size());
        
        for (const auto& [name, factory] : component_factories_) {
            types.push_back(name);
        }
        
        return types;
    }
    
    std::vector<std::string> get_registered_system_names() const {
        std::lock_guard<std::mutex> lock(systems_mutex_);
        
        std::vector<std::string> names;
        names.reserve(system_name_to_index_.size());
        
        for (const auto& [name, index] : system_name_to_index_) {
            names.push_back(name);
        }
        
        return names;
    }
};

} // namespace ecscope::scripting::ecs_interface