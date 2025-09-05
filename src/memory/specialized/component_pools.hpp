#pragma once

/**
 * @file memory/specialized/component_pools.hpp
 * @brief Specialized ECS Component Memory Pools for Optimal Data Layout
 * 
 * This header implements specialized memory pools optimized for ECS component storage
 * with different access patterns and data layout strategies. It provides educational
 * insights into data-oriented design and cache-friendly component organization.
 * 
 * Key Features:
 * - Structure of Arrays (SoA) optimized component pools
 * - Array of Structures (AoS) pools for specific use cases
 * - Component-specific pool optimization based on usage patterns
 * - Cache-line aware component grouping and alignment
 * - Hot/cold data separation within component types
 * - SIMD-friendly component layouts for batch processing
 * - Educational performance comparison tools
 * - Integration with sparse set ECS architecture
 * 
 * Educational Value:
 * - Demonstrates data-oriented design principles in practice
 * - Shows cache performance differences between AoS and SoA
 * - Illustrates component access pattern optimization
 * - Provides insights into memory layout for SIMD processing
 * - Educational examples of data locality optimization
 * 
 * @author ECScope Educational ECS Framework - Component Memory Optimization
 * @date 2025
 */

#include "memory/pools/hierarchical_pools.hpp"
#include "memory/memory_tracker.hpp"
#include "ecs/sparse_set.hpp"
#include "ecs/component.hpp"
#include "ecs/entity.hpp"
#include "core/types.hpp"
#include "core/log.hpp"
#include <atomic>
#include <memory>
#include <vector>
#include <array>
#include <unordered_map>
#include <type_traits>
#include <bit>
#include <span>
#include <concepts>

namespace ecscope::memory::specialized::components {

using namespace ecscope::ecs;

//=============================================================================
// Component Classification and Properties
//=============================================================================

/**
 * @brief Component access patterns for pool optimization
 */
enum class ComponentAccessPattern : u8 {
    ReadOnly      = 0,  // Read-only components (Transform, Static meshes)
    WriteHeavy    = 1,  // Frequently modified (Position, Velocity)
    EventDriven   = 2,  // Modified in response to events (Health, State)
    Computed      = 3,  // Computed each frame (WorldMatrix, BoundingBox)
    Sparse        = 4,  // Present on few entities (AI, PlayerController)
    Dense         = 5,  // Present on most entities (Transform, Renderable)
    Temporal      = 6,  // Short-lived components (Damage, Explosion)
    Persistent    = 7   // Long-lived components (Stats, Configuration)
};

/**
 * @brief Component processing characteristics
 */
enum class ComponentProcessing : u8 {
    Sequential    = 0,  // Processed sequentially by systems
    Parallel      = 1,  // Can be processed in parallel
    SIMD         = 2,  // Benefits from SIMD operations
    GPU          = 3,  // Transferred to/from GPU
    Network      = 4,  // Synchronized over network
    Cached       = 5,  // Heavily cached by systems
    Streamed     = 6   // Streamed from disk/network
};

/**
 * @brief Component memory layout preferences
 */
enum class ComponentLayout : u8 {
    AoS           = 0,  // Array of Structures (better for random access)
    SoA           = 1,  // Structure of Arrays (better for sequential access)
    Hybrid        = 2,  // Mix of AoS and SoA based on field access patterns
    Packed        = 3,  // Tightly packed for memory efficiency
    Aligned       = 4,  // Aligned for SIMD operations
    Interleaved   = 5   // Interleaved for specific access patterns
};

/**
 * @brief Comprehensive component pool properties
 */
template<typename ComponentType>
struct ComponentPoolProperties {
    static_assert(ecs::concepts::Component<ComponentType>, "Must be a valid ECS component");
    
    // Type information
    using component_type = ComponentType;
    static constexpr usize component_size = sizeof(ComponentType);
    static constexpr usize component_alignment = alignof(ComponentType);
    
    // Access characteristics
    ComponentAccessPattern access_pattern;
    ComponentProcessing processing_type;
    ComponentLayout preferred_layout;
    
    // Performance properties
    f32 read_write_ratio;           // Ratio of reads to writes (higher = more reads)
    f32 batch_processing_factor;    // How often processed in batches (0.0-1.0)
    f32 cache_locality_importance;  // Importance of cache locality (0.0-1.0)
    f32 memory_pressure_tolerance;  // Tolerance for memory pressure (0.0-1.0)
    
    // Pool sizing
    usize initial_capacity;         // Initial component capacity
    usize growth_factor;           // How much to grow when full
    usize max_capacity;            // Maximum capacity
    f32 expected_entity_ratio;     // Expected ratio of entities with this component
    
    // Alignment and layout
    usize preferred_alignment;      // Preferred memory alignment
    bool requires_simd_alignment;   // Requires SIMD alignment
    bool supports_vectorization;    // Component supports vectorized operations
    usize cache_line_alignment;    // Alignment relative to cache lines
    
    // Lifecycle properties
    f64 expected_lifetime_seconds;  // Expected component lifetime
    bool is_trivially_destructible; // Can be destroyed without calling destructor
    bool supports_memcpy;          // Supports memcpy for moves/copies
    bool is_pod_compatible;        // Plain Old Data compatible
    
