#pragma once

/**
 * @file advanced_shader_compiler.hpp
 * @brief Advanced Shader Compiler with GLSL/HLSL/SPIR-V Support for ECScope
 * 
 * This system provides comprehensive shader compilation capabilities with support for:
 * - Cross-platform shader compilation (GLSL, HLSL, SPIR-V)
 * - Advanced preprocessing with include resolution
 * - Multi-target code generation and optimization
 * - Real-time compilation and hot-reload
 * - Shader analysis and performance profiling
 * - Binary caching and optimization
 * - Visual shader graph compilation
 * 
 * @author ECScope Educational ECS Framework - Advanced Shader System
 * @date 2024
 */

#include "shader.hpp"
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <optional>
#include <functional>
#include <thread>
#include <mutex>
#include <atomic>
#include <chrono>

namespace ecscope::renderer::shader_compiler {

//=============================================================================
// Shader Compilation Targets and Languages
//=============================================================================

enum class ShaderLanguage : u8 {
    GLSL = 0,           ///< OpenGL Shading Language
    HLSL,               ///< DirectX High Level Shading Language
    SPIRV,              ///< SPIR-V Intermediate Representation
    MSL,                ///< Metal Shading Language (macOS/iOS)
    WGSL,               ///< WebGPU Shading Language
    CUDA,               ///< CUDA C++ (for compute shaders)
    OpenCL              ///< OpenCL C (for compute kernels)
};

enum class CompilationTarget : u8 {
    OpenGL_3_3 = 0,     ///< OpenGL 3.3 Core Profile
    OpenGL_4_0,         ///< OpenGL 4.0 Core Profile
    OpenGL_4_5,         ///< OpenGL 4.5 Core Profile
    OpenGL_4_6,         ///< OpenGL 4.6 Core Profile
    Vulkan_1_0,         ///< Vulkan 1.0
    Vulkan_1_1,         ///< Vulkan 1.1
    Vulkan_1_2,         ///< Vulkan 1.2
    Vulkan_1_3,         ///< Vulkan 1.3
    DirectX_11,         ///< DirectX 11
    DirectX_12,         ///< DirectX 12
    Metal_2_0,          ///< Metal 2.0
    Metal_3_0,          ///< Metal 3.0
    WebGL_1_0,          ///< WebGL 1.0
    WebGL_2_0,          ///< WebGL 2.0
    WebGPU              ///< WebGPU
};

enum class OptimizationLevel : u8 {
    Debug = 0,          ///< No optimization, full debug info
    Development,        ///< Basic optimization, some debug info
    Release,            ///< Full optimization, minimal debug info
    Size,               ///< Optimize for size
    Performance,        ///< Optimize for performance
    Custom              ///< Custom optimization settings
};

//=============================================================================
// Shader Compilation Configuration
//=============================================================================

struct CompilerConfig {
    // Target configuration
    CompilationTarget target{CompilationTarget::OpenGL_4_5};
    ShaderLanguage source_language{ShaderLanguage::GLSL};
    ShaderLanguage output_language{ShaderLanguage::GLSL};
    OptimizationLevel optimization{OptimizationLevel::Development};
    
    // Compilation flags
    bool enable_debug_info{true};
    bool enable_validation{true};
    bool enable_warnings{true};
    bool treat_warnings_as_errors{false};
    bool enable_aggressive_optimization{false};
    
    // Include and preprocessing
    std::vector<std::string> include_paths;
    std::vector<std::string> preprocessor_defines;
    std::unordered_map<std::string, std::string> macro_definitions;
    
    // Output configuration
    bool generate_reflection_data{true};
    bool generate_assembly_output{false};
    bool generate_optimization_report{false};
    bool enable_binary_cache{true};
    std::string cache_directory{"shader_cache"};
    
    // Shader-specific settings
    struct GLSLConfig {
        u32 version{450};
        bool enable_extensions{true};
        bool enable_spirv_cross_compilation{true};
        std::vector<std::string> enabled_extensions;
    } glsl;
    
