#pragma once

#include "ml_prediction_system.hpp"
#include "ecs_behavior_predictor.hpp"
#include "entity.hpp"
#include "component.hpp"
#include "registry.hpp"
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
#include <array>
#include <algorithm>

namespace ecscope::ml {

/**
 * @brief Performance bottleneck types
 */
enum class BottleneckType {
    CPU_Bound,           // CPU processing is the limiting factor
    Memory_Bound,        // Memory allocation/access is the limiting factor
    Cache_Misses,        // Cache performance issues
    Archetype_Lookup,    // Slow archetype operations
    Component_Access,    // Slow component access patterns
    Entity_Creation,     // Entity creation/destruction bottlenecks
    System_Scheduling,   // System execution order issues
    Data_Layout,         // Poor data locality
    Contention,          // Lock contention or threading issues
    Unknown              // Unidentified bottleneck
};

/**
 * @brief Performance metric snapshot
 */
struct PerformanceSnapshot {
    Timestamp timestamp;
    f32 frame_time{0.0f};           // Current frame time in milliseconds
    f32 cpu_usage{0.0f};            // CPU usage percentage (0-100)
    f32 memory_usage{0.0f};         // Memory usage in MB
    f32 memory_pressure{0.0f};      // Memory pressure (0-1)
    usize active_entities{0};       // Number of active entities
    usize active_systems{0};        // Number of active systems
    f32 cache_hit_ratio{0.0f};      // Cache hit ratio (0-1)
    f32 allocation_rate{0.0f};      // Allocations per second
    f32 gc_time{0.0f};              // Garbage collection time
    
    // System-specific metrics
    std::unordered_map<std::string, f32> system_times; // Time per system
    std::unordered_map<std::string, usize> system_entity_counts; // Entities per system
    
    // Advanced metrics
    f32 instruction_throughput{0.0f}; // Instructions per cycle
    f32 branch_misprediction_rate{0.0f}; // Branch mispredictions
    f32 tlb_miss_rate{0.0f};        // Translation lookaside buffer misses
    
    // Calculate derived metrics
    f32 fps() const { return frame_time > 0.0f ? 1000.0f / frame_time : 0.0f; }
    f32 entities_per_ms() const { return frame_time > 0.0f ? active_entities / frame_time : 0.0f; }
    bool is_performance_critical() const { return frame_time > 16.67f; } // < 60 FPS
    
    std::string to_string() const;
};

/**
 * @brief Predicted performance bottleneck
 */
struct PerformanceBottleneckPrediction {
    BottleneckType bottleneck_type{BottleneckType::Unknown};
    f32 probability{0.0f};          // Probability of this bottleneck occurring (0-1)
    f32 severity{0.0f};             // Expected severity impact (0-1)
    f32 confidence{0.0f};           // Model confidence in prediction (0-1)
    f32 time_to_occurrence{0.0f};   // Time until bottleneck is expected (seconds)
    f32 expected_duration{0.0f};    // How long the bottleneck might last (seconds)
    
    // Context information
    std::string system_affected;    // Which system will be affected
    std::string component_type_affected; // Which component type is involved
    usize entities_affected{0};     // Number of entities that might be impacted
    
    // Mitigation suggestions
    std::vector<std::string> mitigation_strategies; // Suggested fixes
    f32 mitigation_effort{0.0f};    // Estimated effort to fix (0-1)
    f32 performance_impact{0.0f};   // Expected performance improvement if fixed (0-1)
    
    // Educational information
    std::string explanation;        // Why this bottleneck is predicted
    std::string root_cause;         // Technical root cause
    std::vector<std::string> warning_signs; // What to look for
    
    bool is_critical() const { return probability > 0.8f && severity > 0.7f; }
    bool is_imminent() const { return time_to_occurrence < 1.0f; }
    
    std::string bottleneck_type_to_string() const;
    std::string to_string() const;
    void print_detailed_analysis() const;
};

/**
 * @brief Performance prediction result
 */
struct PerformancePrediction {
    Timestamp prediction_time;
    f32 predicted_frame_time{0.0f};     // Predicted next frame time
    f32 predicted_fps{0.0f};            // Predicted FPS
    f32 predicted_memory_usage{0.0f};   // Predicted memory usage
    f32 confidence{0.0f};               // Overall prediction confidence
    
