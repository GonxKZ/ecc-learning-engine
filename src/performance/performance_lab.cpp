/**
 * @file performance_lab.cpp
 * @brief Performance Laboratory - Main coordinator implementation for ECScope memory behavior analysis
 * 
 * This implements the heart of ECScope's vision as a "laboratorio de memoria en movimiento" 
 * (memory lab in motion), orchestrating comprehensive performance analysis across
 * memory allocation strategies, ECS architecture patterns, and system integration.
 */

#include "performance/performance_lab.hpp"
#include "performance/memory_experiments.hpp"
#include "performance/allocation_benchmarks.hpp"
#include "core/log.hpp"
#include <algorithm>
#include <numeric>
#include <fstream>
#include <json/json.h>
#include <cmath>

namespace ecscope::performance {

// Laboratory Utility Functions Implementation
namespace lab_utils {

f64 measure_execution_time(std::function<void()> func) {
    auto start = std::chrono::high_resolution_clock::now();
    func();
    auto end = std::chrono::high_resolution_clock::now();
    
    auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
    return static_cast<f64>(duration.count()) / 1'000'000.0; // Convert to milliseconds
}

std::vector<f64> measure_multiple_executions(std::function<void()> func, u32 iterations) {
    std::vector<f64> times;
    times.reserve(iterations);
    
    for (u32 i = 0; i < iterations; ++i) {
        times.push_back(measure_execution_time(func));
    }
    
    return times;
}

f64 calculate_average(const std::vector<f64>& samples) {
    if (samples.empty()) return 0.0;
    return std::accumulate(samples.begin(), samples.end(), 0.0) / samples.size();
}

f64 calculate_standard_deviation(const std::vector<f64>& samples) {
    if (samples.size() < 2) return 0.0;
    
    f64 mean = calculate_average(samples);
    f64 variance = 0.0;
    
    for (f64 sample : samples) {
        f64 diff = sample - mean;
        variance += diff * diff;
    }
    
    variance /= (samples.size() - 1);
    return std::sqrt(variance);
}

usize measure_memory_usage(std::function<void()> func) {
    // Get initial memory usage
    usize initial_memory = memory::MemoryTracker::get_instance().get_current_usage();
    
    func();
    
    // Get final memory usage
    usize final_memory = memory::MemoryTracker::get_instance().get_current_usage();
    
    return (final_memory > initial_memory) ? (final_memory - initial_memory) : 0;
}

f64 estimate_cache_miss_rate(const std::vector<f64>& access_times) {
    if (access_times.empty()) return 0.0;
    
    // Simple heuristic: assume times > 2x median indicate cache misses
    std::vector<f64> sorted_times = access_times;
    std::sort(sorted_times.begin(), sorted_times.end());
    
    f64 median = sorted_times[sorted_times.size() / 2];
    f64 cache_miss_threshold = median * 2.0;
    
    usize cache_misses = std::count_if(sorted_times.begin(), sorted_times.end(),
        [cache_miss_threshold](f64 time) { return time > cache_miss_threshold; });
    
    return static_cast<f64>(cache_misses) / sorted_times.size();
}

f64 calculate_memory_bandwidth(usize bytes_transferred, f64 time_seconds) {
    if (time_seconds <= 0.0) return 0.0;
    return (static_cast<f64>(bytes_transferred) / (1024.0 * 1024.0 * 1024.0)) / time_seconds; // GB/s
}

f64 calculate_confidence_interval(const std::vector<f64>& samples, f64 confidence_level) {
    if (samples.size() < 2) return 0.0;
    
    f64 mean = calculate_average(samples);
    f64 std_dev = calculate_standard_deviation(samples);
    f64 std_error = std_dev / std::sqrt(static_cast<f64>(samples.size()));
    
    // Simple approximation for confidence interval (assumes normal distribution)
    f64 t_value = (confidence_level >= 0.95) ? 1.96 : 1.645; // 95% or 90%
    
    return t_value * std_error;
}

bool is_statistically_significant(const std::vector<f64>& baseline, 
                                 const std::vector<f64>& test, 
                                 f64 significance_level) {
    if (baseline.empty() || test.empty()) return false;
    
    f64 baseline_mean = calculate_average(baseline);
    f64 test_mean = calculate_average(test);
    f64 baseline_std = calculate_standard_deviation(baseline);
    f64 test_std = calculate_standard_deviation(test);
    
    // Simple two-sample t-test approximation
    f64 pooled_std = std::sqrt((baseline_std * baseline_std / baseline.size()) + 
                              (test_std * test_std / test.size()));
    
    if (pooled_std == 0.0) return false;
    
    f64 t_statistic = std::abs(baseline_mean - test_mean) / pooled_std;
    f64 critical_value = (significance_level <= 0.05) ? 1.96 : 1.645;
    
    return t_statistic > critical_value;
}

std::string format_bytes(usize bytes) {
    const char* units[] = {"B", "KB", "MB", "GB", "TB"};
    f64 size = static_cast<f64>(bytes);
    usize unit = 0;
    
    while (size >= 1024.0 && unit < 4) {
        size /= 1024.0;
        ++unit;
    }
    
    char buffer[64];
    std::snprintf(buffer, sizeof(buffer), "%.2f %s", size, units[unit]);
    return std::string(buffer);
}

std::string format_time(f64 milliseconds) {
    if (milliseconds < 1.0) {
        return std::to_string(static_cast<u32>(milliseconds * 1000.0)) + " μs";
    } else if (milliseconds < 1000.0) {
        char buffer[32];
        std::snprintf(buffer, sizeof(buffer), "%.2f ms", milliseconds);
        return std::string(buffer);
    } else {
        char buffer[32];
        std::snprintf(buffer, sizeof(buffer), "%.2f s", milliseconds / 1000.0);
        return std::string(buffer);
    }
}

std::string format_percentage(f64 ratio) {
    char buffer[32];
    std::snprintf(buffer, sizeof(buffer), "%.2f%%", ratio * 100.0);
    return std::string(buffer);
}

std::string format_rate(f64 rate, const std::string& unit) {
    char buffer[64];
    std::snprintf(buffer, sizeof(buffer), "%.2f %s", rate, unit.c_str());
    return std::string(buffer);
}

} // namespace lab_utils

// PerformanceLab Implementation
PerformanceLab::PerformanceLab() noexcept 
    : monitoring_start_time_(0.0)
    , enable_real_time_analysis_(true)
    , snapshot_interval_(0.1) // 100ms default
    , max_history_size_(1000)
{
    // Initialize educational explanations
    educational_explanations_["cache_locality"] = 
        "Cache locality refers to the tendency of a processor to access the same set of memory locations "
        "repetitively over a short period. Good cache locality improves performance by keeping frequently "
        "accessed data in fast cache memory.";
    
    educational_explanations_["soa_vs_aos"] = 
        "Structure of Arrays (SoA) organizes data by storing all instances of each field together, "
        "improving cache locality for operations that access only certain fields. Array of Structures (AoS) "
        "stores complete objects together, better for operations that need all fields of an object.";
    
    educational_explanations_["memory_fragmentation"] = 
        "Memory fragmentation occurs when free memory is broken into small, non-contiguous blocks. "
        "External fragmentation happens between allocated blocks, while internal fragmentation "
        "occurs within allocated blocks due to alignment requirements.";
    
    educational_explanations_["archetype_migration"] = 
        "Archetype migration in ECS systems occurs when entities gain or lose components, requiring "
        "them to move between different memory layouts (archetypes). This operation has performance "
        "costs that can be measured and optimized.";
        
    LOG_INFO("Performance Laboratory initialized with educational mission");
}

PerformanceLab::~PerformanceLab() noexcept {
    shutdown();
}

void PerformanceLab::set_ecs_registry(std::weak_ptr<ecs::Registry> registry) noexcept {
    ecs_registry_ = registry;
}

void PerformanceLab::set_physics_world(std::weak_ptr<physics::PhysicsWorld> world) noexcept {
    physics_world_ = world;
}

void PerformanceLab::set_renderer(std::weak_ptr<renderer::Renderer2D> renderer) noexcept {
    renderer_ = renderer;
}

bool PerformanceLab::initialize() {
    LOG_INFO("Initializing Performance Laboratory components...");
    
    try {
        // Initialize core laboratory components
        memory_experiments_ = std::make_unique<MemoryExperiments>(ecs_registry_);
        allocation_benchmarks_ = std::make_unique<AllocationBenchmarks>();
        
        // TODO: Initialize ECS profiler and system integration analyzer
        // ecs_profiler_ = std::make_unique<ECSProfiler>(ecs_registry_);
        // integration_analyzer_ = std::make_unique<SystemIntegrationAnalyzer>();
        
        // Register standard experiments
        register_experiment(std::make_unique<MemoryAccessExperiment>());
        
        LOG_INFO("Performance Laboratory components initialized successfully");
        return true;
    }
    catch (const std::exception& e) {
        LOG_ERROR("Failed to initialize Performance Laboratory: {}", e.what());
        return false;
    }
}

void PerformanceLab::shutdown() {
    LOG_INFO("Shutting down Performance Laboratory...");
    
    stop_monitoring();
    
    if (experiment_thread_ && experiment_thread_->joinable()) {
        cancel_current_experiment();
        experiment_thread_->join();
    }
    
    experiments_.clear();
    results_cache_.clear();
    performance_history_.clear();
    
    LOG_INFO("Performance Laboratory shutdown complete");
}

void PerformanceLab::reset_all_data() {
    std::lock_guard<std::mutex> lock(state_mutex_);
    
    results_cache_.clear();
    performance_history_.clear();
    current_recommendations_.clear();
    current_insights_.clear();
    
    LOG_INFO("Performance Laboratory data reset");
}

void PerformanceLab::start_monitoring() {
    if (is_monitoring_.load()) {
        LOG_WARNING("Performance monitoring is already active");
        return;
    }
    
    monitoring_start_time_ = core::Time::now();
    should_stop_monitoring_.store(false);
    is_monitoring_.store(true);
    
    monitoring_thread_ = std::make_unique<std::thread>(&PerformanceLab::monitoring_loop, this);
    
    LOG_INFO("Performance monitoring started (interval: {}ms)", snapshot_interval_ * 1000.0);
}

void PerformanceLab::stop_monitoring() {
    if (!is_monitoring_.load()) {
        return;
    }
    
    should_stop_monitoring_.store(true);
    
    if (monitoring_thread_ && monitoring_thread_->joinable()) {
        monitoring_thread_->join();
    }
    
    is_monitoring_.store(false);
    
    LOG_INFO("Performance monitoring stopped");
}

void PerformanceLab::register_experiment(std::unique_ptr<IPerformanceExperiment> experiment) {
    if (!experiment) {
        LOG_ERROR("Cannot register null experiment");
        return;
    }
    
    std::string name = experiment->get_name();
    experiments_.push_back(std::move(experiment));
    
    LOG_INFO("Registered experiment: {}", name);
}

std::vector<std::string> PerformanceLab::get_available_experiments() const {
    std::vector<std::string> names;
    names.reserve(experiments_.size());
    
    for (const auto& experiment : experiments_) {
        names.push_back(experiment->get_name());
    }
    
    return names;
}

std::string PerformanceLab::get_experiment_description(const std::string& name) const {
    for (const auto& experiment : experiments_) {
        if (experiment->get_name() == name) {
            return experiment->get_description();
        }
    }
    return "Experiment not found";
}

BenchmarkResult PerformanceLab::run_experiment(const std::string& name, const ExperimentConfig& config) {
    LOG_INFO("Running experiment: {} (precision: {}, iterations: {})", 
             name, static_cast<int>(config.precision), config.iterations);
    
    // Find the experiment
    IPerformanceExperiment* experiment = nullptr;
    for (const auto& exp : experiments_) {
        if (exp->get_name() == name) {
            experiment = exp.get();
            break;
        }
    }
    
    if (!experiment) {
        LOG_ERROR("Experiment not found: {}", name);
        BenchmarkResult result;
        result.name = name;
        result.is_valid = false;
        result.error_message = "Experiment not found";
        return result;
    }
    
    return execute_experiment_internal(experiment, config);
}

bool PerformanceLab::start_experiment_async(const std::string& name, const ExperimentConfig& config) {
    if (current_status_.load() == ExperimentStatus::Running) {
        LOG_WARNING("Cannot start async experiment: another experiment is running");
        return false;
    }
    
    current_experiment_name_ = name;
    current_status_.store(ExperimentStatus::Running);
    
    experiment_thread_ = std::make_unique<std::thread>([this, name, config]() {
        BenchmarkResult result = run_experiment(name, config);
        
        {
            std::lock_guard<std::mutex> lock(state_mutex_);
            cache_result(name, result);
        }
        
        current_status_.store(result.is_valid ? ExperimentStatus::Completed : ExperimentStatus::Failed);
        
        LOG_INFO("Async experiment completed: {} (valid: {})", name, result.is_valid);
    });
    
    return true;
}

ExperimentStatus PerformanceLab::get_experiment_status() const noexcept {
    return current_status_.load();
}

std::optional<BenchmarkResult> PerformanceLab::get_experiment_result() {
    if (current_status_.load() != ExperimentStatus::Completed) {
        return std::nullopt;
    }
    
    std::lock_guard<std::mutex> lock(state_mutex_);
    auto it = results_cache_.find(current_experiment_name_);
    if (it != results_cache_.end()) {
        return it->second;
    }
    
    return std::nullopt;
}

void PerformanceLab::cancel_current_experiment() {
    if (current_status_.load() == ExperimentStatus::Running) {
        current_status_.store(ExperimentStatus::Cancelled);
        LOG_INFO("Experiment cancelled: {}", current_experiment_name_);
    }
}

std::vector<BenchmarkResult> PerformanceLab::run_experiment_suite(const std::vector<std::string>& experiment_names,
                                                                  const ExperimentConfig& config) {
    LOG_INFO("Running experiment suite with {} experiments", experiment_names.size());
    
    std::vector<BenchmarkResult> results;
    results.reserve(experiment_names.size());
    
    for (const std::string& name : experiment_names) {
        BenchmarkResult result = run_experiment(name, config);
        results.push_back(result);
        
        if (!result.is_valid) {
            LOG_WARNING("Experiment failed: {} - {}", name, result.error_message);
        }
    }
    
    LOG_INFO("Experiment suite completed: {}/{} successful", 
             std::count_if(results.begin(), results.end(), 
                          [](const BenchmarkResult& r) { return r.is_valid; }),
             results.size());
    
    return results;
}

std::vector<BenchmarkResult> PerformanceLab::get_all_results() const {
    std::lock_guard<std::mutex> lock(state_mutex_);
    
    std::vector<BenchmarkResult> results;
    results.reserve(results_cache_.size());
    
    for (const auto& [name, result] : results_cache_) {
        results.push_back(result);
    }
    
    return results;
}

std::optional<BenchmarkResult> PerformanceLab::get_result(const std::string& experiment_name) const {
    std::lock_guard<std::mutex> lock(state_mutex_);
    
    auto it = results_cache_.find(experiment_name);
    if (it != results_cache_.end()) {
        return it->second;
    }
    
    return std::nullopt;
}

void PerformanceLab::clear_results_cache() {
    std::lock_guard<std::mutex> lock(state_mutex_);
    results_cache_.clear();
    LOG_INFO("Results cache cleared");
}

std::vector<SystemPerformanceSnapshot> PerformanceLab::get_performance_history() const {
    return performance_history_;
}

SystemPerformanceSnapshot PerformanceLab::get_current_snapshot() const {
    return capture_snapshot();
}

void PerformanceLab::clear_performance_history() {
    performance_history_.clear();
    LOG_INFO("Performance history cleared");
}

std::vector<PerformanceRecommendation> PerformanceLab::get_current_recommendations() const {
    std::lock_guard<std::mutex> lock(recommendations_mutex_);
    return current_recommendations_;
}

std::vector<std::string> PerformanceLab::get_current_insights() const {
    return current_insights_;
}

void PerformanceLab::force_recommendations_update() {
    update_recommendations();
}

std::string PerformanceLab::get_explanation(const std::string& topic) const {
    auto it = educational_explanations_.find(topic);
    if (it != educational_explanations_.end()) {
        return it->second;
    }
    return "No explanation available for topic: " + topic;
}

std::vector<std::string> PerformanceLab::get_available_explanations() const {
    std::vector<std::string> topics;
    topics.reserve(educational_explanations_.size());
    
    for (const auto& [topic, explanation] : educational_explanations_) {
        topics.push_back(topic);
    }
    
    return topics;
}

void PerformanceLab::add_explanation(const std::string& topic, const std::string& explanation) {
    educational_explanations_[topic] = explanation;
}

void PerformanceLab::set_default_config(const ExperimentConfig& config) {
    default_config_ = config;
}

ExperimentConfig PerformanceLab::get_default_config() const {
    return default_config_;
}

void PerformanceLab::enable_real_time_analysis(bool enable) noexcept {
    enable_real_time_analysis_ = enable;
}

void PerformanceLab::set_snapshot_interval(f64 interval) noexcept {
    snapshot_interval_ = std::max(0.001, interval); // Minimum 1ms
}

void PerformanceLab::set_max_history_size(usize size) noexcept {
    max_history_size_ = std::max(static_cast<usize>(10), size);
}

MemoryExperiments& PerformanceLab::get_memory_experiments() {
    return *memory_experiments_;
}

AllocationBenchmarks& PerformanceLab::get_allocation_benchmarks() {
    return *allocation_benchmarks_;
}

const MemoryExperiments& PerformanceLab::get_memory_experiments() const {
    return *memory_experiments_;
}

const AllocationBenchmarks& PerformanceLab::get_allocation_benchmarks() const {
    return *allocation_benchmarks_;
}

f64 PerformanceLab::estimate_memory_efficiency() const {
    if (!memory_experiments_) return 0.0;
    return memory_experiments_->calculate_memory_efficiency_score();
}

f64 PerformanceLab::estimate_ecs_performance() const {
    // TODO: Implement ECS performance estimation
    return 0.8; // Placeholder
}

f64 PerformanceLab::estimate_overall_health_score() const {
    f64 memory_score = estimate_memory_efficiency();
    f64 ecs_score = estimate_ecs_performance();
    
    // Weight the scores
    return (memory_score * 0.6) + (ecs_score * 0.4);
}

void PerformanceLab::export_results_to_json(const std::string& filename) const {
    // TODO: Implement JSON export
    LOG_INFO("Exporting results to JSON: {}", filename);
}

void PerformanceLab::export_performance_report(const std::string& filename) const {
    // TODO: Implement performance report export
    LOG_INFO("Exporting performance report: {}", filename);
}

void PerformanceLab::export_recommendations_report(const std::string& filename) const {
    // TODO: Implement recommendations report export
    LOG_INFO("Exporting recommendations report: {}", filename);
}

void PerformanceLab::print_current_status() const {
    LOG_INFO("=== Performance Laboratory Status ===");
    LOG_INFO("Monitoring active: {}", is_monitoring_.load());
    LOG_INFO("Current experiment: {}", current_experiment_name_);
    LOG_INFO("Experiment status: {}", static_cast<int>(current_status_.load()));
    LOG_INFO("Available experiments: {}", experiments_.size());
    LOG_INFO("Cached results: {}", results_cache_.size());
    LOG_INFO("Performance history entries: {}", performance_history_.size());
    LOG_INFO("Current recommendations: {}", current_recommendations_.size());
}

void PerformanceLab::print_performance_summary() const {
    SystemPerformanceSnapshot snapshot = get_current_snapshot();
    
    LOG_INFO("=== Performance Summary ===");
    LOG_INFO("Memory usage: {}", lab_utils::format_bytes(snapshot.memory_usage_bytes));
    LOG_INFO("Entity count: {}", snapshot.entity_count);
    LOG_INFO("Archetype count: {}", snapshot.archetype_count);
    LOG_INFO("Memory efficiency: {:.2f}%", estimate_memory_efficiency() * 100.0);
    LOG_INFO("Overall health score: {:.2f}%", estimate_overall_health_score() * 100.0);
}

bool PerformanceLab::validate_system_integration() const {
    bool valid = true;
    
    if (ecs_registry_.expired()) {
        LOG_WARNING("ECS registry is not connected");
        valid = false;
    }
    
    if (physics_world_.expired()) {
        LOG_WARNING("Physics world is not connected");
        valid = false;
    }
    
    if (renderer_.expired()) {
        LOG_WARNING("Renderer is not connected");
        valid = false;
    }
    
    return valid;
}

// Private implementation methods

void PerformanceLab::monitoring_loop() {
    LOG_INFO("Performance monitoring loop started");
    
    auto last_snapshot_time = std::chrono::high_resolution_clock::now();
    
    while (!should_stop_monitoring_.load()) {
        auto now = std::chrono::high_resolution_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_snapshot_time).count();
        
        if (elapsed >= static_cast<long>(snapshot_interval_ * 1000.0)) {
            SystemPerformanceSnapshot snapshot = capture_snapshot();
            
            performance_history_.push_back(snapshot);
            
            // Maintain history size limit
            if (performance_history_.size() > max_history_size_) {
                performance_history_.erase(performance_history_.begin());
            }
            
            if (enable_real_time_analysis_) {
                analyze_performance_trends();
                update_recommendations();
            }
            
            last_snapshot_time = now;
        }
        
        // Sleep for a short time to avoid busy waiting
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    
    LOG_INFO("Performance monitoring loop ended");
}

SystemPerformanceSnapshot PerformanceLab::capture_snapshot() const {
    SystemPerformanceSnapshot snapshot;
    
    snapshot.timestamp = core::Time::now();
    
    // Memory metrics
    auto& memory_tracker = memory::MemoryTracker::get_instance();
    snapshot.memory_usage_bytes = memory_tracker.get_current_usage();
    snapshot.peak_memory_bytes = memory_tracker.get_peak_usage();
    
    // ECS metrics (if available)
    if (auto registry = ecs_registry_.lock()) {
        snapshot.entity_count = registry->get_entity_count();
        snapshot.archetype_count = registry->get_archetype_count();
    }
    
    // TODO: Add more comprehensive metrics collection
    // - CPU usage
    // - Cache metrics
    // - Frame timing
    // - Thread metrics
    
    return snapshot;
}

void PerformanceLab::analyze_performance_trends() {
    if (performance_history_.size() < 2) return;
    
    // Analyze recent performance trends
    const auto& latest = performance_history_.back();
    const auto& previous = performance_history_[performance_history_.size() - 2];
    
    // Check for significant changes
    f64 memory_change_ratio = static_cast<f64>(latest.memory_usage_bytes) / previous.memory_usage_bytes;
    
    if (memory_change_ratio > 1.2) {
        current_insights_.push_back("Memory usage increased significantly");
    } else if (memory_change_ratio < 0.8) {
        current_insights_.push_back("Memory usage decreased significantly");
    }
    
    // Keep insights list manageable
    if (current_insights_.size() > 10) {
        current_insights_.erase(current_insights_.begin());
    }
}

void PerformanceLab::update_recommendations() {
    std::lock_guard<std::mutex> lock(recommendations_mutex_);
    
    current_recommendations_.clear();
    
    // Generate recommendations based on current performance data
    f64 memory_efficiency = estimate_memory_efficiency();
    
    if (memory_efficiency < 0.7) {
        PerformanceRecommendation rec;
        rec.title = "Improve Memory Efficiency";
        rec.description = "Memory efficiency is below optimal levels. Consider using SoA data layouts or optimizing allocation patterns.";
        rec.priority = PerformanceRecommendation::Priority::High;
        rec.category = PerformanceRecommendation::Category::Memory;
        rec.estimated_improvement = (0.8 - memory_efficiency) * 100.0;
        rec.implementation_difficulty = 0.6;
        rec.educational_notes.push_back("SoA layouts improve cache locality for component systems");
        rec.educational_notes.push_back("Arena allocators reduce fragmentation");
        current_recommendations_.push_back(rec);
    }
    
    // Add more recommendations based on system analysis
    // TODO: Implement comprehensive recommendation system
}

BenchmarkResult PerformanceLab::execute_experiment_internal(IPerformanceExperiment* experiment, const ExperimentConfig& config) {
    if (!experiment) {
        BenchmarkResult result;
        result.is_valid = false;
        result.error_message = "Null experiment";
        return result;
    }
    
    try {
        // Setup experiment
        bool setup_success = experiment->setup(config);
        if (!setup_success) {
            BenchmarkResult result;
            result.name = experiment->get_name();
            result.is_valid = false;
            result.error_message = "Experiment setup failed";
            return result;
        }
        
        // Execute experiment
        BenchmarkResult result = experiment->execute();
        
        // Validate and enhance result
        validate_result(result, config);
        result.insights = generate_insights_from_result(result);
        
        // Cache result
        cache_result(experiment->get_name(), result);
        
        // Cleanup
        experiment->cleanup();
        
        return result;
        
    } catch (const std::exception& e) {
        BenchmarkResult result;
        result.name = experiment->get_name();
        result.is_valid = false;
        result.error_message = "Exception during execution: " + std::string(e.what());
        
        try {
            experiment->cleanup();
        } catch (...) {
            // Ignore cleanup errors
        }
        
        return result;
    }
}

void PerformanceLab::validate_result(BenchmarkResult& result, const ExperimentConfig& config) {
    // Basic validation
    if (result.execution_time_ms <= 0.0) {
        result.is_valid = false;
        result.error_message = "Invalid execution time";
        return;
    }
    
    if (result.throughput < 0.0 || result.efficiency_score < 0.0 || result.efficiency_score > 1.0) {
        LOG_WARNING("Suspicious performance metrics in result for {}", result.name);
    }
    
    // Calculate confidence level
    result.confidence_level = std::min(0.95, config.iterations / 100.0);
    
    result.is_valid = true;
}

f64 PerformanceLab::calculate_statistical_confidence(const std::vector<f64>& samples) {
    if (samples.size() < 5) return 0.3;
    if (samples.size() < 10) return 0.5;
    if (samples.size() < 30) return 0.7;
    if (samples.size() < 100) return 0.85;
    return 0.95;
}

std::vector<std::string> PerformanceLab::generate_insights_from_result(const BenchmarkResult& result) {
    std::vector<std::string> insights;
    
    if (result.efficiency_score > 0.9) {
        insights.push_back("Excellent performance - well optimized");
    } else if (result.efficiency_score > 0.7) {
        insights.push_back("Good performance with room for optimization");
    } else if (result.efficiency_score > 0.5) {
        insights.push_back("Moderate performance - optimization recommended");
    } else {
        insights.push_back("Poor performance - optimization needed");
    }
    
    if (result.cache_miss_rate > 0.3) {
        insights.push_back("High cache miss rate detected - consider improving data locality");
    }
    
    if (result.fragmentation_ratio > 0.2) {
        insights.push_back("Significant memory fragmentation - consider using arena allocators");
    }
    
    return insights;
}

void PerformanceLab::cache_result(const std::string& experiment_name, const BenchmarkResult& result) {
    results_cache_[experiment_name] = result;
}

// PerformanceLabFactory Implementation
std::unique_ptr<PerformanceLab> PerformanceLabFactory::create_default_lab() {
    auto lab = std::make_unique<PerformanceLab>();
    
    ExperimentConfig config;
    config.precision = MeasurementPrecision::Normal;
    config.iterations = 100;
    config.warmup_iterations = 10;
    
    lab->set_default_config(config);
    lab->initialize();
    
    return lab;
}

std::unique_ptr<PerformanceLab> PerformanceLabFactory::create_research_lab() {
    auto lab = std::make_unique<PerformanceLab>();
    
    ExperimentConfig config;
    config.precision = MeasurementPrecision::Research;
    config.iterations = 1000;
    config.warmup_iterations = 100;
    config.capture_detailed_metrics = true;
    
    lab->set_default_config(config);
    lab->set_max_history_size(5000);
    lab->set_snapshot_interval(0.05); // 50ms for detailed analysis
    lab->initialize();
    
    return lab;
}

std::unique_ptr<PerformanceLab> PerformanceLabFactory::create_educational_lab() {
    auto lab = std::make_unique<PerformanceLab>();
    
    ExperimentConfig config;
    config.precision = MeasurementPrecision::Normal;
    config.iterations = 50;
    config.warmup_iterations = 5;
    config.enable_visualization = true;
    
    lab->set_default_config(config);
    lab->enable_real_time_analysis(true);
    lab->initialize();
    
    return lab;
}

std::unique_ptr<PerformanceLab> PerformanceLabFactory::create_production_lab() {
    auto lab = std::make_unique<PerformanceLab>();
    
    ExperimentConfig config;
    config.precision = MeasurementPrecision::Fast;
    config.iterations = 10;
    config.warmup_iterations = 2;
    config.max_duration_seconds = 5.0;
    
    lab->set_default_config(config);
    lab->set_snapshot_interval(1.0); // 1 second intervals
    lab->initialize();
    
    return lab;
}

ExperimentConfig PerformanceLabFactory::create_fast_config() {
    ExperimentConfig config;
    config.precision = MeasurementPrecision::Fast;
    config.iterations = 10;
    config.warmup_iterations = 2;
    config.max_duration_seconds = 5.0;
    config.capture_detailed_metrics = false;
    return config;
}

ExperimentConfig PerformanceLabFactory::create_precise_config() {
    ExperimentConfig config;
    config.precision = MeasurementPrecision::Precise;
    config.iterations = 500;
    config.warmup_iterations = 50;
    config.max_duration_seconds = 60.0;
    config.capture_detailed_metrics = true;
    return config;
}

ExperimentConfig PerformanceLabFactory::create_research_config() {
    ExperimentConfig config;
    config.precision = MeasurementPrecision::Research;
    config.iterations = 2000;
    config.warmup_iterations = 200;
    config.max_duration_seconds = 300.0;
    config.capture_detailed_metrics = true;
    return config;
}

ExperimentConfig PerformanceLabFactory::create_educational_config() {
    ExperimentConfig config;
    config.precision = MeasurementPrecision::Normal;
    config.iterations = 100;
    config.warmup_iterations = 10;
    config.max_duration_seconds = 30.0;
    config.capture_detailed_metrics = true;
    config.enable_visualization = true;
    return config;
}

} // namespace ecscope::performance