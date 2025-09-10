/**
 * @file vulkan_renderer.cpp
 * @brief Professional Vulkan Rendering Backend Implementation
 * 
 * High-performance Vulkan implementation with modern features,
 * optimal resource management, and robust error handling.
 * 
 * @author ECScope Development Team
 * @version 1.0.0
 */

#include "ecscope/rendering/vulkan_backend.hpp"
#include <GLFW/glfw3.h>
#include <iostream>
#include <set>
#include <algorithm>
#include <fstream>
#include <chrono>

#ifdef ECSCOPE_HAS_VMA
#define VMA_IMPLEMENTATION
#include "vk_mem_alloc.h"
#endif

namespace ecscope::rendering {

// =============================================================================
// STATIC VALIDATION LAYERS
// =============================================================================

static const std::vector<const char*> s_validation_layers = {
    "VK_LAYER_KHRONOS_validation"
};

static const std::vector<const char*> s_device_extensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME,
    VK_EXT_DEBUG_UTILS_EXTENSION_NAME
};

#ifdef NDEBUG
static constexpr bool ENABLE_VALIDATION_LAYERS = false;
#else
static constexpr bool ENABLE_VALIDATION_LAYERS = true;
#endif

// =============================================================================
// HELPER FUNCTIONS
// =============================================================================

static bool check_validation_layer_support() {
    uint32_t layer_count;
    vkEnumerateInstanceLayerProperties(&layer_count, nullptr);
    
    std::vector<VkLayerProperties> available_layers(layer_count);
    vkEnumerateInstanceLayerProperties(&layer_count, available_layers.data());
    
    for (const char* layer_name : s_validation_layers) {
        bool layer_found = false;
        
        for (const auto& layer_properties : available_layers) {
            if (strcmp(layer_name, layer_properties.layerName) == 0) {
                layer_found = true;
                break;
            }
        }
        
        if (!layer_found) {
            return false;
        }
    }
    
    return true;
}

static std::vector<const char*> get_required_extensions() {
    uint32_t glfw_extension_count = 0;
    const char** glfw_extensions;
    glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);
    
    std::vector<const char*> extensions(glfw_extensions, glfw_extensions + glfw_extension_count);
    
    if (ENABLE_VALIDATION_LAYERS) {
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }
    
    return extensions;
}

// =============================================================================
// DEBUG CALLBACK
// =============================================================================

static VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(
    VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
    VkDebugUtilsMessageTypeFlagsEXT message_type,
    const VkDebugUtilsMessengerCallbackDataEXT* callback_data,
    void* user_data) {
    
    std::string severity_str;
    switch (message_severity) {
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
            severity_str = "VERBOSE";
            break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
            severity_str = "INFO";
            break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
            severity_str = "WARNING";
            break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
            severity_str = "ERROR";
            break;
        default:
            severity_str = "UNKNOWN";
            break;
    }
    
    std::cerr << "[VULKAN " << severity_str << "]: " << callback_data->pMessage << std::endl;
    
    return VK_FALSE;
}

// =============================================================================
// VULKAN RENDERER IMPLEMENTATION
// =============================================================================

VulkanRenderer::VulkanRenderer() 
    : enable_validation_layers_(ENABLE_VALIDATION_LAYERS) {
    
    // Initialize frame statistics
    frame_stats_ = {};
    
    // Reserve space for command buffers to avoid reallocations
    command_buffers_.reserve(MAX_FRAMES_IN_FLIGHT);
}

VulkanRenderer::~VulkanRenderer() {
    shutdown();
}

bool VulkanRenderer::initialize(RenderingAPI api) {
    if (api != RenderingAPI::Vulkan && api != RenderingAPI::Auto) {
        std::cerr << "VulkanRenderer can only initialize with Vulkan API" << std::endl;
        return false;
    }
    
    try {
        // Initialize GLFW
        if (!glfwInit()) {
            std::cerr << "Failed to initialize GLFW" << std::endl;
            return false;
        }
        
        // Check validation layer support
        if (enable_validation_layers_ && !check_validation_layer_support()) {
            std::cerr << "Validation layers requested but not available" << std::endl;
            enable_validation_layers_ = false;
        }
        
        // Create Vulkan instance
        if (!create_instance()) {
            std::cerr << "Failed to create Vulkan instance" << std::endl;
            return false;
        }
        
        // Setup debug messenger
        if (enable_validation_layers_ && !setup_debug_messenger()) {
            std::cerr << "Failed to setup debug messenger" << std::endl;
            return false;
        }
        
        // Create window surface (if window is set)
        if (window_ && !create_surface()) {
            std::cerr << "Failed to create window surface" << std::endl;
            return false;
        }
        
        // Pick physical device
        if (!pick_physical_device()) {
            std::cerr << "Failed to find suitable GPU" << std::endl;
            return false;
        }
        
        // Create logical device
        if (!create_logical_device()) {
            std::cerr << "Failed to create logical device" << std::endl;
            return false;
        }
        
        // Create swapchain (if surface exists)
        if (surface_ != VK_NULL_HANDLE && !create_swapchain()) {
            std::cerr << "Failed to create swapchain" << std::endl;
            return false;
        }
        
        // Create image views
        if (surface_ != VK_NULL_HANDLE && !create_image_views()) {
            std::cerr << "Failed to create image views" << std::endl;
            return false;
        }
        
        // Create render pass
        if (!create_render_pass()) {
            std::cerr << "Failed to create render pass" << std::endl;
            return false;
        }
        
        // Create command pools
        if (!create_command_pool()) {
            std::cerr << "Failed to create command pool" << std::endl;
            return false;
        }
        
        // Create command buffers
        if (!create_command_buffers()) {
            std::cerr << "Failed to create command buffers" << std::endl;
            return false;
        }
        
        // Create synchronization objects
        if (!create_sync_objects()) {
            std::cerr << "Failed to create synchronization objects" << std::endl;
            return false;
        }
        
        std::cout << "Vulkan renderer initialized successfully" << std::endl;
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "Exception during Vulkan initialization: " << e.what() << std::endl;
        return false;
    }
}

