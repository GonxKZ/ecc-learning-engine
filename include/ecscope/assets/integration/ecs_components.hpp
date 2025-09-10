#pragma once

#include "../core/asset_types.hpp"
#include "../core/asset_handle.hpp"
#include "../processing/texture_processor.hpp"
#include "../processing/model_processor.hpp"
#include "../processing/audio_processor.hpp"
#include <vector>
#include <memory>

namespace ecscope::assets {

// =============================================================================
// Asset Component Base
// =============================================================================

class AssetComponent {
public:
    AssetComponent() = default;
    virtual ~AssetComponent() = default;
    
    virtual AssetTypeID getAssetTypeID() const = 0;
    virtual bool isLoaded() const = 0;
    virtual AssetState getState() const = 0;
    virtual void reload() = 0;
    virtual uint64_t getMemoryUsage() const = 0;
    
    // ECS Component metadata
    bool auto_load = true;
    bool persistent = false;
    AssetPriority load_priority = AssetPriority::Normal;
};

// =============================================================================
// Texture Component
// =============================================================================

class TextureComponent : public AssetComponent {
public:
    TextureComponent() = default;
    explicit TextureComponent(const std::string& path) {
        setTexturePath(path);
    }
    
    // Asset interface
    AssetTypeID getAssetTypeID() const override { return TextureAsset::ASSET_TYPE_ID; }
    bool isLoaded() const override { return texture_handle_.isLoaded(); }
    AssetState getState() const override { return texture_handle_.getState(); }
    void reload() override { texture_handle_.reload(); }
    uint64_t getMemoryUsage() const override;
    
    // Texture management
    void setTexturePath(const std::string& path);
    const std::string& getTexturePath() const { return texture_path_; }
    
    AssetHandle<TextureAsset> getTextureHandle() const { return texture_handle_; }
    const TextureData* getTextureData() const;
    
    // Texture properties
    uint32_t getWidth() const;
    uint32_t getHeight() const;
    TextureFormat getFormat() const;
    
    // GPU integration
    uint32_t getGPUHandle() const;
    void setGPUHandle(uint32_t handle);
    
    // Texture atlas support
    struct AtlasRegion {
        float u_min = 0.0f, v_min = 0.0f;
        float u_max = 1.0f, v_max = 1.0f;
        uint32_t width = 0, height = 0;
    };
    
    void setAtlasRegion(const AtlasRegion& region) { atlas_region_ = region; has_atlas_region_ = true; }
    const AtlasRegion& getAtlasRegion() const { return atlas_region_; }
    bool hasAtlasRegion() const { return has_atlas_region_; }
    
    // Streaming and LOD
    void setTargetQuality(AssetQuality quality);
    AssetQuality getCurrentQuality() const;
    
private:
    std::string texture_path_;
    AssetHandle<TextureAsset> texture_handle_;
    AtlasRegion atlas_region_;
    bool has_atlas_region_ = false;
};

// =============================================================================
// Model Component
// =============================================================================

class ModelComponent : public AssetComponent {
public:
    ModelComponent() = default;
    explicit ModelComponent(const std::string& path) {
        setModelPath(path);
    }
    
    // Asset interface
    AssetTypeID getAssetTypeID() const override { return ModelAsset::ASSET_TYPE_ID; }
    bool isLoaded() const override { return model_handle_.isLoaded(); }
    AssetState getState() const override { return model_handle_.getState(); }
    void reload() override { model_handle_.reload(); }
    uint64_t getMemoryUsage() const override;
    
    // Model management
    void setModelPath(const std::string& path);
    const std::string& getModelPath() const { return model_path_; }
    
    AssetHandle<ModelAsset> getModelHandle() const { return model_handle_; }
    const ModelData* getModelData() const;
    
    // LOD management
    void setLODLevel(uint32_t level) { current_lod_level_ = level; }
    uint32_t getLODLevel() const { return current_lod_level_; }
    uint32_t getMaxLODLevel() const;
    