    struct HLSLConfig {
        std::string shader_model{"5_0"};
        bool enable_16bit_types{false};
        bool enable_matrix_packing{true};
        std::string entry_point{"main"};
    } hlsl;
    
    struct SPIRVConfig {
        u32 version{0x00010000}; // 1.0
        bool enable_validation{true};
        bool enable_optimization{true};
        bool generate_debug_info{false};
    } spirv;
};

//=============================================================================
// Compilation Results and Diagnostics
//=============================================================================

struct CompilationDiagnostic {
    enum class Severity {
        Info = 0,
        Warning,
        Error,
        Fatal
    };
    
    Severity severity{Severity::Info};
    std::string message;
    std::string file_path;
    u32 line{0};
    u32 column{0};
    u32 error_code{0};
    std::string suggested_fix;
    
    CompilationDiagnostic() = default;
    CompilationDiagnostic(Severity sev, std::string msg, std::string file = "", u32 ln = 0, u32 col = 0)
        : severity(sev), message(std::move(msg)), file_path(std::move(file)), line(ln), column(col) {}
};

struct CompilationResult {
    bool success{false};
    std::vector<u8> bytecode;                   ///< Compiled shader bytecode
    std::string assembly_code;                  ///< Human-readable assembly
    std::string preprocessed_source;            ///< Source after preprocessing
    std::vector<CompilationDiagnostic> diagnostics;
    
    // Performance and analysis data
    struct PerformanceInfo {
        f32 compilation_time{0.0f};
        f32 optimization_time{0.0f};
        u32 instruction_count{0};
        u32 register_usage{0};
        u32 constant_buffer_usage{0};
        f32 estimated_gpu_cost{1.0f};
        std::string performance_analysis;
    } performance;
    
    // Reflection data
    struct ReflectionData {
        std::vector<resources::UniformInfo> uniforms;
        std::vector<resources::UniformBufferLayout> uniform_buffers;
        std::vector<std::string> samplers;
        std::vector<std::string> storage_buffers;
        u32 local_size_x{1}, local_size_y{1}, local_size_z{1}; // For compute shaders
        std::unordered_map<std::string, std::string> attributes;
    } reflection;
    
    // Cache information
    bool loaded_from_cache{false};
    std::string cache_key;
    
    void add_diagnostic(CompilationDiagnostic::Severity severity, const std::string& message,
                       const std::string& file = "", u32 line = 0, u32 column = 0) {
        diagnostics.emplace_back(severity, message, file, line, column);
    }
    
    bool has_errors() const {
        return std::any_of(diagnostics.begin(), diagnostics.end(),
                          [](const auto& diag) { return diag.severity >= CompilationDiagnostic::Severity::Error; });
    }
    
    bool has_warnings() const {
        return std::any_of(diagnostics.begin(), diagnostics.end(),
                          [](const auto& diag) { return diag.severity == CompilationDiagnostic::Severity::Warning; });
    }
    
    std::string get_diagnostic_summary() const {
        if (diagnostics.empty()) return "No issues";
        
        u32 errors = 0, warnings = 0;
        for (const auto& diag : diagnostics) {
            if (diag.severity >= CompilationDiagnostic::Severity::Error) errors++;
            else if (diag.severity == CompilationDiagnostic::Severity::Warning) warnings++;
        }
        
        return std::to_string(errors) + " errors, " + std::to_string(warnings) + " warnings";
    }
};

//=============================================================================
// Shader Preprocessor System
//=============================================================================

class ShaderPreprocessor {
public:
    struct PreprocessorResult {
        bool success{false};
        std::string processed_source;
        std::vector<std::string> included_files;
        std::vector<CompilationDiagnostic> diagnostics;
        std::unordered_map<std::string, std::string> resolved_macros;
    };
    
    explicit ShaderPreprocessor(const CompilerConfig& config) : config_(config) {}
    
