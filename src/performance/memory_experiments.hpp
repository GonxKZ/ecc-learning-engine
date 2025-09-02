#pragma once

/**
 * @file memory_experiments.hpp
 * @brief Memory Access Pattern Laboratory - Core educational memory experiments
 * 
 * This component implements the heart of ECScope's memory behavior laboratory,
 * providing interactive experiments that demonstrate the real-world performance
 * impact of memory layout decisions, cache behavior, and data access patterns.
 * 
 * Educational Objectives:
 * - Demonstrate SoA (Structure of Arrays) vs AoS (Array of Structures) performance
 * - Visualize cache line utilization and prefetching behavior
 * - Show the impact of data locality on performance
 * - Illustrate memory fragmentation and alignment effects
 * - Provide hands-on experience with archetype migration costs
 * - Teach memory bandwidth optimization techniques
 * 
 * Key Experiments:
 * - SoA vs AoS Comparison: Side-by-side performance analysis
 * - Cache Line Experiment: Demonstrate cache-friendly vs cache-hostile patterns
 * - Memory Prefetching Lab: Show predictable vs random access patterns
 * - Alignment Impact Study: Performance effect of data alignment
 * - Archetype Migration Simulator: ECS-specific memory behavior
 * - Memory Bandwidth Tests: Streaming vs random access performance
 * - Fragmentation Analysis: Visual fragmentation impact demonstration
 * 
 * @author ECScope Educational ECS Framework
 * @date 2024
 */

#include "performance_lab.hpp"
#include "memory/memory_tracker.hpp"
#include "memory/arena.hpp"
#include "memory/pool.hpp"
#include "ecs/registry.hpp"
#include "ecs/archetype.hpp"
#include <vector>
#include <array>
#include <memory>
#include <random>
#include <chrono>
#include <functional>

namespace ecscope::performance {

/**
 * @brief Memory access pattern types for experiments
 */
enum class MemoryAccessPattern : u8 {
    Sequential,         // Linear sequential access
    Reverse,           // Reverse sequential access
    Random,            // Random access pattern
    Strided,           // Strided access (every N elements)
    Circular,          // Circular buffer pattern
    TreeTraversal,     // Tree-like access pattern
    HashLookup,        // Hash table access pattern
    Streaming          // Write-only streaming pattern
};

/**
 * @brief Data structure layout types
 */
enum class DataStructureLayout : u8 {
    AoS,               // Array of Structures
    SoA,               // Structure of Arrays
    Hybrid,            // Mixed layout
    Packed,            // Tightly packed structures
    Aligned,           // Cache-aligned structures
    Interleaved        // Interleaved data layout
};

/**
 * @brief Test data generation configuration
 */
struct TestDataConfig {
    usize element_count;            // Number of elements to test
    usize element_size;             // Size of each element in bytes
    usize cache_line_size;          // Target cache line size
    DataStructureLayout layout;     // Data structure layout
    MemoryAccessPattern pattern;    // Access pattern to use
    bool use_random_data;          // Fill with random data
    u32 random_seed;               // Seed for reproducibility
    usize alignment_bytes;         // Data alignment requirement
    
    TestDataConfig() noexcept
        : element_count(10000), element_size(64), cache_line_size(64),
          layout(DataStructureLayout::AoS), pattern(MemoryAccessPattern::Sequential),
          use_random_data(true), random_seed(42), alignment_bytes(64) {}
};

/**
 * @brief Memory experiment result with detailed metrics
 */
struct MemoryExperimentResult {
    std::string experiment_name;
    TestDataConfig config;
    
    // Performance metrics
    f64 total_time_ms;             // Total execution time
    f64 time_per_element_ns;       // Average time per element
    f64 memory_bandwidth_gbps;     // Achieved memory bandwidth
    f64 cache_efficiency;          // Cache hit ratio (0.0-1.0)
    
    // Memory usage
    usize memory_allocated_bytes;   // Total memory allocated
    usize memory_wasted_bytes;     // Memory wasted due to alignment/fragmentation
    f64 memory_efficiency;         // Memory utilization ratio
    
