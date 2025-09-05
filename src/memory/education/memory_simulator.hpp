#pragma once

/**
 * @file memory/education/memory_simulator.hpp
 * @brief Educational Memory Allocation Simulator and Visualization Engine
 * 
 * This header implements a comprehensive educational memory allocation simulator
 * that demonstrates various memory management concepts, allocation strategies,
 * and their performance characteristics through interactive visualization and
 * real-time analysis.
 * 
 * Key Features:
 * - Interactive memory allocation scenario simulation
 * - Real-time visualization of allocation patterns and fragmentation
 * - Performance comparison between different allocation strategies
 * - Cache behavior simulation and analysis
 * - Memory access pattern visualization and optimization
 * - Educational scenarios for common memory management problems
 * - Step-by-step allocation algorithm demonstrations
 * - Memory optimization strategy tutorials
 * 
 * Educational Value:
 * - Demonstrates memory allocation algorithms in action
 * - Shows cache behavior and locality effects
 * - Illustrates fragmentation formation and mitigation
 * - Provides hands-on learning for memory optimization
 * - Educational examples of allocation strategy trade-offs
 * 
 * @author ECScope Educational ECS Framework - Memory Education
 * @date 2025
 */

#include "memory/memory_tracker.hpp"
#include "memory/specialized/thermal_pools.hpp"
#include "memory/debugging/advanced_debugger.hpp"
#include "core/types.hpp"
#include "core/log.hpp"
#include <atomic>
#include <memory>
#include <vector>
#include <array>
#include <unordered_map>
#include <string>
#include <sstream>
#include <fstream>
#include <chrono>
#include <thread>
#include <mutex>
#include <random>
#include <algorithm>
#include <functional>
#include <queue>

namespace ecscope::memory::education {

//=============================================================================
// Simulation Types and Parameters
//=============================================================================

/**
 * @brief Memory allocation strategies for educational comparison
 */
enum class AllocationStrategy : u8 {
    FirstFit        = 0,  // First available block that fits
    BestFit         = 1,  // Smallest block that fits
    WorstFit        = 2,  // Largest block that fits
    NextFit         = 3,  // Continue from last allocation
    BuddySystem     = 4,  // Binary buddy allocation
    SegregatedFit   = 5,  // Size-segregated free lists
    SlabAllocation  = 6,  // Slab allocator for fixed sizes
    StackAllocation = 7,  // Simple stack-based allocation
    PoolAllocation  = 8   // Pool-based allocation
};

/**
 * @brief Allocation patterns for simulation scenarios
 */
enum class AllocationPattern : u8 {
    Random          = 0,  // Random size allocations
    Sequential      = 1,  // Sequential size increases
    PowerOfTwo      = 2,  // Power-of-2 sizes only
    Bimodal         = 3,  // Small and large sizes
    Exponential     = 4,  // Exponentially distributed sizes
    Uniform         = 5,  // Uniform size distribution
    RealWorld       = 6,  // Real-world allocation patterns
    Pathological    = 7   // Worst-case fragmentation patterns
};

/**
 * @brief Cache simulation parameters
 */
struct CacheParameters {
    usize cache_size;           // Cache size in bytes
    usize cache_line_size;      // Cache line size
    usize associativity;        // Set associativity (1 = direct mapped)
    f64 hit_latency;           // Cache hit latency (cycles)
    f64 miss_latency;          // Cache miss latency (cycles)
    
    CacheParameters() noexcept
        : cache_size(32 * 1024),      // 32KB L1 cache
          cache_line_size(64),        // 64-byte cache lines
          associativity(8),           // 8-way set associative
          hit_latency(1.0),          // 1 cycle hit
          miss_latency(100.0) {}     // 100 cycle miss
};

/**
 * @brief Simulation scenario configuration
 */
struct SimulationScenario {
    std::string name;
    std::string description;
    AllocationStrategy strategy;
    AllocationPattern pattern;
    
    // Memory parameters
    usize heap_size;
    usize min_allocation_size;
    usize max_allocation_size;
    usize num_allocations;
    f64 deallocation_probability;  // Probability of deallocating per step
    
    // Cache simulation
    CacheParameters cache_params;
    bool simulate_cache_behavior;
    
    // Educational parameters
    bool enable_visualization;
    bool step_by_step_mode;
    f64 animation_speed;          // Seconds per step
    
    SimulationScenario() noexcept
        : name("Default Scenario"),
          description("Basic allocation simulation"),
          strategy(AllocationStrategy::FirstFit),
          pattern(AllocationPattern::Random),
          heap_size(1024 * 1024),    // 1MB heap
          min_allocation_size(16),    // 16 bytes minimum
          max_allocation_size(4096),  // 4KB maximum
          num_allocations(1000),      // 1000 allocations
          deallocation_probability(0.3), // 30% chance to deallocate
          simulate_cache_behavior(true),
          enable_visualization(true),
          step_by_step_mode(false),
          animation_speed(0.1) {}
};

//=============================================================================
// Memory Visualization System
//=============================================================================

/**
 * @brief Visual representation of memory blocks
 */
struct MemoryBlock {
    usize offset;           // Offset from heap start
    usize size;            // Block size in bytes
    bool is_allocated;     // Whether block is allocated
    u32 allocation_id;     // Unique allocation ID
    f64 allocation_time;   // When block was allocated
    f64 last_access_time;  // Last access time
    u32 access_count;      // Number of accesses
    f64 temperature;       // Heat value (0.0 = cold, 1.0 = hot)
    
    MemoryBlock() noexcept
        : offset(0), size(0), is_allocated(false), allocation_id(0),
          allocation_time(0.0), last_access_time(0.0), access_count(0), temperature(0.0) {}
    
    MemoryBlock(usize off, usize sz, bool allocated = false) noexcept
        : offset(off), size(sz), is_allocated(allocated), allocation_id(0),
          allocation_time(0.0), last_access_time(0.0), access_count(0), temperature(0.0) {}
};

/**
 * @brief Memory visualization engine
 */
class MemoryVisualizer {
private:
    std::vector<MemoryBlock> memory_blocks_;
    usize heap_size_;
    f64 visualization_scale_;    // Pixels per byte
    f64 current_time_;
    
    mutable std::mutex visualizer_mutex_;
    
    // Heat map parameters
    f64 heat_decay_rate_;        // How fast blocks cool down
    f64 max_temperature_;        // Maximum temperature value
    
public:
    explicit MemoryVisualizer(usize heap_size, f64 scale = 0.001) 
        : heap_size_(heap_size), visualization_scale_(scale), current_time_(0.0),
          heat_decay_rate_(0.95), max_temperature_(1.0) {
        
        // Initialize with one large free block
        memory_blocks_.emplace_back(0, heap_size, false);
        
        LOG_DEBUG("Initialized memory visualizer: heap={}KB, scale={}", 
                 heap_size / 1024, scale);
    }
    
