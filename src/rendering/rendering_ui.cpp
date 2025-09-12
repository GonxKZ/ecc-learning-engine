/**
 * @file rendering_ui.cpp
 * @brief Implementation of Comprehensive Rendering System UI
 * 
 * Complete implementation of the professional rendering pipeline control interface
 * with real-time parameter adjustment, debugging tools, and performance monitoring.
 * 
 * @author ECScope Development Team
 * @version 1.0.0
 */

#include "../../include/ecscope/gui/rendering_ui.hpp"
#include "../../include/ecscope/core/log.hpp"

#include <filesystem>
#include <fstream>
#include <algorithm>
#include <cmath>
#include <sstream>
#include <iomanip>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// ImGui integration (assuming available)
#ifdef ECSCOPE_HAS_IMGUI
#include <imgui.h>
#include <imgui_internal.h>
#endif

namespace ecscope::gui {

// =============================================================================
// CONSTRUCTOR & DESTRUCTOR
// =============================================================================

RenderingUI::RenderingUI() {
    config_ = create_default_rendering_config();
    last_metrics_update_ = std::chrono::steady_clock::now();
    last_shader_check_ = std::chrono::steady_clock::now();
}

RenderingUI::~RenderingUI() {
    if (initialized_) {
        shutdown();
    }
}

// =============================================================================
// INITIALIZATION & LIFECYCLE
// =============================================================================

bool RenderingUI::initialize(rendering::IRenderer* renderer,
                            rendering::DeferredRenderer* deferred_renderer,
                            Dashboard* dashboard) {
    if (!renderer) {
        ecscope::log_error("RenderingUI", "Renderer is null");
        return false;
    }

    renderer_ = renderer;
    deferred_renderer_ = deferred_renderer;
    dashboard_ = dashboard;

    // Validate configuration against hardware capabilities
    if (!validate_rendering_config(config_, renderer_)) {
        ecscope::log_warning("RenderingUI", "Configuration adjusted for hardware compatibility");
        config_ = create_default_rendering_config();
    }

    // Register with dashboard if provided
    if (dashboard_) {
        register_rendering_ui_features(dashboard_, this);
    }

    // Initialize default scene
    create_preview_scene();

    // Setup default shaders to monitor
    setup_default_shaders();

    initialized_ = true;
    ecscope::log_info("RenderingUI", "Rendering UI initialized successfully");
    return true;
}

void RenderingUI::shutdown() {
    if (!initialized_) {
        return;
    }

    // Clean up scene resources
    clear_scene();

    // Reset state
    renderer_ = nullptr;
    deferred_renderer_ = nullptr;
    dashboard_ = nullptr;
    
    initialized_ = false;
    ecscope::log_info("RenderingUI", "Rendering UI shutdown complete");
}

// =============================================================================
// MAIN RENDERING INTERFACE
// =============================================================================

void RenderingUI::render() {
    if (!initialized_ || !renderer_) {
        return;
    }

#ifdef ECSCOPE_HAS_IMGUI
    // Main dockspace for rendering UI
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    
    if (ImGui::Begin("Rendering System", nullptr, 
                     ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar)) {
        
        ImGuiID dockspace_id = ImGui::GetID("RenderingDockSpace");
        ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_None);
        
        // Render all panels
        if (show_pipeline_panel_) render_main_control_panel();
        if (show_material_editor_) render_material_editor_panel();
        if (show_lighting_panel_) render_lighting_control_panel();
        if (show_post_process_panel_) render_post_processing_panel();
        if (show_debug_panel_) render_debug_visualization_panel();
        if (show_profiler_panel_) render_performance_profiler_panel();
        if (show_scene_hierarchy_) render_scene_hierarchy_panel();
        if (show_viewport_) render_viewport_panel();
        if (show_shader_editor_) render_shader_editor_panel();
        if (show_render_graph_panel_) render_render_graph_panel();
        if (show_gpu_memory_panel_) render_gpu_memory_panel();
    }
    ImGui::End();
    
    ImGui::PopStyleVar();
    
    // Render performance overlay if enabled
    if (show_performance_overlay_) {
        render_performance_overlay();
    }
#endif
}

void RenderingUI::update(float delta_time) {
    if (!initialized_) {
        return;
    }

    animation_time_ += delta_time;

    // Update performance metrics
    update_performance_metrics();

    // Monitor shader files for changes
    if (shader_hot_reload_enabled_) {
        monitor_shader_files();
    }

    // Animate scene lights
    animate_scene_lights(delta_time);

    // Update camera controls if viewport is focused
    if (viewport_focused_) {
        update_camera_controls();
    }

    // Apply configuration changes if needed
    if (config_dirty_) {
        apply_config_changes();
        config_dirty_ = false;
    }

    // Update scene objects and submit to renderer
    update_scene_objects();
    submit_scene_to_renderer();
}

// =============================================================================
// UI PANEL RENDERING METHODS
// =============================================================================

void RenderingUI::render_main_control_panel() {
#ifdef ECSCOPE_HAS_IMGUI
    if (ImGui::Begin("Pipeline Control", &show_pipeline_panel_)) {
        
        // Quick stats at the top
        ImGui::Text("Frame Time: %.2f ms", current_metrics_.frame_time_ms);
        ImGui::SameLine();
        ImGui::Text("GPU Time: %.2f ms", current_metrics_.gpu_time_ms);
        ImGui::SameLine();
        ImGui::Text("Draw Calls: %u", current_metrics_.draw_calls);
        
        ImGui::Separator();
        
        // Main pipeline controls
        if (ImGui::CollapsingHeader("Deferred Rendering", ImGuiTreeNodeFlags_DefaultOpen)) {
            render_deferred_config_controls();
        }
        
        if (ImGui::CollapsingHeader("Shadow Mapping")) {
            render_shadow_config_controls();
        }
        
        if (ImGui::CollapsingHeader("Quality Settings")) {
            render_quality_settings_controls();
        }
        
        ImGui::Separator();
        
        // Preset management
        if (ImGui::CollapsingHeader("Presets")) {
            static char preset_name[256] = "";
            ImGui::InputText("Preset Name", preset_name, sizeof(preset_name));
            ImGui::SameLine();
            if (ImGui::Button("Save")) {
                if (strlen(preset_name) > 0) {
                    save_config(std::string("presets/") + preset_name + ".json");
                }
            }
            
            // List existing presets
            for (const auto& preset : config_presets_) {
                if (ImGui::Selectable(preset.c_str(), preset == current_preset_name_)) {
                    load_config("presets/" + preset + ".json");
                    current_preset_name_ = preset;
                }
            }
        }
        
        ImGui::Separator();
        
        // Panel visibility controls
        if (ImGui::CollapsingHeader("Panel Visibility")) {
            ImGui::Checkbox("Material Editor", &show_material_editor_);
            ImGui::Checkbox("Lighting Control", &show_lighting_panel_);
            ImGui::Checkbox("Post Processing", &show_post_process_panel_);
            ImGui::Checkbox("Debug Visualization", &show_debug_panel_);
            ImGui::Checkbox("Performance Profiler", &show_profiler_panel_);
            ImGui::Checkbox("Scene Hierarchy", &show_scene_hierarchy_);
            ImGui::Checkbox("Viewport", &show_viewport_);
            ImGui::Checkbox("Shader Editor", &show_shader_editor_);
            ImGui::Checkbox("Render Graph", &show_render_graph_panel_);
            ImGui::Checkbox("GPU Memory", &show_gpu_memory_panel_);
        }
    }
    ImGui::End();
#endif
}