    // Trend predictions
    f32 performance_trend{0.0f};        // Performance trend (-1 = getting worse, +1 = getting better)
    f32 stability_score{0.0f};          // How stable performance is expected to be (0-1)
    f32 scalability_score{0.0f};        // How well performance will scale with more entities (0-1)
    
    // Bottleneck predictions
    std::vector<PerformanceBottleneckPrediction> predicted_bottlenecks;
    PerformanceBottleneckPrediction primary_bottleneck; // Most likely bottleneck
    
    // Resource predictions
    f32 predicted_cpu_usage{0.0f};
    f32 predicted_memory_pressure{0.0f};
    usize predicted_allocation_count{0};
    
    // System-specific predictions
    std::unordered_map<std::string, f32> predicted_system_times;
    
    bool is_performance_degradation_predicted() const { return performance_trend < -0.3f; }
    bool has_critical_bottlenecks() const {
        return std::any_of(predicted_bottlenecks.begin(), predicted_bottlenecks.end(),
                          [](const auto& b) { return b.is_critical(); });
    }
    
    std::string to_string() const;
    void print_prediction_summary() const;
};

/**
 * @brief Configuration for performance prediction system
 */
struct PerformancePredictionConfig {
    // Sampling configuration
    std::chrono::milliseconds sampling_interval{16}; // 60 FPS sampling
    usize max_history_samples{1000};     // Maximum performance history to keep
    usize min_samples_for_prediction{20}; // Minimum samples needed for prediction
    
    // Prediction thresholds
    f32 performance_degradation_threshold{0.8f}; // When to warn about performance
    f32 bottleneck_probability_threshold{0.6f};  // Minimum probability for bottleneck warning
    f32 critical_frame_time{16.67f};             // Frame time that triggers critical warnings (ms)
    
    // Model configuration
    MLModelConfig performance_model_config{
        .model_name = "PerformancePredictor",
        .input_dimension = 30,  // Will be adjusted based on features
        .output_dimension = 5,  // Predict frame_time, memory_usage, cpu_usage, etc.
        .learning_rate = 0.008f,
        .max_epochs = 800,
        .enable_training_visualization = true
    };
    
    MLModelConfig bottleneck_model_config{
        .model_name = "BottleneckPredictor",
        .input_dimension = 25,
        .output_dimension = static_cast<usize>(BottleneckType::Unknown), // One output per bottleneck type
        .learning_rate = 0.01f,
        .max_epochs = 600,
        .enable_training_visualization = true
    };
    
    // Analysis settings
    bool enable_bottleneck_detection{true};
    bool enable_trend_analysis{true};
    bool enable_system_profiling{true};
    bool enable_memory_profiling{true};
    
    // Educational features
    bool enable_detailed_logging{true};
    bool track_prediction_accuracy{true};
    bool enable_mitigation_suggestions{true};
    
    // Performance settings
    bool enable_async_prediction{true};
    usize prediction_thread_count{2};
    std::chrono::milliseconds prediction_interval{100};
};

/**
 * @brief Statistics for performance predictions
 */
struct PerformancePredictionStats {
    // Prediction accuracy
    usize total_predictions{0};
    usize accurate_predictions{0};
    f32 overall_accuracy{0.0f};
    f32 frame_time_mae{0.0f};          // Mean absolute error for frame time
    f32 memory_prediction_mae{0.0f};    // Mean absolute error for memory
    
    // Bottleneck detection accuracy
    usize bottleneck_predictions{0};
    usize correct_bottleneck_predictions{0};
    f32 bottleneck_detection_accuracy{0.0f};
    std::unordered_map<BottleneckType, f32> bottleneck_type_accuracy;
    
    // Prevention effectiveness
    usize bottlenecks_prevented{0};
    usize mitigation_attempts{0};
    usize successful_mitigations{0};
    f32 prevention_success_rate{0.0f};
    
