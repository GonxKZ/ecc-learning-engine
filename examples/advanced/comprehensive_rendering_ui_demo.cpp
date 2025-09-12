/**
 * @file comprehensive_rendering_ui_demo.cpp
 * @brief Comprehensive Demo of Professional Rendering System UI
 * 
 * This example demonstrates the complete professional rendering pipeline control UI
 * featuring real-time parameter adjustment, visual debugging tools, scene management,
 * and performance optimization for the ECScope engine.
 * 
 * Features demonstrated:
 * - Complete rendering pipeline control interface
 * - Real-time parameter adjustment with live preview
 * - PBR material editor with multiple material presets
 * - Advanced post-processing stack (HDR, bloom, SSAO, SSR, TAA)
 * - Shadow mapping controls with cascade visualization
 * - G-Buffer visualization and render pass debugging
 * - GPU profiling and performance monitoring
 * - Scene hierarchy with object management
 * - 3D viewport with camera controls
 * - Shader hot-reload and debugging interface
 * - Professional dashboard integration
 * 
 * @author ECScope Development Team
 * @version 1.0.0
 */

#include "../../include/ecscope/gui/rendering_ui.hpp"
#include "../../include/ecscope/gui/dashboard.hpp"
#include "../../include/ecscope/rendering/renderer.hpp"
#include "../../include/ecscope/rendering/deferred_renderer.hpp"
#include "../../include/ecscope/rendering/materials.hpp"
#include "../../include/ecscope/core/log.hpp"

#include <iostream>
#include <memory>
#include <chrono>
#include <thread>
#include <random>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Platform-specific includes (simplified for demonstration)
#ifdef _WIN32
#include <windows.h>
#endif

using namespace ecscope;
using namespace ecscope::gui;
using namespace ecscope::rendering;

/**
 * @brief Demo application class
 */
class RenderingUIDemoApp {
public:
    RenderingUIDemoApp() = default;
    ~RenderingUIDemoApp() = default;

    /**
     * @brief Initialize the demo application
     */
    bool initialize() {
        log_info("RenderingUIDemo", "Initializing Comprehensive Rendering UI Demo");
        
        // Create renderer
        renderer_ = RendererFactory::create(RenderingAPI::Auto, nullptr);
        if (!renderer_) {
            log_error("RenderingUIDemo", "Failed to create renderer");
            return false;
        }
        
        log_info("RenderingUIDemo", "Created renderer: " + RendererFactory::api_to_string(renderer_->get_api()));
        
        // Get and log renderer capabilities
        auto caps = renderer_->get_capabilities();
        log_info("RenderingUIDemo", "Renderer Capabilities:");
        log_info("RenderingUIDemo", "  Max Texture Size: " + std::to_string(caps.max_texture_size));
        log_info("RenderingUIDemo", "  Max MSAA Samples: " + std::to_string(caps.max_msaa_samples));
        log_info("RenderingUIDemo", "  Compute Shaders: " + std::string(caps.supports_compute_shaders ? "Yes" : "No"));
        log_info("RenderingUIDemo", "  Bindless Resources: " + std::string(caps.supports_bindless_resources ? "Yes" : "No"));
        log_info("RenderingUIDemo", "  Ray Tracing: " + std::string(caps.supports_ray_tracing ? "Yes" : "No"));
        
        // Create deferred renderer
        deferred_renderer_ = std::make_unique<DeferredRenderer>(renderer_.get());
        
        // Configure deferred renderer with optimal settings
        DeferredConfig config = optimize_g_buffer_format(renderer_.get(), 1920, 1080);
        config.enable_screen_space_reflections = true;
        config.enable_temporal_effects = true;
        config.enable_volumetric_lighting = caps.supports_compute_shaders;
        config.use_compute_shading = caps.supports_compute_shaders;
        config.max_lights_per_tile = caps.supports_compute_shaders ? 1024 : 256;
        config.tile_size = 16;
        config.visualize_g_buffer = false;
        config.visualize_light_complexity = false;
        config.visualize_overdraw = false;
        
        if (!deferred_renderer_->initialize(config)) {
            log_error("RenderingUIDemo", "Failed to initialize deferred renderer");
            return false;
        }
        
        log_info("RenderingUIDemo", "Deferred renderer initialized successfully");
        
        // Create dashboard
        dashboard_ = std::make_unique<Dashboard>();
        if (!dashboard_->initialize(renderer_.get())) {
            log_warning("RenderingUIDemo", "Dashboard initialization failed, continuing without dashboard integration");
        } else {
            log_info("RenderingUIDemo", "Dashboard initialized successfully");
        }
        
        // Create rendering UI
        rendering_ui_ = std::make_unique<RenderingUI>();
        if (!rendering_ui_->initialize(renderer_.get(), deferred_renderer_.get(), dashboard_.get())) {
            log_error("RenderingUIDemo", "Failed to initialize rendering UI");
            return false;
        }
        
        log_info("RenderingUIDemo", "Rendering UI initialized successfully");
        
        // Create comprehensive demo scene
        create_comprehensive_demo_scene();
        
        // Setup performance monitoring
        setup_performance_monitoring();
        
        // Register demo-specific features
        register_demo_features();
        
        initialized_ = true;
        log_info("RenderingUIDemo", "Demo application initialized successfully");
        return true;
    }
    
