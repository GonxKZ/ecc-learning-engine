#pragma once

/**
 * @file shader_debugging_tools.hpp
 * @brief Advanced Shader Debugging and Profiling Tools for ECScope
 * 
 * This system provides comprehensive debugging and profiling capabilities for shaders:
 * - Real-time shader debugging with variable inspection
 * - GPU performance profiling and analysis
 * - Visual debugging overlays and heat maps
 * - Automatic performance regression detection
 * - Educational debugging tutorials
 * - Cross-platform debugging support
 * - Memory usage analysis and optimization
 * - Compilation error analysis with suggestions
 * 
 * @author ECScope Educational ECS Framework - Advanced Shader System
 * @date 2024
 */

#include "shader_runtime_system.hpp"
#include "advanced_shader_library.hpp"
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <functional>
#include <chrono>
#include <array>
#include <optional>
#include <variant>

namespace ecscope::renderer::shader_debugging {

//=============================================================================
// Debug Data Types and Structures
//=============================================================================

enum class DebugDataType : u8 {
    Unknown = 0,
    Float,
    Vec2,
    Vec3,
    Vec4,
    Int,
    IVec2,
    IVec3,
    IVec4,
    Bool,
    Mat2,
    Mat3,
    Mat4,
    Texture2D,
    TextureCube
};

using DebugValue = std::variant<
    f32,                        ///< Float value
    std::array<f32, 2>,         ///< Vec2 value
    std::array<f32, 3>,         ///< Vec3 value
    std::array<f32, 4>,         ///< Vec4 value
    i32,                        ///< Int value
    std::array<i32, 2>,         ///< IVec2 value
    std::array<i32, 3>,         ///< IVec3 value
    std::array<i32, 4>,         ///< IVec4 value
    bool,                       ///< Bool value
    std::array<f32, 4>,         ///< Mat2 (stored as 4 floats)
    std::array<f32, 9>,         ///< Mat3 (stored as 9 floats)
    std::array<f32, 16>,        ///< Mat4 (stored as 16 floats)
    u32                         ///< Texture ID
>;

struct DebugVariable {
    std::string name;
    std::string display_name;
    DebugDataType type;
    DebugValue value;
    std::string source_location;    ///< Line/column in shader source
    bool is_watched{false};         ///< Whether this variable is being watched
    std::string description;        ///< Educational description
    
    // History tracking
    std::vector<DebugValue> value_history;
    std::vector<std::chrono::steady_clock::time_point> timestamps;
    
    void add_value(const DebugValue& val) {
        value = val;
        value_history.push_back(val);
        timestamps.push_back(std::chrono::steady_clock::now());
        
        // Keep only recent history
        if (value_history.size() > 100) {
            value_history.erase(value_history.begin());
            timestamps.erase(timestamps.begin());
        }
    }
};

//=============================================================================
// Performance Profiling System
//=============================================================================

struct GPUProfilerEvent {
    std::string name;
    std::chrono::steady_clock::time_point cpu_start;
    std::chrono::steady_clock::time_point cpu_end;
    f32 gpu_time_ms{0.0f};
    u32 query_id{0};
    bool is_complete{false};
    
    // Additional metrics
    u64 primitives_generated{0};
    u64 vertices_submitted{0};
    u64 fragments_generated{0};
    usize memory_allocated{0};
    usize memory_freed{0};
};

class ShaderPerformanceProfiler {
public:
    struct ProfilingConfig {
        bool enable_gpu_timing{true};
        bool enable_memory_tracking{true};
        bool enable_draw_call_analysis{true};
        bool enable_bandwidth_analysis{true};
        u32 history_frame_count{120}; // 2 seconds at 60fps
        f32 performance_warning_threshold{16.67f}; // 60fps threshold
        bool auto_generate_reports{false};
    };
    
    explicit ShaderPerformanceProfiler(const ProfilingConfig& config = ProfilingConfig{});
    ~ShaderPerformanceProfiler();
    
    // Profiling session management
    void begin_session(const std::string& session_name);
    void end_session();
    bool is_session_active() const { return session_active_; }
    
    // Event profiling
    void begin_event(const std::string& event_name);
    void end_event(const std::string& event_name);
    
    // Shader-specific profiling
    void profile_shader_execution(shader_runtime::ShaderRuntimeManager::ShaderHandle handle,
                                 const std::string& pass_name);
    
