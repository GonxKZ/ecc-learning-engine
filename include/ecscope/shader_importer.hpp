#pragma once

/**
 * @file shader_importer.hpp
 * @brief Advanced Shader Import System for ECScope Asset Pipeline
 * 
 * This system provides comprehensive shader import capabilities with validation,
 * compilation, optimization, and educational features for teaching graphics
 * programming concepts.
 * 
 * Key Features:
 * - Multi-language support (GLSL, HLSL, SPIR-V)
 * - Advanced shader compilation and validation
 * - Cross-compilation and optimization
 * - Educational analysis and learning tools
 * - Integration with rendering system
 * - Real-time shader editing and hot-reload
 * 
 * Educational Value:
 * - Demonstrates shader compilation pipeline
 * - Shows graphics API differences
 * - Illustrates optimization techniques
 * - Provides shader analysis tools
 * - Teaches GPU programming concepts
 * 
 * @author ECScope Educational ECS Framework - Asset Pipeline
 * @date 2025
 */

#include "asset_pipeline.hpp"
#include "core/types.hpp"
#include <vector>
#include <string>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <optional>
#include <variant>

namespace ecscope::assets::importers {

//=============================================================================
// Shader Language and Stage Definitions
//=============================================================================

/**
 * @brief Supported shader languages
 */
enum class ShaderLanguage : u8 {
    Unknown = 0,
    GLSL,           // OpenGL Shading Language
    HLSL,           // High-Level Shading Language (DirectX)
    SPIRV,          // Standard Portable Intermediate Representation - V
    MSL,            // Metal Shading Language (Apple)
    WGSL            // WebGPU Shading Language
};

/**
 * @brief Graphics pipeline stages
 */
enum class ShaderStage : u8 {
    Unknown = 0,
    Vertex,         // Vertex processing
    Fragment,       // Fragment/pixel processing
    Geometry,       // Geometry processing (optional)
    TessControl,    // Tessellation control (optional)
    TessEvaluation, // Tessellation evaluation (optional)
    Compute,        // Compute shaders
    RayGeneration,  // Ray tracing: ray generation
    RayMiss,        // Ray tracing: miss shader
    RayClosestHit,  // Ray tracing: closest hit
    RayAnyHit,      // Ray tracing: any hit
    RayIntersection // Ray tracing: intersection
};

/**
 * @brief Shader profile/version information
 */
struct ShaderProfile {
    ShaderLanguage language;
    u16 major_version{0};
    u16 minor_version{0};
    std::string profile_name; // e.g., "core", "compatibility", "es"
    std::string target_api;   // e.g., "opengl", "vulkan", "directx11"
    
    ShaderProfile() = default;
    ShaderProfile(ShaderLanguage lang, u16 major, u16 minor, const std::string& profile = "")
        : language(lang), major_version(major), minor_version(minor), profile_name(profile) {}
    
    std::string to_string() const;
    bool is_compatible_with(const ShaderProfile& other) const;
    
    static ShaderProfile parse(const std::string& version_string);
    static std::vector<ShaderProfile> get_supported_profiles();
};

//=============================================================================
// Shader Data Structures
//=============================================================================

/**
 * @brief Shader source code with metadata
 */
struct ShaderSource {
    std::string source_code;
    ShaderStage stage{ShaderStage::Unknown};
    ShaderLanguage language{ShaderLanguage::Unknown};
    ShaderProfile profile;
    
    // Include dependencies
    std::vector<std::string> includes;
    std::vector<std::string> include_paths;
    
    // Preprocessor definitions
    std::unordered_map<std::string, std::string> defines;
    
    // Source metadata
    std::string entry_point{"main"};
    std::filesystem::path source_file;
    std::vector<std::string> source_lines; // For error reporting
    
    // Validation
    bool is_valid() const;
    std::vector<std::string> get_dependencies() const;
    std::string preprocess() const;
};

/**
 * @brief Compiled shader with bytecode and reflection data
 */
struct CompiledShader {
    // Compiled bytecode
    std::vector<u8> bytecode;
    ShaderStage stage;
    ShaderLanguage source_language;
    ShaderLanguage target_language;
    
