/**
 * @file comprehensive_example.cpp
 * @brief Comprehensive demonstration of the advanced component system
 * 
 * This example showcases all major features of the component system:
 * - Reflection and runtime type information
 * - Property introspection and manipulation
 * - Validation with custom constraints
 * - Serialization (binary, JSON, XML)
 * - Component metadata and documentation
 * - Factory system with blueprints
 * - Advanced features (hot-reload, dependencies, performance monitoring)
 * 
 * This serves as both a demonstration and a practical guide for using
 * the complete component system in real applications.
 */

#include "../../include/ecscope/components/reflection.hpp"
#include "../../include/ecscope/components/properties.hpp"
#include "../../include/ecscope/components/validation.hpp"
#include "../../include/ecscope/components/serialization.hpp"
#include "../../include/ecscope/components/metadata.hpp"
#include "../../include/ecscope/components/factory.hpp"
#include "../../include/ecscope/components/advanced.hpp"

#include <iostream>
#include <memory>
#include <vector>
#include <fstream>
#include <chrono>

using namespace ecscope::components;

// Example Game Components
struct Transform {
    float x{0.0f}, y{0.0f}, z{0.0f};           // Position
    float rotation_x{0.0f}, rotation_y{0.0f}, rotation_z{0.0f}; // Rotation
    float scale_x{1.0f}, scale_y{1.0f}, scale_z{1.0f};          // Scale
    
    bool operator==(const Transform& other) const noexcept {
        const float epsilon = 0.001f;
        return std::abs(x - other.x) < epsilon && std::abs(y - other.y) < epsilon && std::abs(z - other.z) < epsilon &&
               std::abs(rotation_x - other.rotation_x) < epsilon && std::abs(rotation_y - other.rotation_y) < epsilon &&
               std::abs(rotation_z - other.rotation_z) < epsilon && std::abs(scale_x - other.scale_x) < epsilon &&
               std::abs(scale_y - other.scale_y) < epsilon && std::abs(scale_z - other.scale_z) < epsilon;
    }
    
    std::string to_string() const {
        return "Transform(pos: " + std::to_string(x) + ", " + std::to_string(y) + ", " + std::to_string(z) + 
               ", rot: " + std::to_string(rotation_x) + ", " + std::to_string(rotation_y) + ", " + std::to_string(rotation_z) +
               ", scale: " + std::to_string(scale_x) + ", " + std::to_string(scale_y) + ", " + std::to_string(scale_z) + ")";
    }
};

struct Health {
    std::int32_t current{100};
    std::int32_t maximum{100};
    float regeneration_rate{1.0f};
    bool is_invulnerable{false};
    
    bool operator==(const Health& other) const noexcept {
        return current == other.current && maximum == other.maximum && 
               std::abs(regeneration_rate - other.regeneration_rate) < 0.001f &&
               is_invulnerable == other.is_invulnerable;
    }
    
    float get_health_percentage() const { return static_cast<float>(current) / maximum; }
    void heal(std::int32_t amount) { current = std::min(current + amount, maximum); }
    void damage(std::int32_t amount) { if (!is_invulnerable) current = std::max(current - amount, 0); }
};

struct Renderable {
    std::string mesh_path{"default_mesh.obj"};
    std::string texture_path{"default_texture.png"};
    std::string shader_name{"default_shader"};
    float opacity{1.0f};
    bool visible{true};
    std::int32_t render_layer{0};
    
    bool operator==(const Renderable& other) const noexcept {
        return mesh_path == other.mesh_path && texture_path == other.texture_path &&
               shader_name == other.shader_name && std::abs(opacity - other.opacity) < 0.001f &&
               visible == other.visible && render_layer == other.render_layer;
    }
};

struct PlayerController {
    float move_speed{5.0f};
    float jump_height{3.0f};
    bool can_double_jump{false};
    std::int32_t jump_count{0};
    std::int32_t max_jumps{1};
    
    bool operator==(const PlayerController& other) const noexcept {
        return std::abs(move_speed - other.move_speed) < 0.001f &&
               std::abs(jump_height - other.jump_height) < 0.001f &&
               can_double_jump == other.can_double_jump &&
               jump_count == other.jump_count && max_jumps == other.max_jumps;
    }
};

// Hot Reload Observer Example
class GameHotReloadObserver : public HotReloadObserver {
public:
    void on_hot_reload_event(const HotReloadContext& context) override {
        std::cout << "[HOT RELOAD] Event: ";
        switch (context.event_type) {
            case HotReloadEvent::ComponentModified:
                std::cout << "Component '" << context.component_name << "' was modified";
                break;
            case HotReloadEvent::BlueprintModified:
                std::cout << "Blueprint '" << context.blueprint_name << "' was modified";
                break;
            case HotReloadEvent::PropertyAdded:
                std::cout << "Property '" << context.property_name << "' was added";
                break;
            default:
                std::cout << "Unknown event";
                break;
        }
        std::cout << std::endl;
    }
    
