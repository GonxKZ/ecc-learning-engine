#pragma once

/**
 * @file advanced_shader_library.hpp
 * @brief Comprehensive Shader Library with PBR, Lighting, and Effects for ECScope
 * 
 * This system provides a complete collection of production-ready shaders including:
 * - Physically Based Rendering (PBR) materials and lighting
 * - Advanced lighting models (Blinn-Phong, Cook-Torrance, etc.)
 * - Post-processing effects and filters
 * - Particle and procedural effects
 * - Deferred and forward rendering pipelines
 * - Educational shader demonstrations
 * - Performance-optimized variants
 * - Cross-platform compatibility
 * 
 * @author ECScope Educational ECS Framework - Advanced Shader System
 * @date 2024
 */

#include "shader_runtime_system.hpp"
#include "visual_shader_editor.hpp"
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <functional>
#include <array>

namespace ecscope::renderer::shader_library {

//=============================================================================
// Shader Categories and Types
//=============================================================================

enum class ShaderCategory : u8 {
    // Rendering pipelines
    Forward = 0,            ///< Forward rendering shaders
    Deferred,               ///< Deferred rendering shaders
    PBR,                    ///< Physically based rendering
    Unlit,                  ///< Unlit/simple shaders
    
    // Lighting models
    Phong,                  ///< Phong lighting model
    BlinnPhong,            ///< Blinn-Phong lighting model
    CookTorrance,          ///< Cook-Torrance BRDF
    OrenNayar,             ///< Oren-Nayar diffuse model
    Lambert,               ///< Lambert diffuse model
    
    // Effects
    PostProcessing,        ///< Post-processing effects
    Particles,             ///< Particle system shaders
    Volumetric,            ///< Volumetric effects
    Procedural,            ///< Procedural generation
    Noise,                 ///< Noise-based effects
    
    // Specialized
    Terrain,               ///< Terrain rendering
    Water,                 ///< Water simulation
    Sky,                   ///< Sky and atmosphere
    UI,                    ///< User interface rendering
    Debug,                 ///< Debug visualization
    
    // Educational
    Tutorial,              ///< Educational tutorials
    Demonstration,         ///< Feature demonstrations
    Benchmark,             ///< Performance benchmarks
    
    Custom                 ///< User-defined shaders
};

enum class LightingModel : u8 {
    Unlit = 0,
    Lambert,
    Phong,
    BlinnPhong,
    PBR_MetallicRoughness,
    PBR_SpecularGlossiness,
    CookTorrance,
    OrenNayar,
    Toon,
    Custom
};

enum class MaterialType : u8 {
    Standard = 0,           ///< Standard PBR material
    Metallic,               ///< Metallic materials
    Dielectric,             ///< Non-metallic materials
    Glass,                  ///< Glass/transparent materials
    Emissive,               ///< Emissive/glowing materials
    Subsurface,             ///< Subsurface scattering
    Cloth,                  ///< Fabric materials
    Skin,                   ///< Skin rendering
    Car_Paint,              ///< Automotive paint
    Plastic,                ///< Plastic materials
    Wood,                   ///< Wood materials
    Stone,                  ///< Stone/concrete materials
    Custom
};

//=============================================================================
// Shader Template System
//=============================================================================

struct ShaderTemplate {
    std::string name;
    std::string description;
    ShaderCategory category;
    std::string author{"ECScope"};
    std::string version{"1.0"};
    
    // Source code templates
    std::string vertex_template;
    std::string fragment_template;
    std::string geometry_template;
    std::string compute_template;
    
    // Template parameters
    struct Parameter {
        std::string name;
        std::string display_name;
        std::string description;
        visual_editor::DataType type;
        visual_editor::ShaderValue default_value;
        std::vector<std::string> allowed_values; // For enum-like parameters
        f32 min_value{0.0f}, max_value{1.0f};    // For numeric ranges
        bool is_required{true};
        std::string tooltip;
        
        Parameter() = default;
        Parameter(std::string n, visual_editor::DataType t, visual_editor::ShaderValue def_val)
            : name(std::move(n)), display_name(name), type(t), default_value(def_val) {}
    };
    
    std::vector<Parameter> parameters;
    std::vector<std::string> required_extensions;
    std::vector<std::string> defines;
    
