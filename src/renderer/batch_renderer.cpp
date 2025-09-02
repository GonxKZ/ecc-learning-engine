/**
 * @file renderer/batch_renderer.cpp
 * @brief Sprite Batching System Implementation for ECScope Educational ECS Engine - Phase 7: Renderizado 2D
 * 
 * This implementation provides an advanced sprite batching system designed for educational
 * clarity while achieving maximum rendering performance through modern OpenGL techniques.
 * 
 * Key Features Implemented:
 * - Intelligent sprite batching with automatic texture grouping
 * - Dynamic vertex buffer management with memory pooling
 * - Multi-strategy batching algorithms with adaptive selection
 * - Comprehensive frustum culling and spatial optimization
 * - GPU-optimized vertex layouts and attribute binding
 * 
 * Educational Focus:
 * - Detailed explanation of batching concepts and benefits
 * - Performance analysis and optimization techniques
 * - Memory management patterns for GPU resources
 * - Visual debugging with batch color-coding
 * - Interactive performance tuning for learning
 * 
 * @author ECScope Educational ECS Framework - Phase 7: Renderizado 2D
 * @date 2024
 */

#include "batch_renderer.hpp"
#include "renderer_2d.hpp"
#include "../core/log.hpp"
#include "../memory/memory_tracker.hpp"

// Platform-specific OpenGL headers
#ifdef _WIN32
    #include <windows.h>
    #include <GL/gl.h>
    #include <GL/glext.h>
#elif defined(__linux__)
    #include <GL/gl.h>
    #include <GL/glext.h>
#endif

#include <algorithm>
#include <cmath>
#include <cstring>
#include <chrono>

namespace ecscope::renderer {

//=============================================================================
// OpenGL Vertex Buffer Management Utilities
//=============================================================================

namespace gl_batch_utils {
    /** @brief Create and configure vertex array object for sprite batching */
    u32 create_batch_vao() noexcept {
        u32 vao_id = 0;
        glGenVertexArrays(1, &vao_id);
        glBindVertexArray(vao_id);
        
        core::Log::debug("Created batch VAO with ID {}", vao_id);
        return vao_id;
    }
    
    /** @brief Create vertex buffer with specified usage pattern */
    u32 create_vertex_buffer(usize size, GLenum usage = GL_DYNAMIC_DRAW) noexcept {
        u32 vbo_id = 0;
        glGenBuffers(1, &vbo_id);
        glBindBuffer(GL_ARRAY_BUFFER, vbo_id);
        glBufferData(GL_ARRAY_BUFFER, size, nullptr, usage);
        
        core::Log::debug("Created vertex buffer {} with {} bytes", vbo_id, size);
        return vbo_id;
    }
    
    /** @brief Create index buffer with specified usage pattern */
    u32 create_index_buffer(usize size, GLenum usage = GL_STATIC_DRAW) noexcept {
        u32 ibo_id = 0;
        glGenBuffers(1, &ibo_id);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo_id);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, size, nullptr, usage);
        
        core::Log::debug("Created index buffer {} with {} bytes", ibo_id, size);
        return ibo_id;
    }
    
    /** @brief Set up vertex attributes for BatchVertex structure */
    void setup_batch_vertex_attributes() noexcept {
        // Educational Note: Modern OpenGL uses vertex array objects (VAOs)
        // to store vertex attribute configurations, making rendering more efficient
        
        constexpr usize vertex_size = sizeof(BatchVertex);
        
        // Position attribute (location 0): vec2 at offset 0
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, vertex_size, 
                             reinterpret_cast<void*>(offsetof(BatchVertex, position_x)));
        glEnableVertexAttribArray(0);
        
        // Texture coordinates (location 1): vec2 at offset 8
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, vertex_size,
                             reinterpret_cast<void*>(offsetof(BatchVertex, texture_u)));
        glEnableVertexAttribArray(1);
        
        // Color (location 2): normalized unsigned byte vec4 at offset 16
        glVertexAttribPointer(2, 4, GL_UNSIGNED_BYTE, GL_TRUE, vertex_size,
                             reinterpret_cast<void*>(offsetof(BatchVertex, color_rgba)));
        glEnableVertexAttribArray(2);
        
        // Metadata (location 3): unsigned int at offset 20
        glVertexAttribIPointer(3, 1, GL_UNSIGNED_INT, vertex_size,
                              reinterpret_cast<void*>(offsetof(BatchVertex, metadata)));
        glEnableVertexAttribArray(3);
        
        core::Log::debug("Configured vertex attributes for batching (4 attributes, {} bytes per vertex)", vertex_size);
    }
    
    /** @brief Generate standard quad indices for sprite rendering */
    void generate_quad_indices(std::vector<u16>& indices, usize sprite_count) noexcept {
        indices.clear();
        indices.reserve(sprite_count * 6);
        
        // Educational Note: Each sprite quad uses 6 indices forming 2 triangles
        // This pattern allows vertex reuse, reducing memory bandwidth
        
        for (usize i = 0; i < sprite_count; ++i) {
            u16 base = static_cast<u16>(i * 4);
            
            // Triangle 1: top-left, bottom-left, top-right (CCW)
            indices.push_back(base + 0);
            indices.push_back(base + 1);
            indices.push_back(base + 2);
            
            // Triangle 2: top-right, bottom-left, bottom-right (CCW)
            indices.push_back(base + 2);
            indices.push_back(base + 1);
            indices.push_back(base + 3);
        }
    }
}

//=============================================================================
// Sprite Batch Implementation
//=============================================================================

SpriteBatch::SpriteBatch() noexcept {
    vertices_.reserve(MAX_VERTICES);
    indices_.reserve(MAX_INDICES);
}

SpriteBatch::~SpriteBatch() noexcept {
    destroy_gpu_resources();
}

SpriteBatch::SpriteBatch(SpriteBatch&& other) noexcept
    : sprite_count_(other.sprite_count_)
    , primary_texture_id_(other.primary_texture_id_)
    , material_hash_(other.material_hash_)
    , is_finalized_(other.is_finalized_)
    , vertices_(std::move(other.vertices_))
    , indices_(std::move(other.indices_))
    , vao_id_(other.vao_id_)
    , vbo_id_(other.vbo_id_)
    , ibo_id_(other.ibo_id_)
    , gpu_resources_created_(other.gpu_resources_created_)
    , stats_(other.stats_)
    , render_state_(other.render_state_)
{
    // Clear other's OpenGL resource IDs to prevent double deletion
    other.vao_id_ = 0;
    other.vbo_id_ = 0;
    other.ibo_id_ = 0;
    other.gpu_resources_created_ = false;
}

SpriteBatch& SpriteBatch::operator=(SpriteBatch&& other) noexcept {
    if (this != &other) {
        // Clean up current resources
        destroy_gpu_resources();
        
        // Move data
        sprite_count_ = other.sprite_count_;
        primary_texture_id_ = other.primary_texture_id_;
        material_hash_ = other.material_hash_;
        is_finalized_ = other.is_finalized_;
        vertices_ = std::move(other.vertices_);
        indices_ = std::move(other.indices_);
        vao_id_ = other.vao_id_;
        vbo_id_ = other.vbo_id_;
        ibo_id_ = other.ibo_id_;
        gpu_resources_created_ = other.gpu_resources_created_;
        stats_ = other.stats_;
        render_state_ = other.render_state_;
        
        // Clear other's state
        other.vao_id_ = 0;
        other.vbo_id_ = 0;
        other.ibo_id_ = 0;
        other.gpu_resources_created_ = false;
    }
    return *this;
}

