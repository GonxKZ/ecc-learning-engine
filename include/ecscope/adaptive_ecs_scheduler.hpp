#pragma once

#include "ml_prediction_system.hpp"
#include "ecs_performance_predictor.hpp"
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
#include <algorithm>
#include <condition_variable>
#include <future>

namespace ecscope::ml {

/**
 * @brief System execution priority levels
 */
enum class SystemPriority {
    Critical,    // Must run every frame (physics, input)
    High,        // Should run every frame (rendering, audio)
    Medium,      // Can skip frames occasionally (AI, effects)
    Low,         // Can run infrequently (UI updates, analytics)
    Background   // Can run when resources are available
};

/**
 * @brief System scheduling strategy
 */
enum class SchedulingStrategy {
    FixedOrder,      // Systems run in predetermined order
    DynamicOrder,    // Order changes based on current conditions
    LoadBalanced,    // Distribute load evenly across frames
    PredictiveOrder, // Order based on ML predictions
    AdaptiveHybrid   // Combination of strategies based on performance
};

/**
 * @brief System workload characteristics
 */
struct SystemWorkloadProfile {
    std::string system_name;
    SystemPriority priority{SystemPriority::Medium};
    
    // Performance characteristics
    f32 average_execution_time{0.0f};    // Average time to execute (ms)
    f32 execution_variance{0.0f};        // Variance in execution time
    f32 cpu_intensity{0.5f};             // How CPU intensive (0-1)
    f32 memory_intensity{0.5f};          // How memory intensive (0-1)
    f32 cache_sensitivity{0.5f};         // How sensitive to cache performance (0-1)
    
    // Entity processing characteristics
    usize entities_per_ms{1000};         // Entities processed per millisecond
    f32 entity_count_sensitivity{1.0f};  // How execution time scales with entity count
    f32 component_access_pattern{0.5f};  // Access pattern efficiency (0-1)
    
    // Scheduling preferences
    f32 frame_budget_requirement{5.0f};  // Required frame time budget (ms)
    f32 delay_tolerance{0.0f};           // How much delay is acceptable (frames)
    bool can_run_parallel{false};        // Can run in parallel with other systems
    std::vector<std::string> dependencies; // Systems that must run before this one
    std::vector<std::string> conflicts;    // Systems that cannot run in parallel
    
    // Adaptive characteristics
    f32 load_adaptability{0.5f};        // How well system adapts to high load (0-1)
    f32 quality_degradation_factor{1.0f}; // Factor for quality vs performance trade-off
    bool supports_level_of_detail{false}; // Can reduce quality for performance
    
    // Statistics
    std::vector<f32> recent_execution_times; // Recent execution time history
    usize successful_executions{0};
    usize skipped_executions{0};
    usize failed_executions{0};
    
    // Calculate derived metrics
    f32 reliability_score() const;
    f32 efficiency_score() const;
    f32 predictability_score() const;
    bool is_performance_critical() const { return priority <= SystemPriority::High; }
    
    std::string to_string() const;
};

/**
 * @brief Scheduling decision for a system
 */
struct SystemSchedulingDecision {
    std::string system_name;
    bool should_execute{true};           // Whether to execute this frame
    f32 execution_probability{1.0f};     // Probability of execution (for stochastic scheduling)
    f32 quality_factor{1.0f};            // Quality reduction factor (0-1)
    usize execution_order{0};            // Order in execution queue
    f32 allocated_time_budget{16.67f};   // Allocated frame time (ms)
    
    // Parallel execution
    bool can_run_parallel{false};
    std::vector<std::string> parallel_group; // Systems that can run with this one
    usize thread_affinity{0};            // Preferred thread for execution
    
    // Reasoning (for educational purposes)
    std::string reasoning;               // Why this decision was made
    std::vector<std::string> factors;    // Factors that influenced the decision
    f32 confidence{1.0f};                // Confidence in this scheduling decision
    
    // Validation
    bool is_valid() const { return !system_name.empty() && allocated_time_budget > 0.0f; }
    bool is_high_quality() const { return quality_factor > 0.8f; }
    
    std::string to_string() const;
};

/**
 * @brief Frame scheduling plan
 */
struct FrameSchedulingPlan {
    usize frame_number{0};
    f32 target_frame_time{16.67f};       // Target frame time (60 FPS)
    f32 predicted_frame_time{16.67f};    // Predicted actual frame time
    f32 available_time_budget{16.67f};   // Available processing time
    
