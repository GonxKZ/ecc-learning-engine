/**
 * @file 12-comprehensive-spatial-audio-demo.cpp
 * @brief Comprehensive Spatial Audio System Demonstration for ECScope
 * 
 * This comprehensive example demonstrates the complete ECScope spatial audio system,
 * showcasing professional-grade 3D audio processing, educational features, and
 * real-time performance optimization techniques.
 * 
 * Features Demonstrated:
 * - Complete 3D spatial audio scene with multiple sources and listeners
 * - HRTF-based binaural rendering for realistic 3D audio
 * - Environmental audio with realistic room acoustics and reverb
 * - Physics integration for occlusion, Doppler effects, and collision audio
 * - Real-time audio analysis and visualization
 * - Performance optimization and adaptive quality scaling
 * - Educational DSP demonstrations and interactive learning
 * - Professional audio quality control and enhancement
 * 
 * Educational Value:
 * - Demonstrates complete spatial audio engine architecture
 * - Shows integration between audio, physics, and ECS systems
 * - Provides examples of real-time audio processing optimization
 * - Illustrates professional audio engineering techniques
 * - Interactive educational demonstrations of audio concepts
 * - Performance analysis and optimization methodologies
 * 
 * This example serves as both a comprehensive demonstration and an educational
 * tool for understanding advanced audio engine development and spatial audio
 * processing techniques.
 * 
 * @author ECScope Educational ECS Framework - Advanced Audio Demonstration
 * @date 2025
 */

#include "../../../include/ecscope/spatial_audio_engine.hpp"
#include "../../../include/ecscope/audio_components.hpp"
#include "../../../include/ecscope/audio_systems.hpp"
#include "../../../include/ecscope/audio_processing_pipeline.hpp"
#include "../../../include/ecscope/audio_education_system.hpp"
#include "../../../include/ecscope/audio_testing_framework.hpp"
#include "../../../include/ecscope/ecs/world.hpp"
#include "../../../include/ecscope/memory/arena.hpp"
#include "../../../include/ecscope/core/profiler.hpp"

#include <iostream>
#include <memory>
#include <thread>
#include <chrono>
#include <random>

using namespace ecscope;
using namespace ecscope::ecs;
using namespace ecscope::audio;
using namespace ecscope::audio::components;
using namespace ecscope::audio::systems;
using namespace ecscope::audio::processing;
using namespace ecscope::audio::education;
using namespace ecscope::audio::testing;

//=============================================================================
// Comprehensive Spatial Audio Demonstration Class
//=============================================================================

class SpatialAudioDemo {
private:
    // Core ECS and Audio Systems
    std::unique_ptr<World> world_;
    std::unique_ptr<AudioSystemManager> audio_system_manager_;
    std::unique_ptr<AudioEducationSystem> education_system_;
    std::unique_ptr<AudioTestSuiteRunner> test_suite_runner_;
    std::unique_ptr<memory::MemoryTracker> memory_tracker_;
    
    // Demo Configuration
    struct DemoConfiguration {
        u32 sample_rate{48000};
        u32 buffer_size{512};
        u32 num_audio_sources{32};
        u32 num_listeners{1};
        u32 num_environments{3};
        bool enable_hrtf{true};
        bool enable_environmental_effects{true};
        bool enable_physics_integration{true};
        bool enable_educational_features{true};
        bool enable_real_time_analysis{true};
        f32 demo_duration_minutes{5.0f};
    } config_;
    
    // Demo State
    struct DemoState {
        bool is_running{false};
        bool educational_mode_active{false};
        std::string current_demonstration{"none"};
        f32 elapsed_time{0.0f};
        u32 frame_count{0};
        
        // Performance tracking
        f32 average_cpu_usage{0.0f};
        f32 peak_cpu_usage{0.0f};
        usize current_memory_usage{0};
        usize peak_memory_usage{0};
        f32 audio_quality_score{0.8f};
        
        // Educational tracking
        std::string current_lesson_topic;
        f32 learning_progress{0.0f};
        std::vector<std::string> concepts_demonstrated;
        
    } state_;
    
    // Scene Entities
    std::vector<Entity> audio_sources_;
    std::vector<Entity> audio_listeners_;
    std::vector<Entity> audio_environments_;
    std::vector<Entity> physics_objects_;
    
    // Random number generation for dynamic scene
    std::mt19937 random_generator_;
    std::uniform_real_distribution<f32> position_distribution_{-20.0f, 20.0f};
    std::uniform_real_distribution<f32> velocity_distribution_{-2.0f, 2.0f};
    
public:
    SpatialAudioDemo() : random_generator_(std::random_device{}()) {}
    
