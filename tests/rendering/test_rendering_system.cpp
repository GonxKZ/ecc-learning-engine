/**
 * @file test_rendering_system.cpp
 * @brief Comprehensive Rendering System Tests
 * 
 * Professional test suite for the rendering engine covering all major
 * components: API backends, deferred rendering, materials, and effects.
 * 
 * @author ECScope Development Team
 * @version 1.0.0
 */

#include <gtest/gtest.h>
#include <memory>
#include <array>
#include <vector>
#include <chrono>
#include <thread>

#include "../../include/ecscope/rendering/renderer.hpp"
#include "../../include/ecscope/rendering/vulkan_backend.hpp"
#include "../../include/ecscope/rendering/opengl_backend.hpp"
#include "../../include/ecscope/rendering/deferred_renderer.hpp"
#include "../../include/ecscope/rendering/materials.hpp"
#include "../../include/ecscope/rendering/advanced.hpp"

using namespace ecscope::rendering;

// =============================================================================
// TEST FIXTURES
// =============================================================================

/**
 * @brief Base rendering test fixture
 */
class RenderingTestBase : public ::testing::Test {
protected:
    void SetUp() override {
        // Try to create renderer (will fail in headless CI, that's expected)
        renderer_ = RendererFactory::create(RenderingAPI::Auto, nullptr);
        if (renderer_) {
            ASSERT_TRUE(renderer_->initialize());
        }
    }

    void TearDown() override {
        if (renderer_) {
            renderer_->shutdown();
        }
    }

    std::unique_ptr<IRenderer> renderer_;
};

/**
 * @brief Deferred rendering test fixture
 */
class DeferredRenderingTest : public RenderingTestBase {
protected:
    void SetUp() override {
        RenderingTestBase::SetUp();
        if (renderer_) {
            deferred_renderer_ = std::make_unique<DeferredRenderer>(renderer_.get());
            
            DeferredConfig config;
            config.width = 1920;
            config.height = 1080;
            config.msaa_samples = 1;
            
            ASSERT_TRUE(deferred_renderer_->initialize(config));
        }
    }

    void TearDown() override {
        if (deferred_renderer_) {
            deferred_renderer_->shutdown();
            deferred_renderer_.reset();
        }
        RenderingTestBase::TearDown();
    }

    std::unique_ptr<DeferredRenderer> deferred_renderer_;
};

/**
 * @brief Material system test fixture
 */
class MaterialSystemTest : public RenderingTestBase {
protected:
    void SetUp() override {
        RenderingTestBase::SetUp();
        if (renderer_) {
            material_manager_ = std::make_unique<MaterialManager>(renderer_.get());
            texture_manager_ = std::make_unique<TextureManager>(renderer_.get());
        }
    }

    void TearDown() override {
        material_manager_.reset();
        texture_manager_.reset();
        RenderingTestBase::TearDown();
    }

    std::unique_ptr<MaterialManager> material_manager_;
    std::unique_ptr<TextureManager> texture_manager_;
};

// =============================================================================
// BASIC RENDERER TESTS
// =============================================================================

TEST_F(RenderingTestBase, RendererFactory) {
    // Test API availability detection
    EXPECT_TRUE(RendererFactory::is_api_available(RenderingAPI::OpenGL) || 
                RendererFactory::is_api_available(RenderingAPI::Vulkan));
    
    // Test best API selection
    RenderingAPI best_api = RendererFactory::get_best_api();
    EXPECT_NE(best_api, RenderingAPI::Auto);
    
    // Test API string conversion
    EXPECT_FALSE(RendererFactory::api_to_string(RenderingAPI::OpenGL).empty());
    EXPECT_FALSE(RendererFactory::api_to_string(RenderingAPI::Vulkan).empty());
}

