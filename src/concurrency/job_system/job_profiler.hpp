#pragma once

/**
 * @file job_profiler.hpp
 * @brief Advanced Job System Profiler and Educational Visualization
 * 
 * This comprehensive profiling system provides detailed performance analysis
 * and educational insights into parallel job execution patterns.
 * 
 * Key Features:
 * - Real-time job execution monitoring and profiling
 * - Work-stealing pattern analysis and visualization
 * - Thread utilization and load balancing metrics
 * - Cache performance and NUMA locality analysis
 * - Educational timeline visualization of job execution
 * - Performance bottleneck identification and suggestions
 * - Comparative analysis between sequential and parallel execution
 * 
 * Educational Value:
 * - Visual understanding of parallel execution patterns
 * - Clear demonstration of work-stealing effectiveness
 * - Performance impact of different scheduling strategies
 * - Learning about multi-threading concepts through real examples
 * - Understanding of hardware effects (cache, NUMA) on performance
 * 
 * Visualization Features:
 * - Interactive timeline showing job execution across threads
 * - Work-stealing pattern visualization
 * - Load distribution charts and histograms
 * - Performance comparison graphs
 * - Real-time monitoring dashboard
 * 
 * @author ECScope Educational ECS Framework - Job Profiler
 * @date 2025
 */

#include "work_stealing_job_system.hpp"
#include "core/types.hpp"
#include <chrono>
#include <vector>
#include <unordered_map>
#include <string>
#include <fstream>
#include <mutex>
#include <atomic>
#include <deque>
#include <memory>

namespace ecscope::job_system {

//=============================================================================
// Profiling Data Structures
//=============================================================================

/**
 * @brief Individual job execution record for profiling
 */
struct JobExecutionRecord {
    JobID job_id;
    std::string job_name;
    u32 worker_id;
    u32 cpu_core;
    u32 numa_node;
    
    // Timing information
    std::chrono::high_resolution_clock::time_point submit_time;
    std::chrono::high_resolution_clock::time_point start_time;
    std::chrono::high_resolution_clock::time_point end_time;
    
    // Execution characteristics
    JobPriority priority;
    JobAffinity affinity;
    bool was_stolen = false;
    u32 steal_source_worker = 0;
    
    // Performance metrics
    usize memory_allocated = 0;
    u64 instructions_executed = 0;
    u64 cache_misses = 0;
    f64 cpu_utilization = 0.0;
    
    // Helper functions
    f64 queue_time_ms() const {
        return std::chrono::duration<f64, std::milli>(start_time - submit_time).count();
    }
    
    f64 execution_time_ms() const {
        return std::chrono::duration<f64, std::milli>(end_time - start_time).count();
    }
    
    f64 total_time_ms() const {
        return std::chrono::duration<f64, std::milli>(end_time - submit_time).count();
    }
};

/**
 * @brief Work-stealing event record
 */
struct StealEvent {
    std::chrono::high_resolution_clock::time_point timestamp;
    u32 thief_worker_id;
    u32 victim_worker_id;
    JobID stolen_job_id;
    std::string job_name;
    bool successful;
    
    StealEvent(u32 thief, u32 victim, JobID job_id, const std::string& name, bool success)
        : timestamp(std::chrono::high_resolution_clock::now())
        , thief_worker_id(thief), victim_worker_id(victim)
        , stolen_job_id(job_id), job_name(name), successful(success) {}
};

/**
 * @brief Thread utilization metrics over time
 */
struct ThreadUtilizationSample {
    std::chrono::high_resolution_clock::time_point timestamp;
    u32 worker_id;
    bool is_working;
    std::string current_job_name;
    f64 cpu_usage_percent;
    usize queue_size;
    
    ThreadUtilizationSample(u32 worker, bool working, const std::string& job = "")
        : timestamp(std::chrono::high_resolution_clock::now())
        , worker_id(worker), is_working(working), current_job_name(job)
        , cpu_usage_percent(0.0), queue_size(0) {}
};

/**
 * @brief Performance frame capturing system-wide metrics
 */
struct PerformanceFrame {
    std::chrono::high_resolution_clock::time_point frame_start;
    std::chrono::high_resolution_clock::time_point frame_end;
    u64 frame_number;
    
    // Job statistics for this frame
    u32 jobs_submitted = 0;
    u32 jobs_completed = 0;
    u32 jobs_stolen = 0;
    u32 total_steal_attempts = 0;
    
    // Thread utilization
    std::vector<f64> thread_utilization_percent;
    f64 average_utilization = 0.0;
    f64 load_balance_coefficient = 1.0; // 1.0 = perfect balance
    
