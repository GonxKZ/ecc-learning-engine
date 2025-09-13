#include "../../include/ecscope/gui/cpu_gpu_optimization.hpp"
#include <algorithm>
#include <cstring>

#ifdef _WIN32
#include <windows.h>
#include <d3d11.h>
#pragma comment(lib, "d3d11.lib")
#elif __linux__
#include <GL/gl.h>
#include <GL/glext.h>
#elif __APPLE__
#include <OpenGL/gl3.h>
#include <OpenGL/gl3ext.h>
#endif

namespace ecscope::gui::optimization {

// BatchRenderer implementation
BatchRenderer::BatchRenderer() {
    m_vertices.reserve(MAX_VERTICES);
    m_indices.reserve(MAX_INDICES);
    m_commands.reserve(MAX_COMMANDS);
    
    // Create GPU buffers
#ifdef _WIN32
    // D3D11 buffer creation would go here
#else
    // OpenGL buffer creation
    glGenBuffers(1, &m_vertex_buffer);
    glGenBuffers(1, &m_index_buffer);
    
    // Allocate GPU memory
    glBindBuffer(GL_ARRAY_BUFFER, m_vertex_buffer);
    glBufferData(GL_ARRAY_BUFFER, MAX_VERTICES * sizeof(Vertex), nullptr, GL_DYNAMIC_DRAW);
    
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_index_buffer);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, MAX_INDICES * sizeof(uint16_t), nullptr, GL_DYNAMIC_DRAW);
#endif
}

BatchRenderer::~BatchRenderer() {
#ifdef _WIN32
    // D3D11 cleanup
#else
    // OpenGL cleanup
    if (m_vertex_buffer) glDeleteBuffers(1, &m_vertex_buffer);
    if (m_index_buffer) glDeleteBuffers(1, &m_index_buffer);
#endif
}

void BatchRenderer::BeginBatch() {
    m_vertices.clear();
    m_indices.clear();
    m_commands.clear();
    m_vertex_count = 0;
    m_index_count = 0;
}

void BatchRenderer::AddQuad(const float* positions, const float* uvs, uint32_t color, uint32_t texture_id) {
    if (m_vertex_count + 4 > MAX_VERTICES || m_index_count + 6 > MAX_INDICES) {
        Flush();
        BeginBatch();
    }
    
    // Check if we can merge with the last command
    bool can_merge = false;
    if (!m_commands.empty()) {
        auto& last_cmd = m_commands.back();
        if (last_cmd.texture_id == texture_id && last_cmd.shader_id == 0) {
            can_merge = true;
            last_cmd.index_count += 6;
        }
    }
    
    // Add vertices
    uint32_t base_vertex = static_cast<uint32_t>(m_vertices.size());
    
    for (int i = 0; i < 4; ++i) {
        Vertex v;
        v.position[0] = positions[i * 2];
        v.position[1] = positions[i * 2 + 1];
        v.position[2] = 0.0f;
        v.uv[0] = uvs[i * 2];
        v.uv[1] = uvs[i * 2 + 1];
        v.color = color;
        m_vertices.push_back(v);
    }
    
    // Add indices
    uint16_t quad_indices[6] = {
        static_cast<uint16_t>(base_vertex + 0),
        static_cast<uint16_t>(base_vertex + 1),
        static_cast<uint16_t>(base_vertex + 2),
        static_cast<uint16_t>(base_vertex + 0),
        static_cast<uint16_t>(base_vertex + 2),
        static_cast<uint16_t>(base_vertex + 3)
    };
    
    m_indices.insert(m_indices.end(), quad_indices, quad_indices + 6);
    
    // Add command if not merged
    if (!can_merge) {
        DrawCommand cmd;
        cmd.vertex_offset = base_vertex;
        cmd.index_offset = static_cast<uint32_t>(m_index_count);
        cmd.index_count = 6;
        cmd.instance_count = 1;
        cmd.texture_id = texture_id;
        cmd.shader_id = 0;
        cmd.depth = 0.0f;
        m_commands.push_back(cmd);
    }
    
    m_vertex_count += 4;
    m_index_count += 6;
}

void BatchRenderer::AddTriangle(const Vertex* vertices, uint32_t texture_id) {
    if (m_vertex_count + 3 > MAX_VERTICES || m_index_count + 3 > MAX_INDICES) {
        Flush();
        BeginBatch();
    }
    
    uint32_t base_vertex = static_cast<uint32_t>(m_vertices.size());
    
    // Add vertices
    m_vertices.insert(m_vertices.end(), vertices, vertices + 3);
    
    // Add indices
    for (int i = 0; i < 3; ++i) {
        m_indices.push_back(static_cast<uint16_t>(base_vertex + i));
    }
    
    // Check if we can merge with the last command
    bool merged = false;
    if (!m_commands.empty()) {
        auto& last_cmd = m_commands.back();
        if (last_cmd.texture_id == texture_id && last_cmd.shader_id == 0) {
            last_cmd.index_count += 3;
            merged = true;
        }
    }
    
    if (!merged) {
        DrawCommand cmd;
        cmd.vertex_offset = base_vertex;
        cmd.index_offset = static_cast<uint32_t>(m_index_count);
        cmd.index_count = 3;
        cmd.instance_count = 1;
        cmd.texture_id = texture_id;
        cmd.shader_id = 0;
        cmd.depth = 0.0f;
        m_commands.push_back(cmd);
    }
    
    m_vertex_count += 3;
    m_index_count += 3;
}

