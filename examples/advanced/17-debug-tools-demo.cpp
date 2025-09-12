#include <iostream>
#include <memory>
#include <thread>
#include <chrono>
#include <random>
#include <vector>
#include <string>

#ifdef ECSCOPE_HAS_GUI
#include "ecscope/gui/dashboard.hpp"
#include "ecscope/gui/debug_tools_ui.hpp"
#include "ecscope/gui/gui_manager.hpp"
#endif

class MockGameSystem {
public:
    MockGameSystem() : frame_count_(0), total_entities_(1000), is_running_(false) {
        rng_.seed(std::chrono::steady_clock::now().time_since_epoch().count());
    }

    void initialize() {
#ifdef ECSCOPE_HAS_GUI
        ecscope::gui::DebugToolsManager::instance().initialize();
        
        // Simulate some initial memory allocations
        allocate_mock_memory();
        
        std::cout << "Mock game system initialized with " << total_entities_ << " entities" << std::endl;
#endif
    }

    void start_simulation() {
        is_running_ = true;
        std::cout << "Starting game simulation..." << std::endl;
    }

    void stop_simulation() {
        is_running_ = false;
        std::cout << "Game simulation stopped" << std::endl;
    }

    void update(float delta_time, ecscope::gui::DebugToolsUI* debug_tools) {
        if (!is_running_) return;
        
        frame_count_++;
        
        // Simulate frame profiling
        simulate_frame_work(debug_tools);
        
        // Simulate various game systems
        simulate_rendering_system(debug_tools);
        simulate_physics_system(debug_tools);
        simulate_audio_system(debug_tools);
        simulate_ai_system(debug_tools);
        
        // Occasionally simulate memory operations
        if (frame_count_ % 60 == 0) {
            simulate_memory_operations(debug_tools);
        }
        
        // Generate various log messages
        generate_log_messages(debug_tools);
        
        // Record custom metrics
        record_custom_metrics(debug_tools);
        
        // Simulate performance issues occasionally
        if (uniform_dist_(rng_) < 0.01f) { // 1% chance per frame
            simulate_performance_spike(debug_tools);
        }
    }

private:
    void simulate_frame_work(ecscope::gui::DebugToolsUI* debug_tools) {
        debug_tools->begin_profile_sample("Frame Update");
        
        // Simulate main update work
        std::this_thread::sleep_for(std::chrono::microseconds(
            static_cast<int>(1000 + uniform_dist_(rng_) * 5000))); // 1-6ms
        
        debug_tools->end_profile_sample("Frame Update");
    }

    void simulate_rendering_system(ecscope::gui::DebugToolsUI* debug_tools) {
        debug_tools->begin_profile_sample("Rendering");
        
        // Simulate rendering work
        std::this_thread::sleep_for(std::chrono::microseconds(
            static_cast<int>(2000 + uniform_dist_(rng_) * 8000))); // 2-10ms
        
        uint32_t draw_calls = static_cast<uint32_t>(50 + uniform_dist_(rng_) * 200);
        uint32_t triangles = draw_calls * static_cast<uint32_t>(100 + uniform_dist_(rng_) * 500);
        
        debug_tools->record_custom_metric("Draw Calls", static_cast<float>(draw_calls));
        debug_tools->record_custom_metric("Triangles", static_cast<float>(triangles));
        
        debug_tools->end_profile_sample("Rendering");
    }

    void simulate_physics_system(ecscope::gui::DebugToolsUI* debug_tools) {
        debug_tools->begin_profile_sample("Physics");
        
        // Simulate physics calculations
        std::this_thread::sleep_for(std::chrono::microseconds(
            static_cast<int>(500 + uniform_dist_(rng_) * 2000))); // 0.5-2.5ms
        
        uint32_t active_bodies = static_cast<uint32_t>(total_entities_ * 0.3f + uniform_dist_(rng_) * total_entities_ * 0.2f);
        debug_tools->record_custom_metric("Active Physics Bodies", static_cast<float>(active_bodies));
        
        debug_tools->end_profile_sample("Physics");
    }