    void setLODDistance(float distance) { lod_distance_ = distance; }
    float getLODDistance() const { return lod_distance_; }
    
    // Animation state
    struct AnimationState {
        uint32_t animation_index = 0;
        float current_time = 0.0f;
        float playback_speed = 1.0f;
        bool playing = false;
        bool looping = true;
    };
    
    AnimationState& getAnimationState() { return animation_state_; }
    const AnimationState& getAnimationState() const { return animation_state_; }
    
    // Material overrides
    void setMaterialOverride(uint32_t material_index, AssetHandle<MaterialAsset> material);
    AssetHandle<MaterialAsset> getMaterialOverride(uint32_t material_index) const;
    void clearMaterialOverrides() { material_overrides_.clear(); }
    
    // Rendering flags
    bool visible = true;
    bool cast_shadows = true;
    bool receive_shadows = true;
    
private:
    std::string model_path_;
    AssetHandle<ModelAsset> model_handle_;
    
    // LOD state
    uint32_t current_lod_level_ = 0;
    float lod_distance_ = 0.0f;
    
    // Animation state
    AnimationState animation_state_;
    
    // Material overrides
    std::unordered_map<uint32_t, AssetHandle<MaterialAsset>> material_overrides_;
};

// =============================================================================
// Audio Component
// =============================================================================

class AudioComponent : public AssetComponent {
public:
    AudioComponent() = default;
    explicit AudioComponent(const std::string& path) {
        setAudioPath(path);
    }
    
    // Asset interface
    AssetTypeID getAssetTypeID() const override { return AudioAsset::ASSET_TYPE_ID; }
    bool isLoaded() const override { return audio_handle_.isLoaded(); }
    AssetState getState() const override { return audio_handle_.getState(); }
    void reload() override { audio_handle_.reload(); }
    uint64_t getMemoryUsage() const override;
    
    // Audio management
    void setAudioPath(const std::string& path);
    const std::string& getAudioPath() const { return audio_path_; }
    
    AssetHandle<AudioAsset> getAudioHandle() const { return audio_handle_; }
    const AudioData* getAudioData() const;
    
    // Playback state
    struct PlaybackState {
        bool playing = false;
        bool paused = false;
        bool looping = false;
        float volume = 1.0f;
        float pitch = 1.0f;
        double position = 0.0; // seconds
        uint32_t audio_source_id = 0; // Platform-specific audio source
    };
    
    PlaybackState& getPlaybackState() { return playback_state_; }
    const PlaybackState& getPlaybackState() const { return playback_state_; }
    
    // 3D Audio properties
    struct Spatial3D {
        bool enabled = false;
        float min_distance = 1.0f;
        float max_distance = 100.0f;
        float rolloff_factor = 1.0f;
        bool doppler_enabled = false;
        float doppler_factor = 1.0f;
    };
    
    Spatial3D& getSpatialProperties() { return spatial_3d_; }
    const Spatial3D& getSpatialProperties() const { return spatial_3d_; }
    
    // Audio effects
    struct Effects {
        float reverb_level = 0.0f;
        float echo_delay = 0.0f;
        float echo_decay = 0.0f;
        float low_pass_cutoff = 22050.0f; // Hz
        float high_pass_cutoff = 20.0f;   // Hz
    };
    
    Effects& getEffects() { return effects_; }
    const Effects& getEffects() const { return effects_; }
    
    // Playback control
    void play();
    void pause();
    void stop();
    void setVolume(float volume);
    void setPitch(float pitch);
    void setLooping(bool looping);
    void setPosition(double position);
    
private:
    std::string audio_path_;
    AssetHandle<AudioAsset> audio_handle_;
    PlaybackState playback_state_;
    Spatial3D spatial_3d_;
    Effects effects_;
};

// =============================================================================
// Shader Component
// =============================================================================

class ShaderComponent : public AssetComponent {
public:
    enum class ShaderType {
        Vertex,
        Fragment,
        Geometry,
        Compute,
        TessellationControl,
        TessellationEvaluation
    };
    