    // Compilation information
    std::string entry_point;
    std::string compiler_version;
    std::vector<std::string> compilation_flags;
    f64 compilation_time_ms{0.0};
    
    // Shader reflection data
    struct ReflectionData {
        // Uniforms/Constants
        struct UniformInfo {
            std::string name;
            std::string type;
            u32 location{0};
            u32 binding{0};
            u32 set{0}; // Descriptor set (Vulkan)
            usize size{0};
            usize offset{0};
            bool is_array{false};
            u32 array_size{1};
        };
        std::vector<UniformInfo> uniforms;
        
        // Vertex attributes (for vertex shaders)
        struct AttributeInfo {
            std::string name;
            std::string type;
            u32 location{0};
            usize size{0};
            bool is_builtin{false};
        };
        std::vector<AttributeInfo> vertex_inputs;
        std::vector<AttributeInfo> vertex_outputs;
        
        // Texture samplers
        struct SamplerInfo {
            std::string name;
            std::string type; // sampler2D, samplerCube, etc.
            u32 binding{0};
            u32 set{0};
            bool is_array{false};
            u32 array_size{1};
        };
        std::vector<SamplerInfo> samplers;
        
        // Storage buffers (compute/modern graphics)
        struct StorageInfo {
            std::string name;
            std::string type;
            u32 binding{0};
            u32 set{0};
            bool read_only{false};
            bool write_only{false};
        };
        std::vector<StorageInfo> storage_buffers;
        
        // Push constants (Vulkan)
        struct PushConstantInfo {
            std::string name;
            usize size{0};
            usize offset{0};
        };
        std::vector<PushConstantInfo> push_constants;
        
        // Local workgroup size (compute shaders)
        u32 local_size_x{1};
        u32 local_size_y{1};
        u32 local_size_z{1};
        
    } reflection;
    
    // Performance analysis
    struct PerformanceInfo {
        u32 instruction_count{0};
        u32 texture_reads{0};
        u32 arithmetic_operations{0};
        u32 control_flow_operations{0};
        f32 estimated_cycles{0.0f};
        f32 register_pressure{0.0f};
        std::vector<std::string> performance_warnings;
        std::vector<std::string> optimization_suggestions;
    } performance;
    
    usize get_memory_usage() const;
    bool is_valid() const;
};

/**
 * @brief Complete shader program with all stages
 */
struct ShaderProgram {
    std::string name;
    std::unordered_map<ShaderStage, CompiledShader> shaders;
    
    // Program linking information
    bool is_linked{false};
    std::vector<std::string> link_errors;
    std::vector<std::string> link_warnings;
    
    // Combined reflection data
    std::vector<CompiledShader::ReflectionData::UniformInfo> all_uniforms;
    std::vector<CompiledShader::ReflectionData::SamplerInfo> all_samplers;
    
    // Educational metadata
    std::string purpose_description;
    std::vector<std::string> techniques_demonstrated;
    std::string complexity_level; // "Beginner", "Intermediate", "Advanced"
    std::vector<std::string> learning_objectives;
    
    bool has_stage(ShaderStage stage) const;
    CompiledShader* get_shader(ShaderStage stage);
    const CompiledShader* get_shader(ShaderStage stage) const;
    
    usize get_total_memory_usage() const;
    bool validate_program() const;
};

//=============================================================================
// Shader Analysis and Educational Features
//=============================================================================

/**
 * @brief Comprehensive shader analysis for educational purposes
 */
struct ShaderAnalysis {
    // Source code analysis
    struct SourceAnalysis {
        u32 line_count{0};
        u32 function_count{0};
        u32 variable_count{0};
        u32 texture_sample_count{0};
        u32 conditional_branches{0};
        u32 loop_count{0};
        
        // Complexity metrics
        f32 cyclomatic_complexity{1.0f};
        f32 instruction_complexity{1.0f};
        f32 data_dependency_complexity{1.0f};
        
        // Code quality metrics
        bool has_unused_variables{false};
        bool has_unused_functions{false};
        bool has_dead_code{false};
        std::vector<std::string> style_issues;
        
    } source;
    
