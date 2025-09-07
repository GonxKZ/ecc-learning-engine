/**
 * @file examples/plugins/advanced_system_plugin.cpp
 * @brief Educational Advanced System Plugin Example
 * 
 * This plugin demonstrates advanced ECS system development including:
 * - Multi-threaded system processing
 * - System dependencies and execution order
 * - Performance monitoring and optimization
 * - Advanced event handling and communication
 * - Educational profiling and analysis tools
 * 
 * Learning Objectives:
 * - Understanding advanced ECS system architecture
 * - Multi-threaded game system design
 * - Performance optimization techniques
 * - System interdependencies management
 * - Real-time debugging and profiling
 * 
 * @author ECScope Educational ECS Framework - Plugin System
 * @date 2024
 */

#include "plugin/plugin_api.hpp"
#include "plugin/plugin_core.hpp"
#include "ecs/component.hpp"
#include "ecs/system.hpp"
#include <memory>
#include <string>
#include <vector>
#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <future>
#include <chrono>

using namespace ecscope;
using namespace ecscope::plugin;

//=============================================================================
// Advanced Components for System Demonstration
//=============================================================================

/**
 * @brief Position component with educational features
 */
struct PositionComponent : public ecs::Component<PositionComponent> {
    f32 x{0.0f}, y{0.0f}, z{0.0f};
    f32 previous_x{0.0f}, previous_y{0.0f}, previous_z{0.0f};
    
    PositionComponent() = default;
    PositionComponent(f32 px, f32 py, f32 pz = 0.0f) : x(px), y(py), z(pz) {
        previous_x = x; previous_y = y; previous_z = z;
    }
    
    void update_position(f32 dx, f32 dy, f32 dz = 0.0f) {
        previous_x = x; previous_y = y; previous_z = z;
        x += dx; y += dy; z += dz;
    }
    
    f32 distance_moved() const {
        f32 dx = x - previous_x;
        f32 dy = y - previous_y; 
        f32 dz = z - previous_z;
        return std::sqrt(dx*dx + dy*dy + dz*dz);
    }
    
    std::string to_string() const {
        return "Position(" + std::to_string(x) + ", " + std::to_string(y) + ", " + std::to_string(z) + ")";
    }
};

/**
 * @brief Velocity component with advanced physics
 */
struct VelocityComponent : public ecs::Component<VelocityComponent> {
    f32 vx{0.0f}, vy{0.0f}, vz{0.0f};
    f32 max_speed{10.0f};
    f32 drag{0.98f};
    
    VelocityComponent() = default;
    VelocityComponent(f32 velocity_x, f32 velocity_y, f32 velocity_z = 0.0f)
        : vx(velocity_x), vy(velocity_y), vz(velocity_z) {}
    
    void apply_force(f32 fx, f32 fy, f32 fz = 0.0f) {
        vx += fx; vy += fy; vz += fz;
        clamp_velocity();
    }
    
    void apply_drag() {
        vx *= drag; vy *= drag; vz *= drag;
    }
    
    f32 get_speed() const {
        return std::sqrt(vx*vx + vy*vy + vz*vz);
    }
    
private:
    void clamp_velocity() {
        f32 speed = get_speed();
        if (speed > max_speed) {
            f32 scale = max_speed / speed;
            vx *= scale; vy *= scale; vz *= scale;
        }
    }
};

/**
 * @brief AI behavior component with state machine
 */
struct AIBehaviorComponent : public ecs::Component<AIBehaviorComponent> {
    enum class State {
        Idle, Seeking, Fleeing, Patrolling, Attacking
    };
    
    State current_state{State::Idle};
    State previous_state{State::Idle};
    ecs::Entity target_entity{0};
    f32 detection_range{5.0f};
    f32 attack_range{2.0f};
    f32 state_timer{0.0f};
    
    std::vector<std::pair<f32, f32>> patrol_points;
    usize current_patrol_index{0};
    
    AIBehaviorComponent() = default;
    