    // Performance metrics
    f64 total_execution_time_ms = 0.0;
    f64 total_queue_time_ms = 0.0;
    f64 throughput_jobs_per_sec = 0.0;
    
    // Memory and cache metrics
    usize total_memory_allocated = 0;
    u64 total_cache_misses = 0;
    f64 cache_hit_rate = 0.0;
    
    PerformanceFrame(u64 frame_num) 
        : frame_start(std::chrono::high_resolution_clock::now())
        , frame_number(frame_num) {}
    
    void finalize() {
        frame_end = std::chrono::high_resolution_clock::now();
        f64 frame_duration_sec = std::chrono::duration<f64>(frame_end - frame_start).count();
        if (frame_duration_sec > 0.0) {
            throughput_jobs_per_sec = jobs_completed / frame_duration_sec;
        }
    }
    
    f64 frame_duration_ms() const {
        return std::chrono::duration<f64, std::milli>(frame_end - frame_start).count();
    }
};

//=============================================================================
// Job Profiler Implementation
//=============================================================================

/**
 * @brief Advanced profiler for job system performance analysis
 */
class JobProfiler {
private:
    // Profiling configuration
    bool is_enabled_;
    bool record_detailed_metrics_;
    bool enable_hardware_counters_;
    usize max_records_per_category_;
    
    // Data collection
    std::mutex profiling_mutex_;
    std::deque<JobExecutionRecord> job_records_;
    std::deque<StealEvent> steal_events_;
    std::deque<ThreadUtilizationSample> utilization_samples_;
    std::deque<PerformanceFrame> performance_frames_;
    
    // Real-time metrics
    std::atomic<u64> total_jobs_profiled_{0};
    std::atomic<u64> total_steals_recorded_{0};
    std::atomic<f64> average_job_duration_ms_{0.0};
    std::atomic<f64> average_steal_success_rate_{0.0};
    
    // Profiling session info
    std::chrono::high_resolution_clock::time_point session_start_;
    std::atomic<u64> current_frame_number_{0};
    std::unique_ptr<PerformanceFrame> current_frame_;
    
    // Thread safety
    std::atomic<bool> is_recording_{false};
    
public:
    /**
     * @brief Profiler configuration
     */
    struct Config {
        bool enable_profiling = true;
        bool record_detailed_metrics = true;
        bool enable_hardware_counters = false; // Requires special permissions
        usize max_job_records = 10000;
        usize max_steal_events = 5000;
        usize max_utilization_samples = 2000;
        usize max_performance_frames = 1000;
        
        static Config create_lightweight() {
            Config config;
            config.record_detailed_metrics = false;
            config.enable_hardware_counters = false;
            config.max_job_records = 1000;
            config.max_steal_events = 500;
            config.max_utilization_samples = 200;
            return config;
        }
        
        static Config create_comprehensive() {
            Config config;
            config.record_detailed_metrics = true;
            config.enable_hardware_counters = true;
            config.max_job_records = 50000;
            config.max_steal_events = 20000;
            config.max_utilization_samples = 10000;
            return config;
        }
    };
    
    explicit JobProfiler(const Config& config = Config{});
    ~JobProfiler();
    
    // Profiling control
    void start_profiling();
    void stop_profiling();
    void reset_data();
    bool is_enabled() const { return is_enabled_; }
    
    // Data collection interface
    void record_job_submission(const Job& job);
    void record_job_execution_start(const Job& job, u32 worker_id);
    void record_job_execution_end(const Job& job, u32 worker_id);
    void record_steal_event(u32 thief_worker, u32 victim_worker, 
                           const Job* job, bool successful);
    void record_thread_utilization(u32 worker_id, bool is_working, 
                                 const std::string& current_job = "");
    
    // Frame-based profiling
    void start_frame();
    void end_frame();
    u64 current_frame() const { return current_frame_number_.load(); }
    
    // Analysis and reporting
    struct ProfilingReport {
        // Summary statistics
        u64 total_jobs_executed = 0;
        f64 average_execution_time_ms = 0.0;
        f64 average_queue_time_ms = 0.0;
        f64 total_profiling_duration_sec = 0.0;
        
        // Work-stealing analysis
        u64 total_steals = 0;
        u64 total_steal_attempts = 0;
        f64 steal_success_rate = 0.0;
        std::vector<std::pair<u32, u32>> most_active_steal_pairs; // (thief, victim)
        
        // Thread utilization
        std::vector<f64> average_thread_utilization;
        f64 overall_utilization = 0.0;
        f64 load_balance_score = 1.0; // 1.0 = perfect balance
        
        // Performance insights
        std::vector<std::string> bottleneck_jobs;
        std::vector<std::string> underutilized_threads;
        std::vector<std::string> optimization_suggestions;
        