    ComponentPoolProperties() {
        // Default values based on component analysis
        access_pattern = ComponentAccessPattern::Dense;
        processing_type = ComponentProcessing::Sequential;
        preferred_layout = ComponentLayout::SoA;
        
        read_write_ratio = 3.0f; // 3:1 read to write ratio
        batch_processing_factor = 0.8f;
        cache_locality_importance = 0.9f;
        memory_pressure_tolerance = 0.7f;
        
        initial_capacity = 1024;
        growth_factor = 2;
        max_capacity = 65536;
        expected_entity_ratio = 0.5f;
        
        preferred_alignment = std::max(component_alignment, usize(16));
        requires_simd_alignment = component_size >= 16 && (component_size % 16) == 0;
        supports_vectorization = std::is_arithmetic_v<ComponentType> || 
                               (std::is_aggregate_v<ComponentType> && component_size <= 64);
        cache_line_alignment = core::CACHE_LINE_SIZE;
        
        expected_lifetime_seconds = 10.0; // 10 seconds default
        is_trivially_destructible = std::is_trivially_destructible_v<ComponentType>;
        supports_memcpy = std::is_trivially_copyable_v<ComponentType>;
        is_pod_compatible = std::is_standard_layout_v<ComponentType> && std::is_trivial_v<ComponentType>;
        
        // Adjust based on component characteristics
        adjust_for_component_type();
    }
    
private:
    void adjust_for_component_type() {
        // Analyze component type and adjust properties
        if constexpr (component_size <= 32) {
            // Small components benefit from AoS layout
            preferred_layout = ComponentLayout::AoS;
            cache_locality_importance = 0.7f;
        } else if constexpr (component_size >= 128) {
            // Large components benefit from SoA layout
            preferred_layout = ComponentLayout::SoA;
            cache_locality_importance = 0.95f;
        }
        
        // Arithmetic types are good for SIMD
        if constexpr (std::is_arithmetic_v<ComponentType>) {
            processing_type = ComponentProcessing::SIMD;
            requires_simd_alignment = true;
            preferred_alignment = 32; // AVX alignment
        }
        
        // Adjust for common ECS component patterns
        if constexpr (std::is_same_v<ComponentType, f32> || 
                     std::is_same_v<ComponentType, f64> ||
                     std::is_same_v<ComponentType, i32> ||
                     std::is_same_v<ComponentType, u32>) {
            // Scalar types - highly optimizable
            processing_type = ComponentProcessing::SIMD;
            batch_processing_factor = 0.95f;
            supports_vectorization = true;
        }
        
        // POD types are easier to manage
        if constexpr (std::is_pod_v<ComponentType>) {
            memory_pressure_tolerance = 0.9f;
            expected_lifetime_seconds = 60.0; // Can live longer
        }
    }
};

//=============================================================================
// Structure of Arrays (SoA) Component Pool
//=============================================================================

/**
 * @brief SoA optimized component pool for cache-friendly iteration
 */
template<typename ComponentType>
requires ecs::concepts::Component<ComponentType>
class SoAComponentPool {
private:
    using Properties = ComponentPoolProperties<ComponentType>;
    
    // SoA storage - each field gets its own array
    struct FieldArrays {
        // In a real implementation, this would use reflection or macro magic
        // to automatically separate fields into individual arrays
        std::vector<u8> serialized_data;  // Simplified: all data serialized
        std::vector<ComponentType> components; // For now, keep simple storage
        
        void resize(usize new_size) {
            components.resize(new_size);
            serialized_data.resize(new_size * sizeof(ComponentType));
        }
        
        void reserve(usize capacity) {
            components.reserve(capacity);
            serialized_data.reserve(capacity * sizeof(ComponentType));
        }
    };
    
    // Pool data
    FieldArrays fields_;
    std::vector<Entity> entities_;           // Entity IDs corresponding to components
    std::vector<bool> active_slots_;         // Which slots are active
    std::vector<usize> free_slots_;          // Free slot indices
    
    // Pool properties and configuration
    Properties properties_;
    usize capacity_;
    usize size_;
    usize next_slot_;
    
    // Memory management
    void* raw_memory_;
    usize total_memory_allocated_;
    usize memory_alignment_;
    
    // Performance tracking
    alignas(core::CACHE_LINE_SIZE) std::atomic<u64> allocations_{0};
    alignas(core::CACHE_LINE_SIZE) std::atomic<u64> deallocations_{0};
    alignas(core::CACHE_LINE_SIZE) std::atomic<u64> iterations_{0};
    alignas(core::CACHE_LINE_SIZE) std::atomic<u64> cache_misses_estimated_{0};
    
    mutable std::shared_mutex pool_mutex_;
    
public:
    explicit SoAComponentPool(const Properties& props = Properties{})
        : properties_(props), capacity_(0), size_(0), next_slot_(0),
          raw_memory_(nullptr), total_memory_allocated_(0) {
        
        memory_alignment_ = std::max(properties_.preferred_alignment, core::CACHE_LINE_SIZE);
        initialize_pool();
        
        LOG_DEBUG("Initialized SoA component pool: type={}, capacity={}, alignment={}", 
                 typeid(ComponentType).name(), capacity_, memory_alignment_);
    }
    
    ~SoAComponentPool() {
        cleanup_pool();
        
        LOG_DEBUG("SoA component pool destroyed: allocations={}, peak_size={}", 
                 allocations_.load(), capacity_);
    }
    
    /**
     * @brief Add component for entity with optimal SoA layout
     */
    ComponentType* add_component(Entity entity, const ComponentType& component) {
        std::unique_lock<std::shared_mutex> lock(pool_mutex_);
        
        if (size_ >= capacity_) {
            if (!expand_pool()) {
                LOG_ERROR("Failed to expand SoA component pool");
                return nullptr;
            }
        }
        
        usize slot;
        if (!free_slots_.empty()) {
            // Reuse free slot
            slot = free_slots_.back();
            free_slots_.pop_back();
        } else {
            // Use next sequential slot
            slot = next_slot_++;
        }
        
        // Store component in SoA layout
        fields_.components[slot] = component;
        entities_[slot] = entity;
        active_slots_[slot] = true;
        size_++;
        
        allocations_.fetch_add(1, std::memory_order_relaxed);
        
        return &fields_.components[slot];
    }
    
