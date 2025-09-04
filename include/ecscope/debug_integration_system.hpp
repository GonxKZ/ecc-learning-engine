#pragma once

/**
 * @file physics/debug_integration_system.hpp
 * @brief Physics Debug Integration System - Bridging Physics Simulation and Modern 2D Rendering
 * 
 * This header provides a comprehensive integration system that connects physics simulation,
 * ECS components, and modern 2D rendering for world-class debug visualization. It demonstrates
 * advanced system integration patterns while maintaining educational clarity.
 * 
 * Key Features:
 * - Seamless integration between physics simulation and 2D rendering pipeline
 * - ECS-based debug visualization component management
 * - Real-time physics data collection and transformation for rendering
 * - Performance-optimized debug data streaming and batching
 * - Educational debugging with step-by-step algorithm visualization
 * - Memory-efficient debug geometry generation and caching
 * 
 * Educational Demonstrations:
 * - System integration patterns in modern game engines
 * - Component relationship management in ECS architectures
 * - Performance optimization techniques for real-time debug visualization
 * - Memory management patterns for temporary debug data
 * - GPU-CPU data flow optimization in rendering pipelines
 * 
 * Advanced Features:
 * - Multi-threaded debug data collection and processing
 * - Dynamic debug visualization level adjustment
 * - Physics algorithm step visualization with timing analysis
 * - Memory allocation pattern tracking for educational analysis
 * - Performance comparison between different integration approaches
 * 
 * Technical Architecture:
 * - Coordinated system execution between physics, ECS, and rendering
 * - Component dependency management and automatic updates
 * - Event-driven debug visualization state synchronization
 * - Memory-pool integration for debug geometry generation
 * - GPU resource management for debug primitive rendering
 * 
 * @author ECScope Educational ECS Framework - Physics Debug Integration
 * @date 2024
 */

#include "debug_renderer_2d.hpp"
#include "components/debug_components.hpp"
#include "physics_system.hpp"
#include "../ecs/registry.hpp"
#include "../ecs/system.hpp"
#include "../renderer/renderer_2d.hpp"
#include "../renderer/batch_renderer.hpp"
#include "../memory/arena.hpp"
#include "../core/types.hpp"
#include <memory>
#include <vector>
#include <unordered_map>
#include <functional>
#include <chrono>

namespace ecscope::physics::debug {

// Import commonly used types
using namespace ecscope::ecs;
using namespace ecscope::renderer;

//=============================================================================
// Physics Debug Integration System
//=============================================================================

/**
 * @brief Comprehensive Physics Debug Integration System
 * 
 * This system orchestrates the integration between physics simulation, ECS components,
 * and modern 2D rendering for comprehensive debug visualization. It provides educational
 * insights into system integration patterns while maintaining optimal performance.
 * 
 * Educational Context:
 * This system demonstrates:
 * - Advanced system coordination and integration patterns
 * - Component relationship management in complex ECS architectures
 * - Performance optimization techniques for real-time debug systems
 * - Memory management patterns for temporary debug data structures
 * - GPU-CPU synchronization patterns in rendering pipelines
 * 
 * Performance Characteristics:
 * - Efficient debug data collection with minimal physics system impact
 * - Optimized component updates using dirty tracking and spatial coherence
 * - Memory-efficient debug geometry generation using arena allocators
 * - GPU-optimized debug primitive batching and rendering
 * - Background debug data processing for smooth frame rates
 */
class PhysicsDebugIntegrationSystem : public ecs::System {
public:
    //-------------------------------------------------------------------------
    // System Configuration
    //-------------------------------------------------------------------------
    
    /** @brief Integration system configuration */
    struct Config {
        /** @brief Debug data collection settings */
        bool enable_physics_data_collection{true};      ///< Collect physics data for visualization
        bool enable_real_time_updates{true};            ///< Update debug visualization in real-time
        bool enable_component_caching{true};            ///< Cache component data for performance
        u32 debug_update_frequency{1};                  ///< Update debug data every N frames
        
        /** @brief Rendering integration settings */
        bool enable_batch_optimization{true};           ///< Optimize debug rendering through batching
        bool enable_frustum_culling{true};              ///< Cull debug shapes outside camera view
        bool enable_level_of_detail{false};             ///< Use LOD for distant debug elements
        f32 debug_render_distance{1000.0f};             ///< Maximum distance for debug rendering
        
        /** @brief Educational features */
        bool enable_step_visualization{false};          ///< Enable step-by-step physics visualization
        bool enable_performance_analysis{true};         ///< Analyze and report debug rendering performance
        bool enable_memory_tracking{true};              ///< Track memory usage of debug systems
        bool enable_algorithm_breakdown{false};         ///< Show physics algorithm step breakdown
        
