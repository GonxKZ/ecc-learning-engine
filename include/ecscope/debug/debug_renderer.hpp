#pragma once

#include <vector>
#include <array>
#include <memory>
#include <string>
#include <unordered_map>

namespace ECScope::Debug {

// Forward declarations
struct Matrix4x4;
struct Vector3;
struct Vector2;
struct Color;

/**
 * @brief Debug renderer for visualizing debug information
 */
class DebugRenderer {
public:
    struct Vertex {
        float x, y, z;
        uint32_t color;
        float u, v;
        
        Vertex() = default;
        Vertex(float px, float py, float pz, uint32_t c)
            : x(px), y(py), z(pz), color(c), u(0.0f), v(0.0f) {}
    };
    
    struct Line {
        Vertex start, end;
        float thickness = 1.0f;
        bool depth_test = true;
    };
    
    struct Triangle {
        std::array<Vertex, 3> vertices;
        bool wireframe = false;
        bool depth_test = true;
    };
    
    struct Text {
        std::string content;
        float x, y, z;
        uint32_t color;
        float scale = 1.0f;
        bool screen_space = true;
    };
    
    struct Sphere {
        float x, y, z;
        float radius;
        uint32_t color;
        bool wireframe = false;
        int segments = 16;
    };
    
    struct Box {
        float min_x, min_y, min_z;
        float max_x, max_y, max_z;
        uint32_t color;
        bool wireframe = true;
    };
    
    struct Arrow {
        float start_x, start_y, start_z;
        float end_x, end_y, end_z;
        uint32_t color;
        float head_size = 1.0f;
    };

    DebugRenderer();
    ~DebugRenderer();

    // Lifecycle
    void Initialize();
    void BeginFrame();
    void EndFrame();
    void Render(const Matrix4x4& view_matrix, const Matrix4x4& projection_matrix);
    void Shutdown();

    // 2D Drawing (screen space)
    void DrawLine2D(float x1, float y1, float x2, float y2, uint32_t color, float thickness = 1.0f);
    void DrawRect2D(float x, float y, float width, float height, uint32_t color, bool filled = false);
    void DrawCircle2D(float x, float y, float radius, uint32_t color, bool filled = false, int segments = 32);
    void DrawText2D(float x, float y, const std::string& text, uint32_t color, float scale = 1.0f);
    
    // 3D Drawing (world space)
    void DrawLine3D(const Vector3& start, const Vector3& end, uint32_t color, float thickness = 1.0f);
    void DrawWireBox3D(const Vector3& min, const Vector3& max, uint32_t color);
    void DrawWireSphere3D(const Vector3& center, float radius, uint32_t color, int segments = 16);
    void DrawArrow3D(const Vector3& start, const Vector3& end, uint32_t color, float head_size = 1.0f);
    void DrawText3D(const Vector3& position, const std::string& text, uint32_t color, float scale = 1.0f);
    
    // Coordinate system
    void DrawAxes(const Vector3& origin, float scale = 1.0f);
    void DrawGrid(const Vector3& center, float spacing, int count, uint32_t color);
    
    // Advanced shapes
    void DrawCapsule3D(const Vector3& start, const Vector3& end, float radius, uint32_t color);
    void DrawCone3D(const Vector3& tip, const Vector3& base_center, float radius, uint32_t color);
    void DrawFrustum3D(const Matrix4x4& frustum_matrix, uint32_t color);
    
    // Batch operations
    void DrawLines3D(const std::vector<Vector3>& points, uint32_t color, bool closed = false);
    void DrawPoints3D(const std::vector<Vector3>& points, uint32_t color, float size = 1.0f);
    void DrawTriangles3D(const std::vector<Vector3>& vertices, const std::vector<uint32_t>& indices, uint32_t color);
    
    // State management
    void SetDepthTest(bool enable) { m_depth_test_enabled = enable; }
    void SetBlending(bool enable) { m_blending_enabled = enable; }
    void SetLineWidth(float width) { m_default_line_width = width; }
    void SetPointSize(float size) { m_default_point_size = size; }
    
    // Color utilities
    static uint32_t MakeColor(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255);
    static uint32_t MakeColor(float r, float g, float b, float a = 1.0f);
    
    // Common colors
    static constexpr uint32_t WHITE = 0xFFFFFFFF;
    static constexpr uint32_t BLACK = 0xFF000000;
    static constexpr uint32_t RED = 0xFFFF0000;
    static constexpr uint32_t GREEN = 0xFF00FF00;
    static constexpr uint32_t BLUE = 0xFF0000FF;
    static constexpr uint32_t YELLOW = 0xFFFFFF00;
    static constexpr uint32_t MAGENTA = 0xFFFF00FF;
    static constexpr uint32_t CYAN = 0xFF00FFFF;
    static constexpr uint32_t ORANGE = 0xFFFFA500;
    static constexpr uint32_t PURPLE = 0xFF800080;
    static constexpr uint32_t GRAY = 0xFF808080;

private:
    // Render data
    std::vector<Line> m_lines;
    std::vector<Triangle> m_triangles;
    std::vector<Text> m_texts;
    std::vector<Sphere> m_spheres;
    std::vector<Box> m_boxes;
    std::vector<Arrow> m_arrows;
    
    // State
    bool m_initialized = false;
    bool m_depth_test_enabled = true;
    bool m_blending_enabled = true;
    float m_default_line_width = 1.0f;
    float m_default_point_size = 1.0f;
    
    // Rendering resources
    struct RenderResources;
    std::unique_ptr<RenderResources> m_resources;
    
    // Internal methods
    void InitializeResources();
    void CleanupResources();
    
