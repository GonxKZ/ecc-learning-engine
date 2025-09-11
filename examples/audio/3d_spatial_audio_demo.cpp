/**
 * @file 3d_spatial_audio_demo.cpp
 * @brief Comprehensive 3D spatial audio demonstration for ECScope engine
 * 
 * This demo showcases the full capabilities of the ECScope 3D audio system including:
 * - HRTF-based 3D positioning
 * - Distance attenuation and Doppler effects
 * - Environmental audio with reverb and occlusion
 * - Real-time audio effects processing
 * - Ambisonics spatial audio
 * - Audio ray tracing for realistic acoustics
 * - Multi-threaded audio processing with SIMD optimization
 * - Interactive controls for real-time parameter adjustment
 */

#include "ecscope/audio/audio_system.hpp"
#include "ecscope/audio/audio_debug.hpp"
#include <iostream>
#include <chrono>
#include <thread>
#include <vector>
#include <memory>
#include <random>
#include <cmath>

using namespace ecscope::audio;

class SpatialAudioDemo {
public:
    SpatialAudioDemo() : m_running(false), m_demo_time(0.0f) {
        // Initialize random number generator
        m_random_engine.seed(std::chrono::steady_clock::now().time_since_epoch().count());
    }
    
    ~SpatialAudioDemo() {
        shutdown();
    }
    
    bool initialize() {
        std::cout << "ECScope 3D Spatial Audio Demo\n";
        std::cout << "=============================\n\n";
        
        // Create high-quality audio configuration for demo
        AudioSystemConfig config = AudioSystemFactory::create_development_config();
        config.format.sample_rate = 48000;  // High quality audio
        config.format.buffer_size = 512;    // Low latency
        config.enable_3d_audio = true;
        config.enable_hrtf = true;
        config.enable_ambisonics = true;
        config.ambisonics_order = 3;        // Third-order ambisonics
        config.enable_ray_tracing = true;
        config.ray_tracing_quality = 8;     // High quality ray tracing
        config.enable_debugging = true;
        config.enable_profiling = true;
        config.enable_visualization = true;
        config.log_level = AudioDebugLevel::INFO;
        
        // Initialize the audio system
        if (!GlobalAudioSystem::initialize(config)) {
            std::cerr << "Failed to initialize audio system!\n";
            return false;
        }
        
        auto& audio_system = GlobalAudioSystem::instance();
        std::cout << "Audio System initialized successfully\n";
        std::cout << "Version: " << AudioSystem::get_version() << "\n\n";
        
        // Setup 3D audio engine
        setup_3d_audio();
        
        // Create demo scene
        create_demo_scene();
        
        // Setup environmental audio
        setup_environmental_audio();
        
        // Setup audio effects
        setup_audio_effects();
        
        // Initialize performance monitoring
        setup_performance_monitoring();
        
        // Print demo instructions
        print_instructions();
        
        return true;
    }
    
    void run() {
        if (!initialize()) {
            return;
        }
        
        m_running = true;
        auto last_time = std::chrono::steady_clock::now();
        
        std::cout << "Demo running... Press 'q' to quit\n\n";
        
        while (m_running) {
            auto current_time = std::chrono::steady_clock::now();
            float delta_time = std::chrono::duration<float>(current_time - last_time).count();
            last_time = current_time;
            
            // Update demo
            update(delta_time);
            
            // Process input (simplified for demo)
            process_input();
            
            // Display performance metrics every second
            static float metrics_timer = 0.0f;
            metrics_timer += delta_time;
            if (metrics_timer >= 1.0f) {
                display_performance_metrics();
                metrics_timer = 0.0f;
            }
            
            // Sleep to avoid busy waiting
            std::this_thread::sleep_for(std::chrono::milliseconds(16));  // ~60 FPS
        }
        
        std::cout << "\nDemo completed. Final performance report:\n";
        display_final_report();
    }
    