bool SpriteBatch::can_add_sprite(const RenderableSprite& sprite, const Transform& transform) const noexcept {
    // Educational Note: Batch compatibility determines rendering efficiency
    // Similar sprites can be rendered together, reducing GPU state changes
    
    // Check if batch is full
    if (sprite_count_ >= MAX_SPRITES_PER_BATCH) {
        return false;
    }
    
    // For first sprite, always accept
    if (sprite_count_ == 0) {
        return true;
    }
    
    // Check texture compatibility
    if (sprite.texture.id != primary_texture_id_) {
        return false; // Different textures require separate batches
    }
    
    // Check blend mode compatibility
    if (static_cast<u8>(sprite.blend_mode) != render_state_.blend_mode) {
        return false; // Different blend modes require state changes
    }
    
    // Check Z-order constraints (must be within reasonable range for sorting)
    if (sprite.z_order < render_state_.z_order_min - 10.0f || 
        sprite.z_order > render_state_.z_order_max + 10.0f) {
        return false; // Z-order too different for efficient sorting
    }
    
    return true;
}

bool SpriteBatch::add_sprite(const RenderableSprite& sprite, const Transform& transform) noexcept {
    if (!can_add_sprite(sprite, transform)) {
        return false;
    }
    
    // Educational Note: Adding sprites to a batch involves generating vertex data
    // Each sprite becomes a quad (4 vertices) with appropriate texture coordinates
    
    // Calculate sprite world bounds
    auto sprite_size = sprite.calculate_world_size();
    f32 half_width = sprite_size.width * 0.5f;
    f32 half_height = sprite_size.height * 0.5f;
    
    // Apply pivot offset
    f32 pivot_offset_x = (sprite.pivot.x - 0.5f) * sprite_size.width;
    f32 pivot_offset_y = (sprite.pivot.y - 0.5f) * sprite_size.height;
    
    // Calculate quad vertices in world space
    f32 cos_rot = std::cos(transform.rotation);
    f32 sin_rot = std::sin(transform.rotation);
    
    // Quad corners relative to pivot
    f32 corners[4][2] = {
        {-half_width - pivot_offset_x, -half_height - pivot_offset_y}, // Top-left
        {-half_width - pivot_offset_x,  half_height - pivot_offset_y}, // Bottom-left
        { half_width - pivot_offset_x, -half_height - pivot_offset_y}, // Top-right
        { half_width - pivot_offset_x,  half_height - pivot_offset_y}  // Bottom-right
    };
    
    // UV coordinates (handle flipping)
    f32 uv_coords[4][2];
    if (sprite.render_flags.flip_horizontal) {
        uv_coords[0][0] = sprite.uv_rect.u + sprite.uv_rect.width; // Right edge
        uv_coords[2][0] = sprite.uv_rect.u; // Left edge
    } else {
        uv_coords[0][0] = sprite.uv_rect.u; // Left edge
        uv_coords[2][0] = sprite.uv_rect.u + sprite.uv_rect.width; // Right edge
    }
    uv_coords[1][0] = uv_coords[0][0];
    uv_coords[3][0] = uv_coords[2][0];
    
    if (sprite.render_flags.flip_vertical) {
        uv_coords[0][1] = sprite.uv_rect.v + sprite.uv_rect.height; // Bottom edge
        uv_coords[1][1] = sprite.uv_rect.v; // Top edge
    } else {
        uv_coords[0][1] = sprite.uv_rect.v; // Top edge
        uv_coords[1][1] = sprite.uv_rect.v + sprite.uv_rect.height; // Bottom edge
    }
    uv_coords[2][1] = uv_coords[0][1];
    uv_coords[3][1] = uv_coords[1][1];
    
    // Generate 4 vertices for the sprite quad
    for (int i = 0; i < 4; ++i) {
        BatchVertex vertex;
        
        // Apply rotation and translation
        f32 local_x = corners[i][0];
        f32 local_y = corners[i][1];
        vertex.position_x = transform.position.x + (local_x * cos_rot - local_y * sin_rot);
        vertex.position_y = transform.position.y + (local_x * sin_rot + local_y * cos_rot);
        
        // Set texture coordinates
        vertex.texture_u = uv_coords[i][0];
        vertex.texture_v = uv_coords[i][1];
        
        // Set color modulation
        vertex.color_rgba = sprite.color_modulation.rgba;
        
        // Set metadata (texture ID, blend mode, batch ID)
        vertex.set_texture_id(static_cast<u16>(sprite.texture.id));
        vertex.set_blend_mode(sprite.blend_mode);
        vertex.batch_id = static_cast<u16>(sprite_count_); // For debugging
        
        vertices_.push_back(vertex);
    }
    
    // Update batch state
    if (sprite_count_ == 0) {
        primary_texture_id_ = sprite.texture.id;
        calculate_material_hash(sprite);
        render_state_.z_order_min = sprite.z_order;
        render_state_.z_order_max = sprite.z_order;
    } else {
        render_state_.z_order_min = std::min(render_state_.z_order_min, sprite.z_order);
        render_state_.z_order_max = std::max(render_state_.z_order_max, sprite.z_order);
    }
    
    update_render_state(sprite);
    sprite_count_++;
    is_finalized_ = false;
    
    core::Log::debug("Added sprite {} to batch (total: {})", sprite_count_, sprite_count_);
    return true;
}

void SpriteBatch::reserve(usize sprite_count) noexcept {
    usize vertex_count = sprite_count * 4;
    usize index_count = sprite_count * 6;
    
    vertices_.reserve(vertex_count);
    indices_.reserve(index_count);
    
    core::Log::debug("Reserved space for {} sprites ({} vertices, {} indices)", 
                     sprite_count, vertex_count, index_count);
}

void SpriteBatch::clear() noexcept {
    sprite_count_ = 0;
    primary_texture_id_ = INVALID_TEXTURE_ID;
    material_hash_ = 0;
    is_finalized_ = false;
    
    vertices_.clear();
    indices_.clear();
    
    render_state_ = RenderState{};
    stats_ = BatchStats{};
    
    core::Log::debug("Cleared sprite batch");
}

void SpriteBatch::finalize() noexcept {
    if (is_finalized_ || sprite_count_ == 0) {
        return;
    }
    
    // Educational Note: Finalization prepares the batch for GPU rendering
    // This involves generating indices and uploading data to GPU buffers
    
    auto finalize_start = std::chrono::high_resolution_clock::now();
    
    // Generate indices for all sprites
    generate_indices();
    
    // Create GPU resources if needed
    if (!gpu_resources_created_) {
        create_gpu_resources();
    }
    
    // Upload vertex data to GPU
    upload_vertex_data();
    
    is_finalized_ = true;
    
    auto finalize_end = std::chrono::high_resolution_clock::now();
    auto finalize_time = std::chrono::duration<f32, std::milli>(finalize_end - finalize_start).count();
    
    // Update statistics
    stats_.vertex_buffer_utilization = static_cast<f32>(sprite_count_) / MAX_SPRITES_PER_BATCH;
    stats_.memory_overhead = (vertices_.capacity() * sizeof(BatchVertex) + 
                             indices_.capacity() * sizeof(u16)) / sprite_count_;
    
    core::Log::debug("Finalized batch with {} sprites in {:.3f}ms", sprite_count_, finalize_time);
}