    void simulate_audio_system(ecscope::gui::DebugToolsUI* debug_tools) {
        debug_tools->begin_profile_sample("Audio");
        
        // Simulate audio processing
        std::this_thread::sleep_for(std::chrono::microseconds(
            static_cast<int>(200 + uniform_dist_(rng_) * 800))); // 0.2-1ms
        
        uint32_t active_sources = static_cast<uint32_t>(5 + uniform_dist_(rng_) * 20);
        debug_tools->record_custom_metric("Active Audio Sources", static_cast<float>(active_sources));
        
        debug_tools->end_profile_sample("Audio");
    }

    void simulate_ai_system(ecscope::gui::DebugToolsUI* debug_tools) {
        debug_tools->begin_profile_sample("AI System");
        
        // Simulate AI processing
        std::this_thread::sleep_for(std::chrono::microseconds(
            static_cast<int>(300 + uniform_dist_(rng_) * 1200))); // 0.3-1.5ms
        
        uint32_t ai_agents = static_cast<uint32_t>(total_entities_ * 0.1f);
        debug_tools->record_custom_metric("AI Agents", static_cast<float>(ai_agents));
        
        debug_tools->end_profile_sample("AI System");
    }

    void simulate_memory_operations(ecscope::gui::DebugToolsUI* debug_tools) {
        // Simulate memory allocation
        if (uniform_dist_(rng_) < 0.7f) {
            size_t allocation_size = static_cast<size_t>(1024 + uniform_dist_(rng_) * 10240); // 1KB-10KB
            void* fake_ptr = reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(0x1000000) + allocations_.size() * 8);
            
            allocations_.push_back({fake_ptr, allocation_size});
            
            std::vector<std::string> categories = {"Entities", "Textures", "Audio", "Scripts", "UI", "Network"};
            std::string category = categories[static_cast<size_t>(uniform_dist_(rng_) * categories.size())];
            
            debug_tools->track_memory_allocation(fake_ptr, allocation_size, category);
            
            debug_tools->log_debug("Memory", "Allocated " + std::to_string(allocation_size) + 
                                 " bytes for " + category);
        }
        
