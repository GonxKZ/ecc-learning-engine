#pragma once

/**
 * @file memory/specialized/thermal_pools.hpp
 * @brief Hot/Cold Data Separation Memory Pools for Cache Optimization
 * 
 * This header implements sophisticated hot/cold data separation pools that
 * automatically classify and migrate data based on access patterns to optimize
 * cache performance. It provides educational insights into memory thermal
 * management and cache-conscious data organization.
 * 
 * Key Features:
 * - Automatic hot/cold data classification based on access frequency
 * - Temporal locality-aware data placement and migration
 * - Multi-tier memory hierarchies (hot, warm, cold, frozen)
 * - Predictive data temperature modeling
 * - NUMA-aware thermal pool placement
 * - Cache-line aligned hot data for optimal performance
 * - Educational thermal management visualization
 * - Integration with existing memory tracking infrastructure
 * 
 * Educational Value:
 * - Demonstrates cache hierarchy principles in memory management
 * - Shows temporal locality optimization techniques
 * - Illustrates data access pattern learning algorithms
 * - Provides insights into memory thermal management
 * - Educational examples of predictive caching strategies
 * 
 * @author ECScope Educational ECS Framework - Thermal Memory Management
 * @date 2025
 */

#include "memory/pools/hierarchical_pools.hpp"
#include "memory/memory_tracker.hpp"
#include "memory/analysis/numa_manager.hpp"
#include "core/types.hpp"
#include "core/log.hpp"
#include <atomic>
#include <memory>
#include <vector>
#include <array>
#include <unordered_map>
#include <chrono>
#include <algorithm>
#include <cmath>
#include <span>

namespace ecscope::memory::specialized::thermal {

//=============================================================================
// Thermal Classification and Properties
//=============================================================================

/**
 * @brief Data temperature levels for thermal management
 */
enum class DataTemperature : u8 {
    Frozen    = 0,  // Never or very rarely accessed (archive)
    Cold      = 1,  // Accessed infrequently (< 1 per minute)
    Cool      = 2,  // Accessed occasionally (1-10 per minute)
    Warm      = 3,  // Accessed regularly (10-60 per minute)
    Hot       = 4,  // Accessed frequently (> 60 per minute)
    Blazing   = 5   // Accessed constantly (multiple per second)
};

/**
 * @brief Access pattern classification for thermal prediction
 */
enum class AccessPattern : u8 {
    Unknown       = 0,
    Sequential    = 1,  // Sequential access pattern
    Random        = 2,  // Random access pattern
    Burst         = 3,  // Bursty access (periods of high activity)
    Periodic      = 4,  // Periodic access pattern
    Declining     = 5,  // Access frequency declining over time
    Growing       = 6,  // Access frequency growing over time
    Stable        = 7   // Stable access frequency
};

/**
 * @brief Thermal data properties and statistics
 */
struct ThermalProperties {
    f64 base_temperature;           // Base thermal rating (0.0 - 1.0)
    f64 current_temperature;        // Current thermal rating
    f64 temperature_velocity;       // Rate of temperature change
    f64 cooling_rate;              // How fast data cools down
    f64 heating_rate;              // How fast data heats up
    
    // Access statistics
    u64 total_accesses;            // Total number of accesses
    f64 access_frequency;          // Accesses per second
    f64 last_access_time;          // Last access timestamp
    f64 average_access_interval;   // Average time between accesses
    f64 access_variance;           // Variance in access intervals
    
    // Pattern analysis
    AccessPattern detected_pattern; // Detected access pattern
    f64 pattern_confidence;        // Confidence in pattern detection
    f64 predictability_score;      // How predictable the access pattern is
    
    // Thermal history
    std::array<f64, 32> temperature_history; // Recent temperature samples
    usize history_index;           // Current history index
    f64 peak_temperature;          // Peak temperature ever reached
    f64 average_temperature;       // Long-term average temperature
    
    ThermalProperties() noexcept {
        base_temperature = 0.5;
        current_temperature = 0.5;
        temperature_velocity = 0.0;
        cooling_rate = 0.95;  // Cool down to 95% per time unit
        heating_rate = 1.1;   // Heat up by 10% per access
        
        total_accesses = 0;
        access_frequency = 0.0;
        last_access_time = 0.0;
        average_access_interval = 0.0;
        access_variance = 0.0;
        
        detected_pattern = AccessPattern::Unknown;
        pattern_confidence = 0.0;
        predictability_score = 0.0;
        
        temperature_history.fill(0.5);
        history_index = 0;
        peak_temperature = 0.5;
        average_temperature = 0.5;
    }
    
