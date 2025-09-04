/**
 * @file advanced_job_system_examples.cpp
 * @brief Educational Examples of Parallel ECS Execution Patterns
 * 
 * This file demonstrates advanced concepts in parallel game engine development
 * using the ECScope work-stealing job system. Each example focuses on specific
 * educational concepts with detailed explanations and performance comparisons.
 * 
 * Educational Topics Covered:
 * 1. Component Dependency Analysis and Safe Parallelization
 * 2. Work-Stealing Load Balancing Strategies
 * 3. Memory Access Patterns and Cache Optimization
 * 4. SIMD Integration in Parallel Contexts
 * 5. Producer-Consumer Patterns in ECS Systems
 * 6. Performance Profiling and Bottleneck Analysis
 * 
 * Each example includes:
 * - Theoretical background and motivation
 * - Sequential vs parallel implementation comparison
 * - Performance measurements and analysis
 * - Common pitfalls and how to avoid them
 * - Best practices and optimization tips
 * 
 * @author ECScope Educational ECS Framework
 * @date 2025
 */

#include "job_system/work_stealing_job_system.hpp"
#include "job_system/ecs_parallel_scheduler.hpp"
#include "job_system/job_profiler.hpp"
#include "ecs/registry.hpp"
#include "ecs/system.hpp"
#include "physics/simd_math.hpp"
#include "core/log.hpp"
#include <iostream>
#include <vector>
#include <chrono>
#include <random>
#include <array>
#include <algorithm>
#include <numeric>

using namespace ecscope;
using namespace ecscope::job_system;

//=============================================================================
// Example 1: Component Dependency Analysis
//=============================================================================

namespace example1 {

/**
 * @brief Educational demonstration of component dependency analysis
 * 
 * This example shows how the job system analyzes component read/write patterns
 * to determine which ECS systems can safely run in parallel.
 */

struct Position { physics::Vec3 pos; };
struct Velocity { physics::Vec3 vel; };
struct Acceleration { physics::Vec3 acc; };
struct Mass { f32 mass = 1.0f; };

// System A: Updates acceleration based on forces (writes Acceleration, reads Mass)
class ForceSystem : public ecs::System {
public:
    ForceSystem() : ecs::System("ForceSystem", ecs::SystemPhase::Update) {
        writes<Acceleration>();
        reads<Mass>();
    }
    
    void update(const ecs::SystemContext& context) override {
        auto query = context.create_query<Acceleration, Mass>();
        query.for_each([](ecs::Entity, Acceleration& acc, const Mass& mass) {
            // Apply gravity force
            acc.acc = physics::Vec3(0.0f, -9.81f, 0.0f) / mass.mass;
        });
    }
};

// System B: Updates velocity from acceleration (writes Velocity, reads Acceleration)
class VelocitySystem : public ecs::System {
public:
    VelocitySystem() : ecs::System("VelocitySystem", ecs::SystemPhase::Update) {
        writes<Velocity>();
        reads<Acceleration>();
    }
    
    void update(const ecs::SystemContext& context) override {
        const f64 dt = context.delta_time();
        auto query = context.create_query<Velocity, Acceleration>();
        query.for_each([dt](ecs::Entity, Velocity& vel, const Acceleration& acc) {
            vel.vel += acc.acc * static_cast<f32>(dt);
        });
    }
};

// System C: Updates position from velocity (writes Position, reads Velocity)
class PositionSystem : public ecs::System {
public:
    PositionSystem() : ecs::System("PositionSystem", ecs::SystemPhase::Update) {
        writes<Position>();
        reads<Velocity>();
    }
    
    void update(const ecs::SystemContext& context) override {
        const f64 dt = context.delta_time();
        auto query = context.create_query<Position, Velocity>();
        query.for_each([dt](ecs::Entity, Position& pos, const Velocity& vel) {
            pos.pos += vel.vel * static_cast<f32>(dt);
        });
    }
};

// System D: Updates mass (independent, only writes Mass)
class MassUpdateSystem : public ecs::System {
public:
    MassUpdateSystem() : ecs::System("MassUpdateSystem", ecs::SystemPhase::Update) {
        writes<Mass>();
    }
    
