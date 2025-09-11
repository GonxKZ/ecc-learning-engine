/**
 * @file ai_framework_demo.cpp
 * @brief Comprehensive AI Framework Demonstration
 * 
 * This example demonstrates the complete AI/ML framework integrated with the ECScope engine.
 * It showcases behavior AI, machine learning, procedural content generation, and performance
 * optimization working together in a realistic game scenario.
 * 
 * Features demonstrated:
 * - AI agents with FSM and Behavior Tree behaviors
 * - Neural network for decision making
 * - Procedural terrain generation using noise
 * - Flocking behavior for group movement
 * - Memory system for learning and adaptation
 * - Performance optimization and monitoring
 */

#include "../../include/ecscope/ai/ai.hpp"
#include "../../include/ecscope/registry.hpp"
#include "../../include/ecscope/world.hpp"
#include "../../include/ecscope/core/log.hpp"
#include "../../include/ecscope/core/timer.hpp"

#include <iostream>
#include <vector>
#include <memory>
#include <chrono>
#include <thread>

using namespace ecscope;
using namespace ecscope::ai;

/**
 * @brief Demo Transform Component for positioning
 */
struct TransformComponent {
    Vec3 position{0, 0, 0};
    Vec3 rotation{0, 0, 0};
    Vec3 scale{1, 1, 1};
    Vec3 velocity{0, 0, 0};
    f32 speed = 1.0f;
};

/**
 * @brief Demo Health Component for survival AI
 */
struct HealthComponent {
    f32 current_health = 100.0f;
    f32 max_health = 100.0f;
    f32 regeneration_rate = 1.0f;
    bool is_alive = true;
    
    void take_damage(f32 damage) {
        current_health = std::max(0.0f, current_health - damage);
        is_alive = current_health > 0.0f;
    }
    
    void heal(f32 amount) {
        current_health = std::min(max_health, current_health + amount);
        is_alive = current_health > 0.0f;
    }
    
    f32 health_percentage() const {
        return max_health > 0 ? current_health / max_health : 0.0f;
    }
};

/**
 * @brief Demo Resource Component for resource gathering AI
 */
struct ResourceComponent {
    f32 food = 50.0f;
    f32 water = 50.0f;
    f32 energy = 100.0f;
    f32 max_food = 100.0f;
    f32 max_water = 100.0f;
    f32 max_energy = 100.0f;
    
    void consume_resources(f32 delta_time) {
        f32 consumption_rate = 5.0f; // per second
        food = std::max(0.0f, food - consumption_rate * delta_time);
        water = std::max(0.0f, water - consumption_rate * delta_time);
        
        // Energy depends on food and water
        if (food > 10.0f && water > 10.0f) {
            energy = std::min(max_energy, energy + 10.0f * delta_time);
        } else {
            energy = std::max(0.0f, energy - 20.0f * delta_time);
        }
    }
    
    bool is_hungry() const { return food < 30.0f; }
    bool is_thirsty() const { return water < 30.0f; }
    bool is_tired() const { return energy < 30.0f; }
};

/**
 * @brief AI Demo World - Contains the simulation environment
 */
class AIDemoWorld {
public:
    AIDemoWorld() {
        initialize_world();
        setup_ai_framework();
        create_terrain();
        spawn_agents();
        setup_resources();
    }
    
