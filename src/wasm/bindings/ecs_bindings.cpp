// =============================================================================
// ECScope ECS JavaScript Bindings using Embind
// =============================================================================

#include <emscripten/bind.h>
#include <emscripten/val.h>
#include <memory>
#include <vector>
#include <string>

// ECScope includes
#include "ecscope/registry.hpp"
#include "ecscope/entity.hpp"
#include "ecscope/component.hpp"
#include "ecscope/system.hpp"
#include "ecscope/archetype.hpp"
#include "ecscope/query.hpp"

using namespace emscripten;
using namespace ecscope;

// =============================================================================
// WEBASSEMBLY ECS WRAPPER CLASSES
// =============================================================================

class WasmECSRegistry {
private:
    std::unique_ptr<Registry> registry_;

public:
    WasmECSRegistry() : registry_(std::make_unique<Registry>()) {}

    // Entity management
    uint32_t createEntity() {
        Entity entity = registry_->create();
        return entity.id();
    }

    void destroyEntity(uint32_t entity_id) {
        Entity entity(entity_id);
        if (registry_->valid(entity)) {
            registry_->destroy(entity);
        }
    }

    bool isEntityValid(uint32_t entity_id) {
        Entity entity(entity_id);
        return registry_->valid(entity);
    }

    // Component management (simplified for basic types)
    void addPositionComponent(uint32_t entity_id, float x, float y, float z) {
        Entity entity(entity_id);
        if (registry_->valid(entity)) {
            struct Position { float x, y, z; };
            registry_->emplace<Position>(entity, Position{x, y, z});
        }
    }

    val getPositionComponent(uint32_t entity_id) {
        Entity entity(entity_id);
        if (registry_->valid(entity)) {
            struct Position { float x, y, z; };
            if (registry_->has<Position>(entity)) {
                const auto& pos = registry_->get<Position>(entity);
                val result = val::object();
                result.set("x", pos.x);
                result.set("y", pos.y);
                result.set("z", pos.z);
                return result;
            }
        }
        return val::null();
    }

    void addVelocityComponent(uint32_t entity_id, float x, float y, float z) {
        Entity entity(entity_id);
        if (registry_->valid(entity)) {
            struct Velocity { float x, y, z; };
            registry_->emplace<Velocity>(entity, Velocity{x, y, z});
        }
    }

    val getVelocityComponent(uint32_t entity_id) {
        Entity entity(entity_id);
        if (registry_->valid(entity)) {
            struct Velocity { float x, y, z; };
            if (registry_->has<Velocity>(entity)) {
                const auto& vel = registry_->get<Velocity>(entity);
                val result = val::object();
                result.set("x", vel.x);
                result.set("y", vel.y);
                result.set("z", vel.z);
                return result;
            }
        }
        return val::null();
    }

    void addHealthComponent(uint32_t entity_id, float health, float max_health) {
        Entity entity(entity_id);
        if (registry_->valid(entity)) {
            struct Health { float current, maximum; };
            registry_->emplace<Health>(entity, Health{health, max_health});
        }
    }

    val getHealthComponent(uint32_t entity_id) {
        Entity entity(entity_id);
        if (registry_->valid(entity)) {
            struct Health { float current, maximum; };
            if (registry_->has<Health>(entity)) {
                const auto& health = registry_->get<Health>(entity);
                val result = val::object();
                result.set("current", health.current);
                result.set("maximum", health.maximum);
                return result;
            }
        }
        return val::null();
    }

    // System execution (simplified)
    void runMovementSystem(float delta_time) {
        struct Position { float x, y, z; };
        struct Velocity { float x, y, z; };

        auto view = registry_->view<Position, Velocity>();
        for (auto entity : view) {
            auto& pos = view.get<Position>(entity);
            const auto& vel = view.get<Velocity>(entity);
            
            pos.x += vel.x * delta_time;
            pos.y += vel.y * delta_time;
            pos.z += vel.z * delta_time;
        }
    }

    // Query entities
    val queryEntitiesWithPosition() {
        struct Position { float x, y, z; };
        auto view = registry_->view<Position>();
        
        val entities = val::array();
        int index = 0;
        for (auto entity : view) {
            entities.set(index++, static_cast<uint32_t>(entity.id()));
        }
        return entities;
    }

    val queryEntitiesWithPositionAndVelocity() {
        struct Position { float x, y, z; };
        struct Velocity { float x, y, z; };
        auto view = registry_->view<Position, Velocity>();
        
        val entities = val::array();
        int index = 0;
        for (auto entity : view) {
            entities.set(index++, static_cast<uint32_t>(entity.id()));
        }
        return entities;
    }

    // Statistics
    size_t getEntityCount() const {
        return registry_->size();
    }

