#pragma once

/**
 * @file panel_cache_visualizer.hpp
 * @brief Cache Pattern Visualization Panel for ECScope Educational ECS Engine
 * 
 * This educational panel provides comprehensive cache access pattern visualization and analysis
 * to help developers understand memory locality, cache behavior, and optimization opportunities
 * in their ECS systems. It integrates with the MemoryTracker system to provide real-time
 * insights into cache performance.
 * 
 * Educational Features:
 * - Real-time cache access pattern heat maps with detailed locality analysis
 * - Memory access timeline visualization showing cache hits/misses over time
 * - Cache line utilization graphs with performance impact metrics
 * - Interactive memory locality analysis with visual pattern recognition
 * - Cache hierarchy visualization (L1, L2, L3) with educational explanations
 * - Access pattern classification (sequential, random, strided) with examples
 * - Cache-friendly vs cache-hostile pattern detection and recommendations
 * - Performance impact visualization of different access patterns
 * - Memory bandwidth utilization analysis with optimization suggestions
 * - Allocation clustering analysis for spatial locality optimization
 * - Cache miss prediction based on access patterns with accuracy metrics
 * - Educational tooltips explaining cache concepts and terminology
 * 
 * Interactive Features:
 * - Zoom and pan functionality for detailed memory region analysis
 * - Multi-level filtering by allocator type (Arena, Pool, Standard)
 * - Time-based filtering for temporal pattern analysis over different periods
 * - Category filtering by memory usage type with performance correlation
 * - Real-time parameter adjustment for cache simulation accuracy
 * - Pattern overlay controls for comparing different access strategies
 * 
 * Analysis Tools:
 * - Advanced cache miss prediction algorithms with confidence intervals
 * - Memory bandwidth utilization graphs with bottleneck identification
 * - Allocation clustering analysis for spatial locality optimization
 * - Performance recommendations based on detected patterns
 * - Cache-aware allocation strategy suggestions with expected improvements
 * - Memory access efficiency scoring with detailed breakdowns
 * 
 * Performance Characteristics:
 * - Minimal impact on memory operations (< 2% overhead when enabled)
 * - Efficient data collection with smart sampling and aggregation
 * - Real-time updates with configurable refresh rates
 * - Thread-safe visualization with lock-free data access where possible
 * 
 * @author ECScope Educational ECS Framework  
 * @date 2024
 */

#include "../overlay.hpp"
#include "core/types.hpp"
#include "memory/memory_tracker.hpp"
#include <vector>
#include <string>
#include <array>
#include <unordered_map>
#include <deque>
#include <memory>
#include <chrono>

namespace ecscope::ui {

/**
 * @brief Cache simulation parameters for educational purposes
 */
struct CacheConfig {
    // L1 Cache parameters
    usize l1_size = 32 * 1024;          // 32KB typical L1 cache
    usize l1_line_size = 64;            // 64 bytes per cache line
    u32 l1_associativity = 8;           // 8-way associative
    u32 l1_latency = 4;                 // 4 cycles access time
    
    // L2 Cache parameters  
    usize l2_size = 256 * 1024;         // 256KB typical L2 cache
    usize l2_line_size = 64;            // 64 bytes per cache line
    u32 l2_associativity = 8;           // 8-way associative
    u32 l2_latency = 12;                // 12 cycles access time
    
    // L3 Cache parameters
    usize l3_size = 8 * 1024 * 1024;    // 8MB typical L3 cache
    usize l3_line_size = 64;            // 64 bytes per cache line
    u32 l3_associativity = 16;          // 16-way associative
    u32 l3_latency = 42;                // 42 cycles access time
    
    // Memory parameters
    u32 memory_latency = 300;           // 300 cycles for main memory
    f64 memory_bandwidth = 25.6;        // 25.6 GB/s typical DDR4
    
    // Prefetcher simulation
    bool enable_prefetcher = true;      // Hardware prefetcher enabled
    u32 prefetch_degree = 2;            // Number of cache lines to prefetch
    f64 prefetch_accuracy = 0.85;       // 85% prefetch accuracy
};

/**
 * @brief Cache access event for timeline visualization
 */
struct CacheAccessEvent {
    f64 timestamp;                      // When the access occurred
    void* address;                      // Memory address accessed
    usize size;                         // Size of access
    bool is_write;                      // Read or write access
    bool l1_hit;                        // L1 cache hit
    bool l2_hit;                        // L2 cache hit  
    bool l3_hit;                        // L3 cache hit
    u32 access_latency;                 // Cycles to complete access
    memory::AllocationCategory category; // Type of data accessed
    memory::AccessPattern pattern;       // Detected access pattern
    
