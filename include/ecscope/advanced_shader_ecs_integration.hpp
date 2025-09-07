#pragma once

/**
 * @file advanced_shader_ecs_integration.hpp
 * @brief Complete ECS Integration for Advanced Shader System in ECScope
 * 
 * This system provides seamless integration between the advanced shader system and ECScope's ECS:
 * - Material component system with automatic shader binding
 * - Rendering components with shader-aware optimizations
 * - ECS systems for shader management and updates
 * - Performance-optimized batch rendering with shader sorting
 * - Dynamic LOD and culling systems
 * - Educational components for learning graphics programming
 * - Advanced lighting and shadow systems
 * - Post-processing pipeline integration
 * 
 * @author ECScope Educational ECS Framework - Advanced Shader System
 * @date 2024
 */

#include "shader_runtime_system.hpp"
#include "advanced_shader_library.hpp"
#include "shader_debugging_tools.hpp"
#include "visual_shader_editor.hpp"
#include "components.hpp"
#include "system.hpp"
#include "world.hpp"
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <functional>
#include <optional>
#include <array>

namespace ecscope::renderer::ecs_integration {

//=============================================================================
// Material Component System
//=============================================================================

/**
 * @brief Material Component for ECS entities
 * 
 * Represents a material that can be applied to renderable entities.
 * Integrates with the advanced shader system for automatic shader selection.
 */
struct MaterialComponent {
    // Shader reference
    shader_runtime::ShaderRuntimeManager::ShaderHandle shader_handle{0};
    std::string material_name;
    
    // Material properties
    shader_library::PBRMaterial pbr_properties;
    std::unordered_map<std::string, shader_library::shader_library::DebugValue> custom_uniforms;
    
    // Rendering flags
    bool is_transparent{false};
    bool cast_shadows{true};
    bool receive_shadows{true};
    bool double_sided{false};
    
    // LOD and optimization
    u32 current_lod_level{0};
    f32 lod_bias{0.0f};
    std::vector<shader_runtime::ShaderRuntimeManager::ShaderHandle> lod_variants;
    
    // Animation and dynamic properties
    bool has_animated_properties{false};
    std::unordered_map<std::string, f32> animated_uniform_speeds;
    
    // Educational features
    bool show_debug_info{false};
    std::string educational_description;
    
    MaterialComponent() = default;
    
    MaterialComponent(shader_runtime::ShaderRuntimeManager::ShaderHandle handle, 
                     const std::string& name = "")
        : shader_handle(handle), material_name(name) {}
    
    MaterialComponent(const shader_library::PBRMaterial& pbr_mat, const std::string& name = "")
        : pbr_properties(pbr_mat), material_name(name) {}
};

/**
 * @brief Advanced Texture Component
 * 
 * Enhanced texture component with advanced features for the shader system.
 */
struct AdvancedTextureComponent {
    // Basic texture properties
    u32 texture_id{0};
    std::string texture_path;
    
    // Advanced properties
    enum class FilterMode { Nearest, Linear, Trilinear, Anisotropic };
    enum class WrapMode { Repeat, MirroredRepeat, ClampToEdge, ClampToBorder };
    
    FilterMode min_filter{FilterMode::Linear};
    FilterMode mag_filter{FilterMode::Linear};
    WrapMode wrap_u{WrapMode::Repeat};
    WrapMode wrap_v{WrapMode::Repeat};
    WrapMode wrap_w{WrapMode::Repeat};
    
    // Anisotropic filtering
    f32 max_anisotropy{16.0f};
    
    // Mipmapping
    bool generate_mipmaps{true};
    f32 mipmap_bias{0.0f};
    
    // Compression and format
    enum class TextureFormat { RGB8, RGBA8, RGB16F, RGBA16F, RGB32F, RGBA32F, BC1, BC3, BC7 };
    TextureFormat format{TextureFormat::RGBA8};
    bool use_compression{true};
    
