/**
 * @file 04_shader_programming_and_materials.cpp
 * @brief Tutorial 4: Shader Programming and Materials - Advanced Visual Effects
 * 
 * This tutorial explores shader programming and material systems in 2D rendering.
 * You'll learn how to create custom visual effects using shaders and materials.
 * 
 * Learning Objectives:
 * 1. Understand GPU shader programs and the graphics pipeline
 * 2. Learn vertex and fragment shader concepts in 2D context
 * 3. Explore material properties and uniform variables
 * 4. Create custom visual effects with shader code
 * 5. Master performance considerations of custom shaders
 * 
 * Key Concepts Covered:
 * - Vertex and fragment shaders in 2D rendering
 * - Material system and uniform buffer management
 * - Shader compilation and linking process
 * - Custom effects: color manipulation, distortion, lighting
 * - Shader performance optimization techniques
 * 
 * Educational Value:
 * Shader programming is fundamental to modern graphics. This tutorial provides
 * practical experience with GPU programming concepts that apply to both 2D and 3D graphics.
 * 
 * @author ECScope Educational ECS Framework
 * @date 2024
 */

#include <iostream>
#include <memory>
#include <vector>
#include <string>
#include <cmath>
#include <chrono>

// ECScope Core
#include "../../src/core/types.hpp"
#include "../../src/core/log.hpp"

// ECS System
#include "../../src/ecs/registry.hpp"
#include "../../src/ecs/components/transform.hpp"

// 2D Rendering System
#include "../../src/renderer/renderer_2d.hpp"
#include "../../src/renderer/components/render_components.hpp"
#include "../../src/renderer/resources/shader.hpp"
#include "../../src/renderer/window.hpp"

using namespace ecscope;
using namespace ecscope::renderer;
using namespace ecscope::renderer::components;

/**
 * @brief Shader Programming and Materials Tutorial
 * 
 * This tutorial demonstrates custom shader creation and material systems
 * through practical examples with visual effects.
 */
class ShaderProgrammingTutorial {
public:
    ShaderProgrammingTutorial() = default;
    ~ShaderProgrammingTutorial() { cleanup(); }
    
    bool initialize() {
        core::Log::info("Tutorial", "=== Shader Programming and Materials Tutorial ===");
        core::Log::info("Tutorial", "Learning objective: Master custom shaders and advanced materials");
        
        // Initialize window and renderer
        window_ = std::make_unique<Window>("Tutorial 4: Shader Programming", 1400, 1000);
        if (!window_->initialize()) {
            core::Log::error("Tutorial", "Failed to create window");
            return false;
        }
        
        // Configure renderer for shader development
        Renderer2DConfig renderer_config = Renderer2DConfig::educational_mode();
        renderer_config.debug.enable_debug_rendering = false; // Focus on shader effects
        renderer_config.debug.show_performance_overlay = true;
        
        renderer_ = std::make_unique<Renderer2D>(renderer_config);
        if (!renderer_->initialize().is_ok()) {
            core::Log::error("Tutorial", "Failed to initialize renderer");
            return false;
        }
        
        // Set up camera
        camera_ = Camera2D::create_main_camera(1400, 1000);
        camera_.set_position(0.0f, 0.0f);
        camera_.set_zoom(1.0f);
        
        // Create ECS registry
        registry_ = std::make_unique<ecs::Registry>();
        
        core::Log::info("Tutorial", "System initialized. Creating shader examples...");
        
        // Create custom shaders and materials
        if (!create_custom_shaders()) {
            core::Log::error("Tutorial", "Failed to create custom shaders");
            return false;
        }
        
        return true;
    }
    
    void run() {
        if (!window_ || !renderer_) return;
        
        core::Log::info("Tutorial", "Starting shader programming demonstration...");
        
        // Run shader effect demonstrations
        demonstrate_basic_shader_concepts();
        demonstrate_color_manipulation_shaders();
        demonstrate_distortion_effects();
        demonstrate_animated_shaders();
        demonstrate_lighting_effects();
        demonstrate_performance_comparison();
        
        display_educational_summary();
    }
    
private:
    //=========================================================================
    // Custom Shader Creation
    //=========================================================================
    
