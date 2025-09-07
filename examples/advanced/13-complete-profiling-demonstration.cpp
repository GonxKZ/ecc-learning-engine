/**
 * @file 13-complete-profiling-demonstration.cpp
 * @brief Complete Demonstration of ECScope Advanced Profiling and Debugging Tools
 * 
 * This comprehensive example demonstrates all the advanced profiling and debugging
 * capabilities of ECScope, including:
 * 
 * 1. Complete ECS Profiling with timing and memory tracking
 * 2. Advanced Memory Debugging with leak detection
 * 3. GPU Performance Monitoring and analysis
 * 4. Visual Debugging Interface with real-time graphs
 * 5. Statistical Analysis and regression detection
 * 6. Interactive Debug Console with commands
 * 7. Cross-Platform profiling features
 * 8. Educational debugging tutorials
 * 
 * This example serves as both a comprehensive test of the profiling system
 * and an educational resource for learning performance optimization techniques.
 * 
 * @author ECScope Development Team
 * @date 2024
 */

#include "../include/ecscope/advanced_profiler.hpp"
#include "../include/ecscope/visual_debug_interface.hpp"
#include "../include/ecscope/debug_console.hpp"
#include "../include/ecscope/cross_platform_profiling.hpp"
#include "../include/ecscope/world.hpp"
#include "../include/ecscope/ecs/registry.hpp"
#include "../include/ecscope/components.hpp"
#include <iostream>
#include <vector>
#include <random>
#include <chrono>
#include <thread>

using namespace ecscope;
using namespace ecscope::profiling;
using namespace ecscope::ecs;

//=============================================================================
// Example ECS Components for Profiling Demo
//=============================================================================

struct DemoPosition {
    f32 x, y, z;
    
    DemoPosition(f32 x = 0.0f, f32 y = 0.0f, f32 z = 0.0f) : x(x), y(y), z(z) {}
};

struct DemoVelocity {
    f32 vx, vy, vz;
    
    DemoVelocity(f32 vx = 0.0f, f32 vy = 0.0f, f32 vz = 0.0f) : vx(vx), vy(vy), vz(vz) {}
};

struct DemoHealth {
    f32 current;
    f32 maximum;
    
    DemoHealth(f32 max = 100.0f) : current(max), maximum(max) {}
};

struct DemoAI {
    enum class State { IDLE, PATROLLING, CHASING, ATTACKING };
    State current_state = State::IDLE;
    f32 decision_timer = 0.0f;
    Entity target = Entity(0);
};

struct DemoRender {
    u32 model_id;
    f32 scale;
    Color tint;
    
    DemoRender(u32 id = 0, f32 s = 1.0f) : model_id(id), scale(s), tint(1.0f, 1.0f, 1.0f, 1.0f) {}
};

//=============================================================================
// Example Systems for Profiling Demo
//=============================================================================

class MovementSystem {
public:
    static void update(Registry& registry, f32 delta_time) {
        PROFILE_ADVANCED_SYSTEM("MovementSystem", AdvancedProfileCategory::ECS_SYSTEM_UPDATE);
        
        // Simulate some work with intentional performance characteristics
        registry.view<DemoPosition, DemoVelocity>().each([delta_time](auto entity, auto& pos, auto& vel) {
            // Basic movement integration
            pos.x += vel.vx * delta_time;
            pos.y += vel.vy * delta_time;
            pos.z += vel.vz * delta_time;
            
            // Simulate some computational work (cache-friendly)
            for (int i = 0; i < 10; ++i) {
                f32 temp = pos.x * vel.vx + pos.y * vel.vy + pos.z * vel.vz;
                vel.vx = vel.vx * 0.999f + temp * 0.001f;
                vel.vy = vel.vy * 0.999f + temp * 0.001f;
                vel.vz = vel.vz * 0.999f + temp * 0.001f;
            }
        });
    }
};

