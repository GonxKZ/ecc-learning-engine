#pragma once

/**
 * @file parallel_computing_lab.hpp
 * @brief Comprehensive Parallel Computing Laboratory for ECScope
 * 
 * This advanced educational framework provides hands-on learning tools for
 * parallel programming concepts, real-time visualization of concurrent
 * execution, and comprehensive analysis of parallel algorithms.
 * 
 * Key Components:
 * 1. Job System Visualizer - Real-time thread and job queue visualization
 * 2. Concurrent Data Structure Tester - Lock-free structures and race detection
 * 3. Thread Performance Analyzer - CPU utilization and cache coherency analysis
 * 4. Educational Framework - Tutorials, deadlock detection, algorithm comparison
 * 5. Thread Safety Testing - Race condition simulation and detection
 * 6. Amdahl's Law Visualization - Performance scaling analysis
 * 
 * Educational Value:
 * - Visual understanding of parallel execution patterns
 * - Hands-on experience with lock-free programming
 * - Performance analysis and optimization techniques
 * - Understanding of threading pitfalls and solutions
 * - Quantitative analysis of parallel algorithm efficiency
 * 
 * Integration Features:
 * - Seamless integration with ECScope job system
 * - Real-time performance monitoring
 * - Interactive tutorials with visual feedback
 * - Comprehensive testing framework
 * - Export capabilities for educational reports
 * 
 * @author ECScope Parallel Computing Laboratory
 * @date 2025
 */

#include "work_stealing_job_system.hpp"
#include "job_profiler.hpp"
#include "core/types.hpp"
#include "core/log.hpp"
#include "memory/lockfree_structures.hpp"
#include "memory/cache_aware_structures.hpp"
#include <atomic>
#include <thread>
#include <vector>
#include <queue>
#include <chrono>
#include <memory>
#include <functional>
#include <string>
#include <unordered_map>
#include <mutex>
#include <condition_variable>
#include <fstream>
#include <sstream>
#include <random>
#include <algorithm>
#include <numeric>
#include <future>

namespace ecscope::parallel_lab {

//=============================================================================
// Forward Declarations
//=============================================================================

class JobSystemVisualizer;
class ConcurrentDataTester;
class ThreadPerformanceAnalyzer;
class EducationalFramework;
class ThreadSafetyTester;
class AmdahlsLawVisualizer;
class ParallelComputingLab;

//=============================================================================
// Core Types and Enums
//=============================================================================

/**
 * @brief Visualization update frequency modes
 */
enum class VisualizationMode : u8 {
    Disabled = 0,        // No visualization
    LowFrequency = 1,    // Update every 100ms
    MediumFrequency = 2, // Update every 50ms
    HighFrequency = 3,   // Update every 10ms
    RealTime = 4         // Update continuously
};

/**
 * @brief Thread execution states for visualization
 */
enum class ThreadState : u8 {
    Idle = 0,           // Thread is waiting for work
    Executing = 1,      // Thread is executing a job
    Stealing = 2,       // Thread is attempting to steal work
    Synchronizing = 3,  // Thread is waiting for synchronization
    Blocked = 4,        // Thread is blocked on I/O or lock
    Terminated = 5      // Thread has been terminated
};

/**
 * @brief Performance metrics for analysis
 */
struct PerformanceMetrics {
    std::chrono::high_resolution_clock::time_point timestamp;
    
    // CPU metrics
    f64 cpu_utilization_percent = 0.0;
    f64 system_load_average = 0.0;
    u64 context_switches = 0;
    u64 cache_misses = 0;
    u64 cache_hits = 0;
    
    // Memory metrics
    usize memory_usage_bytes = 0;
    usize peak_memory_bytes = 0;
    u64 memory_allocations = 0;
    u64 memory_deallocations = 0;
    
    // Threading metrics
    u32 active_threads = 0;
    u32 idle_threads = 0;
    u64 jobs_completed = 0;
    u64 jobs_pending = 0;
    u64 steal_operations = 0;
    u64 contention_events = 0;
    
    // Performance ratios
    f64 cache_hit_ratio() const noexcept {
        u64 total = cache_hits + cache_misses;
        return total > 0 ? static_cast<f64>(cache_hits) / total : 0.0;
    }
    
    f64 thread_utilization() const noexcept {
        u32 total = active_threads + idle_threads;
        return total > 0 ? static_cast<f64>(active_threads) / total : 0.0;
    }
    
    f64 steal_success_ratio() const noexcept {
        return steal_operations > 0 ? 1.0 - (static_cast<f64>(contention_events) / steal_operations) : 0.0;
    }
};

/**
 * @brief Educational lesson configuration
 */
struct LessonConfig {
    std::string title;
    std::string description;
    std::vector<std::string> learning_objectives;
    std::vector<std::string> key_concepts;
    std::function<void()> demonstration_function;
    std::function<bool()> validation_function;
    u32 estimated_duration_minutes = 5;
    u32 difficulty_level = 1; // 1-5 scale
};

/**
 * @brief Race condition test case
 */
struct RaceConditionTest {
    std::string name;
    std::string description;
    std::function<void(u32)> test_function; // Parameter is thread count
    std::function<bool()> correctness_check;
    std::function<void()> setup_function;
    std::function<void()> cleanup_function;
    u32 recommended_thread_count = 4;
    u32 test_iterations = 1000;
    bool expect_race_condition = true;
};

//=============================================================================
// Real-Time Job System Visualizer
//=============================================================================

/**
 * @brief Real-time visualization of job system execution
 * 
 * Provides live monitoring and visualization of:
 * - Thread states and job execution
 * - Work-stealing operations
 * - Job queue states across threads
 * - Performance metrics and bottlenecks
 */
class JobSystemVisualizer {
private:
    struct ThreadVisualizationData {
        u32 thread_id;
        u32 cpu_core;
        ThreadState current_state;
        std::string current_job_name;
        std::chrono::high_resolution_clock::time_point state_change_time;
        std::deque<std::pair<ThreadState, std::chrono::high_resolution_clock::time_point>> state_history;
        