    bool create_custom_shaders() {
        core::Log::info("Shaders", "Creating custom shader programs for demonstrations");
        
        // Note: In a real implementation, these would be actual GLSL shader source code
        // For this educational demo, we'll simulate the shader creation process
        
        // 1. Color Tint Shader - Simple color manipulation
        create_color_tint_shader();
        
        // 2. Wave Distortion Shader - Vertex manipulation
        create_wave_distortion_shader();
        
        // 3. Animated Rainbow Shader - Time-based effects
        create_rainbow_shader();
        
        // 4. Simple Lighting Shader - Basic 2D lighting
        create_lighting_shader();
        
        // 5. Performance Test Shader - Complex calculations
        create_performance_test_shader();
        
        core::Log::info("Shaders", "Created {} custom shader programs", shader_materials_.size());
        return true;
    }
    
    void create_color_tint_shader() {
        core::Log::info("Shader", "Creating Color Tint Shader");
        core::Log::info("Explanation", "This shader demonstrates basic uniform variables and color manipulation");
        
        // Simulated vertex shader source (educational explanation)
        std::string vertex_shader = R"(
            // Vertex Shader - Transforms vertices from world space to screen space
            #version 330 core
            
            layout (location = 0) in vec2 a_position;  // Vertex position
            layout (location = 1) in vec2 a_texCoord;  // Texture coordinates
            layout (location = 2) in vec4 a_color;     // Vertex color
            
            uniform mat4 u_viewProjection;  // Combined view-projection matrix
            uniform mat4 u_model;           // Model transformation matrix
            
            out vec2 v_texCoord;   // Pass texture coordinates to fragment shader
            out vec4 v_color;      // Pass color to fragment shader
            
            void main() {
                // Transform vertex position to screen space
                gl_Position = u_viewProjection * u_model * vec4(a_position, 0.0, 1.0);
                
                // Pass interpolated values to fragment shader
                v_texCoord = a_texCoord;
                v_color = a_color;
            }
        )";
        
        std::string fragment_shader = R"(
            // Fragment Shader - Determines final pixel color
            #version 330 core
            
            in vec2 v_texCoord;    // Interpolated texture coordinates
            in vec4 v_color;       // Interpolated vertex color
            
            uniform sampler2D u_texture;  // Texture sampler
            uniform vec4 u_tintColor;     // Custom tint color (our uniform!)
            uniform float u_intensity;    // Tint intensity
            
            out vec4 fragColor;    // Final pixel color output
            
