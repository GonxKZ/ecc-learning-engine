/**
 * @file renderer.hpp
 * @brief Modern Multi-API Rendering Engine - Core Interface
 * 
 * Professional-grade rendering system with Vulkan/OpenGL backends,
 * deferred rendering, and advanced features for production use.
 * 
 * @author ECScope Development Team
 * @version 1.0.0
 */

#pragma once

#include <memory>
#include <vector>
#include <string>
#include <functional>
#include <unordered_map>
#include <variant>
#include <optional>
#include <array>
#include <span>

// Forward declarations for platform independence
struct GLFWwindow;

namespace ecscope::rendering {

// =============================================================================
// CORE TYPES & ENUMERATIONS
// =============================================================================

/**
 * @brief Supported rendering APIs
 */
enum class RenderingAPI : uint8_t {
    OpenGL,   ///< OpenGL 4.5+ backend
    Vulkan,   ///< Vulkan 1.2+ backend
    Auto      ///< Automatically select best available API
};

/**
 * @brief Buffer usage patterns for optimal memory management
 */
enum class BufferUsage : uint8_t {
    Static,    ///< Data written once, read many times (e.g., model data)
    Dynamic,   ///< Data updated frequently (e.g., transforms)
    Streaming, ///< Data updated every frame (e.g., particle data)
    Staging    ///< CPU-GPU transfer buffer
};

/**
 * @brief Texture formats for different use cases
 */
enum class TextureFormat : uint16_t {
    // Color formats
    R8,
    RG8,
    RGB8,
    RGBA8,
    R16F,
    RG16F,
    RGB16F,
    RGBA16F,
    R32F,
    RG32F,
    RGB32F,
    RGBA32F,
    
    // sRGB formats for proper gamma correction
    SRGB8,
    SRGBA8,
    
    // Depth formats
    Depth16,
    Depth24,
    Depth32F,
    Depth24Stencil8,
    Depth32FStencil8,
    
    // Compressed formats for memory efficiency
    BC1_RGB,
    BC1_RGBA,
    BC3_RGBA,
    BC4_R,
    BC5_RG,
    BC6H_RGB_UF16,
    BC7_RGBA
};

/**
 * @brief Primitive topology for geometry rendering
 */
enum class PrimitiveTopology : uint8_t {
    TriangleList,
    TriangleStrip,
    TriangleFan,
    LineList,
    LineStrip,
    PointList
};

/**
 * @brief Blend modes for transparency and effects
 */
enum class BlendMode : uint8_t {
    None,
    Alpha,           ///< Standard alpha blending
    Additive,        ///< Additive blending for effects
    Multiply,        ///< Multiplicative blending
    Screen,          ///< Screen blending
    PremultipliedAlpha
};

/**
 * @brief Culling modes for performance optimization
 */
enum class CullMode : uint8_t {
    None,
    Front,
    Back
};

/**
 * @brief Depth test comparison functions
 */
enum class CompareOp : uint8_t {
    Never,
    Less,
    Equal,
    LessEqual,
    Greater,
    NotEqual,
    GreaterEqual,
    Always
};

/**
 * @brief Texture filtering modes
 */
enum class Filter : uint8_t {
    Nearest,
    Linear,
    Bilinear,
    Trilinear,
    Anisotropic
};

/**
 * @brief Texture addressing modes
 */
enum class AddressMode : uint8_t {
    Repeat,
    MirroredRepeat,
    ClampToEdge,
    ClampToBorder
};

// =============================================================================
// RESOURCE HANDLES & DESCRIPTORS
// =============================================================================

/**
 * @brief Type-safe resource handle template
 */
template<typename Tag>
class ResourceHandle {
public:
    explicit ResourceHandle(uint64_t id = 0) : id_(id) {}
    
    uint64_t id() const { return id_; }
    bool is_valid() const { return id_ != 0; }
    