        // Queue information
        usize local_queue_size = 0;
        u64 total_jobs_executed = 0;
        u64 total_steals_performed = 0;
        u64 total_steals_received = 0;
        
        // Performance data
        f64 utilization_percent = 0.0;
        u64 idle_time_us = 0;
        u64 execution_time_us = 0;
    };
    
    struct JobVisualizationData {
        job_system::JobID job_id;
        std::string job_name;
        job_system::JobPriority priority;
        job_system::JobState state;
        u32 assigned_thread;
        std::chrono::high_resolution_clock::time_point creation_time;
        std::chrono::high_resolution_clock::time_point execution_start_time;
        std::chrono::high_resolution_clock::time_point completion_time;
        bool was_stolen = false;
    };
    
    job_system::JobSystem* job_system_;
    std::vector<ThreadVisualizationData> thread_data_;
    std::deque<JobVisualizationData> job_history_;
    
    // Visualization state
    VisualizationMode mode_;
    std::atomic<bool> is_running_{false};
    std::thread visualization_thread_;
    std::mutex data_mutex_;
    
    // Update timing
    std::chrono::microseconds update_interval_;
    std::chrono::high_resolution_clock::time_point last_update_;
    
    // Output configuration
    bool output_to_console_;
    bool output_to_file_;
    std::string output_filename_;
    std::ofstream output_file_;
    
public:
    explicit JobSystemVisualizer(job_system::JobSystem* system);
    ~JobSystemVisualizer();
    
    // Configuration
    void set_visualization_mode(VisualizationMode mode);
    void set_update_interval(std::chrono::microseconds interval);
    void set_console_output(bool enabled) { output_to_console_ = enabled; }
    void set_file_output(bool enabled, const std::string& filename = "");
    void set_job_history_size(usize max_size);
    
    // Lifecycle
    bool start_visualization();
    void stop_visualization();
    bool is_running() const noexcept { return is_running_.load(std::memory_order_acquire); }
    
    // Data access
    std::vector<ThreadVisualizationData> get_thread_data() const;
    std::vector<JobVisualizationData> get_recent_jobs(u32 count) const;
    std::string generate_text_visualization() const;
    std::string generate_json_data() const;
    
    // Educational features
    void demonstrate_work_stealing();
    void demonstrate_load_balancing();
    void demonstrate_priority_scheduling();
    void demonstrate_cache_effects();
    
    // Statistics
    struct VisualizationStats {
        u32 total_jobs_observed = 0;
        u32 total_steals_observed = 0;
        f64 average_thread_utilization = 0.0;
        f64 load_balance_coefficient = 0.0;
        f64 steal_success_rate = 0.0;
        std::chrono::milliseconds total_observation_time{0};
    };
    
    VisualizationStats get_statistics() const;
    void reset_statistics();
    
private:
    void visualization_main();
    void update_thread_data();
    void update_job_data();
    void output_visualization();
    void render_console_visualization() const;
    void write_file_output() const;
    std::string format_thread_state(const ThreadVisualizationData& data) const;
    std::string create_timeline_visualization() const;
};

//=============================================================================
// Concurrent Data Structure Tester
//=============================================================================

/**
 * @brief Comprehensive testing framework for concurrent data structures
 * 
 * Tests lock-free data structures under high contention scenarios
 * and provides educational insights into concurrent programming challenges.
 */
class ConcurrentDataTester {
public:
    /**
     * @brief Test configuration for concurrent data structure testing
     */
    struct TestConfig {
        u32 thread_count = 4;
        u32 operations_per_thread = 10000;
        u32 test_duration_seconds = 10;
        bool enable_contention_analysis = true;
        bool enable_correctness_checking = true;
        bool enable_performance_monitoring = true;
        f64 read_write_ratio = 0.7; // 70% reads, 30% writes
    };
    
    /**
     * @brief Results from concurrent data structure testing
     */
    struct TestResults {
        std::string structure_name;
        TestConfig config;
        
        // Performance metrics
        f64 total_time_seconds = 0.0;
        u64 total_operations = 0;
        f64 operations_per_second = 0.0;
        f64 average_latency_ns = 0.0;
        
        // Correctness metrics
        bool correctness_verified = false;
        u32 detected_inconsistencies = 0;
        u32 lost_updates = 0;
        u32 spurious_failures = 0;
        
        // Contention metrics
        u64 total_contentions = 0;
        f64 contention_rate = 0.0;
        f64 average_backoff_time_ns = 0.0;
        
        // Thread-specific data
        std::vector<u64> per_thread_operations;
        std::vector<f64> per_thread_success_rates;
        std::vector<u64> per_thread_contentions;
        