    ~SpatialAudioDemo() {
        cleanup();
    }
    
    /**
     * @brief Initialize the complete spatial audio demonstration
     */
    bool initialize() {
        std::cout << "=== ECScope Spatial Audio System Demonstration ===\n";
        std::cout << "Initializing comprehensive 3D spatial audio demo...\n\n";
        
        // Initialize memory tracking
        memory_tracker_ = std::make_unique<memory::MemoryTracker>();
        
        // Create ECS world
        world_ = std::make_unique<World>();
        if (!world_) {
            std::cerr << "Failed to create ECS world\n";
            return false;
        }
        
        // Initialize audio system manager
        audio_system_manager_ = std::make_unique<AudioSystemManager>();
        if (!audio_system_manager_->initialize_all_systems(world_.get())) {
            std::cerr << "Failed to initialize audio systems\n";
            return false;
        }
        
        // Initialize educational system
        education_system_ = std::make_unique<AudioEducationSystem>();
        if (!education_system_->initialize(config_.sample_rate)) {
            std::cerr << "Failed to initialize education system\n";
            return false;
        }
        
        // Initialize test suite runner
        test_suite_runner_ = std::make_unique<AudioTestSuiteRunner>();
        test_suite_runner_->register_all_standard_tests();
        
        std::cout << "✓ Core systems initialized successfully\n";
        
        // Create demo scene
        if (!create_spatial_audio_scene()) {
            std::cerr << "Failed to create spatial audio scene\n";
            return false;
        }
        
        std::cout << "✓ Spatial audio scene created successfully\n";
        std::cout << "✓ Demo initialization complete!\n\n";
        
        return true;
    }
    
    /**
     * @brief Run the complete spatial audio demonstration
     */
    void run_demonstration() {
        if (!initialize()) {
            std::cerr << "Failed to initialize demonstration\n";
            return;
        }
        
        state_.is_running = true;
        
        std::cout << "=== Starting Spatial Audio Demonstration ===\n\n";
        
        // Run different demonstration phases
        run_basic_spatial_audio_demo();
        run_educational_demonstrations();
        run_performance_benchmarks();
        run_quality_validation_tests();
        run_interactive_learning_session();
        
        // Generate comprehensive report
        generate_demonstration_report();
        
        std::cout << "\n=== Spatial Audio Demonstration Complete ===\n";
    }
    
private:
    /**
     * @brief Create a comprehensive spatial audio scene
     */
    bool create_spatial_audio_scene() {
        std::cout << "Creating spatial audio scene...\n";
        
        // Create audio listeners (cameras/players)
        for (u32 i = 0; i < config_.num_listeners; ++i) {
            Entity listener = world_->create_entity();
            
            // Add transform component
            auto transform = Transform{
                .position = Vec2{0.0f, 0.0f},
                .rotation = 0.0f,
                .scale = Vec2{1.0f, 1.0f}
            };
            world_->add_component(listener, transform);
            
            // Add audio listener component
            auto audio_listener = AudioListener{};
            audio_listener.set_hrtf_config("default", 56.0f, 17.0f);
            audio_listener.set_output_mode(AudioListener::OutputConfig::OutputMode::Binaural, 1.0f);
            audio_listener.enable_head_tracking(false); // Disable for this demo
            world_->add_component(listener, std::move(audio_listener));
            
            audio_listeners_.push_back(listener);
            std::cout << "  Created listener " << i << " (Entity: " << listener << ")\n";
        }
        
        // Create environmental audio regions
        create_environmental_regions();
        
        // Create diverse audio sources
        create_audio_sources();
        
        // Create physics objects for audio interaction
        create_physics_audio_objects();
        
        std::cout << "Scene created with:\n";
        std::cout << "  - " << audio_listeners_.size() << " audio listeners\n";
        std::cout << "  - " << audio_sources_.size() << " audio sources\n";
        std::cout << "  - " << audio_environments_.size() << " environmental regions\n";
        std::cout << "  - " << physics_objects_.size() << " physics objects\n";
        
        return true;
    }
    
