/**
 * @file vr_spatial_audio_demo.cpp
 * @brief VR/AR spatial audio demonstration with head tracking and ambisonics
 * 
 * This demo showcases advanced VR/AR audio features:
 * - Head tracking integration for immersive audio
 * - Ambisonics encoding/decoding for 360-degree audio
 * - Room-scale VR audio with precise positioning
 * - Hand gesture audio interaction simulation
 * - Binaural processing for headphone delivery
 * - Real-time audio parameter adjustment based on head movement
 */

#include "ecscope/audio/audio_system.hpp"
#include "ecscope/audio/ambisonics.hpp"
#include <iostream>
#include <chrono>
#include <thread>
#include <vector>
#include <memory>
#include <random>
#include <cmath>

using namespace ecscope::audio;

class VRSpatialAudioDemo {
public:
    VRSpatialAudioDemo() : m_running(false), m_demo_time(0.0f), m_head_tracking_enabled(true) {
        m_random_engine.seed(std::chrono::steady_clock::now().time_since_epoch().count());
        
        // Initialize simulated VR tracking data
        m_head_position = {0.0f, 1.75f, 0.0f};  // Average VR headset height
        m_head_orientation = Quaternion{1.0f, 0.0f, 0.0f, 0.0f};
        m_left_hand_position = {-0.3f, 1.5f, -0.5f};
        m_right_hand_position = {0.3f, 1.5f, -0.5f};
    }
    
    ~VRSpatialAudioDemo() {
        shutdown();
    }
    
    bool initialize() {
        std::cout << "ECScope VR/AR Spatial Audio Demo\n";
        std::cout << "=================================\n\n";
        
        // Create VR-optimized audio configuration
        AudioSystemConfig config = AudioSystemFactory::create_vr_config();
        config.format.sample_rate = 48000;
        config.format.buffer_size = 256;    // Very low latency for VR
        config.enable_3d_audio = true;
        config.enable_hrtf = true;
        config.enable_ambisonics = true;
        config.ambisonics_order = 2;        // Second-order for VR
        config.enable_ray_tracing = true;
        config.ray_tracing_quality = 6;     // Balanced quality for real-time VR
        config.enable_debugging = true;
        config.enable_profiling = true;
        config.log_level = AudioDebugLevel::INFO;
        
        if (!GlobalAudioSystem::initialize(config)) {
            std::cerr << "Failed to initialize VR audio system!\n";
            return false;
        }
        
        std::cout << "VR Audio System initialized successfully\n";
        std::cout << "Target Latency: <10ms for VR compatibility\n\n";
        
        // Setup VR-specific audio processing
        setup_vr_audio_processing();
        
        // Create VR demo scene
        create_vr_scene();
        
        // Setup ambisonics processing
        setup_ambisonics_processing();
        
        // Setup head tracking simulation
        setup_head_tracking();
        
        // Initialize hand interaction audio
        setup_hand_interaction_audio();
        
        print_vr_instructions();
        
        return true;
    }
    
    void run() {
        if (!initialize()) {
            return;
        }
        
        m_running = true;
        auto last_time = std::chrono::steady_clock::now();
        
        std::cout << "VR Demo running... Simulating head tracking and hand interactions\n\n";
        
        while (m_running) {
            auto current_time = std::chrono::steady_clock::now();
            float delta_time = std::chrono::duration<float>(current_time - last_time).count();
            last_time = current_time;
            
            // Update VR simulation
            update(delta_time);
            
            // Check for exit condition
            if (m_demo_time > 180.0f) {  // 3 minutes demo
                m_running = false;
            }
            
            // Display VR metrics
            static float metrics_timer = 0.0f;
            metrics_timer += delta_time;
            if (metrics_timer >= 2.0f) {
                display_vr_metrics();
                metrics_timer = 0.0f;
            }
            
            // Target 90 FPS for VR (11.1ms per frame)
            std::this_thread::sleep_for(std::chrono::microseconds(11111));
        }
        
        std::cout << "\nVR Demo completed. Final analysis:\n";
        display_vr_final_report();
    }
    
