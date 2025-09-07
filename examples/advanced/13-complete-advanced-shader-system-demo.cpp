/**
 * @file 13-complete-advanced-shader-system-demo.cpp
 * @brief Complete Advanced Shader System Integration Demo for ECScope
 * 
 * This comprehensive demo showcases the complete advanced shader system including:
 * - Shader compiler with GLSL/HLSL/SPIR-V support
 * - Visual node-based shader editor
 * - Real-time hot-reload and caching
 * - Comprehensive PBR shader library
 * - Advanced debugging and profiling tools
 * - Full ECS integration with materials and rendering
 * - Educational features and tutorials
 * 
 * @author ECScope Educational ECS Framework - Advanced Shader System Demo
 * @date 2024
 */

#include "../../include/ecscope/world.hpp"
#include "../../include/ecscope/components.hpp"
#include "../../include/ecscope/advanced_shader_compiler.hpp"
#include "../../include/ecscope/visual_shader_editor.hpp"
#include "../../include/ecscope/shader_runtime_system.hpp"
#include "../../include/ecscope/advanced_shader_library.hpp"
#include "../../include/ecscope/shader_debugging_tools.hpp"
#include "../../include/ecscope/advanced_shader_ecs_integration.hpp"

#include <iostream>
#include <memory>
#include <vector>
#include <chrono>
#include <thread>

using namespace ecscope;
using namespace ecscope::renderer;

/**
 * @class CompleteAdvancedShaderSystemDemo
 * @brief Comprehensive demonstration of the advanced shader system
 */
class CompleteAdvancedShaderSystemDemo {
public:
    CompleteAdvancedShaderSystemDemo() 
        : world_() {
        initialize_shader_systems();
        setup_demo_scene();
        setup_educational_content();
    }
    
    void run() {
        std::cout << "\n=== ECScope Advanced Shader System Demo ===\n";
        std::cout << "Showcasing production-ready shader pipeline with educational features\n\n";
        
        // Demonstrate each major component
        demonstrate_shader_compilation();
        demonstrate_visual_shader_editor();
        demonstrate_shader_runtime();
        demonstrate_shader_library();
        demonstrate_debugging_tools();
        demonstrate_ecs_integration();
        demonstrate_performance_analysis();
        demonstrate_educational_features();
        
        // Interactive demo loop
        run_interactive_demo();
    }
    
private:
    World world_;
    
    // Core shader system components
    std::unique_ptr<shader_compiler::AdvancedShaderCompiler> compiler_;
    std::unique_ptr<visual_editor::VisualShaderEditor> visual_editor_;
    std::unique_ptr<shader_runtime::ShaderRuntimeManager> runtime_manager_;
    std::unique_ptr<shader_library::AdvancedShaderLibrary> shader_library_;
    std::unique_ptr<shader_debugging::AdvancedShaderDebugger> debugger_;
    
    // ECS integration systems
    std::unique_ptr<ecs_integration::MaterialManagementSystem> material_system_;
    std::unique_ptr<ecs_integration::AdvancedRenderingSystem> rendering_system_;
    std::unique_ptr<ecs_integration::AdvancedLightingSystem> lighting_system_;
    std::unique_ptr<ecs_integration::ShaderEducationSystem> education_system_;
    
    // Demo entities
    std::vector<Entity> demo_objects_;
    Entity camera_entity_;
    std::vector<Entity> lights_;
    