    // Streaming and LOD
    bool enable_streaming{false};
    u32 max_mip_level{0};
    f32 lod_bias{0.0f};
    
    // Educational features
    bool show_mipmap_levels{false};
    std::array<f32, 4> debug_tint{1.0f, 1.0f, 1.0f, 1.0f};
    
    AdvancedTextureComponent() = default;
    explicit AdvancedTextureComponent(const std::string& path) : texture_path(path) {}
};

/**
 * @brief Shader Variant Component
 * 
 * Allows entities to use different shader variants based on conditions.
 */
struct ShaderVariantComponent {
    struct VariantCondition {
        std::string condition_name;
        std::function<bool(Entity)> condition_check;
        shader_runtime::ShaderRuntimeManager::ShaderHandle shader_handle;
        f32 priority{0.0f}; // Higher priority variants are preferred
    };
    
    std::vector<VariantCondition> variants;
    shader_runtime::ShaderRuntimeManager::ShaderHandle current_variant{0};
    shader_runtime::ShaderRuntimeManager::ShaderHandle fallback_shader{0};
    
    // Performance tracking
    u32 variant_switches_this_frame{0};
    f32 last_switch_time{0.0f};
    
    void add_variant(const std::string& condition, 
                    std::function<bool(Entity)> check,
                    shader_runtime::ShaderRuntimeManager::ShaderHandle handle,
                    f32 priority = 0.0f) {
        variants.push_back({condition, check, handle, priority});
        
        // Sort by priority (highest first)
        std::sort(variants.begin(), variants.end(),
                 [](const VariantCondition& a, const VariantCondition& b) {
                     return a.priority > b.priority;
                 });
    }
};

//=============================================================================
// Advanced Rendering Components
//=============================================================================

/**
 * @brief GPU Instance Data Component
 * 
 * For efficient instanced rendering with per-instance shader data.
 */
struct GPUInstanceDataComponent {
    // Per-instance data
    struct InstanceData {
        std::array<f32, 16> model_matrix;      ///< Model transformation matrix
        std::array<f32, 4> color_tint{1.0f, 1.0f, 1.0f, 1.0f};
        std::array<f32, 4> custom_data{0.0f, 0.0f, 0.0f, 0.0f}; ///< Custom shader data
        f32 scale_factor{1.0f};
        f32 animation_time{0.0f};
        u32 material_id{0};
        u32 flags{0};
    };
    
    std::vector<InstanceData> instances;
    u32 instance_buffer_id{0};
    bool buffer_needs_update{true};
    
    // Culling and LOD
    std::vector<bool> instance_visibility;
    std::vector<u32> instance_lod_levels;
    
    // Performance tracking
    u32 instances_rendered_last_frame{0};
    u32 instances_culled_last_frame{0};
    
    void add_instance(const InstanceData& instance) {
        instances.push_back(instance);
        instance_visibility.push_back(true);
        instance_lod_levels.push_back(0);
        buffer_needs_update = true;
    }
    
    void remove_instance(usize index) {
        if (index < instances.size()) {
            instances.erase(instances.begin() + index);
            instance_visibility.erase(instance_visibility.begin() + index);
            instance_lod_levels.erase(instance_lod_levels.begin() + index);
            buffer_needs_update = true;
        }
    }
    
    void mark_dirty() { buffer_needs_update = true; }
};

/**
 * @brief Lighting Component
 * 
 * Enhanced lighting component for advanced shader-based lighting.
 */
struct AdvancedLightComponent {
    // Basic light properties
    shader_library::Light light_data;
    
    // Shadow properties
    bool cast_shadows{true};
    u32 shadow_map_size{1024};
    f32 shadow_bias{0.0001f};
    f32 shadow_normal_bias{0.1f};
    
    // Advanced shadow features
    bool use_cascade_shadows{false};
    std::vector<f32> cascade_distances{10.0f, 50.0f, 200.0f, 1000.0f};
    bool use_soft_shadows{false};
    f32 shadow_softness{1.0f};
    
