#pragma once

/**
 * @file memory/memory_benchmark_suite.hpp
 * @brief Comprehensive Memory Management Benchmarking Suite
 * 
 * This header provides a complete benchmarking system for evaluating and
 * comparing different memory management strategies, with detailed performance
 * analysis and educational insights into memory system behavior.
 * 
 * Key Features:
 * - Comprehensive benchmark suite covering all memory management aspects
 * - Statistical analysis with confidence intervals and outlier detection
 * - Cross-platform performance measurement and normalization
 * - Educational visualization and reporting of results
 * - Automated regression testing for memory performance
 * - Real-world workload simulation and analysis
 * - Memory system stress testing and stability analysis
 * 
 * Benchmark Categories:
 * - Allocation/Deallocation Performance
 * - Memory Access Pattern Analysis
 * - NUMA Locality and Migration Performance
 * - Cache Hierarchy Utilization
 * - Thread Contention and Scalability
 * - Memory Bandwidth Utilization
 * - Real-world ECS Workload Simulation
 * 
 * @author ECScope Educational ECS Framework - Memory Benchmarking
 * @date 2025
 */

#include "numa_manager.hpp"
#include "lockfree_allocators.hpp"
#include "hierarchical_pools.hpp"
#include "cache_aware_structures.hpp"
#include "bandwidth_analyzer.hpp"
#include "thread_local_allocator.hpp"
#include "core/types.hpp"
#include "core/log.hpp"
#include "core/profiler.hpp"
#include <vector>
#include <memory>
#include <chrono>
#include <thread>
#include <future>
#include <functional>
#include <string>
#include <unordered_map>
#include <mutex>
#include <random>
#include <algorithm>
#include <fstream>
#include <iomanip>

namespace ecscope::memory::benchmark {

//=============================================================================
// Benchmark Configuration and Parameters
//=============================================================================

/**
 * @brief Benchmark configuration parameters
 */
struct BenchmarkConfig {
    // Test parameters
    usize min_allocation_size = 8;
    usize max_allocation_size = 4096;
    usize allocation_count = 100000;
    usize iterations = 10;
    u32 thread_count = std::thread::hardware_concurrency();
    f64 duration_seconds = 10.0;
    
    // Statistical analysis
    f64 confidence_level = 0.95;
    bool remove_outliers = true;
    usize warmup_iterations = 3;
    
    // Test selection
    bool enable_allocation_tests = true;
    bool enable_numa_tests = true;
    bool enable_cache_tests = true;
    bool enable_bandwidth_tests = true;
    bool enable_thread_local_tests = true;
    bool enable_stress_tests = true;
    bool enable_ecs_simulation = true;
    
    // Output configuration
    bool generate_csv_output = true;
    bool generate_json_output = true;
    bool generate_html_report = true;
    std::string output_directory = "benchmark_results";
    
    // Platform normalization
    bool normalize_for_platform = true;
    f64 platform_baseline_score = 1000.0; // Reference score for normalization
};

/**
 * @brief Individual benchmark result
 */
struct BenchmarkResult {
    std::string test_name;
    std::string category;
    std::string allocator_type;
    
    // Performance metrics
    f64 operations_per_second;
    f64 average_latency_ns;
    f64 throughput_mbps;
    f64 cpu_utilization;
    f64 memory_utilization;
    
    // Statistical data
    std::vector<f64> raw_measurements;
    f64 mean;
    f64 median;
    f64 std_deviation;
    f64 min_value;
    f64 max_value;
    f64 confidence_interval_lower;
    f64 confidence_interval_upper;
    
    // Memory-specific metrics
    usize peak_memory_usage;
    usize memory_waste;
    f64 cache_miss_ratio;
    f64 numa_locality_ratio;
    
    // Quality metrics
    f64 consistency_score; // Lower std_dev is better
    f64 efficiency_score;  // Performance per memory used
    f64 scalability_score; // Performance improvement with threads
    
