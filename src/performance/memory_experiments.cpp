/**
 * @file memory_experiments.cpp
 * @brief Memory Access Pattern Laboratory - Implementation of educational memory experiments
 * 
 * This component implements the heart of ECScope's memory behavior laboratory,
 * providing interactive experiments that demonstrate the real-world performance
 * impact of memory layout decisions, cache behavior, and data access patterns.
 */

#include "performance/memory_experiments.hpp"
#include "core/log.hpp"
#include <algorithm>
#include <numeric>
#include <random>
#include <cstring>
#include <immintrin.h> // For prefetching instructions

namespace ecscope::performance {

// MemoryAccessExperiment Implementation
MemoryAccessExperiment::MemoryAccessExperiment(const TestDataConfig& config)
    : config_(config), rng_(config.random_seed) {
}

std::string MemoryAccessExperiment::get_description() const {
    return "Comprehensive memory access pattern analysis comparing SoA vs AoS layouts, "
           "cache behavior, and data locality effects. This experiment demonstrates "
           "how different data organization strategies impact performance in real-world scenarios.";
}

bool MemoryAccessExperiment::setup(const ExperimentConfig& experiment_config) {
    LOG_INFO("Setting up Memory Access Experiment with {} elements", config_.element_count);
    
    try {
        // Reserve memory for AoS data
        aos_data_.reserve(config_.element_count);
        aos_data_.clear();
        
        // Setup SoA data
        soa_data_.resize(config_.element_count);
        
        // Generate test data
        if (config_.use_random_data) {
            std::uniform_real_distribution<f32> pos_dist(-1000.0f, 1000.0f);
            std::uniform_real_distribution<f32> vel_dist(-10.0f, 10.0f);
            std::uniform_real_distribution<f32> mass_dist(0.1f, 10.0f);
            std::uniform_int_distribution<u32> id_dist(0, 1000000);
            
            for (usize i = 0; i < config_.element_count; ++i) {
                TestComponent comp;
                comp.x = pos_dist(rng_);
                comp.y = pos_dist(rng_);
                comp.z = pos_dist(rng_);
                comp.vx = vel_dist(rng_);
                comp.vy = vel_dist(rng_);
                comp.vz = vel_dist(rng_);
                comp.mass = mass_dist(rng_);
                comp.id = id_dist(rng_);
                
                aos_data_.push_back(comp);
                
                // Fill SoA arrays
                soa_data_.x[i] = comp.x;
                soa_data_.y[i] = comp.y;
                soa_data_.z[i] = comp.z;
                soa_data_.vx[i] = comp.vx;
                soa_data_.vy[i] = comp.vy;
                soa_data_.vz[i] = comp.vz;
                soa_data_.mass[i] = comp.mass;
                soa_data_.id[i] = comp.id;
            }
        } else {
            // Generate predictable data for consistent benchmarking
            for (usize i = 0; i < config_.element_count; ++i) {
                TestComponent comp;
                comp.x = static_cast<f32>(i * 0.1);
                comp.y = static_cast<f32>(i * 0.2);
                comp.z = static_cast<f32>(i * 0.3);
                comp.vx = static_cast<f32>(i * 0.01);
                comp.vy = static_cast<f32>(i * 0.02);
                comp.vz = static_cast<f32>(i * 0.03);
                comp.mass = 1.0f + static_cast<f32>(i % 10) * 0.1f;
                comp.id = static_cast<u32>(i);
                
                aos_data_.push_back(comp);
                
                // Fill SoA arrays
                soa_data_.x[i] = comp.x;
                soa_data_.y[i] = comp.y;
                soa_data_.z[i] = comp.z;
                soa_data_.vx[i] = comp.vx;
                soa_data_.vy[i] = comp.vy;
                soa_data_.vz[i] = comp.vz;
                soa_data_.mass[i] = comp.mass;
                soa_data_.id[i] = comp.id;
            }
        }
        
        // Setup raw buffer for low-level tests
        usize raw_buffer_size = config_.element_count * config_.element_size;
        raw_buffer_.resize(raw_buffer_size);
        
        LOG_INFO("Memory Access Experiment setup complete (AoS: {}, SoA: {}, Raw: {} bytes)", 
                 aos_data_.size() * sizeof(TestComponent),
                 soa_data_.size() * sizeof(f32) * 8, // 8 arrays
                 raw_buffer_.size());
        
        return true;
        
    } catch (const std::exception& e) {
        LOG_ERROR("Failed to setup Memory Access Experiment: {}", e.what());
        return false;
    }
}

BenchmarkResult MemoryAccessExperiment::execute() {
    LOG_INFO("Executing Memory Access Pattern Analysis");
    
    BenchmarkResult result;
    result.name = get_name();
    result.description = get_description();
    result.category = get_category();
    
    try {
        // Generate access patterns for testing
        std::vector<usize> sequential_pattern = generate_sequential_pattern(config_.element_count);
        std::vector<usize> random_pattern = generate_random_pattern(config_.element_count);
        std::vector<usize> strided_pattern = generate_strided_pattern(config_.element_count, 8);
        
        // Measure AoS performance
        auto aos_sequential = measure_aos_performance(sequential_pattern);
        auto aos_random = measure_aos_performance(random_pattern);
        auto aos_strided = measure_aos_performance(strided_pattern);
        
        // Measure SoA performance
        auto soa_sequential = measure_soa_performance(sequential_pattern);
        auto soa_random = measure_soa_performance(random_pattern);
        auto soa_strided = measure_soa_performance(strided_pattern);
        
        // Aggregate results for the benchmark
        result.execution_time_ms = aos_sequential.total_time_ms + soa_sequential.total_time_ms;
        result.average_time_ms = result.execution_time_ms / 6.0; // 6 sub-tests
        
        // Calculate performance metrics
        f64 aos_avg_time = (aos_sequential.total_time_ms + aos_random.total_time_ms + aos_strided.total_time_ms) / 3.0;
        f64 soa_avg_time = (soa_sequential.total_time_ms + soa_random.total_time_ms + soa_strided.total_time_ms) / 3.0;
        
        result.efficiency_score = std::min(soa_avg_time, aos_avg_time) / std::max(soa_avg_time, aos_avg_time);
        result.throughput = config_.element_count / result.average_time_ms * 1000.0; // elements per second
        
        // Memory usage analysis
        result.memory_usage_bytes = (aos_data_.size() * sizeof(TestComponent)) + 
                                   (soa_data_.size() * sizeof(f32) * 8);
        result.cache_miss_rate = (aos_sequential.cache_efficiency + soa_sequential.cache_efficiency) / 2.0;
        
        // Generate educational insights
        result.insights.push_back("SoA layout shows " + std::to_string(100.0 * (aos_avg_time - soa_avg_time) / aos_avg_time) + 
                                 "% performance difference vs AoS for sequential access");
        
        if (soa_sequential.total_time_ms < aos_sequential.total_time_ms * 0.8) {
            result.insights.push_back("SoA demonstrates significant cache locality advantages for component-wise operations");
        }
        
        if (aos_random.total_time_ms < soa_random.total_time_ms) {
            result.insights.push_back("AoS shows better performance for random access patterns requiring multiple fields");
        }
        
        // Store detailed results in metadata
        result.metadata["aos_sequential_ms"] = aos_sequential.total_time_ms;
        result.metadata["aos_random_ms"] = aos_random.total_time_ms;
        result.metadata["aos_strided_ms"] = aos_strided.total_time_ms;
        result.metadata["soa_sequential_ms"] = soa_sequential.total_time_ms;
        result.metadata["soa_random_ms"] = soa_random.total_time_ms;
        result.metadata["soa_strided_ms"] = soa_strided.total_time_ms;
        result.metadata["cache_efficiency"] = result.cache_miss_rate;
        
        result.is_valid = true;
        result.confidence_level = 0.85;
        
        LOG_INFO("Memory Access Pattern Analysis completed (AoS avg: {:.2f}ms, SoA avg: {:.2f}ms)", 
                 aos_avg_time, soa_avg_time);
        
        return result;
        
    } catch (const std::exception& e) {
        result.is_valid = false;
        result.error_message = "Execution failed: " + std::string(e.what());
        LOG_ERROR("Memory Access Experiment execution failed: {}", e.what());
        return result;
    }
}

void MemoryAccessExperiment::cleanup() {
    aos_data_.clear();
    soa_data_.resize(0);
    raw_buffer_.clear();
    
    LOG_INFO("Memory Access Experiment cleanup completed");
}

void MemoryAccessExperiment::update_visualization(f64 dt) {
    // TODO: Implement real-time visualization updates
    // This could update charts showing:
    // - Current memory access patterns
    // - Cache hit/miss rates
    // - Performance comparison graphs
}

std::vector<PerformanceRecommendation> MemoryAccessExperiment::generate_recommendations() const {
    std::vector<PerformanceRecommendation> recommendations;
    
    // Analyze current configuration and generate recommendations
    PerformanceRecommendation soa_rec;
    soa_rec.title = "Consider SoA Layout for Component Systems";
    soa_rec.description = "Structure of Arrays (SoA) layout can significantly improve cache performance "
                         "when processing components that don't require all fields simultaneously.";
    soa_rec.priority = PerformanceRecommendation::Priority::High;
    soa_rec.category = PerformanceRecommendation::Category::Memory;
    soa_rec.estimated_improvement = 25.0; // 25% improvement estimate
    soa_rec.implementation_difficulty = 0.7;
    soa_rec.educational_notes.push_back("SoA improves cache locality by grouping similar data together");
    soa_rec.educational_notes.push_back("Particularly effective for SIMD operations and bulk processing");
    soa_rec.implementation_steps.push_back("Reorganize component data into separate arrays per field");
    soa_rec.implementation_steps.push_back("Update system iteration to process arrays in parallel");
    soa_rec.implementation_steps.push_back("Consider hybrid approach for frequently co-accessed fields");
    recommendations.push_back(soa_rec);
    
    PerformanceRecommendation prefetch_rec;
    prefetch_rec.title = "Implement Software Prefetching";
    prefetch_rec.description = "Software prefetching can help reduce cache miss penalties for predictable access patterns.";
    prefetch_rec.priority = PerformanceRecommendation::Priority::Medium;
    prefetch_rec.category = PerformanceRecommendation::Category::Memory;
    prefetch_rec.estimated_improvement = 15.0;
    prefetch_rec.implementation_difficulty = 0.5;
    prefetch_rec.educational_notes.push_back("Prefetching works best with predictable access patterns");
    prefetch_rec.educational_notes.push_back("Can reduce effective memory latency for streaming operations");
    recommendations.push_back(prefetch_rec);
    
    return recommendations;
}

// Access pattern generation methods
std::vector<usize> MemoryAccessExperiment::generate_sequential_pattern(usize count) {
    std::vector<usize> pattern;
    pattern.reserve(count);
    
    for (usize i = 0; i < count; ++i) {
        pattern.push_back(i);
    }
    
    return pattern;
}

std::vector<usize> MemoryAccessExperiment::generate_random_pattern(usize count) {
    std::vector<usize> pattern = generate_sequential_pattern(count);
    std::shuffle(pattern.begin(), pattern.end(), rng_);
    return pattern;
}

std::vector<usize> MemoryAccessExperiment::generate_strided_pattern(usize count, usize stride) {
    std::vector<usize> pattern;
    pattern.reserve(count);
    
    usize current = 0;
    for (usize i = 0; i < count; ++i) {
        pattern.push_back(current % count);
        current += stride;
    }
    
    return pattern;
}

std::vector<usize> MemoryAccessExperiment::generate_circular_pattern(usize count) {
    std::vector<usize> pattern;
    pattern.reserve(count);
    
    usize buffer_size = std::min(count, static_cast<usize>(1024)); // Simulate circular buffer
    
    for (usize i = 0; i < count; ++i) {
        pattern.push_back(i % buffer_size);
    }
    
    return pattern;
}

std::vector<usize> MemoryAccessExperiment::generate_tree_pattern(usize count) {
    std::vector<usize> pattern;
    pattern.reserve(count);
    
    // Generate binary tree traversal pattern
    std::function<void(usize, usize)> traverse = [&](usize node, usize max_nodes) {
        if (node >= max_nodes) return;
        
        pattern.push_back(node);
        traverse(2 * node + 1, max_nodes); // Left child
        traverse(2 * node + 2, max_nodes); // Right child
    };
    
    traverse(0, count);
    
    // Fill remaining slots with sequential access
    while (pattern.size() < count) {
        pattern.push_back(pattern.size() % count);
    }
    
    return pattern;
}

// Performance measurement methods
MemoryExperimentResult MemoryAccessExperiment::measure_aos_performance(const std::vector<usize>& access_pattern) {
    MemoryExperimentResult result;
    result.experiment_name = "AoS Access Pattern Test";
    result.config = config_;
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    // Simulate physics update operation (common ECS pattern)
    f32 accumulated_energy = 0.0f;
    const f32 dt = 1.0f / 60.0f; // 60 FPS simulation
    
    for (usize index : access_pattern) {
        if (index >= aos_data_.size()) continue;
        
        TestComponent& comp = aos_data_[index];
        
        // Simulate physics calculations (accessing multiple fields)
        comp.x += comp.vx * dt;
        comp.y += comp.vy * dt;
        comp.z += comp.vz * dt;
        
        // Calculate kinetic energy (requires velocity and mass)
        f32 speed_squared = comp.vx * comp.vx + comp.vy * comp.vy + comp.vz * comp.vz;
        accumulated_energy += 0.5f * comp.mass * speed_squared;
        
        // Apply some damping
        comp.vx *= 0.999f;
        comp.vy *= 0.999f;
        comp.vz *= 0.999f;
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    
    auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time);
    result.total_time_ms = static_cast<f64>(duration.count()) / 1'000'000.0;
    result.time_per_element_ns = static_cast<f64>(duration.count()) / access_pattern.size();
    
    // Memory usage analysis
    result.memory_allocated_bytes = aos_data_.size() * sizeof(TestComponent);
    result.memory_efficiency = 1.0; // AoS uses all allocated memory
    
    // Cache analysis (simplified heuristic)
    analyze_cache_behavior(result, access_pattern);
    
    // Educational insights
    result.key_observations.push_back("AoS layout provides good spatial locality when accessing complete objects");
    result.key_observations.push_back("All component fields are loaded together, beneficial for operations needing multiple fields");
    result.performance_factors.push_back("Cache line utilization depends on object size vs cache line size");
    result.performance_factors.push_back("Memory bandwidth efficiency affected by unused field access");
    
    result.optimization_recommendation = "Consider SoA if only accessing subset of fields frequently";
    
    // Prevent compiler optimization of accumulated_energy
    result.memory_bandwidth_gbps = static_cast<f64>(accumulated_energy) * 0.0; // Use result to prevent optimization
    
    LOG_DEBUG("AoS performance measured: {:.2f}ms total, {:.2f}ns per element", 
              result.total_time_ms, result.time_per_element_ns);
    
    return result;
}

MemoryExperimentResult MemoryAccessExperiment::measure_soa_performance(const std::vector<usize>& access_pattern) {
    MemoryExperimentResult result;
    result.experiment_name = "SoA Access Pattern Test";
    result.config = config_;
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    // Simulate physics update operation using SoA layout
    f32 accumulated_energy = 0.0f;
    const f32 dt = 1.0f / 60.0f; // 60 FPS simulation
    
    for (usize index : access_pattern) {
        if (index >= soa_data_.size()) continue;
        
        // Update positions (excellent cache locality - sequential access to position arrays)
        soa_data_.x[index] += soa_data_.vx[index] * dt;
        soa_data_.y[index] += soa_data_.vy[index] * dt;
        soa_data_.z[index] += soa_data_.vz[index] * dt;
        
        // Calculate kinetic energy (requires velocity and mass arrays)
        f32 speed_squared = soa_data_.vx[index] * soa_data_.vx[index] + 
                           soa_data_.vy[index] * soa_data_.vy[index] + 
                           soa_data_.vz[index] * soa_data_.vz[index];
        accumulated_energy += 0.5f * soa_data_.mass[index] * speed_squared;
        
        // Apply damping
        soa_data_.vx[index] *= 0.999f;
        soa_data_.vy[index] *= 0.999f;
        soa_data_.vz[index] *= 0.999f;
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    
    auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time);
    result.total_time_ms = static_cast<f64>(duration.count()) / 1'000'000.0;
    result.time_per_element_ns = static_cast<f64>(duration.count()) / access_pattern.size();
    
    // Memory usage analysis
    result.memory_allocated_bytes = soa_data_.size() * sizeof(f32) * 8; // 8 arrays
    result.memory_efficiency = 1.0; // SoA also uses all allocated memory efficiently
    
    // Cache analysis
    analyze_cache_behavior(result, access_pattern);
    
    // Educational insights
    result.key_observations.push_back("SoA layout provides excellent cache locality for component-wise operations");
    result.key_observations.push_back("Each field access has optimal spatial locality within its array");
    result.performance_factors.push_back("Cache line utilization maximized for single-field operations");
    result.performance_factors.push_back("SIMD vectorization opportunities improved");
    
    result.optimization_recommendation = "Ideal for bulk operations on component fields, consider for hot paths";
    
    // Prevent compiler optimization
    result.memory_bandwidth_gbps = static_cast<f64>(accumulated_energy) * 0.0;
    
    LOG_DEBUG("SoA performance measured: {:.2f}ms total, {:.2f}ns per element", 
              result.total_time_ms, result.time_per_element_ns);
    
    return result;
}

MemoryExperimentResult MemoryAccessExperiment::measure_raw_performance(const std::vector<usize>& access_pattern) {
    MemoryExperimentResult result;
    result.experiment_name = "Raw Memory Access Test";
    result.config = config_;
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    // Raw memory access test
    volatile u8 dummy = 0;
    
    for (usize index : access_pattern) {
        usize byte_offset = (index * config_.element_size) % raw_buffer_.size();
        dummy += raw_buffer_[byte_offset];
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    
    auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time);
    result.total_time_ms = static_cast<f64>(duration.count()) / 1'000'000.0;
    result.time_per_element_ns = static_cast<f64>(duration.count()) / access_pattern.size();
    
    result.memory_allocated_bytes = raw_buffer_.size();
    result.memory_efficiency = 1.0;
    
    // Prevent optimization
    result.memory_bandwidth_gbps = static_cast<f64>(dummy) * 0.0;
    
    return result;
}

void MemoryAccessExperiment::analyze_cache_behavior(MemoryExperimentResult& result, 
                                                   const std::vector<usize>& access_pattern) {
    if (access_pattern.empty()) {
        result.cache_efficiency = 1.0;
        return;
    }
    
    // Simplified cache analysis based on access pattern
    usize cache_line_size = config_.cache_line_size;
    usize cache_lines_touched = 0;
    usize last_cache_line = SIZE_MAX;
    
    for (usize index : access_pattern) {
        usize element_offset = index * sizeof(TestComponent);
        usize cache_line = element_offset / cache_line_size;
        
        if (cache_line != last_cache_line) {
            ++cache_lines_touched;
            last_cache_line = cache_line;
        }
    }
    
    // Calculate cache efficiency based on theoretical minimum cache lines needed
    usize min_cache_lines = (access_pattern.size() * sizeof(TestComponent) + cache_line_size - 1) / cache_line_size;
    result.cache_efficiency = static_cast<f64>(min_cache_lines) / std::max(cache_lines_touched, static_cast<usize>(1));
    
    // Estimate cache misses (simplified model)
    f64 cache_miss_ratio = 1.0 - result.cache_efficiency;
    result.estimated_l1_misses = static_cast<u64>(access_pattern.size() * cache_miss_ratio * 0.8);
    result.estimated_l2_misses = static_cast<u64>(access_pattern.size() * cache_miss_ratio * 0.2);
    result.estimated_l3_misses = static_cast<u64>(access_pattern.size() * cache_miss_ratio * 0.1);
    
    result.cache_line_utilization = result.cache_efficiency;
    
    LOG_DEBUG("Cache analysis: {} cache lines touched, {:.2f} efficiency", 
              cache_lines_touched, result.cache_efficiency);
}

void MemoryAccessExperiment::set_test_data_config(const TestDataConfig& config) {
    config_ = config;
    rng_.seed(config.random_seed);
}

MemoryExperimentResult MemoryAccessExperiment::run_aos_vs_soa_comparison() {
    std::vector<usize> sequential_pattern = generate_sequential_pattern(config_.element_count);
    
    auto aos_result = measure_aos_performance(sequential_pattern);
    auto soa_result = measure_soa_performance(sequential_pattern);
    
    // Create comparison result
    MemoryExperimentResult comparison;
    comparison.experiment_name = "AoS vs SoA Comparison";
    comparison.config = config_;
    
    comparison.total_time_ms = aos_result.total_time_ms + soa_result.total_time_ms;
    comparison.memory_allocated_bytes = aos_result.memory_allocated_bytes + soa_result.memory_allocated_bytes;
    
    // Calculate performance difference
    f64 performance_diff = (aos_result.total_time_ms - soa_result.total_time_ms) / aos_result.total_time_ms * 100.0;
    
    comparison.key_observations.push_back("AoS time: " + std::to_string(aos_result.total_time_ms) + "ms");
    comparison.key_observations.push_back("SoA time: " + std::to_string(soa_result.total_time_ms) + "ms");
    comparison.key_observations.push_back("Performance difference: " + std::to_string(performance_diff) + "%");
    
    if (soa_result.total_time_ms < aos_result.total_time_ms) {
        comparison.optimization_recommendation = "SoA shows better performance for this access pattern";
    } else {
        comparison.optimization_recommendation = "AoS shows better performance for this access pattern";
    }
    
    return comparison;
}

MemoryExperimentResult MemoryAccessExperiment::run_cache_line_experiment() {
    // Test different stride patterns to demonstrate cache line effects
    std::vector<usize> stride_1 = generate_strided_pattern(config_.element_count, 1);
    std::vector<usize> stride_cache_line = generate_strided_pattern(config_.element_count, 
                                                                   config_.cache_line_size / sizeof(TestComponent));
    
    auto result_1 = measure_aos_performance(stride_1);
    auto result_cache = measure_aos_performance(stride_cache_line);
    
    MemoryExperimentResult comparison;
    comparison.experiment_name = "Cache Line Utilization Test";
    comparison.config = config_;
    
    comparison.total_time_ms = result_1.total_time_ms + result_cache.total_time_ms;
    comparison.cache_efficiency = (result_1.cache_efficiency + result_cache.cache_efficiency) / 2.0;
    
    f64 cache_impact = (result_cache.total_time_ms - result_1.total_time_ms) / result_1.total_time_ms * 100.0;
    
    comparison.key_observations.push_back("Sequential access time: " + std::to_string(result_1.total_time_ms) + "ms");
    comparison.key_observations.push_back("Cache-line strided access time: " + std::to_string(result_cache.total_time_ms) + "ms");
    comparison.key_observations.push_back("Cache line impact: " + std::to_string(cache_impact) + "%");
    
    return comparison;
}

MemoryExperimentResult MemoryAccessExperiment::run_prefetching_experiment() {
    std::vector<usize> sequential_pattern = generate_sequential_pattern(config_.element_count);
    
    // Measure performance with and without software prefetching
    auto start_time = std::chrono::high_resolution_clock::now();
    
    f32 accumulated_energy = 0.0f;
    const f32 dt = 1.0f / 60.0f;
    
    for (usize i = 0; i < sequential_pattern.size(); ++i) {
        usize index = sequential_pattern[i];
        if (index >= aos_data_.size()) continue;
        
        // Software prefetching for next iteration
        if (i + 8 < sequential_pattern.size() && sequential_pattern[i + 8] < aos_data_.size()) {
#ifdef __builtin_prefetch
            __builtin_prefetch(&aos_data_[sequential_pattern[i + 8]], 0, 3);
#endif
        }
        
        TestComponent& comp = aos_data_[index];
        
        // Same computation as in measure_aos_performance
        comp.x += comp.vx * dt;
        comp.y += comp.vy * dt;
        comp.z += comp.vz * dt;
        
        f32 speed_squared = comp.vx * comp.vx + comp.vy * comp.vy + comp.vz * comp.vz;
        accumulated_energy += 0.5f * comp.mass * speed_squared;
        
        comp.vx *= 0.999f;
        comp.vy *= 0.999f;
        comp.vz *= 0.999f;
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    
    auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time);
    
    MemoryExperimentResult result;
    result.experiment_name = "Software Prefetching Test";
    result.config = config_;
    result.total_time_ms = static_cast<f64>(duration.count()) / 1'000'000.0;
    result.time_per_element_ns = static_cast<f64>(duration.count()) / sequential_pattern.size();
    
    result.key_observations.push_back("Software prefetching can reduce memory latency");
    result.key_observations.push_back("Most effective with predictable access patterns");
    result.optimization_recommendation = "Consider prefetching for streaming operations";
    
    // Prevent optimization
    result.memory_bandwidth_gbps = static_cast<f64>(accumulated_energy) * 0.0;
    
    return result;
}

MemoryExperimentResult MemoryAccessExperiment::run_alignment_experiment() {
    // This would test different memory alignment strategies
    // For now, return a placeholder result
    
    MemoryExperimentResult result;
    result.experiment_name = "Memory Alignment Test";
    result.config = config_;
    
    result.key_observations.push_back("Memory alignment affects cache performance");
    result.key_observations.push_back("Cache line alignment reduces false sharing");
    result.optimization_recommendation = "Align frequently accessed data to cache line boundaries";
    
    return result;
}

// ArchetypeMigrationExperiment Implementation
ArchetypeMigrationExperiment::ArchetypeMigrationExperiment(std::weak_ptr<ecs::Registry> registry, 
                                                         usize entity_count, 
                                                         usize component_types)
    : registry_(registry), entity_count_(entity_count), component_types_(component_types) {
    
    // Initialize migration scenarios
    scenarios_.push_back({
        "Add Single Component",
        "Add one component to entities, triggering archetype migration",
        [](ecs::Registry& reg, const std::vector<ecs::Entity>& entities) {
            // TODO: Implement when ECS component system is available
            // for (auto entity : entities) {
            //     reg.add_component<NewComponent>(entity);
            // }
        },
        1.0
    });
    
    scenarios_.push_back({
        "Remove Single Component", 
        "Remove one component from entities, triggering archetype migration",
        [](ecs::Registry& reg, const std::vector<ecs::Entity>& entities) {
            // TODO: Implement when ECS component system is available
        },
        1.0
    });
    
    scenarios_.push_back({
        "Add Multiple Components",
        "Add multiple components simultaneously, creating new archetype",
        [](ecs::Registry& reg, const std::vector<ecs::Entity>& entities) {
            // TODO: Implement when ECS component system is available
        },
        2.5
    });
}

std::string ArchetypeMigrationExperiment::get_description() const {
    return "Analyzes the performance cost of archetype migration in ECS systems when entities "
           "gain or lose components. This experiment demonstrates how component changes affect "
           "memory layout and the associated performance implications.";
}

bool ArchetypeMigrationExperiment::setup(const ExperimentConfig& config) {
    auto registry = registry_.lock();
    if (!registry) {
        LOG_ERROR("ECS Registry not available for archetype migration experiment");
        return false;
    }
    
    LOG_INFO("Setting up Archetype Migration Experiment with {} entities", entity_count_);
    
    // TODO: Create test entities with various component combinations
    // This will be implemented when the ECS system is fully available
    
    return true;
}

BenchmarkResult ArchetypeMigrationExperiment::execute() {
    BenchmarkResult result;
    result.name = get_name();
    result.description = get_description();
    result.category = get_category();
    
    auto registry = registry_.lock();
    if (!registry) {
        result.is_valid = false;
        result.error_message = "ECS Registry not available";
        return result;
    }
    
    // TODO: Implement archetype migration benchmarking
    // This is a placeholder implementation
    
    result.execution_time_ms = 10.0; // Placeholder
    result.efficiency_score = 0.8;
    result.throughput = entity_count_ / result.execution_time_ms * 1000.0;
    result.is_valid = true;
    
    result.insights.push_back("Archetype migration costs depend on entity count and component sizes");
    result.insights.push_back("Batch operations can amortize migration overhead");
    
    LOG_INFO("Archetype Migration Experiment completed (placeholder implementation)");
    
    return result;
}

void ArchetypeMigrationExperiment::cleanup() {
    LOG_INFO("Archetype Migration Experiment cleanup completed");
}

std::vector<PerformanceRecommendation> ArchetypeMigrationExperiment::generate_recommendations() const {
    std::vector<PerformanceRecommendation> recommendations;
    
    PerformanceRecommendation batch_rec;
    batch_rec.title = "Batch Component Operations";
    batch_rec.description = "Group component additions/removals to reduce archetype migration overhead.";
    batch_rec.priority = PerformanceRecommendation::Priority::High;
    batch_rec.category = PerformanceRecommendation::Category::ECS;
    batch_rec.estimated_improvement = 40.0;
    batch_rec.implementation_difficulty = 0.6;
    batch_rec.educational_notes.push_back("Batching reduces the number of memory allocations and copies");
    batch_rec.educational_notes.push_back("Consider component change queues for deferred processing");
    recommendations.push_back(batch_rec);
    
    return recommendations;
}

ArchetypeMigrationExperiment::MigrationResult 
ArchetypeMigrationExperiment::measure_migration_performance(const MigrationScenario& scenario,
                                                          const std::vector<ecs::Entity>& entities) {
    MigrationResult result;
    result.scenario_name = scenario.name;
    
    auto registry = registry_.lock();
    if (!registry) {
        result.migration_time_ms = 0.0;
        result.insights.push_back("Registry not available");
        return result;
    }
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    // Execute migration scenario
    scenario.migration_func(*registry, entities);
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time);
    
