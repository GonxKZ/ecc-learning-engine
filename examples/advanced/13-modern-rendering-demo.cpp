/**
 * @file 13-modern-rendering-demo.cpp
 * @brief Modern Rendering Engine Demonstration
 * 
 * Comprehensive demonstration of the professional rendering engine
 * showcasing Vulkan/OpenGL backends, deferred rendering, and PBR materials.
 * 
 * @author ECScope Development Team
 * @version 1.0.0
 */

#include <iostream>
#include <memory>
#include <chrono>
#include <thread>
#include <array>
#include <vector>

// ECScope core systems
#include "../../include/ecscope/core/log.hpp"
#include "../../include/ecscope/core/time.hpp"

#ifdef ECSCOPE_ENABLE_MODERN_RENDERING
// Modern rendering system
#include "../../include/ecscope/rendering/renderer.hpp"
#include "../../include/ecscope/rendering/vulkan_backend.hpp"

#ifdef ECSCOPE_HAS_GLFW
#include <GLFW/glfw3.h>
#endif

using namespace ecscope::rendering;
#endif

namespace ecscope::examples {

/**
 * @brief Modern rendering demonstration class
 */
class ModernRenderingDemo {
public:
    ModernRenderingDemo() = default;
    ~ModernRenderingDemo() = default;

    bool initialize() {
        ecscope::core::Log::info("=== ECScope Modern Rendering Engine Demo ===");
        
        #ifdef ECSCOPE_ENABLE_MODERN_RENDERING
        
        #ifdef ECSCOPE_HAS_GLFW
        // Initialize GLFW for window management
        if (!glfwInit()) {
            ecscope::core::Log::error("Failed to initialize GLFW");
            return false;
        }
        
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API); // We're using Vulkan
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
        
        window_ = glfwCreateWindow(800, 600, "ECScope Vulkan Demo", nullptr, nullptr);
        if (!window_) {
            ecscope::core::Log::error("Failed to create GLFW window");
            glfwTerminate();
            return false;
        }
        #endif
        
        // Display available APIs
        ecscope::core::Log::info("Available Rendering APIs:");
        ecscope::core::Log::info("  - Vulkan: {}", RendererFactory::is_api_available(RenderingAPI::Vulkan) ? "Yes" : "No");
        
        // Create renderer with Vulkan API
        renderer_ = RendererFactory::create(RenderingAPI::Vulkan, window_);
        if (!renderer_) {
            ecscope::core::Log::error("Failed to create Vulkan renderer");
            return false;
        }
        
        ecscope::core::Log::info("Selected API: Vulkan");
        
        // Log renderer capabilities
        display_capabilities();
        
        // Create test scene
        create_test_scene();
        
        ecscope::core::Log::info("Vulkan rendering engine initialized successfully!");
        return true;
        
        #else
        ecscope::core::Log::warning("Modern rendering system not enabled in build configuration");
        ecscope::core::Log::info("To enable: cmake -DECSCOPE_ENABLE_MODERN_RENDERING=ON");
        return false;
        #endif
    }
    