    // Educational metadata
    std::string difficulty_level{"Beginner"};
    std::vector<std::string> learning_objectives;
    std::vector<std::string> prerequisites;
    std::string tutorial_text;
    bool is_educational{false};
    
    // Performance characteristics
    f32 estimated_performance_cost{1.0f}; // Relative to simple shader
    std::vector<std::string> performance_notes;
    bool supports_mobile{true};
    
    // Instantiation
    std::string instantiate(const std::unordered_map<std::string, std::string>& parameter_values) const;
    std::string get_parameter_declaration(const Parameter& param) const;
    bool validate_parameters(const std::unordered_map<std::string, std::string>& values) const;
};

//=============================================================================
// PBR Material System
//=============================================================================

struct PBRMaterial {
    // Base properties
    std::array<f32, 3> albedo{0.5f, 0.5f, 0.5f};      ///< Base color/albedo
    f32 metallic{0.0f};                                ///< Metallic factor (0-1)
    f32 roughness{0.5f};                              ///< Roughness factor (0-1)
    f32 ao{1.0f};                                     ///< Ambient occlusion (0-1)
    std::array<f32, 3> emissive{0.0f, 0.0f, 0.0f};   ///< Emissive color
    f32 emissive_strength{0.0f};                      ///< Emissive intensity
    
    // Advanced properties
    f32 ior{1.5f};                                    ///< Index of refraction
    f32 transmission{0.0f};                           ///< Transmission factor
    f32 thickness{0.0f};                              ///< Thickness for transmission
    std::array<f32, 3> absorption{1.0f, 1.0f, 1.0f}; ///< Absorption color
    f32 clearcoat{0.0f};                              ///< Clear coat factor
    f32 clearcoat_roughness{0.0f};                    ///< Clear coat roughness
    std::array<f32, 3> clearcoat_normal{0.0f, 0.0f, 1.0f}; ///< Clear coat normal
    
    // Subsurface scattering
    f32 subsurface{0.0f};                             ///< Subsurface factor
    std::array<f32, 3> subsurface_color{1.0f, 1.0f, 1.0f}; ///< Subsurface color
    f32 subsurface_radius{1.0f};                      ///< Subsurface radius
    
    // Anisotropy
    f32 anisotropy{0.0f};                             ///< Anisotropy factor
    f32 anisotropy_rotation{0.0f};                    ///< Anisotropy rotation
    
    // Texture references (paths or texture IDs)
    std::string albedo_texture;
    std::string normal_texture;
    std::string metallic_roughness_texture;
    std::string ao_texture;
    std::string emissive_texture;
    std::string height_texture;
    std::string opacity_texture;
    
    // Material flags
    bool double_sided{false};
    bool alpha_test{false};
    f32 alpha_cutoff{0.5f};
    bool cast_shadows{true};
    bool receive_shadows{true};
    
    MaterialType get_material_type() const {
        if (transmission > 0.1f) return MaterialType::Glass;
        if (emissive_strength > 0.1f) return MaterialType::Emissive;
        if (metallic > 0.9f) return MaterialType::Metallic;
        if (subsurface > 0.1f) return MaterialType::Subsurface;
        return MaterialType::Standard;
    }
    
    std::string generate_shader_defines() const {
        std::vector<std::string> defines;
        
        if (!albedo_texture.empty()) defines.push_back("USE_ALBEDO_TEXTURE");
        if (!normal_texture.empty()) defines.push_back("USE_NORMAL_TEXTURE");
        if (!metallic_roughness_texture.empty()) defines.push_back("USE_METALLIC_ROUGHNESS_TEXTURE");
        if (!ao_texture.empty()) defines.push_back("USE_AO_TEXTURE");
        if (!emissive_texture.empty()) defines.push_back("USE_EMISSIVE_TEXTURE");
        if (!height_texture.empty()) defines.push_back("USE_HEIGHT_TEXTURE");
        
        if (transmission > 0.0f) defines.push_back("USE_TRANSMISSION");
        if (clearcoat > 0.0f) defines.push_back("USE_CLEARCOAT");
        if (subsurface > 0.0f) defines.push_back("USE_SUBSURFACE");
        if (anisotropy != 0.0f) defines.push_back("USE_ANISOTROPY");
        
        if (alpha_test) defines.push_back("ALPHA_TEST");
        if (double_sided) defines.push_back("DOUBLE_SIDED");
        
        std::string result;
        for (const auto& define : defines) {
            result += "#define " + define + "\n";
        }
        return result;
    }
};

//=============================================================================
// Lighting System
//=============================================================================

struct Light {
    enum class Type {
        Directional = 0,
        Point,
        Spot,
        Area,
        IBL,
        Sky
    };
    
