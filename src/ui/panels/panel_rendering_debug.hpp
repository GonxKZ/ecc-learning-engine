#pragma once

/**
 * @file ui/panels/panel_rendering_debug.hpp
 * @brief Comprehensive Rendering Debug UI Panel for ECScope Educational ECS Engine - Phase 7: Renderizado 2D
 * 
 * This panel provides comprehensive real-time rendering debugging, analysis, and educational tools
 * for the ECScope 2D rendering system. It demonstrates rendering concepts through interactive visualization,
 * performance analysis, and step-by-step pipeline explanations.
 * 
 * Features:
 * - Real-time rendering visualization (draw calls, batches, GPU state, texture atlases)
 * - Interactive rendering controls (debug modes, shader hot-reload, parameter adjustment)
 * - Educational rendering pipeline breakdown with step-by-step explanations
 * - Performance analysis with comprehensive GPU profiling and optimization suggestions
 * - Resource inspector for textures, shaders, buffers, and memory tracking
 * - Learning tools with interactive tutorials and rendering concept explanations
 * 
 * Educational Philosophy:
 * This panel serves as both a debugging tool and an educational platform, making rendering
 * concepts visible and interactive. It provides immediate feedback on rendering changes
 * and demonstrates the mathematical and technical principles underlying 2D graphics.
 * 
 * @author ECScope Educational ECS Framework - Phase 7: Renderizado 2D
 * @date 2024
 */

#include "../overlay.hpp"
#include "ecs/registry.hpp"
#include "renderer/renderer_2d.hpp"
#include "renderer/batch_renderer.hpp"
#include "renderer/components/render_components.hpp"
#include "renderer/resources/texture.hpp"
#include "renderer/resources/shader.hpp"
#include "core/types.hpp"
#include "core/time.hpp"
#include <vector>
#include <string>
#include <unordered_map>
#include <array>
#include <optional>
#include <chrono>
#include <memory>

namespace ecscope::ui {

/**
 * @brief Rendering Debug Panel for comprehensive 2D rendering analysis and education
 * 
 * This panel provides real-time debugging and educational tools for the 2D rendering system.
 * It's designed to be both a practical debugging tool for developers and an educational
 * resource for learning rendering concepts through interactive visualization.
 */
class RenderingDebugPanel : public Panel {
private:
    //=============================================================================
    // Panel State and Configuration
    //=============================================================================
    
    /** @brief Active tab in the rendering debug panel */
    enum class ActiveTab : u8 {
        Visualization = 0,  ///< Real-time rendering visualization
        Performance,        ///< Performance analysis and optimization
        Resources,          ///< Resource inspector and management
        Shaders,           ///< Shader editor and debugging
        Batching,          ///< Batch analysis and optimization
        Learning           ///< Educational tools and tutorials
    } active_tab_{ActiveTab::Visualization};
    
    /** @brief Rendering visualization options */
    struct VisualizationState {
        // Debug rendering modes
        bool show_wireframe{false};
        bool show_batch_colors{false};
        bool show_texture_visualization{false};
        bool show_overdraw_analysis{false};
        bool show_bounding_boxes{false};
        bool show_sprite_origins{false};
        bool show_camera_frustum{true};
        bool show_render_order{false};
        
        // OpenGL state visualization
        bool show_opengl_state{true};
        bool show_buffer_bindings{true};
        bool show_texture_bindings{true};
        bool show_shader_uniforms{true};
        bool show_render_targets{false};
        
        // Performance visualization
        bool show_draw_call_heatmap{false};
        bool show_gpu_timing_overlay{true};
        bool show_memory_usage_overlay{true};
        bool show_batch_efficiency_bars{false};
        
        // Interactive controls
        bool enable_render_step_through{false};
        bool pause_rendering{false};
        u32 current_step{0};
        u32 max_steps{0};
        
        // Color scheme and display
        f32 visualization_opacity{0.7f};
        f32 line_thickness{2.0f};
        bool use_debug_colors{true};
        bool animate_visualizations{true};
        
        // Batch coloring
        u32 batch_color_seed{12345};
        std::array<u32, 16> batch_debug_colors{};
        
        // Culling and filtering
        bool show_only_visible_objects{false};
        bool show_only_batched_objects{false};
        f32 min_sprite_size_filter{0.0f};
        f32 max_sprite_size_filter{1000.0f};
    } visualization_;
    