    // Cache analysis
    u64 estimated_l1_misses;       // Estimated L1 cache misses
    u64 estimated_l2_misses;       // Estimated L2 cache misses
    u64 estimated_l3_misses;       // Estimated L3 cache misses
    f64 cache_line_utilization;    // Average cache line utilization
    
    // Educational insights
    std::vector<std::string> key_observations;
    std::vector<std::string> performance_factors;
    std::string optimization_recommendation;
    
    MemoryExperimentResult() noexcept
        : total_time_ms(0.0), time_per_element_ns(0.0), memory_bandwidth_gbps(0.0),
          cache_efficiency(0.0), memory_allocated_bytes(0), memory_wasted_bytes(0),
          memory_efficiency(0.0), estimated_l1_misses(0), estimated_l2_misses(0),
          estimated_l3_misses(0), cache_line_utilization(0.0) {}
};

/**
 * @brief Component data for ECS experiments
 */
struct TestComponent {
    f32 x, y, z;                   // Position (12 bytes)
    f32 vx, vy, vz;                // Velocity (12 bytes)
    f32 mass;                      // Mass (4 bytes)
    u32 id;                        // ID (4 bytes)
    u8 padding[32];                // Padding to make it larger (total: 64 bytes)
    
    TestComponent() noexcept 
        : x(0.0f), y(0.0f), z(0.0f), vx(0.0f), vy(0.0f), vz(0.0f), 
          mass(1.0f), id(0) {
        std::fill(std::begin(padding), std::end(padding), 0);
    }
};

/**
 * @brief SoA version of test component for comparison
 */
struct TestComponentSoA {
    std::vector<f32> x, y, z;      // Position arrays
    std::vector<f32> vx, vy, vz;   // Velocity arrays  
    std::vector<f32> mass;         // Mass array
    std::vector<u32> id;           // ID array
    
    void resize(usize count) {
        x.resize(count); y.resize(count); z.resize(count);
        vx.resize(count); vy.resize(count); vz.resize(count);
        mass.resize(count); id.resize(count);
    }
    
    usize size() const { return x.size(); }
};

/**
 * @brief Memory access pattern experiment
 */
class MemoryAccessExperiment : public IPerformanceExperiment {
private:
    TestDataConfig config_;
    std::mt19937 rng_;
    
    // Test data storage
    std::vector<TestComponent> aos_data_;
    TestComponentSoA soa_data_;
    std::vector<u8> raw_buffer_;
    
    // Access pattern generators
    std::vector<usize> generate_sequential_pattern(usize count);
    std::vector<usize> generate_random_pattern(usize count);
    std::vector<usize> generate_strided_pattern(usize count, usize stride);
    std::vector<usize> generate_circular_pattern(usize count);
    std::vector<usize> generate_tree_pattern(usize count);
    
    // Performance measurement
    MemoryExperimentResult measure_aos_performance(const std::vector<usize>& access_pattern);
    MemoryExperimentResult measure_soa_performance(const std::vector<usize>& access_pattern);
    MemoryExperimentResult measure_raw_performance(const std::vector<usize>& access_pattern);
    
    // Cache analysis
    void analyze_cache_behavior(MemoryExperimentResult& result, const std::vector<usize>& pattern);
    f64 estimate_cache_misses(const std::vector<usize>& pattern, usize cache_size, usize line_size);
    
public:
    explicit MemoryAccessExperiment(const TestDataConfig& config = TestDataConfig{});
    
    std::string get_name() const override { return "Memory Access Pattern Analysis"; }
    std::string get_description() const override;
    std::string get_category() const override { return "Memory"; }
    
    bool setup(const ExperimentConfig& config) override;
    BenchmarkResult execute() override;
    void cleanup() override;
    
    bool supports_real_time_visualization() const override { return true; }
    void update_visualization(f64 dt) override;
    
    std::vector<PerformanceRecommendation> generate_recommendations() const override;
    
    // Configuration
    void set_test_data_config(const TestDataConfig& config);
    TestDataConfig get_test_data_config() const { return config_; }
    