    /**
     * @brief Update thermal properties based on access
     */
    void record_access(f64 current_time) {
        total_accesses++;
        
        if (last_access_time > 0.0) {
            f64 interval = current_time - last_access_time;
            
            // Update access statistics
            if (average_access_interval == 0.0) {
                average_access_interval = interval;
            } else {
                average_access_interval = average_access_interval * 0.9 + interval * 0.1;
            }
            
            // Update access frequency (exponential moving average)
            f64 instant_frequency = 1.0 / interval;
            if (access_frequency == 0.0) {
                access_frequency = instant_frequency;
            } else {
                access_frequency = access_frequency * 0.9 + instant_frequency * 0.1;
            }
        }
        
        last_access_time = current_time;
        
        // Heat up based on access
        current_temperature = std::min(1.0, current_temperature * heating_rate);
        
        // Update temperature history
        temperature_history[history_index] = current_temperature;
        history_index = (history_index + 1) % temperature_history.size();
        
        // Update peak temperature
        peak_temperature = std::max(peak_temperature, current_temperature);
        
        // Update pattern detection
        update_pattern_detection();
    }
    
    /**
     * @brief Cool down data over time
     */
    void apply_cooling(f64 time_delta) {
        if (time_delta <= 0.0) return;
        
        // Apply exponential cooling
        f64 cooling_factor = std::pow(cooling_rate, time_delta);
        current_temperature *= cooling_factor;
        
        // Update temperature velocity
        f64 old_velocity = temperature_velocity;
        temperature_velocity = (current_temperature - (current_temperature / cooling_factor)) / time_delta;
        
        // Smooth velocity
        temperature_velocity = old_velocity * 0.7 + temperature_velocity * 0.3;
        
        // Update average temperature
        average_temperature = average_temperature * 0.999 + current_temperature * 0.001;
    }
    
    /**
     * @brief Get current data temperature classification
     */
    DataTemperature get_temperature_class() const {
        if (current_temperature >= 0.9) return DataTemperature::Blazing;
        if (current_temperature >= 0.75) return DataTemperature::Hot;
        if (current_temperature >= 0.6) return DataTemperature::Warm;
        if (current_temperature >= 0.4) return DataTemperature::Cool;
        if (current_temperature >= 0.2) return DataTemperature::Cold;
        return DataTemperature::Frozen;
    }
    
    /**
     * @brief Predict future temperature
     */
    f64 predict_temperature(f64 time_ahead) const {
        // Simple linear prediction based on velocity
        f64 predicted = current_temperature + temperature_velocity * time_ahead;
        
        // Apply cooling over time
        f64 cooling_factor = std::pow(cooling_rate, time_ahead);
        predicted *= cooling_factor;
        
        return std::clamp(predicted, 0.0, 1.0);
    }
    
private:
    void update_pattern_detection() {
        if (total_accesses < 10) return; // Need enough data
        
        // Analyze temperature history for patterns
        f64 trend = 0.0;
        f64 variance = 0.0;
        
        for (usize i = 1; i < temperature_history.size(); ++i) {
            f64 diff = temperature_history[i] - temperature_history[i-1];
            trend += diff;
            variance += diff * diff;
        }
        
        trend /= (temperature_history.size() - 1);
        variance /= (temperature_history.size() - 1);
        
        // Classify pattern based on trend and variance
        if (std::abs(trend) < 0.01 && variance < 0.01) {
            detected_pattern = AccessPattern::Stable;
            pattern_confidence = 0.9;
        } else if (trend > 0.05) {
            detected_pattern = AccessPattern::Growing;
            pattern_confidence = 0.8;
        } else if (trend < -0.05) {
            detected_pattern = AccessPattern::Declining;
            pattern_confidence = 0.8;
        } else if (variance > 0.1) {
            detected_pattern = AccessPattern::Burst;
            pattern_confidence = 0.7;
        } else {
            detected_pattern = AccessPattern::Random;
            pattern_confidence = 0.6;
        }
        
        // Update predictability score
        predictability_score = pattern_confidence * (1.0 - variance);
    }
};

//=============================================================================
// Thermal Memory Block
//=============================================================================

/**
 * @brief Memory block with thermal management capabilities
 */
struct ThermalBlock {
    void* data_ptr;                 // Pointer to actual data
    usize data_size;                // Size of data in bytes
    usize data_alignment;           // Data alignment requirement
    ThermalProperties thermal;      // Thermal properties
    
    // Metadata
    u64 block_id;                   // Unique block identifier
    f64 creation_time;              // When block was created
    DataTemperature assigned_tier;  // Current thermal tier assignment
    bool migration_pending;         // Migration to different tier pending
    u32 migration_count;           // Number of times migrated
    
    // NUMA and cache awareness
    u32 preferred_numa_node;       // Preferred NUMA node
    u32 current_numa_node;         // Current NUMA node
    bool is_cache_aligned;         // Data is cache-line aligned
    usize cache_line_offset;       // Offset within cache line
    