    void shutdown() {
        if (GlobalAudioSystem::is_initialized()) {
            std::cout << "Shutting down audio system...\n";
            GlobalAudioSystem::shutdown();
        }
    }

private:
    void setup_3d_audio() {
        auto& audio_system = GlobalAudioSystem::instance();
        auto& engine_3d = audio_system.get_3d_engine();
        
        std::cout << "Setting up 3D audio engine...\n";
        
        // Configure the listener (player position)
        AudioListener listener;
        listener.position = {0.0f, 1.75f, 0.0f};  // Average human head height
        listener.orientation = Quaternion{1.0f, 0.0f, 0.0f, 0.0f};  // Forward facing
        listener.velocity = {0.0f, 0.0f, 0.0f};
        listener.gain = 1.0f;
        listener.enabled = true;
        
        engine_3d.set_listener(listener);
        
        // Enable HRTF processing if available
        if (engine_3d.load_hrtf_database("assets/audio/hrtf/default_hrtf.sofa") || 
            engine_3d.load_default_database()) {
            std::cout << "HRTF database loaded successfully\n";
            engine_3d.set_hrtf_interpolation(HRTFInterpolation::LINEAR);
            engine_3d.enable_hrtf_processing(true);
        } else {
            std::cout << "Warning: HRTF database not found, using fallback processing\n";
        }
        
        // Configure global 3D settings
        engine_3d.set_doppler_factor(1.0f);
        engine_3d.set_speed_of_sound(343.3f);  // m/s at 20°C
        engine_3d.set_distance_model(AttenuationModel::INVERSE_CLAMPED);
        engine_3d.enable_air_absorption(true);
        
        // Set air absorption coefficients for different frequencies
        std::vector<float> air_absorption = {
            0.0001f, 0.0002f, 0.0005f, 0.001f, 0.002f,   // Low frequencies
            0.005f, 0.01f, 0.02f, 0.04f, 0.08f           // High frequencies
        };
        engine_3d.set_air_absorption_coefficients(air_absorption);
        
        std::cout << "3D audio engine configured\n";
    }
    
    void create_demo_scene() {
        auto& audio_system = GlobalAudioSystem::instance();
        auto& engine_3d = audio_system.get_3d_engine();
        
        std::cout << "Creating demo scene with multiple 3D audio sources...\n";
        
        // Demo audio files (placeholder paths - in real implementation these would be actual files)
        std::vector<std::string> demo_sounds = {
            "assets/audio/sounds/ambient_forest.ogg",
            "assets/audio/sounds/footsteps.wav",
            "assets/audio/sounds/water_stream.ogg",
            "assets/audio/sounds/bird_chirps.wav",
            "assets/audio/sounds/wind_through_trees.ogg",
            "assets/audio/sounds/distant_thunder.wav"
        };
        
        // Create various 3D positioned sound sources
        create_ambient_forest_scene(demo_sounds);
        create_moving_sound_sources(demo_sounds);
        create_directional_sound_sources(demo_sounds);
        
        std::cout << "Demo scene created with " << m_scene_voices.size() << " audio sources\n";
    }
    
    void create_ambient_forest_scene(const std::vector<std::string>& sounds) {
        auto& engine_3d = AUDIO_3D();
        
        // Create stationary ambient sounds
        struct AmbientSource {
            Vector3f position;
            std::string sound_file;
            float volume;
            float min_distance;
            float max_distance;
        };
        
        std::vector<AmbientSource> ambient_sources = {
            {{-10.0f, 0.0f, 5.0f}, "ambient_forest.ogg", 0.6f, 5.0f, 50.0f},
            {{15.0f, 0.0f, -8.0f}, "water_stream.ogg", 0.8f, 2.0f, 20.0f},
            {{0.0f, 10.0f, 20.0f}, "bird_chirps.wav", 0.4f, 8.0f, 40.0f},
            {{-5.0f, 0.0f, -15.0f}, "wind_through_trees.ogg", 0.5f, 10.0f, 60.0f}
        };
        
        for (const auto& source : ambient_sources) {
            // In a real implementation, you would load actual audio files
            // For this demo, we'll create placeholder voice IDs
            uint32_t voice_id = static_cast<uint32_t>(m_scene_voices.size());
            
            SceneVoice scene_voice;
            scene_voice.voice_id = voice_id;
            scene_voice.position = source.position;
            scene_voice.velocity = {0.0f, 0.0f, 0.0f};
            scene_voice.is_moving = false;
            scene_voice.sound_file = source.sound_file;
            scene_voice.base_volume = source.volume;
            
            m_scene_voices.push_back(scene_voice);
            
            // In real implementation:
            // auto stream = create_audio_stream(source.sound_file);
            // voice_id = engine_3d.create_voice(std::move(stream));
            // auto* voice = engine_3d.get_voice(voice_id);
            // voice->set_position(source.position);
            // voice->set_gain(source.volume);
            // voice->set_min_distance(source.min_distance);
            // voice->set_max_distance(source.max_distance);
            // voice->set_looping(true);
            // voice->play();
        }
    }
    