    Type type{Type::Directional};
    std::array<f32, 3> position{0.0f, 0.0f, 0.0f};
    std::array<f32, 3> direction{0.0f, -1.0f, 0.0f};
    std::array<f32, 3> color{1.0f, 1.0f, 1.0f};
    f32 intensity{1.0f};
    
    // Point/Spot light attenuation
    f32 range{10.0f};
    f32 constant_attenuation{1.0f};
    f32 linear_attenuation{0.09f};
    f32 quadratic_attenuation{0.032f};
    
    // Spot light properties
    f32 inner_cone_angle{30.0f};
    f32 outer_cone_angle{45.0f};
    
    // Area light properties
    std::array<f32, 2> area_size{1.0f, 1.0f};
    
    // Shadow properties
    bool cast_shadows{true};
    f32 shadow_bias{0.0001f};
    f32 shadow_normal_bias{0.1f};
    i32 shadow_cascade_count{4};
    
    std::string generate_light_struct() const {
        return R"(
struct Light {
    vec3 position;
    vec3 direction;
    vec3 color;
    float intensity;
    float range;
    float innerCone;
    float outerCone;
    int type;
};
)";
    }
};

//=============================================================================
// Shader Library Manager
//=============================================================================

class AdvancedShaderLibrary {
public:
    explicit AdvancedShaderLibrary(shader_runtime::ShaderRuntimeManager* runtime_manager);
    ~AdvancedShaderLibrary() = default;
    
    // Template management
    void register_template(const ShaderTemplate& shader_template);
    void register_builtin_templates();
    
    std::vector<std::string> get_template_names(ShaderCategory category = ShaderCategory::Custom) const;
    const ShaderTemplate* get_template(const std::string& name) const;
    std::vector<ShaderCategory> get_available_categories() const;
    
    // Shader creation from templates
    shader_runtime::ShaderRuntimeManager::ShaderHandle create_shader_from_template(
        const std::string& template_name,
        const std::unordered_map<std::string, std::string>& parameters,
        const std::string& instance_name = "");
    
    // PBR shader creation
    shader_runtime::ShaderRuntimeManager::ShaderHandle create_pbr_shader(
        const PBRMaterial& material,
        LightingModel lighting_model = LightingModel::PBR_MetallicRoughness,
        const std::string& name = "");
    
    shader_runtime::ShaderRuntimeManager::ShaderHandle create_pbr_shader_variant(
        const PBRMaterial& material,
        const std::vector<Light>& lights,
        const std::string& name = "");
    
    // Post-processing effects
    struct PostProcessingChain {
        std::vector<std::string> effects;
        std::unordered_map<std::string, std::unordered_map<std::string, std::string>> parameters;
        
        void add_effect(const std::string& effect_name,
                       const std::unordered_map<std::string, std::string>& params = {}) {
            effects.push_back(effect_name);
            parameters[effect_name] = params;
        }
    };
    
    shader_runtime::ShaderRuntimeManager::ShaderHandle create_post_processing_chain(
        const PostProcessingChain& chain,
        const std::string& name = "");
    
    // Particle system shaders
    struct ParticleSystemConfig {
        bool use_gpu_particles{true};
        bool use_instancing{true};
        bool use_billboard{true};
        bool use_soft_particles{false};
        bool use_lighting{false};
        u32 max_particles{1000};
        std::string texture_atlas;
        f32 size_variance{0.1f};
        std::array<f32, 3> gravity{0.0f, -9.81f, 0.0f};
    };
    
    shader_runtime::ShaderRuntimeManager::ShaderHandle create_particle_shader(
        const ParticleSystemConfig& config,
        const std::string& name = "");
    
    // Terrain shaders
    struct TerrainConfig {
        u32 texture_layers{4};
        bool use_triplanar_mapping{false};
        bool use_height_blending{true};
        bool use_normal_mapping{true};
        bool use_parallax_mapping{false};
        f32 tile_scale{1.0f};
        std::vector<std::string> layer_textures;
    };
    