    /**
     * @brief Remove component for entity
     */
    bool remove_component(Entity entity) {
        std::unique_lock<std::shared_mutex> lock(pool_mutex_);
        
        // Find entity slot
        for (usize i = 0; i < capacity_; ++i) {
            if (active_slots_[i] && entities_[i] == entity) {
                // Mark slot as free
                active_slots_[i] = false;
                free_slots_.push_back(i);
                size_--;
                
                // Clear component data (optional for performance)
                if (properties_.is_trivially_destructible) {
                    std::memset(&fields_.components[i], 0, sizeof(ComponentType));
                } else {
                    fields_.components[i].~ComponentType();
                    new(&fields_.components[i]) ComponentType{};
                }
                
                deallocations_.fetch_add(1, std::memory_order_relaxed);
                return true;
            }
        }
        
        return false;
    }
    
    /**
     * @brief Get component for entity
     */
    ComponentType* get_component(Entity entity) {
        std::shared_lock<std::shared_mutex> lock(pool_mutex_);
        
        for (usize i = 0; i < capacity_; ++i) {
            if (active_slots_[i] && entities_[i] == entity) {
                return &fields_.components[i];
            }
        }
        
        return nullptr;
    }
    
    /**
     * @brief Iterate over all active components (cache-friendly)
     */
    template<typename Func>
    void for_each(Func&& func) const {
        std::shared_lock<std::shared_mutex> lock(pool_mutex_);
        iterations_.fetch_add(1, std::memory_order_relaxed);
        
        // SoA iteration is highly cache-friendly
        for (usize i = 0; i < capacity_; ++i) {
            if (active_slots_[i]) {
                func(entities_[i], fields_.components[i]);
            }
        }
    }
    
    /**
     * @brief Iterate over components with indices (for SIMD processing)
     */
    template<typename Func>
    void for_each_indexed(Func&& func) const {
        std::shared_lock<std::shared_mutex> lock(pool_mutex_);
        iterations_.fetch_add(1, std::memory_order_relaxed);
        
        std::vector<usize> active_indices;
        active_indices.reserve(size_);
        
        for (usize i = 0; i < capacity_; ++i) {
            if (active_slots_[i]) {
                active_indices.push_back(i);
            }
        }
        
        // Process in chunks for SIMD optimization
        constexpr usize simd_width = 8; // Process 8 components at once
        usize chunk_count = (active_indices.size() + simd_width - 1) / simd_width;
        
        for (usize chunk = 0; chunk < chunk_count; ++chunk) {
            usize start_idx = chunk * simd_width;
            usize end_idx = std::min(start_idx + simd_width, active_indices.size());
            
            std::span<const usize> indices_span(
                active_indices.data() + start_idx, 
                end_idx - start_idx
            );
            
            func(indices_span, fields_.components);
        }
    }
    
    /**
     * @brief Get raw component data for external processing
     */
    std::span<ComponentType> get_raw_components() {
        std::shared_lock<std::shared_mutex> lock(pool_mutex_);
        return std::span<ComponentType>(fields_.components.data(), capacity_);
    }
    
    std::span<const ComponentType> get_raw_components() const {
        std::shared_lock<std::shared_mutex> lock(pool_mutex_);
        return std::span<const ComponentType>(fields_.components.data(), capacity_);
    }
    
    /**
     * @brief Get entity list corresponding to components
     */
    std::span<const Entity> get_entities() const {
        std::shared_lock<std::shared_mutex> lock(pool_mutex_);
        return std::span<const Entity>(entities_.data(), capacity_);
    }
    
    /**
     * @brief Get active slots mask
     */
    std::span<const bool> get_active_mask() const {
        std::shared_lock<std::shared_mutex> lock(pool_mutex_);
        return std::span<const bool>(active_slots_.data(), capacity_);
    }
    
    /**
     * @brief Pool statistics and performance metrics
     */
    struct PoolStatistics {
        usize capacity;
        usize size;
        usize free_slots_count;
        f64 utilization_ratio;
        f64 fragmentation_ratio;
        
        u64 total_allocations;
        u64 total_deallocations;
        u64 total_iterations;
        u64 estimated_cache_misses;
        
        usize memory_allocated;
        usize memory_alignment;
        f64 cache_efficiency_estimate;
        
        Properties pool_properties;
        
        // Performance analysis
        f64 allocation_rate;
        f64 iteration_frequency;
        f64 memory_bandwidth_estimate;
    };
    
    PoolStatistics get_statistics() const {
        std::shared_lock<std::shared_mutex> lock(pool_mutex_);
        
        PoolStatistics stats{};
        
        stats.capacity = capacity_;
        stats.size = size_;
        stats.free_slots_count = free_slots_.size();
        stats.utilization_ratio = capacity_ > 0 ? static_cast<f64>(size_) / capacity_ : 0.0;
        stats.fragmentation_ratio = free_slots_.size() > 0 ? 
            static_cast<f64>(free_slots_.size()) / capacity_ : 0.0;
        
        stats.total_allocations = allocations_.load(std::memory_order_relaxed);
        stats.total_deallocations = deallocations_.load(std::memory_order_relaxed);
        stats.total_iterations = iterations_.load(std::memory_order_relaxed);
        stats.estimated_cache_misses = cache_misses_estimated_.load(std::memory_order_relaxed);
        
        stats.memory_allocated = total_memory_allocated_;
        stats.memory_alignment = memory_alignment_;
        stats.pool_properties = properties_;
        
        // Cache efficiency estimate based on SoA layout
        stats.cache_efficiency_estimate = calculate_cache_efficiency();
        
        // Performance estimates
        stats.allocation_rate = static_cast<f64>(stats.total_allocations);
        stats.iteration_frequency = static_cast<f64>(stats.total_iterations);
        stats.memory_bandwidth_estimate = estimate_memory_bandwidth();
        
        return stats;
    }
    
