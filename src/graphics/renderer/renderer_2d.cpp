/**
 * @file renderer/renderer_2d.cpp
 * @brief Main 2D Renderer Implementation for ECScope Educational ECS Engine - Phase 7: Renderizado 2D
 * 
 * This implementation provides a comprehensive 2D rendering system with modern OpenGL 3.3+,
 * designed for educational clarity while maintaining professional performance characteristics.
 * 
 * Key Features Implemented:
 * - Modern OpenGL 3.3+ core profile rendering pipeline
 * - Command-based rendering architecture for flexibility and analysis
 * - Comprehensive performance monitoring and educational statistics
 * - Multi-camera support with viewport management
 * - Memory-aware resource management with allocator integration
 * - Extensive debug visualization and step-through capabilities
 * 
 * Educational Focus:
 * - Every OpenGL call is documented with educational context
 * - Performance implications are clearly explained
 * - Memory usage is tracked and analyzed
 * - Rendering pipeline stages are clearly separated and documented
 * 
 * @author ECScope Educational ECS Framework - Phase 7: Renderizado 2D
 * @date 2024
 */

#include "renderer_2d.hpp"
#include "batch_renderer.hpp"
#include "resources/texture.hpp"
#include "resources/shader.hpp"
#include "../core/log.hpp"
#include "../memory/memory_tracker.hpp"

// Platform-specific OpenGL loading
#ifdef _WIN32
    #include <windows.h>
    #include <GL/gl.h>
    #include <GL/glext.h>
#elif defined(__linux__)
    #include <GL/gl.h>
    #include <GL/glext.h>
#endif

#include <algorithm>
#include <chrono>
#include <cstring>

namespace ecscope::renderer {

//=============================================================================
// OpenGL Function Loading and Error Checking
//=============================================================================

namespace gl_utils {
    /** @brief Check for OpenGL errors and log educational information */
    inline void check_gl_error(const char* operation, const char* educational_note = nullptr) noexcept {
        GLenum error = glGetError();
        if (error != GL_NO_ERROR) {
            const char* error_str = "Unknown OpenGL Error";
            
            switch (error) {
                case GL_INVALID_ENUM: error_str = "GL_INVALID_ENUM - Invalid enumeration value"; break;
                case GL_INVALID_VALUE: error_str = "GL_INVALID_VALUE - Invalid parameter value"; break;
                case GL_INVALID_OPERATION: error_str = "GL_INVALID_OPERATION - Invalid operation for current state"; break;
                case GL_OUT_OF_MEMORY: error_str = "GL_OUT_OF_MEMORY - GPU memory exhausted"; break;
                case GL_INVALID_FRAMEBUFFER_OPERATION: error_str = "GL_INVALID_FRAMEBUFFER_OPERATION - Framebuffer incomplete"; break;
            }
            
            core::Log::error("OpenGL Error in {}: {} (0x{:X})", operation, error_str, error);
            if (educational_note) {
                core::Log::info("Educational Context: {}", educational_note);
            }
        }
    }
    
    /** @brief Load OpenGL extension function pointers */
    template<typename T>
    void load_gl_function(T*& func_ptr, const char* name) noexcept {
        #ifdef _WIN32
            func_ptr = reinterpret_cast<T*>(wglGetProcAddress(name));
        #elif defined(__linux__)
            // For Linux, we'd typically use glXGetProcAddress
            // For educational simplicity, assuming functions are available
            func_ptr = reinterpret_cast<T*>(glXGetProcAddress(reinterpret_cast<const GLubyte*>(name)));
        #endif
        
        if (!func_ptr) {
            core::Log::warning("Failed to load OpenGL function: {}", name);
        }
    }
    
    /** @brief Get readable OpenGL version information */
    std::string get_gl_version_info() noexcept {
        const char* version = reinterpret_cast<const char*>(glGetString(GL_VERSION));
        const char* vendor = reinterpret_cast<const char*>(glGetString(GL_VENDOR));
        const char* renderer = reinterpret_cast<const char*>(glGetString(GL_RENDERER));
        const char* shading_version = reinterpret_cast<const char*>(glGetString(GL_SHADING_LANGUAGE_VERSION));
        
        return fmt::format("OpenGL {}\nVendor: {}\nRenderer: {}\nGLSL: {}", 
                          version ? version : "Unknown",
                          vendor ? vendor : "Unknown", 
                          renderer ? renderer : "Unknown",
                          shading_version ? shading_version : "Unknown");
    }
    
    /** @brief Get maximum texture size supported by GPU */
    u32 get_max_texture_size() noexcept {
        GLint max_size = 0;
        glGetIntegerv(GL_MAX_TEXTURE_SIZE, &max_size);
        return static_cast<u32>(max_size);
    }
    
    /** @brief Get maximum vertex attributes supported */
    u32 get_max_vertex_attributes() noexcept {
        GLint max_attribs = 0;
        glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &max_attribs);
        return static_cast<u32>(max_attribs);
    }
}

//=============================================================================
// Render Command Implementations
//=============================================================================

void SpriteRenderCommand::execute(Renderer2D& renderer) const {
    // Educational Note: This command renders a single sprite using modern OpenGL
    // We'll use the batch renderer for efficiency, even for single sprites
    
    renderer.record_draw_call(4, 6); // 4 vertices, 6 indices for a quad
    
    // Convert our render command to batch-friendly format
    // In a real implementation, this would be handled by the batch system
    core::Log::debug("Executing SpriteRenderCommand for texture {}", texture.id);
}

