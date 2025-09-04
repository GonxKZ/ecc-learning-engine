#pragma once

#include "script_engine.hpp"
#include "core/types.hpp"
#include "job_system/work_stealing_job_system.hpp"
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <functional>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <future>

namespace ecscope::scripting {

// Forward declarations
class ScriptJobScheduler;
class ParallelScriptExecutor;

/**
 * @brief Script execution job for the job system
 */
struct ScriptJob : public job_system::Job {
    std::string script_name;
    std::string function_name;
    std::vector<std::any> arguments;
    ScriptEngine* engine{nullptr};
    
    // Results and error handling
    std::promise<ScriptResult<std::any>> result_promise;
    std::future<ScriptResult<std::any>> result_future;
    
    // Performance metrics
    std::chrono::high_resolution_clock::time_point start_time;
    std::chrono::high_resolution_clock::time_point end_time;
    f64 execution_time_ms{0.0};
    usize memory_used{0};
    
    // Dependencies and synchronization
    std::vector<std::shared_ptr<ScriptJob>> dependencies;
    std::atomic<usize> pending_dependencies{0};
    std::condition_variable dependency_cv;
    std::mutex dependency_mutex;
    
    ScriptJob(const std::string& script, const std::string& function, ScriptEngine* script_engine)
        : script_name(script), function_name(function), engine(script_engine) {
        result_future = result_promise.get_future();
        start_time = std::chrono::high_resolution_clock::now();
    }
    
    void execute() override;
    
    void add_dependency(std::shared_ptr<ScriptJob> dependency);
    void wait_for_dependencies();
    void notify_completion();
    
    f64 get_execution_time_ms() const { return execution_time_ms; }
    bool is_completed() const { return result_future.wait_for(std::chrono::seconds(0)) == std::future_status::ready; }
};

/**
 * @brief Batch script execution job for processing multiple entities
 */
struct BatchScriptJob : public job_system::Job {
    struct EntityBatch {
        std::vector<ecs::Entity> entities;
        std::string script_function;
        std::unordered_map<std::string, std::any> batch_data;
    };
    
    std::string script_name;
    EntityBatch batch;
    ScriptEngine* engine{nullptr};
    ecs::Registry* registry{nullptr};
    
    // Batch processing configuration
    usize batch_size{100};
    bool parallel_entity_processing{true};
    
    // Results
    std::vector<ScriptResult<std::any>> entity_results;
    std::promise<std::vector<ScriptResult<std::any>>> batch_promise;
    std::future<std::vector<ScriptResult<std::any>>> batch_future;
    
    // Performance tracking
    f64 total_execution_time_ms{0.0};
    f64 average_entity_time_ms{0.0};
    usize successful_executions{0};
    usize failed_executions{0};
    
    BatchScriptJob(const std::string& script, ScriptEngine* script_engine, ecs::Registry* ecs_registry)
        : script_name(script), engine(script_engine), registry(ecs_registry) {
        batch_future = batch_promise.get_future();
    }
    
    void execute() override;
    void process_entity_batch();
    void collect_batch_results();
};

/**
 * @brief Configuration for parallel script execution
 */
struct ParallelExecutionConfig {
    // Threading configuration
    usize max_concurrent_scripts{4};           // Maximum scripts running in parallel
    usize script_worker_threads{2};            // Dedicated threads for script execution
    bool use_fiber_scheduling{false};          // Use fiber-based scheduling (advanced)
    
    // Batching configuration
    usize default_batch_size{100};             // Default entities per batch
    usize min_batch_size{10};                  // Minimum batch size before splitting
    usize max_batch_size{1000};                // Maximum batch size before splitting
    bool auto_batch_sizing{true};              // Automatically adjust batch sizes
    
    // Performance tuning
    f64 load_balancing_factor{0.8};            // Load balancing aggressiveness (0-1)
    f64 script_timeout_ms{5000.0};             // Maximum execution time per script
    bool enable_performance_monitoring{true};  // Track performance metrics
    bool enable_memory_monitoring{true};       // Monitor memory usage
    
