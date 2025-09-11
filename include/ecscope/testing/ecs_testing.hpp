#pragma once

#include "test_framework.hpp"
#include "../ecs/registry.hpp"
#include "../ecs/entity.hpp"
#include "../ecs/component.hpp"
#include "../ecs/system.hpp"
#include "../ecs/archetype.hpp"
#include "../ecs/world.hpp"
#include <unordered_set>
#include <type_traits>

namespace ecscope::testing {

// ECS component validator
class ComponentValidator {
public:
    template<typename Component>
    static bool validate_component_structure() {
        static_assert(std::is_trivially_copyable_v<Component>, 
                     "Components must be trivially copyable");
        static_assert(std::is_standard_layout_v<Component>,
                     "Components must have standard layout");
        return true;
    }

    template<typename Component>
    static bool validate_component_size() {
        constexpr size_t max_component_size = 1024; // 1KB max
        static_assert(sizeof(Component) <= max_component_size,
                     "Component size exceeds maximum allowed size");
        return true;
    }

    template<typename Component>
    static bool validate_component_alignment() {
        constexpr size_t alignment = alignof(Component);
        constexpr size_t max_alignment = 64; // 64-byte alignment max
        static_assert(alignment <= max_alignment,
                     "Component alignment exceeds maximum allowed alignment");
        return true;
    }
};

// ECS system performance monitor
class SystemPerformanceMonitor {
public:
    struct SystemMetrics {
        std::string system_name;
        std::chrono::nanoseconds execution_time{0};
        size_t entities_processed{0};
        size_t components_accessed{0};
        double entities_per_second{0.0};
    };

    void start_monitoring(const std::string& system_name) {
        current_system_ = system_name;
        start_time_ = std::chrono::high_resolution_clock::now();
        entities_processed_ = 0;
        components_accessed_ = 0;
    }

    void record_entity_processed() {
        entities_processed_++;
    }

    void record_component_access() {
        components_accessed_++;
    }

    SystemMetrics end_monitoring() {
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time_);
        
        SystemMetrics metrics;
        metrics.system_name = current_system_;
        metrics.execution_time = duration;
        metrics.entities_processed = entities_processed_;
        metrics.components_accessed = components_accessed_;
        
        if (duration.count() > 0) {
            metrics.entities_per_second = static_cast<double>(entities_processed_) / 
                                         (duration.count() / 1000000000.0);
        }

        system_metrics_[current_system_].push_back(metrics);
        return metrics;
    }

    std::vector<SystemMetrics> get_metrics(const std::string& system_name) const {
        auto it = system_metrics_.find(system_name);
        return it != system_metrics_.end() ? it->second : std::vector<SystemMetrics>{};
    }

    double get_average_entities_per_second(const std::string& system_name) const {
        auto metrics = get_metrics(system_name);
        if (metrics.empty()) return 0.0;
        
        double total = 0.0;
        for (const auto& metric : metrics) {
            total += metric.entities_per_second;
        }
        return total / metrics.size();
    }

private:
    std::string current_system_;
    std::chrono::high_resolution_clock::time_point start_time_;
    size_t entities_processed_{0};
    size_t components_accessed_{0};
    std::unordered_map<std::string, std::vector<SystemMetrics>> system_metrics_;
};

// ECS archetype analyzer
class ArchetypeAnalyzer {
public:
    struct ArchetypeInfo {
        size_t component_count;
        size_t entity_count;
        size_t memory_usage;
        std::vector<std::string> component_names;
        double fragmentation_ratio;
    };

    template<typename Registry>
    std::vector<ArchetypeInfo> analyze_archetypes(const Registry& registry) {
        std::vector<ArchetypeInfo> infos;
        
        // This would need to be adapted based on your actual Registry implementation
        // The example assumes access to archetype information
        /*
        for (const auto& archetype : registry.get_archetypes()) {
            ArchetypeInfo info;
            info.component_count = archetype.component_types().size();
            info.entity_count = archetype.entity_count();
            info.memory_usage = archetype.memory_usage();
            info.fragmentation_ratio = calculate_fragmentation(archetype);
            
            for (const auto& type : archetype.component_types()) {
                info.component_names.push_back(type.name());
            }
            
            infos.push_back(info);
        }
        */
        
        return infos;
    }

    double calculate_memory_efficiency(const std::vector<ArchetypeInfo>& infos) {
        if (infos.empty()) return 0.0;
        
        size_t total_memory = 0;
        size_t total_entities = 0;
        
        for (const auto& info : infos) {
            total_memory += info.memory_usage;
            total_entities += info.entity_count;
        }
        
        return total_entities > 0 ? static_cast<double>(total_memory) / total_entities : 0.0;
    }

private:
    template<typename Archetype>
    double calculate_fragmentation(const Archetype& archetype) {
        // Calculate memory fragmentation based on archetype structure
        return 0.0; // Placeholder
    }
};

// ECS query performance tester
class QueryPerformanceTester {
public:
    struct QueryMetrics {
        std::string query_description;
        std::chrono::nanoseconds execution_time;
        size_t entities_matched;
        size_t archetypes_checked;
        double cache_hit_ratio;
    };

