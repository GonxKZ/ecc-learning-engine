#include <ecscope/testing/test_framework.hpp>
#include <ecscope/testing/ecs_testing.hpp>
#include <ecscope/components.hpp>
#include <ecscope/math.hpp>

using namespace ecscope::testing;

// Test basic component structure validation
class ComponentStructureTest : public TestCase {
public:
    ComponentStructureTest() : TestCase("Component Structure Validation", TestCategory::UNIT) {}

    void run() override {
        // Test Transform component
        ASSERT_TRUE(ComponentValidator::validate_component_structure<Transform>());
        ASSERT_TRUE(ComponentValidator::validate_component_size<Transform>());
        ASSERT_TRUE(ComponentValidator::validate_component_alignment<Transform>());

        // Test that Transform is trivially copyable
        static_assert(std::is_trivially_copyable_v<Transform>);
        static_assert(std::is_standard_layout_v<Transform>);
        
        // Verify size constraints
        ASSERT_LT(sizeof(Transform), 256); // Should be reasonably small
    }
};

// Test component lifecycle
class ComponentLifecycleTest : public ECSTestFixture {
public:
    ComponentLifecycleTest() : ECSTestFixture() {
        context_.name = "Component Lifecycle Test";
        context_.category = TestCategory::UNIT;
    }

    void run() override {
        // Test component creation and destruction
        validate_component<Transform>();
        
        // Test component with lifecycle tracking
        auto stats = lifecycle_tester_->test_component_lifecycle<Transform>();
        
        ASSERT_FALSE(stats.has_leaks);
        ASSERT_GT(stats.constructions, 0);
        ASSERT_EQ(stats.constructions, stats.destructions);
    }
};

// Test component data integrity
class ComponentDataIntegrityTest : public TestCase {
public:
    ComponentDataIntegrityTest() : TestCase("Component Data Integrity", TestCategory::UNIT) {}

    void run() override {
        Transform transform;
        transform.position = Vector3{1.0f, 2.0f, 3.0f};
        transform.rotation = Quaternion{0.0f, 0.0f, 0.0f, 1.0f};
        transform.scale = Vector3{1.0f, 1.0f, 1.0f};

        // Test data preservation
        ASSERT_EQ(transform.position.x, 1.0f);
        ASSERT_EQ(transform.position.y, 2.0f);
        ASSERT_EQ(transform.position.z, 3.0f);

        // Test copy semantics
        Transform copy = transform;
        ASSERT_EQ(copy.position.x, transform.position.x);
        ASSERT_EQ(copy.position.y, transform.position.y);
        ASSERT_EQ(copy.position.z, transform.position.z);

        // Test move semantics
        Transform moved = std::move(copy);
        ASSERT_EQ(moved.position.x, transform.position.x);
    }
};

// Parameterized test for different component types
class ComponentTypeTest : public ParameterizedTest<std::string> {
public:
    ComponentTypeTest() : ParameterizedTest("Component Type Test", {
        "Transform", "RigidBody", "Mesh", "Material", "Light"
    }) {
        context_.category = TestCategory::UNIT;
    }

    void run_with_parameter(const std::string& component_type, size_t index) override {
        // This would test different component types
        // In a real implementation, you'd have a registry of component types
        ASSERT_FALSE(component_type.empty());
        
        // Test component type name is valid
        ASSERT_GT(component_type.length(), 0);
        ASSERT_LT(component_type.length(), 50);
    }
};

// Register tests
REGISTER_TEST(ComponentStructureTest);
REGISTER_TEST(ComponentLifecycleTest);
REGISTER_TEST(ComponentDataIntegrityTest);
REGISTER_TEST(ComponentTypeTest);