    void change_state(State new_state) {
        if (new_state != current_state) {
            previous_state = current_state;
            current_state = new_state;
            state_timer = 0.0f;
        }
    }
    
    void add_patrol_point(f32 x, f32 y) {
        patrol_points.emplace_back(x, y);
    }
    
    std::string get_state_name() const {
        switch (current_state) {
            case State::Idle: return "Idle";
            case State::Seeking: return "Seeking";
            case State::Fleeing: return "Fleeing";
            case State::Patrolling: return "Patrolling";
            case State::Attacking: return "Attacking";
            default: return "Unknown";
        }
    }
};

//=============================================================================
// Advanced Systems Implementation
//=============================================================================

/**
 * @brief Multi-threaded movement system with educational profiling
 */
class MovementSystem {
private:
    PluginAPI* api_;
    std::atomic<bool> is_running_{false};
    std::atomic<u32> entities_processed_{0};
    std::atomic<f64> total_processing_time_{0.0};
    
    // Threading
    u32 thread_count_{std::thread::hardware_concurrency()};
    std::vector<std::thread> worker_threads_;
    std::queue<ecs::Entity> entity_queue_;
    std::mutex queue_mutex_;
    std::condition_variable queue_cv_;
    std::atomic<bool> shutdown_requested_{false};

public:
    explicit MovementSystem(PluginAPI* api) : api_(api) {}
    
    ~MovementSystem() {
        shutdown();
    }
    
    void initialize() {
        is_running_ = true;
        
        // Start worker threads for parallel processing
        for (u32 i = 0; i < thread_count_; ++i) {
            worker_threads_.emplace_back(&MovementSystem::worker_thread, this, i);
        }
        
        api_->add_learning_note("Multi-threaded systems can process entities in parallel for better performance");
        LOG_INFO("MovementSystem initialized with {} worker threads", thread_count_);
    }
    
    void shutdown() {
        if (is_running_) {
            shutdown_requested_ = true;
            queue_cv_.notify_all();
            
            for (auto& thread : worker_threads_) {
                if (thread.joinable()) {
                    thread.join();
                }
            }
            
            worker_threads_.clear();
            is_running_ = false;
            
            LOG_INFO("MovementSystem shutdown complete");
        }
    }
    
    void update(f64 delta_time) {
        auto timer = api_->start_timer("MovementSystem::update");
        
        // Query entities with position and velocity components
        auto entities = api_->get_ecs().query_entities<PositionComponent, VelocityComponent>();
        
        if (entities.empty()) return;
        
        // Distribute entities to worker threads
        {
            std::lock_guard<std::mutex> lock(queue_mutex_);
            for (auto entity : entities) {
                entity_queue_.push(entity);
            }
        }
        queue_cv_.notify_all();
        
        // Wait for processing to complete (simple synchronization)
        // In a real system, you'd use more sophisticated synchronization
        std::this_thread::sleep_for(std::chrono::microseconds(100));
        
        // Educational: Log performance metrics
        if (api_->get_config("enable_movement_profiling") == "true") {
            api_->record_performance_metric("entities_per_frame", static_cast<f64>(entities.size()));
            api_->record_performance_metric("movement_update_time_ms", delta_time);
        }
    }
    
    u32 get_entities_processed() const { return entities_processed_.load(); }
    f64 get_average_processing_time() const { 
        return entities_processed_ > 0 ? total_processing_time_ / entities_processed_ : 0.0;
    }

private:
    void worker_thread(u32 thread_id) {
        LOG_INFO("MovementSystem worker thread {} started", thread_id);
        
        while (!shutdown_requested_) {
            ecs::Entity entity = 0;
            
            // Get entity from queue
            {
                std::unique_lock<std::mutex> lock(queue_mutex_);
                queue_cv_.wait(lock, [this] { return !entity_queue_.empty() || shutdown_requested_; });
                
                if (shutdown_requested_) break;
                
                if (!entity_queue_.empty()) {
                    entity = entity_queue_.front();
                    entity_queue_.pop();
                } else {
                    continue;
                }
            }
            
            // Process entity
            process_entity(entity, thread_id);
        }
        
        LOG_INFO("MovementSystem worker thread {} stopped", thread_id);
    }
    
