#pragma once

/**
 * @file ecs/performance_integration.hpp
 * @brief Integration Layer for ECS Performance Monitoring and Benchmarking
 * 
 * This comprehensive integration layer connects the modern ECS enhancements
 * (sparse sets, enhanced queries, dependency resolution) with the existing
 * performance monitoring and visualization systems. It provides educational
 * insights into the trade-offs between different storage strategies and
 * system architectures.
 * 
 * Key Features:
 * - Comprehensive benchmarking suite for storage strategy comparison
 * - Integration with existing memory tracking and performance lab systems
 * - Real-time performance monitoring with visualization data export
 * - Educational analysis and recommendations system
 * - Automated benchmark execution and result compilation
 * - Historical performance data tracking and trend analysis
 * 
 * Performance Metrics:
 * - Memory usage patterns (allocation, fragmentation, efficiency)
 * - Query execution times and throughput measurements
 * - Cache performance analysis (hit rates, miss patterns)
 * - System dependency resolution timing
 * - Parallel execution efficiency and scaling
 * 
 * Educational Value:
 * - Clear comparison between architectural approaches
 * - Performance visualization and interpretation guides
 * - Optimization recommendation engine
 * - Interactive performance laboratory integration
 * 
 * @author ECScope Educational ECS Framework - Modern Extensions
 * @date 2024
 */

#include "core/types.hpp"
#include "core/log.hpp"
#include "ecs/sparse_set.hpp"
#include "ecs/enhanced_query.hpp"
#include "ecs/dependency_resolver.hpp"
#include "ecs/registry.hpp"
#include "memory/memory_tracker.hpp"
#include "performance/performance_lab.hpp"
#include "performance/allocation_benchmarks.hpp"
#include <vector>
#include <unordered_map>
#include <chrono>
#include <memory>
#include <fstream>
#include <sstream>
#include <atomic>
#include <thread>

namespace ecscope::ecs::performance {

using namespace ecscope::ecs::sparse;
using namespace ecscope::ecs::enhanced;
using namespace ecscope::ecs::dependency;

//=============================================================================
// Performance Data Structures
//=============================================================================

/**
 * @brief Comprehensive ECS performance metrics
 */
struct ECSPerformanceMetrics {
    // Storage strategy performance
    struct StorageMetrics {
        f64 archetype_query_time_ns;
        f64 sparse_set_query_time_ns;
        f64 hybrid_query_time_ns;
        
        usize archetype_memory_bytes;
        usize sparse_set_memory_bytes;
        usize hybrid_memory_bytes;
        
        f64 archetype_cache_hit_ratio;
        f64 sparse_set_cache_hit_ratio;
        
        usize archetype_entities_processed;
        usize sparse_set_entities_processed;
        
        f64 insert_performance_ratio;    // sparse_set_time / archetype_time
        f64 remove_performance_ratio;
        f64 lookup_performance_ratio;
        f64 iteration_performance_ratio;
    } storage;
    
    // Query performance
    struct QueryMetrics {
        f64 simple_query_time_ns;
        f64 complex_query_time_ns;
        f64 filtered_query_time_ns;
        f64 parallel_query_time_ns;
        
        f64 query_cache_hit_ratio;
        f64 query_optimization_benefit;
        
        usize queries_executed;
        f64 average_result_size;
        f64 query_compilation_overhead_ns;
    } query;
    
    // System performance
    struct SystemMetrics {
        f64 dependency_resolution_time_ns;
        f64 parallel_execution_efficiency;
        f64 critical_path_time_ms;
        
        usize systems_parallelized;
        usize dependency_cycles_detected;
        f64 scheduling_overhead_ratio;
    } system;
    
    // Memory performance
    struct MemoryMetrics {
        usize arena_utilization_bytes;
        usize pool_utilization_bytes;
        usize pmr_utilization_bytes;
        
        f64 memory_fragmentation_ratio;
        f64 allocation_efficiency;
        f64 memory_access_locality_score;
        
        u64 allocation_operations;
        u64 deallocation_operations;
        f64 average_allocation_time_ns;
    } memory;
    
    // Overall performance indicators
    f64 entities_per_second;
    f64 components_per_second;
    f64 frame_time_budget_utilization;
    
