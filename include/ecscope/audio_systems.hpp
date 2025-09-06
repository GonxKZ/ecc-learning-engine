#pragma once

/**
 * @file audio_systems.hpp
 * @brief ECS Systems for Spatial Audio Processing in ECScope Framework
 * 
 * This header provides comprehensive ECS systems for real-time spatial audio
 * processing, integrating seamlessly with the existing ECScope ECS framework.
 * Systems are designed for optimal performance while providing rich educational content.
 * 
 * Key Systems:
 * - SpatialAudioSystem: Main 3D audio processing system
 * - AudioListenerSystem: Manages audio listeners and HRTF processing  
 * - AudioEnvironmentSystem: Handles environmental audio effects
 * - AudioAnalysisSystem: Real-time audio analysis and visualization
 * - AudioMemorySystem: Memory management for audio resources
 * - AudioPhysicsIntegration: Integration with physics for occlusion/Doppler
 * 
 * Educational Value:
 * - Demonstrates large-scale ECS system architecture
 * - Shows real-time audio system design patterns
 * - Illustrates performance optimization in game engines
 * - Provides insight into professional audio engine architecture
 * - Teaches about system interdependencies and integration
 * - Interactive performance analysis and profiling
 * 
 * Performance Features:
 * - Multi-threaded audio processing with job system integration
 * - Cache-friendly iteration patterns over audio components
 * - SIMD-optimized batch processing of audio sources
 * - Memory pool integration for zero-allocation processing  
 * - Adaptive quality scaling based on performance metrics
 * - Educational profiling without performance impact
 * 
 * @author ECScope Educational ECS Framework - Audio Systems Integration
 * @date 2025
 */

#include "audio_components.hpp"
#include "audio_processing_pipeline.hpp"
#include "audio_education_system.hpp"
#include "ecs/system.hpp"
#include "ecs/world.hpp"
#include "ecs/sparse_set.hpp"
#include "memory/memory_tracker.hpp"
#include "physics/components.hpp"
#include "core/profiler.hpp"
#include <vector>
#include <memory>
#include <atomic>
#include <thread>

namespace ecscope::audio::systems {

// Import commonly used types
using namespace ecscope::ecs;
using namespace ecscope::audio;
using namespace ecscope::audio::components;
using namespace ecscope::audio::processing;
using namespace ecscope::audio::education;

//=============================================================================
// System Base Classes and Utilities
//=============================================================================

/**
 * @brief Base class for audio systems with common functionality
 * 
 * Educational Context:
 * Demonstrates common patterns in ECS system design including
 * component iteration, memory management, and performance tracking.
 */
class AudioSystemBase : public System {
protected:
    // Performance tracking
    mutable std::atomic<u64> update_count_{0};
    mutable std::atomic<f64> total_update_time_ms_{0.0};
    mutable std::atomic<f32> average_entities_processed_{0.0f};
    
    // Educational analytics
    mutable std::string system_description_;
    mutable std::vector<std::string> key_concepts_;
    mutable std::atomic<f32> educational_value_score_{0.7f};
    
    // Memory tracking
    memory::MemoryTracker* memory_tracker_{nullptr};
    memory::AllocationCategory memory_category_{memory::AllocationCategory::Audio_Processing};
    
public:
    explicit AudioSystemBase(memory::MemoryTracker* memory_tracker = nullptr,
                           memory::AllocationCategory category = memory::AllocationCategory::Audio_Processing)
        : memory_tracker_(memory_tracker), memory_category_(category) {}
    
    virtual ~AudioSystemBase() = default;
    
    // Performance monitoring interface
    struct SystemPerformance {
        f64 average_update_time_ms{0.0};
        f32 updates_per_second{0.0f};
        f32 average_entities_processed{0.0f};
        usize memory_usage_bytes{0};
        f32 cpu_usage_percent{0.0f};
        
        // Educational metrics
        f32 educational_value_score{0.7f};
        std::string system_complexity_level;
        std::vector<std::string> optimization_techniques_used;
        std::string performance_analysis;
    };
    
    virtual SystemPerformance get_performance_metrics() const;
    virtual void reset_performance_counters();
    
    // Educational interface
    virtual std::string get_system_description() const = 0;
    virtual std::vector<std::string> get_key_concepts() const = 0;
    virtual std::string generate_educational_summary() const = 0;
    virtual f32 get_educational_value_score() const { return educational_value_score_.load(); }
    
protected:
    // Helper for tracking update performance
    class ScopedUpdateTimer {
    private:
        AudioSystemBase* system_;
        std::chrono::high_resolution_clock::time_point start_time_;
        
