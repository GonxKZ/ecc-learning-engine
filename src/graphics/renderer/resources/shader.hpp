#pragma once

/**
 * @file renderer/resources/shader.hpp
 * @brief Shader Resource Management System for ECScope Educational ECS Engine - Phase 7: Renderizado 2D
 * 
 * This header provides a comprehensive shader management system designed for educational
 * clarity while maintaining professional-grade performance. It includes:
 * 
 * Core Features:
 * - OpenGL 3.3+ shader program compilation and management with RAII
 * - Vertex, fragment, geometry, and compute shader support
 * - Uniform buffer management and automatic reflection
 * - Hot shader reloading for rapid development iteration
 * - Shader validation and comprehensive error reporting
 * 
 * Educational Features:
 * - Detailed documentation of graphics pipeline and shader concepts
 * - Performance metrics and GPU instruction analysis
 * - Interactive shader debugging and uniform inspection
 * - Comprehensive error messages with suggested fixes
 * - Visual shader editor integration support
 * 
 * Advanced Features:
 * - Shader variants and preprocessor definitions
 * - Cross-platform shader compilation (GLSL, HLSL, SPIR-V)
 * - Shader caching and binary program storage
 * - Dynamic shader compilation and linking
 * - GPU performance profiling and optimization hints
 * 
 * Performance Characteristics:
 * - Efficient uniform buffer objects (UBO) and storage buffers
 * - Shader program caching and reuse optimization
 * - Automatic uniform location caching
 * - Background compilation for smooth frame rates
 * - Memory-mapped shader binary loading
 * 
 * @author ECScope Educational ECS Framework - Phase 7: Renderizado 2D
 * @date 2024
 */

#include "../../core/types.hpp"
#include "../../core/result.hpp"
#include "../../memory/memory_tracker.hpp"
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <optional>
#include <functional>
#include <array>

namespace ecscope::renderer::resources {

//=============================================================================
// Forward Declarations and Type Definitions
//=============================================================================

class ShaderManager;
struct ShaderSource;
struct UniformBuffer;

using ShaderID = u32;
constexpr ShaderID INVALID_SHADER_ID = 0;
constexpr ShaderID DEFAULT_SPRITE_SHADER_ID = 1;
constexpr ShaderID DEFAULT_UI_SHADER_ID = 2;

//=============================================================================
// Shader Types and Stages
//=============================================================================

/**
 * @brief Shader Stage Types
 * 
 * Represents different stages in the graphics pipeline where shaders execute.
 * Each stage has specific responsibilities and capabilities.
 * 
 * Educational Context:
 * The graphics pipeline processes vertices through multiple stages:
 * 1. Vertex: Transform vertices from model to clip space
 * 2. Geometry: Generate additional geometry (optional)
 * 3. Fragment: Calculate final pixel colors
 * 4. Compute: General purpose GPU computation (not part of graphics pipeline)
 */
enum class ShaderStage : u8 {
    Vertex = 0,         ///< Vertex shader - processes individual vertices
    Fragment,           ///< Fragment/Pixel shader - processes pixels
    Geometry,           ///< Geometry shader - can generate new vertices
    TessControl,        ///< Tessellation control shader (advanced)
    TessEvaluation,     ///< Tessellation evaluation shader (advanced)
    Compute             ///< Compute shader - general purpose computation
};

/**
 * @brief Shader Data Types for Uniforms
 * 
 * Represents different data types that can be passed to shaders as uniform variables.
 * Each type has specific size and alignment requirements.
 */
enum class ShaderDataType : u8 {
    Unknown = 0,
    
    // Scalar types
    Bool,               ///< Boolean value (stored as 4-byte int)
    Int,                ///< 32-bit signed integer
    UInt,               ///< 32-bit unsigned integer
    Float,              ///< 32-bit floating point
    Double,             ///< 64-bit floating point
    
    // Vector types
    Vec2,               ///< 2-component float vector
    Vec3,               ///< 3-component float vector  
    Vec4,               ///< 4-component float vector
    IVec2,              ///< 2-component int vector
    IVec3,              ///< 3-component int vector
    IVec4,              ///< 4-component int vector
    UVec2,              ///< 2-component uint vector
    UVec3,              ///< 3-component uint vector
    UVec4,              ///< 4-component uint vector
    BVec2,              ///< 2-component bool vector
    BVec3,              ///< 3-component bool vector
    BVec4,              ///< 4-component bool vector
    
    // Matrix types
    Mat2,               ///< 2x2 float matrix
    Mat3,               ///< 3x3 float matrix
    Mat4,               ///< 4x4 float matrix
    Mat2x3,             ///< 2x3 float matrix
    Mat2x4,             ///< 2x4 float matrix
    Mat3x2,             ///< 3x2 float matrix
    Mat3x4,             ///< 3x4 float matrix
    Mat4x2,             ///< 4x2 float matrix
    Mat4x3,             ///< 4x3 float matrix
    
