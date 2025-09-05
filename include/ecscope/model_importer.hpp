#pragma once

/**
 * @file model_importer.hpp
 * @brief Advanced 3D Model Import System for ECScope Asset Pipeline
 * 
 * This system provides comprehensive 3D model import capabilities with support
 * for multiple formats, mesh processing, animation, and educational features.
 * 
 * Key Features:
 * - Multi-format support (OBJ, FBX, glTF, COLLADA, PLY, STL)
 * - Advanced mesh processing and optimization
 * - Animation and rigging support
 * - Material extraction and conversion
 * - Educational analysis and optimization suggestions
 * - Integration with physics and rendering systems
 * 
 * Educational Value:
 * - Demonstrates 3D data structures and algorithms
 * - Shows mesh optimization techniques
 * - Illustrates animation and rigging concepts
 * - Provides performance analysis for rendering
 * - Teaches GPU-friendly data layout
 * 
 * @author ECScope Educational ECS Framework - Asset Pipeline
 * @date 2025
 */

#include "asset_pipeline.hpp"
#include "core/types.hpp"
#include "core/math.hpp"
#include <vector>
#include <string>
#include <memory>
#include <optional>
#include <unordered_map>

namespace ecscope::assets::importers {

//=============================================================================
// 3D Model Data Structures
//=============================================================================

/**
 * @brief 3D vertex with all possible attributes
 */
struct Vertex {
    math::Vec3 position{0.0f, 0.0f, 0.0f};
    math::Vec3 normal{0.0f, 1.0f, 0.0f};
    math::Vec3 tangent{1.0f, 0.0f, 0.0f};
    math::Vec3 bitangent{0.0f, 0.0f, 1.0f};
    math::Vec2 tex_coords{0.0f, 0.0f};
    math::Vec2 tex_coords2{0.0f, 0.0f}; // Second UV set
    math::Vec4 color{1.0f, 1.0f, 1.0f, 1.0f};
    
    // Skinning data (for animated models)
    struct SkinningData {
        u32 bone_ids[4]{0, 0, 0, 0};     // Up to 4 bone influences
        f32 bone_weights[4]{1.0f, 0.0f, 0.0f, 0.0f}; // Corresponding weights
    } skinning;
    
    bool operator==(const Vertex& other) const {
        return position == other.position &&
               normal == other.normal &&
               tex_coords == other.tex_coords;
    }
    
    // Hash function for deduplication
    struct Hash {
        usize operator()(const Vertex& v) const {
            usize h1 = std::hash<f32>{}(v.position.x);
            usize h2 = std::hash<f32>{}(v.position.y);
            usize h3 = std::hash<f32>{}(v.position.z);
            return h1 ^ (h2 << 1) ^ (h3 << 2);
        }
    };
};

/**
 * @brief Material properties extracted from model files
 */
struct Material {
    std::string name;
    
    // Basic material properties
    math::Vec3 ambient{0.2f, 0.2f, 0.2f};
    math::Vec3 diffuse{0.8f, 0.8f, 0.8f};
    math::Vec3 specular{1.0f, 1.0f, 1.0f};
    math::Vec3 emissive{0.0f, 0.0f, 0.0f};
    f32 shininess{32.0f};
    f32 transparency{1.0f};
    f32 metallic{0.0f};
    f32 roughness{0.5f};
    
    // Texture maps
    std::string diffuse_map;
    std::string normal_map;
    std::string specular_map;
    std::string metallic_map;
    std::string roughness_map;
    std::string ao_map;          // Ambient occlusion
    std::string emissive_map;
    std::string height_map;      // Displacement/height
    
    // Advanced properties
    f32 ior{1.5f};               // Index of refraction
    bool double_sided{false};
    bool cast_shadows{true};
    bool receive_shadows{true};
    