    /**
     * @brief Run the main demo loop
     */
    void run() {
        if (!initialized_) {
            log_error("RenderingUIDemo", "Demo not initialized");
            return;
        }
        
        log_info("RenderingUIDemo", "Starting main demo loop");
        
        auto last_time = std::chrono::steady_clock::now();
        int frame_count = 0;
        
        while (running_) {
            auto current_time = std::chrono::steady_clock::now();
            float delta_time = std::chrono::duration<float>(current_time - last_time).count();
            last_time = current_time;
            
            // Update systems
            update(delta_time);
            
            // Render frame
            render();
            
            // Frame counting
            frame_count++;
            if (frame_count % 60 == 0) {
                log_info("RenderingUIDemo", "Frame " + std::to_string(frame_count) + " rendered");
            }
            
            // Demo duration check
            auto elapsed = std::chrono::duration<float>(current_time - start_time_).count();
            if (elapsed > demo_duration_seconds_) {
                log_info("RenderingUIDemo", "Demo completed after " + std::to_string(elapsed) + " seconds");
                break;
            }
            
            // Simulate frame rate limiting
            std::this_thread::sleep_for(std::chrono::milliseconds(16)); // ~60 FPS
        }
        
        // Generate final report
        generate_demo_report();
    }
    
    /**
     * @brief Shutdown the demo application
     */
    void shutdown() {
        if (!initialized_) {
            return;
        }
        
        log_info("RenderingUIDemo", "Shutting down demo application");
        
        if (rendering_ui_) {
            rendering_ui_->shutdown();
        }
        
        if (dashboard_) {
            dashboard_->shutdown();
        }
        
        if (deferred_renderer_) {
            deferred_renderer_->shutdown();
        }
        
        renderer_.reset();
        
        initialized_ = false;
        log_info("RenderingUIDemo", "Demo application shutdown complete");
    }

private:
    /**
     * @brief Create a comprehensive demo scene
     */
    void create_comprehensive_demo_scene() {
        log_info("RenderingUIDemo", "Creating comprehensive demo scene");
        
        // Create various objects with different materials
        create_material_showcase_objects();
        
        // Create complex lighting setup
        create_advanced_lighting_setup();
        
        // Add animated elements
        create_animated_scene_elements();
        
        // Setup environment
        setup_scene_environment();
        
        log_info("RenderingUIDemo", "Demo scene created with " + 
                std::to_string(demo_objects_.size()) + " objects and " +
                std::to_string(demo_lights_.size()) + " lights");
    }
    
