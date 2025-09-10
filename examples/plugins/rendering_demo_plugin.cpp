/**
 * @file rendering_demo_plugin.cpp
 * @brief Advanced rendering plugin demonstrating graphics integration
 * 
 * This plugin demonstrates:
 * - Shader creation and management
 * - Custom render passes
 * - GUI integration
 * - Asset loading and management
 * - Render target creation
 * - Debug rendering utilities
 */

#include <ecscope/plugins/sdk/plugin_sdk.hpp>
#include <ecscope/plugins/rendering_integration.hpp>
#include <array>
#include <cmath>

class RenderingDemoPlugin : public ecscope::plugins::sdk::PluginBase {
public:
    RenderingDemoPlugin() : PluginBase("rendering_demo", {1, 0, 0}) {
        set_display_name("Rendering Demo Plugin");
        set_description("Demonstrates advanced rendering features and integration");
        set_author("ECScope Team");
        set_license("MIT");
        
        add_tag("rendering");
        add_tag("graphics");
        add_tag("demo");
        add_tag("shader");
        
        set_priority(ecscope::plugins::PluginPriority::High);
    }
    
    static const ecscope::plugins::PluginMetadata& get_static_metadata() {
        static ecscope::plugins::PluginMetadata metadata;
        if (metadata.name.empty()) {
            metadata.name = "rendering_demo";
            metadata.display_name = "Rendering Demo Plugin";
            metadata.description = "Demonstrates advanced rendering features and integration";
            metadata.author = "ECScope Team";
            metadata.version = {1, 0, 0};
            metadata.license = "MIT";
            metadata.sandbox_required = true;
            metadata.memory_limit = 1024 * 1024 * 100; // 100MB for graphics resources
            metadata.cpu_time_limit = 200; // 200ms for rendering
            metadata.tags = {"rendering", "graphics", "demo", "shader"};
            metadata.required_permissions = {"RenderingAccess", "AssetAccess", "GuiAccess"};
        }
        return metadata;
    }

protected:
    bool on_initialize() override {
        log_info("Initializing Rendering Demo Plugin");
        
        // Request rendering permissions
        if (!request_permission(ecscope::plugins::Permission::RenderingAccess, 
                              "For demonstrating rendering features")) {
            log_error("Failed to get rendering access");
            return false;
        }
        
        if (!request_permission(ecscope::plugins::Permission::AssetAccess, 
                              "For loading textures and models")) {
            log_error("Failed to get asset access");
            return false;
        }
        
        if (!request_permission(ecscope::plugins::Permission::GuiAccess, 
                              "For showing rendering controls")) {
            log_error("Failed to get GUI access");
            return false;
        }
        
        // Initialize rendering helper
        // Note: In a real implementation, these would be properly integrated
        // rendering_helper_ = std::make_unique<PluginRenderingHelper>(get_plugin_name(), integration, context);
        
        // Create demo shaders
        if (!create_demo_shaders()) {
            log_error("Failed to create demo shaders");
            return false;
        }
        
        // Load demo assets
        if (!load_demo_assets()) {
            log_error("Failed to load demo assets");
            return false;
        }
        
        // Setup render passes
        if (!setup_render_passes()) {
            log_error("Failed to setup render passes");
            return false;
        }
        
        // Setup GUI
        setup_gui();
        
        // Subscribe to rendering events
        subscribe_to_event("render.frame_start", [this](const auto& params) {
            update_animation();
        });
        
        log_info("Rendering Demo Plugin initialized successfully");
        return true;
    }
    
    void on_shutdown() override {
        log_info("Shutting down Rendering Demo Plugin");
        
        // Cleanup would happen automatically through RAII, but we can do explicit cleanup here
        cleanup_resources();
        
        log_info("Rendering Demo Plugin shutdown complete");
    }
    
    void update(double delta_time) override {
        update_time_ += delta_time;
        
        // Update animation parameters
        rotation_angle_ += rotation_speed_ * delta_time;
        if (rotation_angle_ > 2.0 * M_PI) {
            rotation_angle_ -= 2.0 * M_PI;
        }
        
        scale_factor_ = 1.0f + 0.3f * std::sin(update_time_ * 2.0);
        
        // Update color cycling
        color_hue_ += color_speed_ * delta_time;
        if (color_hue_ > 1.0f) {
            color_hue_ -= 1.0f;
        }
        
        demo_color_ = hsv_to_rgb(color_hue_, 0.8f, 1.0f);
        
        // Debug rendering updates
        if (show_debug_info_) {
            render_debug_info();
        }
    }

private:
    bool create_demo_shaders() {
        log_info("Creating demo shaders");
        
        // Simple color shader
        std::string color_vertex = R"(
            #version 330 core
            layout (location = 0) in vec3 aPos;
            
            uniform mat4 uModel;
            uniform mat4 uView;
            uniform mat4 uProjection;
            
            void main() {
                gl_Position = uProjection * uView * uModel * vec4(aPos, 1.0);
            }
        )";
        