    // Texture types
    Sampler2D,          ///< 2D texture sampler
    SamplerCube,        ///< Cube map texture sampler
    Sampler2DArray,     ///< 2D texture array sampler
    
    // Buffer types
    UniformBuffer,      ///< Uniform buffer object
    StorageBuffer       ///< Shader storage buffer object
};

/**
 * @brief Shader Compilation Target
 * 
 * Specifies the target shading language and version for compilation.
 * Enables cross-platform shader development.
 */
enum class ShaderTarget : u8 {
    GLSL_330 = 0,       ///< OpenGL Shading Language 3.30
    GLSL_400,           ///< OpenGL Shading Language 4.00
    GLSL_450,           ///< OpenGL Shading Language 4.50
    GLSL_460,           ///< OpenGL Shading Language 4.60
    HLSL_50,            ///< DirectX High Level Shading Language 5.0
    SPIRV_10,           ///< SPIR-V 1.0 (Vulkan)
    SPIRV_14            ///< SPIR-V 1.4 (Vulkan)
};

//=============================================================================
// Shader Uniform System
//=============================================================================

/**
 * @brief Shader Uniform Variable Information
 * 
 * Contains metadata about a uniform variable including location, type,
 * and size information for efficient uniform management.
 * 
 * Educational Context:
 * Uniforms are global variables in shaders that remain constant for all
 * vertices/fragments in a draw call. They're perfect for transformation
 * matrices, material properties, and lighting parameters.
 */
struct UniformInfo {
    std::string name;               ///< Uniform variable name
    ShaderDataType type;            ///< Data type
    i32 location;                   ///< OpenGL uniform location
    u32 size;                       ///< Size in bytes
    u32 array_size;                 ///< Array size (1 for non-arrays)
    u32 offset;                     ///< Offset within uniform buffer
    
    // Educational information
    const char* type_description;   ///< Human-readable type description
    const char* usage_hint;         ///< Suggested usage pattern
    bool is_builtin;                ///< Whether this is a built-in uniform
    
    UniformInfo() noexcept = default;
    UniformInfo(std::string n, ShaderDataType t, i32 loc, u32 sz, u32 array_sz = 1) noexcept
        : name(std::move(n)), type(t), location(loc), size(sz), array_size(array_sz), 
          offset(0), type_description(get_type_description(t)), usage_hint(""), is_builtin(false) {}
    
    /** @brief Get size of the data type in bytes */
    static u32 get_type_size(ShaderDataType type) noexcept;
    
    /** @brief Get alignment requirement for the data type */
    static u32 get_type_alignment(ShaderDataType type) noexcept;
    
    /** @brief Get human-readable type description */
    static const char* get_type_description(ShaderDataType type) noexcept;
    
    /** @brief Get OpenGL constant for the data type */
    static u32 get_gl_type(ShaderDataType type) noexcept;
};

/**
 * @brief Uniform Buffer Layout
 * 
 * Describes the memory layout of a uniform buffer, including all uniform
 * variables and their offsets. Used for efficient batch uniform updates.
 */
struct UniformBufferLayout {
    std::vector<UniformInfo> uniforms;     ///< Uniform variables in the buffer
    u32 total_size;                        ///< Total buffer size in bytes
    u32 alignment;                         ///< Buffer alignment requirement
    std::string name;                      ///< Buffer name for debugging
    
    UniformBufferLayout() noexcept : total_size(0), alignment(16) {}
    
    /** @brief Add uniform to the layout */
    void add_uniform(const UniformInfo& uniform) noexcept;
    
    /** @brief Calculate final layout with proper alignment */
    void finalize_layout() noexcept;
    
    /** @brief Find uniform by name */
    const UniformInfo* find_uniform(const std::string& name) const noexcept;
    
    /** @brief Get uniform by index */
    const UniformInfo& get_uniform(usize index) const noexcept { return uniforms[index]; }
    
    /** @brief Get number of uniforms */
    usize get_uniform_count() const noexcept { return uniforms.size(); }
    
    /** @brief Validate layout consistency */
    bool is_valid() const noexcept;
};

/**
 * @brief Uniform Buffer Object (UBO) Management
 * 
 * Provides efficient management of uniform buffer objects for batch uniform updates.
 * Significantly more efficient than individual uniform updates for complex shaders.
 * 
 * Educational Context:
 * UBOs allow uploading multiple uniform values in a single operation,
 * reducing CPU-GPU synchronization overhead and improving performance.
 */
class UniformBuffer {
public:
    /** @brief Create uniform buffer with specified layout */
    explicit UniformBuffer(const UniformBufferLayout& layout) noexcept;
    
    /** @brief Destructor releases GPU resources */
    ~UniformBuffer() noexcept;
    
    // Move-only semantics
    UniformBuffer(const UniformBuffer&) = delete;
    UniformBuffer& operator=(const UniformBuffer&) = delete;
    UniformBuffer(UniformBuffer&& other) noexcept;
    UniformBuffer& operator=(UniformBuffer&& other) noexcept;
    
