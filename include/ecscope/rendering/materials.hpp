/**
 * @file materials.hpp
 * @brief Professional PBR Material System
 * 
 * Complete physically-based rendering material system with
 * texture management, shader compilation, and material templates.
 * 
 * @author ECScope Development Team
 * @version 1.0.0
 */

#pragma once

#include "renderer.hpp"
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include <array>
#include <functional>
#include <filesystem>
#include <atomic>
#include <mutex>

namespace ecscope::rendering {

// =============================================================================
// MATERIAL TYPES & ENUMERATIONS
// =============================================================================

/**
 * @brief Material blend modes
 */
enum class MaterialBlendMode : uint8_t {
    Opaque,              ///< Fully opaque materials (no blending)
    Masked,              ///< Alpha testing with cutoff
    Transparent,         ///< Standard alpha blending
    Additive,           ///< Additive blending for effects
    Multiply,           ///< Multiplicative blending
    Screen,             ///< Screen blending
    Overlay,            ///< Overlay blending
    SoftAdditive        ///< Soft additive for particles
};

/**
 * @brief Material shading models
 */
enum class ShadingModel : uint8_t {
    DefaultLit,         ///< Standard PBR metallic-roughness
    Unlit,             ///< No lighting calculations
    Subsurface,        ///< Subsurface scattering
    PreintegratedSkin, ///< Optimized skin rendering
    ClearCoat,         ///< Car paint, lacquered wood
    Cloth,             ///< Fabric materials with anisotropy
    Eye,               ///< Specialized eye rendering
    Hair,              ///< Hair/fur rendering
    TwoSidedFoliage,   ///< Leaves and thin materials
    Water              ///< Water surface rendering
};

/**
 * @brief Material parameter types
 */
enum class MaterialParameterType : uint8_t {
    Float,
    Float2,
    Float3,
    Float4,
    Int,
    Int2,
    Int3,
    Int4,
    Bool,
    Texture2D,
    TextureCube,
    Matrix3,
    Matrix4
};

/**
 * @brief Texture usage in materials
 */
enum class TextureUsage : uint8_t {
    Albedo,             ///< Base color / diffuse
    Normal,             ///< Normal map (tangent space)
    MetallicRoughness,  ///< R: unused, G: roughness, B: metallic
    Occlusion,          ///< Ambient occlusion
    Emission,           ///< Emissive color
    Height,             ///< Height map for parallax
    Opacity,            ///< Alpha/transparency mask
    Subsurface,         ///< Subsurface scattering mask
    Transmission,       ///< Light transmission
    ClearCoat,          ///< Clear coat layer
    ClearCoatRoughness, ///< Clear coat roughness
    ClearCoatNormal,    ///< Clear coat normal
    Anisotropy,         ///< Anisotropy direction and strength
    Custom0,            ///< Custom texture slot 0
    Custom1,            ///< Custom texture slot 1
    Custom2,            ///< Custom texture slot 2
    Custom3             ///< Custom texture slot 3
};

// =============================================================================
// MATERIAL PARAMETER SYSTEM
// =============================================================================

/**
 * @brief Material parameter value variant
 */
class MaterialParameter {
public:
    MaterialParameter() = default;
    MaterialParameter(float value) : type_(MaterialParameterType::Float), float_value_(value) {}
    MaterialParameter(const std::array<float, 2>& value) : type_(MaterialParameterType::Float2), float2_value_(value) {}
    MaterialParameter(const std::array<float, 3>& value) : type_(MaterialParameterType::Float3), float3_value_(value) {}
    MaterialParameter(const std::array<float, 4>& value) : type_(MaterialParameterType::Float4), float4_value_(value) {}
    MaterialParameter(int32_t value) : type_(MaterialParameterType::Int), int_value_(value) {}
    MaterialParameter(const std::array<int32_t, 2>& value) : type_(MaterialParameterType::Int2), int2_value_(value) {}
    MaterialParameter(const std::array<int32_t, 3>& value) : type_(MaterialParameterType::Int3), int3_value_(value) {}
    MaterialParameter(const std::array<int32_t, 4>& value) : type_(MaterialParameterType::Int4), int4_value_(value) {}
    MaterialParameter(bool value) : type_(MaterialParameterType::Bool), bool_value_(value) {}
    MaterialParameter(TextureHandle value) : type_(MaterialParameterType::Texture2D), texture_value_(value) {}

