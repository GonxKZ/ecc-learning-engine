/**
 * @file ecs_inspector_demo.cpp
 * @brief Comprehensive ECS Inspector Integration Demo
 * 
 * Demonstrates the full capabilities of the ECS Inspector integrated with
 * the main dashboard, including real-time debugging, component editing,
 * system monitoring, and performance analysis.
 * 
 * Features Demonstrated:
 * - Entity hierarchy management with parent/child relationships
 * - Real-time component visualization and editing
 * - System execution monitoring and profiling
 * - Archetype analysis and memory tracking
 * - Query builder for testing ECS queries
 * - Component change history and undo/redo
 * - Performance metrics and optimization insights
 * - Dashboard integration and workspace management
 * 
 * @author ECScope Development Team
 * @version 1.0.0
 */

#include "../../include/ecscope/registry.hpp"
#include "../../include/ecscope/gui/dashboard.hpp"
#include "../../include/ecscope/gui/ecs_inspector.hpp"
#include "../../include/ecscope/gui/ecs_inspector_widgets.hpp"
#include "../../include/ecscope/core/log.hpp"
#include "../../include/ecscope/core/time.hpp"

#include <iostream>
#include <random>
#include <thread>
#include <chrono>

// =============================================================================
// DEMO COMPONENTS
// =============================================================================

namespace demo {

/**
 * @brief Transform component for entity positioning
 */
struct Transform {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
    float rotation = 0.0f;
    float scale = 1.0f;
    
    Transform() = default;
    Transform(float x, float y, float z = 0.0f) : x(x), y(y), z(z) {}
    
    void move(float dx, float dy, float dz = 0.0f) {
        x += dx; y += dy; z += dz;
    }
    
    std::string to_string() const {
        return std::format("Transform({:.2f}, {:.2f}, {:.2f}, rot: {:.2f}, scale: {:.2f})", 
                          x, y, z, rotation, scale);
    }
};

/**
 * @brief Velocity component for movement
 */
struct Velocity {
    float vx = 0.0f;
    float vy = 0.0f;
    float vz = 0.0f;
    float max_speed = 100.0f;
    
    Velocity() = default;
    Velocity(float vx, float vy, float vz = 0.0f) : vx(vx), vy(vy), vz(vz) {}
    
    float speed() const {
        return std::sqrt(vx * vx + vy * vy + vz * vz);
    }
    
    void clamp_to_max_speed() {
        float current_speed = speed();
        if (current_speed > max_speed) {
            float scale = max_speed / current_speed;
            vx *= scale;
            vy *= scale;
            vz *= scale;
        }
    }
    
    std::string to_string() const {
        return std::format("Velocity({:.2f}, {:.2f}, {:.2f}, max: {:.2f})", 
                          vx, vy, vz, max_speed);
    }
};

/**
 * @brief Health component for entities
 */
struct Health {
    float current = 100.0f;
    float maximum = 100.0f;
    float regeneration_rate = 1.0f;
    bool invulnerable = false;
    
    Health() = default;
    Health(float max_hp) : current(max_hp), maximum(max_hp) {}
    
    void damage(float amount) {
        if (!invulnerable) {
            current = std::max(0.0f, current - amount);
        }
    }
    
    void heal(float amount) {
        current = std::min(maximum, current + amount);
    }
    
    bool is_alive() const { return current > 0.0f; }
    float health_percentage() const { return maximum > 0 ? current / maximum : 0.0f; }
    
    std::string to_string() const {
        return std::format("Health({:.1f}/{:.1f}, regen: {:.2f}, invuln: {})", 
                          current, maximum, regeneration_rate, invulnerable);
    }
};

/**
 * @brief Render component for visual representation
 */
struct Renderable {
    std::string sprite_name = "default";
    float scale = 1.0f;
    uint32_t color = 0xFFFFFFFF; // RGBA
    int layer = 0;
    bool visible = true;
    float alpha = 1.0f;
    
