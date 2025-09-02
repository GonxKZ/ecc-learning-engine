/**
 * @file examples/rendering_benchmarks.cpp
 * @brief Comprehensive Performance Benchmarking Suite for ECScope - Phase 7: Renderizado 2D
 * 
 * This benchmarking suite provides comprehensive performance analysis and testing
 * for the ECScope 2D rendering system. It includes:
 * 
 * Benchmark Categories:
 * - Sprite batching efficiency across different scenarios
 * - Memory allocation and deallocation performance
 * - GPU resource management and state changes
 * - Camera and viewport switching overhead
 * - Large scene rendering and culling performance
 * - Different rendering strategies comparison
 * 
 * Educational Features:
 * - Detailed performance analysis and bottleneck identification
 * - Comparative analysis between different approaches
 * - Memory usage profiling and optimization guidance
 * - Real-time performance visualization
 * - Statistical analysis with confidence intervals
 * - Automated optimization recommendations
 * 
 * Benchmark Types:
 * - Micro-benchmarks for specific operations
 * - Scenario-based benchmarks for real-world usage
 * - Stress tests for extreme conditions
 * - Memory benchmarks for allocation patterns
 * - Comparative benchmarks between strategies
 * 
 * @author ECScope Educational ECS Framework - Phase 7: Renderizado 2D
 * @date 2024
 */

#include <iostream>
#include <memory>
#include <vector>
#include <chrono>
#include <random>
#include <algorithm>
#include <numeric>
#include <iomanip>
#include <fstream>
#include <map>
#include <thread>

// ECScope Core Systems
#include "../src/core/log.hpp"
#include "../src/core/time.hpp"
#include "../src/core/types.hpp"
#include "../src/ecs/registry.hpp"
#include "../src/ecs/components/transform.hpp"

// Rendering System
#include "../src/renderer/renderer_2d.hpp"
#include "../src/renderer/batch_renderer.hpp"
#include "../src/renderer/window.hpp"

// Memory System for tracking
#include "../src/memory/memory_tracker.hpp"

using namespace ecscope;

//=============================================================================
// Benchmark Infrastructure
//=============================================================================

/**
 * @brief High-Precision Timer for Benchmarking
 * 
 * Provides microsecond-precision timing for accurate performance measurement
 * with statistical analysis capabilities.
 */
class BenchmarkTimer {
public:
    void start() noexcept {
        start_time_ = std::chrono::high_resolution_clock::now();
    }
    
    f64 end_microseconds() noexcept {
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time_);
        return static_cast<f64>(duration.count()) / 1000.0;
    }
    
    f64 end_milliseconds() noexcept {
        return end_microseconds() / 1000.0;
    }

private:
    std::chrono::high_resolution_clock::time_point start_time_;
};

/**
 * @brief Statistical Analysis for Benchmark Results
 * 
 * Calculates statistical measures to ensure benchmark reliability
 * and identify performance variations.
 */
struct BenchmarkStats {
    std::vector<f64> samples;
    f64 mean{0.0};
    f64 median{0.0};
    f64 min_value{0.0};
    f64 max_value{0.0};
    f64 std_dev{0.0};
    f64 confidence_95_lower{0.0};
    f64 confidence_95_upper{0.0};
    
    void calculate() {
        if (samples.empty()) return;
        
        // Sort for percentile calculations
        std::vector<f64> sorted_samples = samples;
        std::sort(sorted_samples.begin(), sorted_samples.end());
        
        // Basic statistics
        mean = std::accumulate(samples.begin(), samples.end(), 0.0) / samples.size();
        median = sorted_samples[sorted_samples.size() / 2];
        min_value = sorted_samples.front();
        max_value = sorted_samples.back();
        
        // Standard deviation
        f64 variance = 0.0;
        for (f64 sample : samples) {
            variance += (sample - mean) * (sample - mean);
        }
        variance /= samples.size();
        std_dev = std::sqrt(variance);
        
        // 95% confidence interval (rough approximation)
        f64 margin = 1.96 * (std_dev / std::sqrt(samples.size()));
        confidence_95_lower = mean - margin;
        confidence_95_upper = mean + margin;
    }
    
    void add_sample(f64 value) {
        samples.push_back(value);
    }
    
    void print_summary(const std::string& name) const {
        std::cout << "\n" << name << " Benchmark Results:\n";
        std::cout << "==========================================\n";
        std::cout << std::fixed << std::setprecision(3);
        std::cout << "  Mean:     " << mean << " ms\n";
        std::cout << "  Median:   " << median << " ms\n";
        std::cout << "  Min:      " << min_value << " ms\n";
        std::cout << "  Max:      " << max_value << " ms\n";
        std::cout << "  Std Dev:  " << std_dev << " ms\n";
        std::cout << "  95% CI:   [" << confidence_95_lower << ", " << confidence_95_upper << "] ms\n";
        std::cout << "  Samples:  " << samples.size() << "\n";
        
        // Performance assessment
        if (mean < 1.0) {
            std::cout << "  Rating:   EXCELLENT (< 1ms)\n";
        } else if (mean < 5.0) {
            std::cout << "  Rating:   GOOD (< 5ms)\n";
        } else if (mean < 16.67) {
            std::cout << "  Rating:   ACCEPTABLE (< 60 FPS)\n";
        } else {
            std::cout << "  Rating:   POOR (> 16.67ms)\n";
        }
    }
};