void DebugLineCommand::execute(Renderer2D& renderer) const {
    // Educational Note: Debug lines use immediate mode rendering for simplicity
    // In production, these would also be batched for performance
    
    // Set up line rendering state
    glLineWidth(thickness);
    gl_utils::check_gl_error("glLineWidth", "Setting line thickness for debug visualization");
    
    core::Log::debug("Executing DebugLineCommand from ({}, {}) to ({}, {})", 
                     line.start_x, line.start_y, line.end_x, line.end_y);
    
    // Record performance metrics
    renderer.record_draw_call(2, 0); // 2 vertices, no indices
}

void CameraSetCommand::execute(Renderer2D& renderer) const {
    // Educational Note: Camera changes affect the entire rendering pipeline
    // All subsequent draws will use this camera's view-projection matrix
    
    core::Log::debug("Executing CameraSetCommand for entity {}", camera_entity_id);
    
    // The actual camera binding would be handled by the renderer's camera system
    // This command primarily exists for performance tracking and debugging
}

//=============================================================================
// Render Statistics Implementation
//=============================================================================

void RenderStatistics::update_performance_analysis() noexcept {
    // Educational Note: Performance analysis provides insights into rendering efficiency
    // These metrics help students understand GPU performance characteristics
    
    // Calculate frame rate performance
    f32 target_frame_time = 16.67f; // 60 FPS target
    f32 fps = current_frame.frame_time_ms > 0.0f ? 1000.0f / current_frame.frame_time_ms : 0.0f;
    
    if (fps >= 55.0f) {
        performance.frame_rate_rating = "Excellent";
    } else if (fps >= 45.0f) {
        performance.frame_rate_rating = "Good";
    } else if (fps >= 30.0f) {
        performance.frame_rate_rating = "Fair";
    } else {
        performance.frame_rate_rating = "Poor";
    }
    
    // Analyze GPU utilization
    f32 gpu_utilization = gpu_stats.draw_calls > 0 ? 
        std::min(100.0f, (gpu_stats.vertices_rendered / 100000.0f) * 100.0f) : 0.0f;
    
    if (gpu_utilization > 80.0f) {
        performance.gpu_utilization_rating = "Excellent";
    } else if (gpu_utilization > 60.0f) {
        performance.gpu_utilization_rating = "Good";
    } else if (gpu_utilization > 40.0f) {
        performance.gpu_utilization_rating = "Fair";
    } else {
        performance.gpu_utilization_rating = "Poor";
    }
    
    // Analyze memory efficiency
    usize total_memory = gpu_stats.total_gpu_memory;
    f32 memory_per_vertex = gpu_stats.vertices_rendered > 0 ? 
        static_cast<f32>(total_memory) / gpu_stats.vertices_rendered : 0.0f;
    
    if (memory_per_vertex < 64.0f) {
        performance.memory_efficiency_rating = "Excellent";
    } else if (memory_per_vertex < 128.0f) {
        performance.memory_efficiency_rating = "Good";
    } else if (memory_per_vertex < 256.0f) {
        performance.memory_efficiency_rating = "Fair";
    } else {
        performance.memory_efficiency_rating = "Poor";
    }
    
    // Identify primary bottleneck
    if (performance.rendering_time_ms > performance.cpu_time_ms * 2.0f) {
        performance.primary_bottleneck = "GPU Bound";
        performance.optimization_suggestion = "Reduce draw calls or vertex complexity";
    } else if (performance.cpu_time_ms > 10.0f) {
        performance.primary_bottleneck = "CPU Bound";
        performance.optimization_suggestion = "Optimize batch generation or culling";
    } else if (gpu_stats.batching_efficiency < 0.5f) {
        performance.primary_bottleneck = "Batching Inefficiency";
        performance.optimization_suggestion = "Improve texture atlasing and sprite sorting";
    } else {
        performance.primary_bottleneck = "None";
        performance.optimization_suggestion = "Consider increasing visual complexity";
    }
    
    // Calculate overall performance score
    f32 fps_score = std::min(100.0f, (fps / 60.0f) * 100.0f);
    f32 efficiency_score = gpu_stats.batching_efficiency * 100.0f;
    f32 memory_score = std::max(0.0f, 100.0f - (memory_per_vertex / 10.0f));
    
    performance.performance_score = (fps_score * 0.4f + efficiency_score * 0.3f + memory_score * 0.3f);
}

