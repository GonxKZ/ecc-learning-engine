/**
 * @file render_graph.cpp
 * @brief Render Graph Implementation
 * 
 * Modern render graph system for automatic resource management
 * and optimal render pass scheduling.
 * 
 * @author ECScope Development Team
 * @version 1.0.0
 */

#include "ecscope/rendering/render_graph.hpp"
#include <algorithm>
#include <unordered_set>
#include <queue>
#include <chrono>
#include <sstream>
#include <iostream>

namespace ecscope::rendering {

// =============================================================================
// RENDER PASS CONTEXT IMPLEMENTATION
// =============================================================================

TextureHandle RenderPassContext::get_texture(const std::string& name) {
    return graph_->get_texture_handle(name);
}

BufferHandle RenderPassContext::get_buffer(const std::string& name) {
    return graph_->get_buffer_handle(name);
}

void RenderPassContext::set_debug_marker(const std::string& name) {
    renderer_->push_debug_marker(name);
}

// =============================================================================
// RENDER GRAPH IMPLEMENTATION
// =============================================================================

RenderGraph::RenderGraph(IRenderer* renderer)
    : renderer_(renderer)
    , compiled_(false)
    , enable_aliasing_(true)
    , enable_async_(false) {
    
    if (!renderer_) {
        throw std::invalid_argument("RenderGraph requires a valid IRenderer instance");
    }
}

RenderGraph::~RenderGraph() {
    clear();
}

void RenderGraph::create_texture(const std::string& name, const TextureDesc& desc) {
    if (resource_indices_.find(name) != resource_indices_.end()) {
        throw std::invalid_argument("Resource with name '" + name + "' already exists");
    }
    
    RenderGraphResource resource;
    resource.name = name;
    resource.type = ResourceType::Texture;
    resource.lifetime = ResourceLifetime::Transient;
    resource.texture_desc = desc;
    
    uint32_t index = static_cast<uint32_t>(resources_.size());
    resources_.push_back(resource);
    resource_indices_[name] = index;
    
    compiled_ = false; // Need to recompile
}

void RenderGraph::create_buffer(const std::string& name, const BufferDesc& desc) {
    if (resource_indices_.find(name) != resource_indices_.end()) {
        throw std::invalid_argument("Resource with name '" + name + "' already exists");
    }
    
    RenderGraphResource resource;
    resource.name = name;
    resource.type = ResourceType::Buffer;
    resource.lifetime = ResourceLifetime::Transient;
    resource.buffer_desc = desc;
    
    uint32_t index = static_cast<uint32_t>(resources_.size());
    resources_.push_back(resource);
    resource_indices_[name] = index;
    
    compiled_ = false;
}

void RenderGraph::import_texture(const std::string& name, TextureHandle handle) {
    imported_textures_[name] = handle;
    
    // Create resource entry for dependency tracking
    RenderGraphResource resource;
    resource.name = name;
    resource.type = ResourceType::Texture;
    resource.lifetime = ResourceLifetime::Imported;
    // Texture desc would need to be queried from renderer
    
    uint32_t index = static_cast<uint32_t>(resources_.size());
    resources_.push_back(resource);
    resource_indices_[name] = index;
    
    compiled_ = false;
}

void RenderGraph::import_buffer(const std::string& name, BufferHandle handle) {
    imported_buffers_[name] = handle;
    
    RenderGraphResource resource;
    resource.name = name;
    resource.type = ResourceType::Buffer;
    resource.lifetime = ResourceLifetime::Imported;
    
    uint32_t index = static_cast<uint32_t>(resources_.size());
    resources_.push_back(resource);
    resource_indices_[name] = index;
    
    compiled_ = false;
}

void RenderGraph::export_texture(const std::string& name) {
    exported_resources_.insert(name);
    
    auto it = resource_indices_.find(name);
    if (it != resource_indices_.end()) {
        resources_[it->second].lifetime = ResourceLifetime::Exported;
    }
}

void RenderGraph::export_buffer(const std::string& name) {
    exported_resources_.insert(name);
    
    auto it = resource_indices_.find(name);
    if (it != resource_indices_.end()) {
        resources_[it->second].lifetime = ResourceLifetime::Exported;
    }
}

TextureHandle RenderGraph::get_exported_texture(const std::string& name) const {
    if (!compiled_) {
        return TextureHandle{};
    }
    
    auto it = resource_indices_.find(name);
    if (it != resource_indices_.end() && it->second < compiled_resources_.size()) {
        const auto& resource = compiled_resources_[it->second];
        if (resource.desc.type == ResourceType::Texture) {
            return resource.texture;
        }
    }
    
    return TextureHandle{};
}

BufferHandle RenderGraph::get_exported_buffer(const std::string& name) const {
    if (!compiled_) {
        return BufferHandle{};
    }
    
    auto it = resource_indices_.find(name);
    if (it != resource_indices_.end() && it->second < compiled_resources_.size()) {
        const auto& resource = compiled_resources_[it->second];
        if (resource.desc.type == ResourceType::Buffer) {
            return resource.buffer;
        }
    }
    
    return BufferHandle{};
}

void RenderGraph::add_pass(const std::string& name,
                          const std::vector<ResourceUsage>& inputs,
                          const std::vector<ResourceUsage>& outputs,
                          RenderPassExecuteFunc execute_func) {
    
    if (pass_indices_.find(name) != pass_indices_.end()) {
        throw std::invalid_argument("Pass with name '" + name + "' already exists");
    }
    
    RenderPass pass;
    pass.name = name;
    pass.inputs = inputs;
    pass.outputs = outputs;
    pass.execute_func = execute_func;
    pass.can_execute_async = false;
    pass.priority = 0;
    
    uint32_t index = static_cast<uint32_t>(passes_.size());
    passes_.push_back(pass);
    pass_indices_[name] = index;
    
    compiled_ = false;
}

void RenderGraph::add_compute_pass(const std::string& name,
                                  const std::vector<ResourceUsage>& inputs,
                                  const std::vector<ResourceUsage>& outputs,
                                  RenderPassExecuteFunc execute_func) {
    
    RenderPass pass;
    pass.name = name;
    pass.inputs = inputs;
    pass.outputs = outputs;
    pass.execute_func = execute_func;
    pass.can_execute_async = true; // Compute passes can potentially run async
    pass.priority = 0;
    
    uint32_t index = static_cast<uint32_t>(passes_.size());
    passes_.push_back(pass);
    pass_indices_[name] = index;
    
    compiled_ = false;
}

bool RenderGraph::compile() {
    auto start_time = std::chrono::high_resolution_clock::now();
    
    try {
        // Clear previous compilation
        compiled_resources_.clear();
        compiled_passes_.clear();
        execution_order_.clear();
        
        stats_ = GraphStats{};
        stats_.total_passes = static_cast<uint32_t>(passes_.size());
        stats_.total_resources = static_cast<uint32_t>(resources_.size());
        
        // Validation
        if (!validate_graph()) {
            std::cerr << "Graph validation failed" << std::endl;
            return false;
        }
        
        // Build dependency graph
        build_dependency_graph();
        
        // Cull unused passes
        cull_unused_passes();
        
        // Schedule passes in dependency order
        schedule_passes();
        
        // Allocate resources
        allocate_resources();
        
        // Memory optimization
        optimize_memory();
        
        compiled_ = true;
        
        auto end_time = std::chrono::high_resolution_clock::now();
        stats_.compile_time_ms = std::chrono::duration<float, std::milli>(end_time - start_time).count();
        
        std::cout << "Render graph compiled successfully in " << stats_.compile_time_ms << "ms" << std::endl;
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "Failed to compile render graph: " << e.what() << std::endl;
        return false;
    }
}

void RenderGraph::execute() {
    if (!compiled_) {
        std::cerr << "Cannot execute uncompiled render graph" << std::endl;
        return;
    }
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    RenderPassContext context(this, renderer_);
    
    // Execute passes in dependency order
    for (uint32_t pass_index : execution_order_) {
        const auto& pass = compiled_passes_[pass_index];
        
        if (pass.is_culled) {
            continue;
        }
        
        // Set debug marker
        renderer_->push_debug_marker("RenderPass: " + pass.desc.name);
        
        try {
            // Execute the pass
            if (pass.desc.execute_func) {
                pass.desc.execute_func(context);
            }
        } catch (const std::exception& e) {
            std::cerr << "Error executing pass '" << pass.desc.name << "': " << e.what() << std::endl;
        }
        
        renderer_->pop_debug_marker();
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    stats_.execute_time_ms = std::chrono::duration<float, std::milli>(end_time - start_time).count();
}

void RenderGraph::clear() {
    // Destroy allocated resources
    for (const auto& resource : compiled_resources_) {
        if (resource.desc.lifetime == ResourceLifetime::Transient) {
            if (resource.desc.type == ResourceType::Texture && resource.texture.is_valid()) {
                renderer_->destroy_texture(resource.texture);
            } else if (resource.desc.type == ResourceType::Buffer && resource.buffer.is_valid()) {
                renderer_->destroy_buffer(resource.buffer);
            }
        }
    }
    
    // Clear all data
    resources_.clear();
    passes_.clear();
    resource_indices_.clear();
    pass_indices_.clear();
    compiled_resources_.clear();
    compiled_passes_.clear();
    execution_order_.clear();
    imported_textures_.clear();
    imported_buffers_.clear();
    exported_resources_.clear();
    
    compiled_ = false;
    stats_ = GraphStats{};
}

TextureHandle RenderGraph::get_texture_handle(const std::string& name) {
    auto it = resource_indices_.find(name);
    if (it != resource_indices_.end() && it->second < compiled_resources_.size()) {
        const auto& resource = compiled_resources_[it->second];
        if (resource.desc.type == ResourceType::Texture) {
            return resource.texture;
        }
    }
    
    // Check imported textures
    auto imported_it = imported_textures_.find(name);
    if (imported_it != imported_textures_.end()) {
        return imported_it->second;
    }
    
    return TextureHandle{};
}

BufferHandle RenderGraph::get_buffer_handle(const std::string& name) {
    auto it = resource_indices_.find(name);
    if (it != resource_indices_.end() && it->second < compiled_resources_.size()) {
        const auto& resource = compiled_resources_[it->second];
        if (resource.desc.type == ResourceType::Buffer) {
            return resource.buffer;
        }
    }
    
    // Check imported buffers
    auto imported_it = imported_buffers_.find(name);
    if (imported_it != imported_buffers_.end()) {
        return imported_it->second;
    }
    
    return BufferHandle{};
}

std::string RenderGraph::export_dot() const {
    std::stringstream ss;
    ss << "digraph RenderGraph {\\n";
    ss << "  rankdir=TB;\\n";
    ss << "  node [shape=box];\\n\\n";
    
    // Add resource nodes
    for (const auto& resource : resources_) {
        ss << "  \"" << resource.name << "\" [style=filled, fillcolor=lightblue];\\n";
    }
    
    // Add pass nodes and edges
    for (const auto& pass : passes_) {
        ss << "  \"" << pass.name << "\" [style=filled, fillcolor=lightgreen];\\n";
        
        // Input edges
        for (const auto& input : pass.inputs) {
            ss << "  \"" << input.resource_name << "\" -> \"" << pass.name << "\";\\n";
        }
        
        // Output edges
        for (const auto& output : pass.outputs) {
            ss << "  \"" << pass.name << "\" -> \"" << output.resource_name << "\";\\n";
        }
    }
    
    ss << "}\\n";
    return ss.str();
}

bool RenderGraph::validate() const {
    return validate_graph();
}

// =============================================================================
// PRIVATE IMPLEMENTATION
// =============================================================================

bool RenderGraph::validate_graph() const {
    // Check that all referenced resources exist
    for (const auto& pass : passes_) {
        for (const auto& input : pass.inputs) {
            if (resource_indices_.find(input.resource_name) == resource_indices_.end()) {
                std::cerr << "Pass '" << pass.name << "' references undefined resource '" 
                         << input.resource_name << "'" << std::endl;
                return false;
            }
        }
        
        for (const auto& output : pass.outputs) {
            if (resource_indices_.find(output.resource_name) == resource_indices_.end()) {
                std::cerr << "Pass '" << pass.name << "' references undefined resource '" 
                         << output.resource_name << "'" << std::endl;
                return false;
            }
        }
    }
    
    return true;
}

void RenderGraph::build_dependency_graph() {
    compiled_resources_.resize(resources_.size());
    compiled_passes_.resize(passes_.size());
    
    // Initialize compiled resources
    for (size_t i = 0; i < resources_.size(); ++i) {
        compiled_resources_[i].desc = resources_[i];
        compiled_resources_[i].first_use_pass = UINT32_MAX;
        compiled_resources_[i].last_use_pass = UINT32_MAX;
        compiled_resources_[i].is_aliased = false;
        compiled_resources_[i].alias_target = nullptr;
    }
    
    // Initialize compiled passes and track resource usage
    for (size_t i = 0; i < passes_.size(); ++i) {
        compiled_passes_[i].desc = passes_[i];
        compiled_passes_[i].execution_order = UINT32_MAX;
        compiled_passes_[i].is_culled = false;
        
        // Track input resources
        for (const auto& input : passes_[i].inputs) {
            auto res_it = resource_indices_.find(input.resource_name);
            if (res_it != resource_indices_.end()) {
                uint32_t res_index = res_it->second;
                compiled_passes_[i].input_resources.push_back(res_index);
                
                // Update resource usage
                auto& resource = compiled_resources_[res_index];
                resource.first_use_pass = std::min(resource.first_use_pass, static_cast<uint32_t>(i));
                resource.last_use_pass = std::max(resource.last_use_pass, static_cast<uint32_t>(i));
            }
        }
        
        // Track output resources
        for (const auto& output : passes_[i].outputs) {
            auto res_it = resource_indices_.find(output.resource_name);
            if (res_it != resource_indices_.end()) {
                uint32_t res_index = res_it->second;
                compiled_passes_[i].output_resources.push_back(res_index);
                
                // Update resource usage
                auto& resource = compiled_resources_[res_index];
                resource.first_use_pass = std::min(resource.first_use_pass, static_cast<uint32_t>(i));
                resource.last_use_pass = std::max(resource.last_use_pass, static_cast<uint32_t>(i));
            }
        }
    }
    
    // Build pass dependencies
    for (size_t i = 0; i < compiled_passes_.size(); ++i) {
        auto& pass = compiled_passes_[i];
        
        // For each input resource, find the pass that writes to it
        for (uint32_t input_res : pass.input_resources) {
            for (size_t j = 0; j < compiled_passes_.size(); ++j) {
                if (i == j) continue;
                
                const auto& other_pass = compiled_passes_[j];
                if (std::find(other_pass.output_resources.begin(), 
                             other_pass.output_resources.end(), 
                             input_res) != other_pass.output_resources.end()) {
                    pass.dependencies.push_back(static_cast<uint32_t>(j));
                }
            }
        }
    }
}

void RenderGraph::cull_unused_passes() {
    // Mark passes that write to exported resources
    std::unordered_set<uint32_t> required_passes;
    
    for (size_t i = 0; i < compiled_passes_.size(); ++i) {
        const auto& pass = compiled_passes_[i];
        
        for (uint32_t output_res : pass.output_resources) {
            const auto& resource = compiled_resources_[output_res];
            if (exported_resources_.find(resource.desc.name) != exported_resources_.end()) {
                required_passes.insert(static_cast<uint32_t>(i));
                break;
            }
        }
    }
    
    // Recursively mark dependencies of required passes
    std::queue<uint32_t> to_process;
    for (uint32_t pass_index : required_passes) {
        to_process.push(pass_index);
    }
    
    while (!to_process.empty()) {
        uint32_t pass_index = to_process.front();
        to_process.pop();
        
        const auto& pass = compiled_passes_[pass_index];
        for (uint32_t dep : pass.dependencies) {
            if (required_passes.find(dep) == required_passes.end()) {
                required_passes.insert(dep);
                to_process.push(dep);
            }
        }
    }
    
    // Cull unrequired passes
    for (size_t i = 0; i < compiled_passes_.size(); ++i) {
        if (required_passes.find(static_cast<uint32_t>(i)) == required_passes.end()) {
            compiled_passes_[i].is_culled = true;
            stats_.culled_passes++;
        }
    }
}

void RenderGraph::schedule_passes() {
    // Simple topological sort for pass scheduling
    std::vector<uint32_t> in_degree(compiled_passes_.size(), 0);
    
    // Calculate in-degrees
    for (size_t i = 0; i < compiled_passes_.size(); ++i) {
        if (compiled_passes_[i].is_culled) continue;
        
        for (uint32_t dep : compiled_passes_[i].dependencies) {
            if (!compiled_passes_[dep].is_culled) {
                in_degree[i]++;
            }
        }
    }
    
    // Topological sort
    std::queue<uint32_t> ready;
    for (size_t i = 0; i < compiled_passes_.size(); ++i) {
        if (!compiled_passes_[i].is_culled && in_degree[i] == 0) {
            ready.push(static_cast<uint32_t>(i));
        }
    }
    
    uint32_t execution_index = 0;
    while (!ready.empty()) {
        uint32_t pass_index = ready.front();
        ready.pop();
        
        compiled_passes_[pass_index].execution_order = execution_index++;
        execution_order_.push_back(pass_index);
        
        // Update dependencies
        for (size_t i = 0; i < compiled_passes_.size(); ++i) {
            if (compiled_passes_[i].is_culled) continue;
            
            const auto& deps = compiled_passes_[i].dependencies;
            if (std::find(deps.begin(), deps.end(), pass_index) != deps.end()) {
                in_degree[i]--;
                if (in_degree[i] == 0) {
                    ready.push(static_cast<uint32_t>(i));
                }
            }
        }
    }
}

void RenderGraph::allocate_resources() {
    allocate_transient_resources();
    if (enable_aliasing_) {
        setup_resource_aliasing();
    }
}

void RenderGraph::optimize_memory() {
    // Calculate memory statistics
    stats_.memory_used = 0;
    stats_.memory_saved = 0;
    stats_.aliased_resources = 0;
    
    for (const auto& resource : compiled_resources_) {
        if (resource.desc.lifetime == ResourceLifetime::Transient) {
            size_t resource_size = 0;
            
            if (resource.desc.type == ResourceType::Texture) {
                const auto& desc = resource.desc.texture_desc;
                // Rough size calculation (this would be more accurate in real implementation)
                resource_size = desc.width * desc.height * 4; // Assume 4 bytes per pixel
            } else if (resource.desc.type == ResourceType::Buffer) {
                resource_size = resource.desc.buffer_desc.size;
            }
            
            stats_.memory_used += resource_size;
            
            if (resource.is_aliased) {
                stats_.aliased_resources++;
                stats_.memory_saved += resource_size;
            }
        }
    }
}

void RenderGraph::allocate_transient_resources() {
    for (auto& resource : compiled_resources_) {
        if (resource.desc.lifetime == ResourceLifetime::Transient) {
            if (resource.desc.type == ResourceType::Texture) {
                resource.texture = renderer_->create_texture(resource.desc.texture_desc);
            } else if (resource.desc.type == ResourceType::Buffer) {
                resource.buffer = renderer_->create_buffer(resource.desc.buffer_desc);
            }
        } else if (resource.desc.lifetime == ResourceLifetime::Imported) {
            // Use imported handles
            if (resource.desc.type == ResourceType::Texture) {
                auto it = imported_textures_.find(resource.desc.name);
                if (it != imported_textures_.end()) {
                    resource.texture = it->second;
                }
            } else if (resource.desc.type == ResourceType::Buffer) {
                auto it = imported_buffers_.find(resource.desc.name);
                if (it != imported_buffers_.end()) {
                    resource.buffer = it->second;
                }
            }
        }
    }
}

void RenderGraph::setup_resource_aliasing() {
    // Simple resource aliasing based on non-overlapping lifetimes
    // In a real implementation, this would be more sophisticated
    
    for (size_t i = 0; i < compiled_resources_.size(); ++i) {
        auto& resource_a = compiled_resources_[i];
        if (resource_a.desc.lifetime != ResourceLifetime::Transient || resource_a.is_aliased) {
            continue;
        }
        
        for (size_t j = i + 1; j < compiled_resources_.size(); ++j) {
            auto& resource_b = compiled_resources_[j];
            if (resource_b.desc.lifetime != ResourceLifetime::Transient || resource_b.is_aliased) {
                continue;
            }
            
            if (can_alias_resources(resource_a, resource_b)) {
                // Alias resource B to resource A
                resource_b.is_aliased = true;
                resource_b.alias_target = &resource_a;
                
                if (resource_a.desc.type == ResourceType::Texture) {
                    resource_b.texture = resource_a.texture;
                } else if (resource_a.desc.type == ResourceType::Buffer) {
                    resource_b.buffer = resource_a.buffer;
                }
                
                // Destroy the original resource B since it's now aliased
                if (resource_b.desc.type == ResourceType::Texture && resource_b.texture.is_valid()) {
                    renderer_->destroy_texture(resource_b.texture);
                } else if (resource_b.desc.type == ResourceType::Buffer && resource_b.buffer.is_valid()) {
                    renderer_->destroy_buffer(resource_b.buffer);
                }
                
                break; // Only alias to one resource for simplicity
            }
        }
    }
}

bool RenderGraph::can_alias_resources(const CompiledResource& a, const CompiledResource& b) const {
    // Check if resources have compatible types and formats
    if (a.desc.type != b.desc.type) {
        return false;
    }
    
    // Check for non-overlapping lifetimes
    if (a.last_use_pass < b.first_use_pass || b.last_use_pass < a.first_use_pass) {
        return true;
    }
    
    return false;
}

} // namespace ecscope::rendering