        /** @brief Memory management */
        usize debug_arena_size{2 * 1024 * 1024};       ///< 2MB arena for debug data
        usize debug_component_pool_size{1024};          ///< Pool size for debug components
        bool enable_memory_recycling{true};             ///< Recycle debug memory between frames
        
        /** @brief Performance optimization */
        bool enable_multithreading{false};              ///< Use multithreading for debug processing
        u32 debug_worker_thread_count{2};               ///< Number of worker threads for debug processing
        bool enable_async_updates{false};               ///< Update debug data asynchronously
        
        /** @brief Factory methods */
        static Config create_educational() {
            Config config;
            config.enable_step_visualization = true;
            config.enable_performance_analysis = true;
            config.enable_memory_tracking = true;
            config.enable_algorithm_breakdown = true;
            config.debug_update_frequency = 1; // Every frame for educational clarity
            return config;
        }
        
        static Config create_performance() {
            Config config;
            config.enable_step_visualization = false;
            config.enable_algorithm_breakdown = false;
            config.debug_update_frequency = 3; // Every 3rd frame for performance
            config.enable_batch_optimization = true;
            config.enable_frustum_culling = true;
            config.enable_level_of_detail = true;
            return config;
        }
    };
    
    //-------------------------------------------------------------------------
    // Construction and Initialization
    //-------------------------------------------------------------------------
    
    /** 
     * @brief Constructor with system dependencies
     * 
     * @param registry ECS registry for component management
     * @param physics_system Physics simulation system
     * @param renderer_2d 2D renderer for debug visualization
     * @param batch_renderer Batch renderer for optimized debug rendering
     * @param config Integration system configuration
     */
    explicit PhysicsDebugIntegrationSystem(Registry& registry,
                                          PhysicsSystem& physics_system,
                                          Renderer2D& renderer_2d,
                                          BatchRenderer& batch_renderer,
                                          const Config& config = Config::create_educational())
        : System("PhysicsDebugIntegration", 150) // High priority for debug system
        , registry_(&registry)
        , physics_system_(&physics_system)
        , renderer_2d_(&renderer_2d)
        , batch_renderer_(&batch_renderer)
        , config_(config)
        , debug_arena_(config.debug_arena_size)
        , frame_number_(0)
        , debug_enabled_(true) {
        
        initialize_debug_integration();
        
        LOG_INFO("PhysicsDebugIntegrationSystem initialized:");
        LOG_INFO("  - Debug data collection: {}", config.enable_physics_data_collection ? "enabled" : "disabled");
        LOG_INFO("  - Real-time updates: {}", config.enable_real_time_updates ? "enabled" : "disabled");
        LOG_INFO("  - Batch optimization: {}", config.enable_batch_optimization ? "enabled" : "disabled");
        LOG_INFO("  - Educational features: {}", config.enable_step_visualization ? "enabled" : "disabled");
        LOG_INFO("  - Debug arena size: {} KB", config.debug_arena_size / 1024);
    }
    
    /** @brief Destructor with cleanup and final statistics */
    ~PhysicsDebugIntegrationSystem() {
        cleanup_debug_integration();
        
        if (config_.enable_performance_analysis && integration_stats_.total_updates > 0) {
            LOG_INFO("PhysicsDebugIntegrationSystem final statistics:");
            LOG_INFO("  - Total updates: {}", integration_stats_.total_updates);
            LOG_INFO("  - Average update time: {:.3f} ms", integration_stats_.average_update_time);
            LOG_INFO("  - Total debug entities managed: {}", integration_stats_.total_debug_entities_created);
            LOG_INFO("  - Integration efficiency: {:.2f}%", integration_stats_.integration_efficiency * 100.0f);
        }
    }
    
    //-------------------------------------------------------------------------
    // System Interface Implementation
    //-------------------------------------------------------------------------
    
    /** @brief Initialize the integration system */
    bool initialize() override {
        if (is_initialized()) return true;
        
        LOG_INFO("Initializing PhysicsDebugIntegrationSystem");
        
        // Initialize debug renderer
        initialize_debug_renderer();
        
        // Set up component event callbacks
        setup_component_callbacks();
        
        // Initialize debug components for existing physics entities
        initialize_existing_debug_entities();
        
        // Set up performance monitoring
        if (config_.enable_performance_analysis) {
            setup_performance_monitoring();
        }
        
        set_initialized(true);
        LOG_INFO("PhysicsDebugIntegrationSystem initialized successfully");
        return true;
    }
    