    ThermalBlock(void* ptr, usize size, usize align = alignof(std::max_align_t))
        : data_ptr(ptr), data_size(size), data_alignment(align) {
        
        static std::atomic<u64> next_id{1};
        block_id = next_id.fetch_add(1, std::memory_order_relaxed);
        
        creation_time = get_current_time();
        assigned_tier = DataTemperature::Cool; // Start cool
        migration_pending = false;
        migration_count = 0;
        
        preferred_numa_node = 0;
        current_numa_node = 0;
        is_cache_aligned = (reinterpret_cast<uintptr_t>(ptr) % core::CACHE_LINE_SIZE) == 0;
        cache_line_offset = reinterpret_cast<uintptr_t>(ptr) % core::CACHE_LINE_SIZE;
    }
    
    /**
     * @brief Record access to this block
     */
    void record_access() {
        thermal.record_access(get_current_time());
        
        // Check if temperature class changed
        DataTemperature new_class = thermal.get_temperature_class();
        if (new_class != assigned_tier) {
            migration_pending = true;
        }
    }
    
    /**
     * @brief Apply thermal cooling over time
     */
    void update_thermal_state(f64 current_time) {
        f64 time_since_last_access = current_time - thermal.last_access_time;
        thermal.apply_cooling(time_since_last_access);
        
        // Check for temperature tier changes
        DataTemperature new_class = thermal.get_temperature_class();
        if (new_class != assigned_tier) {
            migration_pending = true;
        }
    }
    
    /**
     * @brief Mark migration as completed
     */
    void complete_migration(DataTemperature new_tier, u32 new_numa_node) {
        assigned_tier = new_tier;
        current_numa_node = new_numa_node;
        migration_pending = false;
        migration_count++;
    }
    
private:
    f64 get_current_time() const {
        using namespace std::chrono;
        return duration<f64>(steady_clock::now().time_since_epoch()).count();
    }
};

//=============================================================================
// Thermal Tier Pool
//=============================================================================

/**
 * @brief Memory pool for a specific thermal tier
 */
class ThermalTierPool {
private:
    DataTemperature tier_temperature_;
    std::vector<std::unique_ptr<ThermalBlock>> blocks_;
    std::vector<usize> free_block_indices_;
    
    // Pool configuration
    usize initial_capacity_;
    usize max_capacity_;
    usize growth_factor_;
    usize current_capacity_;
    usize current_size_;
    
    // Memory management
    void* raw_memory_;
    usize total_memory_allocated_;
    usize memory_alignment_;
    u32 preferred_numa_node_;
    
    // Performance tracking
    alignas(core::CACHE_LINE_SIZE) std::atomic<u64> allocations_{0};
    alignas(core::CACHE_LINE_SIZE) std::atomic<u64> accesses_{0};
    alignas(core::CACHE_LINE_SIZE) std::atomic<u64> migrations_in_{0};
    alignas(core::CACHE_LINE_SIZE) std::atomic<u64> migrations_out_{0};
    alignas(core::CACHE_LINE_SIZE) std::atomic<f64> average_temperature_{0.0};
    
    mutable std::shared_mutex pool_mutex_;
    
public:
    explicit ThermalTierPool(DataTemperature tier, u32 numa_node = 0)
        : tier_temperature_(tier), preferred_numa_node_(numa_node) {
        
        // Configure pool based on tier
        configure_tier_properties();
        initialize_pool();
        
        LOG_DEBUG("Initialized thermal tier pool: tier={}, numa_node={}, capacity={}", 
                 static_cast<u32>(tier), numa_node, initial_capacity_);
    }
    
    ~ThermalTierPool() {
        cleanup_pool();
    }
    
    /**
     * @brief Allocate block in this thermal tier
     */
    ThermalBlock* allocate_block(usize size, usize alignment = alignof(std::max_align_t)) {
        std::unique_lock<std::shared_mutex> lock(pool_mutex_);
        
        if (current_size_ >= current_capacity_ && !expand_pool()) {
            return nullptr;
        }
        
        // Get next available slot
        usize block_index;
        if (!free_block_indices_.empty()) {
            block_index = free_block_indices_.back();
            free_block_indices_.pop_back();
        } else {
            block_index = current_size_;
        }
        
        // Allocate aligned memory for the data
        usize aligned_size = align_up(size, alignment);
        void* data_memory = allocate_aligned_memory(aligned_size, alignment);
        
        if (!data_memory) {
            free_block_indices_.push_back(block_index);
            return nullptr;
        }
        
        // Create thermal block
        auto block = std::make_unique<ThermalBlock>(data_memory, aligned_size, alignment);
        block->assigned_tier = tier_temperature_;
        block->preferred_numa_node = preferred_numa_node_;
        block->current_numa_node = preferred_numa_node_;
        
        ThermalBlock* result = block.get();
        blocks_[block_index] = std::move(block);
        current_size_++;
        
        allocations_.fetch_add(1, std::memory_order_relaxed);
        
        return result;
    }
    