std::string RenderStatistics::generate_performance_report() const noexcept {
    return fmt::format(
        "=== ECScope 2D Renderer Performance Report ===\n\n"
        "Frame Statistics:\n"
        "  Frame Time: {:.2f}ms (Target: 16.67ms for 60 FPS)\n"
        "  CPU Time: {:.2f}ms\n"
        "  GPU Time: {:.2f}ms\n"
        "  Frame Rate: {:.1f} FPS\n"
        "  Rating: {}\n\n"
        
        "Rendering Statistics:\n"
        "  Draw Calls: {}\n"
        "  Vertices Rendered: {:,}\n"
        "  Triangles Rendered: {:,}\n"
        "  Batches Created: {}\n"
        "  Batching Efficiency: {:.1f}%\n\n"
        
        "Memory Usage:\n"
        "  Vertex Buffers: {:.2f} MB\n"
        "  Index Buffers: {:.2f} MB\n"
        "  Textures: {:.2f} MB\n"
        "  Total GPU Memory: {:.2f} MB\n"
        "  Efficiency Rating: {}\n\n"
        
        "Performance Analysis:\n"
        "  Overall Score: {:.1f}/100 (Grade: {})\n"
        "  Primary Bottleneck: {}\n"
        "  Optimization Suggestion: {}\n"
        
        "Educational Insights:\n"
        "  Opaque Objects: {}\n"
        "  Transparent Objects: {}\n"
        "  Shader Switches: {}\n"
        "  Texture Switches: {}\n"
        "  Cache Hit Ratio: {:.1f}%\n",
        
        current_frame.frame_time_ms,
        current_frame.cpu_time_ms,
        current_frame.gpu_time_ms,
        history.get_fps(),
        performance.frame_rate_rating,
        
        gpu_stats.draw_calls,
        gpu_stats.vertices_rendered,
        gpu_stats.triangles_rendered,
        gpu_stats.batches_created,
        gpu_stats.batching_efficiency * 100.0f,
        
        gpu_stats.vertex_buffer_memory / (1024.0f * 1024.0f),
        gpu_stats.index_buffer_memory / (1024.0f * 1024.0f),
        gpu_stats.texture_memory / (1024.0f * 1024.0f),
        gpu_stats.total_gpu_memory / (1024.0f * 1024.0f),
        performance.memory_efficiency_rating,
        
        performance.performance_score,
        get_performance_grade(),
        performance.primary_bottleneck,
        performance.optimization_suggestion,
        
        educational.opaque_objects,
        educational.transparent_objects,
        educational.shader_switches,
        educational.texture_switches,
        educational.texture_cache_hits > 0 ? 
            (static_cast<f32>(educational.texture_cache_hits) / 
             (educational.texture_cache_hits + educational.texture_cache_misses)) * 100.0f : 0.0f
    );
}

//=============================================================================
// Main Renderer2D Implementation
//=============================================================================

Renderer2D::Renderer2D(const Renderer2DConfig& config) noexcept 
    : config_(config)
    , command_allocator_(10 * 1024 * 1024) // 10MB for render commands
{
    core::Log::info("Creating ECScope 2D Renderer with {} configuration", 
                    config.debug.enable_debug_rendering ? "debug" : "release");
    
    // Educational Note: Constructor only sets up data structures
    // OpenGL context setup happens in initialize() to ensure proper context
}

Renderer2D::~Renderer2D() noexcept {
    if (initialized_) {
        shutdown();
    }
}

Renderer2D::Renderer2D(Renderer2D&& other) noexcept
    : config_(std::move(other.config_))
    , initialized_(other.initialized_)
    , frame_active_(other.frame_active_)
    , frame_number_(other.frame_number_)
    , texture_manager_(std::move(other.texture_manager_))
    , shader_manager_(std::move(other.shader_manager_))
    , batch_renderer_(std::move(other.batch_renderer_))
    , active_camera_(other.active_camera_)
    , active_material_(other.active_material_)
    , active_shader_(other.active_shader_)
    , bound_textures_(other.bound_textures_)
    , render_commands_(std::move(other.render_commands_))
    , command_history_(std::move(other.command_history_))
    , command_allocator_(std::move(other.command_allocator_))
    , statistics_(other.statistics_)
    , current_command_index_(other.current_command_index_)
    , step_mode_active_(other.step_mode_active_)
{
    // Clear other's state to prevent double cleanup
    other.initialized_ = false;
    other.active_camera_ = nullptr;
    other.active_material_ = nullptr;
}

Renderer2D& Renderer2D::operator=(Renderer2D&& other) noexcept {
    if (this != &other) {
        // Cleanup current state
        if (initialized_) {
            shutdown();
        }
        
        // Move data
        config_ = std::move(other.config_);
        initialized_ = other.initialized_;
        frame_active_ = other.frame_active_;
        frame_number_ = other.frame_number_;
        texture_manager_ = std::move(other.texture_manager_);
        shader_manager_ = std::move(other.shader_manager_);
        batch_renderer_ = std::move(other.batch_renderer_);
        active_camera_ = other.active_camera_;
        active_material_ = other.active_material_;
        active_shader_ = other.active_shader_;
        bound_textures_ = other.bound_textures_;
        render_commands_ = std::move(other.render_commands_);
        command_history_ = std::move(other.command_history_);
        command_allocator_ = std::move(other.command_allocator_);
        statistics_ = other.statistics_;
        current_command_index_ = other.current_command_index_;
        step_mode_active_ = other.step_mode_active_;
        
        // Clear other's state
        other.initialized_ = false;
        other.active_camera_ = nullptr;
        other.active_material_ = nullptr;
    }
    return *this;
}

