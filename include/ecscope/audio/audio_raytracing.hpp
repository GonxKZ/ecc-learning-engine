#pragma once

#include "audio_types.hpp"
#include <memory>
#include <vector>
#include <unordered_map>
#include <atomic>
#include <functional>

namespace ecscope::audio {

// Forward declarations
class Audio3DScene;

// Material properties for audio ray tracing
struct AcousticMaterial {
    std::string name;
    float absorption_coefficients[10];  // Per frequency band
    float scattering_coefficient = 0.1f;
    float transmission_coefficient = 0.0f;
    float impedance = 415.0f;  // Air impedance by default
    float density = 1.225f;    // kg/m³
    
    // Frequency-dependent properties
    std::vector<float> frequencies;
    std::vector<float> absorption_spectrum;
    std::vector<float> scattering_spectrum;
    
    // Constructor with common materials
    static AcousticMaterial concrete();
    static AcousticMaterial wood();
    static AcousticMaterial carpet();
    static AcousticMaterial glass();
    static AcousticMaterial metal();
    static AcousticMaterial fabric();
    static AcousticMaterial water();
    static AcousticMaterial air();
};

// Geometric primitives for acoustic simulation
struct AcousticGeometry {
    enum class Type {
        TRIANGLE,
        QUAD,
        SPHERE,
        BOX,
        CYLINDER,
        MESH
    };
    
    Type type;
    std::vector<Vector3f> vertices;
    std::vector<uint32_t> indices;
    AcousticMaterial material;
    Vector3f center;
    Vector3f normal;
    float area = 0.0f;
    uint32_t material_id = 0;
    bool is_portal = false;  // For room-to-room propagation
    
    // Bounding volume for acceleration
    Vector3f aabb_min, aabb_max;
};

// Audio ray for tracing
struct AudioRay {
    Vector3f origin;
    Vector3f direction;
    float energy = 1.0f;
    float frequency = 1000.0f;  // Reference frequency
    int bounce_count = 0;
    int max_bounces = 10;
    float travel_distance = 0.0f;
    float travel_time = 0.0f;
    uint32_t ray_id = 0;
    
    // Ray classification
    enum class Type {
        DIRECT,         // Direct path
        EARLY_REFLECTION, // Early reflections
        LATE_REFLECTION,  // Late reflections/reverb
        DIFFRACTION,    // Diffracted rays
        TRANSMISSION    // Transmitted through materials
    } type = Type::DIRECT;
    
    // History tracking
    std::vector<Vector3f> path_points;
    std::vector<uint32_t> hit_materials;
    std::vector<float> bounce_energies;
};

// Ray intersection result
struct RayIntersection {
    bool hit = false;
    Vector3f point;
    Vector3f normal;
    float distance = std::numeric_limits<float>::max();
    uint32_t geometry_id = 0;
    AcousticMaterial material;
    float u = 0.0f, v = 0.0f;  // Barycentric coordinates
};

// Impulse response generation from ray tracing
struct RayTracingImpulseResponse {
    AudioBuffer left_response;
    AudioBuffer right_response;
    float sample_rate = 44100.0f;
    float length_seconds = 2.0f;
    uint32_t num_rays_traced = 0;
    uint32_t num_reflections_found = 0;
    
    // Analysis data
    float direct_path_delay = 0.0f;
    float early_reflection_time = 0.08f;  // 80ms
    float reverb_time_60db = 1.0f;
    std::vector<float> energy_decay_curve;
};

// Audio ray tracer engine
class AudioRayTracer {
public:
    struct TracingParameters {
        uint32_t num_rays = 10000;
        int max_bounces = 10;
        float min_energy_threshold = 0.001f;
        float max_trace_distance = 1000.0f;
        float ray_spread_angle = 360.0f;  // Degrees
        bool enable_diffraction = true;
        bool enable_transmission = false;
        bool enable_scattering = true;
        float air_absorption_coefficient = 0.0001f;
        float speed_of_sound = 343.3f;
        