void SpriteBatch::render(Renderer2D& renderer) const noexcept {
    if (!is_finalized_ || sprite_count_ == 0) {
        core::Log::warning("Cannot render unfinalized or empty batch");
        return;
    }
    
    // Educational Note: Batch rendering demonstrates modern OpenGL best practices
    // All sprites in the batch are rendered with a single draw call
    
    auto render_start = std::chrono::high_resolution_clock::now();
    
    // Bind vertex array object (contains all vertex attribute setup)
    glBindVertexArray(vao_id_);
    
    // Bind the primary texture for this batch
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, primary_texture_id_);
    
    // Set blend mode for the batch
    switch (render_state_.blend_mode) {
        case RenderableSprite::BlendMode::Alpha:
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            break;
        case RenderableSprite::BlendMode::Additive:
            glBlendFunc(GL_SRC_ALPHA, GL_ONE);
            break;
        case RenderableSprite::BlendMode::Multiply:
            glBlendFunc(GL_DST_COLOR, GL_ZERO);
            break;
        case RenderableSprite::BlendMode::Screen:
            glBlendFunc(GL_ONE_MINUS_DST_COLOR, GL_ONE);
            break;
        case RenderableSprite::BlendMode::Premultiplied:
            glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
            break;
    }
    
    // Execute the draw call
    glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(get_index_count()), 
                   GL_UNSIGNED_SHORT, nullptr);
    
    // Record performance metrics
    record_render();
    renderer.record_draw_call(get_vertex_count(), get_index_count());
    
    auto render_end = std::chrono::high_resolution_clock::now();
    auto render_time = std::chrono::duration<f32, std::milli>(render_end - render_start).count();
    
    // Update render statistics
    stats_.render_count++;
    stats_.total_render_time += render_time;
    stats_.average_render_time = stats_.total_render_time / stats_.render_count;
    
    core::Log::debug("Rendered batch {} sprites in {:.3f}ms", sprite_count_, render_time);
}

usize SpriteBatch::get_memory_usage() const noexcept {
    usize vertex_memory = vertices_.capacity() * sizeof(BatchVertex);
    usize index_memory = indices_.capacity() * sizeof(u16);
    usize batch_overhead = sizeof(SpriteBatch);
    
    return vertex_memory + index_memory + batch_overhead;
}

SpriteBatch::DebugInfo SpriteBatch::get_debug_info() const noexcept {
    DebugInfo info;
    
    info.description = fmt::format("Batch: {} sprites, texture {}", sprite_count_, primary_texture_id_);
    info.debug_tint = Color{
        static_cast<u8>((sprite_count_ * 137) % 255),
        static_cast<u8>((sprite_count_ * 149) % 255), 
        static_cast<u8>((sprite_count_ * 163) % 255),
        128 // Semi-transparent
    };
    
    // Calculate complexity score
    info.complexity_score = 1.0f + (sprite_count_ - 1) * 0.1f; // Base 1.0, +0.1 per additional sprite
    
    // Add optimization hints based on batch characteristics
    info.hint_count = 0;
    if (stats_.vertex_buffer_utilization < 0.5f) {
        info.optimization_hints[info.hint_count++] = "Batch is underutilized - consider smaller batch sizes";
    }
    if (stats_.texture_switches > 1) {
        info.optimization_hints[info.hint_count++] = "Multiple textures in batch - consider texture atlasing";
    }
    if (sprite_count_ == 1) {
        info.optimization_hints[info.hint_count++] = "Single sprite batch - batching not effective";
    }
    
    // Analyze sprite composition
    info.opaque_sprites = 0;
    info.transparent_sprites = 0;
    info.unique_textures = 1; // At least the primary texture
    
    for (const auto& vertex : vertices_) {
        Color vertex_color{vertex.color_rgba};
        if (vertex_color.a == 255) {
            info.opaque_sprites++;
        } else {
            info.transparent_sprites++;
        }
    }
    
    info.average_sprite_size = sprite_count_ > 0 ? 
        static_cast<f32>(vertices_.size() * sizeof(BatchVertex)) / sprite_count_ : 0.0f;
    
    return info;
}

bool SpriteBatch::validate() const noexcept {
    // Educational Note: Validation helps catch batching errors during development
    // These checks ensure the batch is in a consistent state
    
    if (sprite_count_ == 0) {
        return vertices_.empty() && indices_.empty();
    }
    
    if (vertices_.size() != sprite_count_ * 4) {
        core::Log::error("Vertex count mismatch: expected {}, got {}", 
                        sprite_count_ * 4, vertices_.size());
        return false;
    }
    
    if (is_finalized_ && indices_.size() != sprite_count_ * 6) {
        core::Log::error("Index count mismatch: expected {}, got {}", 
                        sprite_count_ * 6, indices_.size());
        return false;
    }
    
    // Validate vertex data
    for (const auto& vertex : vertices_) {
        if (!vertex.is_valid()) {
            core::Log::error("Invalid vertex data detected");
            return false;
        }
    }
    
    return true;
}

//=============================================================================
// Private SpriteBatch Methods
//=============================================================================

void SpriteBatch::create_gpu_resources() noexcept {
    if (gpu_resources_created_) {
        return;
    }
    
    // Educational Note: GPU resource creation demonstrates modern OpenGL buffer management
    // We create vertex array, vertex buffer, and index buffer objects
    
    // Create vertex array object
    vao_id_ = gl_batch_utils::create_batch_vao();
    
    // Create and configure vertex buffer
    usize vertex_buffer_size = MAX_VERTICES * sizeof(BatchVertex);
    vbo_id_ = gl_batch_utils::create_vertex_buffer(vertex_buffer_size, GL_DYNAMIC_DRAW);
    
    // Create and configure index buffer
    usize index_buffer_size = MAX_INDICES * sizeof(u16);
    ibo_id_ = gl_batch_utils::create_index_buffer(index_buffer_size, GL_STATIC_DRAW);
    
    // Set up vertex attribute pointers
    gl_batch_utils::setup_batch_vertex_attributes();
    
    // Unbind VAO to prevent accidental modification
    glBindVertexArray(0);
    
    gpu_resources_created_ = true;
    
    core::Log::info("Created GPU resources for sprite batch (VAO: {}, VBO: {}, IBO: {})", 
                    vao_id_, vbo_id_, ibo_id_);
}

void SpriteBatch::destroy_gpu_resources() noexcept {
    if (!gpu_resources_created_) {
        return;
    }
    
    // Educational Note: Proper GPU resource cleanup prevents memory leaks
    // OpenGL resources must be explicitly deleted
    
    if (vao_id_ != 0) {
        glDeleteVertexArrays(1, &vao_id_);
        vao_id_ = 0;
    }
    
    if (vbo_id_ != 0) {
        glDeleteBuffers(1, &vbo_id_);
        vbo_id_ = 0;
    }
    
    if (ibo_id_ != 0) {
        glDeleteBuffers(1, &ibo_id_);
        ibo_id_ = 0;
    }
    
    gpu_resources_created_ = false;
    
    core::Log::debug("Destroyed GPU resources for sprite batch");
}