    void shutdown() {
        if (GlobalAudioSystem::is_initialized()) {
            std::cout << "Shutting down VR audio system...\n";
            GlobalAudioSystem::shutdown();
        }
    }

private:
    void setup_vr_audio_processing() {
        auto& audio_system = GlobalAudioSystem::instance();
        auto& engine_3d = audio_system.get_3d_engine();
        
        std::cout << "Setting up VR audio processing...\n";
        
        // Configure for binaural headphone output
        AudioListener listener;
        listener.position = m_head_position;
        listener.orientation = m_head_orientation;
        listener.velocity = {0.0f, 0.0f, 0.0f};
        listener.gain = 1.0f;
        listener.enabled = true;
        
        // VR-specific HRTF settings
        listener.head_radius = 0.0875f;  // Average head size
        listener.ear_distance = 0.165f;  // Distance between ears
        
        engine_3d.set_listener(listener);
        
        // Load high-quality HRTF database for VR
        if (engine_3d.load_hrtf_database("assets/audio/hrtf/vr_optimized_hrtf.sofa") ||
            engine_3d.load_default_database()) {
            std::cout << "VR-optimized HRTF database loaded\n";
            engine_3d.set_hrtf_interpolation(HRTFInterpolation::CUBIC);  // Higher quality for VR
            engine_3d.enable_hrtf_processing(true);
        }
        
        // VR-specific 3D audio settings
        engine_3d.set_doppler_factor(0.5f);  // Reduced for comfort in VR
        engine_3d.enable_air_absorption(true);
        engine_3d.set_max_audible_distance(50.0f);  // VR rooms are typically smaller
        
        // Enable advanced features for VR
        engine_3d.enable_distance_delay(true);
        engine_3d.set_crossfade_time(5.0f);  // Fast crossfading for head movement
        
        std::cout << "VR audio processing configured\n";
    }
    
    void create_vr_scene() {
        std::cout << "Creating immersive VR audio scene...\n";
        
        // Create a typical VR room environment (4m x 4m x 3m room)
        create_room_ambience();
        create_interactive_objects();
        create_spatial_music_system();
        create_ui_audio_elements();
        
        std::cout << "VR scene created with " << m_vr_audio_objects.size() << " interactive objects\n";
    }
    
    void create_room_ambience() {
        // Ambient room tone
        VRAudioObject room_ambience;
        room_ambience.id = m_next_object_id++;
        room_ambience.type = VRAudioObject::Type::AMBIENT;
        room_ambience.position = {0.0f, 1.5f, 0.0f};  // Room center
        room_ambience.sound_file = "room_ambience.ogg";
        room_ambience.base_volume = 0.3f;
        room_ambience.min_distance = 0.5f;
        room_ambience.max_distance = 10.0f;
        room_ambience.is_looping = true;
        room_ambience.is_active = true;
        
        m_vr_audio_objects.push_back(room_ambience);
        
        // Corner ambient sounds for room presence
        std::vector<Vector3f> corner_positions = {
            {-2.0f, 0.5f, -2.0f}, {2.0f, 0.5f, -2.0f},
            {-2.0f, 0.5f, 2.0f}, {2.0f, 0.5f, 2.0f}
        };
        
        for (size_t i = 0; i < corner_positions.size(); ++i) {
            VRAudioObject corner_sound;
            corner_sound.id = m_next_object_id++;
            corner_sound.type = VRAudioObject::Type::AMBIENT;
            corner_sound.position = corner_positions[i];
            corner_sound.sound_file = "subtle_ambience_" + std::to_string(i) + ".wav";
            corner_sound.base_volume = 0.2f;
            corner_sound.min_distance = 1.0f;
            corner_sound.max_distance = 5.0f;
            corner_sound.is_looping = true;
            corner_sound.is_active = true;
            
            m_vr_audio_objects.push_back(corner_sound);
        }
    }
    
