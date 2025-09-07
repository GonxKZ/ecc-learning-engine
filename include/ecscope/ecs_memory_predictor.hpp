#pragma once

#include "ml_prediction_system.hpp"
#include "ecs_behavior_predictor.hpp"
#include "entity.hpp"
#include "component.hpp"
#include "registry.hpp"
#include "memory/arena.hpp"
#include "memory/pool.hpp"
#include "memory/memory_tracker.hpp"
#include <vector>
#include <memory>
#include <string>
#include <chrono>
#include <unordered_map>
#include <queue>
#include <thread>
#include <atomic>
#include <mutex>
#include <functional>
#include <algorithm>

namespace ecscope::ml {

/**
 * @brief Memory allocation pattern types
 */
enum class AllocationPattern {
    Sequential,    // Allocations happen in sequence
    Random,        // Random allocation pattern
    Burst,         // Sudden bursts of allocations
    Periodic,      // Periodic allocation cycles
    EntityBased,   // Allocations follow entity lifecycle
    ComponentBased,// Allocations follow component patterns
    SystemBased,   // Allocations follow system execution
    Fragmented,    // Highly fragmented allocation pattern
    Unknown        // Unidentified pattern
};

/**
 * @brief Memory allocation event for tracking
 */
struct MemoryAllocationEvent {
    Timestamp timestamp;
    EntityID entity{ecs::null_entity()};
    std::string component_type;
    std::string allocator_type;       // arena, pool, standard, etc.
    usize allocation_size{0};
    void* allocation_address{nullptr};
    bool is_deallocation{false};
    
    // Context information
    f32 heap_pressure{0.0f};         // Memory pressure at time of allocation
    usize concurrent_allocations{0}; // Other allocations happening simultaneously
    std::string calling_system;     // Which system triggered the allocation
    
    // Performance metrics
    f32 allocation_time{0.0f};       // Time taken for allocation (microseconds)
    bool caused_gc{false};           // Whether allocation triggered garbage collection
    bool caused_expansion{false};    // Whether allocation required memory expansion
    
    std::string to_string() const;
};

/**
 * @brief Memory usage prediction
 */
struct MemoryUsagePrediction {
    Timestamp prediction_time;
    f32 time_horizon{1.0f};          // How far into future (seconds)
    f32 confidence{0.0f};            // Prediction confidence (0-1)
    
    // Memory predictions
    usize predicted_heap_usage{0};      // Predicted heap memory usage (bytes)
    usize predicted_peak_usage{0};      // Predicted peak memory usage
    f32 predicted_fragmentation{0.0f};  // Predicted memory fragmentation (0-1)
    f32 predicted_pressure{0.0f};       // Predicted memory pressure (0-1)
    
    // Allocation predictions
    usize predicted_allocation_count{0}; // Number of allocations expected
    usize predicted_deallocation_count{0}; // Number of deallocations expected
    f32 predicted_allocation_rate{0.0f}; // Allocations per second
    
    // Pattern predictions
    AllocationPattern predicted_pattern{AllocationPattern::Unknown};
    f32 pattern_confidence{0.0f};       // Confidence in pattern prediction
    std::vector<std::string> pattern_factors; // What indicates this pattern
    
    // Risk assessments
    f32 oom_risk{0.0f};                 // Out of memory risk (0-1)
    f32 fragmentation_risk{0.0f};       // Memory fragmentation risk (0-1)
    f32 gc_trigger_probability{0.0f};   // Probability of triggering GC
    
    // Specific allocator predictions
    std::unordered_map<std::string, usize> allocator_usage_predictions;
    std::unordered_map<std::string, f32> allocator_efficiency_predictions;
    
    // Educational information
    std::string prediction_reasoning;   // Why this prediction was made
    std::vector<std::string> warning_signs; // Early warning indicators
    std::vector<std::string> optimization_suggestions; // Suggested optimizations
    
    bool is_memory_critical() const { return predicted_pressure > 0.8f || oom_risk > 0.3f; }
    bool suggests_gc() const { return gc_trigger_probability > 0.7f; }
    
    std::string to_string() const;
    void print_detailed_analysis() const;
};

/**
 * @brief Memory pool optimization suggestion
 */
struct MemoryPoolOptimization {
    std::string allocator_name;
    std::string optimization_type;   // "resize", "defragment", "merge", etc.
    f32 potential_savings{0.0f};     // Estimated memory savings (bytes)
    f32 performance_impact{0.0f};    // Expected performance impact (positive = better)
    f32 implementation_difficulty{0.5f}; // How hard to implement (0-1)
    
    // Specific recommendations
    usize recommended_pool_size{0};
    usize recommended_block_size{0};
    bool recommend_compaction{false};
    bool recommend_expansion{false};
    