    public:
        explicit ScopedUpdateTimer(AudioSystemBase* system) 
            : system_(system), start_time_(std::chrono::high_resolution_clock::now()) {}
        
        ~ScopedUpdateTimer() {
            auto end_time = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time_);
            f64 duration_ms = duration.count() / 1000.0;
            
            system_->update_count_.fetch_add(1, std::memory_order_relaxed);
            system_->total_update_time_ms_.fetch_add(duration_ms, std::memory_order_relaxed);
        }
    };
    
    // Memory allocation helpers
    template<typename T>
    T* allocate_audio_memory(usize count = 1) {
        if (memory_tracker_) {
            return static_cast<T*>(memory_tracker_->allocate(sizeof(T) * count, alignof(T), memory_category_));
        }
        return new T[count];
    }
    
    template<typename T>
    void deallocate_audio_memory(T* ptr, usize count = 1) {
        if (memory_tracker_) {
            memory_tracker_->deallocate(ptr, sizeof(T) * count, memory_category_);
        } else {
            delete[] ptr;
        }
    }
};

//=============================================================================
// Spatial Audio Processing System
//=============================================================================

/**
 * @brief Main spatial audio processing system
 * 
 * Educational Context:
 * This system demonstrates how to efficiently process many audio sources
 * in real-time while maintaining spatial audio quality. Shows techniques
 * for cache-friendly iteration, SIMD optimization, and performance scaling.
 * 
 * Key Responsibilities:
 * - Process all AudioSource components for spatial positioning
 * - Apply distance attenuation, Doppler effects, and directional audio
 * - Integrate with HRTF processing for realistic 3D audio
 * - Manage audio source culling and performance optimization
 * - Provide real-time analysis and educational insights
 */
class SpatialAudioSystem : public AudioSystemBase {
private:
    // Core processing components
    std::unique_ptr<AudioProcessingPipeline> processing_pipeline_;
    std::unique_ptr<simd_ops::SIMDDispatcher> simd_dispatcher_;
    
    // Spatial audio processors
    std::unique_ptr<HRTFProcessor> hrtf_processor_;
    std::unique_ptr<AudioEnvironmentProcessor> environment_processor_;
    
    // Performance optimization
    struct ProcessingState {
        std::vector<Entity> active_sources_;        // Entities with active audio
        std::vector<Entity> audible_sources_;       // Entities within hearing range
        std::vector<f32> source_distances_;         // Cached distances to listeners
        std::vector<f32> source_volumes_;           // Cached effective volumes
        
        // Culling and LOD
        f32 culling_distance_{200.0f};             // Maximum audible distance
        f32 volume_threshold_{0.001f};             // Minimum audible volume
        u32 max_simultaneous_sources_{64};         // Maximum sources to process
        
        // Performance tracking
        u32 sources_processed_{0};
        u32 sources_culled_{0};
        f32 average_processing_time_per_source_ms_{0.0f};
        
    } processing_state_;
    
    // Educational features
    struct EducationalData {
        u32 hrtf_operations_per_frame_{0};
        u32 distance_calculations_per_frame_{0};
        u32 doppler_calculations_per_frame_{0};
        f32 spatial_complexity_score_{0.5f};       // 0-1 based on scene complexity
        std::string current_processing_summary_;
        std::vector<std::string> optimization_techniques_used_;
        
        // Real-time analysis
        f32 average_source_distance_{10.0f};
        f32 spatial_scene_density_{0.3f};          // Sources per cubic meter
        u32 sources_using_advanced_features_{0};   // HRTF, environmental, etc.
        
    } educational_data_;
    
    // Configuration
    struct SystemConfig {
        bool enable_hrtf_processing_{true};
        bool enable_environmental_effects_{true};
        bool enable_doppler_effects_{true};
        bool enable_distance_attenuation_{true};
        bool enable_adaptive_quality_{true};
        f32 quality_scale_factor_{1.0f};          // 0.1 = lowest quality, 1.0 = full quality
        
        // Performance tuning
        bool enable_source_culling_{true};
        bool enable_lod_processing_{true};         // Level-of-detail processing
        u32 update_frequency_hz_{60};              // System update frequency
        
    } config_;
    
public:
    explicit SpatialAudioSystem(memory::MemoryTracker* memory_tracker = nullptr);
    ~SpatialAudioSystem() override;
    
