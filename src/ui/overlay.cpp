#include "overlay.hpp"
#include "core/log.hpp"
#include "core/time.hpp"
#include <algorithm>
#include <sstream>
#include <iomanip>

#ifdef ECSCOPE_HAS_GRAPHICS
#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_opengl3.h"
#include <SDL2/SDL.h>
#include <GL/gl.h>
#endif

namespace ecscope::ui {

// Global UI overlay instance
static std::unique_ptr<UIOverlay> g_ui_overlay = nullptr;

UIOverlay::~UIOverlay() {
    shutdown();
}

core::CoreResult<void> UIOverlay::initialize(renderer::Window& window) {
#ifdef ECSCOPE_HAS_GRAPHICS
    if (initialized_) {
        LOG_WARN("UIOverlay already initialized");
        return core::CoreResult<void>::Ok();
    }
    
    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    imgui_context_ = ImGui::CreateContext();
    ImGui::SetCurrentContext(imgui_context_);
    imgui_io_ = &ImGui::GetIO();
    
    // Enable keyboard and gamepad controls
    imgui_io_->ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    imgui_io_->ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
    
    // Enable docking and multi-viewport
    imgui_io_->ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    imgui_io_->ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
    
    // Setup Dear ImGui style
    setup_style();
    
    // When viewports are enabled we tweak WindowRounding/WindowBg
    // so platform windows can look identical to regular ones
    ImGuiStyle& style = ImGui::GetStyle();
    if (imgui_io_->ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        style.WindowRounding = 0.0f;
        style.Colors[ImGuiCol_WindowBg].w = 1.0f;
    }
    
    // Setup platform/renderer backends
    if (!ImGui_ImplSDL2_InitForOpenGL(window.native_handle(), window.gl_context())) {
        LOG_ERROR("Failed to initialize ImGui SDL2 backend");
        ImGui::DestroyContext(imgui_context_);
        return core::CoreResult<void>::Err(core::CoreError::InitializationFailed);
    }
    
    const char* glsl_version = "#version 330";
    if (!ImGui_ImplOpenGL3_Init(glsl_version)) {
        LOG_ERROR("Failed to initialize ImGui OpenGL3 backend");
        ImGui_ImplSDL2_Shutdown();
        ImGui::DestroyContext(imgui_context_);
        return core::CoreResult<void>::Err(core::CoreError::InitializationFailed);
    }
    
    initialized_ = true;
    LOG_INFO("UIOverlay initialized successfully");
    
    return core::CoreResult<void>::Ok();
#else
    (void)window;
    LOG_WARN("Graphics support not compiled - UIOverlay initialization skipped");
    return core::CoreResult<void>::Err(core::CoreError::NotSupported);
#endif
}

void UIOverlay::shutdown() {
#ifdef ECSCOPE_HAS_GRAPHICS
    if (!initialized_) {
        return;
    }
    
    panels_.clear();
    
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext(imgui_context_);
    
    imgui_context_ = nullptr;
    imgui_io_ = nullptr;
    initialized_ = false;
    
    LOG_INFO("UIOverlay shutdown");
#endif
}

void UIOverlay::begin_frame() {
#ifdef ECSCOPE_HAS_GRAPHICS
    if (!initialized_) return;
    
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();
#endif
}

void UIOverlay::end_frame() {
#ifdef ECSCOPE_HAS_GRAPHICS
    if (!initialized_) return;
    
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    
    // Update and render additional platform windows
    if (imgui_io_->ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
    }
#endif
}

void UIOverlay::render() {
#ifdef ECSCOPE_HAS_GRAPHICS
    if (!initialized_) return;
    
    core::Timer render_timer;
    
    // Render main menu bar
    render_main_menu_bar();
    
    // Render all panels
    for (auto& panel : panels_) {
        if (panel && panel->is_visible()) {
            panel->render();
        }
    }
    
    // Render debug windows
    render_debug_windows();
    
    // Update render stats
    // (render time will be calculated elsewhere)
#endif
}

void UIOverlay::update(f64 delta_time) {
    // Update all panels
    for (auto& panel : panels_) {
        if (panel && panel->is_visible()) {
            panel->update(delta_time);
        }
    }
}

void UIOverlay::remove_panel(const std::string& name) {
    auto it = std::remove_if(panels_.begin(), panels_.end(),
        [&name](const std::unique_ptr<Panel>& panel) {
            return panel && panel->name() == name;
        });
    
    if (it != panels_.end()) {
        panels_.erase(it, panels_.end());
        LOG_INFO("Panel removed: " + name);
    }
}

Panel* UIOverlay::get_panel(const std::string& name) {
    auto it = std::find_if(panels_.begin(), panels_.end(),
        [&name](const std::unique_ptr<Panel>& panel) {
            return panel && panel->name() == name;
        });
    
    return it != panels_.end() ? it->get() : nullptr;
}

const Panel* UIOverlay::get_panel(const std::string& name) const {
    auto it = std::find_if(panels_.begin(), panels_.end(),
        [&name](const std::unique_ptr<Panel>& panel) {
            return panel && panel->name() == name;
        });
    
    return it != panels_.end() ? it->get() : nullptr;
}

void UIOverlay::set_ui_scale(f32 scale) {
    ui_scale_ = std::clamp(scale, 0.5f, 3.0f);
#ifdef ECSCOPE_HAS_GRAPHICS
    if (initialized_ && imgui_io_) {
        imgui_io_->FontGlobalScale = ui_scale_;
    }
#endif
}

void UIOverlay::set_dark_theme(bool dark) {
    dark_theme_ = dark;
    setup_style();
}

void UIOverlay::handle_window_event(renderer::WindowEvent event) {
#ifdef ECSCOPE_HAS_GRAPHICS
    // Handle UI-specific window events
    switch (event) {
        case renderer::WindowEvent::Resize:
            // ImGui will handle this automatically
            break;
        default:
            break;
    }
#else
    (void)event;
#endif
}

UIOverlay::Stats UIOverlay::get_stats() const {
    Stats stats;
    stats.total_panels = static_cast<u32>(panels_.size());
    stats.active_panels = static_cast<u32>(std::count_if(panels_.begin(), panels_.end(),
        [](const std::unique_ptr<Panel>& panel) {
            return panel && panel->is_visible();
        }));
    
#ifdef ECSCOPE_HAS_GRAPHICS
    if (initialized_ && imgui_io_) {
        stats.capturing_mouse = imgui_io_->WantCaptureMouse;
        stats.capturing_keyboard = imgui_io_->WantCaptureKeyboard;
    }
#endif
    
    return stats;
}

void UIOverlay::setup_style() {
#ifdef ECSCOPE_HAS_GRAPHICS
    if (!initialized_) return;
    
    if (dark_theme_) {
        ImGui::StyleColorsDark();
    } else {
        ImGui::StyleColorsLight();
    }
    
    // Customize style for ECScope
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 6.0f;
    style.ChildRounding = 4.0f;
    style.FrameRounding = 3.0f;
    style.GrabRounding = 2.0f;
    style.ScrollbarRounding = 2.0f;
    style.TabRounding = 4.0f;
    
    // Spacing
    style.WindowPadding = ImVec2(8.0f, 8.0f);
    style.FramePadding = ImVec2(5.0f, 3.0f);
    style.ItemSpacing = ImVec2(8.0f, 4.0f);
    style.ItemInnerSpacing = ImVec2(4.0f, 4.0f);
    style.IndentSpacing = 21.0f;
    style.ScrollbarSize = 16.0f;
    style.GrabMinSize = 12.0f;
    
    // Borders
    style.WindowBorderSize = 1.0f;
    style.ChildBorderSize = 1.0f;
    style.FrameBorderSize = 0.0f;
    style.TabBorderSize = 0.0f;
    
    // Apply UI scale
    if (imgui_io_) {
        imgui_io_->FontGlobalScale = ui_scale_;
    }
#endif
}

void UIOverlay::render_main_menu_bar() {
#ifdef ECSCOPE_HAS_GRAPHICS
    if (!initialized_) return;
    
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("View")) {
            // Toggle panels
            for (auto& panel : panels_) {
                if (panel) {
                    bool visible = panel->is_visible();
                    if (ImGui::MenuItem(panel->name().c_str(), nullptr, &visible)) {
                        panel->set_visible(visible);
                    }
                }
            }
            
            ImGui::Separator();
            ImGui::MenuItem("Demo Window", nullptr, &show_demo_window_);
            ImGui::MenuItem("Metrics Window", nullptr, &show_metrics_window_);
            
            ImGui::EndMenu();
        }
        