TEST_F(RenderingTestBase, RendererCapabilities) {
    if (!renderer_) {
        GTEST_SKIP() << "Renderer not available (headless environment)";
    }

    auto caps = renderer_->get_capabilities();
    
    // Basic capability validation
    EXPECT_GT(caps.max_texture_size, 0u);
    EXPECT_GE(caps.max_msaa_samples, 1u);
    EXPECT_GE(caps.max_anisotropy, 1u);
    
    // Log capabilities for debugging
    std::cout << "Renderer Capabilities:\n"
              << "  Max texture size: " << caps.max_texture_size << "\n"
              << "  Max MSAA samples: " << caps.max_msaa_samples << "\n"
              << "  Max anisotropy: " << caps.max_anisotropy << "\n"
              << "  Compute shaders: " << (caps.supports_compute_shaders ? "Yes" : "No") << "\n"
              << "  Ray tracing: " << (caps.supports_ray_tracing ? "Yes" : "No") << "\n";
}

TEST_F(RenderingTestBase, BasicResourceManagement) {
    if (!renderer_) {
        GTEST_SKIP() << "Renderer not available (headless environment)";
    }

    // Test buffer creation
    BufferDesc buffer_desc;
    buffer_desc.size = 1024;
    buffer_desc.usage = BufferUsage::Static;
    buffer_desc.debug_name = "TestBuffer";
    
    std::vector<float> test_data(256, 1.0f);
    BufferHandle buffer = renderer_->create_buffer(buffer_desc, test_data.data());
    EXPECT_TRUE(buffer.is_valid());
    
    // Test texture creation
    TextureDesc texture_desc;
    texture_desc.width = 256;
    texture_desc.height = 256;
    texture_desc.format = TextureFormat::RGBA8;
    texture_desc.debug_name = "TestTexture";
    
    std::vector<uint8_t> texture_data(256 * 256 * 4, 128);
    TextureHandle texture = renderer_->create_texture(texture_desc, texture_data.data());
    EXPECT_TRUE(texture.is_valid());
    
    // Test shader creation
    std::string vertex_shader = R"(
        #version 450 core
        layout(location = 0) in vec3 position;
        layout(location = 1) in vec2 uv;
        
        out vec2 v_uv;
        
        void main() {
            gl_Position = vec4(position, 1.0);
            v_uv = uv;
        }
    )";
    
    std::string fragment_shader = R"(
        #version 450 core
        in vec2 v_uv;
        out vec4 color;
        
        uniform sampler2D u_texture;
        
        void main() {
            color = texture(u_texture, v_uv);
        }
    )";
    
    ShaderHandle shader = renderer_->create_shader(vertex_shader, fragment_shader, "TestShader");
    EXPECT_TRUE(shader.is_valid());
    
    // Clean up
    renderer_->destroy_buffer(buffer);
    renderer_->destroy_texture(texture);
    renderer_->destroy_shader(shader);
}

TEST_F(RenderingTestBase, FrameOperations) {
    if (!renderer_) {
        GTEST_SKIP() << "Renderer not available (headless environment)";
    }

    // Test basic frame operations
    renderer_->begin_frame();
    
    // Set viewport
    Viewport viewport;
    viewport.width = 800.0f;
    viewport.height = 600.0f;
    renderer_->set_viewport(viewport);
    
    // Clear frame
    std::array<float, 4> clear_color = {0.2f, 0.3f, 0.4f, 1.0f};
    renderer_->clear(clear_color, 1.0f, 0);
    
    renderer_->end_frame();
    
    // Get frame statistics
    auto stats = renderer_->get_frame_stats();
    EXPECT_GE(stats.frame_time_ms, 0.0f);
}

// =============================================================================
// DEFERRED RENDERING TESTS
// =============================================================================

TEST_F(DeferredRenderingTest, DeferredConfiguration) {
    if (!deferred_renderer_) {
        GTEST_SKIP() << "Deferred renderer not available (headless environment)";
    }

    // Test configuration access
    const auto& config = deferred_renderer_->get_config();
    EXPECT_EQ(config.width, 1920u);
    EXPECT_EQ(config.height, 1080u);
    
    // Test configuration update
    DeferredConfig new_config = config;
    new_config.enable_motion_vectors = !config.enable_motion_vectors;
    deferred_renderer_->update_config(new_config);
    
    EXPECT_EQ(deferred_renderer_->get_config().enable_motion_vectors, new_config.enable_motion_vectors);
}