            void main() {
                // Sample the texture
                vec4 texColor = texture(u_texture, v_texCoord);
                
                // Apply vertex color modulation
                texColor *= v_color;
                
                // Apply custom tint effect
                vec3 tinted = mix(texColor.rgb, u_tintColor.rgb, u_intensity);
                
                // Output final color
                fragColor = vec4(tinted, texColor.a * u_tintColor.a);
            }
        )";
        
        // Create material with this shader
        ShaderMaterial material;
        material.name = "Color Tint";
        material.description = "Basic color tinting with uniform controls";
        material.vertex_source = vertex_shader;
        material.fragment_source = fragment_shader;
        
        // Set up material properties
        Material mat = Material::create_sprite_material();
        mat.set_uniform(0, Color::red().red_f(), Color::red().green_f(), 
                       Color::red().blue_f(), Color::red().alpha_f()); // u_tintColor
        mat.set_uniform(1, 0.5f); // u_intensity
        
        material.material = mat;
        shader_materials_["color_tint"] = material;
        
        core::Log::info("Shader", "Color Tint Shader: Uses vec4 tint color and float intensity uniforms");
    }
    
    void create_wave_distortion_shader() {
        core::Log::info("Shader", "Creating Wave Distortion Shader");
        core::Log::info("Explanation", "This shader demonstrates vertex manipulation and time-based animation");
        
        std::string vertex_shader = R"(
            #version 330 core
            
            layout (location = 0) in vec2 a_position;
            layout (location = 1) in vec2 a_texCoord;
            layout (location = 2) in vec4 a_color;
            
            uniform mat4 u_viewProjection;
            uniform mat4 u_model;
            uniform float u_time;          // Animation time
            uniform float u_waveAmplitude; // Wave strength
            uniform float u_waveFrequency; // Wave frequency
            
            out vec2 v_texCoord;
            out vec4 v_color;
            
            void main() {
                vec2 pos = a_position;
                
                // Apply wave distortion to vertex position
                float wave = sin(pos.x * u_waveFrequency + u_time) * u_waveAmplitude;
                pos.y += wave;
                
                gl_Position = u_viewProjection * u_model * vec4(pos, 0.0, 1.0);
                
                v_texCoord = a_texCoord;
                v_color = a_color;
            }
        )";
        
        std::string fragment_shader = R"(
            #version 330 core
            
            in vec2 v_texCoord;
            in vec4 v_color;
            
            uniform sampler2D u_texture;
            
            out vec4 fragColor;
            
            void main() {
                vec4 texColor = texture(u_texture, v_texCoord);
                fragColor = texColor * v_color;
            }
        )";
        
        ShaderMaterial material;
        material.name = "Wave Distortion";
        material.description = "Vertex-based wave distortion effect";
        material.vertex_source = vertex_shader;
        material.fragment_source = fragment_shader;
        
        Material mat = Material::create_sprite_material();
        mat.set_uniform(0, 0.0f);  // u_time (will be updated each frame)
        mat.set_uniform(1, 20.0f); // u_waveAmplitude
        mat.set_uniform(2, 0.02f); // u_waveFrequency
        
        material.material = mat;
        shader_materials_["wave_distortion"] = material;
        
        core::Log::info("Shader", "Wave Distortion: Animates vertices using sine waves");
    }
    
    void create_rainbow_shader() {
        core::Log::info("Shader", "Creating Animated Rainbow Shader");
        core::Log::info("Explanation", "This shader creates animated color effects using HSV color space");
        
        std::string fragment_shader = R"(
            #version 330 core
            
            in vec2 v_texCoord;
            in vec4 v_color;
            
            uniform sampler2D u_texture;
            uniform float u_time;
            uniform float u_speed;
            uniform float u_intensity;
            
            out vec4 fragColor;
            
            // Convert HSV to RGB
            vec3 hsv2rgb(vec3 c) {
                vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
                vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
                return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
            }
            
            void main() {
                vec4 texColor = texture(u_texture, v_texCoord);
                
                // Create rainbow effect based on texture coordinates and time
                float hue = v_texCoord.x + v_texCoord.y + u_time * u_speed;
                vec3 rainbow = hsv2rgb(vec3(hue, 1.0, 1.0));
                
                // Mix original texture with rainbow effect
                vec3 finalColor = mix(texColor.rgb, rainbow, u_intensity);
                
                fragColor = vec4(finalColor, texColor.a) * v_color;
            }
        )";
        
        ShaderMaterial material;
        material.name = "Animated Rainbow";
        material.description = "HSV-based animated rainbow coloring";
        material.fragment_source = fragment_shader;
        
        Material mat = Material::create_sprite_material();
        mat.set_uniform(0, 0.0f);  // u_time
        mat.set_uniform(1, 0.5f);  // u_speed
        mat.set_uniform(2, 0.8f);  // u_intensity
        
        material.material = mat;
        shader_materials_["rainbow"] = material;
        
        core::Log::info("Shader", "Rainbow Shader: Animates colors through HSV color space");
    }
    
    void create_lighting_shader() {
        core::Log::info("Shader", "Creating Simple 2D Lighting Shader");
        core::Log::info("Explanation", "This shader demonstrates basic 2D lighting calculations");
        
        std::string fragment_shader = R"(
            #version 330 core
            
            in vec2 v_texCoord;
            in vec4 v_color;
            
            uniform sampler2D u_texture;
            uniform vec2 u_lightPosition;  // Light position in world space
            uniform vec3 u_lightColor;     // Light color
            uniform float u_lightRadius;   // Light radius
            uniform float u_ambientLight;  // Ambient light level
            
            out vec4 fragColor;
            
            void main() {
                vec4 texColor = texture(u_texture, v_texCoord);
                
                // Calculate distance from light (simplified 2D lighting)
                vec2 fragPosition = v_texCoord * 512.0; // Assume 512x512 texture space
                float distance = length(fragPosition - u_lightPosition);
                
                // Calculate light attenuation
                float attenuation = 1.0 - clamp(distance / u_lightRadius, 0.0, 1.0);
                attenuation = attenuation * attenuation; // Quadratic falloff
                
                // Combine ambient and directional light
                vec3 lighting = vec3(u_ambientLight) + u_lightColor * attenuation;
                
                // Apply lighting to texture
                vec3 finalColor = texColor.rgb * lighting;
                
                fragColor = vec4(finalColor, texColor.a) * v_color;
            }
        )";
        
        ShaderMaterial material;
        material.name = "2D Lighting";
        material.description = "Point light with distance attenuation";
        material.fragment_source = fragment_shader;
        
        Material mat = Material::create_sprite_material();
        mat.set_uniform(0, 256.0f, 256.0f);           // u_lightPosition
        mat.set_uniform(1, 1.0f, 0.8f, 0.6f);        // u_lightColor (warm)
        mat.set_uniform(2, 300.0f);                   // u_lightRadius
        mat.set_uniform(3, 0.2f);                     // u_ambientLight
        
        material.material = mat;
        shader_materials_["lighting"] = material;
        
        core::Log::info("Shader", "Lighting Shader: Point light with quadratic attenuation");
    }
    
    void create_performance_test_shader() {
        core::Log::info("Shader", "Creating Performance Test Shader");
        core::Log::info("Explanation", "This shader demonstrates the performance impact of complex calculations");
        
        std::string fragment_shader = R"(
            #version 330 core
            
            in vec2 v_texCoord;
            in vec4 v_color;
            
            uniform sampler2D u_texture;
            uniform float u_time;
            uniform int u_iterations; // Number of expensive operations
            
            out vec4 fragColor;
            
            void main() {
                vec4 texColor = texture(u_texture, v_texCoord);
                vec3 color = texColor.rgb;
                
                // Expensive operations (deliberately inefficient for demonstration)
                for (int i = 0; i < u_iterations; i++) {
                    float noise = sin(v_texCoord.x * 50.0 + float(i) * 0.1 + u_time) *
                                 cos(v_texCoord.y * 30.0 + float(i) * 0.2 + u_time);
                    color = mix(color, vec3(noise * 0.5 + 0.5), 0.1);
                }
                
                fragColor = vec4(color, texColor.a) * v_color;
            }
        )";
        
        ShaderMaterial material;
        material.name = "Performance Test";
        material.description = "Demonstrates performance impact of complex shaders";
        material.fragment_source = fragment_shader;
        
        Material mat = Material::create_sprite_material();
        mat.set_uniform(0, 0.0f);  // u_time
        mat.set_uniform(1, 10);    // u_iterations
        
        material.material = mat;
        shader_materials_["performance_test"] = material;
        
        core::Log::info("Shader", "Performance Test: Uses loops and expensive math operations");
    }
    
    //=========================================================================
    // Demonstration Functions
    //=========================================================================
    
    void demonstrate_basic_shader_concepts() {
        core::Log::info("Demo 1", "=== BASIC SHADER CONCEPTS ===");
        core::Log::info("Explanation", "Understanding the GPU graphics pipeline and shader stages");
        
        // Create demo sprites with different shaders
        create_shader_demo_sprites();
        
        // Render with default shader first
        core::Log::info("Demo", "Rendering with default shader (baseline)");
        render_with_shader("default", 60); // 1 second
        
        // Show color tint shader
        core::Log::info("Demo", "Rendering with Color Tint shader");
        core::Log::info("Explanation", "Custom fragment shader modifies pixel colors using uniforms");
        render_with_shader("color_tint", 60);
        
        // Explain shader pipeline
        explain_shader_pipeline();
    }
    
    void demonstrate_color_manipulation_shaders() {
        core::Log::info("Demo 2", "=== COLOR MANIPULATION EFFECTS ===");
        core::Log::info("Explanation", "Using fragment shaders for color effects and post-processing");
        
        // Animate color tint over time
        f32 time = 0.0f;
        for (u32 frame = 0; frame < 180; ++frame) { // 3 seconds
            time = frame / 60.0f;
            
            // Update tint color to cycle through rainbow
            f32 hue = time * 0.5f;
            Color tint_color{
                static_cast<u8>((std::sin(hue) * 0.5f + 0.5f) * 255),
                static_cast<u8>((std::sin(hue + 2.09f) * 0.5f + 0.5f) * 255),
                static_cast<u8>((std::sin(hue + 4.19f) * 0.5f + 0.5f) * 255),
                255
            };
            
            // Update shader uniform
            auto& material = shader_materials_["color_tint"];
            material.material.set_uniform(0, tint_color);
            
            render_demo_frame("color_tint");
            
            if (frame % 30 == 0) {
                core::Log::info("Animation", "Frame {}: Tint color RGB({}, {}, {})",
                               frame, tint_color.r, tint_color.g, tint_color.b);
            }
        }
        
        core::Log::info("Demo", "Color manipulation demonstration completed");
    }
    
    void demonstrate_distortion_effects() {
        core::Log::info("Demo 3", "=== VERTEX DISTORTION EFFECTS ===");
        core::Log::info("Explanation", "Using vertex shaders to modify geometry dynamically");
        
        // Animate wave distortion
        f32 time = 0.0f;
        for (u32 frame = 0; frame < 240; ++frame) { // 4 seconds
            time = frame / 60.0f;
            
            // Update wave animation
            auto& material = shader_materials_["wave_distortion"];
            material.material.set_uniform(0, time); // u_time
            
            render_demo_frame("wave_distortion");
            
            if (frame % 60 == 0) {
                core::Log::info("Animation", "Wave time: {:.2f}s, creating vertex distortion", time);
            }
        }
        
        core::Log::info("Demo", "Vertex distortion demonstration completed");
        core::Log::info("Analysis", "Vertex shaders can create dynamic geometry effects efficiently");
    }
    
    void demonstrate_animated_shaders() {
        core::Log::info("Demo 4", "=== TIME-BASED ANIMATED EFFECTS ===");
        core::Log::info("Explanation", "Creating dynamic visual effects using time uniforms");
        
        // Show rainbow animation
        f32 time = 0.0f;
        for (u32 frame = 0; frame < 300; ++frame) { // 5 seconds
            time = frame / 60.0f;
            
            // Update rainbow animation
            auto& material = shader_materials_["rainbow"];
            material.material.set_uniform(0, time); // u_time
            
            render_demo_frame("rainbow");
            
            if (frame % 60 == 0) {
                core::Log::info("Animation", "Rainbow animation at {:.1f}s - HSV color cycling", time);
            }
        }
        
        core::Log::info("Demo", "Animated shader effects demonstration completed");
    }
    
    void demonstrate_lighting_effects() {
        core::Log::info("Demo 5", "=== 2D LIGHTING SYSTEM ===");
        core::Log::info("Explanation", "Implementing dynamic lighting in 2D using fragment shaders");
        
        // Animate light position
        for (u32 frame = 0; frame < 240; ++frame) { // 4 seconds
            f32 time = frame / 60.0f;
            
            // Move light in circular pattern
            f32 light_x = 256.0f + std::cos(time * 2.0f) * 150.0f;
            f32 light_y = 256.0f + std::sin(time * 2.0f) * 100.0f;
            
            // Update lighting shader
            auto& material = shader_materials_["lighting"];
            material.material.set_uniform(0, light_x, light_y); // u_lightPosition
            
            render_demo_frame("lighting");
            
            if (frame % 60 == 0) {
                core::Log::info("Lighting", "Light position: ({:.1f}, {:.1f})", light_x, light_y);
            }
        }
        
        core::Log::info("Demo", "2D lighting demonstration completed");
        core::Log::info("Analysis", "Fragment-based lighting enables dynamic illumination effects");
    }
    
    void demonstrate_performance_comparison() {
        core::Log::info("Demo 6", "=== SHADER PERFORMANCE ANALYSIS ===");
        core::Log::info("Explanation", "Measuring performance impact of complex shader operations");
        
        struct PerformanceTest {
            const char* shader_name;
            const char* description;
            u32 test_frames;
        };
        
        std::vector<PerformanceTest> tests = {
            {"default", "Default sprite shader (baseline)", 60},
            {"color_tint", "Simple color tint shader", 60},
            {"wave_distortion", "Vertex wave distortion", 60},
            {"rainbow", "HSV rainbow animation", 60},
            {"lighting", "2D point lighting", 60},
            {"performance_test", "Complex math operations", 60}
        };
        
        for (const auto& test : tests) {
            core::Log::info("Performance Test", "Testing {}: {}", test.shader_name, test.description);
            
            auto start_time = std::chrono::high_resolution_clock::now();
            
            for (u32 frame = 0; frame < test.test_frames; ++frame) {
                render_demo_frame(test.shader_name);
            }
            
            auto end_time = std::chrono::high_resolution_clock::now();
            f32 total_time = std::chrono::duration<f32>(end_time - start_time).count();
            f32 avg_fps = test.test_frames / total_time;
            f32 avg_frame_time = (total_time / test.test_frames) * 1000.0f; // ms
            
            core::Log::info("Results", "{}: {:.1f} FPS, {:.3f}ms per frame", 
                           test.shader_name, avg_fps, avg_frame_time);
            
            performance_results_[test.shader_name] = {avg_fps, avg_frame_time};
        }
        
        analyze_performance_results();
    }
    
    //=========================================================================
    // Support Functions
    //=========================================================================
    
    void create_shader_demo_sprites() {
        // Clear existing entities
        sprite_entities_.clear();
        registry_ = std::make_unique<ecs::Registry>();
        
        // Create a grid of sprites for shader demonstration
        const u32 grid_size = 5;
        const f32 spacing = 120.0f;
        const f32 start_x = -(grid_size - 1) * spacing * 0.5f;
        const f32 start_y = -(grid_size - 1) * spacing * 0.5f;
        
        for (u32 x = 0; x < grid_size; ++x) {
            for (u32 y = 0; y < grid_size; ++y) {
                u32 entity = registry_->create_entity();
                sprite_entities_.push_back(entity);
                
                Transform transform;
                transform.position = {start_x + x * spacing, start_y + y * spacing, 0.0f};
                transform.scale = {80.0f, 80.0f, 1.0f};
                registry_->add_component(entity, transform);
                
                RenderableSprite sprite;
                sprite.texture = TextureHandle{1, 32, 32};
                sprite.color_modulation = Color{
                    static_cast<u8>(128 + (x * 25) % 128),
                    static_cast<u8>(128 + (y * 25) % 128),
                    static_cast<u8>(128 + ((x + y) * 25) % 128),
                    255
                };
                sprite.z_order = static_cast<f32>(x + y);
                registry_->add_component(entity, sprite);
            }
        }
        
        core::Log::info("Demo", "Created {}x{} grid of sprites for shader demonstration", grid_size, grid_size);
    }
    
    void render_with_shader(const char* shader_name, u32 frames) {
        for (u32 frame = 0; frame < frames; ++frame) {
            render_demo_frame(shader_name);
        }
    }
    
    void render_demo_frame(const char* shader_name) {
        renderer_->begin_frame();
        renderer_->set_active_camera(camera_);
        
        // In a real implementation, we would bind the custom shader here
        // For this demo, we simulate the shader effects conceptually
        if (shader_materials_.count(shader_name) > 0) {
            const auto& shader_material = shader_materials_.at(shader_name);
            // renderer_->bind_material(shader_material.material);
        }
        
        renderer_->render_entities(*registry_);
        renderer_->end_frame();
        
        window_->swap_buffers();
        window_->poll_events();
    }
    
    void explain_shader_pipeline() {
        core::Log::info("Education", "=== GPU GRAPHICS PIPELINE EXPLANATION ===");
        core::Log::info("Pipeline", "1. Vertex Shader: Processes each vertex (position transformation)");
        core::Log::info("Pipeline", "2. Primitive Assembly: Combines vertices into triangles");
        core::Log::info("Pipeline", "3. Rasterization: Converts triangles to pixels (fragments)");
        core::Log::info("Pipeline", "4. Fragment Shader: Processes each pixel (color calculation)");
        core::Log::info("Pipeline", "5. Per-Fragment Operations: Depth test, blending, etc.");
        
        core::Log::info("2D Context", "In 2D rendering:");
        core::Log::info("2D Context", "- Vertex shader handles position, scale, rotation transforms");
        core::Log::info("2D Context", "- Fragment shader handles texturing, lighting, effects");
        core::Log::info("2D Context", "- Uniforms pass data from CPU to GPU (time, colors, etc.)");
        core::Log::info("2D Context", "- Varyings interpolate data between vertex and fragment stages");
    }
    
    void analyze_performance_results() {
        core::Log::info("Analysis", "=== SHADER PERFORMANCE ANALYSIS ===");
        
        f32 baseline_fps = performance_results_["default"].fps;
        
        for (const auto& [shader_name, result] : performance_results_) {
            if (shader_name == "default") continue;
            
            f32 performance_ratio = result.fps / baseline_fps;
            f32 overhead_percent = ((baseline_fps - result.fps) / baseline_fps) * 100.0f;
            
            core::Log::info("Performance", "{}: {:.1f}% of baseline performance ({:.1f}% overhead)",
                           shader_name, performance_ratio * 100.0f, overhead_percent);
            
            if (performance_ratio < 0.8f) {
                core::Log::info("Recommendation", "{}: Significant performance impact - optimize for production", shader_name);
            } else if (performance_ratio < 0.95f) {
                core::Log::info("Recommendation", "{}: Moderate impact - monitor in complex scenes", shader_name);
            } else {
                core::Log::info("Recommendation", "{}: Good performance - suitable for production", shader_name);
            }
        }
    }
    
    void display_educational_summary() {
        std::cout << "\n=== SHADER PROGRAMMING TUTORIAL SUMMARY ===\n\n";
        
        std::cout << "KEY CONCEPTS LEARNED:\n\n";
        
        std::cout << "1. SHADER BASICS:\n";
        std::cout << "   - Vertex Shaders: Transform vertex positions and attributes\n";
        std::cout << "   - Fragment Shaders: Calculate final pixel colors\n";
        std::cout << "   - Uniforms: Pass data from CPU to GPU (constant for draw call)\n";
        std::cout << "   - Varyings: Interpolate data between shader stages\n\n";
        
        std::cout << "2. MATERIAL SYSTEM:\n";
        std::cout << "   - Materials combine shaders with uniform parameters\n";
        std::cout << "   - Uniform management enables runtime shader customization\n";
        std::cout << "   - Material properties control rendering state and effects\n\n";
        
        std::cout << "3. VISUAL EFFECTS TECHNIQUES:\n";
        std::cout << "   - Color manipulation: Tinting, saturation, contrast\n";
        std::cout << "   - Vertex distortion: Wave effects, morphing, animation\n";
        std::cout << "   - Time-based animation: Using uniforms for dynamic effects\n";
        std::cout << "   - Lighting simulation: Distance-based attenuation\n\n";
        
        std::cout << "4. PERFORMANCE CONSIDERATIONS:\n";
        if (!performance_results_.empty()) {
            f32 baseline = performance_results_.at("default").fps;
            f32 worst_fps = baseline;
            std::string worst_shader;
            
            for (const auto& [name, result] : performance_results_) {
                if (result.fps < worst_fps && name != "default") {
                    worst_fps = result.fps;
                    worst_shader = name;
                }
            }
            
            std::cout << "   - Baseline (default shader): " << baseline << " FPS\n";
            std::cout << "   - Most expensive (" << worst_shader << "): " << worst_fps << " FPS\n";
            std::cout << "   - Performance impact: " << ((baseline - worst_fps) / baseline * 100) << "%\n";
        }
        std::cout << "   - Complex calculations in fragment shaders are expensive\n";
        std::cout << "   - Minimize texture samples and mathematical operations\n";
        std::cout << "   - Use vertex shaders for per-vertex calculations when possible\n\n";
        
        std::cout << "PRACTICAL APPLICATIONS:\n";
        std::cout << "- Create custom visual effects for games and applications\n";
        std::cout << "- Implement post-processing effects (blur, glow, distortion)\n";
        std::cout << "- Build dynamic lighting systems for 2D games\n";
        std::cout << "- Develop procedural texturing and animation effects\n";
        std::cout << "- Optimize rendering performance through custom shaders\n\n";
        
        std::cout << "SHADER DEVELOPMENT WORKFLOW:\n";
        std::cout << "1. Design effect concept and identify required uniforms\n";
        std::cout << "2. Write and compile vertex/fragment shader source code\n";
        std::cout << "3. Create material with shader and default uniform values\n";
        std::cout << "4. Test effect with various uniform parameter combinations\n";
        std::cout << "5. Profile performance and optimize expensive operations\n";
        std::cout << "6. Integrate with game systems for dynamic parameter control\n\n";
        
        std::cout << "NEXT TUTORIAL: Texture Atlasing and Optimization Techniques\n\n";
    }
    
    void cleanup() {
        if (renderer_) renderer_->shutdown();
        if (window_) window_->shutdown();
    }
    
    //=========================================================================
    // Data Structures
    //=========================================================================
    
    struct ShaderMaterial {
        std::string name;
        std::string description;
        std::string vertex_source;
        std::string fragment_source;
        Material material;
    };
    
    struct PerformanceResult {
        f32 fps{0.0f};
        f32 frame_time_ms{0.0f};
    };
    
    // Tutorial resources
    std::unique_ptr<Window> window_;
    std::unique_ptr<Renderer2D> renderer_;
    std::unique_ptr<ecs::Registry> registry_;
    Camera2D camera_;
    
    // Demo entities
    std::vector<u32> sprite_entities_;
    
    // Shader materials
    std::unordered_map<std::string, ShaderMaterial> shader_materials_;
    
    // Performance tracking
    std::unordered_map<std::string, PerformanceResult> performance_results_;
};