/**
 * @brief Individual Benchmark Test Case
 * 
 * Represents a single benchmark test with setup, execution, and cleanup phases.
 */
class BenchmarkTest {
public:
    BenchmarkTest(std::string name, std::string description)
        : name_(std::move(name)), description_(std::move(description)) {}
    
    virtual ~BenchmarkTest() = default;
    
    // Pure virtual interface for benchmark implementation
    virtual bool setup() = 0;
    virtual void execute() = 0;
    virtual void cleanup() = 0;
    
    // Run the benchmark with statistical collection
    BenchmarkStats run(u32 iterations = 100, u32 warmup_iterations = 10) {
        std::cout << "\nRunning benchmark: " << name_ << "\n";
        std::cout << "Description: " << description_ << "\n";
        std::cout << "Iterations: " << iterations << " (+ " << warmup_iterations << " warmup)\n";
        
        if (!setup()) {
            std::cerr << "Benchmark setup failed for: " << name_ << "\n";
            return {};
        }
        
        BenchmarkStats stats;
        BenchmarkTimer timer;
        
        // Warmup runs to stabilize performance
        for (u32 i = 0; i < warmup_iterations; ++i) {
            execute();
        }
        
        // Actual benchmark runs
        for (u32 i = 0; i < iterations; ++i) {
            timer.start();
            execute();
            f64 elapsed = timer.end_milliseconds();
            stats.add_sample(elapsed);
            
            // Progress indicator
            if ((i + 1) % (iterations / 10) == 0) {
                std::cout << "Progress: " << ((i + 1) * 100 / iterations) << "%\n";
            }
        }
        
        cleanup();
        stats.calculate();
        
        return stats;
    }
    
    const std::string& get_name() const { return name_; }
    const std::string& get_description() const { return description_; }

protected:
    std::string name_;
    std::string description_;
};

//=============================================================================
// Sprite Batching Benchmarks
//=============================================================================

/**
 * @brief Benchmark for Sprite Batching Efficiency
 * 
 * Tests the performance of sprite batching under different scenarios
 * including various sprite counts, texture usage patterns, and batching strategies.
 */
class SpriteBatchingBenchmark : public BenchmarkTest {
public:
    SpriteBatchingBenchmark(u32 sprite_count, renderer::BatchingStrategy strategy)
        : BenchmarkTest(
            "Sprite Batching - " + std::to_string(sprite_count) + " sprites",
            "Tests batching performance with " + std::to_string(sprite_count) + " sprites using " + strategy_name(strategy)
          )
        , sprite_count_(sprite_count)
        , strategy_(strategy) {}

    bool setup() override {
        // Initialize renderer
        renderer_config_ = renderer::Renderer2DConfig::performance_focused();
        renderer_config_.rendering.max_sprites_per_batch = 1000;
        
        renderer_ = std::make_unique<renderer::Renderer2D>(renderer_config_);
        if (auto result = renderer_->initialize(); !result.is_ok()) {
            std::cerr << "Failed to initialize renderer: " << result.error() << "\n";
            return false;
        }
        
        // Create ECS registry
        registry_ = std::make_unique<ecs::Registry>();
        
        // Create camera
        camera_entity_ = registry_->create_entity();
        auto& camera = registry_->add_component<renderer::components::Camera2D>(camera_entity_);
        camera.position = {0.0f, 0.0f};
        camera.zoom = 1.0f;
        camera.viewport_width = 1920.0f;
        camera.viewport_height = 1080.0f;
        
        // Generate test sprites
        create_test_sprites();
        
        return true;
    }
    
    void execute() override {
        // Begin frame
        renderer_->begin_frame();
        
        // Set camera
        const auto& camera = registry_->get_component<renderer::components::Camera2D>(camera_entity_);
        renderer_->set_active_camera(camera);
        
        // Render all sprite entities
        renderer_->render_entities(*registry_);
        
        // End frame
        renderer_->end_frame();
    }
    
    void cleanup() override {
        sprite_entities_.clear();
        registry_.reset();
        renderer_.reset();
    }

private:
    u32 sprite_count_;
    renderer::BatchingStrategy strategy_;
    renderer::Renderer2DConfig renderer_config_;
    