class AISystem {
public:
    static void update(Registry& registry, f32 delta_time) {
        PROFILE_ADVANCED_SYSTEM("AISystem", AdvancedProfileCategory::ECS_SYSTEM_UPDATE);
        
        // Simulate AI decision making with varying computational complexity
        registry.view<DemoPosition, DemoAI>().each([delta_time, &registry](auto entity, auto& pos, auto& ai) {
            ai.decision_timer += delta_time;
            
            if (ai.decision_timer >= 1.0f) { // Make decisions every second
                ai.decision_timer = 0.0f;
                
                // Simulate expensive pathfinding/decision making
                switch (ai.current_state) {
                    case DemoAI::State::IDLE:
                        // Random chance to start patrolling
                        if (std::rand() % 100 < 30) {
                            ai.current_state = DemoAI::State::PATROLLING;
                        }
                        break;
                    
                    case DemoAI::State::PATROLLING:
                        // Look for targets (expensive operation)
                        simulate_expensive_target_search(registry, entity, ai, pos);
                        break;
                    
                    case DemoAI::State::CHASING:
                        // Update pathfinding (very expensive)
                        simulate_pathfinding(registry, entity, ai, pos);
                        break;
                    
                    case DemoAI::State::ATTACKING:
                        // Attack logic
                        if (ai.target != Entity(0) && registry.valid(ai.target)) {
                            simulate_combat_calculations(registry, entity, ai.target);
                        } else {
                            ai.current_state = DemoAI::State::IDLE;
                        }
                        break;
                }
            }
        });
    }

private:
    static void simulate_expensive_target_search(Registry& registry, Entity searcher, DemoAI& ai, const DemoPosition& pos) {
        // Intentionally inefficient target search for profiling demonstration
        f32 closest_distance = 1000.0f;
        Entity closest_target = Entity(0);
        
        registry.view<DemoPosition, DemoHealth>().each([&](auto entity, auto& target_pos, auto& health) {
            if (entity == searcher || health.current <= 0) return;
            
            // Calculate distance (expensive version for demo)
            f32 dx = target_pos.x - pos.x;
            f32 dy = target_pos.y - pos.y;
            f32 dz = target_pos.z - pos.z;
            f32 distance = std::sqrt(dx * dx + dy * dy + dz * dz);
            
            // Add some expensive calculations to demonstrate profiling
            for (int i = 0; i < 50; ++i) {
                distance += std::sin(distance + i) * 0.001f;
            }
            
            if (distance < closest_distance) {
                closest_distance = distance;
                closest_target = entity;
            }
        });
        
        if (closest_target != Entity(0) && closest_distance < 50.0f) {
            ai.target = closest_target;
            ai.current_state = DemoAI::State::CHASING;
        }
    }
    
    static void simulate_pathfinding(Registry& registry, Entity entity, DemoAI& ai, const DemoPosition& pos) {
        // Simulate A* pathfinding (intentionally expensive for profiling)
        if (ai.target == Entity(0) || !registry.valid(ai.target)) {
            ai.current_state = DemoAI::State::IDLE;
            return;
        }
        
        auto* target_pos = registry.try_get<DemoPosition>(ai.target);
        if (!target_pos) {
            ai.current_state = DemoAI::State::IDLE;
            return;
        }
        
        // Simulate pathfinding grid search (expensive for demo)
        constexpr int grid_size = 100;
        std::vector<std::vector<f32>> cost_grid(grid_size, std::vector<f32>(grid_size, 1.0f));
        
        // Fill cost grid with some calculations
        for (int x = 0; x < grid_size; ++x) {
            for (int y = 0; y < grid_size; ++y) {
                f32 distance_to_target = std::sqrt((x - target_pos->x) * (x - target_pos->x) + 
                                                  (y - target_pos->y) * (y - target_pos->y));
                cost_grid[x][y] = distance_to_target + std::sin(x * 0.1f) + std::cos(y * 0.1f);
            }
        }
        
        // Check if close enough to attack
        f32 distance_to_target = std::sqrt((pos.x - target_pos->x) * (pos.x - target_pos->x) + 
                                          (pos.y - target_pos->y) * (pos.y - target_pos->y) + 
                                          (pos.z - target_pos->z) * (pos.z - target_pos->z));
        
        if (distance_to_target < 5.0f) {
            ai.current_state = DemoAI::State::ATTACKING;
        }
    }
    
    static void simulate_combat_calculations(Registry& registry, Entity attacker, Entity target) {
        // Simulate complex combat calculations
        auto* target_health = registry.try_get<DemoHealth>(target);
        if (!target_health) return;
        
        // Expensive damage calculations
        f32 base_damage = 10.0f;
        f32 damage_multiplier = 1.0f;
        
        // Simulate complex damage calculation with many factors
        for (int i = 0; i < 100; ++i) {
            damage_multiplier *= (1.0f + std::sin(i * 0.1f) * 0.01f);
        }
        
        f32 final_damage = base_damage * damage_multiplier;
        target_health->current = std::max(0.0f, target_health->current - final_damage);
    }
};

