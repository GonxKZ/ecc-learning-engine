#pragma once

#include "asset_processor.hpp"
#include <array>
#include <optional>

namespace ecscope::assets::processors {

    // Vertex attributes
    struct Vertex {
        std::array<float, 3> position = {0, 0, 0};
        std::array<float, 3> normal = {0, 1, 0};
        std::array<float, 4> tangent = {1, 0, 0, 1}; // xyz = tangent, w = handedness
        std::array<float, 2> texcoord0 = {0, 0};
        std::array<float, 2> texcoord1 = {0, 0};
        std::array<float, 4> color = {1, 1, 1, 1};
        std::array<uint32_t, 4> joints = {0, 0, 0, 0};
        std::array<float, 4> weights = {0, 0, 0, 0};
        
        bool operator==(const Vertex& other) const;
        bool operator!=(const Vertex& other) const { return !(*this == other); }
    };

    // Mesh data
    struct MeshData {
        std::vector<Vertex> vertices;
        std::vector<uint32_t> indices;
        std::string name;
        uint32_t material_index = 0;
        
        // Bounding information
        std::array<float, 3> bounding_min = {0, 0, 0};
        std::array<float, 3> bounding_max = {0, 0, 0};
        std::array<float, 3> bounding_center = {0, 0, 0};
        float bounding_radius = 0.0f;
        
        // Mesh properties
        bool has_normals = false;
        bool has_tangents = false;
        bool has_texcoords = false;
        bool has_colors = false;
        bool has_skinning = false;
        
        void calculate_bounds();
        void calculate_normals(float smooth_angle = 45.0f);
        void calculate_tangents();
    };

    // Material data
    struct MaterialData {
        std::string name;
        
        // PBR properties
        std::array<float, 4> base_color = {1, 1, 1, 1};
        float metallic = 0.0f;
        float roughness = 1.0f;
        float normal_scale = 1.0f;
        float occlusion_strength = 1.0f;
        std::array<float, 3> emissive = {0, 0, 0};
        
        // Texture references
        std::string albedo_texture;
        std::string normal_texture;
        std::string metallic_roughness_texture;
        std::string occlusion_texture;
        std::string emissive_texture;
        
        // Alpha blending
        enum class AlphaMode {
            OPAQUE,
            MASK,
            BLEND
        } alpha_mode = AlphaMode::OPAQUE;
        float alpha_cutoff = 0.5f;
        
        // Two-sided rendering
        bool double_sided = false;
    };

    // Animation data
    struct AnimationData {
        std::string name;
        float duration = 0.0f;
        
        struct Channel {
            enum class Target {
                TRANSLATION,
                ROTATION,
                SCALE,
                WEIGHTS
            } target = Target::TRANSLATION;
            
            uint32_t node_index = 0;
            std::vector<float> timestamps;
            std::vector<std::vector<float>> values; // Variable size based on target
            
            enum class Interpolation {
                LINEAR,
                STEP,
                CUBICSPLINE
            } interpolation = Interpolation::LINEAR;
        };
        
        std::vector<Channel> channels;
    };

    // Scene node
    struct SceneNode {
        std::string name;
        uint32_t parent_index = UINT32_MAX;
        std::vector<uint32_t> children;
        
        // Transform
        std::array<float, 3> translation = {0, 0, 0};
        std::array<float, 4> rotation = {0, 0, 0, 1}; // Quaternion (x, y, z, w)
        std::array<float, 3> scale = {1, 1, 1};
        std::array<float, 16> matrix = {
            1, 0, 0, 0,
            0, 1, 0, 0,
            0, 0, 1, 0,
            0, 0, 0, 1
        };
        
        // Content
        std::optional<uint32_t> mesh_index;
        std::optional<uint32_t> skin_index;
        std::optional<uint32_t> camera_index;
        std::optional<uint32_t> light_index;
        
        void calculate_matrix();
        void decompose_matrix();
    };

    // Skin data for skeletal animation
    struct SkinData {
        std::string name;
        std::vector<uint32_t> joints;
        uint32_t skeleton_root = UINT32_MAX;
        std::vector<std::array<float, 16>> inverse_bind_matrices;
    };

    // Complete model data
    struct ModelData {
        std::vector<MeshData> meshes;
        std::vector<MaterialData> materials;
        std::vector<SceneNode> nodes;
        std::vector<AnimationData> animations;
        std::vector<SkinData> skins;
        