    std::string observer_name() const override { return "GameHotReloadObserver"; }
};

// Component Registration and Setup
void setup_reflection_system() {
    std::cout << "=== Setting up Reflection System ===" << std::endl;
    
    auto& registry = ReflectionRegistry::instance();
    
    // Register Transform component
    auto& transform_type = registry.register_type<Transform>("Transform");
    transform_type.add_property(PropertyInfo::create_member("x", &Transform::x)
                               .set_description("X position coordinate")
                               .set_category("position"));
    transform_type.add_property(PropertyInfo::create_member("y", &Transform::y)
                               .set_description("Y position coordinate")
                               .set_category("position"));
    transform_type.add_property(PropertyInfo::create_member("z", &Transform::z)
                               .set_description("Z position coordinate")
                               .set_category("position"));
    transform_type.add_property(PropertyInfo::create_member("rotation_x", &Transform::rotation_x)
                               .set_description("X rotation in radians")
                               .set_category("rotation"));
    transform_type.add_property(PropertyInfo::create_member("rotation_y", &Transform::rotation_y)
                               .set_description("Y rotation in radians")
                               .set_category("rotation"));
    transform_type.add_property(PropertyInfo::create_member("rotation_z", &Transform::rotation_z)
                               .set_description("Z rotation in radians")
                               .set_category("rotation"));
    transform_type.add_property(PropertyInfo::create_member("scale_x", &Transform::scale_x)
                               .set_description("X scale factor")
                               .set_category("scale"));
    transform_type.add_property(PropertyInfo::create_member("scale_y", &Transform::scale_y)
                               .set_description("Y scale factor")
                               .set_category("scale"));
    transform_type.add_property(PropertyInfo::create_member("scale_z", &Transform::scale_z)
                               .set_description("Z scale factor")
                               .set_category("scale"));
    
    // Register Health component
    auto& health_type = registry.register_type<Health>("Health");
    health_type.add_property(PropertyInfo::create_member("current", &Health::current)
                            .set_description("Current health points"));
    health_type.add_property(PropertyInfo::create_member("maximum", &Health::maximum)
                            .set_description("Maximum health points"));
    health_type.add_property(PropertyInfo::create_member("regeneration_rate", &Health::regeneration_rate)
                            .set_description("Health regeneration per second"));
    health_type.add_property(PropertyInfo::create_member("is_invulnerable", &Health::is_invulnerable)
                            .set_description("Whether the entity is invulnerable"));
    
    // Register Renderable component
    auto& renderable_type = registry.register_type<Renderable>("Renderable");
    renderable_type.add_property(PropertyInfo::create_member("mesh_path", &Renderable::mesh_path)
                                .set_description("Path to the mesh file"));
    renderable_type.add_property(PropertyInfo::create_member("texture_path", &Renderable::texture_path)
                                .set_description("Path to the texture file"));
    renderable_type.add_property(PropertyInfo::create_member("shader_name", &Renderable::shader_name)
                                .set_description("Name of the shader to use"));
    renderable_type.add_property(PropertyInfo::create_member("opacity", &Renderable::opacity)
                                .set_description("Opacity level (0.0 - 1.0)"));
    renderable_type.add_property(PropertyInfo::create_member("visible", &Renderable::visible)
                                .set_description("Whether the entity is visible"));
    renderable_type.add_property(PropertyInfo::create_member("render_layer", &Renderable::render_layer)
                                .set_description("Rendering layer for sorting"));
    
    // Register PlayerController component
    auto& controller_type = registry.register_type<PlayerController>("PlayerController");
    controller_type.add_property(PropertyInfo::create_member("move_speed", &PlayerController::move_speed)
                                .set_description("Movement speed in units per second"));
    controller_type.add_property(PropertyInfo::create_member("jump_height", &PlayerController::jump_height)
                                .set_description("Jump height in units"));
    controller_type.add_property(PropertyInfo::create_member("can_double_jump", &PlayerController::can_double_jump)
                                .set_description("Whether double jumping is allowed"));
    
    std::cout << "Registered " << registry.type_count() << " component types" << std::endl;
}