    size_t getArchetypeCount() const {
        return registry_->archetype_count();
    }

    val getArchetypeInfo() {
        val archetypes = val::array();
        int index = 0;
        
        // Simplified archetype info
        registry_->for_each_archetype([&](const auto& archetype) {
            val archetype_info = val::object();
            archetype_info.set("id", index);
            archetype_info.set("entity_count", archetype.size());
            archetype_info.set("component_count", archetype.component_count());
            archetypes.set(index++, archetype_info);
        });
        
        return archetypes;
    }

    // Memory usage
    size_t getMemoryUsage() const {
        return registry_->memory_usage();
    }

    // Performance testing
    void createManyEntities(int count) {
        for (int i = 0; i < count; ++i) {
            Entity entity = registry_->create();
            addPositionComponent(entity.id(), 
                static_cast<float>(i % 100), 
                static_cast<float>(i % 50), 
                0.0f);
            
            if (i % 2 == 0) {
                addVelocityComponent(entity.id(), 1.0f, 1.0f, 0.0f);
            }
            
            if (i % 3 == 0) {
                addHealthComponent(entity.id(), 100.0f, 100.0f);
            }
        }
    }

    void benchmarkQueries(int iterations) {
        struct Position { float x, y, z; };
        struct Velocity { float x, y, z; };
        
        for (int i = 0; i < iterations; ++i) {
            auto view = registry_->view<Position, Velocity>();
            volatile size_t count = 0;
            for (auto entity : view) {
                count++;
            }
        }
    }
};

// =============================================================================
// WEBASSEMBLY PERFORMANCE MONITOR FOR ECS
// =============================================================================

class WasmECSPerformanceMonitor {
private:
    std::chrono::high_resolution_clock::time_point last_update_;
    size_t query_count_ = 0;
    size_t system_execution_count_ = 0;
    double total_query_time_ = 0.0;
    double total_system_time_ = 0.0;

public:
    WasmECSPerformanceMonitor() {
        last_update_ = std::chrono::high_resolution_clock::now();
    }

    void beginQuery() {
        query_start_ = std::chrono::high_resolution_clock::now();
    }

    void endQuery() {
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration<double, std::milli>(end - query_start_).count();
        total_query_time_ += duration;
        query_count_++;
    }

    void beginSystemExecution() {
        system_start_ = std::chrono::high_resolution_clock::now();
    }

    void endSystemExecution() {
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration<double, std::milli>(end - system_start_).count();
        total_system_time_ += duration;
        system_execution_count_++;
    }

    val getStatistics() {
        val stats = val::object();
        stats.set("queryCount", query_count_);
        stats.set("systemExecutionCount", system_execution_count_);
        stats.set("averageQueryTime", query_count_ > 0 ? total_query_time_ / query_count_ : 0.0);
        stats.set("averageSystemTime", system_execution_count_ > 0 ? total_system_time_ / system_execution_count_ : 0.0);
        stats.set("totalQueryTime", total_query_time_);
        stats.set("totalSystemTime", total_system_time_);
        return stats;
    }

    void reset() {
        query_count_ = 0;
        system_execution_count_ = 0;
        total_query_time_ = 0.0;
        total_system_time_ = 0.0;
        last_update_ = std::chrono::high_resolution_clock::now();
    }

private:
    std::chrono::high_resolution_clock::time_point query_start_;
    std::chrono::high_resolution_clock::time_point system_start_;
};

// =============================================================================
// WEBASSEMBLY ECS TUTORIAL SYSTEM
// =============================================================================

class WasmECSTutorial {
private:
    WasmECSRegistry registry_;
    WasmECSPerformanceMonitor performance_;
    std::vector<uint32_t> tutorial_entities_;

public:
    // Tutorial Step 1: Basic entity creation
    void step1_CreateEntities() {
        tutorial_entities_.clear();
        
        // Create a player entity
        uint32_t player = registry_.createEntity();
        registry_.addPositionComponent(player, 0.0f, 0.0f, 0.0f);
        registry_.addHealthComponent(player, 100.0f, 100.0f);
        tutorial_entities_.push_back(player);
        
        // Create some NPCs
        for (int i = 0; i < 5; ++i) {
            uint32_t npc = registry_.createEntity();
            registry_.addPositionComponent(npc, 
                static_cast<float>(i * 10), 
                static_cast<float>(i * 5), 
                0.0f);
            registry_.addHealthComponent(npc, 50.0f + i * 10, 100.0f);
            tutorial_entities_.push_back(npc);
        }
    }

    // Tutorial Step 2: Add movement
    void step2_AddMovement() {
        for (size_t i = 1; i < tutorial_entities_.size(); ++i) {
            registry_.addVelocityComponent(tutorial_entities_[i], 
                (i % 2 == 0) ? 1.0f : -1.0f,
                (i % 3 == 0) ? 1.0f : -1.0f,
                0.0f);
        }
    }