    // Volumetric lighting
    bool enable_volumetrics{false};
    f32 volumetric_density{0.1f};
    f32 volumetric_scattering{0.5f};
    std::array<f32, 3> volumetric_color{1.0f, 1.0f, 1.0f};
    
    // Light animation
    bool animate_intensity{false};
    bool animate_color{false};
    f32 animation_speed{1.0f};
    std::array<f32, 3> base_color;
    std::array<f32, 3> animation_color_range{0.0f, 0.0f, 0.0f};
    
    // Performance and culling
    f32 cull_distance{100.0f};
    bool is_visible{true};
    u32 affected_objects{0};
    
    AdvancedLightComponent() {
        base_color = light_data.color;
    }
};

/**
 * @brief Post-Processing Component
 * 
 * Allows entities to apply post-processing effects.
 */
struct PostProcessingComponent {
    // Effect chain
    struct Effect {
        std::string effect_name;
        shader_runtime::ShaderRuntimeManager::ShaderHandle shader_handle;
        std::unordered_map<std::string, shader_library::shader_library::DebugValue> parameters;
        bool enabled{true};
        f32 strength{1.0f};
    };
    
    std::vector<Effect> effects;
    bool enable_post_processing{true};
    
    // Render targets
    u32 input_texture{0};
    u32 output_texture{0};
    std::vector<u32> intermediate_textures;
    
    // Performance
    bool use_half_resolution{false};
    f32 render_scale{1.0f};
    
    void add_effect(const std::string& name, 
                   shader_runtime::ShaderRuntimeManager::ShaderHandle handle,
                   const std::unordered_map<std::string, shader_library::shader_library::DebugValue>& params = {}) {
        effects.push_back({name, handle, params});
    }
    
    void remove_effect(const std::string& name) {
        effects.erase(
            std::remove_if(effects.begin(), effects.end(),
                          [&name](const Effect& effect) { return effect.effect_name == name; }),
            effects.end());
    }
};

//=============================================================================
// Shader Management Systems
//=============================================================================

/**
 * @brief Material Management System
 * 
 * Manages material components and their shader bindings.
 */
class MaterialManagementSystem : public System {
public:
    explicit MaterialManagementSystem(World& world,
                                     shader_runtime::ShaderRuntimeManager* runtime_manager,
                                     shader_library::AdvancedShaderLibrary* library)
        : System(world), runtime_manager_(runtime_manager), shader_library_(library) {}
    
    void initialize() override {
        require<MaterialComponent>();
        
        // Set up material presets
        setup_material_presets();
        
        // Initialize default materials
        create_default_materials();
    }
    
    void update(f32 delta_time) override {
        // Update animated material properties
        update_animated_materials(delta_time);
        
        // Handle shader recompilation if needed
        check_shader_recompilation();
        
        // Update LOD variants based on distance
        update_material_lod();
        
        // Update performance statistics
        update_material_statistics();
    }
    
    // Material creation and management
    MaterialComponent create_pbr_material(const shader_library::PBRMaterial& pbr_properties,
                                         const std::string& name = "");
    
    MaterialComponent create_material_from_template(const std::string& template_name,
                                                   const std::unordered_map<std::string, std::string>& parameters,
                                                   const std::string& name = "");
    
    void update_material_uniforms(Entity entity, const MaterialComponent& material);
    
    // Shader variant management
    void add_material_variant(Entity entity, const std::string& condition,
                             std::function<bool(Entity)> check,
                             shader_runtime::ShaderRuntimeManager::ShaderHandle shader,
                             f32 priority = 0.0f);
    
    // Educational features
    void enable_material_debugging(Entity entity, bool enable = true);
    std::string get_material_explanation(Entity entity) const;
    
    // Performance analysis
    struct MaterialPerformanceReport {
        u32 total_materials{0};
        u32 unique_shaders{0};
        u32 animated_materials{0};
        f32 average_uniform_updates_per_frame{0.0f};
        std::vector<std::string> optimization_suggestions;
    };
    