    CacheAccessEvent() noexcept
        : timestamp(0.0), address(nullptr), size(0), is_write(false),
          l1_hit(false), l2_hit(false), l3_hit(false), access_latency(0),
          category(memory::AllocationCategory::Unknown),
          pattern(memory::AccessPattern::Unknown) {}
};

/**
 * @brief Cache line state for visualization
 */
struct CacheLineInfo {
    void* tag;                          // Cache line tag (address)
    bool valid;                         // Valid bit
    bool dirty;                         // Dirty bit (for write-back)
    u32 access_count;                   // Number of accesses
    f64 last_access_time;               // Last access timestamp
    f64 temperature;                    // Heat value for visualization
    memory::AllocationCategory category; // Data category in this line
    
    CacheLineInfo() noexcept
        : tag(nullptr), valid(false), dirty(false), access_count(0),
          last_access_time(0.0), temperature(0.0),
          category(memory::AllocationCategory::Unknown) {}
};

/**
 * @brief Cache level statistics for educational display
 */
struct CacheLevelStats {
    // Hit/Miss statistics
    u64 total_accesses;                 // Total access count
    u64 hits;                           // Cache hits
    u64 misses;                         // Cache misses
    f64 hit_rate;                       // Hit rate percentage
    f64 miss_rate;                      // Miss rate percentage
    
    // Performance metrics
    f64 average_latency;                // Average access latency
    u64 total_cycles;                   // Total cycles spent
    f64 bandwidth_usage;                // Bandwidth utilization
    
    // Utilization metrics
    u32 lines_used;                     // Number of cache lines in use
    u32 total_lines;                    // Total cache lines
    f64 utilization_rate;               // Cache utilization percentage
    
    // Pattern analysis
    std::array<u64, static_cast<usize>(memory::AccessPattern::Hash) + 1> pattern_counts;
    
    CacheLevelStats() noexcept { reset(); }
    
    void reset() noexcept {
        total_accesses = hits = misses = 0;
        hit_rate = miss_rate = 0.0;
        average_latency = total_cycles = bandwidth_usage = 0.0;
        lines_used = total_lines = 0;
        utilization_rate = 0.0;
        pattern_counts.fill(0);
    }
    
    void update_rates() noexcept {
        if (total_accesses > 0) {
            hit_rate = (static_cast<f64>(hits) / total_accesses) * 100.0;
            miss_rate = (static_cast<f64>(misses) / total_accesses) * 100.0;
        }
        if (total_lines > 0) {
            utilization_rate = (static_cast<f64>(lines_used) / total_lines) * 100.0;
        }
        if (total_accesses > 0) {
            average_latency = static_cast<f64>(total_cycles) / total_accesses;
        }
    }
};

/**
 * @brief Memory access pattern analysis results
 */
struct PatternAnalysis {
    memory::AccessPattern dominant_pattern; // Most common pattern
    f64 pattern_confidence;             // Confidence in pattern detection
    f64 spatial_locality_score;         // Spatial locality rating (0.0-1.0)
    f64 temporal_locality_score;        // Temporal locality rating (0.0-1.0)
    f64 cache_friendliness_score;       // Overall cache friendliness (0.0-1.0)
    
    // Pattern breakdown
    std::unordered_map<memory::AccessPattern, f64> pattern_percentages;
    
    // Recommendations
    std::vector<std::string> recommendations;
    
    // Predicted performance impact
    f64 predicted_cache_miss_rate;      // Predicted miss rate
    f64 predicted_bandwidth_usage;      // Predicted bandwidth usage
    f64 performance_score;              // Overall performance score (0.0-100.0)
    
    PatternAnalysis() noexcept
        : dominant_pattern(memory::AccessPattern::Unknown),
          pattern_confidence(0.0), spatial_locality_score(0.0),
          temporal_locality_score(0.0), cache_friendliness_score(0.0),
          predicted_cache_miss_rate(0.0), predicted_bandwidth_usage(0.0),
          performance_score(0.0) {}
};

/**
 * @brief Memory heat map visualization data
 */
struct MemoryHeatMapData {
    struct HeatCell {
        void* start_address;            // Start of memory region
        usize size;                     // Size of region
        f64 temperature;                // Heat value (0.0 = cold, 1.0 = hot)
        u32 access_count;               // Number of recent accesses
        f64 last_access_time;           // Most recent access
        memory::AllocationCategory category; // Data type
        bool is_cache_line_aligned;     // Cache line alignment
        f64 cache_efficiency;           // Cache usage efficiency
    };
    