    /**
     * @brief Create objects showcasing different materials
     */
    void create_material_showcase_objects() {
        // Material presets for demonstration
        struct MaterialPreset {
            std::string name;
            std::array<float, 3> albedo;
            float metallic;
            float roughness;
            float emission;
            std::array<float, 3> emission_color;
        };
        
        std::vector<MaterialPreset> material_presets = {
            {"Polished Metal", {0.7f, 0.7f, 0.7f}, 1.0f, 0.1f, 0.0f, {0.0f, 0.0f, 0.0f}},
            {"Brushed Metal", {0.6f, 0.6f, 0.6f}, 1.0f, 0.3f, 0.0f, {0.0f, 0.0f, 0.0f}},
            {"Plastic Matte", {0.8f, 0.2f, 0.2f}, 0.0f, 0.8f, 0.0f, {0.0f, 0.0f, 0.0f}},
            {"Plastic Glossy", {0.2f, 0.8f, 0.2f}, 0.0f, 0.2f, 0.0f, {0.0f, 0.0f, 0.0f}},
            {"Ceramic", {0.9f, 0.9f, 0.85f}, 0.0f, 0.1f, 0.0f, {0.0f, 0.0f, 0.0f}},
            {"Rubber", {0.2f, 0.2f, 0.2f}, 0.0f, 0.9f, 0.0f, {0.0f, 0.0f, 0.0f}},
            {"Wood", {0.6f, 0.4f, 0.2f}, 0.0f, 0.8f, 0.0f, {0.0f, 0.0f, 0.0f}},
            {"Glass", {0.95f, 0.95f, 0.95f}, 0.0f, 0.05f, 0.0f, {0.0f, 0.0f, 0.0f}},
            {"Emissive Blue", {0.1f, 0.1f, 0.8f}, 0.0f, 0.2f, 2.0f, {0.3f, 0.3f, 1.0f}},
            {"Emissive Orange", {0.8f, 0.4f, 0.1f}, 0.0f, 0.2f, 1.5f, {1.0f, 0.5f, 0.1f}}
        };
        
        // Create objects in a grid layout
        int grid_size = static_cast<int>(std::ceil(std::sqrt(material_presets.size())));
        float spacing = 3.0f;
        float start_pos = -(grid_size - 1) * spacing * 0.5f;
        
        for (size_t i = 0; i < material_presets.size(); ++i) {
            const auto& preset = material_presets[i];
            
            int x = i % grid_size;
            int z = i / grid_size;
            
            SceneObject obj;
            obj.name = preset.name;
            obj.visible = true;
            obj.cast_shadows = true;
            
            // Position
            obj.transform[12] = start_pos + x * spacing;
            obj.transform[13] = 1.0f;
            obj.transform[14] = start_pos + z * spacing;
            
            // Material
            obj.material.albedo = preset.albedo;
            obj.material.metallic = preset.metallic;
            obj.material.roughness = preset.roughness;
            obj.material.emission_intensity = preset.emission;
            obj.material.emission_color = preset.emission_color;
            obj.material.normal_intensity = 1.0f;
            obj.material.ambient_occlusion = 1.0f;
            
            // Create simple cube mesh (placeholder)
            uint32_t index_count = 0;
            auto [vertex_buffer, index_buffer] = create_cube_mesh(index_count);
            obj.vertex_buffer = vertex_buffer;
            obj.index_buffer = index_buffer;
            obj.index_count = index_count;
            
            demo_objects_.push_back(obj);
            uint32_t obj_id = rendering_ui_->add_scene_object(obj);
            log_info("RenderingUIDemo", "Created object: " + preset.name + " (ID: " + std::to_string(obj_id) + ")");
        }
        
        // Add a ground plane
        SceneObject ground;
        ground.name = "Ground Plane";
        ground.visible = true;
        ground.cast_shadows = false;
        
        // Scale transform for ground
        ground.transform[0] = 20.0f;  // X scale
        ground.transform[5] = 1.0f;   // Y scale
        ground.transform[10] = 20.0f; // Z scale
        ground.transform[13] = 0.0f;  // Y position
        
        ground.material.albedo = {0.5f, 0.5f, 0.5f};
        ground.material.metallic = 0.0f;
        ground.material.roughness = 0.8f;
        
        uint32_t index_count = 0;
        auto [vertex_buffer, index_buffer] = create_plane_mesh(index_count);
        ground.vertex_buffer = vertex_buffer;
        ground.index_buffer = index_buffer;
        ground.index_count = index_count;
        
        demo_objects_.push_back(ground);
        rendering_ui_->add_scene_object(ground);
    }
    