    PreprocessorResult process(const std::string& source, const std::string& source_file = "") const;
    
    void add_include_path(const std::string& path) { include_paths_.push_back(path); }
    void add_define(const std::string& name, const std::string& value = "") {
        defines_[name] = value;
    }
    
    void set_base_defines_for_target(CompilationTarget target);
    
private:
    const CompilerConfig& config_;
    mutable std::vector<std::string> include_paths_;
    mutable std::unordered_map<std::string, std::string> defines_;
    
    std::string resolve_include(const std::string& include_path, const std::string& current_file) const;
    std::string process_line(const std::string& line, const std::string& current_file,
                           std::vector<CompilationDiagnostic>& diagnostics) const;
    std::string expand_macros(const std::string& text) const;
    bool is_condition_true(const std::string& condition) const;
};

//=============================================================================
// Cross-Platform Shader Compiler
//=============================================================================

class AdvancedShaderCompiler {
public:
    using CompilationCallback = std::function<void(const CompilationResult&)>;
    
    explicit AdvancedShaderCompiler(const CompilerConfig& config = CompilerConfig{});
    ~AdvancedShaderCompiler();
    
    // Configuration management
    void set_config(const CompilerConfig& config) { config_ = config; }
    const CompilerConfig& get_config() const { return config_; }
    
    // Synchronous compilation
    CompilationResult compile_shader(const std::string& source, 
                                   resources::ShaderStage stage,
                                   const std::string& entry_point = "main",
                                   const std::string& source_file = "");
    
    CompilationResult compile_compute_shader(const std::string& source,
                                           const std::string& entry_point = "main",
                                           const std::string& source_file = "");
    
    // Multi-stage compilation
    struct MultiStageResult {
        bool success{false};
        std::unordered_map<resources::ShaderStage, CompilationResult> stage_results;
        std::string combined_cache_key;
        f32 total_compilation_time{0.0f};
    };
    
    MultiStageResult compile_multi_stage(const std::unordered_map<resources::ShaderStage, std::string>& sources,
                                       const std::string& base_source_file = "");
    
    // Asynchronous compilation
    struct AsyncCompilationHandle {
        u64 handle_id;
        std::atomic<bool> is_complete{false};
        std::atomic<f32> progress{0.0f};
        std::string status_message;
        std::mutex result_mutex;
        std::optional<CompilationResult> result;
    };
    
    std::shared_ptr<AsyncCompilationHandle> compile_shader_async(
        const std::string& source,
        resources::ShaderStage stage,
        const std::string& entry_point = "main",
        const std::string& source_file = "",
        CompilationCallback callback = nullptr);
    
    bool is_compilation_complete(std::shared_ptr<AsyncCompilationHandle> handle) const;
    std::optional<CompilationResult> get_compilation_result(std::shared_ptr<AsyncCompilationHandle> handle);
    
    // Cross-compilation utilities
    CompilationResult cross_compile(const std::vector<u8>& spirv_bytecode,
                                  ShaderLanguage target_language,
                                  resources::ShaderStage stage);
    
    CompilationResult transpile_hlsl_to_glsl(const std::string& hlsl_source,
                                           resources::ShaderStage stage,
                                           const std::string& entry_point = "main");
    
    CompilationResult transpile_glsl_to_hlsl(const std::string& glsl_source,
                                           resources::ShaderStage stage);
    
    // Optimization and analysis
    CompilationResult optimize_shader(const CompilationResult& base_result,
                                    OptimizationLevel level = OptimizationLevel::Release);
    
    std::string analyze_shader_performance(const CompilationResult& result) const;
    std::vector<std::string> suggest_optimizations(const CompilationResult& result) const;
    
    // Cache management
    void enable_caching(bool enabled) { enable_cache_ = enabled; }
    bool is_caching_enabled() const { return enable_cache_; }
    void clear_cache();
    void precompile_and_cache(const std::vector<std::pair<std::string, resources::ShaderStage>>& shaders);
    
