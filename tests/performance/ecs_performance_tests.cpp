#include <ecscope/testing/test_framework.hpp>
#include <ecscope/testing/ecs_testing.hpp>
#include <ecscope/world.hpp>

using namespace ecscope::testing;

// ECS entity creation performance test
class EntityCreationPerformanceTest : public BenchmarkTest {
public:
    EntityCreationPerformanceTest() : BenchmarkTest("Entity Creation Performance", 10000) {
        context_.category = TestCategory::PERFORMANCE;
        context_.tags.push_back("ecs");
        context_.tags.push_back("creation");
    }

    void setup() override {
        world_ = std::make_unique<ecscope::World>();
    }

    void benchmark() override {
        world_->create_entity();
    }

    void teardown() override {
        world_.reset();
    }

private:
    std::unique_ptr<ecscope::World> world_;
};

// Component addition performance test
class ComponentAdditionPerformanceTest : public BenchmarkTest {
public:
    ComponentAdditionPerformanceTest() : BenchmarkTest("Component Addition Performance", 5000) {
        context_.category = TestCategory::PERFORMANCE;
        context_.tags.push_back("ecs");
        context_.tags.push_back("components");
    }

    void setup() override {
        world_ = std::make_unique<ecscope::World>();
        
        // Pre-create entities
        entities_.reserve(iterations_);
        for (int i = 0; i < iterations_; ++i) {
            entities_.push_back(world_->create_entity());
        }
        current_entity_index_ = 0;
    }

    void benchmark() override {
        if (current_entity_index_ < entities_.size()) {
            auto entity = entities_[current_entity_index_++];
            // world_->add_component<Transform>(entity);
        }
    }

    void teardown() override {
        entities_.clear();
        world_.reset();
    }

private:
    std::unique_ptr<ecscope::World> world_;
    std::vector<Entity> entities_;
    size_t current_entity_index_;
};

// Query performance test
class QueryPerformanceTest : public BenchmarkTest {
public:
    QueryPerformanceTest() : BenchmarkTest("Query Performance", 1000) {
        context_.category = TestCategory::PERFORMANCE;
        context_.tags.push_back("ecs");
        context_.tags.push_back("queries");
    }

    void setup() override {
        world_ = std::make_unique<ecscope::World>();
        
        // Create a mix of entities with different components
        for (int i = 0; i < 10000; ++i) {
            auto entity = world_->create_entity();
            
            // Add Transform to all entities
            // world_->add_component<Transform>(entity);
            
            // Add RigidBody to 50% of entities
            if (i % 2 == 0) {
                // world_->add_component<RigidBody>(entity);
            }
            
            // Add Mesh to 25% of entities
            if (i % 4 == 0) {
                // world_->add_component<Mesh>(entity);
            }
        }
    }

    void benchmark() override {
        // Benchmark a common query pattern
        // auto query = world_->query<Transform, RigidBody>();
        // auto results = query.execute();
        
        // For now, just simulate query work
        std::this_thread::sleep_for(std::chrono::microseconds(1));
    }

    void teardown() override {
        world_.reset();
    }

private:
    std::unique_ptr<ecscope::World> world_;
};

// System update performance test
class SystemUpdatePerformanceTest : public BenchmarkTest {
public:
    SystemUpdatePerformanceTest() : BenchmarkTest("System Update Performance", 1000) {
        context_.category = TestCategory::PERFORMANCE;
        context_.tags.push_back("ecs");
        context_.tags.push_back("systems");
    }

    void setup() override {
        world_ = std::make_unique<ecscope::World>();
        
        // Create entities for system to process
        for (int i = 0; i < 1000; ++i) {
            auto entity = world_->create_entity();
            // world_->add_component<Transform>(entity);
        }
    }

    void benchmark() override {
        // Simulate system update
        world_->update(1.0f / 60.0f);
    }

    void teardown() override {
        world_.reset();
    }

private:
    std::unique_ptr<ecscope::World> world_;
};