    void run_simulation(f32 duration_seconds) {
        LOG_INFO("Starting AI Framework Demo Simulation");
        LOG_INFO("Duration: " + std::to_string(duration_seconds) + " seconds");
        
        auto start_time = std::chrono::high_resolution_clock::now();
        f64 elapsed_time = 0.0;
        f64 last_time = 0.0;
        
        while (elapsed_time < duration_seconds) {
            auto current_time = std::chrono::high_resolution_clock::now();
            elapsed_time = std::chrono::duration<f64>(current_time - start_time).count();
            f64 delta_time = elapsed_time - last_time;
            last_time = elapsed_time;
            
            // Update world
            update_world(delta_time);
            
            // Update AI framework
            ai::update(delta_time);
            
            // Print periodic statistics
            if (static_cast<i32>(elapsed_time) % 10 == 0 && 
                static_cast<i32>(elapsed_time) != last_stats_time_) {
                print_simulation_statistics(elapsed_time);
                last_stats_time_ = static_cast<i32>(elapsed_time);
            }
            
            // Sleep to limit update rate
            std::this_thread::sleep_for(std::chrono::milliseconds(16)); // ~60 FPS
        }
        
        LOG_INFO("AI Framework Demo Completed");
        print_final_statistics();
    }
    
private:
    ecs::Registry registry_;
    std::unique_ptr<AIAgentManager> agent_manager_;
    std::unique_ptr<MLModelManager> ml_manager_;
    std::unique_ptr<PCGManager> pcg_manager_;
    
    // Simulation data
    Grid2D<f32> terrain_;
    std::vector<ecs::Entity> predators_;
    std::vector<ecs::Entity> prey_;
    std::vector<ecs::Entity> resources_;
    std::vector<ecs::Entity> npcs_;
    
    i32 last_stats_time_ = -1;
    
    void initialize_world() {
        LOG_INFO("Initializing AI Demo World");
        
        // Register custom components
        registry_.register_component<TransformComponent>();
        registry_.register_component<HealthComponent>();
        registry_.register_component<ResourceComponent>();
        
        // Add AI systems
        registry_.add_system<AIAgentSystem>();
        registry_.add_system<AISensorSystem>();
        registry_.add_system<AIDecisionSystem>();
        registry_.add_system<AIBehaviorSystem>();
    }
    
    void setup_ai_framework() {
        LOG_INFO("Setting up AI Framework");
        
        AIConfig config;
        config.ai_thread_count = 4;
        config.enable_parallel_processing = true;
        config.ai_memory_pool_size = 128 * 1024 * 1024; // 128MB
        config.enable_neural_networks = true;
        config.enable_noise_generation = true;
        config.enable_ai_visualization = true;
        
        if (!ai::initialize(registry_, config)) {
            LOG_ERROR("Failed to initialize AI framework");
            return;
        }
        
        // Create managers
        agent_manager_ = std::make_unique<AIAgentManager>(registry_);
        ml_manager_ = std::make_unique<MLModelManager>();
        pcg_manager_ = std::make_unique<PCGManager>();
        
        LOG_INFO("AI Framework initialized successfully");
    }
    
    void create_terrain() {
        LOG_INFO("Generating procedural terrain");
        
        // Generate base terrain using Perlin noise
        auto base_noise = pcg_manager_->create_noise_generator(NoiseType::PERLIN, 12345);
        terrain_ = noise_utils::generate_heightmap(base_noise, 200, 200);
        
        // Add some variation with Worley noise for interesting features
        auto feature_noise = pcg_manager_->create_noise_generator(NoiseType::WORLEY, 54321);
        
        for (u32 y = 0; y < terrain_.height; ++y) {
            for (u32 x = 0; x < terrain_.width; ++x) {
                f32 worley_value = pcg_manager_->sample_noise_2d(feature_noise, 
                    static_cast<f32>(x) * 0.05f, static_cast<f32>(y) * 0.05f);
                terrain_(x, y) += worley_value * 0.3f;
            }
        }
        
        LOG_INFO("Terrain generated: " + std::to_string(terrain_.width) + "x" + 
                std::to_string(terrain_.height) + " heightmap");
    }
    
    void spawn_agents() {
        LOG_INFO("Spawning AI agents");
        
        // Create predator agents
        spawn_predators(5);
        
        // Create prey agents
        spawn_prey(15);
        
        // Create NPC agents
        spawn_npcs(8);
        
        LOG_INFO("Spawned " + std::to_string(predators_.size() + prey_.size() + npcs_.size()) + 
                " total agents");
    }
    