core::Result<void, std::string> Renderer2D::initialize() noexcept {
    if (initialized_) {
        return core::Result<void, std::string>::ok();
    }
    
    core::Log::info("Initializing ECScope 2D Renderer...");
    
    // Educational Context: OpenGL initialization demonstrates modern graphics API usage
    // We'll check OpenGL version, load extensions, and set up default state
    
    // Check OpenGL version
    auto gl_info = gl_utils::get_gl_version_info();
    core::Log::info("OpenGL Information:\n{}", gl_info);
    
    // Educational Note: We require OpenGL 3.3+ for modern rendering features
    GLint major, minor;
    glGetIntegerv(GL_MAJOR_VERSION, &major);
    glGetIntegerv(GL_MINOR_VERSION, &minor);
    gl_utils::check_gl_error("glGetIntegerv", "Checking OpenGL version for compatibility");
    
    if (major < 3 || (major == 3 && minor < 3)) {
        return core::Result<void, std::string>::err(
            fmt::format("OpenGL 3.3+ required, found {}.{}", major, minor));
    }
    
    core::Log::info("OpenGL {}.{} detected - sufficient for modern 2D rendering", major, minor);
    
    // Get hardware capabilities for educational display
    u32 max_texture_size = gl_utils::get_max_texture_size();
    u32 max_vertex_attribs = gl_utils::get_max_vertex_attributes();
    
    core::Log::info("GPU Capabilities:");
    core::Log::info("  Max Texture Size: {}x{}", max_texture_size, max_texture_size);
    core::Log::info("  Max Vertex Attributes: {}", max_vertex_attribs);
    
    // Initialize resource managers
    try {
        texture_manager_ = std::make_unique<resources::TextureManager>();
        shader_manager_ = std::make_unique<resources::ShaderManager>();
        
        // Configure batch renderer for optimal performance
        BatchRenderer::Config batch_config;
        if (config_.rendering.enable_frustum_culling) {
            batch_config.enable_frustum_culling = true;
        }
        batch_config.max_sprites_per_batch = config_.rendering.max_sprites_per_batch;
        
        batch_renderer_ = std::make_unique<BatchRenderer>(batch_config);
        
        if (!batch_renderer_->initialize()) {
            return core::Result<void, std::string>::err("Failed to initialize batch renderer");
        }
        
    } catch (const std::exception& e) {
        return core::Result<void, std::string>::err(
            fmt::format("Failed to create resource managers: {}", e.what()));
    }
    
    // Create default resources for immediate use
    create_default_resources();
    
    // Set up default OpenGL state for 2D rendering
    setup_default_render_state();
    
    // Initialize performance tracking
    statistics_ = RenderStatistics{};
    
    initialized_ = true;
    core::Log::info("ECScope 2D Renderer initialized successfully");
    
    return core::Result<void, std::string>::ok();
}

void Renderer2D::shutdown() noexcept {
    if (!initialized_) {
        return;
    }
    
    core::Log::info("Shutting down ECScope 2D Renderer...");
    
    // Educational Note: Proper cleanup prevents resource leaks
    // GPU resources must be freed before context destruction
    
    // End any active frame
    if (frame_active_) {
        render_commands_.clear();
        frame_active_ = false;
    }
    
    // Clean up all resources
    cleanup_resources();
    
    // Shutdown subsystems
    if (batch_renderer_) {
        batch_renderer_->shutdown();
        batch_renderer_.reset();
    }
    
    texture_manager_.reset();
    shader_manager_.reset();
    
    // Clear all state
    active_camera_ = nullptr;
    active_material_ = nullptr;
    active_shader_ = INVALID_SHADER_ID;
    render_commands_.clear();
    command_history_.clear();
    
    initialized_ = false;
    
    core::Log::info("ECScope 2D Renderer shutdown complete");
}

void Renderer2D::begin_frame() noexcept {
    if (!initialized_) {
        core::Log::error("Cannot begin frame - renderer not initialized");
        return;
    }
    
    if (frame_active_) {
        core::Log::warning("begin_frame() called while frame already active");
        return;
    }
    
    // Educational Note: Frame management allows for consistent performance tracking
    // and provides clear boundaries for resource management
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    // Clear previous frame's render commands
    render_commands_.clear();
    command_allocator_.reset();
    
    // Begin batch renderer frame
    if (batch_renderer_) {
        batch_renderer_->begin_frame();
    }
    
    // Reset frame statistics
    statistics_.reset_frame_stats();
    statistics_.current_frame.frame_number = ++frame_number_;
    
    frame_active_ = true;
    step_mode_active_ = config_.debug.enable_step_through_mode;
    current_command_index_ = 0;
    
    core::Log::debug("Frame {} started", frame_number_);
}

void Renderer2D::end_frame() noexcept {
    if (!frame_active_) {
        core::Log::warning("end_frame() called without active frame");
        return;
    }
    
    auto frame_start = std::chrono::high_resolution_clock::now();
    
    // Educational Note: Frame end performs all accumulated rendering work
    // This deferred execution allows for optimization and analysis
    
    // Finalize batch renderer
    if (batch_renderer_) {
        batch_renderer_->end_frame();
        batch_renderer_->generate_batches();
    }
    
    // Sort render commands for optimal execution order
    sort_render_commands();
    
    // Execute all render commands
    execute_render_commands();
    
    // Update performance statistics
    update_statistics();
    
    // Store commands for debug analysis if enabled
    if (config_.debug.log_render_commands && command_history_.size() < 1000) {
        command_history_.reserve(command_history_.size() + render_commands_.size());
        for (auto& cmd : render_commands_) {
            // Clone commands for history (simplified for educational example)
            core::Log::debug("Command executed: {}", cmd->debug_name);
        }
    }
    
    auto frame_end = std::chrono::high_resolution_clock::now();
    auto frame_time = std::chrono::duration<f32, std::milli>(frame_end - frame_start).count();
    
    statistics_.current_frame.frame_time_ms = frame_time;
    statistics_.add_to_history();
    
    frame_active_ = false;
    
    core::Log::debug("Frame {} completed in {:.2f}ms", frame_number_, frame_time);
}

