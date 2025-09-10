/**
 * @file renderer_factory.cpp
 * @brief Renderer Factory Implementation
 * 
 * Factory for creating renderer instances with automatic API detection
 * and fallback support.
 * 
 * @author ECScope Development Team
 * @version 1.0.0
 */

#include "ecscope/rendering/renderer.hpp"
#include "ecscope/rendering/vulkan_backend.hpp"
#include "ecscope/rendering/opengl_backend.hpp"

#include <iostream>
#include <memory>

namespace ecscope::rendering {

std::unique_ptr<IRenderer> RendererFactory::create(RenderingAPI api, GLFWwindow* window) {
    RenderingAPI selected_api = api;
    
    // Auto-detect best API if requested
    if (api == RenderingAPI::Auto) {
        selected_api = get_best_api();
    }
    
    // Try to create requested renderer
    switch (selected_api) {
        case RenderingAPI::Vulkan:
            if (is_api_available(RenderingAPI::Vulkan)) {
                auto renderer = std::make_unique<VulkanRenderer>();
                if (window) {
                    renderer->set_window(window);
                }
                if (renderer->initialize(RenderingAPI::Vulkan)) {
                    std::cout << "Successfully created Vulkan renderer" << std::endl;
                    return std::move(renderer);
                } else {
                    std::cerr << "Failed to initialize Vulkan renderer" << std::endl;
                }
            }
            break;
            
        case RenderingAPI::OpenGL:
            if (is_api_available(RenderingAPI::OpenGL)) {
                auto renderer = std::make_unique<OpenGLRenderer>();
                if (window) {
                    renderer->set_window(window);
                }
                if (renderer->initialize(RenderingAPI::OpenGL)) {
                    std::cout << "Successfully created OpenGL renderer" << std::endl;
                    return std::move(renderer);
                } else {
                    std::cerr << "Failed to initialize OpenGL renderer" << std::endl;
                }
            }
            break;
            
        default:
            std::cerr << "Unknown rendering API requested" << std::endl;
            break;
    }
    
    // Try fallback options if the requested API failed
    if (api != RenderingAPI::Auto) {
        std::cerr << "Requested API " << api_to_string(api) << " failed, trying fallbacks..." << std::endl;
        
        // Try Vulkan as fallback
        if (selected_api != RenderingAPI::Vulkan && is_api_available(RenderingAPI::Vulkan)) {
            auto renderer = std::make_unique<VulkanRenderer>();
            if (window) {
                renderer->set_window(window);
            }
            if (renderer->initialize(RenderingAPI::Vulkan)) {
                std::cout << "Fallback to Vulkan renderer successful" << std::endl;
                return std::move(renderer);
            }
        }
        
        // Try OpenGL as fallback
        if (selected_api != RenderingAPI::OpenGL && is_api_available(RenderingAPI::OpenGL)) {
            auto renderer = std::make_unique<OpenGLRenderer>();
            if (window) {
                renderer->set_window(window);
            }
            if (renderer->initialize(RenderingAPI::OpenGL)) {
                std::cout << "Fallback to OpenGL renderer successful" << std::endl;
                return std::move(renderer);
            }
        }
    }
    
    std::cerr << "Failed to create any renderer" << std::endl;
    return nullptr;
}

bool RendererFactory::is_api_available(RenderingAPI api) {
    switch (api) {
        case RenderingAPI::Vulkan:
            return is_vulkan_available();
            
        case RenderingAPI::OpenGL:
            return is_opengl_available();
            
        default:
            return false;
    }
}

RenderingAPI RendererFactory::get_best_api() {
    // Prefer Vulkan for modern features and performance
    if (is_api_available(RenderingAPI::Vulkan)) {
        return RenderingAPI::Vulkan;
    }
    
    // Fallback to OpenGL if available
    if (is_api_available(RenderingAPI::OpenGL)) {
        return RenderingAPI::OpenGL;
    }
    
    // No supported API found
    return RenderingAPI::Vulkan; // Return Vulkan as default, will fail gracefully
}

std::string RendererFactory::api_to_string(RenderingAPI api) {
    switch (api) {
        case RenderingAPI::Vulkan:
            return "Vulkan";
        case RenderingAPI::OpenGL:
            return "OpenGL";
        case RenderingAPI::Auto:
            return "Auto";
        default:
            return "Unknown";
    }
}

} // namespace ecscope::rendering