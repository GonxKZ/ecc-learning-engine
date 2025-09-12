/**
 * @file responsive_design_demo.cpp
 * @brief ECScope Responsive Design System Demo
 * 
 * Comprehensive demonstration of the ECScope responsive design system,
 * showcasing DPI scaling, adaptive layouts, touch interfaces, and
 * professional game engine UI patterns across different screen sizes.
 * 
 * @author ECScope Development Team
 * @version 1.0.0
 */

#include "../../include/ecscope/gui/gui_manager.hpp"
#include "../../include/ecscope/gui/responsive_design.hpp"
#include "../../include/ecscope/gui/responsive_testing.hpp"
#include "../../include/ecscope/core/log.hpp"

#include <iostream>
#include <memory>
#include <chrono>
#include <thread>

#ifdef ECSCOPE_HAS_IMGUI
#include <imgui.h>
#endif

using namespace ecscope::gui;
using namespace ecscope::core;

// =============================================================================
// DEMO APPLICATION CLASS
// =============================================================================

class ResponsiveDesignDemo {
public:
    ResponsiveDesignDemo() = default;
    ~ResponsiveDesignDemo() = default;

    bool initialize() {
        Log::info("ResponsiveDesignDemo: Initializing...");

        // Configure GUI window
        WindowConfig window_config;
        window_config.title = "ECScope - Responsive Design System Demo";
        window_config.width = 1280;
        window_config.height = 720;
        window_config.resizable = true;
        window_config.vsync = true;

        // Initialize GUI with responsive features
        GUIFlags gui_flags = GUIFlags::EnableDocking | GUIFlags::EnableViewports | 
                           GUIFlags::EnableKeyboardNav | GUIFlags::DarkTheme | GUIFlags::HighDPI;

        if (!InitializeGlobalGUI(window_config, gui_flags, nullptr)) {
            Log::error("ResponsiveDesignDemo: Failed to initialize GUI system");
            return false;
        }

        gui_manager_ = GetGUIManager();
        if (!gui_manager_) {
            Log::error("ResponsiveDesignDemo: Failed to get GUI manager");
            return false;
        }

        // Initialize responsive design system
        ResponsiveConfig responsive_config;
        responsive_config.mode = ResponsiveMode::Adaptive;
        responsive_config.touch_mode = TouchMode::Auto;
        responsive_config.auto_dpi_scaling = true;
        responsive_config.smooth_transitions = true;

        if (!InitializeGlobalResponsiveDesign(gui_manager_->get_main_window(), responsive_config)) {
            Log::error("ResponsiveDesignDemo: Failed to initialize responsive design system");
            return false;
        }

        responsive_manager_ = GetResponsiveDesignManager();
        if (!responsive_manager_) {
            Log::error("ResponsiveDesignDemo: Failed to get responsive design manager");
            return false;
        }

        // Initialize responsive testing framework
        testing::TestConfig test_config;
        test_config.enable_visual_regression = true;
        test_config.enable_performance_testing = true;
        test_config.generate_screenshots = true;

        if (!testing::InitializeGlobalResponsiveTesting(responsive_manager_, test_config)) {
            Log::warning("ResponsiveDesignDemo: Failed to initialize testing framework (non-critical)");
        } else {
            testing_framework_ = testing::GetResponsiveTestingFramework();
        }

        // Setup callbacks for responsive changes
        responsive_manager_->add_screen_size_callback(
            [this](ScreenSize old_size, ScreenSize new_size) {
                on_screen_size_changed(old_size, new_size);
            }
        );

        responsive_manager_->add_dpi_scale_callback(
            [this](float old_scale, float new_scale) {
                on_dpi_scale_changed(old_scale, new_scale);
            }
        );

        // Initialize demo components
        initialize_demo_components();

        Log::info("ResponsiveDesignDemo: Initialized successfully");
        return true;
    }

    void shutdown() {
        Log::info("ResponsiveDesignDemo: Shutting down...");

        testing::ShutdownGlobalResponsiveTesting();
        ShutdownGlobalResponsiveDesign();
        ShutdownGlobalGUI();

        Log::info("ResponsiveDesignDemo: Shutdown complete");
    }

    void run() {
        Log::info("ResponsiveDesignDemo: Starting main loop");

        auto last_time = std::chrono::steady_clock::now();

        while (!gui_manager_->should_close()) {
            // Calculate delta time
            auto current_time = std::chrono::steady_clock::now();
            float delta_time = std::chrono::duration<float>(current_time - last_time).count();
            last_time = current_time;

            // Poll events
            gui_manager_->poll_events();

            // Update responsive system
            responsive_manager_->update(delta_time);

            // Begin frame
            gui_manager_->begin_frame();

            // Render demo UI
            render_demo_ui();

            // End frame
            gui_manager_->end_frame();
        }

        Log::info("ResponsiveDesignDemo: Main loop ended");
    }

private:
    // =============================================================================
    // INITIALIZATION HELPERS
    // =============================================================================