    // Error handling
    bool fail_fast_on_error{false};            // Stop all execution on first error
    usize max_retry_attempts{3};               // Retry failed scripts
    f64 retry_delay_ms{100.0};                 // Delay between retries
    
    // Educational features
    bool enable_execution_tracing{true};       // Trace execution for learning
    bool generate_performance_reports{true};   // Generate performance analysis
    bool compare_sequential_performance{true}; // Compare with sequential execution
    
    static ParallelExecutionConfig create_development() {
        ParallelExecutionConfig config;
        config.max_concurrent_scripts = 2;
        config.enable_execution_tracing = true;
        config.generate_performance_reports = true;
        config.compare_sequential_performance = true;
        return config;
    }
    
    static ParallelExecutionConfig create_production() {
        ParallelExecutionConfig config;
        config.max_concurrent_scripts = 8;
        config.enable_execution_tracing = false;
        config.generate_performance_reports = false;
        config.compare_sequential_performance = false;
        config.use_fiber_scheduling = true;
        return config;
    }
    
    static ParallelExecutionConfig create_educational() {
        ParallelExecutionConfig config;
        config.max_concurrent_scripts = 4;
        config.enable_execution_tracing = true;
        config.generate_performance_reports = true;
        config.compare_sequential_performance = true;
        config.auto_batch_sizing = true;
        config.enable_performance_monitoring = true;
        config.enable_memory_monitoring = true;
        return config;
    }
};

/**
 * @brief Performance metrics for parallel script execution
 */
struct ParallelExecutionMetrics {
    // Execution statistics
    usize total_jobs_executed{0};
    usize successful_jobs{0};
    usize failed_jobs{0};
    usize retried_jobs{0};
    
    // Timing metrics
    f64 total_execution_time_ms{0.0};
    f64 average_job_time_ms{0.0};
    f64 fastest_job_time_ms{std::numeric_limits<f64>::max()};
    f64 slowest_job_time_ms{0.0};
    
    // Parallel efficiency
    f64 parallel_efficiency{0.0};              // Actual speedup / theoretical speedup
    f64 sequential_execution_time_ms{0.0};     // Time if executed sequentially
    f64 parallel_speedup{1.0};                 // Sequential time / parallel time
    f64 cpu_utilization{0.0};                  // Average CPU usage during execution
    
    // Memory metrics
    usize peak_memory_usage{0};
    usize average_memory_usage{0};
    usize memory_allocations{0};
    usize memory_deallocations{0};
    
    // Batching effectiveness
    f64 average_batch_size{0.0};
    usize total_batches{0};
    f64 batch_utilization{0.0};                // How full batches were on average
    
    // Educational insights
    std::vector<std::string> performance_insights;
    std::vector<std::string> optimization_suggestions;
    
    void reset() {
        *this = ParallelExecutionMetrics{};
    }
    
    f64 get_success_rate() const {
        return total_jobs_executed > 0 ? static_cast<f64>(successful_jobs) / total_jobs_executed : 0.0;
    }
    
    std::string generate_summary_report() const;
    std::string generate_detailed_report() const;
    void add_performance_insight(const std::string& insight);
    void add_optimization_suggestion(const std::string& suggestion);
};

/**
 * @brief Script job scheduler for parallel execution
 */
class ScriptJobScheduler {
public:
    explicit ScriptJobScheduler(job_system::WorkStealingJobSystem* job_system,
                               const ParallelExecutionConfig& config = ParallelExecutionConfig{});
    ~ScriptJobScheduler();
    
    // Job submission
    std::shared_ptr<ScriptJob> submit_script_job(ScriptEngine* engine,
                                                const std::string& script_name,
                                                const std::string& function_name,
                                                const std::vector<std::any>& args = {});
    
    std::shared_ptr<BatchScriptJob> submit_batch_job(ScriptEngine* engine,
                                                    ecs::Registry* registry,
                                                    const std::string& script_name,
                                                    const std::vector<ecs::Entity>& entities,
                                                    const std::string& function_name);
    