        f64 load_balance_score() const {
            if (per_thread_operations.empty()) return 0.0;
            
            f64 mean = static_cast<f64>(total_operations) / per_thread_operations.size();
            f64 variance = 0.0;
            
            for (u64 ops : per_thread_operations) {
                f64 diff = static_cast<f64>(ops) - mean;
                variance += diff * diff;
            }
            
            variance /= per_thread_operations.size();
            f64 std_dev = std::sqrt(variance);
            
            return mean > 0.0 ? std::max(0.0, 1.0 - (std_dev / mean)) : 0.0;
        }
    };
    
private:
    std::vector<TestResults> test_history_;
    std::mutex results_mutex_;
    
public:
    ConcurrentDataTester() = default;
    ~ConcurrentDataTester() = default;
    
    // Core testing interface
    template<typename DataStructure>
    TestResults test_lock_free_structure(const std::string& name, 
                                        DataStructure& structure,
                                        const TestConfig& config = TestConfig{});
    
    // Specific structure tests
    TestResults test_lock_free_queue(const TestConfig& config = TestConfig{});
    TestResults test_lock_free_stack(const TestConfig& config = TestConfig{});
    TestResults test_lock_free_hash_map(const TestConfig& config = TestConfig{});
    TestResults test_atomic_counter(const TestConfig& config = TestConfig{});
    TestResults test_rw_lock_structure(const TestConfig& config = TestConfig{});
    
    // Educational demonstrations
    void demonstrate_aba_problem();
    void demonstrate_memory_ordering();
    void demonstrate_false_sharing();
    void demonstrate_lock_contention();
    void demonstrate_wait_free_vs_lock_free();
    
    // Race condition detection
    struct RaceDetectionResult {
        bool race_detected = false;
        std::string race_type;
        std::vector<std::string> affected_operations;
        u32 detection_confidence_percent = 0;
        std::string detailed_analysis;
    };
    
    RaceDetectionResult detect_race_conditions(const TestConfig& config);
    
    // Comparison and analysis
    void compare_structures(const std::vector<std::string>& structure_names,
                          const TestConfig& config = TestConfig{});
    std::string generate_performance_report() const;
    std::string generate_educational_summary() const;
    
    // Statistics and history
    const std::vector<TestResults>& get_test_history() const { return test_history_; }
    void clear_test_history() { 
        std::lock_guard<std::mutex> lock(results_mutex_);
        test_history_.clear(); 
    }
    
private:
    void record_test_result(const TestResults& result) {
        std::lock_guard<std::mutex> lock(results_mutex_);
        test_history_.push_back(result);
    }
    
    template<typename DataStructure, typename Operation>
    void run_threaded_test(DataStructure& structure, Operation operation,
                          const TestConfig& config, TestResults& results);
    
    void analyze_contention_patterns(const TestResults& results) const;
    bool verify_structural_correctness(const TestResults& results) const;
};

//=============================================================================
// Thread Performance Analyzer  
//=============================================================================

/**
 * @brief Advanced thread performance analysis and visualization
 * 
 * Provides detailed analysis of thread performance including:
 * - CPU utilization per thread and core
 * - Cache coherency analysis
 * - NUMA topology effects
 * - Contention bottleneck identification
 */
class ThreadPerformanceAnalyzer {
public:
    /**
     * @brief Performance analysis configuration
     */
    struct AnalysisConfig {
        std::chrono::milliseconds sampling_interval{10};
        bool monitor_cpu_utilization = true;
        bool monitor_cache_performance = true;
        bool monitor_memory_access = true;
        bool monitor_numa_effects = true;
        bool enable_thread_migration_tracking = true;
        u32 history_size = 10000; // Number of samples to keep
    };
    
    /**
     * @brief Comprehensive performance sample
     */
    struct PerformanceSample {
        std::chrono::high_resolution_clock::time_point timestamp;
        u32 thread_id;
        u32 cpu_core;
        u32 numa_node;
        
        // CPU metrics
        f64 cpu_utilization_percent = 0.0;
        u64 instructions_executed = 0;
        u64 cycles_executed = 0;
        f64 ipc_ratio = 0.0; // Instructions per cycle
        
        // Cache metrics
        u64 l1_cache_hits = 0;
        u64 l1_cache_misses = 0;
        u64 l2_cache_hits = 0;
        u64 l2_cache_misses = 0;
        u64 l3_cache_hits = 0;
        u64 l3_cache_misses = 0;
        u64 memory_accesses = 0;
        
        // Memory metrics
        usize private_memory_bytes = 0;
        usize shared_memory_bytes = 0;
        u64 page_faults = 0;
        u64 tlb_misses = 0;
        
        // Thread interaction metrics
        u64 context_switches = 0;
        u64 migrations = 0;
        u64 synchronization_events = 0;
        std::chrono::nanoseconds blocked_time{0};
        
        // Derived metrics
        f64 overall_cache_hit_rate() const {
            u64 total_hits = l1_cache_hits + l2_cache_hits + l3_cache_hits;
            u64 total_misses = l1_cache_misses + l2_cache_misses + l3_cache_misses;
            u64 total_accesses = total_hits + total_misses;
            return total_accesses > 0 ? static_cast<f64>(total_hits) / total_accesses : 0.0;
        }
        