    void initialize_shader_systems() {
        std::cout << "Initializing advanced shader system components...\n";
        
        // 1. Initialize shader compiler with cross-platform support
        shader_compiler::CompilerConfig compiler_config = shader_compiler::utils::create_development_config();
        compiler_config.enable_debug_info = true;
        compiler_config.enable_validation = true;
        compiler_config.generate_reflection_data = true;
        compiler_ = std::make_unique<shader_compiler::AdvancedShaderCompiler>(compiler_config);
        
        std::cout << "  ✓ Shader compiler initialized (GLSL/HLSL/SPIR-V support)\n";
        
        // 2. Initialize runtime manager with hot-reload
        shader_runtime::ShaderRuntimeManager::RuntimeConfig runtime_config = 
            shader_runtime::utils::create_development_config();
        runtime_config.enable_hot_reload = true;
        runtime_config.enable_shader_debugging = true;
        runtime_config.educational_mode = true;
        runtime_manager_ = std::make_unique<shader_runtime::ShaderRuntimeManager>(compiler_.get(), runtime_config);
        
        std::cout << "  ✓ Runtime manager initialized with hot-reload\n";
        
        // 3. Initialize shader library
        shader_library_ = std::make_unique<shader_library::AdvancedShaderLibrary>(runtime_manager_.get());
        
        std::cout << "  ✓ Shader library initialized with PBR materials\n";
        
        // 4. Initialize visual shader editor
        visual_editor_ = std::make_unique<visual_editor::VisualShaderEditor>(compiler_.get());
        
        std::cout << "  ✓ Visual shader editor initialized\n";
        
        // 5. Initialize debugger and profiler
        shader_debugging::AdvancedShaderDebugger::DebugConfig debug_config;
        debug_config.enable_performance_profiling = true;
        debug_config.enable_educational_mode = true;
        debug_config.show_explanatory_tooltips = true;
        debugger_ = std::make_unique<shader_debugging::AdvancedShaderDebugger>(
            runtime_manager_.get(), debug_config);
        
        std::cout << "  ✓ Debugging and profiling tools initialized\n";
        
        // 6. Setup integrations
        shader_library_->register_visual_editor(visual_editor_.get());
        runtime_manager_->register_visual_editor(visual_editor_.get());
        
        std::cout << "  ✓ All system integrations established\n\n";
    }
    
    void setup_demo_scene() {
        std::cout << "Setting up comprehensive demo scene...\n";
        
        // Initialize ECS systems
        material_system_ = std::make_unique<ecs_integration::MaterialManagementSystem>(
            world_, runtime_manager_.get(), shader_library_.get());
        
        ecs_integration::AdvancedRenderingSystem::RenderingConfig render_config;
        render_config.enable_performance_tracking = true;
        render_config.show_rendering_statistics = true;
        rendering_system_ = std::make_unique<ecs_integration::AdvancedRenderingSystem>(
            world_, runtime_manager_.get(), debugger_.get(), render_config);
        
        ecs_integration::AdvancedLightingSystem::LightingConfig light_config;
        light_config.enable_shadows = true;
        light_config.enable_volumetric_lighting = true;
        lighting_system_ = std::make_unique<ecs_integration::AdvancedLightingSystem>(
            world_, runtime_manager_.get(), light_config);
        
        education_system_ = std::make_unique<ecs_integration::ShaderEducationSystem>(
            world_, runtime_manager_.get(), shader_library_.get(), visual_editor_.get());
        
        // Initialize systems
        material_system_->initialize();
        rendering_system_->initialize();
        lighting_system_->initialize();
        education_system_->initialize();
        
        // Create camera
        camera_entity_ = world_.create_entity();
        world_.add_component<components::Transform>(camera_entity_, 
            components::Transform{{0.0f, 5.0f, 10.0f}, {0.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f}});
        rendering_system_->set_camera(camera_entity_);
        
        // Create demo objects with various materials
        create_material_demo_objects();
        create_lighting_demo();
        
        std::cout << "  ✓ Demo scene created with " << demo_objects_.size() << " objects\n";
        std::cout << "  ✓ " << lights_.size() << " lights configured\n\n";
    }
    