    void create_interactive_objects() {
        // VR objects that can be interacted with via hand tracking
        struct InteractiveObjectDef {
            Vector3f position;
            std::string sound_file;
            float interaction_radius;
            VRAudioObject::Type type;
        };
        
        std::vector<InteractiveObjectDef> objects = {
            {{-1.5f, 1.0f, 1.0f}, "crystal_chime.wav", 0.3f, VRAudioObject::Type::INTERACTIVE},
            {{1.5f, 1.2f, -0.5f}, "wooden_block.wav", 0.25f, VRAudioObject::Type::INTERACTIVE},
            {{0.0f, 0.8f, 1.8f}, "metal_bell.wav", 0.4f, VRAudioObject::Type::INTERACTIVE},
            {{-0.8f, 1.5f, -1.2f}, "glass_wind_chimes.wav", 0.35f, VRAudioObject::Type::INTERACTIVE}
        };
        
        for (const auto& obj_def : objects) {
            VRAudioObject interactive_obj;
            interactive_obj.id = m_next_object_id++;
            interactive_obj.type = obj_def.type;
            interactive_obj.position = obj_def.position;
            interactive_obj.sound_file = obj_def.sound_file;
            interactive_obj.base_volume = 0.8f;
            interactive_obj.min_distance = 0.1f;
            interactive_obj.max_distance = 3.0f;
            interactive_obj.interaction_radius = obj_def.interaction_radius;
            interactive_obj.is_active = true;
            
            m_vr_audio_objects.push_back(interactive_obj);
        }
    }
    
    void create_spatial_music_system() {
        // 360-degree spatial music sources
        VRAudioObject spatial_music;
        spatial_music.id = m_next_object_id++;
        spatial_music.type = VRAudioObject::Type::SPATIAL_MUSIC;
        spatial_music.position = {0.0f, 2.5f, 0.0f};  // Overhead
        spatial_music.sound_file = "ambient_spatial_music.ogg";
        spatial_music.base_volume = 0.4f;
        spatial_music.min_distance = 1.0f;
        spatial_music.max_distance = 8.0f;
        spatial_music.is_looping = true;
        spatial_music.is_active = true;
        spatial_music.use_ambisonics = true;
        
        m_vr_audio_objects.push_back(spatial_music);
    }
    
    void create_ui_audio_elements() {
        // VR UI sounds that follow the user
        VRAudioObject ui_sounds;
        ui_sounds.id = m_next_object_id++;
        ui_sounds.type = VRAudioObject::Type::UI_ELEMENT;
        ui_sounds.position = {0.0f, 1.75f, -0.3f};  // In front of head
        ui_sounds.sound_file = "ui_notification.wav";
        ui_sounds.base_volume = 0.6f;
        ui_sounds.follows_head = true;
        ui_sounds.head_relative_position = {0.0f, 0.0f, -0.3f};
        ui_sounds.is_active = false;  // Triggered by events
        
        m_vr_audio_objects.push_back(ui_sounds);
    }
    
    void setup_ambisonics_processing() {
        auto& audio_system = GlobalAudioSystem::instance();
        
        if (auto* ambisonics_processor = audio_system.get_ambisonics_processor()) {
            std::cout << "Setting up ambisonics processing for VR...\n";
            
            // Configure for second-order ambisonics (9 channels)
            ambisonics_processor->set_ambisonic_order(2);
            ambisonics_processor->set_coordinate_system(AmbisonicsCoordinate::ACN);
            ambisonics_processor->set_normalization(AmbisonicsNormalization::SN3D);
            
            // Enable head tracking for ambisonics rotation
            ambisonics_processor->enable_head_tracking(true);
            
            // Setup binaural decoder for headphones
            auto& decoder = ambisonics_processor->get_decoder();
            decoder.setup_binaural_output();
            decoder.set_decoder_type(AmbisonicsDecoder::DecoderType::BINAURAL);
            
            std::cout << "Ambisonics configured for binaural VR output\n";
        }
    }
    
