#include "../include/ecscope/advanced_shader_library.hpp"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <regex>
#include <cmath>

namespace ecscope::renderer::shader_library {

//=============================================================================
// Shader Template Implementation
//=============================================================================

std::string ShaderTemplate::instantiate(const std::unordered_map<std::string, std::string>& parameter_values) const {
    std::string result;
    
    // Start with the fragment shader template
    result = fragment_template;
    
    // Replace template parameters
    for (const auto& [param_name, param_value] : parameter_values) {
        std::string placeholder = "${" + param_name + "}";
        size_t pos = 0;
        while ((pos = result.find(placeholder, pos)) != std::string::npos) {
            result.replace(pos, placeholder.length(), param_value);
            pos += param_value.length();
        }
    }
    
    // Add default values for missing parameters
    for (const auto& param : parameters) {
        std::string placeholder = "${" + param.name + "}";
        if (result.find(placeholder) != std::string::npos) {
            std::string default_val = get_parameter_declaration(param);
            size_t pos = 0;
            while ((pos = result.find(placeholder, pos)) != std::string::npos) {
                result.replace(pos, placeholder.length(), default_val);
                pos += default_val.length();
            }
        }
    }
    
    return result;
}

std::string ShaderTemplate::get_parameter_declaration(const Parameter& param) const {
    switch (param.type) {
        case visual_editor::DataType::Float:
            return std::to_string(std::get<f32>(param.default_value));
        case visual_editor::DataType::Vec2:
            {
                auto vec = std::get<std::array<f32, 2>>(param.default_value);
                return "vec2(" + std::to_string(vec[0]) + ", " + std::to_string(vec[1]) + ")";
            }
        case visual_editor::DataType::Vec3:
            {
                auto vec = std::get<std::array<f32, 3>>(param.default_value);
                return "vec3(" + std::to_string(vec[0]) + ", " + std::to_string(vec[1]) + ", " + std::to_string(vec[2]) + ")";
            }
        case visual_editor::DataType::Vec4:
            {
                auto vec = std::get<std::array<f32, 4>>(param.default_value);
                return "vec4(" + std::to_string(vec[0]) + ", " + std::to_string(vec[1]) + 
                       ", " + std::to_string(vec[2]) + ", " + std::to_string(vec[3]) + ")";
            }
        default:
            return "1.0";
    }
}

bool ShaderTemplate::validate_parameters(const std::unordered_map<std::string, std::string>& values) const {
    for (const auto& param : parameters) {
        if (param.is_required && values.find(param.name) == values.end()) {
            return false;
        }
    }
    return true;
}

//=============================================================================
// Advanced Shader Library Implementation
//=============================================================================

AdvancedShaderLibrary::AdvancedShaderLibrary(shader_runtime::ShaderRuntimeManager* runtime_manager)
    : runtime_manager_(runtime_manager) {
    register_builtin_templates();
}

void AdvancedShaderLibrary::register_template(const ShaderTemplate& shader_template) {
    templates_[shader_template.name] = shader_template;
    templates_by_category_[shader_template.category].push_back(shader_template.name);
    update_statistics();
}

void AdvancedShaderLibrary::register_builtin_templates() {
    create_pbr_templates();
    create_lighting_templates();
    create_post_processing_templates();
    create_particle_templates();
    create_terrain_templates();
    create_water_templates();
    create_debug_templates();
    create_tutorial_templates();
}

std::vector<std::string> AdvancedShaderLibrary::get_template_names(ShaderCategory category) const {
    if (category == ShaderCategory::Custom) {
        std::vector<std::string> all_names;
        for (const auto& [name, template_obj] : templates_) {
            all_names.push_back(name);
        }
        return all_names;
    }
    
    auto it = templates_by_category_.find(category);
    return (it != templates_by_category_.end()) ? it->second : std::vector<std::string>{};
}

const ShaderTemplate* AdvancedShaderLibrary::get_template(const std::string& name) const {
    auto it = templates_.find(name);
    return (it != templates_.end()) ? &it->second : nullptr;
}

shader_runtime::ShaderRuntimeManager::ShaderHandle 
AdvancedShaderLibrary::create_shader_from_template(const std::string& template_name,
                                                  const std::unordered_map<std::string, std::string>& parameters,
                                                  const std::string& instance_name) {
    
    const auto* shader_template = get_template(template_name);
    if (!shader_template) {
        return shader_runtime::ShaderRuntimeManager::INVALID_SHADER_HANDLE;
    }
    
    if (!shader_template->validate_parameters(parameters)) {
        return shader_runtime::ShaderRuntimeManager::INVALID_SHADER_HANDLE;
    }
    
    std::string instance_source = shader_template->instantiate(parameters);
    std::string name = instance_name.empty() ? template_name + "_instance" : instance_name;
    
    shader_runtime::ShaderMetadata metadata;
    metadata.name = name;
    metadata.description = shader_template->description;
    metadata.author = shader_template->author;
    metadata.version = shader_template->version;
    metadata.defines = shader_template->defines;
    
    stats_.created_shaders++;
    
    return runtime_manager_->create_shader(instance_source, resources::ShaderStage::Fragment, name, metadata);
}

shader_runtime::ShaderRuntimeManager::ShaderHandle 
AdvancedShaderLibrary::create_pbr_shader(const PBRMaterial& material,
                                        LightingModel lighting_model,
                                        const std::string& name) {
    
    std::string fragment_source = generate_pbr_fragment_shader(material, lighting_model);
    std::string shader_name = name.empty() ? "PBR_Material" : name;
    
    shader_runtime::ShaderMetadata metadata;
    metadata.name = shader_name;
    metadata.description = "Physically Based Rendering material shader";
    metadata.author = "ECScope";
    
    // Add material-specific defines
    std::string defines_str = material.generate_shader_defines();
    std::istringstream defines_stream(defines_str);
    std::string line;
    while (std::getline(defines_stream, line)) {
        if (!line.empty() && line.find("#define") == 0) {
            size_t space_pos = line.find(' ', 8); // After "#define "
            if (space_pos != std::string::npos) {
                metadata.defines.push_back(line.substr(8, space_pos - 8));
            }
        }
    }
    
    stats_.pbr_shaders++;
    stats_.created_shaders++;
    
    return runtime_manager_->create_shader(fragment_source, resources::ShaderStage::Fragment, shader_name, metadata);
}

void AdvancedShaderLibrary::create_pbr_templates() {
    // Standard PBR Template
    ShaderTemplate pbr_standard;
    pbr_standard.name = "PBR_Standard";
    pbr_standard.description = "Standard Physically Based Rendering material with metallic-roughness workflow";
    pbr_standard.category = ShaderCategory::PBR;
    pbr_standard.difficulty_level = "Intermediate";
    pbr_standard.is_educational = true;
    pbr_standard.estimated_performance_cost = 3.5f;
    
    pbr_standard.vertex_template = builtin_shaders::PBR_VERTEX_SHADER;
    pbr_standard.fragment_template = builtin_shaders::PBR_FRAGMENT_SHADER;
    
    // Add parameters
    pbr_standard.parameters.emplace_back("albedo", visual_editor::DataType::Vec3, 
                                        std::array<f32, 3>{0.5f, 0.5f, 0.5f});
    pbr_standard.parameters.emplace_back("metallic", visual_editor::DataType::Float, 0.0f);
    pbr_standard.parameters.emplace_back("roughness", visual_editor::DataType::Float, 0.5f);
    pbr_standard.parameters.emplace_back("ao", visual_editor::DataType::Float, 1.0f);
    
    pbr_standard.learning_objectives = {
        "Understand Physically Based Rendering principles",
        "Learn about metallic-roughness workflow",
        "Explore BRDF (Bidirectional Reflectance Distribution Function)"
    };
    
    register_template(pbr_standard);
    
    // Glass/Transmission PBR Template
    ShaderTemplate pbr_glass;
    pbr_glass.name = "PBR_Glass";
    pbr_glass.description = "PBR material with transmission and refraction for glass-like surfaces";
    pbr_glass.category = ShaderCategory::PBR;
    pbr_glass.difficulty_level = "Advanced";
    pbr_glass.estimated_performance_cost = 5.0f;
    pbr_glass.supports_mobile = false;
    
    pbr_glass.fragment_template = R"(
#version 330 core

uniform vec3 u_albedo = vec3(${albedo});
uniform float u_transmission = ${transmission};
uniform float u_ior = ${ior};
uniform float u_roughness = ${roughness};
uniform vec3 u_absorption = vec3(${absorption});

in vec3 v_worldPos;
in vec3 v_normal;
in vec2 v_texCoord;
in vec3 v_viewDir;

out vec4 fragColor;

// Glass BRDF implementation
vec3 calculateGlassBRDF(vec3 viewDir, vec3 lightDir, vec3 normal) {
    // Simplified glass BRDF
    vec3 halfVector = normalize(viewDir + lightDir);
    float NdotH = max(dot(normal, halfVector), 0.0);
    float VdotH = max(dot(viewDir, halfVector), 0.0);
    
    // Fresnel term
    float F0 = pow((u_ior - 1.0) / (u_ior + 1.0), 2.0);
    float fresnel = F0 + (1.0 - F0) * pow(1.0 - VdotH, 5.0);
    
    // Transmission
    vec3 transmission = u_albedo * u_transmission * (1.0 - fresnel);
    vec3 reflection = vec3(fresnel);
    
    return transmission + reflection;
}

void main() {
    vec3 normal = normalize(v_normal);
    vec3 viewDir = normalize(v_viewDir);
    
    // Sample environment for reflections/refractions
    vec3 reflectDir = reflect(-viewDir, normal);
    vec3 refractDir = refract(-viewDir, normal, 1.0 / u_ior);
    
    vec3 reflection = vec3(0.8); // Simplified reflection
    vec3 refraction = u_albedo * u_transmission;
    
    // Fresnel blend
    float NdotV = max(dot(normal, viewDir), 0.0);
    float F0 = pow((u_ior - 1.0) / (u_ior + 1.0), 2.0);
    float fresnel = F0 + (1.0 - F0) * pow(1.0 - NdotV, 5.0);
    
    vec3 finalColor = mix(refraction, reflection, fresnel);
    
    fragColor = vec4(finalColor, 1.0 - u_transmission);
}
)";
    