    const Properties& get_properties() const { return properties_; }
    usize get_capacity() const { return capacity_; }
    usize get_size() const { return size_; }
    
private:
    void initialize_pool() {
        capacity_ = properties_.initial_capacity;
        
        // Allocate aligned memory for the entire pool
        usize total_size = calculate_total_memory_needed();
        raw_memory_ = std::aligned_alloc(memory_alignment_, total_size);
        
        if (!raw_memory_) {
            LOG_ERROR("Failed to allocate aligned memory for SoA component pool");
            return;
        }
        
        total_memory_allocated_ = total_size;
        
        // Initialize storage vectors
        fields_.resize(capacity_);
        entities_.resize(capacity_);
        active_slots_.resize(capacity_, false);
        free_slots_.reserve(capacity_);
        
        LOG_DEBUG("Allocated {}KB for SoA component pool", total_size / 1024);
    }
    
    void cleanup_pool() {
        if (raw_memory_) {
            std::free(raw_memory_);
            raw_memory_ = nullptr;
        }
        
        fields_.components.clear();
        entities_.clear();
        active_slots_.clear();
        free_slots_.clear();
        
        total_memory_allocated_ = 0;
    }
    
    bool expand_pool() {
        usize new_capacity = capacity_ * properties_.growth_factor;
        if (new_capacity > properties_.max_capacity) {
            new_capacity = properties_.max_capacity;
        }
        
        if (new_capacity <= capacity_) {
            return false; // Can't expand further
        }
        
        // Reallocate memory
        usize new_total_size = calculate_total_memory_needed(new_capacity);
        void* new_memory = std::aligned_alloc(memory_alignment_, new_total_size);
        
        if (!new_memory) {
            LOG_ERROR("Failed to allocate memory for pool expansion");
            return false;
        }
        
        // Copy existing data if we had any
        if (raw_memory_ && total_memory_allocated_ > 0) {
            std::memcpy(new_memory, raw_memory_, total_memory_allocated_);
            std::free(raw_memory_);
        }
        
        raw_memory_ = new_memory;
        total_memory_allocated_ = new_total_size;
        
        // Resize storage vectors
        usize old_capacity = capacity_;
        capacity_ = new_capacity;
        
        fields_.resize(capacity_);
        entities_.resize(capacity_);
        active_slots_.resize(capacity_, false);
        
        LOG_DEBUG("Expanded SoA component pool: {} -> {} capacity", old_capacity, capacity_);
        return true;
    }
    
    usize calculate_total_memory_needed(usize cap = 0) const {
        if (cap == 0) cap = capacity_;
        
        usize component_array_size = cap * sizeof(ComponentType);
        usize entity_array_size = cap * sizeof(Entity);
        usize bool_array_size = cap * sizeof(bool);
        
        // Add alignment padding
        usize total = component_array_size + entity_array_size + bool_array_size;
        total = align_up(total, memory_alignment_);
        
        return total;
    }
    
    f64 calculate_cache_efficiency() const {
        // SoA layout provides excellent cache efficiency for sequential access
        f64 base_efficiency = 0.95; // SoA is highly cache-friendly
        
        // Adjust based on utilization
        f64 utilization_factor = static_cast<f64>(size_) / capacity_;
        base_efficiency *= (0.5 + 0.5 * utilization_factor);
        
        // Adjust based on component size
        if constexpr (sizeof(ComponentType) <= core::CACHE_LINE_SIZE / 4) {
            base_efficiency *= 1.1; // Small components pack well
        }
        
        return std::min(base_efficiency, 1.0);
    }
    
    f64 estimate_memory_bandwidth() const {
        // Estimate based on component size and access patterns
        f64 bytes_per_access = sizeof(ComponentType);
        f64 access_frequency = static_cast<f64>(iterations_.load());
        
        // SoA provides better bandwidth utilization
        f64 bandwidth_efficiency = 0.8;
        
        return bytes_per_access * access_frequency * bandwidth_efficiency;
    }
    
    constexpr usize align_up(usize value, usize alignment) const {
        return (value + alignment - 1) & ~(alignment - 1);
    }
};

//=============================================================================
// Array of Structures (AoS) Component Pool
//=============================================================================

/**
 * @brief AoS optimized component pool for random access patterns
 */
template<typename ComponentType>
requires ecs::concepts::Component<ComponentType>
class AoSComponentPool {
private:
    using Properties = ComponentPoolProperties<ComponentType>;
    
    struct ComponentSlot {
        ComponentType component;
        Entity entity;
        bool is_active;
        u32 version; // For ABA prevention and versioning
        
        ComponentSlot() : is_active(false), version(0) {}
    };
    
    // Pool data
    std::vector<ComponentSlot> slots_;
    std::vector<usize> free_indices_;
    
    // Pool configuration
    Properties properties_;
    usize capacity_;
    usize size_;
    u32 version_counter_;
    
    // Performance tracking
    alignas(core::CACHE_LINE_SIZE) std::atomic<u64> allocations_{0};
    alignas(core::CACHE_LINE_SIZE) std::atomic<u64> random_accesses_{0};
    alignas(core::CACHE_LINE_SIZE) std::atomic<u64> sequential_iterations_{0};
    