        // Hardware metrics (if available)
        f64 average_cache_hit_rate = 0.0;
        u64 total_memory_allocated = 0;
        std::vector<std::pair<u32, f64>> numa_node_efficiency; // (node, efficiency)
    };
    
    ProfilingReport generate_report() const;
    
    // Visualization data export
    void export_timeline_data(const std::string& filename) const;
    void export_steal_pattern_data(const std::string& filename) const;
    void export_utilization_data(const std::string& filename) const;
    void export_performance_frames(const std::string& filename) const;
    
    // Real-time monitoring
    struct RealTimeMetrics {
        f64 current_throughput_jobs_per_sec = 0.0;
        f64 current_average_execution_time_ms = 0.0;
        std::vector<f64> current_thread_utilization;
        f64 current_steal_rate = 0.0;
        u32 active_jobs = 0;
        u32 queued_jobs = 0;
    };
    
    RealTimeMetrics get_real_time_metrics() const;
    
    // Educational analysis
    struct EducationalInsights {
        struct ParallelEfficiencyAnalysis {
            f64 theoretical_speedup = 1.0;
            f64 actual_speedup = 1.0;
            f64 efficiency_percentage = 100.0;
            std::string bottleneck_explanation;
            std::vector<std::string> improvement_suggestions;
        };
        
        struct WorkStealingAnalysis {
            f64 steal_frequency_per_sec = 0.0;
            f64 steal_effectiveness = 0.0;
            std::string steal_pattern_description;
            std::vector<std::string> load_balance_observations;
        };
        
        struct ThreadingLessons {
            std::vector<std::string> concepts_demonstrated;
            std::vector<std::string> best_practices_shown;
            std::vector<std::string> common_pitfalls_avoided;
            std::string overall_assessment;
        };
        
        ParallelEfficiencyAnalysis efficiency_analysis;
        WorkStealingAnalysis work_stealing_analysis;
        ThreadingLessons threading_lessons;
        
        std::string performance_grade; // A-F grade
        std::vector<std::string> key_takeaways;
    };
    
    EducationalInsights generate_educational_insights() const;
    void print_educational_summary() const;
    
    // Configuration
    void set_recording_enabled(bool enabled) { is_recording_.store(enabled); }
    void set_detailed_metrics(bool enabled) { record_detailed_metrics_ = enabled; }
    void set_max_records(usize max_records) { max_records_per_category_ = max_records; }
    
    // Information
    usize job_record_count() const;
    usize steal_event_count() const;
    usize utilization_sample_count() const;
    f64 profiling_overhead_percent() const;
    
private:
    void cleanup_old_records();
    void update_real_time_metrics();
    f64 calculate_load_balance_coefficient(const std::vector<f64>& thread_loads) const;
    std::string analyze_steal_patterns() const;
    std::vector<std::string> identify_bottlenecks() const;
    void collect_hardware_metrics(JobExecutionRecord& record) const;
    
    template<typename Container>
    void trim_container(Container& container, usize max_size);
};

//=============================================================================
// Educational Visualizer
//=============================================================================

/**
 * @brief Educational visualization system for job execution patterns
 */
class EducationalVisualizer {
private:
    const JobProfiler* profiler_;
    bool enable_real_time_display_;
    bool enable_interactive_mode_;
    
    // Visualization state
    std::chrono::high_resolution_clock::time_point visualization_start_;
    std::atomic<u64> frame_counter_{0};
    
    // Display configuration
    u32 timeline_width_chars_;
    u32 timeline_height_chars_;
    f64 time_scale_ms_per_char_;
    bool show_job_names_;
    bool show_steal_events_;
    bool use_colors_;
    
public:
    /**
     * @brief Visualizer configuration
     */
    struct Config {
        bool enable_real_time_display = true;
        bool enable_interactive_mode = false;
        u32 timeline_width = 120;
        u32 timeline_height = 20;
        f64 time_scale_ms_per_char = 1.0;
        bool show_job_names = true;
        bool show_steal_events = true;
        bool use_colors = true;
    };
    
    explicit EducationalVisualizer(const JobProfiler* profiler, 
                                  const Config& config = Config{});
    ~EducationalVisualizer();
    
    // Visualization control
    void start_visualization();
    void stop_visualization();
    void update_display();
    
    // Static visualizations
    void generate_execution_timeline_svg(const std::string& filename) const;
    void generate_work_stealing_diagram(const std::string& filename) const;
    void generate_thread_utilization_chart(const std::string& filename) const;
    void generate_performance_dashboard(const std::string& filename) const;
    
    // Interactive displays
    void show_real_time_dashboard() const;
    void show_job_execution_timeline() const;
    void show_work_stealing_visualization() const;
    void show_thread_load_bars() const;
    