    shader_runtime::ShaderRuntimeManager::ShaderHandle create_terrain_shader(
        const TerrainConfig& config,
        const std::string& name = "");
    
    // Water shaders
    struct WaterConfig {
        bool use_reflection{true};
        bool use_refraction{true};
        bool use_foam{true};
        bool use_caustics{false};
        f32 wave_amplitude{0.1f};
        f32 wave_frequency{1.0f};
        f32 wave_speed{1.0f};
        std::array<f32, 3> water_color{0.0f, 0.3f, 0.5f};
        f32 transparency{0.8f};
    };
    
    shader_runtime::ShaderRuntimeManager::ShaderHandle create_water_shader(
        const WaterConfig& config,
        const std::string& name = "");
    
    // Utility and helper shaders
    shader_runtime::ShaderRuntimeManager::ShaderHandle create_debug_shader(
        const std::string& debug_mode, // "normals", "uvs", "wireframe", etc.
        const std::string& name = "");
    
    shader_runtime::ShaderRuntimeManager::ShaderHandle create_skybox_shader(
        const std::string& skybox_type = "cubemap", // "cubemap", "procedural", "gradient"
        const std::string& name = "");
    
    // Educational shaders
    std::vector<std::string> get_tutorial_shader_names() const;
    shader_runtime::ShaderRuntimeManager::ShaderHandle create_tutorial_shader(
        const std::string& tutorial_name,
        const std::string& name = "");
    
    std::string get_tutorial_explanation(const std::string& tutorial_name) const;
    std::vector<std::string> get_learning_objectives(const std::string& tutorial_name) const;
    
    // Shader analysis and optimization
    struct ShaderAnalysis {
        f32 complexity_score{0.0f};          // 0-100, higher = more complex
        f32 performance_rating{100.0f};      // 0-100, higher = better performance
        u32 instruction_count{0};            // Estimated instruction count
        u32 texture_samples{0};              // Number of texture samples
        u32 math_operations{0};              // Number of math operations
        
        std::vector<std::string> performance_warnings;
        std::vector<std::string> optimization_suggestions;
        std::vector<std::string> mobile_compatibility_notes;
        
        bool is_mobile_friendly() const {
            return performance_rating > 60.0f && texture_samples <= 4 && complexity_score < 50.0f;
        }
    };
    
    ShaderAnalysis analyze_shader_template(const std::string& template_name,
                                         const std::unordered_map<std::string, std::string>& parameters = {}) const;
    
    std::vector<std::string> suggest_optimizations(const std::string& template_name,
                                                  const std::unordered_map<std::string, std::string>& parameters = {}) const;
    
    // Shader variants and LOD
    struct ShaderLODConfig {
        f32 distance_near{10.0f};
        f32 distance_medium{50.0f};
        f32 distance_far{200.0f};
        
        std::string high_quality_template;
        std::string medium_quality_template;
        std::string low_quality_template;
        
        std::unordered_map<std::string, std::string> lod_parameters;
    };
    
    std::vector<shader_runtime::ShaderRuntimeManager::ShaderHandle> create_lod_variants(
        const ShaderLODConfig& lod_config,
        const std::string& base_name = "");
    
    // Cross-platform compatibility
    struct PlatformCompatibility {
        bool supports_opengl{true};
        bool supports_vulkan{true};
        bool supports_directx{true};
        bool supports_metal{false};
        bool supports_webgl{true};
        bool supports_mobile{true};
        
        std::vector<std::string> required_extensions;
        std::vector<std::string> fallback_shaders;
    };
    
    PlatformCompatibility check_template_compatibility(const std::string& template_name) const;
    std::string get_fallback_shader(const std::string& template_name, const std::string& platform) const;
    
    // Shader library statistics
    struct LibraryStatistics {
        u32 total_templates{0};
        u32 templates_by_category[static_cast<u32>(ShaderCategory::Custom) + 1]{};
        u32 educational_templates{0};
        u32 created_shaders{0};
        u32 pbr_shaders{0};
        u32 post_processing_chains{0};
        f32 average_complexity{0.0f};
        std::vector<std::string> most_popular_templates;
    };
    
    LibraryStatistics get_library_statistics() const;
    
    // Integration with visual editor
    void register_visual_editor(visual_editor::VisualShaderEditor* editor) {
        visual_editor_ = editor;
    }
    