        // Quality settings
        int frequency_bands = 10;
        float min_frequency = 20.0f;
        float max_frequency = 20000.0f;
        bool use_multiband_tracing = true;
        
        // Performance settings
        int thread_count = 0;  // 0 = auto-detect
        bool use_gpu_acceleration = false;
        bool enable_spatial_hashing = true;
        int max_rays_per_frame = 1000;
    };
    
    AudioRayTracer();
    ~AudioRayTracer();
    
    // Configuration
    void initialize(const TracingParameters& params);
    void set_scene_geometry(const std::vector<AcousticGeometry>& geometry);
    void add_acoustic_material(const AcousticMaterial& material);
    void set_tracing_parameters(const TracingParameters& params);
    
    // Ray tracing operations
    RayTracingImpulseResponse trace_impulse_response(
        const Vector3f& source_position,
        const Vector3f& listener_position,
        const AudioListener& listener_orientation
    );
    
    std::vector<AudioRay> trace_rays_from_source(
        const Vector3f& source_position,
        uint32_t num_rays
    );
    
    void trace_single_ray(AudioRay& ray, std::vector<RayIntersection>& intersections);
    
    // Real-time tracing for dynamic scenes
    void start_realtime_tracing(const Vector3f& source_pos, const Vector3f& listener_pos);
    void stop_realtime_tracing();
    void update_realtime_tracing(float delta_time);
    RayTracingImpulseResponse get_current_impulse_response();
    
    // Early reflection calculation
    std::vector<AudioRay> calculate_early_reflections(
        const Vector3f& source_position,
        const Vector3f& listener_position,
        int max_order = 3
    );
    
    // Diffraction modeling
    std::vector<AudioRay> calculate_diffracted_rays(
        const Vector3f& source_position,
        const Vector3f& listener_position,
        const std::vector<Vector3f>& edge_points
    );
    
    // Scene acceleration structures
    void build_bvh_acceleration_structure();
    void build_octree_acceleration_structure();
    void enable_spatial_caching(bool enable, float cache_resolution = 1.0f);
    
    // Performance monitoring
    struct TracingStats {
        uint32_t rays_traced_per_second = 0;
        uint32_t intersections_tested = 0;
        uint32_t intersections_found = 0;
        float average_bounce_count = 0.0f;
        float tracing_time_ms = 0.0f;
        float memory_usage_mb = 0.0f;
    };
    
    TracingStats get_tracing_statistics() const;
    void reset_statistics();
    
    // Visualization and debugging
    std::vector<AudioRay> get_debug_rays() const;
    void enable_ray_visualization(bool enable);
    void set_visualization_parameters(int max_displayed_rays, float ray_thickness);

private:
    class Impl;
    std::unique_ptr<Impl> m_impl;
};

// Spatial audio acceleration structures
class AudioSpatialAcceleration {
public:
    virtual ~AudioSpatialAcceleration() = default;
    
    virtual void build(const std::vector<AcousticGeometry>& geometry) = 0;
    virtual bool intersect_ray(const AudioRay& ray, RayIntersection& intersection) = 0;
    virtual std::vector<uint32_t> query_geometry_in_sphere(
        const Vector3f& center, float radius) = 0;
    virtual void update_dynamic_geometry(uint32_t geometry_id, 
                                       const AcousticGeometry& geometry) = 0;
};

// Bounding Volume Hierarchy for fast ray intersection
class AudioBVH : public AudioSpatialAcceleration {
public:
    AudioBVH();
    ~AudioBVH() override;
    
    void build(const std::vector<AcousticGeometry>& geometry) override;
    bool intersect_ray(const AudioRay& ray, RayIntersection& intersection) override;
    std::vector<uint32_t> query_geometry_in_sphere(
        const Vector3f& center, float radius) override;
    void update_dynamic_geometry(uint32_t geometry_id, 
                               const AcousticGeometry& geometry) override;
    