    std::string reasoning;           // Why this optimization is suggested
    std::vector<std::string> steps; // Steps to implement
    
    std::string to_string() const;
};

/**
 * @brief Configuration for memory prediction system
 */
struct MemoryPredictionConfig {
    // Prediction settings
    f32 prediction_horizon{5.0f};            // How far ahead to predict (seconds)
    usize max_allocation_history{10000};     // Maximum allocation events to track
    f32 min_prediction_confidence{0.6f};     // Minimum confidence for predictions
    
    // Pattern detection
    bool enable_pattern_detection{true};
    usize pattern_detection_window{500};     // Events to consider for pattern detection
    f32 pattern_significance_threshold{0.7f}; // Threshold for pattern recognition
    
    // Model configuration
    MLModelConfig memory_model_config{
        .model_name = "MemoryPredictor",
        .input_dimension = 25,  // Will be adjusted
        .output_dimension = 8,  // Various memory metrics
        .learning_rate = 0.008f,
        .max_epochs = 600,
        .enable_training_visualization = true
    };
    
    MLModelConfig pattern_model_config{
        .model_name = "AllocationPatternClassifier",
        .input_dimension = 20,
        .output_dimension = static_cast<usize>(AllocationPattern::Unknown),
        .learning_rate = 0.01f,
        .max_epochs = 400,
        .enable_training_visualization = true
    };
    
    // Memory thresholds
    f32 pressure_warning_threshold{0.7f};    // When to warn about memory pressure
    f32 pressure_critical_threshold{0.9f};   // When memory is critically low
    f32 fragmentation_threshold{0.6f};       // When fragmentation is problematic
    
    // Optimization settings
    bool enable_automatic_optimization{true};
    bool enable_pool_resizing{true};
    bool enable_compaction_suggestions{true};
    f32 optimization_aggressiveness{0.5f};   // How aggressive optimizations should be
    
    // Educational features
    bool enable_detailed_logging{true};
    bool track_allocation_efficiency{true};
    bool enable_pattern_visualization{true};
};

/**
 * @brief Statistics for memory predictions
 */
struct MemoryPredictionStats {
    // Prediction accuracy
    usize total_predictions{0};
    usize accurate_predictions{0};
    f32 overall_accuracy{0.0f};
    f32 memory_usage_mae{0.0f};        // Mean absolute error for memory usage predictions
    f32 pressure_prediction_accuracy{0.0f}; // Accuracy of pressure predictions
    
    // Pattern detection accuracy
    usize pattern_predictions{0};
    usize correct_pattern_predictions{0};
    f32 pattern_detection_accuracy{0.0f};
    std::unordered_map<AllocationPattern, f32> pattern_accuracy_by_type;
    
    // Optimization effectiveness
    usize optimizations_suggested{0};
    usize optimizations_applied{0};
    f32 average_memory_savings{0.0f};   // Average memory saved per optimization
    f32 average_performance_improvement{0.0f}; // Average performance improvement
    
    // Memory health metrics
    f32 average_memory_efficiency{0.8f}; // How efficiently memory is used
    f32 average_fragmentation_level{0.2f}; // Average fragmentation
    usize oom_events_prevented{0};      // Out of memory events prevented
    
    void reset();
    void update_prediction_accuracy(const MemoryUsagePrediction& prediction,
                                   usize actual_usage, f32 actual_pressure);
    void update_pattern_accuracy(AllocationPattern predicted, AllocationPattern actual);
    void update_optimization_effectiveness(const MemoryPoolOptimization& optimization,
                                         f32 actual_savings, f32 actual_improvement);
    std::string to_string() const;
};

/**
 * @brief Main memory prediction system for ECS
 * 
 * This system monitors memory allocation patterns and uses machine learning to predict
 * future memory usage, detect potential issues, and suggest optimizations. It provides
 * educational insights into memory management in game engines.
 */
class ECSMemoryPredictor {
private:
    MemoryPredictionConfig config_;
    std::unique_ptr<MLModelBase> memory_model_;
    std::unique_ptr<MLModelBase> pattern_model_;
    std::unique_ptr<FeatureExtractor> feature_extractor_;
    std::unique_ptr<ECSBehaviorPredictor> behavior_predictor_;
    
    // Allocation tracking
    std::queue<MemoryAllocationEvent> allocation_history_;
    std::unordered_map<void*, MemoryAllocationEvent> active_allocations_;
    mutable std::mutex allocation_mutex_;
    
    // Pattern analysis
    std::vector<AllocationPattern> detected_patterns_;
    std::unordered_map<AllocationPattern, std::vector<MemoryAllocationEvent>> pattern_examples_;
    f32 current_pattern_confidence_{0.0f};
    
