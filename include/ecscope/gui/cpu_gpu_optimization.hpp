#pragma once

#include "../core/types.hpp"
#include "../jobs/job_system.hpp"
#include <thread>
#include <atomic>
#include <vector>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <immintrin.h> // SIMD intrinsics

namespace ecscope::gui::optimization {

// Forward declarations
class BatchRenderer;
class CommandBuffer;
class RenderStateCache;
class ShaderCache;
class ThreadPool;

// SIMD optimization utilities
namespace simd {

// Vector math operations using SIMD
inline void TransformVertices4x4_SSE(const float* vertices, float* output, 
                                     const float* matrix, size_t count) {
    const __m128 row0 = _mm_load_ps(&matrix[0]);
    const __m128 row1 = _mm_load_ps(&matrix[4]);
    const __m128 row2 = _mm_load_ps(&matrix[8]);
    const __m128 row3 = _mm_load_ps(&matrix[12]);
    
    for (size_t i = 0; i < count; i += 4) {
        __m128 vx = _mm_load_ps(&vertices[i * 4 + 0]);
        __m128 vy = _mm_load_ps(&vertices[i * 4 + 1]);
        __m128 vz = _mm_load_ps(&vertices[i * 4 + 2]);
        __m128 vw = _mm_load_ps(&vertices[i * 4 + 3]);
        
        __m128 x = _mm_add_ps(
            _mm_add_ps(_mm_mul_ps(vx, _mm_shuffle_ps(row0, row0, 0x00)),
                      _mm_mul_ps(vy, _mm_shuffle_ps(row0, row0, 0x55))),
            _mm_add_ps(_mm_mul_ps(vz, _mm_shuffle_ps(row0, row0, 0xAA)),
                      _mm_mul_ps(vw, _mm_shuffle_ps(row0, row0, 0xFF)))
        );
        
        __m128 y = _mm_add_ps(
            _mm_add_ps(_mm_mul_ps(vx, _mm_shuffle_ps(row1, row1, 0x00)),
                      _mm_mul_ps(vy, _mm_shuffle_ps(row1, row1, 0x55))),
            _mm_add_ps(_mm_mul_ps(vz, _mm_shuffle_ps(row1, row1, 0xAA)),
                      _mm_mul_ps(vw, _mm_shuffle_ps(row1, row1, 0xFF)))
        );
        
        __m128 z = _mm_add_ps(
            _mm_add_ps(_mm_mul_ps(vx, _mm_shuffle_ps(row2, row2, 0x00)),
                      _mm_mul_ps(vy, _mm_shuffle_ps(row2, row2, 0x55))),
            _mm_add_ps(_mm_mul_ps(vz, _mm_shuffle_ps(row2, row2, 0xAA)),
                      _mm_mul_ps(vw, _mm_shuffle_ps(row2, row2, 0xFF)))
        );
        
        __m128 w = _mm_add_ps(
            _mm_add_ps(_mm_mul_ps(vx, _mm_shuffle_ps(row3, row3, 0x00)),
                      _mm_mul_ps(vy, _mm_shuffle_ps(row3, row3, 0x55))),
            _mm_add_ps(_mm_mul_ps(vz, _mm_shuffle_ps(row3, row3, 0xAA)),
                      _mm_mul_ps(vw, _mm_shuffle_ps(row3, row3, 0xFF)))
        );
        
        _mm_store_ps(&output[i * 4 + 0], x);
        _mm_store_ps(&output[i * 4 + 1], y);
        _mm_store_ps(&output[i * 4 + 2], z);
        _mm_store_ps(&output[i * 4 + 3], w);
    }
}

// Color conversion using SIMD
inline void ConvertRGBA8ToFloat_SSE(const uint8_t* input, float* output, size_t pixel_count) {
    const __m128 scale = _mm_set1_ps(1.0f / 255.0f);
    
    for (size_t i = 0; i < pixel_count; i += 4) {
        // Load 16 bytes (4 RGBA pixels)
        __m128i pixels = _mm_loadu_si128(reinterpret_cast<const __m128i*>(&input[i * 4]));
        
        // Unpack to 32-bit integers
        __m128i pixels_lo = _mm_unpacklo_epi8(pixels, _mm_setzero_si128());
        __m128i pixels_hi = _mm_unpackhi_epi8(pixels, _mm_setzero_si128());
        
        __m128i pixel0 = _mm_unpacklo_epi16(pixels_lo, _mm_setzero_si128());
        __m128i pixel1 = _mm_unpackhi_epi16(pixels_lo, _mm_setzero_si128());
        __m128i pixel2 = _mm_unpacklo_epi16(pixels_hi, _mm_setzero_si128());
        __m128i pixel3 = _mm_unpackhi_epi16(pixels_hi, _mm_setzero_si128());
        
        // Convert to float and scale
        _mm_store_ps(&output[i * 16 + 0], _mm_mul_ps(_mm_cvtepi32_ps(pixel0), scale));
        _mm_store_ps(&output[i * 16 + 4], _mm_mul_ps(_mm_cvtepi32_ps(pixel1), scale));
        _mm_store_ps(&output[i * 16 + 8], _mm_mul_ps(_mm_cvtepi32_ps(pixel2), scale));
        _mm_store_ps(&output[i * 16 + 12], _mm_mul_ps(_mm_cvtepi32_ps(pixel3), scale));
    }
}

} // namespace simd

// Batch rendering system for minimizing draw calls
class BatchRenderer {
public:
    struct Vertex {
        float position[3];
        float uv[2];
        uint32_t color;
    };
    