        // Simulate memory deallocation
        if (!allocations_.empty() && uniform_dist_(rng_) < 0.3f) {
            size_t index = static_cast<size_t>(uniform_dist_(rng_) * allocations_.size());
            auto alloc = allocations_[index];
            
            debug_tools->track_memory_deallocation(alloc.first);
            debug_tools->log_debug("Memory", "Deallocated " + std::to_string(alloc.second) + " bytes");
            
            allocations_.erase(allocations_.begin() + index);
        }
    }

    void generate_log_messages(ecscope::gui::DebugToolsUI* debug_tools) {
        // Generate various types of log messages
        if (frame_count_ % 120 == 0) { // Every 2 seconds at 60fps
            std::vector<std::string> info_messages = {
                "Player entered new area: Forest Level",
                "Quest completed: Find the Ancient Artifact",
                "Achievement unlocked: Master Explorer",
                "Save game created successfully",
                "Network connection established"
            };
            
            std::string message = info_messages[static_cast<size_t>(uniform_dist_(rng_) * info_messages.size())];
            debug_tools->log_info("Game", message);
        }
        
        // Generate occasional warnings
        if (uniform_dist_(rng_) < 0.005f) { // 0.5% chance per frame
            std::vector<std::string> warnings = {
                "Low memory warning: " + std::to_string(static_cast<int>(uniform_dist_(rng_) * 100)) + "MB remaining",
                "High CPU usage detected: " + std::to_string(static_cast<int>(80 + uniform_dist_(rng_) * 20)) + "%",
                "Network latency high: " + std::to_string(static_cast<int>(200 + uniform_dist_(rng_) * 300)) + "ms",
                "GPU memory usage high: " + std::to_string(static_cast<int>(80 + uniform_dist_(rng_) * 15)) + "%"
            };
            
            std::string warning = warnings[static_cast<size_t>(uniform_dist_(rng_) * warnings.size())];
            debug_tools->log_warning("Performance", warning);
        }
        
        // Generate rare errors
        if (uniform_dist_(rng_) < 0.001f) { // 0.1% chance per frame
            std::vector<std::string> errors = {
                "Failed to load texture: missing_texture.png",
                "Audio system error: Unable to create sound buffer",
                "Network error: Connection timeout",
                "Script error: Null reference exception in player_controller.lua"
            };
            
            std::string error = errors[static_cast<size_t>(uniform_dist_(rng_) * errors.size())];
            debug_tools->log_error("System", error);
        }
        
        // Generate debug messages
        if (frame_count_ % 300 == 0) { // Every 5 seconds
            debug_tools->log_debug("Debug", "Entity count: " + std::to_string(total_entities_) + 
                                 ", Active: " + std::to_string(static_cast<int>(total_entities_ * 0.6f)));
        }
    }

    void record_custom_metrics(ecscope::gui::DebugToolsUI* debug_tools) {
        // Record various game metrics
        float entity_utilization = 0.6f + uniform_dist_(rng_) * 0.3f; // 60-90%
        debug_tools->record_custom_metric("Entity Utilization", entity_utilization * 100.0f);
        
        float network_bandwidth = 1.0f + uniform_dist_(rng_) * 4.0f; // 1-5 Mbps
        debug_tools->record_custom_metric("Network Bandwidth", network_bandwidth);
        
        float disk_io = uniform_dist_(rng_) * 50.0f; // 0-50 MB/s
        debug_tools->record_custom_metric("Disk I/O", disk_io);
        
        float temperature = 45.0f + uniform_dist_(rng_) * 30.0f; // 45-75°C
        debug_tools->record_custom_metric("CPU Temperature", temperature);
    }

    void simulate_performance_spike(ecscope::gui::DebugToolsUI* debug_tools) {
        debug_tools->log_warning("Performance", "Performance spike detected - simulating heavy workload");
        
        debug_tools->begin_profile_sample("Performance Spike");
        
        // Simulate heavy processing
        std::this_thread::sleep_for(std::chrono::milliseconds(50 + static_cast<int>(uniform_dist_(rng_) * 100)));
        
        debug_tools->end_profile_sample("Performance Spike");
        
        // Record spike metrics
        debug_tools->record_custom_metric("Spike Intensity", 90.0f + uniform_dist_(rng_) * 10.0f);
    }

    void allocate_mock_memory() {
        // Simulate initial memory allocations for game systems
        std::vector<std::pair<std::string, size_t>> initial_allocations = {
            {"Entity Pool", 1024 * 1024 * 10},      // 10MB for entities
            {"Texture Cache", 1024 * 1024 * 50},    // 50MB for textures
            {"Audio Buffers", 1024 * 1024 * 20},    // 20MB for audio
            {"Script Runtime", 1024 * 1024 * 5},    // 5MB for scripts
            {"UI System", 1024 * 1024 * 3},         // 3MB for UI
            {"Network Buffers", 1024 * 1024 * 2}    // 2MB for networking
        };
        
        for (const auto& [category, size] : initial_allocations) {
            void* fake_ptr = reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(0x2000000) + allocations_.size() * 8);
            allocations_.push_back({fake_ptr, size});
        }
    }

    uint64_t frame_count_;
    uint32_t total_entities_;
    bool is_running_;
    std::vector<std::pair<void*, size_t>> allocations_;
    
    std::mt19937 rng_;
    std::uniform_real_distribution<float> uniform_dist_{0.0f, 1.0f};
};