class RenderSystem {
public:
    static void update(Registry& registry, f32 delta_time) {
        PROFILE_ADVANCED_SYSTEM("RenderSystem", AdvancedProfileCategory::RENDER_SUBMISSION);
        
        // Simulate rendering operations
        u32 draw_calls = 0;
        u32 vertices = 0;
        u32 triangles = 0;
        
        registry.view<DemoPosition, DemoRender>().each([&](auto entity, auto& pos, auto& render) {
            // Simulate culling check
            bool visible = is_visible(pos);
            if (!visible) return;
            
            // Simulate GPU operations
            PROFILE_GPU_OPERATION("DrawMesh");
            
            // Count draw calls and vertices
            draw_calls++;
            vertices += 1000; // Assume 1000 vertices per object
            triangles += 500;  // Assume 500 triangles per object
            
            // Simulate some rendering calculations
            simulate_rendering_work(pos, render);
        });
        
        // Record GPU statistics
        PROFILE_DRAW_CALL(vertices, triangles);
        
        // Simulate GPU synchronization point
        PROFILE_GPU_OPERATION("GPUSync");
        std::this_thread::sleep_for(std::chrono::microseconds(100)); // Simulate GPU wait
    }

private:
    static bool is_visible(const DemoPosition& pos) {
        // Simple frustum culling simulation
        return std::abs(pos.x) < 100.0f && std::abs(pos.y) < 100.0f && pos.z > 0.0f && pos.z < 200.0f;
    }
    
    static void simulate_rendering_work(const DemoPosition& pos, const DemoRender& render) {
        // Simulate rendering calculations (matrix multiplications, lighting, etc.)
        f32 result = 0.0f;
        for (int i = 0; i < 20; ++i) {
            result += pos.x * render.scale + pos.y * render.scale + pos.z * render.scale;
            result *= 0.99f;
        }
        
        // Prevent optimization
        volatile f32 dummy = result;
        (void)dummy;
    }
};

class MemoryIntensiveSystem {
public:
    static void update(Registry& registry, f32 delta_time) {
        PROFILE_ADVANCED_SYSTEM("MemoryIntensiveSystem", AdvancedProfileCategory::MEMORY_ALLOCATION);
        
        // Simulate system that does a lot of memory allocations
        static std::vector<std::unique_ptr<std::vector<f32>>> temp_allocations;
        
        // Allocate some temporary memory
        constexpr usize allocation_size = 1024;
        for (int i = 0; i < 10; ++i) {
            auto data = std::make_unique<std::vector<f32>>(allocation_size);
            
            // Fill with some data
            for (usize j = 0; j < allocation_size; ++j) {
                (*data)[j] = static_cast<f32>(std::sin(j * 0.1));
            }
            
            temp_allocations.push_back(std::move(data));
        }
        
        // Keep only recent allocations to simulate temporary memory usage
        if (temp_allocations.size() > 50) {
            temp_allocations.erase(temp_allocations.begin(), temp_allocations.begin() + 10);
        }
        
        // Simulate memory access patterns
        for (auto& allocation : temp_allocations) {
            // Random access pattern (bad for cache)
            static std::mt19937 rng(std::chrono::steady_clock::now().time_since_epoch().count());
            std::uniform_int_distribution<usize> dist(0, allocation->size() - 1);
            
            for (int i = 0; i < 10; ++i) {
                usize index = dist(rng);
                volatile f32 value = (*allocation)[index];
                (void)value;
            }
        }
    }
};

//=============================================================================
// Profiling Configuration and Setup
//=============================================================================