    /**
     * @brief Migrate block to this tier from another
     */
    bool migrate_block_in(std::unique_ptr<ThermalBlock> block) {
        std::unique_lock<std::shared_mutex> lock(pool_mutex_);
        
        if (current_size_ >= current_capacity_ && !expand_pool()) {
            return false;
        }
        
        // Potentially reallocate data for optimal placement
        if (should_reallocate_for_tier(block.get())) {
            reallocate_block_for_tier(block.get());
        }
        
        usize block_index;
        if (!free_block_indices_.empty()) {
            block_index = free_block_indices_.back();
            free_block_indices_.pop_back();
        } else {
            block_index = current_size_;
        }
        
        block->complete_migration(tier_temperature_, preferred_numa_node_);
        blocks_[block_index] = std::move(block);
        current_size_++;
        
        migrations_in_.fetch_add(1, std::memory_order_relaxed);
        
        return true;
    }
    
    /**
     * @brief Remove block from this tier (for migration out)
     */
    std::unique_ptr<ThermalBlock> migrate_block_out(u64 block_id) {
        std::unique_lock<std::shared_mutex> lock(pool_mutex_);
        
        for (usize i = 0; i < current_capacity_; ++i) {
            if (blocks_[i] && blocks_[i]->block_id == block_id) {
                auto block = std::move(blocks_[i]);
                free_block_indices_.push_back(i);
                current_size_--;
                
                migrations_out_.fetch_add(1, std::memory_order_relaxed);
                
                return block;
            }
        }
        
        return nullptr;
    }
    
    /**
     * @brief Update thermal states of all blocks in this tier
     */
    void update_thermal_states() {
        std::shared_lock<std::shared_mutex> lock(pool_mutex_);
        
        f64 current_time = get_current_time();
        f64 temperature_sum = 0.0;
        usize active_blocks = 0;
        
        for (auto& block : blocks_) {
            if (block) {
                block->update_thermal_state(current_time);
                temperature_sum += block->thermal.current_temperature;
                active_blocks++;
            }
        }
        
        if (active_blocks > 0) {
            f64 avg_temp = temperature_sum / active_blocks;
            average_temperature_.store(avg_temp, std::memory_order_relaxed);
        }
    }
    
    /**
     * @brief Get blocks that need migration to other tiers
     */
    std::vector<u64> get_migration_candidates() const {
        std::shared_lock<std::shared_mutex> lock(pool_mutex_);
        
        std::vector<u64> candidates;
        
        for (const auto& block : blocks_) {
            if (block && block->migration_pending) {
                DataTemperature target_tier = block->thermal.get_temperature_class();
                if (target_tier != tier_temperature_) {
                    candidates.push_back(block->block_id);
                }
            }
        }
        
        return candidates;
    }
    
    /**
     * @brief Get tier statistics
     */
    struct TierStatistics {
        DataTemperature tier;
        usize capacity;
        usize size;
        usize free_slots;
        f64 utilization_ratio;
        f64 average_temperature;
        u64 total_allocations;
        u64 total_accesses;
        u64 migrations_in;
        u64 migrations_out;
        f64 migration_ratio;
        usize memory_allocated;
        u32 numa_node;
        f64 tier_efficiency_score;
    };
    
    TierStatistics get_statistics() const {
        std::shared_lock<std::shared_mutex> lock(pool_mutex_);
        
        TierStatistics stats{};
        stats.tier = tier_temperature_;
        stats.capacity = current_capacity_;
        stats.size = current_size_;
        stats.free_slots = free_block_indices_.size();
        stats.utilization_ratio = current_capacity_ > 0 ? 
            static_cast<f64>(current_size_) / current_capacity_ : 0.0;
        stats.average_temperature = average_temperature_.load(std::memory_order_relaxed);
        stats.total_allocations = allocations_.load(std::memory_order_relaxed);
        stats.total_accesses = accesses_.load(std::memory_order_relaxed);
        stats.migrations_in = migrations_in_.load(std::memory_order_relaxed);
        stats.migrations_out = migrations_out_.load(std::memory_order_relaxed);
        
        u64 total_migrations = stats.migrations_in + stats.migrations_out;
        if (total_migrations > 0) {
            stats.migration_ratio = static_cast<f64>(stats.migrations_out) / total_migrations;
        }
        
        stats.memory_allocated = total_memory_allocated_;
        stats.numa_node = preferred_numa_node_;
        
        // Calculate tier efficiency (how well-matched blocks are to this tier)
        stats.tier_efficiency_score = calculate_tier_efficiency();
        
        return stats;
    }
    
