/**
 * @file simple_shader_compiler.hpp
 * @brief Simple Shader Compiler for ECScope - Clean and Focused
 * 
 * Provides essential shader compilation for ECScope with:
 * - GLSL compilation and validation
 * - Basic error reporting
 * - Shader caching
 * - Simple preprocessor
 * 
 * @author ECScope Educational ECS Framework
 * @date 2024
 */

#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <optional>

namespace ecscope::renderer {

/**
 * @brief Shader compilation target
 */
enum class ShaderType {
    Vertex,
    Fragment,
    Compute
};

/**
 * @brief Compilation result
 */
struct CompilationResult {
    bool success = false;
    std::string error_message;
    std::vector<uint8_t> bytecode;
    uint32_t shader_id = 0;
    
    // Performance info
    float compilation_time_ms = 0.0f;
    uint32_t instruction_count = 0;
    
    bool has_error() const { return !success || !error_message.empty(); }
};

/**
 * @brief Simple shader compiler for GLSL
 */
class SimpleShaderCompiler {
public:
    /**
     * @brief Compile GLSL shader source
     * @param source Shader source code
     * @param type Shader type
     * @param entry_point Entry point function name (default: "main")
     * @return Compilation result
     */
    CompilationResult compile(const std::string& source, 
                            ShaderType type, 
                            const std::string& entry_point = "main");
    
    /**
     * @brief Compile shader from file
     * @param file_path Path to shader file
     * @param type Shader type
     * @return Compilation result
     */
    CompilationResult compile_from_file(const std::string& file_path, ShaderType type);
    
    /**
     * @brief Enable or disable caching
     */
    void enable_caching(bool enabled) { caching_enabled_ = enabled; }
    
    /**
     * @brief Clear shader cache
     */
    void clear_cache();
    
    /**
     * @brief Get compilation statistics
     */
    struct Statistics {
        uint32_t total_compilations = 0;
        uint32_t cache_hits = 0;
        uint32_t errors = 0;
        float total_time_ms = 0.0f;
    };
    
    const Statistics& get_stats() const { return stats_; }
    
    /**
     * @brief Add preprocessor define
     */
    void add_define(const std::string& name, const std::string& value = "");
    
    /**
     * @brief Remove preprocessor define
     */
    void remove_define(const std::string& name);
    
    /**
     * @brief Set GLSL version (default: 330)
     */
    void set_glsl_version(uint32_t version) { glsl_version_ = version; }

private:
    bool caching_enabled_ = true;
    uint32_t glsl_version_ = 330;
    Statistics stats_;
    
    // Cache: source hash -> compilation result
    std::unordered_map<std::string, CompilationResult> cache_;
    
    // Preprocessor defines
    std::unordered_map<std::string, std::string> defines_;
    
    /**
     * @brief Preprocess shader source
     */
    std::string preprocess(const std::string& source);
    
    /**
     * @brief Generate cache key for source
     */
    std::string generate_cache_key(const std::string& source, ShaderType type);
    
    /**
     * @brief Perform actual OpenGL compilation
     */
    CompilationResult compile_opengl(const std::string& processed_source, ShaderType type);
    
    /**
     * @brief Convert ShaderType to OpenGL enum
     */
    uint32_t shader_type_to_gl(ShaderType type);
    
    /**
     * @brief Validate shader source syntax
     */
    bool validate_syntax(const std::string& source, std::string& error);
};

/**
 * @brief Utility functions
 */
namespace shader_utils {
    
/**
 * @brief Convert shader type to string
 */
const char* shader_type_to_string(ShaderType type);

/**
 * @brief Detect shader type from file extension
 */
std::optional<ShaderType> detect_type_from_extension(const std::string& file_path);

/**
 * @brief Load shader source from file
 */
std::optional<std::string> load_shader_source(const std::string& file_path);

/**
 * @brief Create basic vertex shader template
 */
std::string create_basic_vertex_shader();

/**
 * @brief Create basic fragment shader template
 */
std::string create_basic_fragment_shader();

} // namespace shader_utils

} // namespace ecscope::renderer