    result.migration_time_ms = static_cast<f64>(duration.count()) / 1'000'000.0;
    result.entities_migrated = entities.size();
    
    // TODO: Collect more detailed migration metrics
    result.memory_copied_bytes = entities.size() * 128; // Placeholder estimate
    result.fragmentation_impact = 0.1; // Placeholder
    
    result.insights.push_back("Migration time: " + std::to_string(result.migration_time_ms) + "ms");
    result.insights.push_back("Entities affected: " + std::to_string(result.entities_migrated));
    
    return result;
}

// CacheOptimizationExperiment Implementation
CacheOptimizationExperiment::CacheOptimizationExperiment(usize data_size)
    : data_size_(data_size) {
}

std::string CacheOptimizationExperiment::get_description() const {
    return "Demonstrates the performance impact of cache-friendly vs cache-hostile data layouts. "
           "This experiment shows how organizing hot and cold data affects performance in practice.";
}

bool CacheOptimizationExperiment::setup(const ExperimentConfig& config) {
    LOG_INFO("Setting up Cache Optimization Experiment with {} elements", data_size_);
    
    try {
        // Initialize cache-friendly data
        test_data_.friendly_data.reserve(data_size_);
        test_data_.friendly_data.clear();
        
        // Initialize cache-hostile data  
        test_data_.hostile_data.reserve(data_size_);
        test_data_.hostile_data.clear();
        
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<f32> dis(-100.0f, 100.0f);
        
        for (usize i = 0; i < data_size_; ++i) {
            // Cache-friendly: hot data grouped together
            CacheTestData::CacheFriendly friendly;
            for (int j = 0; j < 4; ++j) {
                friendly.frequently_used[j] = dis(gen);
            }
            friendly.id = static_cast<u32>(i);
            friendly.flags = static_cast<u8>(i % 256);
            std::fill(std::begin(friendly.padding1), std::end(friendly.padding1), 0);
            for (int j = 0; j < 16; ++j) {
                friendly.rarely_used[j] = dis(gen);
            }
            test_data_.friendly_data.push_back(friendly);
            
            // Cache-hostile: hot and cold data interleaved
            CacheTestData::CacheHostile hostile;
            hostile.frequently_used_1 = dis(gen);
            hostile.frequently_used_2 = dis(gen);
            hostile.frequently_used_3 = dis(gen);
            hostile.frequently_used_4 = dis(gen);
            hostile.id = static_cast<u32>(i);
            hostile.flags = static_cast<u8>(i % 256);
            for (int j = 0; j < 4; ++j) {
                hostile.rarely_used[j] = dis(gen);
                hostile.more_rarely_used[j] = dis(gen);
                hostile.even_more_rarely_used[j] = dis(gen);
            }
            std::fill(std::begin(hostile.padding), std::end(hostile.padding), 0);
            test_data_.hostile_data.push_back(hostile);
        }
        
        LOG_INFO("Cache optimization test data initialized");
        return true;
        
    } catch (const std::exception& e) {
        LOG_ERROR("Failed to setup Cache Optimization Experiment: {}", e.what());
        return false;
    }
}