    void reset();
    void update_prediction_accuracy(const PerformancePrediction& prediction,
                                   const PerformanceSnapshot& actual);
    void update_bottleneck_detection(const PerformanceBottleneckPrediction& prediction, bool occurred);
    std::string to_string() const;
};

/**
 * @brief Main performance prediction system
 * 
 * This system monitors ECS performance metrics and uses machine learning to predict
 * performance bottlenecks before they occur. It provides educational insights into
 * performance optimization and bottleneck identification.
 */
class ECSPerformancePredictor {
private:
    PerformancePredictionConfig config_;
    std::unique_ptr<MLModelBase> performance_model_;
    std::unique_ptr<MLModelBase> bottleneck_model_;
    std::unique_ptr<FeatureExtractor> feature_extractor_;
    
    // Performance history
    std::queue<PerformanceSnapshot> performance_history_;
    std::vector<PerformancePrediction> prediction_history_;
    mutable std::mutex history_mutex_;
    
    // Training data
    TrainingDataset performance_dataset_;
    TrainingDataset bottleneck_dataset_;
    
    // Real-time monitoring
    std::unique_ptr<std::thread> monitoring_thread_;
    std::unique_ptr<std::thread> prediction_thread_;
    std::atomic<bool> should_stop_threads_{false};
    
    // Statistics
    PerformancePredictionStats prediction_stats_;
    std::atomic<usize> total_predictions_made_{0};
    std::atomic<usize> bottlenecks_detected_{0};
    
    // Current state
    PerformanceSnapshot current_snapshot_;
    PerformancePrediction latest_prediction_;
    mutable std::mutex current_state_mutex_;
    
    // Bottleneck analysis
    std::unordered_map<BottleneckType, std::vector<PerformanceSnapshot>> bottleneck_examples_;
    std::unordered_map<std::string, f32> system_performance_baselines_;
    
public:
    explicit ECSPerformancePredictor(const PerformancePredictionConfig& config = PerformancePredictionConfig{});
    ~ECSPerformancePredictor();
    
    // Non-copyable but movable
    ECSPerformancePredictor(const ECSPerformancePredictor&) = delete;
    ECSPerformancePredictor& operator=(const ECSPerformancePredictor&) = delete;
    ECSPerformancePredictor(ECSPerformancePredictor&&) = default;
    ECSPerformancePredictor& operator=(ECSPerformancePredictor&&) = default;
    
    // Monitoring and data collection
    void start_monitoring(const ecs::Registry& registry);
    void stop_monitoring();
    PerformanceSnapshot take_snapshot(const ecs::Registry& registry);
    void record_performance_sample(const PerformanceSnapshot& snapshot);
    
    // Prediction methods
    PerformancePrediction predict_performance(const ecs::Registry& registry);
    std::vector<PerformanceBottleneckPrediction> predict_bottlenecks(const ecs::Registry& registry);
    PerformancePrediction predict_with_entity_count(usize predicted_entity_count,
                                                   const ecs::Registry& registry);
    
    // Specific bottleneck detection
    PerformanceBottleneckPrediction detect_memory_bottleneck(const PerformanceSnapshot& snapshot);
    PerformanceBottleneckPrediction detect_cpu_bottleneck(const PerformanceSnapshot& snapshot);
    PerformanceBottleneckPrediction detect_cache_bottleneck(const PerformanceSnapshot& snapshot);
    PerformanceBottleneckPrediction detect_system_bottleneck(const PerformanceSnapshot& snapshot,
                                                            const std::string& system_name);
    
    // Model training and management
    bool train_performance_model();
    bool train_bottleneck_model();
    void collect_training_data(const ecs::Registry& registry);
    void learn_from_bottleneck(const PerformanceBottleneckPrediction& predicted,
                              const PerformanceSnapshot& actual);
    
    // Analysis and insights
    std::vector<std::string> analyze_performance_trends() const;
    std::unordered_map<std::string, f32> get_system_performance_profile() const;
    std::vector<std::string> suggest_optimizations() const;
    f32 calculate_performance_stability() const;
    
    // Statistics and validation
    const PerformancePredictionStats& get_prediction_statistics() const { return prediction_stats_; }
    f32 validate_prediction_accuracy(const std::vector<PerformanceSnapshot>& test_data);
    void evaluate_prediction_vs_reality(const PerformancePrediction& prediction,
                                       const PerformanceSnapshot& reality);
    
    // Configuration and state
    const PerformancePredictionConfig& config() const { return config_; }
    void update_config(const PerformancePredictionConfig& new_config);
    PerformanceSnapshot get_current_snapshot() const;
    PerformancePrediction get_latest_prediction() const;
    