TEST_F(DeferredRenderingTest, GBufferAccess) {
    if (!deferred_renderer_) {
        GTEST_SKIP() << "Deferred renderer not available (headless environment)";
    }

    // Test G-buffer texture access
    TextureHandle albedo = deferred_renderer_->get_g_buffer_texture(GBufferTarget::Albedo);
    TextureHandle normal = deferred_renderer_->get_g_buffer_texture(GBufferTarget::Normal);
    TextureHandle depth = deferred_renderer_->get_depth_buffer();
    
    EXPECT_TRUE(albedo.is_valid());
    EXPECT_TRUE(normal.is_valid());
    EXPECT_TRUE(depth.is_valid());
}

TEST_F(DeferredRenderingTest, LightSubmission) {
    if (!deferred_renderer_) {
        GTEST_SKIP() << "Deferred renderer not available (headless environment)";
    }

    deferred_renderer_->begin_frame();
    
    // Submit various light types
    Light directional_light;
    directional_light.type = LightType::Directional;
    directional_light.direction = {0.0f, -1.0f, 0.0f};
    directional_light.color = {1.0f, 1.0f, 0.9f};
    directional_light.intensity = 3.0f;
    
    Light point_light;
    point_light.type = LightType::Point;
    point_light.position = {5.0f, 2.0f, 0.0f};
    point_light.color = {1.0f, 0.5f, 0.2f};
    point_light.intensity = 10.0f;
    point_light.range = 20.0f;
    
    Light spot_light;
    spot_light.type = LightType::Spot;
    spot_light.position = {-3.0f, 3.0f, 2.0f};
    spot_light.direction = {0.5f, -0.7f, -0.5f};
    spot_light.color = {0.2f, 0.5f, 1.0f};
    spot_light.inner_cone_angle = 15.0f;
    spot_light.outer_cone_angle = 30.0f;
    
    deferred_renderer_->submit_light(directional_light);
    deferred_renderer_->submit_light(point_light);
    deferred_renderer_->submit_light(spot_light);
    
    deferred_renderer_->end_frame();
    
    // Check statistics
    auto stats = deferred_renderer_->get_statistics();
    EXPECT_EQ(stats.light_count, 3u);
}

TEST_F(DeferredRenderingTest, RenderPasses) {
    if (!deferred_renderer_) {
        GTEST_SKIP() << "Deferred renderer not available (headless environment)";
    }

    deferred_renderer_->begin_frame();
    
    // Set camera matrices (identity for test)
    std::array<float, 16> identity = {
        1, 0, 0, 0,
        0, 1, 0, 0,
        0, 0, 1, 0,
        0, 0, 0, 1
    };
    
    deferred_renderer_->set_camera(identity, identity);
    
    // Execute render passes (should not crash)
    deferred_renderer_->geometry_pass();
    deferred_renderer_->shadow_pass();
    deferred_renderer_->lighting_pass();
    deferred_renderer_->post_process_pass();
    deferred_renderer_->composition_pass();
    
    deferred_renderer_->end_frame();
}

// =============================================================================
// MATERIAL SYSTEM TESTS
// =============================================================================