    pbr_glass.parameters.emplace_back("albedo", visual_editor::DataType::Vec3, 
                                     std::array<f32, 3>{0.9f, 0.9f, 0.9f});
    pbr_glass.parameters.emplace_back("transmission", visual_editor::DataType::Float, 0.8f);
    pbr_glass.parameters.emplace_back("ior", visual_editor::DataType::Float, 1.5f);
    pbr_glass.parameters.emplace_back("roughness", visual_editor::DataType::Float, 0.0f);
    pbr_glass.parameters.emplace_back("absorption", visual_editor::DataType::Vec3, 
                                     std::array<f32, 3>{1.0f, 1.0f, 1.0f});
    
    register_template(pbr_glass);
}

void AdvancedShaderLibrary::create_lighting_templates() {
    // Blinn-Phong Lighting Template
    ShaderTemplate blinn_phong;
    blinn_phong.name = "BlinnPhong_Lighting";
    blinn_phong.description = "Classic Blinn-Phong lighting model for educational purposes";
    blinn_phong.category = ShaderCategory::BlinnPhong;
    blinn_phong.difficulty_level = "Beginner";
    blinn_phong.is_educational = true;
    blinn_phong.estimated_performance_cost = 1.5f;
    
    blinn_phong.fragment_template = R"(
#version 330 core

uniform vec3 u_lightPos = vec3(${lightPosition});
uniform vec3 u_lightColor = vec3(${lightColor});
uniform vec3 u_albedo = vec3(${albedo});
uniform float u_shininess = ${shininess};
uniform vec3 u_ambient = vec3(${ambient});

in vec3 v_worldPos;
in vec3 v_normal;
in vec2 v_texCoord;

out vec4 fragColor;

void main() {
    vec3 normal = normalize(v_normal);
    vec3 lightDir = normalize(u_lightPos - v_worldPos);
    vec3 viewDir = normalize(-v_worldPos); // Assuming view is at origin
    vec3 halfwayDir = normalize(lightDir + viewDir);
    
    // Ambient
    vec3 ambient = u_ambient * u_albedo;
    
    // Diffuse
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = diff * u_lightColor * u_albedo;
    
    // Specular (Blinn-Phong)
    float spec = pow(max(dot(normal, halfwayDir), 0.0), u_shininess);
    vec3 specular = spec * u_lightColor;
    
    vec3 result = ambient + diffuse + specular;
    fragColor = vec4(result, 1.0);
}
)";
    
    blinn_phong.parameters.emplace_back("lightPosition", visual_editor::DataType::Vec3,
                                       std::array<f32, 3>{5.0f, 5.0f, 5.0f});
    blinn_phong.parameters.emplace_back("lightColor", visual_editor::DataType::Vec3,
                                       std::array<f32, 3>{1.0f, 1.0f, 1.0f});
    blinn_phong.parameters.emplace_back("albedo", visual_editor::DataType::Vec3,
                                       std::array<f32, 3>{0.8f, 0.8f, 0.8f});
    blinn_phong.parameters.emplace_back("shininess", visual_editor::DataType::Float, 32.0f);
    blinn_phong.parameters.emplace_back("ambient", visual_editor::DataType::Vec3,
                                       std::array<f32, 3>{0.1f, 0.1f, 0.1f});
    
    blinn_phong.learning_objectives = {
        "Understand basic lighting calculations",
        "Learn about ambient, diffuse, and specular components",
        "Compare with Phong lighting model"
    };
    
    register_template(blinn_phong);
}