    /** @brief Check if buffer is valid */
    bool is_valid() const noexcept { return gl_buffer_id_ != 0; }
    
    /** @brief Get OpenGL buffer ID */
    u32 get_gl_id() const noexcept { return gl_buffer_id_; }
    
    /** @brief Get buffer layout */
    const UniformBufferLayout& get_layout() const noexcept { return layout_; }
    
    /** @brief Get buffer size in bytes */
    u32 get_size() const noexcept { return layout_.total_size; }
    
    /** @brief Bind buffer to specified binding point */
    void bind(u32 binding_point) const noexcept;
    
    /** @brief Update entire buffer data */
    void update_data(const void* data) const noexcept;
    
    /** @brief Update partial buffer data */
    void update_sub_data(u32 offset, u32 size, const void* data) const noexcept;
    
    // Type-safe uniform setters
    void set_uniform(const std::string& name, f32 value) noexcept;
    void set_uniform(const std::string& name, f32 x, f32 y) noexcept;
    void set_uniform(const std::string& name, f32 x, f32 y, f32 z) noexcept;
    void set_uniform(const std::string& name, f32 x, f32 y, f32 z, f32 w) noexcept;
    void set_uniform(const std::string& name, i32 value) noexcept;
    void set_uniform(const std::string& name, const f32* matrix, bool transpose = false) noexcept;
    
    /** @brief Upload all cached uniform changes to GPU */
    void upload_changes() noexcept;
    
    /** @brief Get performance statistics */
    struct BufferStats {
        u32 update_count{0};
        u32 upload_count{0};
        f32 total_upload_time{0.0f};
        usize memory_usage{0};
    };
    
    const BufferStats& get_stats() const noexcept { return stats_; }
    void reset_stats() noexcept { stats_ = {}; }

private:
    UniformBufferLayout layout_;
    u32 gl_buffer_id_{0};
    std::vector<u8> cpu_buffer_;            ///< CPU-side buffer for staging
    std::vector<bool> dirty_uniforms_;      ///< Track which uniforms need uploading
    bool buffer_dirty_{false};              ///< Whether buffer needs uploading
    mutable BufferStats stats_{};
    
    void create_gl_buffer() noexcept;
    void destroy_gl_buffer() noexcept;
    void mark_uniform_dirty(usize index) noexcept;
    void set_uniform_data(usize index, const void* data, usize size) noexcept;
};

//=============================================================================
// Shader Source Management
//=============================================================================

/**
 * @brief Shader Source Code Container
 * 
 * Holds shader source code with preprocessing support, include resolution,
 * and educational features for understanding shader development.
 */
struct ShaderSource {
    //-------------------------------------------------------------------------
    // Source Code
    //-------------------------------------------------------------------------
    
    std::string vertex_source;         ///< Vertex shader source code
    std::string fragment_source;       ///< Fragment shader source code
    std::string geometry_source;       ///< Geometry shader source code (optional)
    std::string compute_source;        ///< Compute shader source code (optional)
    
    //-------------------------------------------------------------------------
    // Metadata and Configuration
    //-------------------------------------------------------------------------
    
    std::string name;                  ///< Human-readable shader name
    ShaderTarget target;               ///< Compilation target
    std::vector<std::string> defines;  ///< Preprocessor definitions
    std::vector<std::string> includes; ///< Include search paths
    std::string base_path;             ///< Base path for relative includes
    
    //-------------------------------------------------------------------------
    // Educational Features
    //-------------------------------------------------------------------------
    
    struct SourceInfo {
        u32 vertex_lines{0};            ///< Number of lines in vertex shader
        u32 fragment_lines{0};          ///< Number of lines in fragment shader
        u32 total_characters{0};        ///< Total character count
        bool uses_textures{false};      ///< Whether shader samples textures
        bool uses_uniforms{false};      ///< Whether shader uses uniforms
        bool uses_vertex_colors{false}; ///< Whether shader uses vertex colors
        const char* complexity_rating;  ///< "Simple", "Moderate", "Complex"
        std::vector<std::string> detected_features; ///< Automatically detected features
    } source_info;
    
    //-------------------------------------------------------------------------
    // Construction and Loading
    //-------------------------------------------------------------------------
    
    ShaderSource() noexcept : target(ShaderTarget::GLSL_330) {}
    
    /** @brief Load shader from files */
    static core::Result<ShaderSource, std::string> load_from_files(
        const std::string& vertex_path,
        const std::string& fragment_path,
        const std::string& geometry_path = {},
        ShaderTarget target = ShaderTarget::GLSL_330) noexcept;
    
    /** @brief Load shader from single file with sections */
    static core::Result<ShaderSource, std::string> load_from_single_file(
        const std::string& file_path,
        ShaderTarget target = ShaderTarget::GLSL_330) noexcept;
    
    /** @brief Create from source strings */
    static ShaderSource create_from_strings(
        const std::string& vertex_src,
        const std::string& fragment_src,
        const std::string& geometry_src = {},
        ShaderTarget target = ShaderTarget::GLSL_330) noexcept;
    
