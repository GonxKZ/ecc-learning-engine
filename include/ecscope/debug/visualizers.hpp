#pragma once

#include "debug_system.hpp"
#include "profilers.hpp"
#include <vector>
#include <deque>
#include <array>
#include <memory>
#include <functional>

namespace ECScope::Debug {

// Forward declarations
struct RenderContext;
class GraphRenderer;

/**
 * @brief Real-time performance graph visualizer
 */
class PerformanceGraphVisualizer : public Visualizer {
public:
    struct GraphConfig {
        std::string title = "Performance";
        size_t max_samples = 300;
        float min_value = 0.0f;
        float max_value = 100.0f;
        bool auto_scale = true;
        bool show_average = true;
        bool show_min_max = true;
        
        struct {
            float r = 1.0f, g = 1.0f, b = 1.0f, a = 1.0f;
        } line_color;
        
        struct {
            float r = 0.2f, g = 0.2f, b = 0.2f, a = 0.8f;
        } background_color;
    };

    explicit PerformanceGraphVisualizer(const std::string& name, const GraphConfig& config = {});
    ~PerformanceGraphVisualizer() override;

    void Update(float deltaTime) override;
    void Render() override;

    // Data input
    void AddSample(float value);
    void AddSample(const std::string& series, float value);
    
    // Configuration
    void SetConfig(const GraphConfig& config) { m_config = config; }
    const GraphConfig& GetConfig() const { return m_config; }
    
    void SetTimeWindow(float window_seconds) { m_time_window = window_seconds; }
    float GetTimeWindow() const { return m_time_window; }

private:
    struct DataSeries {
        std::string name;
        std::deque<std::pair<float, float>> samples; // time, value
        GraphConfig config;
        
        float current_value = 0.0f;
        float average_value = 0.0f;
        float min_value = std::numeric_limits<float>::max();
        float max_value = std::numeric_limits<float>::lowest();
    };
    
    GraphConfig m_config;
    std::unordered_map<std::string, DataSeries> m_series;
    DataSeries m_default_series;
    
    float m_time_window = 10.0f;
    float m_current_time = 0.0f;
    
    void UpdateSeries(DataSeries& series);
    void RenderSeries(const DataSeries& series);
};

/**
 * @brief Memory usage visualization with heap maps
 */
class MemoryVisualizer : public Visualizer {
public:
    struct MemoryView {
        enum Type { HeapMap, AllocationTimeline, TagBreakdown, LeakDetection };
        Type type = HeapMap;
        bool enabled = true;
    };
    
    struct HeapMapConfig {
        size_t bytes_per_pixel = 1024;
        size_t width = 512;
        size_t height = 512;
        bool show_free_blocks = true;
        bool show_allocation_age = true;
    };

    explicit MemoryVisualizer(const std::string& name, std::shared_ptr<MemoryProfiler> profiler);
    ~MemoryVisualizer() override;

    void Update(float deltaTime) override;
    void Render() override;

    // Configuration
    void SetView(MemoryView::Type type, bool enabled);
    bool IsViewEnabled(MemoryView::Type type) const;
    
    void SetHeapMapConfig(const HeapMapConfig& config) { m_heap_config = config; }
    const HeapMapConfig& GetHeapMapConfig() const { return m_heap_config; }

private:
    std::shared_ptr<MemoryProfiler> m_profiler;
    std::array<MemoryView, 4> m_views;
    HeapMapConfig m_heap_config;
    
    // Render data
    std::vector<uint32_t> m_heap_map_pixels;
    std::vector<std::pair<float, size_t>> m_allocation_timeline;
    
    void UpdateHeapMap();
    void UpdateAllocationTimeline();
    void RenderHeapMap();
    void RenderAllocationTimeline();
    void RenderTagBreakdown();
    void RenderLeakDetection();
    
    uint32_t GetColorForAllocation(const MemoryProfiler::AllocationInfo& info);
    uint32_t GetColorForMemoryBlock(const MemoryProfiler::MemoryBlock& block);
};

/**
 * @brief ECS entity relationship visualizer
 */
class ECSVisualizer : public Visualizer {
public:
    struct Node {
        uint32_t entity_id;
        std::string name;
        float x, y;
        float radius = 10.0f;
        uint32_t color = 0xFFFFFFFF;
        bool selected = false;
    };
    
    struct Edge {
        uint32_t from_entity;
        uint32_t to_entity;
        std::string relationship_type;
        uint32_t color = 0xFF888888;
        float thickness = 1.0f;
    };
    