    /**
     * @brief Record allocation for visualization
     */
    void record_allocation(usize offset, usize size, u32 allocation_id) {
        std::lock_guard<std::mutex> lock(visualizer_mutex_);
        current_time_ = get_current_time();
        
        // Find and split the appropriate free block
        for (auto it = memory_blocks_.begin(); it != memory_blocks_.end(); ++it) {
            if (!it->is_allocated && it->offset <= offset && 
                it->offset + it->size >= offset + size) {
                
                // Split the free block
                usize original_size = it->size;
                usize original_offset = it->offset;
                
                // Remove original block
                memory_blocks_.erase(it);
                
                // Add blocks before allocation (if any)
                if (offset > original_offset) {
                    memory_blocks_.emplace_back(original_offset, offset - original_offset, false);
                }
                
                // Add allocated block
                MemoryBlock allocated_block(offset, size, true);
                allocated_block.allocation_id = allocation_id;
                allocated_block.allocation_time = current_time_;
                allocated_block.last_access_time = current_time_;
                allocated_block.temperature = 1.0; // Start hot
                memory_blocks_.push_back(allocated_block);
                
                // Add blocks after allocation (if any)
                usize remaining_offset = offset + size;
                usize remaining_size = original_offset + original_size - remaining_offset;
                if (remaining_size > 0) {
                    memory_blocks_.emplace_back(remaining_offset, remaining_size, false);
                }
                
                break;
            }
        }
        
        // Sort blocks by offset for easier visualization
        std::sort(memory_blocks_.begin(), memory_blocks_.end(),
                 [](const MemoryBlock& a, const MemoryBlock& b) {
                     return a.offset < b.offset;
                 });
        
        LOG_TRACE("Recorded allocation: offset={}, size={}, id={}", offset, size, allocation_id);
    }
    
    /**
     * @brief Record deallocation for visualization
     */
    void record_deallocation(usize offset, usize size) {
        std::lock_guard<std::mutex> lock(visualizer_mutex_);
        current_time_ = get_current_time();
        
        // Find the allocated block and mark as free
        for (auto& block : memory_blocks_) {
            if (block.is_allocated && block.offset == offset && block.size == size) {
                block.is_allocated = false;
                block.allocation_id = 0;
                block.temperature = 0.0; // Cool down immediately
                break;
            }
        }
        
        // Coalesce adjacent free blocks
        coalesce_free_blocks();
        
        LOG_TRACE("Recorded deallocation: offset={}, size={}", offset, size);
    }
    
    /**
     * @brief Record memory access for heat visualization
     */
    void record_access(usize offset, usize size) {
        std::lock_guard<std::mutex> lock(visualizer_mutex_);
        current_time_ = get_current_time();
        
        // Heat up accessed blocks
        for (auto& block : memory_blocks_) {
            if (block.is_allocated && 
                block.offset <= offset && 
                block.offset + block.size >= offset + size) {
                
                block.last_access_time = current_time_;
                block.access_count++;
                block.temperature = std::min(block.temperature + 0.2, max_temperature_);
            }
        }
    }
    
    /**
     * @brief Update visualization (decay heat, etc.)
     */
    void update_visualization() {
        std::lock_guard<std::mutex> lock(visualizer_mutex_);
        current_time_ = get_current_time();
        
        // Decay heat over time
        for (auto& block : memory_blocks_) {
            if (block.is_allocated) {
                f64 time_since_access = current_time_ - block.last_access_time;
                f64 decay_factor = std::pow(heat_decay_rate_, time_since_access);
                block.temperature *= decay_factor;
            }
        }
    }
    
    /**
     * @brief Generate visualization data as text representation
     */
    std::string generate_text_visualization(usize width = 80) const {
        std::lock_guard<std::mutex> lock(visualizer_mutex_);
        
        std::ostringstream viz;
        viz << "Memory Layout Visualization (Total: " << heap_size_ / 1024 << "KB)\n";
        viz << std::string(width, '=') << "\n";
        
        // Generate memory map
        std::vector<char> memory_map(width, ' ');
        std::vector<f64> heat_map(width, 0.0);
        
        for (const auto& block : memory_blocks_) {
            usize start_pos = static_cast<usize>((static_cast<f64>(block.offset) / heap_size_) * width);
            usize end_pos = static_cast<usize>((static_cast<f64>(block.offset + block.size) / heap_size_) * width);
            end_pos = std::min(end_pos, width - 1);
            
            char symbol = block.is_allocated ? '#' : '.';
            for (usize i = start_pos; i <= end_pos && i < width; ++i) {
                memory_map[i] = symbol;
                if (block.is_allocated) {
                    heat_map[i] = block.temperature;
                }
            }
        }
        
        // Generate heat-colored visualization
        viz << "Memory Map: ";
        for (usize i = 0; i < width; ++i) {
            if (memory_map[i] == '#') {
                // Use different symbols based on temperature
                if (heat_map[i] > 0.8) {
                    viz << 'H';  // Hot
                } else if (heat_map[i] > 0.5) {
                    viz << 'W';  // Warm
                } else if (heat_map[i] > 0.2) {
                    viz << 'C';  // Cool
                } else {
                    viz << '#';  // Cold
                }
            } else {
                viz << memory_map[i];
            }
        }
        viz << "\n";
        
        // Legend
        viz << "Legend: . = Free, # = Cold, C = Cool, W = Warm, H = Hot\n";
        
        // Statistics
        auto stats = calculate_fragmentation_stats();
        viz << "Fragmentation: " << std::fixed << std::setprecision(1) << stats.fragmentation_ratio * 100 << "%";
        viz << ", Free blocks: " << stats.free_blocks_count;
        viz << ", Largest free: " << stats.largest_free_block / 1024 << "KB\n";
        
        return viz.str();
    }
    
    /**
     * @brief Export visualization data for external tools
     */
    void export_visualization_data(const std::string& filename) const {
        std::lock_guard<std::mutex> lock(visualizer_mutex_);
        
        std::ofstream file(filename);
        if (!file.is_open()) {
            LOG_ERROR("Failed to open visualization export file: {}", filename);
            return;
        }
        
        // Export as CSV
        file << "offset,size,allocated,allocation_id,temperature,access_count\n";
        
        for (const auto& block : memory_blocks_) {
            file << block.offset << "," << block.size << "," 
                 << (block.is_allocated ? 1 : 0) << ","
                 << block.allocation_id << "," << block.temperature << ","
                 << block.access_count << "\n";
        }
        
        file.close();
        LOG_INFO("Exported visualization data to: {}", filename);
    }
    
    /**
     * @brief Calculate fragmentation statistics
     */
    struct FragmentationStats {
        usize total_free_space;
        usize largest_free_block;
        usize free_blocks_count;
        f64 fragmentation_ratio;    // 0.0 = no fragmentation, 1.0 = maximum
        f64 utilization_ratio;      // Fraction of heap in use
        f64 average_block_temperature;
    };
    
