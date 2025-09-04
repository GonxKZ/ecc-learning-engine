#pragma once

#include "core/types.hpp"
#include "core/log.hpp"
#include "entity.hpp"
#include "component.hpp"
#include "signature.hpp"
#include "archetype.hpp"
#include "memory/arena.hpp"
#include "memory/pool.hpp"
#include "memory/pmr_adapters.hpp"
#include "memory/memory_tracker.hpp"
#include <unordered_map>
#include <memory>
#include <vector>
#include <type_traits>
#include <memory_resource>
#include <string>
#include <optional>
#include <chrono>

namespace ecscope::ecs {

// Forward declarations
class Registry;

/**
 * @brief Memory allocation configuration for ECS Registry
 * 
 * Configures which allocators to use for different ECS components and provides
 * educational insights into memory allocation strategies. This configuration
 * allows experimentation with different memory management approaches.
 */
struct AllocatorConfig {
    // Core allocation strategies
    bool enable_archetype_arena{true};     // Use arena allocator for archetype storage
    bool enable_entity_pool{true};         // Use pool allocator for entity management
    bool enable_pmr_containers{true};      // Use PMR containers for registry data structures
    
    // Arena allocator settings
    usize archetype_arena_size{4 * MB};    // Size of arena for archetype storage
    usize entity_pool_capacity{10000};     // Initial entity pool capacity
    
    // Performance and debugging settings
    bool enable_memory_tracking{true};     // Enable comprehensive memory tracking
    bool enable_performance_analysis{true}; // Enable allocation performance comparison
    bool enable_cache_analysis{true};      // Enable cache behavior analysis
    bool enable_debug_validation{true};    // Enable additional validation checks
    
    // Memory pressure handling
    bool enable_pressure_monitoring{true}; // Monitor memory pressure
    f64 pressure_warning_threshold{0.8};   // Warn when 80% memory used
    f64 pressure_critical_threshold{0.95}; // Critical when 95% memory used
    
    // Factory methods for common configurations
    static AllocatorConfig create_educational_focused() {
        AllocatorConfig config;
        config.enable_memory_tracking = true;
        config.enable_performance_analysis = true;
        config.enable_cache_analysis = true;
        config.enable_debug_validation = true;
        config.archetype_arena_size = 2 * MB;  // Smaller for educational examples
        return config;
    }
    
    static AllocatorConfig create_performance_optimized() {
        AllocatorConfig config;
        config.enable_memory_tracking = false;  // Minimal overhead
        config.enable_performance_analysis = false;
        config.enable_cache_analysis = false;
        config.enable_debug_validation = false;
        config.archetype_arena_size = 16 * MB;  // Larger for performance
        config.entity_pool_capacity = 50000;
        return config;
    }
    
    static AllocatorConfig create_memory_conservative() {
        AllocatorConfig config;
        config.enable_archetype_arena = false;  // Use standard allocation
        config.enable_entity_pool = false;
        config.enable_pmr_containers = false;
        config.enable_memory_tracking = true;   // Still track for analysis
        return config;
    }
};

/**
 * @brief Component array allocation strategy selection
 */
enum class ComponentArrayAllocator {
    Standard,    // Use standard malloc/free
    Arena,       // Use arena allocator (linear allocation)
    Pool,        // Use pool allocator (fixed-size blocks)
    PMR          // Use PMR memory resource
};

/**
 * @brief Configuration for archetype memory allocation
 */
struct ArchetypeConfig {
    ComponentArrayAllocator default_allocator{ComponentArrayAllocator::Arena};
    memory::ArenaAllocator* arena_allocator{nullptr};
    std::pmr::memory_resource* pmr_resource{nullptr};
    bool enable_memory_tracking{true};
    bool use_pmr_containers{false};
    usize initial_capacity{1024};
    
    ArchetypeConfig() = default;
    
    ArchetypeConfig(memory::ArenaAllocator& arena, bool tracking = true)
        : default_allocator(ComponentArrayAllocator::Arena)
        , arena_allocator(&arena)
        , enable_memory_tracking(tracking) {}
};

/**
 * @brief Memory statistics for ECS Registry
 */
struct ECSMemoryStats {
    // Allocation counts
    usize total_entities_created{0};
    usize active_entities{0};
    usize total_archetypes{0};
    usize active_component_arrays{0};
    