    std::vector<HeatCell> cells;
    f64 max_temperature;                // Maximum temperature for scaling
    f64 cooling_rate;                   // How fast cells cool down
    usize grid_width;                   // Heat map grid dimensions
    usize grid_height;
    
    MemoryHeatMapData() noexcept
        : max_temperature(1.0), cooling_rate(0.95),
          grid_width(64), grid_height(64) {}
    
    void update_temperatures(f64 delta_time) noexcept;
    void add_access(void* address, usize size) noexcept;
    std::vector<HeatCell> get_hot_regions(f64 min_temp = 0.3) const noexcept;
};

/**
 * @brief Cache Pattern Visualizer Panel - Main UI component
 * 
 * This comprehensive panel provides educational visualization of cache access patterns,
 * memory locality analysis, and performance optimization recommendations. It integrates
 * seamlessly with the MemoryTracker system to provide real-time insights.
 */
class CacheVisualizerPanel : public Panel {
private:
    // Configuration
    CacheConfig cache_config_;
    
    // Event tracking
    static constexpr usize MAX_EVENTS = 10000;  // Maximum events to track
    std::deque<CacheAccessEvent> access_events_;
    mutable std::mutex events_mutex_;
    
    // Cache simulation state
    std::array<CacheLevelStats, 3> cache_stats_;  // L1, L2, L3 stats
    std::vector<std::array<CacheLineInfo, 512>> cache_lines_; // Simulated cache lines
    
    // Heat map visualization
    std::unique_ptr<MemoryHeatMapData> heat_map_data_;
    
    // Pattern analysis
    std::unique_ptr<PatternAnalysis> current_analysis_;
    f64 analysis_update_timer_;
    f64 analysis_update_frequency_;     // How often to update analysis
    
    // Display settings
    bool show_heat_map_;                // Show memory heat map
    bool show_timeline_;                // Show access timeline
    bool show_cache_stats_;             // Show cache statistics
    bool show_pattern_analysis_;        // Show pattern analysis
    bool show_recommendations_;         // Show optimization recommendations
    bool show_cache_hierarchy_;         // Show cache hierarchy visualization
    bool show_bandwidth_graph_;         // Show bandwidth utilization
    
    // Timeline visualization
    static constexpr usize TIMELINE_POINTS = 1000;
    std::array<f32, TIMELINE_POINTS> timeline_l1_hits_;
    std::array<f32, TIMELINE_POINTS> timeline_l2_hits_;
    std::array<f32, TIMELINE_POINTS> timeline_l3_hits_;
    std::array<f32, TIMELINE_POINTS> timeline_memory_accesses_;
    usize timeline_head_;
    
    // Filtering options
    bool filter_by_category_;
    std::array<bool, static_cast<usize>(memory::AllocationCategory::COUNT)> category_filters_;
    bool filter_by_allocator_;
    std::array<bool, static_cast<usize>(memory::AllocatorType::Custom) + 1> allocator_filters_;
    bool filter_by_pattern_;
    std::array<bool, static_cast<usize>(memory::AccessPattern::Hash) + 1> pattern_filters_;
    
    // Time range filtering
    f64 time_range_start_;
    f64 time_range_end_;
    bool enable_time_filtering_;
    
    // Interactive controls
    f32 heat_map_zoom_;                 // Zoom level for heat map
    f32 heat_map_pan_x_;                // Pan position X
    f32 heat_map_pan_y_;                // Pan position Y
    bool heat_map_auto_zoom_;           // Auto-zoom to interesting regions
    
    // Educational tooltips
    bool show_educational_tooltips_;
    std::unordered_map<std::string, std::string> educational_content_;
    
    // Performance monitoring
    f64 update_timer_;
    f64 update_frequency_;              // Update frequency in Hz
    f64 last_update_time_;
    
    // Export capabilities
    std::string export_path_;
    bool enable_data_export_;
    
public:
    /**
     * @brief Construct Cache Visualizer Panel
     */
    CacheVisualizerPanel();
    
    /**
     * @brief Destructor
     */
    ~CacheVisualizerPanel() override = default;
    
    // Panel interface
    void render() override;
    void update(f64 delta_time) override;
    
    // Configuration
    void set_cache_config(const CacheConfig& config) noexcept;
    CacheConfig get_cache_config() const noexcept;
    
    void set_update_frequency(f64 frequency) noexcept;
    f64 update_frequency() const noexcept { return update_frequency_; }
    
    // Data collection interface (called by memory system)
    void record_memory_access(void* address, usize size, bool is_write,
                             memory::AllocationCategory category,
                             memory::AccessPattern pattern) noexcept;
    