    // Statistics and monitoring
    struct CompilerStatistics {
        u32 total_compilations{0};
        u32 successful_compilations{0};
        u32 failed_compilations{0};
        u32 cache_hits{0};
        u32 cache_misses{0};
        f32 total_compilation_time{0.0f};
        f32 average_compilation_time{0.0f};
        f32 cache_hit_ratio{0.0f};
        usize cache_memory_usage{0};
        
        std::unordered_map<resources::ShaderStage, u32> compilations_per_stage;
        std::unordered_map<CompilationTarget, u32> compilations_per_target;
    };
    
    const CompilerStatistics& get_statistics() const { return stats_; }
    void reset_statistics() { stats_ = CompilerStatistics{}; }
    
    // Error reporting and debugging
    std::string format_diagnostics(const std::vector<CompilationDiagnostic>& diagnostics) const;
    std::string generate_compilation_report(const CompilationResult& result) const;
    void set_debug_output_enabled(bool enabled) { debug_output_ = enabled; }
    
private:
    CompilerConfig config_;
    std::unique_ptr<ShaderPreprocessor> preprocessor_;
    
    // Async compilation
    std::vector<std::thread> worker_threads_;
    std::atomic<bool> shutdown_requested_{false};
    std::atomic<u64> next_handle_id_{1};
    
    // Caching system
    bool enable_cache_{true};
    std::unordered_map<std::string, CompilationResult> cache_;
    std::mutex cache_mutex_;
    
    // Statistics
    mutable CompilerStatistics stats_;
    mutable std::mutex stats_mutex_;
    
    // Debug and logging
    bool debug_output_{false};
    
    // Platform-specific compiler interfaces
    CompilationResult compile_glsl(const std::string& source, resources::ShaderStage stage,
                                 const std::string& entry_point, const std::string& source_file);
    CompilationResult compile_hlsl(const std::string& source, resources::ShaderStage stage,
                                 const std::string& entry_point, const std::string& source_file);
    CompilationResult compile_to_spirv(const std::string& source, ShaderLanguage source_lang,
                                     resources::ShaderStage stage, const std::string& entry_point);
    
    // Internal utilities
    std::string generate_cache_key(const std::string& source, resources::ShaderStage stage,
                                 const std::string& entry_point, const CompilerConfig& config) const;
    void update_statistics(const CompilationResult& result, bool cache_hit) const;
    void log_compilation_info(const std::string& message) const;
    void worker_thread_function();
    
    // Shader reflection and analysis
    void extract_reflection_data(CompilationResult& result, const std::vector<u8>& bytecode) const;
    void analyze_performance_metrics(CompilationResult& result, const std::vector<u8>& bytecode) const;
    
    // Platform detection and capability querying
    bool is_platform_supported(CompilationTarget target) const;
    std::vector<std::string> get_available_extensions(CompilationTarget target) const;
    
    // Error handling and validation
    bool validate_source_syntax(const std::string& source, ShaderLanguage language,
                              std::vector<CompilationDiagnostic>& diagnostics) const;
    std::string enhance_error_message(const std::string& original_error,
                                    const std::string& source_context) const;
};

//=============================================================================
// Shader Graph Compiler (for visual node-based editing)
//=============================================================================

struct ShaderNode {
    enum class NodeType {
        Input,              ///< Input node (vertex data, uniforms, etc.)
        Output,             ///< Output node (final color, position, etc.)
        Math,               ///< Mathematical operations
        Texture,            ///< Texture sampling
        Function,           ///< Custom functions
        Conditional,        ///< Conditional logic
        Loop,               ///< Loop constructs
        Custom              ///< Custom user-defined nodes
    };
    
    u32 id;
    NodeType type;
    std::string name;
    std::string operation; // For math nodes: "add", "multiply", etc.
    std::unordered_map<std::string, std::string> parameters;
    std::vector<u32> input_connections;
    std::vector<u32> output_connections;
    
