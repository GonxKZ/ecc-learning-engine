/**
 * @file dashboard_demo.cpp
 * @brief ECScope Dashboard Integration Demo
 * 
 * Comprehensive demonstration of the ECScope dashboard with full
 * Dear ImGui integration, featuring all panels and functionality.
 * 
 * @author ECScope Development Team
 * @version 1.0.0
 */

#include <iostream>
#include <memory>
#include <chrono>
#include <thread>
#include <random>

// ECScope core systems
#include "../../include/ecscope/core/log.hpp"
#include "../../include/ecscope/core/time.hpp"
#include "../../include/ecscope/gui/dashboard.hpp"

#ifdef ECSCOPE_HAS_IMGUI
#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>
#endif

#ifdef ECSCOPE_HAS_GLFW
#include <GLFW/glfw3.h>
#endif

#ifdef ECSCOPE_HAS_OPENGL
#include <GL/gl3w.h>
#endif

namespace ecscope::examples {

/**
 * @brief Dashboard demonstration application
 */
class DashboardDemo {
public:
    DashboardDemo() = default;
    ~DashboardDemo() = default;

    bool initialize() {
        ecscope::core::Log::info("=== ECScope Dashboard Demo ===");
        
#ifndef ECSCOPE_HAS_GLFW
        ecscope::core::Log::error("GLFW not available in build configuration");
        return false;
#endif

#ifndef ECSCOPE_HAS_IMGUI
        ecscope::core::Log::error("ImGui not available in build configuration");
        return false;
#endif

#ifdef ECSCOPE_HAS_GLFW
        // Initialize GLFW
        if (!glfwInit()) {
            ecscope::core::Log::error("Failed to initialize GLFW");
            return false;
        }

        // Create window with graphics context
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
        
        window_ = glfwCreateWindow(1920, 1080, "ECScope Dashboard Demo", nullptr, nullptr);
        if (!window_) {
            ecscope::core::Log::error("Failed to create GLFW window");
            glfwTerminate();
            return false;
        }
        
        glfwMakeContextCurrent(window_);
        glfwSwapInterval(1); // Enable vsync
        
#ifdef ECSCOPE_HAS_OPENGL
        // Initialize OpenGL loader
        if (gl3wInit() != 0) {
            ecscope::core::Log::error("Failed to initialize OpenGL loader");
            return false;
        }
#endif

        // Setup Dear ImGui context
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;         // Enable Docking
        io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;       // Enable Multi-Viewport / Platform Windows

        // Setup Dear ImGui style
        ImGui::StyleColorsDark();

        // When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look identical to regular ones.
        ImGuiStyle& style = ImGui::GetStyle();
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
            style.WindowRounding = 0.0f;
            style.Colors[ImGuiCol_WindowBg].w = 1.0f;
        }

        // Setup Platform/Renderer backends
        ImGui_ImplGlfw_InitForOpenGL(window_, true);
        ImGui_ImplOpenGL3_Init("#version 330");

        // Initialize dashboard
        dashboard_ = std::make_unique<gui::Dashboard>();
        if (!dashboard_->initialize()) {
            ecscope::core::Log::error("Failed to initialize dashboard");
            return false;
        }

        // Setup demo systems and features
        setup_demo_systems();
        setup_demo_features();
        
        ecscope::core::Log::info("Dashboard demo initialized successfully!");
        return true;
        
#endif // ECSCOPE_HAS_GLFW
        return false;
    }