    // Educational metadata
    std::string shader_type{"standard"}; // standard, pbr, toon, etc.
    std::string complexity_rating{"simple"}; // simple, moderate, complex
};

/**
 * @brief Mesh data with indices and material assignment
 */
struct Mesh {
    std::string name;
    std::vector<Vertex> vertices;
    std::vector<u32> indices;
    u32 material_index{0};
    
    // Mesh properties
    math::Vec3 bounding_box_min{0.0f, 0.0f, 0.0f};
    math::Vec3 bounding_box_max{0.0f, 0.0f, 0.0f};
    f32 surface_area{0.0f};
    u32 triangle_count{0};
    
    // Topology information
    bool has_normals{false};
    bool has_tangents{false};
    bool has_tex_coords{false};
    bool has_colors{false};
    bool has_skinning_data{false};
    bool is_manifold{true};      // Watertight mesh
    bool has_degenerate_triangles{false};
    
    void calculate_bounding_box();
    void calculate_surface_area();
    void calculate_normals();
    void calculate_tangents();
    bool validate_topology() const;
    usize get_memory_usage() const;
};

/**
 * @brief Bone data for skeletal animation
 */
struct Bone {
    std::string name;
    u32 parent_index{UINT32_MAX}; // Index to parent bone (UINT32_MAX = root)
    math::Mat4 offset_matrix{1.0f}; // Transforms from mesh space to bone space
    math::Mat4 local_transform{1.0f}; // Local transformation
    std::vector<u32> children_indices;
};

/**
 * @brief Animation keyframe data
 */
struct AnimationKeyframe {
    f32 time{0.0f};
    math::Vec3 position{0.0f, 0.0f, 0.0f};
    math::Quat rotation{0.0f, 0.0f, 0.0f, 1.0f};
    math::Vec3 scale{1.0f, 1.0f, 1.0f};
};

/**
 * @brief Animation channel for a single bone
 */
struct AnimationChannel {
    u32 bone_index{0};
    std::vector<AnimationKeyframe> keyframes;
    
    // Interpolation methods
    enum class Interpolation {
        Linear,
        Step,
        CubicSpline
    } interpolation{Interpolation::Linear};
};

/**
 * @brief Animation clip with all channels
 */
struct Animation {
    std::string name;
    f32 duration{0.0f};
    f32 ticks_per_second{30.0f};
    std::vector<AnimationChannel> channels;
    
    // Educational metadata
    u32 keyframe_count{0};
    f32 complexity_score{0.0f}; // Based on number of bones and keyframes
    std::string animation_type{"unknown"}; // walk, run, idle, etc.
};

/**
 * @brief Complete 3D model with all components
 */
struct Model3D {
    std::string name;
    std::vector<Mesh> meshes;
    std::vector<Material> materials;
    
    // Skeletal animation data
    std::vector<Bone> bones;
    std::vector<Animation> animations;
    math::Mat4 root_transform{1.0f};
    
    // Model properties
    math::Vec3 bounding_box_min{0.0f, 0.0f, 0.0f};
    math::Vec3 bounding_box_max{0.0f, 0.0f, 0.0f};
    f32 bounding_sphere_radius{0.0f};
    math::Vec3 center{0.0f, 0.0f, 0.0f};
    
    // Performance metrics
    u32 total_vertices{0};
    u32 total_triangles{0};
    usize memory_usage_bytes{0};
    
    // Educational classification
    enum class ComplexityLevel {
        Simple,      // < 1K triangles, no animation
        Moderate,    // 1K-10K triangles, basic animation
        Complex,     // 10K-100K triangles, complex animation
        HighPoly     // > 100K triangles, advanced features
    } complexity{ComplexityLevel::Simple};
    
    std::vector<std::string> features; // List of features (animation, textures, etc.)
    
