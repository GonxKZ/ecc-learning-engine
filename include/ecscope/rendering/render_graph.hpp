/**
 * @file render_graph.hpp
 * @brief Modern Render Graph System
 * 
 * Automatic resource management and render pass scheduling system
 * for optimal GPU performance and memory usage.
 * 
 * @author ECScope Development Team
 * @version 1.0.0
 */

#pragma once

#include "renderer.hpp"
#include <memory>
#include <vector>
#include <string>
#include <functional>
#include <unordered_map>
#include <unordered_set>

namespace ecscope::rendering {

// =============================================================================
// RENDER GRAPH DECLARATIONS
// =============================================================================

/**
 * @brief Resource types in the render graph
 */
enum class ResourceType : uint8_t {
    Buffer,
    Texture,
    RenderPass
};

/**
 * @brief Resource access patterns
 */
enum class ResourceAccess : uint8_t {
    Read,
    Write,
    ReadWrite
};

/**
 * @brief Resource lifetime management
 */
enum class ResourceLifetime : uint8_t {
    Transient,    ///< Temporary resource for this frame
    Persistent,   ///< Resource persists across frames
    Imported,     ///< Externally managed resource
    Exported      ///< Resource exported from the graph
};

/**
 * @brief Render graph resource descriptor
 */
struct RenderGraphResource {
    std::string name;
    ResourceType type = ResourceType::Texture;
    ResourceLifetime lifetime = ResourceLifetime::Transient;
    
    union {
        TextureDesc texture_desc;
        BufferDesc buffer_desc;
    };
    
    RenderGraphResource() { texture_desc = TextureDesc{}; }
};

/**
 * @brief Resource usage in a render pass
 */
struct ResourceUsage {
    std::string resource_name;
    ResourceAccess access = ResourceAccess::Read;
    uint32_t subresource_index = 0; ///< For array textures or mip levels
};

/**
 * @brief Forward declaration
 */
class RenderGraph;

/**
 * @brief Render pass execution context
 */
class RenderPassContext {
public:
    RenderPassContext(RenderGraph* graph, IRenderer* renderer)
        : graph_(graph), renderer_(renderer) {}
    
    /**
     * @brief Get a resource handle by name
     */
    TextureHandle get_texture(const std::string& name);
    BufferHandle get_buffer(const std::string& name);
    
    /**
     * @brief Get the underlying renderer
     */
    IRenderer* get_renderer() const { return renderer_; }
    
    /**
     * @brief Set debug marker
     */
    void set_debug_marker(const std::string& name);
    
private:
    RenderGraph* graph_;
    IRenderer* renderer_;
};

/**
 * @brief Render pass function type
 */
using RenderPassExecuteFunc = std::function<void(RenderPassContext&)>;

/**
 * @brief Render pass descriptor
 */
struct RenderPass {
    std::string name;
    std::vector<ResourceUsage> inputs;
    std::vector<ResourceUsage> outputs;
    RenderPassExecuteFunc execute_func;
    
    // Scheduling hints
    bool can_execute_async = false;
    uint32_t priority = 0;
};

/**
 * @brief Modern render graph for automatic resource management
 */
class RenderGraph {
public:
    explicit RenderGraph(IRenderer* renderer);
    ~RenderGraph();

    // Disable copy/move
    RenderGraph(const RenderGraph&) = delete;
    RenderGraph& operator=(const RenderGraph&) = delete;
    RenderGraph(RenderGraph&&) = delete;
    RenderGraph& operator=(RenderGraph&&) = delete;

    // =============================================================================
    // RESOURCE MANAGEMENT
    // =============================================================================

    /**
     * @brief Create a transient texture resource
     */
    void create_texture(const std::string& name, const TextureDesc& desc);

    /**
     * @brief Create a transient buffer resource
     */
    void create_buffer(const std::string& name, const BufferDesc& desc);

    /**
     * @brief Import an external texture
     */
    void import_texture(const std::string& name, TextureHandle handle);

    /**
     * @brief Import an external buffer
     */
    void import_buffer(const std::string& name, BufferHandle handle);

    /**
     * @brief Mark a resource for export
     */
    void export_texture(const std::string& name);
    void export_buffer(const std::string& name);

    /**
     * @brief Get exported resource handles
     */
    TextureHandle get_exported_texture(const std::string& name) const;
    BufferHandle get_exported_buffer(const std::string& name) const;

    // =============================================================================
    // PASS MANAGEMENT
    // =============================================================================

    /**
     * @brief Add a render pass to the graph
     */
    void add_pass(const std::string& name,
                  const std::vector<ResourceUsage>& inputs,
                  const std::vector<ResourceUsage>& outputs,
                  RenderPassExecuteFunc execute_func);

    /**
     * @brief Add a compute pass to the graph
     */
    void add_compute_pass(const std::string& name,
                          const std::vector<ResourceUsage>& inputs,
                          const std::vector<ResourceUsage>& outputs,
                          RenderPassExecuteFunc execute_func);

