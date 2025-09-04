/**
 * @file memory/memory_benchmark_suite.cpp
 * @brief Implementation of comprehensive memory system benchmarks
 */

#include "memory/memory_benchmark_suite.hpp"
#include "memory/numa_manager.hpp"
#include "memory/lockfree_allocators.hpp"
#include "memory/hierarchical_pools.hpp"
#include "memory/thread_local_allocator.hpp"
#include "memory/bandwidth_analyzer.hpp"
#include "core/log.hpp"
#include "core/profiler.hpp"
#include <algorithm>
#include <numeric>
#include <random>
#include <future>
#include <sstream>
#include <iomanip>

namespace ecscope::memory::benchmark {

//=============================================================================
// AllocationBenchmark Implementation
//=============================================================================

AllocationBenchmark::AllocationBenchmark(const BenchmarkConfiguration& config)
    : config_(config) {
    
    // Initialize random number generator
    rng_.seed(config.random_seed);
    
    // Pre-generate allocation sizes for consistent testing
    generate_allocation_patterns();
    
    LOG_DEBUG("Initialized allocation benchmark with {} iterations", config.iteration_count);
}

BenchmarkResult AllocationBenchmark::run_allocator_comparison() {
    PROFILE_FUNCTION();
    
    BenchmarkResult result{};
    result.benchmark_name = "Allocator Comparison";
    result.configuration = config_;
    result.start_time = get_current_time();
    
    LOG_INFO("Running allocator comparison benchmark...");
    
    // Test standard allocator
    auto standard_result = benchmark_standard_allocator();
    result.individual_results["Standard"] = standard_result;
    
    // Test lock-free allocator
    auto lockfree_result = benchmark_lockfree_allocator();
    result.individual_results["LockFree"] = lockfree_result;
    
    // Test hierarchical pools
    auto hierarchical_result = benchmark_hierarchical_allocator();
    result.individual_results["Hierarchical"] = hierarchical_result;
    
    // Test thread-local allocator
    auto thread_local_result = benchmark_thread_local_allocator();
    result.individual_results["ThreadLocal"] = thread_local_result;
    
    // Test NUMA-aware allocator
    auto numa_result = benchmark_numa_allocator();
    result.individual_results["NUMA"] = numa_result;
    
    result.end_time = get_current_time();
    result.total_duration_seconds = result.end_time - result.start_time;
    
    // Calculate comparative metrics
    calculate_comparative_metrics(result);
    
    LOG_INFO("Allocator comparison completed in {:.2f}s", result.total_duration_seconds);
    
    return result;
}

BenchmarkResult AllocationBenchmark::run_threading_stress_test() {
    PROFILE_FUNCTION();
    
    BenchmarkResult result{};
    result.benchmark_name = "Threading Stress Test";
    result.configuration = config_;
    result.start_time = get_current_time();
    
    LOG_INFO("Running threading stress test with {} threads...", config_.thread_count);
    
    std::vector<std::future<SingleAllocatorResult>> futures;
    
    // Test each allocator under high thread contention
    for (const auto& allocator_type : {"Standard", "LockFree", "ThreadLocal"}) {
        auto future = std::async(std::launch::async, [this, allocator_type]() {
            return run_concurrent_allocation_test(allocator_type);
        });
        futures.push_back(std::move(future));
    }
    
    // Collect results
    usize index = 0;
    for (const auto& allocator_type : {"Standard", "LockFree", "ThreadLocal"}) {
        result.individual_results[allocator_type] = futures[index++].get();
    }
    
    result.end_time = get_current_time();
    result.total_duration_seconds = result.end_time - result.start_time;
    
    // Calculate threading-specific metrics
    calculate_threading_metrics(result);
    
    LOG_INFO("Threading stress test completed in {:.2f}s", result.total_duration_seconds);
    
    return result;
}

BenchmarkResult AllocationBenchmark::run_memory_pattern_analysis() {
    PROFILE_FUNCTION();
    
    BenchmarkResult result{};
    result.benchmark_name = "Memory Pattern Analysis";
    result.configuration = config_;
    result.start_time = get_current_time();
    
    LOG_INFO("Running memory pattern analysis...");
    
    // Test different allocation patterns
    auto sequential_result = benchmark_sequential_pattern();
    result.individual_results["Sequential"] = sequential_result;
    
    auto random_result = benchmark_random_pattern();
    result.individual_results["Random"] = random_result;
    
    auto mixed_result = benchmark_mixed_pattern();
    result.individual_results["Mixed"] = mixed_result;
    
    auto burst_result = benchmark_burst_pattern();
    result.individual_results["Burst"] = burst_result;
    
    result.end_time = get_current_time();
    result.total_duration_seconds = result.end_time - result.start_time;
    
    // Calculate pattern-specific metrics
    calculate_pattern_metrics(result);
    
    LOG_INFO("Memory pattern analysis completed in {:.2f}s", result.total_duration_seconds);
    
    return result;
}

// Private implementation methods

void AllocationBenchmark::generate_allocation_patterns() {
    allocation_sizes_.clear();
    allocation_sizes_.reserve(config_.iteration_count);
    
    // Generate size distribution based on configuration
    std::uniform_int_distribution<usize> size_dist(config_.min_allocation_size, config_.max_allocation_size);
    
    // Add some common sizes with higher probability
    std::vector<usize> common_sizes = {8, 16, 32, 64, 128, 256, 512, 1024, 2048, 4096};
    std::uniform_int_distribution<usize> pattern_choice(0, 9);
    
    for (usize i = 0; i < config_.iteration_count; ++i) {
        if (pattern_choice(rng_) < 3) {
            // 30% chance to use common size
            usize common_index = pattern_choice(rng_) % common_sizes.size();
            allocation_sizes_.push_back(common_sizes[common_index]);
        } else {
            // 70% chance to use random size in range
            allocation_sizes_.push_back(size_dist(rng_));
        }
    }
}

SingleAllocatorResult AllocationBenchmark::benchmark_standard_allocator() {
    PROFILE_SCOPE("StandardAllocator");
    
    SingleAllocatorResult result{};
    result.allocator_name = "Standard";
    
    std::vector<void*> pointers;
    pointers.reserve(config_.iteration_count);
    
    // Allocation phase
    auto alloc_start = std::chrono::high_resolution_clock::now();
    
    for (usize i = 0; i < config_.iteration_count; ++i) {
        void* ptr = std::malloc(allocation_sizes_[i]);
        if (ptr) {
            pointers.push_back(ptr);
            result.successful_allocations++;
            result.total_bytes_allocated += allocation_sizes_[i];
        } else {
            result.failed_allocations++;
        }
    }
    
    auto alloc_end = std::chrono::high_resolution_clock::now();
    result.allocation_time_seconds = std::chrono::duration<f64>(alloc_end - alloc_start).count();
    
    // Deallocation phase
    auto dealloc_start = std::chrono::high_resolution_clock::now();
    
    for (void* ptr : pointers) {
        std::free(ptr);
    }
    
    auto dealloc_end = std::chrono::high_resolution_clock::now();
    result.deallocation_time_seconds = std::chrono::duration<f64>(dealloc_end - dealloc_start).count();
    
    // Calculate metrics
    calculate_allocator_metrics(result);
    
    return result;
}

SingleAllocatorResult AllocationBenchmark::benchmark_lockfree_allocator() {
    PROFILE_SCOPE("LockFreeAllocator");
    
    auto& allocator = lockfree::get_global_lockfree_allocator();
    
    SingleAllocatorResult result{};
    result.allocator_name = "LockFree";
    
    std::vector<void*> pointers;
    pointers.reserve(config_.iteration_count);
    
    // Allocation phase
    auto alloc_start = std::chrono::high_resolution_clock::now();
    
    for (usize i = 0; i < config_.iteration_count; ++i) {
        void* ptr = allocator.allocate(allocation_sizes_[i]);
        if (ptr) {
            pointers.push_back(ptr);
            result.successful_allocations++;
            result.total_bytes_allocated += allocation_sizes_[i];
        } else {
            result.failed_allocations++;
        }
    }
    
    auto alloc_end = std::chrono::high_resolution_clock::now();
    result.allocation_time_seconds = std::chrono::duration<f64>(alloc_end - alloc_start).count();
    
    // Deallocation phase
    auto dealloc_start = std::chrono::high_resolution_clock::now();
    
    for (void* ptr : pointers) {
        allocator.deallocate(ptr);
    }
    
    auto dealloc_end = std::chrono::high_resolution_clock::now();
    result.deallocation_time_seconds = std::chrono::duration<f64>(dealloc_end - dealloc_start).count();
    
    // Get allocator-specific stats
    auto stats = allocator.get_statistics();
    result.allocator_specific_metrics["arena_allocations"] = static_cast<f64>(stats.arena_allocations);
    result.allocator_specific_metrics["pool_allocations"] = static_cast<f64>(stats.pool_allocations);
    result.allocator_specific_metrics["distribution_ratio"] = stats.allocation_distribution_ratio;
    
    calculate_allocator_metrics(result);
    
    return result;
}

SingleAllocatorResult AllocationBenchmark::benchmark_hierarchical_allocator() {
    PROFILE_SCOPE("HierarchicalAllocator");
    
    auto& allocator = hierarchical::get_global_hierarchical_allocator();
    
    SingleAllocatorResult result{};
    result.allocator_name = "Hierarchical";
    
    std::vector<void*> pointers;
    pointers.reserve(config_.iteration_count);
    
    // Allocation phase
    auto alloc_start = std::chrono::high_resolution_clock::now();
    
    for (usize i = 0; i < config_.iteration_count; ++i) {
        void* ptr = allocator.allocate(allocation_sizes_[i]);
        if (ptr) {
            pointers.push_back(ptr);
            result.successful_allocations++;
            result.total_bytes_allocated += allocation_sizes_[i];
        } else {
            result.failed_allocations++;
        }
    }
    
    auto alloc_end = std::chrono::high_resolution_clock::now();
    result.allocation_time_seconds = std::chrono::duration<f64>(alloc_end - alloc_start).count();
    
    // Deallocation phase
    auto dealloc_start = std::chrono::high_resolution_clock::now();
    
    for (void* ptr : pointers) {
        allocator.deallocate(ptr);
    }
    
    auto dealloc_end = std::chrono::high_resolution_clock::now();
    result.deallocation_time_seconds = std::chrono::duration<f64>(dealloc_end - dealloc_start).count();
    
    // Get allocator-specific stats
    auto stats = allocator.get_statistics();
    result.allocator_specific_metrics["l1_hit_rate"] = stats.l1_hit_rate;
    result.allocator_specific_metrics["l2_hit_rate"] = stats.l2_hit_rate;
    result.allocator_specific_metrics["cache_efficiency"] = stats.overall_cache_efficiency;
    result.allocator_specific_metrics["active_size_classes"] = static_cast<f64>(stats.active_size_classes);
    
    calculate_allocator_metrics(result);
    
    return result;
}

SingleAllocatorResult AllocationBenchmark::benchmark_thread_local_allocator() {
    PROFILE_SCOPE("ThreadLocalAllocator");
    
    auto& registry = thread_local::get_global_thread_local_registry();
    thread_local::ThreadRegistrationGuard guard;
    
    auto& pool = registry.get_primary_pool();
    
    SingleAllocatorResult result{};
    result.allocator_name = "ThreadLocal";
    
    std::vector<void*> pointers;
    pointers.reserve(config_.iteration_count);
    
    // Allocation phase
    auto alloc_start = std::chrono::high_resolution_clock::now();
    
    for (usize i = 0; i < config_.iteration_count; ++i) {
        void* ptr = pool.allocate(allocation_sizes_[i]);
        if (ptr) {
            pointers.push_back(ptr);
            result.successful_allocations++;
            result.total_bytes_allocated += allocation_sizes_[i];
        } else {
            result.failed_allocations++;
        }
    }
    
    auto alloc_end = std::chrono::high_resolution_clock::now();
    result.allocation_time_seconds = std::chrono::duration<f64>(alloc_end - alloc_start).count();
    
    // Deallocation phase
    auto dealloc_start = std::chrono::high_resolution_clock::now();
    
    for (void* ptr : pointers) {
        pool.deallocate(ptr);
    }
    
    auto dealloc_end = std::chrono::high_resolution_clock::now();
    result.deallocation_time_seconds = std::chrono::duration<f64>(dealloc_end - dealloc_start).count();
    
    // Get allocator-specific stats
    auto stats = pool.get_statistics();
    result.allocator_specific_metrics["hit_rate"] = stats.hit_rate;
    result.allocator_specific_metrics["utilization"] = stats.utilization_ratio;
    result.allocator_specific_metrics["cross_thread_accesses"] = static_cast<f64>(stats.cross_thread_accesses);
    result.allocator_specific_metrics["active_size_classes"] = static_cast<f64>(stats.active_size_classes);
    
    calculate_allocator_metrics(result);
    
    return result;
}

SingleAllocatorResult AllocationBenchmark::benchmark_numa_allocator() {
    PROFILE_SCOPE("NUMAAllocator");
    
    auto& numa_manager = numa::get_global_numa_manager();
    
    SingleAllocatorResult result{};
    result.allocator_name = "NUMA";
    
    std::vector<void*> pointers;
    pointers.reserve(config_.iteration_count);
    
    // Set thread affinity for consistent NUMA allocation
    numa_manager.set_current_thread_affinity(0);
    
    // Allocation phase
    auto alloc_start = std::chrono::high_resolution_clock::now();
    
    for (usize i = 0; i < config_.iteration_count; ++i) {
        void* ptr = numa_manager.allocate(allocation_sizes_[i]);
        if (ptr) {
            pointers.push_back(ptr);
            result.successful_allocations++;
            result.total_bytes_allocated += allocation_sizes_[i];
        } else {
            result.failed_allocations++;
        }
    }
    
    auto alloc_end = std::chrono::high_resolution_clock::now();
    result.allocation_time_seconds = std::chrono::duration<f64>(alloc_end - alloc_start).count();
    
    // Deallocation phase
    auto dealloc_start = std::chrono::high_resolution_clock::now();
    
    for (usize i = 0; i < pointers.size(); ++i) {
        numa_manager.deallocate(pointers[i], allocation_sizes_[i]);
    }
    
    auto dealloc_end = std::chrono::high_resolution_clock::now();
    result.deallocation_time_seconds = std::chrono::duration<f64>(dealloc_end - dealloc_start).count();
    
    // Get allocator-specific stats
    auto metrics = numa_manager.get_performance_metrics();
    result.allocator_specific_metrics["local_access_ratio"] = metrics.local_access_ratio;
    result.allocator_specific_metrics["cross_node_penalty"] = metrics.cross_node_penalty_factor;
    result.allocator_specific_metrics["total_migrations"] = static_cast<f64>(metrics.total_migrations);
    
    calculate_allocator_metrics(result);
    
    return result;
}

SingleAllocatorResult AllocationBenchmark::run_concurrent_allocation_test(const std::string& allocator_type) {
    SingleAllocatorResult result{};
    result.allocator_name = allocator_type + "_Concurrent";
    
    std::vector<std::future<void>> worker_futures;
    std::atomic<usize> total_successful{0};
    std::atomic<usize> total_failed{0};
    std::atomic<usize> total_bytes{0};
    
    auto test_start = std::chrono::high_resolution_clock::now();
    
    // Launch worker threads
    for (u32 t = 0; t < config_.thread_count; ++t) {
        auto future = std::async(std::launch::async, [this, t, &total_successful, &total_failed, &total_bytes, allocator_type]() {
            concurrent_worker(t, allocator_type, total_successful, total_failed, total_bytes);
        });
        worker_futures.push_back(std::move(future));
    }
    
    // Wait for completion
    for (auto& future : worker_futures) {
        future.wait();
    }
    
    auto test_end = std::chrono::high_resolution_clock::now();
    
    result.allocation_time_seconds = std::chrono::duration<f64>(test_end - test_start).count();
    result.successful_allocations = total_successful.load();
    result.failed_allocations = total_failed.load();
    result.total_bytes_allocated = total_bytes.load();
    
    calculate_allocator_metrics(result);
    
    return result;
}

void AllocationBenchmark::concurrent_worker(u32 worker_id, const std::string& allocator_type,
                                          std::atomic<usize>& successful, std::atomic<usize>& failed,
                                          std::atomic<usize>& total_bytes) {
    
    thread_local::ThreadRegistrationGuard guard; // For thread-local allocator
    
    std::vector<void*> local_pointers;
    usize iterations_per_thread = config_.iteration_count / config_.thread_count;
    local_pointers.reserve(iterations_per_thread);
    
    // Worker-specific random seed
    std::mt19937 local_rng(config_.random_seed + worker_id);
    std::uniform_int_distribution<usize> size_dist(config_.min_allocation_size, config_.max_allocation_size);
    
    for (usize i = 0; i < iterations_per_thread; ++i) {
        usize size = size_dist(local_rng);
        void* ptr = nullptr;
        
        // Allocate based on allocator type
        if (allocator_type == "Standard") {
            ptr = std::malloc(size);
        } else if (allocator_type == "LockFree") {
            ptr = lockfree::get_global_lockfree_allocator().allocate(size);
        } else if (allocator_type == "ThreadLocal") {
            ptr = thread_local::get_global_thread_local_registry().get_primary_pool().allocate(size);
        }
        
        if (ptr) {
            local_pointers.push_back(ptr);
            successful.fetch_add(1, std::memory_order_relaxed);
            total_bytes.fetch_add(size, std::memory_order_relaxed);
        } else {
            failed.fetch_add(1, std::memory_order_relaxed);
        }
    }
    
    // Deallocate
    for (void* ptr : local_pointers) {
        if (allocator_type == "Standard") {
            std::free(ptr);
        } else if (allocator_type == "LockFree") {
            lockfree::get_global_lockfree_allocator().deallocate(ptr);
        } else if (allocator_type == "ThreadLocal") {
            thread_local::get_global_thread_local_registry().get_primary_pool().deallocate(ptr);
        }
    }
}

SingleAllocatorResult AllocationBenchmark::benchmark_sequential_pattern() {
    // Sequential allocation and deallocation
    return benchmark_standard_allocator(); // Placeholder - would implement specific pattern
}

SingleAllocatorResult AllocationBenchmark::benchmark_random_pattern() {
    // Random interleaved allocation and deallocation
    SingleAllocatorResult result{};
    result.allocator_name = "RandomPattern";
    
    std::vector<void*> active_pointers;
    std::uniform_real_distribution<f64> action_dist(0.0, 1.0);
    
    auto start = std::chrono::high_resolution_clock::now();
    
    for (usize i = 0; i < config_.iteration_count; ++i) {
        if (active_pointers.empty() || action_dist(rng_) < 0.7) {
            // 70% chance to allocate (or forced if no active pointers)
            void* ptr = std::malloc(allocation_sizes_[i % allocation_sizes_.size()]);
            if (ptr) {
                active_pointers.push_back(ptr);
                result.successful_allocations++;
                result.total_bytes_allocated += allocation_sizes_[i % allocation_sizes_.size()];
            } else {
                result.failed_allocations++;
            }
        } else {
            // 30% chance to deallocate
            if (!active_pointers.empty()) {
                std::uniform_int_distribution<usize> ptr_dist(0, active_pointers.size() - 1);
                usize index = ptr_dist(rng_);
                std::free(active_pointers[index]);
                active_pointers.erase(active_pointers.begin() + index);
            }
        }
    }
    
    // Cleanup remaining allocations
    for (void* ptr : active_pointers) {
        std::free(ptr);
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    result.allocation_time_seconds = std::chrono::duration<f64>(end - start).count();
    
    calculate_allocator_metrics(result);
    
    return result;
}

SingleAllocatorResult AllocationBenchmark::benchmark_mixed_pattern() {
    // Mixed pattern: some sequential, some random
    auto sequential_result = benchmark_sequential_pattern();
    auto random_result = benchmark_random_pattern();
    
    // Combine results
    SingleAllocatorResult result{};
    result.allocator_name = "MixedPattern";
    result.successful_allocations = (sequential_result.successful_allocations + random_result.successful_allocations) / 2;
    result.failed_allocations = (sequential_result.failed_allocations + random_result.failed_allocations) / 2;
    result.total_bytes_allocated = (sequential_result.total_bytes_allocated + random_result.total_bytes_allocated) / 2;
    result.allocation_time_seconds = (sequential_result.allocation_time_seconds + random_result.allocation_time_seconds) / 2;
    
    calculate_allocator_metrics(result);
    
    return result;
}

SingleAllocatorResult AllocationBenchmark::benchmark_burst_pattern() {
    // Burst pattern: periods of intensive allocation followed by intensive deallocation
    SingleAllocatorResult result{};
    result.allocator_name = "BurstPattern";
    
    const usize burst_size = config_.iteration_count / 10; // 10 bursts
    std::vector<void*> burst_pointers;
    
    auto start = std::chrono::high_resolution_clock::now();
    
    for (usize burst = 0; burst < 10; ++burst) {
        burst_pointers.clear();
        burst_pointers.reserve(burst_size);
        
        // Allocation burst
        for (usize i = 0; i < burst_size; ++i) {
            usize size_index = (burst * burst_size + i) % allocation_sizes_.size();
            void* ptr = std::malloc(allocation_sizes_[size_index]);
            if (ptr) {
                burst_pointers.push_back(ptr);
                result.successful_allocations++;
                result.total_bytes_allocated += allocation_sizes_[size_index];
            } else {
                result.failed_allocations++;
            }
        }
        
        // Deallocation burst
        for (void* ptr : burst_pointers) {
            std::free(ptr);
        }
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    result.allocation_time_seconds = std::chrono::duration<f64>(end - start).count();
    
    calculate_allocator_metrics(result);
    
    return result;
}

void AllocationBenchmark::calculate_allocator_metrics(SingleAllocatorResult& result) {
    if (result.successful_allocations > 0) {
        result.average_allocation_time_ns = (result.allocation_time_seconds * 1e9) / result.successful_allocations;
        result.average_allocation_size = static_cast<f64>(result.total_bytes_allocated) / result.successful_allocations;
        result.throughput_allocations_per_second = result.successful_allocations / 
            std::max(result.allocation_time_seconds, 1e-9);
        result.throughput_bytes_per_second = result.total_bytes_allocated / 
            std::max(result.allocation_time_seconds, 1e-9);
    }
    
    f64 total_time = result.allocation_time_seconds + result.deallocation_time_seconds;
    if (total_time > 0) {
        result.memory_efficiency_score = result.successful_allocations / total_time;
    }
}

void AllocationBenchmark::calculate_comparative_metrics(BenchmarkResult& result) {
    if (result.individual_results.empty()) return;
    
    // Find fastest allocator as baseline
    f64 best_allocation_time = std::numeric_limits<f64>::max();
    std::string fastest_allocator;
    
    for (const auto& [name, individual_result] : result.individual_results) {
        if (individual_result.allocation_time_seconds < best_allocation_time) {
            best_allocation_time = individual_result.allocation_time_seconds;
            fastest_allocator = name;
        }
    }
    
    // Calculate relative performance
    for (auto& [name, individual_result] : result.individual_results) {
        f64 relative_performance = best_allocation_time / 
            std::max(individual_result.allocation_time_seconds, 1e-9);
        individual_result.allocator_specific_metrics["relative_performance"] = relative_performance;
        individual_result.allocator_specific_metrics["speed_improvement"] = 
            (relative_performance - 1.0) * 100.0;
    }
    
    result.summary_metrics["fastest_allocator"] = fastest_allocator;
    result.summary_metrics["best_allocation_time"] = best_allocation_time;
}

void AllocationBenchmark::calculate_threading_metrics(BenchmarkResult& result) {
    // Calculate contention and scalability metrics
    f64 total_contention_factor = 0.0;
    f64 total_scalability_factor = 0.0;
    usize valid_results = 0;
    
    for (const auto& [name, individual_result] : result.individual_results) {
        if (individual_result.successful_allocations > 0) {
            f64 theoretical_ideal_time = individual_result.allocation_time_seconds / config_.thread_count;
            f64 contention_factor = individual_result.allocation_time_seconds / 
                std::max(theoretical_ideal_time, 1e-9);
            
            individual_result.allocator_specific_metrics["contention_factor"] = contention_factor;
            total_contention_factor += contention_factor;
            
            f64 scalability_factor = config_.thread_count / contention_factor;
            individual_result.allocator_specific_metrics["scalability_factor"] = scalability_factor;
            total_scalability_factor += scalability_factor;
            
            valid_results++;
        }
    }
    
    if (valid_results > 0) {
        result.summary_metrics["average_contention_factor"] = total_contention_factor / valid_results;
        result.summary_metrics["average_scalability_factor"] = total_scalability_factor / valid_results;
    }
}

void AllocationBenchmark::calculate_pattern_metrics(BenchmarkResult& result) {
    // Analyze how different patterns affect performance
    std::map<std::string, f64> pattern_efficiency;
    
    for (const auto& [pattern_name, individual_result] : result.individual_results) {
        if (individual_result.successful_allocations > 0) {
            f64 efficiency = individual_result.throughput_allocations_per_second / 
                           individual_result.average_allocation_time_ns;
            pattern_efficiency[pattern_name] = efficiency;
        }
    }
    
    // Find most efficient pattern
    auto best_pattern = std::max_element(pattern_efficiency.begin(), pattern_efficiency.end(),
                                       [](const auto& a, const auto& b) {
                                           return a.second < b.second;
                                       });
    
    if (best_pattern != pattern_efficiency.end()) {
        result.summary_metrics["most_efficient_pattern"] = best_pattern->first;
        result.summary_metrics["best_pattern_efficiency"] = best_pattern->second;
    }
}

f64 AllocationBenchmark::get_current_time() const {
    using namespace std::chrono;
    return duration<f64>(steady_clock::now().time_since_epoch()).count();
}

//=============================================================================
// Global Benchmark Instance
//=============================================================================

std::unique_ptr<AllocationBenchmark> create_benchmark_suite(const BenchmarkConfiguration& config) {
    return std::make_unique<AllocationBenchmark>(config);
}

} // namespace ecscope::memory::benchmark