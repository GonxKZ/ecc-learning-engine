/**
 * @file ecs_inspector_widgets.cpp
 * @brief Implementation of advanced ECS inspector widgets
 */

#include "../../include/ecscope/gui/ecs_inspector_widgets.hpp"
#include "../../include/ecscope/core/log.hpp"

#include <algorithm>
#include <cmath>
#include <numeric>
#include <sstream>
#include <iomanip>

#ifdef ECSCOPE_HAS_IMGUI
#include <imgui.h>
#include <imgui_internal.h>
#endif

namespace ecscope::gui {

namespace {
    // Color constants for consistent theming
    constexpr ImVec4 COLOR_NODE_DEFAULT = ImVec4(0.2f, 0.6f, 1.0f, 1.0f);
    constexpr ImVec4 COLOR_NODE_SELECTED = ImVec4(1.0f, 0.8f, 0.2f, 1.0f);
    constexpr ImVec4 COLOR_NODE_HOVERED = ImVec4(0.4f, 0.8f, 1.0f, 1.0f);
    constexpr ImVec4 COLOR_EDGE_DEFAULT = ImVec4(0.7f, 0.7f, 0.7f, 1.0f);
    constexpr ImVec4 COLOR_EDGE_HIGHLIGHTED = ImVec4(1.0f, 1.0f, 0.0f, 1.0f);
    
    // Utility functions
    ImU32 vec4_to_imu32(const Vec4& color) {
        return IM_COL32(
            static_cast<int>(color.x * 255),
            static_cast<int>(color.y * 255),
            static_cast<int>(color.z * 255),
            static_cast<int>(color.w * 255)
        );
    }
    
    Vec4 imu32_to_vec4(ImU32 color) {
        return Vec4(
            static_cast<float>((color >> 24) & 0xFF) / 255.0f,
            static_cast<float>((color >> 16) & 0xFF) / 255.0f,
            static_cast<float>((color >> 8) & 0xFF) / 255.0f,
            static_cast<float>(color & 0xFF) / 255.0f
        );
    }
    
    std::string format_bytes(size_t bytes) {
        const char* units[] = {"B", "KB", "MB", "GB"};
        size_t unit = 0;
        double size = static_cast<double>(bytes);
        
        while (size >= 1024.0 && unit < 3) {
            size /= 1024.0;
            unit++;
        }
        
        return std::format("{:.2f} {}", size, units[unit]);
    }
    