    // Visual editor properties
    f32 x_position{0.0f}, y_position{0.0f};
    bool is_selected{false};
    bool is_valid{true};
};

struct ShaderConnection {
    u32 from_node;
    std::string from_output;
    u32 to_node;
    std::string to_input;
    std::string data_type; // "float", "vec3", "mat4", etc.
    bool is_valid{true};
};

class ShaderGraph {
public:
    void add_node(const ShaderNode& node) { nodes_[node.id] = node; }
    void remove_node(u32 node_id);
    void add_connection(const ShaderConnection& connection);
    void remove_connection(u32 from_node, const std::string& from_output,
                          u32 to_node, const std::string& to_input);
    
    bool validate_graph() const;
    std::string compile_to_glsl(resources::ShaderStage stage) const;
    std::string compile_to_hlsl(resources::ShaderStage stage) const;
    
    const std::unordered_map<u32, ShaderNode>& get_nodes() const { return nodes_; }
    const std::vector<ShaderConnection>& get_connections() const { return connections_; }
    
    // Serialization for saving/loading graphs
    std::string serialize_to_json() const;
    bool deserialize_from_json(const std::string& json);
    
private:
    std::unordered_map<u32, ShaderNode> nodes_;
    std::vector<ShaderConnection> connections_;
    u32 next_node_id_{1};
    
    std::vector<u32> get_execution_order() const;
    std::string generate_node_code(const ShaderNode& node, resources::ShaderStage stage) const;
    std::string get_node_input_value(u32 node_id, const std::string& input_name) const;
    bool has_cycles() const;
};

class ShaderGraphCompiler {
public:
    explicit ShaderGraphCompiler(AdvancedShaderCompiler* base_compiler)
        : compiler_(base_compiler) {}
    
    CompilationResult compile_graph(const ShaderGraph& graph,
                                  resources::ShaderStage stage,
                                  const std::string& graph_name = "");
    
    MultiStageResult compile_multi_stage_graph(
        const std::unordered_map<resources::ShaderStage, ShaderGraph>& graphs,
        const std::string& base_name = "");
    
    // Graph optimization
    ShaderGraph optimize_graph(const ShaderGraph& input_graph) const;
    std::vector<std::string> analyze_graph_performance(const ShaderGraph& graph) const;
    
private:
    AdvancedShaderCompiler* compiler_;
    
    void optimize_dead_code(ShaderGraph& graph) const;
    void optimize_constant_folding(ShaderGraph& graph) const;
    void optimize_common_subexpressions(ShaderGraph& graph) const;
};

//=============================================================================
// Utility Functions and Integration
//=============================================================================

namespace utils {
    const char* shader_language_to_string(ShaderLanguage lang);
    const char* compilation_target_to_string(CompilationTarget target);
    const char* optimization_level_to_string(OptimizationLevel level);
    
    std::optional<ShaderLanguage> string_to_shader_language(const std::string& str);
    std::optional<CompilationTarget> string_to_compilation_target(const std::string& str);
    std::optional<OptimizationLevel> string_to_optimization_level(const std::string& str);
    
    // Platform-specific utilities
    bool is_glsl_supported();
    bool is_hlsl_supported();
    bool is_spirv_supported();
    bool is_vulkan_available();
    bool is_directx_available();
    
    // File format detection
    ShaderLanguage detect_shader_language_from_extension(const std::string& file_path);
    std::string get_default_extension_for_language(ShaderLanguage lang);
    
    // Configuration helpers
    CompilerConfig create_config_for_target(CompilationTarget target);
    CompilerConfig create_development_config();
    CompilerConfig create_release_config();
    CompilerConfig create_debug_config();
    
    // Performance utilities
    std::string format_compilation_time(f32 time_seconds);
    std::string format_memory_usage(usize bytes);
    f32 estimate_shader_complexity(const std::string& source);
}

} // namespace ecscope::renderer::shader_compiler