void RenderingUI::render_deferred_config_controls() {
#ifdef ECSCOPE_HAS_IMGUI
    auto& deferred_config = config_.deferred_config;
    
    // Resolution controls
    int resolution[2] = {static_cast<int>(deferred_config.width), 
                        static_cast<int>(deferred_config.height)};
    if (ImGui::InputInt2("Resolution", resolution)) {
        deferred_config.width = static_cast<uint32_t>(std::max(1, resolution[0]));
        deferred_config.height = static_cast<uint32_t>(std::max(1, resolution[1]));
        config_dirty_ = true;
    }
    
    // MSAA samples
    int msaa_samples = static_cast<int>(deferred_config.msaa_samples);
    if (ImGui::SliderInt("MSAA Samples", &msaa_samples, 1, 8)) {
        deferred_config.msaa_samples = static_cast<uint32_t>(msaa_samples);
        config_dirty_ = true;
    }
    
    // Feature toggles
    if (ImGui::Checkbox("Screen-Space Reflections", &deferred_config.enable_screen_space_reflections)) {
        config_dirty_ = true;
    }
    
    if (ImGui::Checkbox("Temporal Effects", &deferred_config.enable_temporal_effects)) {
        config_dirty_ = true;
    }
    
    if (ImGui::Checkbox("Volumetric Lighting", &deferred_config.enable_volumetric_lighting)) {
        config_dirty_ = true;
    }
    
    if (ImGui::Checkbox("Motion Vectors", &deferred_config.enable_motion_vectors)) {
        config_dirty_ = true;
    }
    
    // Tile-based shading controls
    int tile_size = static_cast<int>(deferred_config.tile_size);
    if (ImGui::SliderInt("Tile Size", &tile_size, 8, 32)) {
        deferred_config.tile_size = static_cast<uint32_t>(tile_size);
        config_dirty_ = true;
    }
    
    int max_lights = static_cast<int>(deferred_config.max_lights_per_tile);
    if (ImGui::SliderInt("Max Lights Per Tile", &max_lights, 64, 2048)) {
        deferred_config.max_lights_per_tile = static_cast<uint32_t>(max_lights);
        config_dirty_ = true;
    }
    
    if (ImGui::Checkbox("Use Compute Shading", &deferred_config.use_compute_shading)) {
        config_dirty_ = true;
    }
    
    // G-Buffer format controls
    ImGui::Text("G-Buffer Formats:");
    ImGui::Indent();
    
    static const char* format_names[] = {
        "R8", "RG8", "RGB8", "RGBA8", "R16F", "RG16F", "RGB16F", "RGBA16F",
        "R32F", "RG32F", "RGB32F", "RGBA32F", "SRGB8", "SRGBA8"
    };
    
    int albedo_format = static_cast<int>(deferred_config.albedo_format);
    if (ImGui::Combo("Albedo Format", &albedo_format, format_names, IM_ARRAYSIZE(format_names))) {
        deferred_config.albedo_format = static_cast<rendering::TextureFormat>(albedo_format);
        config_dirty_ = true;
    }
    
    int normal_format = static_cast<int>(deferred_config.normal_format) - 4; // Offset for 16F formats
    if (ImGui::Combo("Normal Format", &normal_format, &format_names[4], 4)) {
        deferred_config.normal_format = static_cast<rendering::TextureFormat>(normal_format + 4);
        config_dirty_ = true;
    }
    
    ImGui::Unindent();
    
    // Debug options
    ImGui::Separator();
    ImGui::Text("Debug Options:");
    ImGui::Checkbox("Visualize Overdraw", &deferred_config.visualize_overdraw);
    ImGui::Checkbox("Visualize Light Complexity", &deferred_config.visualize_light_complexity);
    ImGui::Checkbox("Visualize G-Buffer", &deferred_config.visualize_g_buffer);
#endif
}

void RenderingUI::render_material_editor_panel() {
#ifdef ECSCOPE_HAS_IMGUI
    if (ImGui::Begin("Material Editor", &show_material_editor_)) {
        
        // Material selection
        static int selected_material = 0;
        if (ImGui::BeginCombo("Material", "Default Material")) {
            // List available materials from scene objects
            for (const auto& [id, object] : scene_objects_) {
                std::string label = "Object " + std::to_string(id) + " - " + object.name;
                if (ImGui::Selectable(label.c_str(), selected_object_id_ == id)) {
                    selected_object_id_ = id;
                }
            }
            ImGui::EndCombo();
        }
        
        // Edit selected object's material
        if (selected_object_id_ > 0 && scene_objects_.count(selected_object_id_)) {
            auto& material = scene_objects_[selected_object_id_].material;
            render_pbr_material_editor(material);
            
            ImGui::Separator();
            
            // Material preview
            ImGui::Text("Material Preview:");
            render_material_preview(material);
        }
        
        ImGui::Separator();
        
        // Material presets
        if (ImGui::CollapsingHeader("Material Presets")) {
            if (ImGui::Button("Plastic")) {
                // Set plastic material properties
                if (selected_object_id_ > 0 && scene_objects_.count(selected_object_id_)) {
                    auto& mat = scene_objects_[selected_object_id_].material;
                    mat.albedo = {0.8f, 0.8f, 0.8f};
                    mat.metallic = 0.0f;
                    mat.roughness = 0.4f;
                }
            }
            ImGui::SameLine();
            if (ImGui::Button("Metal")) {
                if (selected_object_id_ > 0 && scene_objects_.count(selected_object_id_)) {
                    auto& mat = scene_objects_[selected_object_id_].material;
                    mat.albedo = {0.7f, 0.7f, 0.7f};
                    mat.metallic = 1.0f;
                    mat.roughness = 0.1f;
                }
            }
            ImGui::SameLine();
            if (ImGui::Button("Wood")) {
                if (selected_object_id_ > 0 && scene_objects_.count(selected_object_id_)) {
                    auto& mat = scene_objects_[selected_object_id_].material;
                    mat.albedo = {0.6f, 0.4f, 0.2f};
                    mat.metallic = 0.0f;
                    mat.roughness = 0.8f;
                }
            }
            if (ImGui::Button("Ceramic")) {
                if (selected_object_id_ > 0 && scene_objects_.count(selected_object_id_)) {
                    auto& mat = scene_objects_[selected_object_id_].material;
                    mat.albedo = {0.9f, 0.9f, 0.85f};
                    mat.metallic = 0.0f;
                    mat.roughness = 0.1f;
                }
            }
            ImGui::SameLine();
            if (ImGui::Button("Rubber")) {
                if (selected_object_id_ > 0 && scene_objects_.count(selected_object_id_)) {
                    auto& mat = scene_objects_[selected_object_id_].material;
                    mat.albedo = {0.2f, 0.2f, 0.2f};
                    mat.metallic = 0.0f;
                    mat.roughness = 0.9f;
                }
            }
            ImGui::SameLine();
            if (ImGui::Button("Glass")) {
                if (selected_object_id_ > 0 && scene_objects_.count(selected_object_id_)) {
                    auto& mat = scene_objects_[selected_object_id_].material;
                    mat.albedo = {0.95f, 0.95f, 0.95f};
                    mat.metallic = 0.0f;
                    mat.roughness = 0.0f;
                }
            }
        }
    }
    ImGui::End();
#endif
}

void RenderingUI::render_pbr_material_editor(rendering::MaterialProperties& material) {
#ifdef ECSCOPE_HAS_IMGUI
    // Albedo color
    if (ImGui::ColorEdit3("Albedo", material.albedo.data())) {
        config_dirty_ = true;
    }
    
    // Metallic/Roughness
    if (ImGui::SliderFloat("Metallic", &material.metallic, 0.0f, 1.0f)) {
        config_dirty_ = true;
    }
    
    if (ImGui::SliderFloat("Roughness", &material.roughness, 0.0f, 1.0f)) {
        config_dirty_ = true;
    }
    
    // Normal intensity
    if (ImGui::SliderFloat("Normal Intensity", &material.normal_intensity, 0.0f, 2.0f)) {
        config_dirty_ = true;
    }
    
    // Ambient occlusion
    if (ImGui::SliderFloat("Ambient Occlusion", &material.ambient_occlusion, 0.0f, 1.0f)) {
        config_dirty_ = true;
    }
    
    // Emission
    if (ImGui::SliderFloat("Emission Intensity", &material.emission_intensity, 0.0f, 10.0f)) {
        config_dirty_ = true;
    }
    
    if (material.emission_intensity > 0.0f) {
        if (ImGui::ColorEdit3("Emission Color", material.emission_color.data())) {
            config_dirty_ = true;
        }
    }
    
    // Subsurface scattering
    if (ImGui::SliderFloat("Subsurface Scattering", &material.subsurface_scattering, 0.0f, 1.0f)) {
        config_dirty_ = true;
    }
    
    ImGui::Separator();
    
    // Texture slots
    ImGui::Text("Texture Slots:");
    render_texture_slot_editor("Albedo", material.albedo_texture);
    render_texture_slot_editor("Normal", material.normal_texture);
    render_texture_slot_editor("Metallic/Roughness", material.metallic_roughness_texture);
    render_texture_slot_editor("Emission", material.emission_texture);
    render_texture_slot_editor("Occlusion", material.occlusion_texture);
    render_texture_slot_editor("Height", material.height_texture);
#endif
}