    void initialize_demo_components() {
        // Initialize demo data
        demo_data_.game_objects = {
            "Player", "Enemy_01", "Enemy_02", "Powerup_Health", "Powerup_Weapon",
            "Platform_01", "Platform_02", "Background_Sky", "Background_Mountains",
            "UI_HealthBar", "UI_ScoreText", "Audio_BGM", "Audio_SFX_Jump"
        };

        demo_data_.components = {
            "Transform", "Renderer", "Collider", "RigidBody", "AudioSource",
            "Camera", "Light", "Script", "Animation", "ParticleSystem"
        };

        demo_data_.performance_metrics = {
            {"FPS", 60.0f}, {"Frame Time", 16.6f}, {"Draw Calls", 45.0f},
            {"Vertices", 12580.0f}, {"Memory", 256.7f}, {"GPU Memory", 1024.3f}
        };

        demo_data_.touch_enabled = responsive_manager_->is_touch_enabled();
        demo_data_.current_screen_size = responsive_manager_->get_screen_size();
        demo_data_.current_dpi_scale = responsive_manager_->get_dpi_scale();
    }

    // =============================================================================
    // MAIN UI RENDERING
    // =============================================================================

    void render_demo_ui() {
        // Create main dockspace
        create_dockspace();

        // Render responsive panels
        render_dashboard_panel();
        render_ecs_inspector_panel();
        render_performance_panel();
        render_responsive_controls_panel();
        render_testing_panel();
        render_asset_browser_panel();
        render_console_panel();

        // Show responsive information overlay
        if (show_responsive_info_) {
            render_responsive_info_overlay();
        }

        // Handle demo shortcuts
        handle_shortcuts();
    }

    void create_dockspace() {
#ifdef ECSCOPE_HAS_IMGUI
        ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->WorkPos);
        ImGui::SetNextWindowSize(viewport->WorkSize);
        ImGui::SetNextWindowViewport(viewport->ID);

        ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking |
                                       ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse |
                                       ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                                       ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

        bool open = true;
        ImGui::Begin("ResponsiveDesignDemo_Dockspace", &open, window_flags);
        ImGui::PopStyleVar(3);

        // Create main menu
        if (ImGui::BeginMenuBar()) {
            render_main_menu();
            ImGui::EndMenuBar();
        }

        // Create dockspace
        ImGuiID dockspace_id = ImGui::GetID("ResponsiveDesignDemo_DockSpace");
        ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_PassthruCentralNode);

        ImGui::End();