    template<typename Query>
    QueryMetrics benchmark_query(Query& query, int iterations = 1000) {
        QueryMetrics metrics;
        metrics.query_description = get_query_description(query);
        
        auto start = std::chrono::high_resolution_clock::now();
        
        for (int i = 0; i < iterations; ++i) {
            auto results = query.execute();
            if (i == 0) {
                metrics.entities_matched = results.size();
            }
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        metrics.execution_time = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start) / iterations;
        
        return metrics;
    }

    template<typename Registry>
    std::vector<QueryMetrics> benchmark_common_queries(Registry& registry) {
        std::vector<QueryMetrics> results;
        
        // Benchmark common query patterns
        // These would need to be adapted based on your actual Query API
        /*
        auto single_component_query = registry.query<Transform>();
        results.push_back(benchmark_query(single_component_query));
        
        auto multi_component_query = registry.query<Transform, Velocity>();
        results.push_back(benchmark_query(multi_component_query));
        
        auto optional_component_query = registry.query<Transform>().with_optional<Health>();
        results.push_back(benchmark_query(optional_component_query));
        */
        
        return results;
    }

private:
    template<typename Query>
    std::string get_query_description(const Query& query) {
        // Generate human-readable description of query
        return "Query"; // Placeholder
    }
};

// ECS component lifecycle tester
class ComponentLifecycleTester {
public:
    struct LifecycleStats {
        size_t constructions{0};
        size_t destructions{0};
        size_t copies{0};
        size_t moves{0};
        bool has_leaks{false};
    };

    template<typename Component>
    class TestComponent : public Component {
    public:
        template<typename... Args>
        TestComponent(Args&&... args) : Component(std::forward<Args>(args)...) {
            stats_.constructions++;
        }

        TestComponent(const TestComponent& other) : Component(other) {
            stats_.copies++;
        }

        TestComponent(TestComponent&& other) noexcept : Component(std::move(other)) {
            stats_.moves++;
        }

        ~TestComponent() {
            stats_.destructions++;
        }

        static LifecycleStats get_stats() { return stats_; }
        static void reset_stats() { stats_ = LifecycleStats{}; }

    private:
        static inline LifecycleStats stats_;
    };

    template<typename Component>
    LifecycleStats test_component_lifecycle() {
        TestComponent<Component>::reset_stats();
        
        {
            // Test construction
            TestComponent<Component> comp1;
            
            // Test copy
            TestComponent<Component> comp2 = comp1;
            
            // Test move
            TestComponent<Component> comp3 = std::move(comp1);
            
            // Test assignment operations...
        }
        
        auto stats = TestComponent<Component>::get_stats();
        stats.has_leaks = stats.constructions != stats.destructions;
        
        return stats;
    }
};

// ECS system dependency validator
class SystemDependencyValidator {
public:
    struct DependencyGraph {
        std::unordered_map<std::string, std::vector<std::string>> dependencies;
        std::unordered_map<std::string, std::vector<std::string>> dependents;
    };

    template<typename SystemManager>
    bool validate_system_dependencies(const SystemManager& manager) {
        auto graph = build_dependency_graph(manager);
        
        // Check for circular dependencies
        if (has_circular_dependencies(graph)) {
            return false;
        }
        
        // Check for conflicting resource access
        if (has_resource_conflicts(manager)) {
            return false;
        }
        
        return true;
    }

    template<typename SystemManager>
    DependencyGraph build_dependency_graph(const SystemManager& manager) {
        DependencyGraph graph;
        
        // This would need to be adapted based on your actual SystemManager API
        /*
        for (const auto& system : manager.get_systems()) {
            const auto& name = system.name();
            const auto& deps = system.dependencies();
            
            graph.dependencies[name] = deps;
            for (const auto& dep : deps) {
                graph.dependents[dep].push_back(name);
            }
        }
        */
        
        return graph;
    }

    bool has_circular_dependencies(const DependencyGraph& graph) {
        std::unordered_set<std::string> visited;
        std::unordered_set<std::string> in_stack;
        
        for (const auto& [node, _] : graph.dependencies) {
            if (visited.find(node) == visited.end()) {
                if (has_cycle_dfs(node, graph, visited, in_stack)) {
                    return true;
                }
            }
        }
        
        return false;
    }

private:
    bool has_cycle_dfs(const std::string& node, const DependencyGraph& graph,
                      std::unordered_set<std::string>& visited,
                      std::unordered_set<std::string>& in_stack) {
        visited.insert(node);
        in_stack.insert(node);
        
        auto it = graph.dependencies.find(node);
        if (it != graph.dependencies.end()) {
            for (const auto& neighbor : it->second) {
                if (in_stack.find(neighbor) != in_stack.end()) {
                    return true; // Back edge found
                }
                if (visited.find(neighbor) == visited.end()) {
                    if (has_cycle_dfs(neighbor, graph, visited, in_stack)) {
                        return true;
                    }
                }
            }
        }
        
        in_stack.erase(node);
        return false;
    }

    template<typename SystemManager>
    bool has_resource_conflicts(const SystemManager& manager) {
        // Check for systems that write to the same resources without proper synchronization
        return false; // Placeholder
    }
};