        f64 memory_bandwidth_mb_per_sec() const {
            // Simplified calculation based on cache misses
            constexpr u64 cache_line_size = 64;
            u64 memory_bytes = l3_cache_misses * cache_line_size;
            return static_cast<f64>(memory_bytes) / (1024.0 * 1024.0);
        }
    };
    
    /**
     * @brief Analysis results and insights
     */
    struct AnalysisResults {
        std::chrono::high_resolution_clock::time_point analysis_start;
        std::chrono::high_resolution_clock::time_point analysis_end;
        
        // Overall system metrics
        f64 average_system_utilization = 0.0;
        f64 peak_system_utilization = 0.0;
        f64 average_cache_hit_rate = 0.0;
        f64 total_memory_bandwidth_gb_per_sec = 0.0;
        
        // Per-thread analysis
        std::vector<f64> per_thread_utilization;
        std::vector<f64> per_thread_cache_hit_rates;
        std::vector<u32> thread_migration_counts;
        std::vector<std::chrono::nanoseconds> per_thread_blocked_time;
        
        // Bottleneck identification
        std::vector<std::string> identified_bottlenecks;
        std::vector<std::string> optimization_suggestions;
        
        // NUMA analysis
        struct NumaAnalysis {
            std::vector<f64> per_node_utilization;
            std::vector<f64> cross_node_memory_access_rates;
            std::vector<std::string> numa_optimization_suggestions;
        } numa_analysis;
        
        // Cache coherency analysis
        struct CacheCoherencyAnalysis {
            f64 false_sharing_probability = 0.0;
            std::vector<std::string> potential_false_sharing_locations;
            f64 cache_line_contention_rate = 0.0;
            std::vector<std::string> coherency_optimization_suggestions;
        } cache_analysis;
        
        f64 overall_efficiency_score() const {
            return (average_system_utilization + average_cache_hit_rate) / 2.0;
        }
    };
    
private:
    job_system::JobSystem* job_system_;
    AnalysisConfig config_;
    
    // Monitoring state
    std::atomic<bool> is_monitoring_{false};
    std::thread monitoring_thread_;
    
    // Sample storage
    std::deque<PerformanceSample> sample_history_;
    std::mutex sample_mutex_;
    
    // Hardware detection
    u32 cpu_core_count_;
    u32 numa_node_count_;
    std::vector<u32> cores_per_numa_node_;
    
public:
    explicit ThreadPerformanceAnalyzer(job_system::JobSystem* system);
    ~ThreadPerformanceAnalyzer();
    
    // Configuration
    void set_analysis_config(const AnalysisConfig& config) { config_ = config; }
    void set_sampling_interval(std::chrono::milliseconds interval) { 
        config_.sampling_interval = interval; 
    }
    
    // Monitoring lifecycle
    bool start_monitoring();
    void stop_monitoring();
    bool is_monitoring() const noexcept { return is_monitoring_.load(std::memory_order_acquire); }
    
    // Analysis and results
    AnalysisResults analyze_performance() const;
    AnalysisResults analyze_time_range(std::chrono::high_resolution_clock::time_point start,
                                     std::chrono::high_resolution_clock::time_point end) const;
    
    // Specific analysis functions
    std::vector<std::string> identify_cpu_bottlenecks() const;
    std::vector<std::string> identify_cache_bottlenecks() const;
    std::vector<std::string> identify_numa_issues() const;
    std::vector<std::string> suggest_optimizations() const;
    
    // Educational demonstrations
    void demonstrate_cpu_affinity_effects();
    void demonstrate_cache_locality_importance();
    void demonstrate_false_sharing_impact();
    void demonstrate_numa_awareness_benefits();
    void demonstrate_contention_analysis();
    
    // Visualization and reporting
    std::string generate_performance_report() const;
    std::string generate_cpu_utilization_chart() const;
    std::string generate_cache_analysis_chart() const;
    void export_timeline_data(const std::string& filename) const;
    
    // Real-time monitoring
    PerformanceSample get_current_sample() const;
    std::vector<PerformanceSample> get_recent_samples(u32 count) const;
    void clear_sample_history() {
        std::lock_guard<std::mutex> lock(sample_mutex_);
        sample_history_.clear();
    }
    
private:
    void monitoring_main();
    PerformanceSample collect_sample();
    void detect_hardware_topology();
    void analyze_cache_coherency(AnalysisResults& results) const;
    void analyze_numa_effects(AnalysisResults& results) const;
    std::vector<std::string> generate_optimization_suggestions(const AnalysisResults& results) const;
};

//=============================================================================
// Educational Framework
//=============================================================================

/**
 * @brief Comprehensive educational framework for parallel programming
 * 
 * Provides interactive tutorials, guided experiments, and educational
 * demonstrations of parallel programming concepts with visual feedback.
 */
class EducationalFramework {
public:
    /**
     * @brief Tutorial progress tracking
     */
    struct TutorialProgress {
        std::string tutorial_id;
        std::string student_id;
        u32 current_lesson = 0;
        u32 total_lessons = 0;
        std::vector<bool> lesson_completed;
        std::vector<f64> lesson_scores;
        std::chrono::high_resolution_clock::time_point start_time;
        std::chrono::high_resolution_clock::time_point last_activity;
        
        f64 overall_progress_percent() const {
            if (total_lessons == 0) return 0.0;
            u32 completed = static_cast<u32>(std::count(lesson_completed.begin(), lesson_completed.end(), true));
            return static_cast<f64>(completed) / total_lessons * 100.0;
        }
        