#endif
    }

    void render_main_menu() {
#ifdef ECSCOPE_HAS_IMGUI
        if (ImGui::BeginMenu("View")) {
            ImGui::MenuItem("Dashboard", nullptr, &show_dashboard_);
            ImGui::MenuItem("ECS Inspector", nullptr, &show_ecs_inspector_);
            ImGui::MenuItem("Performance", nullptr, &show_performance_);
            ImGui::MenuItem("Responsive Controls", nullptr, &show_responsive_controls_);
            ImGui::MenuItem("Testing", nullptr, &show_testing_);
            ImGui::MenuItem("Asset Browser", nullptr, &show_asset_browser_);
            ImGui::MenuItem("Console", nullptr, &show_console_);
            ImGui::Separator();
            ImGui::MenuItem("Responsive Info Overlay", "F2", &show_responsive_info_);
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Responsive")) {
            if (ImGui::BeginMenu("Screen Size Simulation")) {
                if (ImGui::MenuItem("Mobile Portrait (360x640)")) simulate_screen_size(360, 640, 2.0f);
                if (ImGui::MenuItem("Mobile Landscape (640x360)")) simulate_screen_size(640, 360, 2.0f);
                if (ImGui::MenuItem("Tablet (768x1024)")) simulate_screen_size(768, 1024, 1.5f);
                if (ImGui::MenuItem("Laptop (1366x768)")) simulate_screen_size(1366, 768, 1.0f);
                if (ImGui::MenuItem("Desktop (1920x1080)")) simulate_screen_size(1920, 1080, 1.0f);
                if (ImGui::MenuItem("4K (3840x2160)")) simulate_screen_size(3840, 2160, 1.5f);
                ImGui::Separator();
                if (ImGui::MenuItem("Reset to Native")) reset_screen_simulation();
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("UI Scale")) {
                if (ImGui::MenuItem("75%")) responsive_manager_->set_user_ui_scale(0.75f);
                if (ImGui::MenuItem("100%")) responsive_manager_->set_user_ui_scale(1.0f);
                if (ImGui::MenuItem("125%")) responsive_manager_->set_user_ui_scale(1.25f);
                if (ImGui::MenuItem("150%")) responsive_manager_->set_user_ui_scale(1.5f);
                if (ImGui::MenuItem("200%")) responsive_manager_->set_user_ui_scale(2.0f);
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Touch Mode")) {
                bool disabled = responsive_manager_->get_config().touch_mode == TouchMode::Disabled;
                bool enabled = responsive_manager_->get_config().touch_mode == TouchMode::Enabled;
                bool auto_mode = responsive_manager_->get_config().touch_mode == TouchMode::Auto;

                if (ImGui::MenuItem("Disabled", nullptr, disabled)) set_touch_mode(TouchMode::Disabled);
                if (ImGui::MenuItem("Enabled", nullptr, enabled)) set_touch_mode(TouchMode::Enabled);
                if (ImGui::MenuItem("Auto", nullptr, auto_mode)) set_touch_mode(TouchMode::Auto);
                ImGui::EndMenu();
            }

            ImGui::Separator();

            if (ImGui::MenuItem("Run Responsive Tests")) {
                run_responsive_tests();
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Help")) {
            ImGui::MenuItem("About", nullptr, &show_about_);
            if (ImGui::MenuItem("Show Demo Controls", "F1")) {
                show_demo_controls_ = !show_demo_controls_;
            }
            ImGui::EndMenu();
        }
#endif
    }

    // =============================================================================
    // PANEL RENDERING
    // =============================================================================

    void render_dashboard_panel() {
        if (!show_dashboard_) return;

#ifdef ECSCOPE_HAS_IMGUI
        if (responsive_manager_->begin_responsive_window("Dashboard", &show_dashboard_)) {
            render_dashboard_content();
        }
        responsive_manager_->end_responsive_window();
#endif
    }

    void render_dashboard_content() {
#ifdef ECSCOPE_HAS_IMGUI
        ResponsiveWidget widget(responsive_manager_);

        // Adaptive layout for dashboard cards
        std::vector<std::function<void()>> dashboard_cards = {
            [this]() { render_project_info_card(); },
            [this]() { render_performance_summary_card(); },
            [this]() { render_recent_activity_card(); },
            [this]() { render_quick_actions_card(); }
        };

        widget.adaptive_layout(dashboard_cards, 2, 4);
#endif
    }

    void render_project_info_card() {
#ifdef ECSCOPE_HAS_IMGUI
        ImGui::BeginChild("ProjectInfo", ImVec2(0, responsive_manager_->scale(200.0f)), true);
        
        ResponsiveWidget widget(responsive_manager_);
        
        // Title with responsive font
        ImGui::PushFont(responsive_manager_->get_font(responsive_manager_->get_screen_size(), "h2"));
        widget.responsive_text("Project Information", true);
        ImGui::PopFont();
        
        widget.responsive_separator();
        
        ImGui::Text("Name: ECScope Game Engine");
        ImGui::Text("Version: 1.0.0");
        ImGui::Text("Build: Debug");
        ImGui::Text("Platform: %s", get_platform_string().c_str());
        
        widget.responsive_spacing();
        
        ImGui::Text("Resolution: %dx%d", 
                   responsive_manager_->get_primary_display().width,
                   responsive_manager_->get_primary_display().height);
        ImGui::Text("DPI Scale: %.2fx", responsive_manager_->get_dpi_scale());
        ImGui::Text("Screen Size: %s", get_screen_size_string().c_str());
        
        ImGui::EndChild();
#endif
    }

    void render_performance_summary_card() {
#ifdef ECSCOPE_HAS_IMGUI
        ImGui::BeginChild("PerfSummary", ImVec2(0, responsive_manager_->scale(200.0f)), true);
        
        ResponsiveWidget widget(responsive_manager_);
        
        ImGui::PushFont(responsive_manager_->get_font(responsive_manager_->get_screen_size(), "h2"));
        widget.responsive_text("Performance", true);
        ImGui::PopFont();
        
        widget.responsive_separator();
        
        for (const auto& [metric, value] : demo_data_.performance_metrics) {
            ImGui::Text("%s: %.1f", metric.c_str(), value);
        }
        
        widget.responsive_spacing();
        
        // Performance bars (responsive sizing)
        float bar_height = responsive_manager_->get_spacing("medium");
        ImGui::ProgressBar(0.85f, ImVec2(-1.0f, bar_height), "GPU Usage");
        ImGui::ProgressBar(0.62f, ImVec2(-1.0f, bar_height), "Memory");
        
        ImGui::EndChild();
#endif
    }

    void render_recent_activity_card() {
#ifdef ECSCOPE_HAS_IMGUI
        ImGui::BeginChild("RecentActivity", ImVec2(0, responsive_manager_->scale(200.0f)), true);
        
        ResponsiveWidget widget(responsive_manager_);
        
        ImGui::PushFont(responsive_manager_->get_font(responsive_manager_->get_screen_size(), "h2"));
        widget.responsive_text("Recent Activity", true);
        ImGui::PopFont();
        
        widget.responsive_separator();
        
        ImGui::Text("• Component added to Entity_001");
        ImGui::Text("• Texture atlas updated");
        ImGui::Text("• Audio system initialized");
        ImGui::Text("• 15 entities created");
        ImGui::Text("• Scene saved successfully");
        
        ImGui::EndChild();
#endif
    }

    void render_quick_actions_card() {
#ifdef ECSCOPE_HAS_IMGUI
        ImGui::BeginChild("QuickActions", ImVec2(0, responsive_manager_->scale(200.0f)), true);
        
        ResponsiveWidget widget(responsive_manager_);
        
        ImGui::PushFont(responsive_manager_->get_font(responsive_manager_->get_screen_size(), "h2"));
        widget.responsive_text("Quick Actions", true);
        ImGui::PopFont();
        
        widget.responsive_separator();
        
        // Responsive buttons
        if (responsive_manager_->responsive_button("New Scene")) {
            Log::info("New Scene clicked");
        }
        
        if (responsive_manager_->responsive_button("Load Project")) {
            Log::info("Load Project clicked");
        }
        
        if (responsive_manager_->responsive_button("Build Game")) {
            Log::info("Build Game clicked");
        }
        
        if (responsive_manager_->responsive_button("Run Tests")) {
            run_responsive_tests();
        }
        
        ImGui::EndChild();
#endif
    }

    void render_ecs_inspector_panel() {
        if (!show_ecs_inspector_) return;

#ifdef ECSCOPE_HAS_IMGUI
        if (responsive_manager_->begin_responsive_window("ECS Inspector", &show_ecs_inspector_)) {
            render_ecs_inspector_content();
        }
        responsive_manager_->end_responsive_window();
#endif
    }

    void render_ecs_inspector_content() {
#ifdef ECSCOPE_HAS_IMGUI
        ResponsiveWidget widget(responsive_manager_);
        
        // Adaptive columns for entities and components
        responsive_manager_->begin_adaptive_columns(2, 3);
        
        // Entities column
        ImGui::BeginChild("Entities", ImVec2(0, 0), true);
        ImGui::PushFont(responsive_manager_->get_font(responsive_manager_->get_screen_size(), "h3"));
        widget.responsive_text("Entities");
        ImGui::PopFont();
        
        widget.responsive_separator();
        
        static int selected_entity = 0;
        for (int i = 0; i < demo_data_.game_objects.size(); ++i) {
            if (responsive_manager_->responsive_selectable(demo_data_.game_objects[i].c_str(), selected_entity == i)) {
                selected_entity = i;
            }
        }
        ImGui::EndChild();
        
        ImGui::NextColumn();
        
        // Components column
        ImGui::BeginChild("Components", ImVec2(0, 0), true);
        ImGui::PushFont(responsive_manager_->get_font(responsive_manager_->get_screen_size(), "h3"));
        widget.responsive_text("Components");
        ImGui::PopFont();
        
        widget.responsive_separator();
        
        if (selected_entity < demo_data_.game_objects.size()) {
            ImGui::Text("Entity: %s", demo_data_.game_objects[selected_entity].c_str());
            widget.responsive_spacing();
            
            for (const auto& component : demo_data_.components) {
                ImGui::Text("• %s", component.c_str());
            }
        }
        
        ImGui::EndChild();
        
        responsive_manager_->end_adaptive_columns();
#endif
    }

    void render_performance_panel() {
        if (!show_performance_) return;

#ifdef ECSCOPE_HAS_IMGUI
        if (responsive_manager_->begin_responsive_window("Performance Monitor", &show_performance_)) {
            render_performance_content();
        }
        responsive_manager_->end_responsive_window();
#endif
    }

    void render_performance_content() {
#ifdef ECSCOPE_HAS_IMGUI
        ResponsiveWidget widget(responsive_manager_);
        
        // Performance metrics with responsive layout
        responsive_manager_->begin_adaptive_columns(2, 4);
        
        for (const auto& [metric, value] : demo_data_.performance_metrics) {
            ImGui::BeginChild((metric + "_metric").c_str(), 
                            ImVec2(0, responsive_manager_->scale(100.0f)), true);
            
            widget.responsive_text(metric.c_str(), true);
            
            ImGui::PushFont(responsive_manager_->get_font(responsive_manager_->get_screen_size(), "h1"));
            widget.responsive_text(std::to_string(static_cast<int>(value)).c_str(), true);
            ImGui::PopFont();
            
            ImGui::EndChild();
            ImGui::NextColumn();
        }
        
        responsive_manager_->end_adaptive_columns();
        
        widget.responsive_spacing();
        
        // Performance history (simulated)
        ImGui::Text("Performance History:");
        static float values[90] = {};
        static int values_offset = 0;
        static float refresh_time = 0.0;
        
        refresh_time += ImGui::GetIO().DeltaTime;
        if (refresh_time >= 1.0f / 60.0f) {
            values[values_offset] = demo_data_.performance_metrics["FPS"] + 
                                  (rand() % 20 - 10); // Simulate variation
            values_offset = (values_offset + 1) % 90;
            refresh_time = 0.0f;
        }
        
        float graph_height = responsive_manager_->scale(80.0f);
        ImGui::PlotLines("##FPS", values, 90, values_offset, "FPS", 0.0f, 100.0f, 
                        ImVec2(-1, graph_height));
#endif
    }

    void render_responsive_controls_panel() {
        if (!show_responsive_controls_) return;

#ifdef ECSCOPE_HAS_IMGUI
        if (responsive_manager_->begin_responsive_window("Responsive Controls", &show_responsive_controls_)) {
            render_responsive_controls_content();
        }
        responsive_manager_->end_responsive_window();
#endif
    }

    void render_responsive_controls_content() {
#ifdef ECSCOPE_HAS_IMGUI
        ResponsiveWidget widget(responsive_manager_);
        
        ImGui::PushFont(responsive_manager_->get_font(responsive_manager_->get_screen_size(), "h2"));
        widget.responsive_text("Responsive Design Controls");
        ImGui::PopFont();
        
        widget.responsive_separator();
        
        // Screen size simulation
        ImGui::Text("Screen Size Simulation:");
        static int screen_preset = 4; // Default to desktop
        const char* presets[] = {
            "Mobile Portrait (360x640)",
            "Mobile Landscape (640x360)",
            "Tablet (768x1024)",
            "Laptop (1366x768)",
            "Desktop (1920x1080)",
            "4K (3840x2160)"
        };
        
        if (widget.responsive_combo("Preset", &screen_preset, presets, 6)) {
            apply_screen_preset(screen_preset);
        }
        
        widget.responsive_spacing();
        
        // DPI scaling
        ImGui::Text("DPI Scale:");
        float dpi_scale = responsive_manager_->get_dpi_scale();
        if (ImGui::SliderFloat("##DPIScale", &dpi_scale, 0.5f, 4.0f, "%.2fx")) {
            // Note: This would require extending the responsive system to support manual DPI override
            Log::info("DPI scale changed to: %.2f", dpi_scale);
        }
        
        // UI scale
        ImGui::Text("UI Scale:");
        float ui_scale = responsive_manager_->get_effective_ui_scale();
        if (ImGui::SliderFloat("##UIScale", &ui_scale, 0.5f, 3.0f, "%.2fx")) {
            responsive_manager_->set_user_ui_scale(ui_scale);
        }
        
        widget.responsive_spacing();
        
        // Touch mode
        ImGui::Text("Touch Mode:");
        TouchMode current_mode = responsive_manager_->get_config().touch_mode;
        int mode_index = static_cast<int>(current_mode);
        const char* touch_modes[] = {"Disabled", "Enabled", "Auto"};
        
        if (widget.responsive_combo("##TouchMode", &mode_index, touch_modes, 3)) {
            set_touch_mode(static_cast<TouchMode>(mode_index));
        }
        
        widget.responsive_spacing();
        
        // Current state display
        ImGui::Text("Current State:");
        ImGui::Text("  Screen Size: %s", get_screen_size_string().c_str());
        ImGui::Text("  DPI Scale: %.2fx", responsive_manager_->get_dpi_scale());
        ImGui::Text("  Effective Scale: %.2fx", responsive_manager_->get_effective_ui_scale());
        ImGui::Text("  Touch Enabled: %s", responsive_manager_->is_touch_enabled() ? "Yes" : "No");
#endif
    }

    void render_testing_panel() {
        if (!show_testing_) return;

#ifdef ECSCOPE_HAS_IMGUI
        if (responsive_manager_->begin_responsive_window("Responsive Testing", &show_testing_)) {
            render_testing_content();
        }
        responsive_manager_->end_responsive_window();
#endif
    }

    void render_testing_content() {
#ifdef ECSCOPE_HAS_IMGUI
        ResponsiveWidget widget(responsive_manager_);
        
        ImGui::PushFont(responsive_manager_->get_font(responsive_manager_->get_screen_size(), "h2"));
        widget.responsive_text("Responsive Testing Framework");
        ImGui::PopFont();
        
        widget.responsive_separator();
        
        if (!testing_framework_) {
            ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.5f, 1.0f), "Testing framework not available");
            return;
        }
        
        // Test execution controls
        if (responsive_manager_->responsive_button("Run All Tests")) {
            run_all_tests();
        }
        
        ImGui::SameLine();
        if (responsive_manager_->responsive_button("Run Layout Tests")) {
            run_layout_tests();
        }
        
        ImGui::SameLine();
        if (responsive_manager_->responsive_button("Run Performance Tests")) {
            run_performance_tests();
        }
        
        widget.responsive_spacing();
        
        // Test results display
        if (!last_test_results_.results.empty()) {
            ImGui::Text("Last Test Results:");
            ImGui::Text("  Total: %d", last_test_results_.total_tests);
            ImGui::Text("  Passed: %d", last_test_results_.passed_tests);
            ImGui::Text("  Failed: %d", last_test_results_.failed_tests);
            ImGui::Text("  Duration: %lld ms", last_test_results_.total_execution_time.count());
            
            widget.responsive_spacing();
            
            // Detailed results
            if (ImGui::CollapsingHeader("Detailed Results")) {
                for (const auto& result : last_test_results_.results) {
                    ImVec4 color = (result.result == testing::TestResult::Pass) ? 
                                  ImVec4(0.5f, 1.0f, 0.5f, 1.0f) : ImVec4(1.0f, 0.5f, 0.5f, 1.0f);
                    
                    ImGui::TextColored(color, "%s: %s", 
                                     result.test_name.c_str(), 
                                     testing::ResponsiveTestingFramework::test_result_to_string(result.result).c_str());
                    
                    if (!result.message.empty()) {
                        ImGui::Text("    %s", result.message.c_str());
                    }
                }
            }
        }