    struct ViewConfig {
        bool show_entity_names = true;
        bool show_component_types = true;
        bool show_system_dependencies = true;
        bool auto_layout = true;
        float node_spacing = 50.0f;
        float edge_length = 100.0f;
    };

    explicit ECSVisualizer(const std::string& name);
    ~ECSVisualizer() override;

    void Update(float deltaTime) override;
    void Render() override;

    // Data input
    void AddEntity(uint32_t entity_id, const std::string& name = "");
    void RemoveEntity(uint32_t entity_id);
    void AddRelationship(uint32_t from, uint32_t to, const std::string& type);
    void RemoveRelationship(uint32_t from, uint32_t to);
    
    // Interaction
    void SelectEntity(uint32_t entity_id);
    void ClearSelection();
    uint32_t GetSelectedEntity() const { return m_selected_entity; }
    
    // Configuration
    void SetViewConfig(const ViewConfig& config) { m_config = config; }
    const ViewConfig& GetViewConfig() const { return m_config; }

private:
    std::vector<Node> m_nodes;
    std::vector<Edge> m_edges;
    ViewConfig m_config;
    
    uint32_t m_selected_entity = 0;
    
    // Layout system
    void UpdateLayout();
    void ApplyForceDirectedLayout();
    void ApplyHierarchicalLayout();
    
    // Rendering
    void RenderNodes();
    void RenderEdges();
    void RenderTooltips();
    
    // Utilities
    Node* FindNode(uint32_t entity_id);
    bool IsEntityVisible(uint32_t entity_id) const;
};

/**
 * @brief Physics debug rendering (collision shapes, forces)
 */
class PhysicsDebugVisualizer : public Visualizer {
public:
    struct DebugDrawConfig {
        bool show_collision_shapes = true;
        bool show_aabb = false;
        bool show_velocity_vectors = true;
        bool show_force_vectors = true;
        bool show_contact_points = true;
        bool show_joint_constraints = true;
        
        float vector_scale = 1.0f;
        float contact_point_size = 3.0f;
        
        struct Colors {
            uint32_t static_body = 0xFF00FF00;
            uint32_t dynamic_body = 0xFFFF0000;
            uint32_t kinematic_body = 0xFF0000FF;
            uint32_t velocity = 0xFFFFFF00;
            uint32_t force = 0xFFFF00FF;
            uint32_t contact = 0xFFFFFFFF;
            uint32_t aabb = 0xFF888888;
        } colors;
    };

    explicit PhysicsDebugVisualizer(const std::string& name);
    ~PhysicsDebugVisualizer() override;

    void Update(float deltaTime) override;
    void Render() override;

    // Configuration
    void SetConfig(const DebugDrawConfig& config) { m_config = config; }
    const DebugDrawConfig& GetConfig() const { return m_config; }

    // Physics data input (would integrate with physics system)
    void AddRigidBody(uint32_t id, const void* shape_data, const float* transform);
    void RemoveRigidBody(uint32_t id);
    void AddContact(const float* point, const float* normal, float depth);
    void AddForceVector(const float* origin, const float* force);
    void ClearTemporaryData();

private:
    DebugDrawConfig m_config;
    
    struct RigidBodyData {
        uint32_t id;
        std::vector<float> vertices;
        std::array<float, 16> transform;
        uint32_t color;
        bool is_static;
    };
    
    struct ContactData {
        std::array<float, 3> point;
        std::array<float, 3> normal;
        float depth;
    };
    
    struct ForceData {
        std::array<float, 3> origin;
        std::array<float, 3> force;
    };
    
    std::vector<RigidBodyData> m_rigid_bodies;
    std::vector<ContactData> m_contacts;
    std::vector<ForceData> m_forces;
    
    void RenderRigidBodies();
    void RenderContacts();
    void RenderForces();
    void RenderAABB(const RigidBodyData& body);
};

/**
 * @brief Rendering debug views (wireframe, normals, overdraw)
 */
class RenderingDebugVisualizer : public Visualizer {
public:
    enum class DebugMode {
        Normal,
        Wireframe,
        Normals,
        Overdraw,
        TextureCoordinates,
        MipMapLevels,
        LightComplexity,
        ShaderComplexity
    };
    
    struct RenderStats {
        size_t draw_calls = 0;
        size_t vertices_rendered = 0;
        size_t triangles_rendered = 0;
        size_t texture_switches = 0;
        size_t shader_switches = 0;
        size_t render_targets_switches = 0;
        
        double vertex_shader_time_ms = 0.0;
        double fragment_shader_time_ms = 0.0;
        double gpu_frame_time_ms = 0.0;
        
        size_t vram_usage_mb = 0;
        double gpu_utilization = 0.0;
    };