    // System interface
    bool initialize(World* world) override;
    void update(World* world, f32 delta_time) override;
    void cleanup() override;
    
    // Configuration interface
    void set_hrtf_processing_enabled(bool enabled) { config_.enable_hrtf_processing_ = enabled; }
    void set_environmental_effects_enabled(bool enabled) { config_.enable_environmental_effects_ = enabled; }
    void set_doppler_effects_enabled(bool enabled) { config_.enable_doppler_effects_ = enabled; }
    void set_quality_scale_factor(f32 factor) { config_.quality_scale_factor_ = std::clamp(factor, 0.1f, 1.0f); }
    
    // Performance optimization interface
    void set_culling_distance(f32 distance) { processing_state_.culling_distance_ = distance; }
    void set_volume_threshold(f32 threshold) { processing_state_.volume_threshold_ = threshold; }
    void set_max_simultaneous_sources(u32 max_sources) { processing_state_.max_simultaneous_sources_ = max_sources; }
    
    // Educational interface implementation
    std::string get_system_description() const override;
    std::vector<std::string> get_key_concepts() const override;
    std::string generate_educational_summary() const override;
    
    // Spatial audio specific analysis
    struct SpatialAnalysis {
        u32 total_audio_sources{0};
        u32 audible_sources{0};
        u32 sources_using_hrtf{0};
        u32 sources_using_environmental{0};
        f32 average_processing_cost_ms{0.0f};
        f32 spatial_scene_complexity{0.5f};        // 0-1 complexity score
        
        std::string performance_bottleneck;        // Main performance limitation
        std::string quality_assessment;            // Overall audio quality
        std::vector<std::string> optimization_suggestions;
        
        // Educational insights
        std::string spatial_audio_explanation;     // Current spatial processing explanation
        std::string performance_analysis;          // Performance analysis for education
    };
    
    SpatialAnalysis get_spatial_analysis() const;
    
    // Component access for other systems
    HRTFProcessor* get_hrtf_processor() { return hrtf_processor_.get(); }
    AudioEnvironmentProcessor* get_environment_processor() { return environment_processor_.get(); }
    AudioProcessingPipeline* get_processing_pipeline() { return processing_pipeline_.get(); }
    
private:
    // Core processing functions
    void process_audio_sources(World* world, f32 delta_time);
    void update_spatial_parameters(World* world);
    void perform_source_culling(World* world);
    void apply_level_of_detail_processing(World* world);
    
    // Spatial audio calculations
    void calculate_source_distances(World* world);
    void apply_distance_attenuation(AudioSource& source, f32 distance);
    void apply_doppler_effects(AudioSource& source, const spatial_math::Vec3& relative_velocity);
    void apply_directional_processing(AudioSource& source, const spatial_math::Vec3& listener_direction);
    
    // HRTF and environmental processing
    void process_hrtf_for_source(AudioSource& source, const spatial_math::Transform3D::RelativePosition& relative_pos);
    void apply_environmental_effects(AudioSource& source, const AudioEnvironment* environment);
    
    // Performance optimization
    void update_adaptive_quality(f32 cpu_load_percent);
    void prioritize_audio_sources(std::vector<Entity>& sources, World* world);
    
    // Educational data generation
    void update_educational_metrics(World* world);
    void analyze_spatial_scene_complexity(World* world);
    void generate_performance_insights();
};

//=============================================================================
// Audio Listener Management System
//=============================================================================

/**
 * @brief System managing audio listeners and binaural processing
 * 
 * Educational Context:
 * Demonstrates how to manage multiple audio listeners (for local multiplayer),
 * HRTF database management, and binaural audio rendering techniques.
 * Shows advanced optimization techniques for spatial audio rendering.
 */
class AudioListenerSystem : public AudioSystemBase {
private:
    // Listener processing state
    struct ListenerState {
        std::vector<Entity> active_listeners_;
        Entity primary_listener_{0};               // Primary listener for single-player
        u32 max_listeners_{4};                     // Support up to 4 listeners
        
        // HRTF processing per listener
        std::unordered_map<Entity, std::unique_ptr<HRTFProcessor>> hrtf_processors_;
        std::string current_hrtf_database_{"default"};
        
        // Performance tracking per listener
        std::unordered_map<Entity, f32> processing_costs_;
        std::unordered_map<Entity, u32> sources_processed_;
        
    } listener_state_;
    
