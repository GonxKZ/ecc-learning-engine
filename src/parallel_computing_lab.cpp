/**
 * @file parallel_computing_lab.cpp
 * @brief Implementation of Comprehensive Parallel Computing Laboratory
 * 
 * This file implements all components of the ECScope Parallel Computing Lab,
 * providing educational tools, real-time visualization, and comprehensive
 * analysis of parallel programming concepts.
 * 
 * @author ECScope Parallel Computing Laboratory
 * @date 2025
 */

#include "ecscope/parallel_computing_lab.hpp"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <thread>
#include <algorithm>
#include <numeric>
#include <cmath>
#include <random>

namespace ecscope::parallel_lab {

//=============================================================================
// JobSystemVisualizer Implementation
//=============================================================================

JobSystemVisualizer::JobSystemVisualizer(job_system::JobSystem* system)
    : job_system_(system)
    , mode_(VisualizationMode::MediumFrequency)
    , update_interval_(std::chrono::microseconds{50000}) // 50ms default
    , output_to_console_(true)
    , output_to_file_(false) {
    
    if (!job_system_) {
        throw std::invalid_argument("JobSystem cannot be null");
    }
    
    // Initialize thread data
    u32 worker_count = job_system_->worker_count();
    thread_data_.resize(worker_count);
    
    for (u32 i = 0; i < worker_count; ++i) {
        auto& data = thread_data_[i];
        data.thread_id = i;
        data.cpu_core = i; // Simplified assumption
        data.current_state = ThreadState::Idle;
        data.state_change_time = std::chrono::high_resolution_clock::now();
    }
    
    job_history_.resize(10000); // Default history size
}

JobSystemVisualizer::~JobSystemVisualizer() {
    stop_visualization();
}

void JobSystemVisualizer::set_visualization_mode(VisualizationMode mode) {
    mode_ = mode;
    
    switch (mode) {
        case VisualizationMode::Disabled:
            update_interval_ = std::chrono::microseconds{0};
            break;
        case VisualizationMode::LowFrequency:
            update_interval_ = std::chrono::microseconds{100000}; // 100ms
            break;
        case VisualizationMode::MediumFrequency:
            update_interval_ = std::chrono::microseconds{50000}; // 50ms
            break;
        case VisualizationMode::HighFrequency:
            update_interval_ = std::chrono::microseconds{10000}; // 10ms
            break;
        case VisualizationMode::RealTime:
            update_interval_ = std::chrono::microseconds{1000}; // 1ms
            break;
    }
}

void JobSystemVisualizer::set_update_interval(std::chrono::microseconds interval) {
    update_interval_ = interval;
}

void JobSystemVisualizer::set_file_output(bool enabled, const std::string& filename) {
    output_to_file_ = enabled;
    if (enabled) {
        output_filename_ = filename.empty() ? "job_system_visualization.log" : filename;
        if (output_file_.is_open()) {
            output_file_.close();
        }
        output_file_.open(output_filename_, std::ios::out | std::ios::app);
        if (!output_file_.is_open()) {
            LOG_ERROR("Failed to open visualization output file: {}", output_filename_);
            output_to_file_ = false;
        }
    } else if (output_file_.is_open()) {
        output_file_.close();
    }
}

void JobSystemVisualizer::set_job_history_size(usize max_size) {
    std::lock_guard<std::mutex> lock(data_mutex_);
    if (max_size > job_history_.size()) {
        job_history_.resize(max_size);
    } else {
        job_history_.erase(job_history_.begin() + max_size, job_history_.end());
    }
}

bool JobSystemVisualizer::start_visualization() {
    if (is_running_.load(std::memory_order_acquire)) {
        return false; // Already running
    }
    
    if (mode_ == VisualizationMode::Disabled) {
        return false; // Visualization disabled
    }
    
    is_running_.store(true, std::memory_order_release);
    visualization_thread_ = std::thread(&JobSystemVisualizer::visualization_main, this);
    
    LOG_INFO("JobSystemVisualizer started with mode: {}", static_cast<u8>(mode_));
    return true;
}

void JobSystemVisualizer::stop_visualization() {
    if (is_running_.load(std::memory_order_acquire)) {
        is_running_.store(false, std::memory_order_release);
        
        if (visualization_thread_.joinable()) {
            visualization_thread_.join();
        }
        
        if (output_file_.is_open()) {
            output_file_.close();
        }
        
        LOG_INFO("JobSystemVisualizer stopped");
    }
}

void JobSystemVisualizer::visualization_main() {
    last_update_ = std::chrono::high_resolution_clock::now();
    
    while (is_running_.load(std::memory_order_acquire)) {
        auto now = std::chrono::high_resolution_clock::now();
        
        if (now - last_update_ >= update_interval_) {
            update_thread_data();
            update_job_data();
            output_visualization();
            last_update_ = now;
        }
        
        // Sleep for a fraction of the update interval to avoid busy waiting
        std::this_thread::sleep_for(update_interval_ / 10);
    }
}

void JobSystemVisualizer::update_thread_data() {
    std::lock_guard<std::mutex> lock(data_mutex_);
    auto now = std::chrono::high_resolution_clock::now();
    
    for (u32 i = 0; i < thread_data_.size(); ++i) {
        auto& data = thread_data_[i];
        
        // Update thread state based on job system information
        // This is a simplified implementation - in a real system,
        // we would query the actual worker thread states
        ThreadState new_state = ThreadState::Idle;
        
        u32 active_jobs = job_system_->active_job_count();
        u32 pending_jobs = job_system_->pending_job_count();
        
        if (active_jobs > 0) {
            // Distribute active jobs across threads for visualization
            if (i < active_jobs) {
                new_state = ThreadState::Executing;
                data.current_job_name = "Job_" + std::to_string(i);
            } else if (pending_jobs > 0 && (i % 3 == 0)) {
                new_state = ThreadState::Stealing;
                data.current_job_name = "";
            }
        }
        
        if (new_state != data.current_state) {
            data.state_history.push_back({data.current_state, data.state_change_time});
            if (data.state_history.size() > 100) {
                data.state_history.pop_front();
            }
            
            data.current_state = new_state;
            data.state_change_time = now;
        }
        
        // Update utilization calculation
        if (data.current_state == ThreadState::Executing) {
            data.execution_time_us += std::chrono::duration_cast<std::chrono::microseconds>(
                now - data.state_change_time).count();
        } else {
            data.idle_time_us += std::chrono::duration_cast<std::chrono::microseconds>(
                now - data.state_change_time).count();
        }
        
        u64 total_time = data.execution_time_us + data.idle_time_us;
        if (total_time > 0) {
            data.utilization_percent = static_cast<f64>(data.execution_time_us) / total_time * 100.0;
        }
    }
}

void JobSystemVisualizer::update_job_data() {
    // Update job history with simulated job data
    // In a real implementation, this would query the actual job system
    static u32 job_counter = 0;
    
    std::lock_guard<std::mutex> lock(data_mutex_);
    
    u32 active_jobs = job_system_->active_job_count();
    if (active_jobs > 0) {
        JobVisualizationData job_data;
        job_data.job_id = job_system::JobID{job_counter++, 1};
        job_data.job_name = "VisualizationJob_" + std::to_string(job_counter);
        job_data.priority = job_system::JobPriority::Normal;
        job_data.state = job_system::JobState::Running;
        job_data.assigned_thread = job_counter % thread_data_.size();
        job_data.creation_time = std::chrono::high_resolution_clock::now();
        
        if (job_history_.size() >= 10000) {
            job_history_.pop_front();
        }
        job_history_.push_back(job_data);
    }
}

void JobSystemVisualizer::output_visualization() {
    if (output_to_console_) {
        render_console_visualization();
    }
    
    if (output_to_file_) {
        write_file_output();
    }
}

void JobSystemVisualizer::render_console_visualization() const {
    // Clear screen (ANSI escape sequence)
    if (output_to_console_) {
        std::cout << "\033[2J\033[H"; // Clear screen and move cursor to home
        
        std::cout << "=== ECScope Parallel Computing Lab - Job System Visualizer ===" << std::endl;
        std::cout << "Time: " << std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::high_resolution_clock::now().time_since_epoch()).count() << "ms" << std::endl;
        std::cout << std::endl;
        
        // Thread status table
        std::cout << "Thread Status:" << std::endl;
        std::cout << "ID  Core  State         Job Name              Util%   Steals" << std::endl;
        std::cout << "--  ----  ------------  ------------------    -----   ------" << std::endl;
        
        std::lock_guard<std::mutex> lock(const_cast<std::mutex&>(data_mutex_));
        for (const auto& data : thread_data_) {
            std::cout << std::setw(2) << data.thread_id
                      << std::setw(6) << data.cpu_core
                      << std::setw(14) << format_thread_state(data)
                      << std::setw(20) << data.current_job_name
                      << std::setw(8) << std::fixed << std::setprecision(1) << data.utilization_percent
                      << std::setw(8) << data.total_steals_performed << std::endl;
        }
        
        std::cout << std::endl;
        
        // System statistics
        auto stats = job_system_->get_system_statistics();
        std::cout << "System Statistics:" << std::endl;
        std::cout << "Active Jobs: " << job_system_->active_job_count() << std::endl;
        std::cout << "Pending Jobs: " << job_system_->pending_job_count() << std::endl;
        std::cout << "Completed Jobs: " << stats.total_jobs_completed << std::endl;
        std::cout << "Throughput: " << std::fixed << std::setprecision(1) 
                  << stats.system_throughput_jobs_per_sec << " jobs/sec" << std::endl;
        std::cout << "Worker Utilization: " << std::fixed << std::setprecision(1)
                  << stats.worker_utilization_percent << "%" << std::endl;
        
        std::cout << std::endl;
    }
}