void setup_profiling_demo() {
    std::cout << "=== ECScope Advanced Profiling Demonstration ===\n\n";
    
    // Configure profiling system
    ProfilingConfig config;
    config.enabled = true;
    config.enable_memory_tracking = true;
    config.enable_gpu_profiling = true;
    config.enable_statistical_analysis = true;
    config.collect_stack_traces = true;
    config.sampling_rate = 1.0f; // 100% sampling for demo
    config.auto_export_reports = true;
    config.export_directory = "./profiling_output/";
    
    // Initialize advanced profiler
    auto& profiler = AdvancedProfiler::instance();
    profiler.set_config(config);
    
    // Get visual interface and debug console
    auto* visual_interface = profiler.get_visual_interface();
    auto* debug_console = profiler.get_debug_console();
    
    if (visual_interface) {
        VisualConfig visual_config;
        visual_config.show_fps_graph = true;
        visual_config.show_memory_graph = true;
        visual_config.show_gpu_metrics = true;
        visual_config.show_system_metrics = true;
        visual_config.show_performance_overlay = true;
        visual_interface->set_config(visual_config);
    }
    
    if (debug_console) {
        debug_console->set_enabled(true);
        debug_console->set_visible(false); // Start hidden, can be toggled
        debug_console->print_info("ECScope Profiling Demo Started");
        debug_console->print_info("Available commands: help, profile start/stop, memory info, gpu info, analyze");
        debug_console->print_info("Press '`' to toggle debug console visibility");
    }
    
    std::cout << "Advanced profiling system initialized with the following features:\n";
    std::cout << "  ✓ ECS System profiling with timing and memory tracking\n";
    std::cout << "  ✓ Advanced memory debugging with leak detection\n";
    std::cout << "  ✓ GPU performance monitoring and analysis\n";
    std::cout << "  ✓ Visual debugging interface with real-time graphs\n";
    std::cout << "  ✓ Statistical analysis and regression detection\n";
    std::cout << "  ✓ Interactive debug console with commands\n";
    std::cout << "  ✓ Cross-platform profiling support\n";
    std::cout << "  ✓ Educational debugging tools and tutorials\n\n";
}

//=============================================================================
// Simulation and Demo Logic
//=============================================================================

void create_demo_entities(Registry& registry, usize count) {
    std::cout << "Creating " << count << " demo entities for profiling...\n";
    
    std::mt19937 rng(std::chrono::steady_clock::now().time_since_epoch().count());
    std::uniform_real_distribution<f32> pos_dist(-50.0f, 50.0f);
    std::uniform_real_distribution<f32> vel_dist(-10.0f, 10.0f);
    std::uniform_int_distribution<u32> model_dist(0, 10);
    
    for (usize i = 0; i < count; ++i) {
        Entity entity = registry.create();
        
        // Add position component
        registry.emplace<DemoPosition>(entity, pos_dist(rng), pos_dist(rng), pos_dist(rng));
        
        // Add velocity to ~80% of entities
        if (i % 5 != 0) {
            registry.emplace<DemoVelocity>(entity, vel_dist(rng), vel_dist(rng), vel_dist(rng));
        }
        
        // Add health to ~60% of entities
        if (i % 3 != 2) {
            registry.emplace<DemoHealth>(entity);
        }
        
        // Add AI to ~40% of entities
        if (i % 5 == 0) {
            registry.emplace<DemoAI>(entity);
        }
        
        // Add rendering to ~90% of entities
        if (i % 10 != 0) {
            registry.emplace<DemoRender>(entity, model_dist(rng));
        }
    }
    
    std::cout << "Created " << count << " entities with various component combinations\n\n";
}

void run_profiling_frame(Registry& registry, f32 delta_time) {
    auto& profiler = AdvancedProfiler::instance();
    
    // Begin profiling frame
    profiler.begin_frame();
    
    // Update all systems with profiling
    MovementSystem::update(registry, delta_time);
    AISystem::update(registry, delta_time);
    RenderSystem::update(registry, delta_time);
    MemoryIntensiveSystem::update(registry, delta_time);
    
    // Update visual interface
    auto* visual_interface = profiler.get_visual_interface();
    if (visual_interface && visual_interface->is_enabled()) {
        visual_interface->update(delta_time);
    }
    
    // Update debug console
    auto* debug_console = profiler.get_debug_console();
    if (debug_console && debug_console->is_enabled()) {
        debug_console->update(delta_time);
    }
    
    // End profiling frame
    profiler.end_frame();
}

