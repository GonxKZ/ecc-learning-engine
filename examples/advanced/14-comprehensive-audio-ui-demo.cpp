/**
 * @file 14-comprehensive-audio-ui-demo.cpp
 * @brief Comprehensive Audio System UI Demo
 * 
 * Demonstrates professional-grade audio interface with:
 * - Real-time 3D visualization of audio sources and listeners
 * - HRTF processing visualization with head tracking
 * - Sound propagation and attenuation visualization  
 * - Audio effects chain editing with real-time preview
 * - Spatial audio controls and environmental presets
 * - Performance monitoring and debugging tools
 * 
 * @author ECScope Development Team
 * @version 1.0.0
 */

#include <iostream>
#include <memory>
#include <vector>
#include <chrono>
#include <thread>
#include <random>
#include <cmath>

// ECScope includes
#include "../../include/ecscope/core/log.hpp"
#include "../../include/ecscope/audio/audio_system.hpp"
#include "../../include/ecscope/gui/dashboard.hpp"
#include "../../include/ecscope/gui/audio_ui.hpp"
#include "../../include/ecscope/gui/audio_effects_ui.hpp"
#include "../../include/ecscope/gui/spatial_audio_ui.hpp"

// ImGui includes (if available)
#ifdef ECSCOPE_HAS_IMGUI
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <GLFW/glfw3.h>
#include <GL/gl3w.h>
#endif

using namespace ecscope;
using namespace ecscope::audio;
using namespace ecscope::gui;

// =============================================================================
// DEMO APPLICATION CLASS
// =============================================================================

class AudioUIDemo {
public:
    AudioUIDemo() = default;
    ~AudioUIDemo() = default;

    bool initialize() {
        // Initialize logging
        core::Logger::initialize();
        core::Logger::set_level(core::LogLevel::INFO);
        ECSCOPE_LOG_INFO("Initializing Comprehensive Audio UI Demo");

#ifdef ECSCOPE_HAS_IMGUI
        // Initialize GLFW
        if (!glfwInit()) {
            ECSCOPE_LOG_ERROR("Failed to initialize GLFW");
            return false;
        }

        // Create window
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

        window_ = glfwCreateWindow(1920, 1080, "ECScope Audio UI Demo", nullptr, nullptr);
        if (!window_) {
            ECSCOPE_LOG_ERROR("Failed to create GLFW window");
            glfwTerminate();
            return false;
        }

        glfwMakeContextCurrent(window_);
        glfwSwapInterval(1); // Enable vsync

        // Initialize OpenGL loader
        if (gl3wInit() != 0) {
            ECSCOPE_LOG_ERROR("Failed to initialize OpenGL loader");
            return false;
        }

        // Setup ImGui
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
        io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

        ImGui::StyleColorsDark();
        
        // Setup Platform/Renderer backends
        ImGui_ImplGlfw_InitForOpenGL(window_, true);
        ImGui_ImplOpenGL3_Init("#version 330");
#endif

        // Initialize audio system
        AudioSystemConfig audio_config = AudioSystemFactory::create_gaming_config();
        audio_config.enable_3d_audio = true;
        audio_config.enable_hrtf = true;
        audio_config.enable_ambisonics = true;
        audio_config.enable_ray_tracing = true;
        audio_config.enable_debugging = true;
        audio_config.enable_visualization = true;
        audio_config.log_level = AudioDebugLevel::INFO;
        
        audio_system_ = std::make_unique<AudioSystem>();
        if (!audio_system_->initialize(audio_config)) {
            ECSCOPE_LOG_ERROR("Failed to initialize audio system");
            return false;
        }

        // Initialize GUI dashboard
        dashboard_ = std::make_unique<Dashboard>();
        if (!dashboard_->initialize()) {
            ECSCOPE_LOG_ERROR("Failed to initialize dashboard");
            return false;
        }

        // Initialize audio UI
        audio_ui_ = std::make_unique<AudioSystemUI>();
        if (!audio_ui_->initialize(audio_system_.get(), dashboard_.get())) {
            ECSCOPE_LOG_ERROR("Failed to initialize audio UI");
            return false;
        }

        // Initialize effects editor
        effects_editor_ = std::make_unique<AudioEffectsChainEditor>();
        if (!effects_editor_->initialize(&audio_system_->get_pipeline())) {
            ECSCOPE_LOG_ERROR("Failed to initialize effects editor");
            return false;
        }

        // Initialize spatial audio controller
        spatial_controller_ = std::make_unique<SpatialAudioController>();
        if (!spatial_controller_->initialize(audio_system_.get())) {
            ECSCOPE_LOG_ERROR("Failed to initialize spatial controller");
            return false;
        }

        // Setup demo scene
        setup_demo_scene();

        ECSCOPE_LOG_INFO("Audio UI Demo initialized successfully");
        return true;
    }