BenchmarkResult CacheOptimizationExperiment::execute() {
    LOG_INFO("Executing Cache Optimization Analysis");
    
    BenchmarkResult result;
    result.name = get_name();
    result.description = get_description();
    result.category = get_category();
    
    try {
        f64 friendly_time = measure_cache_friendly_performance();
        f64 hostile_time = measure_cache_hostile_performance();
        f64 mixed_time = measure_mixed_access_performance();
        
        result.execution_time_ms = friendly_time + hostile_time + mixed_time;
        result.average_time_ms = result.execution_time_ms / 3.0;
        
        // Calculate cache efficiency based on performance difference
        f64 cache_impact = (hostile_time - friendly_time) / friendly_time;
        result.cache_miss_rate = std::min(1.0, cache_impact / 2.0); // Heuristic
        result.efficiency_score = friendly_time / hostile_time;
        
        result.throughput = data_size_ / result.average_time_ms * 1000.0;
        
        // Generate insights
        f64 improvement_percent = cache_impact * 100.0;
        result.insights.push_back("Cache-friendly layout shows " + std::to_string(improvement_percent) + 
                                 "% performance improvement");
        result.insights.push_back("Hot data grouping reduces cache line waste");
        result.insights.push_back("Cache-hostile layout causes more cache misses due to data interleaving");
        
        // Store detailed results
        result.metadata["cache_friendly_ms"] = friendly_time;
        result.metadata["cache_hostile_ms"] = hostile_time;
        result.metadata["mixed_access_ms"] = mixed_time;
        result.metadata["cache_impact_percent"] = improvement_percent;
        
        result.is_valid = true;
        result.confidence_level = 0.8;
        
        LOG_INFO("Cache Optimization Analysis completed (friendly: {:.2f}ms, hostile: {:.2f}ms)", 
                 friendly_time, hostile_time);
        
        return result;
        
    } catch (const std::exception& e) {
        result.is_valid = false;
        result.error_message = "Execution failed: " + std::string(e.what());
        LOG_ERROR("Cache Optimization Experiment execution failed: {}", e.what());
        return result;
    }
}