    // Training data
    TrainingDataset memory_dataset_;
    TrainingDataset pattern_dataset_;
    
    // Current state
    MemoryUsagePrediction latest_prediction_;
    std::vector<MemoryPoolOptimization> pending_optimizations_;
    
    // Statistics and monitoring
    MemoryPredictionStats prediction_stats_;
    std::vector<f32> memory_usage_history_;
    std::vector<f32> pressure_history_;
    
    // Background processing
    std::unique_ptr<std::thread> monitoring_thread_;
    std::unique_ptr<std::thread> analysis_thread_;
    std::atomic<bool> should_stop_threads_{false};
    
    // Integration with memory systems
    std::unordered_map<std::string, memory::ArenaAllocator*> registered_arenas_;
    std::unordered_map<std::string, memory::PoolAllocator*> registered_pools_;
    std::function<void(const MemoryAllocationEvent&)> allocation_callback_;
    
    // Performance tracking
    std::atomic<usize> total_predictions_made_{0};
    std::atomic<usize> optimizations_applied_{0};
    std::atomic<usize> memory_issues_prevented_{0};
    
public:
    explicit ECSMemoryPredictor(const MemoryPredictionConfig& config = MemoryPredictionConfig{});
    ~ECSMemoryPredictor();
    
    // Non-copyable but movable
    ECSMemoryPredictor(const ECSMemoryPredictor&) = delete;
    ECSMemoryPredictor& operator=(const ECSMemoryPredictor&) = delete;
    ECSMemoryPredictor(ECSMemoryPredictor&&) = default;
    ECSMemoryPredictor& operator=(ECSMemoryPredictor&&) = default;
    
    // Allocator registration and integration
    void register_arena_allocator(const std::string& name, memory::ArenaAllocator* arena);
    void register_pool_allocator(const std::string& name, memory::PoolAllocator* pool);
    void unregister_allocator(const std::string& name);
    
    // Allocation tracking
    void track_allocation(const MemoryAllocationEvent& event);
    void track_deallocation(void* address);
    void start_monitoring(const ecs::Registry& registry);
    void stop_monitoring();
    
    // Prediction methods
    MemoryUsagePrediction predict_memory_usage(const ecs::Registry& registry, f32 time_horizon = 1.0f);
    std::vector<MemoryUsagePrediction> predict_memory_usage_timeline(const ecs::Registry& registry,
                                                                    f32 max_time = 10.0f,
                                                                    f32 time_step = 1.0f);
    AllocationPattern predict_allocation_pattern(const std::vector<MemoryAllocationEvent>& recent_events);
    
    // Optimization and analysis
    std::vector<MemoryPoolOptimization> analyze_memory_efficiency(const ecs::Registry& registry);
    std::vector<MemoryPoolOptimization> suggest_pool_optimizations();
    bool apply_optimization(const MemoryPoolOptimization& optimization);
    void optimize_memory_automatically(const ecs::Registry& registry);
    
    // Pattern detection
    AllocationPattern detect_current_pattern();
    f32 calculate_pattern_confidence(AllocationPattern pattern) const;
    std::vector<AllocationPattern> get_historical_patterns() const;
    
    // Model training and learning
    bool train_memory_model();
    bool train_pattern_model();
    void collect_training_data(const ecs::Registry& registry);
    void learn_from_allocation_results();
    
    // Analysis and insights
    f32 calculate_memory_efficiency(const ecs::Registry& registry) const;
    f32 calculate_fragmentation_level() const;
    f32 estimate_memory_pressure(const ecs::Registry& registry) const;
    std::vector<std::string> identify_memory_hotspots() const;
    
    // Statistics and validation
    const MemoryPredictionStats& get_prediction_statistics() const { return prediction_stats_; }
    f32 validate_prediction_accuracy(const std::vector<MemoryAllocationEvent>& test_data);
    void evaluate_prediction_vs_reality(const MemoryUsagePrediction& prediction,
                                       usize actual_usage, f32 actual_pressure);
    
    // Configuration and state
    const MemoryPredictionConfig& config() const { return config_; }
    void update_config(const MemoryPredictionConfig& new_config);
    MemoryUsagePrediction get_latest_prediction() const { return latest_prediction_; }
    
    // Educational features
    std::string generate_memory_analysis_report() const;
    std::string explain_allocation_pattern(AllocationPattern pattern) const;
    void print_memory_health_summary() const;
    std::string visualize_allocation_timeline() const;
    std::string get_memory_optimization_guide() const;
    
    // Integration with behavior predictor
    void set_behavior_predictor(std::unique_ptr<ECSBehaviorPredictor> predictor);
    const ECSBehaviorPredictor* get_behavior_predictor() const { return behavior_predictor_.get(); }
    