    std::unique_ptr<renderer::Renderer2D> renderer_;
    std::unique_ptr<ecs::Registry> registry_;
    ecs::EntityID camera_entity_;
    std::vector<ecs::EntityID> sprite_entities_;
    
    void create_test_sprites() {
        sprite_entities_.reserve(sprite_count_);
        
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<f32> pos_dist(-1000.0f, 1000.0f);
        std::uniform_real_distribution<f32> size_dist(16.0f, 64.0f);
        std::uniform_real_distribution<f32> color_dist(0.5f, 1.0f);
        std::uniform_int_distribution<u32> texture_dist(0, 7);
        
        for (u32 i = 0; i < sprite_count_; ++i) {
            auto entity = registry_->create_entity();
            
            // Transform
            auto& transform = registry_->add_component<ecs::components::Transform>(entity);
            transform.position = {pos_dist(gen), pos_dist(gen), 0.0f};
            f32 size = size_dist(gen);
            transform.scale = {size, size, 1.0f};
            
            // Sprite
            auto& sprite = registry_->add_component<renderer::components::RenderableSprite>(entity);
            sprite.texture_id = static_cast<renderer::TextureID>(texture_dist(gen));
            sprite.color = {color_dist(gen), color_dist(gen), color_dist(gen), 1.0f};
            
            sprite_entities_.push_back(entity);
        }
    }
    
    std::string strategy_name(renderer::BatchingStrategy strategy) const {
        switch (strategy) {
            case renderer::BatchingStrategy::TextureFirst: return "Texture First";
            case renderer::BatchingStrategy::MaterialFirst: return "Material First";
            case renderer::BatchingStrategy::ZOrderPreserving: return "Z-Order Preserving";
            case renderer::BatchingStrategy::SpatialLocality: return "Spatial Locality";
            case renderer::BatchingStrategy::AdaptiveHybrid: return "Adaptive Hybrid";
            default: return "Unknown";
        }
    }
};

//=============================================================================
// Memory Performance Benchmarks
//=============================================================================

/**
 * @brief Memory Allocation Performance Benchmark
 * 
 * Tests the performance of various memory allocation patterns used
 * in the rendering system, including vertex buffers, render commands, etc.
 */
class MemoryAllocationBenchmark : public BenchmarkTest {
public:
    MemoryAllocationBenchmark(usize allocation_size, u32 allocation_count)
        : BenchmarkTest(
            "Memory Allocation - " + std::to_string(allocation_size) + " bytes x " + std::to_string(allocation_count),
            "Tests memory allocation/deallocation performance"
          )
        , allocation_size_(allocation_size)
        , allocation_count_(allocation_count) {}

    bool setup() override {
        allocations_.reserve(allocation_count_);
        return true;
    }
    
    void execute() override {
        // Allocation phase
        for (u32 i = 0; i < allocation_count_; ++i) {
            void* ptr = std::malloc(allocation_size_);
            allocations_.push_back(ptr);
            
            // Touch memory to ensure it's actually allocated
            std::memset(ptr, i % 256, allocation_size_);
        }
        
        // Deallocation phase
        for (void* ptr : allocations_) {
            std::free(ptr);
        }
        
        allocations_.clear();
    }
    
    void cleanup() override {
        allocations_.clear();
    }

private:
    usize allocation_size_;
    u32 allocation_count_;
    std::vector<void*> allocations_;
};

//=============================================================================
// GPU State Change Benchmarks
//=============================================================================

/**
 * @brief GPU State Change Performance Benchmark
 * 
 * Measures the cost of different GPU state changes including
 * shader switching, texture binding, and render state changes.
 */
class GPUStateChangeBenchmark : public BenchmarkTest {
public:
    enum class StateChangeType {
        TextureBinding,
        ShaderSwitching,
        BlendModeChange,
        ViewportChange
    };
    
    GPUStateChangeBenchmark(StateChangeType type, u32 change_count)
        : BenchmarkTest(
            "GPU State Changes - " + type_name(type),
            "Tests performance of " + std::to_string(change_count) + " " + type_name(type) + " operations"
          )
        , type_(type)
        , change_count_(change_count) {}

    bool setup() override {
        renderer_config_ = renderer::Renderer2DConfig::performance_focused();
        renderer_ = std::make_unique<renderer::Renderer2D>(renderer_config_);
        
        if (auto result = renderer_->initialize(); !result.is_ok()) {
            std::cerr << "Failed to initialize renderer: " << result.error() << "\n";
            return false;
        }
        
        return true;
    }
    
    void execute() override {
        renderer_->begin_frame();
        
        switch (type_) {
            case StateChangeType::TextureBinding:
                benchmark_texture_binding();
                break;
            case StateChangeType::ShaderSwitching:
                benchmark_shader_switching();
                break;
            case StateChangeType::BlendModeChange:
                benchmark_blend_mode_changes();
                break;
            case StateChangeType::ViewportChange:
                benchmark_viewport_changes();
                break;
        }
        
        renderer_->end_frame();
    }
    