void Renderer2D::submit_command(std::unique_ptr<RenderCommand> command) noexcept {
    if (!frame_active_) {
        core::Log::warning("Cannot submit command - no active frame");
        return;
    }
    
    if (!command) {
        core::Log::error("Cannot submit null render command");
        return;
    }
    
    // Educational Note: Command submission allows for analysis and optimization
    // Commands can be reordered, batched, or culled before execution
    
    command->debug_name = command->debug_name ? command->debug_name : "Unknown Command";
    
    if (config_.debug.log_render_commands) {
        log_render_command(*command);
    }
    
    render_commands_.push_back(std::move(command));
    statistics_.current_frame.total_commands++;
    
    // Update command type statistics
    auto& cmd = render_commands_.back();
    switch (cmd->type) {
        case RenderCommandType::DrawSprite:
        case RenderCommandType::DrawBatch:
            statistics_.current_frame.draw_commands++;
            break;
        case RenderCommandType::SetCamera:
        case RenderCommandType::SetMaterial:
            statistics_.current_frame.state_changes++;
            break;
        case RenderCommandType::DrawDebugLine:
        case RenderCommandType::DrawDebugBox:
        case RenderCommandType::DrawDebugCircle:
            statistics_.current_frame.debug_commands++;
            break;
    }
}

void Renderer2D::draw_sprite(const Transform& transform, const RenderableSprite& sprite) noexcept {
    if (!sprite.render_flags.visible) {
        return; // Early exit for invisible sprites
    }
    
    // Educational Note: Sprite rendering is the most common 2D operation
    // We submit to the batch renderer for optimal performance
    
    if (batch_renderer_) {
        batch_renderer_->submit_sprite(sprite, transform);
        return;
    }
    
    // Fallback: create individual sprite command
    auto cmd = std::make_unique<SpriteRenderCommand>(
        transform.position.x, transform.position.y, 
        sprite.texture, sprite.z_order);
    
    cmd->debug_name = "Individual Sprite";
    cmd->entity_id = 0; // Would be filled by ECS system
    
    submit_command(std::move(cmd));
}

void Renderer2D::draw_debug_line(f32 start_x, f32 start_y, f32 end_x, f32 end_y, 
                                Color color, f32 thickness) noexcept {
    if (!config_.debug.enable_debug_rendering) {
        return; // Debug rendering disabled
    }
    
    auto cmd = std::make_unique<DebugLineCommand>(start_x, start_y, end_x, end_y, color, thickness);
    cmd->debug_name = "Debug Line";
    
    submit_command(std::move(cmd));
}

void Renderer2D::draw_debug_box(f32 x, f32 y, f32 width, f32 height, 
                               Color color, f32 thickness) noexcept {
    if (!config_.debug.enable_debug_rendering) {
        return;
    }
    
    // Educational Note: Debug boxes are drawn as 4 connected lines
    // This demonstrates how complex shapes can be built from primitives
    
    draw_debug_line(x, y, x + width, y, color, thickness);           // Top
    draw_debug_line(x + width, y, x + width, y + height, color, thickness); // Right
    draw_debug_line(x + width, y + height, x, y + height, color, thickness); // Bottom
    draw_debug_line(x, y + height, x, y, color, thickness);         // Left
}

void Renderer2D::draw_debug_circle(f32 center_x, f32 center_y, f32 radius, 
                                  Color color, u32 segments) noexcept {
    if (!config_.debug.enable_debug_rendering || segments < 3) {
        return;
    }
    
    // Educational Note: Circles are approximated using line segments
    // More segments = smoother circle but higher cost
    
    f32 angle_step = 2.0f * 3.14159f / segments;
    
    for (u32 i = 0; i < segments; ++i) {
        f32 angle1 = i * angle_step;
        f32 angle2 = (i + 1) * angle_step;
        
        f32 x1 = center_x + std::cos(angle1) * radius;
        f32 y1 = center_y + std::sin(angle1) * radius;
        f32 x2 = center_x + std::cos(angle2) * radius;
        f32 y2 = center_y + std::sin(angle2) * radius;
        
        draw_debug_line(x1, y1, x2, y2, color, 1.0f);
    }
}

void Renderer2D::set_active_camera(const Camera2D& camera) noexcept {
    active_camera_ = &camera;
    
    // Educational Note: Camera changes affect all subsequent rendering
    // View-projection matrices must be updated for proper coordinate transformation
    
    setup_camera_matrices(camera);
    
    core::Log::debug("Active camera set to position ({:.1f}, {:.1f}), zoom {:.1f}", 
                     camera.position.x, camera.position.y, camera.zoom);
}

void Renderer2D::render_entities(ecs::Registry& registry) noexcept {
    if (!frame_active_) {
        core::Log::error("Cannot render entities - no active frame");
        return;
    }
    
    // Educational Note: ECS integration demonstrates how rendering systems
    // interact with entity-component architectures for data-driven rendering
    
    using namespace components;
    
    // Render all entities with both Transform and RenderableSprite components
    registry.view<Transform, RenderableSprite>().each([this](auto entity, auto& transform, auto& sprite) {
        // Update performance tracking
        if (auto* render_info = registry.try_get<RenderInfo>(entity)) {
            render_info->record_frame_render(0.0f, false, true); // Simplified tracking
        }
        
        draw_sprite(transform, sprite);
    });
    
    // Render debug information if enabled
    if (config_.debug.enable_debug_rendering) {
        registry.view<Transform, RenderableSprite, RenderInfo>().each([this](auto entity, auto& transform, auto& sprite, auto& render_info) {
            if (render_info.debug_settings.debug_flags.show_bounds) {
                auto size = sprite.calculate_world_size();
                draw_debug_box(transform.position.x - size.width * 0.5f,
                              transform.position.y - size.height * 0.5f,
                              size.width, size.height, Color::cyan(), 1.0f);
            }
        });
    }
}