void AdvancedShaderLibrary::create_post_processing_templates() {
    // Tone Mapping Template
    ShaderTemplate tone_mapping;
    tone_mapping.name = "ToneMapping";
    tone_mapping.description = "HDR tone mapping with multiple tone mapping operators";
    tone_mapping.category = ShaderCategory::PostProcessing;
    tone_mapping.difficulty_level = "Intermediate";
    tone_mapping.estimated_performance_cost = 1.2f;
    
    tone_mapping.fragment_template = builtin_shaders::TONE_MAPPING_SHADER;
    
    tone_mapping.parameters.emplace_back("exposure", visual_editor::DataType::Float, 1.0f);
    tone_mapping.parameters.emplace_back("gamma", visual_editor::DataType::Float, 2.2f);
    tone_mapping.parameters.emplace_back("operator", visual_editor::DataType::Int, 0);
    
    auto& operator_param = tone_mapping.parameters.back();
    operator_param.allowed_values = {"Reinhard", "ACES", "Uncharted2", "Linear"};
    
    register_template(tone_mapping);
    
    // Bloom Effect Template
    ShaderTemplate bloom;
    bloom.name = "Bloom";
    bloom.description = "Gaussian bloom effect for bright areas";
    bloom.category = ShaderCategory::PostProcessing;
    bloom.difficulty_level = "Intermediate";
    bloom.estimated_performance_cost = 2.5f;
    
    bloom.fragment_template = builtin_shaders::BLOOM_SHADER;
    
    bloom.parameters.emplace_back("threshold", visual_editor::DataType::Float, 1.0f);
    bloom.parameters.emplace_back("intensity", visual_editor::DataType::Float, 0.5f);
    bloom.parameters.emplace_back("blur_radius", visual_editor::DataType::Float, 5.0f);
    
    register_template(bloom);
}