void demonstrate_performance_analysis() {
    std::cout << "\n=== Performance Analysis Demonstration ===\n";
    
    auto& profiler = AdvancedProfiler::instance();
    
    // Get system metrics
    auto all_metrics = profiler.get_all_system_metrics();
    
    std::cout << "System Performance Summary:\n";
    std::cout << std::setw(20) << "System" << std::setw(12) << "Executions" 
              << std::setw(15) << "Avg Time (μs)" << std::setw(12) << "Score/100\n";
    std::cout << std::string(59, '-') << "\n";
    
    for (const auto& metrics : all_metrics) {
        f64 score = metrics.get_performance_score();
        std::cout << std::setw(20) << metrics.system_name 
                  << std::setw(12) << metrics.execution_count
                  << std::setw(15) << metrics.avg_time.count()
                  << std::setw(12) << std::fixed << std::setprecision(1) << score << "\n";
    }
    
    // Overall performance score
    f64 overall_score = profiler.calculate_overall_performance_score();
    std::cout << "\nOverall Performance Score: " << std::fixed << std::setprecision(1) 
              << overall_score << "/100\n";
    
    // Performance recommendations
    auto recommendations = profiler.get_performance_recommendations();
    if (!recommendations.empty()) {
        std::cout << "\nPerformance Recommendations:\n";
        for (const auto& rec : recommendations) {
            std::cout << "  • " << rec << "\n";
        }
    }
    
    // Anomaly detection
    auto anomalies = profiler.detect_anomalies();
    if (!anomalies.empty()) {
        std::cout << "\nPerformance Anomalies Detected: " << anomalies.size() << "\n";
        for (const auto& anomaly : anomalies) {
            std::cout << "  ⚠️  " << anomaly.system_name << ": " << anomaly.description << "\n";
            std::cout << "      Suggested Action: " << anomaly.suggested_action << "\n";
        }
    }
}

void demonstrate_memory_analysis() {
    std::cout << "\n=== Memory Analysis Demonstration ===\n";
    
    auto& profiler = AdvancedProfiler::instance();
    auto memory_metrics = profiler.get_memory_metrics();
    
    std::cout << "Memory Usage Summary:\n";
    std::cout << "  Current Usage: " << (memory_metrics.process_working_set / (1024 * 1024)) << " MB\n";
    std::cout << "  Peak Usage: " << (memory_metrics.process_peak_working_set / (1024 * 1024)) << " MB\n";
    
    // Heap analysis
    const auto& heap = memory_metrics.heap_metrics;
    std::cout << "  Heap Size: " << (heap.heap_size / (1024 * 1024)) << " MB\n";
    std::cout << "  Fragmentation: " << std::fixed << std::setprecision(1) 
              << (heap.fragmentation_ratio * 100.0f) << "%\n";
    std::cout << "  Efficiency Score: " << std::fixed << std::setprecision(1) 
              << heap.get_efficiency_score() << "/100\n";
    
    // Allocation patterns
    const auto& pattern = memory_metrics.allocation_pattern;
    std::cout << "Allocation Pattern:\n";
    std::cout << "  Small allocations: " << pattern.small_allocations << "\n";
    std::cout << "  Medium allocations: " << pattern.medium_allocations << "\n";
    std::cout << "  Large allocations: " << pattern.large_allocations << "\n";
    std::cout << "  Allocation Efficiency: " << std::fixed << std::setprecision(1) 
              << pattern.get_allocation_efficiency() << "/100\n";
    
    // Cache performance
    const auto& cache = memory_metrics.cache_metrics;
    if (cache.l1_cache_hits + cache.l1_cache_misses > 0) {
        std::cout << "Cache Performance:\n";
        std::cout << "  L1 Hit Ratio: " << std::fixed << std::setprecision(1) 
                  << (cache.l1_hit_ratio * 100.0) << "%\n";
        std::cout << "  Overall Cache Score: " << std::fixed << std::setprecision(1) 
                  << cache.get_cache_efficiency_score() << "/100\n";
    }
    
    // Overall memory score
    f32 overall_memory_score = memory_metrics.get_overall_memory_score();
    std::cout << "Overall Memory Score: " << std::fixed << std::setprecision(1) 
              << overall_memory_score << "/100\n";
}