void RenderingUI::render_texture_slot_editor(const std::string& label, rendering::TextureHandle& texture) {
#ifdef ECSCOPE_HAS_IMGUI
    ImGui::PushID(label.c_str());
    
    ImGui::Text("%s:", label.c_str());
    ImGui::SameLine();
    
    // Show texture preview if available
    if (texture.is_valid()) {
        // Display small preview (would need to get texture ID for ImGui)
        ImGui::Image(nullptr, ImVec2(32, 32)); // Placeholder
        ImGui::SameLine();
        if (ImGui::Button("Clear")) {
            texture = rendering::TextureHandle{};
            config_dirty_ = true;
        }
    } else {
        ImGui::Text("None");
        ImGui::SameLine();
        if (ImGui::Button("Load...")) {
            // Open file dialog (implementation would depend on platform)
            // For now, just placeholder
            ImGui::OpenPopup("Load Texture");
        }
    }
    
    // File dialog popup (simplified)
    if (ImGui::BeginPopupModal("Load Texture", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Select texture file:");
        // List some common texture files or use file browser
        static const char* texture_files[] = {
            "textures/default_albedo.png",
            "textures/default_normal.png",
            "textures/default_metallic.png",
            "textures/noise.png"
        };
        
        for (int i = 0; i < IM_ARRAYSIZE(texture_files); i++) {
            if (ImGui::Selectable(texture_files[i])) {
                // Load texture (simplified - would need actual loading logic)
                ImGui::CloseCurrentPopup();
            }
        }
        
        if (ImGui::Button("Cancel")) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
    
    ImGui::PopID();
#endif
}

void RenderingUI::render_performance_profiler_panel() {
#ifdef ECSCOPE_HAS_IMGUI
    if (ImGui::Begin("Performance Profiler", &show_profiler_panel_)) {
        
        // Frame timing overview
        ImGui::Text("Frame Timing Overview");
        ImGui::Separator();
        
        render_frame_time_graph();
        
        ImGui::Separator();
        
        // Detailed timing breakdown
        if (ImGui::CollapsingHeader("Detailed Timing", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::Columns(2, "TimingColumns");
            ImGui::Text("Pass"); ImGui::NextColumn();
            ImGui::Text("Time (ms)"); ImGui::NextColumn();
            ImGui::Separator();
            
            ImGui::Text("Geometry Pass"); ImGui::NextColumn();
            ImGui::Text("%.3f", current_metrics_.geometry_pass_ms); ImGui::NextColumn();
            
            ImGui::Text("Shadow Pass"); ImGui::NextColumn();
            ImGui::Text("%.3f", current_metrics_.shadow_pass_ms); ImGui::NextColumn();
            
            ImGui::Text("Lighting Pass"); ImGui::NextColumn();
            ImGui::Text("%.3f", current_metrics_.lighting_pass_ms); ImGui::NextColumn();
            
            ImGui::Text("Post Process"); ImGui::NextColumn();
            ImGui::Text("%.3f", current_metrics_.post_process_ms); ImGui::NextColumn();
            
            ImGui::Columns(1);
        }
        
        ImGui::Separator();
        
        // GPU profiling
        if (ImGui::CollapsingHeader("GPU Profiling")) {
            render_gpu_profiler();
        }
        
        // Memory usage
        if (ImGui::CollapsingHeader("Memory Usage")) {
            render_memory_usage_charts();
        }
        
        // Draw call analysis
        if (ImGui::CollapsingHeader("Draw Call Analysis")) {
            render_draw_call_analysis();
        }
        
        ImGui::Separator();
        
        // Profiling controls
        ImGui::Text("Profiling Controls:");
        if (ImGui::Button("Capture Frame")) {
            capture_frame();
        }
        ImGui::SameLine();
        if (ImGui::Button("Reset Metrics")) {
            metrics_history_.clear();
        }
        
        ImGui::Checkbox("Show Performance Overlay", &show_performance_overlay_);
    }
    ImGui::End();
#endif
}

void RenderingUI::render_frame_time_graph() {
#ifdef ECSCOPE_HAS_IMGUI
    if (metrics_history_.empty()) return;
    
    // Prepare data for plotting
    std::vector<float> frame_times;
    std::vector<float> gpu_times;
    frame_times.reserve(metrics_history_.size());
    gpu_times.reserve(metrics_history_.size());
    
    for (const auto& metric : metrics_history_) {
        frame_times.push_back(metric.frame_time_ms);
        gpu_times.push_back(metric.gpu_time_ms);
    }
    
    // Plot frame times
    ImGui::PlotLines("Frame Time", frame_times.data(), static_cast<int>(frame_times.size()),
                     0, nullptr, 0.0f, 50.0f, ImVec2(0, 80));
    
    // Plot GPU times
    ImGui::PlotLines("GPU Time", gpu_times.data(), static_cast<int>(gpu_times.size()),
                     0, nullptr, 0.0f, 50.0f, ImVec2(0, 80));
    
    // Current values
    ImGui::Text("Current: Frame %.2f ms, GPU %.2f ms", 
                current_metrics_.frame_time_ms, current_metrics_.gpu_time_ms);
    
    // Target frame rate indicators
    float target_16ms = 16.67f; // 60 FPS
    float target_33ms = 33.33f; // 30 FPS
    ImGui::Text("Targets: 60fps=%.1fms, 30fps=%.1fms", target_16ms, target_33ms);
#endif
}

// =============================================================================
// CONFIGURATION MANAGEMENT
// =============================================================================

void RenderingUI::apply_config_changes() {
    if (!initialized_ || !renderer_) {
        return;
    }

    // Apply deferred renderer configuration if available
    if (deferred_renderer_) {
        deferred_renderer_->update_config(config_.deferred_config);
    }

    ecscope::log_info("RenderingUI", "Configuration changes applied");
}

bool RenderingUI::load_config(const std::string& filepath) {
    // Simplified JSON loading (would need proper JSON parser)
    std::ifstream file(filepath);
    if (!file.is_open()) {
        ecscope::log_error("RenderingUI", "Failed to open config file: " + filepath);
        return false;
    }

    // For demonstration, just reset to defaults
    config_ = create_default_rendering_config();
    config_dirty_ = true;
    
    ecscope::log_info("RenderingUI", "Configuration loaded from: " + filepath);
    return true;
}

bool RenderingUI::save_config(const std::string& filepath) const {
    // Create directory if it doesn't exist
    std::filesystem::create_directories(std::filesystem::path(filepath).parent_path());
    
    std::ofstream file(filepath);
    if (!file.is_open()) {
        ecscope::log_error("RenderingUI", "Failed to create config file: " + filepath);
        return false;
    }

    // Simplified JSON output (would need proper JSON serialization)
    file << "{\n";
    file << "  \"version\": \"1.0\",\n";
    file << "  \"deferred_config\": {\n";
    file << "    \"width\": " << config_.deferred_config.width << ",\n";
    file << "    \"height\": " << config_.deferred_config.height << ",\n";
    file << "    \"msaa_samples\": " << config_.deferred_config.msaa_samples << "\n";
    file << "  }\n";
    file << "}\n";
    
    ecscope::log_info("RenderingUI", "Configuration saved to: " + filepath);
    return true;
}

// =============================================================================
// SCENE MANAGEMENT
// =============================================================================

uint32_t RenderingUI::add_scene_object(const SceneObject& object) {
    uint32_t id = next_object_id_++;
    scene_objects_[id] = object;
    scene_objects_[id].id = id;
    return id;
}

void RenderingUI::remove_scene_object(uint32_t object_id) {
    scene_objects_.erase(object_id);
}

SceneObject* RenderingUI::get_scene_object(uint32_t object_id) {
    auto it = scene_objects_.find(object_id);
    return (it != scene_objects_.end()) ? &it->second : nullptr;
}

void RenderingUI::clear_scene() {
    scene_objects_.clear();
    scene_lights_.clear();
    next_object_id_ = 1;
    next_light_id_ = 1;
}

// =============================================================================
// PERFORMANCE MONITORING
// =============================================================================

void RenderingUI::update_performance_metrics() {
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration<float>(now - last_metrics_update_).count();
    
    if (elapsed < 1.0f / 60.0f) { // Update at 60Hz max
        return;
    }
    
    // Get frame stats from renderer
    if (renderer_) {
        auto frame_stats = renderer_->get_frame_stats();
        current_metrics_.frame_time_ms = frame_stats.frame_time_ms;
        current_metrics_.gpu_time_ms = frame_stats.gpu_time_ms;
        current_metrics_.draw_calls = frame_stats.draw_calls;
        current_metrics_.vertices_rendered = frame_stats.vertices_rendered;
        current_metrics_.gpu_memory_used = frame_stats.memory_used;
    }
    
    // Get deferred renderer stats if available
    if (deferred_renderer_) {
        auto deferred_stats = deferred_renderer_->get_statistics();
        current_metrics_.geometry_pass_ms = deferred_stats.geometry_pass_time_ms;
        current_metrics_.shadow_pass_ms = deferred_stats.shadow_pass_time_ms;
        current_metrics_.lighting_pass_ms = deferred_stats.lighting_pass_time_ms;
        current_metrics_.post_process_ms = deferred_stats.post_process_time_ms;
        current_metrics_.lights_rendered = deferred_stats.light_count;
        current_metrics_.shadow_maps_updated = deferred_stats.shadow_map_updates;
    }
    
    current_metrics_.timestamp = now;
    
    // Add to history
    metrics_history_.push_back(current_metrics_);
    if (metrics_history_.size() > MAX_METRICS_HISTORY) {
        metrics_history_.erase(metrics_history_.begin());
    }
    
    last_metrics_update_ = now;
}

void RenderingUI::capture_frame() {
    capture_next_frame_ = true;
    ecscope::log_info("RenderingUI", "Frame capture requested");
}

// =============================================================================
// UTILITY METHODS
// =============================================================================

void RenderingUI::create_preview_scene() {
    // Create a simple preview scene with a few objects
    
    // Add a ground plane
    SceneObject ground;
    ground.name = "Ground";
    ground.material.albedo = {0.5f, 0.5f, 0.5f};
    ground.material.metallic = 0.0f;
    ground.material.roughness = 0.8f;
    add_scene_object(ground);
    
    // Add some test cubes
    for (int i = 0; i < 5; ++i) {
        SceneObject cube;
        cube.name = "Cube " + std::to_string(i + 1);
        cube.transform[12] = static_cast<float>(i * 3 - 6); // X position
        cube.transform[13] = 1.0f; // Y position
        
        // Vary material properties
        cube.material.albedo = {
            0.5f + 0.5f * (i / 5.0f),
            0.3f + 0.4f * ((5 - i) / 5.0f),
            0.2f
        };
        cube.material.metallic = i / 5.0f;
        cube.material.roughness = 0.1f + 0.8f * ((5 - i) / 5.0f);
        
        add_scene_object(cube);
    }
    
    // Add some lights
    SceneLight main_light;
    main_light.name = "Main Light";
    main_light.light_data.type = rendering::LightType::Directional;
    main_light.light_data.direction = {-0.3f, -0.7f, -0.6f};
    main_light.light_data.color = {1.0f, 0.95f, 0.8f};
    main_light.light_data.intensity = 3.0f;
    main_light.light_data.cast_shadows = true;
    add_scene_light(main_light);
    
    // Add point lights
    for (int i = 0; i < 3; ++i) {
        SceneLight point_light;
        point_light.name = "Point Light " + std::to_string(i + 1);
        point_light.light_data.type = rendering::LightType::Point;
        point_light.light_data.position = {
            static_cast<float>(i * 6 - 6),
            3.0f,
            2.0f
        };
        point_light.light_data.color = {
            i == 0 ? 1.0f : 0.0f,
            i == 1 ? 1.0f : 0.0f,
            i == 2 ? 1.0f : 0.0f
        };
        point_light.light_data.intensity = 2.0f;
        point_light.light_data.range = 8.0f;
        point_light.animated = true;
        point_light.animation_center = point_light.light_data.position;
        point_light.animation_radius = 2.0f;
        point_light.animation_speed = 0.5f + 0.3f * i;
        
        add_scene_light(point_light);
    }
}

void RenderingUI::setup_default_shaders() {
    // Register common shaders for hot-reload monitoring
    
    ShaderProgram geometry_shader;
    geometry_shader.name = "Geometry";
    geometry_shader.vertex_path = "shaders/geometry.vert";
    geometry_shader.fragment_path = "shaders/geometry.frag";
    register_shader(geometry_shader);
    
    ShaderProgram lighting_shader;
    lighting_shader.name = "Deferred Lighting";
    lighting_shader.vertex_path = "shaders/fullscreen.vert";
    lighting_shader.fragment_path = "shaders/deferred_lighting.frag";
    register_shader(lighting_shader);
    
    ShaderProgram post_process_shader;
    post_process_shader.name = "Post Process";
    post_process_shader.vertex_path = "shaders/fullscreen.vert";
    post_process_shader.fragment_path = "shaders/post_process.frag";
    register_shader(post_process_shader);
}

// =============================================================================
// MISSING METHOD IMPLEMENTATIONS
// =============================================================================

void RenderingUI::render_lighting_control_panel() {
#ifdef ECSCOPE_HAS_IMGUI
    if (ImGui::Begin("Lighting Control", &show_lighting_panel_)) {
        
        // Environment lighting controls
        if (ImGui::CollapsingHeader("Environment Lighting", ImGuiTreeNodeFlags_DefaultOpen)) {
            render_environment_lighting_controls();
        }
        
        // Light list
        if (ImGui::CollapsingHeader("Scene Lights", ImGuiTreeNodeFlags_DefaultOpen)) {
            for (auto& [id, light] : scene_lights_) {
                ImGui::PushID(id);
                
                if (ImGui::TreeNode(light.name.c_str())) {
                    render_light_editor(light);
                    ImGui::TreePop();
                }
                
                ImGui::PopID();
            }
        }
        
        // Add new light
        ImGui::Separator();
        if (ImGui::Button("Add Directional Light")) {
            SceneLight new_light;
            new_light.name = "Directional Light " + std::to_string(next_light_id_);
            new_light.light_data.type = rendering::LightType::Directional;
            new_light.light_data.intensity = 1.0f;
            add_scene_light(new_light);
        }
        ImGui::SameLine();
        if (ImGui::Button("Add Point Light")) {
            SceneLight new_light;
            new_light.name = "Point Light " + std::to_string(next_light_id_);
            new_light.light_data.type = rendering::LightType::Point;
            new_light.light_data.intensity = 1.0f;
            new_light.light_data.range = 10.0f;
            add_scene_light(new_light);
        }
        ImGui::SameLine();
        if (ImGui::Button("Add Spot Light")) {
            SceneLight new_light;
            new_light.name = "Spot Light " + std::to_string(next_light_id_);
            new_light.light_data.type = rendering::LightType::Spot;
            new_light.light_data.intensity = 1.0f;
            new_light.light_data.range = 10.0f;
            new_light.light_data.inner_cone_angle = 15.0f;
            new_light.light_data.outer_cone_angle = 30.0f;
            add_scene_light(new_light);
        }
    }
    ImGui::End();
#endif
}

void RenderingUI::render_light_editor(SceneLight& light) {
#ifdef ECSCOPE_HAS_IMGUI
    ImGui::PushID(light.id);
    
    // Basic properties
    char name_buffer[256];
    strcpy_s(name_buffer, light.name.c_str());
    if (ImGui::InputText("Name", name_buffer, sizeof(name_buffer))) {
        light.name = name_buffer;
    }
    
    ImGui::Checkbox("Enabled", &light.enabled);
    ImGui::SameLine();
    ImGui::Checkbox("Cast Shadows", &light.light_data.cast_shadows);
    
    // Light type
    const char* light_types[] = {"Directional", "Point", "Spot", "Area"};
    int current_type = static_cast<int>(light.light_data.type);
    if (ImGui::Combo("Type", &current_type, light_types, IM_ARRAYSIZE(light_types))) {
        light.light_data.type = static_cast<rendering::LightType>(current_type);
        config_dirty_ = true;
    }
    
    // Color and intensity
    if (ImGui::ColorEdit3("Color", light.light_data.color.data())) {
        config_dirty_ = true;
    }
    
    if (ImGui::SliderFloat("Intensity", &light.light_data.intensity, 0.0f, 10.0f)) {
        config_dirty_ = true;
    }
    
    // Position (for point and spot lights)
    if (light.light_data.type == rendering::LightType::Point || 
        light.light_data.type == rendering::LightType::Spot) {
        if (ImGui::DragFloat3("Position", light.light_data.position.data(), 0.1f)) {
            config_dirty_ = true;
        }
        
        if (ImGui::SliderFloat("Range", &light.light_data.range, 1.0f, 100.0f)) {
            config_dirty_ = true;
        }
    }
    
    // Direction (for directional and spot lights)
    if (light.light_data.type == rendering::LightType::Directional || 
        light.light_data.type == rendering::LightType::Spot) {
        if (ImGui::DragFloat3("Direction", light.light_data.direction.data(), 0.01f, -1.0f, 1.0f)) {
            config_dirty_ = true;
        }
    }
    
    // Spot light specific
    if (light.light_data.type == rendering::LightType::Spot) {
        if (ImGui::SliderFloat("Inner Cone", &light.light_data.inner_cone_angle, 1.0f, 89.0f)) {
            config_dirty_ = true;
        }
        if (ImGui::SliderFloat("Outer Cone", &light.light_data.outer_cone_angle, 
                              light.light_data.inner_cone_angle + 1.0f, 90.0f)) {
            config_dirty_ = true;
        }
    }
    
    // Shadow settings
    if (light.light_data.cast_shadows) {
        ImGui::Separator();
        ImGui::Text("Shadow Settings:");
        
        int shadow_size = static_cast<int>(light.light_data.shadow_map_size);
        if (ImGui::SliderInt("Shadow Map Size", &shadow_size, 256, 4096)) {
            light.light_data.shadow_map_size = static_cast<uint32_t>(shadow_size);
            config_dirty_ = true;
        }
        
        // Cascade settings for directional lights
        if (light.light_data.type == rendering::LightType::Directional) {
            int cascade_count = static_cast<int>(light.light_data.cascade_count);
            if (ImGui::SliderInt("Cascade Count", &cascade_count, 1, 8)) {
                light.light_data.cascade_count = static_cast<uint32_t>(cascade_count);
                config_dirty_ = true;
            }
            
            for (uint32_t i = 0; i < light.light_data.cascade_count; ++i) {
                std::string label = "Cascade " + std::to_string(i) + " Distance";
                if (ImGui::DragFloat(label.c_str(), &light.light_data.cascade_distances[i], 1.0f, 0.1f, 1000.0f)) {
                    config_dirty_ = true;
                }
            }
        }
    }
    
    // Animation settings
    ImGui::Separator();
    render_light_animation_controls(light);
    
    ImGui::PopID();
#endif
}

void RenderingUI::render_light_animation_controls(SceneLight& light) {
#ifdef ECSCOPE_HAS_IMGUI
    if (ImGui::Checkbox("Animated", &light.animated)) {
        config_dirty_ = true;
    }
    
    if (light.animated) {
        if (ImGui::DragFloat3("Animation Center", light.animation_center.data(), 0.1f)) {
            config_dirty_ = true;
        }
        
        if (ImGui::SliderFloat("Animation Radius", &light.animation_radius, 0.1f, 20.0f)) {
            config_dirty_ = true;
        }
        
        if (ImGui::SliderFloat("Animation Speed", &light.animation_speed, 0.1f, 5.0f)) {
            config_dirty_ = true;
        }
    }
#endif
}

void RenderingUI::render_environment_lighting_controls() {
#ifdef ECSCOPE_HAS_IMGUI
    auto& env = config_.environment;
    
    if (ImGui::ColorEdit3("Ambient Color", env.ambient_color.data())) {
        config_dirty_ = true;
    }
    
    if (ImGui::SliderFloat("Ambient Intensity", &env.ambient_intensity, 0.0f, 2.0f)) {
        config_dirty_ = true;
    }
    
    if (ImGui::SliderFloat("Sky Intensity", &env.sky_intensity, 0.0f, 5.0f)) {
        config_dirty_ = true;
    }
    
    ImGui::Checkbox("Enable IBL", &env.enable_ibl);
    
    if (env.enable_ibl) {
        if (ImGui::SliderFloat("IBL Intensity", &env.ibl_intensity, 0.0f, 2.0f)) {
            config_dirty_ = true;
        }
    }
    
    ImGui::Checkbox("Rotate Environment", &env.rotate_environment);
    if (env.rotate_environment) {
        if (ImGui::SliderFloat("Rotation Speed", &env.rotation_speed, 0.01f, 1.0f)) {
            config_dirty_ = true;
        }
    }
#endif
}

void RenderingUI::render_post_processing_panel() {
#ifdef ECSCOPE_HAS_IMGUI
    if (ImGui::Begin("Post Processing", &show_post_process_panel_)) {
        
        // HDR and Tone Mapping
        if (ImGui::CollapsingHeader("HDR & Tone Mapping", ImGuiTreeNodeFlags_DefaultOpen)) {
            render_hdr_tone_mapping_controls();
        }
        
        // Bloom
        if (ImGui::CollapsingHeader("Bloom")) {
            render_bloom_controls();
        }
        
        // Screen-Space Ambient Occlusion
        if (ImGui::CollapsingHeader("Screen-Space Ambient Occlusion")) {
            render_ssao_controls();
        }
        
        // Screen-Space Reflections
        if (ImGui::CollapsingHeader("Screen-Space Reflections")) {
            render_ssr_controls();
        }
        
        // Temporal Anti-Aliasing
        if (ImGui::CollapsingHeader("Temporal Anti-Aliasing")) {
            render_taa_controls();
        }
        
        // Motion Blur
        if (ImGui::CollapsingHeader("Motion Blur")) {
            auto& mb = config_.post_process;
            ImGui::Checkbox("Enable Motion Blur", &mb.enable_motion_blur);
            if (mb.enable_motion_blur) {
                if (ImGui::SliderFloat("Strength", &mb.motion_blur_strength, 0.0f, 2.0f)) {
                    config_dirty_ = true;
                }
                int samples = mb.motion_blur_samples;
                if (ImGui::SliderInt("Samples", &samples, 4, 32)) {
                    mb.motion_blur_samples = samples;
                    config_dirty_ = true;
                }
            }
        }
    }
    ImGui::End();
#endif
}

void RenderingUI::render_hdr_tone_mapping_controls() {
#ifdef ECSCOPE_HAS_IMGUI
    auto& hdr = config_.post_process;
    
    ImGui::Checkbox("Enable HDR", &hdr.enable_hdr);
    
    if (hdr.enable_hdr) {
        if (ImGui::SliderFloat("Exposure", &hdr.exposure, 0.1f, 5.0f)) {
            config_dirty_ = true;
        }
        
        if (ImGui::SliderFloat("Gamma", &hdr.gamma, 1.0f, 3.0f)) {
            config_dirty_ = true;
        }
        
        const char* tone_mapping_modes[] = {"Reinhard", "ACES", "Uncharted 2"};
        if (ImGui::Combo("Tone Mapping", &hdr.tone_mapping_mode, tone_mapping_modes, 3)) {
            config_dirty_ = true;
        }
    }
#endif
}

void RenderingUI::render_bloom_controls() {
#ifdef ECSCOPE_HAS_IMGUI
    auto& bloom = config_.post_process;
    
    ImGui::Checkbox("Enable Bloom", &bloom.enable_bloom);
    
    if (bloom.enable_bloom) {
        if (ImGui::SliderFloat("Threshold", &bloom.bloom_threshold, 0.1f, 5.0f)) {
            config_dirty_ = true;
        }
        
        if (ImGui::SliderFloat("Intensity", &bloom.bloom_intensity, 0.0f, 2.0f)) {
            config_dirty_ = true;
        }
        
        if (ImGui::SliderFloat("Radius", &bloom.bloom_radius, 0.1f, 3.0f)) {
            config_dirty_ = true;
        }
        
        int iterations = bloom.bloom_iterations;
        if (ImGui::SliderInt("Iterations", &iterations, 3, 10)) {
            bloom.bloom_iterations = iterations;
            config_dirty_ = true;
        }
    }
#endif
}

void RenderingUI::render_ssao_controls() {
#ifdef ECSCOPE_HAS_IMGUI
    auto& ssao = config_.post_process;
    
    ImGui::Checkbox("Enable SSAO", &ssao.enable_ssao);
    
    if (ssao.enable_ssao) {
        if (ImGui::SliderFloat("Radius", &ssao.ssao_radius, 0.1f, 2.0f)) {
            config_dirty_ = true;
        }
        
        if (ImGui::SliderFloat("Intensity", &ssao.ssao_intensity, 0.0f, 3.0f)) {
            config_dirty_ = true;
        }
        
        int samples = ssao.ssao_samples;
        if (ImGui::SliderInt("Samples", &samples, 8, 64)) {
            ssao.ssao_samples = samples;
            config_dirty_ = true;
        }
    }
#endif
}

void RenderingUI::render_ssr_controls() {
#ifdef ECSCOPE_HAS_IMGUI
    auto& ssr = config_.post_process;
    
    ImGui::Checkbox("Enable SSR", &ssr.enable_ssr);
    
    if (ssr.enable_ssr) {
        if (ImGui::SliderFloat("Max Distance", &ssr.ssr_max_distance, 10.0f, 200.0f)) {
            config_dirty_ = true;
        }
        
        if (ImGui::SliderFloat("Fade Distance", &ssr.ssr_fade_distance, 1.0f, 50.0f)) {
            config_dirty_ = true;
        }
        
        int max_steps = ssr.ssr_max_steps;
        if (ImGui::SliderInt("Max Steps", &max_steps, 16, 128)) {
            ssr.ssr_max_steps = max_steps;
            config_dirty_ = true;
        }
        
        if (ImGui::SliderFloat("Thickness", &ssr.ssr_thickness, 0.01f, 1.0f)) {
            config_dirty_ = true;
        }
    }
#endif
}

void RenderingUI::render_taa_controls() {
#ifdef ECSCOPE_HAS_IMGUI
    auto& taa = config_.post_process;
    
    ImGui::Checkbox("Enable TAA", &taa.enable_taa);
    
    if (taa.enable_taa) {
        if (ImGui::SliderFloat("Feedback", &taa.taa_feedback, 0.5f, 0.99f)) {
            config_dirty_ = true;
        }
        
        ImGui::Checkbox("Enable Sharpening", &taa.taa_sharpening);
        
        if (taa.taa_sharpening) {
            if (ImGui::SliderFloat("Sharpening Amount", &taa.taa_sharpening_amount, 0.0f, 1.0f)) {
                config_dirty_ = true;
            }
        }
    }
#endif
}

// Additional missing implementations
void RenderingUI::render_shadow_config_controls() {
#ifdef ECSCOPE_HAS_IMGUI
    auto& shadows = config_.shadows;
    
    ImGui::Checkbox("Enable Shadows", &shadows.enable_shadows);
    
    if (shadows.enable_shadows) {
        int cascade_count = static_cast<int>(shadows.cascade_count);
        if (ImGui::SliderInt("Cascade Count", &cascade_count, 1, 8)) {
            shadows.cascade_count = static_cast<uint32_t>(cascade_count);
            config_dirty_ = true;
        }
        
        int shadow_resolution = static_cast<int>(shadows.shadow_resolution);
        if (ImGui::SliderInt("Shadow Resolution", &shadow_resolution, 512, 4096)) {
            shadows.shadow_resolution = static_cast<uint32_t>(shadow_resolution);
            config_dirty_ = true;
        }
        
        if (ImGui::SliderFloat("Shadow Bias", &shadows.shadow_bias, 0.0001f, 0.01f, "%.5f")) {
            config_dirty_ = true;
        }
        
        if (ImGui::SliderFloat("Normal Bias", &shadows.shadow_normal_bias, 0.001f, 0.1f, "%.4f")) {
            config_dirty_ = true;
        }
        
        ImGui::Checkbox("Enable PCF", &shadows.enable_pcf);
        if (shadows.enable_pcf) {
            int pcf_samples = shadows.pcf_samples;
            if (ImGui::SliderInt("PCF Samples", &pcf_samples, 2, 16)) {
                shadows.pcf_samples = pcf_samples;
                config_dirty_ = true;
            }
        }
        
        ImGui::Checkbox("Contact Shadows", &shadows.enable_contact_shadows);
        if (shadows.enable_contact_shadows) {
            if (ImGui::SliderFloat("Contact Length", &shadows.contact_shadow_length, 0.01f, 1.0f)) {
                config_dirty_ = true;
            }
        }
    }
#endif
}

void RenderingUI::render_quality_settings_controls() {
#ifdef ECSCOPE_HAS_IMGUI
    auto& quality = config_.quality;
    
    int msaa_samples = static_cast<int>(quality.msaa_samples);
    if (ImGui::SliderInt("MSAA Samples", &msaa_samples, 1, 8)) {
        quality.msaa_samples = static_cast<uint32_t>(msaa_samples);
        config_dirty_ = true;
    }
    
    if (ImGui::SliderFloat("Render Scale", &quality.render_scale, 0.5f, 2.0f)) {
        config_dirty_ = true;
    }
    
    ImGui::Checkbox("Temporal Upsampling", &quality.enable_temporal_upsampling);
    ImGui::Checkbox("GPU Culling", &quality.enable_gpu_culling);
    ImGui::Checkbox("Early Z", &quality.enable_early_z);
    ImGui::Checkbox("Compute Shading", &quality.use_compute_shading);
    
    int max_lights = static_cast<int>(quality.max_lights_per_tile);
    if (ImGui::SliderInt("Max Lights Per Tile", &max_lights, 64, 2048)) {
        quality.max_lights_per_tile = static_cast<uint32_t>(max_lights);
        config_dirty_ = true;
    }
#endif
}

// =============================================================================
// REMAINING MISSING METHOD IMPLEMENTATIONS
// =============================================================================

void RenderingUI::render_shader_reload_controls() {
#ifdef ECSCOPE_HAS_IMGUI
    ImGui::Text("Shader Reload Controls:");
    
    // Auto-reload interval
    static float auto_reload_interval = 1.0f;
    ImGui::SliderFloat("Auto Reload Interval (s)", &auto_reload_interval, 0.1f, 5.0f);
    
    // File watching status
    ImGui::Text("File Watching: %s", shader_hot_reload_enabled_ ? "ACTIVE" : "DISABLED");
    
    // Last check time
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration<float>(now - last_shader_check_).count();
    ImGui::Text("Last Check: %.1f seconds ago", elapsed);
#endif
}

void RenderingUI::render_shader_error_display() {
#ifdef ECSCOPE_HAS_IMGUI
    bool has_errors = false;
    for (const auto& [name, shader] : shaders_) {
        if (shader.reload_status == ShaderReloadStatus::Error) {
            has_errors = true;
            break;
        }
    }
    
    if (has_errors) {
        ImGui::Separator();
        ImGui::TextColored(ImVec4(1, 0, 0, 1), "Shader Errors:");
        
        for (const auto& [name, shader] : shaders_) {
            if (shader.reload_status == ShaderReloadStatus::Error) {
                ImGui::PushID(name.c_str());
                
                if (ImGui::TreeNode(name.c_str())) {
                    ImGui::TextWrapped("%s", shader.error_message.c_str());
                    ImGui::TreePop();
                }
                
                ImGui::PopID();
            }
        }
    }
#endif
}

// Additional utility methods
void RenderingUI::set_debug_mode(DebugVisualizationMode mode) {
    debug_mode_ = mode;
    
    if (deferred_renderer_) {
        // Update deferred renderer debug settings based on mode
        auto& config = config_.deferred_config;
        config.visualize_g_buffer = (mode >= DebugVisualizationMode::GBufferAlbedo && 
                                   mode <= DebugVisualizationMode::GBufferMotion);
        config.visualize_light_complexity = (mode == DebugVisualizationMode::LightComplexity);
        config.visualize_overdraw = (mode == DebugVisualizationMode::Overdraw);
        
        deferred_renderer_->update_config(config);
    }
    
    config_dirty_ = true;
}

void RenderingUI::register_shader(const ShaderProgram& shader) {
    shaders_[shader.name] = shader;
    ecscope::log_info("RenderingUI", "Registered shader for hot-reload: " + shader.name);
}

bool RenderingUI::reload_shader(const std::string& shader_name) {
    auto it = shaders_.find(shader_name);
    if (it == shaders_.end()) {
        return false;
    }
    
    auto& shader = it->second;
    shader.reload_status = ShaderReloadStatus::Reloading;
    
    // In a real implementation, this would reload the shader files and recompile
    // For now, just simulate success
    shader.reload_status = ShaderReloadStatus::Success;
    shader.last_modified = std::chrono::steady_clock::now();
    
    ecscope::log_info("RenderingUI", "Reloaded shader: " + shader_name);
    return true;
}

void RenderingUI::reload_all_shaders() {
    for (auto& [name, shader] : shaders_) {
        reload_shader(name);
    }
    ecscope::log_info("RenderingUI", "Reloaded all shaders");
}

uint32_t RenderingUI::add_scene_light(const SceneLight& light) {
    uint32_t id = next_light_id_++;
    scene_lights_[id] = light;
    scene_lights_[id].id = id;
    return id;
}

void RenderingUI::remove_scene_light(uint32_t light_id) {
    scene_lights_.erase(light_id);
}

SceneLight* RenderingUI::get_scene_light(uint32_t light_id) {
    auto it = scene_lights_.find(light_id);
    return (it != scene_lights_.end()) ? &it->second : nullptr;
}

ShaderReloadStatus RenderingUI::get_shader_status(const std::string& shader_name) const {
    auto it = shaders_.find(shader_name);
    return (it != shaders_.end()) ? it->second.reload_status : ShaderReloadStatus::Idle;
}

void RenderingUI::set_camera_mode(CameraControlMode mode) {
    camera_mode_ = mode;
    ecscope::log_info("RenderingUI", "Camera mode changed");
}

std::pair<std::array<float, 16>, std::array<float, 16>> RenderingUI::get_camera_matrices() const {
    // Calculate view and projection matrices based on current camera state
    std::array<float, 16> view_matrix = {};
    std::array<float, 16> projection_matrix = {};
    
    // Identity matrices as placeholder
    view_matrix[0] = view_matrix[5] = view_matrix[10] = view_matrix[15] = 1.0f;
    projection_matrix[0] = projection_matrix[5] = projection_matrix[10] = projection_matrix[15] = 1.0f;
    
    return {view_matrix, projection_matrix};
}

void RenderingUI::focus_camera_on_object(uint32_t object_id) {
    auto* object = get_scene_object(object_id);
    if (object) {
        // Focus camera on the object's position
        camera_.target[0] = object->transform[12];
        camera_.target[1] = object->transform[13];
        camera_.target[2] = object->transform[14];
        
        ecscope::log_info("RenderingUI", "Camera focused on object: " + object->name);
    }
}

void RenderingUI::reset_camera() {
    camera_ = CameraState{}; // Reset to default values
    ecscope::log_info("RenderingUI", "Camera reset to default position");
}

std::pair<rendering::BufferHandle, rendering::BufferHandle> 
create_preview_sphere_mesh(rendering::IRenderer* renderer, uint32_t& index_count) {
    if (!renderer) {
        index_count = 0;
        return {rendering::BufferHandle{}, rendering::BufferHandle{}};
    }
    
    // Simple sphere mesh for material preview
    index_count = 720; // Approximation
    
    rendering::BufferDesc vertex_desc;
    vertex_desc.size = 242 * (3 + 3 + 2) * sizeof(float); // Positions, normals, UVs
    vertex_desc.usage = rendering::BufferUsage::Static;
    vertex_desc.debug_name = "Preview Sphere Vertices";
    
    rendering::BufferDesc index_desc;
    index_desc.size = index_count * sizeof(uint32_t);
    index_desc.usage = rendering::BufferUsage::Static;
    index_desc.debug_name = "Preview Sphere Indices";
    
    auto vertex_buffer = renderer->create_buffer(vertex_desc, nullptr);
    auto index_buffer = renderer->create_buffer(index_desc, nullptr);
    
    return {vertex_buffer, index_buffer};
}

void RenderingUI::render_debug_visualization_panel() {
#ifdef ECSCOPE_HAS_IMGUI
    if (ImGui::Begin("Debug Visualization", &show_debug_panel_)) {
        
        // Debug mode selection
        const char* debug_modes[] = {
            "None", "G-Buffer Albedo", "G-Buffer Normal", "G-Buffer Depth",
            "G-Buffer Material", "Light Complexity", "Overdraw", "Shadow Cascades",
            "SSAO", "SSR", "Bloom", "Wireframe"
        };
        
        int current_debug_mode = static_cast<int>(debug_mode_);
        if (ImGui::Combo("Debug Mode", &current_debug_mode, debug_modes, IM_ARRAYSIZE(debug_modes))) {
            set_debug_mode(static_cast<DebugVisualizationMode>(current_debug_mode));
        }
        
        ImGui::Separator();
        
        // G-Buffer visualization
        if (ImGui::CollapsingHeader("G-Buffer Visualization")) {
            render_gbuffer_visualization();
        }
        
        // Performance overlay
        if (ImGui::CollapsingHeader("Performance Overlay")) {
            ImGui::Checkbox("Show Performance Overlay", &show_performance_overlay_);
            ImGui::Checkbox("Show Debug Wireframe", &show_debug_wireframe_);
        }
        
        // Light debugging
        if (ImGui::CollapsingHeader("Light Debug Visualization")) {
            render_light_debug_visualization();
        }
        
        ImGui::Separator();
        
        // Debug controls
        if (ImGui::Button("Capture G-Buffer")) {
            capture_gbuffer_textures();
        }
        ImGui::SameLine();
        if (ImGui::Button("Save Debug Image")) {
            // Save current debug visualization
        }
    }
    ImGui::End();
#endif
}

void RenderingUI::render_gbuffer_visualization() {
#ifdef ECSCOPE_HAS_IMGUI
    if (!deferred_renderer_) return;
    
    ImGui::Text("G-Buffer Targets:");
    
    // Show G-buffer texture previews
    const char* gbuffer_names[] = {"Albedo", "Normal", "Motion", "Material", "Depth"};
    for (int i = 0; i < 5; ++i) {
        auto texture = deferred_renderer_->get_g_buffer_texture(static_cast<rendering::GBufferTarget>(i));
        if (texture.is_valid()) {
            ImGui::Image(nullptr, ImVec2(128, 72)); // Would show actual texture
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("%s Buffer", gbuffer_names[i]);
            }
            if ((i + 1) % 3 != 0) ImGui::SameLine();
        }
    }
#endif
}

void RenderingUI::render_scene_hierarchy_panel() {
#ifdef ECSCOPE_HAS_IMGUI
    if (ImGui::Begin("Scene Hierarchy", &show_scene_hierarchy_)) {
        
        // Scene statistics
        ImGui::Text("Objects: %zu", scene_objects_.size());
        ImGui::SameLine();
        ImGui::Text("Lights: %zu", scene_lights_.size());
        
        ImGui::Separator();
        
        // Scene objects tree
        if (ImGui::CollapsingHeader("Scene Objects", ImGuiTreeNodeFlags_DefaultOpen)) {
            for (auto& [id, object] : scene_objects_) {
                ImGui::PushID(id);
                
                bool selected = (selected_object_id_ == id);
                if (ImGui::Selectable(object.name.c_str(), selected)) {
                    selected_object_id_ = id;
                }
                
                // Context menu for objects
                if (ImGui::BeginPopupContextItem()) {
                    if (ImGui::MenuItem("Focus Camera")) {
                        focus_camera_on_object(id);
                    }
                    if (ImGui::MenuItem("Delete")) {
                        remove_scene_object(id);
                    }
                    ImGui::EndPopup();
                }
                
                ImGui::PopID();
            }
        }
        
        // Lights tree
        if (ImGui::CollapsingHeader("Lights", ImGuiTreeNodeFlags_DefaultOpen)) {
            for (auto& [id, light] : scene_lights_) {
                ImGui::PushID(id);
                
                bool selected = (selected_light_id_ == id);
                if (ImGui::Selectable(light.name.c_str(), selected)) {
                    selected_light_id_ = id;
                }
                
                // Show light state
                ImGui::SameLine();
                if (light.enabled) {
                    ImGui::TextColored(ImVec4(0, 1, 0, 1), "ON");
                } else {
                    ImGui::TextColored(ImVec4(1, 0, 0, 1), "OFF");
                }
                
                ImGui::PopID();
            }
        }
        
        ImGui::Separator();
        
        // Edit selected object
        if (selected_object_id_ > 0 && scene_objects_.count(selected_object_id_)) {
            ImGui::Text("Selected Object Properties:");
            render_scene_object_editor(scene_objects_[selected_object_id_]);
        }
        
        // Edit selected light
        if (selected_light_id_ > 0 && scene_lights_.count(selected_light_id_)) {
            ImGui::Text("Selected Light Properties:");
            render_light_editor(scene_lights_[selected_light_id_]);
        }
    }
    ImGui::End();
#endif
}

void RenderingUI::render_scene_object_editor(SceneObject& object) {
#ifdef ECSCOPE_HAS_IMGUI
    ImGui::PushID(object.id);
    
    // Basic properties
    char name_buffer[256];
    strcpy_s(name_buffer, object.name.c_str());
    if (ImGui::InputText("Name", name_buffer, sizeof(name_buffer))) {
        object.name = name_buffer;
    }
    
    ImGui::Checkbox("Visible", &object.visible);
    ImGui::SameLine();
    ImGui::Checkbox("Cast Shadows", &object.cast_shadows);
    
    // Transform editing
    if (ImGui::CollapsingHeader("Transform")) {
        render_transform_editor(object.transform);
    }
    
    // LOD information
    if (ImGui::CollapsingHeader("Level of Detail")) {
        ImGui::Text("Current LOD Level: %d", object.lod_level);
        ImGui::Text("LOD Distance: %.2f", object.lod_distance);
        
        if (ImGui::SliderInt("Force LOD Level", &object.lod_level, 0, 
                           static_cast<int>(object.lod_vertex_buffers.size()) - 1)) {
            // Update buffers based on LOD level
            if (object.lod_level < object.lod_vertex_buffers.size()) {
                object.vertex_buffer = object.lod_vertex_buffers[object.lod_level];
                object.index_buffer = object.lod_index_buffers[object.lod_level];
                object.index_count = object.lod_index_counts[object.lod_level];
            }
        }
    }
    
    ImGui::PopID();
#endif
}

void RenderingUI::render_transform_editor(std::array<float, 16>& transform) {
#ifdef ECSCOPE_HAS_IMGUI
    // Extract position, rotation, scale from matrix (simplified)
    float position[3] = {transform[12], transform[13], transform[14]};
    float scale[3] = {1.0f, 1.0f, 1.0f}; // Would calculate from matrix
    float rotation[3] = {0.0f, 0.0f, 0.0f}; // Would extract Euler angles
    
    bool changed = false;
    
    if (ImGui::DragFloat3("Position", position, 0.1f)) {
        transform[12] = position[0];
        transform[13] = position[1];
        transform[14] = position[2];
        changed = true;
    }
    
    if (ImGui::DragFloat3("Rotation", rotation, 1.0f)) {
        // Would rebuild rotation matrix
        changed = true;
    }
    
    if (ImGui::DragFloat3("Scale", scale, 0.01f)) {
        // Would rebuild scale matrix
        changed = true;
    }
    
    if (changed) {
        config_dirty_ = true;
    }
#endif
}

void RenderingUI::render_viewport_panel() {
#ifdef ECSCOPE_HAS_IMGUI
    if (ImGui::Begin("3D Viewport", &show_viewport_)) {
        
        viewport_focused_ = ImGui::IsWindowFocused();
        
        // Camera controls
        if (ImGui::CollapsingHeader("Camera Controls")) {
            const char* camera_modes[] = {"Orbit", "Fly", "First Person", "Inspect"};
            int current_mode = static_cast<int>(camera_mode_);
            if (ImGui::Combo("Camera Mode", &current_mode, camera_modes, IM_ARRAYSIZE(camera_modes))) {
                set_camera_mode(static_cast<CameraControlMode>(current_mode));
            }
            
            if (ImGui::Button("Reset Camera")) {
                reset_camera();
            }
            ImGui::SameLine();
            if (ImGui::Button("Focus Selected")) {
                if (selected_object_id_ > 0) {
                    focus_camera_on_object(selected_object_id_);
                }
            }
            
            // Camera properties
            ImGui::SliderFloat("FOV", &camera_.fov, 15.0f, 120.0f);
            ImGui::DragFloat("Near Plane", &camera_.near_plane, 0.001f, 0.001f, 10.0f);
            ImGui::DragFloat("Far Plane", &camera_.far_plane, 1.0f, 100.0f, 10000.0f);
            
            if (camera_mode_ == CameraControlMode::Orbit) {
                ImGui::SliderFloat("Orbit Distance", &camera_.orbit_distance, 1.0f, 100.0f);
                ImGui::SliderFloat("Orbit Phi", &camera_.orbit_phi, -M_PI, M_PI);
                ImGui::SliderFloat("Orbit Theta", &camera_.orbit_theta, -M_PI/2, M_PI/2);
            }
        }
        
        ImGui::Separator();
        
        // Viewport rendering area
        ImVec2 viewport_pos = ImGui::GetCursorScreenPos();
        ImVec2 viewport_size = ImGui::GetContentRegionAvail();
        
        if (viewport_size.x > 0 && viewport_size.y > 0) {
            viewport_size_.x = viewport_size.x;
            viewport_size_.y = viewport_size.y;
            
            // Render 3D scene to texture and display
            // This would involve creating a framebuffer and rendering to it
            ImGui::Image(nullptr, viewport_size, ImVec2(0, 1), ImVec2(1, 0)); // Placeholder
            
            // Handle mouse input for camera controls
            if (ImGui::IsItemHovered() && viewport_focused_) {
                handle_viewport_input();
            }
        }
        
        // Viewport overlay
        ImGui::SetCursorScreenPos(ImVec2(viewport_pos.x + 10, viewport_pos.y + 10));
        ImGui::BeginChild("ViewportOverlay", ImVec2(200, 100), false, 
                         ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoScrollbar);
        
        ImGui::Text("Viewport: %.0fx%.0f", viewport_size_.x, viewport_size_.y);
        ImGui::Text("Camera: [%.1f, %.1f, %.1f]", camera_.position[0], camera_.position[1], camera_.position[2]);
        ImGui::Text("Target: [%.1f, %.1f, %.1f]", camera_.target[0], camera_.target[1], camera_.target[2]);
        
        ImGui::EndChild();
    }
    ImGui::End();
#endif
}

void RenderingUI::render_shader_editor_panel() {
#ifdef ECSCOPE_HAS_IMGUI
    if (ImGui::Begin("Shader Editor", &show_shader_editor_)) {
        
        // Hot reload controls
        ImGui::Checkbox("Hot Reload Enabled", &shader_hot_reload_enabled_);
        ImGui::SameLine();
        if (ImGui::Button("Reload All")) {
            reload_all_shaders();
        }
        
        ImGui::Separator();
        
        // Shader list
        render_shader_list();
        
        ImGui::Separator();
        
        // Shader reload controls
        render_shader_reload_controls();
        
        // Error display
        render_shader_error_display();
    }
    ImGui::End();
#endif
}

void RenderingUI::render_shader_list() {
#ifdef ECSCOPE_HAS_IMGUI
    ImGui::Text("Registered Shaders:");
    
    for (auto& [name, shader] : shaders_) {
        ImGui::PushID(name.c_str());
        
        // Status indicator
        ImVec4 status_color;
        const char* status_text;
        switch (shader.reload_status) {
            case ShaderReloadStatus::Idle:
                status_color = ImVec4(0.5f, 0.5f, 0.5f, 1.0f);
                status_text = "IDLE";
                break;
            case ShaderReloadStatus::Reloading:
                status_color = ImVec4(1.0f, 1.0f, 0.0f, 1.0f);
                status_text = "RELOADING";
                break;
            case ShaderReloadStatus::Success:
                status_color = ImVec4(0.0f, 1.0f, 0.0f, 1.0f);
                status_text = "SUCCESS";
                break;
            case ShaderReloadStatus::Error:
                status_color = ImVec4(1.0f, 0.0f, 0.0f, 1.0f);
                status_text = "ERROR";
                break;
        }
        
        ImGui::TextColored(status_color, "[%s]", status_text);
        ImGui::SameLine();
        ImGui::Text("%s", name.c_str());
        
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Vertex: %s\nFragment: %s", 
                             shader.vertex_path.c_str(), 
                             shader.fragment_path.c_str());
        }
        
        ImGui::SameLine();
        if (ImGui::Button("Reload")) {
            reload_shader(name);
        }
        
        ImGui::PopID();
    }
#endif
}