void VulkanRenderer::shutdown() {
    if (device_ != VK_NULL_HANDLE) {
        vkDeviceWaitIdle(device_);
    }
    
    // Clean up synchronization objects
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        if (render_finished_semaphores_.size() > i) {
            vkDestroySemaphore(device_, render_finished_semaphores_[i], nullptr);
        }
        if (image_available_semaphores_.size() > i) {
            vkDestroySemaphore(device_, image_available_semaphores_[i], nullptr);
        }
        if (in_flight_fences_.size() > i) {
            vkDestroyFence(device_, in_flight_fences_[i], nullptr);
        }
    }
    
    // Clean up command buffers and pools
    if (graphics_command_pool_ != VK_NULL_HANDLE) {
        vkDestroyCommandPool(device_, graphics_command_pool_, nullptr);
    }
    if (transfer_command_pool_ != VK_NULL_HANDLE) {
        vkDestroyCommandPool(device_, transfer_command_pool_, nullptr);
    }
    
    // Clean up swapchain
    for (auto framebuffer : swapchain_framebuffers_) {
        vkDestroyFramebuffer(device_, framebuffer, nullptr);
    }
    
    if (render_pass_ != VK_NULL_HANDLE) {
        vkDestroyRenderPass(device_, render_pass_, nullptr);
    }
    
    for (auto image_view : swapchain_image_views_) {
        vkDestroyImageView(device_, image_view, nullptr);
    }
    
    if (swapchain_ != VK_NULL_HANDLE) {
        vkDestroySwapchainKHR(device_, swapchain_, nullptr);
    }
    
    // Clean up resources
    for (auto& [id, buffer] : buffers_) {
        if (buffer.buffer != VK_NULL_HANDLE) {
            vkDestroyBuffer(device_, buffer.buffer, nullptr);
        }
        if (buffer.memory != VK_NULL_HANDLE) {
            vkFreeMemory(device_, buffer.memory, nullptr);
        }
    }
    buffers_.clear();
    
    for (auto& [id, texture] : textures_) {
        if (texture.image_view != VK_NULL_HANDLE) {
            vkDestroyImageView(device_, texture.image_view, nullptr);
        }
        if (texture.sampler != VK_NULL_HANDLE) {
            vkDestroySampler(device_, texture.sampler, nullptr);
        }
        if (texture.image != VK_NULL_HANDLE) {
            vkDestroyImage(device_, texture.image, nullptr);
        }
        if (texture.memory != VK_NULL_HANDLE) {
            vkFreeMemory(device_, texture.memory, nullptr);
        }
    }
    textures_.clear();
    
    for (auto& [id, shader] : shaders_) {
        if (shader.graphics_pipeline != VK_NULL_HANDLE) {
            vkDestroyPipeline(device_, shader.graphics_pipeline, nullptr);
        }
        if (shader.compute_pipeline != VK_NULL_HANDLE) {
            vkDestroyPipeline(device_, shader.compute_pipeline, nullptr);
        }
        if (shader.pipeline_layout != VK_NULL_HANDLE) {
            vkDestroyPipelineLayout(device_, shader.pipeline_layout, nullptr);
        }
        if (shader.descriptor_set_layout != VK_NULL_HANDLE) {
            vkDestroyDescriptorSetLayout(device_, shader.descriptor_set_layout, nullptr);
        }
        if (shader.render_pass != VK_NULL_HANDLE) {
            vkDestroyRenderPass(device_, shader.render_pass, nullptr);
        }
        if (shader.vertex_module != VK_NULL_HANDLE) {
            vkDestroyShaderModule(device_, shader.vertex_module, nullptr);
        }
        if (shader.fragment_module != VK_NULL_HANDLE) {
            vkDestroyShaderModule(device_, shader.fragment_module, nullptr);
        }
        if (shader.compute_module != VK_NULL_HANDLE) {
            vkDestroyShaderModule(device_, shader.compute_module, nullptr);
        }
    }
    shaders_.clear();
    
    for (auto& [id, fence] : fences_) {
        vkDestroyFence(device_, fence, nullptr);
    }
    fences_.clear();
    
    // Clean up device
    if (device_ != VK_NULL_HANDLE) {
        vkDestroyDevice(device_, nullptr);
        device_ = VK_NULL_HANDLE;
    }
    
    // Clean up surface
    if (surface_ != VK_NULL_HANDLE) {
        vkDestroySurfaceKHR(instance_, surface_, nullptr);
        surface_ = VK_NULL_HANDLE;
    }
    
    // Clean up debug messenger
    if (debug_messenger_ != VK_NULL_HANDLE) {
        auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
            instance_, "vkDestroyDebugUtilsMessengerEXT");
        if (func != nullptr) {
            func(instance_, debug_messenger_, nullptr);
        }
        debug_messenger_ = VK_NULL_HANDLE;
    }
    
    // Clean up instance
    if (instance_ != VK_NULL_HANDLE) {
        vkDestroyInstance(instance_, nullptr);
        instance_ = VK_NULL_HANDLE;
    }
    
    // Cleanup GLFW
    glfwTerminate();
}

IRenderer::RendererCaps VulkanRenderer::get_capabilities() const {
    RendererCaps caps;
    
    if (physical_device_ != VK_NULL_HANDLE) {
        caps.max_texture_size = device_properties_.limits.maxImageDimension2D;
        caps.max_3d_texture_size = device_properties_.limits.maxImageDimension3D;
        caps.max_array_texture_layers = device_properties_.limits.maxImageArrayLayers;
        caps.max_msaa_samples = static_cast<uint32_t>(device_properties_.limits.framebufferColorSampleCounts);
        caps.max_anisotropy = static_cast<uint32_t>(device_properties_.limits.maxSamplerAnisotropy);
        
        caps.supports_compute_shaders = device_features_.tessellationShader;
        caps.supports_tessellation = device_features_.tessellationShader;
        caps.supports_geometry_shaders = device_features_.geometryShader;
        caps.supports_bindless_resources = false; // Would need to check for descriptor indexing
        caps.supports_ray_tracing = false; // Would need to check for ray tracing extensions
    }
    
    return caps;
}