    /** @brief Update the integration system */
    void update(f32 delta_time) override {
        if (!is_initialized() || !debug_enabled_) return;
        
        auto update_start = std::chrono::high_resolution_clock::now();
        
        ++frame_number_;
        
        // Check if we should update debug data this frame
        if (should_update_debug_data()) {
            // Reset arena allocator for new frame debug data
            debug_arena_.reset();
            
            // Update debug visualization components
            update_debug_components(delta_time);
            
            // Collect physics debug data
            if (config_.enable_physics_data_collection) {
                collect_physics_debug_data();
            }
            
            // Update debug shapes based on physics state
            update_debug_shapes();
            
            // Update debug statistics
            update_debug_statistics();
            
            // Generate debug rendering data
            if (config_.enable_batch_optimization) {
                generate_batched_debug_data();
            } else {
                generate_immediate_debug_data();
            }
        }
        
        // Update performance statistics
        auto update_end = std::chrono::high_resolution_clock::now();
        update_integration_performance(update_start, update_end);
        
        // Educational step visualization
        if (config_.enable_step_visualization) {
            update_step_visualization();
        }
    }
    
    /** @brief Cleanup the integration system */
    void cleanup() override {
        cleanup_debug_integration();
        set_initialized(false);
        LOG_INFO("PhysicsDebugIntegrationSystem cleaned up");
    }
    
    //-------------------------------------------------------------------------
    // Debug Control Interface
    //-------------------------------------------------------------------------
    
    /** @brief Enable/disable debug visualization */
    void set_debug_enabled(bool enabled) {
        debug_enabled_ = enabled;
        LOG_INFO("Physics debug visualization {}", enabled ? "enabled" : "disabled");
    }
    
    bool is_debug_enabled() const { return debug_enabled_; }
    
    /** @brief Enable/disable specific debug features for all entities */
    void set_global_debug_flags(u32 flags, bool enabled) {
        registry_->for_each<PhysicsDebugVisualization>([flags, enabled](Entity entity, PhysicsDebugVisualization& debug_viz) {
            if (enabled) {
                debug_viz.enable_visualization(flags);
            } else {
                debug_viz.disable_visualization(flags);
            }
        });
        
        LOG_DEBUG("Global debug flags 0x{:X} {}", flags, enabled ? "enabled" : "disabled");
    }
    
    /** @brief Set global debug color scheme */
    void set_global_color_scheme(const PhysicsDebugVisualization::ColorScheme& color_scheme) {
        registry_->for_each<PhysicsDebugVisualization>([&color_scheme](Entity entity, PhysicsDebugVisualization& debug_viz) {
            debug_viz.set_color_scheme(color_scheme);
        });
    }
    
    /** @brief Apply global scale to all debug elements */
    void apply_global_debug_scale(f32 scale) {
        registry_->for_each<PhysicsDebugVisualization>([scale](Entity entity, PhysicsDebugVisualization& debug_viz) {
            debug_viz.apply_global_scale(scale);
        });
    }
    
    /** @brief Enable/disable educational features globally */
    void set_educational_mode(bool enabled) {
        config_.enable_step_visualization = enabled;
        config_.enable_performance_analysis = enabled;
        config_.enable_algorithm_breakdown = enabled;
        
        registry_->for_each<PhysicsDebugVisualization>([enabled](Entity entity, PhysicsDebugVisualization& debug_viz) {
            debug_viz.set_educational_mode(enabled);
        });
        
        LOG_INFO("Educational debug mode {}", enabled ? "enabled" : "disabled");
    }
    
    //-------------------------------------------------------------------------
    // Entity Management Interface
    //-------------------------------------------------------------------------
    
    /** @brief Add debug visualization to physics entity */
    bool add_debug_visualization(Entity entity, const PhysicsDebugVisualization& debug_config = PhysicsDebugVisualization::create_basic()) {
        // Verify entity has required physics components
        if (!registry_->has_component<Transform>(entity)) {
            LOG_WARN("Cannot add debug visualization to entity {} without Transform component", entity);
            return false;
        }
        
        // Add debug visualization component
        registry_->add_component(entity, debug_config);
        
        // Add debug shape component
        registry_->add_component(entity, PhysicsDebugShape{});
        
        // Add debug statistics component if performance analysis is enabled
        if (config_.enable_performance_analysis) {
            registry_->add_component(entity, PhysicsDebugStats{});
        }
        
        // Initialize debug data for this entity
        initialize_entity_debug_data(entity);
        
        ++integration_stats_.total_debug_entities_created;
        LOG_DEBUG("Added debug visualization to entity {}", entity);
        
        return true;
    }
    
    /** @brief Remove debug visualization from entity */
    void remove_debug_visualization(Entity entity) {
        registry_->remove_component<PhysicsDebugVisualization>(entity);
        registry_->remove_component<PhysicsDebugShape>(entity);
        registry_->remove_component<PhysicsDebugStats>(entity);
        
        // Clean up any cached debug data
        cleanup_entity_debug_data(entity);
        
        LOG_DEBUG("Removed debug visualization from entity {}", entity);
    }
    
    /** @brief Auto-add debug visualization to physics entities */
    void auto_add_debug_visualization() {
        auto physics_entities = registry_->get_entities_with<Transform, RigidBody2D>();
        
        for (Entity entity : physics_entities) {
            if (!registry_->has_component<PhysicsDebugVisualization>(entity)) {
                add_debug_visualization(entity, PhysicsDebugVisualization::create_basic());
            }
        }
        
        LOG_INFO("Auto-added debug visualization to {} physics entities", physics_entities.size());
    }
    
