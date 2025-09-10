#pragma once

#include "plugin_interface.hpp"
#include "plugin_context.hpp"
#include <memory>
#include <vector>
#include <unordered_map>
#include <functional>
#include <string>
#include <any>

// Forward declarations for rendering system
namespace ecscope::rendering {
    class Renderer;
    class ResourceManager;
    class Shader;
    class Texture;
    class Mesh;
    class Material;
    class RenderTarget;
    class Pipeline;
    struct RenderCommand;
    struct Vertex;
}

namespace ecscope::assets {
    class AssetManager;
    template<typename T> class Asset;
    class AssetLoader;
}

namespace ecscope::plugins {

    /**
     * @brief Plugin shader management
     */
    class PluginShaderManager {
    public:
        PluginShaderManager();
        ~PluginShaderManager();
        
        // Shader registration
        bool register_shader(const std::string& plugin_name, const std::string& shader_name, 
                           const std::string& vertex_source, const std::string& fragment_source,
                           const std::string& geometry_source = "", const std::string& compute_source = "");
        
        bool register_shader_from_files(const std::string& plugin_name, const std::string& shader_name,
                                       const std::string& vertex_file, const std::string& fragment_file,
                                       const std::string& geometry_file = "", const std::string& compute_file = "");
        
        void unregister_shader(const std::string& plugin_name, const std::string& shader_name);
        void unregister_all_shaders(const std::string& plugin_name);
        
        // Shader access
        rendering::Shader* get_shader(const std::string& plugin_name, const std::string& shader_name);
        std::vector<std::string> get_plugin_shaders(const std::string& plugin_name) const;
        bool has_shader(const std::string& plugin_name, const std::string& shader_name) const;
        
        // Shader compilation and validation
        bool compile_shader(const std::string& plugin_name, const std::string& shader_name);
        bool validate_shader(const std::string& plugin_name, const std::string& shader_name);
        std::string get_shader_error(const std::string& plugin_name, const std::string& shader_name) const;
        
        // Hot reloading
        bool enable_hot_reload(const std::string& plugin_name, const std::string& shader_name, bool enable = true);
        void check_for_changes();
        
    private:
        struct ShaderInfo {
            std::string plugin_name;
            std::string shader_name;
            std::string vertex_source;
            std::string fragment_source;
            std::string geometry_source;
            std::string compute_source;
            std::vector<std::string> source_files;
            std::unique_ptr<rendering::Shader> shader;
            bool hot_reload_enabled{false};
            uint64_t last_modified{0};
            std::string last_error;
        };
        
        std::unordered_map<std::string, std::unordered_map<std::string, ShaderInfo>> plugin_shaders_;
        mutable std::mutex shaders_mutex_;
        
        bool compile_shader_impl(ShaderInfo& info);
        uint64_t get_file_modification_time(const std::string& file_path) const;
    };
    
    /**
     * @brief Plugin texture and asset management
     */
    class PluginAssetManager {
    public:
        PluginAssetManager();
        ~PluginAssetManager();
        
        // Asset registration and loading
        bool register_texture(const std::string& plugin_name, const std::string& texture_name, const std::string& file_path);
        bool register_mesh(const std::string& plugin_name, const std::string& mesh_name, const std::string& file_path);
        bool register_material(const std::string& plugin_name, const std::string& material_name, const std::string& config_path);
        
        // Asset access
        rendering::Texture* get_texture(const std::string& plugin_name, const std::string& texture_name);
        rendering::Mesh* get_mesh(const std::string& plugin_name, const std::string& mesh_name);
        rendering::Material* get_material(const std::string& plugin_name, const std::string& material_name);
        
        // Asset queries
        std::vector<std::string> get_plugin_textures(const std::string& plugin_name) const;
        std::vector<std::string> get_plugin_meshes(const std::string& plugin_name) const;
        std::vector<std::string> get_plugin_materials(const std::string& plugin_name) const;
        
        // Asset cleanup
        void unregister_asset(const std::string& plugin_name, const std::string& asset_name);
        void unregister_all_assets(const std::string& plugin_name);
        