    std::string format_duration_ms(float milliseconds) {
        if (milliseconds < 1.0f) {
            return std::format("{:.1f}μs", milliseconds * 1000.0f);
        } else if (milliseconds < 1000.0f) {
            return std::format("{:.2f}ms", milliseconds);
        } else {
            return std::format("{:.2f}s", milliseconds / 1000.0f);
        }
    }
}

// =============================================================================
// BASIC PROPERTY EDITORS IMPLEMENTATION
// =============================================================================

template<>
std::string BasicPropertyEditor<int>::get_type_name() const {
    return "int";
}

template<>
std::string BasicPropertyEditor<float>::get_type_name() const {
    return "float";
}

template<>
std::string BasicPropertyEditor<bool>::get_type_name() const {
    return "bool";
}

template<>
std::string BasicPropertyEditor<std::string>::get_type_name() const {
    return "string";
}

template<>
bool BasicPropertyEditor<int>::render_property(const std::string& property_name, void* data, bool& changed) {
#ifdef ECSCOPE_HAS_IMGUI
    int* value = static_cast<int*>(data);
    if (min_value_ != max_value_) {
        changed = ImGui::SliderInt(property_name.c_str(), value, min_value_, max_value_);
    } else {
        changed = ImGui::InputInt(property_name.c_str(), value);
    }
    return true;
#else
    return false;
#endif
}

template<>
bool BasicPropertyEditor<float>::render_property(const std::string& property_name, void* data, bool& changed) {
#ifdef ECSCOPE_HAS_IMGUI
    float* value = static_cast<float*>(data);
    if (min_value_ != max_value_) {
        changed = ImGui::SliderFloat(property_name.c_str(), value, min_value_, max_value_);
    } else {
        changed = ImGui::InputFloat(property_name.c_str(), value);
    }
    return true;
#else
    return false;
#endif
}

template<>
bool BasicPropertyEditor<bool>::render_property(const std::string& property_name, void* data, bool& changed) {
#ifdef ECSCOPE_HAS_IMGUI
    bool* value = static_cast<bool*>(data);
    changed = ImGui::Checkbox(property_name.c_str(), value);
    return true;
#else
    return false;
#endif
}

template<>
bool BasicPropertyEditor<std::string>::render_property(const std::string& property_name, void* data, bool& changed) {
#ifdef ECSCOPE_HAS_IMGUI
    std::string* value = static_cast<std::string*>(data);
    
    // Create a buffer for ImGui text input
    constexpr size_t BUFFER_SIZE = 256;
    static char buffer[BUFFER_SIZE];
    
    strncpy(buffer, value->c_str(), BUFFER_SIZE - 1);
    buffer[BUFFER_SIZE - 1] = '\0';
    
    changed = ImGui::InputText(property_name.c_str(), buffer, BUFFER_SIZE);
    
    if (changed) {
        *value = std::string(buffer);
    }
    
    return true;
#else
    return false;
#endif
}

// =============================================================================
// COMPONENT EDITOR IMPLEMENTATION
// =============================================================================

ComponentEditor::ComponentEditor(const std::string& component_name, size_t component_size)
    : component_name_(component_name), component_size_(component_size) {
}

template<typename T>
void ComponentEditor::register_property(const std::string& name, size_t offset, 
                                       const std::string& display_name,
                                       const std::string& description) {
    PropertyInfo info;
    info.name = name;
    info.display_name = display_name.empty() ? name : display_name;
    info.description = description;
    info.offset = offset;
    info.editor = std::make_unique<BasicPropertyEditor<T>>();
    
    properties_.push_back(std::move(info));
}

bool ComponentEditor::render_component_editor(void* component_data, bool show_advanced) {
    if (!component_data) {
        return false;
    }
    
#ifdef ECSCOPE_HAS_IMGUI
    bool any_changed = false;
    
    // Component header
    ImGui::Text("Component: %s", component_name_.c_str());
    ImGui::Text("Size: %zu bytes", component_size_);
    
    ImGui::Separator();
    
    // Show descriptions toggle
    if (ImGui::Checkbox("Show Descriptions", &show_descriptions_)) {
        // Toggle descriptions
    }
    
    // Properties
    for (auto& property : properties_) {
        if (property.advanced && !show_advanced) {
            continue;
        }
        
        void* property_data = static_cast<char*>(component_data) + property.offset;
        bool property_changed = false;
        
        ImGui::PushID(property.name.c_str());
        
        if (property.readonly) {
            ImGui::BeginDisabled();
        }
        
        bool rendered = property.editor->render_property(property.display_name, property_data, property_changed);
        
        if (property.readonly) {
            ImGui::EndDisabled();
        }
        
        // Show description as tooltip
        if (show_descriptions_ && !property.description.empty() && ImGui::IsItemHovered()) {
            ImGui::SetTooltip("%s", property.description.c_str());
        }
        
        if (property_changed) {
            any_changed = true;
        }
        
        ImGui::PopID();
    }
    
    return any_changed;
#else
    return false;
#endif
}

bool ComponentEditor::validate_component(const void* component_data) const {
    if (!component_data) {
        return false;
    }
    
    for (const auto& property : properties_) {
        const void* property_data = static_cast<const char*>(component_data) + property.offset;
        if (!property.editor->validate_property(property_data)) {
            return false;
        }
    }
    
    return true;
}

// =============================================================================
// SYSTEM DEPENDENCY GRAPH IMPLEMENTATION
// =============================================================================

SystemDependencyGraph::SystemDependencyGraph() {
    // Initialize with default settings
}

void SystemDependencyGraph::update_graph(const SystemGraph& system_graph) {
    nodes_.clear();
    edges_.clear();
    
    // Create nodes for each system
    for (const auto& [system_id, stats] : system_graph.systems) {
        GraphNode node;
        node.system_id = system_id;
        node.display_name = stats.name;
        node.category = stats.category;
        
        // Color based on category
        node.color = get_category_color(stats.category);
        
        // Position will be set by layout algorithm
        node.position.pos = Vec2(0, 0);
        node.position.size = Vec2(120, 60);
        
        nodes_[system_id] = node;
    }
    
    // Create edges for dependencies
    for (const auto& [system_id, stats] : system_graph.systems) {
        for (const SystemID& dependency : stats.dependencies) {
            if (nodes_.find(dependency) != nodes_.end()) {
                GraphEdge edge;
                edge.from = dependency;
                edge.to = system_id;
                edges_.push_back(edge);
            }
        }
    }
    
    // Apply automatic layout if not manual
    if (current_layout_ != LayoutAlgorithm::Manual) {
        apply_automatic_layout();
    }
}

void SystemDependencyGraph::render_dependency_graph(const Rect& canvas_rect) {
#ifdef ECSCOPE_HAS_IMGUI
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    
    // Handle canvas interaction
    Vec2 canvas_origin = Vec2(canvas_rect.min.x + canvas_scroll_.x, canvas_rect.min.y + canvas_scroll_.y);
    
    // Draw background
    draw_list->AddRectFilled(
        ImVec2(canvas_rect.min.x, canvas_rect.min.y),
        ImVec2(canvas_rect.max.x, canvas_rect.max.y),
        IM_COL32(30, 30, 30, 255)
    );
    
    // Draw grid
    const float GRID_SIZE = 50.0f * zoom_level_;
    for (float x = fmod(canvas_scroll_.x, GRID_SIZE); x < canvas_rect.width(); x += GRID_SIZE) {
        draw_list->AddLine(
            ImVec2(canvas_rect.min.x + x, canvas_rect.min.y),
            ImVec2(canvas_rect.min.x + x, canvas_rect.max.y),
            IM_COL32(60, 60, 60, 100)
        );
    }
    for (float y = fmod(canvas_scroll_.y, GRID_SIZE); y < canvas_rect.height(); y += GRID_SIZE) {
        draw_list->AddLine(
            ImVec2(canvas_rect.min.x, canvas_rect.min.y + y),
            ImVec2(canvas_rect.max.x, canvas_rect.min.y + y),
            IM_COL32(60, 60, 60, 100)
        );
    }
    
    // Render edges first (so they appear behind nodes)
    for (const auto& edge : edges_) {
        auto from_it = nodes_.find(edge.from);
        auto to_it = nodes_.find(edge.to);
        
        if (from_it != nodes_.end() && to_it != nodes_.end()) {
            Vec2 from_center = get_node_center(from_it->second);
            Vec2 to_center = get_node_center(to_it->second);
            
            from_center.x += canvas_origin.x;
            from_center.y += canvas_origin.y;
            to_center.x += canvas_origin.x;
            to_center.y += canvas_origin.y;
            
            ImU32 edge_color = edge.highlighted ? vec4_to_imu32(Vec4(1, 1, 0, 1)) : vec4_to_imu32(edge.color);
            
            draw_list->AddLine(
                ImVec2(from_center.x, from_center.y),
                ImVec2(to_center.x, to_center.y),
                edge_color,
                edge.thickness * zoom_level_
            );
            
            // Draw arrow
            Vec2 direction = Vec2(to_center.x - from_center.x, to_center.y - from_center.y).normalized();
            Vec2 arrow_pos = Vec2(to_center.x - direction.x * 30, to_center.y - direction.y * 30);
            Vec2 arrow_left = Vec2(arrow_pos.x - direction.y * 10, arrow_pos.y + direction.x * 10);
            Vec2 arrow_right = Vec2(arrow_pos.x + direction.y * 10, arrow_pos.y - direction.x * 10);
            
            draw_list->AddTriangleFilled(
                ImVec2(to_center.x, to_center.y),
                ImVec2(arrow_left.x, arrow_left.y),
                ImVec2(arrow_right.x, arrow_right.y),
                edge_color
            );
        }
    }
    
    // Render nodes
    for (auto& [system_id, node] : nodes_) {
        Vec2 node_pos = Vec2(
            canvas_origin.x + node.position.pos.x * zoom_level_,
            canvas_origin.y + node.position.pos.y * zoom_level_
        );
        Vec2 node_size = Vec2(
            node.position.size.x * zoom_level_,
            node.position.size.y * zoom_level_
        );
        
        // Check if node is in viewport
        Rect node_rect(node_pos, node_pos + node_size);
        if (!node_rect.overlaps(canvas_rect)) {
            continue;
        }
        
        // Determine node color
        ImU32 node_color;
        if (node.selected) {
            node_color = vec4_to_imu32(COLOR_NODE_SELECTED);
        } else if (node.highlighted) {
            node_color = vec4_to_imu32(COLOR_NODE_HOVERED);
        } else {
            node_color = vec4_to_imu32(node.color);
        }
        
        // Draw node background
        draw_list->AddRectFilled(
            ImVec2(node_pos.x, node_pos.y),
            ImVec2(node_pos.x + node_size.x, node_pos.y + node_size.y),
            node_color,
            5.0f * zoom_level_
        );
        
        // Draw node border
        draw_list->AddRect(
            ImVec2(node_pos.x, node_pos.y),
            ImVec2(node_pos.x + node_size.x, node_pos.y + node_size.y),
            IM_COL32(255, 255, 255, 200),
            5.0f * zoom_level_,
            0,
            2.0f * zoom_level_
        );
        
        // Draw node text
        ImVec2 text_size = ImGui::CalcTextSize(node.display_name.c_str());
        Vec2 text_pos = Vec2(
            node_pos.x + (node_size.x - text_size.x) * 0.5f,
            node_pos.y + (node_size.y - text_size.y) * 0.5f
        );
        
        draw_list->AddText(
            ImVec2(text_pos.x, text_pos.y),
            IM_COL32(255, 255, 255, 255),
            node.display_name.c_str()
        );
        
        // Handle node interaction
        if (ImGui::IsMouseHoveringRect(ImVec2(node_pos.x, node_pos.y), 
                                      ImVec2(node_pos.x + node_size.x, node_pos.y + node_size.y))) {
            hovered_system_ = system_id;
            
            if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
                selected_system_ = system_id;
                node.selected = true;
                
                // Deselect other nodes
                for (auto& [other_id, other_node] : nodes_) {
                    if (other_id != system_id) {
                        other_node.selected = false;
                    }
                }
            }
            
            // Drag handling
            if (current_layout_ == LayoutAlgorithm::Manual) {
                if (ImGui::IsMouseDragging(ImGuiMouseButton_Left) && node.selected) {
                    ImVec2 delta = ImGui::GetIO().MouseDelta;
                    node.position.pos.x += delta.x / zoom_level_;
                    node.position.pos.y += delta.y / zoom_level_;
                }
            }
        } else if (hovered_system_ == system_id) {
            hovered_system_.reset();
        }
    }
    