TEST_F(MaterialSystemTest, MaterialCreation) {
    if (!material_manager_) {
        GTEST_SKIP() << "Material manager not available (headless environment)";
    }

    // Test basic material creation
    Material material("TestMaterial");
    material.set_shading_model(ShadingModel::DefaultLit);
    material.set_blend_mode(MaterialBlendMode::Opaque);
    
    // Set PBR parameters
    material.set_albedo({0.7f, 0.3f, 0.2f});
    material.set_metallic(0.1f);
    material.set_roughness(0.6f);
    material.set_emission({1.0f, 0.5f, 0.0f}, 2.0f);
    
    // Verify parameters
    auto albedo = material.get_albedo();
    EXPECT_FLOAT_EQ(albedo[0], 0.7f);
    EXPECT_FLOAT_EQ(albedo[1], 0.3f);
    EXPECT_FLOAT_EQ(albedo[2], 0.2f);
    
    EXPECT_FLOAT_EQ(material.get_metallic(), 0.1f);
    EXPECT_FLOAT_EQ(material.get_roughness(), 0.6f);
    EXPECT_FLOAT_EQ(material.get_emission_intensity(), 2.0f);
}

TEST_F(MaterialSystemTest, MaterialTemplates) {
    // Test template creation (these should work without renderer)
    auto pbr_material = MaterialTemplate::create_standard_pbr();
    auto glass_material = MaterialTemplate::create_glass();
    auto metal_material = MaterialTemplate::create_metal();
    auto emissive_material = MaterialTemplate::create_emissive();
    
    // Verify different shading models
    EXPECT_EQ(pbr_material.get_shading_model(), ShadingModel::DefaultLit);
    EXPECT_EQ(glass_material.get_blend_mode(), MaterialBlendMode::Transparent);
    EXPECT_GT(metal_material.get_metallic(), 0.8f);
    EXPECT_GT(emissive_material.get_emission_intensity(), 0.0f);
}

TEST_F(MaterialSystemTest, MaterialParameters) {
    if (!material_manager_) {
        GTEST_SKIP() << "Material manager not available (headless environment)";
    }

    Material material("ParameterTest");
    
    // Test different parameter types
    material.set_parameter("float_param", MaterialParameter(3.14f));
    material.set_parameter("float3_param", MaterialParameter(std::array<float, 3>{1.0f, 2.0f, 3.0f}));
    material.set_parameter("int_param", MaterialParameter(42));
    material.set_parameter("bool_param", MaterialParameter(true));
    
    // Verify parameters
    EXPECT_TRUE(material.has_parameter("float_param"));
    EXPECT_FLOAT_EQ(material.get_parameter("float_param").as_float(), 3.14f);
    
    auto float3_param = material.get_parameter("float3_param").as_float3();
    EXPECT_FLOAT_EQ(float3_param[0], 1.0f);
    EXPECT_FLOAT_EQ(float3_param[1], 2.0f);
    EXPECT_FLOAT_EQ(float3_param[2], 3.0f);
    
    EXPECT_EQ(material.get_parameter("int_param").as_int(), 42);
    EXPECT_TRUE(material.get_parameter("bool_param").as_bool());
}

TEST_F(MaterialSystemTest, ShaderGeneration) {
    if (!material_manager_) {
        GTEST_SKIP() << "Material manager not available (headless environment)";
    }

    Material material("ShaderTest");
    
    // Generate shaders
    std::string vertex_shader = material.generate_vertex_shader();
    std::string fragment_shader = material.generate_fragment_shader();
    
    // Basic validation
    EXPECT_FALSE(vertex_shader.empty());
    EXPECT_FALSE(fragment_shader.empty());
    
    // Check for required elements
    EXPECT_NE(vertex_shader.find("#version"), std::string::npos);
    EXPECT_NE(fragment_shader.find("#version"), std::string::npos);
    EXPECT_NE(vertex_shader.find("void main()"), std::string::npos);
    EXPECT_NE(fragment_shader.find("void main()"), std::string::npos);
    
    // Test hash consistency
    uint64_t hash1 = material.get_shader_hash();
    uint64_t hash2 = material.get_shader_hash();
    EXPECT_EQ(hash1, hash2);
    
    // Modify material and verify hash changes
    material.set_albedo({0.5f, 0.5f, 0.5f});
    uint64_t hash3 = material.get_shader_hash();
    EXPECT_NE(hash1, hash3);
}