    FragmentationStats calculate_fragmentation_stats() const {
        FragmentationStats stats{};
        
        usize total_allocated = 0;
        f64 total_temperature = 0.0;
        usize allocated_blocks = 0;
        
        for (const auto& block : memory_blocks_) {
            if (block.is_allocated) {
                total_allocated += block.size;
                total_temperature += block.temperature;
                allocated_blocks++;
            } else {
                stats.total_free_space += block.size;
                stats.largest_free_block = std::max(stats.largest_free_block, block.size);
                stats.free_blocks_count++;
            }
        }
        
        stats.utilization_ratio = static_cast<f64>(total_allocated) / heap_size_;
        
        if (stats.total_free_space > 0) {
            stats.fragmentation_ratio = 1.0 - (static_cast<f64>(stats.largest_free_block) / stats.total_free_space);
        }
        
        if (allocated_blocks > 0) {
            stats.average_block_temperature = total_temperature / allocated_blocks;
        }
        
        return stats;
    }
    
    const std::vector<MemoryBlock>& get_blocks() const { return memory_blocks_; }
    
private:
    void coalesce_free_blocks() {
        // Sort blocks by offset
        std::sort(memory_blocks_.begin(), memory_blocks_.end(),
                 [](const MemoryBlock& a, const MemoryBlock& b) {
                     return a.offset < b.offset;
                 });
        
        // Coalesce adjacent free blocks
        for (auto it = memory_blocks_.begin(); it != memory_blocks_.end() - 1; ) {
            auto next_it = it + 1;
            
            if (!it->is_allocated && !next_it->is_allocated && 
                it->offset + it->size == next_it->offset) {
                
                // Merge blocks
                it->size += next_it->size;
                memory_blocks_.erase(next_it);
            } else {
                ++it;
            }
        }
    }
    
    f64 get_current_time() const {
        using namespace std::chrono;
        return duration<f64>(steady_clock::now().time_since_epoch()).count();
    }
};

//=============================================================================
// Cache Simulator
//=============================================================================

/**
 * @brief Simple cache simulator for educational purposes
 */
class CacheSimulator {
private:
    struct CacheLine {
        u64 tag;
        bool valid;
        f64 last_access_time;
        u32 access_count;
        
        CacheLine() : tag(0), valid(false), last_access_time(0.0), access_count(0) {}
    };
    
    std::vector<std::vector<CacheLine>> cache_sets_;
    CacheParameters params_;
    
    // Statistics
    alignas(core::CACHE_LINE_SIZE) std::atomic<u64> cache_hits_{0};
    alignas(core::CACHE_LINE_SIZE) std::atomic<u64> cache_misses_{0};
    alignas(core::CACHE_LINE_SIZE) std::atomic<u64> total_accesses_{0};
    
    mutable std::mutex cache_mutex_;
    
public:
    explicit CacheSimulator(const CacheParameters& params) : params_(params) {
        usize num_sets = params_.cache_size / (params_.cache_line_size * params_.associativity);
        cache_sets_.resize(num_sets);
        
        for (auto& set : cache_sets_) {
            set.resize(params_.associativity);
        }
        
        LOG_DEBUG("Initialized cache simulator: {}KB, {}-way, {} sets", 
                 params_.cache_size / 1024, params_.associativity, num_sets);
    }
    
    /**
     * @brief Simulate memory access and return hit/miss
     */
    bool access_memory(u64 address, bool& was_hit, f64& access_latency) {
        std::lock_guard<std::mutex> lock(cache_mutex_);
        
        total_accesses_.fetch_add(1, std::memory_order_relaxed);
        f64 current_time = get_current_time();
        
        // Calculate cache line address and set index
        u64 cache_line_addr = address / params_.cache_line_size;
        usize set_index = cache_line_addr % cache_sets_.size();
        u64 tag = cache_line_addr / cache_sets_.size();
        
        auto& cache_set = cache_sets_[set_index];
        
        // Check for cache hit
        for (auto& line : cache_set) {
            if (line.valid && line.tag == tag) {
                // Cache hit
                line.last_access_time = current_time;
                line.access_count++;
                
                cache_hits_.fetch_add(1, std::memory_order_relaxed);
                was_hit = true;
                access_latency = params_.hit_latency;
                
                LOG_TRACE("Cache hit: addr={}, set={}, tag={}", address, set_index, tag);
                return true;
            }
        }
        
        // Cache miss - find replacement victim
        auto victim_it = find_lru_victim(cache_set);
        victim_it->valid = true;
        victim_it->tag = tag;
        victim_it->last_access_time = current_time;
        victim_it->access_count = 1;
        
        cache_misses_.fetch_add(1, std::memory_order_relaxed);
        was_hit = false;
        access_latency = params_.miss_latency;
        
        LOG_TRACE("Cache miss: addr={}, set={}, tag={}", address, set_index, tag);
        return false;
    }
    
    /**
     * @brief Get cache performance statistics
     */
    struct CacheStatistics {
        u64 total_accesses;
        u64 cache_hits;
        u64 cache_misses;
        f64 hit_rate;
        f64 miss_rate;
        f64 average_access_latency;
        CacheParameters parameters;
    };
    
    CacheStatistics get_statistics() const {
        std::lock_guard<std::mutex> lock(cache_mutex_);
        
        CacheStatistics stats{};
        stats.total_accesses = total_accesses_.load();
        stats.cache_hits = cache_hits_.load();
        stats.cache_misses = cache_misses_.load();
        stats.parameters = params_;
        
        if (stats.total_accesses > 0) {
            stats.hit_rate = static_cast<f64>(stats.cache_hits) / stats.total_accesses;
            stats.miss_rate = static_cast<f64>(stats.cache_misses) / stats.total_accesses;
            
            stats.average_access_latency = 
                stats.hit_rate * params_.hit_latency + 
                stats.miss_rate * params_.miss_latency;
        }
        
        return stats;
    }
    
    void reset_statistics() {
        std::lock_guard<std::mutex> lock(cache_mutex_);
        cache_hits_.store(0);
        cache_misses_.store(0);
        total_accesses_.store(0);
    }
    
private:
    std::vector<CacheLine>::iterator find_lru_victim(std::vector<CacheLine>& cache_set) {
        // Find least recently used cache line
        auto lru_it = cache_set.begin();
        f64 oldest_time = lru_it->last_access_time;
        
        for (auto it = cache_set.begin(); it != cache_set.end(); ++it) {
            if (!it->valid) {
                return it; // Return invalid line immediately
            }
            
            if (it->last_access_time < oldest_time) {
                oldest_time = it->last_access_time;
                lru_it = it;
            }
        }
        
        return lru_it;
    }
    
    f64 get_current_time() const {
        using namespace std::chrono;
        return duration<f64>(steady_clock::now().time_since_epoch()).count();
    }
};
          cache_line_size(64),        // 64-byte cache lines
          associativity(8),           // 8-way set associative
          hit_latency(1.0),           // 1 cycle hit
          miss_latency(100.0) {}      // 100 cycle miss
};