    // Job management
    void wait_for_job(std::shared_ptr<ScriptJob> job);
    void wait_for_all_jobs();
    void cancel_job(std::shared_ptr<ScriptJob> job);
    void cancel_all_jobs();
    
    // Batch processing
    void process_entities_parallel(ScriptEngine* engine,
                                 ecs::Registry* registry,
                                 const std::string& script_name,
                                 const std::string& function_name,
                                 const std::vector<ecs::Entity>& entities);
    
    // Performance monitoring
    const ParallelExecutionMetrics& get_metrics() const { return metrics_; }
    void reset_metrics() { metrics_.reset(); }
    
    // Configuration
    void set_config(const ParallelExecutionConfig& config);
    const ParallelExecutionConfig& get_config() const { return config_; }
    
    // Educational features
    void demonstrate_parallel_benefits(ScriptEngine* engine,
                                     const std::string& script_name,
                                     const std::vector<std::any>& test_data);
    
    void explain_parallel_execution_concepts() const;
    std::string analyze_script_parallelizability(ScriptEngine* engine, const std::string& script_name) const;

private:
    job_system::WorkStealingJobSystem* job_system_;
    ParallelExecutionConfig config_;
    ParallelExecutionMetrics metrics_;
    
    // Active job tracking
    std::vector<std::shared_ptr<ScriptJob>> active_jobs_;
    std::vector<std::shared_ptr<BatchScriptJob>> active_batch_jobs_;
    mutable std::mutex jobs_mutex_;
    
    // Performance monitoring
    std::chrono::high_resolution_clock::time_point session_start_time_;
    std::atomic<bool> monitoring_active_{false};
    std::thread monitoring_thread_;
    
    // Internal methods
    void update_metrics_for_job(const ScriptJob& job);
    void update_metrics_for_batch(const BatchScriptJob& batch_job);
    void optimize_batch_sizes();
    void balance_load_across_workers();
    
    // Monitoring thread
    void run_performance_monitoring();
    void collect_system_metrics();
    void analyze_job_performance();
    
    // Educational analysis
    void generate_performance_insights();
    void suggest_optimizations();
    f64 estimate_sequential_execution_time(const std::vector<std::shared_ptr<ScriptJob>>& jobs) const;
};

/**
 * @brief Parallel script executor with ECS integration
 */
class ParallelScriptExecutor {
public:
    explicit ParallelScriptExecutor(job_system::WorkStealingJobSystem* job_system,
                                   ecs::Registry* registry);
    ~ParallelScriptExecutor();
    
    // System execution
    void execute_system_parallel(ScriptEngine* engine,
                                const std::string& system_script,
                                const std::string& system_function);
    
    // Entity processing
    template<typename... Components>
    void for_each_parallel(ScriptEngine* engine,
                          const std::string& script_name,
                          const std::string& function_name,
                          usize batch_size = 100);
    
    // Component system parallel execution
    void update_transform_system_parallel(ScriptEngine* engine, const std::string& script_name);
    void update_physics_system_parallel(ScriptEngine* engine, const std::string& script_name);
    void update_rendering_system_parallel(ScriptEngine* engine, const std::string& script_name);
    
    // Advanced parallel patterns
    void execute_pipeline_parallel(ScriptEngine* engine,
                                  const std::vector<std::string>& pipeline_stages,
                                  const std::unordered_map<std::string, std::any>& shared_data);
    
    void execute_map_reduce(ScriptEngine* engine,
                           const std::string& map_script,
                           const std::string& reduce_script,
                           const std::vector<std::any>& input_data);
    
    // Performance comparison
    void benchmark_sequential_vs_parallel(ScriptEngine* engine,
                                        const std::string& script_name,
                                        const std::vector<std::any>& test_data,
                                        usize iterations = 10);
    
    // Educational demonstrations
    void demonstrate_race_conditions(ScriptEngine* engine);
    void demonstrate_load_balancing(ScriptEngine* engine);
    void demonstrate_memory_synchronization(ScriptEngine* engine);
    void demonstrate_script_parallelization_patterns(ScriptEngine* engine);

private:
    std::unique_ptr<ScriptJobScheduler> scheduler_;
    ecs::Registry* registry_;
    