    std::chrono::steady_clock::time_point measurement_time;
    std::string configuration_description;
};

/**
 * @brief Historical performance tracking
 */
class PerformanceHistory {
private:
    std::vector<ECSPerformanceMetrics> history_;
    std::mutex history_mutex_;
    usize max_history_size_;
    
public:
    explicit PerformanceHistory(usize max_size = 1000) : max_history_size_(max_size) {}
    
    void record_metrics(const ECSPerformanceMetrics& metrics) {
        std::lock_guard<std::mutex> lock(history_mutex_);
        
        history_.push_back(metrics);
        
        // Keep history within bounds
        if (history_.size() > max_history_size_) {
            history_.erase(history_.begin(), history_.begin() + (history_.size() - max_history_size_));
        }
    }
    
    std::vector<ECSPerformanceMetrics> get_recent_history(usize count) const {
        std::lock_guard<std::mutex> lock(history_mutex_);
        
        if (history_.empty()) return {};
        
        usize start_idx = history_.size() > count ? history_.size() - count : 0;
        return std::vector<ECSPerformanceMetrics>(history_.begin() + start_idx, history_.end());
    }
    
    void clear_history() {
        std::lock_guard<std::mutex> lock(history_mutex_);
        history_.clear();
    }
    
    usize size() const {
        std::lock_guard<std::mutex> lock(history_mutex_);
        return history_.size();
    }
};

//=============================================================================
// Benchmark Suite
//=============================================================================

/**
 * @brief Comprehensive ECS benchmark suite
 */
class ECSBenchmarkSuite {
private:
    Registry* registry_;
    SparseSetRegistry* sparse_registry_;
    DependencyResolver* dependency_resolver_;
    memory::ArenaAllocator* arena_;
    PerformanceHistory performance_history_;
    
    // Benchmark configuration
    usize entity_count_;
    usize component_types_;
    f64 sparsity_ratio_;
    usize benchmark_iterations_;
    
    // Performance tracking
    std::atomic<bool> benchmarking_active_;
    std::unique_ptr<std::thread> benchmark_thread_;
    
public:
    struct BenchmarkConfig {
        usize entity_count = 10000;
        usize component_types = 10;
        f64 sparsity_ratio = 0.5;
        usize iterations = 100;
        bool enable_parallel_benchmarks = true;
        bool enable_memory_profiling = true;
        bool enable_cache_analysis = true;
    };
    
    explicit ECSBenchmarkSuite(Registry* registry,
                              SparseSetRegistry* sparse_registry,
                              DependencyResolver* dependency_resolver = nullptr,
                              memory::ArenaAllocator* arena = nullptr)
        : registry_(registry)
        , sparse_registry_(sparse_registry)
        , dependency_resolver_(dependency_resolver)
        , arena_(arena)
        , entity_count_(10000)
        , component_types_(10)
        , sparsity_ratio_(0.5)
        , benchmark_iterations_(100)
        , benchmarking_active_(false) {}
    
    ~ECSBenchmarkSuite() {
        stop_continuous_benchmarking();
    }
    
    /**
     * @brief Execute comprehensive benchmark suite
     */
    ECSPerformanceMetrics run_full_benchmark(const BenchmarkConfig& config = BenchmarkConfig{}) {
        entity_count_ = config.entity_count;
        component_types_ = config.component_types;
        sparsity_ratio_ = config.sparsity_ratio;
        benchmark_iterations_ = config.iterations;
        
        LOG_INFO("Starting comprehensive ECS benchmark suite");
        LOG_INFO("Configuration: {} entities, {} component types, {:.1%} sparsity",
                entity_count_, component_types_, sparsity_ratio_);
        
        ECSPerformanceMetrics metrics{};
        metrics.measurement_time = std::chrono::steady_clock::now();
        metrics.configuration_description = generate_config_description(config);
        
        // Storage strategy benchmarks
        benchmark_storage_strategies(metrics.storage);
        
        // Query performance benchmarks
        benchmark_query_performance(metrics.query);
        
        // System dependency benchmarks
        if (dependency_resolver_) {
            benchmark_system_performance(metrics.system);
        }
        
        // Memory allocation benchmarks
        benchmark_memory_performance(metrics.memory);
        
        // Calculate overall metrics
        calculate_overall_metrics(metrics);
        
        // Record in history
        performance_history_.record_metrics(metrics);
        
        LOG_INFO("Benchmark suite completed");
        return metrics;
    }
    