        // Asset streaming and caching
        void set_cache_policy(const std::string& plugin_name, bool enable_caching, size_t max_cache_size);
        bool preload_plugin_assets(const std::string& plugin_name);
        void unload_plugin_assets(const std::string& plugin_name);
        
        // Asset dependencies
        bool add_asset_dependency(const std::string& plugin_name, const std::string& asset_name, const std::string& dependency_name);
        std::vector<std::string> get_asset_dependencies(const std::string& plugin_name, const std::string& asset_name) const;
        
    private:
        template<typename AssetType>
        struct AssetInfo {
            std::string plugin_name;
            std::string asset_name;
            std::string file_path;
            std::unique_ptr<AssetType> asset;
            std::vector<std::string> dependencies;
            bool loaded{false};
            uint64_t last_access_time{0};
        };
        
        std::unordered_map<std::string, std::unordered_map<std::string, AssetInfo<rendering::Texture>>> plugin_textures_;
        std::unordered_map<std::string, std::unordered_map<std::string, AssetInfo<rendering::Mesh>>> plugin_meshes_;
        std::unordered_map<std::string, std::unordered_map<std::string, AssetInfo<rendering::Material>>> plugin_materials_;
        
        mutable std::mutex assets_mutex_;
        
        template<typename AssetType>
        bool load_asset(AssetInfo<AssetType>& info);
        
        void update_access_time(const std::string& plugin_name, const std::string& asset_name);
    };
    
    /**
     * @brief Plugin render pipeline management
     */
    class PluginRenderPipeline {
    public:
        struct RenderPass {
            std::string name;
            std::string plugin_name;
            int priority{1000};
            std::function<void(rendering::Renderer*)> render_function;
            std::vector<std::string> required_resources;
            std::vector<std::string> output_targets;
            bool enabled{true};
        };
        
        PluginRenderPipeline();
        ~PluginRenderPipeline();
        
        // Render pass registration
        bool register_render_pass(const std::string& plugin_name, const RenderPass& pass);
        void unregister_render_pass(const std::string& plugin_name, const std::string& pass_name);
        void unregister_all_render_passes(const std::string& plugin_name);
        
        // Render pass management
        void set_render_pass_priority(const std::string& plugin_name, const std::string& pass_name, int priority);
        void enable_render_pass(const std::string& plugin_name, const std::string& pass_name, bool enable = true);
        bool is_render_pass_enabled(const std::string& plugin_name, const std::string& pass_name) const;
        
        // Pipeline execution
        void execute_render_passes(rendering::Renderer* renderer);
        std::vector<RenderPass*> get_render_passes_by_priority() const;
        
        // Resource management
        bool create_render_target(const std::string& plugin_name, const std::string& target_name, 
                                 int width, int height, const std::string& format = "RGBA8");
        rendering::RenderTarget* get_render_target(const std::string& plugin_name, const std::string& target_name);
        void destroy_render_target(const std::string& plugin_name, const std::string& target_name);
        
        // Debug and profiling
        void enable_profiling(bool enable = true) { profiling_enabled_ = enable; }
        struct PassProfile {
            std::string pass_name;
            double execution_time_ms{0.0};
            uint64_t draw_calls{0};
            uint64_t vertices_drawn{0};
        };
        std::vector<PassProfile> get_profiling_data() const;
        void clear_profiling_data();
        
    private:
        std::unordered_map<std::string, std::vector<RenderPass>> plugin_render_passes_;
        std::unordered_map<std::string, std::unordered_map<std::string, std::unique_ptr<rendering::RenderTarget>>> plugin_render_targets_;
        
        mutable std::mutex pipeline_mutex_;
        bool profiling_enabled_{false};
        mutable std::vector<PassProfile> profiling_data_;
        
        void sort_render_passes();
        void profile_render_pass(const RenderPass& pass, rendering::Renderer* renderer);
    };
    
    /**
     * @brief Plugin GUI integration
     */
    class PluginGUIManager {
    public:
        PluginGUIManager();
        ~PluginGUIManager();
        