    Renderable() = default;
    Renderable(const std::string& sprite, uint32_t col = 0xFFFFFFFF) 
        : sprite_name(sprite), color(col) {}
    
    void set_color_rgb(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255) {
        color = (a << 24) | (b << 16) | (g << 8) | r;
    }
    
    std::string to_string() const {
        return std::format("Renderable(sprite: {}, scale: {:.2f}, layer: {}, visible: {})", 
                          sprite_name, scale, layer, visible);
    }
};

/**
 * @brief Tag component for AI entities
 */
struct AIAgent {
    enum class Behavior {
        Idle,
        Patrol,
        Chase,
        Attack,
        Flee
    };
    
    Behavior current_behavior = Behavior::Idle;
    float aggression = 0.5f;
    float detection_range = 50.0f;
    float reaction_time = 1.0f;
    std::string ai_script = "basic_ai";
    
    AIAgent() = default;
    AIAgent(Behavior behavior) : current_behavior(behavior) {}
    
    std::string behavior_to_string() const {
        switch (current_behavior) {
            case Behavior::Idle: return "Idle";
            case Behavior::Patrol: return "Patrol";
            case Behavior::Chase: return "Chase";
            case Behavior::Attack: return "Attack";
            case Behavior::Flee: return "Flee";
            default: return "Unknown";
        }
    }
    
    std::string to_string() const {
        return std::format("AIAgent(behavior: {}, aggr: {:.2f}, detect: {:.1f})", 
                          behavior_to_string(), aggression, detection_range);
    }
};

/**
 * @brief Player tag component
 */
struct Player {
    std::string name = "Player";
    int level = 1;
    int experience = 0;
    int score = 0;
    
    Player() = default;
    Player(const std::string& player_name) : name(player_name) {}
    
    std::string to_string() const {
        return std::format("Player(name: {}, lvl: {}, exp: {}, score: {})", 
                          name, level, experience, score);
    }
};

} // namespace demo

// =============================================================================
// DEMO SYSTEMS
// =============================================================================

namespace demo {

/**
 * @brief Movement system that updates positions based on velocity
 */
class MovementSystem {
public:
    MovementSystem(const std::string& name = "MovementSystem") 
        : system_name_(name), entities_processed_(0) {}
    
    void update(ecscope::ecs::Registry& registry, float delta_time) {
        auto start_time = std::chrono::high_resolution_clock::now();
        entities_processed_ = 0;
        
        registry.for_each<Transform, Velocity>([&](ecscope::ecs::Entity entity, Transform& transform, Velocity& velocity) {
            // Update position based on velocity
            transform.x += velocity.vx * delta_time;
            transform.y += velocity.vy * delta_time;
            transform.z += velocity.vz * delta_time;
            
            // Clamp velocity to max speed
            velocity.clamp_to_max_speed();
            
            entities_processed_++;
        });
        
        auto end_time = std::chrono::high_resolution_clock::now();
        last_execution_time_ = std::chrono::duration<float, std::milli>(end_time - start_time);
    }
    
    const std::string& name() const { return system_name_; }
    std::chrono::duration<float, std::milli> last_execution_time() const { return last_execution_time_; }
    uint64_t entities_processed() const { return entities_processed_; }
    
private:
    std::string system_name_;
    std::chrono::duration<float, std::milli> last_execution_time_{0};
    uint64_t entities_processed_;
};

/**
 * @brief Health system that handles regeneration and death
 */
class HealthSystem {
public:
    HealthSystem(const std::string& name = "HealthSystem") 
        : system_name_(name), entities_processed_(0) {}
    
    void update(ecscope::ecs::Registry& registry, float delta_time) {
        auto start_time = std::chrono::high_resolution_clock::now();
        entities_processed_ = 0;
        
        std::vector<ecscope::ecs::Entity> entities_to_remove;
        
        registry.for_each<Health>([&](ecscope::ecs::Entity entity, Health& health) {
            // Apply regeneration
            if (health.current < health.maximum && health.regeneration_rate > 0) {
                health.heal(health.regeneration_rate * delta_time);
            }
            
            // Check for death
            if (!health.is_alive()) {
                entities_to_remove.push_back(entity);
            }
            
            entities_processed_++;
        });
        
        // Remove dead entities
        for (auto entity : entities_to_remove) {
            registry.destroy_entity(entity);
        }
        
        auto end_time = std::chrono::high_resolution_clock::now();
        last_execution_time_ = std::chrono::duration<float, std::milli>(end_time - start_time);
    }
    