    void setup_head_tracking() {
        std::cout << "Setting up head tracking simulation...\n";
        
        // Initialize head tracking parameters
        m_head_tracking_smoothing = 0.85f;  // Smooth head movement
        m_head_movement_scale = 1.0f;
        
        // Simulate VR headset motion patterns
        m_head_motion_patterns = {
            {0.5f, 0.2f, 0.1f},   // Natural head bobbing
            {0.3f, 0.4f, 0.15f},  // Side-to-side looking
            {0.2f, 0.1f, 0.3f}    // Up-down movement
        };
        
        std::cout << "Head tracking simulation ready\n";
    }
    
    void setup_hand_interaction_audio() {
        std::cout << "Setting up hand interaction audio...\n";
        
        // Initialize hand tracking audio feedback
        m_hand_interaction_enabled = true;
        m_haptic_audio_enabled = true;
        
        // Hand interaction sound effects
        m_hand_sounds = {
            {"grab", "hand_grab.wav"},
            {"release", "hand_release.wav"},
            {"hover", "hand_hover.wav"},
            {"touch", "hand_touch.wav"},
            {"gesture", "hand_gesture.wav"}
        };
        
        std::cout << "Hand interaction audio configured\n";
    }
    
    void update(float delta_time) {
        m_demo_time += delta_time;
        
        // Update audio system
        auto& audio_system = GlobalAudioSystem::instance();
        audio_system.update(delta_time);
        
        // Simulate VR head tracking
        update_head_tracking(delta_time);
        
        // Simulate hand tracking and interactions
        update_hand_tracking(delta_time);
        
        // Update VR audio objects
        update_vr_audio_objects(delta_time);
        
        // Update ambisonics rotation based on head movement
        update_ambisonics_rotation(delta_time);
        
        // Simulate VR-specific audio events
        simulate_vr_events(delta_time);
        
        // Update audio visualization for VR debugging
        update_vr_visualization();
    }
    
    void update_head_tracking(float delta_time) {
        static float head_tracking_time = 0.0f;
        head_tracking_time += delta_time;
        
        if (!m_head_tracking_enabled) return;
        
        // Simulate natural head movement patterns
        Vector3f target_head_pos = m_head_position;
        Quaternion target_head_orient = m_head_orientation;
        
        // Natural head bobbing and swaying
        float bob_amplitude = 0.02f;
        float sway_amplitude = 0.05f;
        
        target_head_pos.y += bob_amplitude * std::sin(head_tracking_time * 2.0f);
        target_head_pos.x += sway_amplitude * std::sin(head_tracking_time * 0.7f);
        
        // Head rotation simulation (looking around)
        float look_yaw = 15.0f * std::sin(head_tracking_time * 0.3f);
        float look_pitch = 5.0f * std::sin(head_tracking_time * 0.5f);
        target_head_orient = Quaternion::from_euler(look_pitch, look_yaw, 0.0f);
        
        // Apply smoothing to reduce motion sickness
        m_head_position = lerp_vector3(m_head_position, target_head_pos, 
                                      1.0f - std::pow(m_head_tracking_smoothing, delta_time));
        
        // Update listener position in audio system
        auto& engine_3d = AUDIO_3D();
        auto listener = engine_3d.get_listener();
        listener.position = m_head_position;
        listener.orientation = m_head_orientation;
        
        // Calculate head velocity for enhanced 3D audio
        static Vector3f prev_head_pos = m_head_position;
        listener.velocity = (m_head_position - prev_head_pos) / delta_time;
        prev_head_pos = m_head_position;
        
        engine_3d.set_listener(listener);
    }
    