    void record_draw_call(shader_runtime::ShaderRuntimeManager::ShaderHandle handle,
                         u32 vertex_count, u32 instance_count = 1);
    
    void record_memory_usage(shader_runtime::ShaderRuntimeManager::ShaderHandle handle,
                           usize bytes_allocated, usize bytes_freed = 0);
    
    // Performance analysis
    struct PerformanceFrame {
        u32 frame_number;
        f32 total_frame_time{0.0f};
        f32 cpu_time{0.0f};
        f32 gpu_time{0.0f};
        u32 draw_calls{0};
        u32 shader_switches{0};
        usize memory_usage{0};
        
        std::vector<GPUProfilerEvent> events;
        std::chrono::steady_clock::time_point timestamp;
    };
    
    void begin_frame();
    void end_frame();
    
    const PerformanceFrame* get_current_frame() const;
    const PerformanceFrame* get_frame(u32 frame_number) const;
    std::vector<PerformanceFrame> get_recent_frames(u32 count = 60) const;
    
    // Performance statistics
    struct PerformanceStatistics {
        f32 average_frame_time{0.0f};
        f32 min_frame_time{FLT_MAX};
        f32 max_frame_time{0.0f};
        f32 frame_time_variance{0.0f};
        
        f32 average_cpu_time{0.0f};
        f32 average_gpu_time{0.0f};
        
        u32 average_draw_calls{0};
        u32 peak_draw_calls{0};
        
        usize average_memory_usage{0};
        usize peak_memory_usage{0};
        
        // Performance warnings
        u32 frame_drops{0};
        u32 gpu_stalls{0};
        u32 memory_warnings{0};
        
        std::vector<std::string> bottlenecks;
        std::vector<std::string> optimization_suggestions;
    };
    
    PerformanceStatistics calculate_statistics(u32 frame_count = 60) const;
    
    // Hot spot analysis
    struct HotSpot {
        std::string name;
        f32 total_time{0.0f};
        f32 average_time{0.0f};
        f32 percentage_of_frame{0.0f};
        u32 call_count{0};
        std::string category; // "shader", "draw_call", "memory", etc.
        
        bool operator<(const HotSpot& other) const {
            return total_time > other.total_time;
        }
    };
    
    std::vector<HotSpot> identify_hot_spots(u32 frame_count = 60) const;
    
    // Report generation
    std::string generate_performance_report(u32 frame_count = 60) const;
    std::string generate_bottleneck_analysis() const;
    std::string generate_optimization_recommendations() const;
    
    // Configuration
    void set_config(const ProfilingConfig& config) { config_ = config; }
    const ProfilingConfig& get_config() const { return config_; }
    
    // Integration with runtime manager
    void set_runtime_manager(shader_runtime::ShaderRuntimeManager* manager) {
        runtime_manager_ = manager;
    }
    
private:
    ProfilingConfig config_;
    shader_runtime::ShaderRuntimeManager* runtime_manager_{nullptr};
    
    bool session_active_{false};
    std::string session_name_;
    std::chrono::steady_clock::time_point session_start_;
    
    // Frame tracking
    std::vector<PerformanceFrame> frame_history_;
    u32 current_frame_number_{0};
    std::optional<PerformanceFrame> current_frame_;
    std::chrono::steady_clock::time_point frame_start_time_;
    
    // Event tracking
    std::unordered_map<std::string, std::chrono::steady_clock::time_point> active_events_;
    std::vector<GPUProfilerEvent> completed_events_;
    
    // GPU query management
    std::vector<u32> available_queries_;
    std::unordered_map<std::string, u32> active_queries_;
    u32 next_query_id_{1};
    
    // OpenGL/DirectX GPU timing
    void init_gpu_timing();
    void cleanup_gpu_timing();
    u32 create_gpu_timer_query();
    f32 get_gpu_timer_result(u32 query_id);
    bool is_gpu_timer_ready(u32 query_id);
    
    // Analysis helpers
    void detect_performance_issues();
    void update_statistics();
};

//=============================================================================
// Visual Debug Overlay System
//=============================================================================

class ShaderDebugOverlay {
public:
    enum class OverlayType {
        VariableWatch = 0,      ///< Watch shader variables
        PerformanceGraph,       ///< Performance graphs
        MemoryUsage,            ///< Memory usage visualization
        DrawCallAnalysis,       ///< Draw call breakdown
        TextureViewer,          ///< Texture visualization
        ShaderHeatMap,          ///< Performance heat map
        CompilationErrors,      ///< Compilation error display
        Educational             ///< Educational overlays
    };
    