    // =============================================================================
    // GRAPH COMPILATION & EXECUTION
    // =============================================================================

    /**
     * @brief Compile the render graph
     * This performs dependency analysis, resource allocation, and optimization
     */
    bool compile();

    /**
     * @brief Execute the compiled render graph
     */
    void execute();

    /**
     * @brief Clear the graph (for next frame)
     */
    void clear();

    // =============================================================================
    // OPTIMIZATION & DEBUGGING
    // =============================================================================

    /**
     * @brief Enable/disable automatic resource aliasing
     */
    void set_resource_aliasing(bool enable) { enable_aliasing_ = enable; }

    /**
     * @brief Enable/disable async execution
     */
    void set_async_execution(bool enable) { enable_async_ = enable; }

    /**
     * @brief Get graph statistics
     */
    struct GraphStats {
        uint32_t total_passes = 0;
        uint32_t culled_passes = 0;
        uint32_t total_resources = 0;
        uint32_t aliased_resources = 0;
        uint64_t memory_used = 0;
        uint64_t memory_saved = 0;
        float compile_time_ms = 0.0f;
        float execute_time_ms = 0.0f;
    };

    GraphStats get_statistics() const { return stats_; }

    /**
     * @brief Export graph to DOT format for visualization
     */
    std::string export_dot() const;

    /**
     * @brief Validate graph for correctness
     */
    bool validate() const;

    // Internal access for RenderPassContext
    TextureHandle get_texture_handle(const std::string& name);
    BufferHandle get_buffer_handle(const std::string& name);

private:
    // =============================================================================
    // INTERNAL STRUCTURES
    // =============================================================================

    struct CompiledResource {
        RenderGraphResource desc;
        union {
            TextureHandle texture;
            BufferHandle buffer;
        };
        uint32_t first_use_pass = UINT32_MAX;
        uint32_t last_use_pass = UINT32_MAX;
        bool is_aliased = false;
        CompiledResource* alias_target = nullptr;
        
        CompiledResource() { texture = TextureHandle{}; }
    };

    struct CompiledPass {
        RenderPass desc;
        std::vector<uint32_t> input_resources;
        std::vector<uint32_t> output_resources;
        std::vector<uint32_t> dependencies;
        uint32_t execution_order = UINT32_MAX;
        bool is_culled = false;
    };

    // =============================================================================
    // COMPILATION STAGES
    // =============================================================================

    bool validate_graph() const;
    void build_dependency_graph();
    void cull_unused_passes();
    void schedule_passes();
    void allocate_resources();
    void optimize_memory();

    // =============================================================================
    // RESOURCE ALLOCATION
    // =============================================================================

    void allocate_transient_resources();
    void setup_resource_aliasing();
    bool can_alias_resources(const CompiledResource& a, const CompiledResource& b) const;

    // =============================================================================
    // MEMBER VARIABLES
    // =============================================================================

    IRenderer* renderer_;
    bool compiled_ = false;
    bool enable_aliasing_ = true;
    bool enable_async_ = false;

    // Graph definition
    std::vector<RenderGraphResource> resources_;
    std::vector<RenderPass> passes_;
    std::unordered_map<std::string, uint32_t> resource_indices_;
    std::unordered_map<std::string, uint32_t> pass_indices_;

    // Compiled graph
    std::vector<CompiledResource> compiled_resources_;
    std::vector<CompiledPass> compiled_passes_;
    std::vector<uint32_t> execution_order_;

    // Imported/exported resources
    std::unordered_map<std::string, TextureHandle> imported_textures_;
    std::unordered_map<std::string, BufferHandle> imported_buffers_;
    std::unordered_set<std::string> exported_resources_;

    // Statistics
    mutable GraphStats stats_;
};

// =============================================================================
// RENDER GRAPH BUILDER UTILITY
// =============================================================================

/**
 * @brief Fluent interface for building render graphs
 */
class RenderGraphBuilder {
public:
    explicit RenderGraphBuilder(RenderGraph* graph) : graph_(graph) {}

    RenderGraphBuilder& texture(const std::string& name, const TextureDesc& desc) {
        graph_->create_texture(name, desc);
        return *this;
    }

    RenderGraphBuilder& buffer(const std::string& name, const BufferDesc& desc) {
        graph_->create_buffer(name, desc);
        return *this;
    }

    RenderGraphBuilder& import_texture(const std::string& name, TextureHandle handle) {
        graph_->import_texture(name, handle);
        return *this;
    }

    RenderGraphBuilder& pass(const std::string& name,
                           const std::vector<ResourceUsage>& inputs,
                           const std::vector<ResourceUsage>& outputs,
                           RenderPassExecuteFunc func) {
        graph_->add_pass(name, inputs, outputs, func);
        return *this;
    }

    bool compile() {
        return graph_->compile();
    }

private:
    RenderGraph* graph_;
};

} // namespace ecscope::rendering