        f64 average_score() const {
            if (lesson_scores.empty()) return 0.0;
            return std::accumulate(lesson_scores.begin(), lesson_scores.end(), 0.0) / lesson_scores.size();
        }
    };
    
    /**
     * @brief Interactive lesson with hands-on components
     */
    class InteractiveLesson {
    private:
        LessonConfig config_;
        std::function<void()> interactive_component_;
        std::vector<std::pair<std::string, std::function<bool()>>> validation_steps_;
        
    public:
        explicit InteractiveLesson(const LessonConfig& config) : config_(config) {}
        
        void set_interactive_component(std::function<void()> component) {
            interactive_component_ = component;
        }
        
        void add_validation_step(const std::string& description, std::function<bool()> validator) {
            validation_steps_.emplace_back(description, validator);
        }
        
        bool execute_lesson(TutorialProgress& progress);
        const LessonConfig& config() const { return config_; }
    };
    
private:
    job_system::JobSystem* job_system_;
    JobSystemVisualizer* visualizer_;
    ConcurrentDataTester* data_tester_;
    ThreadPerformanceAnalyzer* performance_analyzer_;
    
    // Tutorial management
    std::unordered_map<std::string, std::vector<std::unique_ptr<InteractiveLesson>>> tutorials_;
    std::unordered_map<std::string, TutorialProgress> student_progress_;
    std::mutex tutorial_mutex_;
    
    // Deadlock detection system
    class DeadlockDetector {
    private:
        struct ResourceNode {
            std::string resource_name;
            std::thread::id holding_thread;
            std::vector<std::thread::id> waiting_threads;
            std::mutex access_mutex;
        };
        
        std::unordered_map<std::string, std::unique_ptr<ResourceNode>> resources_;
        std::mutex detector_mutex_;
        
    public:
        void register_resource(const std::string& name);
        void acquire_resource(const std::string& name, std::thread::id thread);
        void release_resource(const std::string& name, std::thread::id thread);
        std::vector<std::string> detect_potential_deadlocks();
        std::string generate_dependency_graph() const;
    };
    
    std::unique_ptr<DeadlockDetector> deadlock_detector_;
    
public:
    explicit EducationalFramework(job_system::JobSystem* system,
                                 JobSystemVisualizer* visualizer,
                                 ConcurrentDataTester* data_tester,
                                 ThreadPerformanceAnalyzer* performance_analyzer);
    ~EducationalFramework();
    
    // Tutorial management
    void create_tutorial(const std::string& tutorial_id, const std::string& title,
                        const std::string& description);
    void add_lesson_to_tutorial(const std::string& tutorial_id, 
                               const LessonConfig& lesson_config);
    bool start_tutorial(const std::string& tutorial_id, const std::string& student_id);
    bool advance_lesson(const std::string& student_id);
    TutorialProgress get_progress(const std::string& student_id) const;
    
    // Built-in tutorials
    void create_basic_threading_tutorial();
    void create_work_stealing_tutorial();
    void create_lock_free_programming_tutorial();
    void create_performance_optimization_tutorial();
    void create_deadlock_prevention_tutorial();
    
    // Interactive demonstrations
    void demonstrate_race_conditions();
    void demonstrate_atomic_operations();
    void demonstrate_memory_barriers();
    void demonstrate_producer_consumer_pattern();
    void demonstrate_reader_writer_locks();
    void demonstrate_wait_free_algorithms();
    
    // Deadlock detection and education
    void enable_deadlock_detection() { /* Initialize deadlock detector */ }
    void disable_deadlock_detection() { /* Cleanup deadlock detector */ }
    std::vector<std::string> check_for_deadlocks();
    void demonstrate_deadlock_scenarios();
    void demonstrate_deadlock_prevention_techniques();
    
    // Assessment and feedback
    struct AssessmentResult {
        std::string student_id;
        std::string assessment_id;
        f64 score_percentage = 0.0;
        std::vector<std::string> correct_answers;
        std::vector<std::string> incorrect_answers;
        std::vector<std::string> feedback_points;
        std::chrono::high_resolution_clock::time_point completion_time;
    };
    
    AssessmentResult conduct_assessment(const std::string& student_id,
                                       const std::string& assessment_id);
    
    // Reporting and analytics
    std::string generate_progress_report(const std::string& student_id) const;
    std::string generate_class_analytics() const;
    void export_learning_data(const std::string& filename) const;
    
private:
    void initialize_built_in_tutorials();
    bool validate_lesson_completion(const InteractiveLesson& lesson, 
                                   TutorialProgress& progress);
    void update_progress(const std::string& student_id, u32 lesson_index, f64 score);
};

//=============================================================================
// Thread Safety Tester
//=============================================================================

/**
 * @brief Comprehensive thread safety testing framework
 * 
 * Systematically tests for race conditions, provides simulation
 * of threading issues, and validates thread-safe implementations.
 */
class ThreadSafetyTester {
public:
    /**
     * @brief Thread safety test categories
     */
    enum class TestCategory : u8 {
        RaceConditions = 0,    // Test for data race conditions
        DeadlockDetection = 1, // Test for deadlock scenarios
        LivelockDetection = 2, // Test for livelock scenarios
        AtomicOperations = 3,  // Validate atomic operation correctness
        MemoryOrdering = 4,    // Test memory ordering constraints
        LockFreeCorrectness = 5 // Validate lock-free algorithm correctness
    };
    