    /** @brief Performance monitoring and analysis */
    struct PerformanceMonitoring {
        // Real-time metrics
        static constexpr usize HISTORY_SIZE = 180; // 3 seconds at 60fps
        std::array<f32, HISTORY_SIZE> frame_times{};
        std::array<f32, HISTORY_SIZE> render_times{};
        std::array<f32, HISTORY_SIZE> gpu_times{};
        std::array<u32, HISTORY_SIZE> draw_call_counts{};
        std::array<u32, HISTORY_SIZE> vertex_counts{};
        std::array<u32, HISTORY_SIZE> batch_counts{};
        std::array<usize, HISTORY_SIZE> gpu_memory_usage{};
        std::array<f32, HISTORY_SIZE> batching_efficiency{};
        
        usize history_index{0};
        f64 last_update_time{0.0};
        f32 update_interval{1.0f/60.0f}; // 60Hz update rate
        
        // Analysis results
        f32 average_fps{60.0f};
        f32 average_frame_time{16.67f};
        f32 worst_frame_time{16.67f};
        f32 gpu_utilization{0.0f};
        f32 cpu_render_percentage{0.0f};
        const char* performance_grade{"A"};
        
        // Bottleneck analysis
        const char* primary_bottleneck{"None"};
        const char* secondary_bottleneck{"None"};
        std::vector<std::string> optimization_suggestions;
        f32 performance_score{100.0f};
        
        // Memory tracking
        usize total_gpu_memory{0};
        usize vertex_buffer_memory{0};
        usize index_buffer_memory{0};
        usize texture_memory{0};
        usize shader_memory{0};
        usize render_target_memory{0};
        
        // GPU profiling
        bool gpu_profiling_enabled{false};
        f32 vertex_shader_time{0.0f};
        f32 fragment_shader_time{0.0f};
        f32 rasterization_time{0.0f};
        f32 texture_sampling_time{0.0f};
        f32 blending_time{0.0f};
        
        // Advanced metrics
        bool show_advanced_metrics{false};
        f32 pixel_fill_rate{0.0f};
        f32 vertex_throughput{0.0f};
        f32 texture_bandwidth{0.0f};
        u32 state_changes_per_frame{0};
        u32 redundant_state_changes{0};
    } performance_;
    
    /** @brief Resource inspection and management */
    struct ResourceInspection {
        // Resource browser state
        enum class ResourceType : u8 {
            Textures = 0,
            Shaders,
            Buffers,
            RenderTargets,
            Materials
        } selected_type{ResourceType::Textures};
        
        // Texture inspection
        renderer::resources::TextureID selected_texture{0};
        bool show_texture_preview{true};
        f32 texture_preview_scale{1.0f};
        bool show_texture_mips{false};
        bool show_texture_atlas_layout{true};
        std::string texture_search_filter;
        
        // Shader inspection
        renderer::resources::ShaderID selected_shader{0};
        bool show_shader_source{true};
        bool show_shader_uniforms{true};
        bool show_shader_attributes{true};
        bool enable_shader_hot_reload{true};
        std::string shader_search_filter;
        std::string shader_compile_errors;
        
        // Buffer inspection
        u32 selected_buffer_id{0};
        bool show_vertex_data{false};
        bool show_index_data{false};
        bool show_buffer_usage_stats{true};
        u32 buffer_data_offset{0};
        u32 buffer_data_count{100};
        
        // Memory analysis
        bool show_memory_fragmentation{true};
        bool show_resource_dependencies{true};
        bool show_resource_usage_timeline{false};
        bool track_resource_hot_reloads{true};
        
        // Resource filtering
        bool show_only_used_resources{false};
        bool show_only_large_resources{false};
        usize large_resource_threshold{1024 * 1024}; // 1MB
        
        // Resource creation tools
        bool show_resource_creation_tools{false};
        struct TextureCreationParams {
            u32 width{256};
            u32 height{256};
            u32 format{0}; // GL_RGBA8
            bool generate_mipmaps{true};
            std::string debug_name;
        } texture_creation;
    } resources_;
    
    /** @brief Shader debugging and editing */
    struct ShaderDebugging {
        // Shader editor state
        bool show_vertex_shader_editor{false};
        bool show_fragment_shader_editor{false};
        std::string vertex_shader_source;
        std::string fragment_shader_source;
        std::string shader_compile_log;
        bool shader_compilation_successful{true};
        