    // Tutorial Step 3: Run systems
    void step3_RunSystems(float delta_time) {
        performance_.beginSystemExecution();
        registry_.runMovementSystem(delta_time);
        performance_.endSystemExecution();
    }

    // Tutorial Step 4: Query entities
    val step4_QueryEntities() {
        performance_.beginQuery();
        val moving_entities = registry_.queryEntitiesWithPositionAndVelocity();
        performance_.endQuery();
        return moving_entities;
    }

    // Get tutorial state
    val getTutorialState() {
        val state = val::object();
        state.set("entityCount", tutorial_entities_.size());
        state.set("registryStats", registry_.getArchetypeInfo());
        state.set("performance", performance_.getStatistics());
        
        val entities = val::array();
        for (size_t i = 0; i < tutorial_entities_.size(); ++i) {
            uint32_t id = tutorial_entities_[i];
            val entity = val::object();
            entity.set("id", id);
            entity.set("position", registry_.getPositionComponent(id));
            entity.set("velocity", registry_.getVelocityComponent(id));
            entity.set("health", registry_.getHealthComponent(id));
            entities.set(static_cast<int>(i), entity);
        }
        state.set("entities", entities);
        
        return state;
    }

    void resetTutorial() {
        for (uint32_t id : tutorial_entities_) {
            registry_.destroyEntity(id);
        }
        tutorial_entities_.clear();
        performance_.reset();
    }

    WasmECSRegistry& getRegistry() { return registry_; }
    WasmECSPerformanceMonitor& getPerformance() { return performance_; }
};

// =============================================================================
// EMBIND BINDINGS
// =============================================================================

EMSCRIPTEN_BINDINGS(ecscope_ecs) {
    // Registry bindings
    class_<WasmECSRegistry>("ECSRegistry")
        .constructor<>()
        .function("createEntity", &WasmECSRegistry::createEntity)
        .function("destroyEntity", &WasmECSRegistry::destroyEntity)
        .function("isEntityValid", &WasmECSRegistry::isEntityValid)
        .function("addPositionComponent", &WasmECSRegistry::addPositionComponent)
        .function("getPositionComponent", &WasmECSRegistry::getPositionComponent)
        .function("addVelocityComponent", &WasmECSRegistry::addVelocityComponent)
        .function("getVelocityComponent", &WasmECSRegistry::getVelocityComponent)
        .function("addHealthComponent", &WasmECSRegistry::addHealthComponent)
        .function("getHealthComponent", &WasmECSRegistry::getHealthComponent)
        .function("runMovementSystem", &WasmECSRegistry::runMovementSystem)
        .function("queryEntitiesWithPosition", &WasmECSRegistry::queryEntitiesWithPosition)
        .function("queryEntitiesWithPositionAndVelocity", &WasmECSRegistry::queryEntitiesWithPositionAndVelocity)
        .function("getEntityCount", &WasmECSRegistry::getEntityCount)
        .function("getArchetypeCount", &WasmECSRegistry::getArchetypeCount)
        .function("getArchetypeInfo", &WasmECSRegistry::getArchetypeInfo)
        .function("getMemoryUsage", &WasmECSRegistry::getMemoryUsage)
        .function("createManyEntities", &WasmECSRegistry::createManyEntities)
        .function("benchmarkQueries", &WasmECSRegistry::benchmarkQueries);

    // Performance monitor bindings
    class_<WasmECSPerformanceMonitor>("ECSPerformanceMonitor")
        .constructor<>()
        .function("beginQuery", &WasmECSPerformanceMonitor::beginQuery)
        .function("endQuery", &WasmECSPerformanceMonitor::endQuery)
        .function("beginSystemExecution", &WasmECSPerformanceMonitor::beginSystemExecution)
        .function("endSystemExecution", &WasmECSPerformanceMonitor::endSystemExecution)
        .function("getStatistics", &WasmECSPerformanceMonitor::getStatistics)
        .function("reset", &WasmECSPerformanceMonitor::reset);

    // Tutorial system bindings
    class_<WasmECSTutorial>("ECSTutorial")
        .constructor<>()
        .function("step1_CreateEntities", &WasmECSTutorial::step1_CreateEntities)
        .function("step2_AddMovement", &WasmECSTutorial::step2_AddMovement)
        .function("step3_RunSystems", &WasmECSTutorial::step3_RunSystems)
        .function("step4_QueryEntities", &WasmECSTutorial::step4_QueryEntities)
        .function("getTutorialState", &WasmECSTutorial::getTutorialState)
        .function("resetTutorial", &WasmECSTutorial::resetTutorial);
}