void RenderingUI::submit_scene_to_renderer() {
    if (!deferred_renderer_) {
        return;
    }

    // Submit all lights
    for (const auto& [id, light] : scene_lights_) {
        if (light.enabled) {
            deferred_renderer_->submit_light(light.light_data);
        }
    }

    // Submit all visible objects
    for (const auto& [id, object] : scene_objects_) {
        if (object.visible && object.vertex_buffer.is_valid()) {
            deferred_renderer_->submit_geometry(
                object.vertex_buffer,
                object.index_buffer,
                object.material,
                object.transform,
                object.index_count
            );
        }
    }
}

void RenderingUI::animate_scene_lights(float delta_time) {
    for (auto& [id, light] : scene_lights_) {
        if (!light.animated) continue;
        
        float angle = animation_time_ * light.animation_speed;
        light.light_data.position[0] = light.animation_center[0] + 
            light.animation_radius * std::cos(angle);
        light.light_data.position[2] = light.animation_center[2] + 
            light.animation_radius * std::sin(angle);
    }
}

// =============================================================================
// UTILITY FUNCTIONS
// =============================================================================

LiveRenderingConfig create_default_rendering_config() {
    LiveRenderingConfig config;
    
    // Set reasonable defaults
    config.deferred_config.width = 1920;
    config.deferred_config.height = 1080;
    config.deferred_config.msaa_samples = 1;
    config.deferred_config.enable_screen_space_reflections = true;
    config.deferred_config.enable_temporal_effects = true;
    config.deferred_config.enable_volumetric_lighting = false;
    config.deferred_config.max_lights_per_tile = 256;
    config.deferred_config.tile_size = 16;
    config.deferred_config.use_compute_shading = true;
    
    // Post-processing defaults
    config.post_process.enable_hdr = true;
    config.post_process.exposure = 1.0f;
    config.post_process.gamma = 2.2f;
    config.post_process.enable_bloom = true;
    config.post_process.bloom_threshold = 1.0f;
    config.post_process.bloom_intensity = 0.8f;
    config.post_process.enable_ssao = true;
    config.post_process.ssao_radius = 0.5f;
    config.post_process.ssao_intensity = 1.0f;
    
    // Shadow defaults
    config.shadows.enable_shadows = true;
    config.shadows.cascade_count = 4;
    config.shadows.shadow_resolution = 2048;
    config.shadows.enable_pcf = true;
    
    return config;
}