void AdvancedShaderLibrary::create_particle_templates() {
    // GPU Particles Template
    ShaderTemplate gpu_particles;
    gpu_particles.name = "GPU_Particles";
    gpu_particles.description = "GPU-based particle system with compute shader";
    gpu_particles.category = ShaderCategory::Particles;
    gpu_particles.difficulty_level = "Advanced";
    gpu_particles.estimated_performance_cost = 4.0f;
    
    gpu_particles.compute_template = builtin_shaders::PARTICLE_COMPUTE_SHADER;
    gpu_particles.vertex_template = builtin_shaders::PARTICLE_VERTEX_SHADER;
    gpu_particles.fragment_template = builtin_shaders::PARTICLE_FRAGMENT_SHADER;
    
    gpu_particles.parameters.emplace_back("maxParticles", visual_editor::DataType::Int, 1000);
    gpu_particles.parameters.emplace_back("gravity", visual_editor::DataType::Vec3,
                                         std::array<f32, 3>{0.0f, -9.81f, 0.0f});
    gpu_particles.parameters.emplace_back("startColor", visual_editor::DataType::Vec4,
                                         std::array<f32, 4>{1.0f, 1.0f, 1.0f, 1.0f});
    gpu_particles.parameters.emplace_back("endColor", visual_editor::DataType::Vec4,
                                         std::array<f32, 4>{1.0f, 0.0f, 0.0f, 0.0f});
    
    register_template(gpu_particles);
}

void AdvancedShaderLibrary::create_terrain_templates() {
    // Multi-texture Terrain Template
    ShaderTemplate terrain;
    terrain.name = "Terrain_MultiTexture";
    terrain.description = "Multi-layered terrain with height-based blending";
    terrain.category = ShaderCategory::Terrain;
    terrain.difficulty_level = "Intermediate";
    terrain.estimated_performance_cost = 3.0f;
    
    terrain.fragment_template = builtin_shaders::TERRAIN_FRAGMENT_SHADER;
    
    terrain.parameters.emplace_back("textureScale", visual_editor::DataType::Float, 16.0f);
    terrain.parameters.emplace_back("blendSharpness", visual_editor::DataType::Float, 4.0f);
    terrain.parameters.emplace_back("layerCount", visual_editor::DataType::Int, 4);
    
    register_template(terrain);
}

void AdvancedShaderLibrary::create_water_templates() {
    // Animated Water Template
    ShaderTemplate water;
    water.name = "Water_Animated";
    water.description = "Animated water surface with reflection and refraction";
    water.category = ShaderCategory::Water;
    water.difficulty_level = "Advanced";
    water.estimated_performance_cost = 4.5f;
    water.supports_mobile = false;
    
    water.vertex_template = builtin_shaders::WATER_VERTEX_SHADER;
    water.fragment_template = builtin_shaders::WATER_FRAGMENT_SHADER;
    
    water.parameters.emplace_back("waveAmplitude", visual_editor::DataType::Float, 0.1f);
    water.parameters.emplace_back("waveFrequency", visual_editor::DataType::Float, 2.0f);
    water.parameters.emplace_back("waveSpeed", visual_editor::DataType::Float, 1.0f);
    water.parameters.emplace_back("waterColor", visual_editor::DataType::Vec3,
                                 std::array<f32, 3>{0.0f, 0.3f, 0.5f});
    water.parameters.emplace_back("transparency", visual_editor::DataType::Float, 0.8f);
    
    register_template(water);
}

void AdvancedShaderLibrary::create_debug_templates() {
    // Normal Visualization Template
    ShaderTemplate debug_normals;
    debug_normals.name = "Debug_Normals";
    debug_normals.description = "Visualize surface normals as colors";
    debug_normals.category = ShaderCategory::Debug;
    debug_normals.difficulty_level = "Beginner";
    debug_normals.is_educational = true;
    debug_normals.estimated_performance_cost = 0.8f;
    
    debug_normals.fragment_template = builtin_shaders::DEBUG_NORMALS_SHADER;
    
    register_template(debug_normals);
    
    // UV Visualization Template
    ShaderTemplate debug_uvs;
    debug_uvs.name = "Debug_UVs";
    debug_uvs.description = "Visualize UV coordinates as colors";
    debug_uvs.category = ShaderCategory::Debug;
    debug_uvs.difficulty_level = "Beginner";
    debug_uvs.is_educational = true;
    debug_uvs.estimated_performance_cost = 0.5f;
    
    debug_uvs.fragment_template = builtin_shaders::DEBUG_UVS_SHADER;
    
    register_template(debug_uvs);
}