    void spawn_predators(u32 count) {
        for (u32 i = 0; i < count; ++i) {
            AgentConfig config;
            config.name = "Predator_" + std::to_string(i);
            config.type = AgentType::ENEMY;
            config.behavior_type = BehaviorType::FSM;
            config.intelligence = 0.7f + (utils::random_float() * 0.3f);
            config.aggression = 0.8f + (utils::random_float() * 0.2f);
            config.curiosity = 0.4f + (utils::random_float() * 0.3f);
            config.risk_tolerance = 0.6f + (utils::random_float() * 0.4f);
            config.update_frequency = 20.0f;
            config.decision_frequency = 5.0f;
            
            // Add vision sensor
            SensorData vision_sensor;
            vision_sensor.type = SensorData::Type::VISION;
            vision_sensor.range = 25.0f;
            vision_sensor.angle = 120.0f;
            config.sensors.push_back(vision_sensor);
            
            ecs::Entity predator = agent_manager_->create_agent(config.name, config);
            
            // Add custom components
            auto& transform = registry_.add_component<TransformComponent>(predator);
            transform.position = get_random_terrain_position();
            transform.speed = 3.0f + utils::random_float() * 2.0f;
            
            auto& health = registry_.add_component<HealthComponent>(predator);
            health.max_health = 120.0f;
            health.current_health = health.max_health;
            
            auto& resources = registry_.add_component<ResourceComponent>(predator);
            
            // Create predator behavior (FSM)
            create_predator_fsm(predator);
            
            predators_.push_back(predator);
        }
    }
    
    void spawn_prey(u32 count) {
        for (u32 i = 0; i < count; ++i) {
            AgentConfig config;
            config.name = "Prey_" + std::to_string(i);
            config.type = AgentType::BASIC;
            config.behavior_type = BehaviorType::BEHAVIOR_TREE;
            config.intelligence = 0.5f + (utils::random_float() * 0.3f);
            config.aggression = 0.1f + (utils::random_float() * 0.2f);
            config.curiosity = 0.6f + (utils::random_float() * 0.4f);
            config.risk_tolerance = 0.2f + (utils::random_float() * 0.3f);
            config.update_frequency = 30.0f;
            config.decision_frequency = 8.0f;
            
            // Add multiple sensors
            SensorData vision;
            vision.type = SensorData::Type::VISION;
            vision.range = 20.0f;
            vision.angle = 180.0f; // Wider field of view for prey
            config.sensors.push_back(vision);
            
            SensorData hearing;
            hearing.type = SensorData::Type::HEARING;
            hearing.range = 15.0f;
            config.sensors.push_back(hearing);
            
            ecs::Entity prey_entity = agent_manager_->create_agent(config.name, config);
            
            // Add custom components
            auto& transform = registry_.add_component<TransformComponent>(prey_entity);
            transform.position = get_random_terrain_position();
            transform.speed = 2.5f + utils::random_float() * 1.5f;
            
            auto& health = registry_.add_component<HealthComponent>(prey_entity);
            health.max_health = 80.0f;
            health.current_health = health.max_health;
            
            auto& resources = registry_.add_component<ResourceComponent>(prey_entity);
            
            // Create prey behavior (Behavior Tree)
            create_prey_behavior_tree(prey_entity);
            
            // Add flocking behavior
            auto& flocking = registry_.add_component<FlockingComponent>(prey_entity);
            flocking.flock_id = "prey_herd";
            flocking.separation_weight = 2.0f;
            flocking.alignment_weight = 1.5f;
            flocking.cohesion_weight = 1.0f;
            flocking.max_speed = transform.speed;
            
            prey_.push_back(prey_entity);
        }
    }
    