    // Handle canvas panning
    if (ImGui::IsWindowHovered() && !hovered_system_.has_value()) {
        if (ImGui::IsMouseDragging(ImGuiMouseButton_Right)) {
            ImVec2 delta = ImGui::GetIO().MouseDelta;
            canvas_scroll_.x += delta.x;
            canvas_scroll_.y += delta.y;
        }
        
        // Handle zooming
        float wheel = ImGui::GetIO().MouseWheel;
        if (wheel != 0.0f) {
            float zoom_delta = wheel > 0 ? 1.1f : 1.0f / 1.1f;
            zoom_level_ = std::clamp(zoom_level_ * zoom_delta, 0.1f, 5.0f);
        }
    }
#endif
}

std::optional<SystemID> SystemDependencyGraph::get_selected_system() const {
    return selected_system_;
}

void SystemDependencyGraph::set_layout_algorithm(LayoutAlgorithm algorithm) {
    current_layout_ = algorithm;
    if (algorithm != LayoutAlgorithm::Manual) {
        apply_automatic_layout();
    }
}

void SystemDependencyGraph::apply_automatic_layout() {
    switch (current_layout_) {
        case LayoutAlgorithm::Hierarchical:
            apply_hierarchical_layout();
            break;
        case LayoutAlgorithm::Circular:
            apply_circular_layout();
            break;
        case LayoutAlgorithm::ForceDirected:
            apply_force_directed_layout();
            break;
        case LayoutAlgorithm::Manual:
        default:
            break;
    }
}