    void calculate_bounds();
    void calculate_statistics();
    void optimize_for_rendering();
    bool validate_model() const;
};

//=============================================================================
// Model Analysis and Educational Insights
//=============================================================================

/**
 * @brief Comprehensive model analysis for educational purposes
 */
struct ModelAnalysis {
    // Geometry analysis
    struct GeometryInfo {
        u32 vertex_count{0};
        u32 triangle_count{0};
        u32 mesh_count{0};
        f32 triangle_quality_score{1.0f}; // Based on aspect ratios
        bool has_degenerate_triangles{false};
        bool is_watertight{true};
        f32 geometric_complexity{0.0f}; // Based on surface curvature
        
        // UV mapping analysis
        bool has_uv_coordinates{false};
        f32 uv_coverage{1.0f}; // How much of UV space is used
        bool has_uv_overlaps{false};
        f32 texture_density{1.0f}; // Texels per world unit
        
    } geometry;
    
    // Material analysis
    struct MaterialInfo {
        u32 material_count{0};
        bool uses_pbr_materials{false};
        bool has_texture_maps{false};
        std::vector<std::string> used_map_types;
        f32 material_complexity{0.0f};
        
    } materials;
    
    // Animation analysis
    struct AnimationInfo {
        bool has_skeletal_animation{false};
        u32 bone_count{0};
        u32 animation_count{0};
        f32 total_animation_time{0.0f};
        u32 total_keyframes{0};
        f32 animation_complexity{0.0f};
        
    } animation;
    
    // Performance analysis
    struct PerformanceInfo {
        usize memory_usage_estimate{0};
        usize gpu_memory_estimate{0};
        f32 render_cost_score{1.0f}; // Estimated render cost
        std::vector<std::string> performance_warnings;
        std::vector<std::string> optimization_suggestions;
        
        // Rendering compatibility
        bool suitable_for_realtime{true};
        bool needs_level_of_detail{false};
        bool suitable_for_mobile{true};
        
    } performance;
    
    // Educational insights
    struct EducationalInfo {
        Model3D::ComplexityLevel complexity_level{Model3D::ComplexityLevel::Simple};
        std::string learning_focus; // What this model is good for teaching
        std::vector<std::string> concepts_demonstrated;
        std::vector<std::string> techniques_used;
        f32 educational_value{0.5f}; // 0.0-1.0 scale
        
        // Suggested exercises
        std::vector<std::string> suggested_exercises;
        
    } educational;
    
    // Overall assessment
    f32 overall_quality{1.0f};
    std::string quality_summary;
    std::vector<std::string> issues_found;
};

//=============================================================================
// Model Import Settings
//=============================================================================

/**
 * @brief Extended model import settings with all processing options
 */
struct ModelImportSettings : public ImportSettings {
    // Geometry processing
    f32 scale_factor{1.0f};
    bool generate_normals{false};
    bool generate_tangents{false};
    f32 smoothing_angle{45.0f};
    bool flip_normals{false};
    bool flip_winding_order{false};
    
    // Mesh optimization
    bool optimize_meshes{true};
    bool merge_vertices{true};
    f32 vertex_merge_threshold{0.00001f};
    bool remove_degenerate_triangles{true};
    bool optimize_vertex_cache{true};
    bool optimize_vertex_fetch{true};
    
    // Material processing
    bool import_materials{true};
    bool convert_to_pbr{false};
    std::string texture_search_path;
    bool embed_textures{false};
    
    // Animation settings
    bool import_animations{true};
    f32 animation_sample_rate{30.0f};
    bool optimize_animations{true};
    bool remove_redundant_keyframes{true};
    
    // LOD generation
    bool generate_lods{false};
    std::vector<f32> lod_reduction_factors{0.75f, 0.5f, 0.25f};
    
    // Educational settings
    bool calculate_educational_metrics{true};
    bool generate_learning_suggestions{true};
    bool create_wireframe_version{false};
    
    // Validation settings
    bool strict_validation{false};
    bool warn_about_issues{true};
    f32 max_acceptable_triangle_count{100000.0f};
    