    //-------------------------------------------------------------------------
    // Preprocessing and Validation
    //-------------------------------------------------------------------------
    
    /** @brief Add preprocessor definition */
    void add_define(const std::string& name, const std::string& value = {}) noexcept {
        if (value.empty()) {
            defines.push_back("#define " + name);
        } else {
            defines.push_back("#define " + name + " " + value);
        }
    }
    
    /** @brief Add include search path */
    void add_include_path(const std::string& path) noexcept {
        includes.push_back(path);
    }
    
    /** @brief Process includes and defines */
    core::Result<ShaderSource, std::string> preprocess() const noexcept;
    
    /** @brief Validate shader source syntax */
    core::Result<void, std::string> validate_syntax() const noexcept;
    
    /** @brief Analyze source code for educational insights */
    void analyze_source() noexcept;
    
    /** @brief Get preprocessed source for specific stage */
    std::string get_preprocessed_source(ShaderStage stage) const noexcept;
    
    //-------------------------------------------------------------------------
    // Utility Functions
    //-------------------------------------------------------------------------
    
    /** @brief Check if shader has specific stage */
    bool has_stage(ShaderStage stage) const noexcept;
    
    /** @brief Get source for specific stage */
    const std::string& get_stage_source(ShaderStage stage) const noexcept;
    
    /** @brief Count total lines across all stages */
    u32 get_total_line_count() const noexcept;
    
    /** @brief Estimate compilation complexity */
    f32 estimate_compilation_time() const noexcept;
    
    /** @brief Validate source completeness */
    bool is_valid() const noexcept {
        return !vertex_source.empty() && !fragment_source.empty();
    }
    
private:
    std::string resolve_includes(const std::string& source, const std::string& current_path) const noexcept;
    std::string apply_defines(const std::string& source) const noexcept;
    void detect_features(const std::string& source, SourceInfo& info) const noexcept;
};

//=============================================================================
// Shader Program - Compiled and Linked Shader
//=============================================================================

/**
 * @brief Compiled Shader Program
 * 
 * Represents a complete, compiled and linked shader program ready for use
 * in rendering. Provides comprehensive uniform management and educational
 * features for understanding GPU programming.
 * 
 * Educational Context:
 * A shader program is the result of compiling multiple shader stages
 * (vertex, fragment, etc.) and linking them together. It's the executable
 * code that runs on the GPU during rendering.
 */
class ShaderProgram {
public:
    //-------------------------------------------------------------------------
    // Construction and Compilation
    //-------------------------------------------------------------------------
    
    /** @brief Default constructor creates invalid program */
    ShaderProgram() noexcept = default;
    
    /** @brief Create and compile shader program from source */
    explicit ShaderProgram(const ShaderSource& source) noexcept;
    
    /** @brief Destructor releases GPU resources */
    ~ShaderProgram() noexcept;
    
    // Move-only semantics
    ShaderProgram(const ShaderProgram&) = delete;
    ShaderProgram& operator=(const ShaderProgram&) = delete;
    ShaderProgram(ShaderProgram&& other) noexcept;
    ShaderProgram& operator=(ShaderProgram&& other) noexcept;
    
    //-------------------------------------------------------------------------
    // Program State and Information
    //-------------------------------------------------------------------------
    
    /** @brief Check if program compiled and linked successfully */
    bool is_valid() const noexcept { return gl_program_id_ != 0 && is_linked_; }
    
    /** @brief Get OpenGL program ID */
    u32 get_gl_id() const noexcept { return gl_program_id_; }
    
    /** @brief Get shader program name */
    const std::string& get_name() const noexcept { return name_; }
    
    /** @brief Set debug name */
    void set_name(const std::string& name) noexcept { name_ = name; }
    
    /** @brief Get compilation/linking errors */
    const std::string& get_error_log() const noexcept { return error_log_; }
    
    /** @brief Check if program has specific shader stage */
    bool has_stage(ShaderStage stage) const noexcept;
    
    //-------------------------------------------------------------------------
    // Program Binding and Usage
    //-------------------------------------------------------------------------
    
    /** @brief Bind shader program for rendering */
    void bind() const noexcept;
    
    /** @brief Unbind shader program */
    void unbind() const noexcept;
    
    /** @brief Check if program is currently bound */
    bool is_bound() const noexcept;
    
    //-------------------------------------------------------------------------
    // Uniform Management
    //-------------------------------------------------------------------------
    
    /** @brief Get all uniform information */
    const std::vector<UniformInfo>& get_uniforms() const noexcept { return uniforms_; }
    
    /** @brief Get uniform information by name */
    const UniformInfo* get_uniform_info(const std::string& name) const noexcept;
    
    /** @brief Check if uniform exists */
    bool has_uniform(const std::string& name) const noexcept;
    
    /** @brief Get uniform location */
    i32 get_uniform_location(const std::string& name) const noexcept;
    