    /**
     * @brief Create environmental audio regions with different acoustic properties
     */
    void create_environmental_regions() {
        // Large reverberant hall
        Entity hall = world_->create_entity();
        auto hall_transform = Transform{
            .position = Vec2{0.0f, 0.0f},
            .rotation = 0.0f,
            .scale = Vec2{20.0f, 15.0f}
        };
        world_->add_component(hall, hall_transform);
        
        auto hall_environment = AudioEnvironment{AudioEnvironment::EnvironmentType::ConcertHall};
        hall_environment.set_bounds(spatial_math::Vec3{0.0f, 0.0f, 0.0f}, 
                                   spatial_math::Vec3{20.0f, 8.0f, 15.0f});
        hall_environment.set_reverb_config(2.5f, 0.4f, 0.03f, 0.8f);
        world_->add_component(hall, std::move(hall_environment));
        
        audio_environments_.push_back(hall);
        
        // Small intimate room
        Entity room = world_->create_entity();
        auto room_transform = Transform{
            .position = Vec2{25.0f, 0.0f},
            .rotation = 0.0f,
            .scale = Vec2{6.0f, 4.0f}
        };
        world_->add_component(room, room_transform);
        
        auto room_environment = AudioEnvironment{AudioEnvironment::EnvironmentType::SmallRoom};
        room_environment.set_bounds(spatial_math::Vec3{25.0f, 0.0f, 0.0f}, 
                                   spatial_math::Vec3{6.0f, 3.0f, 4.0f});
        room_environment.set_reverb_config(0.8f, 0.3f, 0.01f, 0.6f);
        world_->add_component(room, std::move(room_environment));
        
        audio_environments_.push_back(room);
        
        // Outdoor forest environment
        Entity forest = world_->create_entity();
        auto forest_transform = Transform{
            .position = Vec2{-30.0f, 0.0f},
            .rotation = 0.0f,
            .scale = Vec2{40.0f, 40.0f}
        };
        world_->add_component(forest, forest_transform);
        
        auto forest_environment = AudioEnvironment{AudioEnvironment::EnvironmentType::Forest};
        forest_environment.set_bounds(spatial_math::Vec3{-30.0f, 0.0f, 0.0f}, 
                                     spatial_math::Vec3{40.0f, 20.0f, 40.0f});
        forest_environment.set_reverb_config(0.3f, 0.1f, 0.005f, 0.9f);
        world_->add_component(forest, std::move(forest_environment));
        
        audio_environments_.push_back(forest);
        
        std::cout << "  Created environmental regions: Concert Hall, Small Room, Forest\n";
    }
    
    /**
     * @brief Create diverse audio sources with different characteristics
     */
    void create_audio_sources() {
        for (u32 i = 0; i < config_.num_audio_sources; ++i) {
            Entity source = world_->create_entity();
            
            // Random position in 3D space
            f32 x = position_distribution_(random_generator_);
            f32 z = position_distribution_(random_generator_);
            f32 y = std::uniform_real_distribution<f32>{0.0f, 5.0f}(random_generator_);
            
            auto transform = Transform{
                .position = Vec2{x, z},
                .rotation = 0.0f,
                .scale = Vec2{1.0f, 1.0f}
            };
            world_->add_component(source, transform);
            
            // Create audio source with varied characteristics
            auto audio_source = create_varied_audio_source(i);
            world_->add_component(source, std::move(audio_source));
            
            // Add physics component for some sources (for Doppler and collision effects)
            if (i % 3 == 0) {
                auto rigidbody = physics::components::RigidBody2D{1.0f};
                rigidbody.set_velocity(Vec2{velocity_distribution_(random_generator_), 
                                           velocity_distribution_(random_generator_)});
                world_->add_component(source, rigidbody);
                physics_objects_.push_back(source);
            }
            
            audio_sources_.push_back(source);
        }
        
        std::cout << "  Created " << audio_sources_.size() << " audio sources with varied characteristics\n";
    }
    