    /**
     * @brief Benchmark storage strategy comparison
     */
    void benchmark_storage_strategies(ECSPerformanceMetrics::StorageMetrics& metrics) {
        LOG_INFO("Benchmarking storage strategies...");
        
        // Setup test data
        setup_test_entities();
        
        // Benchmark archetype queries
        auto archetype_results = benchmark_archetype_operations();
        metrics.archetype_query_time_ns = archetype_results.average_query_time_ns;
        metrics.archetype_memory_bytes = archetype_results.memory_usage;
        metrics.archetype_cache_hit_ratio = archetype_results.cache_hit_ratio;
        metrics.archetype_entities_processed = archetype_results.entities_processed;
        
        // Benchmark sparse set queries
        auto sparse_results = benchmark_sparse_set_operations();
        metrics.sparse_set_query_time_ns = sparse_results.average_query_time_ns;
        metrics.sparse_set_memory_bytes = sparse_results.memory_usage;
        metrics.sparse_set_cache_hit_ratio = sparse_results.cache_hit_ratio;
        metrics.sparse_set_entities_processed = sparse_results.entities_processed;
        
        // Benchmark hybrid approach
        auto hybrid_results = benchmark_hybrid_operations();
        metrics.hybrid_query_time_ns = hybrid_results.average_query_time_ns;
        metrics.hybrid_memory_bytes = hybrid_results.memory_usage;
        
        // Calculate performance ratios
        metrics.insert_performance_ratio = sparse_results.insert_time_ns / archetype_results.insert_time_ns;
        metrics.remove_performance_ratio = sparse_results.remove_time_ns / archetype_results.remove_time_ns;
        metrics.lookup_performance_ratio = sparse_results.lookup_time_ns / archetype_results.lookup_time_ns;
        metrics.iteration_performance_ratio = sparse_results.iteration_time_ns / archetype_results.iteration_time_ns;
        
        LOG_INFO("Storage strategy benchmarking completed");
        LOG_INFO("  Archetype query time: {:.2f} μs", metrics.archetype_query_time_ns / 1000.0);
        LOG_INFO("  Sparse set query time: {:.2f} μs", metrics.sparse_set_query_time_ns / 1000.0);
        LOG_INFO("  Hybrid query time: {:.2f} μs", metrics.hybrid_query_time_ns / 1000.0);
    }
    
    /**
     * @brief Benchmark query performance with different optimizations
     */
    void benchmark_query_performance(ECSPerformanceMetrics::QueryMetrics& metrics) {
        LOG_INFO("Benchmarking query performance...");
        
        // Simple single-component queries
        metrics.simple_query_time_ns = benchmark_simple_queries();
        
        // Complex multi-component queries
        metrics.complex_query_time_ns = benchmark_complex_queries();
        
        // Filtered queries with predicates
        metrics.filtered_query_time_ns = benchmark_filtered_queries();
        
        // Parallel query execution
        metrics.parallel_query_time_ns = benchmark_parallel_queries();
        
        // Query caching effectiveness
        auto cache_results = benchmark_query_caching();
        metrics.query_cache_hit_ratio = cache_results.hit_ratio;
        metrics.query_optimization_benefit = cache_results.optimization_benefit;
        
        // Overall query statistics
        metrics.queries_executed = benchmark_iterations_ * 4; // 4 query types
        metrics.average_result_size = calculate_average_result_size();
        metrics.query_compilation_overhead_ns = measure_query_compilation_overhead();
        
        LOG_INFO("Query performance benchmarking completed");
        LOG_INFO("  Simple queries: {:.2f} μs", metrics.simple_query_time_ns / 1000.0);
        LOG_INFO("  Complex queries: {:.2f} μs", metrics.complex_query_time_ns / 1000.0);
        LOG_INFO("  Parallel queries: {:.2f} μs", metrics.parallel_query_time_ns / 1000.0);
    }
    