    // Memory usage
    usize archetype_arena_used{0};
    usize archetype_arena_total{0};
    usize entity_pool_used{0};
    usize entity_pool_total{0};
    usize pmr_containers_used{0};
    
    // Performance metrics
    f64 average_entity_creation_time{0.0};
    f64 average_component_access_time{0.0};
    f64 cache_hit_ratio{0.0};
    f64 memory_efficiency{0.0};
    f64 performance_improvement{0.0}; // vs standard allocation
    
    // Educational insights
    usize cache_friendly_allocations{0};
    usize fragmentation_events{0};
    f64 allocation_pattern_score{0.0}; // How cache-friendly our patterns are
    
    void reset() {
        *this = ECSMemoryStats{};
    }
    
    f64 arena_utilization() const {
        return archetype_arena_total > 0 ? 
            static_cast<f64>(archetype_arena_used) / archetype_arena_total : 0.0;
    }
    
    f64 pool_utilization() const {
        return entity_pool_total > 0 ?
            static_cast<f64>(entity_pool_used) / entity_pool_total : 0.0;
    }
};

/**
 * @brief Performance comparison between allocation strategies
 */
struct PerformanceComparison {
    std::string operation_name;
    f64 standard_allocator_time{0.0};  // Time with standard malloc/free
    f64 custom_allocator_time{0.0};    // Time with custom allocators
    f64 speedup_factor{0.0};           // custom vs standard (>1 = faster)
    usize operations_tested{0};
    
    bool is_improvement() const {
        return speedup_factor > 1.0;
    }
    
    f64 improvement_percentage() const {
        return (speedup_factor - 1.0) * 100.0;
    }
};

/**
 * @brief Enhanced ECS Registry with Custom Memory Management Integration
 * 
 * This registry provides comprehensive memory management integration with Arena,
 * Pool, and PMR allocators for educational and performance benefits. It demonstrates
 * how different allocation strategies affect ECS performance and memory usage patterns.
 * 
 * Key Educational Features:
 * - Multiple allocation strategy support
 * - Real-time memory usage monitoring
 * - Performance comparison between strategies
 * - Cache-friendly memory layout optimization
 * - Comprehensive memory tracking and visualization
 * 
 * Memory Management Architecture:
 * - Arena allocators for archetype component storage (SoA arrays)
 * - Pool allocators for entity ID management and recycling
 * - PMR containers for registry internal data structures
 * - Memory tracker integration for comprehensive analysis
 * 
 * Performance Benefits:
 * - Cache-friendly component storage through arena allocation
 * - Fast entity creation/destruction through pool allocation
 * - Reduced memory fragmentation
 * - Predictable allocation patterns
 * - Zero-overhead abstractions when tracking is disabled
 */
class Registry {
private:
    // Core ECS data structures with PMR support
    std::pmr::memory_resource* pmr_resource_;
    std::pmr::unordered_map<Entity, usize> entity_to_archetype_;     // Maps entity to archetype index
    std::pmr::vector<std::unique_ptr<Archetype>> archetypes_;        // All archetypes
    std::pmr::unordered_map<ComponentSignature, usize> signature_to_archetype_; // Fast archetype lookup
    
    // Custom allocator management
    AllocatorConfig allocator_config_;
    std::unique_ptr<memory::ArenaAllocator> archetype_arena_;        // Arena for archetype storage
    std::unique_ptr<memory::PoolAllocator> entity_pool_;             // Pool for entity management
    std::unique_ptr<memory::pmr::HybridMemoryResource> hybrid_resource_; // Hybrid PMR resource
    
    // Memory tracking and statistics
    std::unique_ptr<ECSMemoryStats> memory_stats_;
    std::pmr::vector<PerformanceComparison> performance_comparisons_;
    u32 allocator_instance_id_;                                      // Unique ID for tracking
    
    // Educational and debugging features
    bool enable_educational_logging_;
    std::pmr::string registry_name_;
    std::chrono::high_resolution_clock::time_point creation_time_;
    