    const std::string& name() const { return system_name_; }
    std::chrono::duration<float, std::milli> last_execution_time() const { return last_execution_time_; }
    uint64_t entities_processed() const { return entities_processed_; }
    
private:
    std::string system_name_;
    std::chrono::duration<float, std::milli> last_execution_time_{0};
    uint64_t entities_processed_;
};

/**
 * @brief AI system that updates AI behavior
 */
class AISystem {
public:
    AISystem(const std::string& name = "AISystem") 
        : system_name_(name), entities_processed_(0) {}
    
    void update(ecscope::ecs::Registry& registry, float delta_time) {
        auto start_time = std::chrono::high_resolution_clock::now();
        entities_processed_ = 0;
        
        registry.for_each<AIAgent, Transform, Velocity>([&](ecscope::ecs::Entity entity, 
                                                           AIAgent& ai, Transform& transform, Velocity& velocity) {
            // Simple AI behavior simulation
            switch (ai.current_behavior) {
                case AIAgent::Behavior::Idle:
                    // Occasionally switch to patrol
                    if (rand() % 1000 < 10) { // 1% chance per frame
                        ai.current_behavior = AIAgent::Behavior::Patrol;
                    }
                    velocity.vx = velocity.vy = 0.0f;
                    break;
                    
                case AIAgent::Behavior::Patrol:
                    // Simple random movement
                    velocity.vx = (rand() % 200 - 100) * 0.01f; // -1 to 1
                    velocity.vy = (rand() % 200 - 100) * 0.01f;
                    
                    // Occasionally switch back to idle
                    if (rand() % 1000 < 5) { // 0.5% chance
                        ai.current_behavior = AIAgent::Behavior::Idle;
                    }
                    break;
                    
                case AIAgent::Behavior::Chase:
                    // Would need target tracking - simplified for demo
                    velocity.vx *= 1.5f; // Increase speed when chasing
                    velocity.vy *= 1.5f;
                    break;
                    
                default:
                    break;
            }
            
            entities_processed_++;
        });
        
        auto end_time = std::chrono::high_resolution_clock::now();
        last_execution_time_ = std::chrono::duration<float, std::milli>(end_time - start_time);
    }
    
    const std::string& name() const { return system_name_; }
    std::chrono::duration<float, std::milli> last_execution_time() const { return last_execution_time_; }
    uint64_t entities_processed() const { return entities_processed_; }
    
private:
    std::string system_name_;
    std::chrono::duration<float, std::milli> last_execution_time_{0};
    uint64_t entities_processed_;
};

/**
 * @brief Rendering system for display updates
 */
class RenderSystem {
public:
    RenderSystem(const std::string& name = "RenderSystem") 
        : system_name_(name), entities_processed_(0) {}
    
    void update(ecscope::ecs::Registry& registry, float delta_time) {
        auto start_time = std::chrono::high_resolution_clock::now();
        entities_processed_ = 0;
        
        registry.for_each<Transform, Renderable>([&](ecscope::ecs::Entity entity, 
                                                    const Transform& transform, const Renderable& render) {
            if (!render.visible) return;
            
            // Simulate rendering work
            std::this_thread::sleep_for(std::chrono::microseconds(10));
            
            entities_processed_++;
        });
        
        auto end_time = std::chrono::high_resolution_clock::now();
        last_execution_time_ = std::chrono::duration<float, std::milli>(end_time - start_time);
    }
    