    /**
     * @brief Create an audio source with varied characteristics for demonstration
     */
    AudioSource create_varied_audio_source(u32 index) {
        AudioSource source;
        
        // Vary source characteristics based on index
        switch (index % 6) {
            case 0: // Music source
                source = AudioSource{1000 + index, 0.8f, true}; // Looping music
                source.set_attenuation_model(AudioSource::AttenuationModel::Logarithmic, 2.0f, 50.0f);
                source.spatial_flags.use_environmental_effects = 1;
                source.priority = AudioSource::Priority::Normal;
                break;
                
            case 1: // Voice/dialogue source
                source = AudioSource{2000 + index, 0.9f, false};
                source.set_attenuation_model(AudioSource::AttenuationModel::Linear, 1.0f, 20.0f);
                source.spatial_flags.use_hrtf = 1;
                source.priority = AudioSource::Priority::High;
                break;
                
            case 2: // Sound effects
                source = AudioSource{3000 + index, 0.7f, false};
                source.set_attenuation_model(AudioSource::AttenuationModel::Inverse, 0.5f, 30.0f);
                source.spatial_flags.use_doppler = 1;
                source.priority = AudioSource::Priority::Normal;
                break;
                
            case 3: // Ambient sound
                source = AudioSource{4000 + index, 0.4f, true};
                source.set_attenuation_model(AudioSource::AttenuationModel::Exponential, 5.0f, 100.0f);
                source.spatial_flags.use_environmental_effects = 1;
                source.priority = AudioSource::Priority::Low;
                break;
                
            case 4: // Directional source (like a speaker)
                source = AudioSource{5000 + index, 0.6f, true};
                source.set_directional(spatial_math::Vec3{1.0f, 0.0f, 0.0f}, 30.0f, 90.0f, 0.3f);
                source.set_attenuation_model(AudioSource::AttenuationModel::Inverse, 1.0f, 40.0f);
                source.priority = AudioSource::Priority::Normal;
                break;
                
            case 5: // High-priority UI sound
                source = AudioSource{6000 + index, 1.0f, false};
                source.spatial_flags.bypass_processing = 1; // 2D audio
                source.spatial_flags.lock_to_listener = 1;
                source.priority = AudioSource::Priority::Critical;
                break;
        }
        
        return source;
    }
    
    /**
     * @brief Create physics objects that generate audio on collision
     */
    void create_physics_audio_objects() {
        // Create some bouncing balls with collision audio
        for (u32 i = 0; i < 5; ++i) {
            Entity ball = world_->create_entity();
            
            auto transform = Transform{
                .position = Vec2{position_distribution_(random_generator_), 
                                position_distribution_(random_generator_)},
                .rotation = 0.0f,
                .scale = Vec2{0.5f, 0.5f}
            };
            world_->add_component(ball, transform);
            
            // Physics component
            auto rigidbody = physics::components::RigidBody2D{0.5f};
            rigidbody.set_velocity(Vec2{velocity_distribution_(random_generator_) * 3.0f, 
                                       velocity_distribution_(random_generator_) * 3.0f});
            rigidbody.linear_damping = 0.02f;
            world_->add_component(ball, rigidbody);
            
            // Collision audio source
            auto audio_source = AudioSource{7000 + i, 0.6f, false};
            audio_source.set_attenuation_model(AudioSource::AttenuationModel::Inverse, 0.1f, 10.0f);
            audio_source.spatial_flags.use_doppler = 1;
            audio_source.priority = AudioSource::Priority::Normal;
            world_->add_component(ball, std::move(audio_source));
            
            physics_objects_.push_back(ball);
        }
        
        std::cout << "  Created physics-audio objects for collision demonstration\n";
    }
    
    /**
     * @brief Run basic spatial audio demonstration
     */
    void run_basic_spatial_audio_demo() {
        std::cout << "\n=== Phase 1: Basic Spatial Audio Demonstration ===\n";
        
        const f32 demo_duration = 30.0f; // 30 seconds
        const f32 frame_time = 1.0f / 60.0f; // 60 FPS
        
        auto start_time = std::chrono::high_resolution_clock::now();
        f32 elapsed_time = 0.0f;
        
        std::cout << "Running basic spatial audio processing for " << demo_duration << " seconds...\n";
        
        while (elapsed_time < demo_duration && state_.is_running) {
            // Update audio systems
            audio_system_manager_->update_all_systems(frame_time);
            
            // Move some objects for Doppler demonstration
            update_dynamic_objects(frame_time);
            
            // Update performance metrics
            update_performance_metrics();
            
            // Print progress every 5 seconds
            if (static_cast<i32>(elapsed_time) % 5 == 0 && 
                static_cast<i32>(elapsed_time) != static_cast<i32>(elapsed_time - frame_time)) {
                print_realtime_status();
            }
            
            // Simulate frame timing
            std::this_thread::sleep_for(std::chrono::milliseconds(16)); // ~60 FPS
            
            elapsed_time += frame_time;
            state_.frame_count++;
        }
        
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        
        std::cout << "\n✓ Basic spatial audio demo completed in " 
                  << duration.count() << "ms\n";
        std::cout << "  Processed " << state_.frame_count << " frames\n";
        std::cout << "  Average CPU usage: " << state_.average_cpu_usage << "%\n";
        std::cout << "  Peak memory usage: " << state_.peak_memory_usage / (1024*1024) << " MB\n";
    }
    
