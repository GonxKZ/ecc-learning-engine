/**
 * @file vulkan_backend.hpp
 * @brief Professional Vulkan Rendering Backend
 * 
 * High-performance Vulkan implementation with modern features,
 * optimal resource management, and robust error handling.
 * 
 * @author ECScope Development Team
 * @version 1.0.0
 */

#pragma once

#include "renderer.hpp"
#include <vulkan/vulkan.h>
#include <memory>
#include <vector>
#include <unordered_map>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <array>

// Forward declaration
struct GLFWwindow;

namespace ecscope::rendering {

// =============================================================================
// VULKAN-SPECIFIC STRUCTURES
// =============================================================================

/**
 * @brief Vulkan buffer resource
 */
struct VulkanBuffer {
    VkBuffer buffer = VK_NULL_HANDLE;
    VkDeviceMemory memory = VK_NULL_HANDLE;
    VkDeviceSize size = 0;
    VkBufferUsageFlags usage = 0;
    VkMemoryPropertyFlags memory_properties = 0;
    void* mapped_data = nullptr;
    std::string debug_name;

    bool is_mapped() const { return mapped_data != nullptr; }
};

/**
 * @brief Vulkan texture resource
 */
struct VulkanTexture {
    VkImage image = VK_NULL_HANDLE;
    VkImageView image_view = VK_NULL_HANDLE;
    VkDeviceMemory memory = VK_NULL_HANDLE;
    VkSampler sampler = VK_NULL_HANDLE;
    VkFormat format = VK_FORMAT_UNDEFINED;
    VkExtent3D extent = {};
    uint32_t mip_levels = 1;
    uint32_t array_layers = 1;
    VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT;
    VkImageLayout current_layout = VK_IMAGE_LAYOUT_UNDEFINED;
    std::string debug_name;
};

/**
 * @brief Vulkan shader resource
 */
struct VulkanShader {
    VkShaderModule vertex_module = VK_NULL_HANDLE;
    VkShaderModule fragment_module = VK_NULL_HANDLE;
    VkShaderModule compute_module = VK_NULL_HANDLE;
    VkPipelineLayout pipeline_layout = VK_NULL_HANDLE;
    VkPipeline graphics_pipeline = VK_NULL_HANDLE;
    VkPipeline compute_pipeline = VK_NULL_HANDLE;
    VkDescriptorSetLayout descriptor_set_layout = VK_NULL_HANDLE;
    VkRenderPass render_pass = VK_NULL_HANDLE;
    std::string debug_name;
};

/**
 * @brief Vulkan command buffer with state
 */
struct VulkanCommandBuffer {
    VkCommandBuffer command_buffer = VK_NULL_HANDLE;
    VkCommandPool command_pool = VK_NULL_HANDLE;
    VkFence fence = VK_NULL_HANDLE;
    bool is_recording = false;
    bool is_submitted = false;
};

// =============================================================================
// VULKAN RENDERER IMPLEMENTATION
// =============================================================================

/**
 * @brief High-performance Vulkan rendering backend
 * 
 * This implementation provides:
 * - Efficient memory management with custom allocators
 * - Automatic resource lifecycle management
 * - Command buffer pooling and recycling
 * - Pipeline state caching
 * - Robust error handling and debugging
 */
class VulkanRenderer : public IRenderer {
public:
    VulkanRenderer();
    ~VulkanRenderer() override;

    // Disable copy/move
    VulkanRenderer(const VulkanRenderer&) = delete;
    VulkanRenderer& operator=(const VulkanRenderer&) = delete;
    VulkanRenderer(VulkanRenderer&&) = delete;
    VulkanRenderer& operator=(VulkanRenderer&&) = delete;

    // =============================================================================
    // IRenderer INTERFACE IMPLEMENTATION
    // =============================================================================

    bool initialize(RenderingAPI api = RenderingAPI::Vulkan) override;
    void shutdown() override;
    RenderingAPI get_api() const override { return RenderingAPI::Vulkan; }
    RendererCaps get_capabilities() const override;

    // Resource management
    BufferHandle create_buffer(const BufferDesc& desc, const void* initial_data = nullptr) override;
    TextureHandle create_texture(const TextureDesc& desc, const void* initial_data = nullptr) override;
    ShaderHandle create_shader(const std::string& vertex_source, 
                              const std::string& fragment_source,
                              const std::string& debug_name = "") override;
    ShaderHandle create_compute_shader(const std::string& compute_source,
                                     const std::string& debug_name = "") override;

    void destroy_buffer(BufferHandle handle) override;
    void destroy_texture(TextureHandle handle) override;
    void destroy_shader(ShaderHandle handle) override;

    // Resource updates
    void update_buffer(BufferHandle handle, size_t offset, size_t size, const void* data) override;
    void update_texture(TextureHandle handle, uint32_t mip_level, uint32_t array_layer,
                       uint32_t x, uint32_t y, uint32_t z,
                       uint32_t width, uint32_t height, uint32_t depth,
                       const void* data) override;
    void generate_mipmaps(TextureHandle handle) override;