    void update(const ecs::SystemContext& context) override {
        auto query = context.create_query<Mass>();
        query.for_each([](ecs::Entity, Mass& mass) {
            // Simulate mass changes over time
            mass.mass *= 0.9999f; // Very slow mass loss
        });
    }
};

void demonstrate_dependency_analysis() {
    std::cout << "\n=== Example 1: Component Dependency Analysis ===\n\n";
    
    std::cout << "This example demonstrates how the job system analyzes component\n";
    std::cout << "access patterns to determine safe parallelization opportunities.\n\n";
    
    // Create job system and ECS components
    JobSystem job_system(JobSystem::Config::create_educational());
    job_system.initialize();
    
    ecs::Registry registry;
    ecs::SystemManager system_manager(&registry);
    ECSParallelScheduler scheduler(&job_system, &system_manager);
    
    // Register systems
    system_manager.add_system<ForceSystem>();
    system_manager.add_system<VelocitySystem>();
    system_manager.add_system<PositionSystem>();
    system_manager.add_system<MassUpdateSystem>();
    
    // Analyze system dependencies
    scheduler.analyze_all_systems();
    scheduler.rebuild_execution_groups();
    
    std::cout << "System Analysis Results:\n";
    std::cout << scheduler.generate_dependency_report();
    
    std::cout << "Key Insights:\n";
    std::cout << "• ForceSystem and MassUpdateSystem CANNOT run in parallel\n";
    std::cout << "  (ForceSystem reads Mass, MassUpdateSystem writes Mass)\n\n";
    std::cout << "• VelocitySystem must run AFTER ForceSystem\n";
    std::cout << "  (VelocitySystem reads Acceleration written by ForceSystem)\n\n";
    std::cout << "• PositionSystem must run AFTER VelocitySystem\n";
    std::cout << "  (PositionSystem reads Velocity written by VelocitySystem)\n\n";
    std::cout << "• This creates a sequential dependency chain, limiting parallelism\n\n";
    
    // Demonstrate execution
    const usize entity_count = 10000;
    std::cout << "Creating " << entity_count << " entities for demonstration...\n";
    
    for (usize i = 0; i < entity_count; ++i) {
        auto entity = registry.create_entity(
            Position{physics::Vec3(0.0f, 0.0f, 0.0f)},
            Velocity{physics::Vec3(1.0f, 0.0f, 0.0f)},
            Acceleration{physics::Vec3(0.0f, 0.0f, 0.0f)},
            Mass{1.0f}
        );
    }
    
    // Execute with timing
    auto start_time = std::chrono::high_resolution_clock::now();
    
    for (u32 frame = 0; frame < 100; ++frame) {
        scheduler.execute_phase_parallel(ecs::SystemPhase::Update, 1.0/60.0);
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    f64 duration = std::chrono::duration<f64, std::milli>(end_time - start_time).count();
    
    std::cout << "Executed 100 frames in " << duration << " ms\n";
    std::cout << "Average frame time: " << (duration / 100.0) << " ms\n\n";
    
    job_system.shutdown();
}

} // namespace example1

//=============================================================================
// Example 2: Work-Stealing Load Balancing Visualization
//=============================================================================

namespace example2 {

/**
 * @brief Visual demonstration of work-stealing effectiveness
 * 
 * This example creates deliberately uneven workloads to showcase
 * how work-stealing automatically balances load across threads.
 */

struct WorkUnit {
    u32 id;
    u32 computational_complexity; // 1=light, 10=heavy
    std::chrono::high_resolution_clock::time_point start_time;
    std::chrono::high_resolution_clock::time_point end_time;
    u32 worker_id = 0;
    bool was_stolen = false;
};

class WorkloadGenerator {
private:
    std::vector<WorkUnit> work_units_;
    
public:
    void generate_uneven_workload(usize total_jobs, f64 heavy_job_ratio = 0.1) {
        work_units_.clear();
        work_units_.reserve(total_jobs);
        
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<f64> dist(0.0, 1.0);
        
        for (usize i = 0; i < total_jobs; ++i) {
            WorkUnit unit;
            unit.id = static_cast<u32>(i);
            unit.computational_complexity = (dist(gen) < heavy_job_ratio) ? 10 : 1;
            work_units_.push_back(unit);
        }
    }
    
