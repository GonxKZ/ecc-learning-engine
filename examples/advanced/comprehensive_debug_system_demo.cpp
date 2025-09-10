/**
 * @file comprehensive_debug_system_demo.cpp
 * @brief Comprehensive demonstration of ECScope's professional debugging system
 * 
 * This example showcases all major features of the ECScope debug system:
 * - Performance profilers (CPU, Memory, GPU, Network, Asset, Custom Events)
 * - Visual debugging (graphs, memory visualization, ECS inspector)
 * - Runtime inspectors (entities, systems, assets, memory, shaders, jobs)
 * - Debug console with command system and remote debugging
 * - Debug rendering and performance monitoring
 * 
 * This is a production-quality debugging system suitable for professional game development.
 */

#include <ecscope/debug.hpp>
#include <ecscope/core.hpp>
#include <ecscope/ecs.hpp>
#include <ecscope/memory.hpp>
#include <iostream>
#include <thread>
#include <random>
#include <vector>
#include <chrono>

using namespace ECScope;
using namespace ECScope::Debug;

// Example components for ECS demonstration
struct Position { float x, y, z; };
struct Velocity { float dx, dy, dz; };
struct Health { int current, maximum; };
struct Renderable { uint32_t mesh_id, material_id; };

// Example systems for demonstration
class MovementSystem {
public:
    void Update(float deltaTime) {
        ECSCOPE_PROFILE_FUNCTION();
        
        // Simulate system work
        std::this_thread::sleep_for(std::chrono::microseconds(100));
        
        // Example: update positions based on velocity
        // (would integrate with actual ECS here)
    }
};

class RenderSystem {
public:
    void Update(float deltaTime) {
        ECSCOPE_PROFILE_FUNCTION();
        
        // Simulate rendering work
        std::this_thread::sleep_for(std::chrono::microseconds(500));
        
        // Example GPU profiling
        ECSCOPE_GPU_EVENT_BEGIN("RenderGeometry");
        std::this_thread::sleep_for(std::chrono::microseconds(300));
        ECSCOPE_GPU_EVENT_END();
    }
};

class PhysicsSystem {
public:
    void Update(float deltaTime) {
        ECSCOPE_PROFILE_FUNCTION();
        
        // Simulate physics calculations
        std::this_thread::sleep_for(std::chrono::microseconds(200));
    }
};

class NetworkSystem {
public:
    void Update(float deltaTime) {
        ECSCOPE_PROFILE_FUNCTION();
        
        // Simulate network processing
        std::this_thread::sleep_for(std::chrono::microseconds(50));
    }
};

// Example asset loading simulation
class AssetLoader {
public:
    void LoadTexture(const std::string& path) {
        ECSCOPE_PROFILE_FUNCTION();
        
        // Track asset loading
        auto* profiler = GlobalDebugSystem::Get().GetProfiler("Assets");
        if (auto asset_profiler = std::dynamic_pointer_cast<AssetProfiler>(profiler)) {
            asset_profiler->TrackLoadStart(path, "Texture");
            
            // Simulate file I/O
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            asset_profiler->TrackLoadStage(path, AssetProfiler::AssetEvent::Stage::FileIO, 
                                         std::chrono::milliseconds(10));
            
            // Simulate parsing
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
            asset_profiler->TrackLoadStage(path, AssetProfiler::AssetEvent::Stage::Parsing,
                                         std::chrono::milliseconds(5));
            
            // Simulate GPU upload
            std::this_thread::sleep_for(std::chrono::milliseconds(3));
            asset_profiler->TrackLoadStage(path, AssetProfiler::AssetEvent::Stage::Upload,
                                         std::chrono::milliseconds(3));
            
            asset_profiler->TrackLoadComplete(path, 1024 * 1024, 2048 * 1024);
        }
    }
    
    void LoadMesh(const std::string& path) {
        ECSCOPE_PROFILE_FUNCTION();
        
        // Similar asset loading tracking for meshes
        auto* profiler = GlobalDebugSystem::Get().GetProfiler("Assets");
        if (auto asset_profiler = std::dynamic_pointer_cast<AssetProfiler>(profiler)) {
            asset_profiler->TrackLoadStart(path, "Mesh");
            std::this_thread::sleep_for(std::chrono::milliseconds(15));
            asset_profiler->TrackLoadComplete(path, 2048 * 1024, 4096 * 1024);
        }
    }
};