    mutable std::shared_mutex pool_mutex_;
    
public:
    explicit AoSComponentPool(const Properties& props = Properties{}) 
        : properties_(props), capacity_(0), size_(0), version_counter_(1) {
        
        initialize_pool();
        
        LOG_DEBUG("Initialized AoS component pool: type={}, capacity={}", 
                 typeid(ComponentType).name(), capacity_);
    }
    
    /**
     * @brief Add component with AoS optimization for locality
     */
    ComponentType* add_component(Entity entity, const ComponentType& component) {
        std::unique_lock<std::shared_mutex> lock(pool_mutex_);
        
        if (size_ >= capacity_ && !expand_pool()) {
            return nullptr;
        }
        
        usize slot_index;
        if (!free_indices_.empty()) {
            slot_index = free_indices_.back();
            free_indices_.pop_back();
        } else {
            slot_index = size_;
        }
        
        auto& slot = slots_[slot_index];
        slot.component = component;
        slot.entity = entity;
        slot.is_active = true;
        slot.version = version_counter_++;
        
        size_++;
        allocations_.fetch_add(1, std::memory_order_relaxed);
        
        return &slot.component;
    }
    
    /**
     * @brief Get component with optimal AoS access
     */
    ComponentType* get_component(Entity entity) {
        std::shared_lock<std::shared_mutex> lock(pool_mutex_);
        random_accesses_.fetch_add(1, std::memory_order_relaxed);
        
        // AoS is optimal for random access by entity
        for (auto& slot : slots_) {
            if (slot.is_active && slot.entity == entity) {
                return &slot.component;
            }
        }
        
        return nullptr;
    }
    
    /**
     * @brief Iterate with AoS locality benefits
     */
    template<typename Func>
    void for_each(Func&& func) const {
        std::shared_lock<std::shared_mutex> lock(pool_mutex_);
        sequential_iterations_.fetch_add(1, std::memory_order_relaxed);
        
        // AoS iteration has good spatial locality for small components
        for (const auto& slot : slots_) {
            if (slot.is_active) {
                func(slot.entity, slot.component);
            }
        }
    }
    
    /**
     * @brief AoS statistics
     */
    struct AoSStatistics {
        usize capacity;
        usize size;
        f64 utilization_ratio;
        u64 total_allocations;
        u64 random_accesses;
        u64 sequential_iterations;
        f64 random_access_efficiency;
        Properties pool_properties;
    };
    
    AoSStatistics get_statistics() const {
        std::shared_lock<std::shared_mutex> lock(pool_mutex_);
        
        AoSStatistics stats{};
        stats.capacity = capacity_;
        stats.size = size_;
        stats.utilization_ratio = capacity_ > 0 ? static_cast<f64>(size_) / capacity_ : 0.0;
        stats.total_allocations = allocations_.load();
        stats.random_accesses = random_accesses_.load();
        stats.sequential_iterations = sequential_iterations_.load();
        stats.pool_properties = properties_;
        
        // AoS excels at random access
        stats.random_access_efficiency = 0.9; // High efficiency for random access
        
        return stats;
    }
    
private:
    void initialize_pool() {
        capacity_ = properties_.initial_capacity;
        slots_.resize(capacity_);
        free_indices_.reserve(capacity_);
    }
    
    bool expand_pool() {
        usize new_capacity = capacity_ * properties_.growth_factor;
        if (new_capacity > properties_.max_capacity) {
            return false;
        }
        
        slots_.resize(new_capacity);
        capacity_ = new_capacity;
        return true;
    }
};

//=============================================================================
// Hot/Cold Data Separation Pool
//=============================================================================

/**
 * @brief Component pool with hot/cold data separation for cache optimization
 */
template<typename ComponentType>
requires ecs::concepts::Component<ComponentType>
class HotColdComponentPool {
private:
    using Properties = ComponentPoolProperties<ComponentType>;
    
    // Separate hot and cold data based on access patterns
    struct HotData {
        ComponentType component;  // The actual component data
        Entity entity;           // Entity owner
        f64 last_access_time;    // For hot/cold classification
        u32 access_count;        // Access frequency counter
    };
    
    struct ColdData {
        Entity entity;           // Entity reference
        u32 creation_time;       // When component was created
        u32 version;            // Component version
        bool needs_migration;    // Needs migration to hot pool
    };
    
    // Hot pool for frequently accessed components
    std::vector<HotData> hot_pool_;
    std::vector<usize> hot_free_slots_;
    
    // Cold pool for rarely accessed components  
    std::vector<ComponentType> cold_components_;
    std::vector<ColdData> cold_metadata_;
    std::vector<usize> cold_free_slots_;
    
    // Pool configuration and state
    Properties properties_;
    usize hot_capacity_;
    usize cold_capacity_;
    usize hot_size_;
    usize cold_size_;
    
    // Hot/cold classification parameters
    f64 hot_threshold_accesses_per_second_;
    f64 cold_migration_delay_seconds_;
    f64 last_migration_check_;
    
    // Performance tracking
    alignas(core::CACHE_LINE_SIZE) std::atomic<u64> hot_hits_{0};
    alignas(core::CACHE_LINE_SIZE) std::atomic<u64> cold_hits_{0};
    alignas(core::CACHE_LINE_SIZE) std::atomic<u64> hot_to_cold_migrations_{0};
    alignas(core::CACHE_LINE_SIZE) std::atomic<u64> cold_to_hot_migrations_{0};
    