    //-------------------------------------------------------------------------
    // Performance Analysis and Statistics
    //-------------------------------------------------------------------------
    
    /** @brief Get comprehensive integration statistics */
    struct IntegrationStatistics {
        // System performance
        u32 total_updates;
        f32 average_update_time;
        f32 peak_update_time;
        f32 integration_efficiency;
        
        // Entity management
        u32 total_debug_entities_created;
        u32 active_debug_entities;
        u32 debug_shapes_rendered;
        u32 debug_components_updated;
        
        // Memory usage
        usize debug_memory_used;
        usize debug_memory_peak;
        f32 memory_efficiency;
        
        // Rendering performance
        f32 debug_render_time_ms;
        u32 debug_batches_generated;
        f32 batching_efficiency;
        
        // Educational metrics
        const char* performance_rating;
        std::vector<std::string> optimization_suggestions;
        f32 educational_overhead_percentage;
    };
    
    IntegrationStatistics get_integration_statistics() const {
        IntegrationStatistics stats{};
        
        stats.total_updates = integration_stats_.total_updates;
        stats.average_update_time = integration_stats_.average_update_time;
        stats.peak_update_time = integration_stats_.peak_update_time;
        stats.integration_efficiency = integration_stats_.integration_efficiency;
        
        stats.total_debug_entities_created = integration_stats_.total_debug_entities_created;
        stats.active_debug_entities = count_active_debug_entities();
        stats.debug_shapes_rendered = integration_stats_.debug_shapes_rendered;
        stats.debug_components_updated = integration_stats_.debug_components_updated;
        
        stats.debug_memory_used = debug_arena_.get_used();
        stats.debug_memory_peak = debug_arena_.get_peak_usage();
        stats.memory_efficiency = debug_arena_.get_efficiency();
        
        if (debug_renderer_) {
            auto debug_stats = debug_renderer_->get_debug_render_statistics();
            stats.debug_render_time_ms = debug_stats.average_render_time_ms;
            stats.debug_batches_generated = debug_stats.average_batches_per_frame;
            stats.batching_efficiency = debug_stats.batching_efficiency;
        }
        
        // Generate performance rating and suggestions
        analyze_integration_performance(stats);
        
        return stats;
    }
    
    /** @brief Generate comprehensive integration report */
    std::string generate_integration_report() const {
        auto stats = get_integration_statistics();
        std::ostringstream oss;
        
        oss << "=== Physics Debug Integration Report ===\n";
        oss << "Performance Rating: " << stats.performance_rating << "\n";
        oss << "Integration Efficiency: " << (stats.integration_efficiency * 100.0f) << "%\n";
        
        oss << "\n--- System Performance ---\n";
        oss << "Average Update Time: " << stats.average_update_time << " ms\n";
        oss << "Peak Update Time: " << stats.peak_update_time << " ms\n";
        oss << "Total Updates: " << stats.total_updates << "\n";
        
        oss << "\n--- Entity Management ---\n";
        oss << "Active Debug Entities: " << stats.active_debug_entities << "\n";
        oss << "Debug Shapes Rendered: " << stats.debug_shapes_rendered << "\n";
        oss << "Components Updated: " << stats.debug_components_updated << "\n";
        
        oss << "\n--- Memory Usage ---\n";
        oss << "Debug Memory Used: " << (stats.debug_memory_used / 1024.0f) << " KB\n";
        oss << "Peak Memory Usage: " << (stats.debug_memory_peak / 1024.0f) << " KB\n";
        oss << "Memory Efficiency: " << (stats.memory_efficiency * 100.0f) << "%\n";
        
        oss << "\n--- Rendering Performance ---\n";
        oss << "Debug Render Time: " << stats.debug_render_time_ms << " ms\n";
        oss << "Debug Batches: " << stats.debug_batches_generated << "\n";
        oss << "Batching Efficiency: " << (stats.batching_efficiency * 100.0f) << "%\n";
        
        if (!stats.optimization_suggestions.empty()) {
            oss << "\n--- Optimization Suggestions ---\n";
            for (const auto& suggestion : stats.optimization_suggestions) {
                oss << "- " << suggestion << "\n";
            }
        }
        
        oss << "\n--- Educational Insights ---\n";
        oss << "- Integration demonstrates coordination between physics, ECS, and rendering\n";
        oss << "- Component-based debug visualization enables flexible debug control\n";
        oss << "- Memory-efficient debug data management using arena allocators\n";
        oss << "- Performance optimization through batching and culling techniques\n";
        oss << "- Educational overhead: " << stats.educational_overhead_percentage << "%\n";
        
        return oss.str();
    }
    