void setup_validation_system() {
    std::cout << "\n=== Setting up Validation System ===" << std::endl;
    
    // Transform validation
    validate_property<Transform>("x").range(-1000.0f, 1000.0f).build();
    validate_property<Transform>("y").range(-1000.0f, 1000.0f).build();
    validate_property<Transform>("z").range(-1000.0f, 1000.0f).build();
    validate_property<Transform>("rotation_x").range(-6.28f, 6.28f).build(); // -2π to 2π
    validate_property<Transform>("rotation_y").range(-6.28f, 6.28f).build();
    validate_property<Transform>("rotation_z").range(-6.28f, 6.28f).build();
    validate_property<Transform>("scale_x").range(0.01f, 100.0f).build();
    validate_property<Transform>("scale_y").range(0.01f, 100.0f).build();
    validate_property<Transform>("scale_z").range(0.01f, 100.0f).build();
    
    // Health validation
    validate_property<Health>("current").range(0, 10000).build();
    validate_property<Health>("maximum").range(1, 10000).build();
    validate_property<Health>("regeneration_rate").range(0.0f, 100.0f).build();
    
    // Custom validation for health consistency
    auto& validation_manager = ValidationManager::instance();
    validation_manager.add_component_rule<Health>(ComponentValidationRule(
        "health_consistency",
        "Current health should not exceed maximum health",
        [](const void* component, const TypeInfo& type_info, ValidationContext context) -> EnhancedValidationResult {
            const auto* health = static_cast<const Health*>(component);
            if (health->current > health->maximum) {
                return EnhancedValidationResult::error(
                    ValidationMessage(ValidationSeverity::Error, "HEALTH_OVERFLOW",
                                    "Current health (" + std::to_string(health->current) + 
                                    ") exceeds maximum (" + std::to_string(health->maximum) + ")"),
                    context
                );
            }
            return EnhancedValidationResult::success(context);
        }
    ));
    
    // Renderable validation
    validate_property<Renderable>("mesh_path").string().min_length(1).max_length(256).build();
    validate_property<Renderable>("texture_path").string().min_length(1).max_length(256).build();
    validate_property<Renderable>("shader_name").string().min_length(1).max_length(64).build();
    validate_property<Renderable>("opacity").range(0.0f, 1.0f).build();
    validate_property<Renderable>("render_layer").range(-100, 100).build();
    
    // PlayerController validation
    validate_property<PlayerController>("move_speed").range(0.0f, 50.0f).build();
    validate_property<PlayerController>("jump_height").range(0.0f, 20.0f).build();
    
    std::cout << "Validation system configured with custom rules" << std::endl;
}

void setup_metadata_system() {
    std::cout << "\n=== Setting up Metadata System ===" << std::endl;
    
    auto& meta_registry = MetadataRegistry::instance();
    
    // Transform metadata
    metadata<Transform>("Transform")
        .description("3D transformation component with position, rotation, and scale")
        .category(ComponentCategory::Transform)
        .complexity(ComponentComplexity::Simple)
        .lifecycle(ComponentLifecycle::Stable)
        .version(1, 2, 0)
        .author("ECScope Team", "team@ecscope.dev")
        .tag("transform")
        .tag("3d")
        .tag("core")
        .example("Basic Transform", 
                "Create a transform at origin with unit scale",
                "Transform transform;\ntransform.x = 10.0f;\ntransform.y = 5.0f;")
        .example("Rotated Transform",
                "Create a rotated transform",
                "Transform transform;\ntransform.rotation_y = 1.57f; // 90 degrees");
    
    // Health metadata
    metadata<Health>("Health")
        .description("Health component with damage and regeneration systems")
        .category(ComponentCategory::Logic)
        .complexity(ComponentComplexity::Moderate)
        .lifecycle(ComponentLifecycle::Stable)
        .version(2, 0, 1)
        .author("ECScope Team", "team@ecscope.dev")
        .tag("health")
        .tag("gameplay")
        .tag("rpg")
        .example("Player Health",
                "Create a player health component",
                "Health health;\nhealth.maximum = 100;\nhealth.current = 100;\nhealth.regeneration_rate = 2.0f;");
    
    // Renderable metadata  
    metadata<Renderable>("Renderable")
        .description("Rendering component with mesh, texture, and shader information")
        .category(ComponentCategory::Rendering)
        .complexity(ComponentComplexity::Moderate)
        .lifecycle(ComponentLifecycle::Stable)
        .version(1, 1, 0)
        .author("ECScope Team", "team@ecscope.dev")
        .tag("rendering")
        .tag("graphics")
        .tag("visual");
    
    // PlayerController metadata
    metadata<PlayerController>("PlayerController")
        .description("Player input controller with movement and jumping")
        .category(ComponentCategory::Input)
        .complexity(ComponentComplexity::Complex)
        .lifecycle(ComponentLifecycle::Stable)
        .version(1, 0, 0)
        .author("ECScope Team", "team@ecscope.dev")
        .tag("player")
        .tag("input")
        .tag("controller");
    
    std::cout << "Registered metadata for " << meta_registry.metadata_count() << " component types" << std::endl;
}

