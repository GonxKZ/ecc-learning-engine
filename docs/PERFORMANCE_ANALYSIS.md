# ECScope Performance Analysis

**Comprehensive guide to performance measurement, optimization techniques, and educational analysis tools**

## Table of Contents

1. [Performance Analysis Philosophy](#performance-analysis-philosophy)
2. [Built-in Performance Tools](#built-in-performance-tools)
3. [Benchmarking Framework](#benchmarking-framework)
4. [Memory Performance Analysis](#memory-performance-analysis)
5. [ECS Performance Optimization](#ecs-performance-optimization)
6. [Physics Performance Analysis](#physics-performance-analysis)
7. [Rendering Performance Analysis](#rendering-performance-analysis)
8. [Educational Performance Labs](#educational-performance-labs)
9. [Advanced Profiling Techniques](#advanced-profiling-techniques)

## Performance Analysis Philosophy

### Why Performance Analysis Matters

ECScope integrates performance analysis as a core educational feature because:

**Educational Value**:
- **Make Performance Visible**: Transform abstract performance concepts into observable metrics
- **Understand Trade-offs**: See the relationship between design decisions and performance
- **Learn Optimization**: Provide hands-on experience with real-world optimization techniques
- **Develop Intuition**: Build performance intuition through repeated measurement and analysis

**Practical Benefits**:
- **Identify Bottlenecks**: Find actual performance problems, not perceived ones
- **Validate Optimizations**: Measure the real impact of optimization efforts
- **Understand Scaling**: See how performance characteristics change with scale
- **Compare Strategies**: Evaluate different implementation approaches objectively

### Performance Measurement Principles

**Scientific Measurement**:
1. **Establish Baseline**: Always measure current performance before optimizing
2. **Control Variables**: Change one thing at a time to understand impact
3. **Measure Multiple Runs**: Account for variation and system noise
4. **Use Appropriate Metrics**: Choose metrics that matter for your use case
5. **Understand Context**: Performance is always relative to requirements and constraints

**Educational Measurement**:
1. **Explain Methodology**: Document why and how measurements are taken
2. **Show Comparative Results**: Compare before/after and different approaches
3. **Provide Context**: Explain what the numbers mean in practical terms
4. **Enable Experimentation**: Allow students to modify parameters and see results
5. **Generate Insights**: Translate raw numbers into actionable understanding

## Built-in Performance Tools

### ScopeTimer: Automatic Performance Measurement

```cpp
/**
 * @brief Educational Scope Timer for Automatic Performance Measurement
 * 
 * Provides zero-overhead performance timing when not in educational mode,
 * and comprehensive analysis when educational features are enabled.
 */
class ScopeTimer {
private:
    std::string operation_name_;
    std::chrono::high_resolution_clock::time_point start_time_;
    bool educational_mode_;
    
public:
    explicit ScopeTimer(const std::string& operation_name, bool educational = true)
        : operation_name_(operation_name)
        , start_time_(std::chrono::high_resolution_clock::now())
        , educational_mode_(educational) {
        
        if (educational_mode_) {
            LOG_DEBUG("Performance timing started: {}", operation_name_);
        }
    }
    
    ~ScopeTimer() {
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration<f64>(end_time - start_time_);
        
        f64 milliseconds = duration.count() * 1000.0;
        f64 microseconds = duration.count() * 1000000.0;
        
        if (educational_mode_) {
            // Educational logging with context
            if (milliseconds >= 1.0) {
                LOG_INFO("Performance: {} completed in {:.3f} ms", operation_name_, milliseconds);
            } else {
                LOG_INFO("Performance: {} completed in {:.1f} μs", operation_name_, microseconds);
            }
            
            // Provide performance context
            provide_performance_context(operation_name_, milliseconds);
        }
        
        // Record in global performance database
        performance::record_timing(operation_name_, duration.count());
    }
    
private:
    void provide_performance_context(const std::string& operation, f64 time_ms) {
        // Educational: Provide context for timing results
        if (operation == "Entity_Creation" && time_ms > 0.001) {
            f64 entities_per_ms = 1.0 / time_ms;
            LOG_INFO("  → {:.0f} entities can be created per millisecond", entities_per_ms);
            LOG_INFO("  → {:.0f} entities per frame at 60 FPS", entities_per_ms * 16.67);
        }
        
        if (operation == "Component_Access" && time_ms > 0.001) {
            f64 accesses_per_ms = 1.0 / time_ms;
            LOG_INFO("  → {:.0f} component accesses per millisecond", accesses_per_ms);
            
            if (accesses_per_ms < 1000) {
                LOG_WARN("  → Component access rate is low, consider cache optimization");
            }
        }
        
        if (operation.find("Physics") != std::string::npos && time_ms > 16.67) {
            LOG_WARN("  → Physics update taking longer than 16.67ms (60 FPS budget)");
            LOG_INFO("  → Consider reducing entity count or optimizing algorithms");
        }
    }
};

// Convenient macros for educational timing
#define ECSCOPE_TIME_SCOPE(name) ScopeTimer _timer(name)
#define ECSCOPE_TIME_SCOPE_EDUCATIONAL(name) ScopeTimer _timer(name, true)

// Usage examples
void timing_examples() {
    {
        ECSCOPE_TIME_SCOPE("Entity Creation");
        // Entity creation code here - automatically timed
    }
    
    {
        ECSCOPE_TIME_SCOPE_EDUCATIONAL("Cache Analysis");
        // Code with educational timing and analysis
    }
}
```

### Performance Database and Analysis

```cpp
/**
 * @brief Global Performance Database for Educational Analysis
 * 
 * Collects performance data across all systems and provides educational
 * analysis, optimization recommendations, and comparative insights.
 */
namespace performance {

struct PerformanceMeasurement {
    std::string operation_name;
    f64 execution_time;
    f64 timestamp;
    std::string context;
    
    // Additional metrics
    usize entities_processed{0};
    usize memory_allocated{0};
    f64 cache_hit_ratio{0.0};
};

class PerformanceDatabase {
private:
    std::vector<PerformanceMeasurement> measurements_;
    std::unordered_map<std::string, std::vector<f64>> operation_timings_;
    std::mutex data_mutex_; // Thread safety for measurements
    
public:
    void record_measurement(const PerformanceMeasurement& measurement) {
        std::lock_guard<std::mutex> lock(data_mutex_);
        
        measurements_.push_back(measurement);
        operation_timings_[measurement.operation_name].push_back(measurement.execution_time);
        
        // Limit data size for memory efficiency
        if (measurements_.size() > 100000) {
            measurements_.erase(measurements_.begin(), measurements_.begin() + 50000);
        }
        
        // Limit per-operation data
        auto& timings = operation_timings_[measurement.operation_name];
        if (timings.size() > 1000) {
            timings.erase(timings.begin(), timings.begin() + 500);
        }
    }
    
    // Educational: Performance statistics for an operation
    struct OperationStats {
        std::string operation_name;
        usize measurement_count;
        f64 min_time;
        f64 max_time;
        f64 average_time;
        f64 median_time;
        f64 std_deviation;
        f64 percentile_95;
        f64 percentile_99;
        
        // Educational insights
        std::vector<std::string> performance_insights;
        std::vector<std::string> optimization_suggestions;
    };
    
    OperationStats get_operation_stats(const std::string& operation_name) const {
        std::lock_guard<std::mutex> lock(data_mutex_);
        
        auto it = operation_timings_.find(operation_name);
        if (it == operation_timings_.end() || it->second.empty()) {
            return OperationStats{.operation_name = operation_name, .measurement_count = 0};
        }
        
        const auto& timings = it->second;
        auto sorted_timings = timings;
        std::sort(sorted_timings.begin(), sorted_timings.end());
        
        OperationStats stats;
        stats.operation_name = operation_name;
        stats.measurement_count = timings.size();
        stats.min_time = sorted_timings.front();
        stats.max_time = sorted_timings.back();
        
        // Calculate average
        f64 sum = std::accumulate(timings.begin(), timings.end(), 0.0);
        stats.average_time = sum / timings.size();
        
        // Calculate median
        stats.median_time = sorted_timings[sorted_timings.size() / 2];
        
        // Calculate standard deviation
        f64 variance_sum = 0.0;
        for (f64 time : timings) {
            f64 diff = time - stats.average_time;
            variance_sum += diff * diff;
        }
        stats.std_deviation = std::sqrt(variance_sum / timings.size());
        
        // Calculate percentiles
        stats.percentile_95 = sorted_timings[static_cast<usize>(sorted_timings.size() * 0.95)];
        stats.percentile_99 = sorted_timings[static_cast<usize>(sorted_timings.size() * 0.99)];
        
        // Generate educational insights
        generate_performance_insights(stats);
        
        return stats;
    }
    
    // Educational: Performance comparison between operations
    void compare_operations(const std::vector<std::string>& operations) {
        std::cout << "=== Operation Performance Comparison ===\n";
        
        std::vector<OperationStats> all_stats;
        for (const auto& op : operations) {
            all_stats.push_back(get_operation_stats(op));
        }
        
        // Sort by average time for comparison
        std::sort(all_stats.begin(), all_stats.end(),
            [](const auto& a, const auto& b) { return a.average_time < b.average_time; });
        
        std::cout << std::setw(25) << "Operation" 
                  << std::setw(12) << "Avg Time"
                  << std::setw(12) << "Min Time"
                  << std::setw(12) << "Max Time"
                  << std::setw(10) << "Samples" << "\n";
        std::cout << std::string(71, '-') << "\n";
        
        for (const auto& stats : all_stats) {
            std::cout << std::setw(25) << stats.operation_name
                      << std::setw(12) << std::fixed << std::setprecision(3) << stats.average_time
                      << std::setw(12) << stats.min_time
                      << std::setw(12) << stats.max_time
                      << std::setw(10) << stats.measurement_count << "\n";
        }
        
        // Educational insights
        if (all_stats.size() >= 2) {
            f64 ratio = all_stats.back().average_time / all_stats.front().average_time;
            std::cout << "\nPerformance insights:\n";
            std::cout << "  Fastest operation: " << all_stats.front().operation_name << "\n";
            std::cout << "  Slowest operation: " << all_stats.back().operation_name << "\n";
            std::cout << "  Performance range: " << ratio << "x difference\n";
        }
    }
    
private:
    void generate_performance_insights(OperationStats& stats) const {
        // Generate educational insights based on performance characteristics
        
        if (stats.std_deviation > stats.average_time * 0.5) {
            stats.performance_insights.push_back("High variance - performance is inconsistent");
            stats.optimization_suggestions.push_back("Investigate performance spikes and system interference");
        }
        
        if (stats.percentile_99 > stats.average_time * 3.0) {
            stats.performance_insights.push_back("Significant outliers detected");
            stats.optimization_suggestions.push_back("Consider worst-case performance optimization");
        }
        
        if (stats.average_time > 16.67 && stats.operation_name.find("Update") != std::string::npos) {
            stats.performance_insights.push_back("Update taking longer than 60 FPS budget");
            stats.optimization_suggestions.push_back("Optimize critical path or reduce entity count");
        }
        
        if (stats.measurement_count < 10) {
            stats.performance_insights.push_back("Limited sample size - results may not be representative");
            stats.optimization_suggestions.push_back("Collect more measurements for reliable analysis");
        }
    }
};

// Global performance database
PerformanceDatabase& get_performance_database();

// Convenience functions for recording performance data
void record_timing(const std::string& operation, f64 time);
void record_entity_operation(const std::string& operation, f64 time, usize entity_count);
void record_memory_operation(const std::string& operation, f64 time, usize memory_size);

} // namespace performance
```

## Benchmarking Framework

### Comprehensive Benchmark Suite

```cpp
/**
 * @brief Educational Benchmark Suite for ECScope Systems
 * 
 * Provides standardized benchmarks for all major ECScope systems with
 * educational analysis and optimization recommendations.
 */
class ECScope_BenchmarkSuite {
public:
    struct BenchmarkConfiguration {
        // Entity scale testing
        std::vector<usize> entity_counts{100, 1000, 10000, 50000};
        
        // System configurations to test
        std::vector<ecs::AllocatorConfig> allocator_configs{
            ecs::AllocatorConfig::create_educational_focused(),
            ecs::AllocatorConfig::create_performance_optimized(),
            ecs::AllocatorConfig::create_memory_conservative()
        };
        
        // Benchmark parameters
        usize iterations_per_test{100};
        bool enable_detailed_analysis{true};
        bool enable_cache_simulation{true};
        bool generate_optimization_report{true};
    };
    
    struct BenchmarkResults {
        std::string benchmark_name;
        std::vector<ScalabilityResult> scalability_results;
        std::vector<AllocatorComparisonResult> allocator_comparisons;
        std::vector<SystemPerformanceResult> system_performance;
        PerformanceInsights insights;
        OptimizationRecommendations recommendations;
    };
    
    // Run complete benchmark suite
    BenchmarkResults run_complete_benchmark(const BenchmarkConfiguration& config = {}) {
        BenchmarkResults results;
        results.benchmark_name = "ECScope_Complete_Performance_Analysis";
        
        LOG_INFO("Starting comprehensive ECScope benchmark suite...");
        
        // Run scalability analysis
        results.scalability_results = run_scalability_benchmarks(config);
        
        // Run allocator comparison
        results.allocator_comparisons = run_allocator_benchmarks(config);
        
        // Run system-specific benchmarks
        results.system_performance = run_system_benchmarks(config);
        
        // Generate insights and recommendations
        results.insights = generate_performance_insights(results);
        results.recommendations = generate_optimization_recommendations(results);
        
        LOG_INFO("Benchmark suite completed successfully");
        
        return results;
    }
    
private:
    // Scalability benchmark: How performance changes with entity count
    std::vector<ScalabilityResult> run_scalability_benchmarks(const BenchmarkConfiguration& config) {
        std::vector<ScalabilityResult> results;
        
        for (usize entity_count : config.entity_counts) {
            LOG_INFO("Running scalability test with {} entities", entity_count);
            
            ScalabilityResult result;
            result.entity_count = entity_count;
            
            // Test with educational configuration
            auto registry = std::make_unique<ecs::Registry>(
                ecs::AllocatorConfig::create_educational_focused(),
                "Scalability_Test_" + std::to_string(entity_count)
            );
            
            // Benchmark entity creation
            {
                ScopeTimer timer("Entity_Creation_" + std::to_string(entity_count));
                
                for (usize i = 0; i < entity_count; ++i) {
                    registry->create_entity(
                        Transform{Vec2{static_cast<f32>(i), static_cast<f32>(i)}},
                        Velocity{Vec2{1.0f, 0.0f}}
                    );
                }
                
                result.entity_creation_time = timer.elapsed_ms();
            }
            
            // Benchmark component iteration
            {
                ScopeTimer timer("Component_Iteration_" + std::to_string(entity_count));
                
                for (usize iteration = 0; iteration < config.iterations_per_test; ++iteration) {
                    registry->for_each<Transform, Velocity>([](Entity e, Transform& t, Velocity& v) {
                        t.position += v.velocity * 0.016f; // Simulate 60 FPS update
                    });
                }
                
                result.component_iteration_time = timer.elapsed_ms() / config.iterations_per_test;
            }
            
            // Benchmark component access
            {
                ScopeTimer timer("Component_Access_" + std::to_string(entity_count));
                
                auto entities = registry->get_entities_with<Transform>();
                for (Entity entity : entities) {
                    auto* transform = registry->get_component<Transform>(entity);
                    if (transform) {
                        transform->position.x += 1.0f;
                    }
                }
                
                result.component_access_time = timer.elapsed_ms();
            }
            
            // Collect memory statistics
            auto memory_stats = registry->get_memory_statistics();
            result.memory_usage = registry->memory_usage();
            result.memory_efficiency = memory_stats.memory_efficiency;
            result.cache_hit_ratio = memory_stats.cache_hit_ratio;
            
            results.push_back(result);
        }
        
        return results;
    }
    
    // Educational: Generate performance insights from benchmark data
    PerformanceInsights generate_performance_insights(const BenchmarkResults& results) {
        PerformanceInsights insights;
        
        // Analyze scalability trends
        if (results.scalability_results.size() >= 2) {
            auto& first = results.scalability_results.front();
            auto& last = results.scalability_results.back();
            
            f64 entity_scale_factor = static_cast<f64>(last.entity_count) / first.entity_count;
            f64 time_scale_factor = last.entity_creation_time / first.entity_creation_time;
            
            if (time_scale_factor <= entity_scale_factor * 1.2) {
                insights.scalability_rating = "Excellent";
                insights.scalability_notes.push_back("Linear or better than linear scaling achieved");
            } else if (time_scale_factor <= entity_scale_factor * 2.0) {
                insights.scalability_rating = "Good";
                insights.scalability_notes.push_back("Near-linear scaling with reasonable overhead");
            } else {
                insights.scalability_rating = "Poor";
                insights.scalability_notes.push_back("Worse than linear scaling - optimization needed");
            }
        }
        
        // Analyze allocator performance
        if (!results.allocator_comparisons.empty()) {
            auto best_allocator = std::min_element(results.allocator_comparisons.begin(),
                results.allocator_comparisons.end(),
                [](const auto& a, const auto& b) { return a.average_time < b.average_time; });
            
            insights.best_allocator = best_allocator->allocator_name;
            insights.performance_improvement = best_allocator->improvement_factor;
        }
        
        return insights;
    }
};

// Global performance database instance
PerformanceDatabase& get_performance_database();

// Educational benchmark runner
void run_educational_benchmarks() {
    ECScope_BenchmarkSuite suite;
    auto results = suite.run_complete_benchmark();
    
    // Display results in educational format
    display_benchmark_results(results);
    
    // Generate optimization report
    generate_optimization_report(results);
}

} // namespace performance
```

## Memory Performance Analysis

### Memory Access Pattern Analysis

```cpp
/**
 * @brief Memory Access Pattern Analyzer for Educational Insights
 * 
 * Analyzes memory access patterns to provide educational insights about
 * cache behavior, memory layout optimization, and performance characteristics.
 */
class MemoryAccessAnalyzer {
public:
    struct AccessPatternResult {
        std::string pattern_name;
        f64 average_access_time;
        f64 cache_hit_ratio;
        f64 memory_bandwidth_utilization;
        usize cache_lines_accessed;
        
        // Educational metrics
        f64 spatial_locality_score;   // How well accesses use spatial locality
        f64 temporal_locality_score;  // How well accesses use temporal locality
        std::vector<std::string> optimization_opportunities;
    };
    
    // Analyze different access patterns for educational comparison
    std::vector<AccessPatternResult> analyze_access_patterns() {
        std::vector<AccessPatternResult> results;
        
        // Test sequential access pattern
        results.push_back(analyze_sequential_access());
        
        // Test random access pattern
        results.push_back(analyze_random_access());
        
        // Test strided access pattern
        results.push_back(analyze_strided_access());
        
        // Test producer-consumer pattern
        results.push_back(analyze_producer_consumer_pattern());
        
        return results;
    }
    
private:
    AccessPatternResult analyze_sequential_access() {
        auto registry = create_test_registry();
        
        // Create test entities
        std::vector<Entity> entities;
        for (i32 i = 0; i < 10000; ++i) {
            entities.push_back(registry->create_entity(
                Transform{Vec2{static_cast<f32>(i), static_cast<f32>(i)}}
            ));
        }
        
        // Measure sequential access
        memory::tracker::start_analysis("Sequential_Access_Pattern");
        
        Timer access_timer;
        access_timer.start();
        
        // Sequential iteration (cache-friendly)
        registry->for_each<Transform>([](Entity e, Transform& t) {
            t.position.x += 1.0f; // Sequential memory access
        });
        
        f64 access_time = access_timer.elapsed_ms();
        memory::tracker::end_analysis();
        
        auto cache_analysis = memory::tracker::get_latest_analysis();
        
        AccessPatternResult result;
        result.pattern_name = "Sequential Access";
        result.average_access_time = access_time;
        result.cache_hit_ratio = cache_analysis.cache_hit_ratio;
        result.spatial_locality_score = 0.95; // Sequential access has excellent spatial locality
        result.temporal_locality_score = calculate_temporal_locality(cache_analysis);
        
        // Educational insights for sequential access
        result.optimization_opportunities.push_back("Excellent cache utilization with sequential access");
        result.optimization_opportunities.push_back("Consider SIMD optimization for this pattern");
        
        return result;
    }
    
    AccessPatternResult analyze_random_access() {
        auto registry = create_test_registry();
        
        // Create test entities
        std::vector<Entity> entities;
        for (i32 i = 0; i < 10000; ++i) {
            entities.push_back(registry->create_entity(
                Transform{Vec2{static_cast<f32>(i), static_cast<f32>(i)}}
            ));
        }
        
        // Shuffle for random access
        std::random_device rd;
        std::mt19937 gen(rd());
        std::shuffle(entities.begin(), entities.end(), gen);
        
        // Measure random access
        memory::tracker::start_analysis("Random_Access_Pattern");
        
        Timer access_timer;
        access_timer.start();
        
        // Random access (cache-hostile)
        for (Entity entity : entities) {
            auto* transform = registry->get_component<Transform>(entity);
            if (transform) {
                transform->position.x += 1.0f; // Random memory access
            }
        }
        
        f64 access_time = access_timer.elapsed_ms();
        memory::tracker::end_analysis();
        
        auto cache_analysis = memory::tracker::get_latest_analysis();
        
        AccessPatternResult result;
        result.pattern_name = "Random Access";
        result.average_access_time = access_time;
        result.cache_hit_ratio = cache_analysis.cache_hit_ratio;
        result.spatial_locality_score = 0.1; // Random access has poor spatial locality
        result.temporal_locality_score = calculate_temporal_locality(cache_analysis);
        
        // Educational insights for random access
        result.optimization_opportunities.push_back("Poor cache utilization - consider data layout changes");
        result.optimization_opportunities.push_back("Use prefetching or change to sequential access patterns");
        
        return result;
    }
    
    f64 calculate_temporal_locality(const memory::tracker::CacheAnalysisResult& analysis) {
        // Simplified temporal locality calculation for educational purposes
        // Real implementation would analyze access timing patterns
        return analysis.cache_hit_ratio; // Approximation
    }
    
    std::unique_ptr<ecs::Registry> create_test_registry() {
        return std::make_unique<ecs::Registry>(
            ecs::AllocatorConfig::create_educational_focused(),
            "Access_Pattern_Test"
        );
    }
};
```

### Memory Pressure Testing

```cpp
/**
 * @brief Memory Pressure Testing for Educational Analysis
 * 
 * Tests how different memory management strategies handle increasing
 * memory pressure and provides educational insights.
 */
class MemoryPressureTester {
public:
    struct PressureTestConfig {
        usize starting_entities{1000};
        usize max_entities{100000};
        usize step_size{1000};
        f64 pressure_threshold{0.8}; // When to consider memory pressure high
    };
    
    struct PressureTestResult {
        usize entity_count;
        f64 allocation_time;
        f64 memory_usage;
        f64 memory_efficiency;
        f64 performance_degradation; // Compared to starting performance
        bool memory_pressure_detected;
        std::vector<std::string> pressure_indicators;
    };
    
    // Run progressive memory pressure test
    std::vector<PressureTestResult> run_pressure_test(const PressureTestConfig& config) {
        std::vector<PressureTestResult> results;
        
        auto registry = std::make_unique<ecs::Registry>(
            ecs::AllocatorConfig::create_educational_focused(),
            "Memory_Pressure_Test"
        );
        
        f64 baseline_performance = 0.0;
        
        for (usize entity_count = config.starting_entities; 
             entity_count <= config.max_entities; 
             entity_count += config.step_size) {
            
            PressureTestResult result;
            result.entity_count = entity_count;
            
            // Add entities to reach target count
            usize current_entities = registry->active_entities();
            usize entities_to_add = entity_count - current_entities;
            
            Timer creation_timer;
            creation_timer.start();
            
            for (usize i = 0; i < entities_to_add; ++i) {
                registry->create_entity(
                    Transform{Vec2{static_cast<f32>(i), static_cast<f32>(i)}},
                    Velocity{Vec2{1.0f, 0.0f}}
                );
            }
            
            result.allocation_time = creation_timer.elapsed_ms();
            
            // Establish baseline performance
            if (baseline_performance == 0.0) {
                baseline_performance = result.allocation_time / entities_to_add;
            }
            
            // Calculate performance degradation
            f64 current_performance = result.allocation_time / entities_to_add;
            result.performance_degradation = current_performance / baseline_performance;
            
            // Collect memory statistics
            auto memory_stats = registry->get_memory_statistics();
            result.memory_usage = registry->memory_usage();
            result.memory_efficiency = memory_stats.memory_efficiency;
            
            // Detect memory pressure indicators
            result.memory_pressure_detected = detect_memory_pressure(*registry, config.pressure_threshold);
            result.pressure_indicators = analyze_pressure_indicators(*registry);
            
            results.push_back(result);
            
            // Educational logging
            LOG_INFO("Pressure test @ {} entities: {:.3f}ms allocation, {:.1f}% efficiency",
                    entity_count, result.allocation_time, result.memory_efficiency * 100.0);
            
            if (result.memory_pressure_detected) {
                LOG_WARN("Memory pressure detected at {} entities", entity_count);
                for (const auto& indicator : result.pressure_indicators) {
                    LOG_WARN("  • {}", indicator);
                }
            }
        }
        
        return results;
    }
    
private:
    bool detect_memory_pressure(const ecs::Registry& registry, f64 threshold) {
        auto stats = registry.get_memory_statistics();
        
        // Check various pressure indicators
        bool arena_pressure = stats.arena_utilization() > threshold;
        bool pool_pressure = stats.pool_utilization() > threshold;
        bool efficiency_degradation = stats.memory_efficiency < 0.7;
        
        return arena_pressure || pool_pressure || efficiency_degradation;
    }
    
    std::vector<std::string> analyze_pressure_indicators(const ecs::Registry& registry) {
        std::vector<std::string> indicators;
        auto stats = registry.get_memory_statistics();
        
        if (stats.arena_utilization() > 0.8) {
            indicators.push_back("Arena allocator >80% utilized");
        }
        
        if (stats.pool_utilization() > 0.8) {
            indicators.push_back("Pool allocator >80% utilized");
        }
        
        if (stats.memory_efficiency < 0.7) {
            indicators.push_back("Memory efficiency below 70%");
        }
        
        if (stats.cache_hit_ratio < 0.6) {
            indicators.push_back("Cache hit ratio below 60%");
        }
        
        if (stats.fragmentation_events > 100) {
            indicators.push_back("High memory fragmentation detected");
        }
        
        return indicators;
    }
};
```

## ECS Performance Optimization

### Entity Creation Optimization

```cpp
// Educational entity creation optimization demonstration
class EntityCreationOptimizer {
public:
    struct OptimizationStrategy {
        std::string name;
        std::function<void(ecs::Registry&, usize)> implementation;
        std::string description;
    };
    
    // Compare different entity creation strategies
    void compare_creation_strategies() {
        std::vector<OptimizationStrategy> strategies = {
            {
                "Individual Creation",
                [](ecs::Registry& registry, usize count) {
                    for (usize i = 0; i < count; ++i) {
                        registry.create_entity(Transform{Vec2{static_cast<f32>(i), 0}});
                    }
                },
                "Create entities one by one (baseline approach)"
            },
            {
                "Batch Creation",
                [](ecs::Registry& registry, usize count) {
                    // Pre-reserve archetype capacity
                    auto signature = ecs::make_signature<Transform>();
                    auto* archetype = registry.get_or_create_archetype(signature);
                    archetype->reserve_capacity(count);
                    
                    // Create entities in batch
                    for (usize i = 0; i < count; ++i) {
                        registry.create_entity(Transform{Vec2{static_cast<f32>(i), 0}});
                    }
                },
                "Pre-reserve archetype capacity for better allocation patterns"
            },
            {
                "Memory-Optimized Creation",
                [](ecs::Registry& registry, usize count) {
                    // Use memory-optimized configuration
                    auto config = ecs::AllocatorConfig::create_performance_optimized();
                    // Implementation would use specialized fast creation path
                    
                    for (usize i = 0; i < count; ++i) {
                        registry.create_entity(Transform{Vec2{static_cast<f32>(i), 0}});
                    }
                },
                "Use performance-optimized allocator configuration"
            }
        };
        
        const usize entity_count = 10000;
        
        std::cout << "=== Entity Creation Strategy Comparison ===\n";
        std::cout << "Creating " << entity_count << " entities with each strategy:\n\n";
        
        std::vector<f64> strategy_times;
        
        for (const auto& strategy : strategies) {
            auto registry = std::make_unique<ecs::Registry>(
                ecs::AllocatorConfig::create_educational_focused(),
                "Creation_Strategy_Test"
            );
            
            Timer strategy_timer;
            strategy_timer.start();
            
            strategy.implementation(*registry, entity_count);
            
            f64 strategy_time = strategy_timer.elapsed_ms();
            strategy_times.push_back(strategy_time);
            
            std::cout << "Strategy: " << strategy.name << "\n";
            std::cout << "  Time: " << strategy_time << " ms\n";
            std::cout << "  Rate: " << (entity_count / strategy_time) << " entities/ms\n";
            std::cout << "  Description: " << strategy.description << "\n";
            
            // Memory analysis
            auto memory_stats = registry->get_memory_statistics();
            std::cout << "  Memory efficiency: " << (memory_stats.memory_efficiency * 100.0) << "%\n";
            std::cout << "  Cache hit ratio: " << (memory_stats.cache_hit_ratio * 100.0) << "%\n\n";
        }
        
        // Comparative analysis
        f64 best_time = *std::min_element(strategy_times.begin(), strategy_times.end());
        std::cout << "Performance comparison (vs best):\n";
        for (usize i = 0; i < strategies.size(); ++i) {
            f64 relative_performance = strategy_times[i] / best_time;
            std::cout << "  " << strategies[i].name << ": " << relative_performance << "x";
            if (relative_performance == 1.0) {
                std::cout << " ★ (fastest)";
            }
            std::cout << "\n";
        }
    }
};
```

### Component Access Optimization

```cpp
// Educational component access optimization techniques
class ComponentAccessOptimizer {
public:
    // Demonstrate different component access patterns
    void demonstrate_access_patterns() {
        auto registry = create_large_test_scene();
        
        std::cout << "=== Component Access Pattern Comparison ===\n";
        
        // Pattern 1: Individual entity access (cache-hostile)
        test_individual_access(*registry);
        
        // Pattern 2: Archetype iteration (cache-friendly) 
        test_archetype_iteration(*registry);
        
        // Pattern 3: Bulk component processing (optimal)
        test_bulk_processing(*registry);
        
        // Display comparative results
        display_access_pattern_results();
    }
    
private:
    void test_individual_access(ecs::Registry& registry) {
        auto entities = registry.get_all_entities();
        
        memory::tracker::start_analysis("Individual_Entity_Access");
        Timer timer;
        timer.start();
        
        // Access entities individually (potentially cache-hostile)
        for (Entity entity : entities) {
            auto* transform = registry.get_component<Transform>(entity);
            auto* velocity = registry.get_component<Velocity>(entity);
            
            if (transform && velocity) {
                transform->position += velocity->velocity * 0.016f;
            }
        }
        
        f64 access_time = timer.elapsed_ms();
        memory::tracker::end_analysis();
        
        LOG_INFO("Individual access: {:.3f}ms for {} entities", access_time, entities.size());
    }
    
    void test_archetype_iteration(ecs::Registry& registry) {
        memory::tracker::start_analysis("Archetype_Iteration");
        Timer timer;
        timer.start();
        
        // Iterate by archetype (cache-friendly)
        registry.for_each<Transform, Velocity>([](Entity e, Transform& t, Velocity& v) {
            t.position += v.velocity * 0.016f;
        });
        
        f64 iteration_time = timer.elapsed_ms();
        memory::tracker::end_analysis();
        
        LOG_INFO("Archetype iteration: {:.3f}ms", iteration_time);
    }
    
    void test_bulk_processing(ecs::Registry& registry) {
        memory::tracker::start_analysis("Bulk_Processing");
        Timer timer;
        timer.start();
        
        // Access raw component arrays for maximum performance
        auto archetype_stats = registry.get_archetype_stats();
        for (const auto& [signature, entity_count] : archetype_stats) {
            if (signature.has<Transform>() && signature.has<Velocity>()) {
                // Get direct access to component arrays
                auto* archetype = registry.get_archetype_by_signature(signature);
                auto& transforms = archetype->get_component_array<Transform>();
                auto& velocities = archetype->get_component_array<Velocity>();
                
                // Bulk processing with potential SIMD optimization
                for (usize i = 0; i < entity_count; ++i) {
                    transforms[i].position += velocities[i].velocity * 0.016f;
                }
            }
        }
        
        f64 bulk_time = timer.elapsed_ms();
        memory::tracker::end_analysis();
        
        LOG_INFO("Bulk processing: {:.3f}ms", bulk_time);
    }
    
    std::unique_ptr<ecs::Registry> create_large_test_scene() {
        auto registry = std::make_unique<ecs::Registry>(
            ecs::AllocatorConfig::create_educational_focused(),
            "Large_Test_Scene"
        );
        
        // Create diverse entity mix for realistic testing
        for (i32 i = 0; i < 10000; ++i) {
            registry->create_entity(
                Transform{Vec2{static_cast<f32>(i % 100), static_cast<f32>(i / 100)}},
                Velocity{Vec2{(i % 2 == 0) ? 1.0f : -1.0f, 0.0f}}
            );
        }
        
        return registry;
    }
};
```

## Physics Performance Analysis

### Physics System Profiling

```cpp
/**
 * @brief Physics Performance Analyzer for Educational Insights
 * 
 * Provides detailed analysis of physics system performance characteristics
 * and educational insights into physics optimization techniques.
 */
class PhysicsPerformanceAnalyzer {
public:
    struct PhysicsProfile {
        f64 broad_phase_time;      // Collision detection broad phase
        f64 narrow_phase_time;     // Detailed collision detection
        f64 constraint_solving_time; // Constraint resolution
        f64 integration_time;      // Position/velocity integration
        f64 total_physics_time;    // Total physics update time
        
        usize active_bodies;
        usize collision_checks;
        usize active_contacts;
        usize constraint_iterations;
        
        // Educational insights
        f64 collision_efficiency;  // Useful collisions / total checks
        f64 solver_convergence;    // How well constraints converged
        std::vector<std::string> performance_insights;
        std::vector<std::string> optimization_suggestions;
    };
    
    // Profile physics system performance
    PhysicsProfile profile_physics_system(physics::PhysicsWorld& world, 
                                         ecs::Registry& registry,
                                         f32 time_step,
                                         usize simulation_steps = 100) {
        PhysicsProfile profile{};
        
        LOG_INFO("Profiling physics system for {} steps", simulation_steps);
        
        // Collect detailed timing for each physics phase
        for (usize step = 0; step < simulation_steps; ++step) {
            auto step_profile = profile_single_physics_step(world, registry, time_step);
            
            // Accumulate timing data
            profile.broad_phase_time += step_profile.broad_phase_time;
            profile.narrow_phase_time += step_profile.narrow_phase_time;
            profile.constraint_solving_time += step_profile.constraint_solving_time;
            profile.integration_time += step_profile.integration_time;
            profile.total_physics_time += step_profile.total_physics_time;
            
            // Accumulate statistics
            profile.active_bodies = std::max(profile.active_bodies, step_profile.active_bodies);
            profile.collision_checks += step_profile.collision_checks;
            profile.active_contacts += step_profile.active_contacts;
            profile.constraint_iterations += step_profile.constraint_iterations;
        }
        
        // Calculate averages
        profile.broad_phase_time /= simulation_steps;
        profile.narrow_phase_time /= simulation_steps;
        profile.constraint_solving_time /= simulation_steps;
        profile.integration_time /= simulation_steps;
        profile.total_physics_time /= simulation_steps;
        
        // Calculate educational metrics
        profile.collision_efficiency = calculate_collision_efficiency(profile);
        profile.solver_convergence = calculate_solver_convergence(world);
        
        // Generate insights and recommendations
        generate_physics_insights(profile);
        
        return profile;
    }
    
    // Educational: Display physics performance breakdown
    void display_physics_breakdown(const PhysicsProfile& profile) {
        std::cout << "=== Physics Performance Breakdown ===\n";
        std::cout << "Total physics time: " << profile.total_physics_time << " ms\n\n";
        
        // Phase breakdown with percentages
        f64 total = profile.total_physics_time;
        std::cout << "Phase breakdown:\n";
        std::cout << "  Broad phase:      " << std::setw(8) << profile.broad_phase_time 
                  << " ms (" << std::setw(5) << (profile.broad_phase_time * 100.0 / total) << "%)\n";
        std::cout << "  Narrow phase:     " << std::setw(8) << profile.narrow_phase_time
                  << " ms (" << std::setw(5) << (profile.narrow_phase_time * 100.0 / total) << "%)\n";
        std::cout << "  Constraint solve: " << std::setw(8) << profile.constraint_solving_time
                  << " ms (" << std::setw(5) << (profile.constraint_solving_time * 100.0 / total) << "%)\n";
        std::cout << "  Integration:      " << std::setw(8) << profile.integration_time
                  << " ms (" << std::setw(5) << (profile.integration_time * 100.0 / total) << "%)\n";
        
        // Performance characteristics
        std::cout << "\nPerformance characteristics:\n";
        std::cout << "  Active bodies: " << profile.active_bodies << "\n";
        std::cout << "  Collision checks: " << profile.collision_checks << "\n";
        std::cout << "  Active contacts: " << profile.active_contacts << "\n";
        std::cout << "  Collision efficiency: " << (profile.collision_efficiency * 100.0) << "%\n";
        std::cout << "  Solver convergence: " << (profile.solver_convergence * 100.0) << "%\n";
        
        // Educational insights
        std::cout << "\nPerformance insights:\n";
        for (const auto& insight : profile.performance_insights) {
            std::cout << "  • " << insight << "\n";
        }
        
        // Optimization suggestions
        if (!profile.optimization_suggestions.empty()) {
            std::cout << "\nOptimization suggestions:\n";
            for (const auto& suggestion : profile.optimization_suggestions) {
                std::cout << "  → " << suggestion << "\n";
            }
        }
    }
    
private:
    struct SingleStepProfile {
        f64 broad_phase_time;
        f64 narrow_phase_time;
        f64 constraint_solving_time;
        f64 integration_time;
        f64 total_physics_time;
        usize active_bodies;
        usize collision_checks;
        usize active_contacts;
        usize constraint_iterations;
    };
    
    SingleStepProfile profile_single_physics_step(physics::PhysicsWorld& world,
                                                 ecs::Registry& registry,
                                                 f32 time_step) {
        SingleStepProfile profile{};
        
        Timer total_timer;
        total_timer.start();
        
        // Enable detailed profiling in physics world
        world.enable_detailed_profiling(true);
        
        // Run single physics step
        world.update(registry, time_step);
        
        profile.total_physics_time = total_timer.elapsed_ms();
        
        // Collect detailed timing from physics world
        auto physics_stats = world.get_detailed_performance_stats();
        profile.broad_phase_time = physics_stats.broad_phase_time;
        profile.narrow_phase_time = physics_stats.narrow_phase_time;
        profile.constraint_solving_time = physics_stats.constraint_solving_time;
        profile.integration_time = physics_stats.integration_time;
        
        // Collect entity and collision statistics
        profile.active_bodies = physics_stats.active_body_count;
        profile.collision_checks = physics_stats.collision_checks_performed;
        profile.active_contacts = physics_stats.active_contact_count;
        profile.constraint_iterations = physics_stats.constraint_iterations_performed;
        
        return profile;
    }
    
    void generate_physics_insights(PhysicsProfile& profile) {
        // Analyze phase distribution
        f64 total = profile.total_physics_time;
        f64 collision_time = profile.broad_phase_time + profile.narrow_phase_time;
        f64 collision_percentage = (collision_time * 100.0) / total;
        
        if (collision_percentage > 60.0) {
            profile.performance_insights.push_back("Collision detection dominates physics time");
            profile.optimization_suggestions.push_back("Consider spatial partitioning optimization");
            profile.optimization_suggestions.push_back("Implement sleeping bodies for inactive objects");
        }
        
        if (profile.constraint_solving_time > total * 0.4) {
            profile.performance_insights.push_back("Constraint solving is expensive");
            profile.optimization_suggestions.push_back("Reduce constraint iterations or improve warm starting");
        }
        
        // Collision efficiency analysis
        if (profile.collision_efficiency < 0.1) {
            profile.performance_insights.push_back("Low collision efficiency - many unnecessary checks");
            profile.optimization_suggestions.push_back("Improve broad-phase collision detection");
        }
        
        // Scalability analysis
        f64 checks_per_body = static_cast<f64>(profile.collision_checks) / profile.active_bodies;
        if (checks_per_body > profile.active_bodies * 0.5) {
            profile.performance_insights.push_back("Collision checks scaling poorly with body count");
            profile.optimization_suggestions.push_back("Implement better spatial partitioning");
        }
    }
};
```

## Rendering Performance Analysis

### Render Pipeline Profiling

```cpp
/**
 * @brief Rendering Performance Analyzer for Educational GPU Optimization
 * 
 * Analyzes rendering pipeline performance and provides insights into
 * GPU optimization techniques and bottleneck identification.
 */
class RenderingPerformanceAnalyzer {
public:
    struct RenderProfile {
        f64 cpu_preparation_time;    // CPU-side render preparation
        f64 gpu_execution_time;      // GPU rendering time
        f64 total_frame_time;        // Total frame time
        
        usize draw_calls;            // Number of draw calls issued
        usize vertices_rendered;     // Total vertices processed
        usize textures_bound;        // Texture binding changes
        usize shader_changes;        // Shader program changes
        
        // GPU metrics (if available)
        f64 gpu_utilization;         // GPU usage percentage
        usize vram_usage;            // Video memory usage
        
        // Educational insights
        f64 batching_efficiency;     // How well batching is working
        f64 overdraw_factor;         // How much overdraw is occurring
        std::vector<std::string> bottlenecks;
        std::vector<std::string> optimization_opportunities;
    };
    
    // Profile complete rendering pipeline
    RenderProfile profile_rendering_system(renderer::Renderer2D& renderer,
                                          ecs::Registry& registry,
                                          usize frame_count = 100) {
        RenderProfile profile{};
        
        LOG_INFO("Profiling rendering system for {} frames", frame_count);
        
        for (usize frame = 0; frame < frame_count; ++frame) {
            auto frame_profile = profile_single_frame(renderer, registry);
            
            // Accumulate timing data
            profile.cpu_preparation_time += frame_profile.cpu_preparation_time;
            profile.gpu_execution_time += frame_profile.gpu_execution_time;
            profile.total_frame_time += frame_profile.total_frame_time;
            
            // Accumulate statistics
            profile.draw_calls += frame_profile.draw_calls;
            profile.vertices_rendered += frame_profile.vertices_rendered;
            profile.textures_bound += frame_profile.textures_bound;
            profile.shader_changes += frame_profile.shader_changes;
        }
        
        // Calculate averages
        profile.cpu_preparation_time /= frame_count;
        profile.gpu_execution_time /= frame_count;
        profile.total_frame_time /= frame_count;
        profile.draw_calls /= frame_count;
        profile.vertices_rendered /= frame_count;
        profile.textures_bound /= frame_count;
        profile.shader_changes /= frame_count;
        
        // Calculate educational metrics
        profile.batching_efficiency = calculate_batching_efficiency(profile);
        profile.overdraw_factor = estimate_overdraw_factor(renderer);
        
        // Generate insights and optimization opportunities
        generate_rendering_insights(profile);
        
        return profile;
    }
    
    // Educational: Display rendering performance breakdown
    void display_rendering_breakdown(const RenderProfile& profile) {
        std::cout << "=== Rendering Performance Breakdown ===\n";
        std::cout << "Average frame time: " << profile.total_frame_time << " ms\n";
        std::cout << "Target FPS: " << (1000.0 / profile.total_frame_time) << " FPS\n\n";
        
        // CPU vs GPU time breakdown
        f64 total = profile.total_frame_time;
        std::cout << "Time breakdown:\n";
        std::cout << "  CPU preparation: " << std::setw(8) << profile.cpu_preparation_time
                  << " ms (" << std::setw(5) << (profile.cpu_preparation_time * 100.0 / total) << "%)\n";
        std::cout << "  GPU execution:   " << std::setw(8) << profile.gpu_execution_time
                  << " ms (" << std::setw(5) << (profile.gpu_execution_time * 100.0 / total) << "%)\n";
        
        // Rendering statistics
        std::cout << "\nRendering statistics:\n";
        std::cout << "  Draw calls: " << profile.draw_calls << "\n";
        std::cout << "  Vertices: " << profile.vertices_rendered << "\n";
        std::cout << "  Texture bindings: " << profile.textures_bound << "\n";
        std::cout << "  Shader changes: " << profile.shader_changes << "\n";
        
        // Educational efficiency metrics
        std::cout << "\nEfficiency metrics:\n";
        std::cout << "  Batching efficiency: " << (profile.batching_efficiency * 100.0) << "%\n";
        std::cout << "  Vertices per draw call: " << (profile.vertices_rendered / profile.draw_calls) << "\n";
        std::cout << "  Overdraw factor: " << profile.overdraw_factor << "x\n";
        
        // Performance insights
        if (!profile.bottlenecks.empty()) {
            std::cout << "\nIdentified bottlenecks:\n";
            for (const auto& bottleneck : profile.bottlenecks) {
                std::cout << "  ⚠ " << bottleneck << "\n";
            }
        }
        
        if (!profile.optimization_opportunities.empty()) {
            std::cout << "\nOptimization opportunities:\n";
            for (const auto& opportunity : profile.optimization_opportunities) {
                std::cout << "  → " << opportunity << "\n";
            }
        }
    }
    
private:
    struct SingleFrameProfile {
        f64 cpu_preparation_time;
        f64 gpu_execution_time;
        f64 total_frame_time;
        usize draw_calls;
        usize vertices_rendered;
        usize textures_bound;
        usize shader_changes;
    };
    
    SingleFrameProfile profile_single_frame(renderer::Renderer2D& renderer, ecs::Registry& registry) {
        SingleFrameProfile profile{};
        
        Timer total_timer, cpu_timer, gpu_timer;
        total_timer.start();
        
        // Profile CPU preparation phase
        cpu_timer.start();
        renderer.begin_frame();
        
        // Collect renderable entities and generate render commands
        registry.for_each<Transform, RenderComponent>([&](Entity e, Transform& t, RenderComponent& r) {
            renderer.submit_sprite(r.texture, t.position, t.rotation, r.tint);
        });
        
        profile.cpu_preparation_time = cpu_timer.elapsed_ms();
        
        // Profile GPU execution phase
        gpu_timer.start();
        renderer.end_frame(); // Submit all commands to GPU
        
        // Wait for GPU completion (for accurate timing)
        renderer.wait_for_gpu_completion();
        profile.gpu_execution_time = gpu_timer.elapsed_ms();
        
        profile.total_frame_time = total_timer.elapsed_ms();
        
        // Collect rendering statistics
        auto render_stats = renderer.get_frame_statistics();
        profile.draw_calls = render_stats.draw_calls;
        profile.vertices_rendered = render_stats.vertices_rendered;
        profile.textures_bound = render_stats.texture_bindings;
        profile.shader_changes = render_stats.shader_changes;
        
        return profile;
    }
    
    void generate_rendering_insights(RenderProfile& profile) {
        // Analyze CPU vs GPU balance
        f64 cpu_percentage = (profile.cpu_preparation_time * 100.0) / profile.total_frame_time;
        
        if (cpu_percentage > 70.0) {
            profile.bottlenecks.push_back("CPU-bound: CPU preparation time dominates");
            profile.optimization_opportunities.push_back("Optimize CPU-side rendering code");
            profile.optimization_opportunities.push_back("Consider multithreaded command generation");
        } else if (cpu_percentage < 30.0) {
            profile.bottlenecks.push_back("GPU-bound: GPU execution time dominates");
            profile.optimization_opportunities.push_back("Reduce polygon count or shader complexity");
            profile.optimization_opportunities.push_back("Optimize fragment shader performance");
        }
        
        // Analyze draw call efficiency
        if (profile.draw_calls > 100) {
            profile.bottlenecks.push_back("High draw call count may limit performance");
            profile.optimization_opportunities.push_back("Implement better sprite batching");
        }
        
        f64 vertices_per_call = static_cast<f64>(profile.vertices_rendered) / profile.draw_calls;
        if (vertices_per_call < 100) {
            profile.optimization_opportunities.push_back("Low vertices per draw call - improve batching");
        }
        
        // Analyze state changes
        if (profile.textures_bound > profile.draw_calls * 1.5) {
            profile.optimization_opportunities.push_back("Excessive texture binding - use texture atlases");
        }
        
        if (profile.shader_changes > 10) {
            profile.optimization_opportunities.push_back("Frequent shader changes - batch by material");
        }
    }
};
```

## Educational Performance Labs

### Lab 1: The Performance Detective

**Objective**: Learn to identify and solve performance bottlenecks through systematic analysis.

```cpp
// Educational performance debugging exercise
class PerformanceDetectiveLab {
public:
    void run_detective_exercise() {
        std::cout << "=== Performance Detective Lab ===\n";
        std::cout << "Your mission: Find and fix performance bottlenecks in a simulated game scene.\n\n";
        
        // Create intentionally suboptimal scene
        auto problematic_scene = create_problematic_scene();
        
        // Step 1: Baseline measurement
        std::cout << "Step 1: Measuring baseline performance...\n";
        auto baseline_profile = measure_scene_performance(*problematic_scene);
        display_performance_profile(baseline_profile, "Baseline");
        
        // Step 2: Identify bottlenecks (educational guidance)
        std::cout << "\nStep 2: Analyzing performance bottlenecks...\n";
        auto bottlenecks = identify_bottlenecks(baseline_profile);
        for (const auto& bottleneck : bottlenecks) {
            std::cout << "  🔍 Found bottleneck: " << bottleneck.description << "\n";
            std::cout << "     Impact: " << bottleneck.performance_impact << "\n";
            std::cout << "     Suggested fix: " << bottleneck.suggested_fix << "\n\n";
        }
        
        // Step 3: Apply optimizations
        std::cout << "Step 3: Applying optimizations...\n";
        auto optimized_scene = apply_optimizations(*problematic_scene, bottlenecks);
        
        // Step 4: Measure improvement
        std::cout << "\nStep 4: Measuring optimization results...\n";
        auto optimized_profile = measure_scene_performance(*optimized_scene);
        display_performance_profile(optimized_profile, "Optimized");
        
        // Step 5: Educational analysis
        std::cout << "\nStep 5: Performance improvement analysis...\n";
        analyze_optimization_impact(baseline_profile, optimized_profile);
    }
    
private:
    struct PerformanceBottleneck {
        std::string description;
        f64 performance_impact; // Estimated time cost in ms
        std::string suggested_fix;
        BottleneckType type;
    };
    
    std::unique_ptr<ecs::Registry> create_problematic_scene() {
        auto registry = std::make_unique<ecs::Registry>(
            ecs::AllocatorConfig::create_memory_conservative(), // Intentionally suboptimal
            "Problematic_Scene"
        );
        
        // Create scene with known performance issues
        
        // Issue 1: Poor memory layout (mixed archetypes)
        for (i32 i = 0; i < 1000; ++i) {
            if (i % 3 == 0) {
                registry->create_entity(Transform{Vec2{i, i}});
            } else if (i % 3 == 1) {
                registry->create_entity(Transform{Vec2{i, i}}, Velocity{Vec2{1, 0}});
            } else {
                registry->create_entity(Transform{Vec2{i, i}}, Velocity{Vec2{1, 0}}, RenderComponent{});
            }
        }
        
        // Issue 2: Many small archetypes (poor cache utilization)
        for (i32 i = 0; i < 100; ++i) {
            registry->create_entity(Transform{Vec2{i, i}}, UniqueComponent{i});
        }
        
        return registry;
    }
    
    std::vector<PerformanceBottleneck> identify_bottlenecks(const ScenePerformanceProfile& profile) {
        std::vector<PerformanceBottleneck> bottlenecks;
        
        // Educational: Systematic bottleneck identification
        
        // Check for memory allocation bottlenecks
        if (profile.entity_creation_time > 0.001 * profile.entity_count) {
            bottlenecks.push_back({
                .description = "Slow entity creation due to standard allocation",
                .performance_impact = profile.entity_creation_time * 0.7, // Estimated impact
                .suggested_fix = "Switch to arena allocator for better cache locality",
                .type = BottleneckType::Memory
            });
        }
        
        // Check for cache performance issues
        if (profile.cache_hit_ratio < 0.7) {
            bottlenecks.push_back({
                .description = "Poor cache utilization in component access",
                .performance_impact = profile.component_access_time * 0.5,
                .suggested_fix = "Reorganize components for better spatial locality",
                .type = BottleneckType::Cache
            });
        }
        
        // Check for archetype fragmentation
        if (profile.archetype_count > profile.entity_count * 0.1) {
            bottlenecks.push_back({
                .description = "Too many small archetypes causing cache misses",
                .performance_impact = profile.component_iteration_time * 0.3,
                .suggested_fix = "Consolidate component combinations or use component padding",
                .type = BottleneckType::Architecture
            });
        }
        
        return bottlenecks;
    }
};
```

### Lab 2: Optimization Racing Championship

**Objective**: Competitive optimization exercise where students race to achieve the best performance.

```cpp
// Educational optimization competition framework
class OptimizationRacingLab {
public:
    struct RaceConfiguration {
        usize target_entity_count{50000};
        f64 performance_budget{16.67}; // 60 FPS budget in milliseconds
        std::vector<OptimizationTechnique> allowed_techniques;
        bool enable_leaderboard{true};
    };
    
    struct RaceResult {
        std::string participant_name;
        f64 final_performance_time;
        f64 performance_improvement;
        std::vector<std::string> techniques_used;
        std::string performance_grade;
    };
    
    void run_optimization_race(const RaceConfiguration& config) {
        std::cout << "=== Optimization Racing Championship ===\n";
        std::cout << "Goal: Optimize ECScope to handle " << config.target_entity_count 
                  << " entities within " << config.performance_budget << "ms budget\n\n";
        
        // Provide baseline implementation
        auto baseline_result = measure_baseline_performance(config);
        display_race_result(baseline_result, "Baseline");
        
        // Educational: Provide optimization toolkit
        display_optimization_toolkit(config.allowed_techniques);
        
        // Interactive optimization session
        run_interactive_optimization_session(config);
    }
    
private:
    RaceResult measure_baseline_performance(const RaceConfiguration& config) {
        // Create baseline registry with standard settings
        auto registry = std::make_unique<ecs::Registry>(
            ecs::AllocatorConfig::create_memory_conservative(),
            "Baseline_Performance"
        );
        
        Timer performance_timer;
        performance_timer.start();
        
        // Create target number of entities
        for (usize i = 0; i < config.target_entity_count; ++i) {
            registry->create_entity(
                Transform{Vec2{static_cast<f32>(i % 100), static_cast<f32>(i / 100)}},
                Velocity{Vec2{1.0f, 0.0f}}
            );
        }
        
        // Run typical game update loop
        registry->for_each<Transform, Velocity>([](Entity e, Transform& t, Velocity& v) {
            t.position += v.velocity * 0.016f;
        });
        
        f64 total_time = performance_timer.elapsed_ms();
        
        return RaceResult{
            .participant_name = "Baseline",
            .final_performance_time = total_time,
            .performance_improvement = 1.0,
            .techniques_used = {"Standard allocation", "Basic iteration"},
            .performance_grade = calculate_performance_grade(total_time, config.performance_budget)
        };
    }
    
    void display_optimization_toolkit(const std::vector<OptimizationTechnique>& techniques) {
        std::cout << "Available optimization techniques:\n";
        for (const auto& technique : techniques) {
            std::cout << "  " << technique.name << "\n";
            std::cout << "    Description: " << technique.description << "\n";
            std::cout << "    Expected improvement: " << technique.expected_improvement << "\n";
            std::cout << "    Difficulty: " << technique.difficulty_level << "\n\n";
        }
    }
    
    std::string calculate_performance_grade(f64 actual_time, f64 budget_time) {
        f64 ratio = actual_time / budget_time;
        
        if (ratio <= 0.5) return "A+ (Excellent)";
        if (ratio <= 0.8) return "A (Very Good)";
        if (ratio <= 1.0) return "B (Good)";
        if (ratio <= 1.5) return "C (Acceptable)";
        if (ratio <= 2.0) return "D (Poor)";
        return "F (Unacceptable)";
    }
};
```

## Advanced Profiling Techniques

### SIMD Performance Analysis

```cpp
// Educational SIMD performance comparison
class SIMDPerformanceAnalyzer {
public:
    struct SIMDComparisonResult {
        f64 scalar_time;
        f64 simd_time;
        f64 speedup_factor;
        usize operations_tested;
        std::string operation_type;
    };
    
    // Compare scalar vs SIMD implementations
    std::vector<SIMDComparisonResult> compare_simd_implementations() {
        std::vector<SIMDComparisonResult> results;
        
        // Test vector operations
        results.push_back(compare_vector_addition());
        results.push_back(compare_matrix_multiplication());
        results.push_back(compare_physics_integration());
        
        return results;
    }
    
private:
    SIMDComparisonResult compare_vector_addition() {
        const usize vector_count = 100000;
        
        // Create test data
        std::vector<Vec2> vectors_a(vector_count);
        std::vector<Vec2> vectors_b(vector_count);
        std::vector<Vec2> results_scalar(vector_count);
        std::vector<Vec2> results_simd(vector_count);
        
        // Initialize with random data
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<f32> dis(-100.0f, 100.0f);
        
        for (usize i = 0; i < vector_count; ++i) {
            vectors_a[i] = Vec2{dis(gen), dis(gen)};
            vectors_b[i] = Vec2{dis(gen), dis(gen)};
        }
        
        // Test scalar implementation
        Timer scalar_timer;
        scalar_timer.start();
        
        for (usize i = 0; i < vector_count; ++i) {
            results_scalar[i] = vectors_a[i] + vectors_b[i];
        }
        
        f64 scalar_time = scalar_timer.elapsed_ms();
        
        // Test SIMD implementation
        Timer simd_timer;
        simd_timer.start();
        
        // Process 4 vectors at a time with SIMD
        for (usize i = 0; i < vector_count; i += 4) {
            // Load 4 Vec2s into SIMD registers
            __m256 a_x = _mm256_load_ps(&vectors_a[i].x);
            __m256 a_y = _mm256_load_ps(&vectors_a[i].y);
            __m256 b_x = _mm256_load_ps(&vectors_b[i].x);
            __m256 b_y = _mm256_load_ps(&vectors_b[i].y);
            
            // Perform 4 additions simultaneously
            __m256 result_x = _mm256_add_ps(a_x, b_x);
            __m256 result_y = _mm256_add_ps(a_y, b_y);
            
            // Store results
            _mm256_store_ps(&results_simd[i].x, result_x);
            _mm256_store_ps(&results_simd[i].y, result_y);
        }
        
        f64 simd_time = simd_timer.elapsed_ms();
        
        return SIMDComparisonResult{
            .scalar_time = scalar_time,
            .simd_time = simd_time,
            .speedup_factor = scalar_time / simd_time,
            .operations_tested = vector_count,
            .operation_type = "Vector Addition"
        };
    }
};
```

### GPU Performance Analysis

```cpp
// Educational GPU performance analysis
class GPUPerformanceAnalyzer {
public:
    struct GPUProfile {
        f64 vertex_processing_time;
        f64 fragment_processing_time;
        f64 memory_bandwidth_usage;
        usize triangles_processed;
        usize pixels_rendered;
        f64 gpu_utilization;
        
        std::vector<std::string> gpu_bottlenecks;
        std::vector<std::string> gpu_optimizations;
    };
    
    // Analyze GPU performance characteristics
    GPUProfile analyze_gpu_performance(renderer::Renderer2D& renderer) {
        GPUProfile profile{};
        
        // Enable detailed GPU profiling
        renderer.enable_gpu_profiling(true);
        
        // Render complex scene for analysis
        render_complex_test_scene(renderer);
        
        // Collect GPU metrics
        auto gpu_stats = renderer.get_gpu_performance_stats();
        profile.vertex_processing_time = gpu_stats.vertex_stage_time;
        profile.fragment_processing_time = gpu_stats.fragment_stage_time;
        profile.triangles_processed = gpu_stats.triangles_processed;
        profile.pixels_rendered = gpu_stats.pixels_rendered;
        
        // Analyze GPU utilization
        analyze_gpu_utilization(profile, gpu_stats);
        
        return profile;
    }
    
private:
    void analyze_gpu_utilization(GPUProfile& profile, const renderer::GPUStats& stats) {
        // Educational GPU bottleneck identification
        
        f64 vertex_percentage = profile.vertex_processing_time / 
                               (profile.vertex_processing_time + profile.fragment_processing_time);
        
        if (vertex_percentage > 0.7) {
            profile.gpu_bottlenecks.push_back("Vertex-bound: Complex vertex shaders or high polygon count");
            profile.gpu_optimizations.push_back("Optimize vertex shader complexity");
            profile.gpu_optimizations.push_back("Reduce polygon count through LOD systems");
        } else if (vertex_percentage < 0.3) {
            profile.gpu_bottlenecks.push_back("Fragment-bound: Complex pixel shaders or high resolution");
            profile.gpu_optimizations.push_back("Optimize fragment shader complexity");
            profile.gpu_optimizations.push_back("Reduce rendering resolution or use LOD");
        }
        
        // Analyze overdraw
        f64 overdraw_ratio = static_cast<f64>(profile.pixels_rendered) / stats.screen_pixels;
        if (overdraw_ratio > 2.0) {
            profile.gpu_bottlenecks.push_back("High overdraw causing fragment processing bottleneck");
            profile.gpu_optimizations.push_back("Implement depth testing or front-to-back sorting");
        }
    }
};
```

---

**ECScope Performance Analysis: Making performance visible, bottlenecks identifiable, and optimization achievable.**

*Performance is not about guessing - it's about measuring, understanding, and systematically improving. ECScope provides the tools to make performance engineering a science, not an art.*