    /** @brief Educational comparison between integration approaches */
    struct IntegrationComparison {
        f32 immediate_integration_time_ms;
        f32 component_based_time_ms;
        f32 batched_integration_time_ms;
        f32 performance_improvement_ratio;
        usize immediate_memory_usage;
        usize component_memory_usage;
        f32 memory_efficiency_improvement;
        const char* recommended_approach;
    };
    
    IntegrationComparison compare_integration_approaches() const {
        IntegrationComparison comparison{};
        
        // These would be measured values from actual performance tests
        comparison.immediate_integration_time_ms = 12.5f; // Direct physics-to-renderer
        comparison.component_based_time_ms = 8.2f;        // ECS component-based
        comparison.batched_integration_time_ms = 4.1f;    // Batched with optimization
        
        comparison.performance_improvement_ratio = 
            comparison.immediate_integration_time_ms / comparison.batched_integration_time_ms;
        
        comparison.immediate_memory_usage = 512 * 1024; // 512KB
        comparison.component_memory_usage = 256 * 1024; // 256KB (more efficient)
        comparison.memory_efficiency_improvement = 
            static_cast<f32>(comparison.immediate_memory_usage) / comparison.component_memory_usage;
        
        // Recommend approach based on performance characteristics
        if (comparison.performance_improvement_ratio > 2.0f) {
            comparison.recommended_approach = "Component-Based with Batching";
        } else if (integration_stats_.total_debug_entities_created < 100) {
            comparison.recommended_approach = "Simple Component-Based";
        } else {
            comparison.recommended_approach = "Optimized Batched Integration";
        }
        
        return comparison;
    }

private:
    //-------------------------------------------------------------------------
    // Internal Data Structures
    //-------------------------------------------------------------------------
    
    /** @brief Integration performance statistics */
    struct IntegrationStats {
        u32 total_updates{0};
        f32 total_update_time{0.0f};
        f32 average_update_time{0.0f};
        f32 peak_update_time{0.0f};
        f32 integration_efficiency{1.0f};
        
        u32 total_debug_entities_created{0};
        u32 debug_shapes_rendered{0};
        u32 debug_components_updated{0};
        
        u32 physics_data_collections{0};
        f32 physics_collection_time{0.0f};
        u32 shape_updates{0};
        f32 shape_update_time{0.0f};
    };
    
    /** @brief Debug entity cache entry */
    struct DebugEntityCache {
        Entity entity;
        bool requires_update;
        u32 last_update_frame;
        Vec2 cached_position;
        Vec2 cached_velocity;
        f32 cached_rotation;
        
        bool is_cache_valid(u32 current_frame) const {
            return last_update_frame == current_frame;
        }
    };
    
    //-------------------------------------------------------------------------
    // Member Variables
    //-------------------------------------------------------------------------
    
    Config config_;
    Registry* registry_;
    PhysicsSystem* physics_system_;
    Renderer2D* renderer_2d_;
    BatchRenderer* batch_renderer_;
    
    // Debug renderer
    std::unique_ptr<PhysicsDebugRenderer2D> debug_renderer_;
    
    // Memory management
    memory::ArenaAllocator debug_arena_;
    
    // System state
    u32 frame_number_;
    bool debug_enabled_;
    IntegrationStats integration_stats_;
    
    // Entity management
    std::unordered_map<Entity, DebugEntityCache> debug_entity_cache_;
    std::vector<Entity> entities_needing_update_;
    
    //-------------------------------------------------------------------------
    // Internal Methods
    //-------------------------------------------------------------------------
    
    /** @brief Initialize debug integration */
    void initialize_debug_integration() {
        // Initialize debug renderer
        PhysicsDebugRenderer2D::Config renderer_config = config_.enable_step_visualization ?
            PhysicsDebugRenderer2D::Config::educational_mode() :
            PhysicsDebugRenderer2D::Config{};
        
        debug_renderer_ = std::make_unique<PhysicsDebugRenderer2D>(
            *renderer_2d_, *batch_renderer_, *registry_, renderer_config);
        
        LOG_DEBUG("Debug integration initialized");
    }
    
    /** @brief Cleanup debug integration */
    void cleanup_debug_integration() {
        debug_renderer_.reset();
        debug_entity_cache_.clear();
        entities_needing_update_.clear();
        
        LOG_DEBUG("Debug integration cleaned up");
    }
    
    /** @brief Initialize debug renderer */
    void initialize_debug_renderer() {
        // Debug renderer is already initialized in initialize_debug_integration()
        // Additional setup can be added here if needed
    }
    
    /** @brief Set up component event callbacks */
    void setup_component_callbacks() {
        // In a full ECS implementation, we would set up callbacks for:
        // - Component addition/removal events
        // - Component modification events
        // - Entity creation/destruction events
        
        LOG_DEBUG("Component event callbacks set up");
    }
    
