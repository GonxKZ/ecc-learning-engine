#pragma once

#include "asset_processor.hpp"
#include <unordered_map>
#include <set>

namespace ecscope::assets::processors {

    // Shader types
    enum class ShaderType {
        UNKNOWN = 0,
        VERTEX,
        FRAGMENT,
        GEOMETRY,
        COMPUTE,
        TESSELLATION_CONTROL,
        TESSELLATION_EVALUATION,
        RAY_GENERATION,
        RAY_CLOSEST_HIT,
        RAY_MISS,
        RAY_ANY_HIT,
        RAY_INTERSECTION,
        CALLABLE,
        COUNT
    };

    // Shader languages
    enum class ShaderLanguage {
        UNKNOWN = 0,
        GLSL,
        HLSL,
        SPIRV,
        MSL,     // Metal Shading Language
        WGSL,    // WebGPU Shading Language
        COUNT
    };

    // Shader compilation targets
    enum class CompilationTarget {
        OPENGL,
        OPENGL_ES,
        VULKAN,
        DIRECT3D11,
        DIRECT3D12,
        METAL,
        WEBGPU,
        COUNT
    };

    // Shader optimization levels
    enum class OptimizationLevel {
        NONE = 0,
        SIZE,        // Optimize for binary size
        PERFORMANCE, // Optimize for runtime performance
        DEBUG,       // Preserve debug information
        COUNT
    };

    // Shader compilation settings
    struct ShaderCompilationSettings {
        ShaderLanguage source_language = ShaderLanguage::GLSL;
        ShaderLanguage target_language = ShaderLanguage::SPIRV;
        CompilationTarget target_platform = CompilationTarget::VULKAN;
        OptimizationLevel optimization = OptimizationLevel::PERFORMANCE;
        
        // Version settings
        int glsl_version = 450;
        int hlsl_version = 50; // Shader Model 5.0
        int spirv_version = 100; // SPIR-V 1.0
        
        // Feature flags
        bool enable_debug_info = false;
        bool enable_16bit_types = false;
        bool enable_64bit_types = false;
        bool enable_multiview = false;
        bool enable_variable_pointers = false;
        
        // Validation settings
        bool strict_validation = true;
        bool warnings_as_errors = false;
        bool generate_reflection = true;
        
        // Include directories
        std::vector<std::string> include_directories;
        
        // Preprocessor definitions
        std::unordered_map<std::string, std::string> defines;
        
        // Entry point (for HLSL/MSL)
        std::string entry_point = "main";
    };

    // Shader reflection data
    struct ShaderReflection {
        // Input/Output
        struct Variable {
            std::string name;
            std::string type;
            int location = -1;
            int binding = -1;
            int set = -1; // Descriptor set for Vulkan
            int offset = -1; // For uniform buffer members
            size_t size = 0;
            int array_size = 1;
            bool is_builtin = false;
        };
        
        std::vector<Variable> inputs;
        std::vector<Variable> outputs;
        std::vector<Variable> uniforms;
        std::vector<Variable> uniform_buffers;
        std::vector<Variable> storage_buffers;
        std::vector<Variable> textures;
        std::vector<Variable> samplers;
        std::vector<Variable> images;
        
        // Compute shader specific
        std::array<int, 3> local_size = {1, 1, 1};
        
        // Constants
        struct SpecConstant {
            int id;
            std::string name;
            std::string type;
            std::vector<uint8_t> default_value;
        };
        std::vector<SpecConstant> specialization_constants;
        
        // Resource usage
        int texture_slots_used = 0;
        int uniform_buffer_slots_used = 0;
        int storage_buffer_slots_used = 0;
        
        // Statistics
        size_t instruction_count = 0;
        size_t register_count = 0;
        size_t constant_count = 0;
    };

    // Shader metadata
    struct ShaderMetadata {
        ShaderType type = ShaderType::UNKNOWN;
        ShaderLanguage language = ShaderLanguage::UNKNOWN;
        CompilationTarget target = CompilationTarget::OPENGL;
        
        // Version information
        int language_version = 0;
        std::string profile;
        
        // Compilation info
        bool is_compiled = false;
        std::string compiler_version;
        std::chrono::system_clock::time_point compile_time;
        
        // Dependencies
        std::vector<std::string> included_files;
        std::vector<std::string> defines_used;
        
        // Binary info
        size_t binary_size = 0;
        std::string binary_format; // e.g., "SPIR-V", "DXBC", "MSL"
        
        // Performance hints
        int estimated_alu_instructions = 0;
        int estimated_texture_instructions = 0;
        int estimated_registers_used = 0;
        
        ShaderReflection reflection;
    };