/**
 * @brief Simulated memory block
 */
struct SimulatedBlock {
    void* address;                  // Simulated address
    usize size;                     // Block size
    bool is_allocated;              // Allocation status
    f64 allocation_time;            // When allocated
    f64 last_access_time;           // Last access time
    u32 access_count;              // Number of accesses
    AllocationStrategy strategy;    // Strategy used to allocate
    u32 allocation_id;             // Unique allocation ID
    
    // Visualization data
    f32 visual_x, visual_y;        // Position in visualization
    f32 heat;                      // Access heat for visualization
    
    SimulatedBlock() noexcept
        : address(nullptr), size(0), is_allocated(false),
          allocation_time(0.0), last_access_time(0.0), access_count(0),
          strategy(AllocationStrategy::FirstFit), allocation_id(0),
          visual_x(0.0f), visual_y(0.0f), heat(0.0f) {}
};

/**
 * @brief Memory simulation state
 */
struct SimulationState {
    // Memory layout
    std::vector<SimulatedBlock> memory_blocks;
    usize total_memory_size;
    usize allocated_memory;
    usize free_memory;
    f64 fragmentation_ratio;
    
    // Performance metrics
    u64 total_allocations;
    u64 successful_allocations;
    u64 failed_allocations;
    f64 average_allocation_time;
    f64 total_allocation_overhead;
    
    // Cache simulation
    u64 cache_hits;
    u64 cache_misses;
    f64 cache_hit_ratio;
    f64 estimated_access_time;
    
    // Timing
    f64 simulation_time;
    f64 real_time_elapsed;
    
    SimulationState() noexcept {
        reset();
    }
    
    void reset() {
        memory_blocks.clear();
        total_memory_size = 0;
        allocated_memory = 0;
        free_memory = 0;
        fragmentation_ratio = 0.0;
        
        total_allocations = 0;
        successful_allocations = 0;
        failed_allocations = 0;
        average_allocation_time = 0.0;
        total_allocation_overhead = 0.0;
        
        cache_hits = 0;
        cache_misses = 0;
        cache_hit_ratio = 0.0;
        estimated_access_time = 0.0;
        
        simulation_time = 0.0;
        real_time_elapsed = 0.0;
    }
};

//=============================================================================
// Cache Simulator
//=============================================================================

/**
 * @brief Simple cache simulator for educational purposes
 */
class CacheSimulator {
private:
    struct CacheEntry {
        u64 tag;                    // Address tag
        bool valid;                 // Valid bit
        f64 last_access_time;       // LRU tracking
        u32 access_count;           // Access frequency
        
        CacheEntry() noexcept 
            : tag(0), valid(false), last_access_time(0.0), access_count(0) {}
    };
    
    CacheParameters params_;
    std::vector<std::vector<CacheEntry>> cache_sets_;
    
    // Statistics
    std::atomic<u64> total_accesses_{0};
    std::atomic<u64> cache_hits_{0};
    std::atomic<u64> cache_misses_{0};
    std::atomic<f64> total_access_time_{0.0};
    
    // Cache configuration
    usize num_sets_;
    usize offset_bits_;
    usize index_bits_;
    usize tag_bits_;
    
public:
    explicit CacheSimulator(const CacheParameters& params = CacheParameters{})
        : params_(params) {
        
        // Calculate cache geometry
        num_sets_ = params_.cache_size / (params_.cache_line_size * params_.associativity);
        offset_bits_ = static_cast<usize>(std::log2(params_.cache_line_size));
        index_bits_ = static_cast<usize>(std::log2(num_sets_));
        tag_bits_ = 64 - offset_bits_ - index_bits_; // Assume 64-bit addresses
        
        // Initialize cache sets
        cache_sets_.resize(num_sets_);
        for (auto& set : cache_sets_) {
            set.resize(params_.associativity);
        }
        
        LOG_DEBUG("Initialized cache simulator: {}KB, {}-way, {} sets, {} byte lines",
                 params_.cache_size / 1024, params_.associativity, num_sets_, params_.cache_line_size);
    }
    
    /**
     * @brief Simulate memory access
     */
    struct AccessResult {
        bool is_hit;
        f64 access_latency;
        u64 tag;
        usize set_index;
        usize way_index;
        bool eviction_occurred;
        u64 evicted_tag;
    };
    
    AccessResult simulate_access(uintptr_t address) {
        AccessResult result{};
        
        // Extract address components
        usize offset = address & ((1UL << offset_bits_) - 1);
        usize index = (address >> offset_bits_) & ((1UL << index_bits_) - 1);
        u64 tag = address >> (offset_bits_ + index_bits_);
        
        result.tag = tag;
        result.set_index = index;
        
        f64 current_time = get_current_time();
        
        auto& cache_set = cache_sets_[index];
        
        // Check for cache hit
        for (usize way = 0; way < cache_set.size(); ++way) {
            auto& entry = cache_set[way];
            if (entry.valid && entry.tag == tag) {
                // Cache hit
                result.is_hit = true;
                result.access_latency = params_.hit_latency;
                result.way_index = way;
                
                entry.last_access_time = current_time;
                entry.access_count++;
                
                cache_hits_.fetch_add(1, std::memory_order_relaxed);
                break;
            }
        }
        
        if (!result.is_hit) {
            // Cache miss - need to find replacement
            result.access_latency = params_.miss_latency;
            
            // Find empty way or LRU way
            usize replacement_way = 0;
            f64 oldest_time = current_time;
            bool found_empty = false;
            
            for (usize way = 0; way < cache_set.size(); ++way) {
                auto& entry = cache_set[way];
                if (!entry.valid) {
                    replacement_way = way;
                    found_empty = true;
                    break;
                }
                
                if (entry.last_access_time < oldest_time) {
                    oldest_time = entry.last_access_time;
                    replacement_way = way;
                }
            }
            
            // Replace entry
            auto& replacement_entry = cache_set[replacement_way];
            if (replacement_entry.valid && !found_empty) {
                result.eviction_occurred = true;
                result.evicted_tag = replacement_entry.tag;
            }
            
            replacement_entry.tag = tag;
            replacement_entry.valid = true;
            replacement_entry.last_access_time = current_time;
            replacement_entry.access_count = 1;
            
            result.way_index = replacement_way;
            
            cache_misses_.fetch_add(1, std::memory_order_relaxed);
        }
        
        total_accesses_.fetch_add(1, std::memory_order_relaxed);
        total_access_time_.fetch_add(result.access_latency, std::memory_order_relaxed);
        
        return result;
    }
    
    /**
     * @brief Get cache statistics
     */
    struct CacheStatistics {
        u64 total_accesses;
        u64 cache_hits;
        u64 cache_misses;
        f64 hit_ratio;
        f64 miss_ratio;
        f64 average_access_time;
        f64 cache_utilization;
        
