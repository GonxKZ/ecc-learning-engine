#include "../include/ecscope/advanced_shader_compiler.hpp"
#include <fstream>
#include <sstream>
#include <filesystem>
#include <regex>
#include <algorithm>
#include <chrono>
#include <future>

// Platform-specific includes
#ifdef _WIN32
    #include <d3dcompiler.h>
    #pragma comment(lib, "d3dcompiler.lib")
#endif

// External shader compilation libraries
#ifdef ECSCOPE_USE_GLSLANG
    #include <glslang/Public/ShaderLang.h>
    #include <glslang/Public/ResourceLimits.h>
    #include <SPIRV/GlslangToSpv.h>
#endif

#ifdef ECSCOPE_USE_SPIRV_CROSS
    #include <spirv_cross/spirv_cross.hpp>
    #include <spirv_cross/spirv_glsl.hpp>
    #include <spirv_cross/spirv_hlsl.hpp>
#endif

namespace ecscope::renderer::shader_compiler {

//=============================================================================
// Shader Preprocessor Implementation
//=============================================================================

ShaderPreprocessor::PreprocessorResult 
ShaderPreprocessor::process(const std::string& source, const std::string& source_file) const {
    PreprocessorResult result;
    
    // Set up base defines for the target platform
    auto temp_defines = defines_;
    
    // Add platform-specific defines
    switch (config_.target) {
        case CompilationTarget::OpenGL_3_3:
            temp_defines["GL_VERSION"] = "330";
            temp_defines["OPENGL"] = "1";
            break;
        case CompilationTarget::OpenGL_4_5:
            temp_defines["GL_VERSION"] = "450";
            temp_defines["OPENGL"] = "1";
            temp_defines["GL_ARB_separate_shader_objects"] = "1";
            break;
        case CompilationTarget::Vulkan_1_0:
            temp_defines["VULKAN"] = "1";
            temp_defines["VK_VERSION"] = "100";
            break;
        case CompilationTarget::DirectX_11:
            temp_defines["DIRECTX"] = "1";
            temp_defines["DX_VERSION"] = "11";
            temp_defines["HLSL"] = "1";
            break;
        default:
            break;
    }
    
    // Add user-defined preprocessor defines
    for (const auto& define : config_.preprocessor_defines) {
        auto pos = define.find('=');
        if (pos != std::string::npos) {
            temp_defines[define.substr(0, pos)] = define.substr(pos + 1);
        } else {
            temp_defines[define] = "1";
        }
    }
    
    // Process the source line by line
    std::istringstream source_stream(source);
    std::string line;
    std::ostringstream processed;
    u32 line_number = 0;
    std::vector<std::string> ifdef_stack;
    
    while (std::getline(source_stream, line)) {
        ++line_number;
        
        // Handle preprocessor directives
        if (line.find("#include") == 0) {
            // Extract include path
            std::regex include_regex(R"(#include\s*[<"](.*)[">])");
            std::smatch match;
            if (std::regex_search(line, match, include_regex)) {
                std::string include_path = match[1].str();
                std::string resolved_path = resolve_include(include_path, source_file);
                
                if (!resolved_path.empty()) {
                    std::ifstream include_file(resolved_path);
                    if (include_file.is_open()) {
                        std::string include_content(
                            (std::istreambuf_iterator<char>(include_file)),
                            std::istreambuf_iterator<char>());
                        
                        // Recursively process included file
                        auto include_result = process(include_content, resolved_path);
                        if (include_result.success) {
                            processed << "// BEGIN INCLUDE: " << include_path << "\n";
                            processed << include_result.processed_source << "\n";
                            processed << "// END INCLUDE: " << include_path << "\n";
                            
                            result.included_files.push_back(resolved_path);
                            for (const auto& nested : include_result.included_files) {
                                result.included_files.push_back(nested);
                            }
                        } else {
                            result.add_diagnostic(CompilationDiagnostic::Severity::Error,
                                                "Failed to process include: " + include_path,
                                                source_file, line_number, 0);
                        }
                    } else {
                        result.add_diagnostic(CompilationDiagnostic::Severity::Error,
                                            "Cannot open include file: " + include_path,
                                            source_file, line_number, 0);
                    }
                } else {
                    result.add_diagnostic(CompilationDiagnostic::Severity::Error,
                                        "Cannot resolve include path: " + include_path,
                                        source_file, line_number, 0);
                }
            }
        }
        else if (line.find("#define") == 0) {
            // Parse define directive
            std::regex define_regex(R"(#define\s+(\w+)(?:\s+(.*))?$)");
            std::smatch match;
            if (std::regex_search(line, match, define_regex)) {
                std::string name = match[1].str();
                std::string value = match.size() > 2 ? match[2].str() : "1";
                temp_defines[name] = value;
                result.resolved_macros[name] = value;
                processed << line << "\n";
            }
        }
        else if (line.find("#ifdef") == 0 || line.find("#ifndef") == 0) {
            // Handle conditional compilation
            bool is_ifndef = line.find("#ifndef") == 0;
            std::regex ifdef_regex(is_ifndef ? R"(#ifndef\s+(\w+))" : R"(#ifdef\s+(\w+))");
            std::smatch match;
            if (std::regex_search(line, match, ifdef_regex)) {
                std::string macro = match[1].str();
                bool is_defined = temp_defines.find(macro) != temp_defines.end();
                bool should_include = is_ifndef ? !is_defined : is_defined;
                ifdef_stack.push_back(should_include ? "true" : "false");
                processed << line << "\n";
            }
        }
        else if (line.find("#endif") == 0) {
            if (!ifdef_stack.empty()) {
                ifdef_stack.pop_back();
            }
            processed << line << "\n";
        }
        else if (line.find("#version") == 0) {
            // Handle version directive - may need modification for cross-compilation
            processed << line << "\n";
        }
        else {
            // Regular line - expand macros if needed
            bool should_include = ifdef_stack.empty() || 
                                std::all_of(ifdef_stack.begin(), ifdef_stack.end(),
                                          [](const std::string& state) { return state == "true"; });
            
            if (should_include) {
                std::string processed_line = expand_macros(line);
                processed << processed_line << "\n";
            }
        }
    }
    
    result.success = true;
    result.processed_source = processed.str();
    return result;
}

std::string ShaderPreprocessor::resolve_include(const std::string& include_path, 
                                               const std::string& current_file) const {
    // Try relative to current file first
    if (!current_file.empty()) {
        std::filesystem::path current_path(current_file);
        std::filesystem::path relative_path = current_path.parent_path() / include_path;
        if (std::filesystem::exists(relative_path)) {
            return relative_path.string();
        }
    }
    
    // Try each include path
    for (const auto& include_dir : config_.include_paths) {
        std::filesystem::path full_path = std::filesystem::path(include_dir) / include_path;
        if (std::filesystem::exists(full_path)) {
            return full_path.string();
        }
    }
    
    // Try additional include paths
    for (const auto& path : include_paths_) {
        std::filesystem::path full_path = std::filesystem::path(path) / include_path;
        if (std::filesystem::exists(full_path)) {
            return full_path.string();
        }
    }
    
    return "";
}

std::string ShaderPreprocessor::expand_macros(const std::string& text) const {
    std::string result = text;
    
    // Simple macro expansion - can be enhanced for more complex cases
    for (const auto& [name, value] : defines_) {
        std::regex macro_regex("\\b" + name + "\\b");
        result = std::regex_replace(result, macro_regex, value);
    }
    
    return result;
}

void ShaderPreprocessor::set_base_defines_for_target(CompilationTarget target) {
    defines_.clear();
    
    switch (target) {
        case CompilationTarget::OpenGL_3_3:
            defines_["GL_core_profile"] = "1";
            defines_["GL_ES"] = "0";
            break;
        case CompilationTarget::OpenGL_4_5:
            defines_["GL_core_profile"] = "1";
            defines_["GL_ARB_separate_shader_objects"] = "1";
            break;
        case CompilationTarget::Vulkan_1_0:
            defines_["VULKAN"] = "1";
            defines_["SPIRV"] = "1";
            break;
        case CompilationTarget::DirectX_11:
            defines_["HLSL"] = "1";
            defines_["DIRECTX"] = "1";
            break;
        default:
            break;
    }
}

//=============================================================================
// Advanced Shader Compiler Implementation
//=============================================================================

AdvancedShaderCompiler::AdvancedShaderCompiler(const CompilerConfig& config)
    : config_(config), preprocessor_(std::make_unique<ShaderPreprocessor>(config)) {
    
    // Initialize compiler backends
    #ifdef ECSCOPE_USE_GLSLANG
    glslang::InitializeProcess();
    #endif
    
    // Start worker threads for async compilation
    u32 thread_count = std::max(1u, std::thread::hardware_concurrency() / 2);
    for (u32 i = 0; i < thread_count; ++i) {
        worker_threads_.emplace_back(&AdvancedShaderCompiler::worker_thread_function, this);
    }
    
    // Create cache directory if it doesn't exist
    if (config_.enable_binary_cache && !config_.cache_directory.empty()) {
        std::filesystem::create_directories(config_.cache_directory);
    }
}

AdvancedShaderCompiler::~AdvancedShaderCompiler() {
    shutdown_requested_ = true;
    for (auto& thread : worker_threads_) {
        if (thread.joinable()) {
            thread.join();
        }
    }
    
    #ifdef ECSCOPE_USE_GLSLANG
    glslang::FinalizeProcess();
    #endif
}

CompilationResult AdvancedShaderCompiler::compile_shader(const std::string& source,
                                                       resources::ShaderStage stage,
                                                       const std::string& entry_point,
                                                       const std::string& source_file) {
    auto start_time = std::chrono::high_resolution_clock::now();
    
    CompilationResult result;
    result.cache_key = generate_cache_key(source, stage, entry_point, config_);
    
    // Check cache first
    if (enable_cache_) {
        std::lock_guard<std::mutex> lock(cache_mutex_);
        auto cache_it = cache_.find(result.cache_key);
        if (cache_it != cache_.end()) {
            result = cache_it->second;
            result.loaded_from_cache = true;
            update_statistics(result, true);
            return result;
        }
    }
    
    // Preprocess the source
    auto preprocess_result = preprocessor_->process(source, source_file);
    if (!preprocess_result.success) {
        result.diagnostics = std::move(preprocess_result.diagnostics);
        result.success = false;
        update_statistics(result, false);
        return result;
    }
    
    result.preprocessed_source = preprocess_result.processed_source;
    
    // Compile based on source and target languages
    switch (config_.source_language) {
        case ShaderLanguage::GLSL:
            result = compile_glsl(preprocess_result.processed_source, stage, entry_point, source_file);
            break;
        case ShaderLanguage::HLSL:
            result = compile_hlsl(preprocess_result.processed_source, stage, entry_point, source_file);
            break;
        case ShaderLanguage::SPIRV:
            result.add_diagnostic(CompilationDiagnostic::Severity::Error, 
                                "SPIR-V input not supported - compile from GLSL or HLSL first");
            result.success = false;
            break;
        default:
            result.add_diagnostic(CompilationDiagnostic::Severity::Error, 
                                "Unsupported source language");
            result.success = false;
            break;
    }
    
    // Record compilation time
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
    result.performance.compilation_time = duration.count() / 1000.0f;
    
    // Extract reflection data and analyze performance
    if (result.success && !result.bytecode.empty()) {
        extract_reflection_data(result, result.bytecode);
        analyze_performance_metrics(result, result.bytecode);
    }
    
    // Cache the result
    if (enable_cache_ && result.success) {
        std::lock_guard<std::mutex> lock(cache_mutex_);
        cache_[result.cache_key] = result;
    }
    
    update_statistics(result, false);
    
    if (debug_output_) {
        log_compilation_info("Compiled " + (source_file.empty() ? "inline shader" : source_file) + 
                           " in " + std::to_string(result.performance.compilation_time) + "ms");
    }
    
    return result;
}

CompilationResult AdvancedShaderCompiler::compile_glsl(const std::string& source,
                                                      resources::ShaderStage stage,
                                                      const std::string& entry_point,
                                                      const std::string& source_file) {
    CompilationResult result;
    
    #ifdef ECSCOPE_USE_GLSLANG
    // Initialize glslang resources
    const TBuiltInResource* resources = GetDefaultResources();
    
    // Determine shader type
    EShLanguage glsl_stage;
    switch (stage) {
        case resources::ShaderStage::Vertex: glsl_stage = EShLangVertex; break;
        case resources::ShaderStage::Fragment: glsl_stage = EShLangFragment; break;
        case resources::ShaderStage::Geometry: glsl_stage = EShLangGeometry; break;
        case resources::ShaderStage::Compute: glsl_stage = EShLangCompute; break;
        case resources::ShaderStage::TessControl: glsl_stage = EShLangTessControl; break;
        case resources::ShaderStage::TessEvaluation: glsl_stage = EShLangTessEvaluation; break;
        default:
            result.add_diagnostic(CompilationDiagnostic::Severity::Error, "Unsupported shader stage");
            return result;
    }
    
    // Create shader object
    glslang::TShader shader(glsl_stage);
    const char* source_ptr = source.c_str();
    shader.setStrings(&source_ptr, 1);
    
    // Set version and profile
    int client_version = 100; // Default
    glslang::EShTargetClientVersion client_ver = glslang::EShTargetVulkan_1_0;
    glslang::EShTargetLanguageVersion target_ver = glslang::EShTargetSpv_1_0;
    
    if (config_.target == CompilationTarget::Vulkan_1_0 || 
        config_.target == CompilationTarget::Vulkan_1_1) {
        client_ver = glslang::EShTargetVulkan_1_0;
        target_ver = glslang::EShTargetSpv_1_0;
    } else if (config_.target == CompilationTarget::OpenGL_4_5) {
        client_ver = glslang::EShTargetOpenGL_450;
        client_version = 450;
    }
    
    shader.setEnvInput(glslang::EShSourceGlsl, glsl_stage, glslang::EShClientVulkan, client_version);
    shader.setEnvClient(glslang::EShClientVulkan, client_ver);
    shader.setEnvTarget(glslang::EShTargetSpv, target_ver);
    
    // Compile the shader
    EShMessages messages = static_cast<EShMessages>(EShMsgSpvRules | EShMsgVulkanRules);
    if (config_.enable_debug_info) {
        messages = static_cast<EShMessages>(messages | EShMsgDebugInfo);
    }
    
    bool compile_success = shader.parse(resources, 100, false, messages);
    
    if (!compile_success) {
        result.add_diagnostic(CompilationDiagnostic::Severity::Error, shader.getInfoLog());
        return result;
    }
    
    // Link the shader into a program
    glslang::TProgram program;
    program.addShader(&shader);
    
    bool link_success = program.link(messages);
    if (!link_success) {
        result.add_diagnostic(CompilationDiagnostic::Severity::Error, program.getInfoLog());
        return result;
    }
    
    // Generate SPIR-V
    std::vector<unsigned int> spirv;
    spv::SpvBuildLogger logger;
    glslang::SpvOptions spv_options;
    
    if (config_.enable_debug_info) {
        spv_options.generateDebugInfo = true;
    }
    if (config_.optimization != OptimizationLevel::Debug) {
        spv_options.optimizeSize = (config_.optimization == OptimizationLevel::Size);
    }
    
    glslang::GlslangToSpv(*program.getIntermediate(glsl_stage), spirv, &logger, &spv_options);
    
    if (!logger.getAllMessages().empty()) {
        result.add_diagnostic(CompilationDiagnostic::Severity::Warning, logger.getAllMessages());
    }
    
    // Convert to byte array
    result.bytecode.resize(spirv.size() * sizeof(unsigned int));
    std::memcpy(result.bytecode.data(), spirv.data(), result.bytecode.size());
    
    result.success = true;
    
    #else
    result.add_diagnostic(CompilationDiagnostic::Severity::Error, 
                         "GLSL compilation not available - ECScope compiled without glslang support");
    #endif
    
    return result;
}

CompilationResult AdvancedShaderCompiler::compile_hlsl(const std::string& source,
                                                      resources::ShaderStage stage,
                                                      const std::string& entry_point,
                                                      const std::string& source_file) {
    CompilationResult result;
    
    #ifdef _WIN32
    // Determine shader profile
    const char* target_profile = nullptr;
    switch (stage) {
        case resources::ShaderStage::Vertex: target_profile = "vs_5_0"; break;
        case resources::ShaderStage::Fragment: target_profile = "ps_5_0"; break;
        case resources::ShaderStage::Geometry: target_profile = "gs_5_0"; break;
        case resources::ShaderStage::Compute: target_profile = "cs_5_0"; break;
        case resources::ShaderStage::TessControl: target_profile = "hs_5_0"; break;
        case resources::ShaderStage::TessEvaluation: target_profile = "ds_5_0"; break;
        default:
            result.add_diagnostic(CompilationDiagnostic::Severity::Error, "Unsupported shader stage for HLSL");
            return result;
    }
    
    // Set compilation flags
    UINT compile_flags = 0;
    switch (config_.optimization) {
        case OptimizationLevel::Debug:
            compile_flags |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
            break;
        case OptimizationLevel::Development:
            compile_flags |= D3DCOMPILE_OPTIMIZATION_LEVEL1;
            break;
        case OptimizationLevel::Release:
            compile_flags |= D3DCOMPILE_OPTIMIZATION_LEVEL3;
            break;
        case OptimizationLevel::Size:
            compile_flags |= D3DCOMPILE_OPTIMIZATION_LEVEL1;
            break;
        case OptimizationLevel::Performance:
            compile_flags |= D3DCOMPILE_OPTIMIZATION_LEVEL3;
            break;
        default:
            break;
    }
    
    if (config_.treat_warnings_as_errors) {
        compile_flags |= D3DCOMPILE_WARNINGS_ARE_ERRORS;
    }
    
    // Compile the shader
    ID3DBlob* shader_blob = nullptr;
    ID3DBlob* error_blob = nullptr;
    
    HRESULT hr = D3DCompile(
        source.c_str(),
        source.length(),
        source_file.empty() ? nullptr : source_file.c_str(),
        nullptr, // Defines
        D3D_COMPILE_STANDARD_FILE_INCLUDE,
        entry_point.c_str(),
        target_profile,
        compile_flags,
        0,
        &shader_blob,
        &error_blob
    );
    
    if (FAILED(hr)) {
        if (error_blob) {
            std::string error_msg(static_cast<const char*>(error_blob->GetBufferPointer()),
                                error_blob->GetBufferSize());
            result.add_diagnostic(CompilationDiagnostic::Severity::Error, error_msg);
            error_blob->Release();
        } else {
            result.add_diagnostic(CompilationDiagnostic::Severity::Error, "Unknown HLSL compilation error");
        }
        return result;
    }
    
    // Copy bytecode
    if (shader_blob) {
        result.bytecode.resize(shader_blob->GetBufferSize());
        std::memcpy(result.bytecode.data(), shader_blob->GetBufferPointer(), 
                   shader_blob->GetBufferSize());
        shader_blob->Release();
    }
    
    if (error_blob) {
        error_blob->Release();
    }
    
    result.success = true;
    
    #else
    result.add_diagnostic(CompilationDiagnostic::Severity::Error, 
                         "HLSL compilation not available on this platform");
    #endif
    
    return result;
}

CompilationResult AdvancedShaderCompiler::cross_compile(const std::vector<u8>& spirv_bytecode,
                                                       ShaderLanguage target_language,
                                                       resources::ShaderStage stage) {
    CompilationResult result;
    
    #ifdef ECSCOPE_USE_SPIRV_CROSS
    try {
        // Create SPIRV-Cross compiler
        std::unique_ptr<spirv_cross::Compiler> compiler;
        
        switch (target_language) {
            case ShaderLanguage::GLSL: {
                auto glsl_compiler = std::make_unique<spirv_cross::CompilerGLSL>(
                    reinterpret_cast<const uint32_t*>(spirv_bytecode.data()),
                    spirv_bytecode.size() / sizeof(uint32_t));
                
                spirv_cross::CompilerGLSL::Options options;
                options.version = config_.glsl.version;
                options.es = false;
                options.force_temporary = false;
                options.vulkan_semantics = (config_.target == CompilationTarget::Vulkan_1_0 ||
                                          config_.target == CompilationTarget::Vulkan_1_1);
                glsl_compiler->set_common_options(options);
                
                compiler = std::move(glsl_compiler);
                break;
            }
            case ShaderLanguage::HLSL: {
                auto hlsl_compiler = std::make_unique<spirv_cross::CompilerHLSL>(
                    reinterpret_cast<const uint32_t*>(spirv_bytecode.data()),
                    spirv_bytecode.size() / sizeof(uint32_t));
                
                spirv_cross::CompilerHLSL::Options options;
                options.shader_model = 50; // Default to SM 5.0
                hlsl_compiler->set_hlsl_options(options);
                
                compiler = std::move(hlsl_compiler);
                break;
            }
            default:
                result.add_diagnostic(CompilationDiagnostic::Severity::Error,
                                    "Unsupported target language for cross-compilation");
                return result;
        }
        
        // Compile to target language
        std::string cross_compiled = compiler->compile();
        result.preprocessed_source = cross_compiled;
        
        // For now, we'll store the source as "bytecode" since some backends expect source code
        result.bytecode.assign(cross_compiled.begin(), cross_compiled.end());
        result.success = true;
        
    } catch (const spirv_cross::CompilerError& e) {
        result.add_diagnostic(CompilationDiagnostic::Severity::Error, 
                            std::string("SPIRV-Cross error: ") + e.what());
    } catch (const std::exception& e) {
        result.add_diagnostic(CompilationDiagnostic::Severity::Error, 
                            std::string("Cross-compilation error: ") + e.what());
    }
    
    #else
    result.add_diagnostic(CompilationDiagnostic::Severity::Error,
                         "Cross-compilation not available - ECScope compiled without SPIRV-Cross support");
    #endif
    
    return result;
}

std::string AdvancedShaderCompiler::generate_cache_key(const std::string& source,
                                                      resources::ShaderStage stage,
                                                      const std::string& entry_point,
                                                      const CompilerConfig& config) const {
    // Create a hash of the input parameters
    std::hash<std::string> hasher;
    std::string combined = source + std::to_string(static_cast<u32>(stage)) + entry_point + 
                          std::to_string(static_cast<u32>(config.target)) +
                          std::to_string(static_cast<u32>(config.optimization));
    
    // Add preprocessor defines to hash
    for (const auto& define : config.preprocessor_defines) {
        combined += define;
    }
    
    return std::to_string(hasher(combined));
}

void AdvancedShaderCompiler::extract_reflection_data(CompilationResult& result,
                                                    const std::vector<u8>& bytecode) const {
    #ifdef ECSCOPE_USE_SPIRV_CROSS
    try {
        spirv_cross::Compiler compiler(
            reinterpret_cast<const uint32_t*>(bytecode.data()),
            bytecode.size() / sizeof(uint32_t));
        
        // Get shader resources
        spirv_cross::ShaderResources resources = compiler.get_shader_resources();
        
        // Extract uniform buffer information
        for (const auto& ubo : resources.uniform_buffers) {
            resources::UniformBufferLayout layout;
            layout.name = compiler.get_name(ubo.id);
            
            const auto& type = compiler.get_type(ubo.type_id);
            for (u32 i = 0; i < type.member_types.size(); ++i) {
                const auto& member_type = compiler.get_type(type.member_types[i]);
                resources::UniformInfo uniform;
                uniform.name = compiler.get_member_name(ubo.type_id, i);
                uniform.size = compiler.get_declared_struct_member_size(type, i);
                uniform.offset = compiler.type_struct_member_offset(type, i);
                
                // Convert SPIR-V type to our type system
                if (member_type.basetype == spirv_cross::SPIRType::Float) {
                    if (member_type.vecsize == 1) uniform.type = resources::ShaderDataType::Float;
                    else if (member_type.vecsize == 2) uniform.type = resources::ShaderDataType::Vec2;
                    else if (member_type.vecsize == 3) uniform.type = resources::ShaderDataType::Vec3;
                    else if (member_type.vecsize == 4) uniform.type = resources::ShaderDataType::Vec4;
                } else if (member_type.basetype == spirv_cross::SPIRType::Int) {
                    if (member_type.vecsize == 1) uniform.type = resources::ShaderDataType::Int;
                    else if (member_type.vecsize == 2) uniform.type = resources::ShaderDataType::IVec2;
                    else if (member_type.vecsize == 3) uniform.type = resources::ShaderDataType::IVec3;
                    else if (member_type.vecsize == 4) uniform.type = resources::ShaderDataType::IVec4;
                }
                
                layout.uniforms.push_back(uniform);
            }
            
            layout.finalize_layout();
            result.reflection.uniform_buffers.push_back(layout);
        }
        
        // Extract sampler information
        for (const auto& sampler : resources.sampled_images) {
            result.reflection.samplers.push_back(compiler.get_name(sampler.id));
        }
        
        // Extract compute shader local size
        if (!resources.stage_inputs.empty() || !resources.stage_outputs.empty()) {
            // This is a compute shader
            auto execution_model = compiler.get_execution_model();
            if (execution_model == spv::ExecutionModelGLCompute) {
                const auto& entry_point = compiler.get_entry_point("main", spv::ExecutionModelGLCompute);
                result.reflection.local_size_x = entry_point.workgroup_size.x;
                result.reflection.local_size_y = entry_point.workgroup_size.y;
                result.reflection.local_size_z = entry_point.workgroup_size.z;
            }
        }
        
    } catch (const std::exception& e) {
        // Failed to extract reflection data - not critical
        result.add_diagnostic(CompilationDiagnostic::Severity::Warning,
                            "Failed to extract reflection data: " + std::string(e.what()));
    }
    #endif
}

void AdvancedShaderCompiler::analyze_performance_metrics(CompilationResult& result,
                                                        const std::vector<u8>& bytecode) const {
    // Basic performance analysis - can be enhanced with more sophisticated analysis
    result.performance.instruction_count = bytecode.size() / 4; // Rough estimate
    
    // Estimate GPU cost based on bytecode size and complexity
    f32 base_cost = 1.0f;
    f32 size_factor = std::min(10.0f, bytecode.size() / 1024.0f); // Size in KB
    result.performance.estimated_gpu_cost = base_cost + (size_factor * 0.1f);
    
    // Analyze source for performance hints
    const std::string& source = result.preprocessed_source;
    
    // Count expensive operations
    u32 texture_samples = std::count(source.begin(), source.end(), 't') + 
                         std::count(source.begin(), source.end(), 's'); // Rough heuristic
    u32 math_ops = std::count(source.begin(), source.end(), '*') + 
                   std::count(source.begin(), source.end(), '/');
    
    result.performance.vertex_shader_cost = 0.3f + (math_ops * 0.01f);
    result.performance.fragment_shader_cost = 0.5f + (texture_samples * 0.05f) + (math_ops * 0.01f);
    
    // Generate performance analysis text
    std::ostringstream analysis;
    analysis << "Shader Performance Analysis:\n";
    analysis << "- Estimated instruction count: " << result.performance.instruction_count << "\n";
    analysis << "- Estimated GPU cost: " << result.performance.estimated_gpu_cost << "x baseline\n";
    analysis << "- Texture samples detected: " << texture_samples << "\n";
    analysis << "- Math operations detected: " << math_ops << "\n";
    
    if (result.performance.estimated_gpu_cost > 5.0f) {
        analysis << "- WARNING: High complexity shader detected\n";
    }
    if (texture_samples > 8) {
        analysis << "- WARNING: Many texture samples may impact performance\n";
    }
    
    result.performance.performance_analysis = analysis.str();
}

void AdvancedShaderCompiler::update_statistics(const CompilationResult& result, bool cache_hit) const {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    
    stats_.total_compilations++;
    if (result.success) {
        stats_.successful_compilations++;
    } else {
        stats_.failed_compilations++;
    }
    
    if (cache_hit) {
        stats_.cache_hits++;
    } else {
        stats_.cache_misses++;
    }
    
    stats_.total_compilation_time += result.performance.compilation_time;
    stats_.average_compilation_time = stats_.total_compilation_time / stats_.total_compilations;
    
    if (stats_.total_compilations > 0) {
        stats_.cache_hit_ratio = static_cast<f32>(stats_.cache_hits) / stats_.total_compilations;
    }
}

void AdvancedShaderCompiler::worker_thread_function() {
    while (!shutdown_requested_) {
        // Worker thread implementation for async compilation
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

void AdvancedShaderCompiler::log_compilation_info(const std::string& message) const {
    if (debug_output_) {
        // In a real implementation, this would use a proper logging system
        std::cout << "[ShaderCompiler] " << message << std::endl;
    }
}

//=============================================================================
// Utility Functions Implementation
//=============================================================================

namespace utils {
    const char* shader_language_to_string(ShaderLanguage lang) {
        switch (lang) {
            case ShaderLanguage::GLSL: return "GLSL";
            case ShaderLanguage::HLSL: return "HLSL";
            case ShaderLanguage::SPIRV: return "SPIR-V";
            case ShaderLanguage::MSL: return "MSL";
            case ShaderLanguage::WGSL: return "WGSL";
            case ShaderLanguage::CUDA: return "CUDA";
            case ShaderLanguage::OpenCL: return "OpenCL";
            default: return "Unknown";
        }
    }
    
    const char* compilation_target_to_string(CompilationTarget target) {
        switch (target) {
            case CompilationTarget::OpenGL_3_3: return "OpenGL 3.3";
            case CompilationTarget::OpenGL_4_0: return "OpenGL 4.0";
            case CompilationTarget::OpenGL_4_5: return "OpenGL 4.5";
            case CompilationTarget::OpenGL_4_6: return "OpenGL 4.6";
            case CompilationTarget::Vulkan_1_0: return "Vulkan 1.0";
            case CompilationTarget::Vulkan_1_1: return "Vulkan 1.1";
            case CompilationTarget::DirectX_11: return "DirectX 11";
            case CompilationTarget::DirectX_12: return "DirectX 12";
            case CompilationTarget::WebGL_1_0: return "WebGL 1.0";
            case CompilationTarget::WebGL_2_0: return "WebGL 2.0";
            case CompilationTarget::WebGPU: return "WebGPU";
            default: return "Unknown";
        }
    }
    
    CompilerConfig create_development_config() {
        CompilerConfig config;
        config.optimization = OptimizationLevel::Development;
        config.enable_debug_info = true;
        config.enable_validation = true;
        config.enable_warnings = true;
        config.generate_reflection_data = true;
        return config;
    }
    
    CompilerConfig create_release_config() {
        CompilerConfig config;
        config.optimization = OptimizationLevel::Release;
        config.enable_debug_info = false;
        config.enable_validation = false;
        config.enable_warnings = false;
        config.generate_reflection_data = false;
        config.enable_aggressive_optimization = true;
        return config;
    }
    
    bool is_glsl_supported() {
        #ifdef ECSCOPE_USE_GLSLANG
        return true;
        #else
        return false;
        #endif
    }
    
    bool is_hlsl_supported() {
        #ifdef _WIN32
        return true;
        #else
        return false;
        #endif
    }
    
    bool is_spirv_supported() {
        #ifdef ECSCOPE_USE_SPIRV_CROSS
        return true;
        #else
        return false;
        #endif
    }
}

} // namespace ecscope::renderer::shader_compiler