    void update_hand_tracking(float delta_time) {
        static float hand_tracking_time = 0.0f;
        hand_tracking_time += delta_time;
        
        // Simulate hand movement patterns
        float hand_motion_frequency = 0.8f;
        float hand_motion_amplitude = 0.3f;
        
        // Left hand movement
        m_left_hand_position.x = -0.3f + hand_motion_amplitude * std::sin(hand_tracking_time * hand_motion_frequency);
        m_left_hand_position.y = 1.5f + 0.1f * std::cos(hand_tracking_time * hand_motion_frequency * 1.3f);
        m_left_hand_position.z = -0.5f + 0.2f * std::sin(hand_tracking_time * hand_motion_frequency * 0.7f);
        
        // Right hand movement
        m_right_hand_position.x = 0.3f + hand_motion_amplitude * std::cos(hand_tracking_time * hand_motion_frequency * 0.9f);
        m_right_hand_position.y = 1.5f + 0.1f * std::sin(hand_tracking_time * hand_motion_frequency * 1.1f);
        m_right_hand_position.z = -0.5f + 0.2f * std::cos(hand_tracking_time * hand_motion_frequency * 0.6f);
        
        // Check for hand-object interactions
        check_hand_interactions();
    }
    
    void check_hand_interactions() {
        for (auto& obj : m_vr_audio_objects) {
            if (obj.type != VRAudioObject::Type::INTERACTIVE) continue;
            
            // Check left hand interaction
            float left_distance = distance(m_left_hand_position, obj.position);
            bool left_interacting = left_distance < obj.interaction_radius;
            
            // Check right hand interaction
            float right_distance = distance(m_right_hand_position, obj.position);
            bool right_interacting = right_distance < obj.interaction_radius;
            
            bool currently_interacting = left_interacting || right_interacting;
            
            // Trigger interaction sounds
            if (currently_interacting && !obj.is_being_interacted) {
                trigger_interaction_sound(obj, "touch");
                obj.is_being_interacted = true;
                obj.interaction_intensity = 0.8f;
            } else if (!currently_interacting && obj.is_being_interacted) {
                trigger_interaction_sound(obj, "release");
                obj.is_being_interacted = false;
                obj.interaction_intensity = 0.0f;
            }
            
            // Update interaction intensity based on proximity
            if (currently_interacting) {
                float min_distance = std::min(left_distance, right_distance);
                obj.interaction_intensity = 1.0f - (min_distance / obj.interaction_radius);
            }
        }
    }
    
    void trigger_interaction_sound(const VRAudioObject& obj, const std::string& interaction_type) {
        // In real implementation, this would trigger actual audio playback
        std::cout << "Hand interaction: " << interaction_type << " with object " << obj.id 
                  << " (" << obj.sound_file << ")\n";
        
        // Simulate haptic feedback audio
        if (m_haptic_audio_enabled) {
            // Play haptic feedback sound at hand position
            Vector3f hand_pos = distance(m_left_hand_position, obj.position) < 
                               distance(m_right_hand_position, obj.position) ? 
                               m_left_hand_position : m_right_hand_position;
            
            // In real implementation:
            // auto& engine_3d = AUDIO_3D();
            // auto stream = create_audio_stream(m_hand_sounds[interaction_type]);
            // uint32_t voice_id = engine_3d.create_voice(std::move(stream));
            // auto* voice = engine_3d.get_voice(voice_id);
            // voice->set_position(hand_pos);
            // voice->set_gain(0.3f);
            // voice->play();
        }
    }
    
    void update_vr_audio_objects(float delta_time) {
        for (auto& obj : m_vr_audio_objects) {
            if (!obj.is_active) continue;
            
            // Update objects that follow the head
            if (obj.follows_head) {
                // Transform head-relative position to world space
                Vector3f world_offset = transform_vector(obj.head_relative_position, m_head_orientation);
                obj.position = m_head_position + world_offset;
            }
            
            // Update interaction-based parameters
            if (obj.type == VRAudioObject::Type::INTERACTIVE) {
                // Modulate volume based on interaction intensity
                obj.current_volume = obj.base_volume * (1.0f + obj.interaction_intensity * 0.5f);
            }
            
            // Update ambisonics objects
            if (obj.use_ambisonics) {
                // In real implementation, update ambisonics encoding based on position
            }
        }
    }
    