        CacheParameters parameters;
        usize num_sets;
        usize offset_bits;
        usize index_bits;
        usize tag_bits;
    };
    
    CacheStatistics get_statistics() const {
        CacheStatistics stats{};
        
        stats.total_accesses = total_accesses_.load();
        stats.cache_hits = cache_hits_.load();
        stats.cache_misses = cache_misses_.load();
        
        if (stats.total_accesses > 0) {
            stats.hit_ratio = static_cast<f64>(stats.cache_hits) / stats.total_accesses;
            stats.miss_ratio = static_cast<f64>(stats.cache_misses) / stats.total_accesses;
            stats.average_access_time = total_access_time_.load() / stats.total_accesses;
        }
        
        // Calculate cache utilization
        usize valid_entries = 0;
        for (const auto& cache_set : cache_sets_) {
            for (const auto& entry : cache_set) {
                if (entry.valid) valid_entries++;
            }
        }
        
        usize total_entries = cache_sets_.size() * params_.associativity;
        stats.cache_utilization = static_cast<f64>(valid_entries) / total_entries;
        
        stats.parameters = params_;
        stats.num_sets = num_sets_;
        stats.offset_bits = offset_bits_;
        stats.index_bits = index_bits_;
        stats.tag_bits = tag_bits_;
        
        return stats;
    }
    
    void reset_statistics() {
        total_accesses_.store(0);
        cache_hits_.store(0);
        cache_misses_.store(0);
        total_access_time_.store(0.0);
        
        // Clear cache
        for (auto& cache_set : cache_sets_) {
            for (auto& entry : cache_set) {
                entry = CacheEntry{};
            }
        }
    }
    
private:
    f64 get_current_time() const {
        using namespace std::chrono;
        return duration<f64>(steady_clock::now().time_since_epoch()).count();
    }
};

//=============================================================================
// Allocation Strategy Simulators
//=============================================================================

/**
 * @brief Base class for allocation strategy simulation
 */
class AllocationStrategySimulator {
protected:
    std::vector<SimulatedBlock> memory_blocks_;
    usize total_memory_;
    std::atomic<u32> next_allocation_id_{1};
    
    // Performance tracking
    std::atomic<f64> total_allocation_time_{0.0};
    std::atomic<u64> allocation_count_{0};
    
public:
    explicit AllocationStrategySimulator(usize total_memory) 
        : total_memory_(total_memory) {
        
        // Initialize with one large free block
        SimulatedBlock initial_block;
        initial_block.address = reinterpret_cast<void*>(0x1000); // Simulated base address
        initial_block.size = total_memory;
        initial_block.is_allocated = false;
        initial_block.allocation_time = 0.0;
        
        memory_blocks_.push_back(initial_block);
    }
    
    virtual ~AllocationStrategySimulator() = default;
    
    /**
     * @brief Attempt allocation with this strategy
     */
    virtual SimulatedBlock* allocate(usize size, const std::string& tag = "") = 0;
    
    /**
     * @brief Deallocate block
     */
    virtual bool deallocate(SimulatedBlock* block) {
        if (!block || !block->is_allocated) {
            return false;
        }
        
        block->is_allocated = false;
        
        // Coalesce adjacent free blocks
        coalesce_free_blocks();
        
        return true;
    }
    
    /**
     * @brief Get current memory state
     */
    SimulationState get_simulation_state() const {
        SimulationState state;
        
        state.memory_blocks = memory_blocks_;
        state.total_memory_size = total_memory_;
        
        // Calculate metrics
        usize allocated = 0;
        usize free = 0;
        usize free_blocks = 0;
        usize largest_free = 0;
        
        for (const auto& block : memory_blocks_) {
            if (block.is_allocated) {
                allocated += block.size;
            } else {
                free += block.size;
                free_blocks++;
                largest_free = std::max(largest_free, block.size);
            }
        }
        
        state.allocated_memory = allocated;
        state.free_memory = free;
        
        // Calculate fragmentation
        if (free > 0 && free_blocks > 1) {
            state.fragmentation_ratio = 1.0 - (static_cast<f64>(largest_free) / free);
        }
        
        state.allocation_count = allocation_count_.load();
        if (state.allocation_count > 0) {
            state.average_allocation_time = total_allocation_time_.load() / state.allocation_count;
        }
        
        return state;
    }
    
    /**
     * @brief Get strategy name
     */
    virtual const char* get_strategy_name() const = 0;
    
    /**
     * @brief Reset simulator state
     */
    virtual void reset() {
        memory_blocks_.clear();
        
        SimulatedBlock initial_block;
        initial_block.address = reinterpret_cast<void*>(0x1000);
        initial_block.size = total_memory_;
        initial_block.is_allocated = false;
        
        memory_blocks_.push_back(initial_block);
        
        total_allocation_time_.store(0.0);
        allocation_count_.store(0);
        next_allocation_id_.store(1);
    }
    
protected:
    void coalesce_free_blocks() {
        // Sort blocks by address
        std::sort(memory_blocks_.begin(), memory_blocks_.end(),
                 [](const SimulatedBlock& a, const SimulatedBlock& b) {
                     return a.address < b.address;
                 });
        
        // Coalesce adjacent free blocks
        for (size_t i = 0; i < memory_blocks_.size() - 1; ) {
            auto& current = memory_blocks_[i];
            auto& next = memory_blocks_[i + 1];
            
            if (!current.is_allocated && !next.is_allocated) {
                // Check if blocks are adjacent
                char* current_end = static_cast<char*>(current.address) + current.size;
                if (current_end == next.address) {
                    // Merge blocks
                    current.size += next.size;
                    memory_blocks_.erase(memory_blocks_.begin() + i + 1);
                    continue; // Don't increment i, check next block
                }
            }
            
            ++i;
        }
    }
    
    f64 get_current_time() const {
        using namespace std::chrono;
        return duration<f64>(steady_clock::now().time_since_epoch()).count();
    }
};

/**
 * @brief First-fit allocation strategy simulator
 */
class FirstFitSimulator : public AllocationStrategySimulator {
public:
    explicit FirstFitSimulator(usize total_memory) 
        : AllocationStrategySimulator(total_memory) {}
    
    SimulatedBlock* allocate(usize size, const std::string& tag = "") override {
        auto start_time = get_current_time();
        
        // Find first block that fits
        for (auto& block : memory_blocks_) {
            if (!block.is_allocated && block.size >= size) {
                // Split block if necessary
                if (block.size > size) {
                    SimulatedBlock new_block;
                    new_block.address = static_cast<char*>(block.address) + size;
                    new_block.size = block.size - size;
                    new_block.is_allocated = false;
                    
                    memory_blocks_.push_back(new_block);
                    
                    block.size = size;
                }
                
                block.is_allocated = true;
                block.allocation_time = start_time;
                block.allocation_id = next_allocation_id_.fetch_add(1);
                block.strategy = AllocationStrategy::FirstFit;
                
                auto end_time = get_current_time();
                total_allocation_time_.fetch_add(end_time - start_time, std::memory_order_relaxed);
                allocation_count_.fetch_add(1, std::memory_order_relaxed);
                
                return &block;
            }
        }
        
        return nullptr; // No suitable block found
    }
    