    // Type-safe uniform setters
    void set_uniform(const std::string& name, f32 value) const noexcept;
    void set_uniform(const std::string& name, f32 x, f32 y) const noexcept;
    void set_uniform(const std::string& name, f32 x, f32 y, f32 z) const noexcept;
    void set_uniform(const std::string& name, f32 x, f32 y, f32 z, f32 w) const noexcept;
    void set_uniform(const std::string& name, i32 value) const noexcept;
    void set_uniform(const std::string& name, bool value) const noexcept;
    void set_uniform(const std::string& name, const f32* values, i32 count) const noexcept;
    void set_uniform_matrix(const std::string& name, const f32* matrix, bool transpose = false) const noexcept;
    void set_uniform_matrix3(const std::string& name, const f32* matrix, bool transpose = false) const noexcept;
    void set_uniform_matrix4(const std::string& name, const f32* matrix, bool transpose = false) const noexcept;
    
    // Direct uniform setting by location (faster)
    void set_uniform(i32 location, f32 value) const noexcept;
    void set_uniform(i32 location, f32 x, f32 y) const noexcept;
    void set_uniform(i32 location, f32 x, f32 y, f32 z) const noexcept;
    void set_uniform(i32 location, f32 x, f32 y, f32 z, f32 w) const noexcept;
    void set_uniform(i32 location, i32 value) const noexcept;
    void set_uniform_matrix4(i32 location, const f32* matrix, bool transpose = false) const noexcept;
    
    //-------------------------------------------------------------------------
    // Uniform Buffer Management
    //-------------------------------------------------------------------------
    
    /** @brief Get uniform buffer layout by name */
    const UniformBufferLayout* get_uniform_buffer_layout(const std::string& name) const noexcept;
    
    /** @brief Bind uniform buffer to program */
    void bind_uniform_buffer(const std::string& buffer_name, u32 binding_point) const noexcept;
    
    /** @brief Get all uniform buffer layouts */
    const std::vector<UniformBufferLayout>& get_uniform_buffer_layouts() const noexcept { 
        return uniform_buffer_layouts_; 
    }
    
    //-------------------------------------------------------------------------
    // Performance Monitoring
    //-------------------------------------------------------------------------
    
    /** @brief Shader performance statistics */
    struct ShaderStats {
        u32 bind_count{0};                  ///< Number of times shader was bound
        u32 uniform_updates{0};             ///< Number of uniform updates
        f32 total_bind_time{0.0f};          ///< Total time spent binding
        f32 total_uniform_time{0.0f};       ///< Total time spent updating uniforms
        f32 compilation_time{0.0f};         ///< Time spent compiling
        f32 link_time{0.0f};                ///< Time spent linking
        u32 draw_calls_with_shader{0};      ///< Draw calls using this shader
        
        // GPU performance estimates
        f32 estimated_gpu_cost{1.0f};       ///< Estimated GPU execution cost
        f32 vertex_shader_cost{0.5f};       ///< Vertex stage cost estimate
        f32 fragment_shader_cost{0.5f};     ///< Fragment stage cost estimate
    };
    
    /** @brief Get performance statistics */
    const ShaderStats& get_stats() const noexcept { return stats_; }
    
    /** @brief Reset performance counters */
    void reset_stats() noexcept { stats_ = {}; }
    
    //-------------------------------------------------------------------------
    // Educational and Debug Features
    //-------------------------------------------------------------------------
    
    /** @brief Comprehensive shader program information */
    struct ProgramInfo {
        std::string name;
        bool is_valid;
        bool has_vertex_stage;
        bool has_fragment_stage;
        bool has_geometry_stage;
        u32 uniform_count;
        u32 uniform_buffer_count;
        usize estimated_memory_usage;
        f32 compilation_complexity;
        const char* performance_rating;      ///< "Excellent", "Good", "Fair", "Poor"
        std::vector<std::string> optimization_hints;
        std::vector<std::string> detected_features;
    };
    
    /** @brief Get comprehensive program information */
    ProgramInfo get_program_info() const noexcept;
    
    /** @brief Generate detailed shader report for educational purposes */
    std::string generate_shader_report() const noexcept;
    
    /** @brief Validate shader program state */
    bool validate_program() const noexcept;
    
    /** @brief Get shader IL/assembly (if available) */
    std::string get_shader_assembly(ShaderStage stage) const noexcept;
    
    /** @brief Analyze shader for performance bottlenecks */
    std::vector<std::string> analyze_performance() const noexcept;
    
    //-------------------------------------------------------------------------
    // Hot Reloading and Development
    //-------------------------------------------------------------------------
    
    /** @brief Recompile shader from updated source */
    core::Result<void, std::string> reload_from_source(const ShaderSource& new_source) noexcept;
    
    /** @brief Check if shader needs recompilation (file changed) */
    bool needs_recompilation() const noexcept;
    