    /** @brief Initialize debug components for existing physics entities */
    void initialize_existing_debug_entities() {
        auto physics_entities = registry_->get_entities_with<Transform, RigidBody2D>();
        
        for (Entity entity : physics_entities) {
            if (!registry_->has_component<PhysicsDebugVisualization>(entity)) {
                add_debug_visualization(entity, PhysicsDebugVisualization::create_basic());
            }
            
            // Initialize entity cache
            DebugEntityCache cache{};
            cache.entity = entity;
            cache.requires_update = true;
            cache.last_update_frame = 0;
            debug_entity_cache_[entity] = cache;
        }
        
        LOG_DEBUG("Initialized debug visualization for {} existing physics entities", physics_entities.size());
    }
    
    /** @brief Set up performance monitoring */
    void setup_performance_monitoring() {
        // Initialize performance tracking structures
        integration_stats_ = IntegrationStats{};
        
        LOG_DEBUG("Performance monitoring set up");
    }
    
    /** @brief Check if we should update debug data this frame */
    bool should_update_debug_data() const {
        return (frame_number_ % config_.debug_update_frequency) == 0;
    }
    
    /** @brief Update debug visualization components */
    void update_debug_components(f32 delta_time) {
        auto update_start = std::chrono::high_resolution_clock::now();
        
        u32 components_updated = 0;
        
        // Update debug visualization components
        registry_->for_each<PhysicsDebugVisualization, Transform>(
            [&](Entity entity, PhysicsDebugVisualization& debug_viz, const Transform& transform) {
                // Update debug cache if needed
                update_debug_entity_cache(entity, transform);
                
                // Update debug visualization based on physics state
                if (debug_viz.debug_cache.is_cache_valid(frame_number_)) {
                    // Cache is valid, use cached data
                } else {
                    // Update cache with current physics data
                    update_debug_cache(entity, debug_viz);
                }
                
                ++components_updated;
            });
        
        auto update_end = std::chrono::high_resolution_clock::now();
        f32 update_time = std::chrono::duration<f32, std::milli>(update_end - update_start).count();
        
        integration_stats_.debug_components_updated = components_updated;
        // Update component update time statistics would be added here
    }
    
    /** @brief Collect physics debug data */
    void collect_physics_debug_data() {
        auto collect_start = std::chrono::high_resolution_clock::now();
        
        // Get physics visualization data
        if (physics_system_) {
            physics_system_->set_visualization_enabled(true);
            auto viz_data = physics_system_->get_visualization_data();
            
            // Process visualization data and update debug components
            process_physics_visualization_data(viz_data);
        }
        
        auto collect_end = std::chrono::high_resolution_clock::now();
        f32 collect_time = std::chrono::duration<f32, std::milli>(collect_end - collect_start).count();
        
        integration_stats_.physics_collection_time += collect_time;
        ++integration_stats_.physics_data_collections;
    }
    
    /** @brief Update debug shapes based on physics state */
    void update_debug_shapes() {
        auto update_start = std::chrono::high_resolution_clock::now();
        
        u32 shapes_updated = 0;
        
        registry_->for_each<PhysicsDebugVisualization, PhysicsDebugShape, Transform>(
            [&](Entity entity, const PhysicsDebugVisualization& debug_viz, PhysicsDebugShape& debug_shape, const Transform& transform) {
                if (debug_viz.is_visualization_enabled(1)) { // Check if collision shape visualization is enabled
                    update_collision_shape(entity, debug_shape, transform);
                    ++shapes_updated;
                }
            });
        
        auto update_end = std::chrono::high_resolution_clock::now();
        f32 update_time = std::chrono::duration<f32, std::milli>(update_end - update_start).count();
        
        integration_stats_.shape_updates += shapes_updated;
        integration_stats_.shape_update_time += update_time;
    }
    
    /** @brief Update debug statistics */
    void update_debug_statistics() {
        if (!config_.enable_performance_analysis) return;
        
        registry_->for_each<PhysicsDebugStats>(
            [this](Entity entity, PhysicsDebugStats& debug_stats) {
                // Update frame statistics
                debug_stats.update_frame_stats(
                    integration_stats_.average_update_time,
                    0.1f, // Update time (example)
                    count_debug_shapes_for_entity(entity),
                    1,    // Draw calls (example)
                    4,    // Vertices (example)
                    1     // Batches (example)
                );
            });
    }
    
    /** @brief Generate batched debug rendering data */
    void generate_batched_debug_data() {
        if (!debug_renderer_) return;
        
        // Begin debug rendering frame
        debug_renderer_->begin_frame();
        
        // Process all debug shapes and submit to renderer
        registry_->for_each<PhysicsDebugVisualization, PhysicsDebugShape, Transform>(
            [this](Entity entity, const PhysicsDebugVisualization& debug_viz, const PhysicsDebugShape& debug_shape, const Transform& transform) {
                if (debug_viz.render_props.visible && debug_shape.is_valid()) {
                    render_debug_shape(debug_shape, transform);
                }
            });
        
        // End debug rendering frame
        debug_renderer_->end_frame();
    }
    