    DataTemperature get_tier() const { return tier_temperature_; }
    
private:
    void configure_tier_properties() {
        switch (tier_temperature_) {
            case DataTemperature::Blazing:
                initial_capacity_ = 64;    // Small, high-performance pool
                max_capacity_ = 256;
                growth_factor_ = 2;
                memory_alignment_ = 64;    // Cache line aligned
                break;
                
            case DataTemperature::Hot:
                initial_capacity_ = 256;
                max_capacity_ = 1024;
                growth_factor_ = 2;
                memory_alignment_ = 64;
                break;
                
            case DataTemperature::Warm:
                initial_capacity_ = 512;
                max_capacity_ = 2048;
                growth_factor_ = 2;
                memory_alignment_ = 32;
                break;
                
            case DataTemperature::Cool:
                initial_capacity_ = 1024;
                max_capacity_ = 4096;
                growth_factor_ = 2;
                memory_alignment_ = 16;
                break;
                
            case DataTemperature::Cold:
                initial_capacity_ = 2048;
                max_capacity_ = 8192;
                growth_factor_ = 2;
                memory_alignment_ = 16;
                break;
                
            case DataTemperature::Frozen:
                initial_capacity_ = 4096;
                max_capacity_ = 16384;
                growth_factor_ = 2;
                memory_alignment_ = 8;     // Minimal alignment for frozen data
                break;
        }
    }
    
    void initialize_pool() {
        current_capacity_ = initial_capacity_;
        current_size_ = 0;
        
        blocks_.resize(current_capacity_);
        free_block_indices_.reserve(current_capacity_);
        
        // Pre-allocate some memory for this tier
        usize initial_memory_size = initial_capacity_ * 1024; // 1KB per slot initially
        raw_memory_ = std::aligned_alloc(memory_alignment_, initial_memory_size);
        
        if (raw_memory_) {
            total_memory_allocated_ = initial_memory_size;
        } else {
            total_memory_allocated_ = 0;
            LOG_WARNING("Failed to pre-allocate memory for thermal tier {}", 
                       static_cast<u32>(tier_temperature_));
        }
    }
    
    void cleanup_pool() {
        blocks_.clear();
        free_block_indices_.clear();
        
        if (raw_memory_) {
            std::free(raw_memory_);
            raw_memory_ = nullptr;
        }
        
        total_memory_allocated_ = 0;
    }
    
    bool expand_pool() {
        usize new_capacity = current_capacity_ * growth_factor_;
        if (new_capacity > max_capacity_) {
            new_capacity = max_capacity_;
        }
        
        if (new_capacity <= current_capacity_) {
            return false;
        }
        
        blocks_.resize(new_capacity);
        current_capacity_ = new_capacity;
        
        LOG_DEBUG("Expanded thermal tier {} pool to {} capacity", 
                 static_cast<u32>(tier_temperature_), new_capacity);
        
        return true;
    }
    
    void* allocate_aligned_memory(usize size, usize alignment) {
        // In a real implementation, this would use NUMA-aware allocation
        return std::aligned_alloc(alignment, size);
    }
    
    bool should_reallocate_for_tier(const ThermalBlock* block) const {
        // Hot tiers benefit from cache-aligned allocation
        if (tier_temperature_ >= DataTemperature::Hot && !block->is_cache_aligned) {
            return true;
        }
        
        // NUMA node mismatch
        if (block->current_numa_node != preferred_numa_node_) {
            return true;
        }
        
        return false;
    }
    
    void reallocate_block_for_tier(ThermalBlock* block) {
        // Reallocate with tier-optimal properties
        usize optimal_alignment = (tier_temperature_ >= DataTemperature::Hot) ? 
            core::CACHE_LINE_SIZE : memory_alignment_;
        
        void* new_memory = std::aligned_alloc(optimal_alignment, block->data_size);
        if (new_memory) {
            // Copy data to new location
            std::memcpy(new_memory, block->data_ptr, block->data_size);
            
            // Free old memory
            std::free(block->data_ptr);
            
            // Update block
            block->data_ptr = new_memory;
            block->data_alignment = optimal_alignment;
            block->is_cache_aligned = (optimal_alignment >= core::CACHE_LINE_SIZE);
            block->cache_line_offset = 
                reinterpret_cast<uintptr_t>(new_memory) % core::CACHE_LINE_SIZE;
        }
    }
    
    f64 calculate_tier_efficiency() const {
        f64 efficiency = 1.0;
        
        // Check how well blocks match this tier's temperature
        for (const auto& block : blocks_) {
            if (block) {
                DataTemperature block_temp = block->thermal.get_temperature_class();
                if (block_temp != tier_temperature_) {
                    // Penalize mismatched blocks
                    i32 temp_diff = static_cast<i32>(block_temp) - static_cast<i32>(tier_temperature_);
                    efficiency -= 0.1 * std::abs(temp_diff);
                }
            }
        }
        
        return std::max(0.0, efficiency);
    }
    
    constexpr usize align_up(usize value, usize alignment) const {
        return (value + alignment - 1) & ~(alignment - 1);
    }
    
    f64 get_current_time() const {
        using namespace std::chrono;
        return duration<f64>(steady_clock::now().time_since_epoch()).count();
    }
};

//=============================================================================
// Thermal Pool Manager
//=============================================================================

/**
 * @brief Manager for all thermal tiers with automatic migration
 */
class ThermalPoolManager {
private:
    // Thermal tier pools
    std::array<std::unique_ptr<ThermalTierPool>, 6> tier_pools_;
    
