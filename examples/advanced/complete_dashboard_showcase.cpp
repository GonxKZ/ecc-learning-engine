/**
 * @file complete_dashboard_showcase.cpp
 * @brief Complete ECScope Dashboard Showcase
 * 
 * Comprehensive demonstration showcasing all ECScope dashboard features:
 * - Professional UI/UX design
 * - Complete feature integration
 * - System monitoring
 * - Performance visualization
 * - Workspace management
 * - Theme system
 * 
 * @author ECScope Development Team
 * @version 1.0.0
 */

#include <iostream>
#include <memory>
#include <chrono>
#include <thread>
#include <random>
#include <vector>
#include <string>

// ECScope core systems
#include "../../include/ecscope/core/log.hpp"
#include "../../include/ecscope/core/time.hpp"
#include "../../include/ecscope/gui/gui_manager.hpp"
#include "../../include/ecscope/gui/dashboard.hpp"

// Optional rendering integration
#ifdef ECSCOPE_ENABLE_MODERN_RENDERING
#include "../../include/ecscope/rendering/renderer.hpp"
#endif

namespace ecscope::examples {

/**
 * @brief Custom GUI component for demonstration
 */
class DemoInspectorComponent : public gui::SimpleGUIComponent {
public:
    DemoInspectorComponent() : gui::SimpleGUIComponent("Demo Inspector") {}
    
    void render() override {
        if (!enabled_) return;
        
#ifdef ECSCOPE_HAS_IMGUI
        ImGui::Begin("Demo Inspector", &enabled_);
        
        ImGui::Text("ECScope Dashboard Showcase");
        ImGui::Separator();
        
        // Demo controls
        if (ImGui::CollapsingHeader("Demo Controls")) {
            if (ImGui::Button("Simulate Load")) {
                simulate_load_ = !simulate_load_;
            }
            ImGui::SameLine();
            ImGui::Text("Status: %s", simulate_load_ ? "Running" : "Idle");
            
            ImGui::SliderFloat("CPU Load", &simulated_cpu_load_, 0.0f, 100.0f, "%.1f%%");
            ImGui::SliderFloat("Memory Usage", &simulated_memory_usage_, 0.0f, 2048.0f, "%.0f MB");
            
            if (ImGui::Button("Toggle System Health")) {
                healthy_systems_ = !healthy_systems_;
            }
        }
        
        // Live stats
        if (ImGui::CollapsingHeader("Live Statistics", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::Text("Frame Rate: %.1f FPS", ImGui::GetIO().Framerate);
            ImGui::Text("Frame Time: %.3f ms", 1000.0f / ImGui::GetIO().Framerate);
            
            // Create some fake data for visualization
            static std::vector<float> frame_times;
            if (frame_times.size() > 100) {
                frame_times.erase(frame_times.begin());
            }
            frame_times.push_back(1000.0f / ImGui::GetIO().Framerate);
            
            ImGui::PlotLines("Frame Times", frame_times.data(), (int)frame_times.size(), 
                           0, nullptr, 0.0f, 50.0f, ImVec2(0, 80));
        }
        
        // Feature showcase
        if (ImGui::CollapsingHeader("Feature Showcase")) {
            ImGui::Text("All 18 ECScope engine systems are showcased:");
            
            const char* systems[] = {
                "ECS Architecture", "Memory Management", "Modern Rendering",
                "Shader System", "Physics Engine", "Audio System",
                "Networking", "Asset Pipeline", "Performance Profiler",
                "Visual Debugger", "Plugin System", "Scene Management",
                "Input System", "Threading", "Serialization",
                "Resource Management", "Math Library", "Utility Systems"
            };
            
            for (int i = 0; i < 18; ++i) {
                bool enabled = true;
                ImGui::Checkbox(systems[i], &enabled);
                if (i % 3 == 2) ImGui::Separator();
            }
        }
        
        ImGui::End();
#endif
    }
    
    void update(float delta_time) override {
        update_time_ += delta_time;
        
        // Update simulated data
        if (simulate_load_) {
            simulated_cpu_load_ = 30.0f + 20.0f * std::sin(update_time_ * 2.0f);
            simulated_memory_usage_ = 800.0f + 200.0f * std::cos(update_time_ * 1.5f);
        }
    }
    
    float get_simulated_cpu_load() const { return simulated_cpu_load_; }
    float get_simulated_memory_usage() const { return simulated_memory_usage_; }
    bool are_systems_healthy() const { return healthy_systems_; }
    
private:
    float update_time_ = 0.0f;
    bool simulate_load_ = false;
    float simulated_cpu_load_ = 10.0f;
    float simulated_memory_usage_ = 500.0f;
    bool healthy_systems_ = true;
};

/**
 * @brief Complete dashboard showcase application
 */
class CompleteDashboardShowcase {
public:
    CompleteDashboardShowcase() = default;
    ~CompleteDashboardShowcase() = default;