    std::string serialize() const override;
    bool deserialize(const std::string& data) override;
    std::string calculate_hash() const override;
};

//=============================================================================
// Mesh Processing Utilities
//=============================================================================

/**
 * @brief Advanced mesh processing algorithms
 */
class MeshProcessor {
public:
    // Normal calculation
    static void calculate_smooth_normals(Mesh& mesh, f32 smoothing_angle = 45.0f);
    static void calculate_flat_normals(Mesh& mesh);
    
    // Tangent calculation
    static bool calculate_tangents(Mesh& mesh);
    static bool calculate_tangents_mikktspace(Mesh& mesh); // Industry standard
    
    // Mesh optimization
    static u32 merge_duplicate_vertices(Mesh& mesh, f32 threshold = 0.00001f);
    static u32 remove_degenerate_triangles(Mesh& mesh);
    static void optimize_vertex_cache(Mesh& mesh);
    static void optimize_vertex_fetch(Mesh& mesh);
    
    // Topology operations
    static bool make_manifold(Mesh& mesh);
    static std::vector<std::vector<u32>> find_connected_components(const Mesh& mesh);
    
    // Quality metrics
    static f32 calculate_triangle_quality(const Mesh& mesh);
    static f32 calculate_mesh_compactness(const Mesh& mesh);
    static bool validate_mesh_topology(const Mesh& mesh);
    
    // LOD generation
    static Mesh generate_simplified_mesh(const Mesh& mesh, f32 reduction_factor);
    static std::vector<Mesh> generate_lod_chain(const Mesh& mesh, const std::vector<f32>& factors);
    
    // Educational utilities
    struct ProcessingStep {
        std::string operation;
        std::string description;
        u32 vertices_before;
        u32 vertices_after;
        u32 triangles_before;
        u32 triangles_after;
        f64 processing_time_ms;
        std::string quality_impact;
    };
    
    static std::vector<ProcessingStep> process_with_logging(
        Mesh& mesh,
        const ModelImportSettings& settings
    );
    
private:
    // Internal optimization algorithms
    static void forsyth_vertex_cache_optimization(std::vector<u32>& indices);
    static f32 calculate_triangle_area(const math::Vec3& a, const math::Vec3& b, const math::Vec3& c);
    static f32 calculate_triangle_aspect_ratio(const math::Vec3& a, const math::Vec3& b, const math::Vec3& c);
};

//=============================================================================
// Model Importer Base Class
//=============================================================================

/**
 * @brief Base class for 3D model importers with common functionality
 */
class Model3DImporter : public AssetImporter {
protected:
    // Common processing pipeline
    ImportResult process_model_data(
        Model3D model,
        const ModelImportSettings& settings,
        const std::filesystem::path& source_path,
        memory::MemoryTracker* memory_tracker
    ) const;
    
    // Analysis helpers
    ModelAnalysis analyze_model_data(const Model3D& model) const;
    void analyze_geometry(const Model3D& model, ModelAnalysis::GeometryInfo& info) const;
    void analyze_materials(const Model3D& model, ModelAnalysis::MaterialInfo& info) const;
    void analyze_animations(const Model3D& model, ModelAnalysis::AnimationInfo& info) const;
    void analyze_performance(const Model3D& model, ModelAnalysis::PerformanceInfo& info) const;
    void generate_educational_insights(const Model3D& model, ModelAnalysis::EducationalInfo& info) const;
    
    // Validation
    bool validate_model_data(const Model3D& model, std::vector<std::string>& issues) const;
    
    // Memory tracking helpers
    void track_model_memory(
        const Model3D& model,
        memory::MemoryTracker* tracker,
        const std::string& category
    ) const;
    
public:
    AssetType asset_type() const override { return AssetType::Model; }
    
    // Educational interface
    std::string get_educational_description() const override;
    std::vector<std::string> get_learning_objectives() const override;
    
    // Model-specific analysis
    virtual ModelAnalysis analyze_model_file(const std::filesystem::path& file_path) const;
    virtual std::string generate_model_report(const std::filesystem::path& file_path) const;
    