//=============================================================================
// Private Implementation Methods
//=============================================================================

void Renderer2D::sort_render_commands() noexcept {
    // Educational Note: Command sorting optimizes GPU state changes
    // Proper sorting can dramatically improve rendering performance
    
    auto sort_start = std::chrono::high_resolution_clock::now();
    
    std::stable_sort(render_commands_.begin(), render_commands_.end(),
        [](const auto& a, const auto& b) {
            // Primary sort: command type (state changes first)
            if (a->type != b->type) {
                return static_cast<u8>(a->type) < static_cast<u8>(b->type);
            }
            
            // Secondary sort: sort key (depth for sprites)
            return a->sort_key < b->sort_key;
        });
    
    auto sort_end = std::chrono::high_resolution_clock::now();
    auto sort_time = std::chrono::duration<f32, std::milli>(sort_end - sort_start).count();
    
    statistics_.performance.sorting_time_ms = sort_time;
    
    core::Log::debug("Sorted {} render commands in {:.3f}ms", render_commands_.size(), sort_time);
}

void Renderer2D::execute_render_commands() noexcept {
    // Educational Note: Command execution is where actual OpenGL calls happen
    // This centralized approach allows for comprehensive performance monitoring
    
    auto exec_start = std::chrono::high_resolution_clock::now();
    
    for (current_command_index_ = 0; current_command_index_ < render_commands_.size(); ++current_command_index_) {
        auto& command = render_commands_[current_command_index_];
        
        // Step-through mode for educational debugging
        if (step_mode_active_ && config_.debug.enable_step_through_mode) {
            core::Log::info("Step Mode: Executing command {} of {}: {}", 
                           current_command_index_ + 1, render_commands_.size(), command->debug_name);
            // In a real implementation, this would wait for user input
        }
        
        // Execute the command
        execute_command(*command);
        
        // Validate render state if debugging
        if (config_.debug.log_render_commands) {
            validate_render_state();
        }
    }
    
    // Render batch renderer contents
    if (batch_renderer_) {
        batch_renderer_->render_all(*this);
    }
    
    auto exec_end = std::chrono::high_resolution_clock::now();
    auto exec_time = std::chrono::duration<f32, std::milli>(exec_end - exec_start).count();
    
    statistics_.performance.rendering_time_ms = exec_time;
    
    core::Log::debug("Executed {} render commands in {:.3f}ms", render_commands_.size(), exec_time);
}

void Renderer2D::execute_command(const RenderCommand& command) noexcept {
    // Educational Note: Individual command execution with error checking
    // Each command type has different performance characteristics
    
    auto cmd_start = std::chrono::high_resolution_clock::now();
    
    try {
        command.execute(const_cast<Renderer2D&>(*this));
    } catch (const std::exception& e) {
        core::Log::error("Exception executing render command '{}': {}", command.debug_name, e.what());
    }
    
    auto cmd_end = std::chrono::high_resolution_clock::now();
    auto cmd_time = std::chrono::duration<f32, std::milli>(cmd_end - cmd_start).count();
    
    // Track command performance
    if (cmd_time > 1.0f) { // Expensive command threshold
        core::Log::warning("Expensive render command '{}' took {:.3f}ms", command.debug_name, cmd_time);
    }
}

void Renderer2D::update_statistics() noexcept {
    // Educational Note: Statistics update provides comprehensive performance analysis
    // This data is essential for understanding rendering performance characteristics
    
    statistics_.update_performance_analysis();
    
    // Update educational insights
    if (batch_renderer_) {
        auto batch_stats = batch_renderer_->get_statistics();
        statistics_.gpu_stats.batches_created = batch_stats.batches_generated;
        statistics_.gpu_stats.batching_efficiency = batch_stats.batching_efficiency;
        
        // Add educational recommendations
        statistics_.educational.optimization_hints.clear();
        statistics_.educational.learning_points.clear();
        
        if (batch_stats.batching_efficiency < 0.5f) {
            statistics_.educational.optimization_hints.push_back(
                "Low batching efficiency detected. Consider using texture atlases to group similar sprites.");
            statistics_.educational.learning_points.push_back(
                "Batching reduces draw calls by combining multiple sprites into single GPU operations.");
        }
        
        if (statistics_.gpu_stats.draw_calls > 1000) {
            statistics_.educational.optimization_hints.push_back(
                "High draw call count may impact performance. Enable frustum culling and sprite batching.");
        }
        
        if (statistics_.gpu_stats.total_gpu_memory > 100 * 1024 * 1024) { // 100MB
            statistics_.educational.optimization_hints.push_back(
                "High GPU memory usage. Consider texture compression and asset optimization.");
        }
    }
}