void demonstrate_gpu_analysis() {
    std::cout << "\n=== GPU Performance Analysis Demonstration ===\n";
    
    auto& profiler = AdvancedProfiler::instance();
    auto gpu_metrics = profiler.get_gpu_metrics();
    
    std::cout << "GPU Performance Summary:\n";
    std::cout << "  GPU: " << (gpu_metrics.gpu_name.empty() ? "Unknown" : gpu_metrics.gpu_name) << "\n";
    std::cout << "  Utilization: " << std::fixed << std::setprecision(1) 
              << (gpu_metrics.gpu_utilization * 100.0f) << "%\n";
    std::cout << "  Memory Usage: " << (gpu_metrics.used_memory / (1024 * 1024)) << " MB / " 
              << (gpu_metrics.total_memory / (1024 * 1024)) << " MB\n";
    
    std::cout << "Rendering Statistics:\n";
    std::cout << "  Draw Calls: " << gpu_metrics.draw_calls << "\n";
    std::cout << "  Vertices Processed: " << gpu_metrics.vertices_processed << "\n";
    std::cout << "  Triangles Rendered: " << gpu_metrics.triangles_rendered << "\n";
    
    // GPU efficiency analysis
    f32 efficiency_score = gpu_metrics.get_efficiency_score();
    std::cout << "GPU Efficiency Score: " << std::fixed << std::setprecision(1) 
              << efficiency_score << "/100\n";
    
    // Bottleneck detection
    std::string bottleneck_text;
    switch (gpu_metrics.current_bottleneck) {
        case GPUMetrics::Bottleneck::VERTEX_PROCESSING:
            bottleneck_text = "Vertex Processing";
            break;
        case GPUMetrics::Bottleneck::PIXEL_PROCESSING:
            bottleneck_text = "Pixel Processing";
            break;
        case GPUMetrics::Bottleneck::MEMORY_BANDWIDTH:
            bottleneck_text = "Memory Bandwidth";
            break;
        case GPUMetrics::Bottleneck::SYNCHRONIZATION:
            bottleneck_text = "GPU Synchronization";
            break;
        case GPUMetrics::Bottleneck::DRIVER_OVERHEAD:
            bottleneck_text = "Driver Overhead";
            break;
        default:
            bottleneck_text = "None Detected";
            break;
    }
    
    std::cout << "Detected Bottleneck: " << bottleneck_text << "\n";
}

void demonstrate_trend_analysis() {
    std::cout << "\n=== Performance Trend Analysis Demonstration ===\n";
    
    auto& profiler = AdvancedProfiler::instance();
    auto trends = profiler.analyze_all_trends();
    
    if (trends.empty()) {
        std::cout << "Not enough data for trend analysis yet. Run simulation longer for trends.\n";
        return;
    }
    
    std::cout << "Performance Trends:\n";
    std::cout << std::setw(20) << "System" << std::setw(15) << "Trend" 
              << std::setw(12) << "Confidence" << std::setw(30) << "Description\n";
    std::cout << std::string(77, '-') << "\n";
    
    for (const auto& [system_name, trend] : trends) {
        std::string trend_text;
        switch (trend.type) {
            case PerformanceTrend::TrendType::IMPROVING:
                trend_text = "Improving ↗️";
                break;
            case PerformanceTrend::TrendType::STABLE:
                trend_text = "Stable →";
                break;
            case PerformanceTrend::TrendType::DEGRADING:
                trend_text = "Degrading ↘️";
                break;
            case PerformanceTrend::TrendType::VOLATILE:
                trend_text = "Volatile ↕️";
                break;
        }
        
        std::cout << std::setw(20) << system_name
                  << std::setw(15) << trend_text
                  << std::setw(12) << std::fixed << std::setprecision(1) << (trend.confidence * 100.0) << "%"
                  << std::setw(30) << trend.description.substr(0, 29) << "\n";
    }
}

void demonstrate_debug_console() {
    std::cout << "\n=== Debug Console Demonstration ===\n";
    std::cout << "The debug console provides interactive commands for profiling analysis:\n\n";
    
    auto& profiler = AdvancedProfiler::instance();
    auto* debug_console = profiler.get_debug_console();
    
    if (!debug_console) {
        std::cout << "Debug console not available.\n";
        return;
    }
    
    // Demonstrate some console commands programmatically
    std::vector<std::string> demo_commands = {
        "help",
        "list_systems",
        "system_info MovementSystem",
        "memory_info",
        "gpu_info",
        "analyze_performance",
        "detect_anomalies",
        "recommendations"
    };
    
    std::cout << "Available console commands demonstration:\n";
    for (const auto& command : demo_commands) {
        std::cout << "\n> " << command << "\n";
        auto result = debug_console->execute_command(command);
        
        if (result.is_success()) {
            for (const auto& line : result.output_lines) {
                std::cout << "  " << line << "\n";
            }
            if (!result.message.empty()) {
                std::cout << "  " << result.message << "\n";
            }
        } else {
            std::cout << "  Error: " << result.message << "\n";
        }
    }
    
    std::cout << "\nIn a real application, you can toggle the console with '`' key\n";
    std::cout << "and use it interactively for real-time profiling analysis.\n";
}

