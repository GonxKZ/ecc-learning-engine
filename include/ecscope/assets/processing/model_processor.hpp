#pragma once

#include "../core/asset_types.hpp"
#include <vector>
#include <memory>
#include <array>

namespace ecscope::assets {

// =============================================================================
// 3D Math Structures
// =============================================================================

struct Vec3 {
    float x, y, z;
    Vec3() : x(0), y(0), z(0) {}
    Vec3(float x_, float y_, float z_) : x(x_), y(y_), z(z_) {}
    
    Vec3 operator+(const Vec3& other) const { return {x + other.x, y + other.y, z + other.z}; }
    Vec3 operator-(const Vec3& other) const { return {x - other.x, y - other.y, z - other.z}; }
    Vec3 operator*(float scalar) const { return {x * scalar, y * scalar, z * scalar}; }
    
    float length() const;
    Vec3 normalized() const;
    Vec3 cross(const Vec3& other) const;
    float dot(const Vec3& other) const;
};

struct Vec2 {
    float x, y;
    Vec2() : x(0), y(0) {}
    Vec2(float x_, float y_) : x(x_), y(y_) {}
};

struct Vec4 {
    float x, y, z, w;
    Vec4() : x(0), y(0), z(0), w(0) {}
    Vec4(float x_, float y_, float z_, float w_) : x(x_), y(y_), z(z_), w(w_) {}
};

using Matrix4 = std::array<std::array<float, 4>, 4>;

// =============================================================================
// Vertex Data Structures
// =============================================================================

struct Vertex {
    Vec3 position;
    Vec3 normal;
    Vec2 tex_coords;
    Vec3 tangent;
    Vec3 bitangent;
    Vec4 color = {1.0f, 1.0f, 1.0f, 1.0f};
    
    // Skinning data
    std::array<uint32_t, 4> bone_ids = {0, 0, 0, 0};
    std::array<float, 4> bone_weights = {0.0f, 0.0f, 0.0f, 0.0f};
};

struct BoundingBox {
    Vec3 min;
    Vec3 max;
    
    BoundingBox() : min(FLT_MAX, FLT_MAX, FLT_MAX), max(-FLT_MAX, -FLT_MAX, -FLT_MAX) {}
    BoundingBox(const Vec3& min_, const Vec3& max_) : min(min_), max(max_) {}
    
    void expand(const Vec3& point);
    void expand(const BoundingBox& other);
    Vec3 center() const { return (min + max) * 0.5f; }
    Vec3 size() const { return max - min; }
    float volume() const;
};

// =============================================================================
// Material System
// =============================================================================

enum class MaterialType : uint8_t {
    Standard,
    PBR,
    Unlit,
    Transparent,
    Emissive
};

struct MaterialProperty {
    enum Type { Float, Vec2, Vec3, Vec4, Texture, Bool, Int };
    
    Type type;
    std::string name;
    
    union {
        float float_value;
        Vec2 vec2_value;
        Vec3 vec3_value;
        Vec4 vec4_value;
        bool bool_value;
        int int_value;
        AssetID texture_id;
    };
    
    MaterialProperty(const std::string& n, float v) : type(Float), name(n), float_value(v) {}
    MaterialProperty(const std::string& n, Vec3 v) : type(Vec3), name(n), vec3_value(v) {}
    MaterialProperty(const std::string& n, AssetID v) : type(Texture), name(n), texture_id(v) {}
};

struct Material {
    std::string name;
    MaterialType type = MaterialType::Standard;
    std::vector<MaterialProperty> properties;
    
    // Common PBR properties
    Vec3 albedo = {1.0f, 1.0f, 1.0f};
    float metallic = 0.0f;
    float roughness = 0.5f;
    float emission_strength = 0.0f;
    float alpha = 1.0f;
    
    // Texture asset IDs
    AssetID albedo_texture = INVALID_ASSET_ID;
    AssetID normal_texture = INVALID_ASSET_ID;
    AssetID metallic_roughness_texture = INVALID_ASSET_ID;
    AssetID emission_texture = INVALID_ASSET_ID;
    AssetID occlusion_texture = INVALID_ASSET_ID;
    