    // Shader processor
    class ShaderProcessor : public BaseAssetProcessor {
    public:
        ShaderProcessor();
        
        // AssetProcessor implementation
        std::vector<std::string> get_supported_extensions() const override;
        bool can_process(const std::string& file_path, const AssetMetadata& metadata) const override;
        
        ProcessingResult process(const std::vector<uint8_t>& input_data,
                               const AssetMetadata& input_metadata,
                               const ProcessingOptions& options) override;
        
        bool validate_input(const std::vector<uint8_t>& input_data,
                           const AssetMetadata& metadata) const override;
        
        AssetMetadata extract_metadata(const std::vector<uint8_t>& data,
                                     const std::string& file_path) const override;
        
        size_t estimate_output_size(size_t input_size,
                                   const ProcessingOptions& options) const override;
        
        // Shader-specific operations
        ProcessingResult compile_shader(const std::string& source_code,
                                      ShaderType type,
                                      const ShaderCompilationSettings& settings) const;
        
        ProcessingResult cross_compile(const std::vector<uint8_t>& spirv_binary,
                                     ShaderLanguage target_language,
                                     const ShaderCompilationSettings& settings) const;
        
        ProcessingResult optimize_shader(const std::vector<uint8_t>& binary,
                                       OptimizationLevel level) const;
        
        ProcessingResult validate_shader(const std::vector<uint8_t>& binary,
                                       ShaderLanguage language) const;
        
        ProcessingResult reflect_shader(const std::vector<uint8_t>& binary,
                                      ShaderLanguage language) const;
        
        // Preprocessing
        ProcessingResult preprocess_shader(const std::string& source_code,
                                         const ShaderCompilationSettings& settings) const;
        
        std::vector<std::string> extract_includes(const std::string& source_code) const;
        std::vector<std::string> extract_defines(const std::string& source_code) const;
        
        // Cross-compilation utilities
        ProcessingResult spirv_to_glsl(const std::vector<uint8_t>& spirv,
                                     int glsl_version = 450) const;
        ProcessingResult spirv_to_hlsl(const std::vector<uint8_t>& spirv,
                                     int shader_model = 50) const;
        ProcessingResult spirv_to_msl(const std::vector<uint8_t>& spirv,
                                    int msl_version = 20) const;
        ProcessingResult spirv_to_wgsl(const std::vector<uint8_t>& spirv) const;
        
        ProcessingResult glsl_to_spirv(const std::string& glsl_source,
                                     ShaderType type,
                                     const ShaderCompilationSettings& settings) const;
        ProcessingResult hlsl_to_spirv(const std::string& hlsl_source,
                                     ShaderType type,
                                     const ShaderCompilationSettings& settings) const;
        
        // Analysis
        ShaderType detect_shader_type(const std::string& source_code,
                                     const std::string& file_path) const;
        ShaderLanguage detect_shader_language(const std::string& source_code) const;
        
        struct ShaderComplexity {
            int instruction_count = 0;
            int branch_count = 0;
            int loop_count = 0;
            int texture_sample_count = 0;
            int math_operation_count = 0;
            float estimated_cycles = 0.0f;
        };
        
        ShaderComplexity analyze_complexity(const std::vector<uint8_t>& binary,
                                           ShaderLanguage language) const;
        
        // Validation and error checking
        struct CompilationError {
            std::string message;
            std::string file_path;
            int line = -1;
            int column = -1;
            enum class Severity {
                INFO,
                WARNING,
                ERROR,
                FATAL
            } severity = Severity::ERROR;
        };
        
        std::vector<CompilationError> get_compilation_errors() const;
        std::vector<CompilationError> validate_spirv(const std::vector<uint8_t>& spirv) const;
        
        // Format utilities
        static const char* shader_type_to_string(ShaderType type);
        static ShaderType string_to_shader_type(const std::string& type_str);
        static const char* language_to_string(ShaderLanguage language);
        static ShaderLanguage string_to_language(const std::string& language_str);
        static const char* target_to_string(CompilationTarget target);
        static CompilationTarget string_to_target(const std::string& target_str);
        
        // File extension mapping
        static ShaderType extension_to_shader_type(const std::string& extension);
        static ShaderLanguage extension_to_language(const std::string& extension);
        
    private:
        // Compiler implementations
        ProcessingResult compile_glsl_to_spirv(const std::string& source,
                                             ShaderType type,
                                             const ShaderCompilationSettings& settings) const;
        ProcessingResult compile_hlsl_to_dxbc(const std::string& source,
                                            ShaderType type,
                                            const ShaderCompilationSettings& settings) const;
        ProcessingResult compile_hlsl_to_spirv(const std::string& source,
                                             ShaderType type,
                                             const ShaderCompilationSettings& settings) const;
        