    ShaderComponent() = default;
    explicit ShaderComponent(const std::string& path, ShaderType type = ShaderType::Fragment) 
        : shader_type_(type) {
        setShaderPath(path);
    }
    
    // Asset interface
    AssetTypeID getAssetTypeID() const override { return ShaderAsset::ASSET_TYPE_ID; }
    bool isLoaded() const override { return shader_handle_.isLoaded(); }
    AssetState getState() const override { return shader_handle_.getState(); }
    void reload() override { shader_handle_.reload(); }
    uint64_t getMemoryUsage() const override;
    
    // Shader management
    void setShaderPath(const std::string& path);
    const std::string& getShaderPath() const { return shader_path_; }
    
    void setShaderType(ShaderType type) { shader_type_ = type; }
    ShaderType getShaderType() const { return shader_type_; }
    
    AssetHandle<ShaderAsset> getShaderHandle() const { return shader_handle_; }
    
    // Shader parameters
    struct Parameter {
        std::string name;
        enum Type { Float, Vec2, Vec3, Vec4, Int, Matrix4, Texture } type;
        
        union {
            float float_val;
            float vec2_val[2];
            float vec3_val[3];
            float vec4_val[4];
            int int_val;
            float matrix4_val[16];
            AssetID texture_id;
        };
        
        Parameter(const std::string& n, float v) : name(n), type(Float), float_val(v) {}
        Parameter(const std::string& n, const float* v, Type t) : name(n), type(t) {
            if (t == Vec2) { memcpy(vec2_val, v, 2 * sizeof(float)); }
            else if (t == Vec3) { memcpy(vec3_val, v, 3 * sizeof(float)); }
            else if (t == Vec4) { memcpy(vec4_val, v, 4 * sizeof(float)); }
            else if (t == Matrix4) { memcpy(matrix4_val, v, 16 * sizeof(float)); }
        }
    };
    
    void setParameter(const Parameter& param);
    const Parameter* getParameter(const std::string& name) const;
    const std::vector<Parameter>& getAllParameters() const { return parameters_; }
    
    // GPU resource
    uint32_t getGPUHandle() const { return gpu_handle_; }
    void setGPUHandle(uint32_t handle) { gpu_handle_ = handle; }
    
private:
    std::string shader_path_;
    AssetHandle<ShaderAsset> shader_handle_;
    ShaderType shader_type_ = ShaderType::Fragment;
    std::vector<Parameter> parameters_;
    uint32_t gpu_handle_ = 0;
};

// =============================================================================
// Material Component
// =============================================================================

class MaterialComponent : public AssetComponent {
public:
    MaterialComponent() = default;
    explicit MaterialComponent(const std::string& path) {
        setMaterialPath(path);
    }
    
    // Asset interface
    AssetTypeID getAssetTypeID() const override { return MaterialAsset::ASSET_TYPE_ID; }
    bool isLoaded() const override { return material_handle_.isLoaded(); }
    AssetState getState() const override { return material_handle_.getState(); }
    void reload() override { material_handle_.reload(); }
    uint64_t getMemoryUsage() const override;
    
    // Material management
    void setMaterialPath(const std::string& path);
    const std::string& getMaterialPath() const { return material_path_; }
    
    AssetHandle<MaterialAsset> getMaterialHandle() const { return material_handle_; }
    
    // Material property overrides
    void setFloatProperty(const std::string& name, float value);
    void setVec3Property(const std::string& name, float x, float y, float z);
    void setTextureProperty(const std::string& name, AssetHandle<TextureAsset> texture);
    
    float getFloatProperty(const std::string& name, float default_value = 0.0f) const;
    std::array<float, 3> getVec3Property(const std::string& name, const std::array<float, 3>& default_value = {0, 0, 0}) const;
    AssetHandle<TextureAsset> getTextureProperty(const std::string& name) const;
    
    // Rendering state
    bool transparent = false;
    bool double_sided = false;
    float alpha_cutoff = 0.5f;
    
private:
    std::string material_path_;
    AssetHandle<MaterialAsset> material_handle_;
    
