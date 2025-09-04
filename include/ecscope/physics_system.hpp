#pragma once

/**
 * @file physics/physics_system.hpp
 * @brief ECS Physics System Integration for ECScope Phase 5: Física 2D
 * 
 * This header provides the integration layer between the PhysicsWorld2D and the ECS
 * system scheduling. It implements proper ECS system patterns while maintaining
 * the educational focus and performance optimization goals.
 * 
 * Key Features:
 * - ECS System integration with proper scheduling
 * - Physics component lifecycle management  
 * - Educational step-by-step simulation modes
 * - Performance profiling and optimization
 * - Memory management integration
 * - Event system integration with ECS
 * 
 * Architecture:
 * - PhysicsSystem: Main ECS system that orchestrates physics simulation
 * - ComponentManagers: Specialized managers for physics component types
 * - SystemScheduling: Integration with ECS system scheduling and dependencies
 * - PerformanceProfiler: Educational performance analysis and optimization
 * 
 * @author ECScope Educational ECS Framework - Phase 5: Física 2D
 * @date 2024
 */

#include "world.hpp"
#include "../ecs/registry.hpp"
#include "../ecs/archetype.hpp" 
#include "../core/types.hpp"
#include "../core/log.hpp"
#include <memory>
#include <vector>
#include <unordered_map>
#include <functional>
#include <chrono>

namespace ecscope::physics {

// Import necessary types
using namespace ecscope::ecs;

//=============================================================================
// Physics System Configuration
//=============================================================================

/**
 * @brief Configuration for physics system integration with ECS
 */
struct PhysicsSystemConfig {
    /** @brief Physics world configuration */
    PhysicsWorldConfig world_config{PhysicsWorldConfig::create_educational()};
    
    /** @brief ECS integration settings */
    bool auto_add_missing_components{true};    // Automatically add required components
    bool validate_component_consistency{true}; // Validate physics components on each frame
    bool enable_component_lifecycle_events{true}; // Track component add/remove events
    
    /** @brief System scheduling configuration */
    u32 system_priority{100};                 // Priority in system execution order
    bool allow_parallel_execution{false};     // Whether this system can run in parallel
    std::vector<std::string> system_dependencies; // Systems this depends on
    
    /** @brief Educational features */
    bool enable_system_profiling{true};       // Profile system performance
    bool enable_component_visualization{true}; // Visualize component data
    bool enable_system_debugging{true};       // Enable debug mode features
    
    /** @brief Performance optimization */
    u32 batch_size{64};                       // Entities to process per batch
    bool enable_dirty_tracking{true};         // Only update changed entities
    bool enable_spatial_coherence{true};      // Optimize for spatial locality
    
    /** @brief Factory methods */
    static PhysicsSystemConfig create_educational() {
        PhysicsSystemConfig config;
        config.world_config = PhysicsWorldConfig::create_educational();
        config.enable_system_profiling = true;
        config.enable_component_visualization = true;
        config.enable_system_debugging = true;
        config.validate_component_consistency = true;
        return config;
    }
    
    static PhysicsSystemConfig create_performance() {
        PhysicsSystemConfig config;
        config.world_config = PhysicsWorldConfig::create_performance();
        config.allow_parallel_execution = true;
        config.enable_system_profiling = false;
        config.enable_component_visualization = false;
        config.enable_system_debugging = false;
        config.validate_component_consistency = false;
        config.batch_size = 128;
        return config;
    }
};

//=============================================================================
// Physics Component Managers
//=============================================================================

/**
 * @brief Specialized manager for physics components with educational features
 * 
 * This class manages the lifecycle and relationships of physics components,
 * providing educational insights into component management in ECS systems.
 */
class PhysicsComponentManager {
private:
    Registry* registry_;
    PhysicsSystemConfig config_;
    
    /** @brief Component tracking for educational analysis */
    struct ComponentStats {
        u32 total_rigid_bodies{0};
        u32 total_colliders{0};
        u32 total_force_accumulators{0};
        u32 total_physics_materials{0};
        u32 total_constraints{0};
        u32 total_triggers{0};
        
        u32 components_added_this_frame{0};
        u32 components_removed_this_frame{0};
        u32 components_modified_this_frame{0};
        