    explicit RenderingDebugVisualizer(const std::string& name);
    ~RenderingDebugVisualizer() override;

    void Update(float deltaTime) override;
    void Render() override;

    // Debug mode control
    void SetDebugMode(DebugMode mode) { m_debug_mode = mode; }
    DebugMode GetDebugMode() const { return m_debug_mode; }

    // Statistics input
    void UpdateRenderStats(const RenderStats& stats) { m_render_stats = stats; }
    const RenderStats& GetRenderStats() const { return m_render_stats; }

private:
    DebugMode m_debug_mode = DebugMode::Normal;
    RenderStats m_render_stats;
    
    void RenderStatsOverlay();
    void RenderDebugModeControls();
};

/**
 * @brief Network topology and message flow visualization
 */
class NetworkVisualizer : public Visualizer {
public:
    struct NetworkNode {
        std::string id;
        std::string address;
        float x, y;
        uint32_t color = 0xFF00AAFF;
        bool is_local = false;
        size_t connections = 0;
        double latency_ms = 0.0;
    };
    
    struct NetworkConnection {
        std::string from_id;
        std::string to_id;
        double bandwidth_bps = 0.0;
        double latency_ms = 0.0;
        size_t packets_sent = 0;
        size_t packets_received = 0;
        bool is_active = true;
    };
    
    struct MessageFlow {
        std::string from_id;
        std::string to_id;
        std::string message_type;
        size_t size_bytes;
        float progress = 0.0f; // 0.0 to 1.0
        uint32_t color = 0xFFFFFF00;
    };

    explicit NetworkVisualizer(const std::string& name, std::shared_ptr<NetworkProfiler> profiler);
    ~NetworkVisualizer() override;

    void Update(float deltaTime) override;
    void Render() override;

    // Network topology management
    void AddNode(const std::string& id, const std::string& address, bool is_local = false);
    void RemoveNode(const std::string& id);
    void AddConnection(const std::string& from_id, const std::string& to_id);
    void RemoveConnection(const std::string& from_id, const std::string& to_id);
    
    // Message flow visualization
    void AddMessageFlow(const std::string& from_id, const std::string& to_id, 
                       const std::string& type, size_t size);

private:
    std::shared_ptr<NetworkProfiler> m_profiler;
    
    std::vector<NetworkNode> m_nodes;
    std::vector<NetworkConnection> m_connections;
    std::vector<MessageFlow> m_message_flows;
    
    void UpdateTopology();
    void UpdateMessageFlows(float deltaTime);
    void RenderNodes();
    void RenderConnections();
    void RenderMessageFlows();
    void RenderNetworkStats();
    
    NetworkNode* FindNode(const std::string& id);
    NetworkConnection* FindConnection(const std::string& from_id, const std::string& to_id);
};

/**
 * @brief Multi-purpose chart renderer for custom data visualization
 */
class ChartVisualizer : public Visualizer {
public:
    enum class ChartType {
        Line,
        Bar,
        Pie,
        Scatter,
        Histogram,
        Heatmap
    };
    
    struct DataPoint {
        float x, y;
        std::string label;
        uint32_t color = 0xFFFFFFFF;
    };
    
    struct ChartConfig {
        ChartType type = ChartType::Line;
        std::string title;
        std::string x_label;
        std::string y_label;
        
        float min_x = 0.0f, max_x = 100.0f;
        float min_y = 0.0f, max_y = 100.0f;
        bool auto_scale_x = true;
        bool auto_scale_y = true;
        
        bool show_grid = true;
        bool show_legend = true;
        bool show_values = false;
    };

    explicit ChartVisualizer(const std::string& name, const ChartConfig& config = {});
    ~ChartVisualizer() override;

    void Update(float deltaTime) override;
    void Render() override;

    // Data management
    void AddDataPoint(float x, float y, const std::string& label = "");
    void AddDataSeries(const std::string& series_name, const std::vector<DataPoint>& points);
    void ClearData();
    void ClearSeries(const std::string& series_name);
    
    // Configuration
    void SetConfig(const ChartConfig& config) { m_config = config; }
    const ChartConfig& GetConfig() const { return m_config; }

private:
    ChartConfig m_config;
    std::unordered_map<std::string, std::vector<DataPoint>> m_data_series;
    std::vector<DataPoint> m_default_series;
    
    void RenderLineChart();
    void RenderBarChart();
    void RenderPieChart();
    void RenderScatterChart();
    void RenderHistogram();
    void RenderHeatmap();
    
    void RenderGrid();
    void RenderAxes();
    void RenderLegend();
    void CalculateAutoScale();
};

} // namespace ECScope::Debug