    /** @brief Get source file timestamps */
    const std::unordered_map<std::string, u64>& get_file_timestamps() const noexcept { 
        return file_timestamps_; 
    }

private:
    //-------------------------------------------------------------------------
    // Internal State
    //-------------------------------------------------------------------------
    
    u32 gl_program_id_{0};                      ///< OpenGL program object ID
    std::string name_;                          ///< Debug name
    bool is_linked_{false};                     ///< Whether program linked successfully
    std::string error_log_;                     ///< Compilation/linking errors
    
    // Shader stages
    std::array<u32, 6> stage_ids_{};            ///< OpenGL shader IDs for each stage
    std::bitset<6> active_stages_;              ///< Which stages are present
    
    // Uniform management
    std::vector<UniformInfo> uniforms_;         ///< All uniform variables
    std::unordered_map<std::string, i32> uniform_locations_; ///< Cached uniform locations
    std::vector<UniformBufferLayout> uniform_buffer_layouts_; ///< UBO layouts
    
    // Performance and debug
    mutable ShaderStats stats_{};               ///< Performance statistics
    std::unordered_map<std::string, u64> file_timestamps_; ///< File modification times
    
    //-------------------------------------------------------------------------
    // Internal Methods
    //-------------------------------------------------------------------------
    
    void create_program() noexcept;
    void destroy_program() noexcept;
    core::Result<void, std::string> compile_and_link(const ShaderSource& source) noexcept;
    u32 compile_shader_stage(ShaderStage stage, const std::string& source) noexcept;
    bool link_program() noexcept;
    void reflect_uniforms() noexcept;
    void reflect_uniform_buffers() noexcept;
    void cache_uniform_locations() noexcept;
    void record_bind() const noexcept;
    void record_uniform_update() const noexcept;
    
    // OpenGL utility functions
    static u32 stage_to_gl_shader_type(ShaderStage stage) noexcept;
    static const char* stage_to_string(ShaderStage stage) noexcept;
    static ShaderDataType gl_type_to_shader_type(u32 gl_type) noexcept;
};

//=============================================================================
// Shader Manager - Central Shader Resource Management
//=============================================================================

/**
 * @brief Centralized Shader Resource Manager
 * 
 * Manages all shader resources in the system with compilation caching,
 * hot reloading, and comprehensive educational features.
 * 
 * Educational Context:
 * The shader manager demonstrates:
 * - Resource pooling and caching for compiled shaders
 * - Hot reloading for rapid development iteration
 * - Shader variant management with preprocessor definitions
 * - Performance monitoring and optimization
 * - Cross-platform shader compilation strategies
 */
class ShaderManager {
public:
    //-------------------------------------------------------------------------
    // Manager Configuration
    //-------------------------------------------------------------------------
    
    struct Config {
        bool enable_hot_reload{true};           ///< Enable file change detection
        bool enable_shader_cache{true};         ///< Enable compiled shader caching
        bool enable_binary_cache{true};         ///< Cache binary shader programs
        std::string cache_directory{"shaders/cache"}; ///< Cache directory path
        std::string shader_directory{"shaders"}; ///< Shader source directory
        
        // Compilation settings
        ShaderTarget default_target{ShaderTarget::GLSL_330}; ///< Default compilation target
        bool validate_shaders{true};            ///< Validate shader syntax
        bool optimize_shaders{true};            ///< Enable shader optimization
        bool generate_debug_info{false};        ///< Include debug information
        
        // Educational settings
        bool collect_statistics{true};          ///< Collect usage statistics
        bool enable_profiling{false};           ///< Enable detailed profiling
        bool log_shader_operations{false};      ///< Log all shader operations
        u32 max_error_log_size{4096};          ///< Maximum error log size
    };
    
    //-------------------------------------------------------------------------
    // Construction and Initialization
    //-------------------------------------------------------------------------
    
    /** @brief Create shader manager with configuration */
    explicit ShaderManager(const Config& config = Config{}) noexcept;
    
    /** @brief Destructor cleans up all resources */
    ~ShaderManager() noexcept;
    
    // Non-copyable, moveable
    ShaderManager(const ShaderManager&) = delete;
    ShaderManager& operator=(const ShaderManager&) = delete;
    ShaderManager(ShaderManager&&) noexcept = default;
    ShaderManager& operator=(ShaderManager&&) noexcept = default;
    
    //-------------------------------------------------------------------------
    // Shader Creation and Loading
    //-------------------------------------------------------------------------
    
    /** @brief Create shader from source code */
    core::Result<ShaderID, std::string> create_shader(const ShaderSource& source, 
                                                      const std::string& name = {}) noexcept;
    
    /** @brief Load shader from files */
    core::Result<ShaderID, std::string> load_shader(const std::string& vertex_path,
                                                    const std::string& fragment_path,
                                                    const std::string& geometry_path = {},
                                                    const std::string& name = {}) noexcept;
    
    /** @brief Load shader from single file with sections */
    core::Result<ShaderID, std::string> load_shader_single_file(const std::string& file_path,
                                                               const std::string& name = {}) noexcept;
    