#endif
    }

    void render_asset_browser_panel() {
        if (!show_asset_browser_) return;

#ifdef ECSCOPE_HAS_IMGUI
        if (responsive_manager_->begin_responsive_window("Asset Browser", &show_asset_browser_)) {
            render_asset_browser_content();
        }
        responsive_manager_->end_responsive_window();
#endif
    }

    void render_asset_browser_content() {
#ifdef ECSCOPE_HAS_IMGUI
        ResponsiveWidget widget(responsive_manager_);
        
        // Simulate asset browser with adaptive layout
        std::vector<std::string> asset_categories = {
            "Textures", "Models", "Audio", "Scripts", "Prefabs", "Materials"
        };
        
        std::vector<std::function<void()>> category_widgets;
        for (const auto& category : asset_categories) {
            category_widgets.push_back([this, category]() {
                ImGui::BeginChild((category + "_assets").c_str(), 
                                ImVec2(0, responsive_manager_->scale(150.0f)), true);
                
                ResponsiveWidget widget(responsive_manager_);
                widget.responsive_text(category.c_str(), true);
                
                // Simulate asset thumbnails
                int item_count = 8;
                int columns = responsive_manager_->is_screen_at_most(ScreenSize::Small) ? 2 : 4;
                
                for (int i = 0; i < item_count; ++i) {
                    if (i % columns != 0) ImGui::SameLine();
                    
                    std::string item_name = category + "_" + std::to_string(i + 1);
                    ImVec2 button_size = responsive_manager_->get_touch_button_size();
                    
                    if (ImGui::Button(item_name.c_str(), button_size)) {
                        Log::info("Asset selected: %s", item_name.c_str());
                    }
                }
                
                ImGui::EndChild();
            });
        }
        
        widget.adaptive_layout(category_widgets, 2, 3);
#endif
    }

    void render_console_panel() {
        if (!show_console_) return;

#ifdef ECSCOPE_HAS_IMGUI
        if (responsive_manager_->begin_responsive_window("Console", &show_console_)) {
            render_console_content();
        }
        responsive_manager_->end_responsive_window();
#endif
    }

    void render_console_content() {
#ifdef ECSCOPE_HAS_IMGUI
        ResponsiveWidget widget(responsive_manager_);
        
        // Console output area
        ImGui::BeginChild("ConsoleOutput", ImVec2(0, -responsive_manager_->scale(30.0f)), true);
        
        // Simulate console messages
        static std::vector<std::string> console_messages = {
            "[INFO] ECScope Engine initialized",
            "[INFO] Responsive design system loaded",
            "[DEBUG] Screen size detected: Desktop (1920x1080)",
            "[INFO] Touch mode: Disabled",
            "[DEBUG] DPI scale: 1.00x",
            "[INFO] All systems ready",
            "[DEBUG] Frame rate: 60 FPS"
        };
        
        for (const auto& message : console_messages) {
            ImGui::Text("%s", message.c_str());
        }
        
        // Auto-scroll to bottom
        if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
            ImGui::SetScrollHereY(1.0f);
        
        ImGui::EndChild();
        
        // Console input
        static char input_buffer[256] = "";
        ImGui::SetNextItemWidth(-1.0f);
        
        if (widget.responsive_input_text("##ConsoleInput", input_buffer, sizeof(input_buffer))) {
            if (strlen(input_buffer) > 0) {
                console_messages.push_back(std::string("> ") + input_buffer);
                console_messages.push_back("[INFO] Command executed: " + std::string(input_buffer));
                input_buffer[0] = '\0';
            }
        }
#endif
    }

    void render_responsive_info_overlay() {
#ifdef ECSCOPE_HAS_IMGUI
        ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(ImVec2(viewport->WorkPos.x + 10, viewport->WorkPos.y + 10));
        
        ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize |
                                ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing |
                                ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoMove;
        
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.0f, 0.0f, 0.0f, 0.8f));
        
        if (ImGui::Begin("Responsive Info", &show_responsive_info_, flags)) {
            const DisplayInfo& display = responsive_manager_->get_primary_display();
            
            ImGui::Text("Responsive Design System Info");
            ImGui::Separator();
            
            ImGui::Text("Resolution: %dx%d", display.width, display.height);
            ImGui::Text("Screen Size: %s", get_screen_size_string().c_str());
            ImGui::Text("DPI Scale: %.2fx", responsive_manager_->get_dpi_scale());
            ImGui::Text("UI Scale: %.2fx", responsive_manager_->get_effective_ui_scale());
            ImGui::Text("Touch: %s", responsive_manager_->is_touch_enabled() ? "Enabled" : "Disabled");
            
            // Performance info
            auto perf = gui_manager_->get_performance_metrics();
            ImGui::Text("FPS: %.1f", perf.frame_rate);
            ImGui::Text("Frame Time: %.2f ms", perf.cpu_time_ms);
            
            ImGui::Text("Press F2 to hide");
        }
        ImGui::End();
        
        ImGui::PopStyleColor();