    // Performance analysis
    struct PerformanceAnalysis {
        // Estimated performance costs
        f32 vertex_cost{1.0f};        // For vertex shaders
        f32 fragment_cost{1.0f};      // For fragment shaders
        f32 texture_bandwidth_cost{1.0f};
        f32 arithmetic_cost{1.0f};
        f32 memory_bandwidth_cost{1.0f};
        
        // Bottleneck analysis
        enum class BottleneckType {
            None, TextureBandwidth, ArithmeticIntensity, 
            MemoryBandwidth, VertexThroughput, FragmentThroughput
        } likely_bottleneck{BottleneckType::None};
        
        // Platform compatibility
        bool suitable_for_mobile{true};
        bool suitable_for_integrated_gpu{true};
        bool requires_high_end_gpu{false};
        
        std::vector<std::string> performance_warnings;
        std::vector<std::string> optimization_opportunities;
        
    } performance;
    
    // Educational insights
    struct EducationalInsights {
        // Concepts demonstrated
        std::vector<std::string> graphics_concepts;
        std::vector<std::string> math_concepts;
        std::vector<std::string> programming_concepts;
        
        // Difficulty assessment
        enum class DifficultyLevel {
            Beginner, Intermediate, Advanced, Expert
        } difficulty{DifficultyLevel::Beginner};
        
        std::string difficulty_explanation;
        std::vector<std::string> prerequisites;
        std::vector<std::string> learning_outcomes;
        
        // Teaching opportunities
        std::vector<std::string> explanation_points;
        std::vector<std::string> interactive_exercises;
        std::string visualization_suggestions;
        
        f32 educational_value{0.5f}; // 0.0-1.0
        
    } educational;
    
    // Quality assessment
    struct QualityMetrics {
        f32 overall_quality{1.0f};
        std::vector<std::string> quality_issues;
        std::vector<std::string> best_practices_violations;
        
        bool follows_naming_conventions{true};
        bool has_proper_documentation{false};
        bool uses_modern_features{true};
        
        std::string maintainability_rating; // "Poor", "Fair", "Good", "Excellent"
        
    } quality;
};

/**
 * @brief Shader code analyzer for educational insights
 */
class ShaderAnalyzer {
private:
    // AST parsing for detailed analysis
    struct ASTNode {
        enum class NodeType {
            Function, Variable, Expression, Statement, Block
        } type;
        
        std::string name;
        std::string data_type;
        std::vector<std::unique_ptr<ASTNode>> children;
        u32 line_number{0};
        u32 column{0};
    };
    
    // Pattern recognition for educational concepts
    std::unordered_map<std::string, std::vector<std::string>> concept_patterns_;
    
public:
    ShaderAnalyzer();
    
    // Main analysis function
    ShaderAnalysis analyze_shader(const ShaderSource& source) const;
    ShaderAnalysis analyze_program(const ShaderProgram& program) const;
    
    // Specific analysis functions
    void analyze_source_complexity(const ShaderSource& source, ShaderAnalysis::SourceAnalysis& analysis) const;
    void analyze_performance_characteristics(const ShaderSource& source, ShaderAnalysis::PerformanceAnalysis& analysis) const;
    void generate_educational_insights(const ShaderSource& source, ShaderAnalysis::EducationalInsights& insights) const;
    void assess_code_quality(const ShaderSource& source, ShaderAnalysis::QualityMetrics& quality) const;
    
    // Educational content generation
    std::string generate_concept_explanation(const ShaderSource& source) const;
    std::string generate_optimization_guide(const ShaderSource& source) const;
    std::vector<std::string> suggest_exercises(const ShaderSource& source) const;
    
    // Interactive analysis for real-time feedback
    struct RealTimeAnalysis {
        std::vector<std::string> current_errors;
        std::vector<std::string> current_warnings;
        std::vector<std::string> suggestions;
        f32 completion_percentage{0.0f};
        bool is_syntactically_correct{false};
    };
    
