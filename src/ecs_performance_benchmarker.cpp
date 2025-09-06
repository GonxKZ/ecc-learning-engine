/**
 * @file ecs_performance_benchmarker.cpp
 * @brief Implementation of comprehensive ECS performance benchmarker
 * 
 * This file implements the ECS performance benchmarking system that provides
 * detailed analysis of ECS architectures, memory patterns, and system performance
 * with educational insights and optimization recommendations.
 * 
 * @author ECScope Educational ECS Framework - Performance Benchmarking Suite
 * @date 2025
 */

#include "ecs_performance_benchmarker.hpp"
#include <algorithm>
#include <numeric>
#include <cmath>
#include <sstream>
#include <iomanip>

namespace ecscope::performance::ecs {

//=============================================================================
// ECSBenchmarkResult Implementation
//=============================================================================

void ECSBenchmarkResult::calculate_statistics() {
    if (raw_timings.empty()) return;
    
    // Sort for percentile calculations
    std::vector<f64> sorted_timings = raw_timings;
    std::sort(sorted_timings.begin(), sorted_timings.end());
    
    // Basic statistics
    min_time_us = sorted_timings.front();
    max_time_us = sorted_timings.back();
    average_time_us = std::accumulate(sorted_timings.begin(), sorted_timings.end(), 0.0) / sorted_timings.size();
    
    // Median
    usize mid = sorted_timings.size() / 2;
    if (sorted_timings.size() % 2 == 0) {
        median_time_us = (sorted_timings[mid - 1] + sorted_timings[mid]) / 2.0;
    } else {
        median_time_us = sorted_timings[mid];
    }
    
    // Standard deviation
    f64 variance = 0.0;
    for (f64 time : sorted_timings) {
        f64 diff = time - average_time_us;
        variance += diff * diff;
    }
    std_deviation_us = std::sqrt(variance / sorted_timings.size());
    
    // Consistency score (inverse of coefficient of variation)
    consistency_score = average_time_us > 0 ? 1.0 - (std_deviation_us / average_time_us) : 0.0;
    consistency_score = std::max(0.0, std::min(1.0, consistency_score));
    
    // Calculate throughput metrics
    if (average_time_us > 0) {
        entities_per_second = entity_count / (average_time_us / 1000000.0);
        operations_per_second = 1000000.0 / average_time_us;
    }
    
    // Memory efficiency (entities per MB)
    if (peak_memory_usage > 0) {
        memory_efficiency = static_cast<f64>(entity_count) / (peak_memory_usage / (1024.0 * 1024.0));
    }
    
    is_valid = true;
}

std::string ECSBenchmarkResult::to_csv_row() const {
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(3);
    ss << test_name << "," << static_cast<int>(category) << "," 
       << static_cast<int>(architecture_type) << "," << entity_count << ","
       << average_time_us << "," << min_time_us << "," << max_time_us << ","
       << std_deviation_us << "," << entities_per_second << ","
       << peak_memory_usage << "," << cache_hit_ratio << ","
       << consistency_score << "," << scalability_factor;
    return ss.str();
}

std::string ECSBenchmarkResult::to_json() const {
    std::ostringstream ss;
    ss << "{\n";
    ss << "  \"test_name\": \"" << test_name << "\",\n";
    ss << "  \"category\": \"" << category_to_string(category) << "\",\n";
    ss << "  \"architecture_type\": \"" << architecture_to_string(architecture_type) << "\",\n";
    ss << "  \"entity_count\": " << entity_count << ",\n";
    ss << "  \"average_time_us\": " << std::fixed << std::setprecision(3) << average_time_us << ",\n";
    ss << "  \"entities_per_second\": " << entities_per_second << ",\n";
    ss << "  \"memory_efficiency\": " << memory_efficiency << ",\n";
    ss << "  \"consistency_score\": " << consistency_score << "\n";
    ss << "}";
    return ss.str();
}

//=============================================================================
// Individual Benchmark Test Implementations
//=============================================================================

std::unique_ptr<ecs::Registry> IECSBenchmarkTest::create_test_registry(
    ECSArchitectureType architecture, const ECSBenchmarkConfig& config) {
    
    // Configure allocator based on architecture preferences
    ecs::AllocatorConfig alloc_config;
    switch (architecture) {
        case ECSArchitectureType::Archetype_SoA:
            alloc_config = ecs::AllocatorConfig::create_performance_optimized();
            break;
        case ECSArchitectureType::SparseSet:
            alloc_config = ecs::AllocatorConfig::create_balanced();
            break;
        default:
            alloc_config = ecs::AllocatorConfig::create_default();
            break;
    }
    
    alloc_config.arena_size = config.arena_size;
    alloc_config.enable_memory_tracking = config.enable_memory_tracking;
    
    return std::make_unique<ecs::Registry>(alloc_config, "BenchmarkRegistry");
}

void IECSBenchmarkTest::populate_test_entities(ecs::Registry& registry, u32 count) {
    std::mt19937 rng(42); // Fixed seed for reproducibility
    std::uniform_real_distribution<f32> pos_dist(-1000.0f, 1000.0f);
    std::uniform_real_distribution<f32> vel_dist(-100.0f, 100.0f);
    std::uniform_real_distribution<f32> health_dist(50.0f, 100.0f);
    
    for (u32 i = 0; i < count; ++i) {
        Entity entity = registry.create_entity();
        
        // Add basic components to most entities
        registry.add_component(entity, BenchmarkPosition{
            pos_dist(rng), pos_dist(rng), pos_dist(rng)
        });
        
        registry.add_component(entity, BenchmarkVelocity{
            vel_dist(rng), vel_dist(rng), vel_dist(rng)
        });
        
        // Add health to 80% of entities
        if (i % 5 != 0) {
            registry.add_component(entity, BenchmarkHealth{health_dist(rng)});
        }
        
        // Add transform to 60% of entities
        if (i % 3 != 0) {
            registry.add_component(entity, BenchmarkTransform{});
        }
        
        // Add large component to 20% of entities for memory pressure testing
        if (i % 5 == 0) {
            registry.add_component(entity, BenchmarkLargeComponent{});
        }
    }
}

//=============================================================================
// EntityLifecycleBenchmark Implementation
//=============================================================================

ECSBenchmarkResult EntityLifecycleBenchmark::run_benchmark(
    ECSArchitectureType architecture, u32 entity_count, const ECSBenchmarkConfig& config) {
    
    ECSBenchmarkResult result;
    result.test_name = get_name();
    result.category = get_category();
    result.architecture_type = architecture;
    result.entity_count = entity_count;
    result.config = config;
    
    auto registry = create_test_registry(architecture, config);
    
    // Warmup
    for (u32 i = 0; i < config.warmup_iterations; ++i) {
        std::vector<Entity> entities;
        for (u32 j = 0; j < std::min(entity_count / 10, 100u); ++j) {
            entities.push_back(registry->create_entity());
        }
        for (Entity e : entities) {
            registry->destroy_entity(e);
        }
    }
    
    // Measure entity creation
    auto creation_times = measure_execution_times([&]() {
        std::vector<Entity> entities;
        entities.reserve(entity_count);
        for (u32 i = 0; i < entity_count; ++i) {
            entities.push_back(registry->create_entity());
        }
        
        // Clean up for next iteration
        for (Entity e : entities) {
            registry->destroy_entity(e);
        }
    }, config.iterations);
    
    result.raw_timings = creation_times;
    result.calculate_statistics();
    
    LOG_INFO("EntityLifecycle benchmark completed: {} entities, {:.2f}μs avg", 
             entity_count, result.average_time_us);
    
    return result;
}

//=============================================================================
// ComponentManipulationBenchmark Implementation
//=============================================================================

ECSBenchmarkResult ComponentManipulationBenchmark::run_benchmark(
    ECSArchitectureType architecture, u32 entity_count, const ECSBenchmarkConfig& config) {
    
    ECSBenchmarkResult result;
    result.test_name = get_name();
    result.category = get_category();
    result.architecture_type = architecture;
    result.entity_count = entity_count;
    result.config = config;
    
    auto registry = create_test_registry(architecture, config);
    
    // Pre-create entities
    std::vector<Entity> entities;
    entities.reserve(entity_count);
    for (u32 i = 0; i < entity_count; ++i) {
        entities.push_back(registry->create_entity());
    }
    
    // Warmup
    for (u32 i = 0; i < config.warmup_iterations; ++i) {
        for (u32 j = 0; j < std::min(entity_count / 10, 100u); ++j) {
            registry->add_component(entities[j], BenchmarkPosition{});
            registry->remove_component<BenchmarkPosition>(entities[j]);
        }
    }
    
    // Measure component addition and removal
    auto manipulation_times = measure_execution_times([&]() {
        // Add components
        for (u32 i = 0; i < entity_count; ++i) {
            registry->add_component(entities[i], BenchmarkPosition{});
            registry->add_component(entities[i], BenchmarkVelocity{});
        }
        
        // Remove components
        for (u32 i = 0; i < entity_count; ++i) {
            registry->remove_component<BenchmarkVelocity>(entities[i]);
            registry->remove_component<BenchmarkPosition>(entities[i]);
        }
    }, config.iterations);
    
    result.raw_timings = manipulation_times;
    result.calculate_statistics();
    
    // Calculate components per second (2 additions + 2 removals per entity)
    result.components_per_second = (entity_count * 4) / (result.average_time_us / 1000000.0);
    
    return result;
}

//=============================================================================
// QueryIterationBenchmark Implementation
//=============================================================================

ECSBenchmarkResult QueryIterationBenchmark::run_benchmark(
    ECSArchitectureType architecture, u32 entity_count, const ECSBenchmarkConfig& config) {
    
    ECSBenchmarkResult result;
    result.test_name = get_name();
    result.category = get_category();
    result.architecture_type = architecture;
    result.entity_count = entity_count;
    result.config = config;
    
    auto registry = create_test_registry(architecture, config);
    populate_test_entities(*registry, entity_count);
    
    // Warmup
    for (u32 i = 0; i < config.warmup_iterations; ++i) {
        u32 count = 0;
        registry->view<BenchmarkPosition, BenchmarkVelocity>().each(
            [&count](Entity, BenchmarkPosition& pos, BenchmarkVelocity& vel) {
                pos.x += vel.x * 0.016f; // Simulate 60fps update
                pos.y += vel.y * 0.016f;
                pos.z += vel.z * 0.016f;
                ++count;
            });
    }
    
    // Measure query iteration
    auto iteration_times = measure_execution_times([&]() {
        u32 processed = 0;
        
        // Test multiple query patterns
        registry->view<BenchmarkPosition, BenchmarkVelocity>().each(
            [&processed](Entity, BenchmarkPosition& pos, BenchmarkVelocity& vel) {
                pos.x += vel.x * 0.016f;
                pos.y += vel.y * 0.016f;
                pos.z += vel.z * 0.016f;
                ++processed;
            });
            
        registry->view<BenchmarkHealth>().each(
            [&processed](Entity, BenchmarkHealth& health) {
                health.current = std::min(health.max, health.current + 0.1f);
                ++processed;
            });
            
        // Force computation to prevent optimization
        volatile u32 dummy = processed;
        (void)dummy;
    }, config.iterations);
    
    result.raw_timings = iteration_times;
    result.calculate_statistics();
    
    return result;
}

//=============================================================================
// RandomAccessBenchmark Implementation
//=============================================================================

ECSBenchmarkResult RandomAccessBenchmark::run_benchmark(
    ECSArchitectureType architecture, u32 entity_count, const ECSBenchmarkConfig& config) {
    
    ECSBenchmarkResult result;
    result.test_name = get_name();
    result.category = get_category();
    result.architecture_type = architecture;
    result.entity_count = entity_count;
    result.config = config;
    
    auto registry = create_test_registry(architecture, config);
    populate_test_entities(*registry, entity_count);
    
    // Create random access pattern
    std::vector<Entity> entities;
    registry->view<BenchmarkPosition>().each(
        [&entities](Entity entity, const BenchmarkPosition&) {
            entities.push_back(entity);
        });
    
    std::mt19937 rng(42);
    std::shuffle(entities.begin(), entities.end(), rng);
    
    // Warmup with random access
    for (u32 i = 0; i < config.warmup_iterations; ++i) {
        for (u32 j = 0; j < std::min(static_cast<u32>(entities.size()), 100u); ++j) {
            if (registry->has_component<BenchmarkPosition>(entities[j])) {
                auto& pos = registry->get_component<BenchmarkPosition>(entities[j]);
                pos.x += 1.0f;
            }
        }
    }
    
    // Measure random access performance
    auto access_times = measure_execution_times([&]() {
        f32 sum = 0.0f;
        
        for (Entity entity : entities) {
            if (registry->has_component<BenchmarkPosition>(entity)) {
                const auto& pos = registry->get_component<BenchmarkPosition>(entity);
                sum += pos.x + pos.y + pos.z;
            }
            
            if (registry->has_component<BenchmarkVelocity>(entity)) {
                const auto& vel = registry->get_component<BenchmarkVelocity>(entity);
                sum += vel.x + vel.y + vel.z;
            }
        }
        
        // Force computation to prevent optimization
        volatile f32 dummy = sum;
        (void)dummy;
    }, config.iterations);
    
    result.raw_timings = access_times;
    result.calculate_statistics();
    
    return result;
}

//=============================================================================
// ECSPerformanceBenchmarker Implementation
//=============================================================================

ECSPerformanceBenchmarker::ECSPerformanceBenchmarker(const ECSBenchmarkConfig& config)
    : config_(config) {
    
    // Initialize memory tracking if enabled
    if (config_.enable_memory_tracking) {
        memory_tracker_ = std::make_unique<memory::MemoryTracker>();
    }
    
    // Initialize sparse set analysis if enabled
    if (config_.analyze_cache_behavior) {
        sparse_set_analyzer_ = std::make_unique<visualization::SparseSetAnalyzer>();
    }
    
    initialize_standard_tests();
    
    LOG_INFO("ECS Performance Benchmarker initialized with {} tests", tests_.size());
}

void ECSPerformanceBenchmarker::initialize_standard_tests() {
    tests_.push_back(std::make_unique<EntityLifecycleBenchmark>());
    tests_.push_back(std::make_unique<ComponentManipulationBenchmark>());
    tests_.push_back(std::make_unique<QueryIterationBenchmark>());
    tests_.push_back(std::make_unique<RandomAccessBenchmark>());
    
    // Add conditional tests based on configuration
    if (config_.enable_archetype_migration) {
        tests_.push_back(std::make_unique<ArchetypeMigrationBenchmark>());
    }
    
    if (config_.test_multi_threading) {
        tests_.push_back(std::make_unique<MultiThreadingBenchmark>());
    }
    
    if (config_.enable_stress_testing) {
        tests_.push_back(std::make_unique<MemoryPressureBenchmark>());
    }
    
    if (config_.test_physics_integration) {
        tests_.push_back(std::make_unique<PhysicsIntegrationBenchmark>());
    }
    
    if (config_.test_rendering_integration) {
        tests_.push_back(std::make_unique<RenderingIntegrationBenchmark>());
    }
}

void ECSPerformanceBenchmarker::run_all_benchmarks() {
    LOG_INFO("Starting comprehensive ECS benchmark suite");
    LOG_INFO("Configuration: {} architectures, {} entity counts, {} tests",
             config_.architectures.size(), config_.entity_counts.size(), tests_.size());
    
    is_running_.store(true);
    progress_.store(0.0);
    
    results_.clear();
    
    usize total_tests = config_.architectures.size() * config_.entity_counts.size() * tests_.size();
    usize completed_tests = 0;
    
    auto benchmark_start = std::chrono::high_resolution_clock::now();
    
    for (ECSArchitectureType architecture : config_.architectures) {
        LOG_INFO("Testing architecture: {}", architecture_to_string(architecture));
        
        for (u32 entity_count : config_.entity_counts) {
            LOG_INFO("  Entity count: {}", entity_count);
            
            for (auto& test : tests_) {
                if (!test->supports_architecture(architecture)) {
                    ++completed_tests;
                    continue;
                }
                
                if (!is_running_.load()) {
                    LOG_INFO("Benchmark cancelled by user");
                    return;
                }
                
                execute_single_test(*test, architecture, entity_count);
                
                ++completed_tests;
                update_progress(static_cast<f64>(completed_tests) / total_tests);
            }
        }
    }
    
    auto benchmark_end = std::chrono::high_resolution_clock::now();
    f64 total_time = std::chrono::duration<f64>(benchmark_end - benchmark_start).count();
    
    analyze_results();
    
    is_running_.store(false);
    progress_.store(1.0);
    
    LOG_INFO("ECS benchmark suite completed in {:.2f} seconds", total_time);
    LOG_INFO("Generated {} benchmark results", results_.size());
    
    // Generate reports if configured
    if (config_.generate_comparative_report) {
        export_comparative_report(config_.output_directory + "/ecs_comparative_report.txt");
    }
    
    if (config_.export_raw_data) {
        export_results_csv(config_.output_directory + "/ecs_benchmark_results.csv");
        export_results_json(config_.output_directory + "/ecs_benchmark_results.json");
    }
}

void ECSPerformanceBenchmarker::execute_single_test(IECSBenchmarkTest& test,
                                                   ECSArchitectureType architecture,
                                                   u32 entity_count) {
    log_benchmark_start(test.get_name(), architecture, entity_count);
    
    try {
        // Start memory tracking if enabled
        if (memory_tracker_) {
            memory_tracker_->reset();
        }
        
        // Run the benchmark
        ECSBenchmarkResult result = test.run_benchmark(architecture, entity_count, config_);
        
        // Collect memory statistics
        if (memory_tracker_) {
            auto memory_stats = memory_tracker_->get_current_stats();
            result.peak_memory_usage = memory_stats.peak_allocation;
            result.average_memory_usage = memory_stats.current_allocation;
            result.allocation_count = memory_stats.total_allocations;
        }
        
        // Add metadata
        result.platform_info = get_platform_info();
        result.timestamp = get_timestamp();
        
        std::lock_guard<std::mutex> lock(results_mutex_);
        results_.push_back(result);
        
        log_benchmark_result(result);
        
    } catch (const std::exception& e) {
        LOG_ERROR("Benchmark failed: {} - {}", test.get_name(), e.what());
        
        ECSBenchmarkResult failed_result;
        failed_result.test_name = test.get_name();
        failed_result.architecture_type = architecture;
        failed_result.entity_count = entity_count;
        failed_result.is_valid = false;
        failed_result.error_message = e.what();
        
        std::lock_guard<std::mutex> lock(results_mutex_);
        results_.push_back(failed_result);
    }
}

void ECSPerformanceBenchmarker::analyze_results() {
    if (results_.empty()) return;
    
    LOG_INFO("Analyzing benchmark results...");
    
    architecture_comparisons_.clear();
    
    // Group results by architecture
    std::unordered_map<ECSArchitectureType, std::vector<ECSBenchmarkResult*>> by_architecture;
    for (auto& result : results_) {
        if (result.is_valid) {
            by_architecture[result.architecture_type].push_back(&result);
        }
    }
    
    // Analyze each architecture
    for (const auto& [architecture, arch_results] : by_architecture) {
        ArchitectureComparison comparison;
        comparison.architecture = architecture;
        
        // Calculate overall score (weighted average of normalized performance)
        f64 total_score = 0.0;
        u32 valid_scores = 0;
        
        // Group by test name for analysis
        std::unordered_map<std::string, std::vector<ECSBenchmarkResult*>> by_test;
        for (auto* result : arch_results) {
            by_test[result->test_name].push_back(result);
        }
        
        for (const auto& [test_name, test_results] : by_test) {
            if (test_results.empty()) continue;
            
            // Calculate average performance for this test
            f64 avg_performance = 0.0;
            for (auto* result : test_results) {
                avg_performance += result->entities_per_second;
            }
            avg_performance /= test_results.size();
            
            comparison.test_scores[test_name] = avg_performance;
            
            // Normalize score (simple normalization, could be improved)
            f64 normalized_score = std::log10(avg_performance + 1.0) / 10.0;
            total_score += normalized_score;
            ++valid_scores;
        }
        
        comparison.overall_score = valid_scores > 0 ? total_score / valid_scores : 0.0;
        
        generate_insights_for_architecture(comparison);
        architecture_comparisons_.push_back(comparison);
    }
    
    // Sort by overall score (descending)
    std::sort(architecture_comparisons_.begin(), architecture_comparisons_.end(),
             [](const ArchitectureComparison& a, const ArchitectureComparison& b) {
                 return a.overall_score > b.overall_score;
             });
    
    LOG_INFO("Analysis complete. {} architectures analyzed.", architecture_comparisons_.size());
}

void ECSPerformanceBenchmarker::generate_insights_for_architecture(ArchitectureComparison& comparison) {
    // Find strengths and weaknesses based on test performance
    std::vector<std::pair<std::string, f64>> test_scores;
    for (const auto& [test, score] : comparison.test_scores) {
        test_scores.emplace_back(test, score);
    }
    
    // Sort by performance
    std::sort(test_scores.begin(), test_scores.end(),
             [](const auto& a, const auto& b) { return a.second > b.second; });
    
    // Top 3 are strengths
    usize strength_count = std::min(test_scores.size(), 3uz);
    for (usize i = 0; i < strength_count; ++i) {
        comparison.strengths.push_back("Strong " + test_scores[i].first + " performance");
    }
    
    // Bottom 2 are weaknesses
    usize weakness_start = test_scores.size() > 2 ? test_scores.size() - 2 : 0;
    for (usize i = weakness_start; i < test_scores.size(); ++i) {
        comparison.weaknesses.push_back("Weak " + test_scores[i].first + " performance");
    }
}

std::string ECSPerformanceBenchmarker::generate_comparative_report() const {
    std::ostringstream report;
    
    report << "=== ECS Architecture Performance Comparison Report ===\n\n";
    
    if (architecture_comparisons_.empty()) {
        report << "No analysis data available.\n";
        return report.str();
    }
    
    // Executive Summary
    report << "Executive Summary:\n";
    report << "-----------------\n";
    
    const auto& best = architecture_comparisons_.front();
    report << "Best Overall Performance: " << architecture_to_string(best.architecture)
           << " (Score: " << std::fixed << std::setprecision(3) << best.overall_score << ")\n";
    
    if (architecture_comparisons_.size() > 1) {
        const auto& worst = architecture_comparisons_.back();
        f64 improvement = (best.overall_score - worst.overall_score) / worst.overall_score * 100.0;
        report << "Performance Range: " << std::fixed << std::setprecision(1) << improvement 
               << "% difference between best and worst\n";
    }
    
    report << "\n";
    
    // Detailed Architecture Analysis
    for (const auto& comparison : architecture_comparisons_) {
        report << "Architecture: " << architecture_to_string(comparison.architecture) << "\n";
        report << "Overall Score: " << std::fixed << std::setprecision(3) << comparison.overall_score << "\n";
        
        if (!comparison.strengths.empty()) {
            report << "Strengths:\n";
            for (const auto& strength : comparison.strengths) {
                report << "  + " << strength << "\n";
            }
        }
        
        if (!comparison.weaknesses.empty()) {
            report << "Weaknesses:\n";
            for (const auto& weakness : comparison.weaknesses) {
                report << "  - " << weakness << "\n";
            }
        }
        
        report << "\n";
    }
    
    // Test-by-Test Comparison
    report << "Detailed Test Results:\n";
    report << "---------------------\n";
    
    // Get all unique test names
    std::set<std::string> all_tests;
    for (const auto& comparison : architecture_comparisons_) {
        for (const auto& [test, score] : comparison.test_scores) {
            all_tests.insert(test);
        }
    }
    
    for (const std::string& test_name : all_tests) {
        report << test_name << ":\n";
        
        for (const auto& comparison : architecture_comparisons_) {
            auto it = comparison.test_scores.find(test_name);
            if (it != comparison.test_scores.end()) {
                report << "  " << std::left << std::setw(20) << architecture_to_string(comparison.architecture)
                       << ": " << std::fixed << std::setprecision(0) << it->second << " entities/sec\n";
            }
        }
        report << "\n";
    }
    
    return report.str();
}

//=============================================================================
// Utility Functions
//=============================================================================

std::string ECSPerformanceBenchmarker::architecture_to_string(ECSArchitectureType architecture) {
    switch (architecture) {
        case ECSArchitectureType::Archetype_SoA: return "Archetype (SoA)";
        case ECSArchitectureType::Archetype_AoS: return "Archetype (AoS)";
        case ECSArchitectureType::ComponentArray: return "Component Array";
        case ECSArchitectureType::SparseSet: return "Sparse Set";
        case ECSArchitectureType::Hybrid: return "Hybrid";
        default: return "Unknown";
    }
}

std::string ECSPerformanceBenchmarker::category_to_string(ECSBenchmarkCategory category) {
    switch (category) {
        case ECSBenchmarkCategory::Architecture: return "Architecture";
        case ECSBenchmarkCategory::Memory: return "Memory";
        case ECSBenchmarkCategory::Scaling: return "Scaling";
        case ECSBenchmarkCategory::Systems: return "Systems";
        case ECSBenchmarkCategory::Integration: return "Integration";
        case ECSBenchmarkCategory::Stress: return "Stress";
        case ECSBenchmarkCategory::Regression: return "Regression";
        default: return "Unknown";
    }
}

void ECSPerformanceBenchmarker::log_benchmark_start(const std::string& test_name,
                                                   ECSArchitectureType architecture,
                                                   u32 entity_count) const {
    LOG_INFO("    Running {}: {} architecture, {} entities", 
             test_name, architecture_to_string(architecture), entity_count);
}

void ECSPerformanceBenchmarker::log_benchmark_result(const ECSBenchmarkResult& result) const {
    LOG_INFO("      Result: {:.2f}μs avg, {:.0f} entities/sec, consistency: {:.3f}",
             result.average_time_us, result.entities_per_second, result.consistency_score);
}

std::string ECSPerformanceBenchmarker::get_timestamp() const {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    std::ostringstream ss;
    ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
    return ss.str();
}

std::string ECSPerformanceBenchmarker::get_platform_info() const {
    std::ostringstream info;
    info << "Threads: " << std::thread::hardware_concurrency();
    // Could add more platform-specific information here
    return info.str();
}

void ECSPerformanceBenchmarker::export_results_csv(const std::string& filename) const {
    std::ofstream file(filename);
    if (!file.is_open()) {
        LOG_ERROR("Failed to open CSV file: {}", filename);
        return;
    }
    
    // CSV Header
    file << "Test,Category,Architecture,EntityCount,AvgTimeUs,MinTimeUs,MaxTimeUs,StdDevUs,"
         << "EntitiesPerSec,PeakMemoryBytes,CacheHitRatio,ConsistencyScore,ScalabilityFactor\n";
    
    std::lock_guard<std::mutex> lock(results_mutex_);
    for (const auto& result : results_) {
        if (result.is_valid) {
            file << result.to_csv_row() << "\n";
        }
    }
    
    LOG_INFO("Exported CSV results to: {}", filename);
}

void ECSPerformanceBenchmarker::export_comparative_report(const std::string& filename) const {
    std::ofstream file(filename);
    if (!file.is_open()) {
        LOG_ERROR("Failed to open report file: {}", filename);
        return;
    }
    
    file << generate_comparative_report();
    
    LOG_INFO("Exported comparative report to: {}", filename);
}

//=============================================================================
// Factory Implementation
//=============================================================================

std::unique_ptr<ECSPerformanceBenchmarker> ECSBenchmarkSuiteFactory::create_quick_suite() {
    auto config = ECSBenchmarkConfig::create_quick();
    return std::make_unique<ECSPerformanceBenchmarker>(config);
}

std::unique_ptr<ECSPerformanceBenchmarker> ECSBenchmarkSuiteFactory::create_comprehensive_suite() {
    auto config = ECSBenchmarkConfig::create_comprehensive();
    return std::make_unique<ECSPerformanceBenchmarker>(config);
}

std::unique_ptr<ECSPerformanceBenchmarker> ECSBenchmarkSuiteFactory::create_educational_suite() {
    auto config = ECSBenchmarkConfig::create_quick();
    config.generate_comparative_report = true;
    config.generate_visualization_data = true;
    config.export_raw_data = true;
    return std::make_unique<ECSPerformanceBenchmarker>(config);
}

} // namespace ecscope::performance::ecs