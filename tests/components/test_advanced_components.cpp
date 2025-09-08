#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "../../include/ecscope/components/reflection.hpp"
#include "../../include/ecscope/components/properties.hpp"
#include "../../include/ecscope/components/validation.hpp"
#include "../../include/ecscope/components/serialization.hpp"
#include "../../include/ecscope/components/metadata.hpp"
#include "../../include/ecscope/components/factory.hpp"
#include "../../include/ecscope/components/advanced.hpp"

using namespace ecscope::components;
using namespace testing;

// Test component classes
struct Position {
    float x{0.0f};
    float y{0.0f};
    float z{0.0f};
    
    bool operator==(const Position& other) const noexcept {
        return std::abs(x - other.x) < 0.001f && 
               std::abs(y - other.y) < 0.001f && 
               std::abs(z - other.z) < 0.001f;
    }
    
    std::string to_string() const {
        return "Position(" + std::to_string(x) + ", " + std::to_string(y) + ", " + std::to_string(z) + ")";
    }
};

struct Velocity {
    float dx{0.0f};
    float dy{0.0f};
    float dz{0.0f};
    
    bool operator==(const Velocity& other) const noexcept {
        return std::abs(dx - other.dx) < 0.001f && 
               std::abs(dy - other.dy) < 0.001f && 
               std::abs(dz - other.dz) < 0.001f;
    }
};

struct Health {
    std::int32_t current{100};
    std::int32_t maximum{100};
    
    bool operator==(const Health& other) const noexcept {
        return current == other.current && maximum == other.maximum;
    }
};

struct Name {
    std::string value{"Unnamed"};
    
    bool operator==(const Name& other) const noexcept {
        return value == other.value;
    }
};

// Test Fixtures
class ReflectionSystemTest : public Test {
protected:
    void SetUp() override {
        // Register test components
        auto& registry = ReflectionRegistry::instance();
        
        // Register Position
        auto& pos_type = registry.register_type<Position>("Position");
        pos_type.add_property(PropertyInfo::create_member("x", &Position::x)
                             .set_description("X coordinate"));
        pos_type.add_property(PropertyInfo::create_member("y", &Position::y)
                             .set_description("Y coordinate"));
        pos_type.add_property(PropertyInfo::create_member("z", &Position::z)
                             .set_description("Z coordinate"));
                             
        // Register Health
        auto& health_type = registry.register_type<Health>("Health");
        health_type.add_property(PropertyInfo::create_member("current", &Health::current)
                                .set_description("Current health points"));
        health_type.add_property(PropertyInfo::create_member("maximum", &Health::maximum)
                                .set_description("Maximum health points"));
                                
        // Register Name
        auto& name_type = registry.register_type<Name>("Name");
        name_type.add_property(PropertyInfo::create_member("value", &Name::value)
                              .set_description("Entity name"));
    }
};

class PropertySystemTest : public ReflectionSystemTest {
protected:
    void SetUp() override {
        ReflectionSystemTest::SetUp();
        
        // Set up enhanced properties with constraints
        auto& prop_system = PropertySystem::instance();
        
        // Position validation
        validate_property<Position>("x").range(-1000.0f, 1000.0f).build();
        validate_property<Position>("y").range(-1000.0f, 1000.0f).build();
        validate_property<Position>("z").range(-1000.0f, 1000.0f).build();
        
        // Health validation
        validate_property<Health>("current").range(0, 1000).build();
        validate_property<Health>("maximum").range(1, 1000).build();
        
        // Name validation
        validate_property<Name>("value").string().min_length(1).max_length(100).build();
    }
};