    /** @brief Create shader variant with different defines */
    core::Result<ShaderID, std::string> create_shader_variant(ShaderID base_shader_id,
                                                             const std::vector<std::string>& defines,
                                                             const std::string& variant_name = {}) noexcept;
    
    /** @brief Create default system shaders */
    void create_default_shaders() noexcept;
    
    //-------------------------------------------------------------------------
    // Shader Access and Management
    //-------------------------------------------------------------------------
    
    /** @brief Get shader program by ID */
    ShaderProgram* get_shader(ShaderID id) noexcept;
    const ShaderProgram* get_shader(ShaderID id) const noexcept;
    
    /** @brief Check if shader exists */
    bool has_shader(ShaderID id) const noexcept;
    
    /** @brief Remove shader and free resources */
    void remove_shader(ShaderID id) noexcept;
    
    /** @brief Remove all shaders */
    void clear_all_shaders() noexcept;
    
    /** @brief Get shader ID by name */
    ShaderID find_shader(const std::string& name) const noexcept;
    
    /** @brief Set shader debug name */
    void set_shader_name(ShaderID id, const std::string& name) noexcept;
    
    //-------------------------------------------------------------------------
    // Hot Reloading and Development
    //-------------------------------------------------------------------------
    
    /** @brief Enable/disable hot reloading */
    void set_hot_reload_enabled(bool enabled) noexcept { config_.enable_hot_reload = enabled; }
    
    /** @brief Check for file changes and reload if needed */
    void update_hot_reload() noexcept;
    
    /** @brief Force reload of specific shader */
    core::Result<void, std::string> reload_shader(ShaderID id) noexcept;
    
    /** @brief Force reload of all shaders */
    void reload_all_shaders() noexcept;
    
    /** @brief Get list of shaders that need reloading */
    std::vector<ShaderID> get_shaders_needing_reload() const noexcept;
    
    //-------------------------------------------------------------------------
    // Caching and Optimization
    //-------------------------------------------------------------------------
    
    /** @brief Save compiled shader cache to disk */
    void save_shader_cache() noexcept;
    
    /** @brief Load compiled shader cache from disk */
    void load_shader_cache() noexcept;
    
    /** @brief Clear shader cache */
    void clear_shader_cache() noexcept;
    
    /** @brief Precompile shaders for faster loading */
    void precompile_shaders() noexcept;
    
    /** @brief Optimize shaders for current GPU */
    void optimize_for_current_gpu() noexcept;
    
    //-------------------------------------------------------------------------
    // Performance Monitoring and Statistics
    //-------------------------------------------------------------------------
    
    /** @brief Comprehensive manager statistics */
    struct Statistics {
        // Resource counts
        u32 total_shaders{0};
        u32 compiled_shaders{0};
        u32 failed_compilations{0};
        u32 shader_variants{0};
        
        // Performance metrics
        f32 total_compilation_time{0.0f};
        f32 average_compilation_time{0.0f};
        f32 worst_compilation_time{0.0f};
        u32 cache_hits{0};
        u32 cache_misses{0};
        f32 cache_hit_ratio{0.0f};
        
        // Hot reload statistics
        u32 hot_reloads_performed{0};
        u32 hot_reload_failures{0};
        f32 total_reload_time{0.0f};
        
        // Memory usage
        usize total_memory_bytes{0};
        usize cache_memory_bytes{0};
        
        // Educational insights
        const char* performance_rating{"A"};    // Letter grade A-F
        const char* optimization_suggestions[8];
        u32 suggestion_count{0};
        
        void add_suggestion(const char* suggestion) noexcept {
            if (suggestion_count < 8) {
                optimization_suggestions[suggestion_count++] = suggestion;
            }
        }
    };
    
    /** @brief Get comprehensive statistics */
    Statistics get_statistics() const noexcept;
    
    /** @brief Reset performance counters */
    void reset_statistics() noexcept;
    
    /** @brief Update statistics (call once per frame) */
    void update_statistics() noexcept;
    
    //-------------------------------------------------------------------------
    // Educational and Debug Features
    //-------------------------------------------------------------------------
    
    /** @brief Get list of all shader IDs */
    std::vector<ShaderID> get_all_shader_ids() const noexcept;
    
    /** @brief Get shader information for UI display */
    struct ShaderDisplayInfo {
        ShaderID id;
        std::string name;
        bool is_valid;
        bool has_vertex_stage;
        bool has_fragment_stage;
        u32 uniform_count;
        f32 compilation_time;
        const char* performance_rating;
    };
    
    std::vector<ShaderDisplayInfo> get_shader_list() const noexcept;
    
    /** @brief Generate comprehensive shader report */
    std::string generate_shader_report() const noexcept;
    
    /** @brief Generate performance report */
    std::string generate_performance_report() const noexcept;
    
    /** @brief Validate all shaders */
    bool validate_all_shaders() const noexcept;
    
    /** @brief Get shader compilation log */
    std::string get_compilation_log() const noexcept;
    