void SystemDependencyGraph::apply_hierarchical_layout() {
    if (nodes_.empty()) return;
    
    // Topological sort to determine hierarchy levels
    std::vector<SystemID> topo_order = get_topological_order();
    std::unordered_map<SystemID, int> levels;
    
    // Assign levels based on dependencies
    for (const SystemID& system_id : topo_order) {
        int max_dependency_level = -1;
        
        auto node_it = nodes_.find(system_id);
        if (node_it == nodes_.end()) continue;
        
        // Find dependencies and their levels
        for (const auto& edge : edges_) {
            if (edge.to == system_id) {
                auto dep_level_it = levels.find(edge.from);
                if (dep_level_it != levels.end()) {
                    max_dependency_level = std::max(max_dependency_level, dep_level_it->second);
                }
            }
        }
        
        levels[system_id] = max_dependency_level + 1;
    }
    
    // Position nodes based on levels
    std::unordered_map<int, std::vector<SystemID>> level_systems;
    for (const auto& [system_id, level] : levels) {
        level_systems[level].push_back(system_id);
    }
    
    const float LEVEL_HEIGHT = 150.0f;
    const float SYSTEM_WIDTH = 200.0f;
    
    for (const auto& [level, systems] : level_systems) {
        float y_pos = level * LEVEL_HEIGHT;
        float total_width = systems.size() * SYSTEM_WIDTH;
        float start_x = -total_width * 0.5f;
        
        for (size_t i = 0; i < systems.size(); ++i) {
            auto& node = nodes_[systems[i]];
            node.position.pos.x = start_x + i * SYSTEM_WIDTH;
            node.position.pos.y = y_pos;
        }
    }
}

void SystemDependencyGraph::apply_circular_layout() {
    if (nodes_.empty()) return;
    
    const float RADIUS = 200.0f;
    const float CENTER_X = 0.0f;
    const float CENTER_Y = 0.0f;
    
    float angle_step = 2.0f * M_PI / nodes_.size();
    float current_angle = 0.0f;
    
    for (auto& [system_id, node] : nodes_) {
        node.position.pos.x = CENTER_X + RADIUS * cos(current_angle);
        node.position.pos.y = CENTER_Y + RADIUS * sin(current_angle);
        current_angle += angle_step;
    }
}