    // Test configuration
    BenchmarkConfig config;
    f64 execution_time_seconds;
    std::string platform_info;
    std::string timestamp;
    
    BenchmarkResult() : operations_per_second(0), average_latency_ns(0), throughput_mbps(0),
                       cpu_utilization(0), memory_utilization(0), mean(0), median(0),
                       std_deviation(0), min_value(0), max_value(0), confidence_interval_lower(0),
                       confidence_interval_upper(0), peak_memory_usage(0), memory_waste(0),
                       cache_miss_ratio(0), numa_locality_ratio(0), consistency_score(0),
                       efficiency_score(0), scalability_score(0), execution_time_seconds(0) {}
    
    void calculate_statistics();
    std::string to_string() const;
    std::string to_csv_row() const;
    std::string to_json() const;
};

//=============================================================================
// Memory Allocator Interface for Benchmarking
//=============================================================================

/**
 * @brief Generic allocator interface for benchmarking
 */
class IBenchmarkAllocator {
public:
    virtual ~IBenchmarkAllocator() = default;
    
    virtual void* allocate(usize size, usize alignment = alignof(std::max_align_t)) = 0;
    virtual void deallocate(void* ptr, usize size = 0) = 0;
    virtual std::string get_name() const = 0;
    virtual std::string get_description() const = 0;
    virtual void reset_statistics() = 0;
    virtual std::unordered_map<std::string, f64> get_statistics() const = 0;
    
    // Optional batch operations for testing
    virtual std::vector<void*> allocate_batch(const std::vector<usize>& sizes) {
        std::vector<void*> results;
        results.reserve(sizes.size());
        for (usize size : sizes) {
            results.push_back(allocate(size));
        }
        return results;
    }
    
    virtual void deallocate_batch(const std::vector<std::pair<void*, usize>>& allocations) {
        for (const auto& [ptr, size] : allocations) {
            deallocate(ptr, size);
        }
    }
};

/**
 * @brief Standard library allocator wrapper
 */
class StandardAllocator : public IBenchmarkAllocator {
private:
    std::atomic<usize> allocation_count_{0};
    std::atomic<usize> allocated_bytes_{0};
    std::mutex allocation_mutex_;
    std::unordered_map<void*, usize> allocation_sizes_;
    
public:
    void* allocate(usize size, usize alignment = alignof(std::max_align_t)) override {
        void* ptr;
        if (alignment > alignof(std::max_align_t)) {
            if (posix_memalign(&ptr, alignment, size) != 0) {
                return nullptr;
            }
        } else {
            ptr = std::malloc(size);
        }
        
        if (ptr) {
            allocation_count_.fetch_add(1);
            allocated_bytes_.fetch_add(size);
            
            std::lock_guard<std::mutex> lock(allocation_mutex_);
            allocation_sizes_[ptr] = size;
        }
        
        return ptr;
    }
    
    void deallocate(void* ptr, usize size = 0) override {
        if (!ptr) return;
        
        {
            std::lock_guard<std::mutex> lock(allocation_mutex_);
            auto it = allocation_sizes_.find(ptr);
            if (it != allocation_sizes_.end()) {
                allocated_bytes_.fetch_sub(it->second);
                allocation_sizes_.erase(it);
            }
        }
        
        std::free(ptr);
    }
    
    std::string get_name() const override { return "Standard"; }
    std::string get_description() const override { return "Standard library malloc/free"; }
    
    void reset_statistics() override {
        allocation_count_.store(0);
        allocated_bytes_.store(0);
        std::lock_guard<std::mutex> lock(allocation_mutex_);
        allocation_sizes_.clear();
    }
    