    bool initialize() {
        ecscope::core::Log::info("=== ECScope Complete Dashboard Showcase ===");
        ecscope::core::Log::info("Initializing comprehensive dashboard demonstration...");
        
        // Configure window for professional presentation
        gui::WindowConfig window_config;
        window_config.title = "ECScope Dashboard Showcase - Professional Engine Interface";
        window_config.width = 1920;
        window_config.height = 1080;
        window_config.resizable = true;
        window_config.vsync = true;
        window_config.samples = 4; // 4x MSAA for smooth UI
        
        // Configure GUI with all professional features enabled
        gui::GUIFlags gui_flags = 
            gui::GUIFlags::EnableDocking | 
            gui::GUIFlags::EnableViewports | 
            gui::GUIFlags::EnableKeyboardNav | 
            gui::GUIFlags::DarkTheme |
            gui::GUIFlags::HighDPI;
        
        // Initialize global GUI manager
        if (!gui::InitializeGlobalGUI(window_config, gui_flags)) {
            ecscope::core::Log::error("Failed to initialize GUI system");
            return false;
        }
        
        gui_manager_ = gui::GetGUIManager();
        if (!gui_manager_) {
            ecscope::core::Log::error("Failed to get GUI manager instance");
            return false;
        }
        
        // Get dashboard instance
        dashboard_ = gui_manager_->get_dashboard();
        if (!dashboard_) {
            ecscope::core::Log::error("Failed to get dashboard instance");
            return false;
        }
        
        // Setup comprehensive demonstration
        setup_professional_features();
        setup_system_monitoring();
        setup_demo_components();
        
        ecscope::core::Log::info("Dashboard showcase initialized successfully!");
        ecscope::core::Log::info("Features demonstrated:");
        ecscope::core::Log::info("  ✓ Professional UI/UX design with modern theming");
        ecscope::core::Log::info("  ✓ Feature gallery with 18+ engine systems");
        ecscope::core::Log::info("  ✓ Real-time system monitoring and health checks");
        ecscope::core::Log::info("  ✓ Performance visualization and metrics");
        ecscope::core::Log::info("  ✓ Flexible docking and workspace management");
        ecscope::core::Log::info("  ✓ Navigation, search, and accessibility features");
        ecscope::core::Log::info("  ✓ Integration with core engine systems");
        
        return true;
    }

    void run() {
        if (!gui_manager_ || !dashboard_) {
            ecscope::core::Log::error("Dashboard showcase not properly initialized");
            return;
        }

        ecscope::core::Log::info("Starting interactive dashboard showcase...");
        ecscope::core::Log::info("Controls:");
        ecscope::core::Log::info("  F1  - Toggle dashboard visibility");
        ecscope::core::Log::info("  F11 - Toggle fullscreen mode");
        ecscope::core::Log::info("  ESC - Exit application");
        
        auto last_time = std::chrono::high_resolution_clock::now();
        
        // Main application loop
        while (!gui_manager_->should_close()) {
            // Calculate delta time
            auto current_time = std::chrono::high_resolution_clock::now();
            float delta_time = std::chrono::duration<float>(current_time - last_time).count();
            last_time = current_time;
            
            // Poll events
            gui_manager_->poll_events();
            
            // Update systems
            update_showcase_systems(delta_time);
            
            // Render frame
            {
                gui::ScopedGUIFrame frame(gui_manager_);
                
                // Update and render GUI
                gui_manager_->update(delta_time);
                
                // Show welcome message on first frame
                if (show_welcome_) {
                    show_welcome_dialog();
                }
            }
            
            // Maintain target frame rate
            std::this_thread::sleep_for(std::chrono::microseconds(16667)); // ~60 FPS
        }
        
        ecscope::core::Log::info("Dashboard showcase completed successfully!");
    }