        f64 component_memory_usage{0.0};
        f64 component_update_time{0.0};
    } stats_;
    
    /** @brief Dirty tracking for performance optimization */
    std::unordered_set<Entity> dirty_entities_;
    std::unordered_set<Entity> entities_to_validate_;
    
public:
    explicit PhysicsComponentManager(Registry& registry, const PhysicsSystemConfig& config)
        : registry_(&registry), config_(config) {
        
        if (config_.enable_component_lifecycle_events) {
            LOG_INFO("PhysicsComponentManager: Component lifecycle tracking enabled");
        }
    }
    
    /** @brief Update component statistics and validation */
    void update_frame_stats() {
        stats_.components_added_this_frame = 0;
        stats_.components_removed_this_frame = 0;
        stats_.components_modified_this_frame = 0;
        
        // Count all physics components
        stats_.total_rigid_bodies = 0;
        stats_.total_colliders = 0;
        stats_.total_force_accumulators = 0;
        
        registry_->for_each<RigidBody2D>([this](Entity entity, const RigidBody2D& rb) {
            ++stats_.total_rigid_bodies;
        });
        
        registry_->for_each<Collider2D>([this](Entity entity, const Collider2D& collider) {
            ++stats_.total_colliders;
        });
        
        registry_->for_each<ForceAccumulator>([this](Entity entity, const ForceAccumulator& forces) {
            ++stats_.total_force_accumulators;
        });
    }
    
    /** @brief Validate physics component consistency */
    bool validate_physics_entity(Entity entity) {
        bool is_valid = true;
        
        // Check required component combinations
        auto* transform = registry_->get_component<Transform>(entity);
        auto* rigidbody = registry_->get_component<RigidBody2D>(entity);
        auto* collider = registry_->get_component<Collider2D>(entity);
        
        if (rigidbody && !transform) {
            LOG_WARN("Entity {} has RigidBody2D but no Transform component", entity);
            is_valid = false;
        }
        
        if (collider && !transform) {
            LOG_WARN("Entity {} has Collider2D but no Transform component", entity);
            is_valid = false;
        }
        
        // Validate component data integrity
        if (rigidbody && !rigidbody->is_valid()) {
            LOG_WARN("Entity {} has invalid RigidBody2D component", entity);
            is_valid = false;
        }
        
        if (collider && !collider->is_valid()) {
            LOG_WARN("Entity {} has invalid Collider2D component", entity);
            is_valid = false;
        }
        
        return is_valid;
    }
    
    /** @brief Auto-add missing required components */
    void ensure_required_components(Entity entity) {
        if (!config_.auto_add_missing_components) return;
        
        auto* rigidbody = registry_->get_component<RigidBody2D>(entity);
        auto* collider = registry_->get_component<Collider2D>(entity);
        
        // If entity has physics components, ensure it has Transform
        if ((rigidbody || collider) && !registry_->has_component<Transform>(entity)) {
            registry_->add_component<Transform>(entity, Transform{});
            LOG_DEBUG("Auto-added Transform component to entity {}", entity);
        }
        
        // If entity has RigidBody2D, ensure it has ForceAccumulator
        if (rigidbody && !registry_->has_component<ForceAccumulator>(entity)) {
            registry_->add_component<ForceAccumulator>(entity, ForceAccumulator{});
            LOG_DEBUG("Auto-added ForceAccumulator component to entity {}", entity);
        }
    }
    
    /** @brief Mark entity as dirty for selective updates */
    void mark_entity_dirty(Entity entity) {
        if (config_.enable_dirty_tracking) {
            dirty_entities_.insert(entity);
        }
    }
    
    /** @brief Get entities that need physics updates */
    std::vector<Entity> get_physics_entities() {
        std::vector<Entity> entities;
        
        if (config_.enable_dirty_tracking && !dirty_entities_.empty()) {
            // Return only dirty entities
            entities.reserve(dirty_entities_.size());
            entities.insert(entities.end(), dirty_entities_.begin(), dirty_entities_.end());
            dirty_entities_.clear();
        } else {
            // Return all physics entities
            entities = registry_->get_entities_with<Transform, RigidBody2D>();
        }
        
        return entities;
    }
    
    /** @brief Get component statistics for educational display */
    const ComponentStats& get_statistics() const { return stats_; }
    