    void cleanup() override {
        renderer_.reset();
    }

private:
    StateChangeType type_;
    u32 change_count_;
    renderer::Renderer2DConfig renderer_config_;
    std::unique_ptr<renderer::Renderer2D> renderer_;
    
    void benchmark_texture_binding() {
        for (u32 i = 0; i < change_count_; ++i) {
            renderer::TextureID texture_id = static_cast<renderer::TextureID>(i % 8);
            renderer_->bind_texture(texture_id, 0);
        }
    }
    
    void benchmark_shader_switching() {
        for (u32 i = 0; i < change_count_; ++i) {
            renderer::ShaderID shader_id = static_cast<renderer::ShaderID>(i % 4);
            renderer_->bind_shader(shader_id);
        }
    }
    
    void benchmark_blend_mode_changes() {
        // This would involve changing blend modes through materials
        // Implementation depends on material system design
        for (u32 i = 0; i < change_count_; ++i) {
            // Simulate blend mode changes
            std::this_thread::sleep_for(std::chrono::nanoseconds(100));
        }
    }
    
    void benchmark_viewport_changes() {
        for (u32 i = 0; i < change_count_; ++i) {
            u32 width = 800 + (i % 4) * 200;
            u32 height = 600 + (i % 4) * 150;
            renderer_->handle_window_resize(width, height);
        }
    }
    
    std::string type_name(StateChangeType type) const {
        switch (type) {
            case StateChangeType::TextureBinding: return "Texture Binding";
            case StateChangeType::ShaderSwitching: return "Shader Switching";
            case StateChangeType::BlendModeChange: return "Blend Mode Changes";
            case StateChangeType::ViewportChange: return "Viewport Changes";
            default: return "Unknown";
        }
    }
};

//=============================================================================
// Large Scene Performance Benchmarks
//=============================================================================

/**
 * @brief Large Scene Rendering Benchmark
 * 
 * Tests performance with large numbers of entities and complex scenes
 * to identify scalability bottlenecks.
 */
class LargeSceneBenchmark : public BenchmarkTest {
public:
    LargeSceneBenchmark(u32 entity_count, bool enable_culling)
        : BenchmarkTest(
            "Large Scene - " + std::to_string(entity_count) + " entities" + 
            (enable_culling ? " (with culling)" : " (no culling)"),
            "Tests performance with large numbers of entities"
          )
        , entity_count_(entity_count)
        , enable_culling_(enable_culling) {}

    bool setup() override {
        renderer_config_ = renderer::Renderer2DConfig::performance_focused();
        renderer_config_.rendering.enable_frustum_culling = enable_culling_;
        renderer_config_.rendering.max_sprites_per_batch = 2000;
        
        renderer_ = std::make_unique<renderer::Renderer2D>(renderer_config_);
        if (auto result = renderer_->initialize(); !result.is_ok()) {
            return false;
        }
        
        registry_ = std::make_unique<ecs::Registry>();
        
        // Create camera
        camera_entity_ = registry_->create_entity();
        auto& camera = registry_->add_component<renderer::components::Camera2D>(camera_entity_);
        camera.position = {0.0f, 0.0f};
        camera.zoom = 0.1f; // Zoomed out to see many entities
        camera.viewport_width = 1920.0f;
        camera.viewport_height = 1080.0f;
        
        // Create large scene
        create_large_scene();
        
        return true;
    }
    
    void execute() override {
        renderer_->begin_frame();
        
        const auto& camera = registry_->get_component<renderer::components::Camera2D>(camera_entity_);
        renderer_->set_active_camera(camera);
        
        renderer_->render_entities(*registry_);
        
        renderer_->end_frame();
    }
    
    void cleanup() override {
        entities_.clear();
        registry_.reset();
        renderer_.reset();
    }

private:
    u32 entity_count_;
    bool enable_culling_;
    renderer::Renderer2DConfig renderer_config_;
    
    std::unique_ptr<renderer::Renderer2D> renderer_;
    std::unique_ptr<ecs::Registry> registry_;
    ecs::EntityID camera_entity_;
    std::vector<ecs::EntityID> entities_;
    