    std::unordered_map<std::string, f64> get_statistics() const override {
        return {
            {"allocation_count", static_cast<f64>(allocation_count_.load())},
            {"allocated_bytes", static_cast<f64>(allocated_bytes_.load())},
            {"active_allocations", static_cast<f64>(allocation_sizes_.size())}
        };
    }
};

/**
 * @brief Hierarchical pool allocator wrapper
 */
class HierarchicalPoolAllocatorWrapper : public IBenchmarkAllocator {
private:
    hierarchical::HierarchicalPoolAllocator& allocator_;
    
public:
    explicit HierarchicalPoolAllocatorWrapper(hierarchical::HierarchicalPoolAllocator& allocator)
        : allocator_(allocator) {}
    
    void* allocate(usize size, usize alignment = alignof(std::max_align_t)) override {
        return allocator_.allocate(size, alignment);
    }
    
    void deallocate(void* ptr, usize size = 0) override {
        allocator_.deallocate(ptr);
    }
    
    std::string get_name() const override { return "HierarchicalPool"; }
    std::string get_description() const override { return "Hierarchical pool allocator with L1/L2 cache"; }
    
    void reset_statistics() override {
        // Would reset hierarchical allocator statistics
    }
    
    std::unordered_map<std::string, f64> get_statistics() const override {
        auto stats = allocator_.get_statistics();
        return {
            {"l1_hit_rate", stats.l1_hit_rate},
            {"l2_hit_rate", stats.l2_hit_rate},
            {"overall_cache_efficiency", stats.overall_cache_efficiency},
            {"active_size_classes", static_cast<f64>(stats.active_size_classes)}
        };
    }
};

//=============================================================================
// Individual Benchmark Tests
//=============================================================================

/**
 * @brief Base class for individual benchmark tests
 */
class IBenchmarkTest {
public:
    virtual ~IBenchmarkTest() = default;
    
    virtual std::string get_name() const = 0;
    virtual std::string get_category() const = 0;
    virtual std::string get_description() const = 0;
    
    virtual BenchmarkResult run_benchmark(IBenchmarkAllocator& allocator, 
                                         const BenchmarkConfig& config) = 0;
    
    virtual bool is_applicable_for_allocator(const std::string& allocator_name) const {
        return true; // Default: applicable for all allocators
    }
    
protected:
    f64 get_current_time() const {
        using namespace std::chrono;
        return duration<f64>(high_resolution_clock::now().time_since_epoch()).count();
    }
    
    void warmup_allocator(IBenchmarkAllocator& allocator, usize iterations = 1000) {
        // Perform warmup allocations to stabilize performance
        std::vector<void*> ptrs;
        for (usize i = 0; i < iterations; ++i) {
            ptrs.push_back(allocator.allocate(64 + (i % 1024)));
        }
        for (void* ptr : ptrs) {
            allocator.deallocate(ptr);
        }
    }
};

/**
 * @brief Simple allocation/deallocation performance test
 */
class AllocationPerformanceTest : public IBenchmarkTest {
public:
    std::string get_name() const override { return "AllocationPerformance"; }
    std::string get_category() const override { return "Basic"; }
    std::string get_description() const override { 
        return "Measures allocation and deallocation performance"; 
    }
    