    void shutdown() {
        ecscope::core::Log::info("Shutting down dashboard showcase...");
        
        // Shutdown global GUI system
        gui::ShutdownGlobalGUI();
        
        ecscope::core::Log::info("Dashboard showcase shut down successfully");
    }

private:
    void setup_professional_features() {
        if (!dashboard_) return;
        
        // Create comprehensive feature set showcasing all ECScope capabilities
        std::vector<gui::FeatureInfo> professional_features;
        
        // Core Architecture Features
        professional_features.push_back({
            "ecs_advanced", "Advanced ECS Architecture",
            "High-performance Entity-Component-System with archetype storage, dependency injection, and parallel execution scheduling.",
            "", gui::FeatureCategory::Core, true, true,
            [this]() { launch_ecs_demo(); },
            [this]() { return demo_inspector_->are_systems_healthy(); },
            {}, "2.0.0", "https://docs.ecscope.engine/ecs"
        });
        
        professional_features.push_back({
            "memory_pool", "Memory Pool Management",
            "Advanced memory allocators with pool management, leak detection, and real-time memory tracking with fragmentation analysis.",
            "", gui::FeatureCategory::Core, true, false,
            [this]() { launch_memory_profiler(); },
            []() { return true; },
            {"ecs_advanced"}, "2.0.0", ""
        });
        
        // Rendering Engine Features
        professional_features.push_back({
            "vulkan_rendering", "Vulkan Rendering Engine",
            "Modern Vulkan-based rendering pipeline with deferred rendering, PBR materials, and advanced lighting techniques.",
            "", gui::FeatureCategory::Rendering, true, true,
            [this]() { launch_vulkan_demo(); },
            []() { return true; },
            {"shader_compiler"}, "2.0.0", ""
        });
        
        professional_features.push_back({
            "shader_compiler", "Real-time Shader Compiler",
            "Hot-reloadable shader compilation system with SPIR-V optimization and cross-platform shader variants.",
            "", gui::FeatureCategory::Rendering, true, false,
            [this]() { launch_shader_editor(); },
            []() { return true; },
            {}, "2.0.0", ""
        });
        
        professional_features.push_back({
            "deferred_renderer", "Deferred Rendering Pipeline",
            "Multi-pass deferred rendering with G-buffer optimization, light culling, and screen-space techniques.",
            "", gui::FeatureCategory::Rendering, true, false,
            []() { /* Launch deferred renderer */ },
            []() { return true; },
            {"vulkan_rendering"}, "2.0.0", ""
        });
        
        // Physics Simulation
        professional_features.push_back({
            "physics_3d", "3D Physics Simulation",
            "High-performance 3D physics engine with broadphase collision detection, constraint solving, and fluid dynamics.",
            "", gui::FeatureCategory::Physics, true, true,
            [this]() { launch_physics_demo(); },
            [this]() { return demo_inspector_->are_systems_healthy(); },
            {}, "2.0.0", ""
        });
        
        // Audio Systems
        professional_features.push_back({
            "spatial_audio", "3D Spatial Audio System",
            "Real-time 3D audio processing with HRTF, reverb zones, and multi-channel output support.",
            "", gui::FeatureCategory::Audio, true, false,
            []() { /* Launch audio demo */ },
            []() { return true; },
            {}, "2.0.0", ""
        });
        
        // Networking
        professional_features.push_back({
            "multiplayer_net", "Multiplayer Networking",
            "High-performance networking stack with prediction, rollback, and anti-cheat integration.",
            "", gui::FeatureCategory::Networking, true, false,
            []() { /* Launch network demo */ },
            []() { return true; },
            {}, "2.0.0", ""
        });
        
        // Development Tools
        professional_features.push_back({
            "asset_processor", "Asset Processing Pipeline",
            "Automated asset pipeline with hot-reloading, texture compression, and mesh optimization.",
            "", gui::FeatureCategory::Tools, true, false,
            []() { /* Launch asset processor */ },
            []() { return true; },
            {}, "2.0.0", ""
        });
        
        professional_features.push_back({
            "scene_editor", "Visual Scene Editor",
            "Comprehensive scene editor with component editing, prefab system, and real-time preview.",
            "", gui::FeatureCategory::Tools, true, true,
            [this]() { launch_scene_editor(); },
            []() { return true; },
            {"ecs_advanced", "vulkan_rendering"}, "2.0.0", ""
        });
        
        // Performance & Debugging
        professional_features.push_back({
            "gpu_profiler", "GPU Performance Profiler",
            "Real-time GPU profiling with draw call analysis, memory bandwidth monitoring, and bottleneck detection.",
            "", gui::FeatureCategory::Performance, true, true,
            [this]() { launch_gpu_profiler(); },
            []() { return true; },
            {"vulkan_rendering"}, "2.0.0", ""
        });
        
        professional_features.push_back({
            "cpu_profiler", "CPU Performance Profiler",
            "Hierarchical CPU profiling with timing analysis, cache miss detection, and thread visualization.",
            "", gui::FeatureCategory::Performance, true, false,
            []() { /* Launch CPU profiler */ },
            []() { return true; },
            {}, "2.0.0", ""
        });
        
        professional_features.push_back({
            "visual_debugger", "Integrated Visual Debugger",
            "Visual debugging tools with breakpoint management, variable inspection, and call stack analysis.",
            "", gui::FeatureCategory::Debugging, true, false,
            []() { /* Launch debugger */ },
            []() { return true; },
            {}, "2.0.0", ""
        });
        
        professional_features.push_back({
            "memory_analyzer", "Memory Analysis Tools",
            "Advanced memory debugging with leak detection, allocation tracking, and heap visualization.",
            "", gui::FeatureCategory::Debugging, true, false,
            []() { /* Launch memory analyzer */ },
            []() { return true; },
            {"memory_pool"}, "2.0.0", ""
        });
        
        // Register all features with the dashboard
        for (const auto& feature : professional_features) {
            dashboard_->register_feature(feature);
        }
        
        ecscope::core::Log::info("Registered {} professional features", professional_features.size());
    }
    