void BatchRenderer::AddMesh(const Vertex* vertices, size_t vertex_count,
                           const uint16_t* indices, size_t index_count,
                           uint32_t texture_id, uint32_t shader_id) {
    if (m_vertex_count + vertex_count > MAX_VERTICES || 
        m_index_count + index_count > MAX_INDICES) {
        Flush();
        BeginBatch();
    }
    
    uint32_t base_vertex = static_cast<uint32_t>(m_vertices.size());
    
    // Add vertices
    m_vertices.insert(m_vertices.end(), vertices, vertices + vertex_count);
    
    // Add indices with offset
    for (size_t i = 0; i < index_count; ++i) {
        m_indices.push_back(static_cast<uint16_t>(base_vertex + indices[i]));
    }
    
    // Add draw command
    DrawCommand cmd;
    cmd.vertex_offset = base_vertex;
    cmd.index_offset = static_cast<uint32_t>(m_index_count);
    cmd.index_count = static_cast<uint32_t>(index_count);
    cmd.instance_count = 1;
    cmd.texture_id = texture_id;
    cmd.shader_id = shader_id;
    cmd.depth = 0.0f;
    m_commands.push_back(cmd);
    
    m_vertex_count += vertex_count;
    m_index_count += index_count;
}

void BatchRenderer::EndBatch() {
    if (m_sorting_enabled) {
        SortCommands();
    }
    
    if (m_instancing_enabled) {
        MergeCommands();
    }
}

void BatchRenderer::Flush() {
    if (m_commands.empty()) return;
    
    EndBatch();
    
    // Upload vertex data
#ifdef _WIN32
    // D3D11 upload
#else
    // OpenGL upload
    glBindBuffer(GL_ARRAY_BUFFER, m_vertex_buffer);
    glBufferSubData(GL_ARRAY_BUFFER, 0, m_vertices.size() * sizeof(Vertex), m_vertices.data());
    
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_index_buffer);
    glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, m_indices.size() * sizeof(uint16_t), m_indices.data());
#endif
    
    ExecuteCommands();
    
    // Clear for next batch
    BeginBatch();
}

void BatchRenderer::FlushImmediate() {
    Flush();
}

void BatchRenderer::SetMaxBatchSize(size_t vertices, size_t indices) {
    // Would reallocate buffers if needed
}

void BatchRenderer::SortCommands() {
    // Sort by depth, then by texture, then by shader to minimize state changes
    std::sort(m_commands.begin(), m_commands.end(),
        [](const DrawCommand& a, const DrawCommand& b) {
            if (std::abs(a.depth - b.depth) > 0.001f)
                return a.depth < b.depth;
            if (a.shader_id != b.shader_id)
                return a.shader_id < b.shader_id;
            return a.texture_id < b.texture_id;
        });
}

void BatchRenderer::MergeCommands() {
    if (m_commands.size() < 2) return;
    
    std::vector<DrawCommand> merged;
    merged.reserve(m_commands.size());
    
    DrawCommand current = m_commands[0];
    
    for (size_t i = 1; i < m_commands.size(); ++i) {
        const auto& cmd = m_commands[i];
        
        // Check if we can merge (same state, consecutive indices)
        if (cmd.texture_id == current.texture_id &&
            cmd.shader_id == current.shader_id &&
            cmd.index_offset == current.index_offset + current.index_count) {
            // Extend current command
            current.index_count += cmd.index_count;
        } else {
            // Can't merge, save current and start new
            merged.push_back(current);
            current = cmd;
        }
    }
    
    merged.push_back(current);
    m_commands = std::move(merged);
}

void BatchRenderer::ExecuteCommands() {
    uint32_t current_texture = 0;
    uint32_t current_shader = 0;
    
    for (const auto& cmd : m_commands) {
        // Minimize state changes
        if (cmd.shader_id != current_shader) {
            // Bind shader
            current_shader = cmd.shader_id;
        }
        
        if (cmd.texture_id != current_texture) {
            // Bind texture
            current_texture = cmd.texture_id;
        }
        
        // Draw
#ifdef _WIN32
        // D3D11 draw call
#else
        // OpenGL draw call
        if (cmd.instance_count > 1) {
            glDrawElementsInstanced(GL_TRIANGLES, cmd.index_count, GL_UNSIGNED_SHORT,
                                  reinterpret_cast<void*>(cmd.index_offset * sizeof(uint16_t)),
                                  cmd.instance_count);
        } else {
            glDrawElements(GL_TRIANGLES, cmd.index_count, GL_UNSIGNED_SHORT,
                         reinterpret_cast<void*>(cmd.index_offset * sizeof(uint16_t)));
        }
#endif
        
        m_draw_call_count++;
    }
}

// CommandBuffer implementation
CommandBuffer::CommandBuffer() {
    m_commands.reserve(1024);
}

void CommandBuffer::Begin() {
    m_recording = true;
    m_commands.clear();
    m_state_cache = StateCache{};
}