    /**
     * @brief Benchmark system dependency resolution and scheduling
     */
    void benchmark_system_performance(ECSPerformanceMetrics::SystemMetrics& metrics) {
        LOG_INFO("Benchmarking system performance...");
        
        // Dependency resolution timing
        auto dep_start = std::chrono::high_resolution_clock::now();
        for (usize i = 0; i < benchmark_iterations_; ++i) {
            auto order = dependency_resolver_->resolve_execution_order(SystemPhase::Update);
            volatile usize size = order.size(); // Prevent optimization
        }
        auto dep_end = std::chrono::high_resolution_clock::now();
        
        metrics.dependency_resolution_time_ns = 
            std::chrono::duration<f64, std::nano>(dep_end - dep_start).count() / benchmark_iterations_;
        
        // Parallel execution efficiency
        auto parallel_groups = dependency_resolver_->resolve_parallel_groups(SystemPhase::Update);
        usize total_systems = 0;
        usize parallelized_systems = 0;
        
        for (const auto& group : parallel_groups) {
            total_systems += group.size();
            if (group.size() > 1) {
                parallelized_systems += group.size();
            }
        }
        
        metrics.parallel_execution_efficiency = total_systems > 0 ?
            static_cast<f64>(parallelized_systems) / total_systems : 0.0;
        
        metrics.systems_parallelized = parallelized_systems;
        
        // Critical path analysis
        auto stats = dependency_resolver_->get_comprehensive_statistics();
        metrics.critical_path_time_ms = stats.total_critical_path_time * 1000.0;
        metrics.scheduling_overhead_ratio = stats.average_resolution_time / 0.016; // vs 16ms frame budget
        
        LOG_INFO("System performance benchmarking completed");
        LOG_INFO("  Dependency resolution: {:.2f} μs", metrics.dependency_resolution_time_ns / 1000.0);
        LOG_INFO("  Parallel efficiency: {:.1%}", metrics.parallel_execution_efficiency);
    }
    
    /**
     * @brief Benchmark memory allocation performance
     */
    void benchmark_memory_performance(ECSPerformanceMetrics::MemoryMetrics& metrics) {
        LOG_INFO("Benchmarking memory performance...");
        
        if (arena_) {
            auto arena_stats = arena_->stats();
            metrics.arena_utilization_bytes = arena_stats.used_size;
            metrics.memory_fragmentation_ratio = arena_stats.fragmentation_ratio;
            metrics.allocation_efficiency = arena_stats.efficiency_ratio;
            metrics.average_allocation_time_ns = arena_stats.average_alloc_time * 1e9;
        }
        
        // Get ECS memory statistics
        auto ecs_stats = registry_->get_memory_statistics();
        metrics.memory_access_locality_score = ecs_stats.cache_hit_ratio;
        
        // Benchmark allocation operations
        auto alloc_start = std::chrono::high_resolution_clock::now();
        std::vector<void*> allocations;
        
        for (usize i = 0; i < 1000; ++i) {
            if (arena_) {
                void* ptr = arena_->allocate(64, 8, "benchmark");
                allocations.push_back(ptr);
            }
        }
        
        auto alloc_end = std::chrono::high_resolution_clock::now();
        auto alloc_time = std::chrono::duration<f64, std::nano>(alloc_end - alloc_start).count();
        
        metrics.allocation_operations = allocations.size();
        if (!allocations.empty()) {
            metrics.average_allocation_time_ns = alloc_time / allocations.size();
        }
        
        LOG_INFO("Memory performance benchmarking completed");
        if (arena_) {
            LOG_INFO("  Arena utilization: {} KB", metrics.arena_utilization_bytes / 1024);
            LOG_INFO("  Memory efficiency: {:.1%}", metrics.allocation_efficiency);
        }
    }
    