    void create_material_demo_objects() {
        std::cout << "Creating material demonstration objects...\n";
        
        // 1. Standard PBR materials
        create_pbr_material_sphere("Standard Plastic", {0.2f, 0.6f, 0.9f}, 0.0f, 0.3f, {-4.0f, 0.0f, 0.0f});
        create_pbr_material_sphere("Brushed Metal", {0.7f, 0.7f, 0.8f}, 1.0f, 0.2f, {-2.0f, 0.0f, 0.0f});
        create_pbr_material_sphere("Rough Metal", {0.6f, 0.5f, 0.4f}, 1.0f, 0.8f, {0.0f, 0.0f, 0.0f});
        create_pbr_material_sphere("Smooth Glass", {0.9f, 0.9f, 0.9f}, 0.0f, 0.0f, {2.0f, 0.0f, 0.0f});
        create_pbr_material_sphere("Emissive", {1.0f, 0.5f, 0.2f}, 0.0f, 0.3f, {4.0f, 0.0f, 0.0f});
        
        // 2. Advanced materials with transmission
        create_glass_material_sphere("Clear Glass", {0.95f, 0.95f, 0.95f}, 0.9f, 1.5f, {-3.0f, 2.0f, 0.0f});
        create_glass_material_sphere("Colored Glass", {0.8f, 0.9f, 0.7f}, 0.8f, 1.4f, {-1.0f, 2.0f, 0.0f});
        
        // 3. Animated materials
        create_animated_material_sphere("Pulsing Emissive", {1.0f, 0.3f, 0.3f}, {1.0f, 2.0f, 0.0f});
        create_animated_material_sphere("Color Shifting", {0.5f, 0.5f, 0.5f}, {3.0f, 2.0f, 0.0f});
        
        // 4. Educational materials for tutorials
        create_tutorial_material_sphere("Normal Mapping Demo", {5.0f, 2.0f, 0.0f});
        create_tutorial_material_sphere("Parallax Mapping Demo", {7.0f, 2.0f, 0.0f});
        
        std::cout << "  ✓ " << demo_objects_.size() << " material demonstration objects created\n";
    }
    
    void create_pbr_material_sphere(const std::string& name, const std::array<f32, 3>& albedo, 
                                   f32 metallic, f32 roughness, const std::array<f32, 3>& position) {
        Entity entity = world_.create_entity();
        
        // Transform component
        world_.add_component<components::Transform>(entity,
            components::Transform{position, {0.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f}});
        
        // Render component
        world_.add_component<components::RenderComponent>(entity, components::RenderComponent{});
        
        // Material component with PBR properties
        shader_library::PBRMaterial pbr_material;
        pbr_material.albedo = albedo;
        pbr_material.metallic = metallic;
        pbr_material.roughness = roughness;
        pbr_material.ao = 1.0f;
        
        auto material_component = material_system_->create_pbr_material(pbr_material, name);
        world_.add_component<ecs_integration::MaterialComponent>(entity, material_component);
        
        demo_objects_.push_back(entity);
    }
    
    void create_glass_material_sphere(const std::string& name, const std::array<f32, 3>& color,
                                     f32 transmission, f32 ior, const std::array<f32, 3>& position) {
        Entity entity = world_.create_entity();
        
        world_.add_component<components::Transform>(entity,
            components::Transform{position, {0.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f}});
        world_.add_component<components::RenderComponent>(entity, components::RenderComponent{});
        
        auto material_component = ecs_integration::utils::create_glass_material(color, transmission, ior);
        material_component.material_name = name;
        world_.add_component<ecs_integration::MaterialComponent>(entity, material_component);
        
        demo_objects_.push_back(entity);
    }
    
    void create_animated_material_sphere(const std::string& name, const std::array<f32, 3>& base_color,
                                        const std::array<f32, 3>& position) {
        Entity entity = world_.create_entity();
        
        world_.add_component<components::Transform>(entity,
            components::Transform{position, {0.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f}});
        world_.add_component<components::RenderComponent>(entity, components::RenderComponent{});
        
        auto material_component = ecs_integration::utils::create_emissive_material(base_color, 2.0f);
        material_component.material_name = name;
        material_component.has_animated_properties = true;
        material_component.animated_uniform_speeds["emissive_intensity"] = 2.0f;
        material_component.animated_uniform_speeds["color_shift"] = 1.0f;
        
        world_.add_component<ecs_integration::MaterialComponent>(entity, material_component);
        
        demo_objects_.push_back(entity);
    }
    