void CommandBuffer::End() {
    m_recording = false;
    Optimize();
}

void CommandBuffer::Reset() {
    m_commands.clear();
    m_state_cache = StateCache{};
}

void CommandBuffer::SetViewport(int x, int y, int width, int height) {
    if (!m_recording) return;
    
    // Check for redundant state change
    if (m_state_cache.viewport[0] == x &&
        m_state_cache.viewport[1] == y &&
        m_state_cache.viewport[2] == width &&
        m_state_cache.viewport[3] == height) {
        return;
    }
    
    Command cmd;
    cmd.type = CommandType::SET_VIEWPORT;
    cmd.data.viewport = {x, y, width, height};
    m_commands.push_back(cmd);
    
    m_state_cache.viewport[0] = x;
    m_state_cache.viewport[1] = y;
    m_state_cache.viewport[2] = width;
    m_state_cache.viewport[3] = height;
}

void CommandBuffer::SetScissor(int x, int y, int width, int height) {
    if (!m_recording) return;
    
    Command cmd;
    cmd.type = CommandType::SET_SCISSOR;
    cmd.data.scissor = {x, y, width, height};
    m_commands.push_back(cmd);
}

void CommandBuffer::SetShader(uint32_t shader_id) {
    if (!m_recording) return;
    
    if (m_state_cache.current_shader == shader_id) {
        return; // Skip redundant change
    }
    
    Command cmd;
    cmd.type = CommandType::SET_SHADER;
    cmd.data.shader.id = shader_id;
    m_commands.push_back(cmd);
    
    m_state_cache.current_shader = shader_id;
}

void CommandBuffer::SetTexture(uint32_t slot, uint32_t texture_id) {
    if (!m_recording) return;
    
    if (slot < 16 && m_state_cache.current_textures[slot] == texture_id) {
        return; // Skip redundant change
    }
    
    Command cmd;
    cmd.type = CommandType::SET_TEXTURE;
    cmd.data.texture = {slot, texture_id};
    m_commands.push_back(cmd);
    
    if (slot < 16) {
        m_state_cache.current_textures[slot] = texture_id;
    }
}

void CommandBuffer::SetUniform(uint32_t location, const float* values, size_t count) {
    if (!m_recording) return;
    
    Command cmd;
    cmd.type = CommandType::SET_UNIFORM;
    cmd.data.uniform.location = location;
    std::memcpy(cmd.data.uniform.values, values, std::min(count, size_t(16)) * sizeof(float));
    m_commands.push_back(cmd);
}

void CommandBuffer::SetBlendState(bool enable, uint32_t src, uint32_t dst) {
    if (!m_recording) return;
    
    Command cmd;
    cmd.type = CommandType::SET_BLEND_STATE;
    cmd.data.blend = {enable, src, dst};
    m_commands.push_back(cmd);
    
    m_state_cache.blend_enabled = enable;
}

void CommandBuffer::SetDepthState(bool enable, bool write, uint32_t func) {
    if (!m_recording) return;
    
    Command cmd;
    cmd.type = CommandType::SET_DEPTH_STATE;
    cmd.data.depth = {enable, write, func};
    m_commands.push_back(cmd);
    
    m_state_cache.depth_enabled = enable;
}

void CommandBuffer::Draw(uint32_t vertex_count, uint32_t vertex_offset) {
    if (!m_recording) return;
    
    Command cmd;
    cmd.type = CommandType::DRAW;
    cmd.data.draw = {vertex_count, vertex_offset};
    m_commands.push_back(cmd);
}

void CommandBuffer::DrawIndexed(uint32_t index_count, uint32_t index_offset) {
    if (!m_recording) return;
    
    Command cmd;
    cmd.type = CommandType::DRAW_INDEXED;
    cmd.data.draw = {index_count, index_offset};
    m_commands.push_back(cmd);
}

void CommandBuffer::DrawInstanced(uint32_t vertex_count, uint32_t instance_count, uint32_t vertex_offset) {
    if (!m_recording) return;
    
    Command cmd;
    cmd.type = CommandType::DRAW_INSTANCED;
    cmd.data.draw_instanced = {vertex_count, vertex_offset, instance_count};
    m_commands.push_back(cmd);
}

void CommandBuffer::Clear(float r, float g, float b, float a, uint32_t mask) {
    if (!m_recording) return;
    
    Command cmd;
    cmd.type = CommandType::CLEAR;
    cmd.data.clear = {r, g, b, a, mask};
    m_commands.push_back(cmd);
}

void CommandBuffer::CopyTexture(uint32_t src, uint32_t dst, int x, int y, int width, int height) {
    if (!m_recording) return;
    
    Command cmd;
    cmd.type = CommandType::COPY_TEXTURE;
    cmd.data.copy = {src, dst, x, y, width, height};
    m_commands.push_back(cmd);
}

void CommandBuffer::DispatchCompute(uint32_t x, uint32_t y, uint32_t z) {
    if (!m_recording) return;
    
    Command cmd;
    cmd.type = CommandType::DISPATCH_COMPUTE;
    cmd.data.compute = {x, y, z};
    m_commands.push_back(cmd);
}