    void setup_system_monitoring() {
        if (!dashboard_) return;
        
        // Register comprehensive system monitors
        dashboard_->register_system_monitor("ECS Core", [this]() {
            gui::SystemStatus status;
            status.name = "ECS Core";
            status.healthy = demo_inspector_->are_systems_healthy();
            status.cpu_usage = get_random_float(8.0f, 15.0f);
            status.memory_usage = static_cast<size_t>(get_random_float(80.0f, 120.0f)) * 1024 * 1024;
            status.status_message = status.healthy ? "Optimal performance" : "Performance degraded";
            return status;
        });
        
        dashboard_->register_system_monitor("Vulkan Renderer", [this]() {
            gui::SystemStatus status;
            status.name = "Vulkan Renderer";
            status.healthy = demo_inspector_->are_systems_healthy();
            status.cpu_usage = get_random_float(15.0f, 30.0f);
            status.memory_usage = static_cast<size_t>(get_random_float(300.0f, 500.0f)) * 1024 * 1024;
            status.status_message = status.healthy ? "GPU optimal" : "High GPU usage";
            return status;
        });
        
        dashboard_->register_system_monitor("Physics Engine", [this]() {
            gui::SystemStatus status;
            status.name = "Physics Engine";
            status.healthy = demo_inspector_->are_systems_healthy();
            status.cpu_usage = demo_inspector_->get_simulated_cpu_load();
            status.memory_usage = static_cast<size_t>(demo_inspector_->get_simulated_memory_usage()) * 1024 * 1024;
            status.status_message = status.healthy ? "Simulation stable" : "High complexity detected";
            return status;
        });
        
        dashboard_->register_system_monitor("Audio System", [this]() {
            gui::SystemStatus status;
            status.name = "Audio System";
            status.healthy = true; // Audio is always stable in demo
            status.cpu_usage = get_random_float(3.0f, 8.0f);
            status.memory_usage = static_cast<size_t>(get_random_float(40.0f, 80.0f)) * 1024 * 1024;
            status.status_message = "3D audio pipeline active";
            return status;
        });
        
        dashboard_->register_system_monitor("Networking", [this]() {
            gui::SystemStatus status;
            status.name = "Networking";
            status.healthy = network_connected_;
            status.cpu_usage = get_random_float(2.0f, 6.0f);
            status.memory_usage = static_cast<size_t>(get_random_float(20.0f, 40.0f)) * 1024 * 1024;
            status.status_message = network_connected_ ? "Connected (4 players)" : "Offline mode";
            return status;
        });
        
        dashboard_->register_system_monitor("Asset Pipeline", [this]() {
            gui::SystemStatus status;
            status.name = "Asset Pipeline";
            status.healthy = true;
            status.cpu_usage = get_random_float(1.0f, 5.0f);
            status.memory_usage = static_cast<size_t>(get_random_float(30.0f, 60.0f)) * 1024 * 1024;
            status.status_message = "Hot-reload enabled";
            return status;
        });
        
        ecscope::core::Log::info("System monitoring configured for 6 core systems");
    }
    