    // Statistics for observability
    std::atomic<usize> total_entities_created_{0};
    std::atomic<usize> active_entities_{0};
    
public:
    /**
     * @brief Create registry with specified allocator configuration
     * 
     * @param config Memory allocation configuration
     * @param name Human-readable name for debugging and visualization
     */
    explicit Registry(const AllocatorConfig& config = AllocatorConfig::create_educational_focused(),
                     const std::string& name = "ECS_Registry")
        : pmr_resource_(std::pmr::get_default_resource()) // Initialize first
        , entity_to_archetype_(pmr_resource_)
        , archetypes_(pmr_resource_)
        , signature_to_archetype_(pmr_resource_)
        , allocator_config_(config)
        , performance_comparisons_(pmr_resource_)
        , allocator_instance_id_(generate_allocator_id())
        , enable_educational_logging_(config.enable_memory_tracking)
        , registry_name_(name, pmr_resource_)
        , creation_time_(std::chrono::high_resolution_clock::now()) {
        
        initialize_allocators();
        initialize_memory_tracking();
        
        // Update PMR resource after custom allocators are initialized
        update_pmr_resource();
        
        // Reserve initial capacity with consideration for custom allocators
        const usize initial_entity_capacity = config.entity_pool_capacity;
        const usize initial_archetype_capacity = 64;
        
        entity_to_archetype_.reserve(initial_entity_capacity);
        archetypes_.reserve(initial_archetype_capacity);
        signature_to_archetype_.reserve(initial_archetype_capacity);
        
        if (enable_educational_logging_) {
            LOG_INFO("ECS Registry '{}' created with custom memory management", name);
            LOG_INFO("  - Arena allocator: {} (size: {} MB)", 
                    config.enable_archetype_arena ? "enabled" : "disabled",
                    config.archetype_arena_size / (1024 * 1024));
            LOG_INFO("  - Entity pool: {} (capacity: {})", 
                    config.enable_entity_pool ? "enabled" : "disabled",
                    config.entity_pool_capacity);
            LOG_INFO("  - PMR containers: {}", 
                    config.enable_pmr_containers ? "enabled" : "disabled");
        }
    }
    
    /**
     * @brief Destructor with comprehensive cleanup and final statistics
     */
    ~Registry() {
        if (enable_educational_logging_) {
            auto end_time = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration<f64>(end_time - creation_time_).count();
            
            LOG_INFO("ECS Registry '{}' destroyed after {:.2f} seconds", registry_name_.c_str(), duration);
            LOG_INFO("Final statistics:");
            LOG_INFO("  - Total entities created: {}", total_entities_created_.load());
            LOG_INFO("  - Peak active entities: {}", active_entities_.load());
            LOG_INFO("  - Total archetypes: {}", archetypes_.size());
            
            if (memory_stats_) {
                LOG_INFO("Memory efficiency: {:.2f}%", memory_stats_->memory_efficiency * 100.0);
                LOG_INFO("Arena utilization: {:.2f}%", memory_stats_->arena_utilization() * 100.0);
                LOG_INFO("Pool utilization: {:.2f}%", memory_stats_->pool_utilization() * 100.0);
                
                if (memory_stats_->performance_improvement != 0.0) {
                    LOG_INFO("Performance improvement: {:.2f}x vs standard allocation", 
                            memory_stats_->performance_improvement);
                }
            }
        }
        
        cleanup_memory_tracking();
    }
    
    // Non-copyable but movable
    Registry(const Registry&) = delete;
    Registry& operator=(const Registry&) = delete;
    Registry(Registry&&) = default;
    Registry& operator=(Registry&&) = default;
    
    // Create entity with components
    template<typename... Components>
    requires (Component<std::remove_cvref_t<Components>> && ...)
    Entity create_entity(Components&&... components) {
        using ComponentTypes = std::tuple<std::remove_cvref_t<Components>...>;
        ComponentSignature signature = make_signature<std::remove_cvref_t<Components>...>();
        Archetype* archetype = get_or_create_archetype(signature);
        
        // Register component types in archetype if not already present
        (archetype->add_component_type<std::remove_cvref_t<Components>>(), ...);
        
        // Create entity in archetype
        Entity entity = archetype->create_entity(std::forward<Components>(components)...);
        
        // Update tracking
        entity_to_archetype_[entity] = get_archetype_index(*archetype);
        ++total_entities_created_;
        ++active_entities_;
        
        return entity;
    }
    