    // Block registry for fast lookup
    std::unordered_map<u64, DataTemperature> block_tier_map_;
    mutable std::shared_mutex block_map_mutex_;
    
    // Migration management
    std::thread migration_thread_;
    std::atomic<bool> migration_enabled_{true};
    std::atomic<f64> migration_check_interval_{1.0}; // 1 second
    std::atomic<bool> shutdown_requested_{false};
    
    // NUMA integration
    numa::NumaManager& numa_manager_;
    
    // Performance tracking
    alignas(core::CACHE_LINE_SIZE) std::atomic<u64> total_allocations_{0};
    alignas(core::CACHE_LINE_SIZE) std::atomic<u64> total_migrations_{0};
    alignas(core::CACHE_LINE_SIZE) std::atomic<u64> successful_migrations_{0};
    alignas(core::CACHE_LINE_SIZE) std::atomic<f64> migration_efficiency_{0.0};
    
public:
    explicit ThermalPoolManager(numa::NumaManager& numa_mgr = numa::get_global_numa_manager())
        : numa_manager_(numa_mgr) {
        
        initialize_tier_pools();
        start_migration_thread();
        
        LOG_INFO("Initialized thermal pool manager with {} temperature tiers", tier_pools_.size());
    }
    
    ~ThermalPoolManager() {
        shutdown_requested_.store(true);
        if (migration_thread_.joinable()) {
            migration_thread_.join();
        }
        
        LOG_INFO("Thermal pool manager shutdown. Total migrations: {}", total_migrations_.load());
    }
    
    /**
     * @brief Allocate thermally-managed memory block
     */
    ThermalBlock* allocate(usize size, usize alignment = alignof(std::max_align_t),
                          DataTemperature initial_tier = DataTemperature::Cool) {
        
        auto& tier_pool = get_tier_pool(initial_tier);
        ThermalBlock* block = tier_pool.allocate_block(size, alignment);
        
        if (block) {
            std::unique_lock<std::shared_mutex> lock(block_map_mutex_);
            block_tier_map_[block->block_id] = initial_tier;
            total_allocations_.fetch_add(1, std::memory_order_relaxed);
        }
        
        return block;
    }
    
    /**
     * @brief Record access to a thermal block
     */
    void record_access(ThermalBlock* block) {
        if (!block) return;
        
        block->record_access();
        
        // Immediate migration check for blazing hot data
        DataTemperature new_temp = block->thermal.get_temperature_class();
        if (new_temp == DataTemperature::Blazing && block->assigned_tier != DataTemperature::Blazing) {
            // Prioritize blazing hot data for immediate migration
            migrate_block_immediate(block->block_id, new_temp);
        }
    }
    
    /**
     * @brief Deallocate thermal block
     */
    void deallocate(ThermalBlock* block) {
        if (!block) return;
        
        u64 block_id = block->block_id;
        DataTemperature tier;
        
        {
            std::shared_lock<std::shared_mutex> lock(block_map_mutex_);
            auto it = block_tier_map_.find(block_id);
            if (it == block_tier_map_.end()) {
                LOG_WARNING("Attempted to deallocate unknown thermal block {}", block_id);
                return;
            }
            tier = it->second;
        }
        
        // Free the data memory
        if (block->data_ptr) {
            std::free(block->data_ptr);
        }
        
        // Remove from tier pool (simplified - in real implementation would remove from pool)
        {
            std::unique_lock<std::shared_mutex> lock(block_map_mutex_);
            block_tier_map_.erase(block_id);
        }
    }
    
    /**
     * @brief Force thermal state update for all blocks
     */
    void update_thermal_states() {
        for (auto& tier_pool : tier_pools_) {
            if (tier_pool) {
                tier_pool->update_thermal_states();
            }
        }
    }
    
    /**
     * @brief Get comprehensive thermal statistics
     */
    struct ThermalManagerStatistics {
        struct TierStats {
            DataTemperature tier;
            usize total_blocks;
            usize memory_used;
            f64 average_temperature;
            f64 utilization_ratio;
            u64 migrations_in;
            u64 migrations_out;
            f64 tier_efficiency;
        };
        
        std::vector<TierStats> tier_statistics;
        u64 total_allocations;
        u64 total_migrations;
        u64 successful_migrations;
        f64 migration_efficiency;
        f64 overall_thermal_efficiency;
        DataTemperature hottest_tier;
        DataTemperature most_utilized_tier;
        std::string performance_summary;
    };
    