void CommandBuffer::Execute() {
    for (const auto& cmd : m_commands) {
        switch (cmd.type) {
            case CommandType::SET_VIEWPORT:
#ifdef _WIN32
                // D3D11 viewport
#else
                glViewport(cmd.data.viewport.x, cmd.data.viewport.y,
                          cmd.data.viewport.width, cmd.data.viewport.height);
#endif
                break;
                
            case CommandType::SET_SCISSOR:
#ifdef _WIN32
                // D3D11 scissor
#else
                glScissor(cmd.data.scissor.x, cmd.data.scissor.y,
                         cmd.data.scissor.width, cmd.data.scissor.height);
#endif
                break;
                
            case CommandType::SET_SHADER:
#ifdef _WIN32
                // D3D11 shader
#else
                glUseProgram(cmd.data.shader.id);
#endif
                break;
                
            case CommandType::SET_TEXTURE:
#ifdef _WIN32
                // D3D11 texture
#else
                glActiveTexture(GL_TEXTURE0 + cmd.data.texture.slot);
                glBindTexture(GL_TEXTURE_2D, cmd.data.texture.id);
#endif
                break;
                
            case CommandType::DRAW:
#ifdef _WIN32
                // D3D11 draw
#else
                glDrawArrays(GL_TRIANGLES, cmd.data.draw.offset, cmd.data.draw.count);
#endif
                break;
                
            case CommandType::DRAW_INDEXED:
#ifdef _WIN32
                // D3D11 draw indexed
#else
                glDrawElements(GL_TRIANGLES, cmd.data.draw.count, GL_UNSIGNED_SHORT,
                             reinterpret_cast<void*>(cmd.data.draw.offset * sizeof(uint16_t)));
#endif
                break;
                
            case CommandType::CLEAR:
#ifdef _WIN32
                // D3D11 clear
#else
                glClearColor(cmd.data.clear.r, cmd.data.clear.g, cmd.data.clear.b, cmd.data.clear.a);
                glClear(cmd.data.clear.mask);
#endif
                break;
                
            default:
                break;
        }
    }
}

void CommandBuffer::ExecuteSecondary(CommandBuffer& secondary) {
    m_commands.insert(m_commands.end(), secondary.m_commands.begin(), secondary.m_commands.end());
}

void CommandBuffer::Optimize() {
    if (m_commands.size() < 2) return;
    
    std::vector<Command> optimized;
    optimized.reserve(m_commands.size());
    
    StateCache state;
    
    for (const auto& cmd : m_commands) {
        bool redundant = false;
        
        switch (cmd.type) {
            case CommandType::SET_SHADER:
                if (state.current_shader == cmd.data.shader.id) {
                    redundant = true;
                } else {
                    state.current_shader = cmd.data.shader.id;
                }
                break;
                
            case CommandType::SET_TEXTURE:
                if (cmd.data.texture.slot < 16) {
                    if (state.current_textures[cmd.data.texture.slot] == cmd.data.texture.id) {
                        redundant = true;
                    } else {
                        state.current_textures[cmd.data.texture.slot] = cmd.data.texture.id;
                    }
                }
                break;
                
            case CommandType::SET_VIEWPORT:
                if (state.viewport[0] == cmd.data.viewport.x &&
                    state.viewport[1] == cmd.data.viewport.y &&
                    state.viewport[2] == cmd.data.viewport.width &&
                    state.viewport[3] == cmd.data.viewport.height) {
                    redundant = true;
                } else {
                    state.viewport[0] = cmd.data.viewport.x;
                    state.viewport[1] = cmd.data.viewport.y;
                    state.viewport[2] = cmd.data.viewport.width;
                    state.viewport[3] = cmd.data.viewport.height;
                }
                break;
                
            default:
                break;
        }
        
        if (!redundant) {
            optimized.push_back(cmd);
        }
    }
    
    m_commands = std::move(optimized);
}

// RenderStateCache implementation
RenderStateCache::RenderStateCache() {
    // Initialize to invalid state to force first set
    m_current_state.shader_id = static_cast<uint32_t>(-1);
    std::fill(std::begin(m_current_state.texture_ids), std::end(m_current_state.texture_ids), 
              static_cast<uint32_t>(-1));
}

bool RenderStateCache::SetShader(uint32_t shader_id) {
    if (m_current_state.shader_id == shader_id) {
        m_redundant_changes++;
        return false;
    }
    
    m_current_state.shader_id = shader_id;
    m_state_changes++;
    return true;
}

bool RenderStateCache::SetTexture(uint32_t slot, uint32_t texture_id) {
    if (slot >= 8) return false;
    
    if (m_current_state.texture_ids[slot] == texture_id) {
        m_redundant_changes++;
        return false;
    }
    
    m_current_state.texture_ids[slot] = texture_id;
    m_state_changes++;
    return true;
}

bool RenderStateCache::SetVertexBuffer(uint32_t buffer_id) {
    if (m_current_state.vertex_buffer == buffer_id) {
        m_redundant_changes++;
        return false;
    }
    
    m_current_state.vertex_buffer = buffer_id;
    m_state_changes++;
    return true;
}

bool RenderStateCache::SetIndexBuffer(uint32_t buffer_id) {
    if (m_current_state.index_buffer == buffer_id) {
        m_redundant_changes++;
        return false;
    }
    
    m_current_state.index_buffer = buffer_id;
    m_state_changes++;
    return true;
}