    // Educational features
    std::string generate_performance_report() const;
    std::string explain_bottleneck(const PerformanceBottleneckPrediction& bottleneck) const;
    void print_performance_analysis() const;
    std::string visualize_performance_trends() const;
    std::string get_optimization_guide() const;
    
    // Callbacks for real-time integration
    using BottleneckCallback = std::function<void(const PerformanceBottleneckPrediction&)>;
    using PerformancePredictionCallback = std::function<void(const PerformancePrediction&)>;
    
    void set_bottleneck_callback(BottleneckCallback callback) { bottleneck_callback_ = callback; }
    void set_prediction_callback(PerformancePredictionCallback callback) { prediction_callback_ = callback; }
    
    // Advanced analysis
    std::vector<std::string> identify_performance_regressions() const;
    f32 predict_scalability_limit(const ecs::Registry& registry) const;
    std::unordered_map<BottleneckType, f32> get_bottleneck_likelihood_distribution() const;
    
private:
    // Internal implementation
    void initialize_models();
    void initialize_feature_extraction();
    void start_background_threads();
    void stop_background_threads();
    
    // Data processing
    FeatureVector extract_performance_features(const PerformanceSnapshot& snapshot);
    FeatureVector extract_bottleneck_features(const PerformanceSnapshot& snapshot);
    TrainingSample create_performance_training_sample(const PerformanceSnapshot& current,
                                                     const PerformanceSnapshot& future);
    TrainingSample create_bottleneck_training_sample(const PerformanceSnapshot& snapshot,
                                                    BottleneckType bottleneck_type);
    
    // Prediction implementation
    PerformancePrediction make_performance_prediction_internal(const PerformanceSnapshot& snapshot);
    std::vector<PerformanceBottleneckPrediction> detect_bottlenecks_internal(const PerformanceSnapshot& snapshot);
    
    // Analysis helpers
    f32 calculate_trend(const std::vector<f32>& values) const;
    f32 calculate_stability(const std::vector<f32>& values) const;
    BottleneckType classify_bottleneck(const PerformanceSnapshot& snapshot) const;
    
    // Background thread functions
    void monitoring_thread_function(const ecs::Registry& registry);
    void prediction_thread_function();
    
    // Bottleneck-specific analysis
    bool is_memory_bottleneck(const PerformanceSnapshot& snapshot) const;
    bool is_cpu_bottleneck(const PerformanceSnapshot& snapshot) const;
    bool is_cache_bottleneck(const PerformanceSnapshot& snapshot) const;
    
    // Mitigation strategy generation
    std::vector<std::string> generate_mitigation_strategies(BottleneckType bottleneck_type,
                                                           const std::string& context) const;
    
    // Educational content generation
    std::string explain_bottleneck_type(BottleneckType type) const;
    std::string generate_optimization_suggestions(const PerformanceSnapshot& snapshot) const;
    
    // Callbacks
    BottleneckCallback bottleneck_callback_;
    PerformancePredictionCallback prediction_callback_;
};

/**
 * @brief Utility functions for performance prediction
 */
namespace performance_utils {

// Performance analysis
f32 calculate_performance_score(const PerformanceSnapshot& snapshot);
f32 calculate_efficiency_ratio(const PerformanceSnapshot& snapshot);
bool is_performance_regression(const std::vector<PerformanceSnapshot>& history, f32 threshold = 0.1f);

// Bottleneck analysis
BottleneckType identify_primary_bottleneck(const PerformanceSnapshot& snapshot);
f32 calculate_bottleneck_severity(const PerformanceSnapshot& snapshot, BottleneckType type);
std::vector<std::string> get_bottleneck_warning_signs(BottleneckType type);

// Educational utilities
std::string create_performance_timeline(const std::vector<PerformanceSnapshot>& history);
std::string explain_performance_metrics(const PerformanceSnapshot& snapshot);
std::string visualize_bottleneck_prediction(const PerformanceBottleneckPrediction& prediction);

// Optimization suggestions
std::vector<std::string> suggest_memory_optimizations(f32 memory_pressure);
std::vector<std::string> suggest_cpu_optimizations(f32 cpu_usage);
std::vector<std::string> suggest_cache_optimizations(f32 cache_hit_ratio);

} // namespace performance_utils

} // namespace ecscope::ml