    // Create entity without components
    Entity create_entity() {
        ComponentSignature empty_signature;
        Archetype* archetype = get_or_create_archetype(empty_signature);
        
        Entity entity = archetype->create_entity();
        entity_to_archetype_[entity] = get_archetype_index(*archetype);
        ++total_entities_created_;
        ++active_entities_;
        
        return entity;
    }
    
    // Destroy entity
    bool destroy_entity(Entity entity) {
        auto it = entity_to_archetype_.find(entity);
        if (it == entity_to_archetype_.end()) {
            return false; // Entity doesn't exist
        }
        
        usize archetype_index = it->second;
        if (archetype_index < archetypes_.size()) {
            archetypes_[archetype_index]->remove_entity(entity);
        }
        
        entity_to_archetype_.erase(it);
        --active_entities_;
        return true;
    }
    
    // Check if entity exists
    bool is_valid(Entity entity) const {
        return entity_to_archetype_.find(entity) != entity_to_archetype_.end();
    }
    
    // Add component to entity with memory-efficient archetype migration
    template<Component T>
    bool add_component(Entity entity, const T& component) {
        auto start_time = std::chrono::high_resolution_clock::now();
        
        auto it = entity_to_archetype_.find(entity);
        if (it == entity_to_archetype_.end()) {
            if (enable_educational_logging_) {
                LOG_WARN("Attempted to add component to non-existent entity {}", entity);
            }
            return false;
        }
        
        usize current_archetype_index = it->second;
        Archetype* current_archetype = archetypes_[current_archetype_index].get();
        
        // If entity already has this component, update it
        if (current_archetype->has_component<T>()) {
            bool result = current_archetype->add_component_to_entity(entity, component);
            
            // Track memory access for cache analysis
            if (allocator_config_.enable_cache_analysis) {
                track_component_access<T>(entity, false); // write access
            }
            
            record_component_operation_time(start_time, "component_update");
            return result;
        }
        
        // Entity doesn't have this component, need to migrate to new archetype
        ComponentSignature new_signature = current_archetype->signature();
        new_signature.set<T>();
        
        Archetype* new_archetype = get_or_create_archetype(new_signature);
        new_archetype->add_component_type<T>();
        
        // Perform memory-efficient archetype migration
        bool migration_result = migrate_entity_to_archetype(entity, current_archetype_index, 
                                                           get_archetype_index(*new_archetype));
        
        if (migration_result) {
            // Add the new component to the migrated entity
            bool add_result = new_archetype->add_component_to_entity(entity, component);
            
            if (allocator_config_.enable_memory_tracking) {
                track_archetype_migration(current_archetype->signature(), new_signature);
            }
            
            record_component_operation_time(start_time, "component_add_with_migration");
            return add_result;
        }
        
        return false;
    }
    
    // Remove component from entity
    template<Component T>
    bool remove_component(Entity entity) {
        // TODO: Implement component removal with archetype migration
        // This is complex as it requires moving the entity to a new archetype
        // For now, return false (not implemented)
        LOG_WARN("Component removal not yet implemented");
        return false;
    }
    
    // Get component from entity with access pattern tracking
    template<Component T>
    T* get_component(Entity entity) {
        auto it = entity_to_archetype_.find(entity);
        if (it == entity_to_archetype_.end()) {
            return nullptr;
        }
        
        usize archetype_index = it->second;
        if (archetype_index < archetypes_.size()) {
            T* component = archetypes_[archetype_index]->get_component<T>(entity);
            
            // Track memory access for cache analysis and educational insights
            if (component && allocator_config_.enable_cache_analysis) {
                track_component_access<T>(entity, true); // read access
            }
            
            return component;
        }
        
        return nullptr;
    }
    