    void spawn_npcs(u32 count) {
        for (u32 i = 0; i < count; ++i) {
            AgentConfig config;
            config.name = "NPC_" + std::to_string(i);
            config.type = AgentType::NPC;
            config.behavior_type = BehaviorType::UTILITY_AI;
            config.intelligence = 0.6f + (utils::random_float() * 0.4f);
            config.aggression = 0.3f + (utils::random_float() * 0.3f);
            config.curiosity = 0.7f + (utils::random_float() * 0.3f);
            config.social_tendency = 0.8f + (utils::random_float() * 0.2f);
            config.update_frequency = 15.0f;
            config.decision_frequency = 3.0f;
            
            // Balanced sensors
            SensorData vision;
            vision.type = SensorData::Type::VISION;
            vision.range = 18.0f;
            vision.angle = 100.0f;
            config.sensors.push_back(vision);
            
            ecs::Entity npc = agent_manager_->create_agent(config.name, config);
            
            // Add custom components
            auto& transform = registry_.add_component<TransformComponent>(npc);
            transform.position = get_random_terrain_position();
            transform.speed = 2.0f + utils::random_float();
            
            auto& health = registry_.add_component<HealthComponent>(npc);
            health.max_health = 100.0f;
            health.current_health = health.max_health;
            
            auto& resources = registry_.add_component<ResourceComponent>(npc);
            
            // Add communication
            auto& comm = registry_.add_component<CommunicationComponent>(npc);
            comm.communication_range = 30.0f;
            comm.group_memberships.push_back("villagers");
            
            // Add emotional state
            auto& emotion = registry_.add_component<EmotionalComponent>(npc);
            emotion.happiness = 0.6f + utils::random_float() * 0.3f;
            emotion.emotional_stability = 0.7f;
            
            npcs_.push_back(npc);
        }
    }
    
    void setup_resources() {
        LOG_INFO("Placing environmental resources");
        
        // Spawn food sources
        for (u32 i = 0; i < 20; ++i) {
            ecs::Entity food_source = registry_.create_entity();
            
            auto& transform = registry_.add_component<TransformComponent>(food_source);
            transform.position = get_random_terrain_position();
            
            // Resources provide food when consumed
            auto& resource = registry_.add_component<ResourceComponent>(food_source);
            resource.food = 200.0f; // Large food source
            
            resources_.push_back(food_source);
        }
        
        // Spawn water sources
        for (u32 i = 0; i < 10; ++i) {
            ecs::Entity water_source = registry_.create_entity();
            
            auto& transform = registry_.add_component<TransformComponent>(water_source);
            transform.position = get_random_terrain_position();
            
            auto& resource = registry_.add_component<ResourceComponent>(water_source);
            resource.water = 500.0f; // Large water source
            
            resources_.push_back(water_source);
        }
        
        LOG_INFO("Placed " + std::to_string(resources_.size()) + " resource nodes");
    }
    
    Vec3 get_random_terrain_position() {
        f32 x = utils::random_float() * terrain_.width;
        f32 y = utils::random_float() * terrain_.height;
        f32 z = terrain_(static_cast<u32>(x) % terrain_.width, 
                        static_cast<u32>(y) % terrain_.height);
        return Vec3{x, y, z};
    }
    
