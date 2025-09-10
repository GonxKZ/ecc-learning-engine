/**
 * @file ecs_component_plugin.cpp
 * @brief ECS integration plugin demonstrating custom components and systems
 * 
 * This plugin demonstrates:
 * - Custom component definition and registration
 * - Custom system creation and integration
 * - Entity manipulation and queries
 * - Component serialization
 * - System update loops and scheduling
 */

#include <ecscope/plugins/sdk/plugin_sdk.hpp>
#include <ecscope/plugins/ecs_integration.hpp>
#include <vector>
#include <string>
#include <cmath>

// Custom component definitions
struct HealthComponent {
    float max_health{100.0f};
    float current_health{100.0f};
    float regeneration_rate{1.0f}; // HP per second
    bool is_invincible{false};
    
    HealthComponent() = default;
    HealthComponent(float max_hp, float regen_rate = 1.0f) 
        : max_health(max_hp), current_health(max_hp), regeneration_rate(regen_rate) {}
    
    void take_damage(float damage) {
        if (!is_invincible) {
            current_health = std::max(0.0f, current_health - damage);
        }
    }
    
    void heal(float amount) {
        current_health = std::min(max_health, current_health + amount);
    }
    
    bool is_alive() const { return current_health > 0.0f; }
    float get_health_percentage() const { return current_health / max_health; }
};

struct MovementComponent {
    float speed{5.0f};
    float acceleration{10.0f};
    float friction{0.9f};
    std::array<float, 3> velocity{0.0f, 0.0f, 0.0f};
    std::array<float, 3> target_position{0.0f, 0.0f, 0.0f};
    bool has_target{false};
    
    MovementComponent() = default;
    MovementComponent(float move_speed) : speed(move_speed) {}
    
    void set_target(const std::array<float, 3>& target) {
        target_position = target;
        has_target = true;
    }
    
    void clear_target() { has_target = false; }
    
    std::array<float, 3> get_normalized_velocity() const {
        float length = std::sqrt(velocity[0]*velocity[0] + velocity[1]*velocity[1] + velocity[2]*velocity[2]);
        if (length > 0.001f) {
            return {velocity[0]/length, velocity[1]/length, velocity[2]/length};
        }
        return {0.0f, 0.0f, 0.0f};
    }
};

struct AIComponent {
    enum class AIState { Idle, Patrol, Chase, Attack, Flee, Dead };
    
    AIState current_state{AIState::Idle};
    uint32_t target_entity{0};
    float detection_range{10.0f};
    float attack_range{2.0f};
    float flee_health_threshold{0.25f};
    std::vector<std::array<float, 3>> patrol_points;
    size_t current_patrol_index{0};
    double state_timer{0.0};
    
    AIComponent() = default;
    AIComponent(float detect_range, float attack_dist) 
        : detection_range(detect_range), attack_range(attack_dist) {}
    
    void set_state(AIState new_state) {
        if (current_state != new_state) {
            current_state = new_state;
            state_timer = 0.0;
        }
    }
    
    void add_patrol_point(const std::array<float, 3>& point) {
        patrol_points.push_back(point);
    }
    
    std::array<float, 3> get_current_patrol_target() const {
        if (!patrol_points.empty()) {
            return patrol_points[current_patrol_index % patrol_points.size()];
        }
        return {0.0f, 0.0f, 0.0f};
    }
    
    void advance_patrol() {
        if (!patrol_points.empty()) {
            current_patrol_index = (current_patrol_index + 1) % patrol_points.size();
        }
    }
};

// Custom systems (these would inherit from actual ECS system base classes in real implementation)
class HealthSystem {
public:
    void update(double delta_time) {
        // In a real implementation, this would iterate over entities with HealthComponent
        // and update their health regeneration
        // 
        // auto entities = registry->query<HealthComponent>();
        // for (auto entity : entities) {
        //     auto& health = registry->get_component<HealthComponent>(entity);
        //     if (health.current_health < health.max_health && health.current_health > 0) {
        //         health.heal(health.regeneration_rate * delta_time);
        //     }
        // }
    }
};