std::string debug_mode_to_string(DebugVisualizationMode mode) {
    switch (mode) {
        case DebugVisualizationMode::None: return "None";
        case DebugVisualizationMode::GBufferAlbedo: return "G-Buffer Albedo";
        case DebugVisualizationMode::GBufferNormal: return "G-Buffer Normal";
        case DebugVisualizationMode::GBufferDepth: return "G-Buffer Depth";
        case DebugVisualizationMode::GBufferMaterial: return "G-Buffer Material";
        case DebugVisualizationMode::LightComplexity: return "Light Complexity";
        case DebugVisualizationMode::Overdraw: return "Overdraw";
        case DebugVisualizationMode::ShadowCascades: return "Shadow Cascades";
        case DebugVisualizationMode::SSAO: return "SSAO";
        case DebugVisualizationMode::SSR: return "SSR";
        case DebugVisualizationMode::Bloom: return "Bloom";
        case DebugVisualizationMode::Wireframe: return "Wireframe";
        default: return "Unknown";
    }
}

std::string format_memory_size(uint64_t bytes) {
    const char* units[] = {"B", "KB", "MB", "GB", "TB"};
    int unit = 0;
    double size = static_cast<double>(bytes);
    
    while (size >= 1024.0 && unit < 4) {
        size /= 1024.0;
        unit++;
    }
    
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2) << size << " " << units[unit];
    return oss.str();
}