    // Callbacks for real-time integration
    using MemoryPredictionCallback = std::function<void(const MemoryUsagePrediction&)>;
    using OptimizationCallback = std::function<void(const MemoryPoolOptimization&)>;
    using PatternChangeCallback = std::function<void(AllocationPattern, f32)>;
    
    void set_prediction_callback(MemoryPredictionCallback callback) { prediction_callback_ = callback; }
    void set_optimization_callback(OptimizationCallback callback) { optimization_callback_ = callback; }
    void set_pattern_change_callback(PatternChangeCallback callback) { pattern_change_callback_ = callback; }
    
    // Advanced analysis
    std::unordered_map<std::string, f32> analyze_allocator_efficiency() const;
    f32 predict_memory_scalability(usize additional_entities, const ecs::Registry& registry) const;
    std::vector<std::string> detect_memory_leaks() const;
    
private:
    // Internal implementation
    void initialize_models();
    void initialize_feature_extraction();
    void start_background_threads();
    void stop_background_threads();
    
    // Data processing
    FeatureVector extract_memory_features(const ecs::Registry& registry) const;
    FeatureVector extract_pattern_features(const std::vector<MemoryAllocationEvent>& events) const;
    TrainingSample create_memory_training_sample(const ecs::Registry& registry, f32 future_usage);
    TrainingSample create_pattern_training_sample(const std::vector<MemoryAllocationEvent>& events,
                                                 AllocationPattern pattern);
    
    // Prediction implementation
    MemoryUsagePrediction make_memory_prediction_internal(const ecs::Registry& registry, f32 time_horizon);
    AllocationPattern classify_allocation_pattern(const std::vector<MemoryAllocationEvent>& events);
    
    // Analysis helpers
    f32 calculate_allocation_rate() const;
    f32 calculate_deallocation_rate() const;
    f32 analyze_allocation_timing_patterns() const;
    std::vector<std::string> identify_allocation_trends() const;
    
    // Background thread functions
    void monitoring_thread_function(const ecs::Registry& registry);
    void analysis_thread_function();
    
    // Optimization implementation
    MemoryPoolOptimization analyze_arena_efficiency(const std::string& name,
                                                   memory::ArenaAllocator* arena) const;
    MemoryPoolOptimization analyze_pool_efficiency(const std::string& name,
                                                  memory::PoolAllocator* pool) const;
    bool implement_pool_optimization(const MemoryPoolOptimization& optimization);
    
    // Pattern analysis implementation
    bool is_sequential_pattern(const std::vector<MemoryAllocationEvent>& events) const;
    bool is_burst_pattern(const std::vector<MemoryAllocationEvent>& events) const;
    bool is_periodic_pattern(const std::vector<MemoryAllocationEvent>& events) const;
    f32 calculate_pattern_strength(const std::vector<MemoryAllocationEvent>& events,
                                  AllocationPattern pattern) const;
    
    // Memory health assessment
    f32 assess_memory_health(const ecs::Registry& registry) const;
    std::vector<std::string> generate_health_warnings(f32 health_score) const;
    
    // Educational content generation
    std::string explain_memory_concept(const std::string& concept) const;
    std::string generate_optimization_explanation(const MemoryPoolOptimization& optimization) const;
    
    // Callbacks
    MemoryPredictionCallback prediction_callback_;
    OptimizationCallback optimization_callback_;
    PatternChangeCallback pattern_change_callback_;
};

/**
 * @brief Utility functions for memory prediction
 */
namespace memory_prediction_utils {

// Memory analysis
f32 calculate_memory_fragmentation(const std::vector<MemoryAllocationEvent>& allocations);
f32 analyze_allocation_locality(const std::vector<MemoryAllocationEvent>& allocations);
std::vector<std::string> identify_allocation_hotspots(const std::vector<MemoryAllocationEvent>& allocations);

// Pattern analysis
AllocationPattern classify_allocation_pattern_simple(const std::vector<MemoryAllocationEvent>& events);
f32 calculate_pattern_consistency(const std::vector<AllocationPattern>& patterns);
std::string describe_allocation_pattern(AllocationPattern pattern);

// Educational utilities
std::string visualize_memory_usage_over_time(const std::vector<f32>& memory_usage,
                                           const std::vector<Timestamp>& timestamps);
std::string create_allocation_heatmap(const std::vector<MemoryAllocationEvent>& allocations);
std::string explain_memory_optimization(const MemoryPoolOptimization& optimization);

// Optimization helpers
f32 calculate_potential_memory_savings(const std::vector<MemoryAllocationEvent>& allocations);
std::vector<std::string> suggest_memory_best_practices(const MemoryPredictionStats& stats);

} // namespace memory_prediction_utils

} // namespace ecscope::ml