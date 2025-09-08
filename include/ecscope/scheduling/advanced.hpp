#pragma once

/**
 * @file advanced.hpp
 * @brief Advanced scheduling features including budget management, checkpointing, and multi-frame pipelining
 * 
 * This comprehensive advanced scheduling module provides world-class scheduling capabilities
 * and optimization features for professional-grade system scheduling:
 * 
 * - Multi-frame pipelining for overlapped execution
 * - System execution budget management with time slicing
 * - State checkpointing and rollback capabilities
 * - Dynamic load balancing across CPU cores
 * - Predictive scheduling based on performance history
 * - System execution path optimization
 * - Resource contention prediction and mitigation
 * - Adaptive scheduling parameters
 * - Event-driven conditional system triggers
 * - System execution replay and analysis
 * - Performance regression detection
 * - Automatic system optimization
 * 
 * Advanced Features:
 * - Frame-level execution pipelining
 * - Hierarchical time budget allocation
 * - System priority boost/decay algorithms
 * - Critical path scheduling optimization
 * - Resource conflict prediction and avoidance
 * - Dynamic thread pool scaling
 * - NUMA-aware system placement
 * - Cache-friendly system batching
 * 
 * Optimization Capabilities:
 * - Machine learning-based scheduling hints
 * - Historical performance analysis
 * - Workload prediction and adaptation
 * - Dynamic system reordering
 * - Resource usage optimization
 * - Latency minimization algorithms
 * - Throughput maximization strategies
 * 
 * @author ECScope Professional ECS Framework
 * @date 2024
 */

#include "../core/types.hpp"
#include "../core/log.hpp"
#include "../core/time.hpp"
#include "execution_context.hpp"
#include "profiling.hpp"
#include <memory>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <string>
#include <functional>
#include <atomic>
#include <mutex>
#include <shared_mutex>
#include <condition_variable>
#include <thread>
#include <future>
#include <chrono>
#include <queue>
#include <deque>
#include <stack>
#include <optional>
#include <variant>
#include <algorithm>
#include <numeric>
#include <cmath>
#include <fstream>

namespace ecscope::scheduling {

// Forward declarations
class System;
class SystemManager;
class Scheduler;
class ManagedSystem;

using ecscope::core::u8;
using ecscope::core::u16;
using ecscope::core::u32;
using ecscope::core::u64;
using ecscope::core::f32;
using ecscope::core::f64;
using ecscope::core::usize;

/**
 * @brief Time budget allocation strategy for system scheduling
 */
enum class BudgetAllocationStrategy : u8 {
    Equal = 0,              // Equal time budget for all systems
    Weighted,               // Budget based on system weights/priorities
    Adaptive,               // Budget adapts based on historical performance
    Predictive,             // Budget based on predicted execution times
    Dynamic,                // Budget changes during runtime based on workload
    Proportional            // Budget proportional to system complexity/requirements
};

/**
 * @brief Frame pipelining mode for multi-frame execution
 */
enum class PipeliningMode : u8 {
    Disabled = 0,           // No pipelining
    Simple,                 // Simple double-buffering
    Triple,                 // Triple-buffering for smooth execution
    Adaptive,               // Adaptive pipelining based on frame time
    Aggressive              // Maximum overlap with dependency satisfaction
};

/**
 * @brief System execution budget with time slicing and priority management
 */
class ExecutionBudget {
private:
    // Budget configuration
    f64 allocated_time_ns_;         // Allocated time budget in nanoseconds
    f64 consumed_time_ns_;          // Time consumed so far
    f64 reserved_time_ns_;          // Reserved time for critical operations
    f64 overtime_allowance_ns_;     // Allowed overtime before penalties
    
    // Time slicing
    f64 slice_duration_ns_;         // Duration of each time slice
    u32 total_slices_;              // Total number of slices
    u32 consumed_slices_;           // Number of slices consumed
    
    // Priority and penalties
    f32 priority_multiplier_;       // Priority-based budget multiplier
    f64 penalty_accumulation_;      // Accumulated penalty for budget overruns
    f64 bonus_accumulation_;        // Accumulated bonus for under-budget execution
    
    // Performance tracking
    std::vector<f64> execution_history_;    // Historical execution times
    f64 predicted_execution_time_;          // Predicted execution time
    f64 confidence_level_;                  // Confidence in prediction
    
    // Adaptive parameters
    f64 adaptation_rate_;           // How quickly budget adapts to changes
    f64 safety_margin_;             // Safety margin for budget allocation
    bool enable_prediction_;        // Enable predictive budget allocation
    