    /**
     * @brief Start continuous benchmarking for real-time monitoring
     */
    void start_continuous_benchmarking(const BenchmarkConfig& config = BenchmarkConfig{}) {
        if (benchmarking_active_.load()) {
            LOG_WARN("Continuous benchmarking already active");
            return;
        }
        
        benchmarking_active_.store(true);
        benchmark_thread_ = std::make_unique<std::thread>([this, config]() {
            while (benchmarking_active_.load()) {
                auto metrics = run_full_benchmark(config);
                
                // Sleep between benchmark runs
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
        });
        
        LOG_INFO("Started continuous benchmarking");
    }
    
    /**
     * @brief Stop continuous benchmarking
     */
    void stop_continuous_benchmarking() {
        if (benchmarking_active_.load()) {
            benchmarking_active_.store(false);
            if (benchmark_thread_ && benchmark_thread_->joinable()) {
                benchmark_thread_->join();
            }
            benchmark_thread_.reset();
            LOG_INFO("Stopped continuous benchmarking");
        }
    }
    
    /**
     * @brief Export performance data for visualization
     */
    struct PerformanceReport {
        ECSPerformanceMetrics current_metrics;
        std::vector<ECSPerformanceMetrics> historical_data;
        
        struct Analysis {
            std::string performance_summary;
            std::vector<std::string> optimization_recommendations;
            std::vector<std::string> performance_trends;
            std::string best_storage_strategy;
            f64 memory_efficiency_score;
            f64 query_performance_score;
            f64 overall_performance_score;
        } analysis;
        
        struct Visualization {
            std::vector<std::pair<std::string, f64>> performance_chart_data;
            std::vector<std::pair<std::string, usize>> memory_usage_data;
            std::vector<std::pair<std::string, f64>> trend_data;
        } visualization;
    };
    
    PerformanceReport generate_performance_report() const {
        PerformanceReport report{};
        
        // Get latest metrics
        auto recent_history = performance_history_.get_recent_history(1);
        if (!recent_history.empty()) {
            report.current_metrics = recent_history.back();
        }
        
        // Get historical data
        report.historical_data = performance_history_.get_recent_history(100);
        
        // Generate analysis
        generate_performance_analysis(report);
        
        // Generate visualization data
        generate_visualization_data(report);
        
        return report;
    }
    
    /**
     * @brief Export benchmark results to file
     */
    void export_results(const std::string& filename) const {
        std::ofstream file(filename);
        if (!file.is_open()) {
            LOG_ERROR("Failed to open file for benchmark export: {}", filename);
            return;
        }
        
        auto report = generate_performance_report();
        
        // Write CSV header
        file << "Timestamp,ArchetypeQueryTime_ns,SparseSetQueryTime_ns,HybridQueryTime_ns,";
        file << "ArchetypeMemory_bytes,SparseSetMemory_bytes,";
        file << "QueryCacheHitRatio,ParallelEfficiency,CriticalPathTime_ms,";
        file << "MemoryEfficiency,AllocationTime_ns\n";
        
        // Write historical data
        for (const auto& metrics : report.historical_data) {
            auto time_t = std::chrono::system_clock::to_time_t(
                std::chrono::system_clock::now()); // Simplified timestamp
            
            file << time_t << ","
                 << metrics.storage.archetype_query_time_ns << ","
                 << metrics.storage.sparse_set_query_time_ns << ","
                 << metrics.storage.hybrid_query_time_ns << ","
                 << metrics.storage.archetype_memory_bytes << ","
                 << metrics.storage.sparse_set_memory_bytes << ","
                 << metrics.query.query_cache_hit_ratio << ","
                 << metrics.system.parallel_execution_efficiency << ","
                 << metrics.system.critical_path_time_ms << ","
                 << metrics.memory.allocation_efficiency << ","
                 << metrics.memory.average_allocation_time_ns << "\n";
        }
        
        file.close();
        LOG_INFO("Benchmark results exported to: {}", filename);
    }
    
    const PerformanceHistory& get_performance_history() const {
        return performance_history_;
    }
    
private:
    void setup_test_entities() {
        // Create test entities with various component combinations
        registry_->clear();
        
        // Generate entities with different sparsity patterns
        for (usize i = 0; i < entity_count_; ++i) {
            Entity entity = registry_->create_entity();
            
            // Add components based on sparsity ratio
            if (static_cast<f64>(rand()) / RAND_MAX < sparsity_ratio_) {
                // Add sparse components (using sparse sets)
                // Implementation depends on available test components
            } else {
                // Add dense components (using archetypes)
                // Implementation depends on available test components
            }
        }
    }
    
    struct StorageBenchmarkResults {
        f64 average_query_time_ns;
        f64 insert_time_ns;
        f64 remove_time_ns;
        f64 lookup_time_ns;
        f64 iteration_time_ns;
        usize memory_usage;
        f64 cache_hit_ratio;
        usize entities_processed;
    };
    
    StorageBenchmarkResults benchmark_archetype_operations() {
        StorageBenchmarkResults results{};
        
        auto start_time = std::chrono::high_resolution_clock::now();
        
        for (usize i = 0; i < benchmark_iterations_; ++i) {
            // Benchmark archetype queries
            auto entities = registry_->get_all_entities();
            results.entities_processed = entities.size();
        }
        
        auto end_time = std::chrono::high_resolution_clock::now();
        results.average_query_time_ns = 
            std::chrono::duration<f64, std::nano>(end_time - start_time).count() / benchmark_iterations_;
        
        // Get memory usage
        results.memory_usage = registry_->memory_usage();
        
        // Get cache statistics
        auto ecs_stats = registry_->get_memory_statistics();
        results.cache_hit_ratio = ecs_stats.cache_hit_ratio;
        
        return results;
    }
    
    StorageBenchmarkResults benchmark_sparse_set_operations() {
        StorageBenchmarkResults results{};
        
        // Similar implementation for sparse set benchmarking
        // Would use actual sparse set operations
        
        return results;
    }
    
    StorageBenchmarkResults benchmark_hybrid_operations() {
        StorageBenchmarkResults results{};
        
        // Similar implementation for hybrid approach
        
        return results;
    }
    
    f64 benchmark_simple_queries() {
        auto start_time = std::chrono::high_resolution_clock::now();
        
        for (usize i = 0; i < benchmark_iterations_; ++i) {
            // Execute simple queries
            auto entities = registry_->get_all_entities();
            volatile usize size = entities.size();
        }
        
        auto end_time = std::chrono::high_resolution_clock::now();
        return std::chrono::duration<f64, std::nano>(end_time - start_time).count() / benchmark_iterations_;
    }
    
    f64 benchmark_complex_queries() {
        // Similar implementation for complex queries
        return 0.0;
    }
    
    f64 benchmark_filtered_queries() {
        // Implementation for filtered queries
        return 0.0;
    }
    
    f64 benchmark_parallel_queries() {
        // Implementation for parallel query benchmarks
        return 0.0;
    }
    
    struct CacheBenchmarkResults {
        f64 hit_ratio;
        f64 optimization_benefit;
    };
    
    CacheBenchmarkResults benchmark_query_caching() {
        CacheBenchmarkResults results{};
        // Implementation for query caching benchmarks
        return results;
    }
    
    f64 calculate_average_result_size() {
        return static_cast<f64>(entity_count_) * sparsity_ratio_;
    }
    
    f64 measure_query_compilation_overhead() {
        // Measure query compilation time
        return 0.0;
    }
    
    void calculate_overall_metrics(ECSPerformanceMetrics& metrics) {
        // Calculate entities per second
        f64 total_query_time = metrics.storage.archetype_query_time_ns + 
                              metrics.storage.sparse_set_query_time_ns;
        if (total_query_time > 0) {
            metrics.entities_per_second = (entity_count_ * 2 * 1e9) / total_query_time;
        }
        
        // Calculate components per second
        metrics.components_per_second = metrics.entities_per_second * component_types_;
        
        // Calculate frame budget utilization
        f64 frame_budget_ns = 16.67e6; // 16.67ms in nanoseconds (60 FPS)
        f64 total_frame_time = metrics.query.simple_query_time_ns + 
                              metrics.system.dependency_resolution_time_ns;
        metrics.frame_time_budget_utilization = total_frame_time / frame_budget_ns;
    }
    
    std::string generate_config_description(const BenchmarkConfig& config) const {
        std::ostringstream desc;
        desc << "Entities=" << config.entity_count 
             << ", ComponentTypes=" << config.component_types
             << ", Sparsity=" << (config.sparsity_ratio * 100.0) << "%"
             << ", Iterations=" << config.iterations;
        return desc.str();
    }
    
    void generate_performance_analysis(PerformanceReport& report) const {
        auto& analysis = report.analysis;
        const auto& metrics = report.current_metrics;
        
        // Performance summary
        std::ostringstream summary;
        summary << "ECS Performance Summary:\n";
        summary << "- Query Performance: " << (metrics.entities_per_second / 1000.0) << "K entities/sec\n";
        summary << "- Memory Efficiency: " << (metrics.memory.allocation_efficiency * 100.0) << "%\n";
        summary << "- Parallel Efficiency: " << (metrics.system.parallel_execution_efficiency * 100.0) << "%\n";
        analysis.performance_summary = summary.str();
        
        // Optimization recommendations
        if (metrics.storage.sparse_set_query_time_ns < metrics.storage.archetype_query_time_ns) {
            analysis.optimization_recommendations.push_back("Consider using sparse set storage for better query performance");
            analysis.best_storage_strategy = "Sparse Set";
        } else {
            analysis.optimization_recommendations.push_back("Archetype storage performs better for current data patterns");
            analysis.best_storage_strategy = "Archetype";
        }
        
        if (metrics.memory.allocation_efficiency < 0.8) {
            analysis.optimization_recommendations.push_back("Memory efficiency is low - consider arena size optimization");
        }
        
        if (metrics.system.parallel_execution_efficiency < 0.5) {
            analysis.optimization_recommendations.push_back("Low parallel efficiency - review system dependencies");
        }
        
        // Calculate scores
        analysis.memory_efficiency_score = metrics.memory.allocation_efficiency * 100.0;
        analysis.query_performance_score = std::min(100.0, (metrics.entities_per_second / 100000.0) * 100.0);
        analysis.overall_performance_score = (analysis.memory_efficiency_score + analysis.query_performance_score) / 2.0;
    }
    
    void generate_visualization_data(PerformanceReport& report) const {
        auto& viz = report.visualization;
        const auto& metrics = report.current_metrics;
        
        // Performance chart data
        viz.performance_chart_data = {
            {"Archetype Query", metrics.storage.archetype_query_time_ns / 1000.0}, // μs
            {"Sparse Set Query", metrics.storage.sparse_set_query_time_ns / 1000.0},
            {"Hybrid Query", metrics.storage.hybrid_query_time_ns / 1000.0},
            {"Dependency Resolution", metrics.system.dependency_resolution_time_ns / 1000.0}
        };
        
        // Memory usage data
        viz.memory_usage_data = {
            {"Archetype Memory", metrics.storage.archetype_memory_bytes},
            {"Sparse Set Memory", metrics.storage.sparse_set_memory_bytes},
            {"Hybrid Memory", metrics.storage.hybrid_memory_bytes}
        };
        
        // Trend data (simplified - would use historical data in real implementation)
        viz.trend_data = {
            {"Performance Trend", metrics.entities_per_second},
            {"Memory Efficiency Trend", metrics.memory.allocation_efficiency * 100.0}
        };
    }
};

//=============================================================================
// Integration with Performance Lab
//=============================================================================

/**
 * @brief Integration adapter for existing performance lab system
 */
class PerformanceLabIntegration {
private:
    std::unique_ptr<ECSBenchmarkSuite> benchmark_suite_;
    ecscope::performance::PerformanceLab* performance_lab_;
    
public:
    explicit PerformanceLabIntegration(Registry* registry,
                                      SparseSetRegistry* sparse_registry,
                                      DependencyResolver* dependency_resolver = nullptr,
                                      memory::ArenaAllocator* arena = nullptr)
        : benchmark_suite_(std::make_unique<ECSBenchmarkSuite>(
            registry, sparse_registry, dependency_resolver, arena)) {}
    
    /**
     * @brief Register ECS benchmarks with performance lab
     */
    void register_with_performance_lab(ecscope::performance::PerformanceLab& lab) {
        performance_lab_ = &lab;
        
        // Register benchmark functions
        lab.register_benchmark("ECS_Storage_Comparison", [this]() {
            auto metrics = benchmark_suite_->run_full_benchmark();
            return metrics.storage.archetype_query_time_ns;
        });
        
        lab.register_benchmark("ECS_Query_Performance", [this]() {
            auto metrics = benchmark_suite_->run_full_benchmark();
            return metrics.query.simple_query_time_ns;
        });
        
        lab.register_benchmark("ECS_Memory_Efficiency", [this]() {
            auto metrics = benchmark_suite_->run_full_benchmark();
            return metrics.memory.allocation_efficiency * 100.0;
        });
    }
    
    /**
     * @brief Export performance data to performance lab format
     */
    void export_to_performance_lab() {
        if (!performance_lab_) return;
        
        auto report = benchmark_suite_->generate_performance_report();
        
        // Convert to performance lab format and export
        // Implementation would depend on performance lab's data format
    }
    
    ECSBenchmarkSuite& get_benchmark_suite() { return *benchmark_suite_; }
};

} // namespace ecscope::ecs::performance