    // System execution plan
    std::vector<SystemSchedulingDecision> system_schedule;
    std::unordered_map<std::string, usize> execution_order_map;
    
    // Parallel execution groups
    std::vector<std::vector<std::string>> parallel_groups;
    usize required_thread_count{1};
    
    // Quality and performance trade-offs
    f32 overall_quality_factor{1.0f};   // Overall quality reduction
    f32 performance_safety_margin{2.0f}; // Extra time buffer (ms)
    bool uses_predictive_scheduling{false};
    
    // Statistics and validation
    f32 plan_confidence{1.0f};           // Confidence in this plan
    f32 expected_cpu_usage{0.5f};        // Expected CPU utilization
    f32 expected_memory_pressure{0.3f};  // Expected memory pressure
    
    // Educational information
    std::string optimization_strategy;   // Which strategy was used
    std::vector<std::string> applied_optimizations; // What optimizations were applied
    
    // Validation methods
    bool is_valid() const;
    bool is_achievable() const { return predicted_frame_time <= target_frame_time * 1.1f; }
    f32 efficiency_score() const;
    
    std::string to_string() const;
    void print_execution_plan() const;
};

/**
 * @brief Configuration for adaptive scheduler
 */
struct AdaptiveSchedulerConfig {
    // Scheduling strategy
    SchedulingStrategy strategy{SchedulingStrategy::AdaptiveHybrid};
    f32 target_frame_rate{60.0f};        // Target FPS
    f32 frame_time_tolerance{0.1f};      // Acceptable frame time variance
    
    // AI/ML settings
    MLModelConfig scheduling_model_config{
        .model_name = "SystemScheduler",
        .input_dimension = 20,  // Will be adjusted
        .output_dimension = 10, // System execution probabilities
        .learning_rate = 0.012f,
        .max_epochs = 400,
        .enable_training_visualization = true
    };
    
    // Performance thresholds
    f32 cpu_usage_threshold{0.8f};       // When to start load balancing
    f32 memory_pressure_threshold{0.7f}; // When to reduce memory-intensive systems
    f32 frame_skip_threshold{20.0f};     // Frame time that triggers skipping (ms)
    
    // Adaptation settings
    bool enable_quality_scaling{true};   // Allow quality reduction for performance
    bool enable_system_skipping{true};   // Allow skipping non-critical systems
    bool enable_parallel_execution{true}; // Use multiple threads
    usize max_thread_count{4};           // Maximum threads to use
    
    // Learning settings
    bool enable_online_learning{true};   // Learn from scheduling results
    usize learning_window_size{100};     // Frames to consider for learning
    f32 adaptation_rate{0.1f};           // How quickly to adapt to changes
    
    // Educational features
    bool enable_scheduling_visualization{true};
    bool track_optimization_effectiveness{true};
    bool enable_detailed_logging{false}; // Can be verbose
};

/**
 * @brief Statistics for adaptive scheduling
 */
struct AdaptiveSchedulingStats {
    // Frame rate statistics
    f32 average_frame_rate{60.0f};
    f32 frame_rate_variance{0.0f};
    f32 target_achievement_rate{1.0f};   // Percentage of frames that met target
    usize frames_processed{0};
    
    // System execution statistics
    std::unordered_map<std::string, usize> system_execution_counts;
    std::unordered_map<std::string, usize> system_skip_counts;
    std::unordered_map<std::string, f32> system_average_times;
    
    // Optimization effectiveness
    usize optimization_attempts{0};
    usize successful_optimizations{0};
    f32 performance_improvement{0.0f};   // Average improvement from optimizations
    f32 quality_preservation_rate{1.0f}; // How well quality was preserved
    
    // Scheduling accuracy
    usize scheduling_decisions{0};
    usize correct_predictions{0};
    f32 scheduling_accuracy{1.0f};
    f32 time_prediction_mae{0.0f};       // Mean absolute error for time predictions
    
    // Resource utilization
    f32 average_cpu_utilization{0.5f};
    f32 average_memory_usage{0.3f};
    f32 thread_utilization_efficiency{1.0f}; // How well threads are utilized
    