    // Frame management
    void begin_frame() override;
    void end_frame() override;
    void set_render_target(TextureHandle color_target = TextureHandle{},
                          TextureHandle depth_target = TextureHandle{}) override;
    void clear(const std::array<float, 4>& color = {0.0f, 0.0f, 0.0f, 1.0f},
              float depth = 1.0f, uint8_t stencil = 0) override;
    void set_viewport(const Viewport& viewport) override;
    void set_scissor(const ScissorRect& scissor) override;

    // Pipeline state management
    void set_shader(ShaderHandle handle) override;
    void set_render_state(const RenderState& state) override;
    void set_vertex_buffers(std::span<const BufferHandle> buffers,
                           std::span<const uint64_t> offsets = {}) override;
    void set_index_buffer(BufferHandle buffer, size_t offset = 0, bool use_32bit_indices = true) override;
    void set_vertex_layout(const VertexLayout& layout) override;

    // Resource binding
    void bind_texture(uint32_t slot, TextureHandle texture) override;
    void bind_textures(uint32_t first_slot, std::span<const TextureHandle> textures) override;
    void bind_uniform_buffer(uint32_t slot, BufferHandle buffer, 
                            size_t offset = 0, size_t size = 0) override;
    void bind_storage_buffer(uint32_t slot, BufferHandle buffer,
                            size_t offset = 0, size_t size = 0) override;
    void set_push_constants(uint32_t offset, uint32_t size, const void* data) override;

    // Draw commands
    void draw_indexed(const DrawIndexedCommand& cmd) override;
    void draw(const DrawCommand& cmd) override;
    void dispatch(const DispatchCommand& cmd) override;

    // Debugging & profiling
    void push_debug_marker(const std::string& name) override;
    void pop_debug_marker() override;
    void insert_debug_marker(const std::string& name) override;
    FrameStats get_frame_stats() const override;

    // Advanced features
    void wait_idle() override;
    uint64_t create_fence() override;
    void wait_for_fence(uint64_t fence_id, uint64_t timeout_ns = UINT64_MAX) override;
    bool is_fence_signaled(uint64_t fence_id) override;

    // =============================================================================
    // VULKAN-SPECIFIC METHODS
    // =============================================================================

    /**
     * @brief Get Vulkan device handle
     */
    VkDevice get_device() const { return device_; }

    /**
     * @brief Get Vulkan physical device
     */
    VkPhysicalDevice get_physical_device() const { return physical_device_; }

    /**
     * @brief Get current command buffer
     */
    VkCommandBuffer get_current_command_buffer() const;

    /**
     * @brief Set window for surface creation
     */
    void set_window(GLFWwindow* window) { window_ = window; }

private:
    // =============================================================================
    // INTERNAL STRUCTURES
    // =============================================================================

    struct QueueFamilyIndices {
        std::optional<uint32_t> graphics_family;
        std::optional<uint32_t> present_family;
        std::optional<uint32_t> compute_family;
        std::optional<uint32_t> transfer_family;

        bool is_complete() const {
            return graphics_family.has_value() && present_family.has_value();
        }
    };

    struct SwapChainSupportDetails {
        VkSurfaceCapabilitiesKHR capabilities;
        std::vector<VkSurfaceFormatKHR> formats;
        std::vector<VkPresentModeKHR> present_modes;
    };

    struct MemoryTypeInfo {
        uint32_t type_index;
        VkMemoryPropertyFlags properties;
        VkDeviceSize heap_size;
    };

    // =============================================================================
    // INITIALIZATION HELPERS
    // =============================================================================

    bool create_instance();
    bool setup_debug_messenger();
    bool create_surface();
    bool pick_physical_device();
    bool create_logical_device();
    bool create_swapchain();
    bool create_image_views();
    bool create_render_pass();
    bool create_command_pool();
    bool create_command_buffers();
    bool create_sync_objects();

    // Device selection helpers
    bool is_device_suitable(VkPhysicalDevice device);
    QueueFamilyIndices find_queue_families(VkPhysicalDevice device);
    bool check_device_extension_support(VkPhysicalDevice device);
    SwapChainSupportDetails query_swapchain_support(VkPhysicalDevice device);

    // Swapchain helpers
    VkSurfaceFormatKHR choose_swap_surface_format(const std::vector<VkSurfaceFormatKHR>& available_formats);
    VkPresentModeKHR choose_swap_present_mode(const std::vector<VkPresentModeKHR>& available_present_modes);
    VkExtent2D choose_swap_extent(const VkSurfaceCapabilitiesKHR& capabilities);

    // =============================================================================
    // MEMORY MANAGEMENT
    // =============================================================================

    uint32_t find_memory_type(uint32_t type_filter, VkMemoryPropertyFlags properties);
    bool allocate_buffer_memory(VkBuffer buffer, VkMemoryPropertyFlags properties, VkDeviceMemory& memory);
    bool allocate_image_memory(VkImage image, VkMemoryPropertyFlags properties, VkDeviceMemory& memory);

    // =============================================================================
    // RESOURCE HELPERS
    // =============================================================================

    VkShaderModule create_shader_module(const std::vector<uint32_t>& code);
    std::vector<uint32_t> compile_glsl_to_spirv(const std::string& source, VkShaderStageFlagBits stage);