    void run() {
        if (!dashboard_) {
            ecscope::core::Log::error("Dashboard not properly initialized");
            return;
        }

        ecscope::core::Log::info("Running dashboard demo...");
        
#ifdef ECSCOPE_HAS_GLFW
        auto last_time = std::chrono::high_resolution_clock::now();
        
        while (!glfwWindowShouldClose(window_)) {
            glfwPollEvents();

            // Calculate delta time
            auto current_time = std::chrono::high_resolution_clock::now();
            float delta_time = std::chrono::duration<float>(current_time - last_time).count();
            last_time = current_time;

            // Update demo systems
            update_demo_systems(delta_time);

            // Start the Dear ImGui frame
            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();

            // Render dashboard
            dashboard_->render();

            // Rendering
            ImGui::Render();
            int display_w, display_h;
            glfwGetFramebufferSize(window_, &display_w, &display_h);
            glViewport(0, 0, display_w, display_h);
            glClearColor(0.10f, 0.10f, 0.10f, 1.00f);
            glClear(GL_COLOR_BUFFER_BIT);
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

            // Update and Render additional Platform Windows
            ImGuiIO& io = ImGui::GetIO();
            if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
                GLFWwindow* backup_current_context = glfwGetCurrentContext();
                ImGui::UpdatePlatformWindows();
                ImGui::RenderPlatformWindowsDefault();
                glfwMakeContextCurrent(backup_current_context);
            }

            glfwSwapBuffers(window_);
        }
#endif
    }

    void shutdown() {
        ecscope::core::Log::info("Shutting down dashboard demo...");

        if (dashboard_) {
            dashboard_->shutdown();
            dashboard_.reset();
        }

#ifdef ECSCOPE_HAS_IMGUI
        // Cleanup ImGui
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
#endif

#ifdef ECSCOPE_HAS_GLFW
        if (window_) {
            glfwDestroyWindow(window_);
            window_ = nullptr;
        }
        glfwTerminate();
#endif

        ecscope::core::Log::info("Dashboard demo shut down successfully");
    }