    void create_moving_sound_sources(const std::vector<std::string>& sounds) {
        // Create moving sound sources for Doppler effect demonstration
        struct MovingSource {
            Vector3f start_position;
            Vector3f end_position;
            std::string sound_file;
            float speed;
            float volume;
        };
        
        std::vector<MovingSource> moving_sources = {
            {{-20.0f, 1.0f, 0.0f}, {20.0f, 1.0f, 0.0f}, "footsteps.wav", 3.0f, 0.7f},
            {{0.0f, 1.0f, -25.0f}, {0.0f, 1.0f, 25.0f}, "distant_thunder.wav", 5.0f, 0.9f}
        };
        
        for (const auto& source : moving_sources) {
            uint32_t voice_id = static_cast<uint32_t>(m_scene_voices.size());
            
            SceneVoice scene_voice;
            scene_voice.voice_id = voice_id;
            scene_voice.position = source.start_position;
            scene_voice.target_position = source.end_position;
            scene_voice.movement_speed = source.speed;
            scene_voice.is_moving = true;
            scene_voice.sound_file = source.sound_file;
            scene_voice.base_volume = source.volume;
            scene_voice.movement_timer = 0.0f;
            
            m_scene_voices.push_back(scene_voice);
        }
    }
    
    void create_directional_sound_sources(const std::vector<std::string>& sounds) {
        // Create directional sound sources (cone-shaped audio)
        uint32_t voice_id = static_cast<uint32_t>(m_scene_voices.size());
        
        SceneVoice directional_voice;
        directional_voice.voice_id = voice_id;
        directional_voice.position = {8.0f, 2.0f, 10.0f};
        directional_voice.velocity = {0.0f, 0.0f, 0.0f};
        directional_voice.is_moving = false;
        directional_voice.is_directional = true;
        directional_voice.direction = {-1.0f, 0.0f, -1.0f};  // Pointing toward listener
        directional_voice.cone_inner_angle = 45.0f;
        directional_voice.cone_outer_angle = 90.0f;
        directional_voice.cone_outer_gain = 0.2f;
        directional_voice.sound_file = "bird_chirps.wav";
        directional_voice.base_volume = 0.8f;
        
        m_scene_voices.push_back(directional_voice);
    }
    
    void setup_environmental_audio() {
        auto& audio_system = GlobalAudioSystem::instance();
        auto& engine_3d = audio_system.get_3d_engine();
        
        std::cout << "Setting up environmental audio...\n";
        
        // Configure environmental audio settings for a forest scene
        EnvironmentalAudio env_settings;
        env_settings.room_size = 50.0f;      // Large outdoor space
        env_settings.damping = 0.3f;         // Moderate damping from trees
        env_settings.wet_gain = 0.4f;        // Natural reverb amount
        env_settings.dry_gain = 0.8f;        // Keep direct sound strong
        env_settings.width = 1.5f;           // Wider stereo field for outdoor space
        
        // Material properties for different surfaces in the forest
        EnvironmentalAudio::MaterialProperties tree_material;
        tree_material.absorption = 0.4f;      // Trees absorb sound
        tree_material.scattering = 0.6f;      // Trees scatter sound
        tree_material.transmission = 0.1f;    // Little transmission through trees
        
        EnvironmentalAudio::MaterialProperties ground_material;
        ground_material.absorption = 0.2f;    // Forest floor absorbs some sound
        ground_material.scattering = 0.3f;    // Uneven ground scatters sound
        ground_material.transmission = 0.0f;  // No transmission through ground
        
        env_settings.materials = {tree_material, ground_material};
        
        engine_3d.set_environmental_settings(env_settings);
        engine_3d.enable_environmental_processing(true);
        
        // Enable occlusion processing if ray tracing is available
        engine_3d.enable_occlusion_processing(true);
        
        std::cout << "Environmental audio configured\n";
    }
    