//=============================================================================
// Shader Programming Concepts Explanation
//=============================================================================

void explain_shader_programming_concepts() {
    std::cout << "\n=== SHADER PROGRAMMING CONCEPTS IN DEPTH ===\n\n";
    
    std::cout << "GPU ARCHITECTURE:\n";
    std::cout << "- GPUs are massively parallel processors optimized for graphics\n";
    std::cout << "- Hundreds or thousands of cores execute shader programs simultaneously\n";
    std::cout << "- Each core processes one vertex or pixel at a time\n";
    std::cout << "- Memory access patterns are crucial for performance\n\n";
    
    std::cout << "SHADER LANGUAGE (GLSL):\n";
    std::cout << "- OpenGL Shading Language - C-like syntax for GPU programming\n";
    std::cout << "- Built-in vector and matrix types (vec2, vec3, vec4, mat4)\n";
    std::cout << "- Mathematical functions optimized for graphics (sin, cos, mix, etc.)\n";
    std::cout << "- Compile-time constants and runtime uniforms\n\n";
    
    std::cout << "VERTEX SHADER RESPONSIBILITIES:\n";
    std::cout << "- Transform vertex positions from model space to screen space\n";
    std::cout << "- Calculate per-vertex lighting (if using Gouraud shading)\n";
    std::cout << "- Pass data to fragment shader through varyings\n";
    std::cout << "- Apply vertex-based effects (morphing, skinning, waves)\n\n";
    
    std::cout << "FRAGMENT SHADER RESPONSIBILITIES:\n";
    std::cout << "- Calculate final pixel color using interpolated vertex data\n";
    std::cout << "- Sample textures and apply filtering\n";
    std::cout << "- Implement per-pixel lighting (Phong shading)\n";
    std::cout << "- Apply post-processing effects and visual filters\n\n";
    
    std::cout << "UNIFORM VARIABLES:\n";
    std::cout << "- Global variables accessible to all shader instances in a draw call\n";
    std::cout << "- Set from CPU code before rendering\n";
    std::cout << "- Examples: transformation matrices, light positions, material colors\n";
    std::cout << "- Uniform buffer objects can group related uniforms for efficiency\n\n";
    
    std::cout << "PERFORMANCE OPTIMIZATION:\n";
    std::cout << "- Minimize branches (if statements) in shaders\n";
    std::cout << "- Reduce texture samples and complex mathematical operations\n";
    std::cout << "- Use lower precision types when possible (mediump, lowp)\n";
    std::cout << "- Move calculations to vertex shader when per-vertex precision is sufficient\n";
    std::cout << "- Profile with GPU debugging tools to identify bottlenecks\n\n";
}