    // Educational analytics
    struct ListenerAnalytics {
        f32 average_hrtf_processing_time_ms_{0.0f};
        f32 binaural_rendering_quality_score_{0.8f}; // 0-1 quality assessment
        u32 total_hrtf_convolutions_per_frame_{0};
        std::string spatial_audio_quality_assessment_;
        
        // Multi-listener complexity
        f32 multi_listener_overhead_percent_{0.0f};
        std::string listener_configuration_description_;
        
    } analytics_;
    
public:
    explicit AudioListenerSystem(memory::MemoryTracker* memory_tracker = nullptr);
    ~AudioListenerSystem() override;
    
    // System interface
    bool initialize(World* world) override;
    void update(World* world, f32 delta_time) override;
    void cleanup() override;
    
    // Listener management
    void set_primary_listener(Entity listener_entity);
    Entity get_primary_listener() const { return listener_state_.primary_listener_; }
    std::vector<Entity> get_active_listeners() const { return listener_state_.active_listeners_; }
    
    // HRTF management
    bool load_hrtf_database(const std::string& database_name);
    void set_hrtf_enabled_for_listener(Entity listener, bool enabled);
    std::string get_current_hrtf_database() const { return listener_state_.current_hrtf_database_; }
    
    // Educational interface implementation
    std::string get_system_description() const override;
    std::vector<std::string> get_key_concepts() const override;
    std::string generate_educational_summary() const override;
    
    // Listener-specific analysis
    struct ListenerAnalysis {
        u32 active_listeners{0};
        Entity primary_listener{0};
        f32 hrtf_processing_cost_ms{0.0f};
        f32 binaural_rendering_quality{0.8f};
        std::string current_hrtf_profile;
        
        // Per-listener breakdown
        std::unordered_map<Entity, f32> listener_processing_costs;
        std::unordered_map<Entity, std::string> listener_configurations;
        
        // Educational insights
        std::string hrtf_explanation;
        std::string binaural_processing_explanation;
        std::vector<std::string> optimization_techniques;
    };
    
    ListenerAnalysis get_listener_analysis() const;
    
private:
    // Core processing
    void update_active_listeners(World* world);
    void process_listeners(World* world, f32 delta_time);
    void update_listener_transforms(World* world);
    
    // HRTF processing
    void initialize_hrtf_processors();
    void process_hrtf_for_listener(Entity listener, World* world);
    void optimize_hrtf_processing_quality(Entity listener, f32 cpu_budget_ms);
    
    // Multi-listener optimization
    void prioritize_listeners_by_importance(std::vector<Entity>& listeners, World* world);
    void balance_processing_load_across_listeners();
    
    // Educational analysis
    void analyze_binaural_rendering_quality();
    void update_listener_analytics(World* world);
};

//=============================================================================
// Environmental Audio System
//=============================================================================

/**
 * @brief System managing environmental audio effects and room acoustics
 * 
 * Educational Context:
 * Demonstrates advanced environmental audio processing, room acoustics
 * simulation, and the integration of audio with 3D scene geometry.
 * Shows techniques for realistic audio environments in games and simulations.
 */
class AudioEnvironmentSystem : public AudioSystemBase {
private:
    // Environment processing state
    struct EnvironmentState {
        std::vector<Entity> active_environments_;
        std::unordered_map<Entity, std::unique_ptr<AudioEnvironmentProcessor>> processors_;
        Entity global_environment_{0};             // Global fallback environment
        
        // Spatial environment management
        std::vector<Entity> sources_in_environments_;
        std::unordered_map<Entity, std::vector<Entity>> environment_source_mapping_;
        
        // Processing optimization
        f32 environment_update_interval_ms_{16.67f}; // ~60 FPS
        std::chrono::high_resolution_clock::time_point last_update_time_;
        
    } environment_state_;
    
    // Educational metrics
    struct EnvironmentAnalytics {
        u32 active_environments_{0};
        u32 sources_affected_by_environments_{0};
        f32 reverb_processing_cost_ms_{0.0f};
        f32 occlusion_processing_cost_ms_{0.0f};
        f32 environmental_complexity_score_{0.5f}; // 0-1 complexity
        
        std::string dominant_acoustic_characteristic_;
        std::vector<std::string> active_environmental_effects_;
        std::string performance_optimization_status_;
        
    } analytics_;
    
public:
    explicit AudioEnvironmentSystem(memory::MemoryTracker* memory_tracker = nullptr);
    ~AudioEnvironmentSystem() override;
    
    // System interface
    bool initialize(World* world) override;
    void update(World* world, f32 delta_time) override;
    void cleanup() override;
    