        if (ImGui::BeginMenu("Settings")) {
            if (ImGui::BeginMenu("Theme")) {
                if (ImGui::MenuItem("Dark", nullptr, dark_theme_)) {
                    set_dark_theme(true);
                }
                if (ImGui::MenuItem("Light", nullptr, !dark_theme_)) {
                    set_dark_theme(false);
                }
                ImGui::EndMenu();
            }
            
            if (ImGui::SliderFloat("UI Scale", &ui_scale_, 0.5f, 3.0f, "%.1f")) {
                set_ui_scale(ui_scale_);
            }
            
            ImGui::EndMenu();
        }
        
        if (ImGui::BeginMenu("Help")) {
            ImGui::MenuItem("About ECScope", nullptr, false, false);
            ImGui::EndMenu();
        }
        
        ImGui::EndMainMenuBar();
    }
#endif
}

void UIOverlay::render_debug_windows() {
#ifdef ECSCOPE_HAS_GRAPHICS
    if (!initialized_) return;
    
    if (show_demo_window_) {
        ImGui::ShowDemoWindow(&show_demo_window_);
    }
    
    if (show_metrics_window_) {
        ImGui::ShowMetricsWindow(&show_metrics_window_);
    }
#endif
}

// Global UI overlay management
UIOverlay& get_ui_overlay() {
    if (!g_ui_overlay) {
        g_ui_overlay = std::make_unique<UIOverlay>();
        LOG_INFO("UI overlay instance created");
    }
    return *g_ui_overlay;
}