    template<Component T>
    const T* get_component(Entity entity) const {
        auto it = entity_to_archetype_.find(entity);
        if (it == entity_to_archetype_.end()) {
            return nullptr;
        }
        
        usize archetype_index = it->second;
        if (archetype_index < archetypes_.size()) {
            const T* component = archetypes_[archetype_index]->get_component<T>(entity);
            
            // Track memory access for cache analysis (const version)
            if (component && allocator_config_.enable_cache_analysis) {
                // Note: Using const_cast for tracking purposes only - doesn't modify component data
                const_cast<Registry*>(this)->track_component_access<T>(entity, true);
            }
            
            return component;
        }
        
        return nullptr;
    }
    
    // Check if entity has component
    template<Component T>
    bool has_component(Entity entity) const {
        return get_component<T>(entity) != nullptr;
    }
    
    // Get all entities (for debugging/inspection)
    std::vector<Entity> get_all_entities() const {
        std::vector<Entity> all_entities;
        all_entities.reserve(active_entities_);
        
        for (const auto& archetype : archetypes_) {
            const auto& entities = archetype->entities();
            all_entities.insert(all_entities.end(), entities.begin(), entities.end());
        }
        
        return all_entities;
    }
    
    // Get entities with specific components
    template<Component... Components>
    std::vector<Entity> get_entities_with() const {
        ComponentSignature required = make_signature<Components...>();
        std::vector<Entity> matching_entities;
        
        for (const auto& archetype : archetypes_) {
            if (archetype->signature().is_superset_of(required)) {
                const auto& entities = archetype->entities();
                matching_entities.insert(matching_entities.end(), entities.begin(), entities.end());
            }
        }
        
        return matching_entities;
    }
    
    // Iterate over all entities with specific components
    template<Component... Components>
    void for_each(auto&& func) const {
        ComponentSignature required = make_signature<Components...>();
        
        for (const auto& archetype : archetypes_) {
            if (archetype->signature().is_superset_of(required)) {
                const auto& entities = archetype->entities();
                
                for (Entity entity : entities) {
                    auto components = std::make_tuple(archetype->get_component<Components>(entity)...);
                    
                    // Check if all components are valid
                    bool all_valid = (std::get<Components*>(components) && ...);
                    if (all_valid) {
                        func(entity, *std::get<Components*>(components)...);
                    }
                }
            }
        }
    }
    
    // Statistics
    usize total_entities_created() const { return total_entities_created_; }
    usize active_entities() const { return active_entities_; }
    usize archetype_count() const { return archetypes_.size(); }
    
    // Get archetype information for UI/debugging
    std::vector<std::pair<ComponentSignature, usize>> get_archetype_stats() const {
        std::vector<std::pair<ComponentSignature, usize>> stats;
        stats.reserve(archetypes_.size());
        
        for (const auto& archetype : archetypes_) {
            stats.emplace_back(archetype->signature(), archetype->entity_count());
        }
        
        return stats;
    }
    
    // Comprehensive memory usage information with allocator breakdown
    usize memory_usage() const {
        usize total = sizeof(*this);
        
        // PMR container memory usage
        total += entity_to_archetype_.size() * (sizeof(Entity) + sizeof(usize));
        total += signature_to_archetype_.size() * (sizeof(ComponentSignature) + sizeof(usize));
        
        // Archetype memory usage (includes custom allocator memory)
        for (const auto& archetype : archetypes_) {
            total += archetype->memory_usage();
        }
        
        // Custom allocator overhead
        if (archetype_arena_) {
            total += archetype_arena_->total_size();
        }
        
        if (entity_pool_) {
            total += entity_pool_->total_capacity() * entity_pool_->block_size();
        }
        
        // Memory tracking overhead
        if (memory_stats_) {
            total += sizeof(ECSMemoryStats);
        }
        
        return total;
    }
    
    /**
     * @brief Get detailed memory statistics with educational insights
     */
    ECSMemoryStats get_memory_statistics() const {
        if (!memory_stats_) {
            return ECSMemoryStats{};
        }
        
        // Update current statistics
        memory_stats_->active_entities = active_entities_.load();
        memory_stats_->total_entities_created = total_entities_created_.load();
        memory_stats_->total_archetypes = archetypes_.size();
        memory_stats_->active_component_arrays = count_component_arrays();
        
        // Update allocator utilization
        if (archetype_arena_) {
            memory_stats_->archetype_arena_used = archetype_arena_->used_size();
            memory_stats_->archetype_arena_total = archetype_arena_->total_size();
        }
        
        if (entity_pool_) {
            memory_stats_->entity_pool_used = entity_pool_->allocated_count() * entity_pool_->block_size();
            memory_stats_->entity_pool_total = entity_pool_->total_capacity() * entity_pool_->block_size();
        }
        
        // Calculate efficiency metrics
        update_memory_efficiency_metrics();
        
        return *memory_stats_;
    }
    