    void process_entity(ecs::Entity entity, u32 thread_id) {
        auto start_time = std::chrono::high_resolution_clock::now();
        
        auto* position = api_->get_ecs().get_component<PositionComponent>(entity);
        auto* velocity = api_->get_ecs().get_component<VelocityComponent>(entity);
        
        if (position && velocity) {
            // Apply velocity to position
            f32 dt = 1.0f / 60.0f; // Assuming 60 FPS for simplicity
            position->update_position(velocity->vx * dt, velocity->vy * dt, velocity->vz * dt);
            
            // Apply drag
            velocity->apply_drag();
            
            entities_processed_++;
            
            // Educational: Track processing time
            auto end_time = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration<f64, std::micro>(end_time - start_time).count();
            total_processing_time_ += duration;
        }
    }
};

/**
 * @brief AI behavior system with educational state machine
 */
class AIBehaviorSystem {
private:
    PluginAPI* api_;
    std::unordered_map<ecs::Entity, f64> entity_timers_;
    u32 ai_updates_per_second_{10}; // Update AI less frequently than movement

public:
    explicit AIBehaviorSystem(PluginAPI* api) : api_(api) {}
    
    void initialize() {
        api_->add_learning_note("AI systems often run at lower frequencies than physics for performance");
        api_->add_learning_note("State machines provide clear behavior transitions and debugging");
        LOG_INFO("AIBehaviorSystem initialized");
    }
    
    void update(f64 delta_time) {
        auto timer = api_->start_timer("AIBehaviorSystem::update");
        
        // Update AI at reduced frequency
        static f64 ai_accumulator = 0.0;
        ai_accumulator += delta_time;
        
        f64 ai_timestep = 1.0 / ai_updates_per_second_;
        if (ai_accumulator < ai_timestep) return;
        
        ai_accumulator -= ai_timestep;
        
        // Process AI entities
        auto entities = api_->get_ecs().query_entities<AIBehaviorComponent, PositionComponent>();
        
        for (auto entity : entities) {
            process_ai_entity(entity, ai_timestep);
        }
        
        // Educational metrics
        api_->record_performance_metric("ai_entities_count", static_cast<f64>(entities.size()));
    }

private:
    void process_ai_entity(ecs::Entity entity, f64 delta_time) {
        auto* ai = api_->get_ecs().get_component<AIBehaviorComponent>(entity);
        auto* position = api_->get_ecs().get_component<PositionComponent>(entity);
        
        if (!ai || !position) return;
        
        ai->state_timer += static_cast<f32>(delta_time);
        
        switch (ai->current_state) {
            case AIBehaviorComponent::State::Idle:
                process_idle_state(entity, ai, position);
                break;
                
            case AIBehaviorComponent::State::Patrolling:
                process_patrolling_state(entity, ai, position);
                break;
                
            case AIBehaviorComponent::State::Seeking:
                process_seeking_state(entity, ai, position);
                break;
                
            case AIBehaviorComponent::State::Fleeing:
                process_fleeing_state(entity, ai, position);
                break;
                
            case AIBehaviorComponent::State::Attacking:
                process_attacking_state(entity, ai, position);
                break;
        }
        
        // Educational: Log state changes
        if (ai->current_state != ai->previous_state && 
            api_->get_config("log_ai_state_changes") == "true") {
            LOG_INFO("Entity {} AI state: {} -> {}", entity, 
                    get_state_name(ai->previous_state), ai->get_state_name());
        }
    }
    
    void process_idle_state(ecs::Entity entity, AIBehaviorComponent* ai, PositionComponent* position) {
        // After some time in idle, start patrolling
        if (ai->state_timer > 2.0f && !ai->patrol_points.empty()) {
            ai->change_state(AIBehaviorComponent::State::Patrolling);
        }
    }
    