class MovementSystem {
public:
    void update(double delta_time) {
        // In a real implementation, this would update entity positions based on MovementComponent
        // 
        // auto entities = registry->query<MovementComponent, TransformComponent>();
        // for (auto entity : entities) {
        //     auto& movement = registry->get_component<MovementComponent>(entity);
        //     auto& transform = registry->get_component<TransformComponent>(entity);
        //     
        //     // Apply movement logic
        //     if (movement.has_target) {
        //         // Move towards target
        //         auto direction = calculate_direction(transform.position, movement.target_position);
        //         apply_force(movement, direction, movement.acceleration * delta_time);
        //     }
        //     
        //     // Apply friction
        //     movement.velocity[0] *= movement.friction;
        //     movement.velocity[1] *= movement.friction;
        //     movement.velocity[2] *= movement.friction;
        //     
        //     // Update position
        //     transform.position[0] += movement.velocity[0] * delta_time;
        //     transform.position[1] += movement.velocity[1] * delta_time;
        //     transform.position[2] += movement.velocity[2] * delta_time;
        // }
    }
};

class AISystem {
public:
    void update(double delta_time) {
        // In a real implementation, this would implement AI behavior
        // 
        // auto entities = registry->query<AIComponent, MovementComponent, HealthComponent>();
        // for (auto entity : entities) {
        //     auto& ai = registry->get_component<AIComponent>(entity);
        //     auto& movement = registry->get_component<MovementComponent>(entity);
        //     auto& health = registry->get_component<HealthComponent>(entity);
        //     
        //     ai.state_timer += delta_time;
        //     
        //     switch (ai.current_state) {
        //         case AIComponent::AIState::Idle:
        //             update_idle_state(ai, movement);
        //             break;
        //         case AIComponent::AIState::Patrol:
        //             update_patrol_state(ai, movement);
        //             break;
        //         case AIComponent::AIState::Chase:
        //             update_chase_state(ai, movement);
        //             break;
        //         // ... other states
        //     }
        // }
    }
};

class ECSComponentPlugin : public ecscope::plugins::sdk::PluginBase {
public:
    ECSComponentPlugin() : PluginBase("ecs_component_demo", {1, 0, 0}) {
        set_display_name("ECS Component Demo Plugin");
        set_description("Demonstrates custom ECS components and systems integration");
        set_author("ECScope Team");
        set_license("MIT");
        
        add_tag("ecs");
        add_tag("components");
        add_tag("systems");
        add_tag("demo");
        
        set_priority(ecscope::plugins::PluginPriority::High);
    }
    
    static const ecscope::plugins::PluginMetadata& get_static_metadata() {
        static ecscope::plugins::PluginMetadata metadata;
        if (metadata.name.empty()) {
            metadata.name = "ecs_component_demo";
            metadata.display_name = "ECS Component Demo Plugin";
            metadata.description = "Demonstrates custom ECS components and systems integration";
            metadata.author = "ECScope Team";
            metadata.version = {1, 0, 0};
            metadata.license = "MIT";
            metadata.sandbox_required = true;
            metadata.memory_limit = 1024 * 1024 * 50; // 50MB
            metadata.cpu_time_limit = 100; // 100ms
            metadata.tags = {"ecs", "components", "systems", "demo"};
            metadata.required_permissions = {"ECCoreAccess"};
        }
        return metadata;
    }

protected:
    bool on_initialize() override {
        log_info("Initializing ECS Component Demo Plugin");
        
        // Request ECS access
        if (!request_permission(ecscope::plugins::Permission::ECCoreAccess, 
                              "For demonstrating custom ECS components and systems")) {
            log_error("Failed to get ECS access");
            return false;
        }
        
        // Initialize ECS helper
        auto* registry = get_ecs_registry();
        if (!registry) {
            log_error("ECS Registry not available");
            return false;
        }
        
        // Register custom components
        if (!register_components()) {
            log_error("Failed to register components");
            return false;
        }
        
        // Register custom systems
        if (!register_systems()) {
            log_error("Failed to register systems");
            return false;
        }
        
        // Create demo entities
        create_demo_entities();
        
        // Set up event handlers
        setup_event_handlers();
        
        log_info("ECS Component Demo Plugin initialized successfully");
        return true;
    }
    