    struct DrawCommand {
        uint32_t vertex_offset;
        uint32_t index_offset;
        uint32_t index_count;
        uint32_t instance_count;
        uint32_t texture_id;
        uint32_t shader_id;
        float depth;
    };
    
    static constexpr size_t MAX_VERTICES = 65536;
    static constexpr size_t MAX_INDICES = 98304;
    static constexpr size_t MAX_COMMANDS = 1024;
    
    BatchRenderer();
    ~BatchRenderer();
    
    // Batching operations
    void BeginBatch();
    void AddQuad(const float* positions, const float* uvs, uint32_t color, uint32_t texture_id);
    void AddTriangle(const Vertex* vertices, uint32_t texture_id);
    void AddMesh(const Vertex* vertices, size_t vertex_count, 
                 const uint16_t* indices, size_t index_count,
                 uint32_t texture_id, uint32_t shader_id);
    void EndBatch();
    
    // Rendering
    void Flush();
    void FlushImmediate(); // Force immediate flush
    
    // Optimization settings
    void SetMaxBatchSize(size_t vertices, size_t indices);
    void EnableInstancing(bool enable) { m_instancing_enabled = enable; }
    void EnableSorting(bool enable) { m_sorting_enabled = enable; }
    
    // Statistics
    size_t GetDrawCallCount() const { return m_draw_call_count; }
    size_t GetVertexCount() const { return m_vertex_count; }
    size_t GetBatchedCommandCount() const { return m_commands.size(); }
    
private:
    void SortCommands();
    void MergeCommands();
    void ExecuteCommands();
    
    std::vector<Vertex> m_vertices;
    std::vector<uint16_t> m_indices;
    std::vector<DrawCommand> m_commands;
    
    size_t m_vertex_count = 0;
    size_t m_index_count = 0;
    size_t m_draw_call_count = 0;
    
    bool m_instancing_enabled = true;
    bool m_sorting_enabled = true;
    
    uint32_t m_vertex_buffer = 0;
    uint32_t m_index_buffer = 0;
    uint32_t m_command_buffer = 0;
    
    mutable std::mutex m_mutex;
};

// GPU command buffer for efficient command recording
class CommandBuffer {
public:
    enum class CommandType {
        SET_VIEWPORT,
        SET_SCISSOR,
        SET_SHADER,
        SET_TEXTURE,
        SET_UNIFORM,
        SET_BLEND_STATE,
        SET_DEPTH_STATE,
        DRAW,
        DRAW_INDEXED,
        DRAW_INSTANCED,
        CLEAR,
        COPY_TEXTURE,
        DISPATCH_COMPUTE
    };
    
    struct Command {
        CommandType type;
        union {
            struct { int x, y, width, height; } viewport;
            struct { int x, y, width, height; } scissor;
            struct { uint32_t id; } shader;
            struct { uint32_t slot; uint32_t id; } texture;
            struct { uint32_t location; float values[16]; } uniform;
            struct { bool enable; uint32_t src, dst; } blend;
            struct { bool enable; bool write; uint32_t func; } depth;
            struct { uint32_t count; uint32_t offset; } draw;
            struct { uint32_t count; uint32_t offset; uint32_t instances; } draw_instanced;
            struct { float r, g, b, a; uint32_t mask; } clear;
            struct { uint32_t src, dst; int x, y, width, height; } copy;
            struct { uint32_t x, y, z; } compute;
        } data;
    };
    
    CommandBuffer();
    
    // Command recording
    void Begin();
    void End();
    void Reset();
    
    // State commands
    void SetViewport(int x, int y, int width, int height);
    void SetScissor(int x, int y, int width, int height);
    void SetShader(uint32_t shader_id);
    void SetTexture(uint32_t slot, uint32_t texture_id);
    void SetUniform(uint32_t location, const float* values, size_t count);
    void SetBlendState(bool enable, uint32_t src, uint32_t dst);
    void SetDepthState(bool enable, bool write, uint32_t func);
    