    mutable std::mutex budget_mutex_;

public:
    ExecutionBudget(f64 allocated_time_seconds = 0.016, u32 num_slices = 1);
    
    // Budget management
    bool try_consume_time(f64 time_ns);
    void consume_time(f64 time_ns);
    void release_unused_time(f64 time_ns);
    bool has_available_time(f64 required_time_ns) const;
    f64 get_remaining_time() const;
    f64 get_utilization_percent() const;
    
    // Time slicing
    bool try_consume_slice();
    void release_slice();
    u32 get_remaining_slices() const;
    f64 get_slice_duration() const { return slice_duration_ns_; }
    void set_slice_duration(f64 duration_ns) { slice_duration_ns_ = duration_ns; }
    
    // Priority management
    void set_priority_multiplier(f32 multiplier);
    f32 get_priority_multiplier() const { return priority_multiplier_; }
    void boost_priority(f32 boost_amount);
    void decay_priority(f32 decay_rate);
    
    // Performance tracking
    void record_execution_time(f64 execution_time_ns);
    f64 get_predicted_execution_time() const { return predicted_execution_time_; }
    f64 get_prediction_confidence() const { return confidence_level_; }
    void update_prediction();
    
    // Penalties and bonuses
    void apply_penalty(f64 penalty_amount);
    void apply_bonus(f64 bonus_amount);
    f64 get_net_penalty_bonus() const { return bonus_accumulation_ - penalty_accumulation_; }
    
    // Budget adaptation
    void adapt_budget_size(f64 new_size_seconds);
    void set_adaptation_rate(f64 rate) { adaptation_rate_ = rate; }
    void set_safety_margin(f64 margin) { safety_margin_ = margin; }
    void enable_prediction(bool enable) { enable_prediction_ = enable; }
    
    // Information
    f64 get_allocated_time() const { return allocated_time_ns_; }
    f64 get_consumed_time() const { return consumed_time_ns_; }
    f64 get_reserved_time() const { return reserved_time_ns_; }
    bool is_over_budget() const { return consumed_time_ns_ > allocated_time_ns_; }
    bool is_critically_over_budget() const { 
        return consumed_time_ns_ > (allocated_time_ns_ + overtime_allowance_ns_); 
    }
    
    // Reset and configuration
    void reset_budget();
    void configure(f64 allocated_time_seconds, u32 num_slices, f32 priority_multiplier = 1.0f);
    
    // Statistics
    struct Statistics {
        f64 average_utilization;
        u32 total_overruns;
        f64 total_penalty;
        f64 total_bonus;
        f64 prediction_accuracy;
        usize history_size;
    };
    Statistics get_statistics() const;
    
private:
    void apply_priority_adjustment();
    void update_prediction_confidence();
    f64 calculate_trend() const;
    void cleanup_old_history();
};

/**
 * @brief System state checkpoint for rollback and recovery
 */
class SystemCheckpoint {
private:
    // Checkpoint metadata
    std::string checkpoint_name_;
    u64 checkpoint_timestamp_;
    u64 frame_number_;
    f64 frame_time_;
    
    // System states
    std::unordered_map<u32, std::vector<u8>> system_states_;
    std::unordered_map<u32, SystemLifecycleState> lifecycle_states_;
    
    // Resource states
    std::unordered_map<u32, std::vector<u8>> resource_states_;
    std::unordered_map<u32, bool> resource_lock_states_;
    
    // Performance states
    std::unordered_map<u32, ExecutionBudget> budget_states_;
    std::unordered_map<u32, f64> performance_baselines_;
    
    // Dependency states
    std::vector<std::pair<u32, u32>> dependency_graph_edges_;
    std::unordered_map<u32, std::vector<u32>> dependency_resolution_state_;
    
    // Configuration states
    std::unordered_map<u32, std::vector<u8>> system_configurations_;
    
    mutable std::shared_mutex checkpoint_mutex_;

public:
    SystemCheckpoint(const std::string& name, u64 frame_number, f64 frame_time);
    
    // Checkpoint management
    void capture_system_state(u32 system_id, const ManagedSystem& system);
    void capture_resource_state(u32 resource_id, const void* resource_data, usize data_size);
    void capture_budget_state(u32 system_id, const ExecutionBudget& budget);
    void capture_dependency_state(const std::vector<std::pair<u32, u32>>& edges);
    