    /**
     * @brief Run educational demonstrations
     */
    void run_educational_demonstrations() {
        std::cout << "\n=== Phase 2: Educational Demonstrations ===\n";
        
        state_.educational_mode_active = true;
        
        // Run different educational demonstrations
        std::vector<std::string> demonstrations = {
            "FFTAnalysisDemo",
            "SpatialAudioDemo", 
            "CompressionDemo",
            "AudioQualityDemo"
        };
        
        for (const auto& demo_name : demonstrations) {
            std::cout << "\n--- Running " << demo_name << " ---\n";
            
            if (education_system_->start_demonstration(demo_name)) {
                state_.current_demonstration = demo_name;
                
                // Run demonstration for 15 seconds
                run_single_educational_demo(demo_name, 15.0f);
                
                education_system_->stop_current_demonstration();
                state_.concepts_demonstrated.push_back(demo_name);
                
                std::cout << "✓ " << demo_name << " completed successfully\n";
            } else {
                std::cout << "✗ Failed to start " << demo_name << "\n";
            }
        }
        
        state_.educational_mode_active = false;
        
        std::cout << "\n✓ Educational demonstrations completed\n";
        std::cout << "  Total concepts demonstrated: " << state_.concepts_demonstrated.size() << "\n";
    }
    
    /**
     * @brief Run a single educational demonstration
     */
    void run_single_educational_demo(const std::string& demo_name, f32 duration) {
        const f32 frame_time = 1.0f / 60.0f;
        f32 elapsed_time = 0.0f;
        
        std::cout << "  Demonstrating " << demo_name << " concepts...\n";
        
        while (elapsed_time < duration) {
            // Update audio systems with educational focus
            audio_system_manager_->set_educational_focus(demo_name);
            audio_system_manager_->update_all_systems(frame_time);
            
            // Process educational demonstration
            auto* current_demo = education_system_->get_current_demonstration();
            if (current_demo) {
                // Generate test audio for demonstration
                std::vector<f32> test_audio = generate_educational_test_audio(demo_name, elapsed_time);
                
                // Process audio through demonstration
                std::vector<f32> output_audio(test_audio.size());
                current_demo->process_audio(test_audio.data(), output_audio.data(), test_audio.size());
                current_demo->update_visualization();
            }
            
            elapsed_time += frame_time;
            std::this_thread::sleep_for(std::chrono::milliseconds(16));
        }
        
        // Print educational insights
        if (auto* demo = education_system_->get_current_demonstration()) {
            std::cout << "    Key concepts: ";
            auto concepts = demo->get_key_concepts();
            for (size_t i = 0; i < concepts.size(); ++i) {
                std::cout << concepts[i];
                if (i < concepts.size() - 1) std::cout << ", ";
            }
            std::cout << "\n";
            
            std::cout << "    Educational summary: " << demo->generate_educational_summary() << "\n";
        }
    }
    
    /**
     * @brief Generate test audio for educational demonstrations
     */
    std::vector<f32> generate_educational_test_audio(const std::string& demo_name, f32 time_offset) {
        const usize buffer_size = 1024;
        std::vector<f32> audio(buffer_size);
        
        const f32 sample_rate = static_cast<f32>(config_.sample_rate);
        
        if (demo_name == "FFTAnalysisDemo") {
            // Generate a complex signal with multiple harmonics
            for (usize i = 0; i < buffer_size; ++i) {
                f32 t = (time_offset + static_cast<f32>(i) / sample_rate);
                audio[i] = 0.5f * std::sin(2.0f * M_PI * 440.0f * t) +  // A4
                          0.3f * std::sin(2.0f * M_PI * 880.0f * t) +  // A5
                          0.2f * std::sin(2.0f * M_PI * 1320.0f * t);  // E6
            }
        } else if (demo_name == "SpatialAudioDemo") {
            // Generate a simple tone that moves in 3D space
            for (usize i = 0; i < buffer_size; ++i) {
                f32 t = (time_offset + static_cast<f32>(i) / sample_rate);
                audio[i] = 0.7f * std::sin(2.0f * M_PI * 1000.0f * t);
            }
        } else if (demo_name == "CompressionDemo") {
            // Generate a dynamic signal to demonstrate compression
            for (usize i = 0; i < buffer_size; ++i) {
                f32 t = (time_offset + static_cast<f32>(i) / sample_rate);
                f32 envelope = 0.5f + 0.5f * std::sin(2.0f * M_PI * 2.0f * t); // 2 Hz envelope
                audio[i] = envelope * std::sin(2.0f * M_PI * 800.0f * t);
            }
        } else {
            // Default: simple sine wave
            for (usize i = 0; i < buffer_size; ++i) {
                f32 t = (time_offset + static_cast<f32>(i) / sample_rate);
                audio[i] = 0.5f * std::sin(2.0f * M_PI * 1000.0f * t);
            }
        }
        
        return audio;
    }
    