    RealTimeAnalysis analyze_in_progress(const std::string& partial_source, ShaderStage stage) const;
    
private:
    void initialize_concept_patterns();
    std::unique_ptr<ASTNode> parse_shader_ast(const std::string& source) const;
    void detect_graphics_concepts(const ASTNode* ast, std::vector<std::string>& concepts) const;
    f32 calculate_instruction_complexity(const std::string& source) const;
    std::vector<std::string> detect_optimization_opportunities(const std::string& source) const;
};

//=============================================================================
// Shader Compiler Integration
//=============================================================================

/**
 * @brief Shader compiler wrapper with multiple backend support
 */
class ShaderCompiler {
public:
    // Compilation options
    struct CompilationOptions {
        ShaderLanguage target_language{ShaderLanguage::SPIRV};
        ShaderProfile target_profile;
        std::vector<std::string> defines;
        std::vector<std::string> include_paths;
        
        // Optimization settings
        bool optimize{true};
        bool debug_info{false};
        bool warnings_as_errors{false};
        u32 optimization_level{2}; // 0=none, 1=basic, 2=full, 3=aggressive
        
        // Educational settings
        bool generate_assembly{false};
        bool generate_reflection{true};
        bool generate_performance_info{true};
        
        // Validation settings
        bool strict_validation{true};
        bool validate_spirv{true};
    };
    
    // Compilation result
    struct CompilationResult {
        bool success{false};
        CompiledShader compiled_shader;
        
        std::vector<std::string> errors;
        std::vector<std::string> warnings;
        std::vector<std::string> info_messages;
        
        // Additional outputs for education
        std::string assembly_code;        // Human-readable assembly
        std::string optimization_log;     // What optimizations were applied
        std::string validation_report;    // Detailed validation results
        
        f64 compilation_time_ms{0.0};
    };
    
    // Backend compiler support
    enum class CompilerBackend {
        Default,    // Use best available compiler for target
        GLSLang,    // Reference GLSL compiler
        DXC,        // DirectX Shader Compiler
        SPIRV_Cross,// SPIRV-Cross for cross-compilation
        Shaderc,    // Google's shader compiler
        Custom      // User-provided compiler
    };
    
private:
    CompilerBackend preferred_backend_{CompilerBackend::Default};
    std::string custom_compiler_path_;
    
public:
    ShaderCompiler(CompilerBackend backend = CompilerBackend::Default);
    ~ShaderCompiler() = default;
    
    // Main compilation interface
    CompilationResult compile_shader(
        const ShaderSource& source,
        const CompilationOptions& options = CompilationOptions{}
    ) const;
    
    // Batch compilation
    std::vector<CompilationResult> compile_program(
        const std::vector<ShaderSource>& sources,
        const CompilationOptions& options = CompilationOptions{}
    ) const;
    
    // Cross-compilation
    CompilationResult cross_compile(
        const CompiledShader& spirv_shader,
        ShaderLanguage target_language,
        const ShaderProfile& target_profile = ShaderProfile{}
    ) const;
    
    // Validation only (no bytecode generation)
    struct ValidationResult {
        bool is_valid{false};
        std::vector<std::string> errors;
        std::vector<std::string> warnings;
        ShaderAnalysis analysis;
    };
    
    ValidationResult validate_shader(
        const ShaderSource& source,
        bool perform_analysis = true
    ) const;
    
    // Educational utilities
    std::string disassemble_shader(const CompiledShader& shader) const;
    std::string generate_compilation_explanation(const CompilationResult& result) const;
    
    // Capability queries
    bool supports_language(ShaderLanguage language) const;
    bool supports_stage(ShaderStage stage) const;
    std::vector<ShaderProfile> get_supported_profiles() const;
    
    // Configuration
    void set_backend(CompilerBackend backend);
    void set_custom_compiler_path(const std::string& path);
    void add_include_path(const std::string& path);
    
private:
    // Backend-specific implementations
    CompilationResult compile_with_glslang(const ShaderSource& source, const CompilationOptions& options) const;
    CompilationResult compile_with_dxc(const ShaderSource& source, const CompilationOptions& options) const;
    CompilationResult compile_with_shaderc(const ShaderSource& source, const CompilationOptions& options) const;
    
    CompilerBackend select_best_backend(ShaderLanguage source_lang, ShaderLanguage target_lang) const;
    std::string format_error_messages(const std::vector<std::string>& errors, const ShaderSource& source) const;
};

//=============================================================================
// Shader Import Settings
//=============================================================================

/**
 * @brief Extended shader import settings
 */
struct ShaderImportSettings : public ImportSettings {
    ShaderStage stage{ShaderStage::Unknown};
    ShaderLanguage target_language{ShaderLanguage::SPIRV};
    ShaderProfile target_profile;
    