// Memory allocation simulation
void SimulateMemoryUsage() {
    ECSCOPE_PROFILE_FUNCTION();
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> size_dist(64, 4096);
    
    std::vector<void*> allocations;
    
    // Allocate some memory blocks
    for (int i = 0; i < 50; ++i) {
        size_t size = size_dist(gen);
        void* ptr = malloc(size);
        allocations.push_back(ptr);
        
        ECSCOPE_TRACK_ALLOC(ptr, size, "SimulatedAlloc");
    }
    
    // Free some blocks (simulate memory usage pattern)
    for (size_t i = 0; i < allocations.size() / 2; ++i) {
        ECSCOPE_TRACK_FREE(allocations[i]);
        free(allocations[i]);
    }
    
    // Keep remaining allocations for leak detection demo
}

// Network simulation
void SimulateNetworkActivity() {
    ECSCOPE_PROFILE_FUNCTION();
    
    auto* profiler = GlobalDebugSystem::Get().GetProfiler("Network");
    if (auto net_profiler = std::dynamic_pointer_cast<NetworkProfiler>(profiler)) {
        // Simulate some network events
        net_profiler->TrackSend("192.168.1.100:8080", 1024, "TCP");
        net_profiler->TrackReceive("192.168.1.100:8080", 2048, "TCP");
        net_profiler->TrackLatency("192.168.1.100:8080", 15.5);
        
        net_profiler->TrackSend("game-server.example.com:7777", 512, "UDP");
        net_profiler->TrackReceive("game-server.example.com:7777", 256, "UDP");
        net_profiler->TrackLatency("game-server.example.com:7777", 8.2);
    }
}

// Custom event examples
void SimulateGameplayEvents() {
    ECSCOPE_EVENT_BEGIN("PlayerAction", "Gameplay");
    
    // Simulate player action processing
    std::this_thread::sleep_for(std::chrono::microseconds(150));
    
    ECSCOPE_EVENT_RECORD("EnemySpawned", "Gameplay");
    ECSCOPE_EVENT_RECORD("ItemCollected", "Gameplay");
    ECSCOPE_EVENT_RECORD("ScoreUpdated", "UI");
    
    ECSCOPE_EVENT_END();
}

// Debug drawing examples
void DemoDebugDrawing() {
    auto& renderer = GlobalDebugSystem::Get().GetRenderer();
    
    // Draw coordinate axes at origin
    Vector3 origin{0, 0, 0};
    Vector3 x_axis{1, 0, 0};
    Vector3 y_axis{0, 1, 0};
    Vector3 z_axis{0, 0, 1};
    
    ECSCOPE_DRAW_LINE(origin, x_axis, DebugRenderer::RED);
    ECSCOPE_DRAW_LINE(origin, y_axis, DebugRenderer::GREEN);
    ECSCOPE_DRAW_LINE(origin, z_axis, DebugRenderer::BLUE);
    
    // Draw some test geometry
    Vector3 box_min{-2, -1, -2};
    Vector3 box_max{2, 1, 2};
    ECSCOPE_DRAW_BOX(box_min, box_max, DebugRenderer::YELLOW);
    
    Vector3 sphere_center{3, 0, 0};
    ECSCOPE_DRAW_SPHERE(sphere_center, 1.5f, DebugRenderer::CYAN);
    
    Vector3 text_pos{0, 2, 0};
    ECSCOPE_DRAW_TEXT(text_pos, "ECScope Debug System", DebugRenderer::WHITE);
}