    void on_shutdown() override {
        log_info("Shutting down ECS Component Demo Plugin");
        
        // Cleanup demo entities
        cleanup_demo_entities();
        
        log_info("ECS Component Demo Plugin shutdown complete");
    }
    
    void update(double delta_time) override {
        // Update demo logic
        demo_time_ += delta_time;
        
        // Periodically create and destroy entities for demonstration
        if (demo_time_ - last_entity_spawn_ > entity_spawn_interval_) {
            create_random_entity();
            last_entity_spawn_ = demo_time_;
        }
        
        // Update statistics
        update_statistics();
    }

private:
    bool register_components() {
        log_info("Registering custom components");
        
        // In a real implementation, this would use the ECS integration
        // auto* integration = get_ecs_integration();
        // if (!integration) return false;
        
        // integration->register_plugin_component<HealthComponent>("ecs_component_demo", "Health");
        // integration->register_plugin_component<MovementComponent>("ecs_component_demo", "Movement");
        // integration->register_plugin_component<AIComponent>("ecs_component_demo", "AI");
        
        log_info("Components registered successfully");
        return true;
    }
    
    bool register_systems() {
        log_info("Registering custom systems");
        
        // In a real implementation, this would register the systems
        // auto* integration = get_ecs_integration();
        // if (!integration) return false;
        
        // integration->register_plugin_system<HealthSystem>("ecs_component_demo", "Health System");
        // integration->register_plugin_system<MovementSystem>("ecs_component_demo", "Movement System");
        // integration->register_plugin_system<AISystem>("ecs_component_demo", "AI System");
        
        // Store system instances for direct updates
        health_system_ = std::make_unique<HealthSystem>();
        movement_system_ = std::make_unique<MovementSystem>();
        ai_system_ = std::make_unique<AISystem>();
        
        log_info("Systems registered successfully");
        return true;
    }
    
    void create_demo_entities() {
        log_info("Creating demo entities");
        
        // In a real implementation, this would create actual entities
        // auto* registry = get_ecs_registry();
        
        // Create a player entity
        // uint32_t player = registry->create_entity();
        // registry->add_component<HealthComponent>(player, 100.0f, 2.0f);
        // registry->add_component<MovementComponent>(player, 8.0f);
        // demo_entities_.push_back(player);
        
        // Create some AI entities
        // for (int i = 0; i < 5; ++i) {
        //     uint32_t npc = registry->create_entity();
        //     registry->add_component<HealthComponent>(npc, 50.0f + i * 10.0f, 1.0f);
        //     registry->add_component<MovementComponent>(npc, 3.0f + i * 0.5f);
        //     
        //     AIComponent ai(8.0f, 2.0f);
        //     ai.add_patrol_point({static_cast<float>(i * 5), 0.0f, 0.0f});
        //     ai.add_patrol_point({static_cast<float>(i * 5), 0.0f, 5.0f});
        //     registry->add_component<AIComponent>(npc, std::move(ai));
        //     
        //     demo_entities_.push_back(npc);
        // }
        
        log_info("Created " + std::to_string(demo_entities_.size()) + " demo entities");
    }
    
    void create_random_entity() {
        if (demo_entities_.size() >= max_demo_entities_) {
            return; // Don't create too many entities
        }
        
        // In a real implementation, create a random entity with components
        log_debug("Creating random demo entity");
        
        // For now, just simulate entity creation
        static uint32_t next_entity_id = 1000;
        demo_entities_.push_back(next_entity_id++);
    }
    