void setup_factory_system() {
    std::cout << "\n=== Setting up Factory System ===" << std::endl;
    
    auto& factory_registry = FactoryRegistry::instance();
    
    // Register factories
    factory_registry.register_typed_factory<Transform>("Transform Factory", "Creates Transform components");
    factory_registry.register_typed_factory<Health>("Health Factory", "Creates Health components");
    factory_registry.register_typed_factory<Renderable>("Renderable Factory", "Creates Renderable components");
    factory_registry.register_typed_factory<PlayerController>("PlayerController Factory", "Creates PlayerController components");
    
    // Create blueprints
    
    // Player blueprints
    blueprint<Transform>("PlayerTransform")
        .description("Default transform for player entities")
        .category("player")
        .tag("player")
        .tag("spawn")
        .property("x", 0.0f)
        .property("y", 0.0f)
        .property("z", 0.0f)
        .property("scale_x", 1.0f)
        .property("scale_y", 1.0f)
        .property("scale_z", 1.0f)
        .register_blueprint();
    
    blueprint<Health>("PlayerHealth")
        .description("Standard player health configuration")
        .category("player")
        .tag("player")
        .tag("stats")
        .property("current", 100)
        .property("maximum", 100)
        .property("regeneration_rate", 2.0f)
        .property("is_invulnerable", false)
        .register_blueprint();
    
    blueprint<Renderable>("PlayerRenderable")
        .description("Standard player rendering setup")
        .category("player")
        .tag("player")
        .tag("visual")
        .property("mesh_path", std::string("models/player.obj"))
        .property("texture_path", std::string("textures/player.png"))
        .property("shader_name", std::string("character_shader"))
        .property("opacity", 1.0f)
        .property("visible", true)
        .property("render_layer", 10)
        .register_blueprint();
    
    blueprint<PlayerController>("StandardPlayer")
        .description("Standard player controller configuration")
        .category("player")
        .tag("player")
        .tag("controller")
        .property("move_speed", 5.0f)
        .property("jump_height", 3.0f)
        .property("can_double_jump", false)
        .property("max_jumps", 1)
        .register_blueprint();
    
    // Enemy blueprints
    auto enemy_transform = blueprint<Transform>("EnemyTransform")
        .description("Base transform for enemy entities")
        .category("enemy")
        .tag("enemy")
        .property("x", 0.0f)
        .property("y", 0.0f)
        .property("z", 0.0f)
        .build();
    
    blueprint<Transform>("BossTransform")
        .description("Large boss enemy transform")
        .category("enemy")
        .tag("enemy")
        .tag("boss")
        .inherits(enemy_transform)
        .property("scale_x", 2.0f)
        .property("scale_y", 2.0f)
        .property("scale_z", 2.0f)
        .register_blueprint();
    
    blueprint<Health>("EnemyHealth")
        .description("Basic enemy health")
        .category("enemy")
        .tag("enemy")
        .property("current", 50)
        .property("maximum", 50)
        .property("regeneration_rate", 0.0f)
        .property("is_invulnerable", false)
        .register_blueprint();
    
    std::cout << "Registered " << factory_registry.factory_count() << " factories and " 
              << factory_registry.blueprint_count() << " blueprints" << std::endl;
}

void setup_advanced_features() {
    std::cout << "\n=== Setting up Advanced Features ===" << std::endl;
    
    // Initialize advanced component system
    auto& advanced_system = AdvancedComponentSystem::instance();
    advanced_system.initialize();
    
    // Set up component dependencies
    auto& dep_manager = ComponentDependencyManager::instance();
    dep_manager.add_dependency<Renderable, Transform>("requires", true, 
                               "Renderable components need Transform for positioning");
    dep_manager.add_dependency<PlayerController, Transform>("requires", true,
                               "PlayerController needs Transform for movement");
    
    // Set up memory layout optimization
    auto& layout_optimizer = MemoryLayoutOptimizer::instance();
    layout_optimizer.register_layout_info<Transform>(0.9);      // Very high access frequency
    layout_optimizer.register_layout_info<Health>(0.6);        // Medium-high access frequency
    layout_optimizer.register_layout_info<Renderable>(0.7);    // High access frequency (rendering)
    layout_optimizer.register_layout_info<PlayerController>(0.4); // Medium access frequency
    
    // Set up hot reload system
    auto& hot_reload_manager = HotReloadManager::instance();
    hot_reload_manager.enable_hot_reload();
    
    auto observer = std::make_shared<GameHotReloadObserver>();
    auto observer_handle = hot_reload_manager.register_observer(observer);
    
    // Set up lifecycle hooks
    advanced_system.lifecycle_hooks().register_post_create_hook("debug_create",
        [](void* component, std::type_index type) {
            std::cout << "[LIFECYCLE] Created component of type: " << type.name() << std::endl;
        });
    
    advanced_system.lifecycle_hooks().register_pre_destroy_hook("debug_destroy",
        [](void* component, std::type_index type) -> bool {
            std::cout << "[LIFECYCLE] Destroying component of type: " << type.name() << std::endl;
            return true; // Allow destruction
        });
    
    std::cout << "Advanced features initialized successfully" << std::endl;
}