    MaterialPerformanceReport generate_performance_report() const;
    
private:
    shader_runtime::ShaderRuntimeManager* runtime_manager_;
    shader_library::AdvancedShaderLibrary* shader_library_;
    
    // Material presets and defaults
    std::unordered_map<std::string, shader_library::PBRMaterial> material_presets_;
    std::unordered_map<std::string, shader_runtime::ShaderRuntimeManager::ShaderHandle> default_shaders_;
    
    // Performance tracking
    u32 uniform_updates_this_frame_{0};
    u32 shader_switches_this_frame_{0};
    f32 total_material_update_time_{0.0f};
    
    void setup_material_presets();
    void create_default_materials();
    void update_animated_materials(f32 delta_time);
    void check_shader_recompilation();
    void update_material_lod();
    void update_material_statistics();
};

/**
 * @brief Advanced Rendering System
 * 
 * High-performance rendering system with shader-aware optimizations.
 */
class AdvancedRenderingSystem : public System {
public:
    struct RenderingConfig {
        bool enable_frustum_culling{true};
        bool enable_occlusion_culling{false};
        bool enable_gpu_driven_rendering{false};
        bool enable_temporal_upsampling{false};
        
        // Batching and sorting
        bool enable_draw_call_batching{true};
        bool sort_by_shader{true};
        bool sort_by_material{true};
        bool sort_by_depth{true};
        
        // LOD settings
        bool enable_automatic_lod{true};
        f32 lod_bias{0.0f};
        std::vector<f32> lod_distances{50.0f, 150.0f, 500.0f};
        
        // Performance monitoring
        bool enable_performance_tracking{true};
        bool enable_draw_call_debugging{false};
        
        // Educational features
        bool show_rendering_statistics{false};
        bool highlight_performance_issues{false};
    };
    
    explicit AdvancedRenderingSystem(World& world,
                                    shader_runtime::ShaderRuntimeManager* runtime_manager,
                                    shader_debugging::AdvancedShaderDebugger* debugger = nullptr,
                                    const RenderingConfig& config = RenderingConfig{})
        : System(world), runtime_manager_(runtime_manager), debugger_(debugger), config_(config) {}
    
    void initialize() override {
        require<MaterialComponent>();
        require<components::Transform>();
        require<components::RenderComponent>();
        
        // Initialize rendering pipeline
        setup_rendering_pipeline();
        
        // Initialize GPU resources
        initialize_gpu_resources();
        
        // Setup debug visualization if enabled
        if (debugger_ && config_.enable_performance_tracking) {
            setup_debug_visualization();
        }
    }
    
    void update(f32 delta_time) override {
        // Update frame data
        current_frame_++;
        frame_start_time_ = std::chrono::steady_clock::now();
        
        // Performance profiling
        if (debugger_) {
            debugger_->get_profiler()->begin_frame();
            debugger_->get_profiler()->begin_event("AdvancedRenderingSystem");
        }
        
        // Culling phase
        perform_culling();
        
        // LOD selection
        update_lod_selection();
        
        // Sort render queue
        sort_render_queue();
        
        // Batch render calls
        batch_render_calls();
        
        // Submit draw calls
        submit_draw_calls();
        
        // Update performance metrics
        update_performance_metrics(delta_time);
        
        if (debugger_) {
            debugger_->get_profiler()->end_event("AdvancedRenderingSystem");
            debugger_->get_profiler()->end_frame();
        }
    }
    
    // Rendering pipeline management
    void set_camera(Entity camera_entity) { camera_entity_ = camera_entity; }
    void add_light(Entity light_entity);
    void remove_light(Entity light_entity);
    
    // GPU-driven rendering
    void enable_gpu_driven_rendering(bool enable) { config_.enable_gpu_driven_rendering = enable; }
    void setup_indirect_rendering();
    