// =============================================================================
// INITIALIZATION HELPERS
// =============================================================================

bool VulkanRenderer::create_instance() {
    VkApplicationInfo app_info{};
    app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pApplicationName = "ECScope Rendering Engine";
    app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.pEngineName = "ECScope";
    app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.apiVersion = VK_API_VERSION_1_2;
    
    VkInstanceCreateInfo create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    create_info.pApplicationInfo = &app_info;
    
    auto extensions = get_required_extensions();
    create_info.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    create_info.ppEnabledExtensionNames = extensions.data();
    
    VkDebugUtilsMessengerCreateInfoEXT debug_create_info{};
    if (enable_validation_layers_) {
        create_info.enabledLayerCount = static_cast<uint32_t>(s_validation_layers.size());
        create_info.ppEnabledLayerNames = s_validation_layers.data();
        
        // Setup debug messenger create info for instance creation
        debug_create_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        debug_create_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                                          VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                                          VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        debug_create_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                                       VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                                       VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        debug_create_info.pfnUserCallback = debug_callback;
        
        create_info.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debug_create_info;
    } else {
        create_info.enabledLayerCount = 0;
        create_info.pNext = nullptr;
    }
    
    VkResult result = vkCreateInstance(&create_info, nullptr, &instance_);
    return result == VK_SUCCESS;
}

bool VulkanRenderer::setup_debug_messenger() {
    if (!enable_validation_layers_) return true;
    
    VkDebugUtilsMessengerCreateInfoEXT create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    create_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                                VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                                VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    create_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                             VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                             VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    create_info.pfnUserCallback = debug_callback;
    create_info.pUserData = nullptr;
    
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
        instance_, "vkCreateDebugUtilsMessengerEXT");
    
    if (func != nullptr) {
        VkResult result = func(instance_, &create_info, nullptr, &debug_messenger_);
        return result == VK_SUCCESS;
    }
    
    return false;
}

bool VulkanRenderer::create_surface() {
    if (!window_) return false;
    
    VkResult result = glfwCreateWindowSurface(instance_, window_, nullptr, &surface_);
    return result == VK_SUCCESS;
}

bool VulkanRenderer::pick_physical_device() {
    uint32_t device_count = 0;
    vkEnumeratePhysicalDevices(instance_, &device_count, nullptr);
    
    if (device_count == 0) {
        std::cerr << "Failed to find GPUs with Vulkan support" << std::endl;
        return false;
    }
    
    std::vector<VkPhysicalDevice> devices(device_count);
    vkEnumeratePhysicalDevices(instance_, &device_count, devices.data());
    
    for (const auto& device : devices) {
        if (is_device_suitable(device)) {
            physical_device_ = device;
            break;
        }
    }
    
    if (physical_device_ == VK_NULL_HANDLE) {
        std::cerr << "Failed to find suitable GPU" << std::endl;
        return false;
    }
    
    // Store device properties and features
    vkGetPhysicalDeviceProperties(physical_device_, &device_properties_);
    vkGetPhysicalDeviceFeatures(physical_device_, &device_features_);
    vkGetPhysicalDeviceMemoryProperties(physical_device_, &memory_properties_);
    
    std::cout << "Selected GPU: " << device_properties_.deviceName << std::endl;
    return true;
}

bool VulkanRenderer::is_device_suitable(VkPhysicalDevice device) {
    QueueFamilyIndices indices = find_queue_families(device);
    
    bool extensions_supported = check_device_extension_support(device);
    
    bool swapchain_adequate = false;
    if (extensions_supported) {
        SwapChainSupportDetails swapchain_support = query_swapchain_support(device);
        swapchain_adequate = !swapchain_support.formats.empty() && 
                           !swapchain_support.present_modes.empty();
    }
    
    VkPhysicalDeviceFeatures supported_features;
    vkGetPhysicalDeviceFeatures(device, &supported_features);
    
    return indices.is_complete() && extensions_supported && 
           (surface_ == VK_NULL_HANDLE || swapchain_adequate) &&
           supported_features.samplerAnisotropy;
}

VulkanRenderer::QueueFamilyIndices VulkanRenderer::find_queue_families(VkPhysicalDevice device) {
    QueueFamilyIndices indices;
    
    uint32_t queue_family_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, nullptr);
    
    std::vector<VkQueueFamilyProperties> queue_families(queue_family_count);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, queue_families.data());
    
    int i = 0;
    for (const auto& queue_family : queue_families) {
        if (queue_family.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            indices.graphics_family = i;
        }
        
        if (queue_family.queueFlags & VK_QUEUE_COMPUTE_BIT) {
            indices.compute_family = i;
        }
        
        if (queue_family.queueFlags & VK_QUEUE_TRANSFER_BIT) {
            indices.transfer_family = i;
        }
        
        if (surface_ != VK_NULL_HANDLE) {
            VkBool32 present_support = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface_, &present_support);
            if (present_support) {
                indices.present_family = i;
            }
        } else {
            // For headless rendering, use graphics queue for presentation
            indices.present_family = indices.graphics_family;
        }
        
        if (indices.is_complete()) {
            break;
        }
        
        i++;
    }
    
    return indices;
}

bool VulkanRenderer::check_device_extension_support(VkPhysicalDevice device) {
    uint32_t extension_count;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extension_count, nullptr);
    
    std::vector<VkExtensionProperties> available_extensions(extension_count);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extension_count, available_extensions.data());
    
    std::set<std::string> required_extensions(s_device_extensions.begin(), s_device_extensions.end());
    
    for (const auto& extension : available_extensions) {
        required_extensions.erase(extension.extensionName);
    }
    
    return required_extensions.empty();
}