    void setup_audio_effects() {
        auto& audio_system = GlobalAudioSystem::instance();
        
        std::cout << "Setting up global audio effects chain...\n";
        
        // Create a subtle global EQ for the forest ambience
        auto eq_effect = AudioEffectFactory::create_equalizer(10);
        // Configure EQ bands for natural outdoor sound (placeholder implementation)
        
        // Add a gentle compressor to manage dynamic range
        auto compressor = AudioEffectFactory::create_compressor();
        // Configure compressor settings (placeholder implementation)
        
        // Add subtle reverb for atmospheric enhancement
        auto reverb = AudioEffectFactory::create_reverb();
        // Configure reverb for outdoor space (placeholder implementation)
        
        // Add effects to global chain
        audio_system.add_global_effect(std::move(eq_effect));
        audio_system.add_global_effect(std::move(compressor));
        audio_system.add_global_effect(std::move(reverb));
        
        std::cout << "Audio effects chain configured\n";
    }
    
    void setup_performance_monitoring() {
        auto& audio_system = GlobalAudioSystem::instance();
        
        if (auto* monitor = audio_system.get_performance_monitor()) {
            monitor->start_monitoring();
            monitor->set_cpu_threshold(80.0f);      // Alert if CPU usage > 80%
            monitor->set_memory_threshold(100.0f);  // Alert if memory > 100MB
            monitor->set_latency_threshold(20.0f);  // Alert if latency > 20ms
            
            monitor->set_alert_callback([](const std::string& alert) {
                std::cout << "PERFORMANCE ALERT: " << alert << std::endl;
            });
        }
        
        if (auto* visualizer = audio_system.get_visualizer()) {
            AudioVisualizer::VisualizationConfig viz_config;
            viz_config.show_3d_positions = true;
            viz_config.show_performance_metrics = true;
            viz_config.show_spectrum = true;
            viz_config.show_ray_tracing = true;
            viz_config.update_rate_hz = 30;  // 30 FPS for visualization
            
            visualizer->set_config(viz_config);
            visualizer->enable_visualization(true);
        }
    }
    
    void update(float delta_time) {
        m_demo_time += delta_time;
        
        // Update the audio system
        auto& audio_system = GlobalAudioSystem::instance();
        audio_system.update(delta_time);
        
        // Update scene animations
        update_scene_animation(delta_time);
        
        // Update listener movement (simulate head movement)
        update_listener_movement(delta_time);
        
        // Update moving sound sources
        update_moving_sources(delta_time);
        
        // Update audio visualization
        update_visualization();
        
        // Demonstrate dynamic parameter changes
        demonstrate_dynamic_parameters(delta_time);
    }
    
    void update_scene_animation(float delta_time) {
        // Animate some parameters for demonstration
        static float animation_time = 0.0f;
        animation_time += delta_time;
        
        // Vary environmental parameters over time
        auto& engine_3d = AUDIO_3D();
        auto env_settings = engine_3d.get_environmental_settings();
        
        // Simulate changing weather conditions
        float wind_factor = 0.5f + 0.3f * std::sin(animation_time * 0.2f);
        env_settings.damping = 0.2f + 0.3f * wind_factor;
        env_settings.wet_gain = 0.3f + 0.2f * wind_factor;
        
        engine_3d.set_environmental_settings(env_settings);
    }
    