    // Draw commands
    void Draw(uint32_t vertex_count, uint32_t vertex_offset = 0);
    void DrawIndexed(uint32_t index_count, uint32_t index_offset = 0);
    void DrawInstanced(uint32_t vertex_count, uint32_t instance_count, uint32_t vertex_offset = 0);
    
    // Utility commands
    void Clear(float r, float g, float b, float a, uint32_t mask);
    void CopyTexture(uint32_t src, uint32_t dst, int x, int y, int width, int height);
    void DispatchCompute(uint32_t x, uint32_t y, uint32_t z);
    
    // Execution
    void Execute();
    void ExecuteSecondary(CommandBuffer& secondary);
    
    // Optimization
    void Optimize(); // Remove redundant state changes
    size_t GetCommandCount() const { return m_commands.size(); }
    
private:
    std::vector<Command> m_commands;
    bool m_recording = false;
    
    // State tracking for redundancy elimination
    struct StateCache {
        int viewport[4] = {-1, -1, -1, -1};
        uint32_t current_shader = 0;
        uint32_t current_textures[16] = {0};
        bool blend_enabled = false;
        bool depth_enabled = true;
    } m_state_cache;
    
    mutable std::mutex m_mutex;
};

// Render state cache to minimize state changes
class RenderStateCache {
public:
    struct RenderState {
        uint32_t shader_id = 0;
        uint32_t texture_ids[8] = {0};
        uint32_t vertex_buffer = 0;
        uint32_t index_buffer = 0;
        uint32_t blend_mode = 0;
        uint32_t depth_func = 0;
        bool depth_write = true;
        bool cull_face = true;
        uint32_t cull_mode = 0;
        int viewport[4] = {0, 0, 0, 0};
        int scissor[4] = {0, 0, 0, 0};
    };
    
    RenderStateCache();
    
    bool SetShader(uint32_t shader_id);
    bool SetTexture(uint32_t slot, uint32_t texture_id);
    bool SetVertexBuffer(uint32_t buffer_id);
    bool SetIndexBuffer(uint32_t buffer_id);
    bool SetBlendMode(uint32_t mode);
    bool SetDepthFunc(uint32_t func);
    bool SetDepthWrite(bool write);
    bool SetCullFace(bool enable, uint32_t mode = 0);
    bool SetViewport(int x, int y, int width, int height);
    bool SetScissor(int x, int y, int width, int height);
    
    void Reset();
    const RenderState& GetCurrentState() const { return m_current_state; }
    
    // Statistics
    size_t GetStateChangeCount() const { return m_state_changes; }
    size_t GetRedundantStateChanges() const { return m_redundant_changes; }
    
private:
    RenderState m_current_state;
    std::atomic<size_t> m_state_changes{0};
    std::atomic<size_t> m_redundant_changes{0};
};

// Multi-threaded command generation
class ParallelCommandGenerator {
public:
    using CommandGenFunc = std::function<void(CommandBuffer&, size_t thread_id)>;
    
    ParallelCommandGenerator(size_t thread_count = std::thread::hardware_concurrency());
    ~ParallelCommandGenerator();
    
    void GenerateCommands(const std::vector<CommandGenFunc>& generators);
    void ExecuteCommands();
    
    void SetMainCommandBuffer(CommandBuffer* buffer) { m_main_buffer = buffer; }
    
private:
    std::vector<std::thread> m_threads;
    std::vector<std::unique_ptr<CommandBuffer>> m_thread_buffers;
    CommandBuffer* m_main_buffer = nullptr;
    
    std::queue<CommandGenFunc> m_work_queue;
    std::mutex m_queue_mutex;
    std::condition_variable m_condition;
    std::atomic<bool> m_stop{false};
    
    void WorkerThread(size_t thread_id);
};

// Occlusion culling system
class OcclusionCuller {
public:
    struct BoundingBox {
        float min[3];
        float max[3];
    };
    
    OcclusionCuller();
    
    void BeginFrame(const float* view_matrix, const float* proj_matrix);
    void EndFrame();
    
    bool IsVisible(const BoundingBox& bbox) const;
    void SubmitOccluder(const BoundingBox& bbox);
    
    void EnableHierarchicalZ(bool enable) { m_use_hi_z = enable; }
    void SetResolution(uint32_t width, uint32_t height);
    
    // Statistics
    size_t GetCulledObjectCount() const { return m_culled_objects; }
    size_t GetVisibleObjectCount() const { return m_visible_objects; }
    
private:
    void BuildHierarchicalZ();
    bool TestAgainstHiZ(const BoundingBox& bbox) const;
    
    uint32_t m_depth_buffer = 0;
    uint32_t m_hi_z_pyramid[8] = {0}; // Mip levels
    
    float m_view_proj_matrix[16];
    uint32_t m_width = 0, m_height = 0;
    