    void addProperty(const MaterialProperty& prop);
    const MaterialProperty* getProperty(const std::string& name) const;
};

// =============================================================================
// Mesh Data
// =============================================================================

struct SubMesh {
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    uint32_t material_index = 0;
    BoundingBox bounds;
    
    // LOD information
    uint32_t lod_level = 0;
    float lod_distance = 0.0f;
    
    void calculateBounds();
    void calculateNormals();
    void calculateTangents();
};

struct Mesh {
    std::string name;
    std::vector<SubMesh> sub_meshes;
    BoundingBox bounds;
    
    // LOD chain
    std::vector<uint32_t> lod_indices; // Indices into sub_meshes for each LOD level
    
    void calculateBounds();
    uint32_t getTotalVertexCount() const;
    uint32_t getTotalIndexCount() const;
};

// =============================================================================
// Animation System
// =============================================================================

struct Bone {
    std::string name;
    uint32_t id;
    uint32_t parent_id = UINT32_MAX; // UINT32_MAX = no parent
    Matrix4 offset_matrix;
    Matrix4 bind_pose;
};

struct Skeleton {
    std::string name;
    std::vector<Bone> bones;
    std::unordered_map<std::string, uint32_t> bone_name_to_id;
    Matrix4 global_inverse_transform;
    
    uint32_t findBone(const std::string& name) const;
};

struct KeyFrame {
    float time;
    Vec3 position;
    Vec4 rotation; // Quaternion
    Vec3 scale;
};

struct AnimationChannel {
    uint32_t bone_id;
    std::vector<KeyFrame> keyframes;
    
    KeyFrame interpolate(float time) const;
};

struct Animation {
    std::string name;
    float duration;
    float ticks_per_second = 24.0f;
    std::vector<AnimationChannel> channels;
    bool looping = true;
};

// =============================================================================
// Model Data
// =============================================================================

struct ModelData {
    std::string name;
    std::vector<Mesh> meshes;
    std::vector<Material> materials;
    std::vector<Animation> animations;
    std::unique_ptr<Skeleton> skeleton;
    BoundingBox bounds;
    
    // Scene graph (optional)
    struct Node {
        std::string name;
        Matrix4 transform;
        std::vector<uint32_t> mesh_indices;
        std::vector<uint32_t> child_indices;
        uint32_t parent_index = UINT32_MAX;
    };
    std::vector<Node> nodes;
    uint32_t root_node = 0;
    
    void calculateBounds();
    uint64_t getMemoryUsage() const;
};

// =============================================================================
// Model Processing Options
// =============================================================================

struct ModelProcessingOptions {
    // Mesh optimization
    bool optimize_vertices = true;
    bool optimize_indices = true;
    bool remove_duplicates = true;
    bool weld_vertices = true;
    float weld_threshold = 1e-6f;
    
    // Normal and tangent calculation
    bool calculate_normals = true;
    bool calculate_tangents = true;
    bool smooth_normals = true;
    float smoothing_angle = 60.0f; // degrees
    
    // LOD generation
    bool generate_lods = true;
    uint32_t max_lod_levels = 4;
    std::vector<float> lod_ratios = {1.0f, 0.5f, 0.25f, 0.125f}; // Vertex reduction ratios
    std::vector<float> lod_distances = {0.0f, 50.0f, 100.0f, 200.0f}; // Switch distances
    
    // Animation processing
    bool optimize_animations = true;
    bool compress_animations = false;
    float animation_tolerance = 1e-6f;
    
    // Coordinate system conversion
    bool convert_coordinate_system = false;
    enum CoordinateSystem { RightHanded, LeftHanded } target_coordinate_system = RightHanded;
    
    // Scale and transformation
    bool apply_transform = false;
    Matrix4 transform_matrix;
    float uniform_scale = 1.0f;
    
    // Quality settings
    AssetQuality target_quality = AssetQuality::High;
};

// =============================================================================
// Model Asset
// =============================================================================

class ModelAsset : public Asset {
public:
    ASSET_TYPE(ModelAsset, 1002)
    
    ModelAsset();
    ~ModelAsset() override;
    
    // Asset interface
    AssetLoadResult load(const std::string& path, const AssetLoadParams& params = {}) override;
    void unload() override;
    bool isLoaded() const override { return model_data_ != nullptr; }
    
    uint64_t getMemoryUsage() const override;
    