    void run() {
        if (!initialize()) {
            ECSCOPE_LOG_ERROR("Failed to initialize demo");
            return;
        }

#ifdef ECSCOPE_HAS_IMGUI
        auto last_time = std::chrono::high_resolution_clock::now();

        while (!glfwWindowShouldClose(window_)) {
            glfwPollEvents();

            auto current_time = std::chrono::high_resolution_clock::now();
            float delta_time = std::chrono::duration<float>(current_time - last_time).count();
            last_time = current_time;

            update(delta_time);
            render();

            glfwSwapBuffers(window_);
        }
#else
        // Console-based demo without GUI
        ECSCOPE_LOG_INFO("Running console-based audio demo (GUI not available)");
        
        for (int i = 0; i < 100; ++i) {
            auto current_time = std::chrono::high_resolution_clock::now();
            float delta_time = 0.016f; // ~60 FPS
            
            update(delta_time);
            
            if (i % 30 == 0) { // Log every ~0.5 seconds
                auto metrics = audio_system_->get_system_metrics();
                ECSCOPE_LOG_INFO("Audio System - Active Voices: {}, CPU: {:.1f}%, Latency: {:.2f}ms",
                               metrics.active_voices, metrics.cpu_usage_percent, metrics.latency_ms);
            }
            
            std::this_thread::sleep_for(std::chrono::milliseconds(16));
        }
#endif

        shutdown();
    }

private:
    void setup_demo_scene() {
        ECSCOPE_LOG_INFO("Setting up demo scene");

        // Create listeners
        auto& audio_3d = audio_system_->get_3d_engine();
        
        // Main listener at origin
        AudioListener main_listener;
        main_listener.position = {0.0f, 1.8f, 0.0f}; // Human head height
        main_listener.forward = {0.0f, 0.0f, -1.0f};
        main_listener.up = {0.0f, 1.0f, 0.0f};
        audio_3d.set_listener(main_listener);

        // Register listener with UI
        AudioListenerVisual listener_visual;
        listener_visual.listener_id = 1;
        listener_visual.position = main_listener.position;
        listener_visual.forward = main_listener.forward;
        listener_visual.up = main_listener.up;
        listener_visual.is_active = true;
        listener_visual.show_orientation = true;
        listener_visual.show_hrtf_pattern = true;
        audio_ui_->register_audio_listener(1, listener_visual);
        audio_ui_->set_active_listener(1);

        // Create several audio sources for demonstration
        create_demo_audio_sources();

        // Setup environmental presets
        setup_environmental_presets();

        // Setup effects chain
        setup_effects_chain();

        // Enable various visualizations
        audio_ui_->enable_source_visualization(true);
        audio_ui_->enable_listener_visualization(true);
        audio_ui_->enable_reverb_zones(true);
        audio_ui_->enable_audio_rays(true);
        audio_ui_->enable_doppler_visualization(true);

        ECSCOPE_LOG_INFO("Demo scene setup complete");
    }