bool RenderStateCache::SetBlendMode(uint32_t mode) {
    if (m_current_state.blend_mode == mode) {
        m_redundant_changes++;
        return false;
    }
    
    m_current_state.blend_mode = mode;
    m_state_changes++;
    return true;
}

bool RenderStateCache::SetDepthFunc(uint32_t func) {
    if (m_current_state.depth_func == func) {
        m_redundant_changes++;
        return false;
    }
    
    m_current_state.depth_func = func;
    m_state_changes++;
    return true;
}

bool RenderStateCache::SetDepthWrite(bool write) {
    if (m_current_state.depth_write == write) {
        m_redundant_changes++;
        return false;
    }
    
    m_current_state.depth_write = write;
    m_state_changes++;
    return true;
}

bool RenderStateCache::SetCullFace(bool enable, uint32_t mode) {
    if (m_current_state.cull_face == enable && m_current_state.cull_mode == mode) {
        m_redundant_changes++;
        return false;
    }
    
    m_current_state.cull_face = enable;
    m_current_state.cull_mode = mode;
    m_state_changes++;
    return true;
}

bool RenderStateCache::SetViewport(int x, int y, int width, int height) {
    if (m_current_state.viewport[0] == x &&
        m_current_state.viewport[1] == y &&
        m_current_state.viewport[2] == width &&
        m_current_state.viewport[3] == height) {
        m_redundant_changes++;
        return false;
    }
    
    m_current_state.viewport[0] = x;
    m_current_state.viewport[1] = y;
    m_current_state.viewport[2] = width;
    m_current_state.viewport[3] = height;
    m_state_changes++;
    return true;
}

bool RenderStateCache::SetScissor(int x, int y, int width, int height) {
    if (m_current_state.scissor[0] == x &&
        m_current_state.scissor[1] == y &&
        m_current_state.scissor[2] == width &&
        m_current_state.scissor[3] == height) {
        m_redundant_changes++;
        return false;
    }
    
    m_current_state.scissor[0] = x;
    m_current_state.scissor[1] = y;
    m_current_state.scissor[2] = width;
    m_current_state.scissor[3] = height;
    m_state_changes++;
    return true;
}

void RenderStateCache::Reset() {
    m_current_state = RenderState{};
    m_current_state.shader_id = static_cast<uint32_t>(-1);
    std::fill(std::begin(m_current_state.texture_ids), std::end(m_current_state.texture_ids), 
              static_cast<uint32_t>(-1));
}

// ParallelCommandGenerator implementation
ParallelCommandGenerator::ParallelCommandGenerator(size_t thread_count) {
    m_thread_buffers.resize(thread_count);
    for (auto& buffer : m_thread_buffers) {
        buffer = std::make_unique<CommandBuffer>();
    }
    
    for (size_t i = 0; i < thread_count; ++i) {
        m_threads.emplace_back(&ParallelCommandGenerator::WorkerThread, this, i);
    }
}

ParallelCommandGenerator::~ParallelCommandGenerator() {
    {
        std::unique_lock<std::mutex> lock(m_queue_mutex);
        m_stop = true;
    }
    m_condition.notify_all();
    
    for (auto& thread : m_threads) {
        if (thread.joinable()) {
            thread.join();
        }
    }
}

void ParallelCommandGenerator::GenerateCommands(const std::vector<CommandGenFunc>& generators) {
    {
        std::unique_lock<std::mutex> lock(m_queue_mutex);
        for (const auto& gen : generators) {
            m_work_queue.push(gen);
        }
    }
    m_condition.notify_all();
    
    // Wait for all work to complete
    while (!m_work_queue.empty()) {
        std::this_thread::yield();
    }
}

void ParallelCommandGenerator::ExecuteCommands() {
    if (!m_main_buffer) return;
    
    for (auto& buffer : m_thread_buffers) {
        m_main_buffer->ExecuteSecondary(*buffer);
        buffer->Reset();
    }
}

void ParallelCommandGenerator::WorkerThread(size_t thread_id) {
    while (!m_stop) {
        CommandGenFunc work;
        
        {
            std::unique_lock<std::mutex> lock(m_queue_mutex);
            m_condition.wait(lock, [this] { return m_stop || !m_work_queue.empty(); });
            
            if (m_stop) break;
            
            if (!m_work_queue.empty()) {
                work = m_work_queue.front();
                m_work_queue.pop();
            }
        }
        
        if (work) {
            m_thread_buffers[thread_id]->Begin();
            work(*m_thread_buffers[thread_id], thread_id);
            m_thread_buffers[thread_id]->End();
        }
    }
}

// OcclusionCuller implementation
OcclusionCuller::OcclusionCuller() {
    // Initialize depth pyramid
#ifdef _WIN32
    // D3D11 depth texture creation
#else
    // OpenGL depth texture creation
    glGenTextures(8, m_hi_z_pyramid);
#endif
}