    // State restoration
    bool restore_system_state(u32 system_id, ManagedSystem& system) const;
    bool restore_resource_state(u32 resource_id, void* resource_data, usize data_size) const;
    bool restore_budget_state(u32 system_id, ExecutionBudget& budget) const;
    bool restore_dependency_state(std::vector<std::pair<u32, u32>>& edges) const;
    
    // Information
    const std::string& name() const { return checkpoint_name_; }
    u64 timestamp() const { return checkpoint_timestamp_; }
    u64 frame_number() const { return frame_number_; }
    f64 frame_time() const { return frame_time_; }
    usize system_count() const;
    usize resource_count() const;
    usize total_size_bytes() const;
    
    // Validation
    bool is_valid() const;
    bool contains_system(u32 system_id) const;
    bool contains_resource(u32 resource_id) const;
    std::vector<u32> get_captured_systems() const;
    std::vector<u32> get_captured_resources() const;
    
    // Serialization
    std::vector<u8> serialize() const;
    static std::unique_ptr<SystemCheckpoint> deserialize(const std::vector<u8>& data);
    void save_to_file(const std::string& filename) const;
    static std::unique_ptr<SystemCheckpoint> load_from_file(const std::string& filename);
    
    // Comparison
    f64 similarity_score(const SystemCheckpoint& other) const;
    std::vector<u32> get_differing_systems(const SystemCheckpoint& other) const;
    
private:
    template<typename T>
    std::vector<u8> serialize_object(const T& obj) const;
    
    template<typename T>
    bool deserialize_object(const std::vector<u8>& data, T& obj) const;
    
    void validate_checkpoint_integrity() const;
};

/**
 * @brief Multi-frame execution pipeline for overlapped system execution
 */
class ExecutionPipeline {
private:
    // Pipeline configuration
    PipeliningMode mode_;
    u32 pipeline_depth_;
    f64 frame_overlap_ratio_;       // How much frames can overlap (0.0-1.0)
    bool adaptive_depth_;           // Automatically adjust pipeline depth
    
    // Frame management
    struct PipelineFrame {
        u64 frame_number;
        f64 frame_time;
        f64 start_time;
        f64 estimated_completion_time;
        std::vector<u32> systems_to_execute;
        std::atomic<usize> completed_systems{0};
        std::atomic<bool> frame_complete{false};
        std::unique_ptr<SystemCheckpoint> pre_execution_checkpoint;
        std::unordered_map<u32, std::future<void>> system_futures;
        
        PipelineFrame(u64 num, f64 time) : frame_number(num), frame_time(time), start_time(time) {}
    };
    
    std::deque<std::unique_ptr<PipelineFrame>> active_frames_;
    std::queue<std::unique_ptr<PipelineFrame>> completed_frames_;
    mutable std::mutex frames_mutex_;
    
    // Dependency tracking across frames
    std::unordered_map<u32, std::vector<u64>> system_frame_dependencies_;
    std::unordered_map<u64, std::unordered_set<u32>> frame_blocking_systems_;
    mutable std::shared_mutex dependencies_mutex_;
    
    // Performance tracking
    std::atomic<f64> pipeline_efficiency_;      // 0.0-1.0, higher is better
    std::atomic<f64> average_frame_overlap_;    // Average overlap between frames
    std::atomic<u32> pipeline_stalls_;          // Number of pipeline stalls
    std::atomic<u64> total_frames_processed_;
    
    // Resource contention handling
    std::unordered_map<u32, std::queue<u64>> resource_wait_queues_;
    mutable std::mutex resource_queues_mutex_;
    
    // Configuration
    bool enable_checkpointing_;
    bool enable_frame_skipping_;
    f64 max_frame_latency_seconds_;
    u32 max_concurrent_frames_;

public:
    ExecutionPipeline(PipeliningMode mode = PipeliningMode::Simple, u32 depth = 2);
    ~ExecutionPipeline();
    
    // Pipeline management
    void initialize(u32 initial_depth);
    void shutdown();
    void configure(PipeliningMode mode, u32 depth, f64 overlap_ratio = 0.5);
    
    // Frame execution
    bool begin_frame(u64 frame_number, f64 frame_time, const std::vector<u32>& systems);
    void execute_frame_systems(u64 frame_number, SystemManager* system_manager);
    bool is_frame_complete(u64 frame_number) const;
    void wait_for_frame_completion(u64 frame_number);
    std::unique_ptr<PipelineFrame> complete_frame(u64 frame_number);
    
