#pragma once

/**
 * @file archetype_analyzer.hpp
 * @brief Advanced Archetype Relationship Analysis for Visual ECS Inspector
 * 
 * This module provides sophisticated analysis capabilities for ECS archetypes,
 * focusing on relationships, transitions, memory patterns, and performance
 * characteristics. It integrates with the Visual ECS Inspector to provide
 * educational insights into archetype-based ECS architecture.
 * 
 * Key Features:
 * - Archetype relationship mapping and transition analysis
 * - Component correlation analysis for optimization suggestions
 * - Memory layout efficiency analysis for cache performance
 * - Entity lifecycle tracking across archetype transitions
 * - Performance hotspot identification in archetype operations
 * - Predictive analysis for archetype scalability
 * 
 * Educational Focus:
 * - Understanding archetype-based entity organization
 * - Memory layout implications of component combinations
 * - Performance characteristics of different archetype structures
 * - Optimization strategies for component design
 * - Cache-friendly archetype layouts
 * 
 * @author ECScope Educational ECS Framework
 * @date 2024
 */

#include "core/types.hpp"
#include "ecs/archetype.hpp"
#include "ecs/component.hpp"
#include "memory/memory_tracker.hpp"
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <string>
#include <chrono>
#include <functional>

namespace ecscope::analysis {

/**
 * @brief Archetype transition information
 */
struct ArchetypeTransition {
    u32 from_archetype_id;                  // Source archetype ID
    u32 to_archetype_id;                    // Target archetype ID
    std::vector<core::ComponentID> added_components;    // Components added
    std::vector<core::ComponentID> removed_components;  // Components removed
    u64 transition_count;                   // Number of transitions
    f64 average_transition_time;            // Average time per transition
    f64 last_transition_time;               // Last recorded transition
    f64 transition_frequency;               // Transitions per second
    
    ArchetypeTransition() noexcept
        : from_archetype_id(0), to_archetype_id(0), transition_count(0)
        , average_transition_time(0.0), last_transition_time(0.0)
        , transition_frequency(0.0) {}
};

/**
 * @brief Component correlation data for optimization analysis
 */
struct ComponentCorrelation {
    core::ComponentID component_a;          // First component
    core::ComponentID component_b;          // Second component
    f64 correlation_strength;               // How often they appear together (0.0-1.0)
    u64 cooccurrence_count;                 // Times they appeared together
    u64 total_appearances_a;                // Total appearances of component A
    u64 total_appearances_b;                // Total appearances of component B
    f64 performance_impact;                 // Performance impact of combination
    bool is_beneficial_pairing;             // Whether pairing improves performance
    
    ComponentCorrelation() noexcept
        : component_a(0), component_b(0), correlation_strength(0.0)
        , cooccurrence_count(0), total_appearances_a(0), total_appearances_b(0)
        , performance_impact(0.0), is_beneficial_pairing(false) {}
    
    f64 calculate_correlation() const noexcept {
        if (total_appearances_a == 0 || total_appearances_b == 0) return 0.0;
        
        // Calculate correlation coefficient
        f64 expected = (static_cast<f64>(total_appearances_a) * total_appearances_b) / 
                      (total_appearances_a + total_appearances_b);
        
        return static_cast<f64>(cooccurrence_count) / std::max(expected, 1.0);
    }
};

/**
 * @brief Archetype memory layout analysis
 */
struct ArchetypeMemoryLayout {
    u32 archetype_id;                       // Archetype identifier
    usize total_memory_usage;               // Total memory used by archetype
    usize component_memory_usage;           // Memory used by component data
    usize metadata_memory_usage;            // Memory used by metadata
    usize alignment_waste;                  // Bytes wasted due to alignment
    f64 cache_efficiency_score;             // Cache efficiency rating (0.0-1.0)
    f64 spatial_locality_score;             // Spatial locality rating (0.0-1.0)
    f64 memory_fragmentation;               // Memory fragmentation level
    
    // Component-specific memory data
    struct ComponentMemoryInfo {
        core::ComponentID component_id;     // Component identifier
        usize size;                         // Component size in bytes
        usize alignment;                    // Required alignment
        usize count;                        // Number of instances
        f64 access_frequency;               // Access frequency
        memory::AccessPattern access_pattern; // Detected access pattern
    };
    
    std::vector<ComponentMemoryInfo> components;
    