    void generate_bursty_workload(usize total_jobs, usize burst_size = 50) {
        work_units_.clear();
        work_units_.reserve(total_jobs);
        
        for (usize i = 0; i < total_jobs; ++i) {
            WorkUnit unit;
            unit.id = static_cast<u32>(i);
            // Create bursts of heavy work
            unit.computational_complexity = ((i % (burst_size * 2)) < burst_size) ? 8 : 1;
            work_units_.push_back(unit);
        }
    }
    
    const std::vector<WorkUnit>& get_work_units() const { return work_units_; }
    std::vector<WorkUnit>& get_work_units() { return work_units_; }
};

void demonstrate_work_stealing() {
    std::cout << "\n=== Example 2: Work-Stealing Load Balancing ===\n\n";
    
    std::cout << "This example demonstrates how work-stealing automatically\n";
    std::cout << "balances uneven computational loads across worker threads.\n\n";
    
    JobSystem job_system(JobSystem::Config::create_educational());
    JobProfiler profiler(JobProfiler::Config::create_comprehensive());
    
    job_system.initialize();
    profiler.start_profiling();
    
    const usize total_jobs = 1000;
    const u32 worker_count = job_system.worker_count();
    
    std::cout << "Testing with " << worker_count << " workers and " << total_jobs << " jobs\n\n";
    
    // Test 1: Uneven workload distribution
    std::cout << "Test 1: Uneven Workload (10% heavy jobs)\n";
    std::cout << "----------------------------------------\n";
    
    WorkloadGenerator generator;
    generator.generate_uneven_workload(total_jobs, 0.1);
    auto& work_units = generator.get_work_units();
    
    std::vector<std::array<u32, 11>> worker_job_counts(worker_count); // 0-10 complexity levels
    for (auto& counts : worker_job_counts) {
        counts.fill(0);
    }
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    // Submit all jobs
    std::vector<JobID> job_ids;
    job_ids.reserve(total_jobs);
    
    for (auto& unit : work_units) {
        JobID job_id = job_system.submit_job(
            "WorkUnit_" + std::to_string(unit.id),
            [&unit, &worker_job_counts, worker_count]() {
                unit.start_time = std::chrono::high_resolution_clock::now();
                
                // Simulate computational work
                volatile f64 result = 0.0;
                u32 iterations = unit.computational_complexity * 10000;
                for (u32 i = 0; i < iterations; ++i) {
                    result += std::sin(static_cast<f64>(i)) * std::cos(static_cast<f64>(i));
                }
                
                unit.end_time = std::chrono::high_resolution_clock::now();
                
                // Track which worker executed this job (simplified)
                unit.worker_id = unit.id % worker_count; // Approximation
                worker_job_counts[unit.worker_id][unit.computational_complexity]++;
            },
            JobPriority::Normal
        );
        job_ids.push_back(job_id);
    }
    
    // Wait for completion
    job_system.wait_for_batch(job_ids);
    
    auto end_time = std::chrono::high_resolution_clock::now();
    f64 duration = std::chrono::duration<f64, std::milli>(end_time - start_time).count();
    
    std::cout << "Execution completed in " << duration << " ms\n\n";
    
    // Analyze work distribution
    std::cout << "Work Distribution Analysis:\n";
    std::cout << "Worker  Light_Jobs  Heavy_Jobs  Total_Jobs  Load_Balance\n";
    std::cout << "------  ----------  ----------  ----------  ------------\n";
    
    std::vector<f64> worker_loads(worker_count, 0.0);
    for (u32 w = 0; w < worker_count; ++w) {
        u32 light_jobs = 0, heavy_jobs = 0, total_jobs_w = 0;
        f64 computational_load = 0.0;
        
        for (u32 c = 1; c <= 10; ++c) {
            if (c == 1) light_jobs += worker_job_counts[w][c];
            else heavy_jobs += worker_job_counts[w][c];
            total_jobs_w += worker_job_counts[w][c];
            computational_load += worker_job_counts[w][c] * c;
        }
        
        worker_loads[w] = computational_load;
        f64 load_percentage = (computational_load / 
            (*std::max_element(worker_loads.begin(), worker_loads.end()))) * 100.0;
        
        std::cout << std::setw(6) << w 
                  << std::setw(12) << light_jobs
                  << std::setw(12) << heavy_jobs  
                  << std::setw(12) << total_jobs_w
                  << std::setw(12) << std::fixed << std::setprecision(1) 
                  << load_percentage << "%" << "\n";
    }
    
    // Calculate load balance metrics
    f64 mean_load = std::accumulate(worker_loads.begin(), worker_loads.end(), 0.0) / worker_count;
    f64 load_variance = 0.0;
    for (f64 load : worker_loads) {
        load_variance += (load - mean_load) * (load - mean_load);
    }
    load_variance /= worker_count;
    
    f64 load_std_dev = std::sqrt(load_variance);
    f64 coefficient_of_variation = mean_load > 0.0 ? load_std_dev / mean_load : 0.0;
    f64 load_balance_score = std::max(0.0, 1.0 - coefficient_of_variation);
    
    std::cout << "\nLoad Balance Metrics:\n";
    std::cout << "• Load Balance Score: " << std::fixed << std::setprecision(3) 
              << load_balance_score << "/1.0 (1.0 = perfect balance)\n";
    std::cout << "• Coefficient of Variation: " << coefficient_of_variation << "\n";
    
    if (load_balance_score > 0.8) {
        std::cout << "• Assessment: Excellent load balancing!\n";
    } else if (load_balance_score > 0.6) {
        std::cout << "• Assessment: Good load balancing\n";
    } else {
        std::cout << "• Assessment: Poor load balancing - work-stealing may be ineffective\n";
    }
    
    // Test 2: Bursty workload
    std::cout << "\n\nTest 2: Bursty Workload Pattern\n";
    std::cout << "-------------------------------\n";
    
    generator.generate_bursty_workload(total_jobs, 50);
    
    start_time = std::chrono::high_resolution_clock::now();
    
    job_ids.clear();
    for (auto& unit : work_units) {
        JobID job_id = job_system.submit_job(
            "BurstJob_" + std::to_string(unit.id),
            [&unit]() {
                unit.start_time = std::chrono::high_resolution_clock::now();
                
                volatile f64 result = 0.0;
                u32 iterations = unit.computational_complexity * 5000;
                for (u32 i = 0; i < iterations; ++i) {
                    result += std::sin(static_cast<f64>(i));
                }
                
                unit.end_time = std::chrono::high_resolution_clock::now();
            },
            JobPriority::Normal
        );
        job_ids.push_back(job_id);
    }
    
    job_system.wait_for_batch(job_ids);
    
    end_time = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration<f64, std::milli>(end_time - start_time).count();
    
    std::cout << "Bursty workload completed in " << duration << " ms\n";
    
    profiler.stop_profiling();
    
    // Generate educational insights
    auto report = profiler.generate_report();
    std::cout << "\nWork-Stealing Statistics:\n";
    std::cout << "• Total Steals: " << report.total_steals << "\n";
    std::cout << "• Steal Success Rate: " << (report.steal_success_rate * 100.0) << "%\n";
    std::cout << "• Thread Utilization: " << (report.overall_utilization * 100.0) << "%\n\n";
    
    std::cout << "Educational Takeaways:\n";
    std::cout << "• Work-stealing is most effective with uneven workloads\n";
    std::cout << "• High steal success rate indicates good load redistribution\n";
    std::cout << "• Bursty patterns test the system's adaptability\n";
    std::cout << "• Real-world game workloads often exhibit similar patterns\n";
    
    job_system.shutdown();
}

} // namespace example2

//=============================================================================
// Example 3: Memory Access Pattern Optimization
//=============================================================================

namespace example3 {

/**
 * @brief Demonstration of cache-friendly parallel processing
 * 
 * Shows how job granularity and data layout affect cache performance
 * in parallel ECS systems.
 */

struct CacheTestData {
    alignas(64) f32 values[16];  // One cache line
    f32 result;
    