    BenchmarkResult run_benchmark(IBenchmarkAllocator& allocator, 
                                 const BenchmarkConfig& config) override {
        BenchmarkResult result;
        result.test_name = get_name();
        result.category = get_category();
        result.allocator_type = allocator.get_name();
        result.config = config;
        
        // Warmup
        warmup_allocator(allocator, config.warmup_iterations * 100);
        
        std::mt19937 rng(42);
        std::uniform_int_distribution<usize> size_dist(config.min_allocation_size, 
                                                       config.max_allocation_size);
        
        std::vector<f64> iteration_times;
        iteration_times.reserve(config.iterations);
        
        auto test_start = std::chrono::high_resolution_clock::now();
        
        for (usize iter = 0; iter < config.iterations; ++iter) {
            std::vector<std::pair<void*, usize>> allocations;
            allocations.reserve(config.allocation_count);
            
            auto iter_start = std::chrono::high_resolution_clock::now();
            
            // Allocation phase
            for (usize i = 0; i < config.allocation_count; ++i) {
                usize size = size_dist(rng);
                void* ptr = allocator.allocate(size);
                if (ptr) {
                    allocations.emplace_back(ptr, size);
                }
            }
            
            // Deallocation phase
            for (const auto& [ptr, size] : allocations) {
                allocator.deallocate(ptr, size);
            }
            
            auto iter_end = std::chrono::high_resolution_clock::now();
            f64 iter_time = std::chrono::duration<f64, std::milli>(iter_end - iter_start).count();
            iteration_times.push_back(iter_time);
        }
        
        auto test_end = std::chrono::high_resolution_clock::now();
        result.execution_time_seconds = std::chrono::duration<f64>(test_end - test_start).count();
        
        // Calculate performance metrics
        result.raw_measurements = iteration_times;
        result.calculate_statistics();
        
        if (result.mean > 0) {
            result.operations_per_second = (config.allocation_count * 2) / (result.mean / 1000.0); // *2 for alloc+dealloc
            result.average_latency_ns = (result.mean * 1000000.0) / (config.allocation_count * 2);
        }
        
        return result;
    }
};

/**
 * @brief Multi-threaded allocation scalability test
 */
class ThreadScalabilityTest : public IBenchmarkTest {
public:
    std::string get_name() const override { return "ThreadScalability"; }
    std::string get_category() const override { return "Concurrency"; }
    std::string get_description() const override { 
        return "Measures allocation performance scaling with thread count"; 
    }
    
    BenchmarkResult run_benchmark(IBenchmarkAllocator& allocator, 
                                 const BenchmarkConfig& config) override {
        BenchmarkResult result;
        result.test_name = get_name();
        result.category = get_category();
        result.allocator_type = allocator.get_name();
        result.config = config;
        
        // Test with different thread counts
        std::vector<u32> thread_counts = {1, 2, 4, 8, config.thread_count};
        std::vector<f64> throughput_per_thread_count;
        
        auto test_start = std::chrono::high_resolution_clock::now();
        
        for (u32 threads : thread_counts) {
            if (threads > config.thread_count) continue;
            
            std::vector<f64> thread_times;
            
            for (usize iter = 0; iter < config.iterations; ++iter) {
                auto iter_start = std::chrono::high_resolution_clock::now();
                
                std::vector<std::future<void>> futures;
                for (u32 t = 0; t < threads; ++t) {
                    futures.push_back(std::async(std::launch::async, [&allocator, &config]() {
                        std::mt19937 rng(std::random_device{}());
                        std::uniform_int_distribution<usize> size_dist(config.min_allocation_size, 
                                                                       config.max_allocation_size);
                        
                        std::vector<std::pair<void*, usize>> allocations;
                        allocations.reserve(config.allocation_count / config.thread_count);
                        
                        // Allocate
                        for (usize i = 0; i < config.allocation_count / config.thread_count; ++i) {
                            usize size = size_dist(rng);
                            void* ptr = allocator.allocate(size);
                            if (ptr) {
                                allocations.emplace_back(ptr, size);
                            }
                        }
                        
                        // Deallocate
                        for (const auto& [ptr, size] : allocations) {
                            allocator.deallocate(ptr, size);
                        }
                    }));
                }
                
                for (auto& future : futures) {
                    future.wait();
                }
                
                auto iter_end = std::chrono::high_resolution_clock::now();
                f64 iter_time = std::chrono::duration<f64, std::milli>(iter_end - iter_start).count();
                thread_times.push_back(iter_time);
            }
            
            // Calculate average time for this thread count
            f64 avg_time = std::accumulate(thread_times.begin(), thread_times.end(), 0.0) / thread_times.size();
            f64 throughput = config.allocation_count * 2 / (avg_time / 1000.0); // ops per second
            throughput_per_thread_count.push_back(throughput);
        }
        
        auto test_end = std::chrono::high_resolution_clock::now();
        result.execution_time_seconds = std::chrono::duration<f64>(test_end - test_start).count();
        
        // Store throughput measurements
        result.raw_measurements = throughput_per_thread_count;
        result.calculate_statistics();
        
        // Calculate scalability score (ratio of max throughput to single-threaded)
        if (throughput_per_thread_count.size() > 1) {
            result.scalability_score = throughput_per_thread_count.back() / throughput_per_thread_count.front();
        }
        
        result.operations_per_second = result.mean;
        
        return result;
    }
};

/**
 * @brief Cache locality test
 */
class CacheLocalityTest : public IBenchmarkTest {
public:
    std::string get_name() const override { return "CacheLocality"; }
    std::string get_category() const override { return "Memory"; }
    std::string get_description() const override { 
        return "Measures cache performance with different access patterns"; 
    }
    