    mutable std::shared_mutex pool_mutex_;
    
public:
    explicit HotColdComponentPool(const Properties& props = Properties{})
        : properties_(props), hot_capacity_(0), cold_capacity_(0), 
          hot_size_(0), cold_size_(0) {
        
        hot_threshold_accesses_per_second_ = 10.0; // 10 accesses/second = hot
        cold_migration_delay_seconds_ = 5.0;       // 5 seconds without access = cold
        last_migration_check_ = get_current_time();
        
        initialize_pools();
        
        LOG_DEBUG("Initialized hot/cold component pool: hot_cap={}, cold_cap={}", 
                 hot_capacity_, cold_capacity_);
    }
    
    /**
     * @brief Add component (starts in hot pool)
     */
    ComponentType* add_component(Entity entity, const ComponentType& component) {
        std::unique_lock<std::shared_mutex> lock(pool_mutex_);
        
        // New components start hot
        if (hot_size_ >= hot_capacity_ && !expand_hot_pool()) {
            // Try cold pool as fallback
            return add_to_cold_pool(entity, component);
        }
        
        usize slot;
        if (!hot_free_slots_.empty()) {
            slot = hot_free_slots_.back();
            hot_free_slots_.pop_back();
        } else {
            slot = hot_size_;
        }
        
        auto& hot_data = hot_pool_[slot];
        hot_data.component = component;
        hot_data.entity = entity;
        hot_data.last_access_time = get_current_time();
        hot_data.access_count = 1;
        
        hot_size_++;
        return &hot_data.component;
    }
    
    /**
     * @brief Get component with access tracking for hot/cold classification
     */
    ComponentType* get_component(Entity entity) {
        std::unique_lock<std::shared_mutex> lock(pool_mutex_);
        
        // Try hot pool first
        for (auto& hot_data : hot_pool_) {
            if (hot_data.entity == entity) {
                hot_data.last_access_time = get_current_time();
                hot_data.access_count++;
                hot_hits_.fetch_add(1, std::memory_order_relaxed);
                return &hot_data.component;
            }
        }
        
        // Try cold pool
        for (usize i = 0; i < cold_size_; ++i) {
            if (cold_metadata_[i].entity == entity) {
                cold_hits_.fetch_add(1, std::memory_order_relaxed);
                
                // Mark for potential migration to hot pool
                cold_metadata_[i].needs_migration = true;
                
                return &cold_components_[i];
            }
        }
        
        return nullptr;
    }
    
    /**
     * @brief Perform hot/cold migration based on access patterns
     */
    void update_hot_cold_classification() {
        std::unique_lock<std::shared_mutex> lock(pool_mutex_);
        
        f64 current_time = get_current_time();
        f64 time_delta = current_time - last_migration_check_;
        
        if (time_delta < 1.0) return; // Check at most once per second
        
        // Check for hot -> cold migrations
        for (usize i = 0; i < hot_size_; ++i) {
            auto& hot_data = hot_pool_[i];
            f64 time_since_access = current_time - hot_data.last_access_time;
            f64 access_rate = hot_data.access_count / time_delta;
            
            if (time_since_access > cold_migration_delay_seconds_ && 
                access_rate < hot_threshold_accesses_per_second_) {
                // Migrate to cold pool
                migrate_to_cold(i);
            }
            
            // Reset access count for next period
            hot_data.access_count = 0;
        }
        
        // Check for cold -> hot migrations
        for (usize i = 0; i < cold_size_; ++i) {
            if (cold_metadata_[i].needs_migration) {
                migrate_to_hot(i);
            }
        }
        
        last_migration_check_ = current_time;
    }
    
    /**
     * @brief Hot/cold pool statistics
     */
    struct HotColdStatistics {
        usize hot_capacity;
        usize hot_size;
        usize cold_capacity;
        usize cold_size;
        f64 hot_utilization;
        f64 cold_utilization;
        u64 hot_hits;
        u64 cold_hits;
        f64 hot_hit_ratio;
        u64 hot_to_cold_migrations;
        u64 cold_to_hot_migrations;
        f64 cache_efficiency_estimate;
        Properties pool_properties;
    };
    
    HotColdStatistics get_statistics() const {
        std::shared_lock<std::shared_mutex> lock(pool_mutex_);
        
        HotColdStatistics stats{};
        stats.hot_capacity = hot_capacity_;
        stats.hot_size = hot_size_;
        stats.cold_capacity = cold_capacity_;
        stats.cold_size = cold_size_;
        stats.hot_utilization = hot_capacity_ > 0 ? static_cast<f64>(hot_size_) / hot_capacity_ : 0.0;
        stats.cold_utilization = cold_capacity_ > 0 ? static_cast<f64>(cold_size_) / cold_capacity_ : 0.0;
        
        stats.hot_hits = hot_hits_.load();
        stats.cold_hits = cold_hits_.load();
        u64 total_hits = stats.hot_hits + stats.cold_hits;
        if (total_hits > 0) {
            stats.hot_hit_ratio = static_cast<f64>(stats.hot_hits) / total_hits;
        }
        
        stats.hot_to_cold_migrations = hot_to_cold_migrations_.load();
        stats.cold_to_hot_migrations = cold_to_hot_migrations_.load();
        stats.pool_properties = properties_;
        
        // Cache efficiency is higher when hot hit ratio is high
        stats.cache_efficiency_estimate = 0.5 + 0.4 * stats.hot_hit_ratio;
        
        return stats;
    }
    
private:
    void initialize_pools() {
        // Split capacity between hot and cold pools
        hot_capacity_ = properties_.initial_capacity * 3 / 4; // 75% hot
        cold_capacity_ = properties_.initial_capacity / 4;     // 25% cold
        
        hot_pool_.resize(hot_capacity_);
        cold_components_.resize(cold_capacity_);
        cold_metadata_.resize(cold_capacity_);
        
        hot_free_slots_.reserve(hot_capacity_);
        cold_free_slots_.reserve(cold_capacity_);
    }
    