    // BVH-specific configuration
    void set_max_leaf_size(int max_size);
    void set_split_method(enum SplitMethod { SAH, MIDDLE, EQUAL_COUNTS } method);
    void enable_packet_traversal(bool enable);

private:
    class Impl;
    std::unique_ptr<Impl> m_impl;
};

// Octree for spatial queries
class AudioOctree : public AudioSpatialAcceleration {
public:
    AudioOctree(const Vector3f& min_bounds, const Vector3f& max_bounds, int max_depth = 8);
    ~AudioOctree() override;
    
    void build(const std::vector<AcousticGeometry>& geometry) override;
    bool intersect_ray(const AudioRay& ray, RayIntersection& intersection) override;
    std::vector<uint32_t> query_geometry_in_sphere(
        const Vector3f& center, float radius) override;
    void update_dynamic_geometry(uint32_t geometry_id, 
                               const AcousticGeometry& geometry) override;
    
    // Octree-specific methods
    void set_max_objects_per_node(int max_objects);
    void enable_adaptive_subdivision(bool enable);

private:
    class Impl;
    std::unique_ptr<Impl> m_impl;
};

// Acoustic simulation using wave equation
class WaveEquationSolver {
public:
    struct SimulationParameters {
        float grid_spacing = 0.1f;  // meters
        float time_step = 0.0001f;  // seconds
        Vector3f domain_min{-10, -10, -10};
        Vector3f domain_max{10, 10, 10};
        int max_time_steps = 44100;  // 1 second at 44.1kHz
        float courant_number = 0.5f;
        bool enable_pml_boundaries = true;  // Perfectly Matched Layer
        int pml_thickness = 10;
    };
    
    WaveEquationSolver();
    ~WaveEquationSolver();
    
    // Configuration
    void initialize(const SimulationParameters& params);
    void set_geometry(const std::vector<AcousticGeometry>& geometry);
    void set_source_position(const Vector3f& position);
    void add_receiver(const Vector3f& position);
    
    // Simulation
    void run_simulation();
    AudioBuffer get_receiver_response(int receiver_id) const;
    void step_simulation(float delta_time);
    
    // Boundary conditions
    void set_boundary_condition(enum BoundaryType {
        RIGID, ABSORBING, PERIODIC, FREE_FIELD
    } type);
    
    // Performance settings
    void enable_gpu_acceleration(bool enable);
    void set_thread_count(int threads);

private:
    class Impl;
    std::unique_ptr<Impl> m_impl;
};

// Convolution-based room acoustics
class ConvolutionRoomAcoustics {
public:
    ConvolutionRoomAcoustics();
    ~ConvolutionRoomAcoustics();
    
    // Room impulse response generation
    void generate_room_impulse_response(
        const std::vector<AcousticGeometry>& room_geometry,
        const Vector3f& source_position,
        const Vector3f& listener_position,
        RayTracingImpulseResponse& response
    );
    
    // Real-time convolution
    void process_audio_with_room(const AudioBuffer& dry_audio,
                               AudioBuffer& wet_audio);
    
    void set_impulse_response(const RayTracingImpulseResponse& response);
    void enable_early_late_split(bool enable);
    void set_early_late_crossover_time(float time_ms);
    
    // Multiple room support
    void add_room_configuration(const std::string& name,
                              const RayTracingImpulseResponse& response);
    void switch_to_room(const std::string& name, float crossfade_time = 1.0f);
    
    // Performance optimization
    void set_convolution_block_size(int block_size);
    void enable_partitioned_convolution(bool enable);

private:
    class Impl;
    std::unique_ptr<Impl> m_impl;
};

// Integrated ray tracing audio processor
class RayTracingAudioProcessor {
public:
    RayTracingAudioProcessor();
    ~RayTracingAudioProcessor();
    