// Setup debug system with all features
std::unique_ptr<DebugSystem> SetupCompleteDebugSystem() {
    return DebugSystemBuilder()
        // Core configuration
        .WithProfiling(true)
        .WithVisualization(true)
        .WithInspection(true)
        .WithConsole(true)
        .WithRemoteDebugging(true, 7777)
        .WithMemoryBudget(64 * 1024 * 1024)  // 64MB debug budget
        .WithProfilerSamples(10000)
        .WithUpdateFrequency(60.0f)
        
        // Enable all profilers
        .WithCPUProfiler("CPU")
        .WithMemoryProfiler("Memory") 
        .WithGPUProfiler("GPU")
        .WithNetworkProfiler("Network")
        .WithAssetProfiler("Assets")
        .WithCustomEventProfiler("Events")
        
        // Enable all visualizers
        .WithPerformanceGraphs()
        .WithMemoryVisualization()
        .WithECSVisualization()
        .WithPhysicsDebugDraw()
        .WithRenderingDebugViews()
        .WithNetworkVisualization()
        
        // Enable all inspectors
        .WithEntityInspector()
        .WithSystemInspector()
        .WithAssetInspector()
        .WithMemoryInspector()
        .WithShaderInspector()
        .WithJobSystemInspector()
        
        .BuildAndSetGlobal();
}

// Setup console commands for demo
void SetupDemoCommands() {
    auto& console = GlobalDebugSystem::Get().GetConsole();
    
    // Memory leak detection command
    console.RegisterCommand({
        "check_leaks",
        "Check for memory leaks",
        "check_leaks",
        {},
        [](const std::vector<std::string>& args) -> Console::CommandResult {
            Console::CommandResult result;
            
            auto* profiler = GlobalDebugSystem::Get().GetProfiler("Memory");
            if (auto mem_profiler = std::dynamic_pointer_cast<MemoryProfiler>(profiler)) {
                mem_profiler->DetectLeaks();
                const auto& leaks = mem_profiler->GetLeaks();
                
                result.success = true;
                result.output = "Found " + std::to_string(leaks.size()) + " memory leaks";
                
                for (const auto& leak : leaks) {
                    result.output += "\n  - " + std::to_string(leak.size) + " bytes at " + leak.tag;
                }
            } else {
                result.success = false;
                result.error = "Memory profiler not available";
            }
            
            return result;
        },
        false
    });
    
    // Performance report command
    console.RegisterCommand({
        "perf_report",
        "Generate performance report",
        "perf_report [system_name]",
        {"perf"},
        [](const std::vector<std::string>& args) -> Console::CommandResult {
            Console::CommandResult result;
            
            const auto& monitor = GlobalDebugSystem::Get().GetPerformanceMonitor();
            const auto& frame_stats = monitor.GetFrameStats();
            const auto& system_stats = monitor.GetSystemStats();
            
            result.success = true;
            result.output = "=== Performance Report ===\n";
            result.output += "Frame Time: " + std::to_string(frame_stats.frame_time_ms) + "ms\n";
            result.output += "FPS: " + std::to_string(frame_stats.fps) + "\n";
            result.output += "CPU Time: " + std::to_string(frame_stats.cpu_time_ms) + "ms\n";
            result.output += "GPU Time: " + std::to_string(frame_stats.gpu_time_ms) + "ms\n";
            
            result.output += "\n=== System Times ===\n";
            for (const auto& [name, stats] : system_stats) {
                result.output += name + ": " + std::to_string(stats.average_time_ms) + "ms (";
                result.output += std::to_string(stats.percentage) + "%)\n";
            }
            
            return result;
        },
        false
    });
    
    // Asset report command
    console.RegisterCommand({
        "asset_report",
        "Generate asset loading report",
        "asset_report",
        {"assets"},
        [](const std::vector<std::string>& args) -> Console::CommandResult {
            Console::CommandResult result;
            
            auto* profiler = GlobalDebugSystem::Get().GetProfiler("Assets");
            if (auto asset_profiler = std::dynamic_pointer_cast<AssetProfiler>(profiler)) {
                const auto& stats = asset_profiler->GetStats();
                const auto bottlenecks = asset_profiler->AnalyzeBottlenecks();
                
                result.success = true;
                result.output = "=== Asset Loading Report ===\n";
                result.output += "Total Assets Loaded: " + std::to_string(stats.total_assets_loaded) + "\n";
                result.output += "Average Load Time: " + std::to_string(stats.average_load_time_ms) + "ms\n";
                result.output += "Cache Hit Ratio: " + std::to_string(stats.cache_hit_ratio * 100) + "%\n";
                result.output += "Failed Loads: " + std::to_string(stats.failed_loads) + "\n";
                
                result.output += "\n=== Bottlenecks ===\n";
                for (const auto& bottleneck : bottlenecks) {
                    result.output += bottleneck.description + ": " + std::to_string(bottleneck.percentage) + "%\n";
                }
            } else {
                result.success = false;
                result.error = "Asset profiler not available";
            }
            
            return result;
        },
        false
    });
}