    ComponentType* add_to_cold_pool(Entity entity, const ComponentType& component) {
        if (cold_size_ >= cold_capacity_) {
            return nullptr; // Both pools full
        }
        
        usize slot;
        if (!cold_free_slots_.empty()) {
            slot = cold_free_slots_.back();
            cold_free_slots_.pop_back();
        } else {
            slot = cold_size_;
        }
        
        cold_components_[slot] = component;
        cold_metadata_[slot].entity = entity;
        cold_metadata_[slot].creation_time = static_cast<u32>(get_current_time());
        cold_metadata_[slot].needs_migration = false;
        
        cold_size_++;
        return &cold_components_[slot];
    }
    
    bool expand_hot_pool() {
        usize new_capacity = hot_capacity_ * 2;
        if (new_capacity > properties_.max_capacity / 2) {
            return false;
        }
        
        hot_pool_.resize(new_capacity);
        hot_capacity_ = new_capacity;
        return true;
    }
    
    void migrate_to_cold(usize hot_index) {
        if (cold_size_ >= cold_capacity_) {
            return; // Cold pool full
        }
        
        // Move from hot to cold
        const auto& hot_data = hot_pool_[hot_index];
        
        usize cold_slot;
        if (!cold_free_slots_.empty()) {
            cold_slot = cold_free_slots_.back();
            cold_free_slots_.pop_back();
        } else {
            cold_slot = cold_size_;
        }
        
        cold_components_[cold_slot] = hot_data.component;
        cold_metadata_[cold_slot].entity = hot_data.entity;
        cold_metadata_[cold_slot].creation_time = static_cast<u32>(get_current_time());
        cold_metadata_[cold_slot].needs_migration = false;
        
        // Free hot slot
        hot_free_slots_.push_back(hot_index);
        cold_size_++;
        
        hot_to_cold_migrations_.fetch_add(1, std::memory_order_relaxed);
    }
    
    void migrate_to_hot(usize cold_index) {
        if (hot_size_ >= hot_capacity_) {
            return; // Hot pool full
        }
        
        // Move from cold to hot
        usize hot_slot;
        if (!hot_free_slots_.empty()) {
            hot_slot = hot_free_slots_.back();
            hot_free_slots_.pop_back();
        } else {
            hot_slot = hot_size_;
        }
        
        auto& hot_data = hot_pool_[hot_slot];
        hot_data.component = cold_components_[cold_index];
        hot_data.entity = cold_metadata_[cold_index].entity;
        hot_data.last_access_time = get_current_time();
        hot_data.access_count = 1;
        
        // Free cold slot
        cold_free_slots_.push_back(cold_index);
        hot_size_++;
        
        cold_to_hot_migrations_.fetch_add(1, std::memory_order_relaxed);
    }
    
    f64 get_current_time() const {
        using namespace std::chrono;
        return duration<f64>(steady_clock::now().time_since_epoch()).count();
    }
};

//=============================================================================
// Component Pool Factory and Manager
//=============================================================================

/**
 * @brief Factory for creating optimal component pools based on usage patterns
 */
class ComponentPoolFactory {
public:
    /**
     * @brief Create optimal pool based on component characteristics
     */
    template<typename ComponentType>
    static auto create_optimal_pool(const ComponentPoolProperties<ComponentType>& properties) {
        switch (properties.preferred_layout) {
            case ComponentLayout::SoA:
                return std::make_unique<SoAComponentPool<ComponentType>>(properties);
                
            case ComponentLayout::AoS:
                return std::make_unique<AoSComponentPool<ComponentType>>(properties);
                
            case ComponentLayout::Hybrid:
                // For hybrid, prefer hot/cold separation
                return std::make_unique<HotColdComponentPool<ComponentType>>(properties);
                
            default:
                // Default to SoA for educational purposes
                return std::make_unique<SoAComponentPool<ComponentType>>(properties);
        }
    }
    
    /**
     * @brief Analyze component type and suggest optimal properties
     */
    template<typename ComponentType>
    static ComponentPoolProperties<ComponentType> analyze_component_characteristics() {
        ComponentPoolProperties<ComponentType> props;
        
        // Analyze based on type characteristics
        if constexpr (sizeof(ComponentType) <= 16) {
            props.preferred_layout = ComponentLayout::AoS;
            props.access_pattern = ComponentAccessPattern::Dense;
        } else if constexpr (sizeof(ComponentType) >= 64) {
            props.preferred_layout = ComponentLayout::SoA;
            props.processing_type = ComponentProcessing::SIMD;
        } else {
            props.preferred_layout = ComponentLayout::Hybrid;
        }
        
        // Adjust for common component patterns
        if constexpr (std::is_arithmetic_v<ComponentType>) {
            props.processing_type = ComponentProcessing::SIMD;
            props.supports_vectorization = true;
            props.batch_processing_factor = 0.9f;
        }
        
        return props;
    }
};

//=============================================================================
// Global Component Pool Manager
//=============================================================================

/**
 * @brief Global manager for all component pools with performance monitoring
 */
class ComponentPoolManager {
private:
    // Type-erased pool interface
    struct PoolInterface {
        virtual ~PoolInterface() = default;
        virtual const std::type_info& get_component_type() const = 0;
        virtual usize get_size() const = 0;
        virtual usize get_capacity() const = 0;
        virtual f64 get_utilization() const = 0;
        virtual std::string get_performance_summary() const = 0;
    };
    