void SpriteBatch::upload_vertex_data() const noexcept {
    if (!gpu_resources_created_ || vertices_.empty()) {
        return;
    }
    
    // Educational Note: Vertex data upload demonstrates GPU memory management
    // We use GL_DYNAMIC_DRAW because batch contents change frequently
    
    glBindBuffer(GL_ARRAY_BUFFER, vbo_id_);
    
    usize vertex_data_size = vertices_.size() * sizeof(BatchVertex);
    glBufferSubData(GL_ARRAY_BUFFER, 0, vertex_data_size, vertices_.data());
    
    // Upload index data if available
    if (!indices_.empty()) {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo_id_);
        
        usize index_data_size = indices_.size() * sizeof(u16);
        glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, index_data_size, indices_.data());
    }
    
    core::Log::debug("Uploaded {} bytes of vertex data and {} bytes of index data",
                     vertex_data_size, indices_.size() * sizeof(u16));
}

void SpriteBatch::generate_indices() noexcept {
    if (sprite_count_ == 0) {
        return;
    }
    
    // Educational Note: Index generation creates the triangle pattern for sprite quads
    // This allows efficient rendering with minimal vertex duplication
    
    gl_batch_utils::generate_quad_indices(indices_, sprite_count_);
    
    core::Log::debug("Generated {} indices for {} sprites", indices_.size(), sprite_count_);
}

void SpriteBatch::update_render_state(const RenderableSprite& sprite) noexcept {
    // Educational Note: Render state tracking helps maintain batch compatibility
    // All sprites in a batch must share compatible rendering states
    
    render_state_.blend_mode = sprite.blend_mode;
    render_state_.depth_test_enabled = sprite.render_flags.depth_test_enabled;
    render_state_.depth_write_enabled = sprite.render_flags.depth_write_enabled;
}

void SpriteBatch::calculate_material_hash(const RenderableSprite& sprite) noexcept {
    // Educational Note: Material hashing allows fast batch compatibility checking
    // Similar materials can be batched together for efficiency
    
    // Simple hash combining texture ID and blend mode
    material_hash_ = static_cast<u64>(sprite.texture.id) << 32 | 
                    static_cast<u64>(sprite.blend_mode);
    
    // Could include more material properties for more sophisticated batching
}

void SpriteBatch::record_render() const noexcept {
    // Update render statistics for educational analysis
    const_cast<BatchStats&>(stats_).render_count++;
}

//=============================================================================
// Batch Renderer Implementation
//=============================================================================

BatchRenderer::BatchRenderer(const Config& config) noexcept 
    : config_(config)
    , current_strategy_(config.strategy)
    , sprite_allocator_(1024 * 1024) // 1MB for sprite data
    , batch_allocator_(128 * sizeof(SpriteBatch), sizeof(SpriteBatch)) // Pool for 128 batches
{
    // Initialize strategy effectiveness scores
    strategy_effectiveness_.fill(0.5f); // Start with neutral scores
    
    core::Log::info("Created BatchRenderer with {} strategy", 
                    static_cast<int>(config.strategy));
}

BatchRenderer::~BatchRenderer() noexcept {
    shutdown();
}

BatchRenderer::BatchRenderer(BatchRenderer&& other) noexcept
    : config_(std::move(other.config_))
    , initialized_(other.initialized_)
    , frame_active_(other.frame_active_)
    , frame_number_(other.frame_number_)
    , submitted_sprites_(std::move(other.submitted_sprites_))
    , sprite_allocator_(std::move(other.sprite_allocator_))
    , batches_(std::move(other.batches_))
    , batch_pool_(std::move(other.batch_pool_))
    , batch_allocator_(std::move(other.batch_allocator_))
    , statistics_(other.statistics_)
    , current_strategy_(other.current_strategy_)
    , strategy_effectiveness_(other.strategy_effectiveness_)
{
    other.initialized_ = false;
    other.frame_active_ = false;
}

BatchRenderer& BatchRenderer::operator=(BatchRenderer&& other) noexcept {
    if (this != &other) {
        shutdown();
        
        config_ = std::move(other.config_);
        initialized_ = other.initialized_;
        frame_active_ = other.frame_active_;
        frame_number_ = other.frame_number_;
        submitted_sprites_ = std::move(other.submitted_sprites_);
        sprite_allocator_ = std::move(other.sprite_allocator_);
        batches_ = std::move(other.batches_);
        batch_pool_ = std::move(other.batch_pool_);
        batch_allocator_ = std::move(other.batch_allocator_);
        statistics_ = other.statistics_;
        current_strategy_ = other.current_strategy_;
        strategy_effectiveness_ = other.strategy_effectiveness_;
        
        other.initialized_ = false;
        other.frame_active_ = false;
    }
    return *this;
}

bool BatchRenderer::initialize() noexcept {
    if (initialized_) {
        return true;
    }
    
    core::Log::info("Initializing BatchRenderer...");
    
    // Pre-allocate batch objects in the pool
    try {
        batch_pool_.reserve(config_.vertex_buffer_pool_size);
        for (usize i = 0; i < 32; ++i) { // Start with 32 pre-allocated batches
            batch_pool_.push_back(std::make_unique<SpriteBatch>());
        }
    } catch (const std::exception& e) {
        core::Log::error("Failed to initialize batch pool: {}", e.what());
        return false;
    }
    
    initialized_ = true;
    core::Log::info("BatchRenderer initialized with {} pre-allocated batches", batch_pool_.size());
    
    return true;
}

void BatchRenderer::shutdown() noexcept {
    if (!initialized_) {
        return;
    }
    
    core::Log::info("Shutting down BatchRenderer...");
    
    if (frame_active_) {
        batches_.clear();
        submitted_sprites_.clear();
        frame_active_ = false;
    }
    
    // Clean up batch pool
    batch_pool_.clear();
    batches_.clear();
    
    initialized_ = false;
    core::Log::info("BatchRenderer shutdown complete");
}

void BatchRenderer::begin_frame() noexcept {
    if (!initialized_) {
        core::Log::error("Cannot begin frame - BatchRenderer not initialized");
        return;
    }
    
    if (frame_active_) {
        core::Log::warning("begin_frame() called while frame already active");
        return;
    }
    
    // Educational Note: Frame begin clears previous data and prepares for new submissions
    // This provides a clean slate for each frame's batching operations
    
    // Clear previous frame's data
    submitted_sprites_.clear();
    sprite_allocator_.reset();
    
    // Return batches to pool
    for (auto& batch : batches_) {
        if (batch) {
            batch->clear();
            batch_pool_.push_back(std::move(batch));
        }
    }
    batches_.clear();
    
    // Reset frame statistics
    statistics_.frame_number = ++frame_number_;
    statistics_.sprites_submitted = 0;
    statistics_.batches_generated = 0;
    
    frame_active_ = true;
    
    core::Log::debug("BatchRenderer frame {} started", frame_number_);
}

void BatchRenderer::end_frame() noexcept {
    if (!frame_active_) {
        core::Log::warning("end_frame() called without active frame");
        return;
    }
    
    // Educational Note: Frame end triggers batch generation and finalization
    // This is where the actual batching algorithms run
    
    auto frame_start = std::chrono::high_resolution_clock::now();
    
    // Process submitted sprites
    if (!submitted_sprites_.empty()) {
        calculate_sort_keys();
        
        if (config_.enable_frustum_culling) {
            perform_frustum_culling();
        }
        
        sort_submitted_sprites();
    }
    
    auto frame_end = std::chrono::high_resolution_clock::now();
    auto frame_time = std::chrono::duration<f32, std::milli>(frame_end - frame_start).count();
    
    statistics_.total_batching_time_ms = frame_time;
    
    frame_active_ = false;
    
    core::Log::debug("BatchRenderer frame {} completed with {} sprites in {:.3f}ms", 
                     frame_number_, submitted_sprites_.size(), frame_time);
}