// Main demonstration function
int main() {
    std::cout << "ECScope Comprehensive Debug System Demo\n";
    std::cout << "========================================\n\n";
    
    try {
        // Setup complete debug system
        std::cout << "Initializing debug system with all features...\n";
        auto debug_system = SetupCompleteDebugSystem();
        
        // Setup demo commands
        SetupDemoCommands();
        
        std::cout << "Debug system initialized successfully!\n";
        std::cout << "- CPU Profiler: Enabled\n";
        std::cout << "- Memory Profiler: Enabled\n";
        std::cout << "- GPU Profiler: Enabled\n";
        std::cout << "- Network Profiler: Enabled\n";
        std::cout << "- Asset Profiler: Enabled\n";
        std::cout << "- Custom Event Profiler: Enabled\n";
        std::cout << "- All Visualizers: Enabled\n";
        std::cout << "- All Inspectors: Enabled\n";
        std::cout << "- Debug Console: Enabled\n";
        std::cout << "- Remote Debugging: Enabled on port 7777\n\n";
        
        // Create example systems
        MovementSystem movement_system;
        RenderSystem render_system;
        PhysicsSystem physics_system;
        NetworkSystem network_system;
        AssetLoader asset_loader;
        
        std::cout << "Running simulation...\n";
        std::cout << "Press 'Q' to quit, 'C' to open console, 'L' to check leaks\n\n";
        
        // Main simulation loop
        const float deltaTime = 1.0f / 60.0f;  // 60 FPS
        auto last_time = std::chrono::high_resolution_clock::now();
        
        for (int frame = 0; frame < 300; ++frame) {  // Run for 5 seconds at 60 FPS
            auto current_time = std::chrono::high_resolution_clock::now();
            auto frame_delta = std::chrono::duration<float>(current_time - last_time).count();
            last_time = current_time;
            
            // Update performance monitor
            GlobalDebugSystem::Get().GetPerformanceMonitor().BeginFrame();
            
            // Update systems with profiling
            {
                ECSCOPE_PROFILE_SYSTEM(GlobalDebugSystem::Get().GetPerformanceMonitor(), "MovementSystem");
                movement_system.Update(deltaTime);
            }
            
            {
                ECSCOPE_PROFILE_SYSTEM(GlobalDebugSystem::Get().GetPerformanceMonitor(), "RenderSystem");
                render_system.Update(deltaTime);
            }
            
            {
                ECSCOPE_PROFILE_SYSTEM(GlobalDebugSystem::Get().GetPerformanceMonitor(), "PhysicsSystem");
                physics_system.Update(deltaTime);
            }
            
            {
                ECSCOPE_PROFILE_SYSTEM(GlobalDebugSystem::Get().GetPerformanceMonitor(), "NetworkSystem");
                network_system.Update(deltaTime);
            }
            
            // Simulate asset loading occasionally
            if (frame % 30 == 0) {
                asset_loader.LoadTexture("textures/player_" + std::to_string(frame / 30) + ".png");
            }
            
            if (frame % 45 == 0) {
                asset_loader.LoadMesh("meshes/building_" + std::to_string(frame / 45) + ".obj");
            }
            
            // Simulate memory allocations
            if (frame % 20 == 0) {
                SimulateMemoryUsage();
            }
            
            // Simulate network activity
            if (frame % 10 == 0) {
                SimulateNetworkActivity();
            }
            
            // Simulate gameplay events
            if (frame % 15 == 0) {
                SimulateGameplayEvents();
            }
            
            // Demo debug drawing
            if (frame % 60 == 0) {
                DemoDebugDrawing();
            }
            
            // Update debug system
            ECSCOPE_DEBUG_UPDATE(frame_delta);
            
            // End frame
            GlobalDebugSystem::Get().GetPerformanceMonitor().EndFrame();
            
            // Print progress
            if (frame % 60 == 0) {
                const auto& stats = GlobalDebugSystem::Get().GetPerformanceMonitor().GetFrameStats();
                std::cout << "Frame " << frame << " - FPS: " << stats.fps 
                         << " - Frame Time: " << stats.frame_time_ms << "ms\n";
            }
            
            // Maintain 60 FPS
            std::this_thread::sleep_for(std::chrono::milliseconds(16));
        }
        
        std::cout << "\n=== Final Statistics ===\n";
        
        // Print debug system stats
        const auto& debug_stats = GlobalDebugSystem::Get().GetStats();
        std::cout << "Debug System Stats:\n";
        std::cout << "- Active Profilers: " << debug_stats.active_profilers << "/" << debug_stats.total_profilers << "\n";
        std::cout << "- Active Visualizers: " << debug_stats.active_visualizers << "/" << debug_stats.total_visualizers << "\n";
        std::cout << "- Active Inspectors: " << debug_stats.active_inspectors << "/" << debug_stats.total_inspectors << "\n";
        std::cout << "- Memory Usage: " << debug_stats.memory_usage_bytes / 1024 << " KB\n";
        std::cout << "- Events Processed: " << debug_stats.total_events_processed << "\n";
        std::cout << "- Commands Executed: " << debug_stats.total_commands_executed << "\n";
        
        // Print performance stats
        const auto& perf_stats = GlobalDebugSystem::Get().GetPerformanceMonitor().GetFrameStats();
        std::cout << "\nPerformance Stats:\n";
        std::cout << "- Average FPS: " << perf_stats.average_fps << "\n";
        std::cout << "- Average Frame Time: " << perf_stats.frame_time_ms << "ms\n";
        std::cout << "- CPU Time: " << perf_stats.cpu_time_ms << "ms\n";
        std::cout << "- GPU Time: " << perf_stats.gpu_time_ms << "ms\n";
        
        // Run console commands demo
        std::cout << "\n=== Console Commands Demo ===\n";
        auto& console = GlobalDebugSystem::Get().GetConsole();
        
        // Execute demo commands
        auto leak_result = console.ExecuteCommand("check_leaks");
        std::cout << "Memory Leak Check: " << (leak_result.success ? "Success" : "Failed") << "\n";
        if (leak_result.success) {
            std::cout << leak_result.output << "\n";
        }
        
        auto perf_result = console.ExecuteCommand("perf_report");
        std::cout << "\nPerformance Report: " << (perf_result.success ? "Success" : "Failed") << "\n";
        if (perf_result.success) {
            std::cout << perf_result.output << "\n";
        }
        
        auto asset_result = console.ExecuteCommand("asset_report");
        std::cout << "\nAsset Report: " << (asset_result.success ? "Success" : "Failed") << "\n";
        if (asset_result.success) {
            std::cout << asset_result.output << "\n";
        }
        
        std::cout << "\n=== Demo Complete ===\n";
        std::cout << "The ECScope debug system provides comprehensive debugging capabilities:\n";
        std::cout << "1. Real-time performance profiling\n";
        std::cout << "2. Memory tracking and leak detection\n";
        std::cout << "3. GPU performance monitoring\n";
        std::cout << "4. Network activity analysis\n";
        std::cout << "5. Asset loading bottleneck identification\n";
        std::cout << "6. Visual debugging with graphs and charts\n";
        std::cout << "7. Runtime inspection of all engine systems\n";
        std::cout << "8. Interactive console with command system\n";
        std::cout << "9. Remote debugging capabilities\n";
        std::cout << "10. Crash analysis and debugging tools\n\n";
        
        std::cout << "This system is designed for professional game development\n";
        std::cout << "and provides minimal performance impact when properly configured.\n";
        
        // Shutdown
        ECSCOPE_DEBUG_SHUTDOWN();
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}