    void setup_demo_components() {
        if (!gui_manager_) return;
        
        // Create and register demo inspector component
        demo_inspector_ = std::make_unique<DemoInspectorComponent>();
        gui_manager_->register_component(std::unique_ptr<gui::GUIComponent>(demo_inspector_.release()));
        
        // Apply professional overview workspace
        dashboard_->apply_workspace_preset(gui::WorkspacePreset::Overview);
        
        ecscope::core::Log::info("Demo components initialized");
    }
    
    void update_showcase_systems(float delta_time) {
        // Update performance metrics with realistic professional data
        gui::PerformanceMetrics metrics;
        metrics.frame_rate = get_random_float(58.0f, 62.0f); // Stable 60 FPS
        metrics.frame_time_ms = 1000.0f / metrics.frame_rate;
        metrics.cpu_usage = get_random_float(35.0f, 55.0f); // Professional workload
        metrics.memory_usage = static_cast<size_t>(get_random_float(1200.0f, 1800.0f)) * 1024 * 1024; // 1.2-1.8 GB
        metrics.gpu_memory_usage = static_cast<size_t>(get_random_float(800.0f, 1200.0f)) * 1024 * 1024; // 800MB-1.2GB
        metrics.draw_calls = static_cast<uint32_t>(get_random_float(300, 600)); // High detail scene
        metrics.vertices_rendered = static_cast<uint32_t>(get_random_float(100000, 300000)); // Complex geometry
        metrics.timestamp = std::chrono::steady_clock::now();
        
        dashboard_->update_performance_metrics(metrics);
        
        // Simulate occasional system state changes for demonstration
        static float state_timer = 0.0f;
        state_timer += delta_time;
        
        if (state_timer >= 20.0f) { // Change every 20 seconds
            state_timer = 0.0f;
            network_connected_ = !network_connected_;
            
            if (network_connected_) {
                ecscope::core::Log::info("Demo: Network connected - multiplayer session active");
            } else {
                ecscope::core::Log::info("Demo: Network disconnected - switching to offline mode");
            }
        }
    }
    
    void show_welcome_dialog() {
#ifdef ECSCOPE_HAS_IMGUI
        ImGui::OpenPopup("Welcome to ECScope Dashboard");
        
        ImVec2 center = ImGui::GetMainViewport()->GetCenter();
        ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
        ImGui::SetNextWindowSize(ImVec2(600, 400), ImGuiCond_Appearing);
        
        if (ImGui::BeginPopupModal("Welcome to ECScope Dashboard", &show_welcome_, 
                                  ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoResize)) {
            
            // Header with professional styling
            ImGui::PushFont(ImGui::GetFont()); // Use current font, could be larger if available
            ImGui::TextColored(ImVec4(0.4f, 0.7f, 1.0f, 1.0f), "ECScope Professional Dashboard Showcase");
            ImGui::PopFont();
            
            ImGui::Separator();
            ImGui::Spacing();
            
            ImGui::TextWrapped(
                "Welcome to the comprehensive ECScope Dashboard demonstration! This showcase "
                "presents a professional game engine interface with all modern features:"
            );
            
            ImGui::Spacing();
            ImGui::BulletText("18+ integrated engine systems with live monitoring");
            ImGui::BulletText("Real-time performance metrics and visualization");
            ImGui::BulletText("Professional UI/UX with flexible docking system");
            ImGui::BulletText("Advanced theming and workspace management");
            ImGui::BulletText("Feature gallery with comprehensive system showcase");
            ImGui::BulletText("System health monitoring with diagnostic tools");
            
            ImGui::Spacing();
            ImGui::TextWrapped(
                "Explore the different panels, try the workspace presets from the View menu, "
                "and interact with the feature gallery. The Demo Inspector panel provides "
                "controls for simulating various engine states."
            );
            
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();
            
            // Professional button layout
            float button_width = 120.0f;
            float spacing = ImGui::GetStyle().ItemSpacing.x;
            float total_width = button_width * 3 + spacing * 2;
            float start_x = (ImGui::GetWindowWidth() - total_width) * 0.5f;
            
            ImGui::SetCursorPosX(start_x);
            
            if (ImGui::Button("Start Tour", ImVec2(button_width, 0))) {
                start_guided_tour();
                show_welcome_ = false;
            }
            
            ImGui::SameLine();
            
            if (ImGui::Button("Explore Freely", ImVec2(button_width, 0))) {
                show_welcome_ = false;
            }
            
            ImGui::SameLine();
            
            if (ImGui::Button("Show Help", ImVec2(button_width, 0))) {
                show_help_dialog();
            }
            
            ImGui::EndPopup();
        }
#endif
    }
    