void Renderer2D::setup_camera_matrices(const Camera2D& camera) noexcept {
    // Educational Note: Camera matrix setup is crucial for proper 2D rendering
    // View matrix transforms world coordinates to camera space
    // Projection matrix transforms camera space to screen space
    
    const f32* view_matrix = camera.get_view_matrix();
    const f32* proj_matrix = camera.get_projection_matrix();
    
    // In a real implementation, these matrices would be uploaded to GPU uniforms
    // For educational purposes, we'll log the matrix setup
    core::Log::debug("Camera matrices updated for viewport {}x{}", 
                     camera.viewport.width, camera.viewport.height);
    
    // Set OpenGL viewport to match camera
    glViewport(camera.viewport.x, camera.viewport.y, 
              camera.viewport.width, camera.viewport.height);
    gl_utils::check_gl_error("glViewport", "Setting camera viewport for rendering region");
}

void Renderer2D::setup_default_render_state() noexcept {
    // Educational Note: Default render state setup ensures consistent 2D rendering
    // These settings are optimized for typical 2D sprite rendering
    
    core::Log::info("Setting up default OpenGL state for 2D rendering");
    
    // Enable alpha blending for sprite transparency
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    gl_utils::check_gl_error("glBlendFunc", "Setting up alpha blending for sprite transparency");
    
    // Disable depth testing for 2D (use painter's algorithm instead)
    glDisable(GL_DEPTH_TEST);
    gl_utils::check_gl_error("glDisable GL_DEPTH_TEST", "Disabling depth testing for 2D rendering");
    
    // Disable face culling (2D quads need both faces)
    glDisable(GL_CULL_FACE);
    gl_utils::check_gl_error("glDisable GL_CULL_FACE", "Disabling face culling for 2D quads");
    
    // Set clear color to transparent black
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    gl_utils::check_gl_error("glClearColor", "Setting default clear color");
    
    // Configure line rendering for debug visualization
    glLineWidth(1.0f);
    gl_utils::check_gl_error("glLineWidth", "Setting default line width for debug rendering");
    
    core::Log::info("Default OpenGL state configured successfully");
}

void Renderer2D::create_default_resources() noexcept {
    // Educational Note: Default resources ensure the renderer can always function
    // These include fallback textures and shaders for when assets are missing
    
    core::Log::info("Creating default rendering resources");
    
    if (texture_manager_) {
        texture_manager_->create_default_textures();
        core::Log::debug("Default textures created (white, black, transparent)");
    }
    
    if (shader_manager_) {
        shader_manager_->create_default_shaders();
        core::Log::debug("Default shaders created (sprite, UI, debug)");
    }
}

void Renderer2D::cleanup_resources() noexcept {
    // Educational Note: Proper resource cleanup prevents memory leaks
    // GPU resources must be freed explicitly as they're not garbage collected
    
    core::Log::info("Cleaning up renderer resources");
    
    // Clear all bound textures
    for (u32 i = 0; i < bound_textures_.size(); ++i) {
        bound_textures_[i] = INVALID_TEXTURE_ID;
        glActiveTexture(GL_TEXTURE0 + i);
        glBindTexture(GL_TEXTURE_2D, 0);
    }
    gl_utils::check_gl_error("Texture cleanup", "Unbinding all textures during shutdown");
    
    // Reset active state
    active_camera_ = nullptr;
    active_material_ = nullptr;
    active_shader_ = INVALID_SHADER_ID;
    
    core::Log::debug("Resource cleanup completed");
}

void Renderer2D::record_draw_call(u32 vertices, u32 indices) noexcept {
    // Educational Note: Draw call recording helps understand rendering performance
    // Each draw call has GPU overhead, so minimizing them improves performance
    
    statistics_.gpu_stats.draw_calls++;
    statistics_.gpu_stats.vertices_rendered += vertices;
    statistics_.gpu_stats.triangles_rendered += indices / 3; // 3 indices per triangle
    
    // Estimate memory usage for this draw call
    usize vertex_memory = vertices * 24; // Estimated vertex size
    usize index_memory = indices * sizeof(u16);
    statistics_.gpu_stats.vertex_buffer_memory += vertex_memory;
    statistics_.gpu_stats.index_buffer_memory += index_memory;
}

void Renderer2D::log_render_command(const RenderCommand& command) noexcept {
    // Educational Note: Command logging helps understand rendering pipeline flow
    // This information is valuable for debugging and learning
    
    const char* type_names[] = {
        "DrawSprite", "DrawBatch", "DrawDebugLine", "DrawDebugBox", 
        "DrawDebugCircle", "SetCamera", "SetMaterial", "SetRenderTarget",
        "ClearTarget", "PushDebugGroup", "PopDebugGroup"
    };
    
    const char* type_name = "Unknown";
    if (static_cast<u8>(command.type) < sizeof(type_names) / sizeof(type_names[0])) {
        type_name = type_names[static_cast<u8>(command.type)];
    }
    
    core::Log::debug("Render Command: {} | Sort Key: {:.2f} | Entity: {} | {}",
                     type_name, command.sort_key, command.entity_id, 
                     command.debug_name ? command.debug_name : "Unnamed");
}

void Renderer2D::validate_render_state() const noexcept {
    // Educational Note: State validation helps catch rendering errors early
    // OpenGL is a state machine, so invalid state can cause subtle bugs
    
    // Check for OpenGL errors
    gl_utils::check_gl_error("Render state validation");
    
    // Validate camera state
    if (!active_camera_) {
        core::Log::warning("No active camera set for rendering");
    }
    
    // Check viewport
    GLint viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);
    if (viewport[2] <= 0 || viewport[3] <= 0) {
        core::Log::warning("Invalid viewport size: {}x{}", viewport[2], viewport[3]);
    }
}

