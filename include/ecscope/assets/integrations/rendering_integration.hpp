#pragma once

#include "../concrete_assets.hpp"
#include "../../rendering/renderer.hpp"
#include <memory>

namespace ecscope::assets::integrations {

    // Rendering system integration for assets
    class RenderingAssetIntegration {
    public:
        explicit RenderingAssetIntegration(rendering::Renderer* renderer);
        ~RenderingAssetIntegration();

        // Texture integration
        bool upload_texture(std::shared_ptr<TextureAsset> texture);
        bool update_texture(std::shared_ptr<TextureAsset> texture);
        void release_texture(std::shared_ptr<TextureAsset> texture);
        
        // Mesh integration  
        bool upload_mesh(std::shared_ptr<ModelAsset> model, size_t mesh_index);
        bool update_mesh(std::shared_ptr<ModelAsset> model, size_t mesh_index);
        void release_mesh(std::shared_ptr<ModelAsset> model, size_t mesh_index);
        
        // Shader integration
        bool compile_shader(std::shared_ptr<ShaderAsset> shader);
        bool link_shader_program(const std::vector<std::shared_ptr<ShaderAsset>>& shaders,
                                uint32_t& program_handle);
        void release_shader(std::shared_ptr<ShaderAsset> shader);
        
        // Material integration
        bool setup_material(std::shared_ptr<MaterialAsset> material);
        void bind_material(std::shared_ptr<MaterialAsset> material);
        void release_material(std::shared_ptr<MaterialAsset> material);
        
        // Automatic asset management
        void enable_auto_upload(bool enable) { m_auto_upload = enable; }
        bool is_auto_upload_enabled() const { return m_auto_upload; }
        
        void enable_auto_release(bool enable) { m_auto_release = enable; }
        bool is_auto_release_enabled() const { return m_auto_release; }
        
        // Resource tracking
        size_t get_uploaded_texture_count() const;
        size_t get_uploaded_mesh_count() const;
        size_t get_compiled_shader_count() const;
        size_t get_gpu_memory_usage() const;
        
        // Performance monitoring
        void set_upload_budget_ms(float budget_ms) { m_upload_budget_ms = budget_ms; }
        float get_upload_budget_ms() const { return m_upload_budget_ms; }
        
        void update_frame_budget();
        bool can_afford_upload(size_t estimated_upload_time_ms) const;
        
    private:
        rendering::Renderer* m_renderer;
        
        // Configuration
        bool m_auto_upload = true;
        bool m_auto_release = true;
        float m_upload_budget_ms = 2.0f; // 2ms per frame
        
        // Resource tracking
        std::unordered_set<AssetID> m_uploaded_textures;
        std::unordered_set<AssetID> m_uploaded_meshes; 
        std::unordered_set<AssetID> m_compiled_shaders;
        
        // Performance tracking
        float m_frame_upload_time_used = 0.0f;
        std::chrono::steady_clock::time_point m_frame_start_time;
        
        // Internal methods
        uint32_t create_texture_handle(const TextureAsset& texture);
        uint32_t create_vertex_buffer(const processors::MeshData& mesh);
        uint32_t create_index_buffer(const processors::MeshData& mesh);
        uint32_t compile_shader_stage(const ShaderAsset& shader);
        
        void track_upload_time(std::chrono::milliseconds upload_time);
    };

    // Asset loading callbacks for rendering integration
    class RenderingAssetCallbacks {
    public:
        explicit RenderingAssetCallbacks(RenderingAssetIntegration* integration);
        
        // Asset load callbacks
        void on_texture_loaded(AssetHandle handle);
        void on_model_loaded(AssetHandle handle);
        void on_shader_loaded(AssetHandle handle);
        void on_material_loaded(AssetHandle handle);
        
        // Asset unload callbacks
        void on_texture_unloaded(AssetID asset_id);
        void on_model_unloaded(AssetID asset_id);
        void on_shader_unloaded(AssetID asset_id);
        void on_material_unloaded(AssetID asset_id);
        