class FactorySystemTest : public PropertySystemTest {
protected:
    void SetUp() override {
        PropertySystemTest::SetUp();
        
        // Register factories
        auto& factory_registry = FactoryRegistry::instance();
        factory_registry.register_typed_factory<Position>("Position Factory", "Creates Position components");
        factory_registry.register_typed_factory<Health>("Health Factory", "Creates Health components");
        factory_registry.register_typed_factory<Name>("Name Factory", "Creates Name components");
        
        // Create blueprints
        auto player_pos = blueprint<Position>("PlayerStartPosition")
            .description("Default starting position for players")
            .category("spawn")
            .tag("player")
            .tag("spawn")
            .property("x", 0.0f)
            .property("y", 0.0f)
            .property("z", 0.0f)
            .register_blueprint();
            
        auto player_health = blueprint<Health>("PlayerHealth")
            .description("Standard player health configuration")
            .category("stats")
            .tag("player")
            .property("current", 100)
            .property("maximum", 100)
            .register_blueprint();
    }
};

// Reflection System Tests
TEST_F(ReflectionSystemTest, TypeRegistrationAndRetrieval) {
    auto& registry = ReflectionRegistry::instance();
    
    // Check type registration
    EXPECT_TRUE(registry.is_registered<Position>());
    EXPECT_TRUE(registry.is_registered<Health>());
    EXPECT_TRUE(registry.is_registered<Name>());
    
    // Get type info
    const auto* pos_type = registry.get_type_info<Position>();
    ASSERT_NE(pos_type, nullptr);
    EXPECT_EQ(pos_type->name(), "Position");
    EXPECT_EQ(pos_type->property_count(), 3);
    
    // Check properties
    const auto* x_prop = pos_type->get_property("x");
    ASSERT_NE(x_prop, nullptr);
    EXPECT_EQ(x_prop->name(), "x");
    EXPECT_EQ(x_prop->description(), "X coordinate");
}

TEST_F(ReflectionSystemTest, PropertyAccessAndManipulation) {
    Position pos{10.0f, 20.0f, 30.0f};
    
    auto& registry = ReflectionRegistry::instance();
    const auto* type_info = registry.get_type_info<Position>();
    ASSERT_NE(type_info, nullptr);
    
    // Get property value
    const auto* x_prop = type_info->get_property("x");
    ASSERT_NE(x_prop, nullptr);
    
    auto x_value = x_prop->get_value(&pos);
    EXPECT_TRUE(x_value.has_value());
    EXPECT_EQ(x_value.get<float>(), 10.0f);
    
    // Set property value
    PropertyValue new_x_value(25.0f);
    auto set_result = x_prop->set_value(&pos, new_x_value);
    EXPECT_TRUE(set_result);
    EXPECT_EQ(pos.x, 25.0f);
}

TEST_F(ReflectionSystemTest, TypeAccessor) {
    Position pos{1.0f, 2.0f, 3.0f};
    
    auto& registry = ReflectionRegistry::instance();
    const auto* type_info = registry.get_type_info<Position>();
    ASSERT_NE(type_info, nullptr);
    
    TypeAccessor accessor(&pos, type_info);
    
    // Get property
    auto y_value = accessor.get_property("y");
    EXPECT_EQ(y_value.get<float>(), 2.0f);
    
    // Set property
    auto result = accessor.set_property("z", PropertyValue(5.0f));
    EXPECT_TRUE(result);
    EXPECT_EQ(pos.z, 5.0f);
    
    // Check property existence
    EXPECT_TRUE(accessor.has_property("x"));
    EXPECT_FALSE(accessor.has_property("nonexistent"));
    
    // Get property names
    auto prop_names = accessor.get_property_names();
    EXPECT_EQ(prop_names.size(), 3);
    EXPECT_THAT(prop_names, UnorderedElementsAre("x", "y", "z"));
}

// Property System Tests
TEST_F(PropertySystemTest, PropertyValidation) {
    Position pos{100.0f, 200.0f, 300.0f};
    
    auto& prop_system = PropertySystem::instance();
    
    // Valid values
    auto result1 = prop_system.set_property_value(pos, "x", PropertyValue(50.0f));
    EXPECT_TRUE(result1);
    EXPECT_EQ(pos.x, 50.0f);
    
    // Invalid values (out of range)
    auto result2 = prop_system.set_property_value(pos, "x", PropertyValue(2000.0f));
    EXPECT_FALSE(result2);
    EXPECT_EQ(pos.x, 50.0f); // Should remain unchanged
}