    template<typename ComponentType>
    struct TypedPool : public PoolInterface {
        std::unique_ptr<SoAComponentPool<ComponentType>> soa_pool;
        std::unique_ptr<AoSComponentPool<ComponentType>> aos_pool;
        std::unique_ptr<HotColdComponentPool<ComponentType>> hot_cold_pool;
        ComponentLayout active_layout;
        
        const std::type_info& get_component_type() const override {
            return typeid(ComponentType);
        }
        
        usize get_size() const override {
            switch (active_layout) {
                case ComponentLayout::SoA:
                    return soa_pool ? soa_pool->get_size() : 0;
                case ComponentLayout::AoS:
                    return aos_pool ? aos_pool->get_size() : 0;
                case ComponentLayout::Hybrid:
                    return hot_cold_pool ? hot_cold_pool->get_statistics().hot_size +
                                         hot_cold_pool->get_statistics().cold_size : 0;
                default:
                    return 0;
            }
        }
        
        usize get_capacity() const override {
            switch (active_layout) {
                case ComponentLayout::SoA:
                    return soa_pool ? soa_pool->get_capacity() : 0;
                case ComponentLayout::AoS:
                    return aos_pool ? aos_pool->get_statistics().capacity : 0;
                case ComponentLayout::Hybrid:
                    return hot_cold_pool ? hot_cold_pool->get_statistics().hot_capacity +
                                         hot_cold_pool->get_statistics().cold_capacity : 0;
                default:
                    return 0;
            }
        }
        
        f64 get_utilization() const override {
            usize size = get_size();
            usize capacity = get_capacity();
            return capacity > 0 ? static_cast<f64>(size) / capacity : 0.0;
        }
        
        std::string get_performance_summary() const override {
            f64 util = get_utilization();
            if (util > 0.8) return "High utilization - consider expansion";
            if (util < 0.3) return "Low utilization - consider compaction";
            return "Optimal utilization";
        }
    };
    
    std::unordered_map<std::type_index, std::unique_ptr<PoolInterface>> pools_;
    mutable std::shared_mutex pools_mutex_;
    
    // Global statistics
    alignas(core::CACHE_LINE_SIZE) std::atomic<usize> total_components_{0};
    alignas(core::CACHE_LINE_SIZE) std::atomic<usize> total_memory_used_{0};
    alignas(core::CACHE_LINE_SIZE) std::atomic<u32> active_pool_count_{0};
    
public:
    /**
     * @brief Register component pool
     */
    template<typename ComponentType>
    void register_component_pool(ComponentLayout preferred_layout = ComponentLayout::SoA) {
        std::unique_lock<std::shared_mutex> lock(pools_mutex_);
        
        std::type_index type_idx(typeid(ComponentType));
        if (pools_.find(type_idx) != pools_.end()) {
            return; // Already registered
        }
        
        auto typed_pool = std::make_unique<TypedPool<ComponentType>>();
        typed_pool->active_layout = preferred_layout;
        
        auto properties = ComponentPoolFactory::analyze_component_characteristics<ComponentType>();
        properties.preferred_layout = preferred_layout;
        
        switch (preferred_layout) {
            case ComponentLayout::SoA:
                typed_pool->soa_pool = std::make_unique<SoAComponentPool<ComponentType>>(properties);
                break;
            case ComponentLayout::AoS:
                typed_pool->aos_pool = std::make_unique<AoSComponentPool<ComponentType>>(properties);
                break;
            case ComponentLayout::Hybrid:
                typed_pool->hot_cold_pool = std::make_unique<HotColdComponentPool<ComponentType>>(properties);
                break;
        }
        
        pools_[type_idx] = std::move(typed_pool);
        active_pool_count_.fetch_add(1, std::memory_order_relaxed);
        
        LOG_INFO("Registered component pool: type={}, layout={}", 
                typeid(ComponentType).name(), static_cast<u32>(preferred_layout));
    }
    
    /**
     * @brief Get comprehensive statistics for all pools
     */
    struct GlobalStatistics {
        usize total_pools;
        usize total_components;
        usize total_memory_used;
        f64 average_utilization;
        std::vector<std::pair<std::string, std::string>> pool_summaries;
        std::string overall_assessment;
    };
    
    GlobalStatistics get_global_statistics() const {
        std::shared_lock<std::shared_mutex> lock(pools_mutex_);
        
        GlobalStatistics stats{};
        stats.total_pools = pools_.size();
        stats.total_components = total_components_.load();
        stats.total_memory_used = total_memory_used_.load();
        
        f64 total_utilization = 0.0;
        for (const auto& [type_idx, pool] : pools_) {
            f64 util = pool->get_utilization();
            total_utilization += util;
            
            std::string type_name = type_idx.name();
            std::string summary = pool->get_performance_summary();
            stats.pool_summaries.emplace_back(type_name, summary);
        }
        
        if (stats.total_pools > 0) {
            stats.average_utilization = total_utilization / stats.total_pools;
        }
        
        // Overall assessment
        if (stats.average_utilization > 0.7) {
            stats.overall_assessment = "Component pools are well-utilized";
        } else if (stats.average_utilization > 0.4) {
            stats.overall_assessment = "Component pools have moderate utilization";
        } else {
            stats.overall_assessment = "Component pools are under-utilized";
        }
        
        return stats;
    }
};

//=============================================================================
// Global Component Pool Manager Instance
//=============================================================================

ComponentPoolManager& get_global_component_pool_manager();

} // namespace ecscope::memory::specialized::components