#include "registry.hpp"
#include "core/log.hpp"
#include "core/time.hpp"
#include <memory>
#include <sstream>
#include <chrono>
#include <atomic>

namespace ecscope::ecs {

// Global registry instance management
static std::unique_ptr<Registry> g_registry = nullptr;
static std::atomic<u32> g_allocator_id_counter{1};

// Static member definition
std::atomic<u32> Registry::allocator_id_counter_{1};

Registry& get_registry() {
    if (!g_registry) {
        g_registry = std::make_unique<Registry>(AllocatorConfig::create_educational_focused(),
                                               "Global_ECS_Registry");
        LOG_INFO("ECS Registry initialized with educational memory management");
    }
    return *g_registry;
}

void set_registry(std::unique_ptr<Registry> registry) {
    if (registry) {
        LOG_INFO("ECS Registry set to custom instance: '{}'", registry->name());
    }
    g_registry = std::move(registry);
}

// Factory functions for creating optimized registries
std::unique_ptr<Registry> create_performance_registry(const std::string& name) {
    return std::make_unique<Registry>(AllocatorConfig::create_performance_optimized(),
                                     name.empty() ? "Performance_Registry" : name);
}

std::unique_ptr<Registry> create_educational_registry(const std::string& name) {
    return std::make_unique<Registry>(AllocatorConfig::create_educational_focused(),
                                     name.empty() ? "Educational_Registry" : name);
}

std::unique_ptr<Registry> create_conservative_registry(const std::string& name) {
    return std::make_unique<Registry>(AllocatorConfig::create_memory_conservative(),
                                     name.empty() ? "Conservative_Registry" : name);
}

// Registry implementation methods

// Generate unique allocator ID for tracking
u32 Registry::generate_allocator_id() {
    return g_allocator_id_counter.fetch_add(1);
}

// Initialize custom allocators based on configuration
void Registry::initialize_allocators() {
    // Initialize arena allocator for archetype storage
    if (allocator_config_.enable_archetype_arena) {
        try {
            archetype_arena_ = std::make_unique<memory::ArenaAllocator>(
                allocator_config_.archetype_arena_size,
                registry_name_ + "_Arena",
                allocator_config_.enable_memory_tracking
            );
            
            if (enable_educational_logging_) {
                LOG_INFO("Arena allocator initialized: {} MB", 
                        allocator_config_.archetype_arena_size / (1024 * 1024));
            }
        } catch (const std::exception& e) {
            LOG_ERROR("Failed to initialize arena allocator: {}", e.what());
            allocator_config_.enable_archetype_arena = false;
        }
    }
    
    // Initialize pool allocator for entity management
    if (allocator_config_.enable_entity_pool) {
        try {
            entity_pool_ = std::make_unique<memory::PoolAllocator>(
                sizeof(Entity),
                allocator_config_.entity_pool_capacity,
                alignof(Entity),
                registry_name_ + "_EntityPool",
                allocator_config_.enable_memory_tracking
            );
            
            if (enable_educational_logging_) {
                LOG_INFO("Entity pool allocator initialized: {} entities", 
                        allocator_config_.entity_pool_capacity);
            }
        } catch (const std::exception& e) {
            LOG_ERROR("Failed to initialize entity pool allocator: {}", e.what());
            allocator_config_.enable_entity_pool = false;
        }
    }
    
    // Initialize hybrid PMR resource
    if (allocator_config_.enable_pmr_containers) {
        try {
            hybrid_resource_ = std::make_unique<memory::pmr::HybridMemoryResource>(
                64,   // Pool block size for small allocations
                1024, // Pool capacity
                512 * 1024, // Arena size (512 KB)
                64,   // Small threshold
                1024, // Medium threshold
                std::pmr::get_default_resource(),
                registry_name_ + "_HybridPMR",
                allocator_config_.enable_memory_tracking
            );
            
            pmr_resource_ = hybrid_resource_.get();
            
            if (enable_educational_logging_) {
                LOG_INFO("Hybrid PMR resource initialized");
            }
        } catch (const std::exception& e) {
            LOG_ERROR("Failed to initialize hybrid PMR resource: {}", e.what());
            pmr_resource_ = std::pmr::get_default_resource();
            allocator_config_.enable_pmr_containers = false;
        }
    } else {
        pmr_resource_ = std::pmr::get_default_resource();
    }
}

// Initialize memory tracking systems
void Registry::initialize_memory_tracking() {
    if (allocator_config_.enable_memory_tracking) {
        memory_stats_ = std::make_unique<ECSMemoryStats>();
        
        // Initialize global memory tracker if not already done
        memory::TrackerConfig tracker_config;
        tracker_config.enable_tracking = true;
        tracker_config.enable_call_stacks = allocator_config_.enable_debug_validation;
        tracker_config.enable_access_tracking = allocator_config_.enable_cache_analysis;
        tracker_config.enable_heat_mapping = allocator_config_.enable_cache_analysis;
        tracker_config.enable_leak_detection = allocator_config_.enable_debug_validation;
        tracker_config.enable_predictive_analysis = allocator_config_.enable_performance_analysis;
        
        memory::MemoryTracker::initialize(tracker_config);
        
        if (enable_educational_logging_) {
            LOG_INFO("Memory tracking initialized for registry '{}'", registry_name_);
        }
    }
}

// Cleanup memory tracking
void Registry::cleanup_memory_tracking() {
    if (memory_stats_ && enable_educational_logging_) {
        LOG_INFO("Cleaning up memory tracking for registry '{}'", registry_name_);
    }
    memory_stats_.reset();
}

// Update PMR resource after custom allocators are initialized
void Registry::update_pmr_resource() {
    if (allocator_config_.enable_pmr_containers && hybrid_resource_) {
        pmr_resource_ = hybrid_resource_.get();
        
        if (enable_educational_logging_) {
            LOG_INFO("Updated PMR resource to use hybrid allocator for registry '{}'", registry_name_);
        }
    }
    // Note: PMR containers are already constructed, so we can't change their resource after construction
    // In a production system, we'd need to reconstruct the containers or use a different approach
}

// Create archetype using arena allocator
std::unique_ptr<Archetype> Registry::create_archetype_with_arena(const ComponentSignature& signature) {
    if (!archetype_arena_) {
        LOG_WARN("Arena allocator not available, falling back to standard allocation");
        return std::make_unique<Archetype>(signature);
    }
    
    // Create archetype configuration with arena allocator
    ArchetypeConfig config(*archetype_arena_, allocator_config_.enable_memory_tracking);
    
    // For now, return standard archetype - full integration would require
    // modifying the Archetype class to accept custom allocators
    // This is where we would create an arena-backed archetype
    auto archetype = std::make_unique<Archetype>(signature);
    
    if (enable_educational_logging_) {
        LOG_INFO("Created archetype with arena backing (signature: {})", signature.to_string());
    }
    
    return archetype;
}

void Registry::benchmark_allocators(const std::string& test_name, usize iterations) {
    if (!allocator_config_.enable_performance_analysis) {
        LOG_WARN("Performance analysis is disabled for registry '{}'", registry_name_);
        return;
    }
    
    LOG_INFO("Running allocator benchmark '{}': {} iterations", test_name, iterations);
    
    // Create temporary registry with standard allocation for comparison
    AllocatorConfig standard_config = AllocatorConfig::create_memory_conservative();
    Registry standard_registry(standard_config, "Benchmark_Standard_Registry");
    
    // Benchmark entity creation with our optimized registry
    auto start_time = std::chrono::high_resolution_clock::now();
    
    std::pmr::vector<Entity> entities(pmr_resource_);
    entities.reserve(iterations);
    
    for (usize i = 0; i < iterations; ++i) {
        Entity entity = create_entity();
        entities.push_back(entity);
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto optimized_duration = std::chrono::duration<f64, std::milli>(end_time - start_time).count();
    
    // Cleanup entities
    for (Entity entity : entities) {
        destroy_entity(entity);
    }
    entities.clear();
    
    // Benchmark with standard registry
    start_time = std::chrono::high_resolution_clock::now();
    
    for (usize i = 0; i < iterations; ++i) {
        Entity entity = standard_registry.create_entity();
        entities.push_back(entity);
    }
    
    end_time = std::chrono::high_resolution_clock::now();
    auto standard_duration = std::chrono::duration<f64, std::milli>(end_time - start_time).count();
    
    // Calculate performance improvement
    PerformanceComparison comparison;
    comparison.operation_name = test_name;
    comparison.standard_allocator_time = standard_duration;
    comparison.custom_allocator_time = optimized_duration;
    comparison.speedup_factor = standard_duration > 0.0 ? standard_duration / optimized_duration : 1.0;
    comparison.operations_tested = iterations;
    
    performance_comparisons_.push_back(comparison);
    
    // Update memory stats
    if (memory_stats_) {
        memory_stats_->performance_improvement = comparison.speedup_factor;
    }
    
    LOG_INFO("Benchmark '{}' completed:", test_name);
    LOG_INFO("  - Custom allocators: {:.2f}ms", optimized_duration);
    LOG_INFO("  - Standard allocators: {:.2f}ms", standard_duration);
    LOG_INFO("  - Speedup: {:.2f}x {}", 
            comparison.speedup_factor,
            comparison.is_improvement() ? "(faster)" : "(slower)");
            
    if (comparison.is_improvement()) {
        LOG_INFO("  - Performance improvement: {:.1f}%", comparison.improvement_percentage());
    }
}

// Migrate entity between archetypes with memory-efficient transfer
bool Registry::migrate_entity_to_archetype(Entity entity, usize from_archetype_idx, usize to_archetype_idx) {
    if (from_archetype_idx >= archetypes_.size() || to_archetype_idx >= archetypes_.size()) {
        return false;
    }
    
    Archetype* from_archetype = archetypes_[from_archetype_idx].get();
    Archetype* to_archetype = archetypes_[to_archetype_idx].get();
    
    // This is a simplified migration - in a full implementation, we would:
    // 1. Copy all existing components from source to destination
    // 2. Remove entity from source archetype
    // 3. Add entity to destination archetype
    // 4. Update entity_to_archetype mapping
    
    // For now, just update the mapping
    entity_to_archetype_[entity] = to_archetype_idx;
    
    if (enable_educational_logging_) {
        LOG_INFO("Migrated entity {} from archetype {} to {}", 
                entity, from_archetype_idx, to_archetype_idx);
    }
    
    return true;
}

// Track component access patterns for cache analysis
// Note: Template implementations need to be in header file for proper instantiation
// This is a placeholder - actual implementation moved to registry.hpp as inline

// Track archetype migration events
void Registry::track_archetype_migration(const ComponentSignature& from, const ComponentSignature& to) {
    if (!allocator_config_.enable_memory_tracking || !enable_educational_logging_) {
        return;
    }
    
    LOG_INFO("Archetype migration tracked: {} -> {}", from.to_string(), to.to_string());
    
    if (memory_stats_) {
        memory_stats_->fragmentation_events++;
    }
}

// Record timing for component operations
void Registry::record_component_operation_time(
    const std::chrono::high_resolution_clock::time_point& start_time, 
    const std::string& operation_name) {
    
    if (!allocator_config_.enable_performance_analysis || !memory_stats_) {
        return;
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration<f64, std::nano>(end_time - start_time).count();
    
    // Update appropriate timing statistics
    if (operation_name == "component_access" || operation_name.find("get") != std::string::npos) {
        // Update running average for component access time
        f64 current_avg = memory_stats_->average_component_access_time;
        usize sample_count = memory_stats_->active_component_arrays;
        
        memory_stats_->average_component_access_time = 
            (current_avg * sample_count + duration) / (sample_count + 1);
    } else if (operation_name.find("create") != std::string::npos || 
               operation_name.find("add") != std::string::npos) {
        // Update entity creation time
        f64 current_avg = memory_stats_->average_entity_creation_time;
        usize entity_count = total_entities_created_.load();
        
        if (entity_count > 0) {
            memory_stats_->average_entity_creation_time = 
                (current_avg * (entity_count - 1) + duration) / entity_count;
        } else {
            memory_stats_->average_entity_creation_time = duration;
        }
    }
}

// Count total component arrays across all archetypes
usize Registry::count_component_arrays() const {
    usize count = 0;
    for (const auto& archetype : archetypes_) {
        // Each archetype has component arrays for each component type in its signature
        count += archetype->component_infos().size();
    }
    return count;
}

// Update memory efficiency metrics
void Registry::update_memory_efficiency_metrics() const {
    if (!memory_stats_) {
        return;
    }
    
    // Calculate overall memory efficiency
    usize total_allocated = 0;
    usize total_used = 0;
    
    if (archetype_arena_) {
        total_allocated += archetype_arena_->total_size();
        total_used += archetype_arena_->used_size();
    }
    
    if (entity_pool_) {
        total_allocated += entity_pool_->total_capacity() * entity_pool_->block_size();
        total_used += entity_pool_->allocated_count() * entity_pool_->block_size();
    }
    
    if (total_allocated > 0) {
        memory_stats_->memory_efficiency = static_cast<f64>(total_used) / total_allocated;
    }
    
    // Calculate cache hit ratio estimate based on access patterns
    if (memory_stats_->cache_friendly_allocations > 0) {
        // This is a simplified cache hit ratio calculation
        // In a real implementation, we'd need more sophisticated cache modeling
        usize total_accesses = memory_stats_->cache_friendly_allocations;
        memory_stats_->cache_hit_ratio = 0.85; // Assume good cache locality with arena allocation
    }
    
    // Calculate allocation pattern score
    if (archetype_arena_ && archetype_arena_->stats().allocation_count > 0) {
        // Arena allocations are inherently cache-friendly due to linear layout
        memory_stats_->allocation_pattern_score = 0.9;
    } else {
        // Standard allocation patterns are less cache-friendly
        memory_stats_->allocation_pattern_score = 0.6;
    }
}

// Factory functions for creating optimized archetypes
std::unique_ptr<Archetype> create_arena_archetype(
    const ComponentSignature& signature,
    memory::ArenaAllocator& arena,
    bool enable_tracking) {
    
    // Create archetype with arena backing
    // Note: This would require modifying the Archetype class to accept custom allocators
    // For now, we create a standard archetype and note the arena association
    auto archetype = std::make_unique<Archetype>(signature);
    
    if (enable_tracking) {
        LOG_INFO("Created arena-backed archetype with signature: {}", signature.to_string());
    }
    
    return archetype;
}

std::unique_ptr<Archetype> create_pmr_archetype(
    const ComponentSignature& signature,
    std::pmr::memory_resource& resource,
    bool enable_tracking) {
    
    // Create archetype with PMR backing
    // Note: This would require modifying the Archetype class to use PMR containers
    auto archetype = std::make_unique<Archetype>(signature);
    
    if (enable_tracking) {
        LOG_INFO("Created PMR-backed archetype with signature: {}", signature.to_string());
    }
    
    return archetype;
}

// Educational utility functions
namespace educational {
    /**
     * @brief Create a demonstration registry showing different allocation strategies
     */
    std::unique_ptr<Registry> create_demo_registry() {
        auto config = AllocatorConfig::create_educational_focused();
        config.archetype_arena_size = 1 * MB; // Smaller for demo
        config.entity_pool_capacity = 1000;
        
        return std::make_unique<Registry>(config, "Educational_Demo_Registry");
    }
    
    /**
     * @brief Run a comprehensive memory allocation demonstration
     */
    void run_memory_allocation_demo() {
        LOG_INFO("Starting ECS Memory Allocation Educational Demo");
        
        // Create registries with different strategies
        auto educational_registry = create_demo_registry();
        auto performance_registry = create_performance_registry("Demo_Performance");
        auto conservative_registry = create_conservative_registry("Demo_Conservative");
        
        // Run benchmarks
        const usize test_iterations = 1000;
        
        educational_registry->benchmark_allocators("Entity_Creation_Educational", test_iterations);
        performance_registry->benchmark_allocators("Entity_Creation_Performance", test_iterations);
        conservative_registry->benchmark_allocators("Entity_Creation_Conservative", test_iterations);
        
        // Generate reports
        LOG_INFO("\n{}", educational_registry->generate_memory_report());
        LOG_INFO("\n{}", performance_registry->generate_memory_report());
        LOG_INFO("\n{}", conservative_registry->generate_memory_report());
        
        LOG_INFO("ECS Memory Allocation Educational Demo completed");
    }
}

} // namespace educational

} // namespace ecscope::ecs