void AdvancedShaderLibrary::create_tutorial_templates() {
    // Basic Texture Sampling Tutorial
    ShaderTemplate tutorial_texture;
    tutorial_texture.name = "Tutorial_TextureSampling";
    tutorial_texture.description = "Learn basic texture sampling techniques";
    tutorial_texture.category = ShaderCategory::Tutorial;
    tutorial_texture.difficulty_level = "Beginner";
    tutorial_texture.is_educational = true;
    tutorial_texture.estimated_performance_cost = 1.0f;
    
    tutorial_texture.fragment_template = builtin_shaders::TUTORIAL_TEXTURE_SAMPLING;
    
    tutorial_texture.learning_objectives = {
        "Understand texture coordinate mapping",
        "Learn texture2D function usage",
        "Explore texture filtering options"
    };
    
    tutorial_texture.tutorial_text = R"(
This tutorial demonstrates basic texture sampling in fragment shaders:

1. UV Coordinates: Each vertex has texture coordinates (u, v) that map to the texture
2. texture2D(): The built-in function to sample textures
3. Filtering: How the GPU interpolates between texture pixels

Try modifying the UV coordinates to see how texture mapping works!
)";
    
    register_template(tutorial_texture);
    
    // Normal Mapping Tutorial
    ShaderTemplate tutorial_normals;
    tutorial_normals.name = "Tutorial_NormalMapping";
    tutorial_normals.description = "Learn normal mapping for surface detail";
    tutorial_normals.category = ShaderCategory::Tutorial;
    tutorial_normals.difficulty_level = "Intermediate";
    tutorial_normals.is_educational = true;
    tutorial_normals.estimated_performance_cost = 2.0f;
    
    tutorial_normals.fragment_template = builtin_shaders::TUTORIAL_NORMAL_MAPPING;
    
    tutorial_normals.learning_objectives = {
        "Understand tangent space",
        "Learn normal map interpretation",
        "Explore surface detail enhancement"
    };
    
    register_template(tutorial_normals);
}

std::string AdvancedShaderLibrary::generate_pbr_fragment_shader(const PBRMaterial& material,
                                                               LightingModel lighting_model,
                                                               const std::vector<Light>& lights) const {
    std::ostringstream shader;
    
    shader << "#version 330 core\n\n";
    
    // Material-specific defines
    shader << material.generate_shader_defines();
    
    // Uniforms
    shader << "// Material properties\n";
    shader << "uniform vec3 u_albedo = vec3(" << material.albedo[0] << ", " 
           << material.albedo[1] << ", " << material.albedo[2] << ");\n";
    shader << "uniform float u_metallic = " << material.metallic << ";\n";
    shader << "uniform float u_roughness = " << material.roughness << ";\n";
    shader << "uniform float u_ao = " << material.ao << ";\n";
    
    if (!material.albedo_texture.empty()) {
        shader << "uniform sampler2D u_albedoTexture;\n";
    }
    if (!material.normal_texture.empty()) {
        shader << "uniform sampler2D u_normalTexture;\n";
    }
    if (!material.metallic_roughness_texture.empty()) {
        shader << "uniform sampler2D u_metallicRoughnessTexture;\n";
    }
    
    // Input/Output
    shader << "\nin vec3 v_worldPos;\n";
    shader << "in vec3 v_normal;\n";
    shader << "in vec2 v_texCoord;\n";
    shader << "in vec3 v_tangent;\n";
    shader << "in vec3 v_bitangent;\n\n";
    
    shader << "out vec4 fragColor;\n\n";
    
    // Include BRDF functions
    shader << builtin_shaders::PBR_FUNCTIONS << "\n";
    
    if (!lights.empty()) {
        shader << generate_lighting_calculations(lighting_model, lights);
    }
    
    // Main function
    shader << "void main() {\n";
    shader << "    vec3 albedo = u_albedo;\n";
    shader << "    float metallic = u_metallic;\n";
    shader << "    float roughness = u_roughness;\n";
    shader << "    float ao = u_ao;\n";
    shader << "    vec3 normal = normalize(v_normal);\n\n";
    
    // Sample textures
    if (!material.albedo_texture.empty()) {
        shader << "    albedo *= texture2D(u_albedoTexture, v_texCoord).rgb;\n";
    }
    
    if (!material.normal_texture.empty()) {
        shader << "    vec3 normalMap = texture2D(u_normalTexture, v_texCoord).rgb * 2.0 - 1.0;\n";
        shader << "    mat3 TBN = mat3(normalize(v_tangent), normalize(v_bitangent), normal);\n";
        shader << "    normal = normalize(TBN * normalMap);\n";
    }
    
    if (!material.metallic_roughness_texture.empty()) {
        shader << "    vec2 metallicRoughness = texture2D(u_metallicRoughnessTexture, v_texCoord).bg;\n";
        shader << "    metallic *= metallicRoughness.x;\n";
        shader << "    roughness *= metallicRoughness.y;\n";
    }
    
    // PBR calculations
    shader << "\n    // PBR calculations\n";
    shader << "    vec3 V = normalize(cameraPos - v_worldPos);\n";
    shader << "    vec3 F0 = mix(vec3(0.04), albedo, metallic);\n";
    shader << "    \n";
    shader << "    vec3 Lo = vec3(0.0);\n";
    
    if (!lights.empty()) {
        shader << "    Lo = calculateLighting(v_worldPos, normal, V, albedo, metallic, roughness, F0);\n";
    }
    
    // Ambient lighting
    shader << "    vec3 ambient = vec3(0.03) * albedo * ao;\n";
    shader << "    vec3 color = ambient + Lo;\n";
    
    // Tone mapping and gamma correction
    shader << "    color = color / (color + vec3(1.0));\n";
    shader << "    color = pow(color, vec3(1.0/2.2));\n";
    
    shader << "    fragColor = vec4(color, 1.0);\n";
    shader << "}\n";
    
    return shader.str();
}