TEST_F(PropertySystemTest, PropertyConstraints) {
    Health health{50, 100};
    
    auto& prop_system = PropertySystem::instance();
    
    // Valid health values
    auto result1 = prop_system.set_property_value(health, "current", PropertyValue(75));
    EXPECT_TRUE(result1);
    EXPECT_EQ(health.current, 75);
    
    // Invalid current health (negative)
    auto result2 = prop_system.set_property_value(health, "current", PropertyValue(-10));
    EXPECT_FALSE(result2);
    EXPECT_EQ(health.current, 75); // Should remain unchanged
    
    // Invalid maximum health (zero)
    auto result3 = prop_system.set_property_value(health, "maximum", PropertyValue(0));
    EXPECT_FALSE(result3);
    EXPECT_EQ(health.maximum, 100); // Should remain unchanged
}

TEST_F(PropertySystemTest, StringValidation) {
    Name name{"TestName"};
    
    auto& prop_system = PropertySystem::instance();
    
    // Valid name
    auto result1 = prop_system.set_property_value(name, "value", PropertyValue(std::string("NewName")));
    EXPECT_TRUE(result1);
    EXPECT_EQ(name.value, "NewName");
    
    // Invalid name (too long)
    std::string long_name(200, 'x');
    auto result2 = prop_system.set_property_value(name, "value", PropertyValue(long_name));
    EXPECT_FALSE(result2);
    EXPECT_EQ(name.value, "NewName"); // Should remain unchanged
    
    // Invalid name (empty)
    auto result3 = prop_system.set_property_value(name, "value", PropertyValue(std::string("")));
    EXPECT_FALSE(result3);
    EXPECT_EQ(name.value, "NewName"); // Should remain unchanged
}

// Validation System Tests
TEST_F(PropertySystemTest, ValidationRuleCustomization) {
    using namespace validation_rules;
    
    auto& validation_manager = ValidationManager::instance();
    
    // Add custom validation rule for Position
    validation_manager.get_property_pipeline<Position>("x")
        .add_rule(custom("positive_x", "X coordinate must be positive",
                        [](const PropertyValue& value, const PropertyInfo& prop, ValidationContext ctx) -> EnhancedValidationResult {
                            if (const auto* f = value.try_get<float>()) {
                                if (*f < 0.0f) {
                                    return EnhancedValidationResult::error(
                                        ValidationMessage(ValidationSeverity::Error, "NEGATIVE_X", 
                                                        "X coordinate cannot be negative", prop.name()),
                                        ctx
                                    );
                                }
                            }
                            return EnhancedValidationResult::success(ctx);
                        }));
    
    Position pos{-5.0f, 0.0f, 0.0f};
    auto result = validation_manager.validate_component(pos);
    EXPECT_FALSE(result);
    EXPECT_GT(result.error_count(), 0);
}

// Serialization Tests
TEST_F(ReflectionSystemTest, BinarySerialization) {
    Position pos{10.0f, 20.0f, 30.0f};
    
    ComponentSerializer serializer;
    SerializationContext context;
    context.format = SerializationFormat::Binary;
    
    // Serialize
    std::vector<std::byte> buffer(1024);
    auto serialize_result = serializer.serialize(pos, std::span<std::byte>(buffer), context);
    EXPECT_TRUE(serialize_result);
    EXPECT_GT(serialize_result.bytes_written, 0);
    
    // Deserialize
    Position deserialized_pos{0.0f, 0.0f, 0.0f};
    auto deserialize_result = serializer.deserialize(deserialized_pos, 
                                                   std::span<const std::byte>(buffer.data(), serialize_result.bytes_written), 
                                                   context);
    EXPECT_TRUE(deserialize_result);
    EXPECT_EQ(deserialized_pos, pos);
}

