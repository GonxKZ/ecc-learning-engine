#include "jobs/job_profiler.hpp"
#include <fstream>
#include <sstream>
#include <iomanip>
#include <thread>
#include <algorithm>

namespace ecscope::jobs {

//=============================================================================
// Custom Metric Implementation
//=============================================================================

// Implementation already provided in header as inline methods

//=============================================================================
// Performance Analyzer Implementation
//=============================================================================

PerformanceAnalyzer::PerformanceAnalyzer(const ProfilerConfig& config)
    : config_(config)
    , last_analysis_time_(std::chrono::steady_clock::now())
    , analysis_mutex_("PerformanceAnalyzer_Analysis")
    , bottleneck_mutex_("PerformanceAnalyzer_Bottleneck")
    , anomaly_threshold_sigma_(3.0) {
}

StatisticalAnalysis PerformanceAnalyzer::analyze_metric(const std::string& metric_name, 
                                                       const std::vector<f64>& samples) const {
    StatisticalAnalysis analysis;
    analysis.sample_count = samples.size();
    analysis.analysis_time = std::chrono::steady_clock::now();
    
    if (samples.empty()) {
        return analysis;
    }
    
    // Calculate basic statistics
    f64 sum = 0.0;
    f64 min_val = samples[0];
    f64 max_val = samples[0];
    
    for (f64 sample : samples) {
        sum += sample;
        min_val = std::min(min_val, sample);
        max_val = std::max(max_val, sample);
    }
    
    analysis.mean = sum / samples.size();
    analysis.min_value = min_val;
    analysis.max_value = max_val;
    
    // Calculate variance and standard deviation
    f64 variance_sum = 0.0;
    for (f64 sample : samples) {
        f64 diff = sample - analysis.mean;
        variance_sum += diff * diff;
    }
    
    analysis.variance = variance_sum / samples.size();
    analysis.standard_deviation = std::sqrt(analysis.variance);
    
    if (analysis.mean != 0.0) {
        analysis.coefficient_of_variation = analysis.standard_deviation / analysis.mean;
    }
    
    // Calculate percentiles
    auto sorted_samples = samples;
    std::sort(sorted_samples.begin(), sorted_samples.end());
    
    auto percentile = [&](f64 p) -> f64 {
        if (sorted_samples.empty()) return 0.0;
        usize index = static_cast<usize>(p * (sorted_samples.size() - 1));
        return sorted_samples[std::min(index, sorted_samples.size() - 1)];
    };
    
    analysis.median = percentile(0.5);
    analysis.p50 = analysis.median;
    analysis.p90 = percentile(0.9);
    analysis.p95 = percentile(0.95);
    analysis.p99 = percentile(0.99);
    analysis.p999 = percentile(0.999);
    
    // Confidence interval (simplified)
    f64 z_score = 1.96; // 95% confidence
    f64 margin = z_score * (analysis.standard_deviation / std::sqrt(samples.size()));
    analysis.confidence_lower = analysis.mean - margin;
    analysis.confidence_upper = analysis.mean + margin;
    analysis.confidence_level = 0.95;
    
    return analysis;
}

std::vector<PerformanceBottleneck> PerformanceAnalyzer::detect_bottlenecks(
    const std::vector<PerformanceEvent>& events) const {
    
    std::vector<PerformanceBottleneck> bottlenecks;
    
    if (events.empty()) {
        return bottlenecks;
    }
    
    // Analyze job execution times
    std::unordered_map<JobID, std::vector<f64>, JobID::Hash> job_durations;
    std::unordered_map<u8, std::vector<f64>> worker_utilization;
    
    for (const auto& event : events) {
        if (event.type == PerformanceEventType::JobEnd && event.duration.count() > 0) {
            f64 duration_us = static_cast<f64>(event.duration.count()) / 1000.0;
            job_durations[event.job_id].push_back(duration_us);
            worker_utilization[event.worker_id].push_back(duration_us);
        }
    }
    
    // Detect high-latency jobs
    for (const auto& [job_id, durations] : job_durations) {
        if (durations.size() >= 10) {  // Need sufficient samples
            auto stats = analyze_metric("job_duration", durations);
            
            if (stats.p95 > 10000.0) {  // 10ms threshold
                PerformanceBottleneck bottleneck;
                bottleneck.type = PerformanceBottleneck::Type::HighLatency;
                bottleneck.description = "Job " + std::to_string(job_id.index) + 
                                       " has high execution latency (P95: " + 
                                       std::to_string(stats.p95) + "μs)";
                bottleneck.severity_score = std::min(1.0, stats.p95 / 100000.0);
                bottleneck.affected_jobs.push_back(job_id);
                bottleneck.recommendation = "Consider optimizing job implementation or splitting into smaller tasks";
                bottleneck.estimated_improvement = 0.3; // 30% improvement estimate
                
                bottlenecks.push_back(bottleneck);
            }
        }
    }
    
    // Detect load imbalance
    if (worker_utilization.size() >= 2) {
        std::vector<f64> worker_avg_durations;
        for (const auto& [worker_id, durations] : worker_utilization) {
            f64 avg = 0.0;
            for (f64 duration : durations) avg += duration;
            avg /= durations.size();
            worker_avg_durations.push_back(avg);
        }
        
        auto stats = analyze_metric("worker_load", worker_avg_durations);
        if (stats.coefficient_of_variation > 0.5) {  // High variation
            PerformanceBottleneck bottleneck;
            bottleneck.type = PerformanceBottleneck::Type::PoorLoadBalancing;
            bottleneck.description = "Significant load imbalance detected across workers (CV: " + 
                                   std::to_string(stats.coefficient_of_variation) + ")";
            bottleneck.severity_score = std::min(1.0, stats.coefficient_of_variation);
            bottleneck.recommendation = "Review work-stealing configuration or job distribution";
            bottleneck.estimated_improvement = 0.2; // 20% improvement estimate
            
            bottlenecks.push_back(bottleneck);
        }
    }
    
    return bottlenecks;
}

//=============================================================================
// Job Profiler Implementation
//=============================================================================

JobProfiler::JobProfiler(const ProfilerConfig& config)
    : config_(config)
    , is_profiling_(false)
    , is_shutting_down_(false)
    , global_sequence_number_(1)
    , metrics_mutex_("JobProfiler_Metrics")
    , analyzer_(std::make_unique<PerformanceAnalyzer>(config))
    , current_session_(nullptr)
    , analysis_cv_("JobProfiler_AnalysisCV")
    , analysis_mutex_("JobProfiler_AnalysisThread") {
}

JobProfiler::~JobProfiler() {
    shutdown();
}

bool JobProfiler::initialize(u32 worker_count) {
    if (is_profiling_.load(std::memory_order_acquire)) {
        return false; // Already initialized
    }
    
    // Initialize event buffers for each worker
    event_buffers_.reserve(worker_count);
    for (u32 i = 0; i < worker_count; ++i) {
        event_buffers_.emplace_back(
            std::make_unique<EventBuffer>(config_.event_buffer_size));
    }
    
    // Initialize output files if needed
    if (config_.enable_auto_export) {
        initialize_output_files();
    }
    
    // Start analysis thread
    if (config_.enable_real_time_analysis) {
        analysis_thread_ = std::thread([this]() { process_events_background(); });
    }
    
    return true;
}

void JobProfiler::shutdown() {
    if (is_shutting_down_.exchange(true, std::memory_order_acq_rel)) {
        return; // Already shutting down
    }
    
    // Stop profiling
    end_profiling_session();
    
    // Stop analysis thread
    if (analysis_thread_.joinable()) {
        analysis_cv_.notify_all();
        analysis_thread_.join();
    }
    
    // Flush remaining events
    flush_event_buffers();
    
    // Finalize output files
    finalize_output_files();
}

void JobProfiler::start_profiling_session(const std::string& session_name) {
    if (is_profiling_.load(std::memory_order_acquire)) {
        end_profiling_session();
    }
    
    current_session_ = std::make_unique<ProfilingSession>();
    current_session_->name = session_name;
    current_session_->start_time = std::chrono::steady_clock::now();
    current_session_->level = config_.level;
    
    is_profiling_.store(true, std::memory_order_release);
}

void JobProfiler::end_profiling_session() {
    if (!is_profiling_.exchange(false, std::memory_order_acq_rel)) {
        return; // Not profiling
    }
    
    if (current_session_) {
        current_session_->end_time = std::chrono::steady_clock::now();
        
        // Calculate session statistics
        flush_event_buffers();
        
        // Move to completed sessions
        completed_sessions_.push_back(*current_session_);
        current_session_.reset();
    }
}

void JobProfiler::record_job_start(u8 worker_id, JobID job_id, FiberID fiber_id) {
    if (!should_record_event() || worker_id >= event_buffers_.size()) {
        return;
    }
    
    PerformanceEvent event(PerformanceEventType::JobStart, worker_id, job_id);
    event.fiber_id = fiber_id;
    event.sequence_number = global_sequence_number_.fetch_add(1, std::memory_order_relaxed);
    
    record_event(worker_id, std::move(event));
}

void JobProfiler::record_job_end(u8 worker_id, JobID job_id, FiberID fiber_id, 
                                std::chrono::nanoseconds duration) {
    if (!should_record_event() || worker_id >= event_buffers_.size()) {
        return;
    }
    
    PerformanceEvent event(PerformanceEventType::JobEnd, worker_id, job_id);
    event.fiber_id = fiber_id;
    event.duration = duration;
    event.sequence_number = global_sequence_number_.fetch_add(1, std::memory_order_relaxed);
    
    record_event(worker_id, std::move(event));
}

void JobProfiler::record_fiber_switch(u8 worker_id, FiberID from_fiber, FiberID to_fiber) {
    if (!should_record_event() || worker_id >= event_buffers_.size()) {
        return;
    }
    
    PerformanceEvent event(PerformanceEventType::FiberSwitch, worker_id);
    event.fiber_id = from_fiber;
    event.data.custom_data = (static_cast<u64>(to_fiber.index) << 32) | to_fiber.generation;
    event.sequence_number = global_sequence_number_.fetch_add(1, std::memory_order_relaxed);
    
    record_event(worker_id, std::move(event));
}

void JobProfiler::record_work_steal(u8 worker_id, u8 target_worker, bool success, u32 stolen_count) {
    if (!should_record_event() || worker_id >= event_buffers_.size()) {
        return;
    }
    
    PerformanceEventType event_type = success ? PerformanceEventType::WorkSteal : 
                                              PerformanceEventType::WorkStealFailed;
    
    PerformanceEvent event(event_type, worker_id);
    event.data.work_steal.steal_target_worker = target_worker;
    event.data.work_steal.steal_count = stolen_count;
    event.sequence_number = global_sequence_number_.fetch_add(1, std::memory_order_relaxed);
    
    record_event(worker_id, std::move(event));
}

std::vector<PerformanceBottleneck> JobProfiler::get_current_bottlenecks() const {
    if (!analyzer_) {
        return {};
    }
    
    return analyzer_->get_current_bottlenecks();
}

f64 JobProfiler::get_system_health_score() const {
    if (!analyzer_) {
        return 1.0; // Assume healthy if no analyzer
    }
    
    return analyzer_->calculate_system_health_score();
}

std::string JobProfiler::generate_real_time_report() const {
    std::ostringstream report;
    report << "=== Job System Profiling Report ===\\n";
    
    if (current_session_) {
        auto now = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::seconds>(
            now - current_session_->start_time);
        
        report << "Session: " << current_session_->name << "\\n";
        report << "Duration: " << duration.count() << " seconds\\n";
        report << "Total Events: " << current_session_->total_events << "\\n";
        report << "Jobs Profiled: " << current_session_->total_jobs_profiled << "\\n";
        report << "Overhead: " << std::fixed << std::setprecision(2) 
               << current_session_->overhead_percentage << "%\\n";
    }
    
    // Add system metrics
    auto metrics = get_current_metrics();
    report << "\\n=== Current Metrics ===\\n";
    for (const auto& [name, value] : metrics) {
        report << name << ": " << std::fixed << std::setprecision(3) << value << "\\n";
    }
    
    // Add bottleneck analysis
    auto bottlenecks = get_current_bottlenecks();
    if (!bottlenecks.empty()) {
        report << "\\n=== Detected Bottlenecks ===\\n";
        for (const auto& bottleneck : bottlenecks) {
            report << "- " << bottleneck.description 
                   << " (Severity: " << std::fixed << std::setprecision(2) 
                   << bottleneck.severity_score << ")\\n";
        }
    }
    
    return report.str();
}

void JobProfiler::register_custom_metric(std::unique_ptr<CustomMetric> metric) {
    if (!metric) return;
    
    FiberLockGuard lock(metrics_mutex_);
    std::string name = metric->name();
    custom_metrics_[name] = std::move(metric);
}

void JobProfiler::update_metric(const std::string& name, f64 value) {
    FiberLockGuard lock(metrics_mutex_);
    auto it = custom_metrics_.find(name);
    if (it != custom_metrics_.end()) {
        it->second->update(value);
    }
}

std::unordered_map<std::string, f64> JobProfiler::get_current_metrics() const {
    std::unordered_map<std::string, f64> metrics;
    
    FiberLockGuard lock(metrics_mutex_);
    for (const auto& [name, metric] : custom_metrics_) {
        metric->collect_if_needed();
        metrics[name] = metric->value().value.load(std::memory_order_relaxed);
    }
    
    return metrics;
}

//=============================================================================
// Private Methods
//=============================================================================

void JobProfiler::record_event(u8 worker_id, PerformanceEvent&& event) {
    if (worker_id >= event_buffers_.size() || !event_buffers_[worker_id]) {
        return;
    }
    
    // Fill in common event data
    event.thread_id = std::hash<std::thread::id>{}(std::this_thread::get_id());
    event.cpu_core = 0; // Would need platform-specific code to get actual core
    event.numa_node = 0; // Would need platform-specific code to get NUMA node
    
    if (!event_buffers_[worker_id]->try_push(std::move(event))) {
        // Buffer full - optionally handle overflow
    }
}

bool JobProfiler::should_record_event() const {
    if (!is_profiling_.load(std::memory_order_acquire)) {
        return false;
    }
    
    if (config_.enable_sampling) {
        // Simple sampling based on sequence number
        static thread_local u64 sample_counter = 0;
        sample_counter++;
        return (sample_counter % static_cast<u64>(1.0 / config_.sampling_rate)) == 0;
    }
    
    return true;
}

void JobProfiler::process_events_background() {
    while (!is_shutting_down_.load(std::memory_order_acquire)) {
        flush_event_buffers();
        
        FiberUniqueLock lock(analysis_mutex_);
        analysis_cv_.wait_for(lock, config_.collection_interval);
    }
}

void JobProfiler::flush_event_buffers() {
    for (auto& buffer : event_buffers_) {
        if (!buffer) continue;
        
        std::vector<PerformanceEvent> events;
        PerformanceEvent event;
        
        while (buffer->try_pop(event)) {
            events.push_back(std::move(event));
            
            if (current_session_) {
                current_session_->total_events++;
            }
        }
        
        if (!events.empty() && analyzer_ && config_.enable_real_time_analysis) {
            update_real_time_analysis(events);
        }
        
        // Write events to output files
        for (const auto& evt : events) {
            write_event_to_files(evt);
        }
    }
}

void JobProfiler::initialize_output_files() {
    if (config_.enable_binary_export && !binary_output_) {
        std::string filename = generate_session_filename(".bin");
        binary_output_ = std::make_unique<std::ofstream>(filename, std::ios::binary);
    }
    
    if (config_.enable_json_export && !json_output_) {
        std::string filename = generate_session_filename(".json");
        json_output_ = std::make_unique<std::ofstream>(filename);
        *json_output_ << "{\\n  \\"events\\": [\\n";
    }
    
    if (config_.enable_csv_export && !csv_output_) {
        std::string filename = generate_session_filename(".csv");
        csv_output_ = std::make_unique<std::ofstream>(filename);
        *csv_output_ << "timestamp,type,worker_id,job_id,fiber_id,duration_ns\\n";
    }
}

void JobProfiler::write_event_to_files(const PerformanceEvent& event) {
    if (binary_output_) {
        binary_output_->write(reinterpret_cast<const char*>(&event), sizeof(event));
    }
    
    if (json_output_) {
        static bool first_event = true;
        if (!first_event) {
            *json_output_ << ",\\n";
        }
        first_event = false;
        
        *json_output_ << "    {\\n";
        *json_output_ << "      \\"timestamp\\": " << event.timestamp.time_since_epoch().count() << ",\\n";
        *json_output_ << "      \\"type\\": " << static_cast<int>(event.type) << ",\\n";
        *json_output_ << "      \\"worker_id\\": " << static_cast<int>(event.worker_id) << ",\\n";
        *json_output_ << "      \\"job_id\\": " << event.job_id.index << ",\\n";
        *json_output_ << "      \\"duration_ns\\": " << event.duration.count() << "\\n";
        *json_output_ << "    }";
    }
    
    if (csv_output_) {
        *csv_output_ << event.timestamp.time_since_epoch().count() << ","
                    << static_cast<int>(event.type) << ","
                    << static_cast<int>(event.worker_id) << ","
                    << event.job_id.index << ","
                    << event.fiber_id.index << ","
                    << event.duration.count() << "\\n";
    }
}

void JobProfiler::finalize_output_files() {
    if (json_output_) {
        *json_output_ << "\\n  ]\\n}";
        json_output_->close();
    }
    
    if (binary_output_) {
        binary_output_->close();
    }
    
    if (csv_output_) {
        csv_output_->close();
    }
}

void JobProfiler::update_real_time_analysis(const std::vector<PerformanceEvent>& events) {
    if (analyzer_) {
        analyzer_->update_bottleneck_analysis(events);
    }
}

std::string JobProfiler::generate_session_filename(const std::string& extension) const {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    
    std::ostringstream filename;
    filename << config_.output_directory << "/"
            << config_.session_name_prefix << "_"
            << std::put_time(std::localtime(&time_t), "%Y%m%d_%H%M%S")
            << extension;
    
    return filename.str();
}

} // namespace ecscope::jobs