std::string AdvancedShaderLibrary::generate_lighting_calculations(LightingModel model,
                                                                 const std::vector<Light>& lights) const {
    std::ostringstream code;
    
    code << "vec3 calculateLighting(vec3 worldPos, vec3 normal, vec3 viewDir, vec3 albedo, float metallic, float roughness, vec3 F0) {\n";
    code << "    vec3 totalLight = vec3(0.0);\n";
    
    for (size_t i = 0; i < lights.size(); ++i) {
        const Light& light = lights[i];
        
        switch (light.type) {
            case Light::Type::Directional:
                code << "    // Directional light " << i << "\n";
                code << "    vec3 lightDir" << i << " = normalize(-vec3(" << light.direction[0] 
                     << ", " << light.direction[1] << ", " << light.direction[2] << "));\n";
                code << "    vec3 lightColor" << i << " = vec3(" << light.color[0] << ", "
                     << light.color[1] << ", " << light.color[2] << ") * " << light.intensity << ";\n";
                break;
                
            case Light::Type::Point:
                code << "    // Point light " << i << "\n";
                code << "    vec3 lightPos" << i << " = vec3(" << light.position[0] << ", "
                     << light.position[1] << ", " << light.position[2] << ");\n";
                code << "    vec3 lightDir" << i << " = normalize(lightPos" << i << " - worldPos);\n";
                code << "    float distance" << i << " = length(lightPos" << i << " - worldPos);\n";
                code << "    float attenuation" << i << " = 1.0 / (1.0 + 0.09 * distance" << i
                     << " + 0.032 * distance" << i << " * distance" << i << ");\n";
                code << "    vec3 lightColor" << i << " = vec3(" << light.color[0] << ", "
                     << light.color[1] << ", " << light.color[2] << ") * " << light.intensity
                     << " * attenuation" << i << ";\n";
                break;
                
            default:
                continue;
        }
        
        // Add BRDF calculation
        switch (model) {
            case LightingModel::PBR_MetallicRoughness:
                code << "    totalLight += calculatePBR(normal, viewDir, lightDir" << i
                     << ", lightColor" << i << ", albedo, metallic, roughness, F0);\n";
                break;
                
            case LightingModel::BlinnPhong:
                code << "    totalLight += calculateBlinnPhong(normal, viewDir, lightDir" << i
                     << ", lightColor" << i << ", albedo, 32.0);\n";
                break;
                
            default:
                code << "    totalLight += calculateLambert(normal, lightDir" << i
                     << ", lightColor" << i << ", albedo);\n";
                break;
        }
    }
    
    code << "    return totalLight;\n";
    code << "}\n\n";
    
    return code.str();
}

AdvancedShaderLibrary::ShaderAnalysis 
AdvancedShaderLibrary::analyze_shader_template(const std::string& template_name,
                                              const std::unordered_map<std::string, std::string>& parameters) const {
    
    const auto* shader_template = get_template(template_name);
    if (!shader_template) {
        return ShaderAnalysis{};
    }
    
    std::string source = shader_template->instantiate(parameters);
    return analyze_shader_source(source);
}

AdvancedShaderLibrary::ShaderAnalysis 
AdvancedShaderLibrary::analyze_shader_source(const std::string& source) const {
    ShaderAnalysis analysis;
    
    // Count texture samples
    std::regex texture_regex(R"(texture2D|textureCube|texture)");
    analysis.texture_samples = std::distance(std::sregex_iterator(source.begin(), source.end(), texture_regex),
                                           std::sregex_iterator());
    
    // Count math operations
    std::regex math_regex(R"(\+|\-|\*|\/|pow|sqrt|sin|cos|tan|normalize|dot|cross)");
    analysis.math_operations = std::distance(std::sregex_iterator(source.begin(), source.end(), math_regex),
                                           std::sregex_iterator());
    
    // Estimate instruction count (very rough)
    analysis.instruction_count = analysis.texture_samples * 4 + analysis.math_operations * 2;
    
    // Calculate complexity score
    analysis.complexity_score = std::min(100.0f, 
        (analysis.texture_samples * 5.0f) + 
        (analysis.math_operations * 1.0f) + 
        (static_cast<f32>(source.length()) / 100.0f));
    
    // Calculate performance rating (inverse of complexity)
    analysis.performance_rating = std::max(0.0f, 100.0f - analysis.complexity_score);
    
    // Generate warnings and suggestions
    if (analysis.texture_samples > 8) {
        analysis.performance_warnings.push_back("High texture sample count may impact performance");
        analysis.optimization_suggestions.push_back("Consider combining textures or reducing samples");
    }
    
    if (analysis.math_operations > 50) {
        analysis.performance_warnings.push_back("Many math operations detected");
        analysis.optimization_suggestions.push_back("Look for opportunities to pre-compute values");
    }
    
    if (source.find("for") != std::string::npos) {
        analysis.performance_warnings.push_back("Dynamic loops detected");
        analysis.mobile_compatibility_notes.push_back("Unroll loops for better mobile performance");
    }
    
    if (!analysis.is_mobile_friendly()) {
        analysis.mobile_compatibility_notes.push_back("Shader may be too complex for mobile devices");
    }
    
    return analysis;
}