void SystemDependencyGraph::apply_force_directed_layout() {
    if (nodes_.empty()) return;
    
    // Simple spring-mass system for force-directed layout
    const int ITERATIONS = 100;
    const float SPRING_STRENGTH = 0.1f;
    const float REPULSION_STRENGTH = 1000.0f;
    const float DAMPING = 0.9f;
    
    std::unordered_map<SystemID, Vec2> velocities;
    for (const auto& [system_id, node] : nodes_) {
        velocities[system_id] = Vec2(0, 0);
    }
    
    for (int iter = 0; iter < ITERATIONS; ++iter) {
        std::unordered_map<SystemID, Vec2> forces;
        
        // Initialize forces
        for (const auto& [system_id, node] : nodes_) {
            forces[system_id] = Vec2(0, 0);
        }
        
        // Repulsion forces between all nodes
        for (const auto& [id1, node1] : nodes_) {
            for (const auto& [id2, node2] : nodes_) {
                if (id1 != id2) {
                    Vec2 diff = node1.position.pos - node2.position.pos;
                    float distance = diff.length();
                    if (distance > 0) {
                        Vec2 force = diff.normalized() * (REPULSION_STRENGTH / (distance * distance));
                        forces[id1] += force;
                    }
                }
            }
        }
        
        // Attraction forces along edges
        for (const auto& edge : edges_) {
            auto from_it = nodes_.find(edge.from);
            auto to_it = nodes_.find(edge.to);
            
            if (from_it != nodes_.end() && to_it != nodes_.end()) {
                Vec2 diff = to_it->second.position.pos - from_it->second.position.pos;
                float distance = diff.length();
                Vec2 force = diff * SPRING_STRENGTH * distance;
                
                forces[edge.from] += force;
                forces[edge.to] -= force;
            }
        }
        
        // Update positions
        for (auto& [system_id, node] : nodes_) {
            velocities[system_id] = (velocities[system_id] + forces[system_id]) * DAMPING;
            node.position.pos += velocities[system_id];
        }
    }
}

Vec2 SystemDependencyGraph::get_node_center(const GraphNode& node) const {
    return Vec2(
        node.position.pos.x + node.position.size.x * 0.5f,
        node.position.pos.y + node.position.size.y * 0.5f
    );
}

std::vector<SystemID> SystemDependencyGraph::get_topological_order() const {
    std::vector<SystemID> result;
    std::unordered_map<SystemID, int> in_degree;
    std::unordered_map<SystemID, std::vector<SystemID>> adjacency;
    
    // Initialize in-degree and adjacency list
    for (const auto& [system_id, node] : nodes_) {
        in_degree[system_id] = 0;
        adjacency[system_id] = std::vector<SystemID>();
    }
    
    // Build adjacency list and calculate in-degrees
    for (const auto& edge : edges_) {
        adjacency[edge.from].push_back(edge.to);
        in_degree[edge.to]++;
    }
    
    // Kahn's algorithm for topological sorting
    std::queue<SystemID> queue;
    for (const auto& [system_id, degree] : in_degree) {
        if (degree == 0) {
            queue.push(system_id);
        }
    }
    
    while (!queue.empty()) {
        SystemID current = queue.front();
        queue.pop();
        result.push_back(current);
        
        for (const SystemID& neighbor : adjacency[current]) {
            in_degree[neighbor]--;
            if (in_degree[neighbor] == 0) {
                queue.push(neighbor);
            }
        }
    }
    
    return result;
}

// =============================================================================
// PERFORMANCE CHART IMPLEMENTATION
// =============================================================================

PerformanceChart::PerformanceChart(const std::string& title, size_t max_data_points)
    : title_(title), max_data_points_(max_data_points) {
}

void PerformanceChart::add_series(const std::string& name, const Vec4& color) {
    ChartSeries series;
    series.name = name;
    series.color = color;
    series_[name] = series;
}

void PerformanceChart::add_data_point(const std::string& series_name, float timestamp, float value) {
    auto it = series_.find(series_name);
    if (it == series_.end()) {
        add_series(series_name);
        it = series_.find(series_name);
    }
    
    it->second.data.push_back({timestamp, value});
    
    // Remove old data points
    while (it->second.data.size() > max_data_points_) {
        it->second.data.erase(it->second.data.begin());
    }
    
    // Update auto-scale if needed
    if (auto_scale_y_) {
        update_auto_scale();
    }
}