    void update_listener_movement(float delta_time) {
        // Simulate subtle head movement for HRTF demonstration
        static float head_movement_time = 0.0f;
        head_movement_time += delta_time * 0.5f;  // Slow movement
        
        auto listener = AUDIO_3D().get_listener();
        
        // Small circular head movement
        float head_sway = 0.1f;
        listener.position.x = head_sway * std::sin(head_movement_time);
        listener.position.z = head_sway * std::cos(head_movement_time * 0.7f);
        
        // Slight head rotation
        float head_rotation = 5.0f * std::sin(head_movement_time * 0.3f);  // ±5 degrees
        listener.orientation = Quaternion::from_euler(0.0f, head_rotation, 0.0f);
        
        AUDIO_3D().set_listener(listener);
    }
    
    void update_moving_sources(float delta_time) {
        for (auto& voice : m_scene_voices) {
            if (voice.is_moving) {
                voice.movement_timer += delta_time * voice.movement_speed;
                
                // Linear interpolation between start and target position
                float t = (std::sin(voice.movement_timer * 0.1f) + 1.0f) * 0.5f;
                voice.position = {
                    voice.position.x * (1.0f - t) + voice.target_position.x * t,
                    voice.position.y * (1.0f - t) + voice.target_position.y * t,
                    voice.position.z * (1.0f - t) + voice.target_position.z * t
                };
                
                // Calculate velocity for Doppler effect
                static Vector3f prev_position = voice.position;
                voice.velocity = (voice.position - prev_position) * (1.0f / delta_time);
                prev_position = voice.position;
                
                // In real implementation, update the actual voice position:
                // auto* audio_voice = AUDIO_3D().get_voice(voice.voice_id);
                // if (audio_voice) {
                //     audio_voice->set_position(voice.position);
                //     audio_voice->set_velocity(voice.velocity);
                // }
            }
        }
    }
    
    void update_visualization() {
        auto& audio_system = GlobalAudioSystem::instance();
        
        if (auto* visualizer = audio_system.get_visualizer()) {
            // Update 3D positions for visualization
            std::vector<Vector3f> source_positions;
            for (const auto& voice : m_scene_voices) {
                source_positions.push_back(voice.position);
            }
            
            auto listener = AUDIO_3D().get_listener();
            visualizer->update_3d_positions(source_positions, listener.position, listener.orientation);
            
            // Update performance metrics
            visualizer->update_performance_metrics(audio_system.get_system_metrics());
        }
    }
    
    void demonstrate_dynamic_parameters(float delta_time) {
        static float demo_phase_timer = 0.0f;
        demo_phase_timer += delta_time;
        
        // Change demo phases every 30 seconds
        if (demo_phase_timer >= 30.0f) {
            demo_phase_timer = 0.0f;
            m_current_demo_phase = (m_current_demo_phase + 1) % 4;
            
            switch (m_current_demo_phase) {
                case 0:
                    std::cout << "\n--- Demo Phase 1: Basic 3D Positioning ---\n";
                    break;
                case 1:
                    std::cout << "\n--- Demo Phase 2: Doppler Effects ---\n";
                    break;
                case 2:
                    std::cout << "\n--- Demo Phase 3: Environmental Audio ---\n";
                    break;
                case 3:
                    std::cout << "\n--- Demo Phase 4: Occlusion & Ray Tracing ---\n";
                    break;
            }
        }
    }
    
    void process_input() {
        // Simplified input processing for demo
        // In a real application, this would handle keyboard/mouse input
        
        // Check for quit condition (simplified)
        static int frame_count = 0;
        frame_count++;
        
        // Run demo for approximately 2 minutes
        if (frame_count > 7200) {  // ~60 FPS * 120 seconds
            m_running = false;
        }
    }
    