void BatchRenderer::submit_sprite(const RenderableSprite& sprite, const Transform& transform) noexcept {
    if (!frame_active_) {
        core::Log::warning("Cannot submit sprite - no active frame");
        return;
    }
    
    // Educational Note: Sprite submission creates a record for batching analysis
    // The actual batching happens later during generate_batches()
    
    SubmittedSprite submitted_sprite(sprite, transform, static_cast<u32>(submitted_sprites_.size()));
    submitted_sprites_.push_back(std::move(submitted_sprite));
    statistics_.sprites_submitted++;
    
    core::Log::debug("Submitted sprite {} with texture {}", 
                     submitted_sprites_.size(), sprite.texture.id);
}

void BatchRenderer::submit_sprites(const std::vector<std::pair<RenderableSprite, Transform>>& sprites) noexcept {
    if (!frame_active_) {
        core::Log::warning("Cannot submit sprites - no active frame");
        return;
    }
    
    // Educational Note: Batch sprite submission is more efficient than individual submissions
    // It allows for better memory allocation patterns and reduces function call overhead
    
    submitted_sprites_.reserve(submitted_sprites_.size() + sprites.size());
    
    for (const auto& [sprite, transform] : sprites) {
        SubmittedSprite submitted_sprite(sprite, transform, static_cast<u32>(submitted_sprites_.size()));
        submitted_sprites_.push_back(std::move(submitted_sprite));
    }
    
    statistics_.sprites_submitted += sprites.size();
    
    core::Log::debug("Submitted {} sprites in batch", sprites.size());
}

void BatchRenderer::generate_batches() noexcept {
    if (!initialized_ || submitted_sprites_.empty()) {
        return;
    }
    
    // Educational Note: Batch generation is the core of the batching system
    // Different strategies optimize for different scenarios
    
    auto generation_start = std::chrono::high_resolution_clock::now();
    
    // Select and execute batching strategy
    switch (current_strategy_) {
        case BatchingStrategy::TextureFirst:
            generate_batches_texture_first();
            break;
        case BatchingStrategy::MaterialFirst:
            generate_batches_material_first();
            break;
        case BatchingStrategy::ZOrderPreserving:
            generate_batches_z_order_preserving();
            break;
        case BatchingStrategy::SpatialLocality:
            generate_batches_spatial_locality();
            break;
        case BatchingStrategy::AdaptiveHybrid:
            generate_batches_adaptive_hybrid();
            break;
    }
    
    // Finalize all generated batches
    for (auto& batch : batches_) {
        if (batch) {
            batch->finalize();
        }
    }
    
    auto generation_end = std::chrono::high_resolution_clock::now();
    auto generation_time = std::chrono::duration<f32, std::milli>(generation_end - generation_start).count();
    
    // Update statistics
    statistics_.batches_generated = batches_.size();
    statistics_.batch_generation_time_ms = generation_time;
    statistics_.batching_efficiency = calculate_batching_efficiency();
    
    update_statistics();
    
    core::Log::info("Generated {} batches from {} sprites in {:.3f}ms (efficiency: {:.1f}%)",
                    batches_.size(), submitted_sprites_.size(), generation_time,
                    statistics_.batching_efficiency * 100.0f);
}

void BatchRenderer::render_all(Renderer2D& renderer) noexcept {
    if (batches_.empty()) {
        return;
    }
    
    // Educational Note: Batch rendering executes all generated batches in order
    // This demonstrates the performance benefits of batching multiple sprites
    
    auto render_start = std::chrono::high_resolution_clock::now();
    
    for (usize i = 0; i < batches_.size(); ++i) {
        render_batch(i, renderer);
    }
    
    auto render_end = std::chrono::high_resolution_clock::now();
    auto render_time = std::chrono::duration<f32, std::milli>(render_end - render_start).count();
    
    core::Log::debug("Rendered {} batches in {:.3f}ms", batches_.size(), render_time);
}

void BatchRenderer::render_batch(usize batch_index, Renderer2D& renderer) noexcept {
    if (batch_index >= batches_.size() || !batches_[batch_index]) {
        core::Log::warning("Invalid batch index: {}", batch_index);
        return;
    }
    
    // Educational Note: Individual batch rendering with debug visualization
    // Color coding helps visualize how sprites are grouped into batches
    
    auto& batch = batches_[batch_index];
    
    // Apply debug visualization if enabled
    if (config_.enable_batch_visualization && renderer.is_debug_rendering_enabled()) {
        Color debug_color = get_batch_visualization_color(batch_index);
        // In a real implementation, we'd modify the shader uniform for debug coloring
        core::Log::debug("Rendering batch {} with debug color ({}, {}, {})", 
                         batch_index, debug_color.r, debug_color.g, debug_color.b);
    }
    
    batch->render(renderer);
}

f32 BatchRenderer::get_estimated_gpu_cost() const noexcept {
    // Educational Note: GPU cost estimation helps understand rendering performance
    // This provides educational insights into rendering complexity
    
    f32 total_cost = 0.0f;
    
    for (const auto& batch : batches_) {
        if (batch) {
            // Base cost per batch (draw call overhead)
            total_cost += 1.0f;
            
            // Additional cost per sprite
            total_cost += batch->get_sprite_count() * 0.1f;
            
            // Penalty for small batches (inefficient)
            if (batch->get_sprite_count() < 10) {
                total_cost += 0.5f;
            }
        }
    }
    
    return total_cost;
}

//=============================================================================
// Batch Generation Strategies
//=============================================================================

void BatchRenderer::generate_batches_texture_first() noexcept {
    // Educational Note: Texture-first batching minimizes texture binding changes
    // This strategy groups sprites by texture, then by other properties
    
    core::Log::debug("Using texture-first batching strategy");
    
    // Sort sprites by texture ID
    std::stable_sort(submitted_sprites_.begin(), submitted_sprites_.end(),
        [](const SubmittedSprite& a, const SubmittedSprite& b) {
            return a.sprite.texture.id < b.sprite.texture.id;
        });
    
    // Create batches based on texture groups
    std::unique_ptr<SpriteBatch> current_batch = nullptr;
    
    for (const auto& submitted_sprite : submitted_sprites_) {
        if (!submitted_sprite.is_visible) continue;
        
        // Try to add to current batch
        if (current_batch && current_batch->can_add_sprite(submitted_sprite.sprite, submitted_sprite.transform)) {
            current_batch->add_sprite(submitted_sprite.sprite, submitted_sprite.transform);
        } else {
            // Start new batch
            if (current_batch) {
                batches_.push_back(std::move(current_batch));
            }
            
            current_batch = acquire_batch();
            current_batch->add_sprite(submitted_sprite.sprite, submitted_sprite.transform);
        }
    }
    
    // Add final batch
    if (current_batch && current_batch->get_sprite_count() > 0) {
        batches_.push_back(std::move(current_batch));
    }
}

void BatchRenderer::generate_batches_material_first() noexcept {
    // Educational Note: Material-first batching minimizes render state changes
    // This strategy groups sprites by material properties first
    
    core::Log::debug("Using material-first batching strategy");
    
    // Sort sprites by blend mode, then by texture
    std::stable_sort(submitted_sprites_.begin(), submitted_sprites_.end(),
        [](const SubmittedSprite& a, const SubmittedSprite& b) {
            if (a.sprite.blend_mode != b.sprite.blend_mode) {
                return static_cast<u8>(a.sprite.blend_mode) < static_cast<u8>(b.sprite.blend_mode);
            }
            return a.sprite.texture.id < b.sprite.texture.id;
        });
    
    // Create batches based on material compatibility
    create_batches_from_sorted_sprites();
}