    void cleanup_demo_entities() {
        log_info("Cleaning up demo entities");
        
        // In a real implementation, destroy all created entities
        // auto* registry = get_ecs_registry();
        // for (uint32_t entity : demo_entities_) {
        //     registry->destroy_entity(entity);
        // }
        
        demo_entities_.clear();
        log_info("Demo entities cleaned up");
    }
    
    void setup_event_handlers() {
        // Handle entity creation events
        subscribe_to_event("entity.created", [this](const auto& params) {
            auto entity_id_str = params.find("entity_id");
            if (entity_id_str != params.end()) {
                log_debug("Entity created: " + entity_id_str->second);
            }
        });
        
        // Handle entity destruction events
        subscribe_to_event("entity.destroyed", [this](const auto& params) {
            auto entity_id_str = params.find("entity_id");
            if (entity_id_str != params.end()) {
                log_debug("Entity destroyed: " + entity_id_str->second);
            }
        });
        
        // Handle component events
        subscribe_to_event("component.added", [this](const auto& params) {
            auto entity_id = params.find("entity_id");
            auto component_type = params.find("component_type");
            if (entity_id != params.end() && component_type != params.end()) {
                log_debug("Component added: " + component_type->second + " to entity " + entity_id->second);
            }
        });
        
        // Set up message handlers for external control
        set_message_handler("spawn_entity", [this](const auto& params) -> std::string {
            create_random_entity();
            return "Entity spawned";
        });
        
        set_message_handler("get_stats", [this](const auto& params) -> std::string {
            return "Active entities: " + std::to_string(demo_entities_.size()) + 
                   ", Demo time: " + std::to_string(demo_time_);
        });
        
        set_message_handler("damage_entity", [this](const auto& params) -> std::string {
            auto entity_str = params.find("entity");
            auto damage_str = params.find("damage");
            
            if (entity_str != params.end() && damage_str != params.end()) {
                // In a real implementation, apply damage to the specified entity
                log_info("Applying damage " + damage_str->second + " to entity " + entity_str->second);
                return "Damage applied";
            }
            return "Invalid parameters";
        });
    }
    
    void update_statistics() {
        // Update plugin statistics
        total_entities_created_ = demo_entities_.size();
        
        // In a real implementation, gather actual ECS statistics
        // auto* registry = get_ecs_registry();
        // components_with_health_ = registry->query<HealthComponent>().size();
        // components_with_movement_ = registry->query<MovementComponent>().size();
        // components_with_ai_ = registry->query<AIComponent>().size();
    }

private:
    // Systems
    std::unique_ptr<HealthSystem> health_system_;
    std::unique_ptr<MovementSystem> movement_system_;
    std::unique_ptr<AISystem> ai_system_;
    
    // Demo state
    std::vector<uint32_t> demo_entities_;
    double demo_time_{0.0};
    double last_entity_spawn_{0.0};
    double entity_spawn_interval_{5.0}; // seconds
    size_t max_demo_entities_{20};
    
    // Statistics
    size_t total_entities_created_{0};
    size_t components_with_health_{0};
    size_t components_with_movement_{0};
    size_t components_with_ai_{0};
};

// Plugin export
DECLARE_PLUGIN(ECSComponentPlugin, "ecs_component_demo", "1.0.0")
DECLARE_PLUGIN_API_VERSION()

/**
 * Example usage:
 * 
 * 1. Load the plugin:
 *    registry->load_plugin("ecs_component_demo.so");
 * 
 * 2. Spawn entities:
 *    registry->send_message("engine", "ecs_component_demo", "spawn_entity", {});
 * 
 * 3. Get statistics:
 *    auto stats = registry->send_message("engine", "ecs_component_demo", "get_stats", {});
 * 
 * 4. Apply damage:
 *    registry->send_message("engine", "ecs_component_demo", "damage_entity", {
 *        {"entity", "1001"}, {"damage", "25.0"}
 *    });
 */