    const std::string& name() const { return system_name_; }
    std::chrono::duration<float, std::milli> last_execution_time() const { return last_execution_time_; }
    uint64_t entities_processed() const { return entities_processed_; }
    
private:
    std::string system_name_;
    std::chrono::duration<float, std::milli> last_execution_time_{0};
    uint64_t entities_processed_;
};

} // namespace demo

// =============================================================================
// DEMO ENTITY FACTORY
// =============================================================================

namespace demo {

class EntityFactory {
public:
    static ecscope::ecs::Entity create_player(ecscope::ecs::Registry& registry, 
                                             const std::string& name = "Player",
                                             float x = 0.0f, float y = 0.0f) {
        auto entity = registry.create_entity(
            Transform{x, y, 0.0f},
            Velocity{0.0f, 0.0f},
            Health{100.0f},
            Renderable{"player_sprite", 0xFF00FF00}, // Green
            Player{name}
        );
        
        LOG_INFO("Created player entity {} at ({}, {})", static_cast<uint32_t>(entity), x, y);
        return entity;
    }
    
    static ecscope::ecs::Entity create_enemy(ecscope::ecs::Registry& registry,
                                           float x = 0.0f, float y = 0.0f) {
        auto entity = registry.create_entity(
            Transform{x, y, 0.0f},
            Velocity{(rand() % 200 - 100) * 0.01f, (rand() % 200 - 100) * 0.01f},
            Health{50.0f + rand() % 50}, // 50-100 HP
            Renderable{"enemy_sprite", 0xFFFF0000}, // Red
            AIAgent{AIAgent::Behavior::Patrol}
        );
        
        LOG_DEBUG("Created enemy entity {} at ({}, {})", static_cast<uint32_t>(entity), x, y);
        return entity;
    }
    
    static ecscope::ecs::Entity create_npc(ecscope::ecs::Registry& registry,
                                          const std::string& type = "civilian",
                                          float x = 0.0f, float y = 0.0f) {
        auto entity = registry.create_entity(
            Transform{x, y, 0.0f},
            Health{30.0f},
            Renderable{type + "_sprite", 0xFF0000FF} // Blue
        );
        
        // Sometimes add AI behavior
        if (rand() % 2) {
            registry.add_component(entity, AIAgent{AIAgent::Behavior::Idle});
        }
        
        // Sometimes add velocity for movement
        if (rand() % 3) {
            registry.add_component(entity, Velocity{(rand() % 100 - 50) * 0.005f, 
                                                   (rand() % 100 - 50) * 0.005f});
        }
        
        LOG_DEBUG("Created NPC entity {} of type '{}' at ({}, {})", 
                 static_cast<uint32_t>(entity), type, x, y);
        return entity;
    }
    
    static ecscope::ecs::Entity create_projectile(ecscope::ecs::Registry& registry,
                                                 float x, float y, float vx, float vy) {
        auto entity = registry.create_entity(
            Transform{x, y, 0.0f},
            Velocity{vx, vy},
            Renderable{"projectile_sprite", 0xFFFFFF00} // Yellow
        );
        
        LOG_DEBUG("Created projectile entity {} at ({}, {}) with velocity ({}, {})",
                 static_cast<uint32_t>(entity), x, y, vx, vy);
        return entity;
    }
    
    static ecscope::ecs::Entity create_pickup(ecscope::ecs::Registry& registry,
                                            const std::string& type = "health_potion",
                                            float x = 0.0f, float y = 0.0f) {
        auto entity = registry.create_entity(
            Transform{x, y, 0.0f},
            Renderable{type + "_sprite", 0xFFFF00FF} // Magenta
        );
        
        LOG_DEBUG("Created pickup entity {} of type '{}' at ({}, {})",
                 static_cast<uint32_t>(entity), type, x, y);
        return entity;
    }
};

} // namespace demo

// =============================================================================
// COMPONENT EDITOR REGISTRATION
// =============================================================================