    // Educational and debugging
    void enable_wireframe_mode(bool enable) { wireframe_mode_ = enable; }
    void enable_shader_debugging(Entity entity, bool enable);
    std::string get_rendering_statistics() const;
    
    // Performance analysis
    struct RenderingPerformanceReport {
        u32 total_entities{0};
        u32 rendered_entities{0};
        u32 culled_entities{0};
        u32 draw_calls{0};
        u32 batched_draw_calls{0};
        f32 culling_time{0.0f};
        f32 sorting_time{0.0f};
        f32 rendering_time{0.0f};
        std::vector<std::string> bottlenecks;
    };
    
    RenderingPerformanceReport generate_performance_report() const;
    
private:
    shader_runtime::ShaderRuntimeManager* runtime_manager_;
    shader_debugging::AdvancedShaderDebugger* debugger_;
    RenderingConfig config_;
    
    // Rendering state
    Entity camera_entity_{};
    std::vector<Entity> light_entities_;
    std::vector<Entity> render_queue_;
    
    // Performance tracking
    u32 current_frame_{0};
    std::chrono::steady_clock::time_point frame_start_time_;
    RenderingPerformanceReport current_frame_stats_;
    
    // Debug state
    bool wireframe_mode_{false};
    std::unordered_set<Entity> debug_entities_;
    
    // GPU resources
    u32 uniform_buffer_id_{0};
    u32 instance_buffer_id_{0};
    
    void setup_rendering_pipeline();
    void initialize_gpu_resources();
    void setup_debug_visualization();
    void perform_culling();
    void update_lod_selection();
    void sort_render_queue();
    void batch_render_calls();
    void submit_draw_calls();
    void update_performance_metrics(f32 delta_time);
};

/**
 * @brief Lighting System
 * 
 * Advanced lighting system with shader-based lighting calculations.
 */
class AdvancedLightingSystem : public System {
public:
    struct LightingConfig {
        u32 max_directional_lights{4};
        u32 max_point_lights{32};
        u32 max_spot_lights{16};
        
        // Shadow mapping
        bool enable_shadows{true};
        u32 shadow_map_size{2048};
        bool use_cascade_shadows{true};
        u32 cascade_count{4};
        
        // Advanced lighting
        bool enable_volumetric_lighting{false};
        bool enable_light_scattering{false};
        bool enable_ssr{false}; // Screen space reflections
        bool enable_gi{false};  // Global illumination
        
        // Performance
        bool enable_light_culling{true};
        f32 light_cull_distance{100.0f};
        bool use_clustered_deferred{false};
    };
    
    explicit AdvancedLightingSystem(World& world,
                                   shader_runtime::ShaderRuntimeManager* runtime_manager,
                                   const LightingConfig& config = LightingConfig{})
        : System(world), runtime_manager_(runtime_manager), config_(config) {}
    
    void initialize() override {
        require<AdvancedLightComponent>();
        
        // Initialize shadow mapping
        if (config_.enable_shadows) {
            setup_shadow_mapping();
        }
        
        // Setup lighting uniform buffers
        setup_lighting_uniforms();
        
        // Initialize advanced lighting features
        if (config_.enable_volumetric_lighting) {
            setup_volumetric_lighting();
        }
    }
    
    void update(f32 delta_time) override {
        // Update light animations
        update_animated_lights(delta_time);
        
        // Perform light culling
        if (config_.enable_light_culling) {
            perform_light_culling();
        }
        
        // Update shadow maps
        if (config_.enable_shadows) {
            update_shadow_maps();
        }
        
        // Update lighting uniforms
        update_lighting_uniforms();
        
        // Update volumetric effects
        if (config_.enable_volumetric_lighting) {
            update_volumetric_lighting(delta_time);
        }
    }
    
    // Light management
    void add_light(Entity entity, const AdvancedLightComponent& light);
    void remove_light(Entity entity);
    void update_light(Entity entity, const AdvancedLightComponent& light);
    