    ThermalManagerStatistics get_statistics() const {
        ThermalManagerStatistics stats{};
        
        stats.total_allocations = total_allocations_.load();
        stats.total_migrations = total_migrations_.load();
        stats.successful_migrations = successful_migrations_.load();
        stats.migration_efficiency = migration_efficiency_.load();
        
        f64 total_efficiency = 0.0;
        f64 max_utilization = 0.0;
        f64 max_temperature = 0.0;
        DataTemperature hottest = DataTemperature::Frozen;
        DataTemperature most_utilized = DataTemperature::Frozen;
        
        for (usize i = 0; i < tier_pools_.size(); ++i) {
            if (!tier_pools_[i]) continue;
            
            auto tier_stats = tier_pools_[i]->get_statistics();
            
            ThermalManagerStatistics::TierStats tier_stat{};
            tier_stat.tier = tier_stats.tier;
            tier_stat.total_blocks = tier_stats.size;
            tier_stat.memory_used = tier_stats.memory_allocated;
            tier_stat.average_temperature = tier_stats.average_temperature;
            tier_stat.utilization_ratio = tier_stats.utilization_ratio;
            tier_stat.migrations_in = tier_stats.migrations_in;
            tier_stat.migrations_out = tier_stats.migrations_out;
            tier_stat.tier_efficiency = tier_stats.tier_efficiency_score;
            
            stats.tier_statistics.push_back(tier_stat);
            
            total_efficiency += tier_stat.tier_efficiency;
            
            if (tier_stat.average_temperature > max_temperature) {
                max_temperature = tier_stat.average_temperature;
                hottest = tier_stat.tier;
            }
            
            if (tier_stat.utilization_ratio > max_utilization) {
                max_utilization = tier_stat.utilization_ratio;
                most_utilized = tier_stat.tier;
            }
        }
        
        stats.overall_thermal_efficiency = total_efficiency / tier_pools_.size();
        stats.hottest_tier = hottest;
        stats.most_utilized_tier = most_utilized;
        
        // Generate performance summary
        if (stats.overall_thermal_efficiency > 0.8) {
            stats.performance_summary = "Excellent thermal management - optimal data placement";
        } else if (stats.overall_thermal_efficiency > 0.6) {
            stats.performance_summary = "Good thermal management - some optimization opportunities";
        } else {
            stats.performance_summary = "Poor thermal management - significant migrations needed";
        }
        
        return stats;
    }
    
    /**
     * @brief Configuration and control
     */
    void set_migration_enabled(bool enabled) {
        migration_enabled_.store(enabled);
    }
    
    void set_migration_check_interval(f64 interval_seconds) {
        migration_check_interval_.store(interval_seconds);
    }
    
    void force_migration_cycle() {
        if (!migration_enabled_.load()) return;
        perform_migration_cycle();
    }
    
private:
    void initialize_tier_pools() {
        // Create pools for each temperature tier
        auto available_nodes = numa_manager_.get_topology().get_available_nodes();
        if (available_nodes.empty()) {
            available_nodes.push_back(0); // Fallback
        }
        
        for (usize i = 0; i < tier_pools_.size(); ++i) {
            DataTemperature tier = static_cast<DataTemperature>(i);
            
            // Distribute hot tiers across NUMA nodes for better performance
            u32 numa_node = (tier >= DataTemperature::Hot) ? 
                available_nodes[i % available_nodes.size()] : available_nodes[0];
            
            tier_pools_[i] = std::make_unique<ThermalTierPool>(tier, numa_node);
        }
    }
    
    void start_migration_thread() {
        migration_thread_ = std::thread([this]() {
            migration_worker();
        });
    }
    
    void migration_worker() {
        while (!shutdown_requested_.load()) {
            f64 interval = migration_check_interval_.load();
            std::this_thread::sleep_for(std::chrono::duration<f64>(interval));
            
            if (migration_enabled_.load()) {
                perform_migration_cycle();
            }
        }
    }
    
    void perform_migration_cycle() {
        // Update thermal states first
        update_thermal_states();
        
        // Collect migration candidates from all tiers
        std::vector<std::pair<u64, DataTemperature>> migration_list;
        
        for (auto& tier_pool : tier_pools_) {
            if (!tier_pool) continue;
            
            auto candidates = tier_pool->get_migration_candidates();
            DataTemperature source_tier = tier_pool->get_tier();
            
            for (u64 block_id : candidates) {
                // Determine target tier (simplified)
                DataTemperature target_tier = determine_target_tier(block_id);
                if (target_tier != source_tier) {
                    migration_list.emplace_back(block_id, target_tier);
                }
            }
        }
        
        // Perform migrations
        usize successful_count = 0;
        for (const auto& [block_id, target_tier] : migration_list) {
            if (migrate_block(block_id, target_tier)) {
                successful_count++;
            }
        }
        
        // Update statistics
        total_migrations_.fetch_add(migration_list.size(), std::memory_order_relaxed);
        successful_migrations_.fetch_add(successful_count, std::memory_order_relaxed);
        
        if (migration_list.size() > 0) {
            f64 efficiency = static_cast<f64>(successful_count) / migration_list.size();
            migration_efficiency_.store(efficiency, std::memory_order_relaxed);
            
            LOG_DEBUG("Migration cycle completed: {}/{} successful", 
                     successful_count, migration_list.size());
        }
    }
    