        // Cross-compilation implementations
        class CrossCompiler;
        std::unique_ptr<CrossCompiler> m_cross_compiler;
        
        // SPIR-V utilities
        bool is_valid_spirv(const std::vector<uint8_t>& binary) const;
        ShaderReflection reflect_spirv(const std::vector<uint8_t>& spirv) const;
        std::vector<uint8_t> optimize_spirv(const std::vector<uint8_t>& spirv,
                                           OptimizationLevel level) const;
        
        // Preprocessor
        class ShaderPreprocessor;
        std::unique_ptr<ShaderPreprocessor> m_preprocessor;
        
        std::string preprocess_includes(const std::string& source,
                                      const std::string& base_path,
                                      const std::vector<std::string>& include_dirs,
                                      std::set<std::string>& processed_files) const;
        
        std::string apply_defines(const std::string& source,
                                const std::unordered_map<std::string, std::string>& defines) const;
        
        // Language detection
        bool contains_hlsl_keywords(const std::string& source) const;
        bool contains_glsl_keywords(const std::string& source) const;
        bool contains_msl_keywords(const std::string& source) const;
        
        // Shader type detection
        bool contains_vertex_shader_patterns(const std::string& source) const;
        bool contains_fragment_shader_patterns(const std::string& source) const;
        bool contains_compute_shader_patterns(const std::string& source) const;
        
        // Error handling
        mutable std::vector<CompilationError> m_last_errors;
        void clear_errors() const { m_last_errors.clear(); }
        void add_error(const CompilationError& error) const { m_last_errors.push_back(error); }
        
        // Caching
        class ShaderCache;
        std::unique_ptr<ShaderCache> m_cache;
    };

    // Shader program processor (for linked shader programs)
    class ShaderProgramProcessor {
    public:
        ShaderProgramProcessor();
        ~ShaderProgramProcessor();
        
        struct ShaderStage {
            ShaderType type;
            std::vector<uint8_t> binary;
            ShaderLanguage language;
            std::string entry_point = "main";
        };
        
        // Program linking
        ProcessingResult link_program(const std::vector<ShaderStage>& stages,
                                    const ShaderCompilationSettings& settings) const;
        
        ProcessingResult validate_program(const std::vector<ShaderStage>& stages) const;
        
        // Program reflection
        struct ProgramReflection {
            std::vector<ShaderReflection> stage_reflections;
            
            // Merged interface
            std::vector<ShaderReflection::Variable> vertex_inputs;
            std::vector<ShaderReflection::Variable> fragment_outputs;
            std::vector<ShaderReflection::Variable> all_uniforms;
            std::vector<ShaderReflection::Variable> all_textures;
            
            // Resource layout
            struct ResourceSet {
                int set_number;
                std::vector<ShaderReflection::Variable> resources;
            };
            std::vector<ResourceSet> resource_sets;
            
            // Compatibility info
            bool has_vertex_stage = false;
            bool has_fragment_stage = false;
            bool has_geometry_stage = false;
            bool has_compute_stage = false;
            bool is_graphics_pipeline = false;
            bool is_compute_pipeline = false;
        };
        
        ProgramReflection reflect_program(const std::vector<ShaderStage>& stages) const;
        
        // Program optimization
        ProcessingResult optimize_program(const std::vector<ShaderStage>& stages,
                                        OptimizationLevel level) const;
        
    private:
        class ProgramLinker;
        std::unique_ptr<ProgramLinker> m_linker;
    };

    // Shader utilities
    namespace shader_utils {
        // SPIR-V utilities
        struct SPIRVHeader {
            uint32_t magic;
            uint32_t version;
            uint32_t generator;
            uint32_t bound;
            uint32_t schema;
        };
        
        bool parse_spirv_header(const std::vector<uint8_t>& spirv, SPIRVHeader& header);
        bool is_valid_spirv_magic(uint32_t magic);
        
        // String utilities
        std::string remove_comments(const std::string& source);
        std::string normalize_whitespace(const std::string& source);
        std::vector<std::string> tokenize(const std::string& source);
        
        // Path utilities
        std::string resolve_include_path(const std::string& include_directive,
                                       const std::string& current_file_path,
                                       const std::vector<std::string>& include_directories);
        
        // Hash utilities for caching
        size_t hash_shader_source(const std::string& source,
                                 const ShaderCompilationSettings& settings);
        
        // Platform-specific utilities
        std::string get_platform_shader_cache_path();
        std::vector<std::string> get_system_include_paths();
    }

} // namespace ecscope::assets::processors