void OcclusionCuller::BeginFrame(const float* view_matrix, const float* proj_matrix) {
    // Compute view-projection matrix
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            m_view_proj_matrix[i * 4 + j] = 0;
            for (int k = 0; k < 4; ++k) {
                m_view_proj_matrix[i * 4 + j] += 
                    proj_matrix[i * 4 + k] * view_matrix[k * 4 + j];
            }
        }
    }
    
    m_culled_objects = 0;
    m_visible_objects = 0;
}

void OcclusionCuller::EndFrame() {
    if (m_use_hi_z) {
        BuildHierarchicalZ();
    }
}

bool OcclusionCuller::IsVisible(const BoundingBox& bbox) const {
    // Transform bounding box to clip space
    float corners[8][4];
    int corner_idx = 0;
    
    for (int z = 0; z <= 1; ++z) {
        for (int y = 0; y <= 1; ++y) {
            for (int x = 0; x <= 1; ++x) {
                float world[4] = {
                    x ? bbox.max[0] : bbox.min[0],
                    y ? bbox.max[1] : bbox.min[1],
                    z ? bbox.max[2] : bbox.min[2],
                    1.0f
                };
                
                // Transform to clip space
                for (int i = 0; i < 4; ++i) {
                    corners[corner_idx][i] = 0;
                    for (int j = 0; j < 4; ++j) {
                        corners[corner_idx][i] += m_view_proj_matrix[i * 4 + j] * world[j];
                    }
                }
                
                // Perspective divide
                if (corners[corner_idx][3] != 0) {
                    corners[corner_idx][0] /= corners[corner_idx][3];
                    corners[corner_idx][1] /= corners[corner_idx][3];
                    corners[corner_idx][2] /= corners[corner_idx][3];
                }
                
                corner_idx++;
            }
        }
    }
    
    // Check frustum culling
    for (int i = 0; i < 8; ++i) {
        if (corners[i][0] >= -1.0f && corners[i][0] <= 1.0f &&
            corners[i][1] >= -1.0f && corners[i][1] <= 1.0f &&
            corners[i][2] >= 0.0f && corners[i][2] <= 1.0f) {
            // At least one corner is visible
            
            if (m_use_hi_z) {
                bool occluded = TestAgainstHiZ(bbox);
                if (occluded) {
                    m_culled_objects++;
                    return false;
                }
            }
            
            m_visible_objects++;
            return true;
        }
    }
    
    m_culled_objects++;
    return false;
}

void OcclusionCuller::SubmitOccluder(const BoundingBox& bbox) {
    // Render occluder to depth buffer
    // This would typically render a simplified version of the object
}

void OcclusionCuller::SetResolution(uint32_t width, uint32_t height) {
    m_width = width;
    m_height = height;
    
    // Recreate depth pyramid textures
    uint32_t mip_width = width;
    uint32_t mip_height = height;
    
    for (int i = 0; i < 8 && mip_width > 1 && mip_height > 1; ++i) {
        mip_width /= 2;
        mip_height /= 2;
        
#ifdef _WIN32
        // D3D11 texture creation
#else
        // OpenGL texture creation
        glBindTexture(GL_TEXTURE_2D, m_hi_z_pyramid[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32F,
                    mip_width, mip_height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
#endif
    }
}

void OcclusionCuller::BuildHierarchicalZ() {
    // Build hierarchical Z-buffer pyramid
    // Each level stores the maximum depth of the 2x2 region below it
    
#ifdef _WIN32
    // D3D11 compute shader or pixel shader approach
#else
    // OpenGL compute shader approach
    // This is a simplified version - real implementation would use compute shaders
#endif
}

bool OcclusionCuller::TestAgainstHiZ(const BoundingBox& bbox) const {
    // Test bounding box against hierarchical Z-buffer
    // Start at appropriate mip level based on projected size
    
    // This is a simplified version
    // Real implementation would:
    // 1. Project bbox to screen space
    // 2. Determine appropriate mip level
    // 3. Sample Hi-Z texture
    // 4. Compare depths
    
    return false; // Not occluded
}

// TextureStreamer implementation
TextureStreamer::TextureStreamer(size_t memory_budget_mb)
    : m_memory_budget(memory_budget_mb * 1024 * 1024) {
    m_streaming_thread = std::thread(&TextureStreamer::StreamingThread, this);
}

TextureStreamer::~TextureStreamer() {
    m_stop = true;
    m_condition.notify_all();
    
    if (m_streaming_thread.joinable()) {
        m_streaming_thread.join();
    }
}

void TextureStreamer::RequestTexture(const std::string& path, Priority priority,
                                    std::function<void(uint32_t)> callback) {
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        
        // Check if already loaded
        auto it = m_loaded_textures.find(path);
        if (it != m_loaded_textures.end()) {
            callback(it->second);
            return;
        }
        
        // Add to request queue
        TextureRequest request;
        request.path = path;
        request.mip_level = 0;
        request.priority = priority;
        request.callback = callback;
        
        m_request_queue.push(request);
    }
    
    m_condition.notify_one();
}

void TextureStreamer::RequestMipLevel(uint32_t texture_id, uint32_t mip_level) {
    // Request higher resolution mip level for existing texture
    // This would stream in higher quality texture data
}

void TextureStreamer::Update(float delta_time) {
    // Update streaming priorities based on camera position
    // Evict unused textures if over budget
    
    if (m_current_memory > m_memory_budget) {
        EvictTextures();
    }
}

void TextureStreamer::SetMemoryBudget(size_t budget_mb) {
    m_memory_budget = budget_mb * 1024 * 1024;
}

void TextureStreamer::TrackCameraMovement(const float* position, const float* direction) {
    if (position) {
        std::memcpy(m_camera_pos, position, sizeof(float) * 3);
    }
    if (direction) {
        std::memcpy(m_camera_dir, direction, sizeof(float) * 3);
    }
    
    if (m_predictive_loading) {
        // Predict which textures will be needed based on camera movement
        // and preload them
    }
}

void TextureStreamer::StreamingThread() {
    while (!m_stop) {
        TextureRequest request;
        
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            m_condition.wait(lock, [this] { return m_stop || !m_request_queue.empty(); });
            
            if (m_stop) break;
            
            if (!m_request_queue.empty()) {
                request = m_request_queue.top();
                m_request_queue.pop();
            }
        }
        
        if (request.callback) {
            LoadTexture(request);
        }
    }
}

