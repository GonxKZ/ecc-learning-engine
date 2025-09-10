/**
 * @file deferred_rendering_demo.cpp
 * @brief Comprehensive Deferred Rendering Pipeline Demo
 * 
 * This example demonstrates the complete modern deferred rendering pipeline
 * including G-buffer generation, PBR lighting, shadow mapping, and post-processing.
 * 
 * Features demonstrated:
 * - Multi-target G-buffer generation with PBR materials
 * - Tiled deferred shading with multiple light types
 * - Cascade shadow maps for directional lights
 * - Screen-space ambient occlusion (SSAO)
 * - Screen-space reflections (SSR)
 * - Temporal anti-aliasing (TAA)
 * - HDR pipeline with bloom and tone mapping
 * - Performance profiling and debugging tools
 * 
 * @author ECScope Development Team
 * @version 1.0.0
 */

#include "ecscope/rendering/deferred_renderer.hpp"
#include "ecscope/rendering/renderer.hpp"
#include "ecscope/rendering/render_graph.hpp"
#include <iostream>
#include <memory>
#include <vector>
#include <random>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

using namespace ecscope::rendering;

/**
 * @brief Demo scene data
 */
struct DemoScene {
    // Camera data
    struct {
        std::array<float, 3> position = {0.0f, 5.0f, 10.0f};
        std::array<float, 3> target = {0.0f, 0.0f, 0.0f};
        std::array<float, 3> up = {0.0f, 1.0f, 0.0f};
        float fov = 45.0f;
        float near_plane = 0.1f;
        float far_plane = 1000.0f;
    } camera;
    
    // Scene objects
    struct SceneObject {
        std::array<float, 16> transform;
        MaterialProperties material;
        BufferHandle vertex_buffer;
        BufferHandle index_buffer;
        uint32_t index_count;
    };
    
    std::vector<SceneObject> objects;
    std::vector<Light> lights;
    EnvironmentLighting environment;
};

/**
 * @brief Create a simple cube mesh
 */
std::pair<BufferHandle, BufferHandle> create_cube_mesh(IRenderer* renderer, uint32_t& index_count) {
    // Cube vertices (position, normal, texcoord, tangent)
    struct Vertex {
        float position[3];
        float normal[3];
        float texcoord[2];
        float tangent[3];
    };
    
    std::vector<Vertex> vertices = {
        // Front face
        {{-1.0f, -1.0f,  1.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}},
        {{ 1.0f, -1.0f,  1.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}},
        {{ 1.0f,  1.0f,  1.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}, {1.0f, 0.0f, 0.0f}},
        {{-1.0f,  1.0f,  1.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}, {1.0f, 0.0f, 0.0f}},
        
        // Back face
        {{ 1.0f, -1.0f, -1.0f}, {0.0f, 0.0f, -1.0f}, {0.0f, 0.0f}, {-1.0f, 0.0f, 0.0f}},
        {{-1.0f, -1.0f, -1.0f}, {0.0f, 0.0f, -1.0f}, {1.0f, 0.0f}, {-1.0f, 0.0f, 0.0f}},
        {{-1.0f,  1.0f, -1.0f}, {0.0f, 0.0f, -1.0f}, {1.0f, 1.0f}, {-1.0f, 0.0f, 0.0f}},
        {{ 1.0f,  1.0f, -1.0f}, {0.0f, 0.0f, -1.0f}, {0.0f, 1.0f}, {-1.0f, 0.0f, 0.0f}},
        
        // Add other faces... (truncated for brevity)
    };
    
    std::vector<uint32_t> indices = {
        0, 1, 2, 2, 3, 0,       // Front face
        4, 5, 6, 6, 7, 4,       // Back face
        8, 9, 10, 10, 11, 8,    // Top face
        12, 13, 14, 14, 15, 12, // Bottom face
        16, 17, 18, 18, 19, 16, // Right face
        20, 21, 22, 22, 23, 20  // Left face
    };
    
    index_count = static_cast<uint32_t>(indices.size());
    
    // Create vertex buffer
    BufferDesc vertex_desc;
    vertex_desc.size = sizeof(Vertex) * vertices.size();
    vertex_desc.usage = BufferUsage::Static;
    vertex_desc.debug_name = "Cube Vertices";
    auto vertex_buffer = renderer->create_buffer(vertex_desc, vertices.data());
    
    // Create index buffer
    BufferDesc index_desc;
    index_desc.size = sizeof(uint32_t) * indices.size();
    index_desc.usage = BufferUsage::Static;
    index_desc.debug_name = "Cube Indices";
    auto index_buffer = renderer->create_buffer(index_desc, indices.data());
    
    return {vertex_buffer, index_buffer};
}