    void create_demo_audio_sources() {
        // Create a variety of audio sources for demonstration
        std::vector<std::string> demo_sounds = {
            "engine_idle.wav",
            "footsteps.wav", 
            "ambient_forest.wav",
            "music_loop.wav",
            "voice_dialogue.wav"
        };

        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<float> pos_dist(-10.0f, 10.0f);
        std::uniform_real_distribution<float> vol_dist(0.3f, 1.0f);

        for (size_t i = 0; i < demo_sounds.size(); ++i) {
            // Create audio source (would normally load from file)
            uint32_t source_id = i + 1;
            
            // Create visual representation
            AudioSourceVisual source_visual;
            source_visual.source_id = source_id;
            source_visual.position = {
                pos_dist(gen),
                pos_dist(gen) * 0.5f + 1.0f, // Keep sources roughly at head level
                pos_dist(gen)
            };
            source_visual.velocity = {0.0f, 0.0f, 0.0f};
            source_visual.direction = {0.0f, 0.0f, -1.0f};
            
            source_visual.volume = vol_dist(gen);
            source_visual.pitch = 1.0f;
            source_visual.min_distance = 1.0f;
            source_visual.max_distance = 20.0f;
            source_visual.radius = 0.3f;
            
            source_visual.is_playing = (i % 2 == 0); // Some playing, some not
            source_visual.show_attenuation_sphere = true;
            
            // Different colors for different source types
            switch (i % 5) {
                case 0: source_visual.color = IM_COL32(255, 100, 100, 255); break; // Red
                case 1: source_visual.color = IM_COL32(100, 255, 100, 255); break; // Green  
                case 2: source_visual.color = IM_COL32(100, 100, 255, 255); break; // Blue
                case 3: source_visual.color = IM_COL32(255, 255, 100, 255); break; // Yellow
                case 4: source_visual.color = IM_COL32(255, 100, 255, 255); break; // Magenta
            }

            // Setup directional sources
            if (i == 1) { // Footsteps - directional
                source_visual.cone_inner_angle = 90.0f;
                source_visual.cone_outer_angle = 120.0f;
                source_visual.show_cone = true;
            }

            // Moving source for Doppler demonstration
            if (i == 2) {
                source_visual.velocity = {2.0f, 0.0f, 0.0f}; // Moving right
            }

            audio_ui_->register_audio_source(source_id, source_visual);
            
            // Generate simulated spectrum data
            generate_demo_spectrum_data(source_id);
        }

        // Select first source by default
        audio_ui_->select_audio_source(1);
        
        ECSCOPE_LOG_INFO("Created {} demo audio sources", demo_sounds.size());
    }

    void generate_demo_spectrum_data(uint32_t source_id) {
        // Generate realistic-looking spectrum data for visualization
        AudioSpectrumData spectrum;
        spectrum.sample_rate = 48000.0f;
        spectrum.fft_size = 2048;
        spectrum.frequencies.resize(1024);
        spectrum.magnitudes.resize(1024);
        spectrum.timestamp = std::chrono::steady_clock::now();

        std::random_device rd;
        std::mt19937 gen(rd());
        std::normal_distribution<float> noise_dist(0.0f, 3.0f);

        for (size_t i = 0; i < spectrum.frequencies.size(); ++i) {
            spectrum.frequencies[i] = (static_cast<float>(i) * spectrum.sample_rate) / (2.0f * 1024.0f);
            
            // Create different spectral shapes for different sources
            float base_level = -40.0f;
            switch (source_id % 5) {
                case 1: // Engine - low frequency dominant
                    base_level = -30.0f - (i * 0.02f) + 10.0f * std::exp(-i * 0.01f);
                    break;
                case 2: // Footsteps - mid frequency spikes
                    base_level = -45.0f + 15.0f * std::sin(i * 0.1f) * std::exp(-i * 0.005f);
                    break;
                case 3: // Ambient - broadband
                    base_level = -35.0f - (i * 0.01f) + noise_dist(gen);
                    break;
                case 4: // Music - harmonic structure
                    base_level = -25.0f - (i * 0.015f) + 8.0f * std::sin(i * 0.05f);
                    break;
                case 0: // Voice - formant structure
                    base_level = -30.0f + 12.0f * std::exp(-std::pow((i - 200) / 100.0f, 2));
                    base_level += 8.0f * std::exp(-std::pow((i - 500) / 150.0f, 2));
                    break;
            }
            
            spectrum.magnitudes[i] = base_level + noise_dist(gen);
        }

        audio_ui_->update_spectrum_data(source_id, spectrum);

        // Also generate waveform data
        AudioWaveformData waveform;
        waveform.sample_rate = 48000.0f;
        waveform.duration_seconds = 1.0f;
        waveform.samples_left.resize(48000);
        waveform.samples_right.resize(48000);
        waveform.timestamp = std::chrono::steady_clock::now();

        std::uniform_real_distribution<float> wave_dist(-0.5f, 0.5f);
        for (size_t i = 0; i < waveform.samples_left.size(); ++i) {
            float t = static_cast<float>(i) / waveform.sample_rate;
            float sample = 0.3f * std::sin(2.0f * 3.14159f * 440.0f * t) + 0.1f * wave_dist(gen);
            waveform.samples_left[i] = sample;
            waveform.samples_right[i] = sample * 0.8f; // Slightly different for stereo
        }

        audio_ui_->update_waveform_data(source_id, waveform);
    }

