#include "../include/ecscope/advanced_shader_ecs_integration.hpp"
#include <algorithm>
#include <chrono>
#include <cmath>

namespace ecscope::renderer::ecs_integration {

//=============================================================================
// Material Management System Implementation
//=============================================================================

MaterialComponent MaterialManagementSystem::create_pbr_material(const shader_library::PBRMaterial& pbr_properties,
                                                               const std::string& name) {
    if (!shader_library_) return MaterialComponent{};
    
    // Create shader from PBR material
    auto handle = shader_library_->create_pbr_shader(pbr_properties, shader_library::LightingModel::PBR_MetallicRoughness, name);
    
    MaterialComponent material(handle, name);
    material.pbr_properties = pbr_properties;
    
    // Set rendering flags based on material properties
    material.is_transparent = pbr_properties.transmission > 0.1f;
    material.double_sided = pbr_properties.double_sided;
    material.cast_shadows = pbr_properties.cast_shadows;
    material.receive_shadows = pbr_properties.receive_shadows;
    
    return material;
}

MaterialComponent MaterialManagementSystem::create_material_from_template(const std::string& template_name,
                                                                         const std::unordered_map<std::string, std::string>& parameters,
                                                                         const std::string& name) {
    if (!shader_library_) return MaterialComponent{};
    
    auto handle = shader_library_->create_shader_from_template(template_name, parameters, name);
    
    MaterialComponent material(handle, name.empty() ? template_name : name);
    
    // Set default properties based on template type
    const auto* template_obj = shader_library_->get_template(template_name);
    if (template_obj) {
        material.educational_description = template_obj->description;
        
        // Set performance-based flags
        if (template_obj->estimated_performance_cost > 3.0f) {
            material.lod_bias = 0.5f; // Use lower LOD sooner for expensive materials
        }
    }
    
    return material;
}

void MaterialManagementSystem::update_material_uniforms(Entity entity, const MaterialComponent& material) {
    if (!runtime_manager_ || material.shader_handle == 0) return;
    
    uniform_updates_this_frame_++;
    
    // Bind PBR properties
    const auto& pbr = material.pbr_properties;
    
    // Basic PBR uniforms
    // In a real implementation, you would bind these to the shader
    // This is a simplified demonstration of the concept
    
    // Handle animated properties
    if (material.has_animated_properties) {
        f32 current_time = static_cast<f32>(std::chrono::steady_clock::now().time_since_epoch().count()) / 1e9f;
        
        for (const auto& [uniform_name, speed] : material.animated_uniform_speeds) {
            f32 animated_value = std::sin(current_time * speed);
            // Apply animated value to uniform
        }
    }
    
    // Custom uniforms
    for (const auto& [name, value] : material.custom_uniforms) {
        // Bind custom uniform value
    }
}

void MaterialManagementSystem::setup_material_presets() {
    // Create standard material presets
    shader_library::PBRMaterial plastic_preset;
    plastic_preset.albedo = {0.2f, 0.6f, 0.9f};
    plastic_preset.metallic = 0.0f;
    plastic_preset.roughness = 0.3f;
    material_presets_["plastic"] = plastic_preset;
    
    shader_library::PBRMaterial metal_preset;
    metal_preset.albedo = {0.7f, 0.7f, 0.8f};
    metal_preset.metallic = 1.0f;
    metal_preset.roughness = 0.1f;
    material_presets_["metal"] = metal_preset;
    
    shader_library::PBRMaterial glass_preset;
    glass_preset.albedo = {0.95f, 0.95f, 0.95f};
    glass_preset.metallic = 0.0f;
    glass_preset.roughness = 0.0f;
    glass_preset.transmission = 0.9f;
    glass_preset.ior = 1.5f;
    material_presets_["glass"] = glass_preset;
}

void MaterialManagementSystem::create_default_materials() {
    if (!shader_library_) return;
    
    // Create basic shaders for fallback
    auto basic_template = shader_library_->get_template("PBR_Standard");
    if (basic_template) {
        std::unordered_map<std::string, std::string> default_params;
        auto handle = shader_library_->create_shader_from_template("PBR_Standard", default_params, "default_material");
        default_shaders_["default"] = handle;
    }
    
    // Error material (bright magenta for missing shaders)
    shader_library::PBRMaterial error_material;
    error_material.albedo = {1.0f, 0.0f, 1.0f}; // Magenta
    error_material.emissive = {0.2f, 0.0f, 0.2f};
    auto error_handle = shader_library_->create_pbr_shader(error_material, 
                                                          shader_library::LightingModel::PBR_MetallicRoughness,
                                                          "error_material");
    default_shaders_["error"] = error_handle;
}

void MaterialManagementSystem::update_animated_materials(f32 delta_time) {
    get_world().for_each<MaterialComponent>([&](Entity entity, MaterialComponent& material) {
        if (material.has_animated_properties) {
            update_material_uniforms(entity, material);
        }
    });
}

void MaterialManagementSystem::check_shader_recompilation() {
    // Check if any shaders need recompilation due to hot-reload
    if (runtime_manager_) {
        // This would integrate with the hot-reload system
        // For now, we'll just track shader switches
        shader_switches_this_frame_ = 0;
    }
}

void MaterialManagementSystem::update_material_lod() {
    // Update LOD based on distance to camera
    // This would require camera information which would be provided by the rendering system
    
    get_world().for_each<MaterialComponent>([&](Entity entity, MaterialComponent& material) {
        if (!material.lod_variants.empty()) {
            // Calculate distance to camera and select appropriate LOD
            // For demonstration, we'll use a simple placeholder
            f32 distance = 100.0f; // This would be calculated from camera position
            
            u32 new_lod = 0;
            if (distance > 200.0f) new_lod = std::min(2u, static_cast<u32>(material.lod_variants.size()) - 1);
            else if (distance > 100.0f) new_lod = std::min(1u, static_cast<u32>(material.lod_variants.size()) - 1);
            
            if (new_lod != material.current_lod_level && new_lod < material.lod_variants.size()) {
                material.current_lod_level = new_lod;
                material.shader_handle = material.lod_variants[new_lod];
            }
        }
    });
}

void MaterialManagementSystem::update_material_statistics() {
    // Update performance statistics for the frame
    total_material_update_time_ += 0.1f; // Placeholder timing
}

MaterialManagementSystem::MaterialPerformanceReport MaterialManagementSystem::generate_performance_report() const {
    MaterialPerformanceReport report;
    
    u32 material_count = 0;
    std::unordered_set<shader_runtime::ShaderRuntimeManager::ShaderHandle> unique_shaders;
    u32 animated_count = 0;
    
    get_world().for_each<MaterialComponent>([&](Entity entity, const MaterialComponent& material) {
        material_count++;
        unique_shaders.insert(material.shader_handle);
        if (material.has_animated_properties) {
            animated_count++;
        }
    });
    
    report.total_materials = material_count;
    report.unique_shaders = static_cast<u32>(unique_shaders.size());
    report.animated_materials = animated_count;
    report.average_uniform_updates_per_frame = static_cast<f32>(uniform_updates_this_frame_);
    
    // Generate optimization suggestions
    if (report.unique_shaders < report.total_materials / 10) {
        report.optimization_suggestions.push_back("Consider using shader variants instead of unique shaders");
    }
    if (report.animated_materials > report.total_materials / 2) {
        report.optimization_suggestions.push_back("High number of animated materials may impact performance");
    }
    
    return report;
}

//=============================================================================
// Advanced Rendering System Implementation
//=============================================================================

void AdvancedRenderingSystem::initialize() {
    System::initialize();
    setup_rendering_pipeline();
    initialize_gpu_resources();
    
    if (debugger_ && config_.enable_performance_tracking) {
        setup_debug_visualization();
    }
}

void AdvancedRenderingSystem::setup_rendering_pipeline() {
    // Initialize rendering state
    render_queue_.reserve(1000); // Pre-allocate for performance
    
    // Setup GPU buffers for batched rendering
    if (config_.enable_draw_call_batching) {
        setup_indirect_rendering();
    }
}

void AdvancedRenderingSystem::initialize_gpu_resources() {
    // Create uniform buffers for per-frame data
    // This would create actual OpenGL/DirectX buffers in a real implementation
    uniform_buffer_id_ = 1; // Placeholder
    instance_buffer_id_ = 2; // Placeholder
}

void AdvancedRenderingSystem::perform_culling() {
    auto culling_start = std::chrono::high_resolution_clock::now();
    
    render_queue_.clear();
    current_frame_stats_.total_entities = 0;
    current_frame_stats_.culled_entities = 0;
    
    get_world().for_each<MaterialComponent, components::Transform, components::RenderComponent>(
        [&](Entity entity, const MaterialComponent& material, const components::Transform& transform, const components::RenderComponent& render) {
            current_frame_stats_.total_entities++;
            
            // Simple frustum culling (placeholder)
            bool is_visible = true; // This would perform actual frustum culling
            
            if (config_.enable_frustum_culling && !is_visible) {
                current_frame_stats_.culled_entities++;
                return;
            }
            
            render_queue_.push_back(entity);
        });
    
    current_frame_stats_.rendered_entities = static_cast<u32>(render_queue_.size());
    
    auto culling_end = std::chrono::high_resolution_clock::now();
    auto culling_duration = std::chrono::duration_cast<std::chrono::microseconds>(culling_end - culling_start);
    current_frame_stats_.culling_time = culling_duration.count() / 1000.0f;
}

void AdvancedRenderingSystem::sort_render_queue() {
    auto sorting_start = std::chrono::high_resolution_clock::now();
    
    if (!config_.sort_by_shader && !config_.sort_by_material && !config_.sort_by_depth) {
        return;
    }
    
    std::sort(render_queue_.begin(), render_queue_.end(), [&](Entity a, Entity b) {
        auto* material_a = get_world().try_get_component<MaterialComponent>(a);
        auto* material_b = get_world().try_get_component<MaterialComponent>(b);
        
        if (!material_a || !material_b) return false;
        
        // Sort by transparency first (opaque objects first)
        if (material_a->is_transparent != material_b->is_transparent) {
            return !material_a->is_transparent; // Opaque first
        }
        
        // Sort by shader
        if (config_.sort_by_shader && material_a->shader_handle != material_b->shader_handle) {
            return material_a->shader_handle < material_b->shader_handle;
        }
        
        // Sort by material properties
        if (config_.sort_by_material) {
            // Simple material sorting based on properties
            return material_a->material_name < material_b->material_name;
        }
        
        return false;
    });
    
    auto sorting_end = std::chrono::high_resolution_clock::now();
    auto sorting_duration = std::chrono::duration_cast<std::chrono::microseconds>(sorting_end - sorting_start);
    current_frame_stats_.sorting_time = sorting_duration.count() / 1000.0f;
}

void AdvancedRenderingSystem::batch_render_calls() {
    // Group entities by shader and material for batching
    if (!config_.enable_draw_call_batching) {
        current_frame_stats_.draw_calls = static_cast<u32>(render_queue_.size());
        current_frame_stats_.batched_draw_calls = 0;
        return;
    }
    
    auto batches = utils::group_entities_for_batching(render_queue_, get_world());
    current_frame_stats_.draw_calls = static_cast<u32>(batches.size());
    current_frame_stats_.batched_draw_calls = static_cast<u32>(render_queue_.size()) - current_frame_stats_.draw_calls;
}

void AdvancedRenderingSystem::submit_draw_calls() {
    auto rendering_start = std::chrono::high_resolution_clock::now();
    
    if (debugger_) {
        debugger_->get_profiler()->begin_event("Submit Draw Calls");
    }
    
    // Submit actual draw calls
    for (Entity entity : render_queue_) {
        auto* material = get_world().try_get_component<MaterialComponent>(entity);
        if (!material || material->shader_handle == 0) continue;
        
        if (debugger_) {
            debugger_->get_profiler()->profile_shader_execution(material->shader_handle, "draw_call");
        }
        
        // Bind material uniforms
        // Submit draw call
        // This would integrate with your actual rendering API
    }
    
    if (debugger_) {
        debugger_->get_profiler()->end_event("Submit Draw Calls");
    }
    
    auto rendering_end = std::chrono::high_resolution_clock::now();
    auto rendering_duration = std::chrono::duration_cast<std::chrono::microseconds>(rendering_end - rendering_start);
    current_frame_stats_.rendering_time = rendering_duration.count() / 1000.0f;
}

void AdvancedRenderingSystem::update_lod_selection() {
    // Update LOD selection based on distance from camera
    get_world().for_each<MaterialComponent, components::Transform>([&](Entity entity, MaterialComponent& material, const components::Transform& transform) {
        if (material.lod_variants.empty()) return;
        
        // Calculate distance from camera
        f32 distance = 100.0f; // Placeholder - would calculate from camera position
        
        // Select appropriate LOD level
        u32 lod_level = utils::select_lod_level(distance, config_.lod_distances);
        
        if (lod_level != material.current_lod_level) {
            material.current_lod_level = lod_level;
            auto new_shader = utils::select_lod_shader(material, lod_level);
            if (new_shader != 0) {
                material.shader_handle = new_shader;
            }
        }
    });
}

void AdvancedRenderingSystem::update_performance_metrics(f32 delta_time) {
    // Update performance metrics based on current frame
    
    // Detect bottlenecks
    if (current_frame_stats_.culling_time > 2.0f) {
        current_frame_stats_.bottlenecks.push_back("Culling performance issue");
    }
    if (current_frame_stats_.sorting_time > 1.0f) {
        current_frame_stats_.bottlenecks.push_back("Sorting taking too long");
    }
    if (current_frame_stats_.rendering_time > 10.0f) {
        current_frame_stats_.bottlenecks.push_back("Rendering performance issue");
    }
    if (current_frame_stats_.draw_calls > 1000) {
        current_frame_stats_.bottlenecks.push_back("Too many draw calls");
    }
}

AdvancedRenderingSystem::RenderingPerformanceReport AdvancedRenderingSystem::generate_performance_report() const {
    return current_frame_stats_;
}

void AdvancedRenderingSystem::setup_debug_visualization() {
    if (!debugger_) return;
    
    // Configure debug overlay for rendering system
    auto* overlay = debugger_->get_overlay();
    if (overlay) {
        // Setup rendering-specific debug visualizations
        overlay->set_overlay_enabled(shader_debugging::ShaderDebugOverlay::OverlayType::PerformanceGraph, true);
        overlay->set_overlay_enabled(shader_debugging::ShaderDebugOverlay::OverlayType::DrawCallAnalysis, true);
    }
}

//=============================================================================
// Advanced Lighting System Implementation
//=============================================================================

void AdvancedLightingSystem::initialize() {
    System::initialize();
    
    setup_lighting_uniforms();
    
    if (config_.enable_shadows) {
        setup_shadow_mapping();
    }
    
    if (config_.enable_volumetric_lighting) {
        setup_volumetric_lighting();
    }
}

void AdvancedLightingSystem::setup_shadow_mapping() {
    // Create shadow map textures
    // This would create actual shadow map resources in a real implementation
    shadow_maps_.resize(config_.max_directional_lights + config_.max_point_lights + config_.max_spot_lights);
    
    if (config_.use_cascade_shadows) {
        cascade_shadow_maps_.resize(config_.cascade_count);
    }
}

void AdvancedLightingSystem::setup_lighting_uniforms() {
    // Create uniform buffers for lighting data
    // This would create actual uniform buffer objects in a real implementation
    lights_uniform_buffer_ = 1; // Placeholder
    
    if (config_.enable_shadows) {
        shadow_uniform_buffer_ = 2; // Placeholder
    }
}

void AdvancedLightingSystem::setup_volumetric_lighting() {
    // Initialize volumetric lighting resources
    // This would set up the necessary textures and shaders for volumetric effects
}

void AdvancedLightingSystem::update_animated_lights(f32 delta_time) {
    get_world().for_each<AdvancedLightComponent>([&](Entity entity, AdvancedLightComponent& light) {
        if (light.animate_intensity || light.animate_color) {
            f32 time = static_cast<f32>(std::chrono::steady_clock::now().time_since_epoch().count()) / 1e9f;
            
            if (light.animate_intensity) {
                f32 intensity_multiplier = 0.5f + 0.5f * std::sin(time * light.animation_speed);
                light.light_data.intensity = light.light_data.intensity * intensity_multiplier;
            }
            
            if (light.animate_color) {
                for (int i = 0; i < 3; ++i) {
                    f32 color_offset = light.animation_color_range[i] * 
                                     std::sin(time * light.animation_speed + i * 2.0f);
                    light.light_data.color[i] = light.base_color[i] + color_offset;
                }
            }
        }
    });
}

void AdvancedLightingSystem::perform_light_culling() {
    // Reset light lists
    directional_lights_.clear();
    point_lights_.clear();
    spot_lights_.clear();
    
    // Categorize and cull lights
    get_world().for_each<AdvancedLightComponent, components::Transform>([&](Entity entity, AdvancedLightComponent& light, const components::Transform& transform) {
        // Simple distance culling
        f32 distance_to_camera = 100.0f; // Placeholder
        if (distance_to_camera > light.cull_distance) {
            light.is_visible = false;
            return;
        }
        
        light.is_visible = true;
        
        // Categorize light
        switch (light.light_data.type) {
            case shader_library::Light::Type::Directional:
                if (directional_lights_.size() < config_.max_directional_lights) {
                    directional_lights_.push_back(entity);
                }
                break;
            case shader_library::Light::Type::Point:
                if (point_lights_.size() < config_.max_point_lights) {
                    point_lights_.push_back(entity);
                }
                break;
            case shader_library::Light::Type::Spot:
                if (spot_lights_.size() < config_.max_spot_lights) {
                    spot_lights_.push_back(entity);
                }
                break;
            default:
                break;
        }
    });
}

void AdvancedLightingSystem::update_shadow_maps() {
    // Update shadow maps for lights that cast shadows
    for (Entity light_entity : directional_lights_) {
        auto* light = get_world().try_get_component<AdvancedLightComponent>(light_entity);
        if (light && light->cast_shadows) {
            // Render shadow map for this light
            // This would involve setting up the shadow camera and rendering from the light's perspective
        }
    }
    
    // Similar for point and spot lights
}

void AdvancedLightingSystem::update_lighting_uniforms() {
    // Update uniform buffers with current light data
    // This would upload light data to GPU uniform buffers
}

void AdvancedLightingSystem::update_volumetric_lighting(f32 delta_time) {
    // Update volumetric lighting effects
    if (!config_.enable_volumetric_lighting) return;
    
    // Process volumetric lights
    get_world().for_each<AdvancedLightComponent>([&](Entity entity, const AdvancedLightComponent& light) {
        if (light.enable_volumetrics) {
            // Update volumetric lighting calculations
        }
    });
}

//=============================================================================
// Utility Functions Implementation
//=============================================================================

namespace utils {
    MaterialComponent create_standard_material(const std::array<f32, 3>& albedo, f32 metallic, f32 roughness) {
        shader_library::PBRMaterial pbr_material;
        pbr_material.albedo = albedo;
        pbr_material.metallic = metallic;
        pbr_material.roughness = roughness;
        
        MaterialComponent material;
        material.pbr_properties = pbr_material;
        material.material_name = "Standard Material";
        
        return material;
    }
    
