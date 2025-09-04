/**
 * @file job_profiler.cpp
 * @brief Implementation of Job System Profiler
 */

#include "job_profiler.hpp"
#include "core/log.hpp"
#include <fstream>
#include <iomanip>
#include <sstream>
#include <algorithm>
#include <numeric>

namespace ecscope::job_system {

//=============================================================================
// JobProfiler Implementation
//=============================================================================

JobProfiler::JobProfiler(const Config& config)
    : config_(config)
    , is_profiling_(false)
    , profiling_start_time_(std::chrono::high_resolution_clock::now())
    , current_frame_(0)
    , total_profiling_time_(0.0) {
    
    if (config_.enable_detailed_logging) {
        LOG_INFO("Job Profiler initialized with detailed logging");
    }
    
    if (config_.enable_real_time_display) {
        LOG_INFO("Real-time performance display enabled");
    }
}

JobProfiler::~JobProfiler() {
    if (is_profiling_) {
        stop_profiling();
    }
}

void JobProfiler::start_profiling() {
    std::lock_guard<std::mutex> lock(profiler_mutex_);
    
    if (is_profiling_) {
        LOG_WARN("Profiler already running");
        return;
    }
    
    is_profiling_ = true;
    profiling_start_time_ = std::chrono::high_resolution_clock::now();
    current_frame_ = 0;
    
    // Clear previous data
    job_records_.clear();
    frame_data_.clear();
    worker_statistics_.clear();
    
    LOG_INFO("Job profiler started");
}

void JobProfiler::stop_profiling() {
    std::lock_guard<std::mutex> lock(profiler_mutex_);
    
    if (!is_profiling_) {
        LOG_WARN("Profiler not running");
        return;
    }
    
    is_profiling_ = false;
    auto end_time = std::chrono::high_resolution_clock::now();
    total_profiling_time_ = std::chrono::duration<f64>(end_time - profiling_start_time_).count();
    
    LOG_INFO("Job profiler stopped after {:.2f} seconds", total_profiling_time_);
    
    // Generate final report
    if (config_.generate_report_on_stop) {
        auto report = generate_report();
        print_performance_summary(report);
    }
}

void JobProfiler::record_job_start(const std::string& job_name, JobID job_id, u32 worker_id) {
    if (!is_profiling_) return;
    
    std::lock_guard<std::mutex> lock(profiler_mutex_);
    
    auto now = std::chrono::high_resolution_clock::now();
    
    JobRecord record;
    record.job_name = job_name;
    record.job_id = job_id;
    record.worker_id = worker_id;
    record.start_time = now;
    record.frame_number = current_frame_;
    record.was_stolen = false;  // Will be updated if stolen
    
    active_jobs_[job_id] = std::move(record);
    
    if (config_.enable_detailed_logging) {
        LOG_DEBUG("Job '{}' started on worker {}", job_name, worker_id);
    }
}

void JobProfiler::record_job_end(JobID job_id, bool completed_successfully) {
    if (!is_profiling_) return;
    
    std::lock_guard<std::mutex> lock(profiler_mutex_);
    
    auto it = active_jobs_.find(job_id);
    if (it == active_jobs_.end()) {
        if (config_.enable_detailed_logging) {
            LOG_WARN("Job end recorded for unknown job ID: {}", job_id.value);
        }
        return;
    }
    
    auto now = std::chrono::high_resolution_clock::now();
    JobRecord record = std::move(it->second);
    active_jobs_.erase(it);
    
    record.end_time = now;
    record.execution_time = std::chrono::duration<f64, std::milli>(record.end_time - record.start_time).count();
    record.completed_successfully = completed_successfully;
    
    job_records_.push_back(std::move(record));
    
    // Update worker statistics
    auto& worker_stats = worker_statistics_[record.worker_id];
    worker_stats.total_jobs_executed++;
    worker_stats.total_execution_time += record.execution_time;
    
    if (record.execution_time > worker_stats.max_job_time) {
        worker_stats.max_job_time = record.execution_time;
    }
    
    if (record.execution_time < worker_stats.min_job_time) {
        worker_stats.min_job_time = record.execution_time;
    }
    
    if (config_.enable_detailed_logging) {
        LOG_DEBUG("Job '{}' completed in {:.2f}ms on worker {}", 
                  record.job_name, record.execution_time, record.worker_id);
    }
}

void JobProfiler::record_job_steal(JobID job_id, u32 from_worker, u32 to_worker) {
    if (!is_profiling_) return;
    
    std::lock_guard<std::mutex> lock(profiler_mutex_);
    
    auto it = active_jobs_.find(job_id);
    if (it != active_jobs_.end()) {
        it->second.was_stolen = true;
        it->second.worker_id = to_worker;  // Update to stealing worker
    }
    
    // Update steal statistics
    auto& from_stats = worker_statistics_[from_worker];
    auto& to_stats = worker_statistics_[to_worker];
    
    from_stats.jobs_stolen_by_others++;
    to_stats.jobs_stolen_from_others++;
    
    if (config_.enable_detailed_logging) {
        LOG_DEBUG("Job stolen from worker {} to worker {}", from_worker, to_worker);
    }
}

void JobProfiler::start_frame() {
    if (!is_profiling_) return;
    
    std::lock_guard<std::mutex> lock(profiler_mutex_);
    
    current_frame_++;
    
    FrameData frame_data;
    frame_data.frame_number = current_frame_;
    frame_data.start_time = std::chrono::high_resolution_clock::now();
    frame_data.jobs_submitted = 0;
    frame_data.jobs_completed = 0;
    frame_data.total_frame_time = 0.0;
    
    current_frame_data_ = std::move(frame_data);
}

void JobProfiler::end_frame() {
    if (!is_profiling_) return;
    
    std::lock_guard<std::mutex> lock(profiler_mutex_);
    
    auto now = std::chrono::high_resolution_clock::now();
    current_frame_data_.end_time = now;
    current_frame_data_.total_frame_time = 
        std::chrono::duration<f64, std::milli>(now - current_frame_data_.start_time).count();
    
    // Count jobs completed in this frame
    current_frame_data_.jobs_completed = std::count_if(
        job_records_.begin(), job_records_.end(),
        [this](const JobRecord& record) {
            return record.frame_number == current_frame_;
        });
    
    frame_data_.push_back(current_frame_data_);
    
    // Real-time display update
    if (config_.enable_real_time_display && current_frame_ % config_.display_update_interval == 0) {
        update_real_time_display();
    }
}

void JobProfiler::record_thread_utilization(u32 worker_id, f64 utilization_percentage) {
    if (!is_profiling_) return;
    
    std::lock_guard<std::mutex> lock(profiler_mutex_);
    
    auto& worker_stats = worker_statistics_[worker_id];
    worker_stats.utilization_samples.push_back(utilization_percentage);
    
    // Keep a rolling window of samples
    if (worker_stats.utilization_samples.size() > config_.max_utilization_samples) {
        worker_stats.utilization_samples.pop_front();
    }
}

JobProfiler::Report JobProfiler::generate_report() {
    std::lock_guard<std::mutex> lock(profiler_mutex_);
    
    Report report;
    
    // Basic statistics
    report.total_profiling_time = total_profiling_time_;
    report.total_frames = current_frame_;
    report.total_jobs_executed = job_records_.size();
    
    if (!job_records_.empty()) {
        // Calculate execution time statistics
        f64 total_execution_time = 0.0;
        for (const auto& record : job_records_) {
            total_execution_time += record.execution_time;
        }
        report.average_execution_time_ms = total_execution_time / job_records_.size();
        
        // Find min/max execution times
        auto min_max = std::minmax_element(
            job_records_.begin(), job_records_.end(),
            [](const JobRecord& a, const JobRecord& b) {
                return a.execution_time < b.execution_time;
            });
        
        report.min_execution_time_ms = min_max.first->execution_time;
        report.max_execution_time_ms = min_max.second->execution_time;
        
        // Calculate steal statistics
        usize stolen_jobs = std::count_if(
            job_records_.begin(), job_records_.end(),
            [](const JobRecord& record) { return record.was_stolen; });
        
        report.total_steals = stolen_jobs;
        report.steal_success_rate = static_cast<f64>(stolen_jobs) / job_records_.size();
    }
    
    // Worker utilization
    if (!worker_statistics_.empty()) {
        f64 total_utilization = 0.0;
        usize sample_count = 0;
        
        for (const auto& [worker_id, stats] : worker_statistics_) {
            if (!stats.utilization_samples.empty()) {
                f64 worker_avg = std::accumulate(
                    stats.utilization_samples.begin(),
                    stats.utilization_samples.end(), 0.0) / stats.utilization_samples.size();
                
                total_utilization += worker_avg;
                sample_count++;
            }
        }
        
        if (sample_count > 0) {
            report.overall_utilization = total_utilization / sample_count / 100.0;  // Convert to 0-1 range
        }
    }
    
    // Frame statistics
    if (!frame_data_.empty()) {
        f64 total_frame_time = 0.0;
        for (const auto& frame : frame_data_) {
            total_frame_time += frame.total_frame_time;
        }
        report.average_frame_time_ms = total_frame_time / frame_data_.size();
    }
    
    return report;
}

JobProfiler::EducationalInsights JobProfiler::generate_educational_insights() {
    auto report = generate_report();
    EducationalInsights insights;
    
    // Performance grading
    if (report.overall_utilization > 0.8) {
        insights.performance_grade = "A (Excellent)";
    } else if (report.overall_utilization > 0.6) {
        insights.performance_grade = "B (Good)";
    } else if (report.overall_utilization > 0.4) {
        insights.performance_grade = "C (Fair)";
    } else {
        insights.performance_grade = "D (Needs Improvement)";
    }
    
    // Generate key takeaways
    if (report.steal_success_rate > 0.2) {
        insights.key_takeaways.push_back("High work-stealing activity indicates good load balancing");
    } else {
        insights.key_takeaways.push_back("Low work-stealing suggests either balanced workload or insufficient parallelism");
    }
    
    if (report.overall_utilization < 0.5) {
        insights.key_takeaways.push_back("Low thread utilization - consider increasing parallelizable work");
    }
    
    if (report.average_execution_time_ms < 1.0) {
        insights.key_takeaways.push_back("Very short job execution times may indicate excessive overhead");
    }
    
    if (report.max_execution_time_ms > report.average_execution_time_ms * 10) {
        insights.key_takeaways.push_back("High variance in job execution times suggests uneven workload distribution");
    }
    
    return insights;
}

void JobProfiler::print_educational_summary() {
    auto insights = generate_educational_insights();
    auto report = generate_report();
    
    LOG_INFO("=== Educational Job System Summary ===");
    LOG_INFO("Performance Grade: {}", insights.performance_grade);
    LOG_INFO("Total Jobs Executed: {}", report.total_jobs_executed);
    LOG_INFO("Average Job Time: {:.2f}ms", report.average_execution_time_ms);
    LOG_INFO("Thread Utilization: {:.1f}%", report.overall_utilization * 100.0);
    LOG_INFO("Work-Stealing Rate: {:.1f}%", report.steal_success_rate * 100.0);
    
    LOG_INFO("Key Learning Points:");
    for (const auto& takeaway : insights.key_takeaways) {
        LOG_INFO("  • {}", takeaway);
    }
}

void JobProfiler::export_timeline_data(const std::string& filename) {
    std::lock_guard<std::mutex> lock(profiler_mutex_);
    
    std::ofstream file(filename);
    if (!file.is_open()) {
        LOG_ERROR("Failed to open file for timeline export: {}", filename);
        return;
    }
    
    // CSV header
    file << "JobName,JobID,WorkerID,StartTime,EndTime,ExecutionTime,FrameNumber,WasStolen,Completed\n";
    
    // Convert start time to relative time
    auto base_time = profiling_start_time_;
    
    for (const auto& record : job_records_) {
        auto start_relative = std::chrono::duration<f64, std::milli>(record.start_time - base_time).count();
        auto end_relative = std::chrono::duration<f64, std::milli>(record.end_time - base_time).count();
        
        file << record.job_name << ","
             << record.job_id.value << ","
             << record.worker_id << ","
             << std::fixed << std::setprecision(3) << start_relative << ","
             << end_relative << ","
             << record.execution_time << ","
             << record.frame_number << ","
             << (record.was_stolen ? "1" : "0") << ","
             << (record.completed_successfully ? "1" : "0") << "\n";
    }
    
    file.close();
    LOG_INFO("Timeline data exported to: {}", filename);
}

void JobProfiler::export_performance_frames(const std::string& filename) {
    std::lock_guard<std::mutex> lock(profiler_mutex_);
    
    std::ofstream file(filename);
    if (!file.is_open()) {
        LOG_ERROR("Failed to open file for frame data export: {}", filename);
        return;
    }
    
    // CSV header
    file << "FrameNumber,StartTime,EndTime,FrameTime,JobsSubmitted,JobsCompleted\n";
    
    auto base_time = profiling_start_time_;
    
    for (const auto& frame : frame_data_) {
        auto start_relative = std::chrono::duration<f64, std::milli>(frame.start_time - base_time).count();
        auto end_relative = std::chrono::duration<f64, std::milli>(frame.end_time - base_time).count();
        
        file << frame.frame_number << ","
             << std::fixed << std::setprecision(3) << start_relative << ","
             << end_relative << ","
             << frame.total_frame_time << ","
             << frame.jobs_submitted << ","
             << frame.jobs_completed << "\n";
    }
    
    file.close();
    LOG_INFO("Frame performance data exported to: {}", filename);
}

void JobProfiler::update_real_time_display() {
    if (frame_data_.size() < 10) return;  // Need some data for meaningful display
    
    // Calculate recent performance metrics
    auto recent_frames = std::vector<FrameData>(
        frame_data_.end() - std::min<usize>(10, frame_data_.size()), 
        frame_data_.end());
    
    f64 avg_frame_time = 0.0;
    for (const auto& frame : recent_frames) {
        avg_frame_time += frame.total_frame_time;
    }
    avg_frame_time /= recent_frames.size();
    
    LOG_INFO("Real-time: Frame {}, Avg Time: {:.2f}ms, Jobs: {}", 
             current_frame_, avg_frame_time, job_records_.size());
}

void JobProfiler::print_performance_summary(const Report& report) {
    LOG_INFO("=== Job System Performance Summary ===");
    LOG_INFO("Total Profiling Time: {:.2f} seconds", report.total_profiling_time);
    LOG_INFO("Total Frames: {}", report.total_frames);
    LOG_INFO("Total Jobs Executed: {}", report.total_jobs_executed);
    LOG_INFO("Average Frame Time: {:.2f}ms", report.average_frame_time_ms);
    LOG_INFO("Jobs per Second: {:.1f}", report.total_jobs_executed / report.total_profiling_time);
    LOG_INFO("Average Job Execution Time: {:.2f}ms", report.average_execution_time_ms);
    LOG_INFO("Job Time Range: {:.2f}ms - {:.2f}ms", report.min_execution_time_ms, report.max_execution_time_ms);
    LOG_INFO("Work-Stealing Success Rate: {:.1f}%", report.steal_success_rate * 100.0);
    LOG_INFO("Overall Thread Utilization: {:.1f}%", report.overall_utilization * 100.0);
}

//=============================================================================
// PerformanceComparator Implementation
//=============================================================================

PerformanceComparator::PerformanceComparator(JobSystem* job_system)
    : job_system_(job_system) {
    if (!job_system_) {
        throw std::invalid_argument("JobSystem cannot be null");
    }
}

void PerformanceComparator::benchmark_workload(const std::string& name,
                                             std::function<void()> sequential_func,
                                             std::function<void()> parallel_func,
                                             u32 iterations) {
    
    BenchmarkResult result;
    result.workload_name = name;
    result.iterations = iterations;
    
    LOG_INFO("Benchmarking '{}' with {} iterations", name, iterations);
    
    // Warm-up runs
    sequential_func();
    parallel_func();
    
    // Sequential benchmark
    {
        auto start = std::chrono::high_resolution_clock::now();
        for (u32 i = 0; i < iterations; ++i) {
            sequential_func();
        }
        auto end = std::chrono::high_resolution_clock::now();
        
        result.sequential_time_ms = std::chrono::duration<f64, std::milli>(end - start).count();
        result.sequential_avg_time_ms = result.sequential_time_ms / iterations;
    }
    
    // Parallel benchmark
    {
        auto start = std::chrono::high_resolution_clock::now();
        for (u32 i = 0; i < iterations; ++i) {
            parallel_func();
        }
        auto end = std::chrono::high_resolution_clock::now();
        
        result.parallel_time_ms = std::chrono::duration<f64, std::milli>(end - start).count();
        result.parallel_avg_time_ms = result.parallel_time_ms / iterations;
    }
    
    // Calculate metrics
    result.speedup = result.sequential_time_ms / result.parallel_time_ms;
    result.efficiency = result.speedup / job_system_->worker_count();
    
    benchmark_results_.push_back(result);
    
    LOG_INFO("'{}' Results: Sequential={:.2f}ms, Parallel={:.2f}ms, Speedup={:.2f}x, Efficiency={:.1f}%",
             name, result.sequential_avg_time_ms, result.parallel_avg_time_ms, 
             result.speedup, result.efficiency * 100.0);
}

PerformanceComparator::ComparisonReport PerformanceComparator::generate_comparison_report() const {
    ComparisonReport report;
    
    if (benchmark_results_.empty()) {
        return report;
    }
    
    // Calculate averages
    f64 total_speedup = 0.0;
    f64 total_efficiency = 0.0;
    f64 best_speedup = 0.0;
    
    for (const auto& result : benchmark_results_) {
        total_speedup += result.speedup;
        total_efficiency += result.efficiency;
        
        if (result.speedup > best_speedup) {
            best_speedup = result.speedup;
            report.best_benchmark = result.workload_name;
        }
    }
    
    report.average_speedup = total_speedup / benchmark_results_.size();
    report.average_efficiency = total_efficiency / benchmark_results_.size();
    report.best_speedup = best_speedup;
    
    return report;
}

void PerformanceComparator::print_comparison_table() const {
    LOG_INFO("=== Performance Comparison Table ===");
    LOG_INFO("{:<25} {:<12} {:<12} {:<10} {:<10}", 
             "Workload", "Sequential", "Parallel", "Speedup", "Efficiency");
    LOG_INFO(std::string(75, '-'));
    
    for (const auto& result : benchmark_results_) {
        LOG_INFO("{:<25} {:<12.2f} {:<12.2f} {:<10.2f} {:<10.1f}%",
                 result.workload_name,
                 result.sequential_avg_time_ms,
                 result.parallel_avg_time_ms,
                 result.speedup,
                 result.efficiency * 100.0);
    }
}

void PerformanceComparator::export_comparison_data(const std::string& filename) const {
    std::ofstream file(filename);
    if (!file.is_open()) {
        LOG_ERROR("Failed to open file for comparison export: {}", filename);
        return;
    }
    
    // CSV header
    file << "WorkloadName,Iterations,SequentialTime,ParallelTime,SequentialAvg,ParallelAvg,Speedup,Efficiency\n";
    
    for (const auto& result : benchmark_results_) {
        file << result.workload_name << ","
             << result.iterations << ","
             << std::fixed << std::setprecision(3) << result.sequential_time_ms << ","
             << result.parallel_time_ms << ","
             << result.sequential_avg_time_ms << ","
             << result.parallel_avg_time_ms << ","
             << result.speedup << ","
             << result.efficiency << "\n";
    }
    
    file.close();
    LOG_INFO("Comparison data exported to: {}", filename);
}

//=============================================================================
// EducationalVisualizer Implementation
//=============================================================================

EducationalVisualizer::EducationalVisualizer(JobProfiler* profiler, const Config& config)
    : profiler_(profiler), config_(config), is_running_(false) {
    if (!profiler_) {
        throw std::invalid_argument("JobProfiler cannot be null");
    }
}

void EducationalVisualizer::start_visualization() {
    if (is_running_) {
        LOG_WARN("Visualization already running");
        return;
    }
    
    is_running_ = true;
    LOG_INFO("Educational visualization started");
}

void EducationalVisualizer::stop_visualization() {
    is_running_ = false;
    LOG_INFO("Educational visualization stopped");
}

void EducationalVisualizer::update_display() {
    if (!is_running_) return;
    
    // Update display with current profiler data
    // Implementation would depend on the specific visualization framework
    LOG_DEBUG("Updating educational visualization display");
}

void EducationalVisualizer::print_parallelization_tutorial() const {
    LOG_INFO("=== Parallelization Tutorial ===");
    LOG_INFO("Understanding Work-Stealing Job Systems:");
    LOG_INFO("1. Each worker thread has its own job queue (work-stealing queue)");
    LOG_INFO("2. When a worker finishes its jobs, it 'steals' work from other busy workers");
    LOG_INFO("3. This automatic load balancing maximizes CPU utilization");
    LOG_INFO("4. Job granularity affects performance - too small = overhead, too large = imbalance");
    LOG_INFO("5. Dependencies between jobs create execution ordering constraints");
    LOG_INFO("6. Cache-friendly memory access patterns improve performance significantly");
    LOG_INFO("");
    LOG_INFO("Watch the console output to see these concepts in action!");
}

} // namespace ecscope::job_system