    MaterialParameterType get_type() const { return type_; }

    float as_float() const { return float_value_; }
    std::array<float, 2> as_float2() const { return float2_value_; }
    std::array<float, 3> as_float3() const { return float3_value_; }
    std::array<float, 4> as_float4() const { return float4_value_; }
    int32_t as_int() const { return int_value_; }
    std::array<int32_t, 2> as_int2() const { return int2_value_; }
    std::array<int32_t, 3> as_int3() const { return int3_value_; }
    std::array<int32_t, 4> as_int4() const { return int4_value_; }
    bool as_bool() const { return bool_value_; }
    TextureHandle as_texture() const { return texture_value_; }

private:
    MaterialParameterType type_ = MaterialParameterType::Float;
    
    union {
        float float_value_ = 0.0f;
        std::array<float, 2> float2_value_;
        std::array<float, 3> float3_value_;
        std::array<float, 4> float4_value_;
        int32_t int_value_;
        std::array<int32_t, 2> int2_value_;
        std::array<int32_t, 3> int3_value_;
        std::array<int32_t, 4> int4_value_;
        bool bool_value_;
        TextureHandle texture_value_;
    };
};

/**
 * @brief Material parameter descriptor
 */
struct MaterialParameterDesc {
    std::string name;
    MaterialParameterType type;
    MaterialParameter default_value;
    std::string display_name;
    std::string description;
    MaterialParameter min_value;
    MaterialParameter max_value;
    bool is_texture = false;
    TextureUsage texture_usage = TextureUsage::Custom0;
};

// =============================================================================
// MATERIAL DEFINITION
// =============================================================================

/**
 * @brief Complete material definition
 */
class Material {
public:
    Material(const std::string& name = "DefaultMaterial");
    ~Material() = default;

    // Basic properties
    const std::string& get_name() const { return name_; }
    void set_name(const std::string& name) { name_ = name; }

    ShadingModel get_shading_model() const { return shading_model_; }
    void set_shading_model(ShadingModel model);

    MaterialBlendMode get_blend_mode() const { return blend_mode_; }
    void set_blend_mode(MaterialBlendMode mode);

    bool is_two_sided() const { return two_sided_; }
    void set_two_sided(bool two_sided) { two_sided_ = two_sided; }

    float get_opacity_cutoff() const { return opacity_cutoff_; }
    void set_opacity_cutoff(float cutoff) { opacity_cutoff_ = cutoff; }

    // Parameter management
    void set_parameter(const std::string& name, const MaterialParameter& value);
    MaterialParameter get_parameter(const std::string& name) const;
    bool has_parameter(const std::string& name) const;
    void remove_parameter(const std::string& name);

    // Texture shortcuts
    void set_texture(TextureUsage usage, TextureHandle texture);
    TextureHandle get_texture(TextureUsage usage) const;
    bool has_texture(TextureUsage usage) const;

    // Common PBR parameters (convenience functions)
    void set_albedo(const std::array<float, 3>& color);
    void set_metallic(float metallic);
    void set_roughness(float roughness);
    void set_normal_intensity(float intensity);
    void set_emission(const std::array<float, 3>& color, float intensity = 1.0f);
    void set_subsurface_scattering(float strength);

    std::array<float, 3> get_albedo() const;
    float get_metallic() const;
    float get_roughness() const;
    float get_normal_intensity() const;
    std::array<float, 3> get_emission() const;
    float get_emission_intensity() const;
    float get_subsurface_scattering() const;

    // Shader compilation
    std::string generate_vertex_shader() const;
    std::string generate_fragment_shader() const;
    
    // Hash for shader caching
    uint64_t get_shader_hash() const;
    
    // Serialization
    bool save_to_file(const std::filesystem::path& path) const;
    bool load_from_file(const std::filesystem::path& path);
    
    // Runtime updates
    void mark_dirty() { dirty_ = true; }
    bool is_dirty() const { return dirty_; }
    void clear_dirty_flag() { dirty_ = false; }

private:
    std::string name_;
    ShadingModel shading_model_ = ShadingModel::DefaultLit;
    MaterialBlendMode blend_mode_ = MaterialBlendMode::Opaque;
    bool two_sided_ = false;
    float opacity_cutoff_ = 0.5f;
    