    void process_patrolling_state(ecs::Entity entity, AIBehaviorComponent* ai, PositionComponent* position) {
        if (ai->patrol_points.empty()) {
            ai->change_state(AIBehaviorComponent::State::Idle);
            return;
        }
        
        // Move towards current patrol point
        auto [target_x, target_y] = ai->patrol_points[ai->current_patrol_index];
        f32 dx = target_x - position->x;
        f32 dy = target_y - position->y;
        f32 distance = std::sqrt(dx*dx + dy*dy);
        
        if (distance < 1.0f) {
            // Reached patrol point, move to next
            ai->current_patrol_index = (ai->current_patrol_index + 1) % ai->patrol_points.size();
        } else {
            // Apply movement towards patrol point
            auto* velocity = api_->get_ecs().get_component<VelocityComponent>(entity);
            if (velocity) {
                f32 move_speed = 2.0f;
                velocity->apply_force((dx/distance) * move_speed * 0.1f, (dy/distance) * move_speed * 0.1f);
            }
        }
    }
    
    void process_seeking_state(ecs::Entity entity, AIBehaviorComponent* ai, PositionComponent* position) {
        // Check if target still exists
        if (!api_->get_ecs().get_component<PositionComponent>(ai->target_entity)) {
            ai->change_state(AIBehaviorComponent::State::Patrolling);
            return;
        }
        
        auto* target_pos = api_->get_ecs().get_component<PositionComponent>(ai->target_entity);
        f32 dx = target_pos->x - position->x;
        f32 dy = target_pos->y - position->y;
        f32 distance = std::sqrt(dx*dx + dy*dy);
        
        if (distance <= ai->attack_range) {
            ai->change_state(AIBehaviorComponent::State::Attacking);
        } else if (distance > ai->detection_range * 1.5f) {
            // Lost target
            ai->change_state(AIBehaviorComponent::State::Patrolling);
        } else {
            // Move towards target
            auto* velocity = api_->get_ecs().get_component<VelocityComponent>(entity);
            if (velocity) {
                f32 seek_speed = 4.0f;
                velocity->apply_force((dx/distance) * seek_speed * 0.1f, (dy/distance) * seek_speed * 0.1f);
            }
        }
    }
    
    void process_fleeing_state(ecs::Entity entity, AIBehaviorComponent* ai, PositionComponent* position) {
        // Flee for a limited time, then return to patrolling
        if (ai->state_timer > 5.0f) {
            ai->change_state(AIBehaviorComponent::State::Patrolling);
            return;
        }
        
        if (!api_->get_ecs().get_component<PositionComponent>(ai->target_entity)) {
            ai->change_state(AIBehaviorComponent::State::Patrolling);
            return;
        }
        
        auto* target_pos = api_->get_ecs().get_component<PositionComponent>(ai->target_entity);
        f32 dx = position->x - target_pos->x; // Reverse direction for fleeing
        f32 dy = position->y - target_pos->y;
        f32 distance = std::sqrt(dx*dx + dy*dy);
        
        if (distance < 0.1f) {
            // Avoid division by zero
            dx = 1.0f; dy = 0.0f; distance = 1.0f;
        }
        
        auto* velocity = api_->get_ecs().get_component<VelocityComponent>(entity);
        if (velocity) {
            f32 flee_speed = 6.0f;
            velocity->apply_force((dx/distance) * flee_speed * 0.1f, (dy/distance) * flee_speed * 0.1f);
        }
    }
    