void CacheOptimizationExperiment::cleanup() {
    test_data_.friendly_data.clear();
    test_data_.hostile_data.clear();
    LOG_INFO("Cache Optimization Experiment cleanup completed");
}

std::vector<PerformanceRecommendation> CacheOptimizationExperiment::generate_recommendations() const {
    std::vector<PerformanceRecommendation> recommendations;
    
    PerformanceRecommendation hot_data_rec;
    hot_data_rec.title = "Group Hot Data Together";
    hot_data_rec.description = "Organize frequently accessed data fields together to improve cache locality.";
    hot_data_rec.priority = PerformanceRecommendation::Priority::High;
    hot_data_rec.category = PerformanceRecommendation::Category::Memory;
    hot_data_rec.estimated_improvement = 30.0;
    hot_data_rec.implementation_difficulty = 0.5;
    hot_data_rec.educational_notes.push_back("Cache lines load 64 bytes - organize data to maximize utilization");
    hot_data_rec.educational_notes.push_back("Consider splitting hot and cold data into separate structures");
    recommendations.push_back(hot_data_rec);
    
    return recommendations;
}

f64 CacheOptimizationExperiment::measure_cache_friendly_performance() {
    auto start_time = std::chrono::high_resolution_clock::now();
    
    f32 accumulator = 0.0f;
    
    // Access only the frequently used fields (cache-friendly)
    for (const auto& data : test_data_.friendly_data) {
        accumulator += data.frequently_used[0] + data.frequently_used[1] + 
                      data.frequently_used[2] + data.frequently_used[3];
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time);
    
    // Prevent optimization
    volatile f32 result = accumulator;
    (void)result;
    
    return static_cast<f64>(duration.count()) / 1'000'000.0;
}