TEST_F(ReflectionSystemTest, JSONSerialization) {
    Health health{75, 100};
    
    ComponentSerializer serializer;
    SerializationContext context;
    context.format = SerializationFormat::JSON;
    context.flags = SerializationFlags::Pretty;
    
    // Serialize
    std::vector<std::byte> buffer(1024);
    auto serialize_result = serializer.serialize(health, std::span<std::byte>(buffer), context);
    EXPECT_TRUE(serialize_result);
    EXPECT_GT(serialize_result.bytes_written, 0);
    
    // Check JSON content contains expected fields
    std::string json_str(reinterpret_cast<const char*>(buffer.data()), serialize_result.bytes_written);
    EXPECT_THAT(json_str, HasSubstr("Health"));
    EXPECT_THAT(json_str, HasSubstr("current"));
    EXPECT_THAT(json_str, HasSubstr("maximum"));
}

// Factory System Tests
TEST_F(FactorySystemTest, ComponentCreation) {
    auto& factory_registry = FactoryRegistry::instance();
    
    // Create component using factory
    auto* pos = factory_registry.create_component<Position>();
    ASSERT_NE(pos, nullptr);
    EXPECT_EQ(pos->x, 0.0f);
    EXPECT_EQ(pos->y, 0.0f);
    EXPECT_EQ(pos->z, 0.0f);
    
    delete pos;
}

TEST_F(FactorySystemTest, BlueprintCreation) {
    using namespace factory;
    
    // Create component from blueprint
    auto [pos, result] = create_with_blueprint<Position>("PlayerStartPosition");
    EXPECT_TRUE(result);
    ASSERT_NE(pos, nullptr);
    EXPECT_EQ(pos->x, 0.0f);
    EXPECT_EQ(pos->y, 0.0f);
    EXPECT_EQ(pos->z, 0.0f);
    
    delete pos;
}

TEST_F(FactorySystemTest, ParameterizedCreation) {
    using namespace factory;
    
    std::unordered_map<std::string, PropertyValue> params;
    params["current"] = PropertyValue(80);
    params["maximum"] = PropertyValue(120);
    
    auto [health, result] = create_with_params<Health>(params);
    EXPECT_TRUE(result);
    ASSERT_NE(health, nullptr);
    EXPECT_EQ(health->current, 80);
    EXPECT_EQ(health->maximum, 120);
    
    delete health;
}

TEST_F(FactorySystemTest, BlueprintInheritance) {
    auto& factory_registry = FactoryRegistry::instance();
    
    // Get base blueprint
    auto base_blueprint = factory_registry.get_blueprint("PlayerStartPosition");
    ASSERT_NE(base_blueprint, nullptr);
    
    // Create derived blueprint
    auto derived = blueprint<Position>("BossStartPosition")
        .description("Starting position for boss entities")
        .category("spawn")
        .tag("boss")
        .inherits(base_blueprint)
        .property("z", 10.0f) // Override z coordinate
        .build();
    
    // Check effective properties
    auto effective_props = derived->get_effective_properties();
    EXPECT_EQ(effective_props.size(), 3);
    
    auto z_value = derived->get_effective_property("z");
    EXPECT_EQ(z_value.get<float>(), 10.0f);
    
    auto x_value = derived->get_effective_property("x");
    EXPECT_EQ(x_value.get<float>(), 0.0f); // Inherited from parent
}