    void update_ambisonics_rotation(float delta_time) {
        auto& audio_system = GlobalAudioSystem::instance();
        
        if (auto* ambisonics_processor = audio_system.get_ambisonics_processor()) {
            // Update head rotation for ambisonics field rotation
            ambisonics_processor->update_head_rotation(m_head_orientation);
            
            // Smooth rotation updates to prevent artifacts
            auto& rotator = ambisonics_processor->get_rotator();
            rotator.enable_smooth_rotation(true);
            rotator.set_smoothing_factor(0.1f);
            rotator.update_rotation_smoothly(m_head_orientation, delta_time);
        }
    }
    
    void simulate_vr_events(float delta_time) {
        static float event_timer = 0.0f;
        event_timer += delta_time;
        
        // Simulate periodic VR events
        if (event_timer >= 15.0f) {  // Every 15 seconds
            event_timer = 0.0f;
            
            // Randomly trigger different VR-specific events
            std::uniform_int_distribution<int> event_dist(0, 3);
            int event_type = event_dist(m_random_engine);
            
            switch (event_type) {
                case 0:
                    simulate_teleportation_event();
                    break;
                case 1:
                    simulate_menu_interaction();
                    break;
                case 2:
                    simulate_environment_change();
                    break;
                case 3:
                    simulate_notification_event();
                    break;
            }
        }
    }
    
    void simulate_teleportation_event() {
        std::cout << "VR Event: Teleportation (repositioning audio scene)\n";
        
        // Simulate teleportation by shifting all audio objects
        Vector3f teleport_offset{
            static_cast<float>(std::uniform_real_distribution<double>(-2.0, 2.0)(m_random_engine)),
            0.0f,
            static_cast<float>(std::uniform_real_distribution<double>(-2.0, 2.0)(m_random_engine))
        };
        
        for (auto& obj : m_vr_audio_objects) {
            if (!obj.follows_head && obj.type != VRAudioObject::Type::UI_ELEMENT) {
                obj.position = obj.position + teleport_offset;
            }
        }
    }
    
    void simulate_menu_interaction() {
        std::cout << "VR Event: Menu interaction (UI audio feedback)\n";
        
        // Find UI audio object and trigger it
        for (auto& obj : m_vr_audio_objects) {
            if (obj.type == VRAudioObject::Type::UI_ELEMENT) {
                obj.is_active = true;
                // In real implementation, play UI sound
                
                // Deactivate after short duration
                obj.activation_timer = 1.0f;
            }
        }
    }
    
    void simulate_environment_change() {
        std::cout << "VR Event: Environment change (updating reverb)\n";
        
        // Change environmental audio settings
        auto& engine_3d = AUDIO_3D();
        auto env_settings = engine_3d.get_environmental_settings();
        
        // Randomly adjust room characteristics
        std::uniform_real_distribution<float> room_size_dist(5.0f, 20.0f);
        std::uniform_real_distribution<float> damping_dist(0.1f, 0.8f);
        
        env_settings.room_size = room_size_dist(m_random_engine);
        env_settings.damping = damping_dist(m_random_engine);
        
        engine_3d.set_environmental_settings(env_settings);
    }
    
    void simulate_notification_event() {
        std::cout << "VR Event: Notification (spatial alert sound)\n";
        
        // Create temporary notification sound at random position
        VRAudioObject notification;
        notification.id = 9999;  // Temporary ID
        notification.type = VRAudioObject::Type::INTERACTIVE;
        
        // Position notification in user's peripheral vision
        float angle = std::uniform_real_distribution<float>(45.0f, 135.0f)(m_random_engine);
        float distance = 1.5f;
        notification.position = {
            m_head_position.x + distance * std::cos(angle * M_PI / 180.0f),
            m_head_position.y,
            m_head_position.z + distance * std::sin(angle * M_PI / 180.0f)
        };
        
        notification.sound_file = "notification_spatial.wav";
        notification.base_volume = 0.7f;
        notification.is_active = true;
        notification.activation_timer = 2.0f;  // Play for 2 seconds
        
        m_vr_audio_objects.push_back(notification);
    }
    
