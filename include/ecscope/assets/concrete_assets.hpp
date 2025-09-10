#pragma once

#include "asset.hpp"
#include "processors/texture_processor.hpp"
#include "processors/mesh_processor.hpp"
#include "processors/audio_processor.hpp"
#include "processors/shader_processor.hpp"
#include <memory>

namespace ecscope::assets {

    // Concrete asset implementations for common asset types

    // Texture Asset
    class TextureAsset : public Asset {
    public:
        TextureAsset(AssetID id, const std::string& path);
        ~TextureAsset() override;

        // Asset interface implementation
        void* get_data() override;
        const void* get_data() const override;
        size_t get_data_size() const override;
        bool load(const std::vector<uint8_t>& data) override;
        bool reload() override;
        void unload() override;
        std::shared_ptr<Asset> clone() const override;
        bool serialize(std::vector<uint8_t>& data) const override;
        bool deserialize(const std::vector<uint8_t>& data) override;
        size_t get_memory_usage() const override;

        // Texture-specific interface
        int get_width() const { return m_width; }
        int get_height() const { return m_height; }
        int get_channels() const { return m_channels; }
        int get_mipmap_levels() const { return m_mipmap_levels; }
        processors::TextureFormat get_format() const { return m_format; }
        bool has_alpha() const { return m_has_alpha; }
        bool is_srgb() const { return m_is_srgb; }

        // GPU resource management
        uint32_t get_gpu_handle() const { return m_gpu_handle; }
        void set_gpu_handle(uint32_t handle) { m_gpu_handle = handle; }
        bool is_uploaded_to_gpu() const { return m_gpu_handle != 0; }

        // Mipmap access
        const uint8_t* get_mipmap_data(int level = 0) const;
        size_t get_mipmap_size(int level = 0) const;

    private:
        // Texture data
        std::vector<uint8_t> m_pixel_data;
        int m_width = 0;
        int m_height = 0;
        int m_channels = 0;
        int m_mipmap_levels = 1;
        processors::TextureFormat m_format = processors::TextureFormat::UNKNOWN;
        bool m_has_alpha = false;
        bool m_is_srgb = false;

        // GPU resources
        uint32_t m_gpu_handle = 0;

        // Mipmap offsets
        std::vector<size_t> m_mipmap_offsets;

        bool load_from_processor_result(const processors::ProcessingResult& result);
    };

    // Model/Mesh Asset
    class ModelAsset : public Asset {
    public:
        ModelAsset(AssetID id, const std::string& path);
        ~ModelAsset() override;

        // Asset interface implementation
        void* get_data() override;
        const void* get_data() const override;
        size_t get_data_size() const override;
        bool load(const std::vector<uint8_t>& data) override;
        bool reload() override;
        void unload() override;
        std::shared_ptr<Asset> clone() const override;
        bool serialize(std::vector<uint8_t>& data) const override;
        bool deserialize(const std::vector<uint8_t>& data) override;
        size_t get_memory_usage() const override;

        // Model-specific interface
        const processors::ModelData& get_model_data() const { return m_model_data; }
        processors::ModelData& get_model_data() { return m_model_data; }
        
        size_t get_mesh_count() const { return m_model_data.meshes.size(); }
        size_t get_material_count() const { return m_model_data.materials.size(); }
        size_t get_animation_count() const { return m_model_data.animations.size(); }
        size_t get_vertex_count() const { return m_model_data.total_vertices; }
        size_t get_triangle_count() const { return m_model_data.total_triangles; }

        // Mesh access
        const processors::MeshData& get_mesh(size_t index) const;
        const processors::MaterialData& get_material(size_t index) const;
        const processors::AnimationData& get_animation(size_t index) const;

        // Bounding information
        std::array<float, 3> get_bounding_min() const;
        std::array<float, 3> get_bounding_max() const;
        std::array<float, 3> get_bounding_center() const;
        float get_bounding_radius() const;

        // GPU resource management
        uint32_t get_vertex_buffer_handle(size_t mesh_index) const;
        uint32_t get_index_buffer_handle(size_t mesh_index) const;
        void set_gpu_handles(size_t mesh_index, uint32_t vbo, uint32_t ibo);

    private:
        processors::ModelData m_model_data;
        
        // GPU resources
        std::vector<std::pair<uint32_t, uint32_t>> m_gpu_buffers; // VBO, IBO pairs
        
        bool load_from_processor_result(const processors::ProcessingResult& result);
        void calculate_bounds();
    };

    // Audio Asset
    class AudioAsset : public Asset {
    public:
        AudioAsset(AssetID id, const std::string& path);
        ~AudioAsset() override;