#endif
    }

    // =============================================================================
    // EVENT HANDLERS
    // =============================================================================

    void on_screen_size_changed(ScreenSize old_size, ScreenSize new_size) {
        Log::info("ResponsiveDesignDemo: Screen size changed from %d to %d", 
                 static_cast<int>(old_size), static_cast<int>(new_size));
        
        demo_data_.current_screen_size = new_size;
        
        // Apply appropriate style preset
        switch (new_size) {
            case ScreenSize::XSmall:
            case ScreenSize::Small:
                ResponsiveStylePresets::apply_dashboard_mobile_style();
                break;
            case ScreenSize::Medium:
                ResponsiveStylePresets::apply_dashboard_tablet_style();
                break;
            default:
                ResponsiveStylePresets::apply_dashboard_desktop_style();
                break;
        }
    }

    void on_dpi_scale_changed(float old_scale, float new_scale) {
        Log::info("ResponsiveDesignDemo: DPI scale changed from %.2f to %.2f", old_scale, new_scale);
        demo_data_.current_dpi_scale = new_scale;
    }

    void handle_shortcuts() {
#ifdef ECSCOPE_HAS_IMGUI
        if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_F1))) {
            show_demo_controls_ = !show_demo_controls_;
        }
        
        if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_F2))) {
            show_responsive_info_ = !show_responsive_info_;
        }
        
        if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_F5))) {
            run_responsive_tests();
        }