VulkanRenderer::SwapChainSupportDetails VulkanRenderer::query_swapchain_support(VkPhysicalDevice device) {
    SwapChainSupportDetails details;
    
    if (surface_ == VK_NULL_HANDLE) {
        return details;
    }
    
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface_, &details.capabilities);
    
    uint32_t format_count;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface_, &format_count, nullptr);
    
    if (format_count != 0) {
        details.formats.resize(format_count);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface_, &format_count, details.formats.data());
    }
    
    uint32_t present_mode_count;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface_, &present_mode_count, nullptr);
    
    if (present_mode_count != 0) {
        details.present_modes.resize(present_mode_count);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface_, &present_mode_count, details.present_modes.data());
    }
    
    return details;
}

bool VulkanRenderer::create_logical_device() {
    QueueFamilyIndices indices = find_queue_families(physical_device_);
    
    std::vector<VkDeviceQueueCreateInfo> queue_create_infos;
    std::set<uint32_t> unique_queue_families = {
        indices.graphics_family.value(), 
        indices.present_family.value(),
        indices.compute_family.value_or(indices.graphics_family.value()),
        indices.transfer_family.value_or(indices.graphics_family.value())
    };
    
    float queue_priority = 1.0f;
    for (uint32_t queue_family : unique_queue_families) {
        VkDeviceQueueCreateInfo queue_create_info{};
        queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queue_create_info.queueFamilyIndex = queue_family;
        queue_create_info.queueCount = 1;
        queue_create_info.pQueuePriorities = &queue_priority;
        queue_create_infos.push_back(queue_create_info);
    }
    
    VkPhysicalDeviceFeatures device_features{};
    device_features.samplerAnisotropy = VK_TRUE;
    device_features.fillModeNonSolid = VK_TRUE;
    device_features.wideLines = VK_TRUE;
    
    VkDeviceCreateInfo create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    create_info.pQueueCreateInfos = queue_create_infos.data();
    create_info.queueCreateInfoCount = static_cast<uint32_t>(queue_create_infos.size());
    create_info.pEnabledFeatures = &device_features;
    
    create_info.enabledExtensionCount = static_cast<uint32_t>(s_device_extensions.size());
    create_info.ppEnabledExtensionNames = s_device_extensions.data();
    
    if (enable_validation_layers_) {
        create_info.enabledLayerCount = static_cast<uint32_t>(s_validation_layers.size());
        create_info.ppEnabledLayerNames = s_validation_layers.data();
    } else {
        create_info.enabledLayerCount = 0;
    }
    
    VkResult result = vkCreateDevice(physical_device_, &create_info, nullptr, &device_);
    if (result != VK_SUCCESS) {
        return false;
    }
    
    // Get queue handles
    vkGetDeviceQueue(device_, indices.graphics_family.value(), 0, &graphics_queue_);
    vkGetDeviceQueue(device_, indices.present_family.value(), 0, &present_queue_);
    vkGetDeviceQueue(device_, indices.compute_family.value_or(indices.graphics_family.value()), 0, &compute_queue_);
    vkGetDeviceQueue(device_, indices.transfer_family.value_or(indices.graphics_family.value()), 0, &transfer_queue_);
    
    return true;
}

bool VulkanRenderer::create_swapchain() {
    if (surface_ == VK_NULL_HANDLE) {
        return false; // Can't create swapchain without surface
    }
    
    SwapChainSupportDetails swapchain_support = query_swapchain_support(physical_device_);
    
    VkSurfaceFormatKHR surface_format = choose_swap_surface_format(swapchain_support.formats);
    VkPresentModeKHR present_mode = choose_swap_present_mode(swapchain_support.present_modes);
    VkExtent2D extent = choose_swap_extent(swapchain_support.capabilities);
    
    uint32_t image_count = swapchain_support.capabilities.minImageCount + 1;
    if (swapchain_support.capabilities.maxImageCount > 0 && 
        image_count > swapchain_support.capabilities.maxImageCount) {
        image_count = swapchain_support.capabilities.maxImageCount;
    }
    
    VkSwapchainCreateInfoKHR create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    create_info.surface = surface_;
    create_info.minImageCount = image_count;
    create_info.imageFormat = surface_format.format;
    create_info.imageColorSpace = surface_format.colorSpace;
    create_info.imageExtent = extent;
    create_info.imageArrayLayers = 1;
    create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    
    QueueFamilyIndices indices = find_queue_families(physical_device_);
    uint32_t queue_family_indices[] = {
        indices.graphics_family.value(), 
        indices.present_family.value()
    };
    
    if (indices.graphics_family != indices.present_family) {
        create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        create_info.queueFamilyIndexCount = 2;
        create_info.pQueueFamilyIndices = queue_family_indices;
    } else {
        create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        create_info.queueFamilyIndexCount = 0;
        create_info.pQueueFamilyIndices = nullptr;
    }
    
    create_info.preTransform = swapchain_support.capabilities.currentTransform;
    create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    create_info.presentMode = present_mode;
    create_info.clipped = VK_TRUE;
    create_info.oldSwapchain = VK_NULL_HANDLE;
    
    VkResult result = vkCreateSwapchainKHR(device_, &create_info, nullptr, &swapchain_);
    if (result != VK_SUCCESS) {
        return false;
    }
    
    // Get swapchain images
    vkGetSwapchainImagesKHR(device_, swapchain_, &image_count, nullptr);
    swapchain_images_.resize(image_count);
    vkGetSwapchainImagesKHR(device_, swapchain_, &image_count, swapchain_images_.data());
    
    swapchain_image_format_ = surface_format.format;
    swapchain_extent_ = extent;
    
    return true;
}

VkSurfaceFormatKHR VulkanRenderer::choose_swap_surface_format(const std::vector<VkSurfaceFormatKHR>& available_formats) {
    for (const auto& available_format : available_formats) {
        if (available_format.format == VK_FORMAT_B8G8R8A8_SRGB &&
            available_format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return available_format;
        }
    }
    
    return available_formats[0];
}