    // Performance estimation
    struct PerformanceEstimate {
        f32 load_time_estimate_ms{0.0f};
        usize memory_usage_estimate{0};
        f32 render_cost_score{1.0f};
        bool suitable_for_realtime{true};
    };
    
    virtual PerformanceEstimate estimate_performance(const std::filesystem::path& file_path) const;
};

//=============================================================================
// Format-Specific Importers
//=============================================================================

/**
 * @brief OBJ format importer (simple, educational)
 */
class OBJImporter : public Model3DImporter {
public:
    std::vector<std::string> supported_extensions() const override;
    bool can_import(const std::filesystem::path& file_path) const override;
    
    ImportResult import_asset(
        const std::filesystem::path& source_path,
        const ImportSettings& settings,
        memory::MemoryTracker* memory_tracker = nullptr
    ) const override;
    
    std::unique_ptr<ImportSettings> create_default_settings() const override;
    core::Result<void, const char*> validate_file(const std::filesystem::path& file_path) const override;
    std::string analyze_file(const std::filesystem::path& file_path) const override;
    
private:
    core::Result<Model3D, std::string> parse_obj_file(const std::filesystem::path& file_path) const;
    bool parse_mtl_file(const std::filesystem::path& mtl_path, std::vector<Material>& materials) const;
};

/**
 * @brief glTF format importer (modern, industry standard)
 */
class GLTFImporter : public Model3DImporter {
public:
    std::vector<std::string> supported_extensions() const override;
    bool can_import(const std::filesystem::path& file_path) const override;
    
    ImportResult import_asset(
        const std::filesystem::path& source_path,
        const ImportSettings& settings,
        memory::MemoryTracker* memory_tracker = nullptr
    ) const override;
    
    std::unique_ptr<ImportSettings> create_default_settings() const override;
    core::Result<void, const char*> validate_file(const std::filesystem::path& file_path) const override;
    std::string analyze_file(const std::filesystem::path& file_path) const override;
    
private:
    core::Result<Model3D, std::string> parse_gltf_file(const std::filesystem::path& file_path) const;
    core::Result<Model3D, std::string> parse_glb_file(const std::filesystem::path& file_path) const;
};

/**
 * @brief FBX format importer (industry standard, complex)
 */
class FBXImporter : public Model3DImporter {
public:
    std::vector<std::string> supported_extensions() const override;
    bool can_import(const std::filesystem::path& file_path) const override;
    
    ImportResult import_asset(
        const std::filesystem::path& source_path,
        const ImportSettings& settings,
        memory::MemoryTracker* memory_tracker = nullptr
    ) const override;
    
    std::unique_ptr<ImportSettings> create_default_settings() const override;
    core::Result<void, const char*> validate_file(const std::filesystem::path& file_path) const override;
    std::string analyze_file(const std::filesystem::path& file_path) const override;
    
    // FBX requires external library integration
    bool is_library_available() const;
    std::string get_library_info() const;
    
private:
    core::Result<Model3D, std::string> parse_fbx_file(const std::filesystem::path& file_path) const;
};

/**
 * @brief Simple format importer for STL, PLY files (educational)
 */
class SimpleFormatImporter : public Model3DImporter {
public:
    std::vector<std::string> supported_extensions() const override;
    bool can_import(const std::filesystem::path& file_path) const override;
    
    ImportResult import_asset(
        const std::filesystem::path& source_path,
        const ImportSettings& settings,
        memory::MemoryTracker* memory_tracker = nullptr
    ) const override;
    
    std::unique_ptr<ImportSettings> create_default_settings() const override;
    core::Result<void, const char*> validate_file(const std::filesystem::path& file_path) const override;
    std::string analyze_file(const std::filesystem::path& file_path) const override;
    
private:
    core::Result<Model3D, std::string> parse_stl_file(const std::filesystem::path& file_path) const;
    core::Result<Model3D, std::string> parse_ply_file(const std::filesystem::path& file_path) const;
};

} // namespace ecscope::assets::importers