    ArchetypeMemoryLayout() noexcept
        : archetype_id(0), total_memory_usage(0), component_memory_usage(0)
        , metadata_memory_usage(0), alignment_waste(0), cache_efficiency_score(0.0)
        , spatial_locality_score(0.0), memory_fragmentation(0.0) {}
};

/**
 * @brief Archetype performance metrics
 */
struct ArchetypePerformanceMetrics {
    u32 archetype_id;                       // Archetype identifier
    
    // Entity operations
    f64 entity_creation_time;               // Average entity creation time
    f64 entity_destruction_time;            // Average entity destruction time
    f64 component_access_time;              // Average component access time
    f64 archetype_iteration_time;           // Time to iterate all entities
    
    // System interaction metrics
    u64 system_queries;                     // Times queried by systems
    f64 average_query_time;                 // Average query execution time
    std::unordered_map<std::string, f64> system_interaction_times; // Per-system times
    
    // Memory performance
    f64 cache_miss_rate;                    // Estimated cache miss rate
    f64 memory_bandwidth_usage;             // Memory bandwidth utilization
    f64 allocation_overhead;                // Allocation overhead percentage
    
    // Scalability metrics
    f64 performance_per_entity;             // Performance cost per entity
    f64 memory_per_entity;                  // Memory cost per entity
    f64 scalability_score;                  // Overall scalability rating
    
    ArchetypePerformanceMetrics() noexcept
        : archetype_id(0), entity_creation_time(0.0), entity_destruction_time(0.0)
        , component_access_time(0.0), archetype_iteration_time(0.0)
        , system_queries(0), average_query_time(0.0), cache_miss_rate(0.0)
        , memory_bandwidth_usage(0.0), allocation_overhead(0.0)
        , performance_per_entity(0.0), memory_per_entity(0.0)
        , scalability_score(0.0) {}
};

/**
 * @brief Archetype optimization suggestions
 */
struct ArchetypeOptimizationSuggestions {
    u32 archetype_id;                       // Target archetype
    
    // Component organization suggestions
    std::vector<std::string> component_reordering;    // Suggested component order
    std::vector<core::ComponentID> components_to_split; // Components to separate
    std::vector<std::pair<core::ComponentID, core::ComponentID>> components_to_merge; // Components to combine
    
    // Memory optimization suggestions
    std::vector<std::string> alignment_optimizations;  // Alignment improvements
    std::vector<std::string> layout_optimizations;     // Layout improvements
    bool suggest_soa_conversion;                        // Suggest SoA conversion
    
    // Performance optimization suggestions
    std::vector<std::string> access_pattern_improvements; // Access pattern fixes
    std::vector<std::string> caching_strategies;          // Caching suggestions
    
    // Predicted improvements
    f64 predicted_memory_savings;           // Predicted memory savings (percentage)
    f64 predicted_performance_gain;         // Predicted performance gain (percentage)
    f64 implementation_complexity;          // Implementation difficulty (0.0-1.0)
    
    ArchetypeOptimizationSuggestions() noexcept
        : archetype_id(0), suggest_soa_conversion(false)
        , predicted_memory_savings(0.0), predicted_performance_gain(0.0)
        , implementation_complexity(0.0) {}
};

/**
 * @brief Archetype evolution tracking
 */
struct ArchetypeEvolution {
    struct EvolutionEvent {
        f64 timestamp;                      // When the event occurred
        enum class EventType {
            Created,                        // Archetype was created
            FirstEntity,                    // First entity added
            GrowthSpurt,                    // Rapid entity addition
            Plateau,                        // Stable entity count
            Decline,                        // Entity count decreasing
            Abandoned,                      // No entities remain
            MemoryPressure,                 // Memory pressure detected
            PerformanceBottleneck          // Performance issues detected
        } type;
        
        usize entity_count;                 // Entity count at this time
        usize memory_usage;                 // Memory usage at this time
        f64 performance_score;              // Performance score at this time
        std::string description;            // Human-readable description
        
        EvolutionEvent() noexcept
            : timestamp(0.0), type(EventType::Created), entity_count(0)
            , memory_usage(0), performance_score(0.0) {}
    };
    
    u32 archetype_id;                       // Archetype identifier
    f64 creation_time;                      // When archetype was created
    std::vector<EvolutionEvent> events;     // Timeline of events
    f64 current_trend;                      // Current growth/decline trend
    f64 predicted_lifespan;                 // Predicted remaining lifespan
    