    void process_attacking_state(ecs::Entity entity, AIBehaviorComponent* ai, PositionComponent* position) {
        if (!api_->get_ecs().get_component<PositionComponent>(ai->target_entity)) {
            ai->change_state(AIBehaviorComponent::State::Patrolling);
            return;
        }
        
        auto* target_pos = api_->get_ecs().get_component<PositionComponent>(ai->target_entity);
        f32 dx = target_pos->x - position->x;
        f32 dy = target_pos->y - position->y;
        f32 distance = std::sqrt(dx*dx + dy*dy);
        
        if (distance > ai->attack_range) {
            ai->change_state(AIBehaviorComponent::State::Seeking);
        } else {
            // Attack behavior (educational placeholder)
            if (api_->get_config("log_ai_attacks") == "true") {
                LOG_INFO("Entity {} attacking target {}", entity, ai->target_entity);
            }
        }
    }
    
    std::string get_state_name(AIBehaviorComponent::State state) const {
        switch (state) {
            case AIBehaviorComponent::State::Idle: return "Idle";
            case AIBehaviorComponent::State::Seeking: return "Seeking";
            case AIBehaviorComponent::State::Fleeing: return "Fleeing";
            case AIBehaviorComponent::State::Patrolling: return "Patrolling";
            case AIBehaviorComponent::State::Attacking: return "Attacking";
            default: return "Unknown";
        }
    }
};

//=============================================================================
// Advanced System Plugin Implementation
//=============================================================================

/**
 * @brief Advanced System Plugin demonstrating complex ECS system patterns
 */
class AdvancedSystemPlugin : public IPlugin {
private:
    PluginMetadata metadata_;
    std::unique_ptr<PluginAPI> api_;
    PluginStats stats_;
    
    // Systems
    std::unique_ptr<MovementSystem> movement_system_;
    std::unique_ptr<AIBehaviorSystem> ai_system_;
    
    // Educational tracking
    std::atomic<u32> systems_updated_{0};
    std::atomic<f64> total_system_time_{0.0};
    
public:
    AdvancedSystemPlugin() {
        initialize_metadata();
    }
    
    const PluginMetadata& get_metadata() const override {
        return metadata_;
    }
    
    bool initialize() override {
        if (!api_) {
            LOG_ERROR("Plugin API not available during initialization");
            return false;
        }
        
        // Register components
        if (!register_components()) {
            LOG_ERROR("Failed to register plugin components");
            return false;
        }
        
        // Initialize systems
        movement_system_ = std::make_unique<MovementSystem>(api_.get());
        ai_system_ = std::make_unique<AIBehaviorSystem>(api_.get());
        
        movement_system_->initialize();
        ai_system_->initialize();
        
        // Register systems with ECS
        if (!register_systems()) {
            LOG_ERROR("Failed to register plugin systems");
            return false;
        }
        
        // Add educational content
        setup_educational_content();
        
        LOG_INFO("AdvancedSystemPlugin initialized successfully");
        return true;
    }
    
    void shutdown() override {
        LOG_INFO("AdvancedSystemPlugin shutting down");
        
        if (movement_system_) {
            movement_system_->shutdown();
        }
        
        LOG_INFO("Systems updated {} times with total time: {}ms", 
                systems_updated_.load(), total_system_time_.load());
    }
    
    void update(f64 delta_time) override {
        auto start_time = std::chrono::high_resolution_clock::now();
        
        // Update systems in priority order
        if (movement_system_) {
            movement_system_->update(delta_time);
        }
        
        if (ai_system_) {
            ai_system_->update(delta_time);
        }
        
        // Track performance
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration<f64, std::milli>(end_time - start_time).count();
        
        systems_updated_++;
        total_system_time_ += duration;
        
        stats_.last_activity = std::chrono::system_clock::now();
        stats_.average_frame_time_ms = delta_time;
    }
    
    void handle_event(const PluginEvent& event) override {
        switch (event.type) {
            case PluginEventType::EntityCreated:
                handle_entity_created(event);
                break;
            case PluginEventType::ConfigurationChanged:
                handle_configuration_changed(event);
                break;
            default:
                break;
        }
    }
    