    //-------------------------------------------------------------------------
    // System Integration
    //-------------------------------------------------------------------------
    
    /** @brief Update manager state (call once per frame) */
    void update() noexcept;
    
    /** @brief Handle GPU context lost/restored */
    void handle_context_lost() noexcept;
    void handle_context_restored() noexcept;
    
    /** @brief Set global shader defines (affects all shaders) */
    void set_global_defines(const std::vector<std::string>& defines) noexcept;
    
    /** @brief Get current global defines */
    const std::vector<std::string>& get_global_defines() const noexcept { return global_defines_; }

private:
    //-------------------------------------------------------------------------
    // Internal Data Structures
    //-------------------------------------------------------------------------
    
    struct ShaderEntry {
        std::unique_ptr<ShaderProgram> program;
        ShaderSource source;
        std::string name;
        std::unordered_map<std::string, u64> file_timestamps;
        std::vector<ShaderID> variants;         ///< Variant shader IDs
        ShaderID base_shader{INVALID_SHADER_ID}; ///< Base shader for variants
        bool is_system_shader{false};
        
        ShaderEntry() = default;
        ShaderEntry(std::unique_ptr<ShaderProgram> prog, ShaderSource src, std::string n)
            : program(std::move(prog)), source(std::move(src)), name(std::move(n)) {}
    };
    
    Config config_;
    std::unordered_map<ShaderID, ShaderEntry> shaders_;
    std::unordered_map<std::string, ShaderID> name_to_id_;
    std::vector<std::string> global_defines_;
    
    ShaderID next_shader_id_{3}; // Start after default shaders
    mutable Statistics cached_stats_{};
    bool stats_dirty_{true};
    
    // Caching system
    std::unordered_map<std::string, std::vector<u8>> binary_cache_;
    std::unordered_map<std::string, ShaderSource> source_cache_;
    
    //-------------------------------------------------------------------------
    // Internal Methods
    //-------------------------------------------------------------------------
    
    ShaderID generate_shader_id() noexcept { return next_shader_id_++; }
    void update_cached_statistics() const noexcept;
    std::string generate_cache_key(const ShaderSource& source) const noexcept;
    void save_binary_to_cache(const std::string& key, u32 gl_program_id) noexcept;
    bool load_binary_from_cache(const std::string& key, u32 gl_program_id) noexcept;
    void create_system_shader(ShaderID id, const ShaderSource& source, const std::string& name) noexcept;
    void update_file_timestamps(ShaderID id) noexcept;
    bool check_file_changes(const std::unordered_map<std::string, u64>& timestamps) const noexcept;
    core::Result<ShaderSource, std::string> load_shader_source_with_includes(
        const std::string& vertex_path,
        const std::string& fragment_path,
        const std::string& geometry_path) const noexcept;
};

//=============================================================================
// Built-in Shaders and Default Implementations
//=============================================================================

namespace builtin_shaders {
    /** @brief Get default sprite shader source */
    ShaderSource get_default_sprite_shader() noexcept;
    
    /** @brief Get default UI shader source */
    ShaderSource get_default_ui_shader() noexcept;
    
    /** @brief Get debug wireframe shader source */
    ShaderSource get_debug_wireframe_shader() noexcept;
    
    /** @brief Get solid color shader source */
    ShaderSource get_solid_color_shader() noexcept;
    
    /** @brief Get texture visualization shader */
    ShaderSource get_texture_debug_shader() noexcept;
    
    /** @brief Get performance testing shader (heavy fragment operations) */
    ShaderSource get_performance_test_shader() noexcept;
}

//=============================================================================
// Utility Functions and Integration Helpers
//=============================================================================

namespace utils {
    /** @brief Convert ShaderID to handle for components */
    inline components::ShaderHandle shader_id_to_handle(ShaderID id) noexcept {
        return {id};
    }
    
    /** @brief Convert handle back to ShaderID */
    inline ShaderID handle_to_shader_id(const components::ShaderHandle& handle) noexcept {
        return handle.id;
    }
    
    /** @brief Parse shader stage from string */
    std::optional<ShaderStage> parse_shader_stage(const std::string& stage_name) noexcept;
    
    /** @brief Get shader stage name */
    const char* get_stage_name(ShaderStage stage) noexcept;
    
    /** @brief Parse shader data type from OpenGL type */
    ShaderDataType gl_type_to_data_type(u32 gl_type) noexcept;
    
    /** @brief Get data type size in bytes */
    u32 get_data_type_size(ShaderDataType type) noexcept;
    
    /** @brief Validate shader name */
    bool is_valid_shader_name(const std::string& name) noexcept;
    
    /** @brief Generate shader variant name */
    std::string generate_variant_name(const std::string& base_name, 
                                    const std::vector<std::string>& defines) noexcept;
    
    /** @brief Calculate memory usage for shader program */
    usize estimate_shader_memory_usage(const ShaderProgram& program) noexcept;
}

} // namespace ecscope::renderer::resources