    /**
     * @brief Create advanced lighting setup
     */
    void create_advanced_lighting_setup() {
        // Main directional light (sun)
        SceneLight sun_light;
        sun_light.name = "Main Directional Light";
        sun_light.enabled = true;
        sun_light.light_data.type = LightType::Directional;
        sun_light.light_data.direction = {-0.3f, -0.7f, -0.6f};
        sun_light.light_data.color = {1.0f, 0.95f, 0.8f};
        sun_light.light_data.intensity = 3.0f;
        sun_light.light_data.cast_shadows = true;
        sun_light.light_data.cascade_count = 4;
        sun_light.light_data.cascade_distances = {2.0f, 8.0f, 20.0f, 50.0f, 0.0f, 0.0f, 0.0f, 0.0f};
        sun_light.light_data.shadow_map_size = 2048;
        
        demo_lights_.push_back(sun_light);
        rendering_ui_->add_scene_light(sun_light);
        
        // Fill light (opposite direction, lower intensity)
        SceneLight fill_light;
        fill_light.name = "Fill Light";
        fill_light.enabled = true;
        fill_light.light_data.type = LightType::Directional;
        fill_light.light_data.direction = {0.5f, -0.3f, 0.8f};
        fill_light.light_data.color = {0.8f, 0.9f, 1.0f};
        fill_light.light_data.intensity = 0.8f;
        fill_light.light_data.cast_shadows = false;
        
        demo_lights_.push_back(fill_light);
        rendering_ui_->add_scene_light(fill_light);
        
        // Animated point lights
        std::vector<std::array<float, 3>> light_colors = {
            {1.0f, 0.2f, 0.2f}, // Red
            {0.2f, 1.0f, 0.2f}, // Green
            {0.2f, 0.2f, 1.0f}, // Blue
            {1.0f, 1.0f, 0.2f}, // Yellow
            {1.0f, 0.2f, 1.0f}, // Magenta
            {0.2f, 1.0f, 1.0f}  // Cyan
        };
        
        for (size_t i = 0; i < light_colors.size(); ++i) {
            SceneLight point_light;
            point_light.name = "Animated Point Light " + std::to_string(i + 1);
            point_light.enabled = true;
            point_light.light_data.type = LightType::Point;
            point_light.light_data.position = {
                static_cast<float>(i * 4 - 10),
                3.0f,
                2.0f
            };
            point_light.light_data.color = light_colors[i];
            point_light.light_data.intensity = 2.0f;
            point_light.light_data.range = 8.0f;
            point_light.light_data.cast_shadows = (i % 2 == 0); // Half cast shadows
            point_light.light_data.shadow_map_size = 1024;
            
            // Animation properties
            point_light.animated = true;
            point_light.animation_center = point_light.light_data.position;
            point_light.animation_radius = 2.0f + static_cast<float>(i) * 0.5f;
            point_light.animation_speed = 0.5f + static_cast<float>(i) * 0.2f;
            
            demo_lights_.push_back(point_light);
            rendering_ui_->add_scene_light(point_light);
        }
        
        // Spot lights for dramatic effect
        for (int i = 0; i < 3; ++i) {
            SceneLight spot_light;
            spot_light.name = "Spot Light " + std::to_string(i + 1);
            spot_light.enabled = true;
            spot_light.light_data.type = LightType::Spot;
            spot_light.light_data.position = {
                static_cast<float>(i * 8 - 8),
                6.0f,
                -5.0f
            };
            spot_light.light_data.direction = {0.0f, -1.0f, 0.5f};
            spot_light.light_data.color = {1.0f, 0.9f, 0.8f};
            spot_light.light_data.intensity = 4.0f;
            spot_light.light_data.range = 15.0f;
            spot_light.light_data.inner_cone_angle = 20.0f;
            spot_light.light_data.outer_cone_angle = 35.0f;
            spot_light.light_data.cast_shadows = true;
            spot_light.light_data.shadow_map_size = 1024;
            
            demo_lights_.push_back(spot_light);
            rendering_ui_->add_scene_light(spot_light);
        }
        
        log_info("RenderingUIDemo", "Created " + std::to_string(demo_lights_.size()) + " lights");
    }
    