    MaterialComponent create_metallic_material(const std::array<f32, 3>& albedo, f32 roughness) {
        return create_standard_material(albedo, 1.0f, roughness);
    }
    
    MaterialComponent create_glass_material(const std::array<f32, 3>& color, f32 transmission, f32 ior) {
        shader_library::PBRMaterial pbr_material;
        pbr_material.albedo = color;
        pbr_material.metallic = 0.0f;
        pbr_material.roughness = 0.0f;
        pbr_material.transmission = transmission;
        pbr_material.ior = ior;
        
        MaterialComponent material;
        material.pbr_properties = pbr_material;
        material.material_name = "Glass Material";
        material.is_transparent = true;
        
        return material;
    }
    
    MaterialComponent create_emissive_material(const std::array<f32, 3>& emissive_color, f32 intensity) {
        shader_library::PBRMaterial pbr_material;
        pbr_material.emissive = emissive_color;
        pbr_material.emissive_strength = intensity;
        
        MaterialComponent material;
        material.pbr_properties = pbr_material;
        material.material_name = "Emissive Material";
        
        return material;
    }
    
    f32 calculate_lod_distance(Entity entity, Entity camera) {
        // Placeholder implementation
        return 100.0f; // This would calculate actual distance between entity and camera
    }
    
    u32 select_lod_level(f32 distance, const std::vector<f32>& lod_distances) {
        for (u32 i = 0; i < lod_distances.size(); ++i) {
            if (distance < lod_distances[i]) {
                return i;
            }
        }
        return static_cast<u32>(lod_distances.size()); // Furthest LOD
    }
    