VkPresentModeKHR VulkanRenderer::choose_swap_present_mode(const std::vector<VkPresentModeKHR>& available_present_modes) {
    for (const auto& available_present_mode : available_present_modes) {
        if (available_present_mode == VK_PRESENT_MODE_MAILBOX_KHR) {
            return available_present_mode;
        }
    }
    
    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D VulkanRenderer::choose_swap_extent(const VkSurfaceCapabilitiesKHR& capabilities) {
    if (capabilities.currentExtent.width != UINT32_MAX) {
        return capabilities.currentExtent;
    } else {
        int width, height;
        glfwGetFramebufferSize(window_, &width, &height);
        
        VkExtent2D actual_extent = {
            static_cast<uint32_t>(width),
            static_cast<uint32_t>(height)
        };
        
        actual_extent.width = std::clamp(actual_extent.width,
            capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
        actual_extent.height = std::clamp(actual_extent.height,
            capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
        
        return actual_extent;
    }
}

bool VulkanRenderer::create_image_views() {
    swapchain_image_views_.resize(swapchain_images_.size());
    
    for (size_t i = 0; i < swapchain_images_.size(); i++) {
        VkImageViewCreateInfo create_info{};
        create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        create_info.image = swapchain_images_[i];
        create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
        create_info.format = swapchain_image_format_;
        create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        create_info.subresourceRange.baseMipLevel = 0;
        create_info.subresourceRange.levelCount = 1;
        create_info.subresourceRange.baseArrayLayer = 0;
        create_info.subresourceRange.layerCount = 1;
        
        VkResult result = vkCreateImageView(device_, &create_info, nullptr, &swapchain_image_views_[i]);
        if (result != VK_SUCCESS) {
            return false;
        }
    }
    
    return true;
}

bool VulkanRenderer::create_render_pass() {
    VkAttachmentDescription color_attachment{};
    color_attachment.format = swapchain_image_format_;
    color_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
    color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    color_attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    
    VkAttachmentReference color_attachment_ref{};
    color_attachment_ref.attachment = 0;
    color_attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    
    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &color_attachment_ref;
    
    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    
    VkRenderPassCreateInfo render_pass_info{};
    render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    render_pass_info.attachmentCount = 1;
    render_pass_info.pAttachments = &color_attachment;
    render_pass_info.subpassCount = 1;
    render_pass_info.pSubpasses = &subpass;
    render_pass_info.dependencyCount = 1;
    render_pass_info.pDependencies = &dependency;
    
    VkResult result = vkCreateRenderPass(device_, &render_pass_info, nullptr, &render_pass_);
    return result == VK_SUCCESS;
}

bool VulkanRenderer::create_command_pool() {
    QueueFamilyIndices queue_family_indices = find_queue_families(physical_device_);
    
    // Graphics command pool
    VkCommandPoolCreateInfo pool_info{};
    pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    pool_info.queueFamilyIndex = queue_family_indices.graphics_family.value();
    
    VkResult result = vkCreateCommandPool(device_, &pool_info, nullptr, &graphics_command_pool_);
    if (result != VK_SUCCESS) {
        return false;
    }
    
    // Transfer command pool (if different from graphics)
    if (queue_family_indices.transfer_family.has_value() && 
        queue_family_indices.transfer_family != queue_family_indices.graphics_family) {
        pool_info.queueFamilyIndex = queue_family_indices.transfer_family.value();
        result = vkCreateCommandPool(device_, &pool_info, nullptr, &transfer_command_pool_);
        if (result != VK_SUCCESS) {
            return false;
        }
    } else {
        transfer_command_pool_ = graphics_command_pool_;
    }
    
    return true;
}

bool VulkanRenderer::create_command_buffers() {
    command_buffers_.resize(MAX_FRAMES_IN_FLIGHT);
    
    VkCommandBufferAllocateInfo alloc_info{};
    alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    alloc_info.commandPool = graphics_command_pool_;
    alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    alloc_info.commandBufferCount = static_cast<uint32_t>(command_buffers_.size());
    
    std::vector<VkCommandBuffer> raw_command_buffers(command_buffers_.size());
    VkResult result = vkAllocateCommandBuffers(device_, &alloc_info, raw_command_buffers.data());
    if (result != VK_SUCCESS) {
        return false;
    }
    
    for (size_t i = 0; i < command_buffers_.size(); i++) {
        command_buffers_[i].command_buffer = raw_command_buffers[i];
        command_buffers_[i].command_pool = graphics_command_pool_;
        command_buffers_[i].fence = VK_NULL_HANDLE; // Will be set later
        command_buffers_[i].is_recording = false;
        command_buffers_[i].is_submitted = false;
    }
    
    return true;
}

bool VulkanRenderer::create_sync_objects() {
    image_available_semaphores_.resize(MAX_FRAMES_IN_FLIGHT);
    render_finished_semaphores_.resize(MAX_FRAMES_IN_FLIGHT);
    in_flight_fences_.resize(MAX_FRAMES_IN_FLIGHT);
    images_in_flight_.resize(swapchain_images_.size(), VK_NULL_HANDLE);
    
    VkSemaphoreCreateInfo semaphore_info{};
    semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    
    VkFenceCreateInfo fence_info{};
    fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        VkResult result1 = vkCreateSemaphore(device_, &semaphore_info, nullptr, &image_available_semaphores_[i]);
        VkResult result2 = vkCreateSemaphore(device_, &semaphore_info, nullptr, &render_finished_semaphores_[i]);
        VkResult result3 = vkCreateFence(device_, &fence_info, nullptr, &in_flight_fences_[i]);
        
        if (result1 != VK_SUCCESS || result2 != VK_SUCCESS || result3 != VK_SUCCESS) {
            return false;
        }
        
        // Associate fence with command buffer
        command_buffers_[i].fence = in_flight_fences_[i];
    }
    
    return true;
}

VkCommandBuffer VulkanRenderer::get_current_command_buffer() const {
    if (current_frame_ < command_buffers_.size()) {
        return command_buffers_[current_frame_].command_buffer;
    }
    return VK_NULL_HANDLE;
}

// =============================================================================
// UTILITY FUNCTIONS
// =============================================================================

bool is_vulkan_available() {
    uint32_t extension_count = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &extension_count, nullptr);
    return extension_count > 0;
}

std::vector<const char*> get_required_extensions() {
    return ::ecscope::rendering::get_required_extensions();
}

// =============================================================================
// RENDERER INTERFACE IMPLEMENTATION - REMAINING METHODS
// =============================================================================

// Frame management
void VulkanRenderer::begin_frame() {
    frame_start_time_ = std::chrono::high_resolution_clock::now();
    
    // Wait for the fence to be signaled
    vkWaitForFences(device_, 1, &in_flight_fences_[current_frame_], VK_TRUE, UINT64_MAX);
    
    // Acquire next image from swapchain
    if (surface_ != VK_NULL_HANDLE) {
        VkResult result = vkAcquireNextImageKHR(device_, swapchain_, UINT64_MAX,
                                              image_available_semaphores_[current_frame_], VK_NULL_HANDLE, &image_index_);
        
        if (result == VK_ERROR_OUT_OF_DATE_KHR) {
            // Swapchain needs to be recreated (e.g., window resized)
            // For now, just continue
            return;
        } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
            throw std::runtime_error("Failed to acquire swapchain image!");
        }
        
        // Check if a previous frame is using this image (i.e., there is its fence to wait on)
        if (images_in_flight_[image_index_] != VK_NULL_HANDLE) {
            vkWaitForFences(device_, 1, &images_in_flight_[image_index_], VK_TRUE, UINT64_MAX);
        }
        // Mark the image as now being in use by this frame
        images_in_flight_[image_index_] = in_flight_fences_[current_frame_];
    }
    
    // Reset fence for this frame
    vkResetFences(device_, 1, &in_flight_fences_[current_frame_]);
    
    // Begin recording command buffer
    VkCommandBuffer command_buffer = command_buffers_[current_frame_].command_buffer;
    vkResetCommandBuffer(command_buffer, 0);
    
    VkCommandBufferBeginInfo begin_info{};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.flags = 0;
    begin_info.pInheritanceInfo = nullptr;
    
    VkResult result = vkBeginCommandBuffer(command_buffer, &begin_info);
    if (result != VK_SUCCESS) {
        throw std::runtime_error("Failed to begin recording command buffer!");
    }
    
    command_buffers_[current_frame_].is_recording = true;
    command_buffers_[current_frame_].is_submitted = false;
    
    // Reset frame statistics
    {
        std::lock_guard<std::mutex> lock(stats_mutex_);
        frame_stats_.draw_calls = 0;
        frame_stats_.vertices_rendered = 0;
    }
}