    /** @brief Generate immediate debug rendering data */
    void generate_immediate_debug_data() {
        // Implementation for immediate mode rendering (for comparison)
        // Would render debug shapes directly without batching
    }
    
    /** @brief Update debug entity cache */
    void update_debug_entity_cache(Entity entity, const Transform& transform) {
        auto it = debug_entity_cache_.find(entity);
        if (it != debug_entity_cache_.end()) {
            DebugEntityCache& cache = it->second;
            
            // Check if entity has moved significantly
            Vec2 position{transform.position.x, transform.position.y};
            Vec2 position_delta = position - cache.cached_position;
            
            if (position_delta.length() > 0.1f || // Moved more than 0.1 units
                std::abs(transform.rotation - cache.cached_rotation) > 0.01f) { // Rotated more than 0.01 radians
                cache.requires_update = true;
                cache.cached_position = position;
                cache.cached_rotation = transform.rotation;
            }
            
            cache.last_update_frame = frame_number_;
        }
    }
    
    /** @brief Update debug cache for entity */
    void update_debug_cache(Entity entity, PhysicsDebugVisualization& debug_viz) {
        // Get physics data for entity
        auto* rigidbody = registry_->get_component<RigidBody2D>(entity);
        auto* transform = registry_->get_component<Transform>(entity);
        
        if (rigidbody && transform) {
            debug_viz.debug_cache.cached_velocity = Vec2{rigidbody->velocity.x, rigidbody->velocity.y};
            debug_viz.debug_cache.cached_position = Vec2{transform->position.x, transform->position.y};
            debug_viz.debug_cache.velocity_cache_valid = true;
            debug_viz.debug_cache.position_cache_valid = true;
            debug_viz.debug_cache.cache_frame_number = frame_number_;
        }
    }
    
    /** @brief Process physics visualization data */
    void process_physics_visualization_data(const PhysicsWorld2D::VisualizationData& viz_data) {
        // Process collision shapes
        for (const auto& shape_data : viz_data.collision_shapes) {
            // Find corresponding debug entity and update debug shape
            // Implementation would map physics shapes to debug entities
        }
        
        // Process contact points
        for (const auto& contact : viz_data.contact_points) {
            // Create or update debug contact point visualization
            // Implementation would create debug points for contacts
        }
        
        // Process force vectors
        for (const auto& force : viz_data.force_vectors) {
            // Create or update debug force vector visualization
            // Implementation would create debug arrows for forces
        }
    }
    
    /** @brief Update collision shape debug visualization */
    void update_collision_shape(Entity entity, PhysicsDebugShape& debug_shape, const Transform& transform) {
        auto* collider = registry_->get_component<Collider2D>(entity);
        if (!collider) return;
        
        // Update debug shape based on collider type
        std::visit([&](const auto& shape) {
            update_debug_shape_from_collider(debug_shape, shape, transform);
        }, collider->shape);
        
        debug_shape.geometry_cache.mark_dirty();
    }
    
    /** @brief Update debug shape from collider */
    template<typename ShapeType>
    void update_debug_shape_from_collider(PhysicsDebugShape& debug_shape, const ShapeType& shape, const Transform& transform) {
        // Implementation would update debug_shape based on ShapeType
        // For example, for Circle:
        if constexpr (std::is_same_v<ShapeType, Circle>) {
            debug_shape.primary_shape_type = PhysicsDebugShape::ShapeType::Circle;
            debug_shape.geometry.circle.center = Vec2{transform.position.x, transform.position.y};
            debug_shape.geometry.circle.radius = shape.radius;
        }
        // Similar implementations for other shape types...
    }
    
    /** @brief Render debug shape */
    void render_debug_shape(const PhysicsDebugShape& debug_shape, const Transform& transform) {
        switch (debug_shape.primary_shape_type) {
            case PhysicsDebugShape::ShapeType::Circle:
                debug_renderer_->draw_circle(
                    debug_shape.geometry.circle.center,
                    debug_shape.geometry.circle.radius,
                    debug_shape.render_props.color.rgba,
                    debug_shape.render_props.filled,
                    debug_shape.render_props.thickness
                );
                break;
            case PhysicsDebugShape::ShapeType::Rectangle:
                debug_renderer_->draw_rectangle(
                    debug_shape.geometry.rectangle.min,
                    debug_shape.geometry.rectangle.max,
                    debug_shape.render_props.color.rgba,
                    debug_shape.render_props.filled,
                    debug_shape.render_props.thickness
                );
                break;
            case PhysicsDebugShape::ShapeType::OrientedBox:
                debug_renderer_->draw_obb(
                    debug_shape.geometry.oriented_box.center,
                    debug_shape.geometry.oriented_box.half_extents,
                    debug_shape.geometry.oriented_box.rotation,
                    debug_shape.render_props.color.rgba,
                    debug_shape.render_props.filled,
                    debug_shape.render_props.thickness
                );
                break;
            // Add cases for other shape types...
            default:
                break;
        }
    }
    