    ArchetypeEvolution() noexcept
        : archetype_id(0), creation_time(0.0), current_trend(0.0)
        , predicted_lifespan(0.0) {}
};

/**
 * @brief Main archetype analyzer class
 */
class ArchetypeAnalyzer {
private:
    // Analysis data
    std::unordered_map<u32, ArchetypeMemoryLayout> memory_layouts_;
    std::unordered_map<u32, ArchetypePerformanceMetrics> performance_metrics_;
    std::unordered_map<u32, ArchetypeOptimizationSuggestions> optimization_suggestions_;
    std::unordered_map<u32, ArchetypeEvolution> archetype_evolutions_;
    std::vector<ArchetypeTransition> transitions_;
    std::vector<ComponentCorrelation> component_correlations_;
    
    // Analysis configuration
    bool enable_continuous_analysis_;       // Continuous vs on-demand analysis
    f64 analysis_frequency_;                // Analysis update frequency
    f64 last_analysis_time_;                // Last analysis timestamp
    usize max_tracked_archetypes_;          // Maximum archetypes to track
    
    // Performance tracking
    mutable std::chrono::high_resolution_clock::time_point last_update_;
    f64 analysis_overhead_;                 // Time spent in analysis
    
    // Registry reference (would be injected)
    // const ecs::Registry* registry_;
    
public:
    /**
     * @brief Constructor
     */
    explicit ArchetypeAnalyzer(bool enable_continuous = true);
    
    /**
     * @brief Destructor
     */
    ~ArchetypeAnalyzer() = default;
    
    // Core analysis functions
    void update_analysis(f64 delta_time) noexcept;
    void perform_full_analysis() noexcept;
    void analyze_archetype_relationships() noexcept;
    void analyze_memory_layouts() noexcept;
    void analyze_performance_metrics() noexcept;
    void analyze_component_correlations() noexcept;
    void track_archetype_evolution() noexcept;
    void generate_optimization_suggestions() noexcept;
    
    // Configuration
    void set_continuous_analysis(bool enable) noexcept { enable_continuous_analysis_ = enable; }
    void set_analysis_frequency(f64 frequency) noexcept { analysis_frequency_ = frequency; }
    void set_max_tracked_archetypes(usize max_count) noexcept { max_tracked_archetypes_ = max_count; }
    
    // Data access
    const std::vector<ArchetypeTransition>& get_transitions() const noexcept { return transitions_; }
    const std::vector<ComponentCorrelation>& get_component_correlations() const noexcept { return component_correlations_; }
    
    const ArchetypeMemoryLayout* get_memory_layout(u32 archetype_id) const noexcept;
    const ArchetypePerformanceMetrics* get_performance_metrics(u32 archetype_id) const noexcept;
    const ArchetypeOptimizationSuggestions* get_optimization_suggestions(u32 archetype_id) const noexcept;
    const ArchetypeEvolution* get_archetype_evolution(u32 archetype_id) const noexcept;
    
    // Query functions
    std::vector<u32> get_most_used_archetypes(usize count = 10) const noexcept;
    std::vector<u32> get_memory_inefficient_archetypes(f64 threshold = 0.5) const noexcept;
    std::vector<u32> get_performance_bottleneck_archetypes(f64 threshold = 0.7) const noexcept;
    std::vector<ArchetypeTransition> get_most_frequent_transitions(usize count = 10) const noexcept;
    std::vector<ComponentCorrelation> get_strongest_correlations(usize count = 10) const noexcept;
    
    // Predictive analysis
    f64 predict_archetype_growth(u32 archetype_id, f64 time_horizon) const noexcept;
    f64 predict_memory_usage(u32 archetype_id, f64 time_horizon) const noexcept;
    std::vector<u32> predict_archetype_bottlenecks(f64 time_horizon) const noexcept;
    
    // Optimization analysis
    f64 calculate_cache_efficiency(u32 archetype_id) const noexcept;
    f64 calculate_memory_fragmentation(u32 archetype_id) const noexcept;
    f64 calculate_access_pattern_score(u32 archetype_id) const noexcept;
    
    // Educational insights
    std::vector<std::string> generate_educational_insights() const noexcept;
    std::string explain_archetype_design(u32 archetype_id) const noexcept;
    std::string suggest_component_groupings() const noexcept;
    
    // Export capabilities
    void export_analysis_report(const std::string& filename) const noexcept;
    void export_transition_graph(const std::string& filename) const noexcept;
    void export_correlation_matrix(const std::string& filename) const noexcept;
    void export_performance_summary(const std::string& filename) const noexcept;
    