    void create_predator_fsm(ecs::Entity predator) {
        // Build predator FSM using the builder pattern
        auto fsm = FSMBuilder("PredatorFSM")
            .add_state("patrol")
            .on_enter([this](const Blackboard& bb, ecs::Entity entity, f64) {
                LOG_DEBUG("Predator " + std::to_string(entity.id) + " started patrolling");
                this->set_random_patrol_target(entity);
            })
            .on_update([this](const Blackboard& bb, ecs::Entity entity, f64 dt) {
                this->update_patrol_behavior(entity, dt);
            })
            .transition_to("hunt")
            .when_custom([](const Blackboard& bb, ecs::Entity entity, f64) {
                // Transition to hunt when prey is detected
                if (auto target = bb.get<ecs::Entity>("detected_prey")) {
                    return target->id != 0;
                }
                return false;
            }, "prey_detected")
            
            .add_state("hunt")
            .on_enter([](const Blackboard& bb, ecs::Entity entity, f64) {
                LOG_DEBUG("Predator " + std::to_string(entity.id) + " started hunting");
            })
            .on_update([this](const Blackboard& bb, ecs::Entity entity, f64 dt) {
                this->update_hunt_behavior(entity, dt);
            })
            .transition_to("patrol")
            .when_custom([](const Blackboard& bb, ecs::Entity entity, f64) {
                // Return to patrol if no prey detected
                if (auto target = bb.get<ecs::Entity>("detected_prey")) {
                    return target->id == 0;
                }
                return true;
            }, "prey_lost")
            .after(30.0f) // Timeout after 30 seconds
            
            .add_state("rest")
            .on_enter([](const Blackboard& bb, ecs::Entity entity, f64) {
                LOG_DEBUG("Predator " + std::to_string(entity.id) + " is resting");
            })
            .transition_to("patrol")
            .after(5.0f) // Rest for 5 seconds
            
            .build();
        
        // Assign FSM to behavior component
        auto* behavior = registry_.get_component<BehaviorComponent>(predator);
        if (behavior) {
            behavior->fsm = fsm;
        }
        
        // Start the FSM
        auto* blackboard_comp = registry_.get_component<BlackboardComponent>(predator);
        if (blackboard_comp && blackboard_comp->individual_blackboard) {
            fsm->start("patrol", *blackboard_comp->individual_blackboard, predator);
        }
    }
    
    void create_prey_behavior_tree(ecs::Entity prey_entity) {
        // Build prey behavior tree
        auto bt = BehaviorTreeBuilder("PreyBehavior")
            .selector("main_behavior")
                .sequence("flee_sequence")
                    .condition("threat_detected", [this](ExecutionContext& ctx) {
                        return this->is_threat_detected(ctx.entity);
                    })
                    .action("flee_from_threat", [this](ExecutionContext& ctx) {
                        return this->flee_from_threat(ctx.entity, ctx.delta_time);
                    })
                .end()
                .sequence("forage_sequence")
                    .condition("is_hungry", [this](ExecutionContext& ctx) {
                        return this->is_agent_hungry(ctx.entity);
                    })
                    .action("find_food", [this](ExecutionContext& ctx) {
                        return this->find_and_move_to_food(ctx.entity, ctx.delta_time);
                    })
                .end()
                .sequence("social_sequence")
                    .condition("allies_nearby", [this](ExecutionContext& ctx) {
                        return this->are_allies_nearby(ctx.entity);
                    })
                    .action("join_group", [this](ExecutionContext& ctx) {
                        return this->join_nearby_group(ctx.entity, ctx.delta_time);
                    })
                .end()
                .action("wander", [this](ExecutionContext& ctx) {
                    return this->wander_randomly(ctx.entity, ctx.delta_time);
                })
            .end()
            .build();
        
        // Assign behavior tree
        auto* behavior = registry_.get_component<BehaviorComponent>(prey_entity);
        if (behavior) {
            behavior->behavior_tree = bt;
        }
    }
    
    // Behavior implementations
    void set_random_patrol_target(ecs::Entity entity) {
        auto* transform = registry_.get_component<TransformComponent>(entity);
        if (transform) {
            Vec3 random_target = get_random_terrain_position();
            auto* blackboard_comp = registry_.get_component<BlackboardComponent>(entity);
            if (blackboard_comp && blackboard_comp->individual_blackboard) {
                blackboard_comp->individual_blackboard->set("patrol_target", random_target);
            }
        }
    }
    