void BatchRenderer::generate_batches_z_order_preserving() noexcept {
    // Educational Note: Z-order preserving maintains correct depth sorting
    // Essential for transparent sprites that require back-to-front rendering
    
    core::Log::debug("Using Z-order preserving batching strategy");
    
    // Sort sprites by Z-order (back to front for transparency)
    std::stable_sort(submitted_sprites_.begin(), submitted_sprites_.end(),
        [](const SubmittedSprite& a, const SubmittedSprite& b) {
            return a.sprite.z_order < b.sprite.z_order;
        });
    
    // Create batches while preserving Z-order
    std::unique_ptr<SpriteBatch> current_batch = nullptr;
    
    for (const auto& submitted_sprite : submitted_sprites_) {
        if (!submitted_sprite.is_visible) continue;
        
        // Check if we can add to current batch without breaking Z-order
        bool can_batch = current_batch && 
                        current_batch->can_add_sprite(submitted_sprite.sprite, submitted_sprite.transform);
        
        if (can_batch) {
            current_batch->add_sprite(submitted_sprite.sprite, submitted_sprite.transform);
        } else {
            // Start new batch
            if (current_batch) {
                batches_.push_back(std::move(current_batch));
            }
            
            current_batch = acquire_batch();
            current_batch->add_sprite(submitted_sprite.sprite, submitted_sprite.transform);
        }
    }
    
    // Add final batch
    if (current_batch && current_batch->get_sprite_count() > 0) {
        batches_.push_back(std::move(current_batch));
    }
}

void BatchRenderer::generate_batches_spatial_locality() noexcept {
    // Educational Note: Spatial locality batching improves vertex cache utilization
    // Nearby sprites in world space are grouped together
    
    core::Log::debug("Using spatial locality batching strategy");
    
    // Sort sprites by spatial location (using a simple grid-based approach)
    std::stable_sort(submitted_sprites_.begin(), submitted_sprites_.end(),
        [](const SubmittedSprite& a, const SubmittedSprite& b) {
            // Create spatial grid keys
            constexpr f32 grid_size = 100.0f;
            i32 grid_x_a = static_cast<i32>(a.transform.position.x / grid_size);
            i32 grid_y_a = static_cast<i32>(a.transform.position.y / grid_size);
            i32 grid_x_b = static_cast<i32>(b.transform.position.x / grid_size);
            i32 grid_y_b = static_cast<i32>(b.transform.position.y / grid_size);
            
            if (grid_y_a != grid_y_b) return grid_y_a < grid_y_b;
            if (grid_x_a != grid_x_b) return grid_x_a < grid_x_b;
            
            // Secondary sort by texture for efficiency within grid cells
            return a.sprite.texture.id < b.sprite.texture.id;
        });
    
    // Create batches based on spatial groups
    create_batches_from_sorted_sprites();
}

void BatchRenderer::generate_batches_adaptive_hybrid() noexcept {
    // Educational Note: Adaptive hybrid strategy analyzes sprite distribution
    // and chooses the best strategy for current frame characteristics
    
    core::Log::debug("Using adaptive hybrid batching strategy");
    
    // Analyze current frame characteristics
    usize unique_textures = count_unique_textures();
    usize transparent_sprites = count_transparent_sprites();
    f32 spatial_spread = calculate_spatial_spread();
    
    // Choose best strategy based on analysis
    BatchingStrategy best_strategy = BatchingStrategy::TextureFirst;
    
    if (transparent_sprites > submitted_sprites_.size() / 2) {
        // Many transparent sprites - preserve Z-order
        best_strategy = BatchingStrategy::ZOrderPreserving;
    } else if (unique_textures < 5) {
        // Few textures - material batching is effective
        best_strategy = BatchingStrategy::MaterialFirst;
    } else if (spatial_spread > 1000.0f) {
        // Large world - spatial locality helps
        best_strategy = BatchingStrategy::SpatialLocality;
    } else {
        // Default to texture-first for general case
        best_strategy = BatchingStrategy::TextureFirst;
    }
    
    // Execute chosen strategy
    BatchingStrategy original_strategy = current_strategy_;
    current_strategy_ = best_strategy;
    
    switch (best_strategy) {
        case BatchingStrategy::TextureFirst:
            generate_batches_texture_first();
            break;
        case BatchingStrategy::MaterialFirst:
            generate_batches_material_first();
            break;
        case BatchingStrategy::ZOrderPreserving:
            generate_batches_z_order_preserving();
            break;
        case BatchingStrategy::SpatialLocality:
            generate_batches_spatial_locality();
            break;
        default:
            generate_batches_texture_first();
            break;
    }
    
    current_strategy_ = original_strategy;
    
    core::Log::debug("Adaptive strategy chose: {} (textures: {}, transparent: {}, spread: {:.1f})",
                     static_cast<int>(best_strategy), unique_textures, transparent_sprites, spatial_spread);
}

//=============================================================================
// Private BatchRenderer Helper Methods
//=============================================================================

void BatchRenderer::calculate_sort_keys() noexcept {
    // Educational Note: Sort key calculation affects batching efficiency
    // Different sort keys optimize for different rendering priorities
    
    for (auto& sprite : submitted_sprites_) {
        sprite.sort_key = sprite.sprite.z_order;
        // Could add more sophisticated sorting based on distance, size, etc.
    }
}

void BatchRenderer::perform_frustum_culling() noexcept {
    // Educational Note: Frustum culling removes off-screen sprites early
    // This improves performance by reducing the number of sprites to batch
    
    usize culled_count = 0;
    
    for (auto& sprite : submitted_sprites_) {
        // Simplified frustum culling - would use actual camera frustum in practice
        bool in_frustum = (sprite.transform.position.x > -2000.0f && sprite.transform.position.x < 2000.0f &&
                          sprite.transform.position.y > -2000.0f && sprite.transform.position.y < 2000.0f);
        
        if (!in_frustum) {
            sprite.is_visible = false;
            culled_count++;
        }
    }
    
    statistics_.sprites_submitted -= culled_count;
    
    core::Log::debug("Culled {} sprites outside frustum", culled_count);
}

void BatchRenderer::sort_submitted_sprites() noexcept {
    // Educational Note: Sprite sorting prepares data for efficient batching
    // The sorting algorithm depends on the current batching strategy
    
    auto sort_start = std::chrono::high_resolution_clock::now();
    
    std::stable_sort(submitted_sprites_.begin(), submitted_sprites_.end(),
        [](const SubmittedSprite& a, const SubmittedSprite& b) {
            return a.sort_key < b.sort_key;
        });
    
    auto sort_end = std::chrono::high_resolution_clock::now();
    auto sort_time = std::chrono::duration<f32, std::milli>(sort_end - sort_start).count();
    
    statistics_.sorting_time_ms = sort_time;
    
    core::Log::debug("Sorted {} sprites in {:.3f}ms", submitted_sprites_.size(), sort_time);
}