    /** @brief Generate component usage report */
    std::string generate_component_report() const {
        std::ostringstream oss;
        oss << "=== Physics Component Statistics ===\n";
        oss << "RigidBody2D components: " << stats_.total_rigid_bodies << "\n";
        oss << "Collider2D components: " << stats_.total_colliders << "\n";
        oss << "ForceAccumulator components: " << stats_.total_force_accumulators << "\n";
        oss << "\nThis Frame:\n";
        oss << "  Added: " << stats_.components_added_this_frame << "\n";
        oss << "  Removed: " << stats_.components_removed_this_frame << "\n";
        oss << "  Modified: " << stats_.components_modified_this_frame << "\n";
        oss << "Memory Usage: " << (stats_.component_memory_usage / 1024.0) << " KB\n";
        oss << "Update Time: " << stats_.component_update_time << " ms\n";
        return oss.str();
    }
};

//=============================================================================
// Main Physics System Implementation
//=============================================================================

/**
 * @brief Main ECS Physics System
 * 
 * This is the primary integration point between the ECS and physics simulation.
 * It implements the ECS system pattern while providing comprehensive educational
 * features and performance optimization.
 * 
 * Educational Features:
 * - Step-by-step physics pipeline visualization
 * - Component lifecycle tracking and analysis
 * - Performance profiling with detailed breakdowns
 * - Memory usage analysis and optimization suggestions
 * - Integration patterns demonstration
 * 
 * Performance Features:
 * - Batch processing for cache efficiency
 * - Dirty tracking for selective updates
 * - Spatial coherence optimization
 * - Memory pool integration
 * - Parallel processing support
 */
class PhysicsSystem {
private:
    /** @brief Core system components */
    Registry* registry_;
    PhysicsSystemConfig config_;
    std::unique_ptr<PhysicsWorld2D> physics_world_;
    std::unique_ptr<PhysicsComponentManager> component_manager_;
    
    /** @brief System state */
    bool initialized_{false};
    bool paused_{false};
    u32 system_id_{0};
    f32 last_delta_time_{0.0f};
    
    /** @brief Performance profiling */
    struct SystemProfileData {
        f64 total_update_time{0.0};
        f64 component_sync_time{0.0};
        f64 physics_simulation_time{0.0};
        f64 component_update_time{0.0};
        f64 validation_time{0.0};
        
        u32 entities_processed{0};
        u32 components_synced{0};
        u32 physics_steps{0};
        
        f64 average_update_time{0.0};
        f64 peak_update_time{0.0};
        u64 total_updates{0};
    } profile_data_;
    
    /** @brief Educational debugging */
    struct DebugData {
        bool step_mode_enabled{false};
        u32 current_debug_step{0};
        std::vector<std::string> debug_step_descriptions;
        std::vector<f64> debug_step_timings;
        bool visualization_enabled{false};
    } debug_data_;
    
    /** @brief System dependencies and scheduling */
    std::vector<std::string> dependencies_;
    std::function<bool()> can_execute_predicate_;
    
public:
    /** @brief Constructor */
    explicit PhysicsSystem(Registry& registry, const PhysicsSystemConfig& config = PhysicsSystemConfig::create_educational())
        : registry_(&registry), config_(config) {
        
        // Initialize physics world
        physics_world_ = std::make_unique<PhysicsWorld2D>(registry, config.world_config);
        
        // Initialize component manager
        component_manager_ = std::make_unique<PhysicsComponentManager>(registry, config);
        
        // Set up dependencies
        dependencies_ = config.system_dependencies;
        
        // Generate unique system ID
        static std::atomic<u32> next_system_id{1};
        system_id_ = next_system_id.fetch_add(1);
        
        LOG_INFO("PhysicsSystem {} initialized with configuration:", system_id_);
        LOG_INFO("  - Priority: {}", config.system_priority);
        LOG_INFO("  - Batch size: {}", config.batch_size);
        LOG_INFO("  - Dirty tracking: {}", config.enable_dirty_tracking ? "enabled" : "disabled");
        LOG_INFO("  - Profiling: {}", config.enable_system_profiling ? "enabled" : "disabled");
        
        initialized_ = true;
    }
    