    VkFormat texture_format_to_vulkan(TextureFormat format);
    VkBufferUsageFlags buffer_usage_to_vulkan(BufferUsage usage);
    VkSampleCountFlagBits samples_to_vulkan(uint32_t samples);

    // =============================================================================
    // COMMAND BUFFER MANAGEMENT
    // =============================================================================

    VkCommandBuffer begin_single_time_commands();
    void end_single_time_commands(VkCommandBuffer command_buffer);
    void transition_image_layout(VkImage image, VkFormat format, VkImageLayout old_layout, 
                                VkImageLayout new_layout, uint32_t mip_levels = 1, uint32_t layer_count = 1);

    // =============================================================================
    // DEBUGGING & VALIDATION
    // =============================================================================

    void setup_debug_utils();
    static VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(
        VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
        VkDebugUtilsMessageTypeFlagsEXT message_type,
        const VkDebugUtilsMessengerCallbackDataEXT* callback_data,
        void* user_data);

    // =============================================================================
    // MEMBER VARIABLES
    // =============================================================================

    // Core Vulkan objects
    VkInstance instance_ = VK_NULL_HANDLE;
    VkDebugUtilsMessengerEXT debug_messenger_ = VK_NULL_HANDLE;
    VkSurfaceKHR surface_ = VK_NULL_HANDLE;
    
    VkPhysicalDevice physical_device_ = VK_NULL_HANDLE;
    VkDevice device_ = VK_NULL_HANDLE;
    
    VkQueue graphics_queue_ = VK_NULL_HANDLE;
    VkQueue present_queue_ = VK_NULL_HANDLE;
    VkQueue compute_queue_ = VK_NULL_HANDLE;
    VkQueue transfer_queue_ = VK_NULL_HANDLE;
    
    // Swapchain
    VkSwapchainKHR swapchain_ = VK_NULL_HANDLE;
    std::vector<VkImage> swapchain_images_;
    std::vector<VkImageView> swapchain_image_views_;
    VkFormat swapchain_image_format_;
    VkExtent2D swapchain_extent_;
    
    VkRenderPass render_pass_ = VK_NULL_HANDLE;
    std::vector<VkFramebuffer> swapchain_framebuffers_;
    
    // Command pools and buffers
    VkCommandPool graphics_command_pool_ = VK_NULL_HANDLE;
    VkCommandPool transfer_command_pool_ = VK_NULL_HANDLE;
    std::vector<VulkanCommandBuffer> command_buffers_;
    
    // Synchronization objects
    static constexpr uint32_t MAX_FRAMES_IN_FLIGHT = 2;
    std::vector<VkSemaphore> image_available_semaphores_;
    std::vector<VkSemaphore> render_finished_semaphores_;
    std::vector<VkFence> in_flight_fences_;
    std::vector<VkFence> images_in_flight_;
    uint32_t current_frame_ = 0;
    uint32_t image_index_ = 0;
    
    // Resource management
    std::unordered_map<uint64_t, VulkanBuffer> buffers_;
    std::unordered_map<uint64_t, VulkanTexture> textures_;
    std::unordered_map<uint64_t, VulkanShader> shaders_;
    std::unordered_map<uint64_t, VkFence> fences_;
    
    std::atomic<uint64_t> next_resource_id_{1};
    std::atomic<uint64_t> next_fence_id_{1};
    
    // Current state
    ShaderHandle current_shader_;
    RenderState current_render_state_;
    VertexLayout current_vertex_layout_;
    std::vector<BufferHandle> bound_vertex_buffers_;
    BufferHandle bound_index_buffer_;
    bool index_buffer_32bit_ = true;
    
    // Device properties and capabilities
    VkPhysicalDeviceProperties device_properties_;
    VkPhysicalDeviceFeatures device_features_;
    VkPhysicalDeviceMemoryProperties memory_properties_;
    std::vector<MemoryTypeInfo> memory_types_;
    
    // Frame statistics
    mutable FrameStats frame_stats_;
    std::chrono::high_resolution_clock::time_point frame_start_time_;
    
    // Window handle
    GLFWwindow* window_ = nullptr;
    
    // Validation layers
    bool enable_validation_layers_ = false;
    const std::vector<const char*> validation_layers_ = {
        "VK_LAYER_KHRONOS_validation"
    };
    
    const std::vector<const char*> device_extensions_ = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };
    
    // Threading support
    mutable std::mutex resource_mutex_;
    mutable std::mutex stats_mutex_;
};

// =============================================================================
// HELPER FUNCTIONS
// =============================================================================

/**
 * @brief Check if Vulkan is available on the system
 */
bool is_vulkan_available();

/**
 * @brief Get required Vulkan instance extensions
 */
std::vector<const char*> get_required_extensions();

/**
 * @brief Utility to check Vulkan result codes
 */
#define VK_CHECK_RESULT(result, message) \
    do { \
        if ((result) != VK_SUCCESS) { \
            throw std::runtime_error(std::string("Vulkan error: ") + (message)); \
        } \
    } while(0)

} // namespace ecscope::rendering