    /**
     * @brief Thread safety test results
     */
    struct SafetyTestResults {
        std::string test_name;
        TestCategory category;
        bool safety_verified = false;
        u32 issues_detected = 0;
        std::vector<std::string> detected_issues;
        std::vector<std::string> recommendations;
        f64 test_duration_seconds = 0.0;
        u32 thread_count_tested = 0;
        u64 operations_tested = 0;
        
        struct IssueDetails {
            std::string issue_type;
            std::string description;
            std::vector<std::string> affected_code_locations;
            std::string severity; // "Low", "Medium", "High", "Critical"
            std::string resolution_suggestion;
        };
        
        std::vector<IssueDetails> detailed_issues;
    };
    
private:
    std::vector<RaceConditionTest> race_tests_;
    std::vector<SafetyTestResults> test_history_;
    std::mutex tester_mutex_;
    
    // Test execution infrastructure
    class TestExecutor {
    private:
        u32 thread_count_;
        std::vector<std::thread> test_threads_;
        std::atomic<bool> should_stop_{false};
        std::atomic<u32> active_threads_{0};
        std::barrier<> start_barrier_;
        std::barrier<> end_barrier_;
        
    public:
        explicit TestExecutor(u32 thread_count);
        ~TestExecutor();
        
        template<typename TestFunction>
        void execute_parallel_test(TestFunction test_func, 
                                 std::chrono::milliseconds duration);
        
        void stop_all_threads();
        bool all_threads_finished() const;
    };
    
public:
    ThreadSafetyTester() = default;
    ~ThreadSafetyTester() = default;
    
    // Core testing interface
    SafetyTestResults test_race_conditions(const std::string& test_name,
                                         std::function<void(u32)> test_function,
                                         u32 thread_count = 4,
                                         std::chrono::seconds duration = std::chrono::seconds{5});
    
    SafetyTestResults test_deadlock_susceptibility(const std::string& test_name,
                                                 std::function<void()> test_function,
                                                 u32 thread_count = 4);
    
    SafetyTestResults test_atomic_correctness(const std::string& test_name,
                                            std::function<void()> test_function,
                                            u32 thread_count = 4);
    
    SafetyTestResults test_memory_ordering(const std::string& test_name,
                                         std::function<void()> test_function,
                                         u32 thread_count = 4);
    
    // Specific safety scenarios
    SafetyTestResults test_increment_race_condition();
    SafetyTestResults test_double_checked_locking();
    SafetyTestResults test_producer_consumer_safety();
    SafetyTestResults test_singleton_thread_safety();
    SafetyTestResults test_lock_free_queue_safety();
    SafetyTestResults test_aba_problem_detection();
    
    // Race condition simulation
    void simulate_classic_race_condition();
    void simulate_check_then_act_race();
    void simulate_read_modify_write_race();
    void simulate_initialization_race();
    
    // Educational demonstrations
    void demonstrate_race_condition_fixes();
    void demonstrate_proper_synchronization();
    void demonstrate_atomic_vs_mutex_performance();
    void demonstrate_memory_ordering_effects();
    
    // Batch testing
    std::vector<SafetyTestResults> run_comprehensive_safety_test_suite();
    SafetyTestResults test_custom_concurrent_structure(
        const std::string& structure_name,
        std::function<void()> setup,
        std::function<void(u32)> operations,
        std::function<bool()> correctness_check,
        std::function<void()> cleanup
    );
    
    // Results and analysis
    const std::vector<SafetyTestResults>& get_test_history() const { return test_history_; }
    std::string generate_safety_report() const;
    std::vector<std::string> get_all_detected_issues() const;
    std::vector<std::string> get_safety_recommendations() const;
    
    // Configuration
    void add_custom_race_test(const RaceConditionTest& test) {
        std::lock_guard<std::mutex> lock(tester_mutex_);
        race_tests_.push_back(test);
    }
    
    void clear_test_history() {
        std::lock_guard<std::mutex> lock(tester_mutex_);
        test_history_.clear();
    }
    
private:
    SafetyTestResults execute_safety_test(const std::string& test_name,
                                        TestCategory category,
                                        std::function<void()> test_execution,
                                        std::function<std::vector<std::string>()> issue_detector);
    
    std::vector<std::string> detect_race_conditions_in_execution();
    std::vector<std::string> detect_deadlock_patterns();
    std::vector<std::string> analyze_atomic_operation_correctness();
    bool validate_memory_ordering_constraints();
    
    void record_test_result(const SafetyTestResults& result) {
        std::lock_guard<std::mutex> lock(tester_mutex_);
        test_history_.push_back(result);
    }
};

//=============================================================================
// Amdahl's Law Visualizer
//=============================================================================

/**
 * @brief Visualization and analysis tool for Amdahl's Law and parallel efficiency
 * 
 * Provides quantitative analysis of parallel algorithm performance,
 * scalability predictions, and optimization guidance based on Amdahl's Law.
 */
class AmdahlsLawVisualizer {
public:
    /**
     * @brief Algorithm performance characteristics
     */
    struct AlgorithmProfile {
        std::string algorithm_name;
        f64 sequential_fraction = 0.0;      // Fraction that cannot be parallelized
        f64 parallel_fraction = 1.0;        // Fraction that can be parallelized
        f64 parallelization_overhead = 0.0; // Overhead cost of parallelization
        u32 optimal_thread_count = 0;       // Calculated optimal thread count
        