    struct OverlayConfig {
        bool enable_variable_watch{true};
        bool enable_performance_graphs{true};
        bool enable_memory_visualization{true};
        bool show_fps_counter{true};
        bool show_frame_time_graph{true};
        bool show_gpu_memory_usage{true};
        f32 overlay_alpha{0.8f};
        std::array<f32, 2> overlay_position{10.0f, 10.0f};
        std::array<f32, 2> overlay_size{400.0f, 300.0f};
        
        // Colors
        std::array<f32, 4> background_color{0.0f, 0.0f, 0.0f, 0.7f};
        std::array<f32, 4> text_color{1.0f, 1.0f, 1.0f, 1.0f};
        std::array<f32, 4> graph_color{0.2f, 0.8f, 0.2f, 1.0f};
        std::array<f32, 4> warning_color{1.0f, 0.8f, 0.0f, 1.0f};
        std::array<f32, 4> error_color{1.0f, 0.2f, 0.2f, 1.0f};
    };
    
    explicit ShaderDebugOverlay(const OverlayConfig& config = OverlayConfig{});
    ~ShaderDebugOverlay() = default;
    
    // Overlay management
    void set_active_overlay(OverlayType type) { active_overlay_ = type; }
    OverlayType get_active_overlay() const { return active_overlay_; }
    
    void toggle_overlay(OverlayType type);
    bool is_overlay_enabled(OverlayType type) const;
    void set_overlay_enabled(OverlayType type, bool enabled);
    
    // Variable watching
    void add_watched_variable(const std::string& shader_name, const std::string& variable_name);
    void remove_watched_variable(const std::string& shader_name, const std::string& variable_name);
    void clear_watched_variables();
    
    std::vector<std::string> get_watched_variables() const;
    void update_variable_value(const std::string& shader_name, const std::string& variable_name,
                              const DebugValue& value);
    
    // Performance visualization
    void update_performance_data(const ShaderPerformanceProfiler::PerformanceFrame& frame);
    void set_performance_threshold(f32 threshold_ms) { performance_threshold_ = threshold_ms; }
    
    // Memory visualization
    void update_memory_data(usize total_memory, usize shader_memory, usize texture_memory);
    
    // Error display
    void add_compilation_error(const std::string& shader_name, const std::string& error_message,
                              u32 line_number = 0, u32 column = 0);
    void clear_compilation_errors();
    
    // Educational features
    void set_educational_mode(bool enabled) { educational_mode_ = enabled; }
    void add_educational_annotation(const std::string& annotation, 
                                  const std::array<f32, 2>& screen_position);
    void clear_educational_annotations();
    
    // Rendering (integrate with your UI system)
    void render_overlay(); // Called by your rendering system
    void handle_input(f32 mouse_x, f32 mouse_y, bool mouse_clicked);
    
    // Configuration
    void set_config(const OverlayConfig& config) { config_ = config; }
    const OverlayConfig& get_config() const { return config_; }
    
    // Integration
    void set_profiler(ShaderPerformanceProfiler* profiler) { profiler_ = profiler; }
    
private:
    OverlayConfig config_;
    OverlayType active_overlay_{OverlayType::VariableWatch};
    std::unordered_map<OverlayType, bool> enabled_overlays_;
    
    // Variable watching
    std::unordered_map<std::string, std::vector<DebugVariable>> watched_variables_;
    
    // Performance data
    ShaderPerformanceProfiler* profiler_{nullptr};
    std::vector<f32> frame_time_history_;
    std::vector<f32> gpu_time_history_;
    std::vector<u32> draw_call_history_;
    f32 performance_threshold_{16.67f}; // 60fps
    
    // Memory data
    usize total_memory_{0};
    usize shader_memory_{0};
    usize texture_memory_{0};
    std::vector<usize> memory_history_;
    
    // Error tracking
    struct CompilationError {
        std::string shader_name;
        std::string message;
        u32 line{0};
        u32 column{0};
        std::chrono::steady_clock::time_point timestamp;
    };
    std::vector<CompilationError> compilation_errors_;
    
    // Educational features
    bool educational_mode_{false};
    struct EducationalAnnotation {
        std::string text;
        std::array<f32, 2> position;
        std::chrono::steady_clock::time_point creation_time;
    };
    std::vector<EducationalAnnotation> educational_annotations_;
    