    /**
     * @brief Create animated scene elements
     */
    void create_animated_scene_elements() {
        // Rotating demonstration objects
        for (size_t i = 0; i < 3; ++i) {
            SceneObject rotating_obj;
            rotating_obj.name = "Rotating Object " + std::to_string(i + 1);
            rotating_obj.visible = true;
            rotating_obj.cast_shadows = true;
            
            // Position above the main objects
            rotating_obj.transform[12] = static_cast<float>(i * 6 - 6);
            rotating_obj.transform[13] = 4.0f;
            rotating_obj.transform[14] = 0.0f;
            
            // Shiny metal material
            rotating_obj.material.albedo = {0.8f, 0.8f, 0.8f};
            rotating_obj.material.metallic = 1.0f;
            rotating_obj.material.roughness = 0.1f + static_cast<float>(i) * 0.1f;
            rotating_obj.material.normal_intensity = 1.0f;
            rotating_obj.material.ambient_occlusion = 1.0f;
            
            uint32_t index_count = 0;
            auto [vertex_buffer, index_buffer] = create_sphere_mesh(index_count);
            rotating_obj.vertex_buffer = vertex_buffer;
            rotating_obj.index_buffer = index_buffer;
            rotating_obj.index_count = index_count;
            
            demo_objects_.push_back(rotating_obj);
            uint32_t obj_id = rendering_ui_->add_scene_object(rotating_obj);
            animated_object_ids_.push_back(obj_id);
        }
    }
    
    /**
     * @brief Setup scene environment
     */
    void setup_scene_environment() {
        auto& config = rendering_ui_->get_config();
        
        // Environment lighting
        config.environment.ambient_color = {0.1f, 0.1f, 0.15f};
        config.environment.ambient_intensity = 0.3f;
        config.environment.sky_intensity = 1.2f;
        config.environment.enable_ibl = true;
        config.environment.ibl_intensity = 0.8f;
        config.environment.rotate_environment = true;
        config.environment.rotation_speed = 0.05f;
        
        log_info("RenderingUIDemo", "Scene environment configured");
    }
    
    /**
     * @brief Setup performance monitoring
     */
    void setup_performance_monitoring() {
        performance_targets_ = {
            {"60 FPS", 16.67f},
            {"30 FPS", 33.33f},
            {"High Quality", 20.0f},
            {"Ultra Quality", 25.0f}
        };
        
        current_performance_target_ = 0; // 60 FPS
        log_info("RenderingUIDemo", "Performance monitoring configured");
    }
    
    /**
     * @brief Register demo-specific features
     */
    void register_demo_features() {
        if (!dashboard_) return;
        
        // Demo control feature
        FeatureInfo demo_control;
        demo_control.id = "rendering_demo_control";
        demo_control.name = "Rendering Demo Control";
        demo_control.description = "Control and monitor the comprehensive rendering demo";
        demo_control.icon = "🎮";
        demo_control.category = FeatureCategory::Tools;
        demo_control.launch_callback = [this]() {
            show_demo_controls_ = !show_demo_controls_;
        };
        demo_control.status_callback = [this]() {
            return initialized_ && running_;
        };
        
        dashboard_->register_feature(demo_control);
        
        // Performance benchmark feature
        FeatureInfo perf_benchmark;
        perf_benchmark.id = "rendering_performance_benchmark";
        perf_benchmark.name = "Performance Benchmark";
        perf_benchmark.description = "Run automated performance benchmarks";
        perf_benchmark.icon = "📊";
        perf_benchmark.category = FeatureCategory::Performance;
        perf_benchmark.launch_callback = [this]() {
            start_performance_benchmark();
        };
        
        dashboard_->register_feature(perf_benchmark);
    }
    