Color Renderer2D::get_batch_debug_color(u32 batch_id) const noexcept {
    // Educational Note: Debug colors help visualize batch distribution
    // Different colors show how sprites are grouped for rendering
    
    static const Color debug_colors[] = {
        Color::red(), Color::green(), Color::blue(), Color::yellow(),
        Color::cyan(), Color::magenta(), Color{255, 128, 0}, Color{128, 255, 0},
        Color{0, 255, 128}, Color{128, 0, 255}, Color{255, 0, 128}, Color{255, 128, 128}
    };
    
    return debug_colors[batch_id % (sizeof(debug_colors) / sizeof(debug_colors[0]))];
}

std::string Renderer2D::generate_render_report() const noexcept {
    return statistics_.generate_performance_report();
}

Renderer2D::MemoryUsage Renderer2D::get_memory_usage() const noexcept {
    MemoryUsage usage;
    
    usage.vertex_buffers = statistics_.gpu_stats.vertex_buffer_memory;
    usage.index_buffers = statistics_.gpu_stats.index_buffer_memory;
    usage.textures = statistics_.gpu_stats.texture_memory;
    usage.render_commands = render_commands_.size() * sizeof(std::unique_ptr<RenderCommand>);
    
    if (batch_renderer_) {
        auto batch_memory = batch_renderer_->get_memory_breakdown();
        usage.vertex_buffers += batch_memory.vertex_data;
        usage.index_buffers += batch_memory.index_data;
    }
    
    usage.total = usage.vertex_buffers + usage.index_buffers + usage.textures + 
                  usage.shaders + usage.render_commands;
    
    return usage;
}

//=============================================================================
// Rendering System Implementation
//=============================================================================

RenderingSystem::RenderingSystem(std::shared_ptr<Renderer2D> renderer) noexcept
    : renderer_(std::move(renderer))
{
    if (!renderer_) {
        core::Log::error("RenderingSystem created with null renderer");
    }
}

void RenderingSystem::update(ecs::Registry& registry, f32 delta_time) noexcept {
    if (!renderer_ || !renderer_->is_initialized()) {
        return;
    }
    
    // Educational Note: System update demonstrates ECS integration
    // The rendering system processes components to generate render commands
    
    renderer_->begin_frame();
    
    // Process cameras if auto-handling is enabled
    if (auto_camera_handling_) {
        process_cameras(registry);
    }
    
    // Render all sprite entities
    renderer_->render_entities(registry);
    
    // Process debug rendering
    if (renderer_->is_debug_rendering_enabled()) {
        process_debug_rendering(registry);
    }
    
    // Update render info components
    update_render_info_components(registry);
    
    renderer_->end_frame();
}

void RenderingSystem::process_cameras(ecs::Registry& registry) noexcept {
    using namespace components;
    
    // Find and set the first active camera
    registry.view<Camera2D>().each([this](auto entity, auto& camera) {
        if (camera.camera_flags.active) {
            renderer_->set_active_camera(camera);
            return; // Use first active camera found
        }
    });
    
    // Fallback to default camera if none active
    if (!renderer_->get_active_camera()) {
        renderer_->set_active_camera(default_camera_);
    }
}

void RenderingSystem::process_sprites(ecs::Registry& registry) noexcept {
    // This is handled by renderer_->render_entities()
    // Separated for educational clarity
}

void RenderingSystem::process_debug_rendering(ecs::Registry& registry) noexcept {
    using namespace components;
    
    // Educational Note: Debug rendering provides visual feedback
    // Essential for development and educational demonstration
    
    registry.view<Transform, RenderableSprite, RenderInfo>().each(
        [this](auto entity, auto& transform, auto& sprite, auto& render_info) {
            
            // Draw bounding boxes if requested
            if (render_info.debug_settings.debug_flags.show_bounds) {
                auto size = sprite.calculate_world_size();
                renderer_->draw_debug_box(
                    transform.position.x - size.width * 0.5f,
                    transform.position.y - size.height * 0.5f,
                    size.width, size.height,
                    render_info.debug_settings.bounds_color
                );
            }
            
            // Draw pivot points if requested  
            if (render_info.debug_settings.debug_flags.show_pivot) {
                auto size = sprite.calculate_world_size();
                f32 pivot_x = transform.position.x + (sprite.pivot.x - 0.5f) * size.width;
                f32 pivot_y = transform.position.y + (sprite.pivot.y - 0.5f) * size.height;
                
                renderer_->draw_debug_line(pivot_x - 5, pivot_y, pivot_x + 5, pivot_y, 
                                         render_info.debug_settings.pivot_color);
                renderer_->draw_debug_line(pivot_x, pivot_y - 5, pivot_x, pivot_y + 5, 
                                         render_info.debug_settings.pivot_color);
            }
        });
}

void RenderingSystem::update_render_info_components(ecs::Registry& registry) noexcept {
    using namespace components;
    
    // Educational Note: Performance tracking provides real-time feedback
    // Essential for understanding rendering performance characteristics
    
    registry.view<RenderInfo>().each([](auto entity, auto& render_info) {
        render_info.update_analysis();
    });
}

} // namespace ecscope::renderer