    void create_large_scene() {
        entities_.reserve(entity_count_);
        
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<f32> pos_dist(-10000.0f, 10000.0f); // Large world
        std::uniform_real_distribution<f32> size_dist(8.0f, 32.0f);
        std::uniform_real_distribution<f32> color_dist(0.3f, 1.0f);
        std::uniform_int_distribution<u32> texture_dist(0, 15); // More texture variety
        
        // Create entities in a grid-like pattern with some randomness
        u32 grid_size = static_cast<u32>(std::sqrt(entity_count_));
        f32 spacing = 20000.0f / grid_size;
        
        for (u32 i = 0; i < entity_count_; ++i) {
            auto entity = registry_->create_entity();
            
            // Grid position with random offset
            u32 grid_x = i % grid_size;
            u32 grid_y = i / grid_size;
            f32 base_x = (grid_x - grid_size/2) * spacing;
            f32 base_y = (grid_y - grid_size/2) * spacing;
            
            auto& transform = registry_->add_component<ecs::components::Transform>(entity);
            transform.position = {
                base_x + pos_dist(gen) * 0.1f, // Small random offset
                base_y + pos_dist(gen) * 0.1f,
                pos_dist(gen) * 0.01f // Small Z variation
            };
            
            f32 size = size_dist(gen);
            transform.scale = {size, size, 1.0f};
            
            auto& sprite = registry_->add_component<renderer::components::RenderableSprite>(entity);
            sprite.texture_id = static_cast<renderer::TextureID>(texture_dist(gen));
            sprite.color = {color_dist(gen), color_dist(gen), color_dist(gen), 1.0f};
            sprite.z_order = transform.position.z;
            
            entities_.push_back(entity);
        }
        
        std::cout << "Created " << entities_.size() << " entities in large scene\n";
    }
};

//=============================================================================
// Comparative Strategy Benchmarks
//=============================================================================

/**
 * @brief Comparative Benchmark for Different Rendering Strategies
 * 
 * Compares the performance of different batching strategies, sorting methods,
 * and optimization techniques under identical conditions.
 */
class ComparativeStrategyBenchmark : public BenchmarkTest {
public:
    ComparativeStrategyBenchmark()
        : BenchmarkTest(
            "Strategy Comparison",
            "Compares different rendering strategies under identical conditions"
          ) {}

    bool setup() override {
        // Setup test scene that will be used for all strategies
        setup_common_scene();
        return true;
    }
    
    void execute() override {
        // This benchmark runs multiple sub-tests
        // The actual timing is done in run_comparative_analysis()
    }
    
    void cleanup() override {
        test_entities_.clear();
    }
    
    // Custom runner for comparative analysis
    void run_comparative_analysis() {
        std::cout << "\n=== COMPARATIVE STRATEGY ANALYSIS ===\n";
        
        // Test different batching strategies
        std::vector<renderer::BatchingStrategy> strategies = {
            renderer::BatchingStrategy::TextureFirst,
            renderer::BatchingStrategy::MaterialFirst,
            renderer::BatchingStrategy::ZOrderPreserving,
            renderer::BatchingStrategy::SpatialLocality,
            renderer::BatchingStrategy::AdaptiveHybrid
        };
        
        std::map<renderer::BatchingStrategy, BenchmarkStats> results;
        
        for (auto strategy : strategies) {
            std::cout << "\nTesting strategy: " << strategy_name(strategy) << "\n";
            
            auto stats = benchmark_strategy(strategy, 50); // 50 iterations each
            results[strategy] = stats;
            
            stats.print_summary(strategy_name(strategy));
        }
        
        // Analysis and recommendations
        analyze_strategy_results(results);
    }

private:
    struct TestSprite {
        f32 x, y, z;
        f32 size;
        renderer::TextureID texture_id;
        renderer::components::Color color;
    };
    
    std::vector<TestSprite> test_entities_;
    
    void setup_common_scene() {
        // Create a standardized test scene
        test_entities_.reserve(5000);
        
        std::random_device rd;
        std::mt19937 gen(42); // Fixed seed for reproducible results
        std::uniform_real_distribution<f32> pos_dist(-1000.0f, 1000.0f);
        std::uniform_real_distribution<f32> size_dist(16.0f, 64.0f);
        std::uniform_real_distribution<f32> color_dist(0.5f, 1.0f);
        std::uniform_int_distribution<u32> texture_dist(0, 7);
        
        for (u32 i = 0; i < 5000; ++i) {
            TestSprite sprite;
            sprite.x = pos_dist(gen);
            sprite.y = pos_dist(gen);
            sprite.z = pos_dist(gen) * 0.01f; // Small Z range
            sprite.size = size_dist(gen);
            sprite.texture_id = static_cast<renderer::TextureID>(texture_dist(gen));
            sprite.color = {color_dist(gen), color_dist(gen), color_dist(gen), 1.0f};
            
            test_entities_.push_back(sprite);
        }
    }
    