void BatchRenderer::create_batches_from_sorted_sprites() noexcept {
    // Educational Note: Generic batch creation from pre-sorted sprites
    // This is used by multiple batching strategies after sorting
    
    std::unique_ptr<SpriteBatch> current_batch = nullptr;
    
    for (const auto& submitted_sprite : submitted_sprites_) {
        if (!submitted_sprite.is_visible) continue;
        
        if (current_batch && current_batch->can_add_sprite(submitted_sprite.sprite, submitted_sprite.transform)) {
            current_batch->add_sprite(submitted_sprite.sprite, submitted_sprite.transform);
        } else {
            if (current_batch) {
                batches_.push_back(std::move(current_batch));
            }
            
            current_batch = acquire_batch();
            current_batch->add_sprite(submitted_sprite.sprite, submitted_sprite.transform);
        }
    }
    
    if (current_batch && current_batch->get_sprite_count() > 0) {
        batches_.push_back(std::move(current_batch));
    }
}

std::unique_ptr<SpriteBatch> BatchRenderer::acquire_batch() noexcept {
    // Educational Note: Batch pooling reduces memory allocation overhead
    // Reusing batch objects improves performance and reduces fragmentation
    
    if (!batch_pool_.empty()) {
        auto batch = std::move(batch_pool_.back());
        batch_pool_.pop_back();
        batch->clear(); // Ensure clean state
        return batch;
    }
    
    // Create new batch if pool is empty
    return std::make_unique<SpriteBatch>();
}

void BatchRenderer::release_batch(std::unique_ptr<SpriteBatch> batch) noexcept {
    if (batch && batch_pool_.size() < config_.vertex_buffer_pool_size) {
        batch->clear();
        batch_pool_.push_back(std::move(batch));
    }
    // If pool is full, batch will be automatically destroyed
}

void BatchRenderer::update_statistics() noexcept {
    // Educational Note: Statistics update provides comprehensive performance analysis
    // This data helps understand batching effectiveness and optimization opportunities
    
    if (submitted_sprites_.empty()) {
        statistics_.batching_efficiency = 1.0f;
        return;
    }
    
    // Calculate batching efficiency
    statistics_.batching_efficiency = calculate_batching_efficiency();
    
    // Analyze texture coherence
    statistics_.texture_coherence = calculate_texture_coherence();
    
    // Update strategy effectiveness
    analyze_strategy_effectiveness();
    
    // Generate educational insights
    generate_educational_insights();
}

f32 BatchRenderer::calculate_batching_efficiency() const noexcept {
    if (submitted_sprites_.empty() || batches_.empty()) {
        return 1.0f;
    }
    
    // Educational Note: Batching efficiency measures how well sprites are grouped
    // Higher efficiency means fewer draw calls for the same number of sprites
    
    f32 ideal_batches = 1.0f; // Best case: all sprites in one batch
    f32 actual_batches = static_cast<f32>(batches_.size());
    
    return ideal_batches / actual_batches;
}

f32 BatchRenderer::calculate_texture_coherence() const noexcept {
    // Educational Note: Texture coherence measures how well textures are grouped
    // Better coherence reduces texture binding changes
    
    usize texture_switches = 0;
    u32 last_texture_id = INVALID_TEXTURE_ID;
    
    for (const auto& batch : batches_) {
        if (batch) {
            u32 texture_id = batch->get_primary_texture();
            if (texture_id != last_texture_id && last_texture_id != INVALID_TEXTURE_ID) {
                texture_switches++;
            }
            last_texture_id = texture_id;
        }
    }
    
    if (batches_.size() <= 1) return 1.0f;
    
    f32 max_switches = static_cast<f32>(batches_.size() - 1);
    return 1.0f - (static_cast<f32>(texture_switches) / max_switches);
}

void BatchRenderer::analyze_strategy_effectiveness() noexcept {
    // Educational Note: Strategy effectiveness analysis helps adaptive batching
    // We track how well each strategy performs for different scenarios
    
    if (statistics_.batching_efficiency > 0.8f) {
        strategy_effectiveness_[static_cast<usize>(current_strategy_)] *= 1.05f; // Boost effectiveness
    } else if (statistics_.batching_efficiency < 0.5f) {
        strategy_effectiveness_[static_cast<usize>(current_strategy_)] *= 0.95f; // Reduce effectiveness
    }
    
    // Clamp effectiveness values
    for (auto& effectiveness : strategy_effectiveness_) {
        effectiveness = std::clamp(effectiveness, 0.1f, 2.0f);
    }
}

void BatchRenderer::generate_educational_insights() noexcept {
    // Educational Note: Educational insights help students understand batching
    // These provide actionable recommendations for optimization
    
    statistics_.performance_insights.clear();
    statistics_.optimization_suggestions.clear();
    
    if (statistics_.batching_efficiency < 0.5f) {
        statistics_.performance_insights.push_back(
            "Low batching efficiency detected - sprites are not grouping well");
        statistics_.optimization_suggestions.push_back(
            "Consider using texture atlases to reduce unique texture count");
    }
    
    if (batches_.size() > submitted_sprites_.size() / 4) {
        statistics_.performance_insights.push_back(
            "High batch count relative to sprite count - may indicate poor batching");
        statistics_.optimization_suggestions.push_back(
            "Try different batching strategies or adjust sprite properties");
    }
    
    if (statistics_.texture_coherence < 0.7f) {
        statistics_.performance_insights.push_back(
            "Poor texture coherence - many texture switches between batches");
        statistics_.optimization_suggestions.push_back(
            "Sort sprites by texture or use texture-first batching strategy");
    }
    
    usize small_batches = 0;
    for (const auto& batch : batches_) {
        if (batch && batch->get_sprite_count() < 5) {
            small_batches++;
        }
    }
    
    if (small_batches > batches_.size() / 2) {
        statistics_.performance_insights.push_back(
            "Many small batches detected - batching not very effective");
        statistics_.optimization_suggestions.push_back(
            "Reduce sprite variety or increase batch size limits");
    }
}

usize BatchRenderer::count_unique_textures() const noexcept {
    std::set<u32> unique_textures;
    for (const auto& sprite : submitted_sprites_) {
        unique_textures.insert(sprite.sprite.texture.id);
    }
    return unique_textures.size();
}

usize BatchRenderer::count_transparent_sprites() const noexcept {
    usize count = 0;
    for (const auto& sprite : submitted_sprites_) {
        if (sprite.sprite.uses_transparency()) {
            count++;
        }
    }
    return count;
}

f32 BatchRenderer::calculate_spatial_spread() const noexcept {
    if (submitted_sprites_.empty()) return 0.0f;
    
    f32 min_x = submitted_sprites_[0].transform.position.x;
    f32 max_x = min_x;
    f32 min_y = submitted_sprites_[0].transform.position.y;
    f32 max_y = min_y;
    
    for (const auto& sprite : submitted_sprites_) {
        min_x = std::min(min_x, sprite.transform.position.x);
        max_x = std::max(max_x, sprite.transform.position.x);
        min_y = std::min(min_y, sprite.transform.position.y);
        max_y = std::max(max_y, sprite.transform.position.y);
    }
    
    f32 width = max_x - min_x;
    f32 height = max_y - min_y;
    
    return std::sqrt(width * width + height * height);
}

Color BatchRenderer::get_batch_visualization_color(usize batch_index) const noexcept {
    // Educational Note: Debug colors help visualize batch distribution
    // Each batch gets a unique color for educational visualization
    
    static const Color colors[] = {
        Color::red(), Color::green(), Color::blue(), Color::yellow(),
        Color::cyan(), Color::magenta(), Color{255, 128, 0}, Color{128, 255, 0},
        Color{0, 255, 128}, Color{128, 0, 255}, Color{255, 0, 128}, Color{0, 128, 255}
    };
    
    return colors[batch_index % (sizeof(colors) / sizeof(colors[0]))];
}