void set_ui_overlay(std::unique_ptr<UIOverlay> overlay) {
    g_ui_overlay = std::move(overlay);
    LOG_INFO("UI overlay instance set");
}

// Utility functions
namespace imgui_utils {
    
    void help_marker(const char* desc) {
#ifdef ECSCOPE_HAS_GRAPHICS
        ImGui::TextDisabled("(?)");
        if (ImGui::IsItemHovered()) {
            ImGui::BeginTooltip();
            ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
            ImGui::TextUnformatted(desc);
            ImGui::PopTextWrapPos();
            ImGui::EndTooltip();
        }
#else
        (void)desc;
#endif
    }
    
    bool color_button(const char* desc_id, const float* col, f32 size) {
#ifdef ECSCOPE_HAS_GRAPHICS
        return ImGui::ColorButton(desc_id, ImVec4(col[0], col[1], col[2], col[3]), 
                                  ImGuiColorEditFlags_NoTooltip, ImVec2(size, size));
#else
        (void)desc_id; (void)col; (void)size;
        return false;
#endif
    }
    
    void progress_bar_animated(f32 fraction, const char* overlay) {
#ifdef ECSCOPE_HAS_GRAPHICS
        static f64 last_time = 0.0;
        f64 current_time = core::get_time_seconds();
        f32 animated_fraction = fraction;
        
        if (fraction > 0.0f && fraction < 1.0f) {
            // Add subtle animation for active progress bars
            f32 wave = 0.05f * std::sin(static_cast<f32>(current_time * 3.0));
            animated_fraction = std::clamp(fraction + wave, 0.0f, 1.0f);
        }
        
        ImGui::ProgressBar(animated_fraction, ImVec2(-1.0f, 0.0f), overlay);
        last_time = current_time;
#else
        (void)fraction; (void)overlay;
#endif
    }
    
    std::string format_bytes(usize bytes) {
        const char* units[] = {"B", "KB", "MB", "GB", "TB"};
        constexpr usize unit_count = sizeof(units) / sizeof(units[0]);
        
        f64 size = static_cast<f64>(bytes);
        usize unit_index = 0;
        
        while (size >= 1024.0 && unit_index < unit_count - 1) {
            size /= 1024.0;
            unit_index++;
        }
        
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(unit_index == 0 ? 0 : 2) 
            << size << " " << units[unit_index];
        return oss.str();
    }
    
    std::string format_number(u64 number) {
        if (number < 1000) {
            return std::to_string(number);
        }
        
        const char* units[] = {"", "K", "M", "B", "T"};
        constexpr usize unit_count = sizeof(units) / sizeof(units[0]);
        
        f64 value = static_cast<f64>(number);
        usize unit_index = 0;
        
        while (value >= 1000.0 && unit_index < unit_count - 1) {
            value /= 1000.0;
            unit_index++;
        }
        
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(1) << value << units[unit_index];
        return oss.str();
    }
    
    void plot_frame_times(const f32* values, usize count, f32 scale_min, f32 scale_max) {
#ifdef ECSCOPE_HAS_GRAPHICS
        ImGui::PlotLines("##FrameTimes", values, static_cast<int>(count), 0, nullptr, 
                        scale_min, scale_max, ImVec2(0, 60));
#else
        (void)values; (void)count; (void)scale_min; (void)scale_max;
#endif
    }
    
    void memory_usage_pie_chart(const char* label, usize used, usize total) {
#ifdef ECSCOPE_HAS_GRAPHICS
        f32 fraction = total > 0 ? static_cast<f32>(used) / static_cast<f32>(total) : 0.0f;
        
        ImDrawList* draw_list = ImGui::GetWindowDrawList();
        ImVec2 canvas_pos = ImGui::GetCursorScreenPos();
        ImVec2 canvas_size = ImGui::GetContentRegionAvail();
        
        if (canvas_size.x < 50.0f) canvas_size.x = 50.0f;
        if (canvas_size.y < 50.0f) canvas_size.y = 50.0f;
        
        f32 radius = std::min(canvas_size.x, canvas_size.y) * 0.4f;
        ImVec2 center = ImVec2(canvas_pos.x + canvas_size.x * 0.5f, canvas_pos.y + canvas_size.y * 0.5f);
        
        // Draw background circle
        draw_list->AddCircleFilled(center, radius, IM_COL32(64, 64, 64, 255), 32);
        
        // Draw usage segment
        if (fraction > 0.0f) {
            constexpr f32 PI = 3.14159265359f;
            f32 angle = fraction * 2.0f * PI;
            draw_list->PathArcTo(center, radius, -PI * 0.5f, -PI * 0.5f + angle, 32);
            draw_list->PathLineTo(center);
            draw_list->PathFillConvex(IM_COL32(100, 150, 255, 255));
        }
        
        // Draw label
        ImGui::SetCursorScreenPos(ImVec2(center.x - 50, center.y + radius + 10));
        ImGui::Text("%s: %.1f%%", label, fraction * 100.0f);
        
        ImGui::Dummy(canvas_size);
#else
        (void)label; (void)used; (void)total;
#endif
    }
}

} // namespace ecscope::ui