    // Statistics
    f64 get_analysis_overhead() const noexcept { return analysis_overhead_; }
    usize get_tracked_archetype_count() const noexcept;
    usize get_total_transitions() const noexcept;
    usize get_active_correlations() const noexcept;
    
private:
    // Internal analysis functions
    void collect_archetype_data() noexcept;
    void update_transition_data(const ArchetypeTransition& transition) noexcept;
    void calculate_component_correlations() noexcept;
    void analyze_memory_efficiency() noexcept;
    void track_performance_changes() noexcept;
    void detect_optimization_opportunities() noexcept;
    
    // Utility functions
    bool should_analyze() const noexcept;
    void cleanup_old_data() noexcept;
    f64 calculate_correlation_strength(core::ComponentID comp_a, core::ComponentID comp_b) const noexcept;
    f64 estimate_cache_performance(const ArchetypeMemoryLayout& layout) const noexcept;
    
    // Analysis algorithms
    void perform_clustering_analysis() noexcept;
    void detect_access_patterns() noexcept;
    void analyze_temporal_patterns() noexcept;
    
    // Event tracking
    void record_evolution_event(u32 archetype_id, ArchetypeEvolution::EvolutionEvent::EventType type,
                               const std::string& description) noexcept;
    
    // Configuration
    static constexpr f64 DEFAULT_ANALYSIS_FREQUENCY = 1.0; // 1 Hz
    static constexpr usize DEFAULT_MAX_TRACKED_ARCHETYPES = 100;
    static constexpr f64 CACHE_EFFICIENCY_THRESHOLD = 0.8;
    static constexpr f64 MEMORY_FRAGMENTATION_THRESHOLD = 0.3;
    static constexpr f64 PERFORMANCE_BOTTLENECK_THRESHOLD = 0.7;
};

/**
 * @brief Integration helpers for Visual ECS Inspector
 */
namespace archetype_analysis_integration {
    /**
     * @brief Create analyzer with optimal settings for visualization
     */
    std::unique_ptr<ArchetypeAnalyzer> create_for_visualization();
    
    /**
     * @brief Update visualization data from analyzer
     */
    void update_visualization_data(const ArchetypeAnalyzer& analyzer,
                                  std::vector<ui::ArchetypeNode>& nodes);
    
    /**
     * @brief Generate transition connections for visualization
     */
    void update_transition_connections(const ArchetypeAnalyzer& analyzer,
                                      std::vector<ui::ArchetypeNode>& nodes);
    
    /**
     * @brief Create educational tooltips from analysis data
     */
    std::unordered_map<std::string, std::string> create_educational_tooltips(
        const ArchetypeAnalyzer& analyzer);
    
    /**
     * @brief Generate optimization recommendations UI
     */
    void render_optimization_recommendations(const ArchetypeAnalyzer& analyzer,
                                           u32 selected_archetype_id);
}

/**
 * @brief Real-time archetype monitoring for debugging
 */
class ArchetypeMonitor {
private:
    std::unique_ptr<ArchetypeAnalyzer> analyzer_;
    std::function<void(const std::string&)> alert_callback_;
    
    // Alert thresholds
    f64 memory_pressure_threshold_;
    f64 performance_degradation_threshold_;
    usize entity_count_spike_threshold_;
    
    // Alert state
    bool monitoring_enabled_;
    std::unordered_set<u32> alerted_archetypes_;
    
public:
    explicit ArchetypeMonitor(std::function<void(const std::string&)> alert_callback = nullptr);
    
    void update(f64 delta_time) noexcept;
    void enable_monitoring(bool enabled) noexcept { monitoring_enabled_ = enabled; }
    
    void set_memory_pressure_threshold(f64 threshold) noexcept { memory_pressure_threshold_ = threshold; }
    void set_performance_threshold(f64 threshold) noexcept { performance_degradation_threshold_ = threshold; }
    void set_entity_spike_threshold(usize threshold) noexcept { entity_count_spike_threshold_ = threshold; }
    
    const ArchetypeAnalyzer& analyzer() const noexcept { return *analyzer_; }
    
private:
    void check_memory_pressure() noexcept;
    void check_performance_degradation() noexcept;
    void check_entity_count_spikes() noexcept;
    void send_alert(const std::string& message) noexcept;
};

} // namespace ecscope::analysis