std::string BatchRenderer::generate_batching_report() const noexcept {
    return fmt::format(
        "=== ECScope Batch Renderer Report ===\n\n"
        "Frame Statistics:\n"
        "  Frame Number: {}\n"
        "  Sprites Submitted: {}\n"
        "  Batches Generated: {}\n"
        "  Batching Efficiency: {:.1f}%\n\n"
        
        "Performance Metrics:\n"
        "  Batch Generation Time: {:.3f}ms\n"
        "  Sorting Time: {:.3f}ms\n"
        "  Total Batching Time: {:.3f}ms\n\n"
        
        "Quality Analysis:\n"
        "  Texture Coherence: {:.1f}%\n"
        "  Spatial Coherence: {:.1f}%\n"
        "  Active Strategy: {}\n"
        "  Strategy Effectiveness: {}\n\n"
        
        "Memory Usage:\n"
        "  Vertex Buffer Memory: {:.2f} MB\n"
        "  Index Buffer Memory: {:.2f} MB\n"
        "  Batch Metadata: {:.2f} KB\n"
        "  Total Batching Memory: {:.2f} MB\n\n"
        
        "Educational Insights:\n"
        "  Bottleneck Analysis: {}\n",
        
        statistics_.frame_number,
        statistics_.sprites_submitted,
        statistics_.batches_generated,
        statistics_.batching_efficiency * 100.0f,
        
        statistics_.batch_generation_time_ms,
        statistics_.sorting_time_ms,
        statistics_.total_batching_time_ms,
        
        statistics_.texture_coherence * 100.0f,
        statistics_.spatial_coherence * 100.0f,
        static_cast<int>(statistics_.active_strategy),
        statistics_.strategy_effectiveness,
        
        statistics_.vertex_buffer_memory / (1024.0f * 1024.0f),
        statistics_.index_buffer_memory / (1024.0f * 1024.0f),
        statistics_.batch_metadata_memory / 1024.0f,
        statistics_.total_batching_memory / (1024.0f * 1024.0f),
        
        statistics_.bottleneck_analysis
    );
}

BatchRenderer::MemoryBreakdown BatchRenderer::get_memory_breakdown() const noexcept {
    MemoryBreakdown breakdown;
    
    for (const auto& batch : batches_) {
        if (batch) {
            usize batch_memory = batch->get_memory_usage();
            breakdown.vertex_data += batch->get_vertex_count() * sizeof(BatchVertex);
            breakdown.index_data += batch->get_index_count() * sizeof(u16);
            breakdown.batch_metadata += sizeof(SpriteBatch);
        }
    }
    
    breakdown.gpu_buffers = breakdown.vertex_data + breakdown.index_data;
    breakdown.total = breakdown.vertex_data + breakdown.index_data + breakdown.batch_metadata;
    
    return breakdown;
}

//=============================================================================
// Batching Utilities Implementation
//=============================================================================

namespace batching_utils {
    
usize calculate_optimal_batch_size(usize sprite_count, usize memory_limit) noexcept {
    // Educational Note: Optimal batch size balances memory usage with performance
    // Larger batches reduce draw calls but increase memory requirements
    
    constexpr usize bytes_per_sprite = sizeof(BatchVertex) * 4 + sizeof(u16) * 6;
    usize max_sprites_by_memory = memory_limit / bytes_per_sprite;
    
    // Cap at reasonable maximum for performance
    constexpr usize max_sprites_per_batch = 1000;
    usize optimal_size = std::min({sprite_count, max_sprites_by_memory, max_sprites_per_batch});
    
    return std::max(optimal_size, 1UL); // At least 1 sprite per batch
}

f32 estimate_batching_overhead(usize sprite_count, usize batch_count) noexcept {
    if (sprite_count == 0 || batch_count == 0) return 0.0f;
    
    // Educational Note: Batching overhead includes CPU processing and GPU state changes
    // More batches = more overhead, but better granular control
    
    f32 cpu_overhead = batch_count * 0.1f; // 0.1ms per batch for CPU processing
    f32 gpu_overhead = batch_count * 0.05f; // 0.05ms per batch for GPU state changes
    
    return cpu_overhead + gpu_overhead;
}

f32 calculate_texture_coherence(const std::vector<TextureID>& texture_sequence) noexcept {
    if (texture_sequence.size() <= 1) return 1.0f;
    
    // Educational Note: Texture coherence measures texture binding locality
    // Sequential same textures = good coherence, random switching = poor coherence
    
    usize switches = 0;
    for (usize i = 1; i < texture_sequence.size(); ++i) {
        if (texture_sequence[i] != texture_sequence[i-1]) {
            switches++;
        }
    }
    
    f32 max_switches = static_cast<f32>(texture_sequence.size() - 1);
    return 1.0f - (static_cast<f32>(switches) / max_switches);
}

BatchingStrategy analyze_optimal_strategy(const std::vector<RenderableSprite>& sprites) noexcept {
    if (sprites.empty()) return BatchingStrategy::TextureFirst;
    
    // Educational Note: Strategy analysis chooses the best batching approach
    // based on sprite characteristics and distribution
    
    // Count unique textures
    std::set<u32> unique_textures;
    usize transparent_count = 0;
    
    for (const auto& sprite : sprites) {
        unique_textures.insert(sprite.texture.id);
        if (sprite.uses_transparency()) {
            transparent_count++;
        }
    }
    
    f32 transparency_ratio = static_cast<f32>(transparent_count) / sprites.size();
    
    // Decision logic
    if (transparency_ratio > 0.5f) {
        return BatchingStrategy::ZOrderPreserving; // Preserve depth order
    } else if (unique_textures.size() < 5) {
        return BatchingStrategy::MaterialFirst; // Few textures, optimize materials
    } else if (unique_textures.size() > sprites.size() / 2) {
        return BatchingStrategy::SpatialLocality; // Many textures, use spatial grouping
    } else {
        return BatchingStrategy::TextureFirst; // General case
    }
}

std::string generate_batch_debug_name(const SpriteBatch& batch) noexcept {
    return fmt::format("Batch_{}_tex{}_sprites{}", 
                      &batch, batch.get_primary_texture(), batch.get_sprite_count());
}

f32 calculate_vertex_cache_utilization(const std::vector<u16>& indices) noexcept {
    if (indices.empty()) return 1.0f;
    
    // Educational Note: Vertex cache utilization measures how efficiently
    // the GPU vertex cache is used by the index pattern
    
    constexpr usize cache_size = 32; // Typical GPU vertex cache size
    std::vector<u16> cache;
    usize cache_hits = 0;
    
    for (u16 index : indices) {
        // Check if vertex is in cache
        auto it = std::find(cache.begin(), cache.end(), index);
        if (it != cache.end()) {
            cache_hits++;
            // Move to front (LRU)
            cache.erase(it);
            cache.insert(cache.begin(), index);
        } else {
            // Add to front of cache
            cache.insert(cache.begin(), index);
            if (cache.size() > cache_size) {
                cache.pop_back();
            }
        }
    }
    
    return static_cast<f32>(cache_hits) / indices.size();
}

} // namespace batching_utils

} // namespace ecscope::renderer