    // Shadow mapping
    void enable_shadows_for_light(Entity entity, bool enable = true);
    void set_shadow_quality(Entity entity, u32 shadow_map_size);
    
    // Advanced features
    void enable_volumetric_lighting_for_light(Entity entity, bool enable = true);
    void set_volumetric_parameters(Entity entity, f32 density, f32 scattering);
    
    // Educational features
    void visualize_light_volumes(bool enable = true) { visualize_light_volumes_ = enable; }
    void show_shadow_cascades(bool enable = true) { show_shadow_cascades_ = enable; }
    std::string get_lighting_explanation() const;
    
private:
    shader_runtime::ShaderRuntimeManager* runtime_manager_;
    LightingConfig config_;
    
    // Light tracking
    std::vector<Entity> directional_lights_;
    std::vector<Entity> point_lights_;
    std::vector<Entity> spot_lights_;
    
    // Shadow mapping resources
    std::vector<u32> shadow_maps_;
    std::vector<u32> cascade_shadow_maps_;
    
    // Uniform buffers
    u32 lights_uniform_buffer_{0};
    u32 shadow_uniform_buffer_{0};
    
    // Educational visualization
    bool visualize_light_volumes_{false};
    bool show_shadow_cascades_{false};
    
    void setup_shadow_mapping();
    void setup_lighting_uniforms();
    void setup_volumetric_lighting();
    void update_animated_lights(f32 delta_time);
    void perform_light_culling();
    void update_shadow_maps();
    void update_lighting_uniforms();
    void update_volumetric_lighting(f32 delta_time);
};

//=============================================================================
// Educational and Debug Systems
//=============================================================================

/**
 * @brief Shader Education System
 * 
 * Provides educational features for learning shader programming.
 */
class ShaderEducationSystem : public System {
public:
    explicit ShaderEducationSystem(World& world,
                                  shader_runtime::ShaderRuntimeManager* runtime_manager,
                                  shader_library::AdvancedShaderLibrary* library,
                                  visual_editor::VisualShaderEditor* editor = nullptr)
        : System(world), runtime_manager_(runtime_manager), 
          shader_library_(library), visual_editor_(editor) {}
    
    void initialize() override {
        // Setup educational content
        setup_tutorial_materials();
        
        // Create demonstration entities
        create_demo_entities();
        
        // Initialize interactive examples
        setup_interactive_examples();
    }
    
    void update(f32 delta_time) override {
        // Update tutorial progression
        update_tutorial_state(delta_time);
        
        // Update interactive examples
        update_interactive_examples(delta_time);
        
        // Handle user interactions
        process_educational_input();
    }
    
    // Tutorial management
    void start_tutorial(const std::string& tutorial_name);
    void next_tutorial_step();
    void previous_tutorial_step();
    std::vector<std::string> get_available_tutorials() const;
    
    // Interactive examples
    void create_lighting_demo();
    void create_material_demo();
    void create_post_processing_demo();
    void create_animation_demo();
    
    // Educational components
    struct TutorialComponent {
        std::string tutorial_name;
        u32 current_step{0};
        std::vector<std::string> steps;
        std::vector<std::string> explanations;
        bool is_interactive{false};
        f32 step_duration{5.0f};
        f32 current_step_time{0.0f};
    };
    
    struct InteractiveExampleComponent {
        std::string example_type;
        std::unordered_map<std::string, f32> adjustable_parameters;
        std::function<void(f32)> parameter_callback;
        bool show_parameter_ui{true};
    };
    
    // Analysis and feedback
    std::string analyze_shader_for_education(shader_runtime::ShaderRuntimeManager::ShaderHandle handle) const;
    std::vector<std::string> suggest_learning_improvements(Entity entity) const;
    
private:
    shader_runtime::ShaderRuntimeManager* runtime_manager_;
    shader_library::AdvancedShaderLibrary* shader_library_;
    visual_editor::VisualShaderEditor* visual_editor_;
    