    // Template method implementations
    template<typename... Components>
    std::vector<ecs::Entity> get_entities_with_components();
    
    void create_entity_batches(const std::vector<ecs::Entity>& entities,
                              usize batch_size,
                              std::vector<std::vector<ecs::Entity>>& batches);
};

/**
 * @brief Thread-safe script context manager for parallel execution
 */
class ThreadSafeScriptContext {
public:
    explicit ThreadSafeScriptContext(ScriptEngine* engine);
    ~ThreadSafeScriptContext();
    
    // Thread-safe script operations
    ScriptResult<std::any> execute_function_threadsafe(const std::string& script_name,
                                                      const std::string& function_name,
                                                      const std::vector<std::any>& args);
    
    // Context isolation
    void create_isolated_context(const std::string& context_name);
    void destroy_isolated_context(const std::string& context_name);
    void switch_to_context(const std::string& context_name);
    
    // Memory management
    void collect_garbage_threadsafe();
    usize get_memory_usage_threadsafe(const std::string& script_name) const;
    
    // Error handling
    void set_error_handler(std::function<void(const ScriptError&)> handler);
    std::vector<ScriptError> get_accumulated_errors() const;
    void clear_errors();

private:
    ScriptEngine* engine_;
    mutable std::shared_mutex context_mutex_;
    
    // Context isolation (implementation would depend on specific script engine)
    std::unordered_map<std::string, std::any> isolated_contexts_;
    std::string current_context_;
    
    // Error accumulation
    std::vector<ScriptError> accumulated_errors_;
    std::function<void(const ScriptError&)> error_handler_;
    mutable std::mutex error_mutex_;
    
    // Thread-local storage for script execution
    thread_local static std::unique_ptr<std::any> thread_script_state_;
};

/**
 * @brief Educational parallel scripting examples
 */
class ParallelScriptingEducation {
public:
    static void create_parallel_examples();
    static void demonstrate_threading_concepts();
    static void show_performance_benefits();
    static void explain_common_pitfalls();
    
    // Example generators
    static void generate_parallel_entity_processing_example();
    static void generate_producer_consumer_example();
    static void generate_parallel_physics_example();
    static void generate_load_balancing_example();
    
    // Educational utilities
    static std::string explain_parallel_execution_model();
    static std::string explain_job_system_integration();
    static std::string explain_memory_safety_in_parallel_scripts();
    static std::vector<std::string> list_parallelization_patterns();

private:
    static void create_example_script(const std::string& name, const std::string& content, const std::string& description);
    static void create_benchmark_comparison(const std::string& name, const std::string& sequential_version, const std::string& parallel_version);
};

// Template implementations
template<typename... Components>
void ParallelScriptExecutor::for_each_parallel(ScriptEngine* engine,
                                              const std::string& script_name,
                                              const std::string& function_name,
                                              usize batch_size) {
    auto entities = get_entities_with_components<Components...>();
    
    if (entities.empty()) {
        return;
    }
    
    // Create batches
    std::vector<std::vector<ecs::Entity>> batches;
    create_entity_batches(entities, batch_size, batches);
    
    // Submit batch jobs
    std::vector<std::shared_ptr<BatchScriptJob>> jobs;
    jobs.reserve(batches.size());
    
    for (auto& batch : batches) {
        auto job = scheduler_->submit_batch_job(engine, registry_, script_name, batch, function_name);
        jobs.push_back(job);
    }
    
    // Wait for all batches to complete
    for (auto& job : jobs) {
        scheduler_->wait_for_job(std::static_pointer_cast<ScriptJob>(job));
    }
}

template<typename... Components>
std::vector<ecs::Entity> ParallelScriptExecutor::get_entities_with_components() {
    if (!registry_) {
        return {};
    }
    
    return registry_->get_entities_with<Components...>();
}

} // namespace ecscope::scripting