// Metadata System Tests
TEST_F(ReflectionSystemTest, ComponentMetadata) {
    auto& meta_registry = MetadataRegistry::instance();
    
    // Register metadata
    auto& pos_meta = meta_registry.register_metadata<Position>("Position")
        .description("3D position component")
        .category(ComponentCategory::Transform)
        .complexity(ComponentComplexity::Simple)
        .lifecycle(ComponentLifecycle::Stable)
        .version(1, 0, 0)
        .author("Test Author", "test@example.com")
        .tag("math")
        .tag("transform");
    
    // Add usage example
    pos_meta.add_example(UsageExample(
        "Basic Usage",
        "Create and manipulate a position component",
        "Position pos{10.0f, 20.0f, 30.0f};"
    ));
    
    // Retrieve and verify metadata
    const auto* retrieved_meta = meta_registry.get_metadata<Position>();
    ASSERT_NE(retrieved_meta, nullptr);
    EXPECT_EQ(retrieved_meta->name(), "Position");
    EXPECT_EQ(retrieved_meta->description(), "3D position component");
    EXPECT_EQ(retrieved_meta->category(), ComponentCategory::Transform);
    EXPECT_TRUE(retrieved_meta->has_tag("math"));
    EXPECT_TRUE(retrieved_meta->has_tag("transform"));
    EXPECT_EQ(retrieved_meta->examples().size(), 1);
}

// Advanced Features Tests
TEST_F(ReflectionSystemTest, DependencyManagement) {
    auto& dep_manager = ComponentDependencyManager::instance();
    
    // Add dependencies
    dep_manager.add_dependency<Velocity, Position>("requires", true, "Velocity needs position to work");
    
    // Check dependencies
    EXPECT_TRUE(dep_manager.has_dependency(typeid(Velocity), typeid(Position)));
    
    auto deps = dep_manager.get_dependencies(typeid(Velocity));
    EXPECT_EQ(deps.size(), 1);
    EXPECT_EQ(deps[0].relationship, "requires");
    EXPECT_TRUE(deps[0].is_critical);
    
    // Test creation order resolution
    std::vector<std::type_index> types = {typeid(Velocity), typeid(Position)};
    auto ordered = dep_manager.resolve_creation_order(types);
    
    // Position should come before Velocity due to dependency
    auto pos_it = std::find(ordered.begin(), ordered.end(), typeid(Position));
    auto vel_it = std::find(ordered.begin(), ordered.end(), typeid(Velocity));
    EXPECT_LT(std::distance(ordered.begin(), pos_it), std::distance(ordered.begin(), vel_it));
}

TEST_F(ReflectionSystemTest, MemoryLayoutOptimization) {
    auto& layout_optimizer = MemoryLayoutOptimizer::instance();
    
    // Register layout info
    layout_optimizer.register_layout_info<Position>(0.8); // High access frequency
    layout_optimizer.register_layout_info<Health>(0.5);   // Medium access frequency
    layout_optimizer.register_layout_info<Name>(0.2);     // Low access frequency
    
    // Test optimization
    std::vector<std::type_index> types = {typeid(Name), typeid(Health), typeid(Position)};
    auto optimized = layout_optimizer.optimize_layout(types);
    
    // Position should come first due to higher access frequency
    EXPECT_EQ(optimized[0], typeid(Position));
    
    // Get statistics
    auto stats = layout_optimizer.get_statistics();
    EXPECT_EQ(stats.total_registered_types, 3);
    EXPECT_GT(stats.average_cache_score, 0.0);
}

class MockHotReloadObserver : public HotReloadObserver {
public:
    MOCK_METHOD(void, on_hot_reload_event, (const HotReloadContext& context), (override));
    MOCK_METHOD(std::string, observer_name, (), (const, override));
};

TEST_F(ReflectionSystemTest, HotReloadSystem) {
    auto& hot_reload_manager = HotReloadManager::instance();
    
    // Create mock observer
    auto observer = std::make_shared<MockHotReloadObserver>();
    EXPECT_CALL(*observer, observer_name()).WillRepeatedly(Return("TestObserver"));
    EXPECT_CALL(*observer, on_hot_reload_event(_)).Times(1);
    
    // Register observer
    auto handle = hot_reload_manager.register_observer(observer);
    
    // Trigger event
    HotReloadContext context(HotReloadEvent::ComponentModified, "TestComponent");
    hot_reload_manager.trigger_hot_reload_event(context);
    
    // Unregister observer
    hot_reload_manager.unregister_observer(handle);
}