void TextureStreamer::LoadTexture(const TextureRequest& request) {
    // Simulate texture loading
    // In a real implementation, this would:
    // 1. Load texture from disk
    // 2. Decompress if needed
    // 3. Upload to GPU
    // 4. Generate mipmaps
    
    uint32_t texture_id = static_cast<uint32_t>(std::hash<std::string>{}(request.path));
    
    // Simulate memory usage
    size_t texture_size = 1024 * 1024 * 4; // 4MB for a 1024x1024 RGBA texture
    
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        
        // Check memory budget
        while (m_current_memory + texture_size > m_memory_budget) {
            EvictTextures();
            if (m_loaded_textures.empty()) break;
        }
        
        m_loaded_textures[request.path] = texture_id;
        m_current_memory += texture_size;
    }
    
    // Call callback with loaded texture ID
    request.callback(texture_id);
}

void TextureStreamer::EvictTextures() {
    // Evict least recently used textures
    // In a real implementation, this would track usage and evict based on LRU
    
    if (!m_loaded_textures.empty()) {
        auto it = m_loaded_textures.begin();
        m_loaded_textures.erase(it);
        m_current_memory -= 1024 * 1024 * 4; // Assume 4MB per texture
    }
}

// GPUMemoryManager implementation
GPUMemoryManager::GPUMemoryManager(size_t budget_mb)
    : m_budget(budget_mb * 1024 * 1024) {
}

GPUMemoryManager::Allocation* GPUMemoryManager::Allocate(MemoryType type, size_t size, bool persistent) {
    std::unique_lock<std::mutex> lock(m_mutex);
    
    if (m_used_memory + size > m_budget) {
        // Try to free non-persistent allocations
        Defragment();
        
        if (m_used_memory + size > m_budget) {
            return nullptr; // Out of memory
        }
    }
    
    auto allocation = std::make_unique<Allocation>();
    allocation->id = m_next_id++;
    allocation->type = type;
    allocation->size = size;
    allocation->mapped_ptr = nullptr;
    allocation->is_persistent = persistent;
    
    // Allocate GPU memory based on type
#ifdef _WIN32
    // D3D11 resource creation
#else
    // OpenGL buffer/texture creation
    switch (type) {
        case MemoryType::VERTEX_BUFFER: {
            uint32_t buffer;
            glGenBuffers(1, &buffer);
            glBindBuffer(GL_ARRAY_BUFFER, buffer);
            glBufferData(GL_ARRAY_BUFFER, size, nullptr, GL_DYNAMIC_DRAW);
            allocation->id = buffer;
            break;
        }
        case MemoryType::INDEX_BUFFER: {
            uint32_t buffer;
            glGenBuffers(1, &buffer);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffer);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, size, nullptr, GL_DYNAMIC_DRAW);
            allocation->id = buffer;
            break;
        }
        // ... other types
        default:
            break;
    }
#endif
    
    m_used_memory += size;
    
    Allocation* ptr = allocation.get();
    m_allocation_map[allocation->id] = ptr;
    m_allocations.push_back(std::move(allocation));
    
    return ptr;
}

void GPUMemoryManager::Free(Allocation* allocation) {
    if (!allocation) return;
    
    std::unique_lock<std::mutex> lock(m_mutex);
    
    // Free GPU memory
#ifdef _WIN32
    // D3D11 resource release
#else
    // OpenGL resource deletion
    switch (allocation->type) {
        case MemoryType::VERTEX_BUFFER:
        case MemoryType::INDEX_BUFFER: {
            uint32_t buffer = allocation->id;
            glDeleteBuffers(1, &buffer);
            break;
        }
        // ... other types
        default:
            break;
    }
#endif
    
    m_used_memory -= allocation->size;
    m_allocation_map.erase(allocation->id);
    
    // Remove from allocations vector
    m_allocations.erase(
        std::remove_if(m_allocations.begin(), m_allocations.end(),
            [allocation](const std::unique_ptr<Allocation>& a) {
                return a.get() == allocation;
            }),
        m_allocations.end()
    );
}