        // Shader parameter adjustment
        struct ShaderParameter {
            std::string name;
            enum Type { Float, Vec2, Vec3, Vec4, Int, Mat4 } type;
            union {
                f32 float_value;
                f32 vec_values[4];
                i32 int_value;
                f32 matrix_values[16];
            };
            f32 min_value{-1000.0f};
            f32 max_value{1000.0f};
            bool is_dirty{false};
        };
        std::vector<ShaderParameter> custom_parameters;
        
        // Shader debugging tools
        bool show_uniform_inspector{true};
        bool show_attribute_inspector{true};
        bool enable_shader_profiling{false};
        bool highlight_expensive_instructions{false};
        
        // Hot reload settings
        bool auto_reload_on_file_change{true};
        f32 reload_check_interval{1.0f};
        f64 last_reload_check{0.0};
        std::unordered_map<std::string, std::chrono::file_time_type> shader_file_times;
        
        // Shader effect previews
        bool show_shader_effect_preview{true};
        f32 preview_animation_time{0.0f};
        bool animate_preview{true};
        
        // Error analysis
        bool show_compilation_errors{true};
        bool show_linking_errors{true};
        bool show_uniform_warnings{true};
        std::vector<std::string> shader_warnings;
        std::vector<std::string> optimization_hints;
    } shaders_;
    
    /** @brief Sprite batching analysis */
    struct BatchingAnalysis {
        // Batch visualization
        bool show_batch_breakdown{true};
        bool show_batch_efficiency_graph{true};
        bool show_texture_usage_analysis{true};
        bool show_state_change_analysis{true};
        bool highlight_inefficient_batches{true};
        
        // Batching strategy controls
        renderer::BatchingStrategy selected_strategy{renderer::BatchingStrategy::AdaptiveHybrid};
        bool enable_strategy_comparison{false};
        bool auto_optimize_batching{true};
        
        // Analysis results
        f32 current_batching_efficiency{0.0f};
        u32 total_sprites_submitted{0};
        u32 total_batches_generated{0};
        u32 average_sprites_per_batch{0};
        u32 batch_breaks_this_frame{0};
        
        // Detailed batch statistics
        struct BatchInfo {
            u32 batch_id;
            u32 sprite_count;
            renderer::resources::TextureID primary_texture;
            f32 gpu_cost_estimate;
            f32 memory_usage;
            f32 efficiency_score;
            std::string debug_name;
            renderer::Color debug_color;
        };
        std::vector<BatchInfo> current_frame_batches;
        
        // Interactive batching controls
        bool enable_manual_batching{false};
        bool force_single_batch{false};
        bool disable_batching{false};
        u32 max_sprites_per_batch_override{0};
        
        // Optimization suggestions
        std::vector<std::string> batching_suggestions;
        bool show_texture_atlas_recommendations{true};
        bool show_sorting_recommendations{true};
        bool analyze_draw_call_patterns{true};
    } batching_;
    
    /** @brief Educational features and tutorials */
    struct LearningTools {
        // Tutorial state
        enum class Tutorial : u8 {
            None = 0,
            RenderingPipeline,     ///< Graphics pipeline overview
            SpriteBatching,        ///< Batching concepts and benefits
            TextureManagement,     ///< Texture usage and optimization
            ShaderProgramming,     ///< Shader writing and debugging
            PerformanceOptimization, ///< Rendering performance techniques
            MemoryManagement       ///< GPU memory and resource management
        } active_tutorial{Tutorial::None};
        
        usize tutorial_step{0};
        bool show_conceptual_diagrams{true};
        bool show_mathematical_explanations{false};
        bool show_code_examples{true};
        bool interactive_examples_enabled{true};
        
        // Concept explanations database
        std::unordered_map<std::string, std::string> concept_explanations;
        std::string selected_concept;
        std::string concept_search_filter;
        
        // Interactive experiments
        struct RenderingExperiment {
            std::string name;
            std::string description;
            std::function<void()> setup_function;
            std::function<void()> cleanup_function;
            bool is_active;
            bool requires_restart;
        };
        std::vector<RenderingExperiment> available_experiments;
        i32 current_experiment{-1};
        
        // Educational statistics
        bool track_learning_progress{true};
        usize concepts_explored{0};
        usize tutorials_completed{0};
        usize experiments_run{0};
        f64 total_learning_time{0.0};
        
        // Reference materials
        bool show_opengl_reference{false};
        bool show_performance_guidelines{true};
        bool show_best_practices{true};
        std::vector<std::string> bookmarked_concepts;
    } learning_;
    
    //=============================================================================
    // Caching and Performance
    //=============================================================================
    