void register_demo_component_editors(ecscope::gui::ECSInspector& inspector) {
    using namespace ecscope::gui;
    
    // Register component types with the inspector
    inspector.register_component_type<demo::Transform>("Transform", "Core");
    inspector.register_component_type<demo::Velocity>("Velocity", "Physics");
    inspector.register_component_type<demo::Health>("Health", "Gameplay");
    inspector.register_component_type<demo::Renderable>("Renderable", "Rendering");
    inspector.register_component_type<demo::AIAgent>("AIAgent", "AI");
    inspector.register_component_type<demo::Player>("Player", "Gameplay");
    
    // Create custom component editors
    auto transform_editor = std::make_unique<ComponentEditor>("Transform", sizeof(demo::Transform));
    transform_editor->register_property<float>("x", offsetof(demo::Transform, x), "X Position", "World X coordinate");
    transform_editor->register_property<float>("y", offsetof(demo::Transform, y), "Y Position", "World Y coordinate");
    transform_editor->register_property<float>("z", offsetof(demo::Transform, z), "Z Position", "World Z coordinate");
    transform_editor->register_property<float>("rotation", offsetof(demo::Transform, rotation), "Rotation", "Rotation in degrees");
    transform_editor->register_property<float>("scale", offsetof(demo::Transform, scale), "Scale", "Uniform scale factor");
    
    auto velocity_editor = std::make_unique<ComponentEditor>("Velocity", sizeof(demo::Velocity));
    velocity_editor->register_property<float>("vx", offsetof(demo::Velocity, vx), "X Velocity", "Velocity along X axis");
    velocity_editor->register_property<float>("vy", offsetof(demo::Velocity, vy), "Y Velocity", "Velocity along Y axis");
    velocity_editor->register_property<float>("vz", offsetof(demo::Velocity, vz), "Z Velocity", "Velocity along Z axis");
    velocity_editor->register_property<float>("max_speed", offsetof(demo::Velocity, max_speed), "Max Speed", "Maximum allowed speed");
    
    auto health_editor = std::make_unique<ComponentEditor>("Health", sizeof(demo::Health));
    health_editor->register_property<float>("current", offsetof(demo::Health, current), "Current HP", "Current health points");
    health_editor->register_property<float>("maximum", offsetof(demo::Health, maximum), "Maximum HP", "Maximum health points");
    health_editor->register_property<float>("regeneration_rate", offsetof(demo::Health, regeneration_rate), "Regen Rate", "HP regeneration per second");
    health_editor->register_property<bool>("invulnerable", offsetof(demo::Health, invulnerable), "Invulnerable", "Cannot take damage");
    
    // Register component templates
    ComponentTemplate player_template("Player Character", std::type_index(typeid(demo::Player)));
    player_template.description = "Standard player character setup";
    inspector.register_component_template(player_template);
    
    ComponentTemplate enemy_template("Basic Enemy", std::type_index(typeid(demo::AIAgent)));
    enemy_template.description = "Basic AI enemy configuration";
    inspector.register_component_template(enemy_template);
}

// =============================================================================
// MAIN DEMO APPLICATION
// =============================================================================

class ECSInspectorDemo {
public:
    ECSInspectorDemo() 
        : registry_(ecscope::ecs::AllocatorConfig::create_educational_focused(), "DemoRegistry")
        , inspector_config_(ecscope::gui::InspectorConfig::create_debugging_focused())
        , inspector_(&registry_, inspector_config_)
        , movement_system_("MovementSystem")
        , health_system_("HealthSystem")
        , ai_system_("AISystem")
        , render_system_("RenderSystem")
    {
        // Seed random number generator
        srand(static_cast<unsigned>(time(nullptr)));
    }
    
    bool initialize() {
        LOG_INFO("Initializing ECS Inspector Demo...");
        
        // Initialize dashboard
        if (!dashboard_.initialize()) {
            LOG_ERROR("Failed to initialize dashboard");
            return false;
        }
        
        // Initialize inspector
        if (!inspector_.initialize()) {
            LOG_ERROR("Failed to initialize ECS inspector");
            return false;
        }
        
        // Register inspector with dashboard
        inspector_.register_with_dashboard(&dashboard_);
        
        // Register component editors
        register_demo_component_editors(inspector_);
        
        // Register systems with inspector
        register_systems_with_inspector();
        
        // Create demo entities
        create_demo_world();
        
        LOG_INFO("ECS Inspector Demo initialized successfully");
        return true;
    }
    