    // Environment management
    void set_global_environment(Entity environment_entity);
    Entity get_global_environment() const { return environment_state_.global_environment_; }
    std::vector<Entity> get_active_environments() const { return environment_state_.active_environments_; }
    
    // Environmental effects control
    void set_reverb_enabled(bool enabled);
    void set_occlusion_enabled(bool enabled);
    void set_environmental_update_rate(f32 updates_per_second);
    
    // Educational interface implementation
    std::string get_system_description() const override;
    std::vector<std::string> get_key_concepts() const override;
    std::string generate_educational_summary() const override;
    
    // Environment-specific analysis
    struct EnvironmentAnalysis {
        u32 total_environments{0};
        u32 active_environments{0};
        u32 sources_in_environments{0};
        f32 reverb_processing_cost{0.0f};
        f32 total_environmental_cost{0.0f};
        
        std::string acoustic_scene_description;
        std::string performance_assessment;
        std::vector<std::string> optimization_opportunities;
        
        // Per-environment breakdown
        std::unordered_map<Entity, std::string> environment_descriptions;
        std::unordered_map<Entity, f32> environment_processing_costs;
        
        // Educational insights
        std::string room_acoustics_explanation;
        std::string environmental_audio_principles;
    };
    
    EnvironmentAnalysis get_environment_analysis() const;
    
private:
    // Core processing
    void update_active_environments(World* world);
    void process_environmental_effects(World* world, f32 delta_time);
    void update_source_environment_mapping(World* world);
    
    // Spatial queries for environment membership
    void determine_sources_in_environments(World* world);
    f32 calculate_environment_influence(const AudioEnvironment& env, const spatial_math::Vec3& position);
    
    // Environmental processing
    void apply_reverb_processing(Entity environment, const std::vector<Entity>& sources, World* world);
    void apply_occlusion_processing(Entity environment, const std::vector<Entity>& sources, World* world);
    void blend_environmental_effects(Entity source, const std::vector<Entity>& environments, World* world);
    
    // Performance optimization
    void optimize_environmental_processing(f32 cpu_budget_ms);
    void update_environment_lod(Entity environment, f32 importance_score);
    
    // Educational analysis
    void analyze_acoustic_scene(World* world);
    void update_environmental_analytics(World* world);
};

//=============================================================================
// Audio Analysis System
//=============================================================================

/**
 * @brief System providing real-time audio analysis and visualization
 * 
 * Educational Context:
 * Demonstrates real-time audio analysis techniques, visualization
 * generation, and performance monitoring for educational purposes.
 * Shows how to implement non-critical systems that enhance learning.
 */
class AudioAnalysisSystem : public AudioSystemBase {
private:
    // Analysis processing state
    std::unique_ptr<RealtimeAudioAnalyzer> analyzer_;
    std::unique_ptr<AudioEducationSystem> education_system_;
    
    // Visualization data
    struct VisualizationState {
        // Real-time waveform data
        std::vector<f32> master_waveform_;
        std::vector<f32> frequency_spectrum_;
        std::vector<std::vector<f32>> spectrogram_data_;
        
        // Spatial visualization
        std::vector<spatial_math::Vec3> source_positions_;
        std::vector<f32> source_volumes_;
        std::vector<spatial_math::Vec3> listener_positions_;
        
        // Educational annotations
        std::vector<std::string> current_explanations_;
        std::string active_demonstration_;
        f32 educational_engagement_score_{0.7f};
        
    } visualization_state_;
    
    // Educational session tracking
    struct EducationalSession {
        bool is_active_{false};
        std::string current_student_id_;
        std::string current_lesson_topic_;
        std::chrono::high_resolution_clock::time_point session_start_;
        f32 learning_progress_percent_{0.0f};
        
        // Interaction tracking
        u32 concepts_explored_{0};
        u32 demonstrations_completed_{0};
        f32 engagement_level_{0.5f}; // 0-1 based on interaction patterns
        
    } educational_session_;
    
public:
    explicit AudioAnalysisSystem(memory::MemoryTracker* memory_tracker = nullptr);
    ~AudioAnalysisSystem() override;
    
    // System interface
    bool initialize(World* world) override;
    void update(World* world, f32 delta_time) override;
    void cleanup() override;
    
    // Analysis interface
    RealtimeAudioAnalyzer::AnalysisResults get_latest_analysis() const;
    void set_analysis_enabled(bool enabled);
    void set_visualization_enabled(bool enabled);
    