    void update_patrol_behavior(ecs::Entity entity, f64 delta_time) {
        auto* transform = registry_.get_component<TransformComponent>(entity);
        auto* blackboard_comp = registry_.get_component<BlackboardComponent>(entity);
        
        if (!transform || !blackboard_comp || !blackboard_comp->individual_blackboard) return;
        
        auto target = blackboard_comp->individual_blackboard->get<Vec3>("patrol_target");
        if (target) {
            Vec3 direction = normalize(*target - transform->position);
            transform->velocity = direction * transform->speed;
            transform->position = transform->position + transform->velocity * static_cast<f32>(delta_time);
            
            // Check if reached target
            if (length(*target - transform->position) < 2.0f) {
                set_random_patrol_target(entity);
            }
        }
    }
    
    void update_hunt_behavior(ecs::Entity entity, f64 delta_time) {
        auto* transform = registry_.get_component<TransformComponent>(entity);
        auto* sensor = registry_.get_component<SensorComponent>(entity);
        
        if (!transform || !sensor) return;
        
        // Find nearest prey
        ecs::Entity nearest_prey = ecs::Entity::invalid();
        f32 nearest_distance = std::numeric_limits<f32>::max();
        
        for (auto detected : sensor->detected_entities) {
            auto* detected_agent = registry_.get_component<AIAgentComponent>(detected);
            if (detected_agent && detected_agent->type == AgentType::BASIC) { // Prey
                f32 distance = sensor->entity_distances[detected];
                if (distance < nearest_distance) {
                    nearest_distance = distance;
                    nearest_prey = detected;
                }
            }
        }
        
        if (nearest_prey.id != 0) {
            // Chase prey
            auto* prey_transform = registry_.get_component<TransformComponent>(nearest_prey);
            if (prey_transform) {
                Vec3 direction = normalize(prey_transform->position - transform->position);
                transform->velocity = direction * (transform->speed * 1.5f); // Hunt faster
                transform->position = transform->position + transform->velocity * static_cast<f32>(delta_time);
            }
        }
    }
    
    bool is_threat_detected(ecs::Entity entity) {
        auto* sensor = registry_.get_component<SensorComponent>(entity);
        return sensor && !sensor->detected_threats.empty();
    }
    
    NodeStatus flee_from_threat(ecs::Entity entity, f64 delta_time) {
        auto* transform = registry_.get_component<TransformComponent>(entity);
        auto* sensor = registry_.get_component<SensorComponent>(entity);
        
        if (!transform || !sensor || sensor->detected_threats.empty()) {
            return NodeStatus::FAILURE;
        }
        
        // Flee from nearest threat
        auto nearest_threat = sensor->nearest_threat;
        auto* threat_transform = registry_.get_component<TransformComponent>(nearest_threat);
        
        if (threat_transform) {
            Vec3 flee_direction = normalize(transform->position - threat_transform->position);
            transform->velocity = flee_direction * (transform->speed * 2.0f); // Flee fast
            transform->position = transform->position + transform->velocity * static_cast<f32>(delta_time);
            return NodeStatus::RUNNING;
        }
        
        return NodeStatus::FAILURE;
    }
    
    bool is_agent_hungry(ecs::Entity entity) {
        auto* resources = registry_.get_component<ResourceComponent>(entity);
        return resources && resources->is_hungry();
    }
    