    std::unordered_map<std::string, MaterialParameter> parameters_;
    std::unordered_map<TextureUsage, TextureHandle> textures_;
    
    mutable std::atomic<bool> dirty_{true};
    mutable std::mutex parameter_mutex_;
};

// =============================================================================
// MATERIAL TEMPLATE SYSTEM
// =============================================================================

/**
 * @brief Pre-defined material templates
 */
class MaterialTemplate {
public:
    static Material create_standard_pbr();
    static Material create_unlit();
    static Material create_glass();
    static Material create_metal();
    static Material create_plastic();
    static Material create_ceramic();
    static Material create_rubber();
    static Material create_fabric();
    static Material create_leather();
    static Material create_wood();
    static Material create_concrete();
    static Material create_skin();
    static Material create_vegetation();
    static Material create_water();
    static Material create_ice();
    static Material create_emissive();
    static Material create_hologram();
};

// =============================================================================
// MATERIAL INSTANCE SYSTEM
// =============================================================================

/**
 * @brief Type-safe material handle
 */
using MaterialHandle = ResourceHandle<struct MaterialTag>;

/**
 * @brief Material instance for runtime use
 */
class MaterialInstance {
public:
    MaterialInstance(const Material& base_material);
    ~MaterialInstance() = default;

    // Base material
    const Material& get_base_material() const { return base_material_; }
    
    // Instance parameters (override base material)
    void set_parameter(const std::string& name, const MaterialParameter& value);
    MaterialParameter get_parameter(const std::string& name) const;
    void reset_parameter(const std::string& name);
    
    // Texture overrides
    void set_texture(TextureUsage usage, TextureHandle texture);
    TextureHandle get_texture(TextureUsage usage) const;
    void reset_texture(TextureUsage usage);
    
    // Compiled shader access
    ShaderHandle get_compiled_shader(IRenderer* renderer);
    
    // Resource binding
    void bind_to_renderer(IRenderer* renderer, uint32_t material_slot = 0);
    
    // Updates
    void update_from_base();
    bool needs_recompilation() const;

private:
    const Material& base_material_;
    std::unordered_map<std::string, MaterialParameter> parameter_overrides_;
    std::unordered_map<TextureUsage, TextureHandle> texture_overrides_;
    
    mutable ShaderHandle cached_shader_;
    mutable uint64_t cached_shader_hash_ = 0;
};

// =============================================================================
// MATERIAL MANAGER
// =============================================================================

/**
 * @brief Central material management system
 */
class MaterialManager {
public:
    MaterialManager(IRenderer* renderer);
    ~MaterialManager();

    // Material registration
    MaterialHandle register_material(std::unique_ptr<Material> material);
    MaterialHandle register_material(const Material& material);
    void unregister_material(MaterialHandle handle);
    
    // Material access
    Material* get_material(MaterialHandle handle);
    const Material* get_material(MaterialHandle handle) const;
    
    // Material instance creation
    std::unique_ptr<MaterialInstance> create_instance(MaterialHandle handle);
    
    // Template materials
    MaterialHandle get_default_material();
    MaterialHandle get_error_material();
    MaterialHandle get_template_material(const std::string& template_name);
    
    // Batch operations
    void compile_all_shaders();
    void reload_all_materials();
    void cleanup_unused_materials();
    
    // Asset loading
    MaterialHandle load_material(const std::filesystem::path& path);
    bool save_material(MaterialHandle handle, const std::filesystem::path& path);
    
    // Hot reload support
    void enable_hot_reload(bool enable);
    void check_for_file_changes();
    
    // Statistics
    struct MaterialStats {
        uint32_t material_count = 0;
        uint32_t compiled_shader_count = 0;
        uint32_t unique_texture_count = 0;
        uint64_t total_memory_usage = 0;
        uint32_t hot_reloads_performed = 0;
    };
    
    MaterialStats get_statistics() const;
    
    // Debugging
    void dump_material_info(MaterialHandle handle) const;
    std::vector<MaterialHandle> find_materials_using_texture(TextureHandle texture) const;

private:
    struct MaterialEntry {
        std::unique_ptr<Material> material;
        std::filesystem::path source_path;
        std::filesystem::file_time_type last_modified;
        std::vector<ShaderHandle> compiled_shaders;
        uint32_t reference_count = 0;
    };