// ECS test fixture
class ECSTestFixture : public TestFixture {
public:
    void setup() override {
        // Initialize ECS world/registry for testing
        world_ = std::make_unique<ecscope::World>();
        performance_monitor_ = std::make_unique<SystemPerformanceMonitor>();
        archetype_analyzer_ = std::make_unique<ArchetypeAnalyzer>();
        query_tester_ = std::make_unique<QueryPerformanceTester>();
        lifecycle_tester_ = std::make_unique<ComponentLifecycleTester>();
        dependency_validator_ = std::make_unique<SystemDependencyValidator>();
    }

    void teardown() override {
        world_.reset();
        performance_monitor_.reset();
        archetype_analyzer_.reset();
        query_tester_.reset();
        lifecycle_tester_.reset();
        dependency_validator_.reset();
    }

protected:
    std::unique_ptr<ecscope::World> world_;
    std::unique_ptr<SystemPerformanceMonitor> performance_monitor_;
    std::unique_ptr<ArchetypeAnalyzer> archetype_analyzer_;
    std::unique_ptr<QueryPerformanceTester> query_tester_;
    std::unique_ptr<ComponentLifecycleTester> lifecycle_tester_;
    std::unique_ptr<SystemDependencyValidator> dependency_validator_;

    // Helper methods for creating test entities
    template<typename... Components>
    Entity create_test_entity(Components&&... components) {
        return world_->create_entity(std::forward<Components>(components)...);
    }

    void create_test_entities(size_t count) {
        for (size_t i = 0; i < count; ++i) {
            create_test_entity();
        }
    }

    template<typename Component>
    void validate_component() {
        ASSERT_TRUE(ComponentValidator::validate_component_structure<Component>());
        ASSERT_TRUE(ComponentValidator::validate_component_size<Component>());
        ASSERT_TRUE(ComponentValidator::validate_component_alignment<Component>());
    }
};

// Stress test for ECS performance
class ECSStressTest : public BenchmarkTest {
public:
    ECSStressTest(const std::string& name, size_t entity_count, int iterations = 100)
        : BenchmarkTest(name, iterations), entity_count_(entity_count) {}

protected:
    size_t entity_count_;
    std::unique_ptr<ecscope::World> world_;

    void setup() override {
        world_ = std::make_unique<ecscope::World>();
        
        // Create entities for stress testing
        for (size_t i = 0; i < entity_count_; ++i) {
            world_->create_entity();
        }
    }

    void teardown() override {
        world_.reset();
    }
};

// Memory fragmentation test for ECS
class ECSMemoryFragmentationTest : public TestCase {
public:
    ECSMemoryFragmentationTest() : TestCase("ECS Memory Fragmentation", TestCategory::MEMORY) {}

    void run() override {
        auto world = std::make_unique<ecscope::World>();
        
        // Create entities with different component combinations to test fragmentation
        std::vector<Entity> entities;
        
        // Create entities with single components
        for (int i = 0; i < 1000; ++i) {
            entities.push_back(world->create_entity());
        }
        
        // Delete every other entity to create fragmentation
        for (size_t i = 1; i < entities.size(); i += 2) {
            world->destroy_entity(entities[i]);
        }
        
        // Create new entities and check memory usage
        size_t memory_before = get_memory_usage();
        
        for (int i = 0; i < 500; ++i) {
            world->create_entity();
        }
        
        size_t memory_after = get_memory_usage();
        
        // Check that memory growth is reasonable (not excessive fragmentation)
        double memory_growth_ratio = static_cast<double>(memory_after - memory_before) / memory_before;
        ASSERT_LT(memory_growth_ratio, 2.0); // Less than 100% increase
    }

private:
    size_t get_memory_usage() {
        // Platform-specific memory usage implementation
        return memory_tracker_.get_metrics().memory_used;
    }
};

// Concurrent ECS test
class ECSConcurrencyTest : public TestCase {
public:
    ECSConcurrencyTest() : TestCase("ECS Concurrency", TestCategory::MULTITHREADED) {}

    void run() override {
        auto world = std::make_unique<ecscope::World>();
        
        // Create initial entities
        std::vector<Entity> entities;
        for (int i = 0; i < 1000; ++i) {
            entities.push_back(world->create_entity());
        }
        
        // Test concurrent access
        std::vector<std::future<void>> futures;
        
        // Reader threads
        for (int i = 0; i < 4; ++i) {
            futures.emplace_back(std::async(std::launch::async, [&world, &entities]() {
                for (int j = 0; j < 100; ++j) {
                    for (const auto& entity : entities) {
                        // Simulate reading entity data
                        world->has_entity(entity);
                    }
                }
            }));
        }
        
        // Writer thread
        futures.emplace_back(std::async(std::launch::async, [&world]() {
            for (int i = 0; i < 10; ++i) {
                world->create_entity();
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
        }));
        
        // Wait for all threads to complete
        for (auto& future : futures) {
            future.wait();
        }
        
        // Verify world is still in valid state
        ASSERT_TRUE(world->is_valid());
    }
};

} // namespace ecscope::testing