    std::vector<visual_editor::VisualShaderNode*> create_template_nodes(const std::string& template_name) const;
    
private:
    shader_runtime::ShaderRuntimeManager* runtime_manager_;
    visual_editor::VisualShaderEditor* visual_editor_{nullptr};
    
    std::unordered_map<std::string, ShaderTemplate> templates_;
    std::unordered_map<ShaderCategory, std::vector<std::string>> templates_by_category_;
    
    mutable LibraryStatistics stats_;
    
    // Built-in template creation methods
    void create_pbr_templates();
    void create_lighting_templates();
    void create_post_processing_templates();
    void create_particle_templates();
    void create_terrain_templates();
    void create_water_templates();
    void create_debug_templates();
    void create_tutorial_templates();
    
    // Helper methods
    std::string generate_pbr_fragment_shader(const PBRMaterial& material, 
                                            LightingModel lighting_model,
                                            const std::vector<Light>& lights = {}) const;
    
    std::string generate_lighting_calculations(LightingModel model, 
                                             const std::vector<Light>& lights) const;
    
    std::string generate_brdf_functions(LightingModel model) const;
    
    ShaderAnalysis analyze_shader_source(const std::string& source) const;
    
    void update_statistics();
};

//=============================================================================
// Built-in Shader Source Code
//=============================================================================

namespace builtin_shaders {
    // PBR shader components
    extern const char* PBR_VERTEX_SHADER;
    extern const char* PBR_FRAGMENT_SHADER;
    extern const char* PBR_FUNCTIONS;
    extern const char* LIGHTING_FUNCTIONS;
    
    // Post-processing effects
    extern const char* TONE_MAPPING_SHADER;
    extern const char* BLOOM_SHADER;
    extern const char* SSAO_SHADER;
    extern const char* FXAA_SHADER;
    extern const char* COLOR_GRADING_SHADER;
    
    // Particle system shaders
    extern const char* PARTICLE_VERTEX_SHADER;
    extern const char* PARTICLE_FRAGMENT_SHADER;
    extern const char* PARTICLE_COMPUTE_SHADER;
    
    // Terrain shaders
    extern const char* TERRAIN_VERTEX_SHADER;
    extern const char* TERRAIN_FRAGMENT_SHADER;
    extern const char* TERRAIN_TESSELLATION_SHADER;
    
    // Water shaders
    extern const char* WATER_VERTEX_SHADER;
    extern const char* WATER_FRAGMENT_SHADER;
    extern const char* WATER_WAVE_FUNCTIONS;
    
    // Utility shaders
    extern const char* DEBUG_NORMALS_SHADER;
    extern const char* DEBUG_UVS_SHADER;
    extern const char* WIREFRAME_SHADER;
    extern const char* SKYBOX_SHADER;
    
    // Educational/tutorial shaders
    extern const char* TUTORIAL_BASIC_LIGHTING;
    extern const char* TUTORIAL_TEXTURE_SAMPLING;
    extern const char* TUTORIAL_NORMAL_MAPPING;
    extern const char* TUTORIAL_PARALLAX_MAPPING;
    extern const char* TUTORIAL_SHADOW_MAPPING;
}

//=============================================================================
// Shader Preset System
//=============================================================================

class ShaderPresetManager {
public:
    struct MaterialPreset {
        std::string name;
        std::string description;
        MaterialType type;
        PBRMaterial material;
        std::string preview_image_path;
        std::vector<std::string> tags;
        
        MaterialPreset() = default;
        MaterialPreset(std::string n, MaterialType t, PBRMaterial mat)
            : name(std::move(n)), type(t), material(std::move(mat)) {}
    };
    
    void register_preset(const MaterialPreset& preset);
    void load_presets_from_file(const std::string& file_path);
    void save_presets_to_file(const std::string& file_path) const;
    
    std::vector<std::string> get_preset_names(MaterialType type = MaterialType::Standard) const;
    const MaterialPreset* get_preset(const std::string& name) const;
    
    std::vector<MaterialPreset> search_presets(const std::vector<std::string>& tags) const;
    std::vector<MaterialPreset> get_presets_by_type(MaterialType type) const;
    
    // Create common material presets
    void create_builtin_presets();
    
private:
    std::unordered_map<std::string, MaterialPreset> presets_;
    std::unordered_map<MaterialType, std::vector<std::string>> presets_by_type_;
};

} // namespace ecscope::renderer::shader_library