    /** @brief Destructor with cleanup and final statistics */
    ~PhysicsSystem() {
        if (config_.enable_system_profiling && profile_data_.total_updates > 0) {
            LOG_INFO("PhysicsSystem {} final statistics:", system_id_);
            LOG_INFO("  - Total updates: {}", profile_data_.total_updates);
            LOG_INFO("  - Average update time: {:.3f} ms", profile_data_.average_update_time);
            LOG_INFO("  - Peak update time: {:.3f} ms", profile_data_.peak_update_time);
            LOG_INFO("  - Total entities processed: {}", profile_data_.entities_processed);
        }
    }
    
    //-------------------------------------------------------------------------
    // Core System Interface
    //-------------------------------------------------------------------------
    
    /** @brief Main system update method */
    void update(f32 delta_time) {
        if (!initialized_ || paused_) return;
        
        auto update_start = std::chrono::high_resolution_clock::now();
        last_delta_time_ = delta_time;
        
        // Educational step mode handling
        if (debug_data_.step_mode_enabled) {
            update_step_mode(delta_time);
        } else {
            update_normal_mode(delta_time);
        }
        
        // Update performance statistics
        if (config_.enable_system_profiling) {
            auto update_end = std::chrono::high_resolution_clock::now();
            update_performance_stats(update_start, update_end);
        }
        
        // Update component manager statistics
        component_manager_->update_frame_stats();
    }
    
    /** @brief Initialize system (called once) */
    bool initialize() {
        if (initialized_) return true;
        
        LOG_INFO("Initializing PhysicsSystem {}", system_id_);
        
        // Validate registry has required components registered
        // (This would be extended to check component registration)
        
        // Set up event callbacks
        if (config_.enable_component_lifecycle_events) {
            setup_component_event_callbacks();
        }
        
        // Initialize debug features
        if (config_.enable_system_debugging) {
            initialize_debug_features();
        }
        
        initialized_ = true;
        LOG_INFO("PhysicsSystem {} initialized successfully", system_id_);
        return true;
    }
    
    /** @brief Cleanup system resources */
    void cleanup() {
        LOG_INFO("Cleaning up PhysicsSystem {}", system_id_);
        
        physics_world_.reset();
        component_manager_.reset();
        
        initialized_ = false;
    }
    
    //-------------------------------------------------------------------------
    // System Control Interface
    //-------------------------------------------------------------------------
    
    /** @brief Pause/resume system execution */
    void set_paused(bool paused) { 
        paused_ = paused; 
        if (physics_world_) {
            physics_world_->set_paused(paused);
        }
    }
    
    bool is_paused() const { return paused_; }
    
    /** @brief Enable educational step-by-step mode */
    void enable_step_mode(bool enabled) {
        debug_data_.step_mode_enabled = enabled;
        if (physics_world_) {
            physics_world_->enable_step_mode(enabled);
        }
        
        if (enabled) {
            LOG_INFO("PhysicsSystem {}: Step mode enabled", system_id_);
        }
    }
    
    /** @brief Request next step in step mode */
    void request_step() {
        if (debug_data_.step_mode_enabled && physics_world_) {
            physics_world_->request_step();
        }
    }
    
    /** @brief Reset physics simulation */
    void reset() {
        if (physics_world_) {
            physics_world_->reset();
        }
        
        profile_data_ = SystemProfileData{};
        debug_data_.current_debug_step = 0;
        
        LOG_INFO("PhysicsSystem {} reset", system_id_);
    }
    
    //-------------------------------------------------------------------------
    // Entity and Component Management
    //-------------------------------------------------------------------------
    
    /** @brief Add entity to physics simulation */
    bool add_physics_entity(Entity entity) {
        // Ensure required components are present
        component_manager_->ensure_required_components(entity);
        
        // Validate component consistency
        if (config_.validate_component_consistency) {
            if (!component_manager_->validate_physics_entity(entity)) {
                LOG_WARN("Failed to add entity {} to physics: validation failed", entity);
                return false;
            }
        }
        
        // Add to physics world
        bool success = physics_world_->add_entity(entity);
        if (success) {
            LOG_DEBUG("Added entity {} to physics simulation", entity);
        }
        
        return success;
    }
    