void AdvancedShaderLibrary::update_statistics() {
    stats_.total_templates = templates_.size();
    
    // Reset category counts
    for (int i = 0; i <= static_cast<int>(ShaderCategory::Custom); ++i) {
        stats_.templates_by_category[i] = 0;
    }
    
    // Count templates by category
    for (const auto& [name, shader_template] : templates_) {
        stats_.templates_by_category[static_cast<int>(shader_template.category)]++;
        
        if (shader_template.is_educational) {
            stats_.educational_templates++;
        }
    }
    
    // Calculate average complexity (simplified)
    f32 total_complexity = 0.0f;
    for (const auto& [name, shader_template] : templates_) {
        total_complexity += shader_template.estimated_performance_cost;
    }
    
    if (!templates_.empty()) {
        stats_.average_complexity = total_complexity / templates_.size();
    }
}

AdvancedShaderLibrary::LibraryStatistics AdvancedShaderLibrary::get_library_statistics() const {
    const_cast<AdvancedShaderLibrary*>(this)->update_statistics();
    return stats_;
}

//=============================================================================
// Built-in Shader Source Code
//=============================================================================

namespace builtin_shaders {
    const char* PBR_VERTEX_SHADER = R"(
#version 330 core

layout (location = 0) in vec3 a_position;
layout (location = 1) in vec3 a_normal;
layout (location = 2) in vec2 a_texCoord;
layout (location = 3) in vec3 a_tangent;

uniform mat4 u_model;
uniform mat4 u_view;
uniform mat4 u_projection;
uniform mat3 u_normalMatrix;

out vec3 v_worldPos;
out vec3 v_normal;
out vec2 v_texCoord;
out vec3 v_tangent;
out vec3 v_bitangent;

void main() {
    v_worldPos = vec3(u_model * vec4(a_position, 1.0));
    v_normal = u_normalMatrix * a_normal;
    v_texCoord = a_texCoord;
    v_tangent = u_normalMatrix * a_tangent;
    v_bitangent = cross(v_normal, v_tangent);
    
    gl_Position = u_projection * u_view * vec4(v_worldPos, 1.0);
}
)";

    const char* PBR_FRAGMENT_SHADER = R"(
#version 330 core

uniform vec3 u_cameraPos;
uniform vec3 u_albedo;
uniform float u_metallic;
uniform float u_roughness;
uniform float u_ao;

#ifdef USE_ALBEDO_TEXTURE
uniform sampler2D u_albedoTexture;
#endif

#ifdef USE_NORMAL_TEXTURE
uniform sampler2D u_normalTexture;
#endif

#ifdef USE_METALLIC_ROUGHNESS_TEXTURE
uniform sampler2D u_metallicRoughnessTexture;
#endif

in vec3 v_worldPos;
in vec3 v_normal;
in vec2 v_texCoord;
in vec3 v_tangent;
in vec3 v_bitangent;

out vec4 fragColor;

// PBR functions will be included here
)";

    const char* PBR_FUNCTIONS = R"(
// Distribution function
float DistributionGGX(vec3 N, vec3 H, float roughness) {
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;
    
    float num = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = 3.14159265 * denom * denom;
    
    return num / denom;
}

// Geometry function
float GeometrySchlickGGX(float NdotV, float roughness) {
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;
    
    float num = NdotV;
    float denom = NdotV * (1.0 - k) + k;
    
    return num / denom;
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness) {
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);
    
    return ggx1 * ggx2;
}

// Fresnel function
vec3 fresnelSchlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

// PBR calculation
vec3 calculatePBR(vec3 N, vec3 V, vec3 L, vec3 radiance, vec3 albedo, float metallic, float roughness, vec3 F0) {
    vec3 H = normalize(V + L);
    
    float NDF = DistributionGGX(N, H, roughness);
    float G = GeometrySmith(N, V, L, roughness);
    vec3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);
    
    vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;
    kD *= 1.0 - metallic;
    
    vec3 numerator = NDF * G * F;
    float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
    vec3 specular = numerator / denominator;
    
    float NdotL = max(dot(N, L), 0.0);
    return (kD * albedo / 3.14159265 + specular) * radiance * NdotL;
}
)";

    const char* TONE_MAPPING_SHADER = R"(
#version 330 core

uniform sampler2D u_hdrTexture;
uniform float u_exposure = ${exposure};
uniform float u_gamma = ${gamma};
uniform int u_operator = ${operator};

in vec2 v_texCoord;
out vec4 fragColor;

vec3 reinhardToneMapping(vec3 color) {
    return color / (color + vec3(1.0));
}