    /**
     * @brief Get performance comparison results
     */
    const std::pmr::vector<PerformanceComparison>& get_performance_comparisons() const {
        return performance_comparisons_;
    }
    
    /**
     * @brief Get allocator configuration
     */
    const AllocatorConfig& get_allocator_config() const {
        return allocator_config_;
    }
    
    /**
     * @brief Get registry name for UI display
     */
    const std::string& name() const {
        return registry_name_;
    }
    
    /**
     * @brief Generate comprehensive memory usage report
     */
    std::string generate_memory_report() const {
        std::ostringstream oss;
        
        auto stats = get_memory_statistics();
        
        oss << "=== ECS Registry Memory Report: " << registry_name_ << " ===\n";
        oss << "Entities: " << stats.active_entities << " active, " 
            << stats.total_entities_created << " total created\n";
        oss << "Archetypes: " << stats.total_archetypes << "\n";
        oss << "Component Arrays: " << stats.active_component_arrays << "\n";
        
        oss << "\n--- Memory Usage ---\n";
        oss << "Total Memory: " << (memory_usage() / 1024) << " KB\n";
        
        if (archetype_arena_) {
            oss << "Arena Utilization: " << (stats.arena_utilization() * 100.0) << "%\n";
            oss << "Arena Used: " << (stats.archetype_arena_used / 1024) << " KB\n";
        }
        
        if (entity_pool_) {
            oss << "Pool Utilization: " << (stats.pool_utilization() * 100.0) << "%\n";
            oss << "Pool Used: " << (stats.entity_pool_used / 1024) << " KB\n";
        }
        
        oss << "\n--- Performance Metrics ---\n";
        oss << "Memory Efficiency: " << (stats.memory_efficiency * 100.0) << "%\n";
        oss << "Cache Hit Ratio: " << (stats.cache_hit_ratio * 100.0) << "%\n";
        
        if (stats.performance_improvement > 0.0) {
            oss << "Performance vs Standard: " << stats.performance_improvement << "x faster\n";
        }
        
        return oss.str();
    }
    
    // Clear all entities with memory cleanup
    void clear() {
        if (enable_educational_logging_) {
            LOG_INFO("Clearing ECS Registry '{}' - final stats before cleanup:", registry_name_);
            LOG_INFO("  - Active entities: {}", active_entities_.load());
            LOG_INFO("  - Total archetypes: {}", archetypes_.size());
        }
        
        // Clear ECS data
        entity_to_archetype_.clear();
        archetypes_.clear();
        signature_to_archetype_.clear();
        
        // Reset counters
        total_entities_created_ = 0;
        active_entities_ = 0;
        
        // Reset custom allocators for fresh start
        if (archetype_arena_) {
            archetype_arena_->reset();
        }
        
        if (entity_pool_) {
            entity_pool_->reset();
        }
        
        // Reset statistics
        if (memory_stats_) {
            memory_stats_->reset();
        }
        
        performance_comparisons_.clear();
        
        if (enable_educational_logging_) {
            LOG_INFO("ECS Registry '{}' cleared and reset", registry_name_);
        }
    }
    
    /**
     * @brief Run educational benchmark comparing allocation strategies
     */
    void benchmark_allocators(const std::string& test_name, usize iterations = 10000);
    