    // Tutorial state
    std::string current_tutorial_;
    u32 current_tutorial_step_{0};
    f32 tutorial_timer_{0.0f};
    
    // Demo entities
    std::vector<Entity> demo_entities_;
    
    void setup_tutorial_materials();
    void create_demo_entities();
    void setup_interactive_examples();
    void update_tutorial_state(f32 delta_time);
    void update_interactive_examples(f32 delta_time);
    void process_educational_input();
};

//=============================================================================
// Integration Utilities and Helpers
//=============================================================================

namespace utils {
    // Component creation helpers
    MaterialComponent create_standard_material(const std::array<f32, 3>& albedo = {0.8f, 0.8f, 0.8f},
                                              f32 metallic = 0.0f, f32 roughness = 0.5f);
    
    MaterialComponent create_metallic_material(const std::array<f32, 3>& albedo = {0.7f, 0.7f, 0.8f},
                                             f32 roughness = 0.1f);
    
    MaterialComponent create_glass_material(const std::array<f32, 3>& color = {0.9f, 0.9f, 0.9f},
                                          f32 transmission = 0.8f, f32 ior = 1.5f);
    
    MaterialComponent create_emissive_material(const std::array<f32, 3>& emissive_color = {1.0f, 0.5f, 0.0f},
                                             f32 intensity = 2.0f);
    
    // Shader binding helpers
    void bind_material_uniforms(const MaterialComponent& material,
                               shader_runtime::ShaderRuntimeManager::ShaderHandle handle);
    
    void bind_lighting_uniforms(const std::vector<Entity>& lights,
                               shader_runtime::ShaderRuntimeManager::ShaderHandle handle);
    
    // Performance utilities
    f32 calculate_shader_complexity_score(shader_runtime::ShaderRuntimeManager::ShaderHandle handle);
    std::vector<std::string> analyze_material_performance(const MaterialComponent& material);
    
    // Educational utilities
    std::string generate_shader_explanation(shader_runtime::ShaderRuntimeManager::ShaderHandle handle);
    std::vector<std::string> get_shader_learning_objectives(shader_runtime::ShaderRuntimeManager::ShaderHandle handle);
    std::string format_material_properties(const MaterialComponent& material);
    
    // Conversion utilities
    shader_library::PBRMaterial component_to_pbr_material(const MaterialComponent& material);
    MaterialComponent pbr_material_to_component(const shader_library::PBRMaterial& pbr_material,
                                               const std::string& name = "");
    
    // LOD utilities
    f32 calculate_lod_distance(Entity entity, Entity camera);
    u32 select_lod_level(f32 distance, const std::vector<f32>& lod_distances);
    shader_runtime::ShaderRuntimeManager::ShaderHandle select_lod_shader(const MaterialComponent& material, u32 lod_level);
    
    // Batch rendering utilities
    struct BatchKey {
        shader_runtime::ShaderRuntimeManager::ShaderHandle shader_handle;
        u32 material_id;
        bool is_transparent;
        
        bool operator==(const BatchKey& other) const {
            return shader_handle == other.shader_handle &&
                   material_id == other.material_id &&
                   is_transparent == other.is_transparent;
        }
    };
    
    struct BatchKeyHash {
        usize operator()(const BatchKey& key) const {
            return std::hash<u32>{}(key.shader_handle) ^
                   std::hash<u32>{}(key.material_id) ^
                   std::hash<bool>{}(key.is_transparent);
        }
    };
    
    std::unordered_map<BatchKey, std::vector<Entity>, BatchKeyHash> 
    group_entities_for_batching(const std::vector<Entity>& entities, World& world);
    
    // Debug visualization
    void draw_material_debug_info(Entity entity, World& world);
    void draw_shader_complexity_heatmap(const std::vector<Entity>& entities, World& world);
    void draw_performance_overlay(const AdvancedRenderingSystem::RenderingPerformanceReport& report);
}

} // namespace ecscope::renderer::ecs_integration