    bool operator==(const ResourceHandle& other) const { return id_ == other.id_; }
    bool operator!=(const ResourceHandle& other) const { return !(*this == other); }
    
private:
    uint64_t id_;
};

// Resource handle types
struct BufferTag {};
struct TextureTag {};
struct ShaderTag {};
struct RenderPassTag {};
struct PipelineTag {};
struct DescriptorSetTag {};

using BufferHandle = ResourceHandle<BufferTag>;
using TextureHandle = ResourceHandle<TextureTag>;
using ShaderHandle = ResourceHandle<ShaderTag>;
using RenderPassHandle = ResourceHandle<RenderPassTag>;
using PipelineHandle = ResourceHandle<PipelineTag>;
using DescriptorSetHandle = ResourceHandle<DescriptorSetTag>;

/**
 * @brief Buffer creation descriptor
 */
struct BufferDesc {
    size_t size = 0;
    BufferUsage usage = BufferUsage::Static;
    bool gpu_only = false;  ///< Buffer not accessible from CPU
    std::string debug_name;
};

/**
 * @brief Texture creation descriptor
 */
struct TextureDesc {
    uint32_t width = 0;
    uint32_t height = 0;
    uint32_t depth = 1;       ///< For 3D textures
    uint32_t mip_levels = 1;
    uint32_t array_layers = 1; ///< For texture arrays
    TextureFormat format = TextureFormat::RGBA8;
    uint32_t samples = 1;      ///< For MSAA
    bool render_target = false;
    bool depth_stencil = false;
    std::string debug_name;
};

/**
 * @brief Sampler state descriptor
 */
struct SamplerDesc {
    Filter min_filter = Filter::Linear;
    Filter mag_filter = Filter::Linear;
    Filter mip_filter = Filter::Linear;
    AddressMode address_u = AddressMode::Repeat;
    AddressMode address_v = AddressMode::Repeat;
    AddressMode address_w = AddressMode::Repeat;
    float max_anisotropy = 16.0f;
    CompareOp compare_op = CompareOp::Never;
    float min_lod = 0.0f;
    float max_lod = 1000.0f;
    std::array<float, 4> border_color = {0.0f, 0.0f, 0.0f, 0.0f};
};

/**
 * @brief Vertex attribute descriptor
 */
struct VertexAttribute {
    uint32_t location = 0;      ///< Shader attribute location
    uint32_t binding = 0;       ///< Vertex buffer binding point
    TextureFormat format = TextureFormat::RGB32F;
    uint32_t offset = 0;        ///< Byte offset within vertex
};

/**
 * @brief Vertex input layout descriptor
 */
struct VertexLayout {
    std::vector<VertexAttribute> attributes;
    uint32_t stride = 0;        ///< Vertex size in bytes
};

/**
 * @brief Render state configuration
 */
struct RenderState {
    // Depth/stencil state
    bool depth_test_enable = true;
    bool depth_write_enable = true;
    CompareOp depth_compare_op = CompareOp::Less;
    
    // Blend state
    BlendMode blend_mode = BlendMode::None;
    
    // Rasterizer state
    CullMode cull_mode = CullMode::Back;
    bool wireframe = false;
    
    // Multisample state
    uint32_t samples = 1;
    bool alpha_to_coverage = false;
};

/**
 * @brief Viewport configuration
 */
struct Viewport {
    float x = 0.0f;
    float y = 0.0f;
    float width = 0.0f;
    float height = 0.0f;
    float min_depth = 0.0f;
    float max_depth = 1.0f;
};

/**
 * @brief Scissor rectangle
 */
struct ScissorRect {
    int32_t x = 0;
    int32_t y = 0;
    uint32_t width = 0;
    uint32_t height = 0;
};

// =============================================================================
// RENDER COMMAND STRUCTURES
// =============================================================================

/**
 * @brief Draw command for indexed geometry
 */
struct DrawIndexedCommand {
    uint32_t index_count = 0;
    uint32_t instance_count = 1;
    uint32_t first_index = 0;
    int32_t vertex_offset = 0;
    uint32_t first_instance = 0;
};

/**
 * @brief Draw command for non-indexed geometry
 */
struct DrawCommand {
    uint32_t vertex_count = 0;
    uint32_t instance_count = 1;
    uint32_t first_vertex = 0;
    uint32_t first_instance = 0;
};

/**
 * @brief Compute dispatch command
 */
struct DispatchCommand {
    uint32_t group_count_x = 1;
    uint32_t group_count_y = 1;
    uint32_t group_count_z = 1;
};

// =============================================================================
// ABSTRACT RENDERER INTERFACE
// =============================================================================

/**
 * @brief Main rendering interface - API agnostic
 * 
 * This class provides a unified interface for different rendering APIs
 * (Vulkan, OpenGL) while maintaining high performance and modern features.
 */
class IRenderer {
public:
    virtual ~IRenderer() = default;

    // =============================================================================
    // INITIALIZATION & LIFECYCLE
    // =============================================================================

    /**
     * @brief Initialize the renderer with specified API
     */
    virtual bool initialize(RenderingAPI api = RenderingAPI::Auto) = 0;

    /**
     * @brief Shutdown and cleanup all resources
     */
    virtual void shutdown() = 0;

    /**
     * @brief Get the active rendering API
     */
    virtual RenderingAPI get_api() const = 0;