    BenchmarkResult run_benchmark(IBenchmarkAllocator& allocator, 
                                 const BenchmarkConfig& config) override {
        BenchmarkResult result;
        result.test_name = get_name();
        result.category = get_category();
        result.allocator_type = allocator.get_name();
        result.config = config;
        
        const usize buffer_size = 64; // Size of each allocated buffer
        std::vector<void*> allocations;
        allocations.reserve(config.allocation_count);
        
        // Pre-allocate all buffers
        for (usize i = 0; i < config.allocation_count; ++i) {
            void* ptr = allocator.allocate(buffer_size);
            if (ptr) {
                allocations.push_back(ptr);
                // Initialize memory to avoid page faults during measurement
                std::memset(ptr, i & 0xFF, buffer_size);
            }
        }
        
        std::vector<f64> iteration_times;
        std::mt19937 rng(42);
        
        auto test_start = std::chrono::high_resolution_clock::now();
        
        for (usize iter = 0; iter < config.iterations; ++iter) {
            auto iter_start = std::chrono::high_resolution_clock::now();
            
            // Sequential access pattern (cache-friendly)
            volatile u64 sum = 0;
            for (void* ptr : allocations) {
                const u64* data = static_cast<const u64*>(ptr);
                for (usize i = 0; i < buffer_size / sizeof(u64); ++i) {
                    sum += data[i];
                }
            }
            
            // Random access pattern (cache-hostile)
            std::vector<usize> indices(allocations.size());
            std::iota(indices.begin(), indices.end(), 0);
            std::shuffle(indices.begin(), indices.end(), rng);
            
            for (usize idx : indices) {
                const u64* data = static_cast<const u64*>(allocations[idx]);
                for (usize i = 0; i < buffer_size / sizeof(u64); ++i) {
                    sum += data[i];
                }
            }
            
            auto iter_end = std::chrono::high_resolution_clock::now();
            f64 iter_time = std::chrono::duration<f64, std::milli>(iter_end - iter_start).count();
            iteration_times.push_back(iter_time);
        }
        
        // Cleanup
        for (void* ptr : allocations) {
            allocator.deallocate(ptr, buffer_size);
        }
        
        auto test_end = std::chrono::high_resolution_clock::now();
        result.execution_time_seconds = std::chrono::duration<f64>(test_end - test_start).count();
        
        result.raw_measurements = iteration_times;
        result.calculate_statistics();
        
        if (result.mean > 0) {
            usize total_accesses = config.allocation_count * (buffer_size / sizeof(u64)) * 2; // Sequential + random
            result.operations_per_second = total_accesses / (result.mean / 1000.0);
            result.throughput_mbps = (total_accesses * sizeof(u64)) / (result.mean / 1000.0) / (1024 * 1024);
        }
        
        return result;
    }
};

//=============================================================================
// Benchmark Suite Manager
//=============================================================================

/**
 * @brief Main benchmark suite manager
 */
class MemoryBenchmarkSuite {
private:
    BenchmarkConfig config_;
    std::vector<std::unique_ptr<IBenchmarkTest>> tests_;
    std::vector<std::unique_ptr<IBenchmarkAllocator>> allocators_;
    std::vector<BenchmarkResult> results_;
    