    /** @brief Remove entity from physics simulation */
    bool remove_physics_entity(Entity entity) {
        bool success = physics_world_->remove_entity(entity);
        if (success) {
            LOG_DEBUG("Removed entity {} from physics simulation", entity);
        }
        return success;
    }
    
    /** @brief Get all physics entities managed by this system */
    std::vector<Entity> get_physics_entities() const {
        return physics_world_->get_all_physics_entities();
    }
    
    //-------------------------------------------------------------------------
    // Force and Impulse Application
    //-------------------------------------------------------------------------
    
    /** @brief Apply force to entity */
    void apply_force(Entity entity, const Vec2& force) {
        physics_world_->apply_force(entity, force);
        component_manager_->mark_entity_dirty(entity);
    }
    
    /** @brief Apply impulse to entity */
    void apply_impulse(Entity entity, const Vec2& impulse) {
        physics_world_->apply_impulse(entity, impulse);
        component_manager_->mark_entity_dirty(entity);
    }
    
    //-------------------------------------------------------------------------
    // Physics Queries
    //-------------------------------------------------------------------------
    
    /** @brief Raycast into physics world */
    collision::RaycastResult raycast(const Ray2D& ray, u32 layer_mask = 0xFFFFFFFF) {
        return physics_world_->raycast(ray, layer_mask);
    }
    
    /** @brief Find entities overlapping with shape */
    std::vector<Entity> overlap_shape(const CollisionShape& shape, u32 layer_mask = 0xFFFFFFFF) {
        return physics_world_->overlap_shape(shape, layer_mask);
    }
    
    //-------------------------------------------------------------------------
    // Configuration and Properties
    //-------------------------------------------------------------------------
    
    /** @brief Get system configuration */
    const PhysicsSystemConfig& get_config() const { return config_; }
    
    /** @brief Get physics world reference */
    PhysicsWorld2D& get_physics_world() { return *physics_world_; }
    const PhysicsWorld2D& get_physics_world() const { return *physics_world_; }
    
    /** @brief Get component manager reference */
    PhysicsComponentManager& get_component_manager() { return *component_manager_; }
    
    //-------------------------------------------------------------------------
    // Performance Analysis and Statistics
    //-------------------------------------------------------------------------
    
    /** @brief Get comprehensive system statistics */
    struct SystemStatistics {
        SystemProfileData profile_data;
        PhysicsComponentManager::ComponentStats component_stats;
        PhysicsWorldStats world_stats;
        
        /** @brief Derived performance metrics */
        f32 entities_per_second{0.0f};
        f32 components_per_second{0.0f};
        f32 system_efficiency{0.0f};      // 0-100 scale
        f32 memory_efficiency{0.0f};      // 0-100 scale
        
        std::string performance_rating;    // "Excellent", "Good", "Fair", "Poor"
        std::vector<std::string> optimization_suggestions;
    };
    
    SystemStatistics get_system_statistics() const {
        SystemStatistics stats;
        stats.profile_data = profile_data_;
        stats.component_stats = component_manager_->get_statistics();
        stats.world_stats = physics_world_->get_statistics();
        
        // Calculate derived metrics
        if (profile_data_.average_update_time > 0.0) {
            stats.entities_per_second = static_cast<f32>(profile_data_.entities_processed) / 
                                       (profile_data_.average_update_time / 1000.0);
        }
        
        // Calculate efficiency scores
        f32 target_fps = 60.0f;
        f32 target_frame_time = 1000.0f / target_fps;
        stats.system_efficiency = std::min(100.0f, (target_frame_time / static_cast<f32>(profile_data_.average_update_time)) * 100.0f);
        
        // Performance rating
        if (stats.system_efficiency > 90.0f) {
            stats.performance_rating = "Excellent";
        } else if (stats.system_efficiency > 70.0f) {
            stats.performance_rating = "Good";
        } else if (stats.system_efficiency > 50.0f) {
            stats.performance_rating = "Fair";
        } else {
            stats.performance_rating = "Poor";
        }
        
        // Generate optimization suggestions
        generate_optimization_suggestions(stats);
        
        return stats;
    }
    