void PerformanceChart::render_chart(const Rect& chart_rect) {
#ifdef ECSCOPE_HAS_IMGUI
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    
    // Chart background
    draw_list->AddRectFilled(
        ImVec2(chart_rect.min.x, chart_rect.min.y),
        ImVec2(chart_rect.max.x, chart_rect.max.y),
        IM_COL32(20, 20, 20, 255)
    );
    
    // Chart border
    draw_list->AddRect(
        ImVec2(chart_rect.min.x, chart_rect.min.y),
        ImVec2(chart_rect.max.x, chart_rect.max.y),
        IM_COL32(100, 100, 100, 255)
    );
    
    // Title
    ImVec2 title_size = ImGui::CalcTextSize(title_.c_str());
    ImVec2 title_pos = ImVec2(
        chart_rect.min.x + (chart_rect.width() - title_size.x) * 0.5f,
        chart_rect.min.y + 5
    );
    draw_list->AddText(title_pos, IM_COL32(255, 255, 255, 255), title_.c_str());
    
    // Plot area (leave space for title, legend, and margins)
    float legend_height = show_legend_ ? 30.0f : 0.0f;
    Rect plot_rect(
        chart_rect.min.x + 40.0f,  // Left margin for Y axis labels
        chart_rect.min.y + 30.0f,  // Top margin for title
        chart_rect.max.x - 10.0f,  // Right margin
        chart_rect.max.y - legend_height - 20.0f  // Bottom margin for X axis and legend
    );
    
    // Render grid
    if (show_grid_) {
        render_grid(*reinterpret_cast<DrawList*>(draw_list), plot_rect);
    }
    
    // Render series
    for (const auto& [name, series] : series_) {
        if (series.visible && !series.data.empty()) {
            render_series(*reinterpret_cast<DrawList*>(draw_list), series, plot_rect);
        }
    }
    
    // Render legend
    if (show_legend_) {
        render_legend(*reinterpret_cast<DrawList*>(draw_list), chart_rect);
    }
    
    // Y-axis labels
    const int NUM_Y_LABELS = 5;
    for (int i = 0; i <= NUM_Y_LABELS; ++i) {
        float y_value = min_y_ + (max_y_ - min_y_) * i / NUM_Y_LABELS;
        float y_pos = plot_rect.max.y - (plot_rect.height() * i / NUM_Y_LABELS);
        
        std::string label = std::format("{:.1f}", y_value);
        draw_list->AddText(
            ImVec2(chart_rect.min.x + 5, y_pos - 8),
            IM_COL32(200, 200, 200, 255),
            label.c_str()
        );
    }
#endif
}

void PerformanceChart::render_grid(DrawList& draw_list, const Rect& plot_rect) {
    // Vertical grid lines (time)
    const int NUM_V_LINES = 10;
    for (int i = 0; i <= NUM_V_LINES; ++i) {
        float x = plot_rect.min.x + (plot_rect.width() * i / NUM_V_LINES);
        draw_list.add_line(
            Vec2(x, plot_rect.min.y),
            Vec2(x, plot_rect.max.y),
            0x40FFFFFF  // Semi-transparent white
        );
    }
    
    // Horizontal grid lines (values)
    const int NUM_H_LINES = 5;
    for (int i = 0; i <= NUM_H_LINES; ++i) {
        float y = plot_rect.min.y + (plot_rect.height() * i / NUM_H_LINES);
        draw_list.add_line(
            Vec2(plot_rect.min.x, y),
            Vec2(plot_rect.max.x, y),
            0x40FFFFFF  // Semi-transparent white
        );
    }
}

void PerformanceChart::render_series(DrawList& draw_list, const ChartSeries& series, const Rect& plot_rect) {
    if (series.data.size() < 2) return;
    
    // Find time range
    float current_time = series.data.back().timestamp;
    float min_time = current_time - time_window_;
    
    std::vector<Vec2> points;
    points.reserve(series.data.size());
    
    for (const auto& point : series.data) {
        if (point.timestamp >= min_time) {
            Vec2 screen_pos = value_to_screen(point.timestamp, point.value, plot_rect);
            points.push_back(screen_pos);
        }
    }
    
    // Draw line series
    if (points.size() >= 2) {
        for (size_t i = 1; i < points.size(); ++i) {
            draw_list.add_line(
                points[i-1],
                points[i],
                series.color.to_rgba(),
                series.thickness
            );
        }
    }
    
    // Draw points
    for (const Vec2& point : points) {
        draw_list.add_circle_filled(point, 2.0f, series.color.to_rgba());
    }
}

void PerformanceChart::render_legend(DrawList& draw_list, const Rect& chart_rect) {
    float legend_y = chart_rect.max.y - 25.0f;
    float legend_x = chart_rect.min.x + 50.0f;
    
    for (const auto& [name, series] : series_) {
        if (series.visible) {
            // Color indicator
            draw_list.add_rect_filled(
                Rect(legend_x, legend_y, 10, 10),
                series.color.to_rgba()
            );
            
            // Series name
            draw_list.add_text(
                Vec2(legend_x + 15, legend_y - 2),
                0xFFFFFFFF,
                name
            );
            
            legend_x += 100.0f; // Space between legend items
        }
    }
}