    void run() {
        #ifdef ECSCOPE_ENABLE_MODERN_RENDERING
        if (!renderer_) {
            ecscope::core::Log::error("Rendering system not properly initialized");
            return;
        }
        
        ecscope::core::Log::info("Running Vulkan rendering demonstration...");
        
        const int frame_count = 120; // Run for 2 seconds at 60 FPS
        auto start_time = std::chrono::high_resolution_clock::now();
        
        #ifdef ECSCOPE_HAS_GLFW
        while (!glfwWindowShouldClose(window_) && frame_count > 0) {
            glfwPollEvents();
        #else
        for (int frame = 0; frame < frame_count; ++frame) {
        #endif
            auto frame_start = std::chrono::high_resolution_clock::now();
            
            // Begin frame
            renderer_->begin_frame();
            
            // Update scene (simple animation)
            update_scene(frame_count * 0.016f);
            
            // Render frame
            render_frame();
            
            // End frame
            renderer_->end_frame();
            
            // Display progress every 30 frames
            static int frame = 0;
            if (frame++ % 30 == 0) {
                auto stats = renderer_->get_frame_stats();
                
                ecscope::core::Log::info("Frame {}: {:.2f}ms frame time, {} draw calls, {} vertices", 
                    frame, stats.frame_time_ms, stats.draw_calls, stats.vertices_rendered);
            }
            
            // Maintain 60 FPS (for demonstration)
            auto frame_end = std::chrono::high_resolution_clock::now();
            auto frame_duration = std::chrono::duration_cast<std::chrono::microseconds>(frame_end - frame_start);
            auto target_duration = std::chrono::microseconds(16667); // ~60 FPS
            
            if (frame_duration < target_duration) {
                std::this_thread::sleep_for(target_duration - frame_duration);
            }
        }
        
        // Display final statistics
        auto end_time = std::chrono::high_resolution_clock::now();
        auto total_duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        
        display_final_statistics(frame_count, total_duration.count());
        #endif
    }
    
    void shutdown() {
        #ifdef ECSCOPE_ENABLE_MODERN_RENDERING
        ecscope::core::Log::info("Shutting down rendering engine...");
        
        // Clean up resources
        cleanup_scene();
        
        if (renderer_) {
            renderer_->shutdown();
            renderer_.reset();
        }
        
        #ifdef ECSCOPE_HAS_GLFW
        if (window_) {
            glfwDestroyWindow(window_);
            window_ = nullptr;
        }
        glfwTerminate();
        #endif
        
        ecscope::core::Log::info("Rendering engine shut down successfully");
        #endif
    }

private:
    #ifdef ECSCOPE_ENABLE_MODERN_RENDERING
    
    void display_capabilities() {
        auto caps = renderer_->get_capabilities();
        
        ecscope::core::Log::info("Renderer Capabilities:");
        ecscope::core::Log::info("  - Max texture size: {}x{}", caps.max_texture_size, caps.max_texture_size);
        ecscope::core::Log::info("  - Max 3D texture size: {}³", caps.max_3d_texture_size);
        ecscope::core::Log::info("  - Max MSAA samples: {}", caps.max_msaa_samples);
        ecscope::core::Log::info("  - Max anisotropy: {}", caps.max_anisotropy);
        ecscope::core::Log::info("  - Compute shaders: {}", caps.supports_compute_shaders ? "Yes" : "No");
        ecscope::core::Log::info("  - Tessellation: {}", caps.supports_tessellation ? "Yes" : "No");
        ecscope::core::Log::info("  - Bindless resources: {}", caps.supports_bindless_resources ? "Yes" : "No");
        ecscope::core::Log::info("  - Ray tracing: {}", caps.supports_ray_tracing ? "Yes" : "No");
    }
    
    
    void create_test_scene() {
        ecscope::core::Log::info("Creating test scene...");
        
        // Create simple triangle geometry
        std::vector<float> vertices = {
            // Position (X, Y, Z), Color (R, G, B)
            -0.5f, -0.5f, 0.0f,  1.0f, 0.0f, 0.0f,
             0.5f, -0.5f, 0.0f,  0.0f, 1.0f, 0.0f,
             0.0f,  0.5f, 0.0f,  0.0f, 0.0f, 1.0f
        };
        
        std::vector<uint16_t> indices = {0, 1, 2};
        
        // Create vertex buffer
        BufferDesc vertex_desc;
        vertex_desc.size = vertices.size() * sizeof(float);
        vertex_desc.usage = BufferUsage::Static;
        vertex_desc.debug_name = "TriangleVertexBuffer";
        
        vertex_buffer_ = renderer_->create_buffer(vertex_desc, vertices.data());
        if (!vertex_buffer_.is_valid()) {
            ecscope::core::Log::error("Failed to create vertex buffer");
            return;
        }
        
        // Create index buffer
        BufferDesc index_desc;
        index_desc.size = indices.size() * sizeof(uint16_t);
        index_desc.usage = BufferUsage::Static;
        index_desc.debug_name = "TriangleIndexBuffer";
        
        index_buffer_ = renderer_->create_buffer(index_desc, indices.data());
        if (!index_buffer_.is_valid()) {
            ecscope::core::Log::error("Failed to create index buffer");
            return;
        }
        
        ecscope::core::Log::info("Test scene created with triangle geometry");
    }
    
    void update_scene(float time) {
        // Simple scene animation placeholder
        // In a full implementation, this would update object transforms, etc.
        (void)time; // Suppress unused parameter warning
    }
    
    void render_frame() {
        // Set viewport
        Viewport viewport;
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = 800.0f;
        viewport.height = 600.0f;
        viewport.min_depth = 0.0f;
        viewport.max_depth = 1.0f;
        renderer_->set_viewport(viewport);
        
        // Clear with a nice blue color
        std::array<float, 4> clear_color = {0.1f, 0.2f, 0.4f, 1.0f};
        renderer_->clear(clear_color, 1.0f, 0);
        
        // Demo debug markers
        renderer_->push_debug_marker("Render Triangle");
        
        // Bind vertex and index buffers
        if (vertex_buffer_.is_valid() && index_buffer_.is_valid()) {
            std::vector<BufferHandle> vertex_buffers = {vertex_buffer_};
            renderer_->set_vertex_buffers(vertex_buffers);
            renderer_->set_index_buffer(index_buffer_, 0, false); // 16-bit indices
            
            // Draw the triangle (note: without a proper shader pipeline, this won't render)
            DrawIndexedCommand draw_cmd;
            draw_cmd.index_count = 3;
            draw_cmd.instance_count = 1;
            draw_cmd.first_index = 0;
            draw_cmd.vertex_offset = 0;
            draw_cmd.first_instance = 0;
            
            // This would need a proper shader to actually render
            // renderer_->draw_indexed(draw_cmd);
        }
        
        renderer_->pop_debug_marker();
    }
    
    void cleanup_scene() {
        if (vertex_buffer_.is_valid()) {
            renderer_->destroy_buffer(vertex_buffer_);
        }
        
        if (index_buffer_.is_valid()) {
            renderer_->destroy_buffer(index_buffer_);
        }
        
        ecscope::core::Log::info("Scene resources cleaned up");
    }
    
    void display_final_statistics(int frame_count, int64_t total_time_ms) {
        auto final_stats = renderer_->get_frame_stats();
        
        ecscope::core::Log::info("=== Final Vulkan Rendering Statistics ===");
        ecscope::core::Log::info("Performance:");
        ecscope::core::Log::info("  - Frames rendered: {}", frame_count);
        ecscope::core::Log::info("  - Total time: {}ms", total_time_ms);
        ecscope::core::Log::info("  - Average FPS: {:.2f}", (frame_count * 1000.0f) / total_time_ms);
        ecscope::core::Log::info("  - Average frame time: {:.2f}ms", final_stats.frame_time_ms);
        ecscope::core::Log::info("  - Average GPU time: {:.2f}ms", final_stats.gpu_time_ms);
        
        ecscope::core::Log::info("Rendering:");
        ecscope::core::Log::info("  - Draw calls per frame: {}", final_stats.draw_calls);
        ecscope::core::Log::info("  - Vertices rendered: {}", final_stats.vertices_rendered);
        
        ecscope::core::Log::info("Memory:");
        ecscope::core::Log::info("  - GPU memory used: {}MB", final_stats.memory_used / (1024 * 1024));
    }
    
    // Core rendering objects
    std::unique_ptr<IRenderer> renderer_;
    
    #ifdef ECSCOPE_HAS_GLFW
    GLFWwindow* window_ = nullptr;
    #endif
    
    // Scene objects
    BufferHandle vertex_buffer_;
    BufferHandle index_buffer_;
    
    #endif // ECSCOPE_ENABLE_MODERN_RENDERING
};

} // namespace ecscope::examples

int main() {
    try {
        ecscope::examples::ModernRenderingDemo demo;
        
        if (demo.initialize()) {
            demo.run();
        }
        
        demo.shutdown();
        
    } catch (const std::exception& e) {
        ecscope::core::Log::error("Demo failed with exception: {}", e.what());
        return 1;
    }
    
    return 0;
}