void demonstrate_reflection_features() {
    std::cout << "\n=== Demonstrating Reflection Features ===" << std::endl;
    
    Transform transform{10.0f, 20.0f, 30.0f, 0.0f, 1.57f, 0.0f, 1.0f, 1.0f, 1.0f};
    
    auto& registry = ReflectionRegistry::instance();
    const auto* type_info = registry.get_type_info<Transform>();
    
    std::cout << "Component: " << type_info->name() << std::endl;
    std::cout << "Properties (" << type_info->property_count() << "):" << std::endl;
    
    // Enumerate all properties
    auto properties = type_info->get_all_properties();
    for (const auto* prop : properties) {
        auto value = prop->get_value(&transform);
        std::cout << "  " << prop->name() << " (" << prop->category() << "): ";
        
        if (const auto* f = value.try_get<float>()) {
            std::cout << *f;
        } else {
            std::cout << "unknown type";
        }
        std::cout << " - " << prop->description() << std::endl;
    }
    
    // Demonstrate property modification through reflection
    std::cout << "\nModifying properties through reflection:" << std::endl;
    TypeAccessor accessor(&transform, type_info);
    
    auto original_y = accessor.get_property("y").get<float>();
    std::cout << "Original Y: " << original_y << std::endl;
    
    auto set_result = accessor.set_property("y", PropertyValue(50.0f));
    if (set_result) {
        std::cout << "New Y: " << transform.y << std::endl;
    } else {
        std::cout << "Failed to set Y: " << set_result.error_message << std::endl;
    }
}

void demonstrate_validation_features() {
    std::cout << "\n=== Demonstrating Validation Features ===" << std::endl;
    
    Health health{80, 100, 2.0f, false};
    auto& validation_manager = ValidationManager::instance();
    
    // Valid component validation
    auto result1 = validation_manager.validate_component(health, ValidationContext::Runtime);
    std::cout << "Valid health component: " << (result1 ? "PASS" : "FAIL") << std::endl;
    if (!result1) {
        for (const auto& msg : result1.messages) {
            std::cout << "  Error: " << msg.message << std::endl;
        }
    }
    
    // Invalid component validation (current > maximum)
    Health invalid_health{150, 100, 2.0f, false};
    auto result2 = validation_manager.validate_component(invalid_health, ValidationContext::Runtime);
    std::cout << "Invalid health component: " << (result2 ? "PASS" : "FAIL") << std::endl;
    if (!result2) {
        for (const auto& msg : result2.messages) {
            std::cout << "  " << (msg.is_error() ? "Error" : "Warning") << ": " << msg.message << std::endl;
        }
    }
    
    // Property-level validation
    auto& prop_system = PropertySystem::instance();
    auto result3 = prop_system.set_property_value(health, "regeneration_rate", PropertyValue(-5.0f));
    std::cout << "Setting negative regeneration rate: " << (result3 ? "PASS" : "FAIL") << std::endl;
    if (!result3) {
        std::cout << "  Error: " << result3.error_message << std::endl;
    }
}

void demonstrate_serialization_features() {
    std::cout << "\n=== Demonstrating Serialization Features ===" << std::endl;
    
    // Create test components
    Transform original_transform{15.0f, 25.0f, 35.0f, 0.5f, 1.0f, 1.5f, 1.2f, 1.1f, 0.9f};
    Health original_health{75, 100, 1.5f, false};
    
    ComponentSerializer serializer;
    auto& serialization_manager = SerializationManager::instance();
    
    // Binary serialization
    std::cout << "\nBinary Serialization:" << std::endl;
    {
        SerializationContext context;
        context.format = SerializationFormat::Binary;
        context.version = 1;
        
        std::vector<std::byte> buffer(1024);
        
        // Serialize Transform
        auto serialize_result = serializer.serialize(original_transform, std::span<std::byte>(buffer), context);
        std::cout << "  Transform serialized: " << serialize_result.bytes_written << " bytes" << std::endl;
        
        // Deserialize Transform
        Transform deserialized_transform{};
        auto deserialize_result = serializer.deserialize(deserialized_transform, 
                                                        std::span<const std::byte>(buffer.data(), serialize_result.bytes_written), 
                                                        context);
        std::cout << "  Transform deserialized: " << (deserialized_transform == original_transform ? "SUCCESS" : "FAILED") << std::endl;
    }
    
    // JSON serialization
    std::cout << "\nJSON Serialization:" << std::endl;
    {
        SerializationContext context;
        context.format = SerializationFormat::JSON;
        context.flags = SerializationFlags::Pretty | SerializationFlags::IncludeTypes;
        
        std::vector<std::byte> buffer(2048);
        
        // Serialize Health
        auto serialize_result = serializer.serialize(original_health, std::span<std::byte>(buffer), context);
        std::cout << "  Health serialized: " << serialize_result.bytes_written << " bytes" << std::endl;
        
        // Show JSON content
        std::string json_content(reinterpret_cast<const char*>(buffer.data()), serialize_result.bytes_written);
        std::cout << "  JSON Content:" << std::endl;
        std::cout << json_content << std::endl;
    }
    
    // XML serialization
    std::cout << "\nXML Serialization:" << std::endl;
    {
        SerializationContext context;
        context.format = SerializationFormat::XML;
        context.flags = SerializationFlags::Pretty | SerializationFlags::IncludeTypes;
        
        std::vector<std::byte> buffer(2048);
        
        // Serialize Renderable
        Renderable renderable{"player.obj", "player.png", "basic_shader", 0.8f, true, 5};
        auto serialize_result = serializer.serialize(renderable, std::span<std::byte>(buffer), context);
        std::cout << "  Renderable serialized: " << serialize_result.bytes_written << " bytes" << std::endl;
        
        // Show XML content (first 200 characters)
        std::string xml_content(reinterpret_cast<const char*>(buffer.data()), 
                               std::min(serialize_result.bytes_written, std::size_t(200)));
        std::cout << "  XML Content (preview):" << std::endl;
        std::cout << xml_content << "..." << std::endl;
    }
}