    /** @brief Generate comprehensive performance report */
    std::string generate_performance_report() const {
        auto stats = get_system_statistics();
        std::ostringstream oss;
        
        oss << "=== PhysicsSystem Performance Report ===\n";
        oss << "System ID: " << system_id_ << "\n";
        oss << "Performance Rating: " << stats.performance_rating << "\n";
        oss << "System Efficiency: " << stats.system_efficiency << "%\n";
        oss << "\n--- Timing ---\n";
        oss << "Average Update Time: " << stats.profile_data.average_update_time << " ms\n";
        oss << "Peak Update Time: " << stats.profile_data.peak_update_time << " ms\n";
        oss << "Total Updates: " << stats.profile_data.total_updates << "\n";
        oss << "\n--- Throughput ---\n";
        oss << "Entities/Second: " << stats.entities_per_second << "\n";
        oss << "Components/Second: " << stats.components_per_second << "\n";
        
        oss << "\n--- Component Statistics ---\n";
        oss << component_manager_->generate_component_report();
        
        oss << "\n--- Physics World Statistics ---\n";
        oss << physics_world_->generate_performance_report();
        
        if (!stats.optimization_suggestions.empty()) {
            oss << "\n--- Optimization Suggestions ---\n";
            for (const auto& suggestion : stats.optimization_suggestions) {
                oss << "- " << suggestion << "\n";
            }
        }
        
        return oss.str();
    }
    
    //-------------------------------------------------------------------------
    // Educational Debug Features
    //-------------------------------------------------------------------------
    
    /** @brief Get step-by-step breakdown of current frame */
    std::vector<std::string> get_debug_step_breakdown() const {
        return debug_data_.debug_step_descriptions;
    }
    
    /** @brief Enable/disable visualization features */
    void set_visualization_enabled(bool enabled) {
        debug_data_.visualization_enabled = enabled;
        if (physics_world_) {
            physics_world_->set_debug_render_collision_shapes(enabled);
            physics_world_->set_debug_render_contact_points(enabled);
            physics_world_->set_debug_render_forces(enabled);
        }
    }
    
    /** @brief Get visualization data for educational rendering */
    PhysicsWorld2D::VisualizationData get_visualization_data() const {
        if (physics_world_ && debug_data_.visualization_enabled) {
            return physics_world_->get_visualization_data();
        }
        return PhysicsWorld2D::VisualizationData{};
    }

private:
    //-------------------------------------------------------------------------
    // Internal Update Methods
    //-------------------------------------------------------------------------
    
    /** @brief Normal update mode */
    void update_normal_mode(f32 delta_time) {
        auto sync_start = std::chrono::high_resolution_clock::now();
        
        // 1. Synchronize ECS components with physics world
        synchronize_components();
        
        auto physics_start = std::chrono::high_resolution_clock::now();
        
        // 2. Update physics simulation
        physics_world_->update(delta_time);
        
        auto component_start = std::chrono::high_resolution_clock::now();
        
        // 3. Update component data from physics simulation
        update_components_from_physics();
        
        auto validation_start = std::chrono::high_resolution_clock::now();
        
        // 4. Validate component consistency if enabled
        if (config_.validate_component_consistency) {
            validate_all_physics_entities();
        }
        
        auto end_time = std::chrono::high_resolution_clock::now();
        
        // Update timing statistics
        if (config_.enable_system_profiling) {
            profile_data_.component_sync_time = std::chrono::duration<f64, std::milli>(physics_start - sync_start).count();
            profile_data_.physics_simulation_time = std::chrono::duration<f64, std::milli>(component_start - physics_start).count();
            profile_data_.component_update_time = std::chrono::duration<f64, std::milli>(validation_start - component_start).count();
            profile_data_.validation_time = std::chrono::duration<f64, std::milli>(end_time - validation_start).count();
        }
    }
    
    /** @brief Step-by-step debug mode update */
    void update_step_mode(f32 delta_time) {
        debug_data_.debug_step_descriptions.clear();
        debug_data_.debug_step_timings.clear();
        
        debug_data_.debug_step_descriptions.push_back("Component Synchronization");
        auto step_start = std::chrono::high_resolution_clock::now();
        synchronize_components();
        auto step_end = std::chrono::high_resolution_clock::now();
        debug_data_.debug_step_timings.push_back(std::chrono::duration<f64, std::milli>(step_end - step_start).count());
        
        debug_data_.debug_step_descriptions.push_back("Physics World Update");
        step_start = std::chrono::high_resolution_clock::now();
        physics_world_->update(delta_time);
        step_end = std::chrono::high_resolution_clock::now();
        debug_data_.debug_step_timings.push_back(std::chrono::duration<f64, std::milli>(step_end - step_start).count());
        
        debug_data_.debug_step_descriptions.push_back("Component Data Update");
        step_start = std::chrono::high_resolution_clock::now();
        update_components_from_physics();
        step_end = std::chrono::high_resolution_clock::now();
        debug_data_.debug_step_timings.push_back(std::chrono::duration<f64, std::milli>(step_end - step_start).count());
        
        ++debug_data_.current_debug_step;
    }
    