    // Property overrides
    std::unordered_map<std::string, float> float_overrides_;
    std::unordered_map<std::string, std::array<float, 3>> vec3_overrides_;
    std::unordered_map<std::string, AssetHandle<TextureAsset>> texture_overrides_;
};

// =============================================================================
// Asset Collection Component - For managing multiple related assets
// =============================================================================

class AssetCollectionComponent : public AssetComponent {
public:
    AssetCollectionComponent() = default;
    
    // Asset interface
    AssetTypeID getAssetTypeID() const override { return 0; } // Special type for collections
    bool isLoaded() const override;
    AssetState getState() const override;
    void reload() override;
    uint64_t getMemoryUsage() const override;
    
    // Asset management
    template<typename T>
    void addAsset(const std::string& name, AssetHandle<T> asset);
    
    template<typename T>
    AssetHandle<T> getAsset(const std::string& name) const;
    
    void removeAsset(const std::string& name);
    void clearAssets();
    
    // Collection queries
    std::vector<std::string> getAssetNames() const;
    size_t getAssetCount() const { return assets_.size(); }
    bool hasAsset(const std::string& name) const;
    
    // Batch operations
    void loadAll();
    void unloadAll();
    void reloadAll();
    
    // Asset state queries
    std::vector<std::string> getLoadedAssets() const;
    std::vector<std::string> getUnloadedAssets() const;
    std::vector<std::string> getFailedAssets() const;
    
private:
    struct AssetEntry {
        std::shared_ptr<void> asset_handle; // Type-erased asset handle
        AssetTypeID type_id;
        std::string type_name;
    };
    
    std::unordered_map<std::string, AssetEntry> assets_;
    mutable std::shared_mutex assets_mutex_;
};

// =============================================================================
// Asset Streaming Component - For streaming LOD assets
// =============================================================================

class AssetStreamingComponent : public AssetComponent {
public:
    AssetStreamingComponent() = default;
    
    // Asset interface
    AssetTypeID getAssetTypeID() const override { return 0; } // Special streaming type
    bool isLoaded() const override { return !streaming_assets_.empty(); }
    AssetState getState() const override;
    void reload() override;
    uint64_t getMemoryUsage() const override;
    
    // Streaming configuration
    struct StreamingConfig {
        std::vector<float> lod_distances = {0.0f, 50.0f, 100.0f, 200.0f};
        std::vector<AssetQuality> lod_qualities = {AssetQuality::Ultra, AssetQuality::High, AssetQuality::Medium, AssetQuality::Low};
        float hysteresis_factor = 1.2f; // Prevent LOD thrashing
        bool enable_prediction = true;
    };
    
    void setStreamingConfig(const StreamingConfig& config) { config_ = config; }
    const StreamingConfig& getStreamingConfig() const { return config_; }
    
    // Asset registration for streaming
    template<typename T>
    void addStreamingAsset(const std::string& base_path, const std::vector<std::string>& lod_paths);
    
    // Distance-based streaming
    void updateStreaming(float distance_to_viewer);
    uint32_t getCurrentLODLevel() const { return current_lod_level_; }
    
    // Manual LOD control
    void setTargetLODLevel(uint32_t level);
    void setTargetQuality(AssetQuality quality);
    
private:
    struct StreamingAsset {
        std::string name;
        std::vector<std::shared_ptr<void>> lod_handles; // Type-erased handles for different LODs
        AssetTypeID type_id;
        uint32_t current_lod = 0;
    };
    
    std::vector<StreamingAsset> streaming_assets_;
    StreamingConfig config_;
    uint32_t current_lod_level_ = 0;
    float last_distance_ = 0.0f;
    std::chrono::steady_clock::time_point last_update_;
    
    uint32_t calculateLODLevel(float distance) const;
    bool shouldUpdateLOD(float distance, uint32_t target_lod) const;
};

} // namespace ecscope::assets