    // Compilation settings
    bool compile_shader{true};
    bool optimize_shader{true};
    bool generate_debug_info{false};
    u32 optimization_level{2};
    
    // Preprocessor settings
    std::unordered_map<std::string, std::string> defines;
    std::vector<std::string> include_paths;
    std::string entry_point{"main"};
    
    // Educational settings
    bool perform_analysis{true};
    bool generate_learning_content{true};
    bool create_interactive_examples{false};
    bool generate_documentation{false};
    
    // Validation settings
    bool strict_validation{true};
    bool validate_against_profile{true};
    bool check_performance_issues{true};
    
    // Output settings
    bool generate_assembly_listing{false};
    bool generate_reflection_data{true};
    bool preserve_source_debug_info{false};
    
    std::string serialize() const override;
    bool deserialize(const std::string& data) override;
    std::string calculate_hash() const override;
};

//=============================================================================
// Shader Importer Implementation
//=============================================================================

/**
 * @brief Main shader importer with comprehensive educational features
 */
class ShaderImporter : public AssetImporter {
private:
    std::unique_ptr<ShaderCompiler> compiler_;
    std::unique_ptr<ShaderAnalyzer> analyzer_;
    
    // Caching for expensive operations
    mutable std::unordered_map<std::string, ShaderAnalysis> analysis_cache_;
    mutable std::unordered_map<std::string, ShaderCompiler::CompilationResult> compilation_cache_;
    mutable std::shared_mutex cache_mutex_;
    
    // Performance tracking
    mutable std::atomic<u64> total_imports_{0};
    mutable std::atomic<f64> total_compilation_time_{0.0};
    mutable std::atomic<u64> successful_compilations_{0};
    
public:
    ShaderImporter();
    ~ShaderImporter() = default;
    
    // AssetImporter interface
    std::vector<std::string> supported_extensions() const override;
    AssetType asset_type() const override { return AssetType::Shader; }
    bool can_import(const std::filesystem::path& file_path) const override;
    
    ImportResult import_asset(
        const std::filesystem::path& source_path,
        const ImportSettings& settings,
        memory::MemoryTracker* memory_tracker = nullptr
    ) const override;
    
    std::unique_ptr<ImportSettings> create_default_settings() const override;
    core::Result<void, const char*> validate_file(const std::filesystem::path& file_path) const override;
    std::string analyze_file(const std::filesystem::path& file_path) const override;
    
    // Educational interface
    std::string get_educational_description() const override;
    std::vector<std::string> get_learning_objectives() const override;
    
    // Advanced shader operations
    ShaderAnalysis analyze_shader_file(const std::filesystem::path& file_path) const;
    ShaderAnalysis analyze_shader_source(const ShaderSource& source) const;
    
    // Import with detailed feedback
    ImportResult import_with_analysis(
        const std::filesystem::path& source_path,
        const ShaderImportSettings& settings,
        bool generate_analysis = true,
        memory::MemoryTracker* memory_tracker = nullptr
    ) const;
    
    // Shader compilation testing
    ShaderCompiler::CompilationResult test_compilation(
        const std::filesystem::path& source_path,
        const ShaderImportSettings& settings
    ) const;
    
    // Educational utilities
    std::string generate_shader_tutorial(const std::filesystem::path& file_path) const;
    std::string generate_optimization_guide(const std::filesystem::path& file_path) const;
    std::string generate_cross_platform_guide(const std::filesystem::path& file_path) const;
    
    // Interactive development support
    struct LiveEditResult {
        bool compilation_successful{false};
        std::vector<std::string> errors;
        std::vector<std::string> warnings;
        ShaderAnalyzer::RealTimeAnalysis analysis;
        f64 compilation_time_ms{0.0};
    };
    
    LiveEditResult validate_live_edit(
        const std::string& source_code,
        ShaderStage stage,
        const ShaderImportSettings& settings
    ) const;
    