    /** @brief UI state management */
    struct UIState {
        // Layout configuration
        f32 left_panel_width{400.0f};
        f32 right_panel_width{350.0f};
        f32 graph_height{200.0f};
        f32 preview_window_size{256.0f};
        
        // Search and filtering
        std::string global_search_filter;
        bool show_advanced_options{false};
        bool auto_scroll_logs{true};
        bool use_compact_layout{false};
        
        // Interaction state
        bool mouse_over_viewport{false};
        f32 mouse_viewport_x{0.0f};
        f32 mouse_viewport_y{0.0f};
        bool dragging_camera{false};
        
        // Panel visibility
        bool show_performance_overlay{false};
        bool show_resource_thumbnails{true};
        bool show_help_tooltips{true};
        bool show_debug_annotations{true};
        
        // Color scheme
        bool use_custom_colors{false};
        struct ColorScheme {
            u32 background{0xFF1E1E1E};
            u32 primary{0xFF007ACC};
            u32 secondary{0xFF4CAF50};
            u32 warning{0xFFFF9800};
            u32 error{0xFFF44336};
            u32 text{0xFFFFFFFF};
        } color_scheme;
        
        // Performance settings
        bool limit_ui_refresh_rate{true};
        f32 ui_refresh_rate{30.0f};
        f64 last_ui_update{0.0};
        bool cache_expensive_ui_elements{true};
    } ui_state_;
    
    /** @brief Cached rendering data for performance */
    struct CachedRenderingData {
        // Cached statistics
        renderer::RenderStatistics last_render_stats{};
        renderer::BatchRenderer::BatchingStatistics last_batch_stats{};
        f64 stats_cache_time{0.0};
        static constexpr f64 STATS_CACHE_DURATION = 1.0/30.0; // 30Hz updates
        
        // Cached resource lists
        std::vector<std::string> texture_names;
        std::vector<std::string> shader_names;
        std::vector<std::string> buffer_names;
        f64 resource_list_cache_time{0.0};
        static constexpr f64 RESOURCE_CACHE_DURATION = 2.0; // 2 second cache
        
        // Cached thumbnails and previews
        std::unordered_map<u32, u32> texture_thumbnails; // texture_id -> thumbnail_texture_id
        f64 thumbnail_cache_time{0.0};
        
        // Performance data
        bool data_needs_update{true};
        std::chrono::high_resolution_clock::time_point last_update;
    } cached_data_;

public:
    //=============================================================================
    // Constructor and Core Interface
    //=============================================================================
    
    RenderingDebugPanel();
    ~RenderingDebugPanel() override = default;
    
    // Panel interface implementation
    void render() override;
    void update(f64 delta_time) override;
    
    //=============================================================================
    // Configuration and State Management
    //=============================================================================
    
    /** @brief Set the 2D renderer to debug */
    void set_renderer(std::shared_ptr<renderer::Renderer2D> renderer) noexcept { 
        renderer_ = renderer; 
        invalidate_cache();
    }
    
    /** @brief Get current renderer */
    std::shared_ptr<renderer::Renderer2D> get_renderer() const noexcept { return renderer_; }
    
    /** @brief Set the batch renderer for advanced analysis */
    void set_batch_renderer(std::shared_ptr<renderer::BatchRenderer> batch_renderer) noexcept {
        batch_renderer_ = batch_renderer;
        invalidate_cache();
    }
    
    /** @brief Get current batch renderer */
    std::shared_ptr<renderer::BatchRenderer> get_batch_renderer() const noexcept { return batch_renderer_; }
    
    /** @brief Set active tab */
    void set_active_tab(ActiveTab tab) noexcept { active_tab_ = tab; }
    
    /** @brief Get active tab */
    ActiveTab active_tab() const noexcept { return active_tab_; }
    
    //=============================================================================
    // Visualization Control Interface
    //=============================================================================
    
    /** @brief Enable/disable wireframe rendering */
    void set_wireframe_enabled(bool enabled) noexcept { visualization_.show_wireframe = enabled; }
    bool is_wireframe_enabled() const noexcept { return visualization_.show_wireframe; }
    
    /** @brief Enable/disable batch color visualization */
    void set_batch_colors_enabled(bool enabled) noexcept { visualization_.show_batch_colors = enabled; }
    bool are_batch_colors_enabled() const noexcept { return visualization_.show_batch_colors; }
    