    /**
     * @brief Update demo state
     */
    void update(float delta_time) {
        if (!initialized_) return;
        
        total_time_ += delta_time;
        
        // Update animated objects
        update_animated_objects(delta_time);
        
        // Update rendering UI
        if (rendering_ui_) {
            rendering_ui_->update(delta_time);
        }
        
        // Update dashboard
        if (dashboard_) {
            dashboard_->update(delta_time);
        }
        
        // Auto-cycle through debug modes for demonstration
        if (auto_cycle_debug_modes_) {
            auto_cycle_debug_visualization(delta_time);
        }
        
        // Auto-adjust quality settings for performance testing
        if (performance_benchmark_active_) {
            update_performance_benchmark(delta_time);
        }
    }
    
    /**
     * @brief Render the demo
     */
    void render() {
        if (!initialized_ || !renderer_) return;
        
        // Begin frame
        renderer_->begin_frame();
        
        // Render the scene through deferred renderer
        if (deferred_renderer_) {
            render_deferred_scene();
        }
        
        // Render UI
        if (rendering_ui_) {
            rendering_ui_->render();
        }
        
        // Render dashboard
        if (dashboard_) {
            dashboard_->render();
        }
        
        // Render demo-specific UI
        render_demo_ui();
        
        // End frame
        renderer_->end_frame();
    }
    
    /**
     * @brief Render scene using deferred renderer
     */
    void render_deferred_scene() {
        // Calculate camera matrices
        auto [view_matrix, projection_matrix] = calculate_camera_matrices();
        
        // Begin deferred rendering
        deferred_renderer_->begin_frame();
        deferred_renderer_->set_camera(view_matrix, projection_matrix);
        
        // Set environment
        EnvironmentLighting env;
        env.intensity = rendering_ui_->get_config().environment.sky_intensity;
        env.ambient_color = rendering_ui_->get_config().environment.ambient_color;
        env.rotate_environment = rendering_ui_->get_config().environment.rotate_environment;
        env.rotation_speed = rendering_ui_->get_config().environment.rotation_speed;
        deferred_renderer_->set_environment(env);
        
        // The scene submission is handled by RenderingUI internally
        
        // End deferred rendering
        deferred_renderer_->end_frame();
    }
    
    /**
     * @brief Update animated objects
     */
    void update_animated_objects(float delta_time) {
        // Rotate the animated objects
        for (size_t i = 0; i < animated_object_ids_.size(); ++i) {
            auto* obj = rendering_ui_->get_scene_object(animated_object_ids_[i]);
            if (obj) {
                float rotation_speed = 1.0f + static_cast<float>(i) * 0.3f;
                float angle = total_time_ * rotation_speed;
                
                // Simple rotation around Y axis (this is a simplified implementation)
                obj->transform[0] = std::cos(angle);
                obj->transform[2] = std::sin(angle);
                obj->transform[8] = -std::sin(angle);
                obj->transform[10] = std::cos(angle);
            }
        }
    }
    
    /**
     * @brief Render demo-specific UI
     */
    void render_demo_ui() {
#ifdef ECSCOPE_HAS_IMGUI
        if (show_demo_controls_) {
            render_demo_control_panel();
        }
        
        if (performance_benchmark_active_) {
            render_benchmark_results();
        }
#endif
    }
    