    /**
     * @brief Get renderer capabilities and limits
     */
    struct RendererCaps {
        uint32_t max_texture_size = 0;
        uint32_t max_3d_texture_size = 0;
        uint32_t max_array_texture_layers = 0;
        uint32_t max_msaa_samples = 0;
        uint32_t max_anisotropy = 0;
        bool supports_compute_shaders = false;
        bool supports_tessellation = false;
        bool supports_geometry_shaders = false;
        bool supports_bindless_resources = false;
        bool supports_ray_tracing = false;
    };

    virtual RendererCaps get_capabilities() const = 0;

    // =============================================================================
    // RESOURCE MANAGEMENT
    // =============================================================================

    /**
     * @brief Create a vertex/index/uniform buffer
     */
    virtual BufferHandle create_buffer(const BufferDesc& desc, const void* initial_data = nullptr) = 0;

    /**
     * @brief Create a texture resource
     */
    virtual TextureHandle create_texture(const TextureDesc& desc, const void* initial_data = nullptr) = 0;

    /**
     * @brief Create a shader from source code
     */
    virtual ShaderHandle create_shader(const std::string& vertex_source, 
                                     const std::string& fragment_source,
                                     const std::string& debug_name = "") = 0;

    /**
     * @brief Create a compute shader
     */
    virtual ShaderHandle create_compute_shader(const std::string& compute_source,
                                             const std::string& debug_name = "") = 0;

    /**
     * @brief Destroy a buffer resource
     */
    virtual void destroy_buffer(BufferHandle handle) = 0;

    /**
     * @brief Destroy a texture resource
     */
    virtual void destroy_texture(TextureHandle handle) = 0;

    /**
     * @brief Destroy a shader resource
     */
    virtual void destroy_shader(ShaderHandle handle) = 0;

    // =============================================================================
    // RESOURCE UPDATES
    // =============================================================================

    /**
     * @brief Update buffer data (for dynamic/streaming buffers)
     */
    virtual void update_buffer(BufferHandle handle, size_t offset, size_t size, const void* data) = 0;

    /**
     * @brief Update texture data
     */
    virtual void update_texture(TextureHandle handle, uint32_t mip_level, uint32_t array_layer,
                               uint32_t x, uint32_t y, uint32_t z,
                               uint32_t width, uint32_t height, uint32_t depth,
                               const void* data) = 0;

    /**
     * @brief Generate mipmaps for a texture
     */
    virtual void generate_mipmaps(TextureHandle handle) = 0;

    // =============================================================================
    // FRAME MANAGEMENT
    // =============================================================================

    /**
     * @brief Begin a new frame
     */
    virtual void begin_frame() = 0;

    /**
     * @brief End the current frame and present
     */
    virtual void end_frame() = 0;

    /**
     * @brief Set the main render target (backbuffer)
     */
    virtual void set_render_target(TextureHandle color_target = TextureHandle{},
                                 TextureHandle depth_target = TextureHandle{}) = 0;

    /**
     * @brief Clear the current render target
     */
    virtual void clear(const std::array<float, 4>& color = {0.0f, 0.0f, 0.0f, 1.0f},
                      float depth = 1.0f, uint8_t stencil = 0) = 0;

    /**
     * @brief Set viewport and scissor rectangle
     */
    virtual void set_viewport(const Viewport& viewport) = 0;
    virtual void set_scissor(const ScissorRect& scissor) = 0;

    // =============================================================================
    // PIPELINE STATE MANAGEMENT
    // =============================================================================

    /**
     * @brief Set the active shader pipeline
     */
    virtual void set_shader(ShaderHandle handle) = 0;

    /**
     * @brief Set render state (blending, depth test, etc.)
     */
    virtual void set_render_state(const RenderState& state) = 0;

    /**
     * @brief Bind vertex buffers for rendering
     */
    virtual void set_vertex_buffers(std::span<const BufferHandle> buffers,
                                   std::span<const uint64_t> offsets = {}) = 0;

    /**
     * @brief Bind index buffer for indexed rendering
     */
    virtual void set_index_buffer(BufferHandle buffer, size_t offset = 0, bool use_32bit_indices = true) = 0;

    /**
     * @brief Set vertex input layout
     */
    virtual void set_vertex_layout(const VertexLayout& layout) = 0;

    // =============================================================================
    // RESOURCE BINDING
    // =============================================================================

    /**
     * @brief Bind textures to shader slots
     */
    virtual void bind_texture(uint32_t slot, TextureHandle texture) = 0;
    virtual void bind_textures(uint32_t first_slot, std::span<const TextureHandle> textures) = 0;

    /**
     * @brief Bind uniform/constant buffers
     */
    virtual void bind_uniform_buffer(uint32_t slot, BufferHandle buffer, 
                                   size_t offset = 0, size_t size = 0) = 0;