    const char* get_strategy_name() const override {
        return "First Fit";
    }
};

/**
 * @brief Best-fit allocation strategy simulator
 */
class BestFitSimulator : public AllocationStrategySimulator {
public:
    explicit BestFitSimulator(usize total_memory) 
        : AllocationStrategySimulator(total_memory) {}
    
    SimulatedBlock* allocate(usize size, const std::string& tag = "") override {
        auto start_time = get_current_time();
        
        SimulatedBlock* best_block = nullptr;
        usize best_size = USIZE_MAX;
        
        // Find smallest block that fits
        for (auto& block : memory_blocks_) {
            if (!block.is_allocated && block.size >= size && block.size < best_size) {
                best_block = &block;
                best_size = block.size;
            }
        }
        
        if (best_block) {
            // Split block if necessary
            if (best_block->size > size) {
                SimulatedBlock new_block;
                new_block.address = static_cast<char*>(best_block->address) + size;
                new_block.size = best_block->size - size;
                new_block.is_allocated = false;
                
                memory_blocks_.push_back(new_block);
                
                best_block->size = size;
            }
            
            best_block->is_allocated = true;
            best_block->allocation_time = start_time;
            best_block->allocation_id = next_allocation_id_.fetch_add(1);
            best_block->strategy = AllocationStrategy::BestFit;
            
            auto end_time = get_current_time();
            total_allocation_time_.fetch_add(end_time - start_time, std::memory_order_relaxed);
            allocation_count_.fetch_add(1, std::memory_order_relaxed);
            
            return best_block;
        }
        
        return nullptr;
    }
    
    const char* get_strategy_name() const override {
        return "Best Fit";
    }
};

//=============================================================================
// Educational Scenario Engine
//=============================================================================

/**
 * @brief Educational scenarios for memory management learning
 */
class EducationalScenarios {
public:
    /**
     * @brief Scenario types
     */
    enum class ScenarioType {
        BasicAllocation,        // Simple allocation/deallocation
        FragmentationDemo,      // Demonstrate fragmentation
        CacheLocalityDemo,      // Cache behavior demonstration
        StrategyComparison,     // Compare allocation strategies
        LeakDetection,         // Memory leak scenarios
        RealWorldSimulation    // Real-world allocation patterns
    };
    
    /**
     * @brief Scenario configuration
     */
    struct ScenarioConfig {
        ScenarioType type;
        usize memory_size;
        u32 num_operations;
        AllocationPattern pattern;
        f64 deallocation_probability;
        usize min_allocation_size;
        usize max_allocation_size;
        std::string description;
        std::vector<std::string> learning_objectives;
        
        ScenarioConfig() noexcept 
            : type(ScenarioType::BasicAllocation),
              memory_size(1024 * 1024),      // 1MB
              num_operations(1000),
              pattern(AllocationPattern::Random),
              deallocation_probability(0.3),   // 30% chance to deallocate
              min_allocation_size(16),
              max_allocation_size(1024) {}
    };
    
    /**
     * @brief Create fragmentation demonstration scenario
     */
    static ScenarioConfig create_fragmentation_demo() {
        ScenarioConfig config;
        config.type = ScenarioType::FragmentationDemo;
        config.memory_size = 64 * 1024;  // 64KB for clear visualization
        config.num_operations = 500;
        config.pattern = AllocationPattern::Bimodal; // Mix of small and large
        config.deallocation_probability = 0.7; // High deallocation rate
        config.min_allocation_size = 32;
        config.max_allocation_size = 8192;
        config.description = "Demonstrates how different allocation patterns lead to fragmentation";
        config.learning_objectives = {
            "Understand external fragmentation",
            "See how allocation order affects fragmentation",
            "Learn about coalescing strategies",
            "Compare fragmentation between strategies"
        };
        return config;
    }
    
    /**
     * @brief Create cache locality demonstration
     */
    static ScenarioConfig create_cache_locality_demo() {
        ScenarioConfig config;
        config.type = ScenarioType::CacheLocalityDemo;
        config.memory_size = 256 * 1024; // 256KB
        config.num_operations = 2000;
        config.pattern = AllocationPattern::Sequential;
        config.deallocation_probability = 0.1; // Low deallocation
        config.min_allocation_size = 64;
        config.max_allocation_size = 512;
        config.description = "Shows the impact of memory layout on cache performance";
        config.learning_objectives = {
            "Understand spatial locality",
            "See cache miss patterns",
            "Learn about memory access optimization",
            "Compare sequential vs random access"
        };
        return config;
    }
    
    /**
     * @brief Create strategy comparison scenario
     */
    static ScenarioConfig create_strategy_comparison() {
        ScenarioConfig config;
        config.type = ScenarioType::StrategyComparison;
        config.memory_size = 128 * 1024; // 128KB
        config.num_operations = 800;
        config.pattern = AllocationPattern::RealWorld;
        config.deallocation_probability = 0.4; // Moderate deallocation
        config.min_allocation_size = 16;
        config.max_allocation_size = 2048;
        config.description = "Compares performance of different allocation strategies";
        config.learning_objectives = {
            "Compare first-fit vs best-fit vs worst-fit",
            "Understand allocation speed vs fragmentation trade-offs",
            "See strategy-specific fragmentation patterns",
            "Learn when to use each strategy"
        };
        return config;
    }
    
    /**
     * @brief Get all available scenarios
     */
    static std::vector<ScenarioConfig> get_all_scenarios() {
        return {
            create_fragmentation_demo(),
            create_cache_locality_demo(),
            create_strategy_comparison()
        };
    }
};

//=============================================================================
// Memory Allocation Simulator - Main Engine
//=============================================================================

/**
 * @brief Comprehensive memory allocation simulator with educational features
 */
class MemoryAllocationSimulator {
private:
    // Simulation components
    std::vector<std::unique_ptr<AllocationStrategySimulator>> strategy_simulators_;
    std::unique_ptr<CacheSimulator> cache_simulator_;
    
    // Current simulation state
    std::atomic<bool> simulation_running_{false};
    std::atomic<f64> simulation_speed_{1.0}; // 1.0 = real-time
    EducationalScenarios::ScenarioConfig current_scenario_;
    
    // Visualization data
    struct VisualizationFrame {
        f64 timestamp;
        SimulationState memory_state;
        CacheSimulator::CacheStatistics cache_stats;
        std::string event_description;
        
        VisualizationFrame() : timestamp(0.0) {}
    };
    
    std::vector<VisualizationFrame> visualization_frames_;
    mutable std::mutex frames_mutex_;
    static constexpr usize MAX_FRAMES = 10000;
    