    // UI state
    bool show_details_{false};
    std::string selected_shader_;
    
    // Rendering helpers
    void render_variable_watch();
    void render_performance_graph();
    void render_memory_usage();
    void render_draw_call_analysis();
    void render_compilation_errors();
    void render_educational_overlays();
    
    void render_graph(const std::vector<f32>& data, const std::string& title,
                     const std::array<f32, 2>& position, const std::array<f32, 2>& size,
                     f32 min_val, f32 max_val, const std::array<f32, 4>& color);
};

//=============================================================================
// Shader Debugger Core System
//=============================================================================

class AdvancedShaderDebugger {
public:
    struct DebugConfig {
        bool enable_variable_inspection{true};
        bool enable_performance_profiling{true};
        bool enable_memory_debugging{true};
        bool enable_educational_mode{true};
        bool auto_detect_issues{true};
        
        // Debug breakpoint settings
        bool enable_conditional_breakpoints{false};
        bool enable_watchpoints{true};
        u32 max_watchpoints{16};
        
        // Performance settings
        f32 performance_warning_threshold{16.67f};
        f32 memory_warning_threshold_mb{100.0f};
        bool enable_regression_testing{false};
        
        // Educational settings
        bool show_explanatory_tooltips{true};
        bool highlight_performance_issues{true};
        std::string difficulty_level{"Intermediate"};
    };
    
    explicit AdvancedShaderDebugger(shader_runtime::ShaderRuntimeManager* runtime_manager,
                                   const DebugConfig& config = DebugConfig{});
    ~AdvancedShaderDebugger();
    
    // Debug session management
    void start_debug_session(const std::string& session_name);
    void end_debug_session();
    bool is_debug_session_active() const { return debug_session_active_; }
    
    // Shader debugging
    void attach_to_shader(shader_runtime::ShaderRuntimeManager::ShaderHandle handle);
    void detach_from_shader(shader_runtime::ShaderRuntimeManager::ShaderHandle handle);
    
    std::vector<shader_runtime::ShaderRuntimeManager::ShaderHandle> get_attached_shaders() const;
    
    // Variable inspection
    void add_variable_watch(shader_runtime::ShaderRuntimeManager::ShaderHandle handle,
                          const std::string& variable_name);
    void remove_variable_watch(shader_runtime::ShaderRuntimeManager::ShaderHandle handle,
                             const std::string& variable_name);
    
    std::vector<DebugVariable> get_watched_variables(shader_runtime::ShaderRuntimeManager::ShaderHandle handle) const;
    std::optional<DebugVariable> get_variable_info(shader_runtime::ShaderRuntimeManager::ShaderHandle handle,
                                                  const std::string& variable_name) const;
    
    // Performance analysis
    struct PerformanceIssue {
        enum class Severity {
            Info = 0,
            Warning,
            Critical
        };
        
        std::string description;
        Severity severity;
        std::string shader_name;
        std::string suggested_fix;
        f32 impact_score{0.0f}; // 0-100, higher = more impact
        std::string category; // "Performance", "Memory", "Quality"
        
        std::chrono::steady_clock::time_point detected_time;
        bool is_resolved{false};
    };
    
    std::vector<PerformanceIssue> detect_performance_issues() const;
    std::vector<PerformanceIssue> get_active_issues() const;
    void mark_issue_resolved(usize issue_index);
    
    // Compilation analysis
    struct CompilationAnalysis {
        bool compilation_successful{false};
        std::vector<std::string> errors;
        std::vector<std::string> warnings;
        std::vector<std::string> optimization_hints;
        
        f32 compilation_time{0.0f};
        u32 instruction_count{0};
        u32 register_usage{0};
        usize binary_size{0};
        
        // Cross-platform compatibility
        std::unordered_map<std::string, bool> platform_support;
        std::vector<std::string> compatibility_warnings;
    };
    
    CompilationAnalysis analyze_shader_compilation(shader_runtime::ShaderRuntimeManager::ShaderHandle handle) const;
    
    // Educational debugging features
    struct EducationalExplanation {
        std::string concept;
        std::string explanation;
        std::vector<std::string> key_points;
        std::string code_example;
        std::vector<std::string> related_concepts;
        std::string difficulty_level;
    };
    
    EducationalExplanation get_concept_explanation(const std::string& concept) const;
    std::vector<std::string> suggest_learning_materials(const PerformanceIssue& issue) const;
    