    // Pipeline control
    void flush_pipeline();
    void stall_pipeline();
    void resume_pipeline();
    void adjust_pipeline_depth(u32 new_depth);
    void optimize_pipeline_parameters();
    
    // Dependency management
    void add_frame_dependency(u32 system_id, u64 frame_number);
    void remove_frame_dependency(u32 system_id, u64 frame_number);
    bool are_frame_dependencies_satisfied(u32 system_id, u64 frame_number) const;
    std::vector<u64> get_blocking_frames(u32 system_id) const;
    
    // Resource contention
    bool try_acquire_resource_for_frame(u32 resource_id, u64 frame_number);
    void release_resource_from_frame(u32 resource_id, u64 frame_number);
    void handle_resource_contention(u32 resource_id, u64 requesting_frame);
    
    // Checkpointing
    void enable_checkpointing(bool enable) { enable_checkpointing_ = enable; }
    std::unique_ptr<SystemCheckpoint> get_frame_checkpoint(u64 frame_number) const;
    bool rollback_to_frame(u64 frame_number);
    
    // Performance monitoring
    f64 get_pipeline_efficiency() const { return pipeline_efficiency_.load(); }
    f64 get_average_frame_overlap() const { return average_frame_overlap_.load(); }
    u32 get_pipeline_stalls() const { return pipeline_stalls_.load(); }
    f64 get_throughput_fps() const;
    f64 get_average_frame_latency() const;
    
    // Configuration
    void set_max_frame_latency(f64 latency_seconds) { max_frame_latency_seconds_ = latency_seconds; }
    void set_max_concurrent_frames(u32 max_frames) { max_concurrent_frames_ = max_frames; }
    void set_frame_skipping_enabled(bool enabled) { enable_frame_skipping_ = enabled; }
    void set_adaptive_depth_enabled(bool enabled) { adaptive_depth_ = enabled; }
    
    // Information
    PipeliningMode mode() const { return mode_; }
    u32 pipeline_depth() const { return pipeline_depth_; }
    usize active_frames_count() const;
    usize completed_frames_count() const;
    std::vector<u64> get_active_frame_numbers() const;
    
    // Analysis
    struct PipelineStatistics {
        f64 efficiency;
        f64 average_overlap;
        u32 total_stalls;
        u64 frames_processed;
        f64 throughput_fps;
        f64 average_latency;
        u32 current_depth;
        usize active_frames;
    };
    PipelineStatistics get_statistics() const;
    void reset_statistics();
    
private:
    // Internal frame management
    PipelineFrame* find_frame(u64 frame_number);
    const PipelineFrame* find_frame(u64 frame_number) const;
    void cleanup_completed_frames();
    void detect_and_handle_stalls();
    
    // Performance optimization
    void update_pipeline_efficiency();
    void calculate_frame_overlap();
    void adapt_pipeline_depth_automatically();
    bool should_skip_frame(u64 frame_number) const;
    
    // Dependency resolution
    void resolve_frame_dependencies(u64 frame_number);
    void update_blocking_systems(u64 frame_number);
    
    // Utility functions
    f64 estimate_frame_completion_time(const PipelineFrame& frame) const;
    bool is_pipeline_overloaded() const;
    void handle_pipeline_overload();
    
    static f64 get_current_time_seconds();
};

/**
 * @brief Budget manager for global time budget allocation and management
 */
class BudgetManager {
private:
    // Global budget configuration
    f64 frame_time_budget_seconds_;     // Total frame time budget
    f64 reserved_budget_percent_;       // Reserved budget for critical systems
    BudgetAllocationStrategy strategy_; // Budget allocation strategy
    
    // System budgets
    std::unordered_map<u32, std::unique_ptr<ExecutionBudget>> system_budgets_;
    std::unordered_map<SystemPhase, f64> phase_budgets_;
    mutable std::shared_mutex budgets_mutex_;
    
    // Budget allocation weights
    std::unordered_map<u32, f32> system_weights_;
    std::unordered_map<u32, f32> priority_weights_;
    std::unordered_map<u32, f32> historical_weights_;
    
    // Performance tracking
    std::atomic<f64> total_budget_utilization_;
    std::atomic<u32> budget_overruns_;
    std::atomic<f64> average_slack_time_;
    std::vector<f64> utilization_history_;
    mutable std::mutex history_mutex_;
    
    // Adaptive parameters
    f64 weight_adaptation_rate_;
    f64 emergency_budget_percent_;      // Emergency budget for overruns
    bool enable_dynamic_reallocation_;
    