    void RenderLines(const Matrix4x4& mvp_matrix);
    void RenderTriangles(const Matrix4x4& mvp_matrix);
    void RenderTexts(const Matrix4x4& view_matrix, const Matrix4x4& projection_matrix);
    void RenderSpheres(const Matrix4x4& mvp_matrix);
    void RenderBoxes(const Matrix4x4& mvp_matrix);
    void RenderArrows(const Matrix4x4& mvp_matrix);
    
    void GenerateSphereGeometry(const Sphere& sphere, std::vector<Vertex>& vertices, std::vector<uint32_t>& indices);
    void GenerateBoxGeometry(const Box& box, std::vector<Vertex>& vertices, std::vector<uint32_t>& indices);
    void GenerateArrowGeometry(const Arrow& arrow, std::vector<Vertex>& vertices, std::vector<uint32_t>& indices);
};

/**
 * @brief Performance monitor for tracking system performance
 */
class PerformanceMonitor {
public:
    struct FrameStats {
        double frame_time_ms = 0.0;
        double cpu_time_ms = 0.0;
        double gpu_time_ms = 0.0;
        double wait_time_ms = 0.0;
        
        size_t draw_calls = 0;
        size_t vertices_rendered = 0;
        size_t triangles_rendered = 0;
        
        size_t memory_used_mb = 0;
        size_t vram_used_mb = 0;
        
        double fps = 0.0;
        double average_fps = 0.0;
        double min_fps = 0.0;
        double max_fps = 0.0;
    };
    
    struct SystemStats {
        std::string name;
        double update_time_ms = 0.0;
        double average_time_ms = 0.0;
        double min_time_ms = std::numeric_limits<double>::max();
        double max_time_ms = 0.0;
        size_t update_count = 0;
        double percentage = 0.0;
    };
    
    struct MemoryStats {
        size_t total_allocated = 0;
        size_t current_allocated = 0;
        size_t peak_allocated = 0;
        size_t allocation_count = 0;
        size_t deallocation_count = 0;
        double fragmentation = 0.0;
    };

    PerformanceMonitor();
    ~PerformanceMonitor();

    void Update(float deltaTime);
    void BeginFrame();
    void EndFrame();

    // Performance tracking
    void BeginSystemUpdate(const std::string& system_name);
    void EndSystemUpdate(const std::string& system_name);
    
    class ScopedSystemTimer {
    public:
        ScopedSystemTimer(PerformanceMonitor& monitor, const std::string& system_name)
            : m_monitor(monitor), m_system_name(system_name) {
            m_monitor.BeginSystemUpdate(m_system_name);
        }
        
        ~ScopedSystemTimer() {
            m_monitor.EndSystemUpdate(m_system_name);
        }
        
    private:
        PerformanceMonitor& m_monitor;
        std::string m_system_name;
    };

    // Data access
    const FrameStats& GetFrameStats() const { return m_current_frame; }
    const std::vector<FrameStats>& GetFrameHistory() const { return m_frame_history; }
    const std::unordered_map<std::string, SystemStats>& GetSystemStats() const { return m_system_stats; }
    const MemoryStats& GetMemoryStats() const { return m_memory_stats; }

    // Configuration
    void SetHistorySize(size_t size) { m_history_size = size; }
    size_t GetHistorySize() const { return m_history_size; }
    
    void SetUpdateFrequency(float frequency) { m_update_frequency = frequency; }
    float GetUpdateFrequency() const { return m_update_frequency; }

    // Statistics
    double GetAverageFrameTime() const;
    double GetAverageFPS() const;
    double GetSystemTimePercentage(const std::string& system_name) const;

private:
    FrameStats m_current_frame;
    std::vector<FrameStats> m_frame_history;
    std::unordered_map<std::string, SystemStats> m_system_stats;
    MemoryStats m_memory_stats;
    
    // Timing
    std::chrono::high_resolution_clock::time_point m_frame_start;
    std::chrono::high_resolution_clock::time_point m_last_update;
    std::unordered_map<std::string, std::chrono::high_resolution_clock::time_point> m_system_timers;
    
    // Configuration
    size_t m_history_size = 300;
    float m_update_frequency = 60.0f;
    float m_update_timer = 0.0f;
    
    void UpdateFrameStats();
    void UpdateSystemStats();
    void UpdateMemoryStats();
};

/**
 * @brief ImGui integration for debug UI rendering
 */
class ImGuiDebugRenderer {
public:
    ImGuiDebugRenderer();
    ~ImGuiDebugRenderer();

    void Initialize();
    void BeginFrame();
    void EndFrame();
    void Render();
    void Shutdown();

    // Utility widgets
    void PlotGraph(const std::string& label, const float* values, size_t count, 
                  float scale_min = FLT_MAX, float scale_max = FLT_MAX);
    void PlotHistogram(const std::string& label, const float* values, size_t count,
                      const std::string& overlay_text = "");
    
    void ProgressBar(float fraction, const std::string& overlay = "");
    void ColoredText(const std::string& text, uint32_t color);
    void Tooltip(const std::string& text);
    
    bool CollapsingHeader(const std::string& label, bool default_open = false);
    bool TreeNode(const std::string& label);
    void TreePop();
    
    void Columns(int count, const std::string& id = "", bool border = true);
    void NextColumn();
    
    void Separator();
    void SameLine();
    void NewLine();
    void Spacing();
    
    // Memory visualization
    void MemoryEditor(void* data, size_t size, size_t base_address = 0);
    
    // Performance widgets
    void FPSCounter();
    void PerformanceOverlay(const PerformanceMonitor& monitor);

private:
    bool m_initialized = false;
    
    void SetupStyle();
    void HandleInput();
};

// Utility macros
#define ECSCOPE_PROFILE_SYSTEM(monitor, system_name) \
    ECScope::Debug::PerformanceMonitor::ScopedSystemTimer _system_timer(monitor, system_name)

} // namespace ECScope::Debug