    void create_tutorial_material_sphere(const std::string& name, const std::array<f32, 3>& position) {
        Entity entity = world_.create_entity();
        
        world_.add_component<components::Transform>(entity,
            components::Transform{position, {0.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f}});
        world_.add_component<components::RenderComponent>(entity, components::RenderComponent{});
        
        // Create material from tutorial template
        std::unordered_map<std::string, std::string> tutorial_params;
        tutorial_params["detail_scale"] = "4.0";
        tutorial_params["height_scale"] = "0.05";
        
        auto material_component = material_system_->create_material_from_template(
            "Tutorial_NormalMapping", tutorial_params, name);
        material_component.show_debug_info = true;
        material_component.educational_description = "Demonstrates normal mapping technique";
        
        world_.add_component<ecs_integration::MaterialComponent>(entity, material_component);
        
        demo_objects_.push_back(entity);
    }
    
    void create_lighting_demo() {
        std::cout << "Setting up advanced lighting demonstration...\n";
        
        // 1. Main directional light (sun)
        Entity sun = world_.create_entity();
        world_.add_component<components::Transform>(sun,
            components::Transform{{0.0f, 10.0f, 5.0f}, {-45.0f, 30.0f, 0.0f}, {1.0f, 1.0f, 1.0f}});
        
        ecs_integration::AdvancedLightComponent sun_light;
        sun_light.light_data.type = shader_library::Light::Type::Directional;
        sun_light.light_data.direction = {0.3f, -0.7f, -0.2f};
        sun_light.light_data.color = {1.0f, 0.9f, 0.8f};
        sun_light.light_data.intensity = 3.0f;
        sun_light.cast_shadows = true;
        sun_light.use_cascade_shadows = true;
        world_.add_component<ecs_integration::AdvancedLightComponent>(sun, sun_light);
        lights_.push_back(sun);
        
        // 2. Colored point lights
        create_colored_point_light({-6.0f, 3.0f, 2.0f}, {1.0f, 0.2f, 0.2f}, 2.0f, "Red Light");
        create_colored_point_light({6.0f, 3.0f, 2.0f}, {0.2f, 0.2f, 1.0f}, 2.0f, "Blue Light");
        create_colored_point_light({0.0f, 6.0f, -3.0f}, {0.2f, 1.0f, 0.2f}, 1.5f, "Green Light");
        
        // 3. Animated volumetric light
        Entity volumetric = world_.create_entity();
        world_.add_component<components::Transform>(volumetric,
            components::Transform{{0.0f, 8.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f}});
        
        ecs_integration::AdvancedLightComponent vol_light;
        vol_light.light_data.type = shader_library::Light::Type::Spot;
        vol_light.light_data.direction = {0.0f, -1.0f, 0.0f};
        vol_light.light_data.color = {0.8f, 0.6f, 1.0f};
        vol_light.light_data.intensity = 4.0f;
        vol_light.light_data.inner_cone_angle = 20.0f;
        vol_light.light_data.outer_cone_angle = 35.0f;
        vol_light.enable_volumetrics = true;
        vol_light.volumetric_density = 0.3f;
        vol_light.animate_intensity = true;
        vol_light.animation_speed = 0.5f;
        world_.add_component<ecs_integration::AdvancedLightComponent>(volumetric, vol_light);
        lights_.push_back(volumetric);
        
        std::cout << "  ✓ Advanced lighting setup complete\n";
    }
    
    void create_colored_point_light(const std::array<f32, 3>& position, const std::array<f32, 3>& color, 
                                   f32 intensity, const std::string& name) {
        Entity light = world_.create_entity();
        world_.add_component<components::Transform>(light,
            components::Transform{position, {0.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f}});
        
        ecs_integration::AdvancedLightComponent light_comp;
        light_comp.light_data.type = shader_library::Light::Type::Point;
        light_comp.light_data.position = position;
        light_comp.light_data.color = color;
        light_comp.light_data.intensity = intensity;
        light_comp.light_data.range = 10.0f;
        light_comp.cast_shadows = true;
        world_.add_component<ecs_integration::AdvancedLightComponent>(light, light_comp);
        
        lights_.push_back(light);
    }
    