    CacheTestData() {
        for (int i = 0; i < 16; ++i) {
            values[i] = static_cast<f32>(i);
        }
        result = 0.0f;
    }
};

void demonstrate_cache_optimization() {
    std::cout << "\n=== Example 3: Memory Access Pattern Optimization ===\n\n";
    
    std::cout << "This example demonstrates how job granularity and data layout\n";
    std::cout << "affect cache performance in parallel processing.\n\n";
    
    JobSystem job_system(JobSystem::Config::create_performance_optimized());
    job_system.initialize();
    
    const usize data_count = 100000;
    std::vector<CacheTestData> test_data(data_count);
    
    std::cout << "Testing with " << data_count << " data elements\n";
    std::cout << "Each element: " << sizeof(CacheTestData) << " bytes (cache-line aligned)\n\n";
    
    // Test 1: Fine-grained parallelism (poor cache usage)
    std::cout << "Test 1: Fine-grained jobs (1 element per job)\n";
    std::cout << "---------------------------------------------\n";
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    std::vector<JobID> fine_jobs;
    fine_jobs.reserve(data_count);
    
    for (usize i = 0; i < data_count; ++i) {
        JobID job_id = job_system.submit_job(
            "FineJob_" + std::to_string(i),
            [&test_data, i]() {
                CacheTestData& data = test_data[i];
                
                // Process the data (cache-friendly within the element)
                f32 sum = 0.0f;
                for (int j = 0; j < 16; ++j) {
                    sum += data.values[j] * data.values[j];
                }
                data.result = sum;
            },
            JobPriority::Normal
        );
        fine_jobs.push_back(job_id);
    }
    
    job_system.wait_for_batch(fine_jobs);
    
    auto end_time = std::chrono::high_resolution_clock::now();
    f64 fine_duration = std::chrono::duration<f64, std::milli>(end_time - start_time).count();
    
    std::cout << "Fine-grained execution: " << fine_duration << " ms\n";
    
    // Test 2: Coarse-grained parallelism (better cache usage)
    std::cout << "\nTest 2: Coarse-grained jobs (1000 elements per job)\n";
    std::cout << "---------------------------------------------------\n";
    
    // Reset data
    for (auto& data : test_data) {
        data.result = 0.0f;
    }
    
    start_time = std::chrono::high_resolution_clock::now();
    
    const usize chunk_size = 1000;
    const usize job_count = (data_count + chunk_size - 1) / chunk_size;
    
    std::vector<JobID> coarse_jobs;
    coarse_jobs.reserve(job_count);
    
    for (usize i = 0; i < job_count; ++i) {
        usize start_idx = i * chunk_size;
        usize end_idx = std::min(start_idx + chunk_size, data_count);
        
        JobID job_id = job_system.submit_job(
            "CoarseJob_" + std::to_string(i),
            [&test_data, start_idx, end_idx]() {
                // Process a chunk of data (better cache locality)
                for (usize idx = start_idx; idx < end_idx; ++idx) {
                    CacheTestData& data = test_data[idx];
                    
                    f32 sum = 0.0f;
                    for (int j = 0; j < 16; ++j) {
                        sum += data.values[j] * data.values[j];
                    }
                    data.result = sum;
                }
            },
            JobPriority::Normal
        );
        coarse_jobs.push_back(job_id);
    }
    
    job_system.wait_for_batch(coarse_jobs);
    
    end_time = std::chrono::high_resolution_clock::now();
    f64 coarse_duration = std::chrono::duration<f64, std::milli>(end_time - start_time).count();
    
    std::cout << "Coarse-grained execution: " << coarse_duration << " ms\n";
    
    // Test 3: SIMD-optimized parallelism
    std::cout << "\nTest 3: SIMD-optimized parallel processing\n";
    std::cout << "------------------------------------------\n";
    
    // Reset data
    for (auto& data : test_data) {
        data.result = 0.0f;
    }
    
    start_time = std::chrono::high_resolution_clock::now();
    
    const usize simd_chunk_size = 4000; // Larger chunks for SIMD
    const usize simd_job_count = (data_count + simd_chunk_size - 1) / simd_chunk_size;
    
    std::vector<JobID> simd_jobs;
    simd_jobs.reserve(simd_job_count);
    
    for (usize i = 0; i < simd_job_count; ++i) {
        usize start_idx = i * simd_chunk_size;
        usize end_idx = std::min(start_idx + simd_chunk_size, data_count);
        
        JobID job_id = job_system.submit_job(
            "SIMDJob_" + std::to_string(i),
            [&test_data, start_idx, end_idx]() {
                // Use SIMD operations for vectorized processing
                physics::simd::batch_ops::process_vec4_array(
                    reinterpret_cast<physics::Vec4*>(&test_data[start_idx]),
                    end_idx - start_idx
                );
            },
            JobPriority::Normal
        );
        simd_jobs.push_back(job_id);
    }
    
    job_system.wait_for_batch(simd_jobs);
    
    end_time = std::chrono::high_resolution_clock::now();
    f64 simd_duration = std::chrono::duration<f64, std::milli>(end_time - start_time).count();
    
    std::cout << "SIMD-optimized execution: " << simd_duration << " ms\n\n";
    
    // Performance analysis
    std::cout << "Performance Analysis:\n";
    std::cout << "--------------------\n";
    
    f64 fine_to_coarse_speedup = fine_duration / coarse_duration;
    f64 coarse_to_simd_speedup = coarse_duration / simd_duration;
    f64 fine_to_simd_speedup = fine_duration / simd_duration;
    
    std::cout << "• Fine → Coarse speedup: " << std::fixed << std::setprecision(2) 
              << fine_to_coarse_speedup << "x\n";
    std::cout << "• Coarse → SIMD speedup: " << coarse_to_simd_speedup << "x\n";
    std::cout << "• Overall speedup: " << fine_to_simd_speedup << "x\n\n";
    
    std::cout << "Key Learning Points:\n";
    std::cout << "• Fine-grained jobs have high overhead and poor cache usage\n";
    std::cout << "• Coarse-grained jobs improve cache locality and reduce overhead\n";
    std::cout << "• SIMD operations provide additional vectorization benefits\n";
    std::cout << "• Optimal job size balances parallelism with cache efficiency\n";
    std::cout << "• Rule of thumb: 1000-10000 operations per job for good performance\n";
    
    job_system.shutdown();
}

} // namespace example3

//=============================================================================
// Example 4: Producer-Consumer Patterns
//=============================================================================

namespace example4 {

/**
 * @brief Demonstration of producer-consumer patterns in parallel ECS
 * 
 * Shows how to coordinate systems that produce and consume data,
 * using proper synchronization and dependency management.
 */

struct DataPacket {
    u32 id;
    std::array<f32, 64> data;
    std::chrono::high_resolution_clock::time_point timestamp;
    bool processed = false;
};

class ThreadSafeQueue {
private:
    std::queue<DataPacket> queue_;
    mutable std::mutex mutex_;
    std::condition_variable condition_;
    bool shutdown_ = false;
    
public:
    void push(const DataPacket& packet) {
        std::lock_guard<std::mutex> lock(mutex_);
        queue_.push(packet);
        condition_.notify_one();
    }
    