// =============================================================================
// ADVANCED FEATURES TESTS
// =============================================================================

TEST_F(RenderingTestBase, FrustumCulling) {
    // Test frustum extraction and culling
    std::array<float, 16> view_proj = {
        2.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 2.0f, 0.0f, 0.0f,
        0.0f, 0.0f, -1.0f, -1.0f,
        0.0f, 0.0f, -2.0f, 0.0f
    };
    
    auto frustum = FrustumCuller::extract_frustum(view_proj);
    
    // Test AABB culling
    FrustumCuller::AABB inside_aabb = {
        {-0.5f, -0.5f, -0.5f},
        {0.5f, 0.5f, 0.5f}
    };
    
    FrustumCuller::AABB outside_aabb = {
        {10.0f, 10.0f, 10.0f},
        {11.0f, 11.0f, 11.0f}
    };
    
    EXPECT_TRUE(FrustumCuller::is_aabb_in_frustum(inside_aabb, frustum));
    EXPECT_FALSE(FrustumCuller::is_aabb_in_frustum(outside_aabb, frustum));
    
    // Test sphere culling
    EXPECT_TRUE(FrustumCuller::is_sphere_in_frustum({0.0f, 0.0f, 0.0f}, 1.0f, frustum));
    EXPECT_FALSE(FrustumCuller::is_sphere_in_frustum({10.0f, 10.0f, 10.0f}, 1.0f, frustum));
}

TEST_F(RenderingTestBase, HDRProcessing) {
    // Test tone mapping configuration
    ToneMappingConfig tone_config;
    tone_config.operator_type = ToneMappingOperator::ACES;
    tone_config.exposure = 1.2f;
    tone_config.gamma = 2.2f;
    
    EXPECT_EQ(tone_config.operator_type, ToneMappingOperator::ACES);
    EXPECT_FLOAT_EQ(tone_config.exposure, 1.2f);
    
    // Test bloom configuration
    BloomConfig bloom_config;
    bloom_config.threshold = 1.0f;
    bloom_config.intensity = 0.15f;
    bloom_config.iterations = 6;
    
    EXPECT_FLOAT_EQ(bloom_config.threshold, 1.0f);
    EXPECT_FLOAT_EQ(bloom_config.intensity, 0.15f);
}

// =============================================================================
// PERFORMANCE TESTS
// =============================================================================

TEST_F(RenderingTestBase, PerformanceMeasurement) {
    if (!renderer_) {
        GTEST_SKIP() << "Renderer not available (headless environment)";
    }

    const int num_iterations = 100;
    auto start_time = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < num_iterations; ++i) {
        renderer_->begin_frame();
        
        // Simulate some rendering work
        std::array<float, 4> color = {
            static_cast<float>(i) / num_iterations, 0.5f, 0.8f, 1.0f
        };
        renderer_->clear(color);
        
        renderer_->end_frame();
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
    
    double avg_frame_time = static_cast<double>(duration.count()) / (num_iterations * 1000.0); // ms
    
    std::cout << "Performance Test Results:\n"
              << "  Iterations: " << num_iterations << "\n"
              << "  Total time: " << duration.count() << " μs\n"
              << "  Average frame time: " << avg_frame_time << " ms\n"
              << "  Theoretical FPS: " << (1000.0 / avg_frame_time) << "\n";
    
    // Basic performance expectation (very lenient for CI)
    EXPECT_LT(avg_frame_time, 100.0); // Less than 100ms per frame
}

// =============================================================================
// ERROR HANDLING TESTS
// =============================================================================

TEST_F(RenderingTestBase, InvalidResourceHandling) {
    if (!renderer_) {
        GTEST_SKIP() << "Renderer not available (headless environment)";
    }

    // Test handling of invalid handles
    BufferHandle invalid_buffer;
    TextureHandle invalid_texture;
    ShaderHandle invalid_shader;
    
    EXPECT_FALSE(invalid_buffer.is_valid());
    EXPECT_FALSE(invalid_texture.is_valid());
    EXPECT_FALSE(invalid_shader.is_valid());
    
    // These operations should not crash (but may produce warnings)
    renderer_->destroy_buffer(invalid_buffer);
    renderer_->destroy_texture(invalid_texture);
    renderer_->destroy_shader(invalid_shader);
}