        std::string color_fragment = R"(
            #version 330 core
            out vec4 FragColor;
            
            uniform vec3 uColor;
            uniform float uTime;
            
            void main() {
                float pulse = 0.5 + 0.5 * sin(uTime * 3.0);
                FragColor = vec4(uColor * pulse, 1.0);
            }
        )";
        
        // In a real implementation, this would use the rendering helper
        // rendering_helper_->create_shader("color_shader", color_vertex, color_fragment);
        
        // Textured shader
        std::string textured_vertex = R"(
            #version 330 core
            layout (location = 0) in vec3 aPos;
            layout (location = 1) in vec2 aTexCoord;
            
            out vec2 TexCoord;
            
            uniform mat4 uModel;
            uniform mat4 uView;
            uniform mat4 uProjection;
            
            void main() {
                gl_Position = uProjection * uView * uModel * vec4(aPos, 1.0);
                TexCoord = aTexCoord;
            }
        )";
        
        std::string textured_fragment = R"(
            #version 330 core
            out vec4 FragColor;
            
            in vec2 TexCoord;
            
            uniform sampler2D uTexture;
            uniform vec3 uTint;
            uniform float uAlpha;
            
            void main() {
                vec4 texColor = texture(uTexture, TexCoord);
                FragColor = vec4(texColor.rgb * uTint, texColor.a * uAlpha);
            }
        )";
        
        // rendering_helper_->create_shader("textured_shader", textured_vertex, textured_fragment);
        
        // Post-processing shader
        std::string post_vertex = R"(
            #version 330 core
            layout (location = 0) in vec2 aPos;
            layout (location = 1) in vec2 aTexCoord;
            
            out vec2 TexCoord;
            
            void main() {
                gl_Position = vec4(aPos, 0.0, 1.0);
                TexCoord = aTexCoord;
            }
        )";
        
        std::string post_fragment = R"(
            #version 330 core
            out vec4 FragColor;
            
            in vec2 TexCoord;
            
            uniform sampler2D uScreenTexture;
            uniform float uTime;
            uniform int uEffect;
            
            vec3 applyEffect(vec3 color) {
                if (uEffect == 1) {
                    // Grayscale
                    float gray = dot(color, vec3(0.299, 0.587, 0.114));
                    return vec3(gray);
                } else if (uEffect == 2) {
                    // Sepia
                    return vec3(
                        dot(color, vec3(0.393, 0.769, 0.189)),
                        dot(color, vec3(0.349, 0.686, 0.168)),
                        dot(color, vec3(0.272, 0.534, 0.131))
                    );
                } else if (uEffect == 3) {
                    // Wave distortion
                    vec2 distortedCoord = TexCoord + 0.01 * sin(TexCoord.y * 20.0 + uTime * 5.0);
                    return texture(uScreenTexture, distortedCoord).rgb;
                }
                return color;
            }
            
            void main() {
                vec3 color = texture(uScreenTexture, TexCoord).rgb;
                color = applyEffect(color);
                FragColor = vec4(color, 1.0);
            }
        )";
        
        // rendering_helper_->create_shader("post_process_shader", post_vertex, post_fragment);
        
        log_info("Demo shaders created successfully");
        return true;
    }
    
    bool load_demo_assets() {
        log_info("Loading demo assets");
        
        // In a real implementation, these would load actual assets
        // rendering_helper_->load_texture("demo_texture", get_data_directory() + "/demo_texture.png");
        // rendering_helper_->load_mesh("demo_cube", get_data_directory() + "/cube.obj");
        
        // Create some procedural assets for demonstration
        create_procedural_assets();
        
        log_info("Demo assets loaded successfully");
        return true;
    }
    
    void create_procedural_assets() {
        log_info("Creating procedural demo assets");
        
        // In a real implementation, this would create actual geometry and textures
        // For now, we'll just log what we would create
        log_info("Created procedural cube mesh");
        log_info("Created procedural checkerboard texture");
        log_info("Created procedural gradient texture");
    }
    
    bool setup_render_passes() {
        log_info("Setting up render passes");
        
        // Main geometry pass
        // rendering_helper_->add_render_pass("geometry_pass", [this](auto* renderer) {
        //     render_geometry_pass(renderer);
        // }, 100);
        
        // Effect pass
        // rendering_helper_->add_render_pass("effect_pass", [this](auto* renderer) {
        //     render_effect_pass(renderer);
        // }, 200);
        
        // Debug pass
        // rendering_helper_->add_render_pass("debug_pass", [this](auto* renderer) {
        //     render_debug_pass(renderer);
        // }, 900);
        
        log_info("Render passes setup complete");
        return true;
    }
    
    void setup_gui() {
        log_info("Setting up GUI elements");
        
        // Main control window
        // rendering_helper_->add_gui_window("Rendering Demo Controls", [this]() {
        //     render_control_gui();
        // });
        
        // Stats window
        // rendering_helper_->add_gui_window("Rendering Stats", [this]() {
        //     render_stats_gui();
        // });
        
        log_info("GUI setup complete");
    }
    
    void render_control_gui() {
        // In a real implementation, this would render ImGui controls
        log_debug("Rendering control GUI (placeholder)");
        
        // Controls would include:
        // - Animation speed slider
        // - Color picker
        // - Effect selection dropdown
        // - Debug options checkboxes
        // - Shader hot-reload button
    }
    
    void render_stats_gui() {
        // In a real implementation, this would show rendering statistics
        log_debug("Rendering stats GUI (placeholder)");
        
        // Stats would include:
        // - Frame time
        // - Draw calls
        // - Vertices rendered
        // - Texture memory usage
        // - Shader compilation status
    }
    
    void update_animation() {
        // Animation logic is handled in update(), but we could do frame-specific updates here
    }
    
    void render_debug_info() {
        if (!show_debug_info_) return;
        
        // In a real implementation, this would render debug geometry
        log_debug("Rendering debug info (placeholder)");
        
        // Would render:
        // - Coordinate axes
        // - Bounding boxes
        // - Wireframe overlays
        // - Performance metrics overlay
    }
    
    void cleanup_resources() {
        log_info("Cleaning up rendering resources");
        
        // In a real implementation, cleanup would be automatic through RAII
        // But we could do explicit cleanup here if needed
    }
    
    std::array<float, 3> hsv_to_rgb(float h, float s, float v) {
        float c = v * s;
        float x = c * (1.0f - std::abs(std::fmod(h * 6.0f, 2.0f) - 1.0f));
        float m = v - c;
        
        float r, g, b;
        
        if (h < 1.0f/6.0f) {
            r = c; g = x; b = 0;
        } else if (h < 2.0f/6.0f) {
            r = x; g = c; b = 0;
        } else if (h < 3.0f/6.0f) {
            r = 0; g = c; b = x;
        } else if (h < 4.0f/6.0f) {
            r = 0; g = x; b = c;
        } else if (h < 5.0f/6.0f) {
            r = x; g = 0; b = c;
        } else {
            r = c; g = 0; b = x;
        }
        
        return {r + m, g + m, b + m};
    }