    IRenderer* renderer_;
    std::unordered_map<uint64_t, MaterialEntry> materials_;
    std::atomic<uint64_t> next_material_id_{1};
    
    // Default materials
    MaterialHandle default_material_;
    MaterialHandle error_material_;
    std::unordered_map<std::string, MaterialHandle> template_materials_;
    
    // Hot reload system
    bool hot_reload_enabled_ = false;
    std::vector<std::filesystem::path> watched_directories_;
    
    mutable std::shared_mutex materials_mutex_;
    mutable MaterialStats cached_stats_;
    mutable bool stats_dirty_ = true;
    
    void create_default_materials();
    void update_statistics() const;
    ShaderHandle compile_material_shader(const Material& material);
};

// =============================================================================
// TEXTURE MANAGEMENT
// =============================================================================

/**
 * @brief Texture loading and management for materials
 */
class TextureManager {
public:
    TextureManager(IRenderer* renderer);
    ~TextureManager();

    // Texture loading
    TextureHandle load_texture_2d(const std::filesystem::path& path, bool generate_mipmaps = true);
    TextureHandle load_texture_cube(const std::array<std::filesystem::path, 6>& paths);
    TextureHandle load_texture_hdr(const std::filesystem::path& path);
    
    // Procedural textures
    TextureHandle create_solid_color(const std::array<uint8_t, 4>& color, uint32_t size = 1);
    TextureHandle create_normal_map(uint32_t size = 1); // Default up normal
    TextureHandle create_noise_texture(uint32_t size = 256);
    TextureHandle create_brdf_lut(uint32_t size = 512);
    
    // Texture streaming
    void enable_streaming(bool enable, uint64_t memory_budget_mb = 1024);
    void update_streaming();
    
    // Cache management
    void clear_cache();
    void set_cache_size(uint64_t size_mb);
    
    // Hot reload
    void reload_texture(TextureHandle handle);
    void reload_all_textures();

private:
    IRenderer* renderer_;
    std::unordered_map<std::string, TextureHandle> texture_cache_;
    std::unordered_map<TextureHandle, std::filesystem::path> texture_paths_;
    
    bool streaming_enabled_ = false;
    uint64_t memory_budget_ = 0;
    uint64_t current_memory_usage_ = 0;
    
    mutable std::shared_mutex cache_mutex_;
    
    TextureHandle load_texture_from_memory(const void* data, size_t size, 
                                         const std::string& debug_name,
                                         bool generate_mipmaps = true);
};

// =============================================================================
// SHADER GENERATION UTILITIES
// =============================================================================

/**
 * @brief Shader code generation for materials
 */
class MaterialShaderGenerator {
public:
    static std::string generate_vertex_shader(const Material& material);
    static std::string generate_fragment_shader(const Material& material);
    static std::string generate_defines(const Material& material);
    
    // Shader includes and common code
    static std::string get_common_vertex_code();
    static std::string get_common_fragment_code();
    static std::string get_pbr_lighting_code();
    static std::string get_normal_mapping_code();
    static std::string get_parallax_mapping_code();
    
private:
    static std::string generate_vertex_inputs(const Material& material);
    static std::string generate_vertex_outputs(const Material& material);
    static std::string generate_vertex_transforms(const Material& material);
    
    static std::string generate_fragment_inputs(const Material& material);
    static std::string generate_fragment_samplers(const Material& material);
    static std::string generate_fragment_uniforms(const Material& material);
    static std::string generate_fragment_main(const Material& material);
    
    static std::string generate_surface_calculation(const Material& material);
    static std::string generate_lighting_calculation(const Material& material);
    static std::string generate_output_calculation(const Material& material);
};

// =============================================================================
// UTILITY FUNCTIONS
// =============================================================================

/**
 * @brief Convert texture usage to shader sampler name
 */
std::string texture_usage_to_sampler_name(TextureUsage usage);

/**
 * @brief Get default texture for a specific usage
 */
TextureHandle get_default_texture(TextureUsage usage, IRenderer* renderer);

/**
 * @brief Validate material parameter compatibility
 */
bool validate_material_parameter(const std::string& name, const MaterialParameter& value);

/**
 * @brief Calculate memory usage of a material
 */
uint64_t calculate_material_memory_usage(const Material& material, IRenderer* renderer);

} // namespace ecscope::rendering