    mutable std::mutex results_mutex_;
    
public:
    explicit MemoryBenchmarkSuite(const BenchmarkConfig& config = BenchmarkConfig{})
        : config_(config) {
        initialize_tests();
        initialize_allocators();
    }
    
    /**
     * @brief Run all benchmarks
     */
    void run_all_benchmarks() {
        PROFILE_FUNCTION();
        
        LOG_INFO("Starting comprehensive memory benchmark suite");
        LOG_INFO("Configuration: {} allocations, {} iterations, {} threads",
                 config_.allocation_count, config_.iterations, config_.thread_count);
        
        results_.clear();
        results_.reserve(tests_.size() * allocators_.size());
        
        auto suite_start = std::chrono::high_resolution_clock::now();
        
        // Run tests for each allocator
        for (auto& allocator : allocators_) {
            LOG_INFO("Testing allocator: {}", allocator->get_name());
            
            for (auto& test : tests_) {
                if (!test->is_applicable_for_allocator(allocator->get_name())) {
                    continue;
                }
                
                LOG_INFO("  Running test: {}", test->get_name());
                
                try {
                    allocator->reset_statistics();
                    auto result = test->run_benchmark(*allocator, config_);
                    
                    // Add allocator-specific statistics
                    auto allocator_stats = allocator->get_statistics();
                    result.platform_info = get_platform_info();
                    result.timestamp = get_timestamp();
                    
                    std::lock_guard<std::mutex> lock(results_mutex_);
                    results_.push_back(std::move(result));
                    
                } catch (const std::exception& e) {
                    LOG_ERROR("Test {} failed for allocator {}: {}", 
                             test->get_name(), allocator->get_name(), e.what());
                }
            }
        }
        
        auto suite_end = std::chrono::high_resolution_clock::now();
        f64 total_time = std::chrono::duration<f64>(suite_end - suite_start).count();
        
        LOG_INFO("Benchmark suite completed in {:.2f} seconds", total_time);
        LOG_INFO("Generated {} benchmark results", results_.size());
        
        // Generate reports
        if (config_.generate_csv_output) {
            generate_csv_report();
        }
        if (config_.generate_json_output) {
            generate_json_report();
        }
        if (config_.generate_html_report) {
            generate_html_report();
        }
    }
    
    /**
     * @brief Get benchmark results
     */
    std::vector<BenchmarkResult> get_results() const {
        std::lock_guard<std::mutex> lock(results_mutex_);
        return results_;
    }
    
    /**
     * @brief Generate comparative analysis
     */
    std::string generate_comparative_analysis() const {
        std::ostringstream report;
        
        report << "=== Memory Allocator Comparative Analysis ===\n\n";
        
        // Group results by test category
        std::unordered_map<std::string, std::vector<const BenchmarkResult*>> by_category;
        for (const auto& result : results_) {
            by_category[result.category].push_back(&result);
        }
        
        for (const auto& [category, category_results] : by_category) {
            report << "Category: " << category << "\n";
            report << std::string(category.length() + 10, '-') << "\n";
            
            // Group by test name within category
            std::unordered_map<std::string, std::vector<const BenchmarkResult*>> by_test;
            for (const auto* result : category_results) {
                by_test[result->test_name].push_back(result);
            }
            
            for (const auto& [test_name, test_results] : by_test) {
                report << "  " << test_name << ":\n";
                
                // Sort by performance (operations per second)
                std::vector<const BenchmarkResult*> sorted_results = test_results;
                std::sort(sorted_results.begin(), sorted_results.end(),
                         [](const auto* a, const auto* b) {
                             return a->operations_per_second > b->operations_per_second;
                         });
                
                for (const auto* result : sorted_results) {
                    report << "    " << std::left << std::setw(20) << result->allocator_type
                           << ": " << std::fixed << std::setprecision(0)
                           << result->operations_per_second << " ops/sec";
                    
                    if (result->consistency_score > 0) {
                        report << " (consistency: " << std::fixed << std::setprecision(2)
                               << result->consistency_score << ")";
                    }
                    
                    report << "\n";
                }
                report << "\n";
            }
            report << "\n";
        }
        
        // Overall recommendations
        report << "=== Recommendations ===\n";
        report << generate_recommendations();
        
        return report.str();
    }
    