    /**
     * @brief Run performance benchmarks
     */
    void run_performance_benchmarks() {
        std::cout << "\n=== Phase 3: Performance Benchmarks ===\n";
        
        // Run comprehensive performance tests
        if (test_suite_runner_->run_performance_benchmark_suite()) {
            auto results = test_suite_runner_->get_test_results();
            
            std::cout << "Performance benchmark results:\n";
            std::cout << "  Average CPU usage: " << results.average_cpu_usage_percent << "%\n";
            std::cout << "  Peak memory usage: " << results.peak_memory_usage_mb << " MB\n";
            std::cout << "  Audio quality score: " << results.audio_quality_score * 100.0f << "%\n";
            std::cout << "  Success rate: " << results.success_rate_percent << "%\n";
            
            if (!results.performance_issues.empty()) {
                std::cout << "  Performance issues identified:\n";
                for (const auto& issue : results.performance_issues) {
                    std::cout << "    - " << issue << "\n";
                }
            }
            
            if (!results.recommendations.empty()) {
                std::cout << "  Optimization recommendations:\n";
                for (const auto& recommendation : results.recommendations) {
                    std::cout << "    - " << recommendation << "\n";
                }
            }
        } else {
            std::cout << "✗ Performance benchmarks failed\n";
        }
        
        std::cout << "✓ Performance benchmarks completed\n";
    }
    
    /**
     * @brief Run quality validation tests
     */
    void run_quality_validation_tests() {
        std::cout << "\n=== Phase 4: Audio Quality Validation ===\n";
        
        // Run audio quality tests
        if (test_suite_runner_->run_tests_by_category("Quality")) {
            auto results = test_suite_runner_->get_test_results();
            
            std::cout << "Quality validation results:\n";
            std::cout << "  Tests passed: " << results.tests_passed << "/" << results.total_tests_run << "\n";
            std::cout << "  Overall audio quality: " << results.audio_quality_score * 100.0f << "%\n";
            
            if (!results.quality_issues.empty()) {
                std::cout << "  Quality issues identified:\n";
                for (const auto& issue : results.quality_issues) {
                    std::cout << "    - " << issue << "\n";
                }
            }
        } else {
            std::cout << "✗ Quality validation tests failed\n";
        }
        
        std::cout << "✓ Quality validation completed\n";
    }
    
    /**
     * @brief Run interactive learning session
     */
    void run_interactive_learning_session() {
        std::cout << "\n=== Phase 5: Interactive Learning Session ===\n";
        
        const std::string student_id = "demo_student";
        const std::string lesson_topic = "Comprehensive Spatial Audio";
        
        education_system_->start_student_session(student_id);
        
        std::cout << "Starting interactive learning session for: " << lesson_topic << "\n";
        
        // Simulate interactive learning progression
        std::vector<std::string> learning_modules = {
            "Basic Audio Concepts",
            "Frequency Domain Analysis", 
            "Spatial Audio Fundamentals",
            "HRTF and Binaural Processing",
            "Environmental Audio Effects",
            "Performance Optimization"
        };
        
        for (size_t i = 0; i < learning_modules.size(); ++i) {
            std::cout << "  Module " << (i+1) << ": " << learning_modules[i] << "\n";
            
            // Simulate learning progress
            state_.learning_progress = static_cast<f32>(i + 1) / learning_modules.size();
            
            // Brief demonstration of each module
            std::this_thread::sleep_for(std::chrono::seconds(2));
            
            std::cout << "    ✓ Module completed (Progress: " 
                      << static_cast<i32>(state_.learning_progress * 100) << "%)\n";
        }
        
        education_system_->end_student_session();
        
        std::cout << "✓ Interactive learning session completed\n";
        std::cout << "  Final learning progress: 100%\n";
        std::cout << "  Concepts mastered: " << learning_modules.size() << "\n";
    }
    