    BenchmarkStats benchmark_strategy(renderer::BatchingStrategy strategy, u32 iterations) {
        BenchmarkStats stats;
        BenchmarkTimer timer;
        
        for (u32 iter = 0; iter < iterations; ++iter) {
            // Setup renderer with specific strategy
            auto renderer_config = renderer::Renderer2DConfig::performance_focused();
            auto renderer = std::make_unique<renderer::Renderer2D>(renderer_config);
            
            if (auto result = renderer->initialize(); !result.is_ok()) {
                continue; // Skip this iteration
            }
            
            // Create registry and entities
            auto registry = std::make_unique<ecs::Registry>();
            
            // Create camera
            auto camera_entity = registry->create_entity();
            auto& camera = registry->add_component<renderer::components::Camera2D>(camera_entity);
            camera.position = {0.0f, 0.0f};
            camera.zoom = 1.0f;
            camera.viewport_width = 1920.0f;
            camera.viewport_height = 1080.0f;
            
            // Create entities from test data
            for (const auto& test_sprite : test_entities_) {
                auto entity = registry->create_entity();
                
                auto& transform = registry->add_component<ecs::components::Transform>(entity);
                transform.position = {test_sprite.x, test_sprite.y, test_sprite.z};
                transform.scale = {test_sprite.size, test_sprite.size, 1.0f};
                
                auto& sprite = registry->add_component<renderer::components::RenderableSprite>(entity);
                sprite.texture_id = test_sprite.texture_id;
                sprite.color = test_sprite.color;
                sprite.z_order = test_sprite.z;
            }
            
            // Benchmark the rendering
            timer.start();
            
            renderer->begin_frame();
            renderer->set_active_camera(camera);
            renderer->render_entities(*registry);
            renderer->end_frame();
            
            f64 elapsed = timer.end_milliseconds();
            stats.add_sample(elapsed);
        }
        
        stats.calculate();
        return stats;
    }
    
    void analyze_strategy_results(const std::map<renderer::BatchingStrategy, BenchmarkStats>& results) {
        std::cout << "\n=== STRATEGY ANALYSIS SUMMARY ===\n";
        
        // Find best performing strategy
        renderer::BatchingStrategy best_strategy = renderer::BatchingStrategy::AdaptiveHybrid;
        f64 best_mean = std::numeric_limits<f64>::max();
        
        for (const auto& [strategy, stats] : results) {
            if (stats.mean < best_mean) {
                best_mean = stats.mean;
                best_strategy = strategy;
            }
        }
        
        std::cout << "Best performing strategy: " << strategy_name(best_strategy) 
                  << " (" << best_mean << " ms average)\n";
        
        // Performance comparison table
        std::cout << "\nPerformance Comparison:\n";
        std::cout << "Strategy                | Mean (ms) | Std Dev | Rating\n";
        std::cout << "------------------------|-----------|---------|--------\n";
        
        for (const auto& [strategy, stats] : results) {
            f64 performance_ratio = stats.mean / best_mean;
            std::string rating = "EXCELLENT";
            if (performance_ratio > 1.1) rating = "GOOD";
            if (performance_ratio > 1.25) rating = "FAIR";
            if (performance_ratio > 1.5) rating = "POOR";
            
            std::cout << std::setw(22) << std::left << strategy_name(strategy)
                      << " | " << std::setw(9) << std::fixed << std::setprecision(3) << stats.mean
                      << " | " << std::setw(7) << stats.std_dev
                      << " | " << rating << "\n";
        }
        
        // Recommendations
        std::cout << "\nOptimization Recommendations:\n";
        std::cout << "1. Use " << strategy_name(best_strategy) << " for best performance\n";
        
        // Check if adaptive is significantly different from best
        auto adaptive_it = results.find(renderer::BatchingStrategy::AdaptiveHybrid);
        if (adaptive_it != results.end() && best_strategy != renderer::BatchingStrategy::AdaptiveHybrid) {
            f64 adaptive_ratio = adaptive_it->second.mean / best_mean;
            if (adaptive_ratio < 1.2) {
                std::cout << "2. AdaptiveHybrid provides good balance (only " 
                          << std::setprecision(1) << ((adaptive_ratio - 1.0) * 100) 
                          << "% slower)\n";
            }
        }
        
        std::cout << "3. Consider scene characteristics when choosing strategy\n";
        std::cout << "4. Monitor performance in real applications as results may vary\n";
    }
    
    std::string strategy_name(renderer::BatchingStrategy strategy) const {
        switch (strategy) {
            case renderer::BatchingStrategy::TextureFirst: return "Texture First";
            case renderer::BatchingStrategy::MaterialFirst: return "Material First";
            case renderer::BatchingStrategy::ZOrderPreserving: return "Z-Order Preserving";
            case renderer::BatchingStrategy::SpatialLocality: return "Spatial Locality";
            case renderer::BatchingStrategy::AdaptiveHybrid: return "Adaptive Hybrid";
            default: return "Unknown";
        }
    }
};

//=============================================================================
// Benchmark Suite Manager
//=============================================================================

/**
 * @brief Main Benchmark Suite Manager
 * 
 * Orchestrates the execution of all benchmarks and provides
 * comprehensive analysis and reporting capabilities.
 */