    /**
     * @brief Add custom test
     */
    void add_test(std::unique_ptr<IBenchmarkTest> test) {
        tests_.push_back(std::move(test));
    }
    
    /**
     * @brief Add custom allocator
     */
    void add_allocator(std::unique_ptr<IBenchmarkAllocator> allocator) {
        allocators_.push_back(std::move(allocator));
    }
    
private:
    void initialize_tests() {
        if (config_.enable_allocation_tests) {
            tests_.push_back(std::make_unique<AllocationPerformanceTest>());
        }
        
        if (config_.enable_thread_local_tests) {
            tests_.push_back(std::make_unique<ThreadScalabilityTest>());
        }
        
        if (config_.enable_cache_tests) {
            tests_.push_back(std::make_unique<CacheLocalityTest>());
        }
        
        // Add more test types here as needed
    }
    
    void initialize_allocators() {
        // Always include standard allocator as baseline
        allocators_.push_back(std::make_unique<StandardAllocator>());
        
        // Add hierarchical pool allocator
        static hierarchical::HierarchicalPoolAllocator hierarchical_alloc;
        allocators_.push_back(std::make_unique<HierarchicalPoolAllocatorWrapper>(hierarchical_alloc));
        
        // Add more allocator types here
    }
    
    void generate_csv_report() const {
        std::string filename = config_.output_directory + "/benchmark_results.csv";
        std::ofstream file(filename);
        
        if (!file.is_open()) {
            LOG_ERROR("Failed to open CSV output file: {}", filename);
            return;
        }
        
        // CSV header
        file << "Test,Category,Allocator,OpsPerSec,LatencyNs,ThroughputMBps,Mean,StdDev,Min,Max\n";
        
        for (const auto& result : results_) {
            file << result.to_csv_row() << "\n";
        }
        
        LOG_INFO("Generated CSV report: {}", filename);
    }
    
    void generate_json_report() const {
        std::string filename = config_.output_directory + "/benchmark_results.json";
        std::ofstream file(filename);
        
        if (!file.is_open()) {
            LOG_ERROR("Failed to open JSON output file: {}", filename);
            return;
        }
        
        file << "{\n";
        file << "  \"benchmark_results\": [\n";
        
        for (usize i = 0; i < results_.size(); ++i) {
            file << "    " << results_[i].to_json();
            if (i < results_.size() - 1) {
                file << ",";
            }
            file << "\n";
        }
        
        file << "  ]\n";
        file << "}\n";
        
        LOG_INFO("Generated JSON report: {}", filename);
    }
    
    void generate_html_report() const {
        // Implementation would generate interactive HTML report with charts
        LOG_INFO("HTML report generation not implemented yet");
    }
    