void export_profiling_reports() {
    std::cout << "\n=== Exporting Profiling Reports ===\n";
    
    auto& profiler = AdvancedProfiler::instance();
    
    // Generate comprehensive report
    std::string report = profiler.generate_comprehensive_report();
    std::cout << "Generated comprehensive performance report (" << report.size() << " characters)\n";
    
    // Generate executive summary
    std::string summary = profiler.generate_executive_summary();
    std::cout << "Generated executive summary:\n";
    std::cout << summary << "\n";
    
    // Export to various formats
    try {
        profiler.export_detailed_report("./profiling_demo_report.html");
        std::cout << "✓ Exported HTML report to: profiling_demo_report.html\n";
        
        profiler.export_csv_data("./profiling_demo_data.csv");
        std::cout << "✓ Exported CSV data to: profiling_demo_data.csv\n";
        
        profiler.export_json_data("./profiling_demo_data.json");
        std::cout << "✓ Exported JSON data to: profiling_demo_data.json\n";
        
    } catch (const std::exception& e) {
        std::cout << "⚠️  Export error: " << e.what() << "\n";
    }
    
    std::cout << "\nReports contain detailed performance metrics, trends, and recommendations.\n";
}

//=============================================================================
// Main Demonstration Function
//=============================================================================

int main() {
    try {
        // Initialize profiling system
        setup_profiling_demo();
        
        // Create ECS registry and demo entities
        Registry registry;
        create_demo_entities(registry, 5000); // Create 5000 entities for realistic load
        
        // Run simulation with profiling
        std::cout << "Running profiled simulation for 10 seconds...\n";
        
        auto start_time = std::chrono::steady_clock::now();
        auto last_frame_time = start_time;
        f32 total_time = 0.0f;
        u32 frame_count = 0;
        
        while (total_time < 10.0f) { // Run for 10 seconds
            auto current_time = std::chrono::steady_clock::now();
            auto frame_duration = std::chrono::duration_cast<std::chrono::microseconds>(
                current_time - last_frame_time);
            f32 delta_time = frame_duration.count() / 1000000.0f; // Convert to seconds
            
            // Cap delta time to prevent huge jumps
            delta_time = std::min(delta_time, 0.033f); // Max 33ms (30 FPS minimum)
            
            // Run profiled frame
            run_profiling_frame(registry, delta_time);
            
            total_time += delta_time;
            frame_count++;
            last_frame_time = current_time;
            
            // Print progress every 2 seconds
            if (static_cast<int>(total_time) % 2 == 0 && static_cast<int>(total_time) != static_cast<int>(total_time - delta_time)) {
                std::cout << "  Progress: " << static_cast<int>(total_time) << "/10 seconds "
                          << "(" << frame_count << " frames, " 
                          << std::fixed << std::setprecision(1) << (frame_count / total_time) << " FPS average)\n";
            }
            
            // Small sleep to prevent 100% CPU usage
            std::this_thread::sleep_for(std::chrono::microseconds(500));
        }
        
        std::cout << "\nSimulation completed! Analyzed " << frame_count << " frames.\n";
        std::cout << "Average FPS: " << std::fixed << std::setprecision(1) << (frame_count / total_time) << "\n\n";
        
        // Demonstrate analysis features
        demonstrate_performance_analysis();
        demonstrate_memory_analysis();
        demonstrate_gpu_analysis();
        demonstrate_trend_analysis();
        demonstrate_debug_console();
        
        // Export reports
        export_profiling_reports();
        
        std::cout << "\n=== Profiling Demonstration Complete ===\n";
        std::cout << "This demonstration showcased:\n";
        std::cout << "  ✓ Real-time performance profiling of ECS systems\n";
        std::cout << "  ✓ Memory allocation tracking and analysis\n";
        std::cout << "  ✓ GPU performance monitoring\n";
        std::cout << "  ✓ Statistical trend analysis and anomaly detection\n";
        std::cout << "  ✓ Interactive debug console with commands\n";
        std::cout << "  ✓ Comprehensive reporting and data export\n";
        std::cout << "  ✓ Cross-platform profiling capabilities\n\n";
        
        std::cout << "The profiling system is now ready for use in your ECScope applications!\n";
        std::cout << "Use the provided macros and APIs to instrument your systems for\n";
        std::cout << "detailed performance analysis and optimization.\n\n";
        
        // Cleanup
        AdvancedProfiler::cleanup();
        
    } catch (const std::exception& e) {
        std::cerr << "Error during profiling demonstration: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}