    NodeStatus find_and_move_to_food(ecs::Entity entity, f64 delta_time) {
        auto* transform = registry_.get_component<TransformComponent>(entity);
        if (!transform) return NodeStatus::FAILURE;
        
        // Find nearest food resource
        ecs::Entity nearest_food = ecs::Entity::invalid();
        f32 nearest_distance = std::numeric_limits<f32>::max();
        
        for (auto resource_entity : resources_) {
            auto* resource_transform = registry_.get_component<TransformComponent>(resource_entity);
            auto* resource = registry_.get_component<ResourceComponent>(resource_entity);
            
            if (resource_transform && resource && resource->food > 0) {
                f32 distance = length(resource_transform->position - transform->position);
                if (distance < nearest_distance) {
                    nearest_distance = distance;
                    nearest_food = resource_entity;
                }
            }
        }
        
        if (nearest_food.id != 0) {
            auto* food_transform = registry_.get_component<TransformComponent>(nearest_food);
            if (food_transform) {
                Vec3 direction = normalize(food_transform->position - transform->position);
                transform->velocity = direction * transform->speed;
                transform->position = transform->position + transform->velocity * static_cast<f32>(delta_time);
                
                // Consume food if close enough
                if (nearest_distance < 2.0f) {
                    auto* agent_resources = registry_.get_component<ResourceComponent>(entity);
                    auto* food_resources = registry_.get_component<ResourceComponent>(nearest_food);
                    
                    if (agent_resources && food_resources) {
                        f32 consumed = std::min(10.0f, food_resources->food);
                        agent_resources->food = std::min(agent_resources->max_food, 
                                                       agent_resources->food + consumed);
                        food_resources->food -= consumed;
                        
                        return NodeStatus::SUCCESS;
                    }
                }
                
                return NodeStatus::RUNNING;
            }
        }
        
        return NodeStatus::FAILURE;
    }
    
    bool are_allies_nearby(ecs::Entity entity) {
        auto* sensor = registry_.get_component<SensorComponent>(entity);
        return sensor && !sensor->detected_allies.empty();
    }
    
    NodeStatus join_nearby_group(ecs::Entity entity, f64 delta_time) {
        auto* transform = registry_.get_component<TransformComponent>(entity);
        auto* sensor = registry_.get_component<SensorComponent>(entity);
        
        if (!transform || !sensor || sensor->detected_allies.empty()) {
            return NodeStatus::FAILURE;
        }
        
        // Move toward center of nearby allies
        Vec3 group_center{0, 0, 0};
        u32 ally_count = 0;
        
        for (auto ally : sensor->detected_allies) {
            auto ally_pos = sensor->entity_positions[ally];
            group_center = group_center + ally_pos;
            ally_count++;
        }
        
        if (ally_count > 0) {
            group_center = group_center * (1.0f / ally_count);
            Vec3 direction = normalize(group_center - transform->position);
            transform->velocity = direction * transform->speed * 0.8f; // Move slower when grouping
            transform->position = transform->position + transform->velocity * static_cast<f32>(delta_time);
            
            return NodeStatus::SUCCESS;
        }
        
        return NodeStatus::FAILURE;
    }
    
    NodeStatus wander_randomly(ecs::Entity entity, f64 delta_time) {
        auto* transform = registry_.get_component<TransformComponent>(entity);
        if (!transform) return NodeStatus::FAILURE;
        
        // Simple random walk
        static thread_local std::mt19937 rng(std::random_device{}());
        std::uniform_real_distribution<f32> angle_dist(0, 2 * 3.14159f);
        
        f32 random_angle = angle_dist(rng);
        Vec3 random_direction{std::cos(random_angle), std::sin(random_angle), 0};
        
        transform->velocity = random_direction * (transform->speed * 0.5f);
        transform->position = transform->position + transform->velocity * static_cast<f32>(delta_time);
        
        // Keep within terrain bounds
        transform->position.x = std::clamp(transform->position.x, 0.0f, static_cast<f32>(terrain_.width));
        transform->position.y = std::clamp(transform->position.y, 0.0f, static_cast<f32>(terrain_.height));
        
        return NodeStatus::RUNNING;
    }
    
    void update_world(f64 delta_time) {
        // Update resource consumption
        auto resource_query = registry_.query<ResourceComponent>();
        for (auto entity : resource_query.entities()) {
            auto* resources = registry_.get_component<ResourceComponent>(entity);
            if (resources) {
                resources->consume_resources(static_cast<f32>(delta_time));
            }
        }
        
        // Update health regeneration
        auto health_query = registry_.query<HealthComponent>();
        for (auto entity : health_query.entities()) {
            auto* health = registry_.get_component<HealthComponent>(entity);
            if (health && health->is_alive) {
                health->heal(health->regeneration_rate * static_cast<f32>(delta_time));
            }
        }
        
        // Update ECS systems
        registry_.update_systems(delta_time);
    }
    