    void run() {
        LOG_INFO("Starting ECS Inspector Demo main loop...");
        
        auto last_time = std::chrono::high_resolution_clock::now();
        bool running = true;
        
        while (running) {
            auto current_time = std::chrono::high_resolution_clock::now();
            float delta_time = std::chrono::duration<float>(current_time - last_time).count();
            last_time = current_time;
            
            // Clamp delta time to prevent large jumps
            delta_time = std::min(delta_time, 0.016f); // Max 16ms (60 FPS minimum)
            
            // Update systems
            update_systems(delta_time);
            
            // Update inspector
            inspector_.update(delta_time);
            
            // Update dashboard
            dashboard_.update(delta_time);
            
            // Render
            render_frame();
            
            // Periodically create/destroy entities for demo purposes
            static int frame_counter = 0;
            frame_counter++;
            
            if (frame_counter % 300 == 0) { // Every 5 seconds at 60 FPS
                create_random_entities();
            }
            
            if (frame_counter % 600 == 0) { // Every 10 seconds
                cleanup_entities();
            }
            
            // Simple exit condition for demo (normally would handle window events)
            if (frame_counter > 36000) { // Run for 10 minutes max
                running = false;
            }
            
            // Sleep to maintain reasonable frame rate
            std::this_thread::sleep_for(std::chrono::milliseconds(16)); // ~60 FPS
        }
        
        LOG_INFO("ECS Inspector Demo finished");
    }
    
    void shutdown() {
        LOG_INFO("Shutting down ECS Inspector Demo...");
        
        inspector_.shutdown();
        dashboard_.shutdown();
        
        // Print final statistics
        print_final_statistics();
    }
    
private:
    void register_systems_with_inspector() {
        // Register systems for monitoring
        ecscope::gui::SystemStats movement_stats;
        movement_stats.system_id = "movement_system";
        movement_stats.name = "Movement System";
        movement_stats.category = "Physics";
        inspector_.register_system(movement_stats);
        
        ecscope::gui::SystemStats health_stats;
        health_stats.system_id = "health_system";
        health_stats.name = "Health System";
        health_stats.category = "Gameplay";
        inspector_.register_system(health_stats);
        
        ecscope::gui::SystemStats ai_stats;
        ai_stats.system_id = "ai_system";
        ai_stats.name = "AI System";
        ai_stats.category = "AI";
        inspector_.register_system(ai_stats);
        
        ecscope::gui::SystemStats render_stats;
        render_stats.system_id = "render_system";
        render_stats.name = "Render System";
        render_stats.category = "Rendering";
        inspector_.register_system(render_stats);
    }
    
    void create_demo_world() {
        LOG_INFO("Creating demo world...");
        
        // Create player
        player_entity_ = demo::EntityFactory::create_player(registry_, "Demo Player", 0.0f, 0.0f);
        
        // Create enemies
        for (int i = 0; i < 10; ++i) {
            float x = (rand() % 200 - 100) * 2.0f; // -200 to 200
            float y = (rand() % 200 - 100) * 2.0f;
            demo::EntityFactory::create_enemy(registry_, x, y);
        }
        
        // Create NPCs
        for (int i = 0; i < 15; ++i) {
            float x = (rand() % 300 - 150) * 2.0f; // -300 to 300
            float y = (rand() % 300 - 150) * 2.0f;
            std::string type = (i % 3 == 0) ? "merchant" : (i % 3 == 1) ? "guard" : "civilian";
            demo::EntityFactory::create_npc(registry_, type, x, y);
        }
        
        // Create some pickups
        for (int i = 0; i < 8; ++i) {
            float x = (rand() % 400 - 200) * 2.0f;
            float y = (rand() % 400 - 200) * 2.0f;
            std::string type = (i % 2 == 0) ? "health_potion" : "mana_potion";
            demo::EntityFactory::create_pickup(registry_, type, x, y);
        }
        
        // Create some projectiles
        for (int i = 0; i < 5; ++i) {
            float x = (rand() % 100 - 50) * 1.0f;
            float y = (rand() % 100 - 50) * 1.0f;
            float vx = (rand() % 200 - 100) * 1.0f;
            float vy = (rand() % 200 - 100) * 1.0f;
            demo::EntityFactory::create_projectile(registry_, x, y, vx, vy);
        }
        
        LOG_INFO("Created demo world with {} entities", registry_.active_entities());
    }
    