    // Analysis results
    PatternAnalysis get_current_analysis() const noexcept;
    std::array<CacheLevelStats, 3> get_cache_statistics() const noexcept;
    MemoryHeatMapData get_heat_map_data() const noexcept;
    
    // Export capabilities
    void export_cache_data(const std::string& filename) const noexcept;
    void export_heat_map_image(const std::string& filename) const noexcept;
    void export_access_patterns_csv(const std::string& filename) const noexcept;
    
    // Utility functions
    void reset_cache_simulation() noexcept;
    void clear_access_history() noexcept;
    
private:
    // Rendering functions
    void render_main_controls();
    void render_cache_hierarchy();
    void render_cache_statistics();
    void render_memory_heat_map();
    void render_access_timeline();
    void render_pattern_analysis();
    void render_bandwidth_visualization();
    void render_optimization_recommendations();
    void render_educational_tooltips();
    void render_filtering_controls();
    void render_export_options();
    
    // Cache simulation
    void simulate_memory_access(const CacheAccessEvent& event) noexcept;
    bool simulate_cache_level(usize level, void* address, usize size) noexcept;
    void update_cache_line(usize level, usize set_index, usize way_index,
                          void* tag, memory::AllocationCategory category) noexcept;
    
    // Analysis functions
    void update_pattern_analysis() noexcept;
    void analyze_spatial_locality() noexcept;
    void analyze_temporal_locality() noexcept;
    void generate_recommendations() noexcept;
    
    // Heat map management
    void update_heat_map(f64 delta_time) noexcept;
    void add_heat_map_access(void* address, usize size) noexcept;
    
    // Timeline management
    void update_timeline() noexcept;
    void add_timeline_point(bool l1_hit, bool l2_hit, bool l3_hit) noexcept;
    
    // Filtering
    bool should_include_event(const CacheAccessEvent& event) const noexcept;
    void apply_category_filter(memory::AllocationCategory category, bool enabled) noexcept;
    void apply_time_range_filter(f64 start_time, f64 end_time) noexcept;
    
    // Educational content
    void initialize_educational_content() noexcept;
    void show_cache_concept_tooltip(const char* concept) const noexcept;
    
    // Utility functions
    std::string format_cache_size(usize bytes) const noexcept;
    std::string format_latency(u32 cycles) const noexcept;
    std::string format_bandwidth(f64 gb_per_sec) const noexcept;
    std::string format_percentage(f64 value) const noexcept;
    std::string get_pattern_description(memory::AccessPattern pattern) const noexcept;
    std::string get_category_name(memory::AllocationCategory category) const noexcept;
    
    // Color schemes for visualization
    ImVec4 get_heat_color(f64 temperature) const noexcept;
    ImVec4 get_cache_level_color(usize level) const noexcept;
    ImVec4 get_pattern_color(memory::AccessPattern pattern) const noexcept;
    ImVec4 get_category_color(memory::AllocationCategory category) const noexcept;
    
    // Cache calculation utilities
    usize get_cache_line_index(void* address, usize cache_size, usize line_size) const noexcept;
    usize get_cache_set_index(void* address, usize cache_size, usize line_size, u32 associativity) const noexcept;
    void* get_cache_tag(void* address, usize line_size) const noexcept;
    
    // Performance monitoring
    f64 calculate_cache_efficiency() const noexcept;
    f64 calculate_memory_bandwidth_usage() const noexcept;
    f64 predict_performance_impact(const PatternAnalysis& analysis) const noexcept;
    
    // Graph rendering utilities
    void render_hit_rate_graph();
    void render_latency_distribution();
    void render_bandwidth_usage_graph();
    void render_cache_utilization_bars();
    
    // Interactive features
    void handle_heat_map_interaction();
    void handle_timeline_zoom();
    void handle_cache_hierarchy_click(usize level);
};

// Global integration with memory tracker
namespace cache_visualization {
    /**
     * @brief Initialize cache visualization system
     */
    void initialize() noexcept;
    
    /**
     * @brief Shutdown cache visualization system  
     */
    void shutdown() noexcept;
    
    /**
     * @brief Get the global cache visualizer panel
     */
    CacheVisualizerPanel* get_panel() noexcept;
    
    /**
     * @brief Record a memory access for visualization (called by memory tracker)
     */
    void record_access(void* address, usize size, bool is_write,
                      memory::AllocationCategory category,
                      memory::AccessPattern pattern) noexcept;
    
    /**
     * @brief Enable/disable cache visualization
     */
    void set_enabled(bool enabled) noexcept;
    bool is_enabled() noexcept;
}

} // namespace ecscope::ui