        // Scene structure
        std::vector<uint32_t> root_nodes;
        
        // Metadata
        std::string generator;
        std::string version;
        std::string copyright;
        
        // Statistics
        uint32_t total_vertices = 0;
        uint32_t total_triangles = 0;
        size_t memory_usage = 0;
        
        void calculate_statistics();
        void validate();
    };

    // Mesh optimization settings
    struct MeshOptimizationSettings {
        bool optimize_vertices = true;
        bool optimize_overdraw = true;
        bool optimize_vertex_cache = true;
        bool optimize_vertex_fetch = true;
        
        // Simplification
        bool enable_simplification = false;
        float target_error = 0.01f;
        float target_ratio = 0.5f; // 0.5 = reduce to 50% triangles
        
        // Quantization
        bool quantize_positions = false;
        bool quantize_normals = true;
        bool quantize_texcoords = true;
        uint32_t position_bits = 16;
        uint32_t normal_bits = 10;
        uint32_t texcoord_bits = 12;
        
        // Compression
        bool compress_vertices = true;
        bool compress_indices = true;
    };

    // LOD generation settings
    struct LODSettings {
        bool generate_lods = true;
        int max_lod_levels = 4;
        float lod_ratio = 0.5f; // Each LOD has 50% triangles of previous
        float lod_error_threshold = 0.02f;
        bool preserve_borders = true;
        bool preserve_seams = true;
    };

    // Mesh processor
    class MeshProcessor : public BaseAssetProcessor {
    public:
        MeshProcessor();
        
        // AssetProcessor implementation
        std::vector<std::string> get_supported_extensions() const override;
        bool can_process(const std::string& file_path, const AssetMetadata& metadata) const override;
        bool supports_streaming() const override { return true; }
        
        ProcessingResult process(const std::vector<uint8_t>& input_data,
                               const AssetMetadata& input_metadata,
                               const ProcessingOptions& options) override;
        
        bool validate_input(const std::vector<uint8_t>& input_data,
                           const AssetMetadata& metadata) const override;
        
        AssetMetadata extract_metadata(const std::vector<uint8_t>& data,
                                     const std::string& file_path) const override;
        
        size_t estimate_output_size(size_t input_size,
                                   const ProcessingOptions& options) const override;
        
        // Mesh-specific operations
        ProcessingResult load_mesh(const std::vector<uint8_t>& data,
                                 const std::string& file_path) const;
        
        ProcessingResult optimize_mesh(const ModelData& model,
                                     const MeshOptimizationSettings& settings) const;
        
        ProcessingResult generate_lods(const ModelData& model,
                                     const LODSettings& settings) const;
        
        ProcessingResult compress_mesh(const ModelData& model) const;
        
        ProcessingResult validate_mesh(const ModelData& model) const;
        
        // Mesh analysis
        struct MeshAnalysis {
            uint32_t vertex_count = 0;
            uint32_t triangle_count = 0;
            uint32_t material_count = 0;
            uint32_t animation_count = 0;
            bool has_skinning = false;
            bool has_morph_targets = false;
            bool has_multiple_uvs = false;
            bool has_vertex_colors = false;
            float triangle_density = 0.0f; // triangles per unit area
            std::array<float, 3> bounding_size = {0, 0, 0};
            float surface_area = 0.0f;
        };
        
        MeshAnalysis analyze_mesh(const ModelData& model) const;
        
        // Format conversion
        ProcessingResult convert_to_gltf(const ModelData& model) const;
        ProcessingResult convert_to_obj(const ModelData& model) const;
        ProcessingResult convert_to_fbx(const ModelData& model) const;
        
        // Utility functions
        static void weld_vertices(std::vector<Vertex>& vertices, 
                                 std::vector<uint32_t>& indices,
                                 float threshold = 1e-6f);
        
        static void calculate_smooth_normals(std::vector<Vertex>& vertices,
                                           const std::vector<uint32_t>& indices,
                                           float angle_threshold = 45.0f);
        
        static void calculate_tangents(std::vector<Vertex>& vertices,
                                     const std::vector<uint32_t>& indices);
        
        static std::array<float, 3> calculate_face_normal(const Vertex& v0,
                                                         const Vertex& v1,
                                                         const Vertex& v2);
        