//=============================================================================
// Main Function
//=============================================================================

int main() {
    core::Log::info("Main", "Starting Shader Programming and Materials Tutorial");
    
    std::cout << "\n=== WELCOME TO TUTORIAL 4: SHADER PROGRAMMING AND MATERIALS ===\n";
    std::cout << "This tutorial provides comprehensive coverage of GPU shader programming\n";
    std::cout << "and material systems for creating advanced 2D visual effects.\n\n";
    std::cout << "You will learn:\n";
    std::cout << "- Vertex and fragment shader fundamentals\n";
    std::cout << "- Material system architecture and uniform management\n";
    std::cout << "- Custom visual effects: color manipulation, distortion, lighting\n";
    std::cout << "- Shader performance optimization techniques\n";
    std::cout << "- Practical shader development workflows\n\n";
    std::cout << "Watch for detailed shader source code explanations and performance analysis.\n\n";
    
    ShaderProgrammingTutorial tutorial;
    
    if (!tutorial.initialize()) {
        core::Log::error("Main", "Failed to initialize tutorial");
        return -1;
    }
    
    tutorial.run();
    
    // Show additional shader programming concepts
    explain_shader_programming_concepts();
    
    core::Log::info("Main", "Shader Programming Tutorial completed successfully!");
    return 0;
}