    void reset();
    void update_frame_stats(f32 frame_time, f32 target_time);
    void update_system_execution(const std::string& system_name, f32 execution_time, bool was_skipped);
    std::string to_string() const;
};

/**
 * @brief Adaptive ECS scheduler with AI-driven workload management
 * 
 * This system uses machine learning to optimize system execution order and timing,
 * balancing performance with quality. It learns from performance patterns and adapts
 * to changing workloads in real-time.
 */
class AdaptiveECSScheduler {
private:
    AdaptiveSchedulerConfig config_;
    std::unique_ptr<MLModelBase> scheduling_model_;
    std::unique_ptr<FeatureExtractor> feature_extractor_;
    std::unique_ptr<ECSPerformancePredictor> performance_predictor_;
    
    // System management
    std::unordered_map<std::string, SystemWorkloadProfile> system_profiles_;
    std::unordered_map<std::string, std::function<void()>> registered_systems_;
    std::vector<std::string> system_execution_order_;
    
    // Scheduling state
    FrameSchedulingPlan current_plan_;
    std::queue<FrameSchedulingPlan> plan_history_;
    std::atomic<usize> current_frame_number_{0};
    mutable std::mutex scheduling_mutex_;
    
    // Training data
    TrainingDataset scheduling_dataset_;
    std::vector<std::pair<FrameSchedulingPlan, f32>> plan_results_; // Plan + actual frame time
    
    // Performance monitoring
    std::vector<f32> frame_time_history_;
    std::chrono::high_resolution_clock::time_point frame_start_time_;
    AdaptiveSchedulingStats scheduling_stats_;
    
    // Thread management
    std::vector<std::unique_ptr<std::thread>> worker_threads_;
    std::queue<std::function<void()>> task_queue_;
    std::mutex task_queue_mutex_;
    std::condition_variable task_cv_;
    std::atomic<bool> should_stop_threads_{false};
    
    // Adaptation state
    f32 current_cpu_usage_{0.5f};
    f32 current_memory_pressure_{0.3f};
    f32 recent_performance_trend_{0.0f};
    bool is_performance_critical_{false};
    
public:
    explicit AdaptiveECSScheduler(const AdaptiveSchedulerConfig& config = AdaptiveSchedulerConfig{});
    ~AdaptiveECSScheduler();
    
    // Non-copyable but movable
    AdaptiveECSScheduler(const AdaptiveECSScheduler&) = delete;
    AdaptiveECSScheduler& operator=(const AdaptiveECSScheduler&) = delete;
    AdaptiveECSScheduler(AdaptiveECSScheduler&&) = default;
    AdaptiveECSScheduler& operator=(AdaptiveECSScheduler&&) = default;
    
    // System registration and management
    void register_system(const std::string& name, 
                        std::function<void()> system_function,
                        const SystemWorkloadProfile& profile = SystemWorkloadProfile{});
    void unregister_system(const std::string& name);
    void update_system_profile(const std::string& name, const SystemWorkloadProfile& profile);
    
    // Scheduling operations
    void start_scheduling();
    void stop_scheduling();
    void execute_frame(const ecs::Registry& registry);
    FrameSchedulingPlan create_scheduling_plan(const ecs::Registry& registry);
    void execute_scheduling_plan(const FrameSchedulingPlan& plan);
    
    // Performance integration
    void set_performance_predictor(std::unique_ptr<ECSPerformancePredictor> predictor);
    void update_performance_context(f32 cpu_usage, f32 memory_pressure);
    
    // Model training and adaptation
    bool train_scheduling_model();
    void learn_from_execution_results(const FrameSchedulingPlan& plan, f32 actual_frame_time);
    void adapt_to_current_conditions(const ecs::Registry& registry);
    
    // System profiling and analysis
    SystemWorkloadProfile profile_system(const std::string& system_name, usize sample_count = 100);
    void update_system_statistics(const std::string& system_name, f32 execution_time);
    std::vector<std::string> identify_bottleneck_systems() const;
    
    // Optimization strategies
    FrameSchedulingPlan optimize_for_performance(const FrameSchedulingPlan& base_plan);
    FrameSchedulingPlan optimize_for_quality(const FrameSchedulingPlan& base_plan);
    FrameSchedulingPlan balance_performance_and_quality(const FrameSchedulingPlan& base_plan);
    
    // Configuration and statistics
    const AdaptiveSchedulerConfig& config() const { return config_; }
    void update_config(const AdaptiveSchedulerConfig& new_config);
    const AdaptiveSchedulingStats& get_scheduling_statistics() const { return scheduling_stats_; }
    