void VulkanRenderer::end_frame() {
    VkCommandBuffer command_buffer = command_buffers_[current_frame_].command_buffer;
    
    // End command buffer recording
    VkResult result = vkEndCommandBuffer(command_buffer);
    if (result != VK_SUCCESS) {
        throw std::runtime_error("Failed to record command buffer!");
    }
    
    command_buffers_[current_frame_].is_recording = false;
    
    // Submit command buffer
    VkSubmitInfo submit_info{};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    
    VkSemaphore wait_semaphores[] = {image_available_semaphores_[current_frame_]};
    VkPipelineStageFlags wait_stages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submit_info.waitSemaphoreCount = surface_ != VK_NULL_HANDLE ? 1 : 0;
    submit_info.pWaitSemaphores = wait_semaphores;
    submit_info.pWaitDstStageMask = wait_stages;
    
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &command_buffer;
    
    VkSemaphore signal_semaphores[] = {render_finished_semaphores_[current_frame_]};
    submit_info.signalSemaphoreCount = surface_ != VK_NULL_HANDLE ? 1 : 0;
    submit_info.pSignalSemaphores = signal_semaphores;
    
    result = vkQueueSubmit(graphics_queue_, 1, &submit_info, in_flight_fences_[current_frame_]);
    if (result != VK_SUCCESS) {
        throw std::runtime_error("Failed to submit draw command buffer!");
    }
    
    command_buffers_[current_frame_].is_submitted = true;
    
    // Present to swapchain
    if (surface_ != VK_NULL_HANDLE) {
        VkPresentInfoKHR present_info{};
        present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        present_info.waitSemaphoreCount = 1;
        present_info.pWaitSemaphores = signal_semaphores;
        
        VkSwapchainKHR swapchains[] = {swapchain_};
        present_info.swapchainCount = 1;
        present_info.pSwapchains = swapchains;
        present_info.pImageIndices = &image_index_;
        present_info.pResults = nullptr;
        
        result = vkQueuePresentKHR(present_queue_, &present_info);
        if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
            // Swapchain needs to be recreated
        } else if (result != VK_SUCCESS) {
            throw std::runtime_error("Failed to present swapchain image!");
        }
    }
    
    // Update frame statistics
    auto frame_end_time = std::chrono::high_resolution_clock::now();
    auto frame_duration = std::chrono::duration<float, std::milli>(frame_end_time - frame_start_time_);
    
    {
        std::lock_guard<std::mutex> lock(stats_mutex_);
        frame_stats_.frame_time_ms = frame_duration.count();
        frame_stats_.gpu_time_ms = frame_duration.count(); // Simplified - would need queries for accurate GPU timing
    }
    
    current_frame_ = (current_frame_ + 1) % MAX_FRAMES_IN_FLIGHT;
}

void VulkanRenderer::set_render_target(TextureHandle color_target, TextureHandle depth_target) {
    // For now, we'll use the default swapchain framebuffer
    // In a full implementation, this would create custom framebuffers
    // TODO: Implement custom render target support
}