    void update_vr_visualization() {
        auto& audio_system = GlobalAudioSystem::instance();
        
        if (auto* visualizer = audio_system.get_visualizer()) {
            // Update 3D visualization with VR-specific data
            std::vector<Vector3f> source_positions;
            for (const auto& obj : m_vr_audio_objects) {
                if (obj.is_active) {
                    source_positions.push_back(obj.position);
                }
            }
            
            // Add hand positions for visualization
            source_positions.push_back(m_left_hand_position);
            source_positions.push_back(m_right_hand_position);
            
            visualizer->update_3d_positions(source_positions, m_head_position, m_head_orientation);
            
            // Set VR-specific camera for visualization
            visualizer->set_3d_camera_position(m_head_position, m_head_orientation);
        }
    }
    
    void display_vr_metrics() {
        auto& audio_system = GlobalAudioSystem::instance();
        auto metrics = audio_system.get_system_metrics();
        
        std::cout << "VR Audio Metrics (Frame: " << static_cast<int>(m_demo_time * 90) << "):\n";
        std::cout << "  Latency: " << metrics.latency_ms << " ms ";
        
        if (metrics.latency_ms < 10.0f) {
            std::cout << "(Excellent for VR)";
        } else if (metrics.latency_ms < 20.0f) {
            std::cout << "(Good for VR)";
        } else {
            std::cout << "(Too high for VR!)";
        }
        
        std::cout << "\n  CPU Usage: " << metrics.cpu_usage << "%\n";
        std::cout << "  Active VR Objects: " << count_active_objects() << "\n";
        std::cout << "  Head Position: (" << m_head_position.x << ", " 
                  << m_head_position.y << ", " << m_head_position.z << ")\n";
        std::cout << "  Active Interactions: " << count_active_interactions() << "\n\n";
    }
    
    void display_vr_final_report() {
        std::cout << "VR Audio Demo Summary:\n";
        std::cout << "  Total Runtime: " << m_demo_time << " seconds\n";
        std::cout << "  Simulated Frame Rate: 90 FPS (VR target)\n";
        std::cout << "  Head Tracking: " << (m_head_tracking_enabled ? "Enabled" : "Disabled") << "\n";
        std::cout << "  Hand Interactions: " << (m_hand_interaction_enabled ? "Enabled" : "Disabled") << "\n";
        
        auto& audio_system = GlobalAudioSystem::instance();
        auto final_metrics = audio_system.get_system_metrics();
        
        std::cout << "  Final Performance:\n";
        std::cout << "    Average Latency: " << final_metrics.latency_ms << " ms\n";
        std::cout << "    Peak CPU Usage: " << final_metrics.cpu_usage << "%\n";
        std::cout << "    Memory Usage: " << final_metrics.memory_usage / 1024 / 1024 << " MB\n";
        
        if (final_metrics.buffer_underruns > 0) {
            std::cout << "  Warning: " << final_metrics.buffer_underruns 
                     << " buffer underruns detected (may cause audio glitches in VR)\n";
        }
        
        std::cout << "\nVR Audio System Performance: ";
        if (final_metrics.latency_ms < 10.0f && final_metrics.cpu_usage < 50.0f) {
            std::cout << "EXCELLENT - Ready for production VR\n";
        } else if (final_metrics.latency_ms < 20.0f && final_metrics.cpu_usage < 70.0f) {
            std::cout << "GOOD - Suitable for most VR applications\n";
        } else {
            std::cout << "NEEDS OPTIMIZATION - May cause VR discomfort\n";
        }
    }
    