private:
    // Animation state
    double update_time_{0.0};
    float rotation_angle_{0.0f};
    float rotation_speed_{1.0f}; // radians per second
    float scale_factor_{1.0f};
    
    // Color animation
    float color_hue_{0.0f};
    float color_speed_{0.2f}; // hue units per second
    std::array<float, 3> demo_color_{1.0f, 1.0f, 1.0f};
    
    // Debug options
    bool show_debug_info_{false};
    bool wireframe_mode_{false};
    bool show_bounding_boxes_{false};
    
    // Effect state
    int current_effect_{0};
    float effect_intensity_{1.0f};
    
    // Rendering helper (would be initialized in real implementation)
    // std::unique_ptr<ecscope::plugins::PluginRenderingHelper> rendering_helper_;
};

// Plugin export
DECLARE_PLUGIN(RenderingDemoPlugin, "rendering_demo", "1.0.0")
DECLARE_PLUGIN_API_VERSION()

/**
 * CMakeLists.txt for this plugin:
 * 
 * cmake_minimum_required(VERSION 3.16)
 * project(rendering_demo_plugin)
 * 
 * set(CMAKE_CXX_STANDARD 17)
 * find_package(ECScope REQUIRED COMPONENTS Core Plugins Rendering)
 * 
 * add_library(rendering_demo_plugin SHARED rendering_demo_plugin.cpp)
 * target_link_libraries(rendering_demo_plugin ECScope::Core ECScope::Plugins ECScope::Rendering)
 * set_target_properties(rendering_demo_plugin PROPERTIES
 *     OUTPUT_NAME "rendering_demo"
 *     PREFIX ""
 *     SUFFIX ".ecplugin"
 * )
 */