void VulkanRenderer::clear(const std::array<float, 4>& color, float depth, uint8_t stencil) {
    VkCommandBuffer command_buffer = get_current_command_buffer();
    if (command_buffer == VK_NULL_HANDLE || !command_buffers_[current_frame_].is_recording) {
        return;
    }
    
    VkClearColorValue clear_color;
    clear_color.float32[0] = color[0];
    clear_color.float32[1] = color[1];
    clear_color.float32[2] = color[2];
    clear_color.float32[3] = color[3];
    
    VkClearDepthStencilValue clear_depth_stencil;
    clear_depth_stencil.depth = depth;
    clear_depth_stencil.stencil = stencil;
    
    // For now, clear using render pass clear values
    // In a full implementation, this would use vkCmdClearColorImage and vkCmdClearDepthStencilImage
}

void VulkanRenderer::set_viewport(const Viewport& viewport) {
    VkCommandBuffer command_buffer = get_current_command_buffer();
    if (command_buffer == VK_NULL_HANDLE || !command_buffers_[current_frame_].is_recording) {
        return;
    }
    
    VkViewport vk_viewport{};
    vk_viewport.x = viewport.x;
    vk_viewport.y = viewport.y;
    vk_viewport.width = viewport.width;
    vk_viewport.height = viewport.height;
    vk_viewport.minDepth = viewport.min_depth;
    vk_viewport.maxDepth = viewport.max_depth;
    
    vkCmdSetViewport(command_buffer, 0, 1, &vk_viewport);
}

void VulkanRenderer::set_scissor(const ScissorRect& scissor) {
    VkCommandBuffer command_buffer = get_current_command_buffer();
    if (command_buffer == VK_NULL_HANDLE || !command_buffers_[current_frame_].is_recording) {
        return;
    }
    
    VkRect2D vk_scissor{};
    vk_scissor.offset = {scissor.x, scissor.y};
    vk_scissor.extent = {scissor.width, scissor.height};
    
    vkCmdSetScissor(command_buffer, 0, 1, &vk_scissor);
}

// Shader and pipeline management - simplified implementations
ShaderHandle VulkanRenderer::create_shader(const std::string& vertex_source, const std::string& fragment_source, const std::string& debug_name) {
    // TODO: Implement shader creation with SPIR-V compilation
    // For now, return invalid handle
    std::cerr << "Shader creation not yet implemented!" << std::endl;
    return ShaderHandle{};
}

ShaderHandle VulkanRenderer::create_compute_shader(const std::string& compute_source, const std::string& debug_name) {
    // TODO: Implement compute shader creation
    std::cerr << "Compute shader creation not yet implemented!" << std::endl;
    return ShaderHandle{};
}

void VulkanRenderer::destroy_shader(ShaderHandle handle) {
    if (!handle.is_valid()) return;
    
    std::lock_guard<std::mutex> lock(resource_mutex_);
    auto it = shaders_.find(handle.id());
    if (it != shaders_.end()) {
        // Cleanup shader resources (implemented in the destructor)
        shaders_.erase(it);
    }
}

// Pipeline state management - simplified implementations
void VulkanRenderer::set_shader(ShaderHandle handle) {
    current_shader_ = handle;
    // TODO: Bind pipeline state
}

void VulkanRenderer::set_render_state(const RenderState& state) {
    current_render_state_ = state;
    // TODO: Apply render state to current pipeline
}

void VulkanRenderer::set_vertex_buffers(std::span<const BufferHandle> buffers, std::span<const uint64_t> offsets) {
    VkCommandBuffer command_buffer = get_current_command_buffer();
    if (command_buffer == VK_NULL_HANDLE || !command_buffers_[current_frame_].is_recording) {
        return;
    }
    
    bound_vertex_buffers_.clear();
    bound_vertex_buffers_.reserve(buffers.size());
    
    std::vector<VkBuffer> vk_buffers;
    std::vector<VkDeviceSize> vk_offsets;
    vk_buffers.reserve(buffers.size());
    vk_offsets.reserve(buffers.size());
    
    std::lock_guard<std::mutex> lock(resource_mutex_);
    for (size_t i = 0; i < buffers.size(); ++i) {
        bound_vertex_buffers_.push_back(buffers[i]);
        
        auto it = buffers_.find(buffers[i].id());
        if (it != buffers_.end()) {
            vk_buffers.push_back(it->second.buffer);
            vk_offsets.push_back(i < offsets.size() ? offsets[i] : 0);
        }
    }
    
    if (!vk_buffers.empty()) {
        vkCmdBindVertexBuffers(command_buffer, 0, static_cast<uint32_t>(vk_buffers.size()), vk_buffers.data(), vk_offsets.data());
    }
}

void VulkanRenderer::set_index_buffer(BufferHandle buffer, size_t offset, bool use_32bit_indices) {
    VkCommandBuffer command_buffer = get_current_command_buffer();
    if (command_buffer == VK_NULL_HANDLE || !command_buffers_[current_frame_].is_recording) {
        return;
    }
    
    bound_index_buffer_ = buffer;
    index_buffer_32bit_ = use_32bit_indices;
    
    std::lock_guard<std::mutex> lock(resource_mutex_);
    auto it = buffers_.find(buffer.id());
    if (it != buffers_.end()) {
        VkIndexType index_type = use_32bit_indices ? VK_INDEX_TYPE_UINT32 : VK_INDEX_TYPE_UINT16;
        vkCmdBindIndexBuffer(command_buffer, it->second.buffer, offset, index_type);
    }
}

void VulkanRenderer::set_vertex_layout(const VertexLayout& layout) {
    current_vertex_layout_ = layout;
    // TODO: Apply vertex layout to pipeline
}

// Resource binding - simplified implementations
void VulkanRenderer::bind_texture(uint32_t slot, TextureHandle texture) {
    // TODO: Bind texture to descriptor set
}

void VulkanRenderer::bind_textures(uint32_t first_slot, std::span<const TextureHandle> textures) {
    // TODO: Bind multiple textures to descriptor sets
}

void VulkanRenderer::bind_uniform_buffer(uint32_t slot, BufferHandle buffer, size_t offset, size_t size) {
    // TODO: Bind uniform buffer to descriptor set
}