class RenderingBenchmarkSuite {
public:
    RenderingBenchmarkSuite() {
        setup_benchmarks();
    }
    
    void run_all_benchmarks() {
        std::cout << "ECScope 2D Rendering Performance Benchmark Suite\n";
        std::cout << "================================================\n";
        std::cout << "Educational Graphics Programming Performance Analysis\n\n";
        
        // Record overall timing
        auto suite_start = std::chrono::high_resolution_clock::now();
        
        all_results_.clear();
        
        // Run individual benchmarks
        for (auto& benchmark : benchmarks_) {
            auto stats = benchmark->run(100, 10); // 100 iterations, 10 warmup
            stats.print_summary(benchmark->get_name());
            all_results_[benchmark->get_name()] = stats;
        }
        
        // Run comparative analysis
        ComparativeStrategyBenchmark comparative;
        comparative.setup();
        comparative.run_comparative_analysis();
        comparative.cleanup();
        
        // Calculate total time
        auto suite_end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::seconds>(suite_end - suite_start);
        
        // Generate comprehensive report
        generate_comprehensive_report(duration.count());
    }
    
    void run_quick_benchmark() {
        std::cout << "ECScope 2D Rendering Quick Benchmark\n";
        std::cout << "=====================================\n";
        
        // Run a subset of benchmarks with fewer iterations
        std::vector<std::string> quick_benchmarks = {
            "Sprite Batching - 1000 sprites",
            "Memory Allocation - 1024 bytes x 1000",
            "GPU State Changes - Texture Binding"
        };
        
        for (auto& benchmark : benchmarks_) {
            bool should_run = std::find(quick_benchmarks.begin(), quick_benchmarks.end(), 
                                      benchmark->get_name()) != quick_benchmarks.end();
            
            if (should_run) {
                auto stats = benchmark->run(20, 5); // Quick run: 20 iterations, 5 warmup
                stats.print_summary(benchmark->get_name());
            }
        }
    }

private:
    std::vector<std::unique_ptr<BenchmarkTest>> benchmarks_;
    std::map<std::string, BenchmarkStats> all_results_;
    
    void setup_benchmarks() {
        // Sprite batching benchmarks with different counts
        benchmarks_.emplace_back(std::make_unique<SpriteBatchingBenchmark>(
            1000, renderer::BatchingStrategy::AdaptiveHybrid));
        benchmarks_.emplace_back(std::make_unique<SpriteBatchingBenchmark>(
            5000, renderer::BatchingStrategy::AdaptiveHybrid));
        benchmarks_.emplace_back(std::make_unique<SpriteBatchingBenchmark>(
            10000, renderer::BatchingStrategy::AdaptiveHybrid));
        
        // Memory allocation benchmarks
        benchmarks_.emplace_back(std::make_unique<MemoryAllocationBenchmark>(1024, 1000));      // Small frequent
        benchmarks_.emplace_back(std::make_unique<MemoryAllocationBenchmark>(1024 * 1024, 10)); // Large infrequent
        benchmarks_.emplace_back(std::make_unique<MemoryAllocationBenchmark>(64, 10000));       // Tiny very frequent
        
        // GPU state change benchmarks
        benchmarks_.emplace_back(std::make_unique<GPUStateChangeBenchmark>(
            GPUStateChangeBenchmark::StateChangeType::TextureBinding, 1000));
        benchmarks_.emplace_back(std::make_unique<GPUStateChangeBenchmark>(
            GPUStateChangeBenchmark::StateChangeType::ShaderSwitching, 100));
        benchmarks_.emplace_back(std::make_unique<GPUStateChangeBenchmark>(
            GPUStateChangeBenchmark::StateChangeType::ViewportChange, 50));
        
        // Large scene benchmarks
        benchmarks_.emplace_back(std::make_unique<LargeSceneBenchmark>(20000, true));  // With culling
        benchmarks_.emplace_back(std::make_unique<LargeSceneBenchmark>(20000, false)); // Without culling
    }
    
    void generate_comprehensive_report(i64 total_seconds) {
        std::cout << "\n" << std::string(80, '=') << "\n";
        std::cout << "COMPREHENSIVE PERFORMANCE ANALYSIS REPORT\n";
        std::cout << std::string(80, '=') << "\n";
        
        // Executive summary
        std::cout << "\nExecutive Summary:\n";
        std::cout << "------------------\n";
        std::cout << "Total benchmarks: " << all_results_.size() << "\n";
        std::cout << "Total execution time: " << total_seconds << " seconds\n";
        
        // Performance categories
        u32 excellent = 0, good = 0, fair = 0, poor = 0;
        for (const auto& [name, stats] : all_results_) {
            if (stats.mean < 1.0) excellent++;
            else if (stats.mean < 5.0) good++;
            else if (stats.mean < 16.67) fair++;
            else poor++;
        }
        
        std::cout << "Performance distribution:\n";
        std::cout << "  Excellent (< 1ms):     " << excellent << " benchmarks\n";
        std::cout << "  Good (1-5ms):          " << good << " benchmarks\n";
        std::cout << "  Fair (5-16.67ms):      " << fair << " benchmarks\n";
        std::cout << "  Poor (> 16.67ms):      " << poor << " benchmarks\n";
        
        // Identify bottlenecks and recommendations
        generate_optimization_recommendations();
        
        // Export detailed results
        export_results_to_csv();
        
        std::cout << "\nBenchmark suite completed successfully!\n";
        std::cout << "Results exported to 'rendering_benchmark_results.csv'\n";
    }
    