    void setup_environmental_presets() {
        // This would typically load presets from the spatial controller
        // For demo purposes, we'll just enable some environmental effects
        
        if (spatial_controller_) {
            spatial_controller_->enable_ambisonics(true);
            spatial_controller_->set_ambisonics_order(1); // First order ambisonics
            spatial_controller_->enable_ray_tracing(true);
            spatial_controller_->set_ray_tracing_quality(5); // Medium quality
            spatial_controller_->apply_environmental_preset("Indoor Medium Room");
        }

        ECSCOPE_LOG_INFO("Environmental presets configured");
    }

    void setup_effects_chain() {
        if (effects_editor_) {
            // Add some demo effects to the chain
            effects_editor_->add_effect("EQ");
            effects_editor_->add_effect("Compressor");
            effects_editor_->add_effect("Reverb");
            effects_editor_->add_effect("Limiter");
            
            effects_editor_->enable_audio_analysis(true);
            effects_editor_->enable_performance_monitoring(true);
        }

        ECSCOPE_LOG_INFO("Effects chain configured");
    }

    void update(float delta_time) {
        // Update audio system
        if (audio_system_) {
            audio_system_->update(delta_time);
        }

        // Update dashboard
        if (dashboard_) {
            dashboard_->update(delta_time);
        }

        // Update audio UI
        if (audio_ui_) {
            audio_ui_->update(delta_time);
        }

        // Update effects editor
        if (effects_editor_) {
            effects_editor_->update(delta_time);
        }

        // Update spatial controller
        if (spatial_controller_) {
            spatial_controller_->update(delta_time);
        }

        // Update demo scene (animate sources, etc.)
        update_demo_scene(delta_time);

        // Update performance metrics
        update_performance_metrics();
    }

    void update_demo_scene(float delta_time) {
        static float scene_time = 0.0f;
        scene_time += delta_time;

        // Animate moving source (Doppler effect demo)
        uint32_t moving_source_id = 3;
        auto* moving_source = get_source_visual(moving_source_id);
        if (moving_source) {
            // Circular motion
            float radius = 5.0f;
            float angular_speed = 0.5f; // rad/s
            Vector3f new_pos = {
                radius * std::cos(scene_time * angular_speed),
                1.8f,
                radius * std::sin(scene_time * angular_speed)
            };
            Vector3f new_velocity = {
                -radius * angular_speed * std::sin(scene_time * angular_speed),
                0.0f,
                radius * angular_speed * std::cos(scene_time * angular_speed)
            };
            
            moving_source->position = new_pos;
            moving_source->velocity = new_velocity;
            audio_ui_->update_source_visual(moving_source_id, *moving_source);
        }

        // Generate some demo audio rays
        if (scene_time - last_ray_update_ > 0.1f) { // Update rays every 100ms
            generate_demo_audio_rays();
            last_ray_update_ = scene_time;
        }

        // Update spectrum data periodically
        if (scene_time - last_spectrum_update_ > 0.05f) { // Update spectrum every 50ms
            for (uint32_t source_id = 1; source_id <= 5; ++source_id) {
                generate_demo_spectrum_data(source_id);
            }
            last_spectrum_update_ = scene_time;
        }
    }

    void generate_demo_audio_rays() {
        audio_ui_->clear_audio_rays();

        // Generate rays from playing sources to active listener
        AudioListenerVisual* listener = get_listener_visual(1);
        if (!listener) return;

        for (uint32_t source_id = 1; source_id <= 5; ++source_id) {
            AudioSourceVisual* source = get_source_visual(source_id);
            if (!source || !source->is_playing) continue;

            // Direct ray
            AudioRayVisual direct_ray;
            direct_ray.start = source->position;
            direct_ray.end = listener->position;
            direct_ray.bounce_count = 0;
            direct_ray.intensity = source->volume;
            direct_ray.color = IM_COL32(255, 255, 0, 200);
            direct_ray.is_occluded = false;
            audio_ui_->add_audio_ray(direct_ray);

            // Simulated reflection (simplified)
            AudioRayVisual reflected_ray;
            reflected_ray.start = source->position;
            reflected_ray.end = {listener->position.x + 2.0f, listener->position.y, listener->position.z};
            reflected_ray.reflection_point = {source->position.x, 0.0f, source->position.z}; // Floor reflection
            reflected_ray.bounce_count = 1;
            reflected_ray.intensity = source->volume * 0.3f;
            reflected_ray.color = IM_COL32(255, 128, 0, 150);
            reflected_ray.is_occluded = false;
            audio_ui_->add_audio_ray(reflected_ray);
        }
    }