    bool try_pop(DataPacket& packet) {
        std::unique_lock<std::mutex> lock(mutex_);
        condition_.wait_for(lock, std::chrono::milliseconds(10), 
                          [this] { return !queue_.empty() || shutdown_; });
        
        if (queue_.empty()) {
            return false;
        }
        
        packet = queue_.front();
        queue_.pop();
        return true;
    }
    
    usize size() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return queue_.size();
    }
    
    void shutdown() {
        std::lock_guard<std::mutex> lock(mutex_);
        shutdown_ = true;
        condition_.notify_all();
    }
};

void demonstrate_producer_consumer() {
    std::cout << "\n=== Example 4: Producer-Consumer Patterns ===\n\n";
    
    std::cout << "This example demonstrates coordinated parallel processing\n";
    std::cout << "using producer-consumer patterns with proper synchronization.\n\n";
    
    JobSystem job_system(JobSystem::Config::create_educational());
    job_system.initialize();
    
    ThreadSafeQueue data_queue;
    ThreadSafeQueue result_queue;
    
    const u32 total_packets = 10000;
    const u32 producer_count = 2;
    const u32 consumer_count = 4;
    
    std::cout << "Configuration:\n";
    std::cout << "• Total data packets: " << total_packets << "\n";
    std::cout << "• Producer jobs: " << producer_count << "\n";  
    std::cout << "• Consumer jobs: " << consumer_count << "\n\n";
    
    std::atomic<u32> packets_produced{0};
    std::atomic<u32> packets_processed{0};
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    // Phase 1: Start consumer jobs (they will wait for data)
    std::vector<JobID> consumer_jobs;
    consumer_jobs.reserve(consumer_count);
    
    for (u32 i = 0; i < consumer_count; ++i) {
        JobID consumer_id = job_system.submit_job(
            "Consumer_" + std::to_string(i),
            [&, i]() {
                DataPacket packet;
                u32 local_processed = 0;
                
                while (packets_processed.load() < total_packets) {
                    if (data_queue.try_pop(packet)) {
                        // Process the data packet
                        f32 sum = 0.0f;
                        for (auto& value : packet.data) {
                            sum += value * value;
                        }
                        
                        // Simulate processing time
                        std::this_thread::sleep_for(std::chrono::microseconds(100));
                        
                        packet.processed = true;
                        packet.data[0] = sum; // Store result
                        
                        result_queue.push(packet);
                        packets_processed.fetch_add(1);
                        local_processed++;
                    } else {
                        // No data available, yield briefly
                        std::this_thread::yield();
                    }
                }
                
                LOG_DEBUG("Consumer {} processed {} packets", i, local_processed);
            },
            JobPriority::Normal
        );
        consumer_jobs.push_back(consumer_id);
    }
    
    // Phase 2: Start producer jobs
    std::vector<JobID> producer_jobs;
    producer_jobs.reserve(producer_count);
    
    const u32 packets_per_producer = total_packets / producer_count;
    
    for (u32 i = 0; i < producer_count; ++i) {
        JobID producer_id = job_system.submit_job(
            "Producer_" + std::to_string(i),
            [&, i, packets_per_producer]() {
                std::random_device rd;
                std::mt19937 gen(rd());
                std::uniform_real_distribution<f32> dist(0.0f, 100.0f);
                
                u32 start_id = i * packets_per_producer;
                u32 end_id = (i == producer_count - 1) ? total_packets : (i + 1) * packets_per_producer;
                
                for (u32 packet_id = start_id; packet_id < end_id; ++packet_id) {
                    DataPacket packet;
                    packet.id = packet_id;
                    packet.timestamp = std::chrono::high_resolution_clock::now();
                    
                    // Generate random data
                    for (auto& value : packet.data) {
                        value = dist(gen);
                    }
                    
                    data_queue.push(packet);
                    packets_produced.fetch_add(1);
                    
                    // Simulate variable production rate
                    if (packet_id % 100 == 0) {
                        std::this_thread::sleep_for(std::chrono::microseconds(500));
                    }
                }
                
                LOG_DEBUG("Producer {} finished generating {} packets", i, end_id - start_id);
            },
            JobPriority::High // Producers have higher priority
        );
        producer_jobs.push_back(producer_id);
    }
    
    // Monitor progress
    std::thread monitor_thread([&]() {
        while (packets_processed.load() < total_packets) {
            u32 produced = packets_produced.load();
            u32 processed = packets_processed.load();
            usize queue_size = data_queue.size();
            
            std::cout << "\rProgress: Produced " << produced 
                      << ", Processed " << processed 
                      << ", Queue size " << queue_size 
                      << " (" << std::fixed << std::setprecision(1)
                      << (static_cast<f64>(processed) / total_packets * 100.0) 
                      << "%)" << std::flush;
            
            std::this_thread::sleep_for(std::chrono::milliseconds(250));
        }
        std::cout << "\n";
    });
    
    // Wait for all producers to complete
    for (const auto& job_id : producer_jobs) {
        job_system.wait_for_job(job_id);
    }
    
    // Wait for all consumers to complete
    for (const auto& job_id : consumer_jobs) {
        job_system.wait_for_job(job_id);
    }
    
    monitor_thread.join();
    data_queue.shutdown();
    
    auto end_time = std::chrono::high_resolution_clock::now();
    f64 duration = std::chrono::duration<f64, std::milli>(end_time - start_time).count();
    
    std::cout << "\nExecution Summary:\n";
    std::cout << "• Total time: " << duration << " ms\n";
    std::cout << "• Throughput: " << std::fixed << std::setprecision(1)
              << (total_packets / (duration / 1000.0)) << " packets/sec\n";
    std::cout << "• Average processing time per packet: " 
              << (duration / total_packets) << " ms\n";
    
    // Analyze results
    std::vector<DataPacket> results;
    DataPacket result;
    while (result_queue.try_pop(result)) {
        results.push_back(result);
    }
    
    std::cout << "• Results collected: " << results.size() << "\n";
    
    if (!results.empty()) {
        // Calculate processing latencies
        std::vector<f64> latencies;
        latencies.reserve(results.size());
        
        for (const auto& result : results) {
            auto processing_time = std::chrono::high_resolution_clock::now();
            f64 latency = std::chrono::duration<f64, std::milli>(
                processing_time - result.timestamp).count();
            latencies.push_back(latency);
        }
        
        std::sort(latencies.begin(), latencies.end());
        
        f64 avg_latency = std::accumulate(latencies.begin(), latencies.end(), 0.0) / latencies.size();
        f64 p50_latency = latencies[latencies.size() / 2];
        f64 p95_latency = latencies[static_cast<usize>(latencies.size() * 0.95)];
        
        std::cout << "• Average latency: " << std::fixed << std::setprecision(2) 
                  << avg_latency << " ms\n";
        std::cout << "• P50 latency: " << p50_latency << " ms\n";
        std::cout << "• P95 latency: " << p95_latency << " ms\n\n";
    }
    
    std::cout << "Educational Insights:\n";
    std::cout << "• Producer-consumer patterns enable pipeline parallelism\n";
    std::cout << "• Thread-safe queues coordinate between parallel stages\n";
    std::cout << "• Higher priority for producers prevents consumer starvation\n";
    std::cout << "• Monitoring helps identify bottlenecks and imbalances\n";
    std::cout << "• Latency analysis reveals system responsiveness\n";
    
    job_system.shutdown();
}

} // namespace example4