    // Budget alerts
    std::function<void(u32, f64)> overrun_callback_;
    std::function<void(f64)> low_budget_callback_;
    f64 alert_threshold_percent_;

public:
    BudgetManager(f64 frame_budget_seconds = 0.016, // 16ms for 60 FPS
                  BudgetAllocationStrategy strategy = BudgetAllocationStrategy::Adaptive);
    ~BudgetManager();
    
    // Budget allocation
    void allocate_budget_to_system(u32 system_id, f64 budget_seconds, f32 weight = 1.0f);
    void allocate_budget_to_phase(SystemPhase phase, f64 budget_seconds);
    void reallocate_all_budgets();
    void redistribute_unused_budget();
    
    // Budget access
    ExecutionBudget* get_system_budget(u32 system_id) const;
    f64 get_phase_budget(SystemPhase phase) const;
    f64 get_remaining_frame_budget() const;
    f64 get_total_allocated_budget() const;
    
    // Dynamic reallocation
    void enable_dynamic_reallocation(bool enable) { enable_dynamic_reallocation_ = enable; }
    void perform_dynamic_reallocation();
    void boost_system_budget(u32 system_id, f64 boost_percent);
    void throttle_system_budget(u32 system_id, f64 throttle_percent);
    
    // Weight management
    void set_system_weight(u32 system_id, f32 weight);
    void update_system_weights_from_performance();
    void decay_system_weights(f32 decay_rate = 0.95f);
    void normalize_system_weights();
    
    // Budget strategy
    void set_allocation_strategy(BudgetAllocationStrategy strategy);
    void apply_equal_allocation();
    void apply_weighted_allocation();
    void apply_adaptive_allocation();
    void apply_predictive_allocation();
    void apply_proportional_allocation();
    
    // Budget monitoring
    void record_system_execution(u32 system_id, f64 execution_time);
    void handle_budget_overrun(u32 system_id, f64 overrun_time);
    void update_budget_statistics();
    
    // Alerts and callbacks
    void set_overrun_callback(std::function<void(u32, f64)> callback) {
        overrun_callback_ = std::move(callback);
    }
    void set_low_budget_callback(std::function<void(f64)> callback) {
        low_budget_callback_ = std::move(callback);
    }
    void set_alert_threshold(f64 threshold_percent) { alert_threshold_percent_ = threshold_percent; }
    
    // Configuration
    void set_frame_budget(f64 budget_seconds);
    void set_reserved_budget_percent(f64 percent) { reserved_budget_percent_ = percent; }
    void set_emergency_budget_percent(f64 percent) { emergency_budget_percent_ = percent; }
    void set_weight_adaptation_rate(f64 rate) { weight_adaptation_rate_ = rate; }
    
    // Information
    f64 get_frame_budget() const { return frame_time_budget_seconds_; }
    f64 get_budget_utilization() const { return total_budget_utilization_.load(); }
    u32 get_budget_overruns() const { return budget_overruns_.load(); }
    f64 get_average_slack_time() const { return average_slack_time_.load(); }
    BudgetAllocationStrategy get_strategy() const { return strategy_; }
    
    // Analysis
    std::vector<u32> get_over_budget_systems() const;
    std::vector<u32> get_under_budget_systems() const;
    f64 calculate_budget_efficiency() const;
    std::unordered_map<u32, f64> get_system_utilizations() const;
    
    // Statistics
    struct BudgetStatistics {
        f64 total_utilization;
        u32 total_overruns;
        f64 average_slack;
        usize systems_over_budget;
        usize systems_under_budget;
        f64 efficiency_score;
        f64 variance;
    };
    BudgetStatistics get_statistics() const;
    void reset_statistics();
    
    // Export and reporting
    std::string generate_budget_report() const;
    void export_budget_data(const std::string& filename) const;
    std::vector<std::pair<std::string, f64>> get_budget_recommendations() const;

private:
    // Internal allocation algorithms
    void allocate_equal();
    void allocate_weighted();
    void allocate_adaptive();
    void allocate_predictive();
    void allocate_proportional();
    
    // Utility functions
    f64 calculate_system_weight(u32 system_id) const;
    f64 predict_system_execution_time(u32 system_id) const;
    void update_historical_weights();
    void validate_budget_allocations() const;
    void handle_emergency_budget_situation();
    