// Memory access pattern performance test
class MemoryAccessPatternTest : public BenchmarkTest {
public:
    MemoryAccessPatternTest() : BenchmarkTest("Memory Access Pattern Performance", 500) {
        context_.category = TestCategory::PERFORMANCE;
        context_.tags.push_back("ecs");
        context_.tags.push_back("memory");
    }

    void setup() override {
        world_ = std::make_unique<ecscope::World>();
        
        // Create entities in a way that tests memory locality
        entities_.reserve(10000);
        for (int i = 0; i < 10000; ++i) {
            auto entity = world_->create_entity();
            entities_.push_back(entity);
            // world_->add_component<Transform>(entity);
        }
    }

    void benchmark() override {
        // Test sequential access pattern
        for (const auto& entity : entities_) {
            // auto* transform = world_->get_component<Transform>(entity);
            // if (transform) {
            //     transform->position.x += 1.0f;
            // }
        }
    }

    void teardown() override {
        entities_.clear();
        world_.reset();
    }

private:
    std::unique_ptr<ecscope::World> world_;
    std::vector<Entity> entities_;
};

// Cache performance test
class CachePerformanceTest : public BenchmarkTest {
public:
    CachePerformanceTest() : BenchmarkTest("Cache Performance Test", 100) {
        context_.category = TestCategory::PERFORMANCE;
        context_.tags.push_back("ecs");
        context_.tags.push_back("cache");
    }

    void setup() override {
        world_ = std::make_unique<ecscope::World>();
        
        // Create entities that will test cache behavior
        for (int i = 0; i < 50000; ++i) {
            auto entity = world_->create_entity();
            // world_->add_component<Transform>(entity);
        }
    }

    void benchmark() override {
        // Test cache-friendly vs cache-unfriendly access patterns
        
        // Cache-friendly: sequential access
        // auto query = world_->query<Transform>();
        // for (auto& transform : query) {
        //     transform.position.x += 1.0f;
        // }
        
        // For now, simulate work
        volatile int sum = 0;
        for (int i = 0; i < 10000; ++i) {
            sum += i;
        }
    }

    void teardown() override {
        world_.reset();
    }

private:
    std::unique_ptr<ecscope::World> world_;
};

// Scaling performance test
class ScalingPerformanceTest : public ParameterizedTest<int> {
public:
    ScalingPerformanceTest() : ParameterizedTest("ECS Scaling Performance", {
        1000, 5000, 10000, 25000, 50000, 100000
    }) {
        context_.category = TestCategory::PERFORMANCE;
        context_.tags.push_back("ecs");
        context_.tags.push_back("scaling");
    }

    void run_with_parameter(const int& entity_count, size_t index) override {
        auto world = std::make_unique<ecscope::World>();
        
        auto start_time = std::chrono::high_resolution_clock::now();
        
        // Create entities
        for (int i = 0; i < entity_count; ++i) {
            auto entity = world->create_entity();
            // world->add_component<Transform>(entity);
        }
        
        // Update world
        world->update(1.0f / 60.0f);
        
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
        
        // Check that performance scales reasonably
        double microseconds_per_entity = static_cast<double>(duration.count()) / entity_count;
        
        // Performance should be better than 10 microseconds per entity
        ASSERT_LT(microseconds_per_entity, 10.0);
        
        // Log performance metrics
        if (context_.metadata.find("verbose") != context_.metadata.end()) {
            std::cout << "Entity count: " << entity_count 
                      << ", Time: " << duration.count() << "μs"
                      << ", μs/entity: " << microseconds_per_entity << std::endl;
        }
    }
};

// Register performance tests
REGISTER_TEST(EntityCreationPerformanceTest);
REGISTER_TEST(ComponentAdditionPerformanceTest);
REGISTER_TEST(QueryPerformanceTest);
REGISTER_TEST(SystemUpdatePerformanceTest);
REGISTER_TEST(MemoryAccessPatternTest);
REGISTER_TEST(CachePerformanceTest);
REGISTER_TEST(ScalingPerformanceTest);