        // GUI element registration
        bool register_gui_window(const std::string& plugin_name, const std::string& window_name,
                                std::function<void()> render_function);
        bool register_gui_menu(const std::string& plugin_name, const std::string& menu_name,
                              std::function<void()> render_function);
        bool register_gui_toolbar(const std::string& plugin_name, const std::string& toolbar_name,
                                 std::function<void()> render_function);
        
        // GUI management
        void unregister_gui_element(const std::string& plugin_name, const std::string& element_name);
        void unregister_all_gui_elements(const std::string& plugin_name);
        
        void enable_gui_element(const std::string& plugin_name, const std::string& element_name, bool enable = true);
        bool is_gui_element_enabled(const std::string& plugin_name, const std::string& element_name) const;
        
        // GUI rendering
        void render_plugin_gui(const std::string& plugin_name);
        void render_all_plugin_gui();
        
        // GUI events
        void set_window_close_callback(const std::string& plugin_name, const std::string& window_name,
                                      std::function<bool()> callback); // Return false to prevent closing
        
    private:
        enum class GUIElementType { Window, Menu, Toolbar };
        
        struct GUIElement {
            std::string plugin_name;
            std::string element_name;
            GUIElementType type;
            std::function<void()> render_function;
            std::function<bool()> close_callback;
            bool enabled{true};
        };
        
        std::unordered_map<std::string, std::vector<GUIElement>> plugin_gui_elements_;
        mutable std::mutex gui_mutex_;
    };
    
    /**
     * @brief Main rendering integration manager
     */
    class RenderingPluginIntegration {
    public:
        RenderingPluginIntegration();
        ~RenderingPluginIntegration();
        
        // Initialization
        bool initialize(rendering::Renderer* renderer, rendering::ResourceManager* resource_manager,
                       assets::AssetManager* asset_manager);
        void shutdown();
        bool is_initialized() const { return initialized_; }
        
        // Manager access
        PluginShaderManager* get_shader_manager() { return shader_manager_.get(); }
        PluginAssetManager* get_asset_manager() { return asset_manager_.get(); }
        PluginRenderPipeline* get_render_pipeline() { return render_pipeline_.get(); }
        PluginGUIManager* get_gui_manager() { return gui_manager_.get(); }
        
        // Plugin integration
        bool integrate_plugin(const std::string& plugin_name, IPlugin* plugin);
        void unintegrate_plugin(const std::string& plugin_name);
        bool is_plugin_integrated(const std::string& plugin_name) const;
        
        // High-level rendering operations
        bool create_plugin_material(const std::string& plugin_name, const std::string& material_name,
                                   const std::string& shader_name, const std::map<std::string, std::any>& parameters = {});
        
        bool submit_render_command(const std::string& plugin_name, const rendering::RenderCommand& command);
        void submit_debug_geometry(const std::string& plugin_name, const std::vector<rendering::Vertex>& vertices,
                                  const std::vector<uint32_t>& indices, const std::string& material_name = "");
        
        // Resource sharing
        bool share_texture(const std::string& from_plugin, const std::string& to_plugin, 
                          const std::string& texture_name, const std::string& shared_name = "");
        bool share_shader(const std::string& from_plugin, const std::string& to_plugin,
                         const std::string& shader_name, const std::string& shared_name = "");
        
        // Render hooks
        void add_pre_render_hook(const std::string& plugin_name, std::function<void(rendering::Renderer*)> hook);
        void add_post_render_hook(const std::string& plugin_name, std::function<void(rendering::Renderer*)> hook);
        void remove_render_hooks(const std::string& plugin_name);
        
        // Frame lifecycle
        void begin_frame();
        void end_frame();
        void render_frame();
        
        // Statistics and monitoring
        struct RenderingStatistics {
            size_t total_plugin_shaders{0};
            size_t total_plugin_textures{0};
            size_t total_plugin_meshes{0};
            size_t total_plugin_materials{0};
            size_t total_render_passes{0};
            size_t total_gui_elements{0};
            uint64_t frame_number{0};
            double average_frame_time_ms{0.0};
            uint64_t total_draw_calls{0};
            uint64_t total_vertices{0};
        };
        