    // Educational session management
    void start_educational_session(const std::string& student_id, const std::string& topic);
    void end_educational_session();
    bool is_educational_session_active() const { return educational_session_.is_active_; }
    f32 get_learning_progress() const { return educational_session_.learning_progress_percent_; }
    
    // Demonstration control
    void start_audio_demonstration(const std::string& demonstration_id);
    void stop_current_demonstration();
    std::vector<std::string> get_available_demonstrations() const;
    std::string get_current_demonstration() const { return visualization_state_.active_demonstration_; }
    
    // Educational interface implementation
    std::string get_system_description() const override;
    std::vector<std::string> get_key_concepts() const override;
    std::string generate_educational_summary() const override;
    
    // Visualization data access
    const std::vector<f32>& get_master_waveform() const { return visualization_state_.master_waveform_; }
    const std::vector<f32>& get_frequency_spectrum() const { return visualization_state_.frequency_spectrum_; }
    const std::vector<std::vector<f32>>& get_spectrogram_data() const { return visualization_state_.spectrogram_data_; }
    
    // Spatial audio visualization
    std::vector<std::pair<spatial_math::Vec3, f32>> get_source_visualizations() const;
    std::vector<spatial_math::Vec3> get_listener_visualizations() const;
    
    // Educational analytics
    struct EducationalAnalytics {
        bool session_active{false};
        std::string current_student_id;
        std::string current_topic;
        f32 learning_progress_percent{0.0f};
        f32 engagement_level{0.5f};
        u32 concepts_explored{0};
        u32 demonstrations_completed{0};
        
        std::string recommended_next_steps;
        std::vector<std::string> learning_insights;
        f32 educational_effectiveness_score{0.7f};
    };
    
    EducationalAnalytics get_educational_analytics() const;
    
private:
    // Analysis processing
    void perform_realtime_analysis(World* world);
    void update_visualization_data(World* world);
    void generate_spatial_visualizations(World* world);
    
    // Educational processing
    void update_educational_session();
    void track_learning_progress();
    void analyze_educational_engagement();
    void generate_learning_insights();
    
    // Performance monitoring
    void monitor_audio_system_performance(World* world);
};

//=============================================================================
// Audio Memory Management System
//=============================================================================

/**
 * @brief System managing audio memory allocation and optimization
 * 
 * Educational Context:
 * Demonstrates memory management techniques specific to real-time audio,
 * including memory pools, cache optimization, and garbage collection
 * avoidance in audio systems.
 */
class AudioMemorySystem : public AudioSystemBase {
private:
    // Memory pool management
    std::unique_ptr<AudioBufferPool> audio_buffer_pool_;
    std::unique_ptr<memory::Arena> audio_component_arena_;
    
    // Memory tracking and analytics
    struct MemoryAnalytics {
        usize total_audio_memory_allocated_{0};
        usize peak_audio_memory_usage_{0};
        usize current_audio_buffer_count_{0};
        f32 memory_fragmentation_ratio_{0.0f};
        u32 garbage_collections_avoided_{0};
        
        // Educational metrics
        f32 cache_hit_ratio_{0.95f};
        f32 memory_allocation_efficiency_{0.9f};
        std::string memory_optimization_status_;
        std::vector<std::string> optimization_techniques_used_;
        
    } memory_analytics_;
    
public:
    explicit AudioMemorySystem(memory::MemoryTracker* memory_tracker = nullptr);
    ~AudioMemorySystem() override;
    
    // System interface
    bool initialize(World* world) override;
    void update(World* world, f32 delta_time) override;
    void cleanup() override;
    
    // Memory pool access
    AudioBufferPool* get_audio_buffer_pool() { return audio_buffer_pool_.get(); }
    memory::Arena* get_audio_component_arena() { return audio_component_arena_.get(); }
    
    // Memory management operations
    void garbage_collect_audio_resources();
    void optimize_memory_layout();
    void defragment_audio_pools();
    
    // Educational interface implementation
    std::string get_system_description() const override;
    std::vector<std::string> get_key_concepts() const override;
    std::string generate_educational_summary() const override;
    
    // Memory analytics
    struct MemoryAnalysis {
        usize total_memory_used{0};
        usize peak_memory_used{0};
        usize audio_buffers_allocated{0};
        f32 fragmentation_ratio{0.0f};
        f32 allocation_efficiency{0.9f};
        
        std::string memory_health_status;
        std::vector<std::string> memory_optimization_suggestions;
        