    // System information
    std::vector<std::string> get_registered_systems() const;
    const SystemWorkloadProfile* get_system_profile(const std::string& name) const;
    std::vector<std::string> get_execution_order() const { return system_execution_order_; }
    
    // Educational features
    std::string generate_scheduling_report() const;
    std::string explain_scheduling_decision(const SystemSchedulingDecision& decision) const;
    void print_frame_analysis(const FrameSchedulingPlan& plan, f32 actual_time) const;
    std::string visualize_system_performance() const;
    std::string get_optimization_suggestions() const;
    
    // Advanced features
    void simulate_scheduling_strategies(const ecs::Registry& registry, usize frame_count = 100);
    std::unordered_map<SchedulingStrategy, f32> compare_scheduling_strategies(const ecs::Registry& registry);
    f32 predict_scalability(usize additional_entities, const ecs::Registry& registry);
    
    // Callbacks for integration
    using SchedulingCallback = std::function<void(const FrameSchedulingPlan&)>;
    using PerformanceCallback = std::function<void(f32 frame_time, f32 target_time)>;
    
    void set_scheduling_callback(SchedulingCallback callback) { scheduling_callback_ = callback; }
    void set_performance_callback(PerformanceCallback callback) { performance_callback_ = callback; }
    
private:
    // Internal implementation
    void initialize_models();
    void initialize_thread_pool();
    void cleanup_thread_pool();
    
    // Scheduling algorithms
    FrameSchedulingPlan create_fixed_order_plan(const ecs::Registry& registry);
    FrameSchedulingPlan create_dynamic_order_plan(const ecs::Registry& registry);
    FrameSchedulingPlan create_load_balanced_plan(const ecs::Registry& registry);
    FrameSchedulingPlan create_predictive_order_plan(const ecs::Registry& registry);
    FrameSchedulingPlan create_adaptive_hybrid_plan(const ecs::Registry& registry);
    
    // Feature extraction for ML
    FeatureVector extract_scheduling_features(const ecs::Registry& registry);
    TrainingSample create_scheduling_training_sample(const FrameSchedulingPlan& plan, f32 result_time);
    
    // Execution management
    void execute_system_serial(const std::string& system_name);
    void execute_systems_parallel(const std::vector<std::string>& system_names);
    void worker_thread_function();
    
    // Optimization helpers
    std::vector<std::string> determine_execution_dependencies() const;
    std::vector<std::vector<std::string>> identify_parallel_groups() const;
    f32 estimate_system_execution_time(const std::string& system_name, const ecs::Registry& registry) const;
    
    // Performance analysis
    void analyze_frame_performance(f32 frame_time);
    void update_system_profiles_from_execution();
    bool should_skip_system(const std::string& system_name, f32 remaining_budget) const;
    
    // Learning and adaptation
    void collect_training_data(const FrameSchedulingPlan& plan, f32 actual_time);
    void adapt_system_profiles();
    void update_scheduling_strategy_based_on_performance();
    
    // Utilities
    f32 calculate_plan_confidence(const FrameSchedulingPlan& plan) const;
    std::string generate_scheduling_reasoning(const SystemSchedulingDecision& decision) const;
    
    // Callbacks
    SchedulingCallback scheduling_callback_;
    PerformanceCallback performance_callback_;
};

/**
 * @brief Utility functions for adaptive scheduling
 */
namespace scheduling_utils {

// System analysis
f32 calculate_system_priority_score(const SystemWorkloadProfile& profile);
f32 estimate_parallel_efficiency(const std::vector<std::string>& systems,
                                const std::unordered_map<std::string, SystemWorkloadProfile>& profiles);
bool can_systems_run_parallel(const SystemWorkloadProfile& system1, const SystemWorkloadProfile& system2);

// Scheduling optimization
std::vector<std::string> optimize_execution_order(const std::vector<SystemWorkloadProfile>& systems);
f32 calculate_load_balance_score(const FrameSchedulingPlan& plan);
std::vector<std::string> suggest_system_optimizations(const SystemWorkloadProfile& profile);

// Educational utilities
std::string visualize_frame_schedule(const FrameSchedulingPlan& plan);
std::string explain_scheduling_strategy(SchedulingStrategy strategy);
std::string create_performance_timeline(const std::vector<f32>& frame_times);

} // namespace scheduling_utils

} // namespace ecscope::ml