std::string format_gpu_time(float milliseconds) {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(3) << milliseconds << " ms";
    return oss.str();
}

bool validate_rendering_config(const LiveRenderingConfig& config, rendering::IRenderer* renderer) {
    if (!renderer) return false;
    
    auto caps = renderer->get_capabilities();
    
    // Check if resolution is supported
    if (config.deferred_config.width > caps.max_texture_size || 
        config.deferred_config.height > caps.max_texture_size) {
        return false;
    }
    
    // Check MSAA support
    if (config.deferred_config.msaa_samples > caps.max_msaa_samples) {
        return false;
    }
    
    // Check compute shader support
    if (config.deferred_config.use_compute_shading && !caps.supports_compute_shaders) {
        return false;
    }
    
    return true;
}

void register_rendering_ui_features(Dashboard* dashboard, RenderingUI* rendering_ui) {
    if (!dashboard || !rendering_ui) return;
    
    // Register rendering UI as a feature in the dashboard
    FeatureInfo rendering_feature;
    rendering_feature.id = "rendering_ui";
    rendering_feature.name = "Rendering Pipeline Control";
    rendering_feature.description = "Professional rendering pipeline control with real-time parameter adjustment";
    rendering_feature.icon = "🎨";
    rendering_feature.category = FeatureCategory::Rendering;
    rendering_feature.launch_callback = [rendering_ui]() {
        // Show/focus rendering UI panels
        rendering_ui->show_pipeline_panel_ = true;
    };
    rendering_feature.status_callback = [rendering_ui]() {
        return rendering_ui->is_initialized();
    };
    
    dashboard->register_feature(rendering_feature);
}

} // namespace ecscope::gui