    // Direct experiment execution
    MemoryExperimentResult run_aos_vs_soa_comparison();
    MemoryExperimentResult run_cache_line_experiment();
    MemoryExperimentResult run_prefetching_experiment();
    MemoryExperimentResult run_alignment_experiment();
};

/**
 * @brief Archetype migration experiment for ECS systems
 */
class ArchetypeMigrationExperiment : public IPerformanceExperiment {
private:
    std::weak_ptr<ecs::Registry> registry_;
    usize entity_count_;
    usize component_types_;
    
    // Migration scenarios
    struct MigrationScenario {
        std::string name;
        std::string description;
        std::function<void(ecs::Registry&, const std::vector<ecs::Entity>&)> migration_func;
        f64 expected_cost_multiplier;
    };
    
    std::vector<MigrationScenario> scenarios_;
    
    // Performance measurement
    struct MigrationResult {
        std::string scenario_name;
        f64 migration_time_ms;
        usize entities_migrated;
        usize memory_copied_bytes;
        usize archetypes_created;
        usize archetypes_destroyed;
        f64 fragmentation_impact;
        std::vector<std::string> insights;
    };
    
    MigrationResult measure_migration_performance(const MigrationScenario& scenario,
                                                  const std::vector<ecs::Entity>& entities);
    
public:
    ArchetypeMigrationExperiment(std::weak_ptr<ecs::Registry> registry, 
                                usize entity_count = 10000,
                                usize component_types = 5);
    
    std::string get_name() const override { return "Archetype Migration Analysis"; }
    std::string get_description() const override;
    std::string get_category() const override { return "ECS"; }
    
    bool setup(const ExperimentConfig& config) override;
    BenchmarkResult execute() override;
    void cleanup() override;
    
    std::vector<PerformanceRecommendation> generate_recommendations() const override;
    
    // Configuration
    void set_entity_count(usize count) { entity_count_ = count; }
    void set_component_types(usize types) { component_types_ = types; }
};

/**
 * @brief Cache-friendly vs cache-hostile data layout experiment
 */
class CacheOptimizationExperiment : public IPerformanceExperiment {
private:
    struct CacheTestData {
        // Cache-friendly layout (hot data first)
        struct CacheFriendly {
            f32 frequently_used[4];    // 16 bytes - fits in half cache line
            u32 id;                    // 4 bytes
            u8 flags;                  // 1 byte
            u8 padding1[11];           // Pad to cache line
            f32 rarely_used[16];       // 64 bytes - separate cache line
        };
        
        // Cache-hostile layout (interleaved hot/cold data)
        struct CacheHostile {
            f32 frequently_used_1;     // Hot
            f32 rarely_used[4];        // Cold
            f32 frequently_used_2;     // Hot
            f32 more_rarely_used[4];   // Cold
            f32 frequently_used_3;     // Hot
            f32 even_more_rarely_used[4]; // Cold
            f32 frequently_used_4;     // Hot
            u32 id;
            u8 flags;
            u8 padding[11];
        };
        
        std::vector<CacheFriendly> friendly_data;
        std::vector<CacheHostile> hostile_data;
    };
    
    CacheTestData test_data_;
    usize data_size_;
    
    // Performance tests
    f64 measure_cache_friendly_performance();
    f64 measure_cache_hostile_performance();
    f64 measure_mixed_access_performance();
    
public:
    explicit CacheOptimizationExperiment(usize data_size = 100000);
    
    std::string get_name() const override { return "Cache Optimization Analysis"; }
    std::string get_description() const override;
    std::string get_category() const override { return "Memory"; }
    
    bool setup(const ExperimentConfig& config) override;
    BenchmarkResult execute() override;
    void cleanup() override;
    