Vec2 PerformanceChart::value_to_screen(float timestamp, float value, const Rect& plot_rect) const {
    // Find current time range
    float current_time = std::chrono::duration<float>(
        std::chrono::steady_clock::now().time_since_epoch()
    ).count();
    
    float time_ratio = (timestamp - (current_time - time_window_)) / time_window_;
    float value_ratio = (value - min_y_) / (max_y_ - min_y_);
    
    return Vec2(
        plot_rect.min.x + time_ratio * plot_rect.width(),
        plot_rect.max.y - value_ratio * plot_rect.height()
    );
}

void PerformanceChart::update_auto_scale() {
    if (series_.empty()) return;
    
    float min_val = std::numeric_limits<float>::max();
    float max_val = std::numeric_limits<float>::lowest();
    
    for (const auto& [name, series] : series_) {
        for (const auto& point : series.data) {
            min_val = std::min(min_val, point.value);
            max_val = std::max(max_val, point.value);
        }
    }
    
    if (min_val != std::numeric_limits<float>::max()) {
        // Add some padding
        float range = max_val - min_val;
        float padding = range * 0.1f;
        
        min_y_ = min_val - padding;
        max_y_ = max_val + padding;
        
        // Ensure minimum range
        if (range < 0.001f) {
            min_y_ = min_val - 0.5f;
            max_y_ = max_val + 0.5f;
        }
    }
}

PerformanceChart::ChartStats PerformanceChart::get_series_stats(const std::string& series_name) const {
    ChartStats stats;
    
    auto it = series_.find(series_name);
    if (it == series_.end() || it->second.data.empty()) {
        return stats;
    }
    
    const auto& data = it->second.data;
    stats.data_points = data.size();
    
    if (!data.empty()) {
        stats.current_value = data.back().value;
        
        float sum = 0.0f;
        stats.min_value = std::numeric_limits<float>::max();
        stats.max_value = std::numeric_limits<float>::lowest();
        
        for (const auto& point : data) {
            sum += point.value;
            stats.min_value = std::min(stats.min_value, point.value);
            stats.max_value = std::max(stats.max_value, point.value);
        }
        
        stats.avg_value = sum / data.size();
    }
    
    return stats;
}

// =============================================================================
// MEMORY USAGE WIDGET IMPLEMENTATION
// =============================================================================

MemoryUsageWidget::MemoryUsageWidget() : memory_chart_("Memory Usage") {
    memory_chart_.add_series("Total Memory", Vec4(0.2f, 0.6f, 1.0f, 1.0f));
    memory_chart_.add_series("Arena Memory", Vec4(0.8f, 0.2f, 0.2f, 1.0f));
    memory_chart_.add_series("Pool Memory", Vec4(0.2f, 0.8f, 0.2f, 1.0f));
}

void MemoryUsageWidget::update_memory_info(const ecs::ECSMemoryStats& memory_stats) {
    // Add current memory usage to timeline
    float current_time = std::chrono::duration<float>(
        std::chrono::steady_clock::now().time_since_epoch()
    ).count();
    
    float total_memory = static_cast<float>(memory_stats.archetype_arena_used + memory_stats.entity_pool_used);
    memory_chart_.add_data_point("Total Memory", current_time, total_memory / (1024.0f * 1024.0f)); // MB
    memory_chart_.add_data_point("Arena Memory", current_time, static_cast<float>(memory_stats.archetype_arena_used) / (1024.0f * 1024.0f));
    memory_chart_.add_data_point("Pool Memory", current_time, static_cast<float>(memory_stats.entity_pool_used) / (1024.0f * 1024.0f));
    
    // Update memory blocks
    memory_blocks_.clear();
    
    MemoryBlock total_block;
    total_block.name = "Total ECS Memory";
    total_block.size = memory_stats.archetype_arena_total + memory_stats.entity_pool_total;
    total_block.used = memory_stats.archetype_arena_used + memory_stats.entity_pool_used;
    total_block.color = Vec4(0.3f, 0.7f, 1.0f, 1.0f);
    
    // Arena block
    MemoryBlock arena_block;
    arena_block.name = "Archetype Arena";
    arena_block.size = memory_stats.archetype_arena_total;
    arena_block.used = memory_stats.archetype_arena_used;
    arena_block.color = Vec4(0.8f, 0.3f, 0.3f, 1.0f);
    total_block.sub_blocks.push_back(arena_block);
    
    // Pool block
    MemoryBlock pool_block;
    pool_block.name = "Entity Pool";
    pool_block.size = memory_stats.entity_pool_total;
    pool_block.used = memory_stats.entity_pool_used;
    pool_block.color = Vec4(0.3f, 0.8f, 0.3f, 1.0f);
    total_block.sub_blocks.push_back(pool_block);
    
    memory_blocks_.push_back(total_block);
}