    // Feature launch callbacks
    void launch_ecs_demo() {
        ecscope::core::Log::info("Launching ECS Architecture Demo");
        gui_manager_->show_message_dialog("ECS Demo", 
            "ECS Architecture demonstration would launch here.\n\n"
            "Features:\n"
            "• Archetype-based storage\n"
            "• Parallel system execution\n"
            "• Advanced query engine\n"
            "• Component reflection", "info");
    }
    
    void launch_vulkan_demo() {
        ecscope::core::Log::info("Launching Vulkan Rendering Demo");
        dashboard_->navigate_to_panel(gui::PanelType::Viewport);
        gui_manager_->show_message_dialog("Vulkan Renderer", 
            "Vulkan rendering demo activated!\n\n"
            "The 3D viewport now shows the modern rendering pipeline.", "info");
    }
    
    void launch_physics_demo() {
        ecscope::core::Log::info("Launching Physics Simulation Demo");
        gui_manager_->show_message_dialog("Physics Engine", 
            "Physics simulation demo starting...\n\n"
            "Watch the system monitor for physics load changes.", "info");
    }
    
    void launch_memory_profiler() {
        ecscope::core::Log::info("Launching Memory Profiler");
        dashboard_->show_panel(gui::PanelType::Performance, true);
        dashboard_->navigate_to_panel(gui::PanelType::Performance);
    }
    
    void launch_shader_editor() {
        ecscope::core::Log::info("Launching Shader Editor");
        dashboard_->show_panel(gui::PanelType::Tools, true);
        dashboard_->navigate_to_panel(gui::PanelType::Tools);
    }
    
    void launch_scene_editor() {
        ecscope::core::Log::info("Launching Scene Editor");
        dashboard_->apply_workspace_preset(gui::WorkspacePreset::ContentCreation);
    }
    
    void launch_gpu_profiler() {
        ecscope::core::Log::info("Launching GPU Profiler");
        dashboard_->apply_workspace_preset(gui::WorkspacePreset::Performance);
    }
    
    void start_guided_tour() {
        ecscope::core::Log::info("Starting guided dashboard tour");
        // TODO: Implement guided tour system
    }
    
    void show_help_dialog() {
        gui_manager_->show_message_dialog("Dashboard Help", 
            "ECScope Dashboard Controls:\n\n"
            "F1  - Toggle dashboard visibility\n"
            "F11 - Toggle fullscreen mode\n"
            "Ctrl+S - Save current layout\n"
            "Ctrl+L - Load saved layout\n\n"
            "Use the View menu to switch workspaces and themes.", "info");
    }
    
    float get_random_float(float min, float max) {
        static std::random_device rd;
        static std::mt19937 gen(rd());
        std::uniform_real_distribution<float> dist(min, max);
        return dist(gen);
    }

    // Core objects
    gui::GUIManager* gui_manager_ = nullptr;
    gui::Dashboard* dashboard_ = nullptr;
    
    // Demo components
    std::unique_ptr<DemoInspectorComponent> demo_inspector_;
    
    // Demo state
    bool show_welcome_ = true;
    bool network_connected_ = true;
};

} // namespace ecscope::examples

int main() {
    try {
        ecscope::core::Log::info("Starting ECScope Complete Dashboard Showcase");
        
        ecscope::examples::CompleteDashboardShowcase showcase;
        
        if (showcase.initialize()) {
            showcase.run();
        } else {
            ecscope::core::Log::error("Failed to initialize dashboard showcase");
            return 1;
        }
        
        showcase.shutdown();
        
        ecscope::core::Log::info("ECScope Dashboard Showcase completed successfully");
        
    } catch (const std::exception& e) {
        ecscope::core::Log::error("Dashboard showcase failed with exception: {}", e.what());
        return 1;
    }
    
    return 0;
}