    void update_systems(float delta_time) {
        // Update systems and track performance
        movement_system_.update(registry_, delta_time);
        inspector_.update_system_stats("movement_system", 
                                      movement_system_.last_execution_time(),
                                      movement_system_.entities_processed());
        
        health_system_.update(registry_, delta_time);
        inspector_.update_system_stats("health_system",
                                      health_system_.last_execution_time(),
                                      health_system_.entities_processed());
        
        ai_system_.update(registry_, delta_time);
        inspector_.update_system_stats("ai_system",
                                      ai_system_.last_execution_time(),
                                      ai_system_.entities_processed());
        
        render_system_.update(registry_, delta_time);
        inspector_.update_system_stats("render_system",
                                      render_system_.last_execution_time(),
                                      render_system_.entities_processed());
    }
    
    void render_frame() {
        // Main render loop - in a real application this would involve
        // actual graphics rendering. For this demo, we just render the GUI.
        dashboard_.render();
        inspector_.render();
    }
    
    void create_random_entities() {
        // Periodically create new entities to demonstrate dynamic behavior
        int num_entities = rand() % 3 + 1; // 1-3 entities
        
        for (int i = 0; i < num_entities; ++i) {
            int entity_type = rand() % 4;
            float x = (rand() % 600 - 300) * 1.0f;
            float y = (rand() % 600 - 300) * 1.0f;
            
            switch (entity_type) {
                case 0:
                    demo::EntityFactory::create_enemy(registry_, x, y);
                    break;
                case 1:
                    demo::EntityFactory::create_npc(registry_, "spawned_npc", x, y);
                    break;
                case 2:
                    demo::EntityFactory::create_pickup(registry_, "random_pickup", x, y);
                    break;
                case 3:
                    demo::EntityFactory::create_projectile(registry_, x, y, 
                                                          (rand() % 200 - 100) * 0.5f,
                                                          (rand() % 200 - 100) * 0.5f);
                    break;
            }
        }
        
        LOG_DEBUG("Created {} random entities, total active: {}", 
                 num_entities, registry_.active_entities());
    }
    
    void cleanup_entities() {
        // Remove some entities to prevent unbounded growth
        auto all_entities = registry_.get_all_entities();
        
        if (all_entities.size() > 100) {
            // Remove oldest projectiles and some NPCs
            std::vector<ecscope::ecs::Entity> to_remove;
            
            for (auto entity : all_entities) {
                // Don't remove player
                if (entity == player_entity_) continue;
                
                // Remove projectiles (they don't have health)
                if (!registry_.has_component<demo::Health>(entity)) {
                    to_remove.push_back(entity);
                    if (to_remove.size() >= 10) break;
                }
            }
            
            for (auto entity : to_remove) {
                registry_.destroy_entity(entity);
            }
            
            if (!to_remove.empty()) {
                LOG_DEBUG("Cleaned up {} entities, active: {}", 
                         to_remove.size(), registry_.active_entities());
            }
        }
    }
    