void MemoryUsageWidget::render_memory_overview(const Rect& widget_rect) {
#ifdef ECSCOPE_HAS_IMGUI
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    
    float y_offset = widget_rect.min.y;
    
    for (const auto& block : memory_blocks_) {
        render_memory_block(*reinterpret_cast<DrawList*>(draw_list), block, 
                           Rect(widget_rect.min.x, y_offset, widget_rect.max.x, y_offset + 60), 0);
        y_offset += 80; // Space between blocks
    }
#endif
}

void MemoryUsageWidget::render_memory_block(DrawList& draw_list, const MemoryBlock& block, 
                                           const Rect& rect, int depth) {
    float usage_ratio = block.size > 0 ? static_cast<float>(block.used) / block.size : 0.0f;
    
    // Background
    draw_list.add_rect_filled(rect, 0xFF202020);
    
    // Used memory bar
    Rect used_rect = rect;
    used_rect.max.x = rect.min.x + rect.width() * usage_ratio;
    draw_list.add_rect_filled(used_rect, get_usage_color(usage_ratio).to_rgba());
    
    // Border
    draw_list.add_rect(rect, 0xFF606060);
    
    // Text
    std::string text = std::format("{} ({:.1f}%)", block.name, usage_ratio * 100.0f);
    if (show_sizes_) {
        text += std::format(" - {}/{}", format_bytes(block.used), format_bytes(block.size));
    }
    
    Vec2 text_pos(rect.min.x + 5 + depth * 20, rect.min.y + (rect.height() - 14) * 0.5f);
    draw_list.add_text(text_pos, 0xFFFFFFFF, text);
    
    // Render sub-blocks if expanded
    if (block.expanded) {
        float sub_y = rect.max.y + 5;
        for (const auto& sub_block : block.sub_blocks) {
            Rect sub_rect(rect.min.x + 20, sub_y, rect.max.x, sub_y + 40);
            render_memory_block(draw_list, sub_block, sub_rect, depth + 1);
            sub_y += 45;
        }
    }
}

Vec4 MemoryUsageWidget::get_usage_color(float usage_ratio) const {
    if (usage_ratio < 0.5f) {
        return Vec4(0.2f, 0.8f, 0.2f, 1.0f); // Green
    } else if (usage_ratio < 0.8f) {
        return Vec4(1.0f, 0.8f, 0.2f, 1.0f); // Yellow
    } else {
        return Vec4(0.8f, 0.2f, 0.2f, 1.0f); // Red
    }
}

std::string MemoryUsageWidget::format_bytes(size_t bytes) const {
    return ::ecscope::gui::format_bytes(bytes);
}

void MemoryUsageWidget::render_memory_timeline(const Rect& widget_rect) {
    memory_chart_.render_chart(widget_rect);
}

// =============================================================================
// UTILITY FUNCTIONS IMPLEMENTATION
// =============================================================================

Vec4 get_category_color(const std::string& category) {
    // Hash the category name to get consistent colors
    std::hash<std::string> hasher;
    size_t hash = hasher(category);
    
    float hue = static_cast<float>(hash % 360) / 360.0f;
    float sat = 0.7f;
    float val = 0.9f;
    
    // Convert HSV to RGB (simplified)
    float c = val * sat;
    float x = c * (1.0f - std::abs(std::fmod(hue * 6.0f, 2.0f) - 1.0f));
    float m = val - c;
    
    float r, g, b;
    if (hue < 1.0f/6.0f) { r = c; g = x; b = 0; }
    else if (hue < 2.0f/6.0f) { r = x; g = c; b = 0; }
    else if (hue < 3.0f/6.0f) { r = 0; g = c; b = x; }
    else if (hue < 4.0f/6.0f) { r = 0; g = x; b = c; }
    else if (hue < 5.0f/6.0f) { r = x; g = 0; b = c; }
    else { r = c; g = 0; b = x; }
    
    return Vec4(r + m, g + m, b + m, 1.0f);
}

Vec4 get_performance_color(float performance_ratio) {
    if (performance_ratio < 0.3f) {
        return Vec4(0.8f, 0.2f, 0.2f, 1.0f); // Red (bad)
    } else if (performance_ratio < 0.7f) {
        return Vec4(1.0f, 0.8f, 0.2f, 1.0f); // Yellow (medium)
    } else {
        return Vec4(0.2f, 0.8f, 0.2f, 1.0f); // Green (good)
    }
}

Vec4 get_memory_usage_color(float usage_ratio) {
    if (usage_ratio < 0.5f) {
        return Vec4(0.2f, 0.8f, 0.2f, 1.0f); // Green (low usage)
    } else if (usage_ratio < 0.8f) {
        return Vec4(1.0f, 0.8f, 0.2f, 1.0f); // Yellow (medium usage)
    } else {
        return Vec4(0.8f, 0.2f, 0.2f, 1.0f); // Red (high usage)
    }
}

} // namespace ecscope::gui