    // Debugging utilities
    std::string generate_debug_report() const;
    std::string generate_performance_summary() const;
    std::string generate_optimization_guide() const;
    
    // Regression testing
    struct PerformanceBaseline {
        std::string test_name;
        f32 expected_frame_time{16.67f};
        f32 expected_compile_time{100.0f};
        usize expected_memory_usage{1024 * 1024}; // 1MB
        f32 tolerance_percentage{10.0f};
        
        std::chrono::steady_clock::time_point creation_time;
        u32 test_count{0};
        u32 pass_count{0};
    };
    
    void create_performance_baseline(const std::string& test_name, 
                                   shader_runtime::ShaderRuntimeManager::ShaderHandle handle);
    bool run_regression_test(const std::string& test_name,
                           shader_runtime::ShaderRuntimeManager::ShaderHandle handle);
    
    std::vector<PerformanceBaseline> get_performance_baselines() const;
    
    // Integration with other systems
    ShaderPerformanceProfiler* get_profiler() { return profiler_.get(); }
    ShaderDebugOverlay* get_overlay() { return overlay_.get(); }
    
    void set_library(shader_library::AdvancedShaderLibrary* library) { library_ = library; }
    
    // Update and maintenance
    void update(); // Call once per frame
    
    // Configuration
    void set_config(const DebugConfig& config) { config_ = config; }
    const DebugConfig& get_config() const { return config_; }
    
private:
    DebugConfig config_;
    shader_runtime::ShaderRuntimeManager* runtime_manager_;
    shader_library::AdvancedShaderLibrary* library_{nullptr};
    
    // Core debugging components
    std::unique_ptr<ShaderPerformanceProfiler> profiler_;
    std::unique_ptr<ShaderDebugOverlay> overlay_;
    
    // Debug session state
    bool debug_session_active_{false};
    std::string current_session_;
    std::chrono::steady_clock::time_point session_start_;
    
    // Attached shaders
    std::unordered_set<shader_runtime::ShaderRuntimeManager::ShaderHandle> attached_shaders_;
    std::unordered_map<shader_runtime::ShaderRuntimeManager::ShaderHandle, 
                       std::vector<std::string>> watched_variables_;
    
    // Issue tracking
    std::vector<PerformanceIssue> active_issues_;
    std::vector<PerformanceIssue> resolved_issues_;
    
    // Performance baselines
    std::vector<PerformanceBaseline> performance_baselines_;
    
    // Educational content
    std::unordered_map<std::string, EducationalExplanation> educational_content_;
    
    // Helper methods
    void initialize_educational_content();
    void update_performance_monitoring();
    void check_for_issues();
    PerformanceIssue create_performance_issue(const std::string& description,
                                             PerformanceIssue::Severity severity,
                                             const std::string& shader_name,
                                             const std::string& suggested_fix) const;
    
    // Issue detection algorithms
    void detect_compilation_issues();
    void detect_runtime_performance_issues();
    void detect_memory_issues();
    void detect_quality_issues();
};

//=============================================================================
// Utility Functions and Helpers
//=============================================================================

namespace utils {
    // Debug value conversion utilities
    std::string debug_value_to_string(const DebugValue& value);
    DebugDataType opengl_type_to_debug_type(u32 gl_type);
    std::string debug_type_to_string(DebugDataType type);
    
    // Performance analysis utilities
    f32 calculate_performance_score(f32 frame_time_ms, u32 draw_calls, usize memory_usage);
    std::string categorize_performance_issue(f32 frame_time_ms, f32 gpu_time_ms, u32 draw_calls);
    std::vector<std::string> generate_optimization_suggestions(const ShaderPerformanceProfiler::PerformanceStatistics& stats);
    
    // Educational utilities
    std::string format_shader_explanation(const std::string& concept, const std::string& code_snippet);
    std::vector<std::string> extract_shader_concepts(const std::string& shader_source);
    std::string get_difficulty_rating(const std::string& shader_source);
    
    // Report generation utilities
    std::string format_performance_report(const ShaderPerformanceProfiler::PerformanceStatistics& stats);
    std::string format_debug_timestamp(const std::chrono::steady_clock::time_point& timestamp);
    std::string format_memory_usage(usize bytes);
    
    // Cross-platform debugging utilities
    bool is_debugging_supported();
    std::vector<std::string> get_supported_debug_extensions();
    std::string get_gpu_vendor_specific_advice(const std::string& vendor);
}

} // namespace ecscope::renderer::shader_debugging