    void setup_educational_content() {
        std::cout << "Setting up educational content and tutorials...\n";
        
        // Start with basic shader tutorial
        education_system_->start_tutorial("BasicPBRLighting");
        
        std::cout << "  ✓ Educational tutorials initialized\n\n";
    }
    
    void demonstrate_shader_compilation() {
        std::cout << "=== Shader Compilation System Demo ===\n";
        
        // 1. Basic GLSL compilation
        std::string simple_vertex = R"(
#version 330 core
layout (location = 0) in vec3 a_position;
uniform mat4 u_mvp;
void main() {
    gl_Position = u_mvp * vec4(a_position, 1.0);
}
)";
        
        std::string simple_fragment = R"(
#version 330 core
out vec4 fragColor;
void main() {
    fragColor = vec4(1.0, 0.5, 0.2, 1.0);
}
)";
        
        auto vertex_result = compiler_->compile_shader(simple_vertex, resources::ShaderStage::Vertex);
        auto fragment_result = compiler_->compile_shader(simple_fragment, resources::ShaderStage::Fragment);
        
        std::cout << "GLSL Compilation Results:\n";
        std::cout << "  Vertex Shader: " << (vertex_result.success ? "SUCCESS" : "FAILED") << "\n";
        std::cout << "  Fragment Shader: " << (fragment_result.success ? "SUCCESS" : "FAILED") << "\n";
        std::cout << "  Compilation Time: " << 
            (vertex_result.performance.compilation_time + fragment_result.performance.compilation_time) << " ms\n";
        
        // 2. Cross-compilation demo
        if (vertex_result.success && !vertex_result.bytecode.empty()) {
            auto hlsl_result = compiler_->cross_compile(vertex_result.bytecode, 
                                                       shader_compiler::ShaderLanguage::HLSL,
                                                       resources::ShaderStage::Vertex);
            std::cout << "  Cross-compilation to HLSL: " << (hlsl_result.success ? "SUCCESS" : "FAILED") << "\n";
        }
        
        // 3. Performance analysis
        if (fragment_result.success) {
            std::cout << "  Performance Analysis:\n";
            std::cout << "    Estimated GPU cost: " << fragment_result.performance.estimated_gpu_cost << "x\n";
            std::cout << "    Instruction count: " << fragment_result.performance.instruction_count << "\n";
            std::cout << fragment_result.performance.performance_analysis << "\n";
        }
        
        std::cout << "\n";
    }
    
    void demonstrate_visual_shader_editor() {
        std::cout << "=== Visual Shader Editor Demo ===\n";
        
        // Create a simple shader graph
        visual_editor_->new_graph(resources::ShaderStage::Fragment);
        
        auto* graph = visual_editor_->get_current_graph();
        if (graph) {
            std::cout << "Created new shader graph: " << graph->name << "\n";
            std::cout << "Generated GLSL code preview:\n";
            
            std::string glsl_code = graph->compile_to_glsl();
            std::cout << "```glsl\n" << glsl_code.substr(0, 300) << "...\n```\n";
            
            std::cout << "Graph explanation:\n" << graph->generate_explanation() << "\n";
            
            auto optimization_tips = graph->get_optimization_suggestions();
            if (!optimization_tips.empty()) {
                std::cout << "Optimization suggestions:\n";
                for (const auto& tip : optimization_tips) {
                    std::cout << "  - " << tip << "\n";
                }
            }
        }
        
        std::cout << "\n";
    }
    
    void demonstrate_shader_runtime() {
        std::cout << "=== Shader Runtime System Demo ===\n";
        
        // Load a shader from the library
        auto pbr_shader = shader_library_->create_pbr_shader(
            shader_library::PBRMaterial{}, 
            shader_library::LightingModel::PBR_MetallicRoughness,
            "Demo PBR Shader");
        
        if (pbr_shader != shader_runtime::ShaderRuntimeManager::INVALID_SHADER_HANDLE) {
            std::cout << "Created PBR shader with handle: " << pbr_shader << "\n";
            
            auto state = runtime_manager_->get_shader_state(pbr_shader);
            std::cout << "Shader state: " << (state == shader_runtime::ShaderState::Ready ? "Ready" : "Loading") << "\n";
            
            auto metadata = runtime_manager_->get_shader_metadata(pbr_shader);
            if (metadata) {
                std::cout << "Shader metadata:\n";
                std::cout << "  Name: " << metadata->name << "\n";
                std::cout << "  Description: " << metadata->description << "\n";
                std::cout << "  Author: " << metadata->author << "\n";
            }
        }
        
        // Demonstrate hot-reload capability
        std::cout << "Hot-reload system: " << (runtime_manager_->get_config().enable_hot_reload ? "ENABLED" : "DISABLED") << "\n";
        
        // Show runtime statistics
        auto stats = runtime_manager_->get_runtime_statistics();
        std::cout << "Runtime Statistics:\n";
        std::cout << "  Total shaders: " << stats.total_shaders << "\n";
        std::cout << "  Compiled shaders: " << stats.compiled_shaders << "\n";
        std::cout << "  Cache hit ratio: " << (stats.cache_hit_ratio * 100.0f) << "%\n";
        std::cout << "  Average compile time: " << stats.avg_compile_time << " ms\n";
        std::cout << "\n";
    }
    
    void demonstrate_shader_library() {
        std::cout << "=== Shader Library Demo ===\n";
        
        // Show available templates
        auto categories = shader_library_->get_available_categories();
        std::cout << "Available shader categories (" << categories.size() << "):\n";
        for (auto category : categories) {
            auto templates = shader_library_->get_template_names(category);
            std::cout << "  " << static_cast<int>(category) << ": " << templates.size() << " templates\n";
        }
        
        // Analyze a shader template
        auto analysis = shader_library_->analyze_shader_template("PBR_Standard");
        std::cout << "\nPBR Standard Template Analysis:\n";
        std::cout << "  Complexity score: " << analysis.complexity_score << "/100\n";
        std::cout << "  Performance rating: " << analysis.performance_rating << "/100\n";
        std::cout << "  Texture samples: " << analysis.texture_samples << "\n";
        std::cout << "  Math operations: " << analysis.math_operations << "\n";
        std::cout << "  Mobile friendly: " << (analysis.is_mobile_friendly() ? "Yes" : "No") << "\n";
        
        if (!analysis.optimization_suggestions.empty()) {
            std::cout << "  Optimization suggestions:\n";
            for (const auto& suggestion : analysis.optimization_suggestions) {
                std::cout << "    - " << suggestion << "\n";
            }
        }
        
        // Show library statistics
        auto lib_stats = shader_library_->get_library_statistics();
        std::cout << "\nLibrary Statistics:\n";
        std::cout << "  Total templates: " << lib_stats.total_templates << "\n";
        std::cout << "  Educational templates: " << lib_stats.educational_templates << "\n";
        std::cout << "  Created shaders: " << lib_stats.created_shaders << "\n";
        std::cout << "  PBR shaders: " << lib_stats.pbr_shaders << "\n";
        std::cout << "  Average complexity: " << lib_stats.average_complexity << "\n";
        std::cout << "\n";
    }
    
    void demonstrate_debugging_tools() {
        std::cout << "=== Shader Debugging Tools Demo ===\n";
        
        // Start debug session
        debugger_->start_debug_session("Demo Session");
        
        // Demonstrate performance profiling
        auto* profiler = debugger_->get_profiler();
        profiler->begin_frame();
        profiler->begin_event("Render Demo Objects");
        
        // Simulate some work
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
        
        profiler->end_event("Render Demo Objects");
        profiler->end_frame();
        
        // Get performance statistics
        auto perf_stats = profiler->calculate_statistics(1);
        std::cout << "Performance Statistics:\n";
        std::cout << "  Average frame time: " << perf_stats.average_frame_time << " ms\n";
        std::cout << "  Average CPU time: " << perf_stats.average_cpu_time << " ms\n";
        std::cout << "  Average GPU time: " << perf_stats.average_gpu_time << " ms\n";
        
        // Detect performance issues
        auto issues = debugger_->detect_performance_issues();
        if (!issues.empty()) {
            std::cout << "Performance Issues Detected:\n";
            for (const auto& issue : issues) {
                std::cout << "  - " << issue.description << " (Impact: " << issue.impact_score << ")\n";
                std::cout << "    Suggested fix: " << issue.suggested_fix << "\n";
            }
        } else {
            std::cout << "No performance issues detected.\n";
        }
        
        // Show debug overlay capabilities
        auto* overlay = debugger_->get_overlay();
        std::cout << "\nDebug Overlay Features:\n";
        std::cout << "  Variable watch: " << 
            (overlay->is_overlay_enabled(shader_debugging::ShaderDebugOverlay::OverlayType::VariableWatch) ? "ON" : "OFF") << "\n";
        std::cout << "  Performance graphs: " << 
            (overlay->is_overlay_enabled(shader_debugging::ShaderDebugOverlay::OverlayType::PerformanceGraph) ? "ON" : "OFF") << "\n";
        std::cout << "  Memory visualization: " << 
            (overlay->is_overlay_enabled(shader_debugging::ShaderDebugOverlay::OverlayType::MemoryUsage) ? "ON" : "OFF") << "\n";
        
        debugger_->end_debug_session();
        std::cout << "\n";
    }
    
    void demonstrate_ecs_integration() {
        std::cout << "=== ECS Integration Demo ===\n";
        
        // Update all systems to show integration
        material_system_->update(0.016f); // 60fps
        lighting_system_->update(0.016f);
        rendering_system_->update(0.016f);
        education_system_->update(0.016f);
        
        // Show material system statistics
        auto material_report = material_system_->generate_performance_report();
        std::cout << "Material System Report:\n";
        std::cout << "  Total materials: " << material_report.total_materials << "\n";
        std::cout << "  Unique shaders: " << material_report.unique_shaders << "\n";
        std::cout << "  Animated materials: " << material_report.animated_materials << "\n";
        std::cout << "  Average uniform updates/frame: " << material_report.average_uniform_updates_per_frame << "\n";
        
        // Show rendering system statistics
        auto render_report = rendering_system_->generate_performance_report();
        std::cout << "\nRendering System Report:\n";
        std::cout << "  Total entities: " << render_report.total_entities << "\n";
        std::cout << "  Rendered entities: " << render_report.rendered_entities << "\n";
        std::cout << "  Culled entities: " << render_report.culled_entities << "\n";
        std::cout << "  Draw calls: " << render_report.draw_calls << "\n";
        std::cout << "  Batched draw calls saved: " << render_report.batched_draw_calls << "\n";
        std::cout << "  Culling time: " << render_report.culling_time << " ms\n";
        std::cout << "  Rendering time: " << render_report.rendering_time << " ms\n";
        
        if (!render_report.bottlenecks.empty()) {
            std::cout << "  Bottlenecks detected:\n";
            for (const auto& bottleneck : render_report.bottlenecks) {
                std::cout << "    - " << bottleneck << "\n";
            }
        }
        
        std::cout << "\n";
    }
    
    void demonstrate_performance_analysis() {
        std::cout << "=== Performance Analysis Demo ===\n";
        
        // Analyze individual materials
        for (size_t i = 0; i < std::min(size_t(3), demo_objects_.size()); ++i) {
            Entity entity = demo_objects_[i];
            auto* material = world_.try_get_component<ecs_integration::MaterialComponent>(entity);
            
            if (material) {
                std::cout << "Material '" << material->material_name << "' Analysis:\n";
                
                auto performance_tips = ecs_integration::utils::analyze_material_performance(*material);
                if (!performance_tips.empty()) {
                    for (const auto& tip : performance_tips) {
                        std::cout << "  - " << tip << "\n";
                    }
                } else {
                    std::cout << "  - No performance issues detected\n";
                }
                
                std::cout << "  Properties:\n";
                std::string props = ecs_integration::utils::format_material_properties(*material);
                std::cout << "    " << props.substr(0, 100) << "...\n";
            }
        }
        
        std::cout << "\n";
    }
    
    void demonstrate_educational_features() {
        std::cout << "=== Educational Features Demo ===\n";
        
        // Show available tutorials
        auto tutorials = education_system_->get_available_tutorials();
        std::cout << "Available Tutorials (" << tutorials.size() << "):\n";
        for (const auto& tutorial : tutorials) {
            std::cout << "  - " << tutorial << "\n";
        }
        
        // Generate shader explanations
        if (!demo_objects_.empty()) {
            auto* material = world_.try_get_component<ecs_integration::MaterialComponent>(demo_objects_[0]);
            if (material) {
                std::string explanation = ecs_integration::utils::generate_shader_explanation(material->shader_handle);
                std::cout << "\nShader Explanation for '" << material->material_name << "':\n";
                std::cout << explanation << "\n";
                
                auto learning_objectives = ecs_integration::utils::get_shader_learning_objectives(material->shader_handle);
                if (!learning_objectives.empty()) {
                    std::cout << "Learning Objectives:\n";
                    for (const auto& objective : learning_objectives) {
                        std::cout << "  - " << objective << "\n";
                    }
                }
            }
        }
        
        std::cout << "\n";
    }
    
    void run_interactive_demo() {
        std::cout << "=== Interactive Demo Loop ===\n";
        std::cout << "Running real-time updates for 5 seconds...\n";
        
        const int update_count = 300; // 5 seconds at 60fps
        auto start_time = std::chrono::steady_clock::now();
        
        for (int frame = 0; frame < update_count; ++frame) {
            f32 delta_time = 1.0f / 60.0f;
            f32 time = frame * delta_time;
            
            // Update transform animations
            for (size_t i = 0; i < demo_objects_.size(); ++i) {
                Entity entity = demo_objects_[i];
                auto* transform = world_.try_get_component<components::Transform>(entity);
                if (transform) {
                    // Gentle rotation
                    transform->rotation[1] = time * 30.0f + i * 45.0f;
                    
                    // Subtle bobbing motion
                    transform->position[1] = std::sin(time * 2.0f + i) * 0.2f;
                }
            }
            
            // Update all systems
            runtime_manager_->update();
            material_system_->update(delta_time);
            lighting_system_->update(delta_time);
            rendering_system_->update(delta_time);
            education_system_->update(delta_time);
            debugger_->update();
            
            // Show progress every second
            if (frame % 60 == 0) {
                std::cout << "  Frame " << frame << "/300 - " << (frame / 60 + 1) << "s elapsed\n";
            }
            
            // Simulate frame timing
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        
        auto end_time = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        
        std::cout << "Interactive demo completed in " << duration.count() << " ms\n";
        std::cout << "Average frame time: " << (duration.count() / static_cast<f32>(update_count)) << " ms\n";
    }
};

int main() {
    try {
        CompleteAdvancedShaderSystemDemo demo;
        demo.run();
        
        std::cout << "\n=== Demo Complete ===\n";
        std::cout << "The advanced shader system demonstrated:\n";
        std::cout << "✓ Cross-platform shader compilation (GLSL/HLSL/SPIR-V)\n";
        std::cout << "✓ Visual node-based shader editor\n";
        std::cout << "✓ Real-time hot-reload and binary caching\n";
        std::cout << "✓ Comprehensive PBR shader library\n";
        std::cout << "✓ Advanced debugging and profiling tools\n";
        std::cout << "✓ Full ECS integration with materials and rendering\n";
        std::cout << "✓ Educational features and interactive tutorials\n";
        std::cout << "✓ Production-ready performance optimizations\n";
        
        return 0;
    }
    catch (const std::exception& e) {
        std::cerr << "Demo failed with exception: " << e.what() << std::endl;
        return 1;
    }
}