    // Performance analysis
    void analyze_utilization_patterns();
    void detect_budget_anomalies();
    void optimize_budget_allocation();
};

/**
 * @brief Advanced scheduler controller providing high-level scheduling orchestration
 */
class AdvancedSchedulerController {
private:
    // Core components
    std::unique_ptr<BudgetManager> budget_manager_;
    std::unique_ptr<ExecutionPipeline> execution_pipeline_;
    SystemManager* system_manager_;
    Scheduler* scheduler_;
    
    // Checkpointing system
    std::unordered_map<std::string, std::unique_ptr<SystemCheckpoint>> checkpoints_;
    std::string active_checkpoint_;
    mutable std::shared_mutex checkpoints_mutex_;
    
    // Advanced scheduling features
    bool enable_predictive_scheduling_;
    bool enable_load_balancing_;
    bool enable_adaptive_optimization_;
    std::atomic<f64> scheduling_efficiency_;
    
    // Performance optimization
    std::thread optimization_thread_;
    std::atomic<bool> run_optimization_;
    f64 optimization_interval_seconds_;
    
    // Event-driven scheduling
    std::unordered_map<std::string, std::vector<u32>> event_triggered_systems_;
    std::queue<std::string> pending_events_;
    mutable std::mutex events_mutex_;
    
    // Configuration
    bool enable_multi_frame_pipelining_;
    bool enable_system_checkpointing_;
    bool enable_budget_management_;
    f64 target_frame_time_seconds_;

public:
    AdvancedSchedulerController(SystemManager* system_manager, Scheduler* scheduler);
    ~AdvancedSchedulerController();
    
    // Initialization and configuration
    void initialize(f64 target_frame_time = 0.016);
    void shutdown();
    void configure_advanced_features(bool pipelining = true, bool checkpointing = true, 
                                   bool budget_management = true);
    
    // Budget management
    BudgetManager* get_budget_manager() const { return budget_manager_.get(); }
    void allocate_system_budget(u32 system_id, f64 budget_seconds);
    void enable_dynamic_budget_reallocation(bool enable);
    
    // Execution pipelining
    ExecutionPipeline* get_execution_pipeline() const { return execution_pipeline_.get(); }
    void configure_pipelining(PipeliningMode mode, u32 depth, f64 overlap = 0.5);
    void enable_pipelining(bool enable) { enable_multi_frame_pipelining_ = enable; }
    
    // Checkpointing
    void create_system_checkpoint(const std::string& name);
    bool restore_system_checkpoint(const std::string& name);
    void clear_checkpoints();
    std::vector<std::string> get_available_checkpoints() const;
    
    // Advanced execution
    void execute_advanced_frame(u64 frame_number, f64 frame_time);
    void execute_with_pipelining(u64 frame_number, f64 frame_time);
    void execute_with_budget_management(u64 frame_number, f64 frame_time);
    void execute_with_checkpointing(u64 frame_number, f64 frame_time);
    
    // Event-driven scheduling
    void register_event_triggered_system(const std::string& event_name, u32 system_id);
    void unregister_event_triggered_system(const std::string& event_name, u32 system_id);
    void trigger_event(const std::string& event_name);
    void process_pending_events();
    
    // Performance optimization
    void enable_predictive_scheduling(bool enable) { enable_predictive_scheduling_ = enable; }
    void enable_load_balancing(bool enable) { enable_load_balancing_ = enable; }
    void enable_adaptive_optimization(bool enable) { enable_adaptive_optimization_ = enable; }
    void perform_optimization_pass();
    void set_optimization_interval(f64 interval_seconds) { optimization_interval_seconds_ = interval_seconds; }
    
    // Analysis and monitoring
    f64 get_scheduling_efficiency() const { return scheduling_efficiency_.load(); }
    std::string generate_comprehensive_report() const;
    void export_performance_analysis(const std::string& filename) const;
    
    // Configuration
    void set_target_frame_time(f64 frame_time_seconds) { target_frame_time_seconds_ = frame_time_seconds; }
    f64 get_target_frame_time() const { return target_frame_time_seconds_; }
    
private:
    // Optimization thread
    void optimization_thread_function();
    void optimize_system_scheduling();
    void optimize_resource_allocation();
    void optimize_pipeline_parameters();
    
    // Advanced scheduling algorithms
    void apply_predictive_scheduling();
    void apply_load_balancing();
    void apply_adaptive_optimization();
    
    // Performance analysis
    void analyze_scheduling_performance();
    void detect_scheduling_bottlenecks();
    void calculate_scheduling_efficiency();
    
    // Utility functions
    void validate_advanced_configuration() const;
    void cleanup_expired_checkpoints();
};

} // namespace ecscope::scheduling