        // Asset interface implementation
        void* get_data() override;
        const void* get_data() const override;
        size_t get_data_size() const override;
        bool load(const std::vector<uint8_t>& data) override;
        bool reload() override;
        void unload() override;
        std::shared_ptr<Asset> clone() const override;
        bool serialize(std::vector<uint8_t>& data) const override;
        bool deserialize(const std::vector<uint8_t>& data) override;
        size_t get_memory_usage() const override;

        // Audio-specific interface
        const float* get_pcm_data() const { return m_pcm_data.data(); }
        size_t get_sample_count() const { return m_pcm_data.size(); }
        int get_sample_rate() const { return m_sample_rate; }
        int get_channel_count() const { return m_channels; }
        float get_duration() const { return m_duration; }
        processors::AudioFormat get_format() const { return m_format; }
        
        // Audio analysis data
        float get_peak_amplitude() const { return m_peak_amplitude; }
        float get_rms_amplitude() const { return m_rms_amplitude; }
        float get_tempo() const { return m_tempo; }
        const std::string& get_key() const { return m_key; }
        bool is_music() const { return m_is_music; }
        bool is_speech() const { return m_is_speech; }

        // Streaming support
        bool supports_streaming() const { return m_supports_streaming; }
        void set_streaming_support(bool enable) { m_supports_streaming = enable; }

        // 3D audio properties
        bool is_3d_audio() const { return m_is_3d_audio; }
        void set_3d_audio(bool enable) { m_is_3d_audio = enable; }
        
        // Audio source management
        uint32_t get_audio_source_handle() const { return m_audio_source_handle; }
        void set_audio_source_handle(uint32_t handle) { m_audio_source_handle = handle; }

    private:
        // Audio data
        std::vector<float> m_pcm_data;
        int m_sample_rate = 0;
        int m_channels = 0;
        float m_duration = 0.0f;
        processors::AudioFormat m_format = processors::AudioFormat::UNKNOWN;
        
        // Analysis data
        float m_peak_amplitude = 0.0f;
        float m_rms_amplitude = 0.0f;
        float m_tempo = 0.0f;
        std::string m_key;
        bool m_is_music = false;
        bool m_is_speech = false;
        
        // Properties
        bool m_supports_streaming = false;
        bool m_is_3d_audio = false;
        
        // Audio system resources
        uint32_t m_audio_source_handle = 0;
        
        bool load_from_processor_result(const processors::ProcessingResult& result);
    };

    // Shader Asset
    class ShaderAsset : public Asset {
    public:
        ShaderAsset(AssetID id, const std::string& path);
        ~ShaderAsset() override;

        // Asset interface implementation
        void* get_data() override;
        const void* get_data() const override;
        size_t get_data_size() const override;
        bool load(const std::vector<uint8_t>& data) override;
        bool reload() override;
        void unload() override;
        std::shared_ptr<Asset> clone() const override;
        bool serialize(std::vector<uint8_t>& data) const override;
        bool deserialize(const std::vector<uint8_t>& data) override;
        size_t get_memory_usage() const override;

        // Shader-specific interface
        processors::ShaderType get_shader_type() const { return m_shader_type; }
        processors::ShaderLanguage get_language() const { return m_language; }
        const std::vector<uint8_t>& get_bytecode() const { return m_bytecode; }
        const std::string& get_source_code() const { return m_source_code; }
        
        // Compilation info
        bool is_compiled() const { return m_is_compiled; }
        const std::string& get_compiler_version() const { return m_compiler_version; }
        const std::vector<std::string>& get_compilation_errors() const { return m_compilation_errors; }
        
        // Reflection data
        const processors::ShaderReflection& get_reflection() const { return m_reflection; }
        bool has_reflection_data() const { return m_has_reflection; }
        
        // Uniforms and bindings
        std::vector<processors::ShaderReflection::Variable> get_uniforms() const;
        std::vector<processors::ShaderReflection::Variable> get_textures() const;
        std::vector<processors::ShaderReflection::Variable> get_uniform_buffers() const;
        
        // GPU program management
        uint32_t get_program_handle() const { return m_program_handle; }
        void set_program_handle(uint32_t handle) { m_program_handle = handle; }
        bool is_linked() const { return m_program_handle != 0; }

    private:
        // Shader data
        processors::ShaderType m_shader_type = processors::ShaderType::UNKNOWN;
        processors::ShaderLanguage m_language = processors::ShaderLanguage::UNKNOWN;
        std::vector<uint8_t> m_bytecode;
        std::string m_source_code;
        
        // Compilation state
        bool m_is_compiled = false;
        std::string m_compiler_version;
        std::vector<std::string> m_compilation_errors;
        
        // Reflection data
        processors::ShaderReflection m_reflection;
        bool m_has_reflection = false;
        
        // GPU resources
        uint32_t m_program_handle = 0;
        
        bool load_from_processor_result(const processors::ProcessingResult& result);
    };