        // Measured performance data
        std::vector<std::pair<u32, f64>> thread_count_to_speedup;
        std::vector<std::pair<u32, f64>> thread_count_to_efficiency;
        
        f64 theoretical_max_speedup() const {
            return 1.0 / sequential_fraction;
        }
        
        f64 predicted_speedup(u32 thread_count) const {
            if (parallel_fraction <= 0.0) return 1.0;
            return 1.0 / (sequential_fraction + (parallel_fraction / thread_count) + parallelization_overhead);
        }
        
        f64 efficiency(u32 thread_count) const {
            return predicted_speedup(thread_count) / thread_count;
        }
    };
    
    /**
     * @brief Scalability analysis results
     */
    struct ScalabilityAnalysis {
        std::string algorithm_name;
        AlgorithmProfile profile;
        
        // Scalability metrics
        f64 strong_scaling_efficiency = 0.0;    // Fixed problem size, more processors
        f64 weak_scaling_efficiency = 0.0;      // Proportional problem size increase
        u32 scalability_limit = 0;              // Thread count where efficiency drops below threshold
        f64 parallel_efficiency_at_limit = 0.0;
        
        // Optimization recommendations
        std::vector<std::string> bottleneck_analysis;
        std::vector<std::string> optimization_suggestions;
        
        // Economic analysis
        f64 cost_benefit_ratio = 0.0;          // Performance gain vs resource cost
        u32 economically_optimal_thread_count = 0;
    };
    
private:
    job_system::JobSystem* job_system_;
    std::vector<AlgorithmProfile> algorithm_profiles_;
    std::vector<ScalabilityAnalysis> analysis_history_;
    std::mutex analyzer_mutex_;
    
public:
    explicit AmdahlsLawVisualizer(job_system::JobSystem* system);
    ~AmdahlsLawVisualizer() = default;
    
    // Algorithm profiling
    AlgorithmProfile profile_algorithm(const std::string& name,
                                     std::function<void()> sequential_implementation,
                                     std::function<void(u32)> parallel_implementation,
                                     u32 max_thread_count = 16);
    
    AlgorithmProfile profile_ecs_system(const std::string& system_name,
                                       std::function<void()> system_update,
                                       u32 entity_count = 10000);
    
    // Built-in algorithm demonstrations
    AlgorithmProfile demonstrate_parallel_sum();
    AlgorithmProfile demonstrate_parallel_sort();
    AlgorithmProfile demonstrate_matrix_multiplication();
    AlgorithmProfile demonstrate_monte_carlo_simulation();
    AlgorithmProfile demonstrate_parallel_search();
    
    // Analysis and visualization
    ScalabilityAnalysis analyze_scalability(const AlgorithmProfile& profile);
    void compare_algorithms(const std::vector<std::string>& algorithm_names);
    
    // Educational visualizations
    std::string generate_amdahls_law_chart(const AlgorithmProfile& profile) const;
    std::string generate_speedup_comparison_chart(const std::vector<AlgorithmProfile>& profiles) const;
    std::string generate_efficiency_analysis_chart(const AlgorithmProfile& profile) const;
    
    // Interactive demonstrations
    void demonstrate_sequential_bottleneck_impact();
    void demonstrate_parallelization_overhead_effects();
    void demonstrate_optimal_thread_count_calculation();
    void demonstrate_strong_vs_weak_scaling();
    
    // Prediction and optimization
    f64 predict_performance_gain(const std::string& algorithm_name, u32 thread_count) const;
    u32 calculate_optimal_thread_count(const std::string& algorithm_name, 
                                      f64 efficiency_threshold = 0.8) const;
    std::vector<std::string> suggest_parallelization_improvements(const std::string& algorithm_name) const;
    
    // Data access and management
    const std::vector<AlgorithmProfile>& get_algorithm_profiles() const { return algorithm_profiles_; }
    const std::vector<ScalabilityAnalysis>& get_analysis_history() const { return analysis_history_; }
    
    void save_profiles(const std::string& filename) const;
    bool load_profiles(const std::string& filename);
    
    // Reporting
    std::string generate_scalability_report() const;
    std::string generate_optimization_recommendations() const;
    void export_analysis_data(const std::string& filename) const;
    
private:
    f64 measure_execution_time(std::function<void()> function, u32 iterations = 10) const;
    f64 calculate_sequential_fraction(const AlgorithmProfile& profile) const;
    u32 find_optimal_thread_count(const AlgorithmProfile& profile, f64 efficiency_threshold) const;
    std::vector<std::string> analyze_bottlenecks(const AlgorithmProfile& profile) const;
    
    void record_algorithm_profile(const AlgorithmProfile& profile) {
        std::lock_guard<std::mutex> lock(analyzer_mutex_);
        algorithm_profiles_.push_back(profile);
    }
    
    void record_scalability_analysis(const ScalabilityAnalysis& analysis) {
        std::lock_guard<std::mutex> lock(analyzer_mutex_);
        analysis_history_.push_back(analysis);
    }
};

//=============================================================================
// Main Parallel Computing Laboratory Interface
//=============================================================================

/**
 * @brief Comprehensive parallel computing laboratory integrating all components
 * 
 * Provides unified interface for all parallel computing education and
 * analysis tools, with seamless integration into ECScope systems.
 */
class ParallelComputingLab {
private:
    // Core systems
    job_system::JobSystem* job_system_;
    