    /**
     * @brief Create simple cube mesh (placeholder implementation)
     */
    std::pair<BufferHandle, BufferHandle> create_cube_mesh(uint32_t& index_count) {
        // This is a placeholder - in a real implementation, you would create actual geometry
        index_count = 36; // 6 faces * 2 triangles * 3 vertices
        
        BufferDesc vertex_desc;
        vertex_desc.size = 24 * (3 + 3 + 2) * sizeof(float); // 24 vertices * (pos + normal + uv)
        vertex_desc.usage = BufferUsage::Static;
        vertex_desc.debug_name = "Cube Vertices";
        
        BufferDesc index_desc;
        index_desc.size = index_count * sizeof(uint32_t);
        index_desc.usage = BufferUsage::Static;
        index_desc.debug_name = "Cube Indices";
        
        // Create placeholder buffers
        auto vertex_buffer = renderer_->create_buffer(vertex_desc, nullptr);
        auto index_buffer = renderer_->create_buffer(index_desc, nullptr);
        
        return {vertex_buffer, index_buffer};
    }
    
    /**
     * @brief Create simple plane mesh (placeholder implementation)
     */
    std::pair<BufferHandle, BufferHandle> create_plane_mesh(uint32_t& index_count) {
        index_count = 6; // 2 triangles * 3 vertices
        
        BufferDesc vertex_desc;
        vertex_desc.size = 4 * (3 + 3 + 2) * sizeof(float); // 4 vertices
        vertex_desc.usage = BufferUsage::Static;
        vertex_desc.debug_name = "Plane Vertices";
        
        BufferDesc index_desc;
        index_desc.size = index_count * sizeof(uint32_t);
        index_desc.usage = BufferUsage::Static;
        index_desc.debug_name = "Plane Indices";
        
        auto vertex_buffer = renderer_->create_buffer(vertex_desc, nullptr);
        auto index_buffer = renderer_->create_buffer(index_desc, nullptr);
        
        return {vertex_buffer, index_buffer};
    }
    
    /**
     * @brief Create simple sphere mesh (placeholder implementation)
     */
    std::pair<BufferHandle, BufferHandle> create_sphere_mesh(uint32_t& index_count) {
        index_count = 720; // Approximation for a sphere with reasonable tessellation
        
        BufferDesc vertex_desc;
        vertex_desc.size = 242 * (3 + 3 + 2) * sizeof(float); // Approximation
        vertex_desc.usage = BufferUsage::Static;
        vertex_desc.debug_name = "Sphere Vertices";
        
        BufferDesc index_desc;
        index_desc.size = index_count * sizeof(uint32_t);
        index_desc.usage = BufferUsage::Static;
        index_desc.debug_name = "Sphere Indices";
        
        auto vertex_buffer = renderer_->create_buffer(vertex_desc, nullptr);
        auto index_buffer = renderer_->create_buffer(index_desc, nullptr);
        
        return {vertex_buffer, index_buffer};
    }
    
    /**
     * @brief Calculate camera matrices for demo
     */
    std::pair<std::array<float, 16>, std::array<float, 16>> calculate_camera_matrices() {
        // Simple orbit camera
        float camera_distance = 25.0f;
        float camera_height = 8.0f;
        float camera_angle = total_time_ * 0.2f; // Slow rotation
        
        std::array<float, 16> view_matrix = {};
        std::array<float, 16> projection_matrix = {};
        
        // Simple view matrix (looking at origin)
        float cam_x = camera_distance * std::cos(camera_angle);
        float cam_z = camera_distance * std::sin(camera_angle);
        
        // Identity matrices as placeholders
        view_matrix[0] = view_matrix[5] = view_matrix[10] = view_matrix[15] = 1.0f;
        projection_matrix[0] = projection_matrix[5] = projection_matrix[10] = projection_matrix[15] = 1.0f;
        
        return {view_matrix, projection_matrix};
    }
    