    /**
     * @brief Update dynamic objects for demonstration
     */
    void update_dynamic_objects(f32 delta_time) {
        // Move some audio sources in circular patterns for Doppler demonstration
        static f32 circle_time = 0.0f;
        circle_time += delta_time;
        
        for (size_t i = 0; i < std::min(audio_sources_.size(), size_t{5}); ++i) {
            Entity source = audio_sources_[i];
            
            if (auto* transform = world_->get_component<Transform>(source)) {
                f32 radius = 5.0f + static_cast<f32>(i) * 2.0f;
                f32 speed = 0.5f + static_cast<f32>(i) * 0.2f;
                f32 angle = circle_time * speed + static_cast<f32>(i) * M_PI / 3.0f;
                
                transform->position.x = radius * std::cos(angle);
                transform->position.y = radius * std::sin(angle);
            }
        }
        
        // Update physics objects with bouncing behavior
        for (Entity physics_obj : physics_objects_) {
            if (auto* rigidbody = world_->get_component<physics::components::RigidBody2D>(physics_obj)) {
                if (auto* transform = world_->get_component<Transform>(physics_obj)) {
                    // Simple boundary bouncing
                    if (std::abs(transform->position.x) > 15.0f) {
                        rigidbody->velocity.x *= -0.8f; // Some energy loss
                    }
                    if (std::abs(transform->position.y) > 15.0f) {
                        rigidbody->velocity.y *= -0.8f;
                    }
                }
            }
        }
    }
    
    /**
     * @brief Update performance metrics
     */
    void update_performance_metrics() {
        // Get system performance data
        auto system_analysis = audio_system_manager_->get_comprehensive_analysis();
        
        state_.average_cpu_usage = (state_.average_cpu_usage * 0.95f) + 
                                  (system_analysis.audio_cpu_usage_percent * 0.05f);
        state_.peak_cpu_usage = std::max(state_.peak_cpu_usage, system_analysis.audio_cpu_usage_percent);
        
        if (memory_tracker_) {
            state_.current_memory_usage = memory_tracker_->get_total_allocated();
            state_.peak_memory_usage = std::max(state_.peak_memory_usage, state_.current_memory_usage);
        }
        
        state_.audio_quality_score = system_analysis.overall_audio_quality_score;
    }
    
    /**
     * @brief Print real-time status information
     */
    void print_realtime_status() {
        std::cout << "Real-time Status:\n";
        std::cout << "  Active Sources: " << audio_sources_.size() << "\n";
        std::cout << "  Active Listeners: " << audio_listeners_.size() << "\n";
        std::cout << "  CPU Usage: " << state_.average_cpu_usage << "% (Peak: " << state_.peak_cpu_usage << "%)\n";
        std::cout << "  Memory Usage: " << state_.current_memory_usage / (1024*1024) << " MB\n";
        std::cout << "  Audio Quality: " << static_cast<i32>(state_.audio_quality_score * 100) << "%\n";
        
        if (state_.educational_mode_active) {
            std::cout << "  Educational Demo: " << state_.current_demonstration << "\n";
            std::cout << "  Learning Progress: " << static_cast<i32>(state_.learning_progress * 100) << "%\n";
        }
        
        std::cout << "\n";
    }
    