    // Performance monitoring
    struct ShaderImporterStatistics {
        u64 total_imports{0};
        u64 successful_compilations{0};
        u64 failed_compilations{0};
        f64 success_rate{1.0};
        f64 average_compilation_time{0.0};
        
        std::unordered_map<ShaderStage, u32> stage_distribution;
        std::unordered_map<ShaderLanguage, u32> language_distribution;
        
        u32 cache_hits{0};
        u32 cache_misses{0};
        f64 cache_hit_rate{0.0};
    };
    
    ShaderImporterStatistics get_statistics() const;
    void reset_statistics();
    void clear_caches();
    
    // Configuration
    void set_compiler_backend(ShaderCompiler::CompilerBackend backend);
    void add_include_path(const std::string& path);
    void add_global_define(const std::string& name, const std::string& value = "");
    
private:
    // Internal processing
    ShaderSource parse_shader_file(const std::filesystem::path& file_path) const;
    ShaderStage detect_shader_stage(const std::filesystem::path& file_path, const std::string& source) const;
    ShaderLanguage detect_shader_language(const std::filesystem::path& file_path, const std::string& source) const;
    
    ImportResult process_shader_source(
        const ShaderSource& source,
        const ShaderImportSettings& settings,
        const std::filesystem::path& source_path,
        memory::MemoryTracker* memory_tracker
    ) const;
    
    // Educational content generation
    std::string generate_concept_explanation(const ShaderAnalysis& analysis) const;
    std::string generate_performance_analysis(const ShaderAnalysis& analysis) const;
    std::vector<std::string> generate_learning_exercises(const ShaderSource& source) const;
    
    // Utility functions
    std::string resolve_includes(const std::string& source, const std::filesystem::path& base_path) const;
    std::string apply_defines(const std::string& source, const std::unordered_map<std::string, std::string>& defines) const;
    
    // Cache management
    void cache_analysis(const std::string& key, const ShaderAnalysis& analysis) const;
    std::optional<ShaderAnalysis> get_cached_analysis(const std::string& key) const;
    void cache_compilation_result(const std::string& key, const ShaderCompiler::CompilationResult& result) const;
    std::optional<ShaderCompiler::CompilationResult> get_cached_compilation_result(const std::string& key) const;
};

//=============================================================================
// Educational Shader Examples
//=============================================================================

/**
 * @brief Generate educational shader examples for teaching concepts
 */
class ShaderEducationGenerator {
public:
    // Basic examples
    static ShaderSource generate_basic_vertex_shader();
    static ShaderSource generate_basic_fragment_shader();
    static ShaderSource generate_passthrough_vertex_shader();
    static ShaderSource generate_solid_color_fragment_shader(f32 r, f32 g, f32 b);
    
    // Intermediate examples
    static ShaderSource generate_textured_fragment_shader();
    static ShaderSource generate_lighting_fragment_shader();
    static ShaderSource generate_normal_mapping_shaders();
    
    // Advanced examples
    static ShaderSource generate_pbr_fragment_shader();
    static ShaderSource generate_shadow_mapping_shaders();
    static ShaderSource generate_post_processing_shader(const std::string& effect);
    
    // Compute shader examples
    static ShaderSource generate_simple_compute_shader();
    static ShaderSource generate_image_processing_compute_shader();
    static ShaderSource generate_particle_system_compute_shader();
    
    // Educational exercises
    struct ShaderExercise {
        std::string title;
        std::string description;
        std::string objectives;
        ShaderSource template_shader;
        ShaderSource solution_shader;
        std::string hints;
        std::vector<std::string> test_cases;
    };
    
    static std::vector<ShaderExercise> generate_beginner_exercises();
    static std::vector<ShaderExercise> generate_intermediate_exercises();
    static std::vector<ShaderExercise> generate_advanced_exercises();
    
    // Interactive demonstrations
    static std::vector<ShaderSource> generate_concept_demonstrations();
    static std::vector<ShaderSource> generate_optimization_examples();
    static std::vector<ShaderSource> generate_common_mistakes_examples();
    
private:
    static std::string generate_shader_header(ShaderStage stage, ShaderLanguage language);
    static std::string add_educational_comments(const std::string& source, const std::vector<std::string>& concepts);
};

} // namespace ecscope::assets::importers