    bool migrate_block(u64 block_id, DataTemperature target_tier) {
        DataTemperature source_tier;
        
        {
            std::shared_lock<std::shared_mutex> lock(block_map_mutex_);
            auto it = block_tier_map_.find(block_id);
            if (it == block_tier_map_.end()) {
                return false;
            }
            source_tier = it->second;
        }
        
        if (source_tier == target_tier) {
            return true; // Already in correct tier
        }
        
        // Move block from source to target tier
        auto& source_pool = get_tier_pool(source_tier);
        auto& target_pool = get_tier_pool(target_tier);
        
        auto block = source_pool.migrate_block_out(block_id);
        if (!block) {
            return false;
        }
        
        if (target_pool.migrate_block_in(std::move(block))) {
            std::unique_lock<std::shared_mutex> lock(block_map_mutex_);
            block_tier_map_[block_id] = target_tier;
            return true;
        }
        
        return false;
    }
    
    void migrate_block_immediate(u64 block_id, DataTemperature target_tier) {
        // Immediate migration for critical hot data
        migrate_block(block_id, target_tier);
    }
    
    DataTemperature determine_target_tier(u64 block_id) const {
        // Simplified target tier determination
        return DataTemperature::Cool; // Would analyze thermal properties in real implementation
    }
    
    ThermalTierPool& get_tier_pool(DataTemperature tier) {
        usize index = static_cast<usize>(tier);
        if (index >= tier_pools_.size()) {
            index = static_cast<usize>(DataTemperature::Cool);
        }
        return *tier_pools_[index];
    }
};

//=============================================================================
// Educational Thermal Visualization
//=============================================================================

/**
 * @brief Educational tools for visualizing thermal memory management
 */
class ThermalVisualizationTools {
private:
    const ThermalPoolManager& manager_;
    
public:
    explicit ThermalVisualizationTools(const ThermalPoolManager& mgr) 
        : manager_(mgr) {}
    
    /**
     * @brief Generate educational thermal report
     */
    struct ThermalReport {
        std::string thermal_summary;
        std::vector<std::string> tier_analysis;
        std::vector<std::string> optimization_suggestions;
        std::vector<std::string> educational_insights;
        f64 thermal_efficiency_score;
    };
    
    ThermalReport generate_educational_report() const {
        ThermalReport report{};
        
        auto stats = manager_.get_statistics();
        report.thermal_efficiency_score = stats.overall_thermal_efficiency;
        
        // Thermal summary
        report.thermal_summary = "Thermal Memory Management Analysis:\n";
        report.thermal_summary += "- Total allocations: " + std::to_string(stats.total_allocations) + "\n";
        report.thermal_summary += "- Migration efficiency: " + 
            std::to_string(stats.migration_efficiency * 100.0) + "%\n";
        report.thermal_summary += "- Overall efficiency: " + 
            std::to_string(report.thermal_efficiency_score * 100.0) + "%";
        
        // Tier analysis
        for (const auto& tier_stat : stats.tier_statistics) {
            std::string tier_name = get_tier_name(tier_stat.tier);
            std::string analysis = tier_name + " tier: ";
            analysis += std::to_string(tier_stat.total_blocks) + " blocks, ";
            analysis += std::to_string(static_cast<u32>(tier_stat.utilization_ratio * 100)) + "% utilized";
            report.tier_analysis.push_back(analysis);
        }
        
        // Optimization suggestions
        if (stats.migration_efficiency < 0.7) {
            report.optimization_suggestions.push_back(
                "High migration overhead - consider tuning thermal thresholds");
        }
        
        if (report.thermal_efficiency_score < 0.6) {
            report.optimization_suggestions.push_back(
                "Poor thermal classification - analyze access patterns");
        }
        
        // Educational insights
        report.educational_insights.push_back(
            "Hot data benefits from cache-line alignment and NUMA locality");
        report.educational_insights.push_back(
            "Cold data can use denser packing to save memory");
        report.educational_insights.push_back(
            "Thermal management reduces cache pollution from cold data");
        report.educational_insights.push_back(
            "Access pattern prediction improves proactive thermal management");
        
        return report;
    }
    
private:
    std::string get_tier_name(DataTemperature tier) const {
        switch (tier) {
            case DataTemperature::Blazing: return "Blazing";
            case DataTemperature::Hot: return "Hot";
            case DataTemperature::Warm: return "Warm";
            case DataTemperature::Cool: return "Cool";
            case DataTemperature::Cold: return "Cold";
            case DataTemperature::Frozen: return "Frozen";
            default: return "Unknown";
        }
    }
};

//=============================================================================
// Global Thermal Pool Manager
//=============================================================================

ThermalPoolManager& get_global_thermal_pool_manager();

} // namespace ecscope::memory::specialized::thermal