    /** @brief Enable/disable texture visualization */
    void set_texture_visualization_enabled(bool enabled) noexcept { visualization_.show_texture_visualization = enabled; }
    bool is_texture_visualization_enabled() const noexcept { return visualization_.show_texture_visualization; }
    
    /** @brief Enable/disable overdraw analysis */
    void set_overdraw_analysis_enabled(bool enabled) noexcept { visualization_.show_overdraw_analysis = enabled; }
    bool is_overdraw_analysis_enabled() const noexcept { return visualization_.show_overdraw_analysis; }
    
    /** @brief Set visualization opacity */
    void set_visualization_opacity(f32 opacity) noexcept { 
        visualization_.visualization_opacity = std::clamp(opacity, 0.0f, 1.0f); 
    }
    f32 get_visualization_opacity() const noexcept { return visualization_.visualization_opacity; }
    
    //=============================================================================
    // Performance Monitoring Interface
    //=============================================================================
    
    /** @brief Enable/disable GPU profiling */
    void set_gpu_profiling_enabled(bool enabled) noexcept { performance_.gpu_profiling_enabled = enabled; }
    bool is_gpu_profiling_enabled() const noexcept { return performance_.gpu_profiling_enabled; }
    
    /** @brief Get current performance grade */
    const char* get_performance_grade() const noexcept { return performance_.performance_grade; }
    
    /** @brief Get performance score (0-100) */
    f32 get_performance_score() const noexcept { return performance_.performance_score; }
    
    /** @brief Get primary performance bottleneck */
    const char* get_primary_bottleneck() const noexcept { return performance_.primary_bottleneck; }
    
    /** @brief Get optimization suggestions */
    const std::vector<std::string>& get_optimization_suggestions() const noexcept { 
        return performance_.optimization_suggestions; 
    }
    
    //=============================================================================
    // Resource Inspection Interface
    //=============================================================================
    
    /** @brief Select texture for inspection */
    void select_texture(renderer::resources::TextureID texture_id) noexcept;
    
    /** @brief Get selected texture ID */
    renderer::resources::TextureID get_selected_texture() const noexcept { return resources_.selected_texture; }
    
    /** @brief Select shader for inspection */
    void select_shader(renderer::resources::ShaderID shader_id) noexcept;
    
    /** @brief Get selected shader ID */
    renderer::resources::ShaderID get_selected_shader() const noexcept { return resources_.selected_shader; }
    
    /** @brief Enable/disable shader hot reload */
    void set_shader_hot_reload_enabled(bool enabled) noexcept { 
        shaders_.enable_shader_hot_reload = enabled; 
    }
    bool is_shader_hot_reload_enabled() const noexcept { return shaders_.enable_shader_hot_reload; }
    
    //=============================================================================
    // Learning Tools Interface
    //=============================================================================
    
    /** @brief Start a tutorial */
    void start_tutorial(LearningTools::Tutorial tutorial) noexcept;
    
    /** @brief Get active tutorial */
    LearningTools::Tutorial get_active_tutorial() const noexcept { return learning_.active_tutorial; }
    
    /** @brief Advance tutorial to next step */
    void advance_tutorial_step() noexcept;
    
    /** @brief Get current tutorial step */
    usize get_tutorial_step() const noexcept { return learning_.tutorial_step; }
    
    /** @brief Start rendering experiment */
    void start_experiment(const std::string& experiment_name) noexcept;
    
    /** @brief Stop current experiment */
    void stop_current_experiment() noexcept;

private:
    //=============================================================================
    // Rendering Implementation
    //=============================================================================
    
    // Tab rendering functions
    void render_visualization_tab();
    void render_performance_tab();
    void render_resources_tab();
    void render_shaders_tab();
    void render_batching_tab();
    void render_learning_tab();
    
    // Visualization rendering
    void render_debug_modes_section();
    void render_opengl_state_section();
    void render_render_step_controls();
    void render_viewport_overlay();
    void render_batch_visualization();
    void render_texture_atlas_viewer();
    void render_overdraw_heatmap();
    
    // Performance rendering
    void render_performance_graphs();
    void render_gpu_profiler();
    void render_memory_usage_analysis();
    void render_bottleneck_analysis();
    void render_optimization_suggestions();
    void render_frame_time_breakdown();
    void render_draw_call_analysis();
    void render_gpu_memory_viewer();
    
    // Resource rendering
    void render_texture_browser();
    void render_texture_inspector();
    void render_shader_browser();
    void render_shader_inspector();
    void render_buffer_browser();
    void render_buffer_inspector();
    void render_resource_memory_analysis();
    void render_resource_dependency_graph();
    