vec3 acesToneMapping(vec3 color) {
    float a = 2.51;
    float b = 0.03;
    float c = 2.43;
    float d = 0.59;
    float e = 0.14;
    return clamp((color * (a * color + b)) / (color * (c * color + d) + e), 0.0, 1.0);
}

void main() {
    vec3 hdrColor = texture2D(u_hdrTexture, v_texCoord).rgb;
    hdrColor *= u_exposure;
    
    vec3 mapped;
    if (u_operator == 0) {
        mapped = reinhardToneMapping(hdrColor);
    } else if (u_operator == 1) {
        mapped = acesToneMapping(hdrColor);
    } else {
        mapped = hdrColor; // Linear
    }
    
    mapped = pow(mapped, vec3(1.0 / u_gamma));
    fragColor = vec4(mapped, 1.0);
}
)";

    const char* BLOOM_SHADER = R"(
#version 330 core

uniform sampler2D u_sceneTexture;
uniform float u_threshold = ${threshold};
uniform float u_intensity = ${intensity};

in vec2 v_texCoord;
out vec4 fragColor;

void main() {
    vec3 color = texture2D(u_sceneTexture, v_texCoord).rgb;
    
    // Extract bright areas
    float brightness = dot(color, vec3(0.2126, 0.7152, 0.0722));
    if (brightness > u_threshold) {
        fragColor = vec4(color * u_intensity, 1.0);
    } else {
        fragColor = vec4(0.0, 0.0, 0.0, 1.0);
    }
}
)";

    const char* DEBUG_NORMALS_SHADER = R"(
#version 330 core

in vec3 v_normal;
out vec4 fragColor;

void main() {
    vec3 normal = normalize(v_normal);
    fragColor = vec4(normal * 0.5 + 0.5, 1.0);
}
)";

    const char* DEBUG_UVS_SHADER = R"(
#version 330 core

in vec2 v_texCoord;
out vec4 fragColor;

void main() {
    fragColor = vec4(v_texCoord, 0.0, 1.0);
}
)";

    const char* TUTORIAL_TEXTURE_SAMPLING = R"(
#version 330 core

uniform sampler2D u_texture;
uniform float u_uvScale = 1.0;

in vec2 v_texCoord;
out vec4 fragColor;

void main() {
    // Scale UV coordinates
    vec2 uv = v_texCoord * u_uvScale;
    
    // Sample the texture
    vec4 textureColor = texture2D(u_texture, uv);
    
    fragColor = textureColor;
}
)";

    const char* TUTORIAL_NORMAL_MAPPING = R"(
#version 330 core

uniform sampler2D u_diffuseTexture;
uniform sampler2D u_normalTexture;
uniform vec3 u_lightPos;

in vec3 v_worldPos;
in vec3 v_normal;
in vec2 v_texCoord;
in vec3 v_tangent;
in vec3 v_bitangent;

out vec4 fragColor;

void main() {
    // Sample textures
    vec3 albedo = texture2D(u_diffuseTexture, v_texCoord).rgb;
    vec3 normalMap = texture2D(u_normalTexture, v_texCoord).rgb * 2.0 - 1.0;
    
    // Create TBN matrix
    vec3 N = normalize(v_normal);
    vec3 T = normalize(v_tangent);
    vec3 B = normalize(v_bitangent);
    mat3 TBN = mat3(T, B, N);
    
    // Transform normal from tangent space to world space
    vec3 normal = normalize(TBN * normalMap);
    
    // Simple lighting
    vec3 lightDir = normalize(u_lightPos - v_worldPos);
    float NdotL = max(dot(normal, lightDir), 0.0);
    
    vec3 color = albedo * NdotL;
    fragColor = vec4(color, 1.0);
}
)";

    // Placeholder implementations for other shaders
    const char* PARTICLE_VERTEX_SHADER = "// Particle vertex shader placeholder";
    const char* PARTICLE_FRAGMENT_SHADER = "// Particle fragment shader placeholder";
    const char* PARTICLE_COMPUTE_SHADER = "// Particle compute shader placeholder";
    const char* TERRAIN_VERTEX_SHADER = "// Terrain vertex shader placeholder";
    const char* TERRAIN_FRAGMENT_SHADER = "// Terrain fragment shader placeholder";
    const char* TERRAIN_TESSELLATION_SHADER = "// Terrain tessellation shader placeholder";
    const char* WATER_VERTEX_SHADER = "// Water vertex shader placeholder";
    const char* WATER_FRAGMENT_SHADER = "// Water fragment shader placeholder";
    const char* WATER_WAVE_FUNCTIONS = "// Water wave functions placeholder";
    const char* WIREFRAME_SHADER = "// Wireframe shader placeholder";
    const char* SKYBOX_SHADER = "// Skybox shader placeholder";
    const char* SSAO_SHADER = "// SSAO shader placeholder";
    const char* FXAA_SHADER = "// FXAA shader placeholder";
    const char* COLOR_GRADING_SHADER = "// Color grading shader placeholder";
    const char* LIGHTING_FUNCTIONS = "// Additional lighting functions placeholder";
}

} // namespace ecscope::renderer::shader_library