/**
 * @brief Create a test scene with multiple objects and lights
 */
DemoScene create_demo_scene(IRenderer* renderer) {
    DemoScene scene;
    
    // Create cube mesh
    uint32_t cube_index_count = 0;
    auto [cube_vertex_buffer, cube_index_buffer] = create_cube_mesh(renderer, cube_index_count);
    
    // Create multiple objects with different materials
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> pos_dist(-10.0f, 10.0f);
    std::uniform_real_distribution<float> mat_dist(0.0f, 1.0f);
    
    for (int i = 0; i < 20; ++i) {
        DemoScene::SceneObject obj;
        
        // Random position
        float x = pos_dist(gen);
        float y = pos_dist(gen);
        float z = pos_dist(gen);
        
        // Identity transform with translation
        obj.transform = {\n            1.0f, 0.0f, 0.0f, 0.0f,\n            0.0f, 1.0f, 0.0f, 0.0f,\n            0.0f, 0.0f, 1.0f, 0.0f,\n               x,    y,    z, 1.0f\n        };\n        \n        // Random PBR material\n        obj.material.albedo = {mat_dist(gen), mat_dist(gen), mat_dist(gen)};\n        obj.material.metallic = mat_dist(gen);\n        obj.material.roughness = mat_dist(gen) * 0.8f + 0.1f; // 0.1 to 0.9\n        obj.material.normal_intensity = 1.0f;\n        obj.material.emission_intensity = (i % 5 == 0) ? mat_dist(gen) : 0.0f; // Some emissive\n        obj.material.emission_color = {1.0f, 0.5f, 0.2f};\n        obj.material.ambient_occlusion = 1.0f;\n        \n        obj.vertex_buffer = cube_vertex_buffer;\n        obj.index_buffer = cube_index_buffer;\n        obj.index_count = cube_index_count;\n        \n        scene.objects.push_back(obj);\n    }\n    \n    // Create directional light (sun)\n    Light sun_light;\n    sun_light.type = LightType::Directional;\n    sun_light.direction = {-0.3f, -0.7f, -0.6f};\n    sun_light.color = {1.0f, 0.95f, 0.8f};\n    sun_light.intensity = 3.0f;\n    sun_light.cast_shadows = true;\n    sun_light.cascade_count = 4;\n    sun_light.cascade_distances = {1.0f, 5.0f, 20.0f, 100.0f, 0.0f, 0.0f, 0.0f, 0.0f};\n    scene.lights.push_back(sun_light);\n    \n    // Create multiple point lights\n    for (int i = 0; i < 8; ++i) {\n        Light point_light;\n        point_light.type = LightType::Point;\n        point_light.position = {pos_dist(gen), pos_dist(gen), pos_dist(gen)};\n        point_light.color = {mat_dist(gen), mat_dist(gen), mat_dist(gen)};\n        point_light.intensity = 2.0f + mat_dist(gen) * 3.0f;\n        point_light.range = 5.0f + mat_dist(gen) * 10.0f;\n        point_light.cast_shadows = (i % 2 == 0); // Half cast shadows\n        scene.lights.push_back(point_light);\n    }\n    \n    // Create spot lights\n    for (int i = 0; i < 4; ++i) {\n        Light spot_light;\n        spot_light.type = LightType::Spot;\n        spot_light.position = {pos_dist(gen), 5.0f + mat_dist(gen) * 5.0f, pos_dist(gen)};\n        spot_light.direction = {0.0f, -1.0f, 0.0f};\n        spot_light.color = {1.0f, 1.0f, 1.0f};\n        spot_light.intensity = 5.0f;\n        spot_light.range = 15.0f;\n        spot_light.inner_cone_angle = 15.0f;\n        spot_light.outer_cone_angle = 30.0f;\n        spot_light.cast_shadows = true;\n        scene.lights.push_back(spot_light);\n    }\n    \n    // Setup environment lighting\n    scene.environment.intensity = 0.3f;\n    scene.environment.ambient_color = {0.1f, 0.1f, 0.15f};\n    scene.environment.rotate_environment = false;\n    \n    return scene;\n}\n\n/**\n * @brief Calculate view and projection matrices\n */\nstd::pair<std::array<float, 16>, std::array<float, 16>> calculate_camera_matrices(\n    const DemoScene& scene, uint32_t width, uint32_t height) {\n    \n    // Simple lookAt matrix calculation (placeholder)\n    std::array<float, 16> view_matrix;\n    std::fill(view_matrix.begin(), view_matrix.end(), 0.0f);\n    view_matrix[0] = view_matrix[5] = view_matrix[10] = view_matrix[15] = 1.0f;\n    \n    // Simple perspective projection (placeholder)\n    std::array<float, 16> projection_matrix;\n    std::fill(projection_matrix.begin(), projection_matrix.end(), 0.0f);\n    \n    float aspect = static_cast<float>(width) / static_cast<float>(height);\n    float fov_rad = scene.camera.fov * static_cast<float>(M_PI) / 180.0f;\n    float f = 1.0f / std::tan(fov_rad * 0.5f);\n    \n    projection_matrix[0] = f / aspect;\n    projection_matrix[5] = f;\n    projection_matrix[10] = (scene.camera.far_plane + scene.camera.near_plane) / \n                           (scene.camera.near_plane - scene.camera.far_plane);\n    projection_matrix[11] = -1.0f;\n    projection_matrix[14] = (2.0f * scene.camera.far_plane * scene.camera.near_plane) / \n                           (scene.camera.near_plane - scene.camera.far_plane);\n    \n    return {view_matrix, projection_matrix};\n}\n\n/**\n * @brief Render the scene using deferred rendering\n */\nvoid render_scene(DeferredRenderer& deferred_renderer, const DemoScene& scene,\n                 const std::array<float, 16>& view_matrix,\n                 const std::array<float, 16>& projection_matrix) {\n    \n    // Begin frame\n    deferred_renderer.begin_frame();\n    \n    // Set camera\n    deferred_renderer.set_camera(view_matrix, projection_matrix);\n    \n    // Set environment\n    deferred_renderer.set_environment(scene.environment);\n    \n    // Submit all lights\n    for (const auto& light : scene.lights) {\n        deferred_renderer.submit_light(light);\n    }\n    \n    // Submit all geometry\n    for (const auto& object : scene.objects) {\n        deferred_renderer.submit_geometry(\n            object.vertex_buffer,\n            object.index_buffer,\n            object.material,\n            object.transform,\n            object.index_count\n        );\n    }\n    \n    // End frame (executes all render passes)\n    deferred_renderer.end_frame();\n}\n\n/**\n * @brief Print performance statistics\n */\nvoid print_statistics(const DeferredRenderer& deferred_renderer) {\n    auto stats = deferred_renderer.get_statistics();\n    \n    std::cout << \"\\n=== Deferred Renderer Statistics ===\\n\";\n    std::cout << \"Geometry Draw Calls: \" << stats.geometry_draw_calls << \"\\n\";\n    std::cout << \"Light Count: \" << stats.light_count << \"\\n\";\n    std::cout << \"Shadow Map Updates: \" << stats.shadow_map_updates << \"\\n\";\n    std::cout << \"\\nTiming (ms):\\n\";\n    std::cout << \"  Geometry Pass: \" << stats.geometry_pass_time_ms << \"\\n\";\n    std::cout << \"  Shadow Pass: \" << stats.shadow_pass_time_ms << \"\\n\";\n    std::cout << \"  Lighting Pass: \" << stats.lighting_pass_time_ms << \"\\n\";\n    std::cout << \"  Post-Process: \" << stats.post_process_time_ms << \"\\n\";\n    std::cout << \"\\nMemory Usage:\\n\";\n    std::cout << \"  G-Buffer: \" << stats.g_buffer_memory_mb << \" MB\\n\";\n    std::cout << \"  Shadow Maps: \" << stats.shadow_memory_mb << \" MB\\n\";\n    std::cout << \"\\n\";\n}\n\n/**\n * @brief Demonstrate render graph usage\n */\nvoid demonstrate_render_graph(IRenderer* renderer) {\n    std::cout << \"\\n=== Render Graph Demo ===\\n\";\n    \n    RenderGraph graph(renderer);\n    RenderGraphBuilder builder(&graph);\n    \n    // Build a simple render graph\n    builder.texture(\"GBuffer_Albedo\", TextureDesc{1920, 1080, 1, 1, 1, TextureFormat::RGBA8, 1, true})\n           .texture(\"GBuffer_Normal\", TextureDesc{1920, 1080, 1, 1, 1, TextureFormat::RGBA16F, 1, true})\n           .texture(\"DepthBuffer\", TextureDesc{1920, 1080, 1, 1, 1, TextureFormat::Depth24Stencil8, 1, true, true})\n           .texture(\"HDR_Target\", TextureDesc{1920, 1080, 1, 1, 1, TextureFormat::RGBA16F, 1, true})\n           .pass(\"GeometryPass\", \n                 {}, // No inputs\n                 {{\"GBuffer_Albedo\", ResourceAccess::Write}, {\"GBuffer_Normal\", ResourceAccess::Write}, {\"DepthBuffer\", ResourceAccess::Write}},\n                 [](RenderPassContext& ctx) {\n                     std::cout << \"Executing Geometry Pass\\n\";\n                     // Render geometry to G-buffer\n                 })\n           .pass(\"LightingPass\",\n                 {{\"GBuffer_Albedo\", ResourceAccess::Read}, {\"GBuffer_Normal\", ResourceAccess::Read}, {\"DepthBuffer\", ResourceAccess::Read}},\n                 {{\"HDR_Target\", ResourceAccess::Write}},\n                 [](RenderPassContext& ctx) {\n                     std::cout << \"Executing Lighting Pass\\n\";\n                     // Perform deferred lighting\n                 })\n           .pass(\"PostProcessPass\",\n                 {{\"HDR_Target\", ResourceAccess::Read}},\n                 {}, // Writes to backbuffer\n                 [](RenderPassContext& ctx) {\n                     std::cout << \"Executing Post-Process Pass\\n\";\n                     // Tone mapping and post-processing\n                 });\n    \n    // Compile and execute\n    if (builder.compile()) {\n        std::cout << \"Render graph compiled successfully\\n\";\n        graph.execute();\n        \n        // Print statistics\n        auto stats = graph.get_statistics();\n        std::cout << \"Graph Statistics:\\n\";\n        std::cout << \"  Total Passes: \" << stats.total_passes << \"\\n\";\n        std::cout << \"  Culled Passes: \" << stats.culled_passes << \"\\n\";\n        std::cout << \"  Total Resources: \" << stats.total_resources << \"\\n\";\n        std::cout << \"  Aliased Resources: \" << stats.aliased_resources << \"\\n\";\n        std::cout << \"  Memory Used: \" << stats.memory_used << \" bytes\\n\";\n        std::cout << \"  Memory Saved: \" << stats.memory_saved << \" bytes\\n\";\n    } else {\n        std::cout << \"Failed to compile render graph\\n\";\n    }\n}\n\nint main() {\n    std::cout << \"ECScope Modern Deferred Rendering Pipeline Demo\\n\";\n    std::cout << \"===============================================\\n\";\n    \n    try {\n        // Create renderer (would typically be done with a window)\n        auto renderer = RendererFactory::create(RenderingAPI::Auto, nullptr);\n        if (!renderer) {\n            std::cerr << \"Failed to create renderer\\n\";\n            return -1;\n        }\n        \n        std::cout << \"Created renderer: \" << RendererFactory::api_to_string(renderer->get_api()) << \"\\n\";\n        \n        // Get renderer capabilities\n        auto caps = renderer->get_capabilities();\n        std::cout << \"\\nRenderer Capabilities:\\n\";\n        std::cout << \"  Max Texture Size: \" << caps.max_texture_size << \"\\n\";\n        std::cout << \"  Max MSAA Samples: \" << caps.max_msaa_samples << \"\\n\";\n        std::cout << \"  Compute Shaders: \" << (caps.supports_compute_shaders ? \"Yes\" : \"No\") << \"\\n\";\n        std::cout << \"  Bindless Resources: \" << (caps.supports_bindless_resources ? \"Yes\" : \"No\") << \"\\n\";\n        \n        // Create deferred renderer\n        DeferredRenderer deferred_renderer(renderer.get());\n        \n        // Configure deferred renderer\n        DeferredConfig config = optimize_g_buffer_format(renderer.get(), 1920, 1080);\n        config.enable_screen_space_reflections = true;\n        config.enable_temporal_effects = true;\n        config.enable_volumetric_lighting = false;\n        config.use_compute_shading = caps.supports_compute_shaders;\n        config.max_lights_per_tile = 256;\n        config.tile_size = 16;\n        \n        if (!deferred_renderer.initialize(config)) {\n            std::cerr << \"Failed to initialize deferred renderer\\n\";\n            return -1;\n        }\n        \n        std::cout << \"\\nDeferred renderer initialized successfully\\n\";\n        \n        // Create demo scene\n        auto scene = create_demo_scene(renderer.get());\n        std::cout << \"Created demo scene with \" << scene.objects.size() << \" objects and \" \n                  << scene.lights.size() << \" lights\\n\";\n        \n        // Calculate camera matrices\n        auto [view_matrix, projection_matrix] = calculate_camera_matrices(scene, 1920, 1080);\n        \n        // Render multiple frames for timing\n        std::cout << \"\\nRendering frames...\\n\";\n        \n        const int num_frames = 10;\n        for (int frame = 0; frame < num_frames; ++frame) {\n            std::cout << \"Frame \" << (frame + 1) << \"/\" << num_frames << \"\\n\";\n            \n            // Update scene (animate lights, objects)\n            float time = static_cast<float>(frame) * 0.1f;\n            for (auto& light : scene.lights) {\n                if (light.type == LightType::Point) {\n                    // Animate point lights\n                    light.position[0] += std::sin(time + light.position[0]) * 0.1f;\n                    light.position[2] += std::cos(time + light.position[2]) * 0.1f;\n                }\n            }\n            \n            // Render frame\n            render_scene(deferred_renderer, scene, view_matrix, projection_matrix);\n        }\n        \n        // Print final statistics\n        print_statistics(deferred_renderer);\n        \n        // Demonstrate render graph\n        demonstrate_render_graph(renderer.get());\n        \n        std::cout << \"\\n=== G-Buffer Visualization Test ===\\n\";\n        deferred_renderer.render_g_buffer_debug();\n        std::cout << \"G-buffer debug visualization rendered\\n\";\n        \n        std::cout << \"\\n=== Light Complexity Visualization ===\\n\";\n        deferred_renderer.render_light_complexity();\n        std::cout << \"Light complexity heatmap rendered\\n\";\n        \n        std::cout << \"\\nDemo completed successfully!\\n\";\n        \n    } catch (const std::exception& e) {\n        std::cerr << \"Error: \" << e.what() << \"\\n\";\n        return -1;\n    }\n    \n    return 0;\n}