    // Laboratory components
    std::unique_ptr<JobSystemVisualizer> visualizer_;
    std::unique_ptr<ConcurrentDataTester> data_tester_;
    std::unique_ptr<ThreadPerformanceAnalyzer> performance_analyzer_;
    std::unique_ptr<EducationalFramework> educational_framework_;
    std::unique_ptr<ThreadSafetyTester> safety_tester_;
    std::unique_ptr<AmdahlsLawVisualizer> amdahls_visualizer_;
    
    // Configuration
    bool auto_start_visualization_;
    bool enable_comprehensive_logging_;
    std::string output_directory_;
    
public:
    /**
     * @brief Laboratory configuration
     */
    struct LabConfig {
        bool auto_start_visualization = true;
        bool enable_performance_monitoring = true;
        bool enable_educational_features = true;
        bool enable_safety_testing = true;
        bool enable_comprehensive_logging = true;
        std::string output_directory = "parallel_lab_output";
        VisualizationMode visualization_mode = VisualizationMode::MediumFrequency;
    };
    
    explicit ParallelComputingLab(job_system::JobSystem* system, 
                                 const LabConfig& config = LabConfig{});
    ~ParallelComputingLab();
    
    // Lifecycle management
    bool initialize();
    void shutdown();
    bool is_initialized() const;
    
    // Component access
    JobSystemVisualizer& visualizer() { return *visualizer_; }
    ConcurrentDataTester& data_tester() { return *data_tester_; }
    ThreadPerformanceAnalyzer& performance_analyzer() { return *performance_analyzer_; }
    EducationalFramework& educational_framework() { return *educational_framework_; }
    ThreadSafetyTester& safety_tester() { return *safety_tester_; }
    AmdahlsLawVisualizer& amdahls_visualizer() { return *amdahls_visualizer_; }
    
    // Comprehensive demonstrations
    void run_complete_demonstration();
    void run_educational_workshop();
    void run_performance_analysis_session();
    void run_thread_safety_audit();
    void run_scalability_analysis();
    
    // Integrated tutorials
    void start_beginner_parallel_programming_course();
    void start_advanced_concurrent_programming_course();
    void start_performance_optimization_course();
    void start_lock_free_programming_course();
    
    // Comprehensive reporting
    std::string generate_comprehensive_report() const;
    void export_all_data(const std::string& directory) const;
    void create_educational_summary(const std::string& student_id) const;
    
    // ECScope integration features
    void integrate_with_ecs_systems();
    void monitor_ecs_performance();
    void optimize_ecs_parallelization();
    void validate_ecs_thread_safety();
};

//=============================================================================
// Utility Functions and Helpers
//=============================================================================

namespace utils {
    
    /**
     * @brief CPU topology detection utilities
     */
    struct CpuTopology {
        u32 logical_cores;
        u32 physical_cores;
        u32 numa_nodes;
        std::vector<std::vector<u32>> core_groups; // Cores grouped by NUMA node
    };
    
    CpuTopology detect_cpu_topology();
    void set_thread_affinity(std::thread::id thread_id, u32 core_id);
    void set_numa_policy(u32 numa_node);
    
    /**
     * @brief Performance measurement utilities
     */
    class HighResolutionTimer {
    private:
        std::chrono::high_resolution_clock::time_point start_time_;
        
    public:
        void start() { start_time_ = std::chrono::high_resolution_clock::now(); }
        
        template<typename Duration = std::chrono::nanoseconds>
        typename Duration::rep elapsed() const {
            auto end_time = std::chrono::high_resolution_clock::now();
            return std::chrono::duration_cast<Duration>(end_time - start_time_).count();
        }
    };
    
    /**
     * @brief Statistics calculation utilities
     */
    template<typename Container>
    f64 calculate_mean(const Container& values) {
        if (values.empty()) return 0.0;
        f64 sum = std::accumulate(values.begin(), values.end(), 0.0);
        return sum / values.size();
    }
    
    template<typename Container>
    f64 calculate_standard_deviation(const Container& values) {
        if (values.empty()) return 0.0;
        f64 mean = calculate_mean(values);
        f64 variance = 0.0;
        for (const auto& value : values) {
            f64 diff = static_cast<f64>(value) - mean;
            variance += diff * diff;
        }
        variance /= values.size();
        return std::sqrt(variance);
    }
    
    template<typename Container>
    typename Container::value_type calculate_percentile(const Container& values, f64 percentile) {
        if (values.empty()) return {};
        auto sorted_values = values;
        std::sort(sorted_values.begin(), sorted_values.end());
        usize index = static_cast<usize>(percentile * (sorted_values.size() - 1));
        return sorted_values[index];
    }
    
    /**
     * @brief Chart generation utilities
     */
    std::string generate_ascii_chart(const std::vector<f64>& data, 
                                   const std::string& title = "",
                                   u32 width = 80, u32 height = 20);
    
    std::string generate_histogram(const std::vector<f64>& data,
                                 u32 bin_count = 10,
                                 const std::string& title = "");
    
} // namespace utils

} // namespace ecscope::parallel_lab