TEST_F(ReflectionSystemTest, PerformanceMonitoring) {
    auto& perf_monitor = ComponentPerformanceMonitor::instance();
    
    // Record some metrics
    perf_monitor.record_creation_time(typeid(Position), std::chrono::nanoseconds(1000));
    perf_monitor.record_creation_time(typeid(Position), std::chrono::nanoseconds(2000));
    perf_monitor.record_property_access_time(typeid(Position), std::chrono::nanoseconds(100));
    
    // Check metrics
    const auto& metrics = perf_monitor.get_metrics(typeid(Position));
    EXPECT_EQ(metrics.creation_count.load(), 2);
    EXPECT_EQ(metrics.property_access_count.load(), 1);
    EXPECT_EQ(metrics.average_creation_time_ns(), 1500.0); // (1000 + 2000) / 2
    EXPECT_EQ(metrics.average_property_access_time_ns(), 100.0);
}

// Integration Tests
TEST_F(FactorySystemTest, FullSystemIntegration) {
    using namespace factory;
    
    // Create component with validation
    std::unordered_map<std::string, PropertyValue> params;
    params["x"] = PropertyValue(10.0f);
    params["y"] = PropertyValue(20.0f);
    params["z"] = PropertyValue(30.0f);
    
    auto [pos, result] = create_with_params<Position>(params);
    EXPECT_TRUE(result);
    ASSERT_NE(pos, nullptr);
    
    // Serialize the component
    ComponentSerializer serializer;
    SerializationContext context;
    context.format = SerializationFormat::Binary;
    
    std::vector<std::byte> buffer(1024);
    auto serialize_result = serializer.serialize(*pos, std::span<std::byte>(buffer), context);
    EXPECT_TRUE(serialize_result);
    
    // Create new component and deserialize
    auto* new_pos = create<Position>();
    ASSERT_NE(new_pos, nullptr);
    
    auto deserialize_result = serializer.deserialize(*new_pos,
                                                   std::span<const std::byte>(buffer.data(), serialize_result.bytes_written),
                                                   context);
    EXPECT_TRUE(deserialize_result);
    EXPECT_EQ(*new_pos, *pos);
    
    // Clean up
    destroy(pos);
    destroy(new_pos);
}

TEST_F(FactorySystemTest, AdvancedSystemInitialization) {
    auto& advanced_system = AdvancedComponentSystem::instance();
    
    // Initialize advanced features
    advanced_system.initialize();
    EXPECT_TRUE(advanced_system.is_initialized());
    
    // Set up lifecycle hooks
    bool create_hook_called = false;
    advanced_system.lifecycle_hooks().register_post_create_hook("test_hook",
        [&create_hook_called](void* component, std::type_index type) {
            create_hook_called = true;
        });
    
    // Create component (should trigger hook)
    auto* pos = factory::create<Position>();
    EXPECT_TRUE(create_hook_called);
    
    // Clean up
    factory::destroy(pos);
    advanced_system.shutdown();
    EXPECT_FALSE(advanced_system.is_initialized());
}

// Performance Tests
TEST_F(ReflectionSystemTest, PropertyAccessPerformance) {
    Position pos{1.0f, 2.0f, 3.0f};
    
    auto& registry = ReflectionRegistry::instance();
    const auto* type_info = registry.get_type_info<Position>();
    const auto* x_prop = type_info->get_property("x");
    
    const int iterations = 10000;
    auto start = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < iterations; ++i) {
        auto value = x_prop->get_value(&pos);
        x_prop->set_value(&pos, PropertyValue(static_cast<float>(i)));
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
    auto avg_time = duration.count() / (iterations * 2); // 2 operations per iteration
    
    // Property access should be reasonably fast (under 1000ns on average)
    EXPECT_LT(avg_time, 1000);
    
    std::cout << "Average property access time: " << avg_time << " ns" << std::endl;
}

// Main test runner
int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}