void VulkanRenderer::bind_storage_buffer(uint32_t slot, BufferHandle buffer, size_t offset, size_t size) {
    // TODO: Bind storage buffer to descriptor set
}

void VulkanRenderer::set_push_constants(uint32_t offset, uint32_t size, const void* data) {
    VkCommandBuffer command_buffer = get_current_command_buffer();
    if (command_buffer == VK_NULL_HANDLE || !command_buffers_[current_frame_].is_recording) {
        return;
    }
    
    // TODO: Push constants need a pipeline layout to work
    // vkCmdPushConstants(command_buffer, pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, offset, size, data);
}

// Draw commands
void VulkanRenderer::draw_indexed(const DrawIndexedCommand& cmd) {
    VkCommandBuffer command_buffer = get_current_command_buffer();
    if (command_buffer == VK_NULL_HANDLE || !command_buffers_[current_frame_].is_recording) {
        return;
    }
    
    vkCmdDrawIndexed(command_buffer, cmd.index_count, cmd.instance_count, cmd.first_index, cmd.vertex_offset, cmd.first_instance);
    
    // Update statistics
    {
        std::lock_guard<std::mutex> lock(stats_mutex_);
        frame_stats_.draw_calls++;
        frame_stats_.vertices_rendered += cmd.index_count * cmd.instance_count;
    }
}

void VulkanRenderer::draw(const DrawCommand& cmd) {
    VkCommandBuffer command_buffer = get_current_command_buffer();
    if (command_buffer == VK_NULL_HANDLE || !command_buffers_[current_frame_].is_recording) {
        return;
    }
    
    vkCmdDraw(command_buffer, cmd.vertex_count, cmd.instance_count, cmd.first_vertex, cmd.first_instance);
    
    // Update statistics
    {
        std::lock_guard<std::mutex> lock(stats_mutex_);
        frame_stats_.draw_calls++;
        frame_stats_.vertices_rendered += cmd.vertex_count * cmd.instance_count;
    }
}

void VulkanRenderer::dispatch(const DispatchCommand& cmd) {
    VkCommandBuffer command_buffer = get_current_command_buffer();
    if (command_buffer == VK_NULL_HANDLE || !command_buffers_[current_frame_].is_recording) {
        return;
    }
    
    vkCmdDispatch(command_buffer, cmd.group_count_x, cmd.group_count_y, cmd.group_count_z);
}

// Debugging and profiling
void VulkanRenderer::push_debug_marker(const std::string& name) {
    if (!enable_validation_layers_) return;
    
    VkCommandBuffer command_buffer = get_current_command_buffer();
    if (command_buffer == VK_NULL_HANDLE) return;
    
    VkDebugUtilsLabelEXT label{};
    label.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT;
    label.pLabelName = name.c_str();
    label.color[0] = 1.0f;
    label.color[1] = 1.0f;
    label.color[2] = 1.0f;
    label.color[3] = 1.0f;
    
    auto func = (PFN_vkCmdBeginDebugUtilsLabelEXT)vkGetDeviceProcAddr(device_, "vkCmdBeginDebugUtilsLabelEXT");
    if (func != nullptr) {
        func(command_buffer, &label);
    }
}

void VulkanRenderer::pop_debug_marker() {
    if (!enable_validation_layers_) return;
    
    VkCommandBuffer command_buffer = get_current_command_buffer();
    if (command_buffer == VK_NULL_HANDLE) return;
    
    auto func = (PFN_vkCmdEndDebugUtilsLabelEXT)vkGetDeviceProcAddr(device_, "vkCmdEndDebugUtilsLabelEXT");
    if (func != nullptr) {
        func(command_buffer);
    }
}

void VulkanRenderer::insert_debug_marker(const std::string& name) {
    if (!enable_validation_layers_) return;
    
    VkCommandBuffer command_buffer = get_current_command_buffer();
    if (command_buffer == VK_NULL_HANDLE) return;
    
    VkDebugUtilsLabelEXT label{};
    label.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT;
    label.pLabelName = name.c_str();
    label.color[0] = 1.0f;
    label.color[1] = 0.0f;
    label.color[2] = 0.0f;
    label.color[3] = 1.0f;
    
    auto func = (PFN_vkCmdInsertDebugUtilsLabelEXT)vkGetDeviceProcAddr(device_, "vkCmdInsertDebugUtilsLabelEXT");
    if (func != nullptr) {
        func(command_buffer, &label);
    }
}

IRenderer::FrameStats VulkanRenderer::get_frame_stats() const {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    return frame_stats_;
}

// Advanced features
void VulkanRenderer::wait_idle() {
    if (device_ != VK_NULL_HANDLE) {
        vkDeviceWaitIdle(device_);
    }
}

uint64_t VulkanRenderer::create_fence() {
    VkFenceCreateInfo fence_info{};
    fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fence_info.flags = 0;
    
    VkFence fence;
    VkResult result = vkCreateFence(device_, &fence_info, nullptr, &fence);
    if (result != VK_SUCCESS) {
        return 0;
    }
    
    uint64_t fence_id = next_fence_id_.fetch_add(1);
    
    std::lock_guard<std::mutex> lock(resource_mutex_);
    fences_[fence_id] = fence;
    
    return fence_id;
}

void VulkanRenderer::wait_for_fence(uint64_t fence_id, uint64_t timeout_ns) {
    std::lock_guard<std::mutex> lock(resource_mutex_);
    auto it = fences_.find(fence_id);
    if (it != fences_.end()) {
        vkWaitForFences(device_, 1, &it->second, VK_TRUE, timeout_ns);
    }
}

bool VulkanRenderer::is_fence_signaled(uint64_t fence_id) {
    std::lock_guard<std::mutex> lock(resource_mutex_);
    auto it = fences_.find(fence_id);
    if (it != fences_.end()) {
        VkResult result = vkGetFenceStatus(device_, it->second);
        return result == VK_SUCCESS;
    }
    return false;
}

} // namespace ecscope::rendering