int main() {
    std::cout << "═══════════════════════════════════════════════════════\n";
    std::cout << "  ECScope Debug Tools Demo\n";
    std::cout << "═══════════════════════════════════════════════════════\n\n";

#ifdef ECSCOPE_HAS_GUI
    try {
        // Initialize GUI manager
        auto gui_manager = std::make_unique<ecscope::gui::GuiManager>();
        if (!gui_manager->initialize("ECScope Debug Tools Demo", 1600, 1000)) {
            std::cerr << "Failed to initialize GUI manager\n";
            return 1;
        }

        // Initialize dashboard
        auto dashboard = std::make_unique<ecscope::gui::Dashboard>();
        if (!dashboard->initialize()) {
            std::cerr << "Failed to initialize dashboard\n";
            return 1;
        }

        // Initialize debug tools UI
        auto debug_tools = std::make_unique<ecscope::gui::DebugToolsUI>();
        if (!debug_tools->initialize()) {
            std::cerr << "Failed to initialize debug tools UI\n";
            return 1;
        }

        // Initialize mock game system
        auto game_system = std::make_unique<MockGameSystem>();
        game_system->initialize();

        // Set up debug tools callbacks
        debug_tools->set_performance_alert_callback([](const ecscope::gui::PerformanceAlert& alert) {
            std::cout << "Performance alert: " << alert.description << " (Current: " 
                     << alert.current_value << ", Threshold: " << alert.threshold << ")\n";
        });

        debug_tools->set_memory_leak_callback([](const std::vector<ecscope::gui::MemoryBlock>& leaks) {
            std::cout << "Memory leak detection found " << leaks.size() << " potential leaks\n";
        });

        // Configure performance thresholds
        debug_tools->set_performance_threshold(ecscope::gui::PerformanceMetric::FrameTime, 20.0f); // 20ms = 50fps
        debug_tools->set_performance_threshold(ecscope::gui::PerformanceMetric::CPUUsage, 80.0f);   // 80%
        debug_tools->set_memory_alert_threshold(100 * 1024 * 1024); // 100MB

        // Enable profiling modes
        debug_tools->set_profiler_enabled(ecscope::gui::ProfilerMode::CPU, true);
        debug_tools->set_profiler_enabled(ecscope::gui::ProfilerMode::Memory, true);

        std::cout << "Debug Tools Demo Features:\n";
        std::cout << "• Performance Profiler: Real-time CPU, memory, and custom metric tracking\n";
        std::cout << "• Memory Profiler: Allocation tracking and leak detection\n";
        std::cout << "• Debug Console: Logging system with filtering and command execution\n";
        std::cout << "• Performance Monitor: System metrics and real-time graphs\n";
        std::cout << "• Call Stack Tracer: Function call stack capture and analysis\n";
        std::cout << "• Alerts System: Performance threshold monitoring and notifications\n";
        std::cout << "• Press SPACE to start/stop game simulation\n";
        std::cout << "• Close window to exit\n\n";

        debug_tools->log_info("System", "Debug Tools Demo initialized successfully");

        // Main loop
        auto last_time = std::chrono::high_resolution_clock::now();
        
        while (!gui_manager->should_close()) {
            auto current_time = std::chrono::high_resolution_clock::now();
            float delta_time = std::chrono::duration<float>(current_time - last_time).count();
            last_time = current_time;

            // Handle input for simulation control
            // Note: In a real implementation, you'd handle keyboard input properly
            static bool space_pressed_last_frame = false;
            static bool simulation_running = false;
            
            // Simulate space key press every 10 seconds to start/stop simulation
            static auto last_toggle = std::chrono::steady_clock::now();
            auto now = std::chrono::steady_clock::now();
            if (std::chrono::duration_cast<std::chrono::seconds>(now - last_toggle).count() > 10) {
                simulation_running = !simulation_running;
                if (simulation_running) {
                    game_system->start_simulation();
                } else {
                    game_system->stop_simulation();
                }
                last_toggle = now;
            }

            // Update systems
            game_system->update(delta_time, debug_tools.get());
            debug_tools->update(delta_time);

            // Render frame
            gui_manager->begin_frame();
            
            // Render dashboard with debug tools as a feature
            dashboard->add_feature("Debug Tools", "Comprehensive debugging and profiling interface", [&]() {
                debug_tools->render();
            }, true);
            
            dashboard->render();

            gui_manager->end_frame();
            
            // Small delay to prevent excessive CPU usage
            std::this_thread::sleep_for(std::chrono::milliseconds(16)); // ~60 FPS
        }

        // Cleanup
        debug_tools->shutdown();
        dashboard->shutdown();
        gui_manager->shutdown();

        std::cout << "Debug Tools Demo completed successfully!\n";
        return 0;

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
#else
    std::cout << "❌ GUI system not available\n";
    std::cout << "This demo requires GLFW, OpenGL, and Dear ImGui\n";
    std::cout << "Please build with -DECSCOPE_BUILD_GUI=ON\n";
    return 1;
#endif
}