/**
 * @file audio_ui.cpp
 * @brief Implementation of Comprehensive Audio System UI
 */

#include "../../include/ecscope/gui/audio_ui.hpp"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <sstream>
#include <iomanip>

#ifdef ECSCOPE_HAS_IMGUI
// #include <imgui_stdlib.h>  // Not available in this ImGui version
#endif

namespace ecscope::gui {

// =============================================================================
// CONSTANTS AND UTILITY FUNCTIONS
// =============================================================================

namespace {
    constexpr float PI = 3.14159265359f;
    constexpr float TWO_PI = 2.0f * PI;
    
    // Color schemes for audio visualization
    constexpr ImU32 AUDIO_SOURCE_COLOR = IM_COL32(255, 100, 100, 255);
    constexpr ImU32 AUDIO_LISTENER_COLOR = IM_COL32(100, 255, 100, 255);
    constexpr ImU32 REVERB_ZONE_COLOR = IM_COL32(100, 100, 255, 100);
    constexpr ImU32 AUDIO_RAY_COLOR = IM_COL32(255, 255, 0, 200);
    constexpr ImU32 GRID_COLOR = IM_COL32(128, 128, 128, 64);
    
    float lerp(float a, float b, float t) {
        return a + t * (b - a);
    }
    
    Vector3f lerp(const Vector3f& a, const Vector3f& b, float t) {
        return {
            lerp(a.x, b.x, t),
            lerp(a.y, b.y, t),
            lerp(a.z, b.z, t)
        };
    }
}

// =============================================================================
// AUDIO SYSTEM UI IMPLEMENTATION
// =============================================================================

AudioSystemUI::AudioSystemUI() 
    : last_update_time_(std::chrono::steady_clock::now()) {
}

AudioSystemUI::~AudioSystemUI() {
    shutdown();
}

bool AudioSystemUI::initialize(audio::AudioSystem* audio_system, Dashboard* dashboard) {
    if (!audio_system || !dashboard) {
        return false;
    }
    
    audio_system_ = audio_system;
    dashboard_ = dashboard;
    
    // Initialize visualization components
    visualizer_3d_ = std::make_unique<Audio3DVisualizer>();
    if (!visualizer_3d_->initialize()) {
        return false;
    }
    
    spectrum_analyzer_ = std::make_unique<AudioSpectrumAnalyzer>();
    waveform_display_ = std::make_unique<AudioWaveformDisplay>();
    hrtf_visualizer_ = std::make_unique<HRTFVisualizer>();
    effects_editor_ = std::make_unique<EffectsChainEditor>();
    spatial_controller_ = std::make_unique<SpatialAudioController>();
    performance_monitor_ = std::make_unique<AudioPerformanceMonitor>();
    
    spatial_controller_->set_audio_system(audio_system_);
    
    // Register with dashboard
    FeatureInfo audio_ui_feature;
    audio_ui_feature.id = "audio_ui";
    audio_ui_feature.name = "Audio System UI";
    audio_ui_feature.description = "Professional audio interface with 3D visualization";
    audio_ui_feature.icon = "🔊";  // Using Unicode emoji instead
    audio_ui_feature.category = FeatureCategory::Audio;
    audio_ui_feature.launch_callback = [this]() { set_display_mode(AudioDisplayMode::Overview); };
    audio_ui_feature.status_callback = [this]() { return is_initialized(); };
    
    dashboard_->register_feature(audio_ui_feature);
    
    initialized_ = true;
    return true;
}

void AudioSystemUI::shutdown() {
    if (!initialized_) return;
    
    // Cleanup visualization components
    visualizer_3d_.reset();
    spectrum_analyzer_.reset();
    waveform_display_.reset();
    hrtf_visualizer_.reset();
    effects_editor_.reset();
    spatial_controller_.reset();
    performance_monitor_.reset();
    
    // Clear data
    {
        std::lock_guard<std::mutex> lock(data_mutex_);
        source_visuals_.clear();
        listener_visuals_.clear();
        reverb_zones_.clear();
        audio_rays_.clear();
        spectrum_data_.clear();
        waveform_data_.clear();
        effect_visualizations_.clear();
    }
    
    initialized_ = false;
}

void AudioSystemUI::render() {
    if (!initialized_) return;
    
#ifdef ECSCOPE_HAS_IMGUI
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;
    
    if (ImGui::Begin("Audio System", nullptr, window_flags)) {
        render_main_controls();
        
        ImGui::Separator();
        
        switch (current_mode_) {
            case AudioDisplayMode::Overview:
                render_3d_viewport();
                ImGui::SameLine();
                render_source_inspector();
                render_spectrum_analyzer();
                break;
                
            case AudioDisplayMode::Sources3D:
                render_3d_viewport();
                ImGui::SameLine();
                render_source_properties_panel();
                break;
                
            case AudioDisplayMode::Listener:
                render_listener_controls();
                render_hrtf_visualization();
                break;
                
            case AudioDisplayMode::Effects:
                render_effects_panel();
                break;
                
            case AudioDisplayMode::Spatial:
                render_spatial_controls();
                break;
                
            case AudioDisplayMode::Performance:
                render_performance_panel();
                break;
                
            case AudioDisplayMode::Debugging:
                render_debug_panel();
                break;
        }
    }
    ImGui::End();
#endif
}

void AudioSystemUI::update(float delta_time) {
    if (!initialized_) return;
    
    auto current_time = std::chrono::steady_clock::now();
    // auto time_since_last_update = std::chrono::duration<float>(current_time - last_update_time_).count();
    
    animation_time_ += delta_time;
    
    // Update 3D visualizations
    update_3d_visualizations();
    
    // Update audio analysis
    update_audio_analysis();
    
    // Calculate audio rays if enabled
    if (show_audio_rays_) {
        calculate_audio_rays();
    }
    
    // Process HRTF visualization if needed
    if (current_mode_ == AudioDisplayMode::Listener) {
        process_hrtf_visualization();
    }
    
    last_update_time_ = current_time;
}

void AudioSystemUI::set_display_mode(AudioDisplayMode mode) {
    current_mode_ = mode;
}

void AudioSystemUI::set_3d_render_mode(Audio3DRenderMode mode) {
    render_mode_ = mode;
    if (visualizer_3d_) {
        visualizer_3d_->set_render_mode(mode);
    }
}

void AudioSystemUI::select_audio_source(uint32_t source_id) {
    selected_source_id_ = source_id;
    
    // Update selection in visual data
    std::lock_guard<std::mutex> lock(data_mutex_);
    for (auto& [id, visual] : source_visuals_) {
        visual.is_selected = (id == source_id);
    }
}

void AudioSystemUI::register_audio_source(uint32_t source_id, const AudioSourceVisual& visual) {
    std::lock_guard<std::mutex> lock(data_mutex_);
    source_visuals_[source_id] = visual;
    source_visuals_[source_id].source_id = source_id;
    source_visuals_[source_id].last_update = std::chrono::steady_clock::now();
}

void AudioSystemUI::update_source_visual(uint32_t source_id, const AudioSourceVisual& visual) {
    std::lock_guard<std::mutex> lock(data_mutex_);
    auto it = source_visuals_.find(source_id);
    if (it != source_visuals_.end()) {
        // Smooth animation between positions
        it->second.animated_position = lerp(it->second.animated_position, visual.position, 0.1f);
        it->second = visual;
        it->second.animated_position = lerp(it->second.animated_position, visual.position, 0.1f);
        it->second.last_update = std::chrono::steady_clock::now();
    }
}

void AudioSystemUI::unregister_audio_source(uint32_t source_id) {
    std::lock_guard<std::mutex> lock(data_mutex_);
    source_visuals_.erase(source_id);
    spectrum_data_.erase(source_id);
    waveform_data_.erase(source_id);
}

void AudioSystemUI::register_audio_listener(uint32_t listener_id, const AudioListenerVisual& visual) {
    std::lock_guard<std::mutex> lock(data_mutex_);
    listener_visuals_[listener_id] = visual;
    listener_visuals_[listener_id].listener_id = listener_id;
}

void AudioSystemUI::update_listener_visual(uint32_t listener_id, const AudioListenerVisual& visual) {
    std::lock_guard<std::mutex> lock(data_mutex_);
    auto it = listener_visuals_.find(listener_id);
    if (it != listener_visuals_.end()) {
        it->second = visual;
    }
}

void AudioSystemUI::set_active_listener(uint32_t listener_id) {
    active_listener_id_ = listener_id;
    
    // Update active status in visual data
    std::lock_guard<std::mutex> lock(data_mutex_);
    for (auto& [id, visual] : listener_visuals_) {
        visual.is_active = (id == listener_id);
    }
}

void AudioSystemUI::update_spectrum_data(uint32_t source_id, const AudioSpectrumData& data) {
    std::lock_guard<std::mutex> lock(data_mutex_);
    spectrum_data_[source_id] = data;
}

void AudioSystemUI::update_waveform_data(uint32_t source_id, const AudioWaveformData& data) {
    std::lock_guard<std::mutex> lock(data_mutex_);
    waveform_data_[source_id] = data;
}

void AudioSystemUI::add_audio_ray(const AudioRayVisual& ray) {
    std::lock_guard<std::mutex> lock(data_mutex_);
    audio_rays_.push_back(ray);
}

void AudioSystemUI::clear_audio_rays() {
    std::lock_guard<std::mutex> lock(data_mutex_);
    audio_rays_.clear();
}

// =============================================================================
// PRIVATE RENDERING METHODS
// =============================================================================

void AudioSystemUI::render_main_controls() {
#ifdef ECSCOPE_HAS_IMGUI
    // Display mode selector
    const char* mode_names[] = {
        "Overview", "Sources 3D", "Listener", "Effects", 
        "Spatial", "Performance", "Debug"
    };
    int current_mode_int = static_cast<int>(current_mode_);
    if (ImGui::Combo("Mode", &current_mode_int, mode_names, IM_ARRAYSIZE(mode_names))) {
        set_display_mode(static_cast<AudioDisplayMode>(current_mode_int));
    }
    
    ImGui::SameLine();
    
    // 3D render mode selector
    const char* render_mode_names[] = {"Wireframe", "Solid", "Transparent", "Heatmap"};
    int render_mode_int = static_cast<int>(render_mode_);
    if (ImGui::Combo("Render", &render_mode_int, render_mode_names, IM_ARRAYSIZE(render_mode_names))) {
        set_3d_render_mode(static_cast<Audio3DRenderMode>(render_mode_int));
    }
    
    ImGui::SameLine();
    
    // Visualization toggles
    ImGui::Checkbox("Sources", &show_sources_);
    ImGui::SameLine();
    ImGui::Checkbox("Listeners", &show_listeners_);
    ImGui::SameLine();
    ImGui::Checkbox("Zones", &show_reverb_zones_);
    ImGui::SameLine();
    ImGui::Checkbox("Rays", &show_audio_rays_);
    ImGui::SameLine();
    ImGui::Checkbox("Doppler", &show_doppler_);
#endif
}

void AudioSystemUI::render_3d_viewport() {
#ifdef ECSCOPE_HAS_IMGUI
    ImGui::BeginChild("3D Viewport", ImVec2(0, 400), true);
    
    if (visualizer_3d_) {
        std::lock_guard<std::mutex> lock(data_mutex_);
        visualizer_3d_->render(source_visuals_, listener_visuals_, 
                              camera_position_, camera_target_);
    }
    
    // Handle viewport input
    handle_3d_viewport_input();
    
    // Render coordinate system
    render_coordinate_system();
    
    // Render 3D elements
    if (show_sources_) render_3d_sources();
    if (show_listeners_) render_3d_listeners();
    if (show_reverb_zones_) render_reverb_zones();
    if (show_audio_rays_) render_audio_rays();
    if (show_doppler_) render_doppler_effects();
    
    ImGui::EndChild();
#endif
}

void AudioSystemUI::render_source_inspector() {
#ifdef ECSCOPE_HAS_IMGUI
    ImGui::BeginChild("Source Inspector", ImVec2(300, 0), true);
    
    ImGui::Text("Audio Sources");
    ImGui::Separator();
    
    std::lock_guard<std::mutex> lock(data_mutex_);
    for (const auto& [id, visual] : source_visuals_) {
        ImGui::PushID(id);
        
        bool is_selected = (selected_source_id_ == id);
        if (ImGui::Selectable(("Source " + std::to_string(id)).c_str(), is_selected)) {
            select_audio_source(id);
        }
        
        if (is_selected) {
            ImGui::Indent();
            ImGui::Text("Position: (%.2f, %.2f, %.2f)", 
                       visual.position.x, visual.position.y, visual.position.z);
            ImGui::Text("Volume: %.2f", visual.volume);
            ImGui::Text("Playing: %s", visual.is_playing ? "Yes" : "No");
            ImGui::Text("Distance: %.2f", visual.min_distance);
            ImGui::Unindent();
        }
        
        ImGui::PopID();
    }
    
    ImGui::EndChild();
#endif
}

void AudioSystemUI::render_listener_controls() {
#ifdef ECSCOPE_HAS_IMGUI
    ImGui::Text("Audio Listener Controls");
    ImGui::Separator();
    
    std::lock_guard<std::mutex> lock(data_mutex_);
    auto active_listener_it = listener_visuals_.find(active_listener_id_);
    if (active_listener_it != listener_visuals_.end()) {
        auto& listener = active_listener_it->second;
        
        ImGui::Text("Active Listener: %u", active_listener_id_);
        
        // Position controls
        float pos[3] = {listener.position.x, listener.position.y, listener.position.z};
        if (ImGui::DragFloat3("Position", pos, 0.1f)) {
            listener.position = {pos[0], pos[1], pos[2]};
            // Update in audio system (disabled for compilation)
            // if (audio_system_) {
            //     audio_system_->get_3d_engine().update_listener(
            //         listener.position, 
            //         {listener.forward.x, listener.forward.y, listener.forward.z, 0.0f}, // Convert to quaternion
            //         listener.velocity
            //     );
            // }
        }
        
        // Orientation controls
        float forward[3] = {listener.forward.x, listener.forward.y, listener.forward.z};
        if (ImGui::DragFloat3("Forward", forward, 0.01f, -1.0f, 1.0f)) {
            listener.forward = {forward[0], forward[1], forward[2]};
            // Normalize
            float length = std::sqrt(listener.forward.x * listener.forward.x + 
                                   listener.forward.y * listener.forward.y + 
                                   listener.forward.z * listener.forward.z);
            if (length > 0.0f) {
                listener.forward.x /= length;
                listener.forward.y /= length;
                listener.forward.z /= length;
            }
        }
        
        ImGui::Checkbox("Show Orientation", &listener.show_orientation);
        ImGui::Checkbox("Show HRTF Pattern", &listener.show_hrtf_pattern);
        
        ImGui::SliderFloat("Head Size", &listener.head_size, 0.1f, 2.0f);
    }
#endif
}

void AudioSystemUI::render_effects_panel() {
#ifdef ECSCOPE_HAS_IMGUI
    if (effects_editor_) {
        effects_editor_->render();
    }
#endif
}

void AudioSystemUI::render_spatial_controls() {
#ifdef ECSCOPE_HAS_IMGUI
    if (spatial_controller_) {
        spatial_controller_->render();
    }
#endif
}

void AudioSystemUI::render_performance_panel() {
#ifdef ECSCOPE_HAS_IMGUI
    if (performance_monitor_) {
        performance_monitor_->render();
    }
#endif
}

void AudioSystemUI::render_debug_panel() {
#ifdef ECSCOPE_HAS_IMGUI
    ImGui::Text("Debug Information");
    ImGui::Separator();
    
    // Audio system debug info (disabled for compilation)
    // if (audio_system_) {
    //     auto metrics = audio_system_->get_system_metrics();
        
        ImGui::Text("Active Voices: %u", 0u);  // Placeholder values
        ImGui::Text("Processing Load: %.1f%%", 0.0f);
        ImGui::Text("Memory Usage: %.2f MB", 0.0f);
        ImGui::Text("Latency: %.2f ms", 0.0f);
        
        ImGui::Separator();
        
        // Debug visualization controls
        static bool debug_visualization = false;
        if (ImGui::Checkbox("Debug Visualization", &debug_visualization)) {
            // audio_system_->get_3d_engine().enable_debug_visualization(debug_visualization);
        }
        
        // Ray tracing debug info
        if (show_audio_rays_) {
            std::lock_guard<std::mutex> lock(data_mutex_);
            ImGui::Text("Audio Rays: %zu", audio_rays_.size());
            
            // if (selected_source_id_ != 0) {
            //     auto ray_paths = audio_system_->get_3d_engine().get_debug_ray_paths(selected_source_id_);
            //     ImGui::Text("Ray Paths for Source %u: %zu", selected_source_id_, ray_paths.size());
            // }
        }
    // }
#endif
}

void AudioSystemUI::render_spectrum_analyzer() {
#ifdef ECSCOPE_HAS_IMGUI
    ImGui::BeginChild("Spectrum Analyzer", ImVec2(0, 200), true);
    
    if (spectrum_analyzer_ && selected_source_id_ != 0) {
        std::lock_guard<std::mutex> lock(data_mutex_);
        auto spectrum_it = spectrum_data_.find(selected_source_id_);
        if (spectrum_it != spectrum_data_.end()) {
            spectrum_analyzer_->render(spectrum_it->second);
        }
    }
    
    ImGui::EndChild();
#endif
}

void AudioSystemUI::render_3d_sources() {
#ifdef ECSCOPE_HAS_IMGUI
    // ImDrawList* draw_list = ImGui::GetWindowDrawList();
    
    std::lock_guard<std::mutex> lock(data_mutex_);
    for (const auto& [id, visual] : source_visuals_) {
        Vector3f screen_pos = world_to_screen(visual.animated_position);
        
        if (is_point_in_view_frustum(visual.animated_position)) {
            // Draw source sphere
            ImU32 color = visual.is_selected ? IM_COL32(255, 255, 0, 255) : visual.color;
            if (visual.is_playing) {
                // Add pulsing effect for playing sources
                float pulse = 0.5f + 0.5f * std::sin(animation_time_ * 4.0f);
                color = IM_COL32(
                    static_cast<int>(((color >> 0) & 0xFF) * pulse),
                    static_cast<int>(((color >> 8) & 0xFF) * pulse),
                    static_cast<int>(((color >> 16) & 0xFF) * pulse),
                    (color >> 24) & 0xFF
                );
            }
            
            draw_3d_sphere(visual.animated_position, visual.radius, color, 
                          render_mode_ == Audio3DRenderMode::Wireframe);
            
            // Draw attenuation sphere if enabled
            if (visual.show_attenuation_sphere) {
                draw_3d_sphere(visual.animated_position, visual.max_distance, 
                              IM_COL32(255, 255, 255, 32), true);
            }
            
            // Draw directional cone if enabled
            if (visual.show_cone && visual.cone_inner_angle < 360.0f) {
                draw_3d_cone(visual.animated_position, visual.direction,
                            visual.cone_inner_angle * PI / 180.0f, 
                            visual.max_distance * 0.5f, 
                            IM_COL32(255, 200, 0, 128));
            }
            
            // Draw velocity vector if moving
            if (show_doppler_ && (visual.velocity.x != 0 || visual.velocity.y != 0 || visual.velocity.z != 0)) {
                Vector3f end_pos = {
                    visual.animated_position.x + visual.velocity.x,
                    visual.animated_position.y + visual.velocity.y,
                    visual.animated_position.z + visual.velocity.z
                };
                draw_3d_arrow(visual.animated_position, end_pos, IM_COL32(255, 255, 0, 255));
            }
        }
    }
#endif
}

void AudioSystemUI::render_3d_listeners() {
#ifdef ECSCOPE_HAS_IMGUI
    std::lock_guard<std::mutex> lock(data_mutex_);
    for (const auto& [id, visual] : listener_visuals_) {
        if (is_point_in_view_frustum(visual.position)) {
            ImU32 color = visual.is_active ? IM_COL32(0, 255, 0, 255) : visual.color;
            
            // Draw head as sphere
            draw_3d_sphere(visual.position, visual.head_size, color);
            
            // Draw orientation vectors
            if (visual.show_orientation) {
                Vector3f forward_end = {
                    visual.position.x + visual.forward.x * 2.0f,
                    visual.position.y + visual.forward.y * 2.0f,
                    visual.position.z + visual.forward.z * 2.0f
                };
                Vector3f up_end = {
                    visual.position.x + visual.up.x * 1.5f,
                    visual.position.y + visual.up.y * 1.5f,
                    visual.position.z + visual.up.z * 1.5f
                };
                
                draw_3d_arrow(visual.position, forward_end, IM_COL32(255, 0, 0, 255)); // Red for forward
                draw_3d_arrow(visual.position, up_end, IM_COL32(0, 0, 255, 255));     // Blue for up
            }
            
            // Draw HRTF pattern if enabled
            if (visual.show_hrtf_pattern && !visual.hrtf_pattern_left.empty()) {
                draw_hrtf_pattern(visual.position, visual.hrtf_pattern_left, IM_COL32(255, 128, 0, 255));
            }
        }
    }
#endif
}

void AudioSystemUI::render_audio_rays() {
#ifdef ECSCOPE_HAS_IMGUI
    std::lock_guard<std::mutex> lock(data_mutex_);
    for (const auto& ray : audio_rays_) {
        ImU32 color = ray.is_occluded ? IM_COL32(255, 0, 0, 128) : ray.color;
        
        // Adjust color based on intensity
        float intensity_factor = std::min(1.0f, ray.intensity);
        color = IM_COL32(
            static_cast<int>(((color >> 0) & 0xFF) * intensity_factor),
            static_cast<int>(((color >> 8) & 0xFF) * intensity_factor),
            static_cast<int>(((color >> 16) & 0xFF) * intensity_factor),
            (color >> 24) & 0xFF
        );
        
        draw_3d_line(ray.start, ray.end, color, 1.0f + ray.intensity);
        
        // Draw reflection point if present
        if (ray.bounce_count > 0) {
            draw_3d_sphere(ray.reflection_point, 0.1f, IM_COL32(255, 255, 0, 255));
        }
    }
#endif
}

void AudioSystemUI::render_coordinate_system() {
#ifdef ECSCOPE_HAS_IMGUI
    // Draw coordinate axes at origin
    Vector3f origin{0.0f, 0.0f, 0.0f};
    Vector3f x_axis{1.0f, 0.0f, 0.0f};
    Vector3f y_axis{0.0f, 1.0f, 0.0f};
    Vector3f z_axis{0.0f, 0.0f, 1.0f};
    
    draw_3d_arrow(origin, x_axis, IM_COL32(255, 0, 0, 255));   // X - Red
    draw_3d_arrow(origin, y_axis, IM_COL32(0, 255, 0, 255));   // Y - Green
    draw_3d_arrow(origin, z_axis, IM_COL32(0, 0, 255, 255));   // Z - Blue
#endif
}

// =============================================================================
// UTILITY IMPLEMENTATIONS
// =============================================================================

void AudioSystemUI::update_3d_visualizations() {
    if (!audio_system_) return;
    
    // Update source positions from audio system
    // This would require audio system API to get current source states
    // For now, we'll assume external updates via update_source_visual()
    
    // Smooth animation updates
    std::lock_guard<std::mutex> lock(data_mutex_);
    for (auto& [id, visual] : source_visuals_) {
        // Smooth position interpolation
        visual.animated_position = lerp(visual.animated_position, visual.position, 0.1f);
        
        // Update playing state animation
        if (visual.is_playing) {
            visual.intensity = 0.8f + 0.2f * std::sin(animation_time_ * 6.0f);
        } else {
            visual.intensity = 0.3f;
        }
    }
}

void AudioSystemUI::update_audio_analysis() {
    // This would typically be called from audio processing thread
    // with real audio data. For now, we'll simulate some data.
    
    if (selected_source_id_ != 0) {
        // Generate simulated spectrum data
        AudioSpectrumData spectrum;
        spectrum.sample_rate = 48000.0f;
        spectrum.fft_size = 2048;
        spectrum.frequencies.resize(1024);
        spectrum.magnitudes.resize(1024);
        spectrum.timestamp = std::chrono::steady_clock::now();
        
        for (size_t i = 0; i < spectrum.frequencies.size(); ++i) {
            spectrum.frequencies[i] = (static_cast<float>(i) * spectrum.sample_rate) / (2.0f * 1024.0f);
            // Simulate some audio content
            spectrum.magnitudes[i] = -40.0f + 30.0f * std::sin(animation_time_ + i * 0.01f);
        }
        
        update_spectrum_data(selected_source_id_, spectrum);
    }
}

void AudioSystemUI::calculate_audio_rays() {
    if (!audio_system_) return;
    
    std::lock_guard<std::mutex> lock(data_mutex_);
    audio_rays_.clear();
    
    // Generate rays from each source to active listener
    auto active_listener_it = listener_visuals_.find(active_listener_id_);
    if (active_listener_it != listener_visuals_.end()) {
        const auto& listener = active_listener_it->second;
        
        for (const auto& [source_id, source] : source_visuals_) {
            if (source.is_playing) {
                // Direct ray
                AudioRayVisual direct_ray;
                direct_ray.start = source.position;
                direct_ray.end = listener.position;
                direct_ray.bounce_count = 0;
                direct_ray.intensity = source.volume;
                direct_ray.color = IM_COL32(255, 255, 0, 200);
                direct_ray.is_occluded = false; // Would need physics system integration
                
                audio_rays_.push_back(direct_ray);
                
                // Simulated reflection rays (disabled for compilation)
                // if (audio_system_->get_raytracing_processor()) {
                //     // This would call the real ray tracing system
                //     auto ray_paths = audio_system_->get_3d_engine().get_debug_ray_paths(source_id);
                //     for (const auto& path : ray_paths) {
                //         AudioRayVisual reflected_ray;
                //         reflected_ray.start = source.position;
                //         reflected_ray.end = path; // This would be the reflection endpoint
                //         reflected_ray.reflection_point = path; // Simplified
                //         reflected_ray.bounce_count = 1;
                //         reflected_ray.intensity = source.volume * 0.3f; // Reduced for reflection
                //         reflected_ray.color = IM_COL32(255, 128, 0, 150);
                //         reflected_ray.is_occluded = false;
                //         
                //         audio_rays_.push_back(reflected_ray);
                //     }
                // }
            }
        }
    }
}

Vector3f AudioSystemUI::world_to_screen(const Vector3f& world_pos) const {
    // Simplified projection - in real implementation would use proper camera matrices
    Vector3f relative = {
        world_pos.x - camera_position_.x,
        world_pos.y - camera_position_.y,
        world_pos.z - camera_position_.z
    };
    
    // Simple perspective projection
    float distance = std::sqrt(relative.x * relative.x + relative.y * relative.y + relative.z * relative.z);
    if (distance < 0.1f) distance = 0.1f;
    
    float scale = 100.0f / distance;
    
    ImVec2 viewport_center = ImGui::GetWindowPos();
    viewport_center.x += ImGui::GetWindowSize().x * 0.5f;
    viewport_center.y += ImGui::GetWindowSize().y * 0.5f;
    
    return {
        viewport_center.x + relative.x * scale,
        viewport_center.y - relative.y * scale, // Flip Y for screen coordinates
        distance
    };
}

bool AudioSystemUI::is_point_in_view_frustum(const Vector3f& point) const {
    // Simplified frustum culling
    Vector3f relative = {
        point.x - camera_position_.x,
        point.y - camera_position_.y,
        point.z - camera_position_.z
    };
    
    float distance = std::sqrt(relative.x * relative.x + relative.y * relative.y + relative.z * relative.z);
    return distance >= camera_near_ && distance <= camera_far_;
}

void AudioSystemUI::draw_3d_sphere(const Vector3f& center, float radius, ImU32 color, bool wireframe) {
#ifdef ECSCOPE_HAS_IMGUI
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    Vector3f screen_center = world_to_screen(center);
    
    // Scale radius based on distance
    float screen_radius = radius * 100.0f / screen_center.z;
    if (screen_radius < 2.0f) screen_radius = 2.0f;
    if (screen_radius > 50.0f) screen_radius = 50.0f;
    
    if (wireframe) {
        draw_list->AddCircle(ImVec2(screen_center.x, screen_center.y), screen_radius, color, 16, 2.0f);
    } else {
        draw_list->AddCircleFilled(ImVec2(screen_center.x, screen_center.y), screen_radius, color, 16);
    }
#endif
}

void AudioSystemUI::draw_3d_line(const Vector3f& start, const Vector3f& end, ImU32 color, float thickness) {
#ifdef ECSCOPE_HAS_IMGUI
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    Vector3f screen_start = world_to_screen(start);
    Vector3f screen_end = world_to_screen(end);
    
    draw_list->AddLine(
        ImVec2(screen_start.x, screen_start.y),
        ImVec2(screen_end.x, screen_end.y),
        color, thickness
    );
#endif
}

void AudioSystemUI::handle_3d_viewport_input() {
#ifdef ECSCOPE_HAS_IMGUI
    if (ImGui::IsWindowHovered()) {
        ImGuiIO& io = ImGui::GetIO();
        
        // Handle mouse drag for camera rotation
        if (ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
            ImVec2 mouse_delta = ImGui::GetMouseDragDelta(ImGuiMouseButton_Left);
            
            // Rotate camera around target
            float sensitivity = 0.01f;
            float yaw = mouse_delta.x * sensitivity;
            float pitch = mouse_delta.y * sensitivity;
            
            // Update camera position (simplified rotation)
            Vector3f to_camera = {
                camera_position_.x - camera_target_.x,
                camera_position_.y - camera_target_.y,
                camera_position_.z - camera_target_.z
            };
            
            // Apply yaw rotation
            float cos_yaw = std::cos(yaw);
            float sin_yaw = std::sin(yaw);
            float new_x = to_camera.x * cos_yaw - to_camera.z * sin_yaw;
            float new_z = to_camera.x * sin_yaw + to_camera.z * cos_yaw;
            to_camera.x = new_x;
            to_camera.z = new_z;
            
            camera_position_ = {
                camera_target_.x + to_camera.x,
                camera_target_.y + to_camera.y,
                camera_target_.z + to_camera.z
            };
        }
        
        // Handle mouse wheel for zoom
        if (io.MouseWheel != 0.0f) {
            Vector3f to_target = {
                camera_target_.x - camera_position_.x,
                camera_target_.y - camera_position_.y,
                camera_target_.z - camera_position_.z
            };
            
            float zoom_factor = 1.0f + io.MouseWheel * 0.1f;
            camera_position_ = {
                camera_target_.x - to_target.x * zoom_factor,
                camera_target_.y - to_target.y * zoom_factor,
                camera_target_.z - to_target.z * zoom_factor
            };
        }
    }
#endif
}

// =============================================================================
// SPECIALIZED COMPONENT IMPLEMENTATIONS
// =============================================================================

Audio3DVisualizer::Audio3DVisualizer() = default;
Audio3DVisualizer::~Audio3DVisualizer() = default;

bool Audio3DVisualizer::initialize() {
    return true;
}

void Audio3DVisualizer::render(const std::unordered_map<uint32_t, AudioSourceVisual>& sources,
                              const std::unordered_map<uint32_t, AudioListenerVisual>& listeners,
                              const Vector3f& camera_pos, const Vector3f& camera_target) {
#ifdef ECSCOPE_HAS_IMGUI
    draw_list_ = ImGui::GetWindowDrawList();
    
    if (show_grid_) {
        // Draw ground grid
        ImU32 grid_color = GRID_COLOR;
        for (int i = -10; i <= 10; ++i) {
            // X lines
            Vector3f start{static_cast<float>(i) * grid_size_, 0.0f, -10.0f * grid_size_};
            Vector3f end{static_cast<float>(i) * grid_size_, 0.0f, 10.0f * grid_size_};
            // Draw line using simplified 3D projection
            
            // Z lines  
            start = {-10.0f * grid_size_, 0.0f, static_cast<float>(i) * grid_size_};
            end = {10.0f * grid_size_, 0.0f, static_cast<float>(i) * grid_size_};
            // Draw line using simplified 3D projection
        }
    }
#endif
}

AudioSpectrumAnalyzer::AudioSpectrumAnalyzer() = default;
AudioSpectrumAnalyzer::~AudioSpectrumAnalyzer() = default;

void AudioSpectrumAnalyzer::render(const AudioSpectrumData& data) {
#ifdef ECSCOPE_HAS_IMGUI
    if (data.frequencies.empty() || data.magnitudes.empty()) return;
    
    ImGui::Text("Frequency Spectrum");
    
    // Custom plotting implementation (ImPlot not available)
    {
    // Fallback to simple line drawing
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    ImVec2 canvas_pos = ImGui::GetCursorScreenPos();
    ImVec2 canvas_size = ImGui::GetContentRegionAvail();
    canvas_size.y = 200;
    
    draw_list->AddRectFilled(canvas_pos, 
                           ImVec2(canvas_pos.x + canvas_size.x, canvas_pos.y + canvas_size.y),
                           IM_COL32(50, 50, 50, 255));
    
    // Draw spectrum lines
    for (size_t i = 1; i < data.frequencies.size(); ++i) {
        float x1 = frequency_to_x_position(data.frequencies[i-1], min_frequency_, max_frequency_, 
                                          canvas_size.x, log_frequency_);
        float y1 = magnitude_to_y_position(data.magnitudes[i-1], min_magnitude_db_, max_magnitude_db_, 
                                          canvas_size.y);
        float x2 = frequency_to_x_position(data.frequencies[i], min_frequency_, max_frequency_, 
                                          canvas_size.x, log_frequency_);
        float y2 = magnitude_to_y_position(data.magnitudes[i], min_magnitude_db_, max_magnitude_db_, 
                                          canvas_size.y);
        
        draw_list->AddLine(ImVec2(canvas_pos.x + x1, canvas_pos.y + canvas_size.y - y1),
                          ImVec2(canvas_pos.x + x2, canvas_pos.y + canvas_size.y - y2),
                          IM_COL32(100, 200, 255, 255), 1.0f);
    }
    
    ImGui::Dummy(canvas_size);
    }
#endif
}

// Additional component implementations would follow similar patterns...

} // namespace ecscope::gui