    void print_vr_instructions() {
        std::cout << "VR/AR Spatial Audio Demo Instructions:\n";
        std::cout << "======================================\n\n";
        std::cout << "This demo simulates a VR environment with:\n\n";
        std::cout << "Audio Features:\n";
        std::cout << "- Head tracking with HRTF processing for realistic 3D audio\n";
        std::cout << "- Ambisonics encoding/decoding for 360-degree immersion\n";
        std::cout << "- Hand interaction audio with haptic feedback\n";
        std::cout << "- Ultra-low latency processing (<10ms target for VR)\n";
        std::cout << "- Binaural processing optimized for headphones\n\n";
        std::cout << "Simulated VR Events:\n";
        std::cout << "- Natural head movement and tracking\n";
        std::cout << "- Hand gestures and object interaction\n";
        std::cout << "- Teleportation and scene transitions\n";
        std::cout << "- UI notifications and spatial alerts\n";
        std::cout << "- Dynamic environmental changes\n\n";
        std::cout << "The demo runs for 3 minutes and reports VR-specific metrics.\n";
        std::cout << "Watch for latency warnings - VR requires <20ms for comfort.\n\n";
    }
    
    // Helper functions
    Vector3f lerp_vector3(const Vector3f& a, const Vector3f& b, float t) {
        return {
            a.x + (b.x - a.x) * t,
            a.y + (b.y - a.y) * t,
            a.z + (b.z - a.z) * t
        };
    }
    
    float distance(const Vector3f& a, const Vector3f& b) {
        return std::sqrt((a.x - b.x) * (a.x - b.x) + 
                        (a.y - b.y) * (a.y - b.y) + 
                        (a.z - b.z) * (a.z - b.z));
    }
    
    Vector3f transform_vector(const Vector3f& vec, const Quaternion& quat) {
        // Simplified quaternion-vector multiplication for demo
        return vec;  // In real implementation, perform proper quaternion rotation
    }
    
    int count_active_objects() const {
        int count = 0;
        for (const auto& obj : m_vr_audio_objects) {
            if (obj.is_active) count++;
        }
        return count;
    }
    
    int count_active_interactions() const {
        int count = 0;
        for (const auto& obj : m_vr_audio_objects) {
            if (obj.is_being_interacted) count++;
        }
        return count;
    }

private:
    struct VRAudioObject {
        enum class Type {
            AMBIENT,
            INTERACTIVE,
            SPATIAL_MUSIC,
            UI_ELEMENT
        };
        
        uint32_t id = 0;
        Type type = Type::AMBIENT;
        Vector3f position{0.0f, 0.0f, 0.0f};
        std::string sound_file;
        
        float base_volume = 1.0f;
        float current_volume = 1.0f;
        float min_distance = 1.0f;
        float max_distance = 10.0f;
        float interaction_radius = 0.3f;
        
        bool is_active = false;
        bool is_looping = false;
        bool follows_head = false;
        bool use_ambisonics = false;
        bool is_being_interacted = false;
        
        float interaction_intensity = 0.0f;
        float activation_timer = 0.0f;
        Vector3f head_relative_position{0.0f, 0.0f, 0.0f};
    };
    
    bool m_running;
    float m_demo_time;
    uint32_t m_next_object_id = 1;
    
    // VR tracking simulation
    bool m_head_tracking_enabled;
    Vector3f m_head_position;
    Quaternion m_head_orientation;
    Vector3f m_left_hand_position;
    Vector3f m_right_hand_position;
    
    float m_head_tracking_smoothing;
    float m_head_movement_scale;
    std::vector<Vector3f> m_head_motion_patterns;
    
    // Hand interaction
    bool m_hand_interaction_enabled;
    bool m_haptic_audio_enabled;
    std::unordered_map<std::string, std::string> m_hand_sounds;
    
    // VR scene objects
    std::vector<VRAudioObject> m_vr_audio_objects;
    
    std::mt19937 m_random_engine;
};

int main() {
    try {
        VRSpatialAudioDemo demo;
        demo.run();
        return 0;
    }
    catch (const std::exception& e) {
        std::cerr << "VR Audio Demo failed with exception: " << e.what() << std::endl;
        return 1;
    }
    catch (...) {
        std::cerr << "VR Audio Demo failed with unknown exception" << std::endl;
        return 1;
    }
}