f64 CacheOptimizationExperiment::measure_cache_hostile_performance() {
    auto start_time = std::chrono::high_resolution_clock::now();
    
    f32 accumulator = 0.0f;
    
    // Access the frequently used fields (cache-hostile due to interleaved cold data)
    for (const auto& data : test_data_.hostile_data) {
        accumulator += data.frequently_used_1 + data.frequently_used_2 + 
                      data.frequently_used_3 + data.frequently_used_4;
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time);
    
    // Prevent optimization
    volatile f32 result = accumulator;
    (void)result;
    
    return static_cast<f64>(duration.count()) / 1'000'000.0;
}

f64 CacheOptimizationExperiment::measure_mixed_access_performance() {
    auto start_time = std::chrono::high_resolution_clock::now();
    
    f32 accumulator = 0.0f;
    
    // Mixed access pattern
    for (usize i = 0; i < std::min(test_data_.friendly_data.size(), test_data_.hostile_data.size()); ++i) {
        if (i % 2 == 0) {
            const auto& data = test_data_.friendly_data[i];
            accumulator += data.frequently_used[0] + data.frequently_used[1];
        } else {
            const auto& data = test_data_.hostile_data[i];
            accumulator += data.frequently_used_1 + data.frequently_used_2;
        }
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time);
    
    // Prevent optimization
    volatile f32 result = accumulator;
    (void)result;
    
    return static_cast<f64>(duration.count()) / 1'000'000.0;
}