TEST_F(RenderingTestBase, ResourceLimits) {
    if (!renderer_) {
        GTEST_SKIP() << "Renderer not available (headless environment)";
    }

    auto caps = renderer_->get_capabilities();
    
    // Test texture size limits
    TextureDesc oversized_texture;
    oversized_texture.width = caps.max_texture_size + 1;
    oversized_texture.height = caps.max_texture_size + 1;
    oversized_texture.format = TextureFormat::RGBA8;
    
    // This should either fail gracefully or clamp to max size
    TextureHandle texture = renderer_->create_texture(oversized_texture);
    // We don't assert here since behavior may vary by implementation
    
    if (texture.is_valid()) {
        renderer_->destroy_texture(texture);
    }
}

// =============================================================================
// UTILITY TESTS
// =============================================================================

TEST(RenderingUtilities, UtilityFunctions) {
    // Test LOD calculation
    EXPECT_EQ(calculate_lod_level(1.0f), 0u);
    EXPECT_GT(calculate_lod_level(100.0f), 0u);
    EXPECT_LE(calculate_lod_level(1000.0f), 4u);
    
    // Test Halton sequence generation
    auto halton_seq = generate_halton_sequence(16);
    EXPECT_EQ(halton_seq.size(), 16u);
    
    // Verify sequence properties
    for (const auto& point : halton_seq) {
        EXPECT_GE(point[0], 0.0f);
        EXPECT_LT(point[0], 1.0f);
        EXPECT_GE(point[1], 0.0f);
        EXPECT_LT(point[1], 1.0f);
    }
    
    // Test normal packing/unpacking
    std::array<float, 3> original_normal = {0.0f, 1.0f, 0.0f};
    auto packed = pack_normal(original_normal);
    auto unpacked = unpack_normal(packed);
    
    // Should be approximately equal (within packing precision)
    const float epsilon = 1.0f / 255.0f;
    EXPECT_NEAR(unpacked[0], original_normal[0], epsilon);
    EXPECT_NEAR(unpacked[1], original_normal[1], epsilon);
    EXPECT_NEAR(unpacked[2], original_normal[2], epsilon);
}

// =============================================================================
// INTEGRATION TESTS
// =============================================================================

class RenderingIntegrationTest : public RenderingTestBase {
protected:
    void SetUp() override {
        RenderingTestBase::SetUp();
        if (renderer_) {
            // Set up a complete rendering scenario
            material_manager_ = std::make_unique<MaterialManager>(renderer_.get());
            deferred_renderer_ = std::make_unique<DeferredRenderer>(renderer_.get());
            
            DeferredConfig config;
            config.width = 800;
            config.height = 600;
            deferred_renderer_->initialize(config);
        }
    }

    void TearDown() override {
        if (deferred_renderer_) {
            deferred_renderer_->shutdown();
        }
        material_manager_.reset();
        deferred_renderer_.reset();
        RenderingTestBase::TearDown();
    }

    std::unique_ptr<MaterialManager> material_manager_;
    std::unique_ptr<DeferredRenderer> deferred_renderer_;
};