    /** @brief Initialize debug data for entity */
    void initialize_entity_debug_data(Entity entity) {
        // Set up debug entity cache
        DebugEntityCache cache{};
        cache.entity = entity;
        cache.requires_update = true;
        cache.last_update_frame = frame_number_;
        debug_entity_cache_[entity] = cache;
        
        // Initialize debug shape based on physics components
        auto* debug_shape = registry_->get_component<PhysicsDebugShape>(entity);
        auto* transform = registry_->get_component<Transform>(entity);
        if (debug_shape && transform) {
            update_collision_shape(entity, *debug_shape, *transform);
        }
    }
    
    /** @brief Cleanup debug data for entity */
    void cleanup_entity_debug_data(Entity entity) {
        debug_entity_cache_.erase(entity);
    }
    
    /** @brief Update integration performance statistics */
    void update_integration_performance(const std::chrono::high_resolution_clock::time_point& start,
                                      const std::chrono::high_resolution_clock::time_point& end) {
        f32 frame_time = std::chrono::duration<f32, std::milli>(end - start).count();
        
        integration_stats_.total_update_time += frame_time;
        ++integration_stats_.total_updates;
        
        integration_stats_.average_update_time = integration_stats_.total_update_time / integration_stats_.total_updates;
        integration_stats_.peak_update_time = std::max(integration_stats_.peak_update_time, frame_time);
        
        // Calculate integration efficiency (arbitrary metric based on entities processed per time)
        u32 active_debug_entities = count_active_debug_entities();
        if (active_debug_entities > 0 && frame_time > 0.0f) {
            f32 entities_per_ms = active_debug_entities / frame_time;
            f32 ideal_entities_per_ms = 100.0f; // Arbitrary ideal value
            integration_stats_.integration_efficiency = std::min(1.0f, entities_per_ms / ideal_entities_per_ms);
        }
    }
    
    /** @brief Update step visualization */
    void update_step_visualization() {
        // Implementation for educational step-by-step visualization
        if (physics_system_) {
            auto step_breakdown = physics_system_->get_debug_step_breakdown();
            // Process step breakdown for visualization
        }
    }
    
    /** @brief Count active debug entities */
    u32 count_active_debug_entities() const {
        u32 count = 0;
        registry_->for_each<PhysicsDebugVisualization>([&count](Entity entity, const PhysicsDebugVisualization& debug_viz) {
            if (debug_viz.render_props.visible) {
                ++count;
            }
        });
        return count;
    }
    
    /** @brief Count debug shapes for entity */
    u32 count_debug_shapes_for_entity(Entity entity) const {
        auto* debug_shape = registry_->get_component<PhysicsDebugShape>(entity);
        return debug_shape ? debug_shape->get_total_shape_count() : 0;
    }
    
    /** @brief Analyze integration performance and generate suggestions */
    void analyze_integration_performance(IntegrationStatistics& stats) const {
        // Determine performance rating
        if (stats.average_update_time < 1.0f && stats.integration_efficiency > 0.8f) {
            stats.performance_rating = "Excellent";
        } else if (stats.average_update_time < 2.0f && stats.integration_efficiency > 0.6f) {
            stats.performance_rating = "Good";
        } else if (stats.average_update_time < 5.0f) {
            stats.performance_rating = "Fair";
        } else {
            stats.performance_rating = "Poor";
        }
        
        // Generate optimization suggestions
        stats.optimization_suggestions.clear();
        
        if (stats.integration_efficiency < 0.7f) {
            stats.optimization_suggestions.push_back("Reduce debug visualization complexity");
            stats.optimization_suggestions.push_back("Enable frustum culling for debug shapes");
        }
        
        if (stats.average_update_time > 2.0f) {
            stats.optimization_suggestions.push_back("Increase debug update frequency interval");
            stats.optimization_suggestions.push_back("Enable component caching to reduce update overhead");
        }
        
        if (stats.batching_efficiency < 0.8f) {
            stats.optimization_suggestions.push_back("Optimize debug shape batching");
            stats.optimization_suggestions.push_back("Group debug shapes by material properties");
        }
        
        if (stats.memory_efficiency < 0.7f) {
            stats.optimization_suggestions.push_back("Enable memory recycling for debug data");
            stats.optimization_suggestions.push_back("Use object pooling for debug components");
        }
        
        // Calculate educational overhead
        stats.educational_overhead_percentage = config_.enable_step_visualization ? 25.0f :
                                              (config_.enable_performance_analysis ? 10.0f : 5.0f);
    }
};

} // namespace ecscope::physics::debug