    std::vector<PerformanceRecommendation> generate_recommendations() const override;
};

/**
 * @brief Memory bandwidth utilization experiment
 */
class MemoryBandwidthExperiment : public IPerformanceExperiment {
private:
    struct BandwidthTestConfig {
        usize buffer_size_mb;          // Buffer size in megabytes
        usize access_stride;           // Access stride in bytes
        bool prefetch_enabled;         // Enable software prefetching
        bool write_test;               // Test write performance
        u32 iterations;                // Number of test iterations
    };
    
    std::vector<BandwidthTestConfig> test_configs_;
    std::vector<u8> test_buffer_;
    
    // Performance measurements
    f64 measure_sequential_read_bandwidth(const BandwidthTestConfig& config);
    f64 measure_sequential_write_bandwidth(const BandwidthTestConfig& config);
    f64 measure_random_access_bandwidth(const BandwidthTestConfig& config);
    f64 measure_strided_access_bandwidth(const BandwidthTestConfig& config);
    
public:
    MemoryBandwidthExperiment();
    
    std::string get_name() const override { return "Memory Bandwidth Analysis"; }
    std::string get_description() const override;
    std::string get_category() const override { return "Memory"; }
    
    bool setup(const ExperimentConfig& config) override;
    BenchmarkResult execute() override;
    void cleanup() override;
    
    std::vector<PerformanceRecommendation> generate_recommendations() const override;
};

/**
 * @brief Main memory experiments coordinator
 */
class MemoryExperiments {
private:
    // Available experiments
    std::unique_ptr<MemoryAccessExperiment> access_experiment_;
    std::unique_ptr<ArchetypeMigrationExperiment> migration_experiment_;
    std::unique_ptr<CacheOptimizationExperiment> cache_experiment_;
    std::unique_ptr<MemoryBandwidthExperiment> bandwidth_experiment_;
    
    // Integration with memory tracker
    memory::MemoryTracker& memory_tracker_;
    
    // Experiment results cache
    std::unordered_map<std::string, MemoryExperimentResult> results_cache_;
    mutable std::mutex cache_mutex_;
    
    // Educational content
    std::unordered_map<std::string, std::string> explanations_;
    
    void initialize_educational_content();
    
public:
    explicit MemoryExperiments(std::weak_ptr<ecs::Registry> registry = {});
    ~MemoryExperiments() = default;
    
    // Non-copyable, non-movable
    MemoryExperiments(const MemoryExperiments&) = delete;
    MemoryExperiments& operator=(const MemoryExperiments&) = delete;
    MemoryExperiments(MemoryExperiments&&) = delete;
    MemoryExperiments& operator=(MemoryExperiments&&) = delete;
    
    // Experiment access
    MemoryAccessExperiment& get_access_experiment() { return *access_experiment_; }
    ArchetypeMigrationExperiment& get_migration_experiment() { return *migration_experiment_; }
    CacheOptimizationExperiment& get_cache_experiment() { return *cache_experiment_; }
    MemoryBandwidthExperiment& get_bandwidth_experiment() { return *bandwidth_experiment_; }
    
    // Quick experiment runners
    MemoryExperimentResult run_soa_vs_aos_comparison(const TestDataConfig& config = TestDataConfig{});
    MemoryExperimentResult run_cache_behavior_analysis(usize data_size = 100000);
    MemoryExperimentResult run_archetype_migration_analysis(usize entity_count = 10000);
    MemoryExperimentResult run_memory_bandwidth_analysis();
    
    // Comprehensive analysis
    std::vector<MemoryExperimentResult> run_full_memory_analysis();
    BenchmarkResult run_comparative_analysis();
    
    // Results management
    std::vector<MemoryExperimentResult> get_all_results() const;
    std::optional<MemoryExperimentResult> get_result(const std::string& experiment_name) const;
    void clear_results_cache();
    
    // Educational features
    std::string get_explanation(const std::string& topic) const;
    std::vector<std::string> get_available_explanations() const;
    std::vector<PerformanceRecommendation> get_memory_optimization_recommendations() const;
    
    // Analysis utilities
    f64 calculate_memory_efficiency_score() const;
    std::vector<std::string> identify_memory_bottlenecks() const;
    std::string generate_memory_optimization_report() const;
};

} // namespace ecscope::performance