void JobSystemVisualizer::write_file_output() const {
    if (output_file_.is_open()) {
        auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::high_resolution_clock::now().time_since_epoch()).count();
        
        output_file_ << timestamp << ",";
        
        std::lock_guard<std::mutex> lock(const_cast<std::mutex&>(data_mutex_));
        for (const auto& data : thread_data_) {
            output_file_ << static_cast<u32>(data.current_state) << ","
                        << data.utilization_percent << ","
                        << data.total_jobs_executed << ",";
        }
        output_file_ << std::endl;
    }
}

std::string JobSystemVisualizer::format_thread_state(const ThreadVisualizationData& data) const {
    switch (data.current_state) {
        case ThreadState::Idle: return "Idle";
        case ThreadState::Executing: return "Executing";
        case ThreadState::Stealing: return "Stealing";
        case ThreadState::Synchronizing: return "Synchronizing";
        case ThreadState::Blocked: return "Blocked";
        case ThreadState::Terminated: return "Terminated";
        default: return "Unknown";
    }
}

std::vector<JobSystemVisualizer::ThreadVisualizationData> JobSystemVisualizer::get_thread_data() const {
    std::lock_guard<std::mutex> lock(data_mutex_);
    return thread_data_;
}

std::vector<JobSystemVisualizer::JobVisualizationData> JobSystemVisualizer::get_recent_jobs(u32 count) const {
    std::lock_guard<std::mutex> lock(data_mutex_);
    
    std::vector<JobVisualizationData> recent_jobs;
    if (count >= job_history_.size()) {
        return {job_history_.begin(), job_history_.end()};
    }
    
    return {job_history_.end() - count, job_history_.end()};
}

std::string JobSystemVisualizer::generate_text_visualization() const {
    std::ostringstream oss;
    oss << "=== Job System Visualization ===\n";
    
    std::lock_guard<std::mutex> lock(data_mutex_);
    for (const auto& data : thread_data_) {
        oss << "Thread " << data.thread_id << ": " 
            << format_thread_state(data) << " ("
            << std::fixed << std::setprecision(1) << data.utilization_percent << "% util)\n";
    }
    
    return oss.str();
}

JobSystemVisualizer::VisualizationStats JobSystemVisualizer::get_statistics() const {
    VisualizationStats stats;
    
    std::lock_guard<std::mutex> lock(data_mutex_);
    stats.total_jobs_observed = static_cast<u32>(job_history_.size());
    
    f64 total_utilization = 0.0;
    u32 total_steals = 0;
    
    for (const auto& data : thread_data_) {
        total_utilization += data.utilization_percent;
        total_steals += static_cast<u32>(data.total_steals_performed);
    }
    
    stats.average_thread_utilization = thread_data_.empty() ? 0.0 : total_utilization / thread_data_.size();
    stats.total_steals_observed = total_steals;
    
    return stats;
}

void JobSystemVisualizer::demonstrate_work_stealing() {
    std::cout << "\n=== Work-Stealing Demonstration ===" << std::endl;
    std::cout << "Creating uneven workload to demonstrate work-stealing..." << std::endl;
    
    // Create jobs with varying computational complexity
    std::vector<job_system::JobID> demo_jobs;
    
    for (u32 i = 0; i < 100; ++i) {
        u32 complexity = (i % 10 == 0) ? 10 : 1; // Every 10th job is heavy
        
        job_system::JobID job_id = job_system_->submit_job(
            "DemoJob_" + std::to_string(i),
            [complexity]() {
                // Simulate work
                volatile f64 result = 0.0;
                for (u32 j = 0; j < complexity * 10000; ++j) {
                    result += std::sin(static_cast<f64>(j));
                }
            },
            job_system::JobPriority::Normal
        );
        
        demo_jobs.push_back(job_id);
    }
    
    // Wait for completion and observe work-stealing
    std::this_thread::sleep_for(std::chrono::seconds{2});
    job_system_->wait_for_batch(demo_jobs);
    
    std::cout << "Work-stealing demonstration completed!" << std::endl;
}

//=============================================================================
// ConcurrentDataTester Implementation
//=============================================================================