    void print_final_statistics() {
        LOG_INFO("=== ECS Inspector Demo Final Statistics ===");
        
        // Registry statistics
        LOG_INFO("Registry Statistics:");
        LOG_INFO("  - Total entities created: {}", registry_.total_entities_created());
        LOG_INFO("  - Active entities: {}", registry_.active_entities());
        LOG_INFO("  - Total archetypes: {}", registry_.archetype_count());
        LOG_INFO("  - Memory usage: {} KB", registry_.memory_usage() / 1024);
        
        // Memory statistics
        auto memory_stats = registry_.get_memory_statistics();
        LOG_INFO("Memory Statistics:");
        LOG_INFO("  - Arena utilization: {:.1f}%", memory_stats.arena_utilization() * 100.0);
        LOG_INFO("  - Pool utilization: {:.1f}%", memory_stats.pool_utilization() * 100.0);
        LOG_INFO("  - Memory efficiency: {:.1f}%", memory_stats.memory_efficiency * 100.0);
        LOG_INFO("  - Cache hit ratio: {:.1f}%", memory_stats.cache_hit_ratio * 100.0);
        
        // Inspector statistics
        auto inspector_metrics = inspector_.get_metrics();
        LOG_INFO("Inspector Statistics:");
        LOG_INFO("  - Entities tracked: {}", inspector_metrics.entities_tracked);
        LOG_INFO("  - Components tracked: {}", inspector_metrics.components_tracked);
        LOG_INFO("  - Systems tracked: {}", inspector_metrics.systems_tracked);
        LOG_INFO("  - Average update time: {:.3f} ms", inspector_metrics.last_update_time_ms);
        LOG_INFO("  - Average render time: {:.3f} ms", inspector_metrics.last_render_time_ms);
        
        // System performance
        LOG_INFO("System Performance:");
        auto all_systems = inspector_.get_all_systems();
        for (const auto& system_id : all_systems) {
            auto stats = inspector_.get_system_stats(system_id);
            if (stats) {
                LOG_INFO("  - {}: avg {:.3f}ms, {} executions, {} entities/exec",
                        stats->name,
                        stats->average_execution_time.count(),
                        stats->execution_count,
                        stats->execution_count > 0 ? stats->entities_processed / stats->execution_count : 0);
            }
        }
        
        LOG_INFO("=== End Statistics ===");
    }
    
    // Core components
    ecscope::ecs::Registry registry_;
    ecscope::gui::Dashboard dashboard_;
    ecscope::gui::InspectorConfig inspector_config_;
    ecscope::gui::ECSInspector inspector_;
    
    // Systems
    demo::MovementSystem movement_system_;
    demo::HealthSystem health_system_;
    demo::AISystem ai_system_;
    demo::RenderSystem render_system_;
    
    // Special entities
    ecscope::ecs::Entity player_entity_;
};

// =============================================================================
// MAIN FUNCTION
// =============================================================================

int main() {
    std::cout << "ECScope ECS Inspector Demo\n";
    std::cout << "==========================\n";
    std::cout << "This demo showcases the comprehensive ECS Inspector integrated\n";
    std::cout << "with the main dashboard. It demonstrates:\n";
    std::cout << "- Real-time entity/component visualization and editing\n";
    std::cout << "- System execution monitoring and profiling\n";
    std::cout << "- Archetype analysis and memory tracking\n";
    std::cout << "- Query builder for testing ECS queries\n";
    std::cout << "- Component change history and undo/redo\n";
    std::cout << "- Performance metrics and optimization insights\n";
    std::cout << "\n";
    
    // Initialize logging
    ecscope::log::initialize();
    ecscope::log::set_level(ecscope::log::Level::INFO);
    
    try {
        ECSInspectorDemo demo;
        
        if (!demo.initialize()) {
            LOG_ERROR("Failed to initialize demo");
            return -1;
        }
        
        std::cout << "Demo initialized successfully. Running main loop...\n";
        std::cout << "(In a real application, this would show an interactive window)\n";
        std::cout << "Check the logs for detailed information about ECS operations.\n\n";
        
        demo.run();
        demo.shutdown();
        
        std::cout << "\nDemo completed successfully!\n";
        
    } catch (const std::exception& e) {
        LOG_ERROR("Demo crashed with exception: {}", e.what());
        std::cerr << "Error: " << e.what() << std::endl;
        return -1;
    }
    
    return 0;
}