    /**
     * @brief Force garbage collection of unused memory
     */
    void compact_memory() {
        if (archetype_arena_) {
            // Arena allocators don't support individual deallocation,
            // but we can provide guidance on when to reset
            if (enable_educational_logging_) {
                auto utilization = memory_stats_ ? memory_stats_->arena_utilization() : 0.0;
                LOG_INFO("Arena utilization: {:.2f}% - consider reset if low and fragmented", 
                        utilization * 100.0);
            }
        }
        
        if (entity_pool_) {
            usize removed_chunks = entity_pool_->shrink_pool();
            if (removed_chunks > 0 && enable_educational_logging_) {
                LOG_INFO("Compacted entity pool - removed {} unused chunks", removed_chunks);
            }
        }
    }
    
private:
    // Get or create archetype for signature with custom allocator support
    Archetype* get_or_create_archetype(const ComponentSignature& signature) {
        auto it = signature_to_archetype_.find(signature);
        if (it != signature_to_archetype_.end()) {
            return archetypes_[it->second].get();
        }
        
        // Create new archetype with custom allocator configuration
        std::unique_ptr<Archetype> archetype = create_archetype_with_allocators(signature);
        Archetype* archetype_ptr = archetype.get();
        
        usize index = archetypes_.size();
        archetypes_.push_back(std::move(archetype));
        signature_to_archetype_[signature] = index;
        
        // Update statistics
        if (memory_stats_) {
            memory_stats_->total_archetypes++;
        }
        
        if (enable_educational_logging_) {
            LOG_INFO("Created archetype #{} with signature: {}", 
                    index, signature.to_string());
        }
        
        return archetype_ptr;
    }
    
    // Create archetype with appropriate allocator strategy
    std::unique_ptr<Archetype> create_archetype_with_allocators(const ComponentSignature& signature) {
        if (allocator_config_.enable_archetype_arena && archetype_arena_) {
            return create_archetype_with_arena(signature);
        } else {
            // Fallback to standard archetype
            return std::make_unique<Archetype>(signature);
        }
    }
    
    // Create archetype using arena allocator
    std::unique_ptr<Archetype> create_archetype_with_arena(const ComponentSignature& signature);
    
    // Migrate entity between archetypes with memory-efficient transfer
    bool migrate_entity_to_archetype(Entity entity, usize from_archetype_idx, usize to_archetype_idx);
    
    // Initialize custom allocators based on configuration
    void initialize_allocators();
    
    // Initialize memory tracking systems
    void initialize_memory_tracking();
    
    // Cleanup memory tracking
    void cleanup_memory_tracking();
    
    // Update PMR resource after initialization
    void update_pmr_resource();
    
    // Generate unique allocator ID for tracking
    static u32 generate_allocator_id();
    
    // Track component access patterns for cache analysis
    template<Component T>
    void track_component_access(Entity entity, bool is_read_access);
    
    // Track archetype migration events
    void track_archetype_migration(const ComponentSignature& from, const ComponentSignature& to);
    
    // Record timing for component operations
    void record_component_operation_time(const std::chrono::high_resolution_clock::time_point& start_time, 
                                        const std::string& operation_name);
    
    // Count total component arrays across all archetypes
    usize count_component_arrays() const;
    
    // Update memory efficiency metrics
    void update_memory_efficiency_metrics() const;
    
    // Static counter for unique allocator IDs
    static std::atomic<u32> allocator_id_counter_;
    
    // Get archetype index for a given archetype
    usize get_archetype_index(const Archetype& archetype) const {
        for (usize i = 0; i < archetypes_.size(); ++i) {
            if (archetypes_[i].get() == &archetype) {
                return i;
            }
        }
        return static_cast<usize>(-1); // Not found
    }
};

// Global registry instance (for convenience)
Registry& get_registry();
void set_registry(std::unique_ptr<Registry> registry);

// Template implementations (must be in header for proper instantiation)

// Track component access patterns for cache analysis
template<Component T>
inline void Registry::track_component_access(Entity entity, bool is_read_access) {
    if (!allocator_config_.enable_cache_analysis || !memory_stats_) {
        return;
    }
    
    // Track memory access with the global memory tracker
    auto it = entity_to_archetype_.find(entity);
    if (it != entity_to_archetype_.end()) {
        usize archetype_idx = it->second;
        if (archetype_idx < archetypes_.size()) {
            T* component = archetypes_[archetype_idx]->get_component<T>(entity);
            if (component) {
                memory::tracker::track_access(component, sizeof(T), !is_read_access);
                
                // Update our own cache statistics
                if (is_read_access) {
                    memory_stats_->cache_friendly_allocations++;
                }
            }
        }
    }
}

} // namespace ecscope::ecs