        // Educational insights
        std::string memory_management_explanation;
        std::string performance_impact_analysis;
    };
    
    MemoryAnalysis get_memory_analysis() const;
    
private:
    // Memory pool operations
    void initialize_memory_pools();
    void update_memory_statistics();
    void detect_memory_leaks();
    
    // Educational analysis
    void analyze_memory_usage_patterns();
    void generate_memory_optimization_insights();
};

//=============================================================================
// Audio-Physics Integration System
//=============================================================================

/**
 * @brief System integrating audio with physics for realistic audio interactions
 * 
 * Educational Context:
 * Demonstrates the integration between audio and physics systems,
 * showing how to implement realistic audio behaviors like occlusion,
 * Doppler effects, and physics-based audio responses.
 */
class AudioPhysicsIntegrationSystem : public AudioSystemBase {
private:
    // Physics integration state
    struct PhysicsIntegrationState {
        // Occlusion calculation
        std::vector<Entity> sources_requiring_occlusion_;
        std::unordered_map<Entity, f32> cached_occlusion_values_;
        f32 occlusion_raycast_distance_{100.0f};
        u32 raycast_resolution_{8}; // Number of rays per occlusion test
        
        // Doppler effect calculation
        std::vector<Entity> sources_with_doppler_;
        std::unordered_map<Entity, spatial_math::Vec3> previous_positions_;
        f32 doppler_update_interval_ms_{16.67f}; // ~60 FPS
        
        // Physics-based audio responses
        std::vector<Entity> collision_audio_sources_;
        std::unordered_map<Entity, f32> impact_audio_cooldowns_;
        f32 minimum_impact_velocity_{0.5f}; // m/s
        
    } physics_state_;
    
    // Educational analytics
    struct PhysicsAudioAnalytics {
        u32 occlusion_calculations_per_frame_{0};
        u32 doppler_calculations_per_frame_{0};
        u32 collision_audio_events_per_frame_{0};
        f32 physics_audio_processing_cost_ms_{0.0f};
        
        std::string physics_integration_quality_;
        std::vector<std::string> active_physics_audio_effects_;
        std::string performance_optimization_status_;
        
    } physics_analytics_;
    
public:
    explicit AudioPhysicsIntegrationSystem(memory::MemoryTracker* memory_tracker = nullptr);
    ~AudioPhysicsIntegrationSystem() override;
    
    // System interface
    bool initialize(World* world) override;
    void update(World* world, f32 delta_time) override;
    void cleanup() override;
    
    // Physics integration configuration
    void set_occlusion_enabled(bool enabled);
    void set_doppler_enabled(bool enabled);
    void set_collision_audio_enabled(bool enabled);
    void set_occlusion_raycast_resolution(u32 ray_count);
    void set_minimum_impact_velocity(f32 velocity_ms);
    
    // Educational interface implementation
    std::string get_system_description() const override;
    std::vector<std::string> get_key_concepts() const override;
    std::string generate_educational_summary() const override;
    
    // Physics-audio analysis
    struct PhysicsAudioAnalysis {
        u32 sources_with_occlusion{0};
        u32 sources_with_doppler{0};
        u32 collision_audio_events{0};
        f32 physics_processing_cost{0.0f};
        f32 audio_realism_score{0.8f}; // 0-1 based on physics integration
        
        std::string occlusion_quality_assessment;
        std::string doppler_accuracy_assessment;
        std::vector<std::string> physics_integration_insights;
        
        // Educational explanations
        std::string physics_audio_principles;
        std::string realism_vs_performance_analysis;
    };
    
    PhysicsAudioAnalysis get_physics_audio_analysis() const;
    
private:
    // Occlusion processing
    void update_audio_occlusion(World* world, f32 delta_time);
    f32 calculate_occlusion_for_source(Entity source, Entity listener, World* world);
    bool raycast_occlusion(const spatial_math::Vec3& start, const spatial_math::Vec3& end, World* world);
    
    // Doppler effect processing
    void update_doppler_effects(World* world, f32 delta_time);
    f32 calculate_doppler_shift(const spatial_math::Vec3& source_velocity, 
                               const spatial_math::Vec3& listener_velocity, 
                               const spatial_math::Vec3& relative_direction);
    
    // Collision audio processing
    void process_collision_audio(World* world, f32 delta_time);
    void generate_impact_audio(Entity colliding_entity, f32 impact_velocity, 
                              const physics::components::PhysicsMaterial& material);
    