private:
    void setup_demo_systems() {
        if (!dashboard_) return;

        // Register system monitors
        dashboard_->register_system_monitor("ECS Core", [this]() {
            gui::SystemStatus status;
            status.name = "ECS Core";
            status.healthy = true;
            status.cpu_usage = get_random_float(5.0f, 15.0f);
            status.memory_usage = static_cast<size_t>(get_random_float(50.0f, 100.0f)) * 1024 * 1024; // 50-100 MB
            status.status_message = "All systems operational";
            return status;
        });

        dashboard_->register_system_monitor("Rendering", [this]() {
            gui::SystemStatus status;
            status.name = "Rendering";
            status.healthy = render_system_healthy_;
            status.cpu_usage = get_random_float(10.0f, 25.0f);
            status.memory_usage = static_cast<size_t>(get_random_float(200.0f, 400.0f)) * 1024 * 1024; // 200-400 MB
            status.status_message = render_system_healthy_ ? "GPU optimal" : "High GPU usage detected";
            return status;
        });

        dashboard_->register_system_monitor("Physics", [this]() {
            gui::SystemStatus status;
            status.name = "Physics";
            status.healthy = physics_system_healthy_;
            status.cpu_usage = get_random_float(8.0f, 20.0f);
            status.memory_usage = static_cast<size_t>(get_random_float(30.0f, 80.0f)) * 1024 * 1024; // 30-80 MB
            status.status_message = physics_system_healthy_ ? "Simulation stable" : "Performance degraded";
            return status;
        });

        dashboard_->register_system_monitor("Audio", [this]() {
            gui::SystemStatus status;
            status.name = "Audio";
            status.healthy = true;
            status.cpu_usage = get_random_float(2.0f, 8.0f);
            status.memory_usage = static_cast<size_t>(get_random_float(20.0f, 50.0f)) * 1024 * 1024; // 20-50 MB
            status.status_message = "Audio pipeline active";
            return status;
        });

        dashboard_->register_system_monitor("Networking", [this]() {
            gui::SystemStatus status;
            status.name = "Networking";
            status.healthy = network_connected_;
            status.cpu_usage = get_random_float(1.0f, 5.0f);
            status.memory_usage = static_cast<size_t>(get_random_float(10.0f, 30.0f)) * 1024 * 1024; // 10-30 MB
            status.status_message = network_connected_ ? "Connected" : "Disconnected";
            return status;
        });
    }

    void setup_demo_features() {
        if (!dashboard_) return;

        // Add some additional demo-specific features
        gui::FeatureInfo demo_feature;
        demo_feature.id = "dashboard_demo";
        demo_feature.name = "Dashboard Demo";
        demo_feature.description = "Interactive demonstration of the ECScope dashboard system with all features enabled.";
        demo_feature.category = gui::FeatureCategory::Tools;
        demo_feature.enabled = true;
        demo_feature.favorite = true;
        demo_feature.version = "1.0.0";
        demo_feature.launch_callback = [this]() {
            ecscope::core::Log::info("Dashboard Demo: Launching interactive demo");
            show_demo_dialog_ = true;
        };
        demo_feature.status_callback = []() { return true; };

        dashboard_->register_feature(demo_feature);

        // Add stress test feature
        gui::FeatureInfo stress_test;
        stress_test.id = "stress_test";
        stress_test.name = "System Stress Test";
        stress_test.description = "Stress test all engine systems to validate performance and stability under load.";
        stress_test.category = gui::FeatureCategory::Performance;
        stress_test.enabled = true;
        stress_test.version = "1.0.0";
        stress_test.launch_callback = [this]() {
            ecscope::core::Log::info("Stress Test: Starting system stress test");
            start_stress_test();
        };
        stress_test.status_callback = [this]() { return !stress_test_running_; };

        dashboard_->register_feature(stress_test);

        // Add system toggle feature
        gui::FeatureInfo system_toggle;
        system_toggle.id = "system_toggle";
        system_toggle.name = "System Health Toggle";
        system_toggle.description = "Toggle system health states for demonstration purposes.";
        system_toggle.category = gui::FeatureCategory::Debugging;
        system_toggle.enabled = true;
        system_toggle.version = "1.0.0";
        system_toggle.launch_callback = [this]() {
            ecscope::core::Log::info("System Toggle: Toggling system health states");
            toggle_system_health();
        };
        system_toggle.status_callback = []() { return true; };

        dashboard_->register_feature(system_toggle);
    }

    void update_demo_systems(float delta_time) {
        if (!dashboard_) return;

        // Update performance metrics with realistic data
        gui::PerformanceMetrics metrics;
        metrics.frame_rate = get_random_float(58.0f, 62.0f); // Simulate slight variation around 60 FPS
        metrics.frame_time_ms = 1000.0f / metrics.frame_rate;
        metrics.cpu_usage = get_random_float(25.0f, 45.0f);
        metrics.memory_usage = static_cast<size_t>(get_random_float(800.0f, 1200.0f)) * 1024 * 1024; // 800-1200 MB
        metrics.gpu_memory_usage = static_cast<size_t>(get_random_float(500.0f, 800.0f)) * 1024 * 1024; // 500-800 MB
        metrics.draw_calls = static_cast<uint32_t>(get_random_float(150, 300));
        metrics.vertices_rendered = static_cast<uint32_t>(get_random_float(50000, 150000));
        metrics.timestamp = std::chrono::steady_clock::now();

        dashboard_->update_performance_metrics(metrics);

        // Update stress test
        if (stress_test_running_) {
            stress_test_time_ += delta_time;
            if (stress_test_time_ >= 10.0f) { // Run for 10 seconds
                stress_test_running_ = false;
                stress_test_time_ = 0.0f;
                ecscope::core::Log::info("Stress Test: Completed successfully");
            }
        }

        // Randomly change system health for demo
        static float health_toggle_timer = 0.0f;
        health_toggle_timer += delta_time;
        if (health_toggle_timer >= 15.0f) { // Change every 15 seconds
            health_toggle_timer = 0.0f;
            auto& rng = get_rng();
            std::uniform_real_distribution<float> dist(0.0f, 1.0f);
            
            if (dist(rng) < 0.3f) { // 30% chance to toggle each system
                render_system_healthy_ = !render_system_healthy_;
            }
            if (dist(rng) < 0.2f) { // 20% chance
                physics_system_healthy_ = !physics_system_healthy_;
            }
            if (dist(rng) < 0.1f) { // 10% chance
                network_connected_ = !network_connected_;
            }
        }

        // Render demo dialog if needed
        if (show_demo_dialog_) {
            render_demo_dialog();
        }
    }

    void start_stress_test() {
        stress_test_running_ = true;
        stress_test_time_ = 0.0f;
        ecscope::core::Log::info("Stress Test: Running comprehensive system validation...");
    }

    void toggle_system_health() {
        render_system_healthy_ = !render_system_healthy_;
        physics_system_healthy_ = !physics_system_healthy_;
        network_connected_ = !network_connected_;
        
        ecscope::core::Log::info("System Toggle: Health states changed");
        ecscope::core::Log::info("  - Rendering: {}", render_system_healthy_ ? "Healthy" : "Degraded");
        ecscope::core::Log::info("  - Physics: {}", physics_system_healthy_ ? "Healthy" : "Degraded");
        ecscope::core::Log::info("  - Network: {}", network_connected_ ? "Connected" : "Disconnected");
    }

    void render_demo_dialog() {
#ifdef ECSCOPE_HAS_IMGUI
        if (!show_demo_dialog_) return;

        ImGui::OpenPopup("Dashboard Demo");
        
        if (ImGui::BeginPopupModal("Dashboard Demo", &show_demo_dialog_, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::Text("ECScope Dashboard Demo");
            ImGui::Separator();
            
            ImGui::TextWrapped(
                "Welcome to the ECScope Dashboard demonstration! This interactive demo showcases "
                "all the features of the professional dashboard system:");
            
            ImGui::Spacing();
            ImGui::BulletText("Feature Gallery with categorized system showcase");
            ImGui::BulletText("Real-time system status monitoring");
            ImGui::BulletText("Performance metrics and graphing");
            ImGui::BulletText("Flexible docking and workspace management");
            ImGui::BulletText("Professional theming and styling");
            ImGui::BulletText("Search and navigation functionality");
            
            ImGui::Spacing();
            ImGui::TextWrapped(
                "Try exploring different panels, changing themes, and using the workspace presets "
                "from the View menu. The stress test feature will simulate system load, and the "
                "health toggle demonstrates status monitoring.");
            
            ImGui::Spacing();
            ImGui::Separator();
            
            if (ImGui::Button("Close")) {
                show_demo_dialog_ = false;
            }
            
            ImGui::SameLine();
            if (ImGui::Button("Run Stress Test")) {
                start_stress_test();
                show_demo_dialog_ = false;
            }
            
            ImGui::SameLine();
            if (ImGui::Button("Toggle System Health")) {
                toggle_system_health();
            }
            
            ImGui::EndPopup();
        }
#endif
    }

    float get_random_float(float min, float max) {
        auto& rng = get_rng();
        std::uniform_real_distribution<float> dist(min, max);
        return dist(rng);
    }

    std::mt19937& get_rng() {
        static std::random_device rd;
        static std::mt19937 gen(rd());
        return gen;
    }

    // Core objects
    std::unique_ptr<gui::Dashboard> dashboard_;
    
#ifdef ECSCOPE_HAS_GLFW
    GLFWwindow* window_ = nullptr;
#endif
    
    // Demo state
    bool render_system_healthy_ = true;
    bool physics_system_healthy_ = true;
    bool network_connected_ = true;
    bool stress_test_running_ = false;
    float stress_test_time_ = 0.0f;
    bool show_demo_dialog_ = false;
};

} // namespace ecscope::examples

int main() {
    try {
        ecscope::examples::DashboardDemo demo;
        
        if (demo.initialize()) {
            demo.run();
        } else {
            ecscope::core::Log::error("Failed to initialize dashboard demo");
            return 1;
        }
        
        demo.shutdown();
        
    } catch (const std::exception& e) {
        ecscope::core::Log::error("Dashboard demo failed with exception: {}", e.what());
        return 1;
    }
    
    return 0;
}