    std::unordered_map<std::string, std::string> get_config() const override {
        return {
            {"enable_movement_profiling", "true"},
            {"log_ai_state_changes", "true"},
            {"log_ai_attacks", "false"},
            {"ai_updates_per_second", "10"},
            {"movement_worker_threads", std::to_string(std::thread::hardware_concurrency())},
            {"enable_educational_logging", "true"}
        };
    }
    
    void set_config(const std::unordered_map<std::string, std::string>& config) override {
        for (const auto& [key, value] : config) {
            api_->set_config(key, value);
        }
    }
    
    bool validate() const override {
        return movement_system_ && ai_system_;
    }
    
    PluginStats get_stats() const override {
        auto current_stats = stats_;
        current_stats.total_function_calls = systems_updated_.load();
        current_stats.total_cpu_time_ms = total_system_time_.load();
        return current_stats;
    }
    
    std::string explain_functionality() const override {
        return R"(
=== Advanced System Plugin Educational Overview ===

This plugin demonstrates advanced ECS system development patterns and techniques.

Key Concepts Demonstrated:
1. Multi-threaded System Processing - Parallel entity processing for performance
2. System Dependencies - Managing execution order and interdependencies  
3. Advanced AI State Machines - Complex behavior with state transitions
4. Performance Monitoring - Real-time profiling and optimization metrics
5. Educational Debugging - Comprehensive logging and analysis tools

Systems Provided:
• MovementSystem - Multi-threaded position/velocity processing with worker threads
• AIBehaviorSystem - Intelligent behavior with state machine patterns

Advanced Patterns Shown:
- Thread-safe entity processing with work queues
- Reduced-frequency AI updates for performance optimization
- State machine patterns for complex behavior trees
- Performance metrics collection and analysis
- Educational profiling and debugging tools

This plugin serves as a reference for high-performance system architecture
and demonstrates production-ready patterns for complex game systems.
        )";
    }
    
    std::vector<std::string> get_learning_resources() const override {
        return {
            "Multi-threaded ECS System Design",
            "Performance Optimization Techniques", 
            "AI State Machine Patterns",
            "System Dependencies and Execution Order",
            "Profiling and Performance Analysis",
            "Thread-Safe Component Access",
            "Advanced Debugging Techniques"
        };
    }
    
    void set_api(std::unique_ptr<PluginAPI> api) {
        api_ = std::move(api);
    }