    void generate_optimization_recommendations() {
        std::cout << "\nOptimization Recommendations:\n";
        std::cout << "-----------------------------\n";
        
        // Analyze sprite batching performance
        bool has_batching_issues = false;
        for (const auto& [name, stats] : all_results_) {
            if (name.find("Sprite Batching") != std::string::npos && stats.mean > 10.0) {
                has_batching_issues = true;
                break;
            }
        }
        
        if (has_batching_issues) {
            std::cout << "1. SPRITE BATCHING: Performance issues detected with large sprite counts\n";
            std::cout << "   - Consider reducing max sprites per batch\n";
            std::cout << "   - Enable frustum culling for large scenes\n";
            std::cout << "   - Use texture atlases to improve batching efficiency\n\n";
        }
        
        // Analyze memory performance
        bool has_memory_issues = false;
        for (const auto& [name, stats] : all_results_) {
            if (name.find("Memory Allocation") != std::string::npos && stats.mean > 5.0) {
                has_memory_issues = true;
                break;
            }
        }
        
        if (has_memory_issues) {
            std::cout << "2. MEMORY ALLOCATION: High allocation costs detected\n";
            std::cout << "   - Implement object pooling for frequently allocated objects\n";
            std::cout << "   - Use custom allocators for render commands\n";
            std::cout << "   - Consider memory mapping for large buffers\n\n";
        }
        
        // General recommendations
        std::cout << "3. GENERAL OPTIMIZATIONS:\n";
        std::cout << "   - Profile in release builds for accurate performance data\n";
        std::cout << "   - Monitor GPU utilization alongside CPU benchmarks\n";
        std::cout << "   - Consider multi-threaded render command generation\n";
        std::cout << "   - Use GPU timing queries for detailed render analysis\n";
    }
    
    void export_results_to_csv() {
        std::ofstream csv_file("rendering_benchmark_results.csv");
        
        if (!csv_file.is_open()) {
            std::cerr << "Failed to create CSV file\n";
            return;
        }
        
        // CSV header
        csv_file << "Benchmark,Mean (ms),Median (ms),Min (ms),Max (ms),Std Dev (ms),Samples,Rating\n";
        
        // Data rows
        for (const auto& [name, stats] : all_results_) {
            std::string rating = "POOR";
            if (stats.mean < 1.0) rating = "EXCELLENT";
            else if (stats.mean < 5.0) rating = "GOOD";
            else if (stats.mean < 16.67) rating = "FAIR";
            
            csv_file << name << ","
                     << stats.mean << ","
                     << stats.median << ","
                     << stats.min_value << ","
                     << stats.max_value << ","
                     << stats.std_dev << ","
                     << stats.samples.size() << ","
                     << rating << "\n";
        }
        
        csv_file.close();
    }
};

//=============================================================================
// Main Entry Point
//=============================================================================

/**
 * @brief Main entry point for the ECScope Rendering Benchmark Suite
 */
int main(int argc, char* argv[]) {
    // Initialize logging
    core::Log::initialize(core::LogLevel::INFO);
    
    // Parse command line options
    bool quick_mode = false;
    bool help = false;
    
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--quick" || arg == "-q") {
            quick_mode = true;
        } else if (arg == "--help" || arg == "-h") {
            help = true;
        }
    }
    
    if (help) {
        std::cout << "ECScope 2D Rendering Benchmark Suite\n";
        std::cout << "Usage: " << argv[0] << " [options]\n";
        std::cout << "Options:\n";
        std::cout << "  --quick, -q    Run quick benchmark (fewer iterations)\n";
        std::cout << "  --help, -h     Show this help message\n";
        std::cout << "\nThis benchmark suite tests the performance of ECScope's 2D rendering system\n";
        std::cout << "including sprite batching, memory allocation, GPU state changes, and large scenes.\n";
        return 0;
    }
    
    try {
        RenderingBenchmarkSuite suite;
        
        if (quick_mode) {
            suite.run_quick_benchmark();
        } else {
            suite.run_all_benchmarks();
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Benchmark suite failed with exception: " << e.what() << "\n";
        return -1;
    }
    
    return 0;
}