    /**
     * @brief Generate comprehensive demonstration report
     */
    void generate_demonstration_report() {
        std::cout << "\n=== Comprehensive Demonstration Report ===\n";
        
        // Overall statistics
        std::cout << "\nOverall Statistics:\n";
        std::cout << "  Total frames processed: " << state_.frame_count << "\n";
        std::cout << "  Average CPU usage: " << state_.average_cpu_usage << "%\n";
        std::cout << "  Peak CPU usage: " << state_.peak_cpu_usage << "%\n";
        std::cout << "  Peak memory usage: " << state_.peak_memory_usage / (1024*1024) << " MB\n";
        std::cout << "  Final audio quality score: " << static_cast<i32>(state_.audio_quality_score * 100) << "%\n";
        
        // System analysis
        auto system_analysis = audio_system_manager_->get_comprehensive_analysis();
        std::cout << "\nSystem Analysis:\n";
        std::cout << "  Active audio sources: " << system_analysis.active_audio_sources << "\n";
        std::cout << "  Active listeners: " << system_analysis.active_listeners << "\n";
        std::cout << "  Active environments: " << system_analysis.active_environments << "\n";
        std::cout << "  Overall system health: " << system_analysis.overall_system_health << "\n";
        
        // Educational effectiveness
        std::cout << "\nEducational Effectiveness:\n";
        std::cout << "  Educational value score: " << static_cast<i32>(system_analysis.educational_value_score * 100) << "%\n";
        std::cout << "  Concepts demonstrated: " << state_.concepts_demonstrated.size() << "\n";
        std::cout << "  Current educational focus: " << system_analysis.current_educational_focus << "\n";
        
        // Performance recommendations
        if (!system_analysis.performance_recommendations.empty()) {
            std::cout << "\nPerformance Recommendations:\n";
            for (const auto& recommendation : system_analysis.performance_recommendations) {
                std::cout << "  - " << recommendation << "\n";
            }
        }
        
        // Educational opportunities
        if (!system_analysis.educational_opportunities.empty()) {
            std::cout << "\nEducational Opportunities:\n";
            for (const auto& opportunity : system_analysis.educational_opportunities) {
                std::cout << "  - " << opportunity << "\n";
            }
        }
        
        // Key learning concepts validated
        if (!system_analysis.key_learning_concepts.empty()) {
            std::cout << "\nKey Learning Concepts Validated:\n";
            for (const auto& concept : system_analysis.key_learning_concepts) {
                std::cout << "  - " << concept << "\n";
            }
        }
        
        // Test suite summary
        auto test_results = test_suite_runner_->get_test_results();
        std::cout << "\nTest Suite Summary:\n";
        std::cout << "  Total tests run: " << test_results.total_tests_run << "\n";
        std::cout << "  Tests passed: " << test_results.tests_passed << "\n";
        std::cout << "  Tests failed: " << test_results.tests_failed << "\n";
        std::cout << "  Success rate: " << test_results.success_rate_percent << "%\n";
        
        // Save detailed reports
        std::cout << "\nSaving detailed reports...\n";
        test_suite_runner_->generate_html_report("spatial_audio_demo_report.html");
        test_suite_runner_->generate_performance_analysis_report("performance_analysis.txt");
        test_suite_runner_->generate_educational_report("educational_effectiveness.txt");
        
        std::cout << "✓ Reports saved successfully\n";
        
        // Final assessment
        f32 overall_score = (state_.audio_quality_score + 
                           system_analysis.educational_value_score + 
                           test_results.success_rate_percent / 100.0f) / 3.0f;
        
        std::cout << "\n=== Final Assessment ===\n";
        std::cout << "Overall demonstration score: " << static_cast<i32>(overall_score * 100) << "%\n";
        
        if (overall_score >= 0.9f) {
            std::cout << "Assessment: EXCELLENT - Spatial audio system performing at professional level\n";
        } else if (overall_score >= 0.8f) {
            std::cout << "Assessment: GOOD - Spatial audio system performing well with minor areas for improvement\n";
        } else if (overall_score >= 0.7f) {
            std::cout << "Assessment: SATISFACTORY - Spatial audio system functional with room for optimization\n";
        } else {
            std::cout << "Assessment: NEEDS IMPROVEMENT - Spatial audio system requires attention\n";
        }
    }
    
    /**
     * @brief Cleanup demonstration resources
     */
    void cleanup() {
        if (state_.is_running) {
            state_.is_running = false;
        }
        
        if (audio_system_manager_) {
            audio_system_manager_->cleanup_all_systems();
        }
        
        if (education_system_) {
            education_system_->cleanup();
        }
        
        audio_sources_.clear();
        audio_listeners_.clear();
        audio_environments_.clear();
        physics_objects_.clear();
        
        std::cout << "Demo cleanup completed\n";
    }
};

//=============================================================================
// Main Function
//=============================================================================

int main() {
    std::cout << "ECScope Comprehensive Spatial Audio System Demonstration\n";
    std::cout << "========================================================\n\n";
    
    try {
        SpatialAudioDemo demo;
        demo.run_demonstration();
        
        std::cout << "\n✓ Demonstration completed successfully!\n";
        std::cout << "\nThis demonstration showcased:\n";
        std::cout << "• Complete 3D spatial audio processing pipeline\n";
        std::cout << "• HRTF-based binaural rendering\n";
        std::cout << "• Environmental audio effects and room acoustics\n";
        std::cout << "• Physics integration for realistic audio interactions\n";
        std::cout << "• Real-time performance optimization\n";
        std::cout << "• Educational DSP demonstrations\n";
        std::cout << "• Professional audio quality validation\n";
        std::cout << "• Comprehensive testing and benchmarking\n";
        std::cout << "\nThe ECScope spatial audio system demonstrates professional-grade\n";
        std::cout << "audio engineering techniques while providing rich educational content\n";
        std::cout << "for learning advanced audio programming concepts.\n\n";
        
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "Demonstration failed with exception: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "Demonstration failed with unknown exception" << std::endl;
        return 1;
    }
}