    void update_performance_metrics() {
        if (audio_system_) {
            auto metrics = audio_system_->get_system_metrics();
            audio_ui_->update_performance_metrics(metrics);
        }
    }

    void render() {
#ifdef ECSCOPE_HAS_IMGUI
        // Start ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // Setup docking
        ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->Pos);
        ImGui::SetNextWindowSize(viewport->Size);
        ImGui::SetNextWindowViewport(viewport->ID);
        
        ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
        window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse;
        window_flags |= ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
        window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

        bool open = true;
        ImGui::Begin("ECScope Audio UI Demo", &open, window_flags);

        // Create docking space
        ImGuiID dockspace_id = ImGui::GetID("MainDockspace");
        ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_None);

        // Render menu bar
        render_menu_bar();

        // Render main dashboard
        if (dashboard_) {
            dashboard_->render();
        }

        // Render audio UI
        if (audio_ui_) {
            audio_ui_->render();
        }

        // Render effects editor
        if (effects_editor_ && show_effects_editor_) {
            if (ImGui::Begin("Effects Chain Editor", &show_effects_editor_)) {
                effects_editor_->render();
            }
            ImGui::End();
        }

        // Render spatial controller
        if (spatial_controller_ && show_spatial_controller_) {
            if (ImGui::Begin("Spatial Audio Controller", &show_spatial_controller_)) {
                spatial_controller_->render();
            }
            ImGui::End();
        }

        // Render demo info panel
        render_demo_info();

        ImGui::End(); // Main window

        // Render
        ImGui::Render();
        
        int display_w, display_h;
        glfwGetFramebufferSize(window_, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClear(GL_COLOR_BUFFER_BIT);
        
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        // Handle multi-viewport
        ImGuiIO& io = ImGui::GetIO();
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
            GLFWwindow* backup_current_context = glfwGetCurrentContext();
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
            glfwMakeContextCurrent(backup_current_context);
        }
#endif
    }

#ifdef ECSCOPE_HAS_IMGUI
    void render_menu_bar() {
        if (ImGui::BeginMenuBar()) {
            if (ImGui::BeginMenu("Audio")) {
                ImGui::MenuItem("Effects Chain Editor", nullptr, &show_effects_editor_);
                ImGui::MenuItem("Spatial Controller", nullptr, &show_spatial_controller_);
                ImGui::Separator();
                if (ImGui::MenuItem("Reset Demo Scene")) {
                    setup_demo_scene();
                }
                ImGui::EndMenu();
            }
            
            if (ImGui::BeginMenu("View")) {
                if (ImGui::MenuItem("Show Audio Sources", nullptr, &show_sources_)) {
                    audio_ui_->enable_source_visualization(show_sources_);
                }
                if (ImGui::MenuItem("Show Listeners", nullptr, &show_listeners_)) {
                    audio_ui_->enable_listener_visualization(show_listeners_);
                }
                if (ImGui::MenuItem("Show Audio Rays", nullptr, &show_rays_)) {
                    audio_ui_->enable_audio_rays(show_rays_);
                }
                if (ImGui::MenuItem("Show Doppler Effects", nullptr, &show_doppler_)) {
                    audio_ui_->enable_doppler_visualization(show_doppler_);
                }
                ImGui::EndMenu();
            }
            
            if (ImGui::BeginMenu("Help")) {
                ImGui::MenuItem("Demo Info", nullptr, &show_demo_info_);
                if (ImGui::MenuItem("About")) {
                    ECSCOPE_LOG_INFO("ECScope Audio UI Demo v1.0.0");
                }
                ImGui::EndMenu();
            }
            
            ImGui::EndMenuBar();
        }
    }

    void render_demo_info() {
        if (show_demo_info_) {
            if (ImGui::Begin("Demo Information", &show_demo_info_)) {
                ImGui::Text("ECScope Comprehensive Audio UI Demo");
                ImGui::Separator();
                
                ImGui::Text("Features Demonstrated:");
                ImGui::BulletText("Real-time 3D audio visualization");
                ImGui::BulletText("HRTF processing with head tracking");
                ImGui::BulletText("Sound propagation and ray tracing");
                ImGui::BulletText("Audio effects chain editing");
                ImGui::BulletText("Spatial audio controls");
                ImGui::BulletText("Environmental audio presets");
                ImGui::BulletText("Performance monitoring");
                
                ImGui::Separator();
                
                ImGui::Text("Controls:");
                ImGui::BulletText("Left mouse: Rotate 3D view");
                ImGui::BulletText("Mouse wheel: Zoom in/out");
                ImGui::BulletText("Select sources to edit properties");
                ImGui::BulletText("Drag sliders to adjust parameters");
                
                ImGui::Separator();
                
                if (audio_system_) {
                    auto metrics = audio_system_->get_system_metrics();
                    ImGui::Text("System Status:");
                    ImGui::Text("Active Voices: %u", metrics.active_voices);
                    ImGui::Text("CPU Usage: %.1f%%", metrics.cpu_usage_percent);
                    ImGui::Text("Memory Usage: %.1f MB", metrics.memory_usage_mb);
                    ImGui::Text("Latency: %.2f ms", metrics.latency_ms);
                }
            }
            ImGui::End();
        }
    }