    // Material Asset
    class MaterialAsset : public Asset {
    public:
        MaterialAsset(AssetID id, const std::string& path);
        ~MaterialAsset() override;

        // Asset interface implementation
        void* get_data() override;
        const void* get_data() const override;
        size_t get_data_size() const override;
        bool load(const std::vector<uint8_t>& data) override;
        bool reload() override;
        void unload() override;
        std::shared_ptr<Asset> clone() const override;
        bool serialize(std::vector<uint8_t>& data) const override;
        bool deserialize(const std::vector<uint8_t>& data) override;
        size_t get_memory_usage() const override;

        // Material-specific interface
        const processors::MaterialData& get_material_data() const { return m_material_data; }
        processors::MaterialData& get_material_data() { return m_material_data; }
        
        // Texture references
        AssetID get_albedo_texture() const { return m_albedo_texture_id; }
        AssetID get_normal_texture() const { return m_normal_texture_id; }
        AssetID get_metallic_roughness_texture() const { return m_metallic_roughness_texture_id; }
        
        void set_albedo_texture(AssetID texture_id) { m_albedo_texture_id = texture_id; }
        void set_normal_texture(AssetID texture_id) { m_normal_texture_id = texture_id; }
        void set_metallic_roughness_texture(AssetID texture_id) { m_metallic_roughness_texture_id = texture_id; }
        
        // Shader reference
        AssetID get_shader() const { return m_shader_id; }
        void set_shader(AssetID shader_id) { m_shader_id = shader_id; }

    private:
        processors::MaterialData m_material_data;
        
        // Asset references
        AssetID m_albedo_texture_id = INVALID_ASSET_ID;
        AssetID m_normal_texture_id = INVALID_ASSET_ID;
        AssetID m_metallic_roughness_texture_id = INVALID_ASSET_ID;
        AssetID m_occlusion_texture_id = INVALID_ASSET_ID;
        AssetID m_emissive_texture_id = INVALID_ASSET_ID;
        AssetID m_shader_id = INVALID_ASSET_ID;
    };

    // Config Asset (JSON/XML configuration files)
    class ConfigAsset : public Asset {
    public:
        ConfigAsset(AssetID id, const std::string& path);
        ~ConfigAsset() override;

        // Asset interface implementation
        void* get_data() override;
        const void* get_data() const override;
        size_t get_data_size() const override;
        bool load(const std::vector<uint8_t>& data) override;
        bool reload() override;
        void unload() override;
        std::shared_ptr<Asset> clone() const override;
        bool serialize(std::vector<uint8_t>& data) const override;
        bool deserialize(const std::vector<uint8_t>& data) override;
        size_t get_memory_usage() const override;

        // Config-specific interface
        const std::string& get_config_data() const { return m_config_data; }
        std::string& get_config_data() { return m_config_data; }
        
        // JSON/XML parsing helpers (if needed)
        bool is_json() const;
        bool is_xml() const;
        bool validate_syntax() const;

    private:
        std::string m_config_data;
    };

    // Asset factory implementations
    class TextureAssetFactory : public TypedAssetFactory<TextureAsset> {
    public:
        AssetType get_asset_type() const override { return AssetType::TEXTURE; }
        bool can_load(const std::string& extension) const override;
        std::vector<std::string> get_supported_extensions() const override;
    };

    class ModelAssetFactory : public TypedAssetFactory<ModelAsset> {
    public:
        AssetType get_asset_type() const override { return AssetType::MESH; }
        bool can_load(const std::string& extension) const override;
        std::vector<std::string> get_supported_extensions() const override;
    };

    class AudioAssetFactory : public TypedAssetFactory<AudioAsset> {
    public:
        AssetType get_asset_type() const override { return AssetType::AUDIO; }
        bool can_load(const std::string& extension) const override;
        std::vector<std::string> get_supported_extensions() const override;
    };

    class ShaderAssetFactory : public TypedAssetFactory<ShaderAsset> {
    public:
        AssetType get_asset_type() const override { return AssetType::SHADER; }
        bool can_load(const std::string& extension) const override;
        std::vector<std::string> get_supported_extensions() const override;
    };

    class MaterialAssetFactory : public TypedAssetFactory<MaterialAsset> {
    public:
        AssetType get_asset_type() const override { return AssetType::MATERIAL; }
        bool can_load(const std::string& extension) const override;
        std::vector<std::string> get_supported_extensions() const override;
    };

    class ConfigAssetFactory : public TypedAssetFactory<ConfigAsset> {
    public:
        AssetType get_asset_type() const override { return AssetType::CONFIG; }
        bool can_load(const std::string& extension) const override;
        std::vector<std::string> get_supported_extensions() const override;
    };

    // Utility functions for creating asset factories
    void register_default_asset_factories(AssetManager& manager);

} // namespace ecscope::assets