void demonstrate_factory_features() {
    std::cout << "\n=== Demonstrating Factory Features ===" << std::endl;
    
    using namespace factory;
    
    // Basic factory creation
    std::cout << "\nBasic Factory Creation:" << std::endl;
    auto* transform = create<Transform>();
    std::cout << "  Created Transform: " << transform->to_string() << std::endl;
    destroy(transform);
    
    // Blueprint-based creation
    std::cout << "\nBlueprint-based Creation:" << std::endl;
    auto [player_health, health_result] = create_with_blueprint<Health>("PlayerHealth");
    if (health_result) {
        std::cout << "  Created Player Health: " << player_health->current << "/" << player_health->maximum 
                  << " (regen: " << player_health->regeneration_rate << "/s)" << std::endl;
        destroy(player_health);
    } else {
        std::cout << "  Failed to create Player Health: " << health_result.error_message << std::endl;
    }
    
    // Parameterized creation
    std::cout << "\nParameterized Creation:" << std::endl;
    std::unordered_map<std::string, PropertyValue> params;
    params["x"] = PropertyValue(100.0f);
    params["y"] = PropertyValue(200.0f);
    params["z"] = PropertyValue(300.0f);
    params["scale_x"] = PropertyValue(2.0f);
    params["scale_y"] = PropertyValue(2.0f);
    params["scale_z"] = PropertyValue(2.0f);
    
    auto [custom_transform, transform_result] = create_with_params<Transform>(params);
    if (transform_result) {
        std::cout << "  Created Custom Transform: " << custom_transform->to_string() << std::endl;
        destroy(custom_transform);
    } else {
        std::cout << "  Failed to create Custom Transform: " << transform_result.error_message << std::endl;
    }
    
    // Blueprint inheritance demonstration
    std::cout << "\nBlueprint Inheritance:" << std::endl;
    auto& factory_registry = FactoryRegistry::instance();
    auto boss_blueprint = factory_registry.get_blueprint("BossTransform");
    if (boss_blueprint) {
        auto effective_props = boss_blueprint->get_effective_properties();
        std::cout << "  Boss Transform effective properties:" << std::endl;
        for (const auto& [name, value] : effective_props) {
            std::cout << "    " << name << ": ";
            if (const auto* f = value.try_get<float>()) {
                std::cout << *f;
            }
            std::cout << std::endl;
        }
    }
}