        // Registration with asset manager
        void register_callbacks(AssetManager& manager);
        void unregister_callbacks(AssetManager& manager);
        
    private:
        RenderingAssetIntegration* m_integration;
    };

    // Render asset components for ECS integration
    namespace components {
        
        struct RenderableComponent {
            AssetHandle model;
            AssetHandle material;
            bool visible = true;
            uint32_t render_layer = 0;
            float sort_order = 0.0f;
            
            // Transform (could be separate component)
            std::array<float, 16> transform_matrix = {
                1, 0, 0, 0,
                0, 1, 0, 0, 
                0, 0, 1, 0,
                0, 0, 0, 1
            };
        };
        
        struct SpriteComponent {
            AssetHandle texture;
            std::array<float, 4> uv_rect = {0, 0, 1, 1}; // u_min, v_min, u_max, v_max
            std::array<float, 4> color = {1, 1, 1, 1}; // r, g, b, a
            std::array<float, 2> size = {1, 1}; // width, height
            float rotation = 0.0f;
            uint32_t render_layer = 0;
            bool flip_x = false;
            bool flip_y = false;
        };
        
        struct MaterialComponent {
            AssetHandle material;
            
            // Material parameter overrides
            std::unordered_map<std::string, float> float_params;
            std::unordered_map<std::string, std::array<float, 3>> vector3_params;
            std::unordered_map<std::string, std::array<float, 4>> vector4_params;
            std::unordered_map<std::string, AssetHandle> texture_params;
        };
        
        struct AnimationComponent {
            AssetHandle animation_asset;
            float current_time = 0.0f;
            float playback_speed = 1.0f;
            bool looping = true;
            bool playing = false;
            std::string current_animation;
        };
        
    } // namespace components

    // Rendering systems for ECS
    namespace systems {
        
        class MeshRenderingSystem {
        public:
            explicit MeshRenderingSystem(RenderingAssetIntegration* integration);
            
            void update(float delta_time);
            void render();
            
            // Configuration
            void set_frustum_culling_enabled(bool enabled) { m_frustum_culling = enabled; }
            void set_lod_enabled(bool enabled) { m_lod_enabled = enabled; }
            
        private:
            RenderingAssetIntegration* m_integration;
            bool m_frustum_culling = true;
            bool m_lod_enabled = true;
            
            // Culling and LOD
            bool is_visible(const components::RenderableComponent& renderable);
            QualityLevel select_lod_level(const components::RenderableComponent& renderable);
        };
        
        class SpriteRenderingSystem {
        public:
            explicit SpriteRenderingSystem(RenderingAssetIntegration* integration);
            
            void update(float delta_time);
            void render();
            
            // Batch rendering
            void set_batching_enabled(bool enabled) { m_batching_enabled = enabled; }
            size_t get_batch_count() const { return m_batch_count; }
            
        private:
            RenderingAssetIntegration* m_integration;
            bool m_batching_enabled = true;
            size_t m_batch_count = 0;
            
            // Batching
            struct SpriteBatch {
                AssetHandle texture;
                std::vector<components::SpriteComponent*> sprites;
            };
            
            std::vector<SpriteBatch> m_sprite_batches;
            void build_sprite_batches();
            void render_sprite_batch(const SpriteBatch& batch);
        };
        
        class MaterialSystem {
        public:
            MaterialSystem();
            
            void update(float delta_time);
            
            // Material parameter updates
            void set_global_material_parameter(const std::string& name, float value);
            void set_global_material_parameter(const std::string& name, const std::array<float, 3>& value);
            void set_global_material_parameter(const std::string& name, const std::array<float, 4>& value);
            
        private:
            std::unordered_map<std::string, float> m_global_float_params;
            std::unordered_map<std::string, std::array<float, 3>> m_global_vector3_params;
            std::unordered_map<std::string, std::array<float, 4>> m_global_vector4_params;
        };
        
    } // namespace systems

} // namespace ecscope::assets::integrations