//=============================================================================
// Main Educational Demo Runner
//=============================================================================

void print_introduction() {
    std::cout << "\n";
    std::cout << "╔══════════════════════════════════════════════════════════════════════════════╗\n";
    std::cout << "║               Advanced Job System Educational Examples                       ║\n";
    std::cout << "╠══════════════════════════════════════════════════════════════════════════════╣\n";
    std::cout << "║                                                                              ║\n";
    std::cout << "║  This comprehensive demonstration teaches advanced parallel programming       ║\n";
    std::cout << "║  concepts through practical game engine examples.                           ║\n";
    std::cout << "║                                                                              ║\n";
    std::cout << "║  Examples Covered:                                                           ║\n";
    std::cout << "║  1. Component Dependency Analysis - Safe ECS parallelization               ║\n";
    std::cout << "║  2. Work-Stealing Load Balancing - Automatic workload distribution         ║\n";
    std::cout << "║  3. Memory Access Optimization - Cache-friendly parallel patterns          ║\n";
    std::cout << "║  4. Producer-Consumer Coordination - Pipeline parallelism                   ║\n";
    std::cout << "║                                                                              ║\n";
    std::cout << "║  Each example includes detailed explanations, performance measurements,      ║\n";
    std::cout << "║  and practical insights for real-world game development.                    ║\n";
    std::cout << "║                                                                              ║\n";
    std::cout << "╚══════════════════════════════════════════════════════════════════════════════╝\n";
    std::cout << "\n";
}