    /**
     * @brief Bind storage buffers (for compute shaders)
     */
    virtual void bind_storage_buffer(uint32_t slot, BufferHandle buffer,
                                   size_t offset = 0, size_t size = 0) = 0;

    /**
     * @brief Set push constants (small uniform data)
     */
    virtual void set_push_constants(uint32_t offset, uint32_t size, const void* data) = 0;

    // =============================================================================
    // DRAW COMMANDS
    // =============================================================================

    /**
     * @brief Draw indexed geometry
     */
    virtual void draw_indexed(const DrawIndexedCommand& cmd) = 0;

    /**
     * @brief Draw non-indexed geometry
     */
    virtual void draw(const DrawCommand& cmd) = 0;

    /**
     * @brief Dispatch compute shader
     */
    virtual void dispatch(const DispatchCommand& cmd) = 0;

    // =============================================================================
    // DEBUGGING & PROFILING
    // =============================================================================

    /**
     * @brief Begin a debug/profiling marker
     */
    virtual void push_debug_marker(const std::string& name) = 0;

    /**
     * @brief End the current debug marker
     */
    virtual void pop_debug_marker() = 0;

    /**
     * @brief Insert a debug marker at current point
     */
    virtual void insert_debug_marker(const std::string& name) = 0;

    /**
     * @brief Get frame timing statistics
     */
    struct FrameStats {
        float frame_time_ms = 0.0f;
        float gpu_time_ms = 0.0f;
        uint32_t draw_calls = 0;
        uint32_t vertices_rendered = 0;
        uint64_t memory_used = 0;
    };

    virtual FrameStats get_frame_stats() const = 0;

    // =============================================================================
    // ADVANCED FEATURES
    // =============================================================================

    /**
     * @brief Wait for all GPU operations to complete
     */
    virtual void wait_idle() = 0;

    /**
     * @brief Create a fence for GPU synchronization
     */
    virtual uint64_t create_fence() = 0;

    /**
     * @brief Wait for a specific fence
     */
    virtual void wait_for_fence(uint64_t fence_id, uint64_t timeout_ns = UINT64_MAX) = 0;

    /**
     * @brief Check if fence is signaled
     */
    virtual bool is_fence_signaled(uint64_t fence_id) = 0;
};

// =============================================================================
// RENDERER FACTORY
// =============================================================================

/**
 * @brief Factory function to create renderer instances
 */
class RendererFactory {
public:
    /**
     * @brief Create a renderer instance
     * @param api Preferred rendering API
     * @param window Platform window handle (can be nullptr for headless)
     * @return Unique pointer to renderer or nullptr on failure
     */
    static std::unique_ptr<IRenderer> create(RenderingAPI api = RenderingAPI::Auto,
                                           GLFWwindow* window = nullptr);

    /**
     * @brief Check if a specific API is available
     */
    static bool is_api_available(RenderingAPI api);

    /**
     * @brief Get the best available API for current system
     */
    static RenderingAPI get_best_api();

    /**
     * @brief Get API name as string
     */
    static std::string api_to_string(RenderingAPI api);
};

// =============================================================================
// UTILITY CLASSES
// =============================================================================

/**
 * @brief RAII debug marker helper
 */
class ScopedDebugMarker {
public:
    ScopedDebugMarker(IRenderer* renderer, const std::string& name)
        : renderer_(renderer) {
        if (renderer_) {
            renderer_->push_debug_marker(name);
        }
    }

    ~ScopedDebugMarker() {
        if (renderer_) {
            renderer_->pop_debug_marker();
        }
    }

private:
    IRenderer* renderer_;
};

/**
 * @brief Utility macro for scoped debug markers
 */
#define SCOPED_DEBUG_MARKER(renderer, name) \
    ::ecscope::rendering::ScopedDebugMarker _debug_marker((renderer), (name))

/**
 * @brief Resource pool for efficient resource management
 */
template<typename Handle, typename Desc>
class ResourcePool {
public:
    Handle acquire(IRenderer* renderer, const Desc& desc);
    void release(Handle handle);
    void clear(IRenderer* renderer);

private:
    std::vector<Handle> available_resources_;
    std::unordered_map<uint64_t, Desc> resource_descriptors_;
};

} // namespace ecscope::rendering

// =============================================================================
// HASH FUNCTIONS FOR UNORDERED CONTAINERS
// =============================================================================

namespace std {

template<typename Tag>
struct hash<ecscope::rendering::ResourceHandle<Tag>> {
    size_t operator()(const ecscope::rendering::ResourceHandle<Tag>& handle) const {
        return hash<uint64_t>{}(handle.id());
    }
};

} // namespace std