void demonstrate_advanced_features() {
    std::cout << "\n=== Demonstrating Advanced Features ===" << std::endl;
    
    // Dependency resolution
    std::cout << "\nDependency Resolution:" << std::endl;
    auto& dep_manager = ComponentDependencyManager::instance();
    std::vector<std::type_index> types = {typeid(PlayerController), typeid(Renderable), typeid(Transform)};
    auto resolved_order = dep_manager.resolve_creation_order(types);
    
    std::cout << "  Creation order (respecting dependencies):" << std::endl;
    for (const auto& type : resolved_order) {
        if (type == typeid(Transform)) std::cout << "    1. Transform" << std::endl;
        else if (type == typeid(Renderable)) std::cout << "    2. Renderable" << std::endl;
        else if (type == typeid(PlayerController)) std::cout << "    3. PlayerController" << std::endl;
    }
    
    // Memory layout optimization
    std::cout << "\nMemory Layout Optimization:" << std::endl;
    auto& layout_optimizer = MemoryLayoutOptimizer::instance();
    auto optimized_layout = layout_optimizer.optimize_layout(types);
    
    std::cout << "  Optimized layout (by cache efficiency):" << std::endl;
    for (size_t i = 0; i < optimized_layout.size(); ++i) {
        const auto& type = optimized_layout[i];
        const auto* layout_info = layout_optimizer.get_layout_info(type);
        std::cout << "    " << (i + 1) << ". ";
        
        if (type == typeid(Transform)) std::cout << "Transform";
        else if (type == typeid(Renderable)) std::cout << "Renderable";
        else if (type == typeid(PlayerController)) std::cout << "PlayerController";
        else if (type == typeid(Health)) std::cout << "Health";
        
        if (layout_info) {
            std::cout << " (cache score: " << layout_info->cache_efficiency_score() << ")";
        }
        std::cout << std::endl;
    }
    
    // Performance monitoring
    std::cout << "\nPerformance Monitoring:" << std::endl;
    auto& perf_monitor = ComponentPerformanceMonitor::instance();
    
    // Simulate some operations with timing
    {
        ECSCOPE_MEASURE_CREATION(Transform);
        auto* t = factory::create<Transform>();
        std::this_thread::sleep_for(std::chrono::microseconds(100)); // Simulate work
        factory::destroy(t);
    }
    
    {
        ECSCOPE_MEASURE_CREATION(Health);
        auto* h = factory::create<Health>();
        std::this_thread::sleep_for(std::chrono::microseconds(150)); // Simulate work
        factory::destroy(h);
    }
    
    // Show performance metrics
    const auto& transform_metrics = perf_monitor.get_metrics(typeid(Transform));
    const auto& health_metrics = perf_monitor.get_metrics(typeid(Health));
    
    std::cout << "  Transform - Created: " << transform_metrics.creation_count.load() 
              << ", Avg time: " << transform_metrics.average_creation_time_ns() << "ns" << std::endl;
    std::cout << "  Health - Created: " << health_metrics.creation_count.load()
              << ", Avg time: " << health_metrics.average_creation_time_ns() << "ns" << std::endl;
    
    // Hot reload simulation
    std::cout << "\nHot Reload Simulation:" << std::endl;
    auto& hot_reload_manager = HotReloadManager::instance();
    HotReloadContext context(HotReloadEvent::ComponentModified, "Transform");
    context.metadata["reason"] = "Property validation rules updated";
    hot_reload_manager.trigger_hot_reload_event(context);
}

void demonstrate_metadata_features() {
    std::cout << "\n=== Demonstrating Metadata Features ===" << std::endl;
    
    auto& meta_registry = MetadataRegistry::instance();
    
    // Show component metadata
    const auto* transform_meta = meta_registry.get_metadata<Transform>();
    if (transform_meta) {
        std::cout << "\nTransform Metadata:" << std::endl;
        std::cout << "  Description: " << transform_meta->description() << std::endl;
        std::cout << "  Version: " << transform_meta->version().to_string() << std::endl;
        std::cout << "  Author: " << transform_meta->author() << std::endl;
        std::cout << "  Complexity: " << static_cast<int>(transform_meta->complexity()) << std::endl;
        std::cout << "  Tags: ";
        for (const auto& tag : transform_meta->tags()) {
            std::cout << tag << " ";
        }
        std::cout << std::endl;
        
        std::cout << "  Examples (" << transform_meta->examples().size() << "):" << std::endl;
        for (const auto& example : transform_meta->examples()) {
            std::cout << "    " << example.title << ": " << example.description << std::endl;
        }
    }
    
    // Show components by category
    auto transform_components = meta_registry.get_components_by_category(ComponentCategory::Transform);
    std::cout << "\nTransform Category Components (" << transform_components.size() << "):" << std::endl;
    for (const auto* meta : transform_components) {
        std::cout << "  " << meta->name() << " - " << meta->description() << std::endl;
    }
    
    // Show performance characteristics
    const auto* health_meta = meta_registry.get_metadata<Health>();
    if (health_meta) {
        const auto& perf_info = health_meta->performance_info();
        std::cout << "\nHealth Performance Info:" << std::endl;
        std::cout << "  Memory usage: " << perf_info.memory_usage << " bytes" << std::endl;
        std::cout << "  Cache efficiency: " << perf_info.cache_efficiency_score() << std::endl;
        std::cout << "  Thread safe: " << (perf_info.is_thread_safe ? "Yes" : "No") << std::endl;
    }
}