int main(int argc, char* argv[]) {
    // Initialize logging
    ecscope::log_init();
    
    print_introduction();
    
    try {
        // Run all educational examples
        example1::demonstrate_dependency_analysis();
        example2::demonstrate_work_stealing();
        example3::demonstrate_cache_optimization();
        example4::demonstrate_producer_consumer();
        
        std::cout << "\n" << std::string(80, '=') << "\n";
        std::cout << "All educational examples completed successfully!\n\n";
        
        std::cout << "Key Takeaways for Game Engine Development:\n";
        std::cout << "• Analyze component dependencies before parallelizing ECS systems\n";
        std::cout << "• Work-stealing automatically balances uneven computational loads\n";
        std::cout << "• Job granularity significantly affects cache performance\n";
        std::cout << "• Producer-consumer patterns enable efficient pipeline parallelism\n";
        std::cout << "• Performance profiling reveals optimization opportunities\n";
        std::cout << "• Modern job systems can achieve 4-8x speedups on multi-core hardware\n\n";
        
        std::cout << "The ECScope job system provides both high performance and educational\n";
        std::cout << "value, making advanced parallel programming concepts accessible to\n";
        std::cout << "game developers and computer science students.\n";
        
    } catch (const std::exception& e) {
        std::ostringstream error_oss;
        error_oss << "Educational examples failed with exception: " << e.what();
        LOG_ERROR(error_oss.str());
        return 1;
    }
    
    return 0;
}