private:
    void initialize_metadata() {
        metadata_.name = "AdvancedSystemPlugin";
        metadata_.display_name = "Advanced System Examples";
        metadata_.description = "Educational plugin demonstrating advanced ECS system development";
        metadata_.version = PluginVersion(1, 0, 0);
        metadata_.author = "ECScope Educational Framework";
        metadata_.license = "MIT";
        metadata_.category = PluginCategory::Educational;
        metadata_.priority = PluginPriority::High;
        
        metadata_.is_educational = true;
        metadata_.educational_purpose = "Demonstrate advanced ECS system development patterns";
        metadata_.learning_objectives = {
            "Understand multi-threaded system design",
            "Learn performance optimization techniques", 
            "Master AI state machine patterns",
            "Practice system dependencies management"
        };
        metadata_.difficulty_level = "advanced";
    }
    
    bool register_components() {
        auto& ecs = api_->get_ecs();
        
        if (!ecs.register_component<PositionComponent>("PositionComponent",
            "3D position component with movement tracking", true)) {
            return false;
        }
        
        if (!ecs.register_component<VelocityComponent>("VelocityComponent", 
            "Velocity component with physics integration", true)) {
            return false;
        }
        
        if (!ecs.register_component<AIBehaviorComponent>("AIBehaviorComponent",
            "AI behavior component with state machine", true)) {
            return false;
        }
        
        return true;
    }
    
    bool register_systems() {
        auto& registry = api_->get_registry();
        
        // Register movement system
        auto movement_update = [this](ecs::Registry& reg, f64 delta_time) {
            if (movement_system_) {
                movement_system_->update(delta_time);
            }
        };
        
        if (!registry.register_system_functions("MovementSystem", metadata_.name,
            movement_update, nullptr, nullptr, "Multi-threaded movement processing system",
            PluginPriority::High)) {
            return false;
        }
        
        // Register AI system
        auto ai_update = [this](ecs::Registry& reg, f64 delta_time) {
            if (ai_system_) {
                ai_system_->update(delta_time);
            }
        };
        
        if (!registry.register_system_functions("AIBehaviorSystem", metadata_.name,
            ai_update, nullptr, nullptr, "Advanced AI behavior system with state machines",
            PluginPriority::Normal)) {
            return false;
        }
        
        return true;
    }
    
    void setup_educational_content() {
        api_->add_learning_note("Multi-threaded systems require careful synchronization");
        api_->add_learning_note("AI systems benefit from reduced update frequencies");
        api_->add_learning_note("State machines provide predictable behavior patterns");
        api_->add_learning_note("Performance profiling is essential for optimization");
        
        api_->explain_concept("Thread Pool Pattern",
            "Using worker threads to process entities in parallel improves performance "
            "but requires careful synchronization to avoid race conditions.");
            
        api_->explain_concept("System Dependencies",
            "Some systems must run before others (e.g., AI before physics). "
            "Use priority levels and execution phases to manage dependencies.");
            
        api_->add_code_example("Creating AI Entity", R"cpp(
// Create entity with AI components
auto ai_entity = api.get_ecs().create_entity<PositionComponent, VelocityComponent, AIBehaviorComponent>(
    PositionComponent(0, 0, 0),
    VelocityComponent(0, 0, 0), 
    AIBehaviorComponent()
);

// Configure AI behavior
auto* ai = api.get_ecs().get_component<AIBehaviorComponent>(ai_entity);
if (ai) {
    ai->add_patrol_point(10, 10);
    ai->add_patrol_point(-10, 10);
    ai->add_patrol_point(-10, -10);
    ai->add_patrol_point(10, -10);
    ai->change_state(AIBehaviorComponent::State::Patrolling);
}
        )cpp");
    }
    
    void handle_entity_created(const PluginEvent& event) {
        // Optionally initialize new entities with default components
        stats_.total_events_handled++;
    }
    
    void handle_configuration_changed(const PluginEvent& event) {
        // Reconfigure systems based on new settings
        LOG_INFO("Reconfiguring systems based on updated configuration");
    }
};

//=============================================================================
// Plugin Entry Points
//=============================================================================

extern "C" {

PLUGIN_EXPORT IPlugin* create_plugin() {
    try {
        return new AdvancedSystemPlugin();
    } catch (const std::exception& e) {
        LOG_ERROR("Failed to create AdvancedSystemPlugin: {}", e.what());
        return nullptr;
    }
}

PLUGIN_EXPORT void destroy_plugin(IPlugin* plugin) {
    if (plugin) {
        delete plugin;
    }
}

PLUGIN_EXPORT const char* get_plugin_info() {
    return R"json({
        "name": "AdvancedSystemPlugin",
        "display_name": "Advanced System Examples",
        "description": "Educational plugin demonstrating advanced ECS system development",
        "version": "1.0.0",
        "author": "ECScope Educational Framework",
        "license": "MIT",
        "category": "Educational",
        "is_educational": true,
        "difficulty_level": "advanced",
        "learning_objectives": [
            "Understand multi-threaded system design",
            "Learn performance optimization techniques",
            "Master AI state machine patterns", 
            "Practice system dependencies management"
        ],
        "components": [
            "PositionComponent",
            "VelocityComponent",
            "AIBehaviorComponent"
        ],
        "systems": [
            "MovementSystem",
            "AIBehaviorSystem"
        ],
        "min_engine_version": "1.0.0",
        "supported_platforms": ["Windows", "Linux", "macOS"]
    })json";
}

PLUGIN_EXPORT u32 get_plugin_version() {
    return PLUGIN_API_VERSION;
}

PLUGIN_EXPORT bool validate_plugin() {
    return true;
}

} // extern "C"