    // Statistics and analysis
    struct ComparisonResult {
        AllocationStrategy strategy;
        std::string strategy_name;
        f64 average_fragmentation;
        f64 average_allocation_time;
        f64 allocation_success_rate;
        f64 cache_hit_ratio;
        f64 overall_performance_score;
    };
    
    std::vector<ComparisonResult> comparison_results_;
    
    // Random number generation
    std::mt19937 random_engine_;
    
public:
    MemoryAllocationSimulator() 
        : random_engine_(std::chrono::steady_clock::now().time_since_epoch().count()) {
        
        initialize_simulators();
        
        LOG_INFO("Memory allocation simulator initialized with {} allocation strategies", 
                 strategy_simulators_.size());
    }
    
    /**
     * @brief Run educational scenario simulation
     */
    void run_scenario(const EducationalScenarios::ScenarioConfig& config) {
        current_scenario_ = config;
        simulation_running_.store(true);
        
        LOG_INFO("Starting scenario: {}", config.description);
        
        // Clear previous results
        comparison_results_.clear();
        {
            std::lock_guard<std::mutex> lock(frames_mutex_);
            visualization_frames_.clear();
        }
        
        // Reset all simulators
        for (auto& simulator : strategy_simulators_) {
            simulator->reset();
        }
        
        if (cache_simulator_) {
            cache_simulator_->reset_statistics();
        }
        
        // Run simulation for each strategy
        for (auto& simulator : strategy_simulators_) {
            run_single_strategy_simulation(*simulator, config);
        }
        
        // Analyze results
        analyze_simulation_results();
        
        simulation_running_.store(false);
        
        LOG_INFO("Scenario completed. Results available for analysis.");
    }
    
    /**
     * @brief Get simulation results for analysis
     */
    struct SimulationResults {
        EducationalScenarios::ScenarioConfig scenario_config;
        std::vector<ComparisonResult> strategy_comparisons;
        std::vector<VisualizationFrame> visualization_data;
        
        // Educational insights
        std::string performance_summary;
        std::vector<std::string> key_insights;
        std::vector<std::string> optimization_suggestions;
        std::string best_strategy_for_scenario;
        
        // Metrics
        f64 overall_fragmentation_range;
        f64 performance_variation;
        bool fragmentation_critical;
        bool cache_performance_significant;
    };
    
    SimulationResults get_results() const {
        SimulationResults results;
        results.scenario_config = current_scenario_;
        results.strategy_comparisons = comparison_results_;
        
        {
            std::lock_guard<std::mutex> lock(frames_mutex_);
            results.visualization_data = visualization_frames_;
        }
        
        // Generate educational analysis
        generate_educational_analysis(results);
        
        return results;
    }
    
    /**
     * @brief Run real-time interactive simulation
     */
    void start_interactive_simulation(usize memory_size = 1024 * 1024) {
        if (simulation_running_.load()) {
            LOG_WARNING("Simulation already running");
            return;
        }
        
        simulation_running_.store(true);
        
        // Initialize with a simple first-fit simulator for interactive use
        auto& simulator = *strategy_simulators_[0]; // First-fit
        simulator.reset();
        
        LOG_INFO("Interactive simulation started. Use allocate/deallocate methods to interact.");
    }
    
    /**
     * @brief Interactive allocation
     */
    void* interactive_allocate(usize size, const std::string& tag = "") {
        if (!simulation_running_.load()) {
            LOG_WARNING("No interactive simulation running");
            return nullptr;
        }
        
        auto& simulator = *strategy_simulators_[0];
        auto* block = simulator.allocate(size, tag);
        
        if (block) {
            // Record visualization frame
            record_visualization_frame("Interactive allocation: " + std::to_string(size) + " bytes");
            
            // Simulate cache access
            if (cache_simulator_) {
                cache_simulator_->simulate_access(reinterpret_cast<uintptr_t>(block->address));
            }
        }
        
        return block ? block->address : nullptr;
    }
    
    /**
     * @brief Interactive deallocation
     */
    bool interactive_deallocate(void* address) {
        if (!simulation_running_.load()) {
            return false;
        }
        
        auto& simulator = *strategy_simulators_[0];
        auto state = simulator.get_simulation_state();
        
        // Find block with matching address
        for (auto& block : state.memory_blocks) {
            if (block.address == address && block.is_allocated) {
                SimulatedBlock* sim_block = &block; // This is a bit hacky but works for demo
                bool success = simulator.deallocate(sim_block);
                
                if (success) {
                    record_visualization_frame("Interactive deallocation");
                }
                
                return success;
            }
        }
        
        return false;
    }
    
    /**
     * @brief Stop interactive simulation
     */
    void stop_interactive_simulation() {
        simulation_running_.store(false);
        LOG_INFO("Interactive simulation stopped");
    }
    
    /**
     * @brief Get current simulation state for visualization
     */
    SimulationState get_current_state() const {
        if (!strategy_simulators_.empty()) {
            return strategy_simulators_[0]->get_simulation_state();
        }
        return SimulationState{};
    }
    
    /**
     * @brief Export simulation data for external analysis
     */
    void export_simulation_data(const std::string& filename) const {
        std::ofstream file(filename);
        if (!file.is_open()) {
            LOG_ERROR("Failed to open export file: {}", filename);
            return;
        }
        
        auto results = get_results();
        
        file << "Memory Allocation Simulation Results\n";
        file << "====================================\n\n";
        
        file << "Scenario: " << results.scenario_config.description << "\n";
        file << "Memory Size: " << results.scenario_config.memory_size / 1024 << "KB\n";
        file << "Operations: " << results.scenario_config.num_operations << "\n\n";
        
        file << "Strategy Comparison Results:\n";
        for (const auto& result : results.strategy_comparisons) {
            file << "  " << result.strategy_name << ":\n";
            file << "    Fragmentation: " << (result.average_fragmentation * 100) << "%\n";
            file << "    Allocation Time: " << (result.average_allocation_time * 1000) << "ms\n";
            file << "    Success Rate: " << (result.allocation_success_rate * 100) << "%\n";
            file << "    Performance Score: " << result.overall_performance_score << "\n\n";
        }
        
        file << "Educational Insights:\n";
        for (const auto& insight : results.key_insights) {
            file << "  - " << insight << "\n";
        }
        
        file.close();
        LOG_INFO("Simulation data exported to: {}", filename);
    }
    
    /**
     * @brief Configuration
     */
    void set_simulation_speed(f64 speed_multiplier) {
        simulation_speed_.store(speed_multiplier);
    }
    
    bool is_simulation_running() const {
        return simulation_running_.load();
    }
    
private:
    void initialize_simulators() {
        // Initialize allocation strategy simulators
        const usize default_memory_size = 1024 * 1024; // 1MB
        
        strategy_simulators_.push_back(std::make_unique<FirstFitSimulator>(default_memory_size));
        strategy_simulators_.push_back(std::make_unique<BestFitSimulator>(default_memory_size));
        
        // Initialize cache simulator
        cache_simulator_ = std::make_unique<CacheSimulator>();
    }
    