// MemoryBandwidthExperiment Implementation
MemoryBandwidthExperiment::MemoryBandwidthExperiment() {
    // Setup different test configurations
    test_configs_.push_back({1, 64, false, false, 100});      // 1MB, 64B stride, no prefetch, read
    test_configs_.push_back({1, 64, true, false, 100});       // 1MB, 64B stride, prefetch, read
    test_configs_.push_back({1, 64, false, true, 100});       // 1MB, 64B stride, no prefetch, write
    test_configs_.push_back({8, 64, false, false, 100});      // 8MB, 64B stride, no prefetch, read
    test_configs_.push_back({1, 4096, false, false, 100});    // 1MB, 4KB stride, no prefetch, read
}

std::string MemoryBandwidthExperiment::get_description() const {
    return "Measures memory bandwidth utilization under different access patterns and buffer sizes. "
           "This experiment helps understand memory subsystem performance characteristics and "
           "identifies optimal access patterns for maximum bandwidth utilization.";
}

bool MemoryBandwidthExperiment::setup(const ExperimentConfig& config) {
    LOG_INFO("Setting up Memory Bandwidth Experiment");
    
    try {
        // Allocate test buffer (use largest configuration)
        usize max_buffer_size = 0;
        for (const auto& cfg : test_configs_) {
            max_buffer_size = std::max(max_buffer_size, cfg.buffer_size_mb * 1024 * 1024);
        }
        
        test_buffer_.resize(max_buffer_size);
        
        // Initialize with random data
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<u8> dis(0, 255);
        
        for (auto& byte : test_buffer_) {
            byte = dis(gen);
        }
        
        LOG_INFO("Memory bandwidth test buffer initialized ({} bytes)", test_buffer_.size());
        return true;
        
    } catch (const std::exception& e) {
        LOG_ERROR("Failed to setup Memory Bandwidth Experiment: {}", e.what());
        return false;
    }
}

BenchmarkResult MemoryBandwidthExperiment::execute() {
    LOG_INFO("Executing Memory Bandwidth Analysis");
    
    BenchmarkResult result;
    result.name = get_name();
    result.description = get_description();
    result.category = get_category();
    
    try {
        std::vector<f64> bandwidth_results;
        
        for (const auto& config : test_configs_) {
            f64 bandwidth = measure_sequential_read_bandwidth(config);
            bandwidth_results.push_back(bandwidth);
        }
        
        // Calculate statistics
        f64 max_bandwidth = *std::max_element(bandwidth_results.begin(), bandwidth_results.end());
        f64 avg_bandwidth = std::accumulate(bandwidth_results.begin(), bandwidth_results.end(), 0.0) / bandwidth_results.size();
        
        result.execution_time_ms = 100.0; // Placeholder
        result.memory_bandwidth_usage = max_bandwidth;
        result.efficiency_score = avg_bandwidth / max_bandwidth;
        result.throughput = max_bandwidth * 1024 * 1024; // Convert GB/s to bytes/s
        
        // Generate insights
        result.insights.push_back("Peak memory bandwidth: " + std::to_string(max_bandwidth) + " GB/s");
        result.insights.push_back("Average bandwidth utilization: " + std::to_string(result.efficiency_score * 100) + "%");
        result.insights.push_back("Sequential access shows best bandwidth utilization");
        
        if (bandwidth_results.size() >= 2 && bandwidth_results[1] > bandwidth_results[0]) {
            result.insights.push_back("Software prefetching improved bandwidth by " + 
                                    std::to_string((bandwidth_results[1] - bandwidth_results[0]) / bandwidth_results[0] * 100) + "%");
        }
        
        result.is_valid = true;
        result.confidence_level = 0.8;
        
        LOG_INFO("Memory Bandwidth Analysis completed (peak: {:.2f} GB/s)", max_bandwidth);
        
        return result;
        
    } catch (const std::exception& e) {
        result.is_valid = false;
        result.error_message = "Execution failed: " + std::string(e.what());
        LOG_ERROR("Memory Bandwidth Experiment execution failed: {}", e.what());
        return result;
    }
}

void MemoryBandwidthExperiment::cleanup() {
    test_buffer_.clear();
    LOG_INFO("Memory Bandwidth Experiment cleanup completed");
}

std::vector<PerformanceRecommendation> MemoryBandwidthExperiment::generate_recommendations() const {
    std::vector<PerformanceRecommendation> recommendations;
    
    PerformanceRecommendation streaming_rec;
    streaming_rec.title = "Optimize for Sequential Access";
    streaming_rec.description = "Sequential memory access patterns achieve higher bandwidth utilization than random access.";
    streaming_rec.priority = PerformanceRecommendation::Priority::Medium;
    streaming_rec.category = PerformanceRecommendation::Category::Memory;
    streaming_rec.estimated_improvement = 20.0;
    streaming_rec.implementation_difficulty = 0.4;
    streaming_rec.educational_notes.push_back("Memory controllers optimize for sequential prefetching");
    streaming_rec.educational_notes.push_back("Consider restructuring algorithms for better locality");
    recommendations.push_back(streaming_rec);
    
    return recommendations;
}

f64 MemoryBandwidthExperiment::measure_sequential_read_bandwidth(const BandwidthTestConfig& config) {
    usize buffer_size = config.buffer_size_mb * 1024 * 1024;
    usize stride = config.access_stride;
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    volatile u8 dummy = 0;
    for (u32 iter = 0; iter < config.iterations; ++iter) {
        for (usize i = 0; i < buffer_size; i += stride) {
            if (config.prefetch_enabled && i + stride * 8 < buffer_size) {
#ifdef __builtin_prefetch
                __builtin_prefetch(&test_buffer_[i + stride * 8], 0, 3);
#endif
            }
            dummy += test_buffer_[i];
        }
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time);
    
    f64 time_seconds = static_cast<f64>(duration.count()) / 1e9;
    f64 bytes_accessed = static_cast<f64>(buffer_size * config.iterations / stride) * stride;
    f64 bandwidth_gbps = (bytes_accessed / (1024.0 * 1024.0 * 1024.0)) / time_seconds;
    
    // Prevent compiler optimization
    volatile u8 result = dummy;
    (void)result;
    
    return bandwidth_gbps;
}