TEST_F(RenderingIntegrationTest, CompleteRenderingPipeline) {
    if (!renderer_ || !deferred_renderer_ || !material_manager_) {
        GTEST_SKIP() << "Full rendering pipeline not available (headless environment)";
    }

    // Create test geometry
    std::vector<float> vertices = {
        // Triangle vertices (position + UV)
        -0.5f, -0.5f, 0.0f,  0.0f, 0.0f,
         0.5f, -0.5f, 0.0f,  1.0f, 0.0f,
         0.0f,  0.5f, 0.0f,  0.5f, 1.0f
    };
    
    std::vector<uint32_t> indices = {0, 1, 2};
    
    BufferDesc vertex_desc;
    vertex_desc.size = vertices.size() * sizeof(float);
    vertex_desc.usage = BufferUsage::Static;
    vertex_desc.debug_name = "TestVertexBuffer";
    
    BufferDesc index_desc;
    index_desc.size = indices.size() * sizeof(uint32_t);
    index_desc.usage = BufferUsage::Static;
    index_desc.debug_name = "TestIndexBuffer";
    
    auto vertex_buffer = renderer_->create_buffer(vertex_desc, vertices.data());
    auto index_buffer = renderer_->create_buffer(index_desc, indices.data());
    
    ASSERT_TRUE(vertex_buffer.is_valid());
    ASSERT_TRUE(index_buffer.is_valid());
    
    // Create test material
    auto material = MaterialTemplate::create_standard_pbr();
    material.set_albedo({0.8f, 0.2f, 0.3f});
    material.set_metallic(0.1f);
    material.set_roughness(0.7f);
    
    auto material_handle = material_manager_->register_material(
        std::make_unique<Material>(std::move(material))
    );
    ASSERT_TRUE(material_handle.is_valid());
    
    // Set up camera
    std::array<float, 16> view = {
        1, 0, 0, 0,
        0, 1, 0, 0,
        0, 0, 1, 0,
        0, 0, -5, 1
    };
    
    std::array<float, 16> projection = {
        1.5f, 0, 0, 0,
        0, 2, 0, 0,
        0, 0, -1, -1,
        0, 0, -2, 0
    };
    
    // Render a complete frame
    deferred_renderer_->begin_frame();
    deferred_renderer_->set_camera(view, projection);
    
    // Submit geometry
    MaterialProperties mat_props;
    mat_props.albedo = {0.8f, 0.2f, 0.3f};
    mat_props.metallic = 0.1f;
    mat_props.roughness = 0.7f;
    
    std::array<float, 16> model_matrix = {
        1, 0, 0, 0,
        0, 1, 0, 0,
        0, 0, 1, 0,
        0, 0, 0, 1
    };
    
    deferred_renderer_->submit_geometry(vertex_buffer, index_buffer, mat_props, model_matrix, 3);
    
    // Submit lighting
    Light main_light;
    main_light.type = LightType::Directional;
    main_light.direction = {-0.3f, -0.7f, -0.6f};
    main_light.color = {1.0f, 0.95f, 0.8f};
    main_light.intensity = 3.0f;
    
    deferred_renderer_->submit_light(main_light);
    
    // Execute render passes
    deferred_renderer_->geometry_pass();
    deferred_renderer_->lighting_pass();
    deferred_renderer_->composition_pass();
    
    deferred_renderer_->end_frame();
    
    // Verify statistics
    auto stats = deferred_renderer_->get_statistics();
    EXPECT_EQ(stats.geometry_draw_calls, 1u);
    EXPECT_EQ(stats.light_count, 1u);
    EXPECT_GE(stats.geometry_pass_time_ms, 0.0f);
    
    // Clean up
    renderer_->destroy_buffer(vertex_buffer);
    renderer_->destroy_buffer(index_buffer);
    material_manager_->unregister_material(material_handle);
}

// =============================================================================
// MAIN TEST RUNNER
// =============================================================================

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    
    std::cout << "=== ECScope Rendering System Test Suite ===\n";
    std::cout << "Testing professional rendering engine components:\n";
    std::cout << "  - Multi-API backends (Vulkan/OpenGL)\n";
    std::cout << "  - Deferred rendering pipeline\n";
    std::cout << "  - PBR material system\n";
    std::cout << "  - Advanced rendering effects\n";
    std::cout << "  - Performance optimizations\n";
    std::cout << "===============================================\n\n";
    
    return RUN_ALL_TESTS();
}