    bool m_use_hi_z = true;
    std::atomic<size_t> m_culled_objects{0};
    std::atomic<size_t> m_visible_objects{0};
};

// Texture streaming system
class TextureStreamer {
public:
    enum class Priority {
        IMMEDIATE,  // Load immediately
        HIGH,       // Load ASAP
        NORMAL,     // Standard priority
        LOW,        // Load when idle
        PREFETCH    // Predictive loading
    };
    
    struct TextureRequest {
        std::string path;
        uint32_t mip_level;
        Priority priority;
        std::function<void(uint32_t)> callback;
    };
    
    TextureStreamer(size_t memory_budget_mb = 256);
    ~TextureStreamer();
    
    void RequestTexture(const std::string& path, Priority priority, 
                       std::function<void(uint32_t)> callback);
    void RequestMipLevel(uint32_t texture_id, uint32_t mip_level);
    
    void Update(float delta_time);
    void SetMemoryBudget(size_t budget_mb);
    
    // Predictive loading
    void EnablePredictiveLoading(bool enable) { m_predictive_loading = enable; }
    void TrackCameraMovement(const float* position, const float* direction);
    
private:
    void StreamingThread();
    void LoadTexture(const TextureRequest& request);
    void EvictTextures();
    
    std::priority_queue<TextureRequest> m_request_queue;
    std::unordered_map<std::string, uint32_t> m_loaded_textures;
    
    size_t m_memory_budget;
    std::atomic<size_t> m_current_memory{0};
    
    std::thread m_streaming_thread;
    std::atomic<bool> m_stop{false};
    std::mutex m_mutex;
    std::condition_variable m_condition;
    
    bool m_predictive_loading = false;
    float m_camera_pos[3] = {0, 0, 0};
    float m_camera_dir[3] = {0, 0, 1};
};

// GPU memory manager
class GPUMemoryManager {
public:
    enum class MemoryType {
        VERTEX_BUFFER,
        INDEX_BUFFER,
        UNIFORM_BUFFER,
        TEXTURE_2D,
        TEXTURE_3D,
        RENDER_TARGET,
        COMPUTE_BUFFER
    };
    
    struct Allocation {
        uint32_t id;
        MemoryType type;
        size_t size;
        void* mapped_ptr;
        bool is_persistent;
    };
    
    GPUMemoryManager(size_t budget_mb = 1024);
    
    Allocation* Allocate(MemoryType type, size_t size, bool persistent = false);
    void Free(Allocation* allocation);
    
    void* Map(Allocation* allocation, size_t offset = 0, size_t size = 0);
    void Unmap(Allocation* allocation);
    
    void Defragment();
    float GetFragmentation() const;
    size_t GetUsedMemory() const { return m_used_memory; }
    size_t GetAvailableMemory() const { return m_budget - m_used_memory; }
    
private:
    std::vector<std::unique_ptr<Allocation>> m_allocations;
    std::unordered_map<uint32_t, Allocation*> m_allocation_map;
    
    size_t m_budget;
    std::atomic<size_t> m_used_memory{0};
    uint32_t m_next_id = 1;
    
    mutable std::mutex m_mutex;
};

// CPU-GPU synchronization optimizer
class SyncOptimizer {
public:
    enum class SyncPoint {
        FRAME_BEGIN,
        FRAME_END,
        RENDER_PASS_BEGIN,
        RENDER_PASS_END,
        COMPUTE_DISPATCH,
        BUFFER_UPLOAD,
        TEXTURE_UPLOAD,
        READBACK
    };
    
    SyncOptimizer();
    
    void BeginFrame();
    void EndFrame();
    
    void InsertSyncPoint(SyncPoint type);
    void WaitForSync(SyncPoint type);
    
    // Triple buffering support
    void EnableTripleBuffering(bool enable) { m_triple_buffering = enable; }
    uint32_t GetCurrentFrameIndex() const { return m_current_frame % 3; }
    
    // Async transfers
    void BeginAsyncTransfer();
    void EndAsyncTransfer();
    bool IsTransferComplete() const;
    
    // Performance metrics
    float GetGPUIdleTime() const { return m_gpu_idle_time; }
    float GetCPUWaitTime() const { return m_cpu_wait_time; }
    
private:
    struct SyncObject {
        SyncPoint type;
        uint32_t fence;
        std::chrono::high_resolution_clock::time_point timestamp;
    };
    
    std::vector<SyncObject> m_sync_points;
    uint32_t m_current_frame = 0;
    
    bool m_triple_buffering = true;
    std::atomic<float> m_gpu_idle_time{0.0f};
    std::atomic<float> m_cpu_wait_time{0.0f};
    
    mutable std::mutex m_mutex;
};

} // namespace ecscope::gui::optimization