void run_performance_benchmarks() {
    std::cout << "\n=== Performance Benchmarks ===" << std::endl;
    
    const int iterations = 100000;
    
    // Property access benchmark
    Transform transform{1.0f, 2.0f, 3.0f};
    auto& registry = ReflectionRegistry::instance();
    const auto* type_info = registry.get_type_info<Transform>();
    const auto* x_prop = type_info->get_property("x");
    
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < iterations; ++i) {
        auto value = x_prop->get_value(&transform);
        x_prop->set_value(&transform, PropertyValue(static_cast<float>(i % 100)));
    }
    auto end = std::chrono::high_resolution_clock::now();
    
    auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
    auto avg_time = duration.count() / (iterations * 2); // 2 operations per iteration
    
    std::cout << "Property Access Benchmark:" << std::endl;
    std::cout << "  " << iterations << " iterations, 2 operations each" << std::endl;
    std::cout << "  Total time: " << duration.count() << " ns" << std::endl;
    std::cout << "  Average per operation: " << avg_time << " ns" << std::endl;
    std::cout << "  Operations per second: " << (1000000000 / avg_time) << std::endl;
    
    // Component creation benchmark
    start = std::chrono::high_resolution_clock::now();
    std::vector<Transform*> transforms;
    transforms.reserve(iterations);
    
    for (int i = 0; i < iterations; ++i) {
        transforms.push_back(factory::create<Transform>());
    }
    
    for (auto* t : transforms) {
        factory::destroy(t);
    }
    
    end = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
    avg_time = duration.count() / (iterations * 2); // create + destroy
    
    std::cout << "\nFactory Creation Benchmark:" << std::endl;
    std::cout << "  " << iterations << " create/destroy cycles" << std::endl;
    std::cout << "  Total time: " << duration.count() << " ns" << std::endl;
    std::cout << "  Average per operation: " << avg_time << " ns" << std::endl;
    std::cout << "  Components per second: " << (1000000000 / avg_time) << std::endl;
}

void print_system_statistics() {
    std::cout << "\n=== System Statistics ===" << std::endl;
    
    auto& reflection_registry = ReflectionRegistry::instance();
    auto& factory_registry = FactoryRegistry::instance();
    auto& metadata_registry = MetadataRegistry::instance();
    auto& property_system = PropertySystem::instance();
    auto& validation_manager = ValidationManager::instance();
    auto& layout_optimizer = MemoryLayoutOptimizer::instance();
    
    std::cout << "Reflection System:" << std::endl;
    std::cout << "  Registered types: " << reflection_registry.type_count() << std::endl;
    
    std::cout << "Factory System:" << std::endl;
    std::cout << "  Registered factories: " << factory_registry.factory_count() << std::endl;
    std::cout << "  Available blueprints: " << factory_registry.blueprint_count() << std::endl;
    
    std::cout << "Metadata System:" << std::endl;
    std::cout << "  Component metadata entries: " << metadata_registry.metadata_count() << std::endl;
    
    std::cout << "Property System:" << std::endl;
    auto prop_stats = property_system.get_statistics();
    std::cout << "  Enhanced properties: " << prop_stats.enhanced_property_count << std::endl;
    std::cout << "  Active observers: " << prop_stats.active_observer_count << std::endl;
    
    std::cout << "Validation System:" << std::endl;
    auto validation_stats = validation_manager.get_statistics();
    std::cout << "  Property pipelines: " << validation_stats.total_property_pipelines << std::endl;
    std::cout << "  Component rules: " << validation_stats.total_component_rules << std::endl;
    std::cout << "  Total validation rules: " << validation_stats.total_validation_rules << std::endl;
    
    std::cout << "Memory Layout Optimizer:" << std::endl;
    auto layout_stats = layout_optimizer.get_statistics();
    std::cout << "  Registered types: " << layout_stats.total_registered_types << std::endl;
    std::cout << "  Cache-friendly types: " << layout_stats.cache_friendly_types << std::endl;
    std::cout << "  Average cache score: " << layout_stats.average_cache_score << std::endl;
}

int main() {
    std::cout << "ECScope Advanced Component System - Comprehensive Example" << std::endl;
    std::cout << "=========================================================" << std::endl;
    
    try {
        // Setup all systems
        setup_reflection_system();
        setup_validation_system();
        setup_metadata_system();
        setup_factory_system();
        setup_advanced_features();
        
        // Demonstrate features
        demonstrate_reflection_features();
        demonstrate_validation_features();
        demonstrate_serialization_features();
        demonstrate_factory_features();
        demonstrate_metadata_features();
        demonstrate_advanced_features();
        
        // Performance analysis
        run_performance_benchmarks();
        
        // System overview
        print_system_statistics();
        
        std::cout << "\n=== Example Completed Successfully ===" << std::endl;
        
        // Cleanup
        auto& advanced_system = AdvancedComponentSystem::instance();
        advanced_system.shutdown();
        
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "\nError: " << e.what() << std::endl;
        return 1;
    }
}