    // Educational content
    void print_parallelization_tutorial() const;
    void print_work_stealing_explanation() const;
    void print_performance_analysis_guide() const;
    void demonstrate_concepts_with_data() const;
    
    // Text-based visualization helpers
    std::string create_timeline_ascii(f64 start_time_ms, f64 end_time_ms) const;
    std::string create_thread_utilization_bars() const;
    std::string create_steal_pattern_diagram() const;
    std::string format_performance_summary() const;
    
    // Configuration
    void set_real_time_display(bool enabled) { enable_real_time_display_ = enabled; }
    void set_interactive_mode(bool enabled) { enable_interactive_mode_ = enabled; }
    void set_timeline_scale(f64 ms_per_char) { time_scale_ms_per_char_ = ms_per_char; }
    void set_use_colors(bool use_colors) { use_colors_ = use_colors; }
    
private:
    std::string get_color_code(u32 worker_id) const;
    std::string reset_color_code() const;
    char get_job_char(const JobExecutionRecord& record) const;
    std::string format_duration(f64 duration_ms) const;
    void clear_screen() const;
    void move_cursor(u32 x, u32 y) const;
};

//=============================================================================
// Performance Comparison System
//=============================================================================

/**
 * @brief System for comparing sequential vs parallel performance
 */
class PerformanceComparator {
private:
    struct ExecutionProfile {
        std::string test_name;
        f64 sequential_time_ms = 0.0;
        f64 parallel_time_ms = 0.0;
        u32 worker_count = 1;
        u32 job_count = 0;
        f64 theoretical_speedup = 1.0;
        f64 actual_speedup = 1.0;
        f64 efficiency = 100.0;
        std::string workload_characteristics;
    };
    
    std::vector<ExecutionProfile> comparison_profiles_;
    JobSystem* job_system_;
    
public:
    explicit PerformanceComparator(JobSystem* job_system);
    
    // Benchmarking interface
    template<typename WorkloadFunc>
    void benchmark_workload(const std::string& test_name,
                           WorkloadFunc&& sequential_version,
                           WorkloadFunc&& parallel_version,
                           u32 iterations = 10);
    
    void benchmark_parallel_for(const std::string& test_name,
                               usize work_size,
                               std::function<void(usize)> work_function,
                               u32 iterations = 10);
    
    void benchmark_job_granularity(const std::string& test_name,
                                  std::function<void()> unit_work,
                                  const std::vector<u32>& job_counts);
    
    // Analysis and reporting
    struct ComparisonReport {
        f64 average_speedup = 1.0;
        f64 best_speedup = 1.0;
        f64 worst_speedup = 1.0;
        f64 average_efficiency = 100.0;
        std::vector<ExecutionProfile> profiles;
        std::vector<std::string> insights;
        std::string overall_assessment;
    };
    
    ComparisonReport generate_comparison_report() const;
    void print_comparison_table() const;
    void export_comparison_data(const std::string& filename) const;
    
    // Educational analysis
    void explain_speedup_results() const;
    void demonstrate_amdahls_law() const;
    void show_scalability_analysis() const;
    
private:
    f64 measure_execution_time(std::function<void()> func, u32 iterations) const;
    f64 calculate_theoretical_speedup(u32 worker_count, f64 parallel_fraction) const;
    std::string classify_workload_characteristics(const ExecutionProfile& profile) const;
};

//=============================================================================
// Template Implementations
//=============================================================================

template<typename Container>
void JobProfiler::trim_container(Container& container, usize max_size) {
    while (container.size() > max_size) {
        container.pop_front();
    }
}

template<typename WorkloadFunc>
void PerformanceComparator::benchmark_workload(const std::string& test_name,
                                              WorkloadFunc&& sequential_version,
                                              WorkloadFunc&& parallel_version,
                                              u32 iterations) {
    ExecutionProfile profile;
    profile.test_name = test_name;
    profile.worker_count = job_system_->worker_count();
    
    // Measure sequential execution
    profile.sequential_time_ms = measure_execution_time(
        [&]() { sequential_version(); }, iterations);
    
    // Measure parallel execution
    profile.parallel_time_ms = measure_execution_time(
        [&]() { parallel_version(); }, iterations);
    
    // Calculate speedup and efficiency
    if (profile.parallel_time_ms > 0.0) {
        profile.actual_speedup = profile.sequential_time_ms / profile.parallel_time_ms;
        profile.efficiency = (profile.actual_speedup / profile.worker_count) * 100.0;
    }
    
    profile.workload_characteristics = classify_workload_characteristics(profile);
    comparison_profiles_.push_back(profile);
}

} // namespace ecscope::job_system