    /**
     * @brief Generate comprehensive demo report
     */
    void generate_demo_report() {
        log_info("RenderingUIDemo", "=== COMPREHENSIVE RENDERING UI DEMO REPORT ===");
        
        if (rendering_ui_) {
            auto metrics = rendering_ui_->get_metrics();
            log_info("RenderingUIDemo", "Final Performance Metrics:");
            log_info("RenderingUIDemo", "  Frame Time: " + std::to_string(metrics.frame_time_ms) + " ms");
            log_info("RenderingUIDemo", "  GPU Time: " + std::to_string(metrics.gpu_time_ms) + " ms");
            log_info("RenderingUIDemo", "  Draw Calls: " + std::to_string(metrics.draw_calls));
            log_info("RenderingUIDemo", "  Vertices Rendered: " + std::to_string(metrics.vertices_rendered));
            log_info("RenderingUIDemo", "  GPU Memory Used: " + format_memory_size(metrics.gpu_memory_used));
            
            if (deferred_renderer_) {
                auto deferred_stats = deferred_renderer_->get_statistics();
                log_info("RenderingUIDemo", "Deferred Renderer Stats:");
                log_info("RenderingUIDemo", "  Geometry Pass: " + std::to_string(deferred_stats.geometry_pass_time_ms) + " ms");
                log_info("RenderingUIDemo", "  Shadow Pass: " + std::to_string(deferred_stats.shadow_pass_time_ms) + " ms");
                log_info("RenderingUIDemo", "  Lighting Pass: " + std::to_string(deferred_stats.lighting_pass_time_ms) + " ms");
                log_info("RenderingUIDemo", "  Post Process: " + std::to_string(deferred_stats.post_process_time_ms) + " ms");
                log_info("RenderingUIDemo", "  Light Count: " + std::to_string(deferred_stats.light_count));
                log_info("RenderingUIDemo", "  Shadow Maps: " + std::to_string(deferred_stats.shadow_map_updates));
            }
        }
        
        log_info("RenderingUIDemo", "Demo Objects Created: " + std::to_string(demo_objects_.size()));
        log_info("RenderingUIDemo", "Demo Lights Created: " + std::to_string(demo_lights_.size()));
        log_info("RenderingUIDemo", "Total Runtime: " + std::to_string(total_time_) + " seconds");
        log_info("RenderingUIDemo", "Demo completed successfully!");
    }

    // Additional placeholder methods for completeness
    void auto_cycle_debug_visualization(float delta_time) { /* Implementation */ }
    void update_performance_benchmark(float delta_time) { /* Implementation */ }
    void start_performance_benchmark() { /* Implementation */ }
    void render_demo_control_panel() { /* Implementation */ }
    void render_benchmark_results() { /* Implementation */ }

    // Member variables
    bool initialized_ = false;
    bool running_ = true;
    float demo_duration_seconds_ = 60.0f; // 1 minute demo
    std::chrono::steady_clock::time_point start_time_ = std::chrono::steady_clock::now();
    
    std::unique_ptr<IRenderer> renderer_;
    std::unique_ptr<DeferredRenderer> deferred_renderer_;
    std::unique_ptr<Dashboard> dashboard_;
    std::unique_ptr<RenderingUI> rendering_ui_;
    
    std::vector<SceneObject> demo_objects_;
    std::vector<SceneLight> demo_lights_;
    std::vector<uint32_t> animated_object_ids_;
    
    float total_time_ = 0.0f;
    bool show_demo_controls_ = false;
    bool auto_cycle_debug_modes_ = true;
    bool performance_benchmark_active_ = false;
    
    std::vector<std::pair<std::string, float>> performance_targets_;
    int current_performance_target_ = 0;
};

/**
 * @brief Main entry point for the comprehensive rendering UI demo
 */
int main() {
    std::cout << "ECScope Comprehensive Rendering System UI Demo\n";
    std::cout << "============================================\n\n";
    
    try {
        RenderingUIDemoApp demo_app;
        
        if (!demo_app.initialize()) {
            std::cerr << "Failed to initialize demo application\n";
            return -1;
        }
        
        demo_app.run();
        demo_app.shutdown();
        
        std::cout << "\nDemo completed successfully!\n";
        
    } catch (const std::exception& e) {
        std::cerr << "Demo error: " << e.what() << "\n";
        return -1;
    }
    
    return 0;
}