        RenderingStatistics get_statistics() const;
        std::string generate_rendering_report() const;
        
        // Debug features
        void enable_debug_rendering(bool enable = true) { debug_rendering_enabled_ = enable; }
        void enable_wireframe_mode(const std::string& plugin_name, bool enable = true);
        void set_debug_camera_override(bool enable = true) { debug_camera_override_ = enable; }
        
    private:
        std::unique_ptr<PluginShaderManager> shader_manager_;
        std::unique_ptr<PluginAssetManager> asset_manager_;
        std::unique_ptr<PluginRenderPipeline> render_pipeline_;
        std::unique_ptr<PluginGUIManager> gui_manager_;
        
        // System references
        rendering::Renderer* renderer_{nullptr};
        rendering::ResourceManager* resource_manager_{nullptr};
        assets::AssetManager* core_asset_manager_{nullptr};
        
        // Plugin tracking
        std::unordered_set<std::string> integrated_plugins_;
        std::unordered_map<std::string, std::vector<std::function<void(rendering::Renderer*)>>> pre_render_hooks_;
        std::unordered_map<std::string, std::vector<std::function<void(rendering::Renderer*)>>> post_render_hooks_;
        
        // Frame data
        uint64_t current_frame_{0};
        double frame_start_time_{0.0};
        mutable RenderingStatistics stats_;
        
        // Debug features
        bool debug_rendering_enabled_{false};
        bool debug_camera_override_{false};
        std::unordered_set<std::string> wireframe_plugins_;
        
        // Synchronization
        mutable std::mutex integration_mutex_;
        
        bool initialized_{false};
        
        // Internal helpers
        void cleanup_plugin_rendering_data(const std::string& plugin_name);
        void update_statistics();
    };
    
    /**
     * @brief Helper class for plugin rendering integration
     */
    class PluginRenderingHelper {
    public:
        PluginRenderingHelper(const std::string& plugin_name, RenderingPluginIntegration* integration, 
                             PluginContext* context);
        
        // Shader operations
        bool create_shader(const std::string& shader_name, const std::string& vertex_source, 
                          const std::string& fragment_source, const std::string& geometry_source = "");
        bool load_shader_from_files(const std::string& shader_name, const std::string& vertex_file,
                                   const std::string& fragment_file, const std::string& geometry_file = "");
        rendering::Shader* get_shader(const std::string& shader_name);
        
        // Texture operations
        bool load_texture(const std::string& texture_name, const std::string& file_path);
        rendering::Texture* get_texture(const std::string& texture_name);
        
        // Material operations
        bool create_material(const std::string& material_name, const std::string& shader_name,
                            const std::map<std::string, std::any>& parameters = {});
        rendering::Material* get_material(const std::string& material_name);
        
        // Render pass operations
        bool add_render_pass(const std::string& pass_name, std::function<void(rendering::Renderer*)> render_func,
                            int priority = 1000);
        void remove_render_pass(const std::string& pass_name);
        
        // GUI operations
        bool add_gui_window(const std::string& window_name, std::function<void()> render_func);
        bool add_gui_menu(const std::string& menu_name, std::function<void()> render_func);
        void remove_gui_element(const std::string& element_name);
        
        // Rendering commands
        bool submit_render_command(const rendering::RenderCommand& command);
        void draw_debug_line(const std::array<float, 3>& start, const std::array<float, 3>& end, 
                            const std::array<float, 4>& color = {1.0f, 1.0f, 1.0f, 1.0f});
        void draw_debug_sphere(const std::array<float, 3>& center, float radius, 
                              const std::array<float, 4>& color = {1.0f, 1.0f, 1.0f, 1.0f});
        
    private:
        std::string plugin_name_;
        RenderingPluginIntegration* integration_;
        PluginContext* context_;
    };
    
} // namespace ecscope::plugins