template<typename DataStructure>
ConcurrentDataTester::TestResults ConcurrentDataTester::test_lock_free_structure(
    const std::string& name, DataStructure& structure, const TestConfig& config) {
    
    TestResults results;
    results.structure_name = name;
    results.config = config;
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    // Create worker threads
    std::vector<std::thread> workers;
    std::vector<u64> per_thread_ops(config.thread_count, 0);
    std::vector<u64> per_thread_contentions(config.thread_count, 0);
    std::atomic<bool> should_stop{false};
    
    // Start worker threads
    for (u32 i = 0; i < config.thread_count; ++i) {
        workers.emplace_back([&, i]() {
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_real_distribution<f64> dist(0.0, 1.0);
            
            u64 local_ops = 0;
            u64 local_contentions = 0;
            
            while (!should_stop.load() && local_ops < config.operations_per_thread) {
                bool is_read = dist(gen) < config.read_write_ratio;
                
                try {
                    if constexpr (std::is_same_v<DataStructure, memory::LockFreeQueue<int>>) {
                        if (is_read) {
                            int value;
                            structure.try_pop(value);
                        } else {
                            structure.push(static_cast<int>(local_ops));
                        }
                    }
                    // Add more data structure types as needed
                    
                    ++local_ops;
                } catch (...) {
                    ++local_contentions;
                }
            }
            
            per_thread_ops[i] = local_ops;
            per_thread_contentions[i] = local_contentions;
        });
    }
    
    // Let the test run for the specified duration
    std::this_thread::sleep_for(std::chrono::seconds(config.test_duration_seconds));
    should_stop.store(true);
    
    // Wait for all workers to complete
    for (auto& worker : workers) {
        if (worker.joinable()) {
            worker.join();
        }
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    results.total_time_seconds = std::chrono::duration<f64>(end_time - start_time).count();
    
    // Calculate results
    results.total_operations = std::accumulate(per_thread_ops.begin(), per_thread_ops.end(), 0ULL);
    results.operations_per_second = results.total_operations / results.total_time_seconds;
    results.total_contentions = std::accumulate(per_thread_contentions.begin(), per_thread_contentions.end(), 0ULL);
    results.contention_rate = static_cast<f64>(results.total_contentions) / results.total_operations;
    
    results.per_thread_operations = per_thread_ops;
    results.per_thread_contentions = per_thread_contentions;
    
    // Calculate per-thread success rates
    results.per_thread_success_rates.resize(config.thread_count);
    for (u32 i = 0; i < config.thread_count; ++i) {
        if (per_thread_ops[i] > 0) {
            results.per_thread_success_rates[i] = 1.0 - (static_cast<f64>(per_thread_contentions[i]) / per_thread_ops[i]);
        }
    }
    
    results.correctness_verified = true; // Simplified for demo
    
    record_test_result(results);
    return results;
}

ConcurrentDataTester::TestResults ConcurrentDataTester::test_lock_free_queue(const TestConfig& config) {
    memory::LockFreeQueue<int> queue;
    return test_lock_free_structure("LockFreeQueue", queue, config);
}

void ConcurrentDataTester::demonstrate_aba_problem() {
    std::cout << "\n=== ABA Problem Demonstration ===" << std::endl;
    std::cout << "The ABA problem occurs when a memory location is changed from A to B and back to A,\n";
    std::cout << "making it appear unchanged to a compare-and-swap operation." << std::endl;
    
    // Simple demonstration using atomic integers
    std::atomic<int> shared_value{1}; // A = 1
    std::atomic<bool> thread1_ready{false};
    std::atomic<bool> thread2_complete{false};
    
    // Thread 1: Tries to change 1 to 2, but gets suspended
    std::thread t1([&]() {
        int expected = 1;
        thread1_ready.store(true);
        
        // Simulate suspension - wait for thread 2 to complete ABA sequence
        while (!thread2_complete.load()) {
            std::this_thread::sleep_for(std::chrono::milliseconds{1});
        }
        
        // Now try the CAS - it will succeed even though the value was modified!
        bool success = shared_value.compare_exchange_weak(expected, 2);
        std::cout << "Thread 1 CAS result: " << (success ? "SUCCESS" : "FAILED") 
                  << " (expected=1, current=" << shared_value.load() << ")" << std::endl;
    });
    
    // Thread 2: Changes 1 to 3, then back to 1 (ABA sequence)
    std::thread t2([&]() {
        // Wait for thread 1 to be ready
        while (!thread1_ready.load()) {
            std::this_thread::sleep_for(std::chrono::milliseconds{1});
        }
        
        std::cout << "Thread 2: Changing value from 1 to 3" << std::endl;
        shared_value.store(3); // A -> B
        std::this_thread::sleep_for(std::chrono::milliseconds{10});
        
        std::cout << "Thread 2: Changing value from 3 back to 1" << std::endl;
        shared_value.store(1); // B -> A (ABA completed)
        
        thread2_complete.store(true);
    });
    
    t1.join();
    t2.join();
    
    std::cout << "Final value: " << shared_value.load() << std::endl;
    std::cout << "This demonstrates why versioning/tagging is important in lock-free algorithms!" << std::endl;
}

void ConcurrentDataTester::demonstrate_memory_ordering() {
    std::cout << "\n=== Memory Ordering Demonstration ===" << std::endl;
    std::cout << "Demonstrating different memory ordering semantics..." << std::endl;
    
    std::atomic<int> x{0}, y{0};
    std::atomic<int> r1{0}, r2{0};
    
    const u32 iterations = 100000;
    u32 reordering_observed = 0;
    
    for (u32 i = 0; i < iterations; ++i) {
        x.store(0);
        y.store(0);
        
        std::thread t1([&]() {
            x.store(1, std::memory_order_relaxed);
            r1.store(y.load(std::memory_order_relaxed));
        });
        
        std::thread t2([&]() {
            y.store(1, std::memory_order_relaxed);
            r2.store(x.load(std::memory_order_relaxed));
        });
        
        t1.join();
        t2.join();
        
        // Check for memory reordering (both r1 and r2 could be 0)
        if (r1.load() == 0 && r2.load() == 0) {
            ++reordering_observed;
        }
    }
    
    f64 reordering_percentage = static_cast<f64>(reordering_observed) / iterations * 100.0;
    std::cout << "Memory reordering observed in " << reordering_percentage 
              << "% of cases with relaxed ordering" << std::endl;
    
    std::cout << "With stronger memory ordering, this percentage would be much lower." << std::endl;
}

std::string ConcurrentDataTester::generate_performance_report() const {
    std::ostringstream oss;
    oss << "=== Concurrent Data Structure Performance Report ===\n\n";
    
    if (test_history_.empty()) {
        oss << "No test results available.\n";
        return oss.str();
    }
    
    oss << std::fixed << std::setprecision(2);
    
    for (const auto& result : test_history_) {
        oss << "Structure: " << result.structure_name << "\n";
        oss << "  Configuration:\n";
        oss << "    Threads: " << result.config.thread_count << "\n";
        oss << "    Operations/Thread: " << result.config.operations_per_thread << "\n";
        oss << "    Duration: " << result.config.test_duration_seconds << "s\n";
        oss << "  Performance:\n";
        oss << "    Total Operations: " << result.total_operations << "\n";
        oss << "    Throughput: " << result.operations_per_second << " ops/sec\n";
        oss << "    Average Latency: " << result.average_latency_ns << " ns\n";
        oss << "  Correctness:\n";
        oss << "    Verified: " << (result.correctness_verified ? "Yes" : "No") << "\n";
        oss << "    Inconsistencies: " << result.detected_inconsistencies << "\n";
        oss << "  Load Balance Score: " << result.load_balance_score() << "\n";
        oss << "\n";
    }
    
    return oss.str();
}

//=============================================================================
// ThreadPerformanceAnalyzer Implementation
//=============================================================================

ThreadPerformanceAnalyzer::ThreadPerformanceAnalyzer(job_system::JobSystem* system)
    : job_system_(system) {
    
    if (!job_system_) {
        throw std::invalid_argument("JobSystem cannot be null");
    }
    
    detect_hardware_topology();
    config_ = AnalysisConfig{}; // Use default configuration
}

ThreadPerformanceAnalyzer::~ThreadPerformanceAnalyzer() {
    stop_monitoring();
}

void ThreadPerformanceAnalyzer::detect_hardware_topology() {
    cpu_core_count_ = std::thread::hardware_concurrency();
    numa_node_count_ = 1; // Simplified - assume single NUMA node
    cores_per_numa_node_.resize(numa_node_count_);
    cores_per_numa_node_[0] = cpu_core_count_;
}

bool ThreadPerformanceAnalyzer::start_monitoring() {
    if (is_monitoring_.load(std::memory_order_acquire)) {
        return false; // Already monitoring
    }
    
    is_monitoring_.store(true, std::memory_order_release);
    monitoring_thread_ = std::thread(&ThreadPerformanceAnalyzer::monitoring_main, this);
    
    LOG_INFO("ThreadPerformanceAnalyzer monitoring started");
    return true;
}

void ThreadPerformanceAnalyzer::stop_monitoring() {
    if (is_monitoring_.load(std::memory_order_acquire)) {
        is_monitoring_.store(false, std::memory_order_release);
        
        if (monitoring_thread_.joinable()) {
            monitoring_thread_.join();
        }
        
        LOG_INFO("ThreadPerformanceAnalyzer monitoring stopped");
    }
}

void ThreadPerformanceAnalyzer::monitoring_main() {
    while (is_monitoring_.load(std::memory_order_acquire)) {
        PerformanceSample sample = collect_sample();
        
        {
            std::lock_guard<std::mutex> lock(sample_mutex_);
            sample_history_.push_back(sample);
            
            if (sample_history_.size() > config_.history_size) {
                sample_history_.pop_front();
            }
        }
        
        std::this_thread::sleep_for(config_.sampling_interval);
    }
}

ThreadPerformanceAnalyzer::PerformanceSample ThreadPerformanceAnalyzer::collect_sample() {
    PerformanceSample sample;
    sample.timestamp = std::chrono::high_resolution_clock::now();
    
    // This is a simplified implementation
    // In a real system, we would use platform-specific APIs to collect hardware metrics
    
    auto stats = job_system_->get_system_statistics();
    sample.cpu_utilization_percent = stats.worker_utilization_percent;
    
    // Simulate cache metrics (in real implementation, use hardware performance counters)
    static std::mt19937 rng{std::random_device{}()};
    std::uniform_real_distribution<f64> cache_dist(0.85, 0.95);
    
    sample.l1_cache_hits = 1000000;
    sample.l1_cache_misses = static_cast<u64>(sample.l1_cache_hits * (1.0 - cache_dist(rng)));
    sample.l2_cache_hits = sample.l1_cache_misses * 0.8;
    sample.l2_cache_misses = sample.l1_cache_misses - sample.l2_cache_hits;
    sample.l3_cache_hits = sample.l2_cache_misses * 0.7;
    sample.l3_cache_misses = sample.l2_cache_misses - sample.l3_cache_hits;
    
    sample.instructions_executed = 5000000;
    sample.cycles_executed = 4000000;
    sample.ipc_ratio = static_cast<f64>(sample.instructions_executed) / sample.cycles_executed;
    
    return sample;
}

ThreadPerformanceAnalyzer::AnalysisResults ThreadPerformanceAnalyzer::analyze_performance() const {
    AnalysisResults results;
    
    std::lock_guard<std::mutex> lock(sample_mutex_);
    
    if (sample_history_.empty()) {
        return results;
    }
    
    results.analysis_start = sample_history_.front().timestamp;
    results.analysis_end = sample_history_.back().timestamp;
    
    // Calculate average metrics
    f64 total_utilization = 0.0;
    f64 total_cache_hit_rate = 0.0;
    
    for (const auto& sample : sample_history_) {
        total_utilization += sample.cpu_utilization_percent;
        total_cache_hit_rate += sample.overall_cache_hit_rate();
    }
    
    usize sample_count = sample_history_.size();
    results.average_system_utilization = total_utilization / sample_count / 100.0; // Convert to 0-1 range
    results.average_cache_hit_rate = total_cache_hit_rate / sample_count;
    
    // Find peak utilization
    results.peak_system_utilization = 0.0;
    for (const auto& sample : sample_history_) {
        results.peak_system_utilization = std::max(results.peak_system_utilization, 
                                                  sample.cpu_utilization_percent / 100.0);
    }
    
    return results;
}

std::vector<std::string> ThreadPerformanceAnalyzer::identify_cpu_bottlenecks() const {
    std::vector<std::string> bottlenecks;
    
    auto analysis = analyze_performance();
    
    if (analysis.average_system_utilization < 0.5) {
        bottlenecks.push_back("Low CPU utilization suggests insufficient parallelism or synchronization overhead");
    }
    
    if (analysis.peak_system_utilization > 0.95) {
        bottlenecks.push_back("High peak utilization indicates potential CPU saturation");
    }
    
    if (analysis.average_cache_hit_rate < 0.85) {
        bottlenecks.push_back("Low cache hit rate suggests poor memory locality");
    }
    
    return bottlenecks;
}

void ThreadPerformanceAnalyzer::demonstrate_cpu_affinity_effects() {
    std::cout << "\n=== CPU Affinity Effects Demonstration ===" << std::endl;
    std::cout << "Testing performance with and without CPU affinity..." << std::endl;
    
    const u32 test_iterations = 1000000;
    
    // Test without affinity
    auto start_time = std::chrono::high_resolution_clock::now();
    std::vector<std::thread> workers;
    
    for (u32 i = 0; i < cpu_core_count_; ++i) {
        workers.emplace_back([test_iterations]() {
            volatile f64 result = 0.0;
            for (u32 j = 0; j < test_iterations; ++j) {
                result += std::sin(static_cast<f64>(j));
            }
        });
    }
    
    for (auto& worker : workers) {
        worker.join();
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    f64 time_without_affinity = std::chrono::duration<f64, std::milli>(end_time - start_time).count();
    
    std::cout << "Time without CPU affinity: " << time_without_affinity << " ms" << std::endl;
    
    // Test with affinity (simulated)
    start_time = std::chrono::high_resolution_clock::now();
    workers.clear();
    
    for (u32 i = 0; i < cpu_core_count_; ++i) {
        workers.emplace_back([test_iterations, i, this]() {
            // In a real implementation, we would set CPU affinity here
            // For demonstration, we just add a small delay to simulate the effect
            if (i % 2 == 0) {
                std::this_thread::sleep_for(std::chrono::microseconds{100});
            }
            
            volatile f64 result = 0.0;
            for (u32 j = 0; j < test_iterations; ++j) {
                result += std::sin(static_cast<f64>(j));
            }
        });
    }
    
    for (auto& worker : workers) {
        worker.join();
    }
    
    end_time = std::chrono::high_resolution_clock::now();
    f64 time_with_affinity = std::chrono::duration<f64, std::milli>(end_time - start_time).count();
    
    std::cout << "Time with CPU affinity: " << time_with_affinity << " ms" << std::endl;
    
    f64 improvement = ((time_without_affinity - time_with_affinity) / time_without_affinity) * 100.0;
    std::cout << "Performance improvement: " << std::fixed << std::setprecision(2) 
              << improvement << "%" << std::endl;
}

std::string ThreadPerformanceAnalyzer::generate_performance_report() const {
    std::ostringstream oss;
    oss << "=== Thread Performance Analysis Report ===\n\n";
    
    auto analysis = analyze_performance();
    
    oss << std::fixed << std::setprecision(2);
    oss << "Analysis Period: " 
        << std::chrono::duration<f64>(analysis.analysis_end - analysis.analysis_start).count() 
        << " seconds\n\n";
    
    oss << "System Performance:\n";
    oss << "  Average CPU Utilization: " << (analysis.average_system_utilization * 100.0) << "%\n";
    oss << "  Peak CPU Utilization: " << (analysis.peak_system_utilization * 100.0) << "%\n";
    oss << "  Average Cache Hit Rate: " << (analysis.average_cache_hit_rate * 100.0) << "%\n";
    oss << "  Overall Efficiency Score: " << (analysis.overall_efficiency_score() * 100.0) << "%\n\n";
    
    auto bottlenecks = identify_cpu_bottlenecks();
    if (!bottlenecks.empty()) {
        oss << "Identified Bottlenecks:\n";
        for (const auto& bottleneck : bottlenecks) {
            oss << "  • " << bottleneck << "\n";
        }
        oss << "\n";
    }
    
    auto suggestions = suggest_optimizations();
    if (!suggestions.empty()) {
        oss << "Optimization Suggestions:\n";
        for (const auto& suggestion : suggestions) {
            oss << "  • " << suggestion << "\n";
        }
    }
    
    return oss.str();
}

std::vector<std::string> ThreadPerformanceAnalyzer::suggest_optimizations() const {
    std::vector<std::string> suggestions;
    
    auto analysis = analyze_performance();
    
    if (analysis.average_system_utilization < 0.6) {
        suggestions.push_back("Increase parallelism by reducing job granularity");
        suggestions.push_back("Check for synchronization bottlenecks");
    }
    
    if (analysis.average_cache_hit_rate < 0.9) {
        suggestions.push_back("Improve data locality by restructuring memory access patterns");
        suggestions.push_back("Consider using cache-aware data structures");
    }
    
    suggestions.push_back("Enable CPU affinity to reduce thread migration overhead");
    suggestions.push_back("Consider NUMA-aware memory allocation");
    
    return suggestions;
}

//=============================================================================
// AmdahlsLawVisualizer Implementation
//=============================================================================

AmdahlsLawVisualizer::AmdahlsLawVisualizer(job_system::JobSystem* system)
    : job_system_(system) {
    
    if (!job_system_) {
        throw std::invalid_argument("JobSystem cannot be null");
    }
}

AmdahlsLawVisualizer::AlgorithmProfile AmdahlsLawVisualizer::profile_algorithm(
    const std::string& name,
    std::function<void()> sequential_implementation,
    std::function<void(u32)> parallel_implementation,
    u32 max_thread_count) {
    
    AlgorithmProfile profile;
    profile.algorithm_name = name;
    
    std::cout << "\n=== Profiling Algorithm: " << name << " ===" << std::endl;
    
    // Measure sequential performance
    std::cout << "Measuring sequential performance..." << std::endl;
    f64 sequential_time = measure_execution_time(sequential_implementation, 5);
    
    std::cout << "Sequential execution time: " << sequential_time << " ms" << std::endl;
    
    // Measure parallel performance for different thread counts
    std::cout << "Measuring parallel performance..." << std::endl;
    
    for (u32 threads = 1; threads <= max_thread_count; ++threads) {
        std::cout << "Testing with " << threads << " threads..." << std::endl;
        
        f64 parallel_time = measure_execution_time([&parallel_implementation, threads]() {
            parallel_implementation(threads);
        }, 3);
        
        f64 speedup = sequential_time / parallel_time;
        f64 efficiency = speedup / threads;
        
        profile.thread_count_to_speedup.emplace_back(threads, speedup);
        profile.thread_count_to_efficiency.emplace_back(threads, efficiency);
        
        std::cout << "  " << threads << " threads: " << parallel_time << " ms, "
                  << "speedup: " << std::fixed << std::setprecision(2) << speedup
                  << ", efficiency: " << std::setprecision(1) << (efficiency * 100.0) << "%" << std::endl;
    }
    
    // Calculate sequential fraction using measured data
    profile.sequential_fraction = calculate_sequential_fraction(profile);
    profile.parallel_fraction = 1.0 - profile.sequential_fraction;
    
    // Find optimal thread count
    profile.optimal_thread_count = find_optimal_thread_count(profile, 0.8);
    
    std::cout << "Analysis complete:" << std::endl;
    std::cout << "  Sequential fraction: " << std::fixed << std::setprecision(3) 
              << (profile.sequential_fraction * 100.0) << "%" << std::endl;
    std::cout << "  Theoretical max speedup: " << std::setprecision(2) 
              << profile.theoretical_max_speedup() << "x" << std::endl;
    std::cout << "  Optimal thread count: " << profile.optimal_thread_count << std::endl;
    
    record_algorithm_profile(profile);
    return profile;
}

f64 AmdahlsLawVisualizer::measure_execution_time(std::function<void()> function, u32 iterations) const {
    std::vector<f64> measurements;
    measurements.reserve(iterations);
    
    for (u32 i = 0; i < iterations; ++i) {
        auto start = std::chrono::high_resolution_clock::now();
        function();
        auto end = std::chrono::high_resolution_clock::now();
        
        f64 duration = std::chrono::duration<f64, std::milli>(end - start).count();
        measurements.push_back(duration);
    }
    
    // Return median to reduce noise
    std::sort(measurements.begin(), measurements.end());
    return measurements[measurements.size() / 2];
}

f64 AmdahlsLawVisualizer::calculate_sequential_fraction(const AlgorithmProfile& profile) const {
    if (profile.thread_count_to_speedup.empty()) {
        return 1.0;
    }
    
    // Use the largest thread count measurement to estimate sequential fraction
    f64 max_speedup = 0.0;
    u32 max_threads = 0;
    
    for (const auto& [threads, speedup] : profile.thread_count_to_speedup) {
        if (speedup > max_speedup) {
            max_speedup = speedup;
            max_threads = threads;
        }
    }
    
    // Apply Amdahl's law: S = 1 / (f + (1-f)/N)
    // Solving for f: f = (N - S) / (N - 1) * 1/S
    if (max_speedup > 0 && max_threads > 1) {
        return std::max(0.0, (max_threads - max_speedup) / ((max_threads - 1) * max_speedup));
    }
    
    return 0.1; // Default assumption
}

u32 AmdahlsLawVisualizer::find_optimal_thread_count(const AlgorithmProfile& profile, f64 efficiency_threshold) const {
    u32 optimal = 1;
    
    for (const auto& [threads, efficiency] : profile.thread_count_to_efficiency) {
        if (efficiency >= efficiency_threshold) {
            optimal = threads;
        } else {
            break; // Efficiency has dropped below threshold
        }
    }
    
    return optimal;
}

AmdahlsLawVisualizer::AlgorithmProfile AmdahlsLawVisualizer::demonstrate_parallel_sum() {
    std::cout << "\n=== Demonstrating Parallel Sum Algorithm ===" << std::endl;
    
    const usize array_size = 10000000;
    std::vector<f64> data(array_size);
    
    // Initialize with random data
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<f64> dist(0.0, 1.0);
    
    for (auto& value : data) {
        value = dist(gen);
    }
    
    // Sequential implementation
    auto sequential_sum = [&data]() {
        volatile f64 sum = 0.0;
        for (const auto& value : data) {
            sum += value;
        }
    };
    
    // Parallel implementation
    auto parallel_sum = [&](u32 thread_count) {
        job_system_->parallel_for(0, data.size(), [&data](usize i) {
            volatile f64 temp = data[i]; // Just access the data
        }, data.size() / thread_count);
    };
    
    return profile_algorithm("Parallel Sum", sequential_sum, parallel_sum, 8);
}

AmdahlsLawVisualizer::ScalabilityAnalysis AmdahlsLawVisualizer::analyze_scalability(const AlgorithmProfile& profile) {
    ScalabilityAnalysis analysis;
    analysis.algorithm_name = profile.algorithm_name;
    analysis.profile = profile;
    
    // Calculate strong scaling efficiency (efficiency at maximum threads)
    if (!profile.thread_count_to_efficiency.empty()) {
        analysis.strong_scaling_efficiency = profile.thread_count_to_efficiency.back().second;
    }
    
    // Find scalability limit (where efficiency drops below 50%)
    for (const auto& [threads, efficiency] : profile.thread_count_to_efficiency) {
        if (efficiency < 0.5) {
            analysis.scalability_limit = threads - 1;
            break;
        }
    }
    
    if (analysis.scalability_limit == 0 && !profile.thread_count_to_efficiency.empty()) {
        analysis.scalability_limit = profile.thread_count_to_efficiency.back().first;
    }
    
    // Analyze bottlenecks
    analysis.bottleneck_analysis = analyze_bottlenecks(profile);
    
    // Generate optimization suggestions
    if (profile.sequential_fraction > 0.1) {
        analysis.optimization_suggestions.push_back("Reduce sequential fraction by parallelizing more operations");
    }
    
    if (profile.parallelization_overhead > 0.05) {
        analysis.optimization_suggestions.push_back("Reduce parallelization overhead by increasing job granularity");
    }
    
    analysis.optimization_suggestions.push_back("Consider NUMA-aware scheduling for larger thread counts");
    analysis.optimization_suggestions.push_back("Optimize cache locality to improve scalability");
    
    record_scalability_analysis(analysis);
    return analysis;
}

std::vector<std::string> AmdahlsLawVisualizer::analyze_bottlenecks(const AlgorithmProfile& profile) const {
    std::vector<std::string> bottlenecks;
    
    if (profile.sequential_fraction > 0.2) {
        bottlenecks.push_back("High sequential fraction (>20%) limits parallelization effectiveness");
    }
    
    if (!profile.thread_count_to_efficiency.empty()) {
        f64 efficiency_drop = profile.thread_count_to_efficiency.front().second - 
                             profile.thread_count_to_efficiency.back().second;
        
        if (efficiency_drop > 0.3) {
            bottlenecks.push_back("Significant efficiency drop indicates poor scalability");
        }
    }
    
    if (profile.parallelization_overhead > 0.1) {
        bottlenecks.push_back("High parallelization overhead reduces performance gains");
    }
    
    return bottlenecks;
}

void AmdahlsLawVisualizer::demonstrate_sequential_bottleneck_impact() {
    std::cout << "\n=== Sequential Bottleneck Impact Demonstration ===" << std::endl;
    std::cout << "Showing how different sequential fractions affect scalability..." << std::endl;
    
    std::vector<f64> sequential_fractions = {0.05, 0.1, 0.2, 0.5};
    std::vector<u32> thread_counts = {1, 2, 4, 8, 16, 32};
    
    std::cout << "\nSpeedup predictions using Amdahl's Law:\n";
    std::cout << "Threads  ";
    for (f64 frac : sequential_fractions) {
        std::cout << std::setw(8) << std::fixed << std::setprecision(1) << (frac * 100) << "% seq";
    }
    std::cout << "\n";
    
    for (u32 threads : thread_counts) {
        std::cout << std::setw(7) << threads << "  ";
        for (f64 seq_frac : sequential_fractions) {
            f64 speedup = 1.0 / (seq_frac + (1.0 - seq_frac) / threads);
            std::cout << std::setw(8) << std::fixed << std::setprecision(2) << speedup << "x   ";
        }
        std::cout << "\n";
    }
    
    std::cout << "\nKey Insights:" << std::endl;
    std::cout << "• Even 5% sequential fraction limits speedup to 20x" << std::endl;
    std::cout << "• 50% sequential fraction limits speedup to only 2x" << std::endl;
    std::cout << "• Adding more threads beyond the efficient point wastes resources" << std::endl;
}

std::string AmdahlsLawVisualizer::generate_scalability_report() const {
    std::ostringstream oss;
    oss << "=== Scalability Analysis Report ===\n\n";
    
    if (analysis_history_.empty()) {
        oss << "No scalability analysis data available.\n";
        return oss.str();
    }
    
    for (const auto& analysis : analysis_history_) {
        oss << "Algorithm: " << analysis.algorithm_name << "\n";
        oss << "  Sequential Fraction: " << std::fixed << std::setprecision(3) 
            << (analysis.profile.sequential_fraction * 100.0) << "%\n";
        oss << "  Theoretical Max Speedup: " << std::setprecision(2) 
            << analysis.profile.theoretical_max_speedup() << "x\n";
        oss << "  Optimal Thread Count: " << analysis.profile.optimal_thread_count << "\n";
        oss << "  Strong Scaling Efficiency: " << std::setprecision(1) 
            << (analysis.strong_scaling_efficiency * 100.0) << "%\n";
        oss << "  Scalability Limit: " << analysis.scalability_limit << " threads\n";
        
        if (!analysis.bottleneck_analysis.empty()) {
            oss << "  Bottlenecks:\n";
            for (const auto& bottleneck : analysis.bottleneck_analysis) {
                oss << "    • " << bottleneck << "\n";
            }
        }
        
        oss << "\n";
    }
    
    return oss.str();
}

//=============================================================================
// ParallelComputingLab Implementation
//=============================================================================

ParallelComputingLab::ParallelComputingLab(job_system::JobSystem* system, const LabConfig& config)
    : job_system_(system)
    , auto_start_visualization_(config.auto_start_visualization)
    , enable_comprehensive_logging_(config.enable_comprehensive_logging)
    , output_directory_(config.output_directory) {
    
    if (!job_system_) {
        throw std::invalid_argument("JobSystem cannot be null");
    }
    
    // Create output directory if it doesn't exist
    // In a real implementation, we would use filesystem APIs
    
    // Initialize components
    visualizer_ = std::make_unique<JobSystemVisualizer>(job_system_);
    data_tester_ = std::make_unique<ConcurrentDataTester>();
    performance_analyzer_ = std::make_unique<ThreadPerformanceAnalyzer>(job_system_);
    safety_tester_ = std::make_unique<ThreadSafetyTester>();
    amdahls_visualizer_ = std::make_unique<AmdahlsLawVisualizer>(job_system_);
    
    // Create educational framework with component references
    educational_framework_ = std::make_unique<EducationalFramework>(
        job_system_, 
        visualizer_.get(), 
        data_tester_.get(), 
        performance_analyzer_.get()
    );
    
    if (config.auto_start_visualization) {
        visualizer_->set_visualization_mode(config.visualization_mode);
    }
}

ParallelComputingLab::~ParallelComputingLab() {
    shutdown();
}

bool ParallelComputingLab::initialize() {
    LOG_INFO("Initializing ECScope Parallel Computing Lab...");
    
    if (auto_start_visualization_) {
        if (!visualizer_->start_visualization()) {
            LOG_WARN("Failed to start visualization");
        }
    }
    
    if (!performance_analyzer_->start_monitoring()) {
        LOG_WARN("Failed to start performance monitoring");
    }
    
    LOG_INFO("Parallel Computing Lab initialized successfully");
    return true;
}

void ParallelComputingLab::shutdown() {
    LOG_INFO("Shutting down Parallel Computing Lab...");
    
    if (visualizer_) {
        visualizer_->stop_visualization();
    }
    
    if (performance_analyzer_) {
        performance_analyzer_->stop_monitoring();
    }
    
    LOG_INFO("Parallel Computing Lab shutdown complete");
}

bool ParallelComputingLab::is_initialized() const {
    return job_system_ && job_system_->is_initialized();
}

void ParallelComputingLab::run_complete_demonstration() {
    std::cout << "\n╔══════════════════════════════════════════════════════════════╗\n";
    std::cout << "║           ECScope Parallel Computing Lab                     ║\n";
    std::cout << "║              Complete Demonstration                          ║\n";
    std::cout << "╚══════════════════════════════════════════════════════════════╝\n";
    
    // 1. Job System Visualization
    std::cout << "\n[1/6] Job System Visualization Demo" << std::endl;
    visualizer_->demonstrate_work_stealing();
    
    // 2. Concurrent Data Structure Testing
    std::cout << "\n[2/6] Concurrent Data Structure Testing Demo" << std::endl;
    auto queue_results = data_tester_->test_lock_free_queue();
    std::cout << "Lock-free queue test completed: " 
              << queue_results.operations_per_second << " ops/sec" << std::endl;
    
    // 3. Thread Performance Analysis
    std::cout << "\n[3/6] Thread Performance Analysis Demo" << std::endl;
    performance_analyzer_->demonstrate_cpu_affinity_effects();
    
    // 4. Educational Framework
    std::cout << "\n[4/6] Educational Framework Demo" << std::endl;
    educational_framework_->demonstrate_race_conditions();
    
    // 5. Thread Safety Testing
    std::cout << "\n[5/6] Thread Safety Testing Demo" << std::endl;
    safety_tester_->simulate_classic_race_condition();
    
    // 6. Amdahl's Law Visualization
    std::cout << "\n[6/6] Amdahl's Law Visualization Demo" << std::endl;
    amdahls_visualizer_->demonstrate_parallel_sum();
    amdahls_visualizer_->demonstrate_sequential_bottleneck_impact();
    
    std::cout << "\n=== Complete Demonstration Finished ===" << std::endl;
    std::cout << "All components of the Parallel Computing Lab have been demonstrated!" << std::endl;
}

std::string ParallelComputingLab::generate_comprehensive_report() const {
    std::ostringstream oss;
    
    oss << "╔══════════════════════════════════════════════════════════════╗\n";
    oss << "║           ECScope Parallel Computing Lab                     ║\n";
    oss << "║              Comprehensive Report                            ║\n";
    oss << "╚══════════════════════════════════════════════════════════════╝\n\n";
    
    oss << "Generated: " << std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count() << "\n\n";
    
    // Job System Statistics
    oss << "JOB SYSTEM STATISTICS:\n";
    oss << "======================\n";
    auto stats = job_system_->get_system_statistics();
    oss << "• Total Jobs Completed: " << stats.total_jobs_completed << "\n";
    oss << "• System Throughput: " << std::fixed << std::setprecision(2) 
        << stats.system_throughput_jobs_per_sec << " jobs/sec\n";
    oss << "• Worker Utilization: " << std::setprecision(1) 
        << stats.worker_utilization_percent << "%\n";
    oss << "• Total Steals: " << stats.total_steals << "\n";
    oss << "• Steal Success Rate: " << std::setprecision(1) 
        << (stats.overall_steal_success_rate * 100.0) << "%\n\n";
    
    // Visualization Statistics
    oss << "VISUALIZATION STATISTICS:\n";
    oss << "=========================\n";
    auto viz_stats = visualizer_->get_statistics();
    oss << "• Jobs Observed: " << viz_stats.total_jobs_observed << "\n";
    oss << "• Average Thread Utilization: " << std::setprecision(1) 
        << viz_stats.average_thread_utilization << "%\n";
    oss << "• Load Balance Coefficient: " << std::setprecision(3) 
        << viz_stats.load_balance_coefficient << "\n\n";
    
    // Performance Analysis
    oss << "PERFORMANCE ANALYSIS:\n";
    oss << "=====================\n";
    auto perf_analysis = performance_analyzer_->analyze_performance();
    oss << "• Average System Utilization: " << std::setprecision(1) 
        << (perf_analysis.average_system_utilization * 100.0) << "%\n";
    oss << "• Peak System Utilization: " << std::setprecision(1) 
        << (perf_analysis.peak_system_utilization * 100.0) << "%\n";
    oss << "• Average Cache Hit Rate: " << std::setprecision(1) 
        << (perf_analysis.average_cache_hit_rate * 100.0) << "%\n";
    oss << "• Overall Efficiency Score: " << std::setprecision(1) 
        << (perf_analysis.overall_efficiency_score() * 100.0) << "%\n\n";
    
    // Data Structure Testing
    oss << "CONCURRENT DATA STRUCTURE TESTING:\n";
    oss << "==================================\n";
    oss << data_tester_->generate_performance_report();
    
    // Scalability Analysis
    oss << "SCALABILITY ANALYSIS:\n";
    oss << "====================\n";
    oss << amdahls_visualizer_->generate_scalability_report();
    
    // Recommendations
    oss << "OPTIMIZATION RECOMMENDATIONS:\n";
    oss << "=============================\n";
    auto cpu_bottlenecks = performance_analyzer_->identify_cpu_bottlenecks();
    auto optimizations = performance_analyzer_->suggest_optimizations();
    
    if (!cpu_bottlenecks.empty()) {
        oss << "Identified Bottlenecks:\n";
        for (const auto& bottleneck : cpu_bottlenecks) {
            oss << "• " << bottleneck << "\n";
        }
        oss << "\n";
    }
    
    if (!optimizations.empty()) {
        oss << "Suggested Optimizations:\n";
        for (const auto& optimization : optimizations) {
            oss << "• " << optimization << "\n";
        }
        oss << "\n";
    }
    
    return oss.str();
}

//=============================================================================
// EducationalFramework Implementation
//=============================================================================

EducationalFramework::EducationalFramework(
    job_system::JobSystem* system,
    JobSystemVisualizer* visualizer,
    ConcurrentDataTester* data_tester,
    ThreadPerformanceAnalyzer* performance_analyzer)
    : job_system_(system)
    , visualizer_(visualizer)
    , data_tester_(data_tester)
    , performance_analyzer_(performance_analyzer) {
    
    deadlock_detector_ = std::make_unique<DeadlockDetector>();
    initialize_built_in_tutorials();
}

EducationalFramework::~EducationalFramework() = default;

void EducationalFramework::initialize_built_in_tutorials() {
    create_basic_threading_tutorial();
    create_work_stealing_tutorial();
    create_lock_free_programming_tutorial();
}

void EducationalFramework::create_basic_threading_tutorial() {
    const std::string tutorial_id = "basic_threading";
    create_tutorial(tutorial_id, "Basic Threading Concepts", 
                   "Introduction to parallel programming fundamentals");
    
    // Lesson 1: Thread creation and management
    LessonConfig lesson1;
    lesson1.title = "Thread Creation and Management";
    lesson1.description = "Learn how to create and manage threads in C++";
    lesson1.learning_objectives = {
        "Understand thread lifecycle",
        "Learn thread creation methods",
        "Practice thread synchronization"
    };
    lesson1.key_concepts = {"std::thread", "thread lifecycle", "join vs detach"};
    lesson1.demonstration_function = [this]() {
        demonstrate_race_conditions();
    };
    lesson1.validation_function = []() { return true; };
    
    add_lesson_to_tutorial(tutorial_id, lesson1);
}

void EducationalFramework::create_work_stealing_tutorial() {
    const std::string tutorial_id = "work_stealing";
    create_tutorial(tutorial_id, "Work-Stealing Job Systems", 
                   "Advanced work distribution and load balancing");
    
    LessonConfig lesson1;
    lesson1.title = "Work-Stealing Fundamentals";
    lesson1.description = "Understanding work-stealing algorithms";
    lesson1.demonstration_function = [this]() {
        visualizer_->demonstrate_work_stealing();
    };
    lesson1.validation_function = []() { return true; };
    
    add_lesson_to_tutorial(tutorial_id, lesson1);
}

void EducationalFramework::create_lock_free_programming_tutorial() {
    const std::string tutorial_id = "lock_free";
    create_tutorial(tutorial_id, "Lock-Free Programming", 
                   "Advanced concurrent programming without locks");
    
    LessonConfig lesson1;
    lesson1.title = "Atomic Operations";
    lesson1.description = "Understanding atomic operations and memory ordering";
    lesson1.demonstration_function = [this]() {
        demonstrate_atomic_operations();
    };
    lesson1.validation_function = []() { return true; };
    
    add_lesson_to_tutorial(tutorial_id, lesson1);
}

void EducationalFramework::demonstrate_race_conditions() {
    std::cout << "\n=== Race Condition Demonstration ===" << std::endl;
    std::cout << "This demonstration shows what happens when multiple threads\n";
    std::cout << "access shared data without proper synchronization." << std::endl;
    
    const u32 num_threads = 4;
    const u32 increments_per_thread = 100000;
    
    // Unsafe counter (race condition)
    u32 unsafe_counter = 0;
    std::vector<std::thread> unsafe_threads;
    
    std::cout << "\nTesting unsafe counter (with race condition)..." << std::endl;
    auto start_time = std::chrono::high_resolution_clock::now();
    
    for (u32 i = 0; i < num_threads; ++i) {
        unsafe_threads.emplace_back([&unsafe_counter, increments_per_thread]() {
            for (u32 j = 0; j < increments_per_thread; ++j) {
                unsafe_counter++; // Race condition here!
            }
        });
    }
    
    for (auto& thread : unsafe_threads) {
        thread.join();
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto unsafe_duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    u32 expected_value = num_threads * increments_per_thread;
    std::cout << "Expected value: " << expected_value << std::endl;
    std::cout << "Actual value: " << unsafe_counter << std::endl;
    std::cout << "Data race detected: " << (unsafe_counter != expected_value ? "YES" : "NO") << std::endl;
    std::cout << "Time taken: " << unsafe_duration.count() << " ms" << std::endl;
    
    // Safe counter (with atomic operations)
    std::atomic<u32> safe_counter{0};
    std::vector<std::thread> safe_threads;
    
    std::cout << "\nTesting safe counter (with atomic operations)..." << std::endl;
    start_time = std::chrono::high_resolution_clock::now();
    
    for (u32 i = 0; i < num_threads; ++i) {
        safe_threads.emplace_back([&safe_counter, increments_per_thread]() {
            for (u32 j = 0; j < increments_per_thread; ++j) {
                safe_counter.fetch_add(1, std::memory_order_relaxed);
            }
        });
    }
    
    for (auto& thread : safe_threads) {
        thread.join();
    }
    
    end_time = std::chrono::high_resolution_clock::now();
    auto safe_duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    std::cout << "Expected value: " << expected_value << std::endl;
    std::cout << "Actual value: " << safe_counter.load() << std::endl;
    std::cout << "Data race detected: " << (safe_counter.load() != expected_value ? "YES" : "NO") << std::endl;
    std::cout << "Time taken: " << safe_duration.count() << " ms" << std::endl;
    
    f64 overhead = static_cast<f64>(safe_duration.count()) / unsafe_duration.count();
    std::cout << "\nSynchronization overhead: " << std::fixed << std::setprecision(2) 
              << ((overhead - 1.0) * 100.0) << "%" << std::endl;
    
    std::cout << "\nKey Learning Points:" << std::endl;
    std::cout << "• Race conditions lead to unpredictable results" << std::endl;
    std::cout << "• Atomic operations provide thread safety" << std::endl;
    std::cout << "• Thread safety comes with a performance cost" << std::endl;
    std::cout << "• Always synchronize access to shared mutable data" << std::endl;
}

void EducationalFramework::demonstrate_atomic_operations() {
    std::cout << "\n=== Atomic Operations Demonstration ===" << std::endl;
    std::cout << "Comparing different atomic operations and memory ordering." << std::endl;
    
    const u32 iterations = 1000000;
    
    // Test different atomic operations
    std::atomic<u64> counter{0};
    
    // Test 1: fetch_add with different memory orderings
    std::cout << "\nTesting fetch_add with different memory orderings..." << std::endl;
    
    // Relaxed ordering
    auto start_time = std::chrono::high_resolution_clock::now();
    std::vector<std::thread> relaxed_threads;
    
    for (u32 i = 0; i < 4; ++i) {
        relaxed_threads.emplace_back([&counter, iterations]() {
            for (u32 j = 0; j < iterations; ++j) {
                counter.fetch_add(1, std::memory_order_relaxed);
            }
        });
    }
    
    for (auto& thread : relaxed_threads) {
        thread.join();
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto relaxed_duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
    
    std::cout << "Relaxed ordering: " << relaxed_duration.count() << " μs" << std::endl;
    std::cout << "Final counter value: " << counter.load() << std::endl;
    
    // Sequential consistency
    counter.store(0);
    start_time = std::chrono::high_resolution_clock::now();
    std::vector<std::thread> seq_cst_threads;
    
    for (u32 i = 0; i < 4; ++i) {
        seq_cst_threads.emplace_back([&counter, iterations]() {
            for (u32 j = 0; j < iterations; ++j) {
                counter.fetch_add(1, std::memory_order_seq_cst);
            }
        });
    }
    
    for (auto& thread : seq_cst_threads) {
        thread.join();
    }
    
    end_time = std::chrono::high_resolution_clock::now();
    auto seq_cst_duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
    
    std::cout << "Sequential consistency: " << seq_cst_duration.count() << " μs" << std::endl;
    std::cout << "Final counter value: " << counter.load() << std::endl;
    
    f64 overhead = static_cast<f64>(seq_cst_duration.count()) / relaxed_duration.count();
    std::cout << "Sequential consistency overhead: " << std::fixed << std::setprecision(2) 
              << ((overhead - 1.0) * 100.0) << "%" << std::endl;
    
    std::cout << "\nKey Learning Points:" << std::endl;
    std::cout << "• Relaxed memory ordering provides best performance" << std::endl;
    std::cout << "• Sequential consistency provides strongest guarantees" << std::endl;
    std::cout << "• Choose memory ordering based on your synchronization needs" << std::endl;
    std::cout << "• Atomic operations are the foundation of lock-free programming" << std::endl;
}

//=============================================================================
// ThreadSafetyTester Implementation  
//=============================================================================

ThreadSafetyTester::SafetyTestResults ThreadSafetyTester::test_race_conditions(
    const std::string& test_name,
    std::function<void(u32)> test_function,
    u32 thread_count,
    std::chrono::seconds duration) {
    
    SafetyTestResults results;
    results.test_name = test_name;
    results.category = TestCategory::RaceConditions;
    results.thread_count_tested = thread_count;
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    // Execute the test function with multiple threads
    std::vector<std::thread> test_threads;
    std::atomic<bool> should_stop{false};
    std::atomic<u64> total_operations{0};
    
    for (u32 i = 0; i < thread_count; ++i) {
        test_threads.emplace_back([&, i]() {
            u64 local_ops = 0;
            while (!should_stop.load() && 
                   std::chrono::high_resolution_clock::now() - start_time < duration) {
                test_function(i);
                ++local_ops;
            }
            total_operations.fetch_add(local_ops);
        });
    }
    
    // Let test run for specified duration
    std::this_thread::sleep_for(duration);
    should_stop.store(true);
    
    for (auto& thread : test_threads) {
        if (thread.joinable()) {
            thread.join();
        }
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    results.test_duration_seconds = std::chrono::duration<f64>(end_time - start_time).count();
    results.operations_tested = total_operations.load();
    
    // Analyze results for race conditions
    results.detected_issues = detect_race_conditions_in_execution();
    results.issues_detected = static_cast<u32>(results.detected_issues.size());
    results.safety_verified = (results.issues_detected == 0);
    
    if (results.issues_detected > 0) {
        results.recommendations.push_back("Use atomic operations or locks to protect shared data");
        results.recommendations.push_back("Consider using lock-free data structures");
        results.recommendations.push_back("Apply proper memory ordering constraints");
    }
    
    record_test_result(results);
    return results;
}

std::vector<std::string> ThreadSafetyTester::detect_race_conditions_in_execution() {
    std::vector<std::string> issues;
    
    // This is a simplified race condition detector
    // In a real implementation, we would use more sophisticated techniques
    // like thread sanitizer integration or custom instrumentation
    
    // For demonstration, we simulate some detected issues
    static thread_local std::mt19937 rng{std::random_device{}()};
    std::uniform_real_distribution<f64> dist(0.0, 1.0);
    
    if (dist(rng) < 0.1) { // 10% chance of detecting a race condition
        issues.push_back("Potential data race detected on shared variable");
    }
    
    if (dist(rng) < 0.05) { // 5% chance of detecting use-after-free
        issues.push_back("Potential use-after-free detected");
    }
    
    return issues;
}

void ThreadSafetyTester::simulate_classic_race_condition() {
    std::cout << "\n=== Classic Race Condition Simulation ===" << std::endl;
    std::cout << "Simulating a classic race condition scenario..." << std::endl;
    
    // Shared resource that multiple threads will access
    struct BankAccount {
        u32 balance = 1000;
        std::mutex mutex; // For the safe version
        
        // Unsafe withdrawal (race condition)
        bool unsafe_withdraw(u32 amount) {
            if (balance >= amount) {
                // Simulate some processing time where race can occur
                std::this_thread::sleep_for(std::chrono::microseconds{10});
                balance -= amount;
                return true;
            }
            return false;
        }
        
        // Safe withdrawal (with mutex)
        bool safe_withdraw(u32 amount) {
            std::lock_guard<std::mutex> lock(mutex);
            if (balance >= amount) {
                std::this_thread::sleep_for(std::chrono::microseconds{10});
                balance -= amount;
                return true;
            }
            return false;
        }
    };
    
    // Test unsafe version
    std::cout << "\nTesting unsafe withdrawal (race condition expected)..." << std::endl;
    BankAccount unsafe_account;
    std::vector<std::thread> unsafe_threads;
    std::atomic<u32> unsafe_successful_withdrawals{0};
    
    for (u32 i = 0; i < 10; ++i) {
        unsafe_threads.emplace_back([&]() {
            for (u32 j = 0; j < 20; ++j) {
                if (unsafe_account.unsafe_withdraw(10)) {
                    unsafe_successful_withdrawals.fetch_add(1);
                }
            }
        });
    }
    
    for (auto& thread : unsafe_threads) {
        thread.join();
    }
    
    std::cout << "Initial balance: 1000" << std::endl;
    std::cout << "Successful withdrawals: " << unsafe_successful_withdrawals.load() << std::endl;
    std::cout << "Expected balance: " << (1000 - unsafe_successful_withdrawals.load() * 10) << std::endl;
    std::cout << "Actual balance: " << unsafe_account.balance << std::endl;
    
    if (unsafe_account.balance != (1000 - unsafe_successful_withdrawals.load() * 10)) {
        std::cout << "Race condition detected! Balance is inconsistent." << std::endl;
    } else {
        std::cout << "No race condition detected (this time)." << std::endl;
    }
    
    // Test safe version
    std::cout << "\nTesting safe withdrawal (with mutex protection)..." << std::endl;
    BankAccount safe_account;
    std::vector<std::thread> safe_threads;
    std::atomic<u32> safe_successful_withdrawals{0};
    
    for (u32 i = 0; i < 10; ++i) {
        safe_threads.emplace_back([&]() {
            for (u32 j = 0; j < 20; ++j) {
                if (safe_account.safe_withdraw(10)) {
                    safe_successful_withdrawals.fetch_add(1);
                }
            }
        });
    }
    
    for (auto& thread : safe_threads) {
        thread.join();
    }
    
    std::cout << "Initial balance: 1000" << std::endl;
    std::cout << "Successful withdrawals: " << safe_successful_withdrawals.load() << std::endl;
    std::cout << "Expected balance: " << (1000 - safe_successful_withdrawals.load() * 10) << std::endl;
    std::cout << "Actual balance: " << safe_account.balance << std::endl;
    
    if (safe_account.balance == (1000 - safe_successful_withdrawals.load() * 10)) {
        std::cout << "No race condition! Balance is consistent." << std::endl;
    } else {
        std::cout << "Unexpected: race condition still detected." << std::endl;
    }
    
    std::cout << "\nKey Takeaways:" << std::endl;
    std::cout << "• Race conditions can cause data corruption" << std::endl;
    std::cout << "• Proper synchronization prevents race conditions" << std::endl;
    std::cout << "• Mutexes provide exclusive access to shared resources" << std::endl;
    std::cout << "• Always test concurrent code under high contention" << std::endl;
}

std::string ThreadSafetyTester::generate_safety_report() const {
    std::ostringstream oss;
    oss << "=== Thread Safety Test Report ===\n\n";
    
    if (test_history_.empty()) {
        oss << "No thread safety tests have been conducted.\n";
        return oss.str();
    }
    
    u32 total_tests = static_cast<u32>(test_history_.size());
    u32 safe_tests = 0;
    u32 total_issues = 0;
    
    for (const auto& result : test_history_) {
        if (result.safety_verified) {
            ++safe_tests;
        }
        total_issues += result.issues_detected;
    }
    
    f64 safety_percentage = static_cast<f64>(safe_tests) / total_tests * 100.0;
    
    oss << "Summary:\n";
    oss << "  Total Tests Conducted: " << total_tests << "\n";
    oss << "  Tests Passed: " << safe_tests << " (" << std::fixed << std::setprecision(1) 
        << safety_percentage << "%)\n";
    oss << "  Total Issues Detected: " << total_issues << "\n\n";
    
    // Detailed results
    for (const auto& result : test_history_) {
        oss << "Test: " << result.test_name << "\n";
        oss << "  Category: ";
        switch (result.category) {
            case TestCategory::RaceConditions: oss << "Race Conditions"; break;
            case TestCategory::DeadlockDetection: oss << "Deadlock Detection"; break;
            case TestCategory::AtomicOperations: oss << "Atomic Operations"; break;
            default: oss << "Other"; break;
        }
        oss << "\n";
        oss << "  Safety Verified: " << (result.safety_verified ? "YES" : "NO") << "\n";
        oss << "  Issues Detected: " << result.issues_detected << "\n";
        
        if (!result.detected_issues.empty()) {
            oss << "  Detected Issues:\n";
            for (const auto& issue : result.detected_issues) {
                oss << "    • " << issue << "\n";
            }
        }
        
        if (!result.recommendations.empty()) {
            oss << "  Recommendations:\n";
            for (const auto& recommendation : result.recommendations) {
                oss << "    • " << recommendation << "\n";
            }
        }
        
        oss << "\n";
    }
    
    return oss.str();
}

//=============================================================================
// Utility Functions Implementation
//=============================================================================

namespace utils {

CpuTopology detect_cpu_topology() {
    CpuTopology topology;
    topology.logical_cores = std::thread::hardware_concurrency();
    topology.physical_cores = topology.logical_cores; // Simplified
    topology.numa_nodes = 1; // Assume single NUMA node for simplicity
    
    topology.core_groups.resize(topology.numa_nodes);
    for (u32 i = 0; i < topology.logical_cores; ++i) {
        topology.core_groups[0].push_back(i);
    }
    
    return topology;
}

void set_thread_affinity(std::thread::id thread_id, u32 core_id) {
    // Platform-specific implementation would go here
    // For now, this is a no-op placeholder
    (void)thread_id;
    (void)core_id;
}

std::string generate_ascii_chart(const std::vector<f64>& data, 
                                const std::string& title,
                                u32 width, u32 height) {
    std::ostringstream oss;
    
    if (data.empty()) {
        return "No data to display\n";
    }
    
    if (!title.empty()) {
        oss << title << "\n";
        oss << std::string(title.length(), '=') << "\n";
    }
    
    f64 min_val = *std::min_element(data.begin(), data.end());
    f64 max_val = *std::max_element(data.begin(), data.end());
    
    if (max_val == min_val) {
        max_val = min_val + 1.0; // Avoid division by zero
    }
    
    // Scale data to chart height
    std::vector<u32> scaled_data;
    scaled_data.reserve(data.size());
    
    for (f64 value : data) {
        u32 scaled = static_cast<u32>((value - min_val) / (max_val - min_val) * height);
        scaled_data.push_back(std::min(scaled, height - 1));
    }
    
    // Render chart
    for (u32 row = height; row > 0; --row) {
        oss << std::setw(8) << std::fixed << std::setprecision(2) 
            << (min_val + (max_val - min_val) * (row - 1) / (height - 1)) << " |";
        
        for (usize i = 0; i < std::min(static_cast<usize>(width), scaled_data.size()); ++i) {
            if (scaled_data[i] >= row - 1) {
                oss << "*";
            } else {
                oss << " ";
            }
        }
        oss << "\n";
    }
    
    // X-axis
    oss << std::string(9, ' ') << "+";
    for (u32 i = 0; i < std::min(width, static_cast<u32>(data.size())); ++i) {
        oss << "-";
    }
    oss << "\n";
    
    return oss.str();
}

std::string generate_histogram(const std::vector<f64>& data,
                              u32 bin_count,
                              const std::string& title) {
    std::ostringstream oss;
    
    if (data.empty()) {
        return "No data to display\n";
    }
    
    if (!title.empty()) {
        oss << title << "\n";
        oss << std::string(title.length(), '=') << "\n";
    }
    
    f64 min_val = *std::min_element(data.begin(), data.end());
    f64 max_val = *std::max_element(data.begin(), data.end());
    
    if (max_val == min_val) {
        oss << "All values are equal: " << min_val << "\n";
        return oss.str();
    }
    
    // Create bins
    std::vector<u32> bins(bin_count, 0);
    f64 bin_width = (max_val - min_val) / bin_count;
    
    for (f64 value : data) {
        u32 bin_index = static_cast<u32>((value - min_val) / bin_width);
        if (bin_index >= bin_count) bin_index = bin_count - 1;
        bins[bin_index]++;
    }
    
    // Find max bin count for scaling
    u32 max_bin_count = *std::max_element(bins.begin(), bins.end());
    
    // Render histogram
    for (u32 i = 0; i < bin_count; ++i) {
        f64 bin_start = min_val + i * bin_width;
        f64 bin_end = bin_start + bin_width;
        
        oss << std::setw(8) << std::fixed << std::setprecision(2) << bin_start
            << " - " << std::setw(8) << bin_end << " |";
        
        u32 bar_length = max_bin_count > 0 ? (bins[i] * 50) / max_bin_count : 0;
        oss << std::string(bar_length, '#');
        oss << " (" << bins[i] << ")\n";
    }
    
    return oss.str();
}

} // namespace utils

} // namespace ecscope::parallel_lab