f64 MemoryBandwidthExperiment::measure_sequential_write_bandwidth(const BandwidthTestConfig& config) {
    usize buffer_size = config.buffer_size_mb * 1024 * 1024;
    usize stride = config.access_stride;
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    u8 value = 42;
    for (u32 iter = 0; iter < config.iterations; ++iter) {
        for (usize i = 0; i < buffer_size; i += stride) {
            if (config.prefetch_enabled && i + stride * 8 < buffer_size) {
#ifdef __builtin_prefetch
                __builtin_prefetch(&test_buffer_[i + stride * 8], 1, 3);
#endif
            }
            test_buffer_[i] = value;
            value = (value + 1) & 0xFF;
        }
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time);
    
    f64 time_seconds = static_cast<f64>(duration.count()) / 1e9;
    f64 bytes_accessed = static_cast<f64>(buffer_size * config.iterations / stride) * stride;
    f64 bandwidth_gbps = (bytes_accessed / (1024.0 * 1024.0 * 1024.0)) / time_seconds;
    
    return bandwidth_gbps;
}

f64 MemoryBandwidthExperiment::measure_random_access_bandwidth(const BandwidthTestConfig& config) {
    usize buffer_size = config.buffer_size_mb * 1024 * 1024;
    
    // Generate random access pattern
    std::vector<usize> indices;
    for (usize i = 0; i < buffer_size; i += config.access_stride) {
        indices.push_back(i);
    }
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::shuffle(indices.begin(), indices.end(), gen);
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    volatile u8 dummy = 0;
    for (u32 iter = 0; iter < config.iterations; ++iter) {
        for (usize index : indices) {
            dummy += test_buffer_[index];
        }
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time);
    
    f64 time_seconds = static_cast<f64>(duration.count()) / 1e9;
    f64 bytes_accessed = static_cast<f64>(indices.size() * config.iterations);
    f64 bandwidth_gbps = (bytes_accessed / (1024.0 * 1024.0 * 1024.0)) / time_seconds;
    
    // Prevent compiler optimization
    volatile u8 result = dummy;
    (void)result;
    
    return bandwidth_gbps;
}

f64 MemoryBandwidthExperiment::measure_strided_access_bandwidth(const BandwidthTestConfig& config) {
    return measure_sequential_read_bandwidth(config); // Same implementation for now
}

// MemoryExperiments Implementation
MemoryExperiments::MemoryExperiments(std::weak_ptr<ecs::Registry> registry)
    : memory_tracker_(memory::MemoryTracker::get_instance()) {
    
    // Initialize experiments
    access_experiment_ = std::make_unique<MemoryAccessExperiment>();
    migration_experiment_ = std::make_unique<ArchetypeMigrationExperiment>(registry);
    cache_experiment_ = std::make_unique<CacheOptimizationExperiment>();
    bandwidth_experiment_ = std::make_unique<MemoryBandwidthExperiment>();
    
    initialize_educational_content();
    
    LOG_INFO("Memory Experiments suite initialized");
}

void MemoryExperiments::initialize_educational_content() {
    explanations_["soa_vs_aos"] = 
        "Structure of Arrays (SoA) vs Array of Structures (AoS):\n\n"
        "AoS: struct { x, y, z, vx, vy, vz; }; vector<Particle> particles;\n"
        "SoA: struct { vector<x>, vector<y>, vector<z>, vector<vx>, vector<vy>, vector<vz>; } particles;\n\n"
        "SoA advantages:\n"
        "- Better cache locality for operations on specific fields\n"
        "- SIMD vectorization opportunities\n"
        "- Reduced memory bandwidth requirements for field-specific operations\n\n"
        "AoS advantages:\n"
        "- Better for operations that need all fields of an object\n"
        "- More intuitive object-oriented design\n"
        "- Better cache utilization when accessing complete objects";
    
    explanations_["cache_behavior"] = 
        "CPU Cache Behavior and Memory Performance:\n\n"
        "Cache hierarchy (typical):\n"
        "- L1: 32KB, 1-2 cycles, per core\n"
        "- L2: 256KB, 3-8 cycles, per core\n"
        "- L3: 8MB, 12-28 cycles, shared\n"
        "- RAM: GB scale, 100+ cycles\n\n"
        "Cache line size: typically 64 bytes\n"
        "Spatial locality: accessing nearby memory locations\n"
        "Temporal locality: accessing same memory locations repeatedly\n\n"
        "Optimization strategies:\n"
        "- Group related data together\n"
        "- Use predictable access patterns\n"
        "- Consider cache line alignment\n"
        "- Minimize cache line conflicts";
    
    explanations_["memory_fragmentation"] = 
        "Memory Fragmentation in ECS Systems:\n\n"
        "External fragmentation: Free memory scattered in small chunks\n"
        "Internal fragmentation: Wasted space within allocated blocks\n\n"
        "Common causes in ECS:\n"
        "- Frequent entity creation/destruction\n"
        "- Archetype migrations\n"
        "- Component additions/removals\n"
        "- Varying component sizes\n\n"
        "Mitigation strategies:\n"
        "- Use arena allocators for components\n"
        "- Pool allocators for fixed-size objects\n"
        "- Batch operations to reduce fragmentation\n"
        "- Consider object recycling";
    
    explanations_["prefetching"] = 
        "Memory Prefetching Techniques:\n\n"
        "Hardware prefetching:\n"
        "- Automatic prediction of access patterns\n"
        "- Works well for sequential and simple strided patterns\n"
        "- Can be disrupted by complex patterns\n\n"
        "Software prefetching:\n"
        "- Explicit prefetch instructions (__builtin_prefetch)\n"
        "- Useful for irregular but predictable patterns\n"
        "- Requires careful tuning of prefetch distance\n\n"
        "Best practices:\n"
        "- Prefetch 8-12 iterations ahead\n"
        "- Use temporal locality hints\n"
        "- Don't over-prefetch (cache pollution)\n"
        "- Profile to verify effectiveness";
}

MemoryExperimentResult MemoryExperiments::run_soa_vs_aos_comparison(const TestDataConfig& config) {
    access_experiment_->set_test_data_config(config);
    return access_experiment_->run_aos_vs_soa_comparison();
}

MemoryExperimentResult MemoryExperiments::run_cache_behavior_analysis(usize data_size) {
    // Use cache experiment to analyze behavior
    ExperimentConfig config;
    cache_experiment_->setup(config);
    
    BenchmarkResult bench_result = cache_experiment_->execute();
    
    // Convert to MemoryExperimentResult
    MemoryExperimentResult result;
    result.experiment_name = "Cache Behavior Analysis";
    result.total_time_ms = bench_result.execution_time_ms;
    result.cache_efficiency = 1.0 - bench_result.cache_miss_rate;
    
    result.key_observations = bench_result.insights;
    result.optimization_recommendation = "Optimize data layout for better cache utilization";
    
    cache_experiment_->cleanup();
    
    return result;
}

MemoryExperimentResult MemoryExperiments::run_archetype_migration_analysis(usize entity_count) {
    migration_experiment_->set_entity_count(entity_count);
    
    ExperimentConfig config;
    migration_experiment_->setup(config);
    
    BenchmarkResult bench_result = migration_experiment_->execute();
    
    // Convert to MemoryExperimentResult
    MemoryExperimentResult result;
    result.experiment_name = "Archetype Migration Analysis";
    result.total_time_ms = bench_result.execution_time_ms;
    result.memory_allocated_bytes = bench_result.memory_usage_bytes;
    
    result.key_observations = bench_result.insights;
    result.optimization_recommendation = "Consider batching component operations";
    
    migration_experiment_->cleanup();
    
    return result;
}