    void run_single_strategy_simulation(AllocationStrategySimulator& simulator,
                                       const EducationalScenarios::ScenarioConfig& config) {
        LOG_DEBUG("Running simulation for strategy: {}", simulator.get_strategy_name());
        
        // Reset simulator with correct memory size
        simulator = *std::make_unique<FirstFitSimulator>(config.memory_size); // Simplified
        
        std::vector<SimulatedBlock*> allocated_blocks;
        
        for (u32 op = 0; op < config.num_operations; ++op) {
            // Decide whether to allocate or deallocate
            std::uniform_real_distribution<f64> prob_dist(0.0, 1.0);
            bool should_deallocate = !allocated_blocks.empty() && 
                                   prob_dist(random_engine_) < config.deallocation_probability;
            
            if (should_deallocate) {
                // Deallocate a random allocated block
                std::uniform_int_distribution<usize> block_dist(0, allocated_blocks.size() - 1);
                usize block_index = block_dist(random_engine_);
                
                simulator.deallocate(allocated_blocks[block_index]);
                allocated_blocks.erase(allocated_blocks.begin() + block_index);
            } else {
                // Allocate new block
                usize size = generate_allocation_size(config);
                auto* block = simulator.allocate(size);
                
                if (block) {
                    allocated_blocks.push_back(block);
                    
                    // Simulate cache access
                    if (cache_simulator_) {
                        cache_simulator_->simulate_access(reinterpret_cast<uintptr_t>(block->address));
                    }
                }
            }
            
            // Record periodic visualization frames
            if (op % 50 == 0) {
                std::string description = std::string(simulator.get_strategy_name()) + 
                                        " - Operation " + std::to_string(op);
                record_visualization_frame(description);
            }
        }
        
        // Cleanup remaining allocations
        for (auto* block : allocated_blocks) {
            simulator.deallocate(block);
        }
    }
    
    usize generate_allocation_size(const EducationalScenarios::ScenarioConfig& config) {
        std::uniform_int_distribution<usize> size_dist(config.min_allocation_size, config.max_allocation_size);
        
        switch (config.pattern) {
            case AllocationPattern::Random:
                return size_dist(random_engine_);
                
            case AllocationPattern::PowerOfTwo: {
                usize power = std::uniform_int_distribution<usize>(4, 12)(random_engine_); // 16 to 4096
                return 1UL << power;
            }
            
            case AllocationPattern::Bimodal: {
                bool use_small = std::uniform_real_distribution<f64>(0.0, 1.0)(random_engine_) < 0.7;
                if (use_small) {
                    return std::uniform_int_distribution<usize>(16, 128)(random_engine_);
                } else {
                    return std::uniform_int_distribution<usize>(1024, 4096)(random_engine_);
                }
            }
            
            default:
                return size_dist(random_engine_);
        }
    }
    
    void record_visualization_frame(const std::string& description) {
        VisualizationFrame frame;
        frame.timestamp = get_current_time();
        frame.event_description = description;
        
        if (!strategy_simulators_.empty()) {
            frame.memory_state = strategy_simulators_[0]->get_simulation_state();
        }
        
        if (cache_simulator_) {
            frame.cache_stats = cache_simulator_->get_statistics();
        }
        
        std::lock_guard<std::mutex> lock(frames_mutex_);
        visualization_frames_.push_back(frame);
        
        if (visualization_frames_.size() > MAX_FRAMES) {
            visualization_frames_.erase(visualization_frames_.begin());
        }
    }
    
    void analyze_simulation_results() {
        comparison_results_.clear();
        
        for (const auto& simulator : strategy_simulators_) {
            ComparisonResult result;
            result.strategy = AllocationStrategy::FirstFit; // Simplified
            result.strategy_name = simulator->get_strategy_name();
            
            auto state = simulator->get_simulation_state();
            result.average_fragmentation = state.fragmentation_ratio;
            result.average_allocation_time = state.average_allocation_time;
            
            if (state.total_allocations > 0) {
                result.allocation_success_rate = 
                    static_cast<f64>(state.successful_allocations) / state.total_allocations;
            }
            
            if (cache_simulator_) {
                auto cache_stats = cache_simulator_->get_statistics();
                result.cache_hit_ratio = cache_stats.hit_ratio;
            }
            
            // Calculate overall performance score (0-100)
            result.overall_performance_score = 
                (result.allocation_success_rate * 40) +
                ((1.0 - result.average_fragmentation) * 30) +
                (result.cache_hit_ratio * 20) +
                ((1.0 - std::min(1.0, result.average_allocation_time * 1000)) * 10);
            
            comparison_results_.push_back(result);
        }
        
        // Sort by performance score
        std::sort(comparison_results_.begin(), comparison_results_.end(),
                 [](const ComparisonResult& a, const ComparisonResult& b) {
                     return a.overall_performance_score > b.overall_performance_score;
                 });
    }
    
    void generate_educational_analysis(SimulationResults& results) const {
        // Generate performance summary
        if (!results.strategy_comparisons.empty()) {
            const auto& best = results.strategy_comparisons[0];
            results.best_strategy_for_scenario = best.strategy_name;
            results.performance_summary = 
                "Best performing strategy: " + best.strategy_name + 
                " (Score: " + std::to_string(best.overall_performance_score) + ")";
        }
        
        // Calculate performance variation
        if (results.strategy_comparisons.size() > 1) {
            f64 min_score = results.strategy_comparisons.back().overall_performance_score;
            f64 max_score = results.strategy_comparisons.front().overall_performance_score;
            results.performance_variation = max_score - min_score;
        }
        
        // Generate key insights
        results.key_insights.push_back("Different allocation strategies show significant performance differences");
        results.key_insights.push_back("Fragmentation has major impact on allocation success");
        results.key_insights.push_back("Cache performance correlates with memory layout strategy");
        
        // Generate optimization suggestions
        results.optimization_suggestions.push_back("Use best-fit for memory-constrained scenarios");
        results.optimization_suggestions.push_back("Consider first-fit for speed-critical applications");
        results.optimization_suggestions.push_back("Monitor fragmentation levels in production systems");
        
        // Set flags
        results.fragmentation_critical = 
            !results.strategy_comparisons.empty() && 
            results.strategy_comparisons[0].average_fragmentation > 0.3;
        
        results.cache_performance_significant = 
            !results.strategy_comparisons.empty() && 
            results.strategy_comparisons[0].cache_hit_ratio < 0.8;
    }
    
    f64 get_current_time() const {
        using namespace std::chrono;
        return duration<f64>(steady_clock::now().time_since_epoch()).count();
    }
};

//=============================================================================
// Global Memory Simulator Instance
//=============================================================================

MemoryAllocationSimulator& get_global_memory_simulator();

} // namespace ecscope::memory::education