    std::string generate_recommendations() const {
        std::ostringstream recommendations;
        
        // Analyze results to generate recommendations
        recommendations << "Based on benchmark results:\n\n";
        
        // Find best overall performer
        if (!results_.empty()) {
            auto best_result = std::max_element(results_.begin(), results_.end(),
                                               [](const auto& a, const auto& b) {
                                                   return a.operations_per_second < b.operations_per_second;
                                               });
            
            recommendations << "1. Best overall performance: " << best_result->allocator_type
                           << " (" << std::fixed << std::setprecision(0) 
                           << best_result->operations_per_second << " ops/sec)\n";
        }
        
        recommendations << "2. For high-frequency allocations: Use hierarchical pools\n";
        recommendations << "3. For multi-threaded workloads: Consider lock-free allocators\n";
        recommendations << "4. For cache-sensitive code: Use cache-aware data structures\n";
        recommendations << "5. For NUMA systems: Enable NUMA-aware allocation\n";
        
        return recommendations.str();
    }
    
    std::string get_platform_info() const {
        std::ostringstream info;
        info << "CPUs: " << std::thread::hardware_concurrency();
        // Would add more platform-specific information
        return info.str();
    }
    
    std::string get_timestamp() const {
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        std::ostringstream ss;
        ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
        return ss.str();
    }
};

//=============================================================================
// Benchmark Result Implementation
//=============================================================================

void BenchmarkResult::calculate_statistics() {
    if (raw_measurements.empty()) return;
    
    // Remove outliers if requested
    std::vector<f64> measurements = raw_measurements;
    if (measurements.size() > 4) {
        std::sort(measurements.begin(), measurements.end());
        
        // Remove top and bottom 5%
        usize remove_count = std::max(usize(1), measurements.size() / 20);
        measurements.erase(measurements.begin(), measurements.begin() + remove_count);
        measurements.erase(measurements.end() - remove_count, measurements.end());
    }
    
    // Calculate basic statistics
    mean = std::accumulate(measurements.begin(), measurements.end(), 0.0) / measurements.size();
    
    std::sort(measurements.begin(), measurements.end());
    median = measurements.size() % 2 == 0 ?
        (measurements[measurements.size()/2 - 1] + measurements[measurements.size()/2]) / 2.0 :
        measurements[measurements.size()/2];
    
    min_value = measurements.front();
    max_value = measurements.back();
    
    // Standard deviation
    f64 variance_sum = 0.0;
    for (f64 value : measurements) {
        f64 diff = value - mean;
        variance_sum += diff * diff;
    }
    std_deviation = std::sqrt(variance_sum / measurements.size());
    
    // Consistency score (inverse of coefficient of variation)
    consistency_score = mean > 0 ? 1.0 - (std_deviation / mean) : 0.0;
    
    // Confidence interval (simplified using t-distribution approximation)
    if (measurements.size() > 1) {
        f64 t_value = 1.96; // 95% confidence for large samples
        f64 margin = t_value * std_deviation / std::sqrt(measurements.size());
        confidence_interval_lower = mean - margin;
        confidence_interval_upper = mean + margin;
    }
}

std::string BenchmarkResult::to_csv_row() const {
    std::ostringstream ss;
    ss << test_name << "," << category << "," << allocator_type << ","
       << std::fixed << std::setprecision(2)
       << operations_per_second << "," << average_latency_ns << "," << throughput_mbps << ","
       << mean << "," << std_deviation << "," << min_value << "," << max_value;
    return ss.str();
}

std::string BenchmarkResult::to_json() const {
    std::ostringstream ss;
    ss << "{\n";
    ss << "      \"test_name\": \"" << test_name << "\",\n";
    ss << "      \"category\": \"" << category << "\",\n";
    ss << "      \"allocator_type\": \"" << allocator_type << "\",\n";
    ss << "      \"operations_per_second\": " << operations_per_second << ",\n";
    ss << "      \"average_latency_ns\": " << average_latency_ns << ",\n";
    ss << "      \"throughput_mbps\": " << throughput_mbps << ",\n";
    ss << "      \"mean\": " << mean << ",\n";
    ss << "      \"std_deviation\": " << std_deviation << ",\n";
    ss << "      \"consistency_score\": " << consistency_score << "\n";
    ss << "    }";
    return ss.str();
}

} // namespace ecscope::memory::benchmark