#endif

    // Helper methods to access visual data
    AudioSourceVisual* get_source_visual(uint32_t source_id) {
        // This would normally access the audio UI's internal data
        // For demo purposes, we'll use a local cache
        auto it = demo_source_visuals_.find(source_id);
        return (it != demo_source_visuals_.end()) ? &it->second : nullptr;
    }

    AudioListenerVisual* get_listener_visual(uint32_t listener_id) {
        auto it = demo_listener_visuals_.find(listener_id);
        return (it != demo_listener_visuals_.end()) ? &it->second : nullptr;
    }

    void shutdown() {
        ECSCOPE_LOG_INFO("Shutting down Audio UI Demo");

        if (spatial_controller_) {
            spatial_controller_->shutdown();
            spatial_controller_.reset();
        }

        if (effects_editor_) {
            effects_editor_->shutdown();
            effects_editor_.reset();
        }

        if (audio_ui_) {
            audio_ui_->shutdown();
            audio_ui_.reset();
        }

        if (dashboard_) {
            dashboard_->shutdown();
            dashboard_.reset();
        }

        if (audio_system_) {
            audio_system_->shutdown();
            audio_system_.reset();
        }

#ifdef ECSCOPE_HAS_IMGUI
        if (window_) {
            ImGui_ImplOpenGL3_Shutdown();
            ImGui_ImplGlfw_Shutdown();
            ImGui::DestroyContext();

            glfwDestroyWindow(window_);
            glfwTerminate();
        }
#endif

        core::Logger::shutdown();
    }

private:
    // Core systems
    std::unique_ptr<AudioSystem> audio_system_;
    std::unique_ptr<Dashboard> dashboard_;
    std::unique_ptr<AudioSystemUI> audio_ui_;
    std::unique_ptr<AudioEffectsChainEditor> effects_editor_;
    std::unique_ptr<SpatialAudioController> spatial_controller_;

#ifdef ECSCOPE_HAS_IMGUI
    GLFWwindow* window_ = nullptr;
#endif

    // Demo state
    std::unordered_map<uint32_t, AudioSourceVisual> demo_source_visuals_;
    std::unordered_map<uint32_t, AudioListenerVisual> demo_listener_visuals_;
    float last_ray_update_ = 0.0f;
    float last_spectrum_update_ = 0.0f;

    // UI state
    bool show_effects_editor_ = true;
    bool show_spatial_controller_ = true;
    bool show_demo_info_ = true;
    bool show_sources_ = true;
    bool show_listeners_ = true;
    bool show_rays_ = true;
    bool show_doppler_ = true;
};

// =============================================================================
// MAIN FUNCTION
// =============================================================================

int main(int argc, char* argv[]) {
    try {
        AudioUIDemo demo;
        demo.run();
        return 0;
    }
    catch (const std::exception& e) {
        std::cerr << "Demo failed with exception: " << e.what() << std::endl;
        return 1;
    }
    catch (...) {
        std::cerr << "Demo failed with unknown exception" << std::endl;
        return 1;
    }
}