void* GPUMemoryManager::Map(Allocation* allocation, size_t offset, size_t size) {
    if (!allocation) return nullptr;
    
    if (size == 0) size = allocation->size;
    
#ifdef _WIN32
    // D3D11 map
#else
    // OpenGL map
    switch (allocation->type) {
        case MemoryType::VERTEX_BUFFER:
            glBindBuffer(GL_ARRAY_BUFFER, allocation->id);
            allocation->mapped_ptr = glMapBufferRange(GL_ARRAY_BUFFER, offset, size,
                                                     GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT);
            break;
        case MemoryType::INDEX_BUFFER:
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, allocation->id);
            allocation->mapped_ptr = glMapBufferRange(GL_ELEMENT_ARRAY_BUFFER, offset, size,
                                                     GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT);
            break;
        default:
            break;
    }
#endif
    
    return allocation->mapped_ptr;
}

void GPUMemoryManager::Unmap(Allocation* allocation) {
    if (!allocation || !allocation->mapped_ptr) return;
    
#ifdef _WIN32
    // D3D11 unmap
#else
    // OpenGL unmap
    switch (allocation->type) {
        case MemoryType::VERTEX_BUFFER:
            glBindBuffer(GL_ARRAY_BUFFER, allocation->id);
            glUnmapBuffer(GL_ARRAY_BUFFER);
            break;
        case MemoryType::INDEX_BUFFER:
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, allocation->id);
            glUnmapBuffer(GL_ELEMENT_ARRAY_BUFFER);
            break;
        default:
            break;
    }
#endif
    
    allocation->mapped_ptr = nullptr;
}

void GPUMemoryManager::Defragment() {
    // Remove non-persistent allocations that haven't been used recently
    // In a real implementation, this would track usage patterns
    
    std::vector<Allocation*> to_free;
    
    for (auto& allocation : m_allocations) {
        if (!allocation->is_persistent) {
            // Simple strategy: free 10% of non-persistent allocations
            if (rand() % 10 == 0) {
                to_free.push_back(allocation.get());
            }
        }
    }
    
    for (auto* allocation : to_free) {
        Free(allocation);
    }
}

float GPUMemoryManager::GetFragmentation() const {
    // Calculate fragmentation metric
    // This is simplified - real implementation would track actual GPU memory layout
    
    if (m_allocations.empty()) return 0.0f;
    
    size_t total_allocated = 0;
    size_t largest_free_block = m_budget - m_used_memory;
    
    for (const auto& allocation : m_allocations) {
        total_allocated += allocation->size;
    }
    
    if (total_allocated == 0) return 0.0f;
    
    // Simple fragmentation metric: ratio of largest free block to total free memory
    size_t total_free = m_budget - m_used_memory;
    if (total_free == 0) return 1.0f;
    
    return 1.0f - (static_cast<float>(largest_free_block) / total_free);
}

// SyncOptimizer implementation
SyncOptimizer::SyncOptimizer() {
    m_sync_points.reserve(100);
}

void SyncOptimizer::BeginFrame() {
    m_current_frame++;
    
    // Clear old sync points
    m_sync_points.clear();
    
    InsertSyncPoint(SyncPoint::FRAME_BEGIN);
}

void SyncOptimizer::EndFrame() {
    InsertSyncPoint(SyncPoint::FRAME_END);
    
    // Calculate GPU idle time and CPU wait time
    // This is simplified - real implementation would use GPU timer queries
    
    if (m_sync_points.size() >= 2) {
        auto frame_start = m_sync_points.front().timestamp;
        auto frame_end = m_sync_points.back().timestamp;
        auto frame_duration = std::chrono::duration_cast<std::chrono::microseconds>(frame_end - frame_start);
        
        // Estimate GPU idle time (simplified)
        m_gpu_idle_time = 0.0f; // Would need actual GPU timing
        
        // Estimate CPU wait time (simplified)
        m_cpu_wait_time = 0.0f; // Would need actual sync timing
    }
}

void SyncOptimizer::InsertSyncPoint(SyncPoint type) {
    SyncObject sync;
    sync.type = type;
    sync.timestamp = std::chrono::high_resolution_clock::now();
    
#ifdef _WIN32
    // D3D11 fence creation
#else
    // OpenGL fence sync
    sync.fence = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
#endif
    
    std::unique_lock<std::mutex> lock(m_mutex);
    m_sync_points.push_back(sync);
}

void SyncOptimizer::WaitForSync(SyncPoint type) {
    std::unique_lock<std::mutex> lock(m_mutex);
    
    for (const auto& sync : m_sync_points) {
        if (sync.type == type) {
#ifdef _WIN32
            // D3D11 fence wait
#else
            // OpenGL fence wait
            glClientWaitSync(reinterpret_cast<GLsync>(sync.fence),
                           GL_SYNC_FLUSH_COMMANDS_BIT, 1000000000); // 1 second timeout
#endif
            break;
        }
    }
}

void SyncOptimizer::BeginAsyncTransfer() {
    InsertSyncPoint(SyncPoint::BUFFER_UPLOAD);
}

void SyncOptimizer::EndAsyncTransfer() {
    // Mark transfer complete
}

bool SyncOptimizer::IsTransferComplete() const {
    // Check if async transfer is complete
    // This would check the fence status
    return true;
}

} // namespace ecscope::gui::optimization