    private:
        // Format loaders
        ProcessingResult load_obj(const std::vector<uint8_t>& data) const;
        ProcessingResult load_fbx(const std::vector<uint8_t>& data) const;
        ProcessingResult load_gltf(const std::vector<uint8_t>& data) const;
        ProcessingResult load_dae(const std::vector<uint8_t>& data) const;
        ProcessingResult load_3ds(const std::vector<uint8_t>& data) const;
        ProcessingResult load_ply(const std::vector<uint8_t>& data) const;
        
        // Optimization implementations
        ModelData optimize_vertices_impl(const ModelData& model) const;
        ModelData optimize_overdraw_impl(const ModelData& model) const;
        ModelData simplify_mesh_impl(const ModelData& model, float target_ratio, float target_error) const;
        
        // LOD generation
        MeshData generate_lod_level(const MeshData& mesh, float reduction_ratio, float error_threshold) const;
        
        // Compression
        std::vector<uint8_t> compress_vertices(const std::vector<Vertex>& vertices,
                                              const MeshOptimizationSettings& settings) const;
        std::vector<uint8_t> compress_indices(const std::vector<uint32_t>& indices) const;
        
        // Validation helpers
        bool is_valid_mesh_data(const ModelData& model) const;
        bool has_degenerate_triangles(const MeshData& mesh) const;
        bool has_invalid_normals(const MeshData& mesh) const;
        
        // Geometry utilities
        float calculate_mesh_surface_area(const MeshData& mesh) const;
        float calculate_triangle_quality(const Vertex& v0, const Vertex& v1, const Vertex& v2) const;
        
        // Animation utilities
        bool validate_animation_data(const AnimationData& animation, const std::vector<SceneNode>& nodes) const;
        
        // Memory management
        class MeshProcessingCache;
        std::unique_ptr<MeshProcessingCache> m_cache;
    };

    // Mesh streaming support
    class StreamingMeshProcessor {
    public:
        StreamingMeshProcessor();
        ~StreamingMeshProcessor();
        
        // Streaming operations
        ProcessingResult start_streaming_load(const std::vector<uint8_t>& data,
                                             const std::string& file_path);
        
        ProcessingResult continue_streaming_load(float target_progress = 1.0f);
        bool is_streaming_complete() const;
        float get_streaming_progress() const;
        
        // Progressive mesh loading
        ProcessingResult load_base_mesh(); // Lowest detail first
        ProcessingResult load_additional_detail(float detail_level);
        
        void cancel_streaming();
        void reset();
        
    private:
        struct StreamingState;
        std::unique_ptr<StreamingState> m_state;
    };

    // Mesh utilities
    namespace mesh_utils {
        // Geometric calculations
        std::array<float, 3> cross_product(const std::array<float, 3>& a, const std::array<float, 3>& b);
        float dot_product(const std::array<float, 3>& a, const std::array<float, 3>& b);
        std::array<float, 3> normalize(const std::array<float, 3>& v);
        float vector_length(const std::array<float, 3>& v);
        
        // Transform operations
        std::array<float, 16> matrix_multiply(const std::array<float, 16>& a, const std::array<float, 16>& b);
        std::array<float, 3> transform_point(const std::array<float, 16>& matrix, const std::array<float, 3>& point);
        std::array<float, 3> transform_vector(const std::array<float, 16>& matrix, const std::array<float, 3>& vector);
        
        // Quaternion operations
        std::array<float, 4> quaternion_multiply(const std::array<float, 4>& a, const std::array<float, 4>& b);
        std::array<float, 4> quaternion_from_euler(float x, float y, float z);
        std::array<float, 3> quaternion_to_euler(const std::array<float, 4>& q);
        std::array<float, 16> quaternion_to_matrix(const std::array<float, 4>& q);
        
        // Bounding calculations
        struct AABB {
            std::array<float, 3> min = {FLT_MAX, FLT_MAX, FLT_MAX};
            std::array<float, 3> max = {-FLT_MAX, -FLT_MAX, -FLT_MAX};
            
            void expand(const std::array<float, 3>& point);
            void expand(const AABB& other);
            std::array<float, 3> center() const;
            std::array<float, 3> size() const;
            float radius() const;
            bool contains(const std::array<float, 3>& point) const;
            bool intersects(const AABB& other) const;
        };
        
        AABB calculate_aabb(const std::vector<Vertex>& vertices);
        
        // Hash functions for vertex deduplication
        size_t hash_vertex(const Vertex& vertex, float threshold = 1e-6f);
        bool vertices_equal(const Vertex& a, const Vertex& b, float threshold = 1e-6f);
    }

} // namespace ecscope::assets::processors