#endif
    }

    // =============================================================================
    // UTILITY METHODS
    // =============================================================================

    void simulate_screen_size(int width, int height, float dpi_scale) {
        Log::info("ResponsiveDesignDemo: Simulating screen size %dx%d at %.1fx DPI", width, height, dpi_scale);
        gui_manager_->set_window_size(width, height);
    }

    void reset_screen_simulation() {
        Log::info("ResponsiveDesignDemo: Resetting to native screen size");
        gui_manager_->set_window_size(1280, 720);
    }

    void apply_screen_preset(int preset) {
        switch (preset) {
            case 0: simulate_screen_size(360, 640, 2.0f); break;  // Mobile Portrait
            case 1: simulate_screen_size(640, 360, 2.0f); break;  // Mobile Landscape
            case 2: simulate_screen_size(768, 1024, 1.5f); break; // Tablet
            case 3: simulate_screen_size(1366, 768, 1.0f); break; // Laptop
            case 4: simulate_screen_size(1920, 1080, 1.0f); break; // Desktop
            case 5: simulate_screen_size(3840, 2160, 1.5f); break; // 4K
        }
    }

    void set_touch_mode(TouchMode mode) {
        ResponsiveConfig config = responsive_manager_->get_config();
        config.touch_mode = mode;
        responsive_manager_->set_config(config);
        
        demo_data_.touch_enabled = responsive_manager_->is_touch_enabled();
        
        if (responsive_manager_->is_touch_enabled()) {
            ResponsiveStylePresets::apply_touch_friendly_style();
        }
    }

    void run_responsive_tests() {
        if (!testing_framework_) {
            Log::warning("ResponsiveDesignDemo: Testing framework not available");
            return;
        }
        
        Log::info("ResponsiveDesignDemo: Running responsive tests...");
        last_test_results_ = testing_framework_->run_all_tests();
        
        Log::info("ResponsiveDesignDemo: Tests completed - %d/%d passed", 
                 last_test_results_.passed_tests, last_test_results_.total_tests);
    }

    void run_all_tests() {
        run_responsive_tests();
    }

    void run_layout_tests() {
        if (!testing_framework_) return;
        
        last_test_results_ = testing_framework_->run_tests_by_category(testing::TestCategory::Layout);
        Log::info("ResponsiveDesignDemo: Layout tests completed");
    }

    void run_performance_tests() {
        if (!testing_framework_) return;
        
        last_test_results_ = testing_framework_->run_tests_by_category(testing::TestCategory::Performance);
        Log::info("ResponsiveDesignDemo: Performance tests completed");
    }

    std::string get_platform_string() const {
#ifdef _WIN32
        return "Windows";
#elif defined(__APPLE__)
        return "macOS";
#elif defined(__linux__)
        return "Linux";
#else
        return "Unknown";
#endif
    }

    std::string get_screen_size_string() const {
        switch (responsive_manager_->get_screen_size()) {
            case ScreenSize::XSmall: return "XSmall";
            case ScreenSize::Small: return "Small";
            case ScreenSize::Medium: return "Medium";
            case ScreenSize::Large: return "Large";
            case ScreenSize::XLarge: return "XLarge";
            case ScreenSize::XXLarge: return "XXLarge";
            default: return "Unknown";
        }
    }

    // =============================================================================
    // MEMBER VARIABLES
    // =============================================================================

    // Core systems
    GUIManager* gui_manager_ = nullptr;
    ResponsiveDesignManager* responsive_manager_ = nullptr;
    testing::ResponsiveTestingFramework* testing_framework_ = nullptr;

    // UI state
    bool show_dashboard_ = true;
    bool show_ecs_inspector_ = true;
    bool show_performance_ = true;
    bool show_responsive_controls_ = true;
    bool show_testing_ = true;
    bool show_asset_browser_ = true;
    bool show_console_ = true;
    bool show_responsive_info_ = true;
    bool show_about_ = false;
    bool show_demo_controls_ = false;

    // Demo data
    struct DemoData {
        std::vector<std::string> game_objects;
        std::vector<std::string> components;
        std::unordered_map<std::string, float> performance_metrics;
        bool touch_enabled = false;
        ScreenSize current_screen_size = ScreenSize::Large;
        float current_dpi_scale = 1.0f;
    } demo_data_;

    // Test results
    testing::TestSuiteSummary last_test_results_;
};

// =============================================================================
// MAIN FUNCTION
// =============================================================================

int main() {
    Log::info("=== ECScope Responsive Design System Demo ===");

    ResponsiveDesignDemo demo;

    if (!demo.initialize()) {
        Log::error("Failed to initialize demo application");
        return -1;
    }

    demo.run();
    demo.shutdown();

    Log::info("Demo application completed successfully");
    return 0;
}