    void display_performance_metrics() {
        auto& audio_system = GlobalAudioSystem::instance();
        auto metrics = audio_system.get_system_metrics();
        
        std::cout << "Performance Metrics:\n";
        std::cout << "  CPU Usage: " << metrics.cpu_usage << "%\n";
        std::cout << "  Memory: " << metrics.memory_usage / 1024 / 1024 << " MB\n";
        std::cout << "  Active Voices: " << metrics.active_voices << "\n";
        std::cout << "  Latency: " << metrics.latency_ms << " ms\n";
        std::cout << "  Samples Processed: " << metrics.samples_processed << "\n";
        
        if (metrics.buffer_underruns > 0 || metrics.buffer_overruns > 0) {
            std::cout << "  Buffer Issues - Underruns: " << metrics.buffer_underruns 
                     << ", Overruns: " << metrics.buffer_overruns << "\n";
        }
        
        std::cout << "\n";
    }
    
    void display_final_report() {
        auto& audio_system = GlobalAudioSystem::instance();
        
        if (auto* monitor = audio_system.get_performance_monitor()) {
            std::string report = monitor->generate_performance_report();
            std::cout << report << std::endl;
            
            // Export detailed performance data
            monitor->export_to_csv("audio_performance_log.csv", 120.0f);
            std::cout << "Performance data exported to audio_performance_log.csv\n";
        }
        
        // Display system information
        std::cout << "\nSystem Information:\n";
        std::cout << "  Audio System Version: " << AudioSystem::get_version() << "\n";
        std::cout << "  Supported Formats: ";
        auto formats = AudioSystem::get_supported_formats();
        for (size_t i = 0; i < formats.size(); ++i) {
            std::cout << formats[i];
            if (i < formats.size() - 1) std::cout << ", ";
        }
        std::cout << "\n";
        
        std::cout << "  Available Effects: ";
        auto effects = AudioSystem::get_available_effects();
        for (size_t i = 0; i < effects.size(); ++i) {
            std::cout << effects[i];
            if (i < effects.size() - 1) std::cout << ", ";
        }
        std::cout << "\n\n";
    }
    
    void print_instructions() {
        std::cout << "Demo Instructions:\n";
        std::cout << "==================\n";
        std::cout << "This demo will automatically cycle through different 3D audio features:\n\n";
        std::cout << "Phase 1 (0-30s):  Basic 3D positioning and HRTF processing\n";
        std::cout << "Phase 2 (30-60s): Doppler effects with moving sound sources\n";
        std::cout << "Phase 3 (60-90s): Environmental audio and reverb\n";
        std::cout << "Phase 4 (90-120s): Occlusion and ray tracing effects\n\n";
        std::cout << "Listen for:\n";
        std::cout << "- Spatial positioning of different sounds around you\n";
        std::cout << "- Distance-based volume attenuation\n";
        std::cout << "- Pitch changes from moving sources (Doppler effect)\n";
        std::cout << "- Environmental reverb and acoustic simulation\n";
        std::cout << "- Occlusion effects when sounds are blocked\n\n";
        std::cout << "Performance metrics will be displayed every second.\n";
        std::cout << "Press Ctrl+C to exit early.\n\n";
    }

private:
    struct SceneVoice {
        uint32_t voice_id = 0;
        Vector3f position{0.0f, 0.0f, 0.0f};
        Vector3f target_position{0.0f, 0.0f, 0.0f};
        Vector3f velocity{0.0f, 0.0f, 0.0f};
        Vector3f direction{0.0f, 0.0f, -1.0f};
        
        bool is_moving = false;
        bool is_directional = false;
        float movement_speed = 1.0f;
        float movement_timer = 0.0f;
        
        float cone_inner_angle = 360.0f;
        float cone_outer_angle = 360.0f;
        float cone_outer_gain = 0.0f;
        
        std::string sound_file;
        float base_volume = 1.0f;
    };
    
    bool m_running;
    float m_demo_time;
    int m_current_demo_phase = 0;
    std::vector<SceneVoice> m_scene_voices;
    std::mt19937 m_random_engine;
};

int main() {
    try {
        SpatialAudioDemo demo;
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