    // Educational analysis
    void analyze_physics_integration_quality(World* world);
    void update_physics_audio_analytics();
};

//=============================================================================
// Audio System Manager
//=============================================================================

/**
 * @brief Manager coordinating all audio systems
 * 
 * Educational Context:
 * Demonstrates how to coordinate multiple interdependent systems
 * in a complex real-time application, showing patterns for system
 * communication, dependency management, and performance balancing.
 */
class AudioSystemManager {
private:
    // Managed systems
    std::unique_ptr<SpatialAudioSystem> spatial_audio_system_;
    std::unique_ptr<AudioListenerSystem> listener_system_;
    std::unique_ptr<AudioEnvironmentSystem> environment_system_;
    std::unique_ptr<AudioAnalysisSystem> analysis_system_;
    std::unique_ptr<AudioMemorySystem> memory_system_;
    std::unique_ptr<AudioPhysicsIntegrationSystem> physics_integration_system_;
    
    // System coordination
    World* managed_world_{nullptr};
    bool systems_initialized_{false};
    std::atomic<bool> systems_active_{false};
    
    // Performance management
    struct SystemPerformanceManager {
        f32 target_frame_time_ms_{16.67f}; // 60 FPS
        f32 audio_budget_percent_{25.0f}; // 25% of frame time for audio
        std::unordered_map<std::string, f32> system_time_budgets_;
        std::unordered_map<std::string, f32> system_actual_times_;
        
        bool adaptive_performance_enabled_{true};
        f32 performance_scale_factor_{1.0f}; // Global performance scaling
        
    } performance_manager_;
    
    // Educational coordination
    std::string current_educational_focus_;
    f32 overall_educational_value_score_{0.75f};
    std::vector<std::string> system_interdependency_explanations_;
    
public:
    AudioSystemManager();
    ~AudioSystemManager();
    
    // System lifecycle management
    bool initialize_all_systems(World* world);
    void update_all_systems(f32 delta_time);
    void cleanup_all_systems();
    
    // System access
    SpatialAudioSystem* get_spatial_audio_system() { return spatial_audio_system_.get(); }
    AudioListenerSystem* get_listener_system() { return listener_system_.get(); }
    AudioEnvironmentSystem* get_environment_system() { return environment_system_.get(); }
    AudioAnalysisSystem* get_analysis_system() { return analysis_system_.get(); }
    AudioMemorySystem* get_memory_system() { return memory_system_.get(); }
    AudioPhysicsIntegrationSystem* get_physics_integration_system() { return physics_integration_system_.get(); }
    
    // Performance management
    void set_audio_performance_budget(f32 budget_percent);
    void enable_adaptive_performance(bool enabled);
    f32 get_current_performance_scale() const { return performance_manager_.performance_scale_factor_; }
    
    // Educational interface
    void set_educational_focus(const std::string& focus_area);
    std::string get_current_educational_focus() const { return current_educational_focus_; }
    f32 get_overall_educational_value() const { return overall_educational_value_score_; }
    std::vector<std::string> get_system_interdependency_explanations() const;
    
    // Comprehensive system analysis
    struct SystemAnalysis {
        // Performance metrics
        f32 total_audio_processing_time_ms{0.0f};
        f32 audio_cpu_usage_percent{0.0f};
        std::unordered_map<std::string, f32> system_performance_breakdown;
        
        // Functional metrics
        u32 active_audio_sources{0};
        u32 active_listeners{0};
        u32 active_environments{0};
        f32 overall_audio_quality_score{0.8f};
        
        // Educational metrics
        f32 educational_value_score{0.75f};
        std::string current_educational_focus;
        std::vector<std::string> key_learning_concepts;
        
        // System health
        std::string overall_system_health;
        std::vector<std::string> performance_recommendations;
        std::vector<std::string> educational_opportunities;
    };
    
    SystemAnalysis get_comprehensive_analysis() const;
    
    // Educational reporting
    void generate_system_architecture_report() const;
    void generate_performance_analysis_report() const;
    void generate_educational_effectiveness_report() const;
    
private:
    // System coordination
    void coordinate_system_updates(f32 delta_time);
    void manage_system_dependencies();
    void balance_system_performance();
    
    // Performance optimization
    void update_performance_budgets();
    void apply_adaptive_performance_scaling();
    void analyze_system_bottlenecks();
    
    // Educational analysis
    void update_educational_metrics();
    void analyze_system_interdependencies();
    void generate_learning_insights();
};

} // namespace ecscope::audio::systems