    /** @brief Synchronize ECS components to physics world */
    void synchronize_components() {
        auto entities = component_manager_->get_physics_entities();
        
        for (Entity entity : entities) {
            // Ensure entity is in physics world
            if (!physics_world_->contains_entity(entity)) {
                physics_world_->add_entity(entity);
            }
            
            // Mark for consistency validation
            if (config_.validate_component_consistency) {
                component_manager_->ensure_required_components(entity);
            }
        }
        
        profile_data_.entities_processed = static_cast<u32>(entities.size());
    }
    
    /** @brief Update ECS components from physics simulation results */
    void update_components_from_physics() {
        // Physics world internally updates Transform components during integration
        // Here we could update additional component state if needed
        
        // Update any cached motion state components
        registry_->for_each<MotionState, RigidBody2D, Transform>([](Entity entity, MotionState& motion_state, 
                                                                   const RigidBody2D& rigidbody, const Transform& transform) {
            motion_state.update_motion_cache(rigidbody);
            motion_state.update_transform_cache(transform);
        });
    }
    
    /** @brief Validate all physics entities for consistency */
    void validate_all_physics_entities() {
        auto entities = physics_world_->get_all_physics_entities();
        
        for (Entity entity : entities) {
            if (!component_manager_->validate_physics_entity(entity)) {
                LOG_WARN("Physics entity {} failed validation", entity);
            }
        }
    }
    
    /** @brief Update performance statistics */
    void update_performance_stats(const std::chrono::high_resolution_clock::time_point& start,
                                 const std::chrono::high_resolution_clock::time_point& end) {
        f64 frame_time = std::chrono::duration<f64, std::milli>(end - start).count();
        
        profile_data_.total_update_time += frame_time;
        ++profile_data_.total_updates;
        
        profile_data_.average_update_time = profile_data_.total_update_time / profile_data_.total_updates;
        profile_data_.peak_update_time = std::max(profile_data_.peak_update_time, frame_time);
    }
    
    /** @brief Setup component lifecycle event callbacks */
    void setup_component_event_callbacks() {
        // This would integrate with ECS event system if available
        LOG_DEBUG("Component lifecycle event callbacks set up for PhysicsSystem {}", system_id_);
    }
    
    /** @brief Initialize debug features */
    void initialize_debug_features() {
        debug_data_ = DebugData{};
        LOG_DEBUG("Debug features initialized for PhysicsSystem {}", system_id_);
    }
    
    /** @brief Generate optimization suggestions based on performance data */
    void generate_optimization_suggestions(SystemStatistics& stats) const {
        stats.optimization_suggestions.clear();
        
        if (stats.system_efficiency < 70.0f) {
            stats.optimization_suggestions.push_back("Consider reducing physics time step or constraint iterations");
            stats.optimization_suggestions.push_back("Enable dirty tracking to reduce unnecessary updates");
            stats.optimization_suggestions.push_back("Increase batch size for better cache coherency");
        }
        
        if (stats.profile_data.validation_time > 1.0) {
            stats.optimization_suggestions.push_back("Disable component validation in release builds");
        }
        
        if (stats.component_stats.total_rigid_bodies > 1000) {
            stats.optimization_suggestions.push_back("Consider using sleeping system to reduce active body count");
            stats.optimization_suggestions.push_back("Enable spatial coherence optimization");
        }
        
        if (stats.world_stats.cache_hit_ratio < 0.8f) {
            stats.optimization_suggestions.push_back("Improve cache coherency by optimizing entity spatial layout");
            stats.optimization_suggestions.push_back("Consider using different spatial partitioning cell size");
        }
    }
};

} // namespace ecscope::physics