    void print_simulation_statistics(f64 elapsed_time) {
        LOG_INFO("=== Simulation Statistics (t=" + std::to_string(static_cast<i32>(elapsed_time)) + "s) ===");
        
        // Agent statistics
        auto agent_stats = agent_manager_->get_all_agent_stats();
        usize active_agents = 0;
        f64 avg_frame_time = 0.0;
        
        for (const auto& stats : agent_stats) {
            if (stats.current_state != AIState::INACTIVE) {
                active_agents++;
                avg_frame_time += stats.average_frame_time_ms;
            }
        }
        
        if (active_agents > 0) {
            avg_frame_time /= active_agents;
        }
        
        LOG_INFO("Active agents: " + std::to_string(active_agents) + "/" + std::to_string(agent_stats.size()));
        LOG_INFO("Average frame time: " + std::to_string(avg_frame_time) + "ms");
        
        // AI framework status
        auto framework_status = ai::get_status();
        LOG_INFO("Framework memory usage: " + std::to_string(framework_status.memory_usage_bytes / 1024) + "KB");
        LOG_INFO("Active behaviors: " + std::to_string(framework_status.active_behaviors));
        
        // Resource statistics
        u32 hungry_agents = 0;
        u32 healthy_agents = 0;
        
        auto resource_query = registry_.query<AIAgentComponent, ResourceComponent, HealthComponent>();
        for (auto entity : resource_query.entities()) {
            auto* resources = registry_.get_component<ResourceComponent>(entity);
            auto* health = registry_.get_component<HealthComponent>(entity);
            
            if (resources && resources->is_hungry()) hungry_agents++;
            if (health && health->health_percentage() > 0.8f) healthy_agents++;
        }
        
        LOG_INFO("Hungry agents: " + std::to_string(hungry_agents));
        LOG_INFO("Healthy agents: " + std::to_string(healthy_agents));
        
        LOG_INFO("==========================================");
    }
    
    void print_final_statistics() {
        LOG_INFO("=== Final Simulation Results ===");
        
        auto framework_status = ai::get_status();
        LOG_INFO("Total agents processed: " + std::to_string(framework_status.active_agents));
        LOG_INFO("Total behaviors executed: " + std::to_string(framework_status.active_behaviors));
        LOG_INFO("Final memory usage: " + std::to_string(framework_status.memory_usage_bytes / 1024) + "KB");
        LOG_INFO("Average frame time: " + std::to_string(framework_status.average_frame_time_ms) + "ms");
        
        // Survival statistics
        u32 survivors = 0;
        auto health_query = registry_.query<HealthComponent>();
        for (auto entity : health_query.entities()) {
            auto* health = registry_.get_component<HealthComponent>(entity);
            if (health && health->is_alive) {
                survivors++;
            }
        }
        
        LOG_INFO("Agents survived: " + std::to_string(survivors) + "/" + 
                std::to_string(health_query.entities().size()));
        
        LOG_INFO("================================");
    }
};

/**
 * @brief Main entry point for the AI Framework Demo
 */
int main() {
    try {
        LOG_INFO("ECScope AI Framework Demo Starting");
        
        // Create and run the demo world
        AIDemoWorld demo_world;
        demo_world.run_simulation(120.0f); // Run for 2 minutes
        
        // Shutdown AI framework
        ai::shutdown();
        
        LOG_INFO("AI Framework Demo Completed Successfully");
        return 0;
        
    } catch (const std::exception& e) {
        LOG_ERROR("Demo failed with exception: " + std::string(e.what()));
        return 1;
    } catch (...) {
        LOG_ERROR("Demo failed with unknown exception");
        return 1;
    }
}