    // Initialization
    void initialize(uint32_t sample_rate, uint32_t buffer_size);
    void set_scene(const Audio3DScene& scene);
    void set_ray_tracing_quality(int quality);  // 1-10 scale
    
    // Processing
    void process_3d_audio_with_raytracing(
        const std::vector<AudioBuffer>& source_inputs,
        const std::vector<Vector3f>& source_positions,
        const AudioListener& listener,
        StereoBuffer& output
    );
    
    // Dynamic updates
    void update_source_position(uint32_t source_id, const Vector3f& position);
    void update_listener_position(const AudioListener& listener);
    void update_scene_geometry(const std::vector<AcousticGeometry>& geometry);
    
    // Feature control
    void enable_early_reflections(bool enable);
    void enable_late_reverb(bool enable);
    void enable_diffraction(bool enable);
    void enable_air_absorption(bool enable);
    void set_max_reflection_order(int max_order);
    
    // Performance monitoring
    AudioMetrics get_raytracing_metrics() const;
    float get_raytracing_cpu_usage() const;
    float get_memory_usage_mb() const;

private:
    std::unique_ptr<AudioRayTracer> m_ray_tracer;
    std::unique_ptr<ConvolutionRoomAcoustics> m_convolution_processor;
    
    uint32_t m_sample_rate;
    uint32_t m_buffer_size;
    int m_quality_level = 5;
    
    // Cached impulse responses
    std::unordered_map<std::string, RayTracingImpulseResponse> m_ir_cache;
    
    mutable AudioMetrics m_metrics;
};

// Ray tracing utilities
namespace raytracing_utils {
    // Geometric calculations
    bool ray_triangle_intersection(const AudioRay& ray, 
                                 const Vector3f& v0, const Vector3f& v1, const Vector3f& v2,
                                 float& t, float& u, float& v);
    
    bool ray_sphere_intersection(const AudioRay& ray, 
                               const Vector3f& center, float radius,
                               float& t1, float& t2);
    
    bool ray_aabb_intersection(const AudioRay& ray,
                             const Vector3f& aabb_min, const Vector3f& aabb_max,
                             float& t_min, float& t_max);
    
    // Reflection calculations
    Vector3f calculate_reflection_direction(const Vector3f& incident,
                                          const Vector3f& normal);
    
    Vector3f calculate_refraction_direction(const Vector3f& incident,
                                          const Vector3f& normal,
                                          float n1, float n2);  // Refractive indices
    
    // Energy calculations
    float calculate_absorption_loss(float energy, const AcousticMaterial& material,
                                  float frequency, float angle);
    
    float calculate_transmission_loss(float energy, const AcousticMaterial& material,
                                    float thickness, float frequency);
    
    float calculate_air_absorption_loss(float energy, float distance,
                                      float frequency, float humidity = 50.0f);
    
    // Diffraction modeling
    std::vector<Vector3f> calculate_diffraction_points(
        const Vector3f& source, const Vector3f& receiver,
        const std::vector<Vector3f>& edge_points);
    
    float calculate_diffraction_loss(const Vector3f& source, const Vector3f& receiver,
                                   const Vector3f& edge_point, float frequency);
    
    // Impulse response processing
    void normalize_impulse_response(RayTracingImpulseResponse& response);
    void apply_distance_delay(RayTracingImpulseResponse& response, float distance);
    void merge_impulse_responses(const RayTracingImpulseResponse& a,
                               const RayTracingImpulseResponse& b,
                               RayTracingImpulseResponse& result);
    
    // Quality assessment
    float calculate_rt60(const AudioBuffer& impulse_response, uint32_t sample_rate);
    float calculate_clarity_c80(const AudioBuffer& impulse_response, uint32_t sample_rate);
    float calculate_definition_d50(const AudioBuffer& impulse_response, uint32_t sample_rate);
}

} // namespace ecscope::audio