    // Streaming support
    bool supportsStreaming() const override { return true; }
    void streamIn(AssetQuality quality) override;
    void streamOut() override;
    
    // Model data access
    const ModelData* getModelData() const { return model_data_.get(); }
    ModelData* getModelData() { return model_data_.get(); }
    
    // LOD management
    const Mesh* getMeshForLOD(uint32_t mesh_index, uint32_t lod_level) const;
    uint32_t getLODLevelForDistance(float distance) const;
    
private:
    std::unique_ptr<ModelData> model_data_;
    AssetQuality current_quality_ = AssetQuality::High;
};

// =============================================================================
// Model Processor
// =============================================================================

class ModelProcessor {
public:
    ModelProcessor();
    ~ModelProcessor();
    
    // Main processing interface
    std::unique_ptr<ModelData> processModel(const std::string& input_path,
                                           const ModelProcessingOptions& options = {});
    
    // Individual processing steps
    std::unique_ptr<ModelData> loadFromFile(const std::string& path);
    
    bool optimizeMesh(Mesh& mesh, const ModelProcessingOptions& options);
    bool generateLODs(Mesh& mesh, const ModelProcessingOptions& options);
    bool calculateNormals(SubMesh& submesh, bool smooth = true, float smoothing_angle = 60.0f);
    bool calculateTangents(SubMesh& submesh);
    
    // Animation processing
    bool optimizeAnimations(std::vector<Animation>& animations, float tolerance = 1e-6f);
    bool compressAnimations(std::vector<Animation>& animations);
    
    // Coordinate system conversion
    bool convertCoordinateSystem(ModelData& model, ModelProcessingOptions::CoordinateSystem target);
    
    // Mesh utilities
    static void removeDuplicateVertices(SubMesh& submesh, float threshold = 1e-6f);
    static void optimizeVertexOrder(SubMesh& submesh);
    static void optimizeIndexOrder(SubMesh& submesh);
    
    // LOD generation algorithms
    static std::unique_ptr<SubMesh> generateLOD(const SubMesh& source, float reduction_ratio);
    static bool isEdgeCollapsible(const SubMesh& mesh, uint32_t edge_index);
    
    // Quality level processing
    std::unique_ptr<ModelData> processForQuality(const ModelData& source, AssetQuality quality);
    
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

// =============================================================================
// Model Format Support
// =============================================================================

class ModelFormatLoader {
public:
    virtual ~ModelFormatLoader() = default;
    
    virtual bool canLoad(const std::string& extension) const = 0;
    virtual std::unique_ptr<ModelData> load(const std::string& path) = 0;
    virtual std::string getName() const = 0;
    virtual std::vector<std::string> getSupportedExtensions() const = 0;
};

class OBJLoader : public ModelFormatLoader {
public:
    bool canLoad(const std::string& extension) const override;
    std::unique_ptr<ModelData> load(const std::string& path) override;
    std::string getName() const override { return "OBJ Loader"; }
    std::vector<std::string> getSupportedExtensions() const override;
};

class GLTFLoader : public ModelFormatLoader {
public:
    bool canLoad(const std::string& extension) const override;
    std::unique_ptr<ModelData> load(const std::string& path) override;
    std::string getName() const override { return "GLTF Loader"; }
    std::vector<std::string> getSupportedExtensions() const override;
};

class FBXLoader : public ModelFormatLoader {
public:
    bool canLoad(const std::string& extension) const override;
    std::unique_ptr<ModelData> load(const std::string& path) override;
    std::string getName() const override { return "FBX Loader"; }
    std::vector<std::string> getSupportedExtensions() const override;
};

// =============================================================================
// Model Registry
// =============================================================================

class ModelRegistry {
public:
    static ModelRegistry& instance();
    
    void registerLoader(std::unique_ptr<ModelFormatLoader> loader);
    ModelFormatLoader* getLoader(const std::string& extension) const;
    std::vector<ModelFormatLoader*> getLoaders() const;
    
    std::vector<std::string> getSupportedExtensions() const;
    bool supportsExtension(const std::string& extension) const;
    
private:
    std::vector<std::unique_ptr<ModelFormatLoader>> loaders_;
    std::unordered_map<std::string, ModelFormatLoader*> extension_to_loader_;
};

} // namespace ecscope::assets