    shader_runtime::ShaderRuntimeManager::ShaderHandle select_lod_shader(const MaterialComponent& material, u32 lod_level) {
        if (lod_level < material.lod_variants.size()) {
            return material.lod_variants[lod_level];
        }
        return material.shader_handle; // Fallback to main shader
    }
    
    std::unordered_map<BatchKey, std::vector<Entity>, BatchKeyHash>
    group_entities_for_batching(const std::vector<Entity>& entities, World& world) {
        std::unordered_map<BatchKey, std::vector<Entity>, BatchKeyHash> batches;
        
        for (Entity entity : entities) {
            auto* material = world.try_get_component<MaterialComponent>(entity);
            if (!material) continue;
            
            BatchKey key;
            key.shader_handle = material->shader_handle;
            key.material_id = std::hash<std::string>{}(material->material_name);
            key.is_transparent = material->is_transparent;
            
            batches[key].push_back(entity);
        }
        
        return batches;
    }
    
    std::string generate_shader_explanation(shader_runtime::ShaderRuntimeManager::ShaderHandle handle) {
        // Generate educational explanation for a shader
        // This would analyze the shader and provide educational content
        return "This shader demonstrates basic PBR lighting calculations including diffuse and specular reflection.";
    }
    
    std::string format_material_properties(const MaterialComponent& material) {
        std::ostringstream oss;
        oss << "Material: " << material.material_name << "\n";
        oss << "Shader Handle: " << material.shader_handle << "\n";
        oss << "Transparent: " << (material.is_transparent ? "Yes" : "No") << "\n";
        oss << "Cast Shadows: " << (material.cast_shadows ? "Yes" : "No") << "\n";
        oss << "LOD Level: " << material.current_lod_level << "\n";
        
        const auto& pbr = material.pbr_properties;
        oss << "Albedo: (" << pbr.albedo[0] << ", " << pbr.albedo[1] << ", " << pbr.albedo[2] << ")\n";
        oss << "Metallic: " << pbr.metallic << "\n";
        oss << "Roughness: " << pbr.roughness << "\n";
        
        return oss.str();
    }
    
    shader_library::PBRMaterial component_to_pbr_material(const MaterialComponent& material) {
        return material.pbr_properties;
    }
    
    MaterialComponent pbr_material_to_component(const shader_library::PBRMaterial& pbr_material, const std::string& name) {
        MaterialComponent material;
        material.pbr_properties = pbr_material;
        material.material_name = name;
        material.is_transparent = pbr_material.transmission > 0.1f;
        material.double_sided = pbr_material.double_sided;
        return material;
    }
}

} // namespace ecscope::renderer::ecs_integration