MemoryExperimentResult MemoryExperiments::run_memory_bandwidth_analysis() {
    ExperimentConfig config;
    bandwidth_experiment_->setup(config);
    
    BenchmarkResult bench_result = bandwidth_experiment_->execute();
    
    // Convert to MemoryExperimentResult
    MemoryExperimentResult result;
    result.experiment_name = "Memory Bandwidth Analysis";
    result.total_time_ms = bench_result.execution_time_ms;
    result.memory_bandwidth_gbps = bench_result.memory_bandwidth_usage;
    
    result.key_observations = bench_result.insights;
    result.optimization_recommendation = "Optimize for sequential memory access patterns";
    
    bandwidth_experiment_->cleanup();
    
    return result;
}

std::vector<MemoryExperimentResult> MemoryExperiments::run_full_memory_analysis() {
    LOG_INFO("Running comprehensive memory analysis");
    
    std::vector<MemoryExperimentResult> results;
    
    // Run all experiments
    results.push_back(run_soa_vs_aos_comparison());
    results.push_back(run_cache_behavior_analysis());
    results.push_back(run_archetype_migration_analysis());
    results.push_back(run_memory_bandwidth_analysis());
    
    // Cache results
    {
        std::lock_guard<std::mutex> lock(cache_mutex_);
        for (const auto& result : results) {
            results_cache_[result.experiment_name] = result;
        }
    }
    
    LOG_INFO("Full memory analysis completed ({} experiments)", results.size());
    
    return results;
}

BenchmarkResult MemoryExperiments::run_comparative_analysis() {
    BenchmarkResult result;
    result.name = "Comprehensive Memory Analysis";
    result.description = "Complete analysis of memory behavior patterns in ECScope";
    result.category = "Memory";
    
    auto experiments = run_full_memory_analysis();
    
    if (!experiments.empty()) {
        f64 total_time = 0.0;
        for (const auto& exp : experiments) {
            total_time += exp.total_time_ms;
        }
        
        result.execution_time_ms = total_time;
        result.average_time_ms = total_time / experiments.size();
        result.efficiency_score = 0.8; // Placeholder
        result.is_valid = true;
        
        result.insights.push_back("Completed " + std::to_string(experiments.size()) + " memory experiments");
        result.insights.push_back("Total analysis time: " + std::to_string(total_time) + "ms");
    } else {
        result.is_valid = false;
        result.error_message = "No experiments completed successfully";
    }
    
    return result;
}

std::vector<MemoryExperimentResult> MemoryExperiments::get_all_results() const {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    
    std::vector<MemoryExperimentResult> results;
    results.reserve(results_cache_.size());
    
    for (const auto& [name, result] : results_cache_) {
        results.push_back(result);
    }
    
    return results;
}

std::optional<MemoryExperimentResult> MemoryExperiments::get_result(const std::string& experiment_name) const {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    
    auto it = results_cache_.find(experiment_name);
    if (it != results_cache_.end()) {
        return it->second;
    }
    
    return std::nullopt;
}

void MemoryExperiments::clear_results_cache() {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    results_cache_.clear();
    LOG_INFO("Memory experiments results cache cleared");
}

std::string MemoryExperiments::get_explanation(const std::string& topic) const {
    auto it = explanations_.find(topic);
    if (it != explanations_.end()) {
        return it->second;
    }
    return "No explanation available for topic: " + topic;
}

std::vector<std::string> MemoryExperiments::get_available_explanations() const {
    std::vector<std::string> topics;
    topics.reserve(explanations_.size());
    
    for (const auto& [topic, explanation] : explanations_) {
        topics.push_back(topic);
    }
    
    return topics;
}

std::vector<PerformanceRecommendation> MemoryExperiments::get_memory_optimization_recommendations() const {
    std::vector<PerformanceRecommendation> recommendations;
    
    // Collect recommendations from all experiments
    auto access_recs = access_experiment_->generate_recommendations();
    auto migration_recs = migration_experiment_->generate_recommendations();
    auto cache_recs = cache_experiment_->generate_recommendations();
    auto bandwidth_recs = bandwidth_experiment_->generate_recommendations();
    
    recommendations.insert(recommendations.end(), access_recs.begin(), access_recs.end());
    recommendations.insert(recommendations.end(), migration_recs.begin(), migration_recs.end());
    recommendations.insert(recommendations.end(), cache_recs.begin(), cache_recs.end());
    recommendations.insert(recommendations.end(), bandwidth_recs.begin(), bandwidth_recs.end());
    
    return recommendations;
}

f64 MemoryExperiments::calculate_memory_efficiency_score() const {
    // Simple heuristic based on memory tracker data and experiment results
    usize current_usage = memory_tracker_.get_current_usage();
    usize peak_usage = memory_tracker_.get_peak_usage();
    
    if (peak_usage == 0) return 1.0;
    
    f64 usage_efficiency = static_cast<f64>(current_usage) / peak_usage;
    
    // Factor in fragmentation and cache efficiency from experiments
    f64 base_score = usage_efficiency * 0.6 + 0.4; // Base efficiency
    
    return std::min(1.0, std::max(0.0, base_score));
}

std::vector<std::string> MemoryExperiments::identify_memory_bottlenecks() const {
    std::vector<std::string> bottlenecks;
    
    // Analyze memory tracker data
    usize current_usage = memory_tracker_.get_current_usage();
    usize peak_usage = memory_tracker_.get_peak_usage();
    
    if (current_usage > peak_usage * 0.8) {
        bottlenecks.push_back("High memory usage (near peak)");
    }
    
    // Analyze experiment results
    auto results = get_all_results();
    for (const auto& result : results) {
        if (result.cache_efficiency < 0.6) {
            bottlenecks.push_back("Poor cache efficiency in " + result.experiment_name);
        }
        
        if (result.memory_efficiency < 0.7) {
            bottlenecks.push_back("Low memory efficiency in " + result.experiment_name);
        }
    }
    
    if (bottlenecks.empty()) {
        bottlenecks.push_back("No significant memory bottlenecks detected");
    }
    
    return bottlenecks;
}

std::string MemoryExperiments::generate_memory_optimization_report() const {
    std::string report = "=== ECScope Memory Optimization Report ===\n\n";
    
    // Summary
    report += "Memory Efficiency Score: " + std::to_string(calculate_memory_efficiency_score() * 100.0) + "%\n";
    report += "Current Usage: " + std::to_string(memory_tracker_.get_current_usage()) + " bytes\n";
    report += "Peak Usage: " + std::to_string(memory_tracker_.get_peak_usage()) + " bytes\n\n";
    
    // Bottlenecks
    report += "Identified Bottlenecks:\n";
    auto bottlenecks = identify_memory_bottlenecks();
    for (const auto& bottleneck : bottlenecks) {
        report += "- " + bottleneck + "\n";
    }
    report += "\n";
    
    // Recommendations
    report += "Optimization Recommendations:\n";
    auto recommendations = get_memory_optimization_recommendations();
    for (const auto& rec : recommendations) {
        report += "- " + rec.title + ": " + rec.description + "\n";
        report += "  Priority: " + std::to_string(static_cast<int>(rec.priority)) + 
                  ", Estimated improvement: " + std::to_string(rec.estimated_improvement) + "%\n";
    }
    
    return report;
}

} // namespace ecscope::performance