    // Shader rendering
    void render_shader_editor();
    void render_shader_parameter_controls();
    void render_shader_compilation_output();
    void render_shader_profiler();
    void render_uniform_inspector();
    void render_shader_effect_preview();
    void render_shader_hot_reload_controls();
    
    // Batching rendering
    void render_batch_statistics();
    void render_batch_efficiency_graph();
    void render_batching_strategy_controls();
    void render_texture_usage_analysis();
    void render_batch_inspector();
    void render_draw_call_optimizer();
    void render_batching_suggestions();
    
    // Learning rendering
    void render_tutorial_browser();
    void render_active_tutorial();
    void render_concept_explanations();
    void render_interactive_experiments();
    void render_rendering_pipeline_diagram();
    void render_reference_materials();
    void render_learning_progress();
    
    //=============================================================================
    // Data Management and Caching
    //=============================================================================
    
    void update_performance_metrics(f64 delta_time);
    void update_resource_cache();
    void update_batching_analysis();
    void invalidate_cache() noexcept;
    bool should_update_cache() const noexcept;
    
    void cache_render_statistics();
    void cache_resource_information();
    void cache_batching_statistics();
    void generate_texture_thumbnails();
    
    //=============================================================================
    // Educational Content Management
    //=============================================================================
    
    void initialize_learning_content();
    void setup_rendering_tutorials();
    void setup_concept_explanations();
    void setup_interactive_experiments();
    void load_reference_materials();
    
    const std::string& get_concept_explanation(const std::string& concept) const;
    void advance_current_tutorial();
    void setup_experiment(const std::string& experiment_name);
    void cleanup_experiment(const std::string& experiment_name);
    
    //=============================================================================
    // Utility Functions
    //=============================================================================
    
    // Data extraction and analysis
    bool is_renderer_available() const noexcept;
    bool is_batch_renderer_available() const noexcept;
    
    renderer::RenderStatistics get_current_render_stats() const;
    renderer::BatchRenderer::BatchingStatistics get_current_batch_stats() const;
    
    // Visualization helpers
    void draw_performance_graph(const char* label, const f32* values, usize count, 
                               f32 min_val, f32 max_val, const char* unit = "");
    void draw_memory_usage_pie_chart(const char* label, const std::vector<std::pair<std::string, usize>>& usage);
    void draw_texture_preview(renderer::resources::TextureID texture_id, f32 size);
    void draw_batch_color_legend();
    
    // String formatting utilities
    std::string format_bytes(usize bytes) const;
    std::string format_percentage(f32 value) const;
    std::string format_time(f32 milliseconds) const;
    std::string format_gpu_memory(usize bytes) const;
    std::string get_performance_grade_description(const char* grade) const;
    
    // UI helpers
    void help_tooltip(const char* description);
    bool colored_button(const char* label, const renderer::Color& color, f32 width = 0.0f);
    void render_loading_spinner(const char* label);
    void render_progress_bar_with_text(f32 fraction, const char* text);
    
    // Validation and safety
    bool validate_renderer_state() const;
    void ensure_safe_gl_state();
    
    //=============================================================================
    // Member Variables
    //=============================================================================
    
    std::shared_ptr<renderer::Renderer2D> renderer_; ///< 2D renderer being debugged
    std::shared_ptr<renderer::BatchRenderer> batch_renderer_; ///< Batch renderer for analysis
    
    // Performance tracking
    core::Timer frame_timer_;
    std::chrono::high_resolution_clock::time_point last_frame_time_;
    
    // Educational content databases
    std::unordered_map<std::string, std::function<void()>> tutorial_step_functions_;
    std::unordered_map<std::string, std::string> rendering_concepts_;
    std::unordered_map<std::string, std::string> shader_examples_;
    
    // UI interaction state
    bool showing_texture_selector_{false};
    bool showing_shader_selector_{false};
    f32 texture_preview_zoom_{1.0f};
    std::string current_search_query_;
    
    // Debug visualization temporaries (cleared each frame)
    struct VisualizationFrame {
        std::vector<std::pair<std::string, renderer::Color>> batch_debug_info;
        std::vector<std::pair<f32, f32>> overdraw_pixels;
        std::vector<std::string> gpu_state_changes;
        f32 total_gpu_cost_estimate{0.0f};
        u32 total_state_changes{0};
    } current_frame_viz_;
};

} // namespace ecscope::ui