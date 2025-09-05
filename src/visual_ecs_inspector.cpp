/**
 * @file visual_ecs_inspector.cpp
 * @brief Implementation of Advanced Visual ECS Inspector
 */

#include "ecscope/visual_ecs_inspector.hpp"
#include "ecs/registry.hpp"
#include "ecs/system.hpp"
#include "ecscope/memory_tracker.hpp"
#include "ecscope/system_dependency_visualizer.hpp"
#include "ecscope/sparse_set_visualizer.hpp"
#include "core/time.hpp"
#include "core/log.hpp"
#include <imgui.h>
#include <imgui_internal.h>
#include <algorithm>
#include <numeric>
#include <cmath>
#include <fstream>
#include <sstream>

namespace ecscope::ui {

namespace {
    // Constants for visualization
    constexpr f32 ARCHETYPE_NODE_MIN_SIZE = 80.0f;
    constexpr f32 ARCHETYPE_NODE_MAX_SIZE = 200.0f;
    constexpr f32 SYSTEM_NODE_HEIGHT = 60.0f;
    constexpr f32 CONNECTION_THICKNESS = 2.0f;
    constexpr f32 GRAPH_ZOOM_MIN = 0.1f;
    constexpr f32 GRAPH_ZOOM_MAX = 3.0f;
    constexpr f64 DEFAULT_UPDATE_FREQUENCY = 10.0; // 10 Hz
    
    // Color constants
    constexpr ImU32 COLOR_ARCHETYPE_DEFAULT = IM_COL32(100, 149, 237, 200);
    constexpr ImU32 COLOR_ARCHETYPE_HOT = IM_COL32(255, 69, 0, 200);
    constexpr ImU32 COLOR_ARCHETYPE_SELECTED = IM_COL32(255, 215, 0, 255);
    constexpr ImU32 COLOR_SYSTEM_NORMAL = IM_COL32(144, 238, 144, 200);
    constexpr ImU32 COLOR_SYSTEM_BOTTLENECK = IM_COL32(255, 99, 71, 200);
    constexpr ImU32 COLOR_SYSTEM_OVER_BUDGET = IM_COL32(220, 20, 60, 200);
    constexpr ImU32 COLOR_CONNECTION = IM_COL32(169, 169, 169, 128);
    
    // Utility functions
    f32 lerp(f32 a, f32 b, f32 t) {
        return a + t * (b - a);
    }
    
    ImVec2 lerp_vec2(const ImVec2& a, const ImVec2& b, f32 t) {
        return ImVec2(lerp(a.x, b.x, t), lerp(a.y, b.y, t));
    }
    
    f32 distance(const ImVec2& a, const ImVec2& b) {
        f32 dx = b.x - a.x;
        f32 dy = b.y - a.y;
        return std::sqrt(dx * dx + dy * dy);
    }
    
    bool point_in_rect(const ImVec2& point, const ImVec2& rect_min, const ImVec2& rect_max) {
        return point.x >= rect_min.x && point.x <= rect_max.x &&
               point.y >= rect_min.y && point.y <= rect_max.y;
    }
}

VisualECSInspector::VisualECSInspector()
    : Panel("Visual ECS Inspector")
    , memory_data_(std::make_unique<MemoryVisualizationData>())
    , entity_browser_(std::make_unique<EntityBrowserData>())
    , sparse_set_data_(std::make_unique<SparseSetVisualization>())
    , performance_timeline_(std::make_unique<PerformanceTimeline>())
    , last_update_time_(0.0)
    , update_frequency_(DEFAULT_UPDATE_FREQUENCY)
    , show_archetype_graph_(true)
    , show_system_profiler_(true)
    , show_memory_visualizer_(true)
    , show_entity_browser_(true)
    , show_sparse_set_view_(false)
    , show_performance_timeline_(true)
    , show_educational_hints_(true)
    , graph_pan_offset_(0, 0)
    , graph_zoom_(1.0f)
    , selected_archetype_id_(0)
    , is_dragging_graph_(false)
    , drag_start_pos_(0, 0)
    , filter_hot_archetypes_(false)
    , filter_bottleneck_systems_(false)
    , hot_archetype_threshold_(10.0)
    , system_bottleneck_threshold_(16.6)
    , memory_pressure_threshold_(0.8)
    , show_concept_explanations_(true)
    , enable_data_export_(false)
{
    initialize_color_scheme();
    initialize_educational_content();
}

void VisualECSInspector::render() {
    if (!ImGui::Begin(name_.c_str(), &is_visible_)) {
        ImGui::End();
        return;
    }
    
    render_main_menu_bar();
    
    // Create tab bar for different views
    if (ImGui::BeginTabBar("ECS_Inspector_Tabs")) {
        
        if (show_archetype_graph_ && ImGui::BeginTabItem("Archetype Graph")) {
            render_archetype_graph();
            ImGui::EndTabItem();
        }
        
        if (show_system_profiler_ && ImGui::BeginTabItem("System Profiler")) {
            render_system_profiler();
            ImGui::EndTabItem();
        }
        
        if (show_memory_visualizer_ && ImGui::BeginTabItem("Memory Visualizer")) {
            render_memory_visualizer();
            ImGui::EndTabItem();
        }
        
        if (show_entity_browser_ && ImGui::BeginTabItem("Entity Browser")) {
            render_entity_browser();
            ImGui::EndTabItem();
        }
        
        if (show_sparse_set_view_ && ImGui::BeginTabItem("Sparse Sets")) {
            render_sparse_set_visualization();
            ImGui::EndTabItem();
        }
        
        if (show_performance_timeline_ && ImGui::BeginTabItem("Performance Timeline")) {
            render_performance_timeline();
            ImGui::EndTabItem();
        }
        
        ImGui::EndTabBar();
    }
    
    render_statistics_summary();
    
    if (show_educational_hints_) {
        render_educational_tooltips();
    }
    
    ImGui::End();
}

void VisualECSInspector::update(f64 delta_time) {
    f64 current_time = core::get_time_seconds();
    
    if (should_update_data()) {
        std::lock_guard<std::mutex> lock(data_mutex_);
        
        update_archetype_data();
        update_system_data();
        update_memory_data();
        update_entity_browser_data();
        update_sparse_set_data();
        update_performance_timeline();
        
        last_update_time_ = current_time;
    }
    
    // Update graph layout with simple spring simulation
    if (show_archetype_graph_) {
        layout_archetype_nodes_force_directed();
    }
}

void VisualECSInspector::render_main_menu_bar() {
    if (ImGui::BeginMenuBar()) {
        if (ImGui::BeginMenu("View")) {
            ImGui::MenuItem("Archetype Graph", nullptr, &show_archetype_graph_);
            ImGui::MenuItem("System Profiler", nullptr, &show_system_profiler_);
            ImGui::MenuItem("Memory Visualizer", nullptr, &show_memory_visualizer_);
            ImGui::MenuItem("Entity Browser", nullptr, &show_entity_browser_);
            ImGui::MenuItem("Sparse Set View", nullptr, &show_sparse_set_view_);
            ImGui::MenuItem("Performance Timeline", nullptr, &show_performance_timeline_);
            ImGui::Separator();
            ImGui::MenuItem("Educational Hints", nullptr, &show_educational_hints_);
            ImGui::EndMenu();
        }
        
        if (ImGui::BeginMenu("Filters")) {
            ImGui::MenuItem("Hot Archetypes Only", nullptr, &filter_hot_archetypes_);
            ImGui::MenuItem("Bottleneck Systems Only", nullptr, &filter_bottleneck_systems_);
            ImGui::Separator();
            ImGui::SliderFloat("Hot Threshold", &hot_archetype_threshold_, 1.0f, 100.0f, "%.1f/s");
            ImGui::SliderFloat("Bottleneck Threshold", &system_bottleneck_threshold_, 1.0f, 33.3f, "%.1f ms");
            ImGui::EndMenu();
        }
        
        if (ImGui::BeginMenu("Export")) {
            if (ImGui::MenuItem("Export Archetype Data")) {
                export_archetype_data("archetype_data.json");
            }
            if (ImGui::MenuItem("Export System Performance")) {
                export_system_performance("system_performance.csv");
            }
            if (ImGui::MenuItem("Export Memory Analysis")) {
                export_memory_analysis("memory_analysis.json");
            }
            if (ImGui::MenuItem("Export Performance Timeline")) {
                export_performance_timeline("performance_timeline.csv");
            }
            ImGui::EndMenu();
        }
        
        ImGui::EndMenuBar();
    }
}

void VisualECSInspector::render_archetype_graph() {
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    ImVec2 canvas_pos = ImGui::GetCursorScreenPos();
    ImVec2 canvas_size = ImGui::GetContentRegionAvail();
    
    if (canvas_size.x < 50.0f || canvas_size.y < 50.0f) return;
    
    // Handle graph interaction
    handle_graph_interaction();
    
    // Draw background grid
    ImU32 grid_color = IM_COL32(50, 50, 50, 100);
    f32 grid_size = 50.0f * graph_zoom_;
    for (f32 x = fmodf(graph_pan_offset_.x, grid_size); x < canvas_size.x; x += grid_size) {
        draw_list->AddLine(
            ImVec2(canvas_pos.x + x, canvas_pos.y),
            ImVec2(canvas_pos.x + x, canvas_pos.y + canvas_size.y),
            grid_color
        );
    }
    for (f32 y = fmodf(graph_pan_offset_.y, grid_size); y < canvas_size.y; y += grid_size) {
        draw_list->AddLine(
            ImVec2(canvas_pos.x, canvas_pos.y + y),
            ImVec2(canvas_pos.x + canvas_size.x, canvas_pos.y + y),
            grid_color
        );
    }
    
    // Render connections first (so they appear behind nodes)
    render_archetype_connections();
    
    // Render archetype nodes
    for (auto& node : archetype_nodes_) {
        if (filter_hot_archetypes_ && !node.is_hot) continue;
        render_archetype_node(node);
    }
    
    // Show graph controls
    ImGui::BeginChild("GraphControls", ImVec2(200, 0), true);
    ImGui::Text("Graph Controls");
    ImGui::Separator();
    
    ImGui::SliderFloat("Zoom", &graph_zoom_, GRAPH_ZOOM_MIN, GRAPH_ZOOM_MAX, "%.2fx");
    if (ImGui::Button("Reset View")) {
        graph_pan_offset_ = ImVec2(0, 0);
        graph_zoom_ = 1.0f;
    }
    
    ImGui::Text("Selected: %s", selected_archetype_id_ > 0 ? "Archetype" : "None");
    
    ImGui::Spacing();
    ImGui::Text("Search Archetypes:");
    ImGui::InputText("##ArchetypeSearch", &archetype_search_);
    
    ImGui::Spacing();
    ImGui::Checkbox("Filter Hot Only", &filter_hot_archetypes_);
    
    ImGui::EndChild();
    
    ImGui::SameLine();
    
    // Graph drawing area
    ImGui::BeginChild("GraphCanvas", ImVec2(0, 0), true);
    // Graph interaction is handled above
    ImGui::EndChild();
}

void VisualECSInspector::render_archetype_node(const ArchetypeNode& node) {
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    ImVec2 canvas_pos = ImGui::GetCursorScreenPos();
    
    // Transform node position with pan and zoom
    ImVec2 screen_pos = ImVec2(
        canvas_pos.x + (node.position.x + graph_pan_offset_.x) * graph_zoom_,
        canvas_pos.y + (node.position.y + graph_pan_offset_.y) * graph_zoom_
    );
    
    ImVec2 node_size = ImVec2(node.size.x * graph_zoom_, node.size.y * graph_zoom_);
    
    // Draw node background
    ImU32 node_color = calculate_archetype_node_color(node);
    draw_list->AddRectFilled(
        screen_pos,
        ImVec2(screen_pos.x + node_size.x, screen_pos.y + node_size.y),
        node_color,
        5.0f * graph_zoom_
    );
    
    // Draw node border
    ImU32 border_color = node.is_selected ? COLOR_ARCHETYPE_SELECTED : IM_COL32(100, 100, 100, 255);
    draw_list->AddRect(
        screen_pos,
        ImVec2(screen_pos.x + node_size.x, screen_pos.y + node_size.y),
        border_color,
        5.0f * graph_zoom_,
        0,
        2.0f * graph_zoom_
    );
    
    // Draw text if zoom level allows it
    if (graph_zoom_ > 0.5f) {
        ImVec2 text_pos = ImVec2(screen_pos.x + 5 * graph_zoom_, screen_pos.y + 5 * graph_zoom_);
        
        // Archetype signature (simplified)
        std::string signature_text = "Archetype " + std::to_string(node.archetype_id);
        draw_list->AddText(text_pos, IM_COL32_WHITE, signature_text.c_str());
        
        // Entity count
        text_pos.y += 15 * graph_zoom_;
        std::string entity_text = "Entities: " + std::to_string(node.entity_count);
        draw_list->AddText(text_pos, IM_COL32(200, 200, 200, 255), entity_text.c_str());
        
        // Memory usage
        text_pos.y += 15 * graph_zoom_;
        std::string memory_text = "Memory: " + format_memory_size(node.memory_usage);
        draw_list->AddText(text_pos, IM_COL32(180, 180, 180, 255), memory_text.c_str());
        
        // Activity indicator
        if (node.is_hot) {
            ImVec2 indicator_pos = ImVec2(screen_pos.x + node_size.x - 15 * graph_zoom_, screen_pos.y + 5 * graph_zoom_);
            draw_list->AddCircleFilled(indicator_pos, 5 * graph_zoom_, COLOR_ARCHETYPE_HOT);
        }
    }
}

void VisualECSInspector::render_archetype_connections() {
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    ImVec2 canvas_pos = ImGui::GetCursorScreenPos();
    
    for (const auto& node : archetype_nodes_) {
        if (filter_hot_archetypes_ && !node.is_hot) continue;
        
        for (usize i = 0; i < node.connected_archetypes.size(); ++i) {
            u32 target_id = node.connected_archetypes[i];
            f32 weight = i < node.transition_weights.size() ? node.transition_weights[i] : 1.0f;
            
            // Find target node
            auto target_it = std::find_if(archetype_nodes_.begin(), archetype_nodes_.end(),
                [target_id](const ArchetypeNode& n) { return n.archetype_id == target_id; });
            
            if (target_it != archetype_nodes_.end()) {
                const ArchetypeNode& target = *target_it;
                
                if (filter_hot_archetypes_ && !target.is_hot) continue;
                
                // Transform positions
                ImVec2 start_pos = ImVec2(
                    canvas_pos.x + (node.position.x + node.size.x * 0.5f + graph_pan_offset_.x) * graph_zoom_,
                    canvas_pos.y + (node.position.y + node.size.y * 0.5f + graph_pan_offset_.y) * graph_zoom_
                );
                
                ImVec2 end_pos = ImVec2(
                    canvas_pos.x + (target.position.x + target.size.x * 0.5f + graph_pan_offset_.x) * graph_zoom_,
                    canvas_pos.y + (target.position.y + target.size.y * 0.5f + graph_pan_offset_.y) * graph_zoom_
                );
                
                // Draw connection with thickness based on weight
                f32 thickness = std::max(1.0f, CONNECTION_THICKNESS * weight * graph_zoom_);
                ImU32 connection_color = IM_COL32(169, 169, 169, static_cast<u8>(128 * weight));
                
                draw_list->AddLine(start_pos, end_pos, connection_color, thickness);
                
                // Draw arrowhead if zoom allows
                if (graph_zoom_ > 0.7f) {
                    ImVec2 direction = ImVec2(end_pos.x - start_pos.x, end_pos.y - start_pos.y);
                    f32 length = std::sqrt(direction.x * direction.x + direction.y * direction.y);
                    if (length > 0) {
                        direction.x /= length;
                        direction.y /= length;
                        
                        f32 arrow_size = 10.0f * graph_zoom_;
                        ImVec2 arrow_pos = ImVec2(end_pos.x - direction.x * 15 * graph_zoom_, 
                                                  end_pos.y - direction.y * 15 * graph_zoom_);
                        
                        ImVec2 perp = ImVec2(-direction.y, direction.x);
                        ImVec2 arrow_p1 = ImVec2(arrow_pos.x + perp.x * arrow_size, arrow_pos.y + perp.y * arrow_size);
                        ImVec2 arrow_p2 = ImVec2(arrow_pos.x - perp.x * arrow_size, arrow_pos.y - perp.y * arrow_size);
                        
                        draw_list->AddTriangleFilled(end_pos, arrow_p1, arrow_p2, connection_color);
                    }
                }
            }
        }
    }
}

void VisualECSInspector::render_system_profiler() {
    ImGui::Columns(2, "SystemProfilerColumns", true);
    
    // Left column - System list and performance bars
    ImGui::Text("System Performance");
    ImGui::Separator();
    
    render_system_performance_bars();
    
    ImGui::NextColumn();
    
    // Right column - System details and dependency graph
    ImGui::Text("System Dependencies");
    ImGui::Separator();
    
    render_system_dependency_view();
    
    ImGui::Columns(1);
    
    ImGui::Separator();
    render_system_timeline();
}

void VisualECSInspector::render_system_performance_bars() {
    for (const auto& node : system_nodes_) {
        if (filter_bottleneck_systems_ && !node.is_bottleneck) continue;
        
        // System name and basic info
        ImGui::Text("%s", node.system_name.c_str());
        
        // Execution time bar
        f32 time_ratio = static_cast<f32>(node.average_execution_time / node.time_budget);
        ImVec4 bar_color = time_ratio > 1.0f ? ImVec4(1.0f, 0.3f, 0.3f, 1.0f) : 
                          time_ratio > 0.8f ? ImVec4(1.0f, 0.8f, 0.3f, 1.0f) : 
                                             ImVec4(0.3f, 0.8f, 0.3f, 1.0f);
        
        ImGui::PushStyleColor(ImGuiCol_PlotHistogram, bar_color);
        ImGui::ProgressBar(std::min(time_ratio, 1.0f), ImVec2(-1, 0), "");
        ImGui::PopStyleColor();
        
        // Time information
        ImGui::SameLine();
        ImGui::Text("%.2fms / %.2fms (%.1f%%)", 
                   node.average_execution_time, 
                   node.time_budget, 
                   node.budget_utilization);
        
        // Bottleneck indicator
        if (node.is_bottleneck) {
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "[BOTTLENECK]");
        }
        
        ImGui::Spacing();
    }
}

void VisualECSInspector::render_system_dependency_view() {
    // Simple text-based dependency view for now
    for (const auto& node : system_nodes_) {
        if (filter_bottleneck_systems_ && !node.is_bottleneck) continue;
        
        if (ImGui::TreeNode(node.system_name.c_str())) {
            ImGui::Text("Phase: %d", static_cast<int>(node.phase));
            
            if (!node.dependencies.empty()) {
                ImGui::Text("Dependencies:");
                for (const auto& dep : node.dependencies) {
                    ImGui::BulletText("%s", dep.c_str());
                }
            }
            
            if (!node.dependents.empty()) {
                ImGui::Text("Dependents:");
                for (const auto& dep : node.dependents) {
                    ImGui::BulletText("%s", dep.c_str());
                }
            }
            
            ImGui::TreePop();
        }
    }
}

void VisualECSInspector::render_system_timeline() {
    ImGui::Text("System Execution Timeline");
    
    // Simple timeline visualization
    auto recent_samples = performance_timeline_->get_recent_samples(100);
    if (!recent_samples.empty()) {
        std::vector<f32> frame_times;
        std::vector<f32> system_times;
        
        frame_times.reserve(recent_samples.size());
        system_times.reserve(recent_samples.size());
        
        for (const auto& sample : recent_samples) {
            frame_times.push_back(static_cast<f32>(sample.frame_time * 1000.0)); // Convert to ms
            system_times.push_back(static_cast<f32>(sample.system_time * 1000.0));
        }
        
        ImGui::PlotLines("Frame Time (ms)", frame_times.data(), static_cast<int>(frame_times.size()), 
                        0, nullptr, 0.0f, 33.3f, ImVec2(0, 80));
        
        ImGui::PlotLines("System Time (ms)", system_times.data(), static_cast<int>(system_times.size()), 
                        0, nullptr, 0.0f, 33.3f, ImVec2(0, 80));
    }
}

void VisualECSInspector::render_memory_visualizer() {
    ImGui::Columns(2, "MemoryColumns", true);
    
    // Left column - Memory overview
    render_memory_allocation_map();
    
    ImGui::NextColumn();
    
    // Right column - Category breakdown and analysis
    render_memory_category_breakdown();
    
    ImGui::Columns(1);
    
    ImGui::Separator();
    render_cache_performance_analysis();
}

void VisualECSInspector::render_memory_allocation_map() {
    ImGui::Text("Memory Allocation Map");
    ImGui::Separator();
    
    if (!memory_data_) return;
    
    // Memory pressure gauge
    render_memory_pressure_gauge();
    
    // Simple allocation blocks visualization
    ImGui::Text("Active Allocations: %zu", memory_data_->blocks.size());
    ImGui::Text("Total Allocated: %s", format_memory_size(memory_data_->total_allocated).c_str());
    ImGui::Text("Peak Allocated: %s", format_memory_size(memory_data_->peak_allocated).c_str());
    ImGui::Text("Fragmentation: %.1f%%", memory_data_->fragmentation_ratio * 100.0);
    
    // Allocation blocks (simplified view)
    ImGui::BeginChild("AllocationBlocks", ImVec2(0, 200), true);
    
    for (const auto& block : memory_data_->blocks) {
        if (!block.is_active) continue;
        
        ImU32 block_color = heat_map_color(static_cast<f32>(block.age / 10.0)); // Age-based coloring
        
        ImGui::PushStyleColor(ImGuiCol_Button, block_color);
        ImGui::Button("##block", ImVec2(std::max(10.0f, static_cast<f32>(block.size) / 1024.0f), 20));
        ImGui::PopStyleColor();
        
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Size: %s\nAge: %.2fs\nCategory: %d", 
                             format_memory_size(block.size).c_str(),
                             block.age,
                             static_cast<int>(block.category));
        }
        
        ImGui::SameLine();
    }
    
    ImGui::EndChild();
}

void VisualECSInspector::render_memory_category_breakdown() {
    ImGui::Text("Memory by Category");
    ImGui::Separator();
    
    // Placeholder data - in a real implementation, this would come from the memory tracker
    static const std::array<const char*, 6> categories = {
        "ECS Core", "Components", "Systems", "Renderer", "Physics", "Other"
    };
    
    static const std::array<usize, 6> category_sizes = {
        1024 * 1024,    // 1MB
        512 * 1024,     // 512KB
        256 * 1024,     // 256KB
        2 * 1024 * 1024, // 2MB
        128 * 1024,     // 128KB
        64 * 1024       // 64KB
    };
    
    for (usize i = 0; i < categories.size(); ++i) {
        f32 percentage = static_cast<f32>(category_sizes[i]) / static_cast<f32>(memory_data_->total_allocated);
        
        ImGui::Text("%s", categories[i]);
        ImGui::SameLine();
        ImGui::Text("(%s)", format_memory_size(category_sizes[i]).c_str());
        
        ImGui::ProgressBar(percentage, ImVec2(-1, 0), "");
        
        ImGui::Spacing();
    }
}

void VisualECSInspector::render_memory_pressure_gauge() {
    ImGui::Text("Memory Pressure");
    
    // Simple gauge visualization
    f32 pressure = static_cast<f32>(memory_data_->total_allocated) / static_cast<f32>(memory_data_->peak_allocated);
    if (memory_data_->peak_allocated == 0) pressure = 0.0f;
    
    ImVec4 gauge_color = pressure > 0.9f ? ImVec4(1.0f, 0.2f, 0.2f, 1.0f) :
                        pressure > 0.7f ? ImVec4(1.0f, 0.8f, 0.2f, 1.0f) :
                                         ImVec4(0.2f, 0.8f, 0.2f, 1.0f);
    
    ImGui::PushStyleColor(ImGuiCol_PlotHistogram, gauge_color);
    ImGui::ProgressBar(pressure, ImVec2(-1, 0), "");
    ImGui::PopStyleColor();
    
    ImGui::SameLine();
    ImGui::Text("%.1f%%", pressure * 100.0f);
}

void VisualECSInspector::render_cache_performance_analysis() {
    ImGui::Text("Cache Performance Analysis");
    ImGui::Separator();
    
    ImGui::Text("Estimated Cache Hit Rate: %.1f%%", memory_data_->cache_hit_rate * 100.0);
    
    // Cache-friendly vs cache-hostile patterns
    ImGui::Columns(2, "CacheAnalysis", false);
    
    ImGui::Text("Cache-Friendly Patterns:");
    ImGui::BulletText("Sequential access");
    ImGui::BulletText("Spatial locality");
    ImGui::BulletText("Predictable patterns");
    
    ImGui::NextColumn();
    
    ImGui::Text("Cache-Hostile Patterns:");
    ImGui::BulletText("Random access");
    ImGui::BulletText("Large strides");
    ImGui::BulletText("Frequent pointer chasing");
    
    ImGui::Columns(1);
}

void VisualECSInspector::render_entity_browser() {
    if (!entity_browser_) return;
    
    // Search and filter controls
    render_entity_search_filters();
    
    ImGui::Separator();
    
    // Entity list
    ImGui::Columns(2, "EntityBrowserColumns", true);
    
    render_entity_list();
    
    ImGui::NextColumn();
    
    render_entity_details();
    
    ImGui::Columns(1);
}

void VisualECSInspector::render_entity_search_filters() {
    ImGui::Text("Entity Browser");
    
    // Search box
    ImGui::InputText("Search", &entity_browser_->search_filter);
    ImGui::SameLine();
    if (ImGui::Button("Clear")) {
        entity_browser_->search_filter.clear();
        entity_browser_->component_filter.clear();
        entity_browser_->selected_archetypes.clear();
    }
    
    // Component filter
    ImGui::InputText("Component Filter", &entity_browser_->component_filter);
    
    // Show only modified checkbox
    ImGui::Checkbox("Show Only Recently Modified", &entity_browser_->show_only_modified);
    
    // Sort options
    ImGui::Text("Sort by:");
    ImGui::SameLine();
    
    const char* sort_options[] = { "Entity ID", "Archetype", "Component Count", "Last Modified" };
    int current_sort = static_cast<int>(entity_browser_->sort_mode);
    if (ImGui::Combo("##Sort", &current_sort, sort_options, IM_ARRAYSIZE(sort_options))) {
        entity_browser_->sort_mode = static_cast<EntityBrowserData::SortMode>(current_sort);
    }
    
    ImGui::SameLine();
    ImGui::Checkbox("Ascending", &entity_browser_->sort_ascending);
}

void VisualECSInspector::render_entity_list() {
    ImGui::Text("Entities (%zu)", entity_browser_->entities.size());
    
    ImGui::BeginChild("EntityList", ImVec2(0, 0), true);
    
    for (auto& entity_entry : entity_browser_->entities) {
        if (!entity_entry.matches_filter) continue;
        
        bool is_selected = entity_entry.is_selected;
        if (ImGui::Selectable(("Entity " + std::to_string(entity_entry.entity)).c_str(), is_selected)) {
            // Clear other selections
            for (auto& e : entity_browser_->entities) {
                e.is_selected = false;
            }
            entity_entry.is_selected = true;
        }
        
        // Show archetype info
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "(%s)", entity_entry.archetype_name.c_str());
    }
    
    ImGui::EndChild();
}

void VisualECSInspector::render_entity_details() {
    ImGui::Text("Entity Details");
    ImGui::Separator();
    
    // Find selected entity
    auto selected_it = std::find_if(entity_browser_->entities.begin(), entity_browser_->entities.end(),
        [](const EntityBrowserData::EntityEntry& e) { return e.is_selected; });
    
    if (selected_it != entity_browser_->entities.end()) {
        const auto& entity = *selected_it;
        
        ImGui::Text("Entity ID: %u", entity.entity);
        ImGui::Text("Archetype: %s", entity.archetype_name.c_str());
        ImGui::Text("Components: %zu", entity.components.size());
        ImGui::Text("Last Modified: %.2fs ago", core::get_time_seconds() - entity.last_modified);
        
        ImGui::Separator();
        
        // Component list
        ImGui::Text("Components:");
        for (const auto& component : entity.components) {
            ImGui::BulletText("%s", component.c_str());
        }
        
        // Component editor would go here
        render_component_editor();
    } else {
        ImGui::Text("No entity selected");
    }
}

void VisualECSInspector::render_component_editor() {
    ImGui::Separator();
    ImGui::Text("Component Editor");
    
    // Placeholder for component editing UI
    ImGui::Text("Component editing interface would appear here");
    ImGui::Text("This would allow real-time modification of component values");
}

void VisualECSInspector::render_sparse_set_visualization() {
    if (!sparse_set_data_) return;
    
    ImGui::Text("Sparse Set Storage Visualization");
    ImGui::Separator();
    
    // Get real-time data from sparse set analyzer
    auto& analyzer = visualization::GlobalSparseSetAnalyzer::instance();
    auto sparse_sets = analyzer.get_all_sparse_sets();
    
    if (sparse_sets.empty()) {
        ImGui::Text("No sparse sets registered for analysis.");
        ImGui::Text("Sparse sets will appear here when components are actively used.");
        return;
    }
    
    // Summary statistics
    ImGui::Text("Active Sparse Sets: %zu", sparse_sets.size());
    ImGui::Text("Total Memory Usage: %s", format_memory_size(analyzer.get_total_memory_usage()).c_str());
    ImGui::Text("Average Cache Locality: %.1f%%", analyzer.get_average_cache_locality() * 100.0);
    ImGui::Text("Overall Efficiency: %.1f%%", analyzer.get_overall_efficiency() * 100.0);
    
    ImGui::Separator();
    
    render_component_pool_visualization();
    
    ImGui::Separator();
    
    render_cache_locality_analysis();
    
    ImGui::Separator();
    
    render_educational_sparse_set_content();
}

void VisualECSInspector::render_component_pool_visualization() {
    ImGui::Text("Component Pools");
    
    auto& analyzer = visualization::GlobalSparseSetAnalyzer::instance();
    auto sparse_sets = analyzer.get_all_sparse_sets();
    
    for (const auto* sparse_set : sparse_sets) {
        if (ImGui::TreeNode(sparse_set->name.c_str())) {
            // Basic information
            ImGui::Text("Dense Size: %zu / %zu", sparse_set->dense_size, sparse_set->dense_capacity);
            ImGui::Text("Sparse Capacity: %zu", sparse_set->sparse_capacity);
            f64 utilization = sparse_set->dense_capacity > 0 ? 
                static_cast<f64>(sparse_set->dense_size) / sparse_set->dense_capacity : 0.0;
            ImGui::Text("Utilization: %.1f%%", utilization * 100.0);
            ImGui::Text("Memory Efficiency: %.1f%%", sparse_set->memory_efficiency * 100.0);
            
            // Performance metrics
            ImGui::Separator();
            ImGui::Text("Performance Metrics:");
            ImGui::Text("  Insertion Time: %s", format_time_duration_us(sparse_set->insertion_time_avg).c_str());
            ImGui::Text("  Removal Time: %s", format_time_duration_us(sparse_set->removal_time_avg).c_str());
            ImGui::Text("  Lookup Time: %s", format_time_duration_us(sparse_set->lookup_time_avg).c_str());
            ImGui::Text("  Iteration Time: %s", format_time_duration_us(sparse_set->iteration_time_avg).c_str());
            
            // Access pattern information
            ImGui::Separator();
            ImGui::Text("Access Pattern Analysis:");
            const auto& tracking = sparse_set->access_tracking;
            ImGui::Text("  Read Count: %llu", tracking.read_count);
            ImGui::Text("  Write Count: %llu", tracking.write_count);
            ImGui::Text("  Cache Hits: %llu", tracking.cache_hits);
            ImGui::Text("  Cache Misses: %llu", tracking.cache_misses);
            
            f64 cache_hit_ratio = (tracking.cache_hits + tracking.cache_misses) > 0 ?
                static_cast<f64>(tracking.cache_hits) / (tracking.cache_hits + tracking.cache_misses) : 0.0;
            ImGui::Text("  Cache Hit Ratio: %.1f%%", cache_hit_ratio * 100.0);
            
            // Visualize dense and sparse arrays
            ImGui::Separator();
            render_dense_sparse_arrays_for_set(*sparse_set);
            
            // Optimization suggestions
            if (!sparse_set->optimization_suggestions.empty()) {
                ImGui::Separator();
                ImGui::Text("Optimization Suggestions:");
                for (const auto& suggestion : sparse_set->optimization_suggestions) {
                    ImGui::BulletText("%s", suggestion.c_str());
                }
            }
            
            ImGui::TreePop();
        }
    }
}

void VisualECSInspector::render_dense_sparse_arrays() {
    // Legacy method - redirect to new implementation
    auto& analyzer = visualization::GlobalSparseSetAnalyzer::instance();
    auto sparse_sets = analyzer.get_all_sparse_sets();
    
    if (!sparse_sets.empty()) {
        render_dense_sparse_arrays_for_set(*sparse_sets[0]);
    }
}

void VisualECSInspector::render_dense_sparse_arrays_for_set(const visualization::SparseSetVisualizationData& sparse_set) {
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    ImVec2 cursor_pos = ImGui::GetCursorScreenPos();
    
    f32 cell_size = 12.0f;
    f32 spacing = 2.0f;
    
    // Render Dense Array
    ImGui::Text("Dense Array (Size: %zu):", sparse_set.dense_size);
    
    usize max_cells = std::min(sparse_set.dense_capacity, usize(60)); // Limit for visualization
    
    for (usize i = 0; i < max_cells; ++i) {
        ImVec2 cell_pos = ImVec2(
            cursor_pos.x + i * (cell_size + spacing),
            cursor_pos.y
        );
        
        ImU32 cell_color;
        if (i < sparse_set.dense_size) {
            // Occupied cell - color based on "temperature" (activity)
            f32 activity = static_cast<f32>(i) / std::max(1.0f, static_cast<f32>(sparse_set.dense_size));
            cell_color = heat_map_color(activity);
        } else {
            // Empty cell
            cell_color = IM_COL32(40, 40, 40, 255);
        }
        
        draw_list->AddRectFilled(cell_pos,
                                ImVec2(cell_pos.x + cell_size, cell_pos.y + cell_size),
                                cell_color);
        draw_list->AddRect(cell_pos,
                          ImVec2(cell_pos.x + cell_size, cell_pos.y + cell_size),
                          IM_COL32(100, 100, 100, 255));
        
        // Tooltip with information
        if (ImGui::IsMouseHoveringRect(cell_pos, ImVec2(cell_pos.x + cell_size, cell_pos.y + cell_size))) {
            ImGui::SetTooltip("Dense[%zu]: %s", i, 
                             i < sparse_set.dense_size ? "Occupied" : "Empty");
        }
    }
    
    // Move cursor down for sparse array
    ImGui::Dummy(ImVec2(0, cell_size + spacing * 2));
    cursor_pos = ImGui::GetCursorScreenPos();
    
    // Render Sparse Array (simplified view)
    ImGui::Text("Sparse Array (Capacity: %zu):", sparse_set.sparse_capacity);
    ImGui::Text("Showing mapping validity pattern:");
    
    max_cells = std::min(sparse_set.sparse_capacity, usize(60));
    
    for (usize i = 0; i < max_cells; ++i) {
        ImVec2 cell_pos = ImVec2(
            cursor_pos.x + i * (cell_size + spacing),
            cursor_pos.y
        );
        
        // Simulate sparse array validity (in real implementation, would read actual data)
        bool is_valid = i < sparse_set.dense_size; // Simplified
        
        ImU32 cell_color = is_valid ? IM_COL32(100, 150, 255, 200) : IM_COL32(30, 30, 30, 255);
        
        draw_list->AddRectFilled(cell_pos,
                                ImVec2(cell_pos.x + cell_size, cell_pos.y + cell_size),
                                cell_color);
        draw_list->AddRect(cell_pos,
                          ImVec2(cell_pos.x + cell_size, cell_pos.y + cell_size),
                          IM_COL32(100, 100, 100, 255));
        
        if (ImGui::IsMouseHoveringRect(cell_pos, ImVec2(cell_pos.x + cell_size, cell_pos.y + cell_size))) {
            ImGui::SetTooltip("Sparse[%zu]: %s", i,
                             is_valid ? "Valid mapping" : "Invalid/Empty");
        }
    }
    
    ImGui::Dummy(ImVec2(0, cell_size + spacing * 2));
    
    // Add performance indicators
    ImGui::Separator();
    ImGui::Text("Performance Indicators:");
    
    // Cache locality indicator
    f32 locality_score = static_cast<f32>(sparse_set.cache_locality_score);
    ImGui::Text("Cache Locality: ");
    ImGui::SameLine();
    ImGui::ProgressBar(locality_score, ImVec2(-1, 0), 
                      (std::to_string(static_cast<int>(locality_score * 100)) + "%").c_str());
    
    // Memory efficiency indicator
    f32 efficiency = static_cast<f32>(sparse_set.memory_efficiency);
    ImGui::Text("Memory Efficiency: ");
    ImGui::SameLine();
    ImGui::ProgressBar(efficiency, ImVec2(-1, 0),
                      (std::to_string(static_cast<int>(efficiency * 100)) + "%").c_str());
}

void VisualECSInspector::render_cache_locality_analysis() {
    auto& analyzer = visualization::GlobalSparseSetAnalyzer::instance();
    
    ImGui::Text("Cache Locality Analysis");
    
    f64 avg_locality = analyzer.get_average_cache_locality();
    f64 overall_efficiency = analyzer.get_overall_efficiency();
    
    ImGui::Text("Overall Memory Efficiency: %.1f%%", overall_efficiency * 100.0);
    ImGui::Text("Average Cache Locality Score: %.2f", avg_locality);
    
    // Visual indicators
    ImVec4 locality_color = avg_locality > 0.8 ? ImVec4(0.2f, 0.8f, 0.2f, 1.0f) :
                           avg_locality > 0.6 ? ImVec4(0.8f, 0.8f, 0.2f, 1.0f) :
                                               ImVec4(0.8f, 0.2f, 0.2f, 1.0f);
    
    ImGui::PushStyleColor(ImGuiCol_PlotHistogram, locality_color);
    ImGui::ProgressBar(static_cast<f32>(avg_locality), ImVec2(-1, 0), 
                      "Cache Locality");
    ImGui::PopStyleColor();
    
    ImVec4 efficiency_color = overall_efficiency > 0.7 ? ImVec4(0.2f, 0.8f, 0.2f, 1.0f) :
                             overall_efficiency > 0.5 ? ImVec4(0.8f, 0.8f, 0.2f, 1.0f) :
                                                       ImVec4(0.8f, 0.2f, 0.2f, 1.0f);
    
    ImGui::PushStyleColor(ImGuiCol_PlotHistogram, efficiency_color);
    ImGui::ProgressBar(static_cast<f32>(overall_efficiency), ImVec2(-1, 0),
                      "Memory Efficiency");
    ImGui::PopStyleColor();
    
    // Detailed analysis per sparse set
    ImGui::Separator();
    ImGui::Text("Per-Component Analysis:");
    
    auto sparse_sets = analyzer.get_all_sparse_sets();
    for (const auto* sparse_set : sparse_sets) {
        if (ImGui::TreeNode(("Analysis: " + sparse_set->name).c_str())) {
            ImGui::Text("Cache Locality: %.2f", sparse_set->cache_locality_score);
            ImGui::Text("Spatial Locality: %.2f", sparse_set->spatial_locality);
            ImGui::Text("Temporal Locality: %.2f", sparse_set->temporal_locality);
            ImGui::Text("Cache Lines Used: %zu", sparse_set->cache_line_utilization);
            
            // Access pattern analysis
            const auto& tracking = sparse_set->access_tracking;
            u64 total_accesses = tracking.cache_hits + tracking.cache_misses;
            if (total_accesses > 0) {
                f64 hit_rate = static_cast<f64>(tracking.cache_hits) / total_accesses;
                ImGui::Text("Cache Hit Rate: %.1f%%", hit_rate * 100.0);
                
                ImGui::PushStyleColor(ImGuiCol_PlotHistogram, 
                                     hit_rate > 0.8 ? ImVec4(0.2f, 0.8f, 0.2f, 1.0f) :
                                     hit_rate > 0.6 ? ImVec4(0.8f, 0.8f, 0.2f, 1.0f) :
                                                     ImVec4(0.8f, 0.2f, 0.2f, 1.0f));
                ImGui::ProgressBar(static_cast<f32>(hit_rate), ImVec2(-1, 0));
                ImGui::PopStyleColor();
            }
            
            ImGui::TreePop();
        }
    }
    
    // Recommendations
    ImGui::Separator();
    ImGui::Text("Optimization Recommendations:");
    
    if (avg_locality < 0.8) {
        ImGui::BulletText("Consider component pooling for better cache locality");
    }
    if (overall_efficiency < 0.7) {
        ImGui::BulletText("High fragmentation detected - consider compaction");
    }
    if (analyzer.get_total_sparse_sets() > 10) {
        ImGui::BulletText("Large number of component types - consider archetype optimization");
    }
    
    // Educational insights
    auto insights = analyzer.get_educational_insights();
    if (!insights.empty()) {
        ImGui::Separator();
        ImGui::Text("Educational Insights:");
        for (const auto& insight : insights) {
            ImGui::BulletText("%s", insight.c_str());
        }
    }
}

void VisualECSInspector::render_educational_sparse_set_content() {
    if (!show_educational_hints_) return;
    
    ImGui::Separator();
    ImGui::Text("Educational Content");
    
    auto& analyzer = visualization::GlobalSparseSetAnalyzer::instance();
    
    if (ImGui::CollapsingHeader("Sparse Set Concepts")) {
        ImGui::TextWrapped("%s", analyzer.explain_sparse_set_concept().c_str());
    }
    
    if (ImGui::CollapsingHeader("Cache Locality")) {
        ImGui::TextWrapped("%s", analyzer.explain_cache_locality().c_str());
    }
    
    if (ImGui::CollapsingHeader("Performance Tips")) {
        ImGui::TextWrapped("%s", analyzer.suggest_performance_improvements().c_str());
    }
    
    if (ImGui::CollapsingHeader("Key Benefits of Sparse Sets")) {
        ImGui::BulletText("O(1) insertion, deletion, and lookup operations");
        ImGui::BulletText("Dense arrays enable cache-friendly iteration");
        ImGui::BulletText("Efficient memory usage for sparse data");
        ImGui::BulletText("Excellent for ECS component storage");
        ImGui::BulletText("Supports fast bulk operations");
    }
    
    if (ImGui::CollapsingHeader("Memory Layout Visualization")) {
        ImGui::Text("Dense Array: [Component Data] (Contiguous)");
        ImGui::Text("Sparse Array: [Entity ID -> Dense Index] (Can have gaps)");
        ImGui::Spacing();
        ImGui::TextWrapped("The dense array contains actual component data in contiguous memory, "
                          "while the sparse array maps entity IDs to indices in the dense array. "
                          "This enables both fast random access and cache-friendly iteration.");
    }
}

void VisualECSInspector::render_performance_timeline() {
    ImGui::Text("Performance Timeline");
    ImGui::Separator();
    
    render_timeline_graphs();
    
    ImGui::Separator();
    
    render_performance_metrics();
    
    ImGui::Separator();
    
    render_bottleneck_analysis();
}

void VisualECSInspector::render_timeline_graphs() {
    if (!performance_timeline_) return;
    
    auto recent_samples = performance_timeline_->get_recent_samples(200);
    if (recent_samples.empty()) {
        ImGui::Text("No performance data available");
        return;
    }
    
    // Extract data for plotting
    std::vector<f32> frame_times, system_times, memory_usage, entity_counts;
    
    frame_times.reserve(recent_samples.size());
    system_times.reserve(recent_samples.size());
    memory_usage.reserve(recent_samples.size());
    entity_counts.reserve(recent_samples.size());
    
    for (const auto& sample : recent_samples) {
        frame_times.push_back(static_cast<f32>(sample.frame_time * 1000.0)); // ms
        system_times.push_back(static_cast<f32>(sample.system_time * 1000.0)); // ms
        memory_usage.push_back(static_cast<f32>(sample.memory_usage));
        entity_counts.push_back(static_cast<f32>(sample.entity_count));
    }
    
    // Plot the graphs
    ImGui::PlotLines("Frame Time (ms)", frame_times.data(), static_cast<int>(frame_times.size()), 
                    0, nullptr, 0.0f, 33.3f, ImVec2(0, 100));
    
    ImGui::PlotLines("System Time (ms)", system_times.data(), static_cast<int>(system_times.size()), 
                    0, nullptr, 0.0f, 33.3f, ImVec2(0, 100));
    
    ImGui::PlotLines("Memory Usage (%)", memory_usage.data(), static_cast<int>(memory_usage.size()), 
                    0, nullptr, 0.0f, 100.0f, ImVec2(0, 100));
    
    ImGui::PlotLines("Entity Count", entity_counts.data(), static_cast<int>(entity_counts.size()), 
                    0, nullptr, 0.0f, 0.0f, ImVec2(0, 100));
}

void VisualECSInspector::render_performance_metrics() {
    if (!performance_timeline_) return;
    
    auto recent_samples = performance_timeline_->get_recent_samples(10);
    if (recent_samples.empty()) return;
    
    // Calculate averages
    f64 avg_frame_time = 0.0;
    f64 avg_system_time = 0.0;
    f64 avg_memory_usage = 0.0;
    u64 avg_entity_count = 0;
    
    for (const auto& sample : recent_samples) {
        avg_frame_time += sample.frame_time;
        avg_system_time += sample.system_time;
        avg_memory_usage += sample.memory_usage;
        avg_entity_count += sample.entity_count;
    }
    
    usize sample_count = recent_samples.size();
    avg_frame_time /= sample_count;
    avg_system_time /= sample_count;
    avg_memory_usage /= sample_count;
    avg_entity_count /= sample_count;
    
    ImGui::Text("Performance Metrics (Last %zu frames):", sample_count);
    ImGui::Text("Average Frame Time: %.2f ms (%.1f FPS)", avg_frame_time * 1000.0, 1.0 / avg_frame_time);
    ImGui::Text("Average System Time: %.2f ms", avg_system_time * 1000.0);
    ImGui::Text("Average Memory Usage: %.1f%%", avg_memory_usage);
    ImGui::Text("Average Entity Count: %llu", avg_entity_count);
}

void VisualECSInspector::render_bottleneck_analysis() {
    ImGui::Text("Bottleneck Analysis");
    
    // System bottlenecks
    usize bottleneck_count = std::count_if(system_nodes_.begin(), system_nodes_.end(),
        [](const SystemExecutionNode& node) { return node.is_bottleneck; });
    
    if (bottleneck_count > 0) {
        ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "Found %zu system bottlenecks:", bottleneck_count);
        
        for (const auto& node : system_nodes_) {
            if (node.is_bottleneck) {
                ImGui::BulletText("%s (%.2f ms, %.1f%% over budget)", 
                                 node.system_name.c_str(),
                                 node.average_execution_time,
                                 (node.average_execution_time / node.time_budget - 1.0) * 100.0);
            }
        }
    } else {
        ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.3f, 1.0f), "No system bottlenecks detected");
    }
    
    // Memory pressure
    if (memory_data_) {
        f32 memory_pressure = static_cast<f32>(memory_data_->total_allocated) / static_cast<f32>(memory_data_->peak_allocated);
        
        if (memory_pressure > memory_pressure_threshold_) {
            ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.3f, 1.0f), "High memory pressure detected (%.1f%%)", memory_pressure * 100.0f);
        }
        
        if (memory_data_->fragmentation_ratio > 0.3) {
            ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.3f, 1.0f), "High memory fragmentation (%.1f%%)", memory_data_->fragmentation_ratio * 100.0);
        }
    }
}

void VisualECSInspector::render_statistics_summary() {
    if (!ImGui::CollapsingHeader("Statistics Summary")) return;
    
    ImGui::Columns(3, "StatsColumns", true);
    
    // Archetype statistics
    ImGui::Text("Archetypes");
    ImGui::Separator();
    ImGui::Text("Total: %zu", archetype_nodes_.size());
    
    usize hot_archetypes = std::count_if(archetype_nodes_.begin(), archetype_nodes_.end(),
        [](const ArchetypeNode& node) { return node.is_hot; });
    ImGui::Text("Hot: %zu", hot_archetypes);
    
    usize total_entities = std::accumulate(archetype_nodes_.begin(), archetype_nodes_.end(), 0ULL,
        [](usize sum, const ArchetypeNode& node) { return sum + node.entity_count; });
    ImGui::Text("Total Entities: %zu", total_entities);
    
    ImGui::NextColumn();
    
    // System statistics
    ImGui::Text("Systems");
    ImGui::Separator();
    ImGui::Text("Total: %zu", system_nodes_.size());
    
    usize bottleneck_systems = std::count_if(system_nodes_.begin(), system_nodes_.end(),
        [](const SystemExecutionNode& node) { return node.is_bottleneck; });
    ImGui::Text("Bottlenecks: %zu", bottleneck_systems);
    
    f64 total_system_time = std::accumulate(system_nodes_.begin(), system_nodes_.end(), 0.0,
        [](f64 sum, const SystemExecutionNode& node) { return sum + node.average_execution_time; });
    ImGui::Text("Total Time: %.2f ms", total_system_time);
    
    ImGui::NextColumn();
    
    // Memory statistics
    ImGui::Text("Memory");
    ImGui::Separator();
    if (memory_data_) {
        ImGui::Text("Allocated: %s", format_memory_size(memory_data_->total_allocated).c_str());
        ImGui::Text("Peak: %s", format_memory_size(memory_data_->peak_allocated).c_str());
        ImGui::Text("Fragmentation: %.1f%%", memory_data_->fragmentation_ratio * 100.0);
        ImGui::Text("Cache Hit Rate: %.1f%%", memory_data_->cache_hit_rate * 100.0);
    }
    
    ImGui::Columns(1);
}

void VisualECSInspector::render_educational_tooltips() {
    if (!show_educational_hints_) return;
    
    // Show contextual tooltips based on what's being displayed
    if (ImGui::IsWindowHovered() && ImGui::IsKeyPressed(ImGuiKey_F1)) {
        ImGui::OpenPopup("Educational Help");
    }
    
    if (ImGui::BeginPopup("Educational Help")) {
        ImGui::Text("ECS Educational Concepts");
        ImGui::Separator();
        
        ImGui::BulletText("Archetypes: Groups of entities with identical component signatures");
        ImGui::BulletText("Systems: Logic that operates on entities with specific components");
        ImGui::BulletText("Sparse Sets: Memory-efficient storage for component data");
        ImGui::BulletText("Memory Locality: Keeping related data close in memory for better performance");
        ImGui::BulletText("Cache-Friendly: Data access patterns that minimize cache misses");
        
        ImGui::Separator();
        ImGui::Text("Press F1 while hovering over elements for context-specific help");
        
        ImGui::EndPopup();
    }
}

void VisualECSInspector::handle_graph_interaction() {
    ImGuiIO& io = ImGui::GetIO();
    ImVec2 canvas_pos = ImGui::GetCursorScreenPos();
    ImVec2 canvas_size = ImGui::GetContentRegionAvail();
    
    // Handle mouse input for panning and zooming
    if (ImGui::IsWindowHovered()) {
        // Zoom with mouse wheel
        if (io.MouseWheel != 0) {
            f32 zoom_factor = 1.0f + io.MouseWheel * 0.1f;
            graph_zoom_ = std::clamp(graph_zoom_ * zoom_factor, GRAPH_ZOOM_MIN, GRAPH_ZOOM_MAX);
        }
        
        // Pan with middle mouse button
        if (ImGui::IsMouseDragging(ImGuiMouseButton_Middle)) {
            graph_pan_offset_.x += io.MouseDelta.x / graph_zoom_;
            graph_pan_offset_.y += io.MouseDelta.y / graph_zoom_;
        }
        
        // Node selection with left click
        if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
            ImVec2 mouse_pos = ImVec2(io.MousePos.x - canvas_pos.x, io.MousePos.y - canvas_pos.y);
            
            // Check if clicked on any node
            bool clicked_node = false;
            for (auto& node : archetype_nodes_) {
                ImVec2 node_screen_pos = ImVec2(
                    (node.position.x + graph_pan_offset_.x) * graph_zoom_,
                    (node.position.y + graph_pan_offset_.y) * graph_zoom_
                );
                
                ImVec2 node_size = ImVec2(node.size.x * graph_zoom_, node.size.y * graph_zoom_);
                
                if (point_in_rect(mouse_pos, node_screen_pos, 
                                 ImVec2(node_screen_pos.x + node_size.x, node_screen_pos.y + node_size.y))) {
                    // Clear other selections
                    for (auto& n : archetype_nodes_) {
                        n.is_selected = false;
                    }
                    
                    node.is_selected = true;
                    selected_archetype_id_ = node.archetype_id;
                    clicked_node = true;
                    break;
                }
            }
            
            if (!clicked_node) {
                // Clear selection if clicked on empty space
                for (auto& n : archetype_nodes_) {
                    n.is_selected = false;
                }
                selected_archetype_id_ = 0;
            }
        }
    }
}

bool VisualECSInspector::should_update_data() const noexcept {
    f64 current_time = core::get_time_seconds();
    return (current_time - last_update_time_) >= (1.0 / update_frequency_);
}

void VisualECSInspector::update_archetype_data() noexcept {
    // Get real data from the ECS registry if available
    try {
        auto& registry = ecs::get_registry();
        auto archetype_stats = registry.get_archetype_stats();
        
        archetype_nodes_.clear();
        archetype_nodes_.reserve(archetype_stats.size());
        
        for (usize i = 0; i < archetype_stats.size(); ++i) {
            const auto& [signature, entity_count] = archetype_stats[i];
            
            ArchetypeNode node;
            node.archetype_id = static_cast<u32>(i + 1);
            node.signature_hash = signature.to_string();
            node.entity_count = entity_count;
            node.memory_usage = entity_count * 256; // Estimate based on average component size
            
            // Calculate creation/destruction rates (would need historical data in real implementation)
            node.creation_rate = entity_count * 0.1; // Placeholder
            node.destruction_rate = entity_count * 0.05; // Placeholder
            node.access_frequency = entity_count * 2.0; // Based on entity activity
            
            // Extract component names from signature
            node.component_names.clear();
            for (usize comp_id = 0; comp_id < signature.size() * 64; ++comp_id) {
                if (signature.has_component_id(comp_id)) {
                    // In a real implementation, we'd have a component name registry
                    node.component_names.push_back("Component_" + std::to_string(comp_id));
                }
            }
            
            // Set position in a force-directed layout
            f32 angle = (static_cast<f32>(i) / static_cast<f32>(archetype_stats.size())) * 2.0f * 3.14159f;
            node.position = ImVec2(
                300.0f + 200.0f * std::cos(angle),
                300.0f + 200.0f * std::sin(angle)
            );
            
            node.size = ImVec2(
                std::clamp(ARCHETYPE_NODE_MIN_SIZE + static_cast<f32>(entity_count) * 1.5f, 
                          ARCHETYPE_NODE_MIN_SIZE, ARCHETYPE_NODE_MAX_SIZE),
                80.0f
            );
            
            node.is_hot = node.access_frequency > hot_archetype_threshold_;
            
            // Analyze archetype relationships (component overlap)
            for (usize j = 0; j < archetype_stats.size(); ++j) {
                if (i != j) {
                    const auto& [other_signature, other_count] = archetype_stats[j];
                    usize overlap = signature.overlap_count(other_signature);
                    if (overlap > 0) {
                        node.connected_archetypes.push_back(static_cast<u32>(j + 1));
                        f32 weight = static_cast<f32>(overlap) / static_cast<f32>(signature.count());
                        node.transition_weights.push_back(weight);
                    }
                }
            }
            
            archetype_nodes_.push_back(std::move(node));
        }
    } catch (...) {
        // Fallback to placeholder data if registry is not available
        LOG_WARN("Registry not available, using placeholder archetype data");
        
        archetype_nodes_.clear();
        for (u32 i = 1; i <= 5; ++i) {
            ArchetypeNode node;
            node.archetype_id = i;
            node.signature_hash = "Arch_" + std::to_string(i);
            node.entity_count = 10 + (i * 15);
            node.memory_usage = node.entity_count * 256;
            node.creation_rate = static_cast<f64>(i) * 2.0;
            node.destruction_rate = static_cast<f64>(i) * 0.5;
            node.access_frequency = static_cast<f64>(i) * 5.0;
            
            f32 angle = (static_cast<f32>(i) / 5.0f) * 2.0f * 3.14159f;
            node.position = ImVec2(
                200.0f + 150.0f * std::cos(angle),
                200.0f + 150.0f * std::sin(angle)
            );
            
            node.size = ImVec2(
                std::clamp(ARCHETYPE_NODE_MIN_SIZE + node.entity_count * 2.0f, 
                          ARCHETYPE_NODE_MIN_SIZE, ARCHETYPE_NODE_MAX_SIZE),
                60.0f
            );
            
            node.is_hot = node.access_frequency > hot_archetype_threshold_;
            
            if (i > 1) {
                node.connected_archetypes.push_back(i - 1);
                node.transition_weights.push_back(0.5f + (i * 0.1f));
            }
            
            node.component_names = {"Transform", "Renderer", "Physics"};
            archetype_nodes_.push_back(std::move(node));
        }
    }
}

void VisualECSInspector::update_system_data() noexcept {
    // Generate example system data
    system_nodes_.clear();
    
    static const std::array<const char*, 6> system_names = {
        "Transform System", "Render System", "Physics System", 
        "Audio System", "AI System", "Cleanup System"
    };
    
    for (usize i = 0; i < system_names.size(); ++i) {
        SystemExecutionNode node;
        node.system_name = system_names[i];
        node.phase = static_cast<ecs::SystemPhase>(i % 3);
        node.average_execution_time = 2.0 + (i * 3.0); // ms
        node.last_execution_time = node.average_execution_time * (0.8 + 0.4 * (i % 3));
        node.time_budget = 16.6; // 60 FPS budget
        node.budget_utilization = (node.average_execution_time / node.time_budget) * 100.0;
        node.is_over_budget = node.average_execution_time > node.time_budget;
        node.is_bottleneck = node.is_over_budget || node.budget_utilization > 80.0;
        
        // Add some dependencies
        if (i > 0) {
            node.dependencies.push_back(system_names[i - 1]);
        }
        if (i < system_names.size() - 1) {
            node.dependents.push_back(system_names[i + 1]);
        }
        
        system_nodes_.push_back(std::move(node));
    }
}

void VisualECSInspector::update_memory_data() noexcept {
    if (!memory_data_) return;
    
    try {
        // Get real memory data from the memory tracker
        auto& tracker = memory::MemoryTracker::instance();
        auto global_stats = tracker.get_global_stats();
        auto active_allocations = tracker.get_active_allocations();
        
        memory_data_->blocks.clear();
        memory_data_->blocks.reserve(std::min(active_allocations.size(), usize(100))); // Limit for visualization
        
        usize displayed_count = 0;
        for (const auto& alloc_info : active_allocations) {
            if (displayed_count >= 100) break; // Limit for performance
            
            MemoryVisualizationData::AllocationBlock block;
            block.address = alloc_info.address;
            block.size = alloc_info.size;
            block.category = alloc_info.category;
            block.age = core::get_time_seconds() - alloc_info.allocation_time;
            block.is_active = alloc_info.is_active;
            block.is_hot = alloc_info.is_hot;
            
            // Set visual properties based on allocation data
            block.position = ImVec2(
                static_cast<f32>((displayed_count % 20) * 25),
                static_cast<f32>((displayed_count / 20) * 20)
            );
            
            // Color based on category
            switch (alloc_info.category) {
                case memory::AllocationCategory::ECS_Core:
                    block.color = IM_COL32(100, 200, 100, 200);
                    break;
                case memory::AllocationCategory::ECS_Components:
                    block.color = IM_COL32(100, 100, 200, 200);
                    break;
                case memory::AllocationCategory::Renderer_Meshes:
                    block.color = IM_COL32(200, 100, 100, 200);
                    break;
                default:
                    block.color = IM_COL32(150, 150, 150, 200);
                    break;
            }
            
            memory_data_->blocks.push_back(std::move(block));
            ++displayed_count;
        }
        
        // Update summary statistics
        memory_data_->total_allocated = global_stats.total_allocated;
        memory_data_->peak_allocated = global_stats.peak_allocated;
        memory_data_->fragmentation_ratio = global_stats.fragmentation_ratio;
        memory_data_->cache_hit_rate = 1.0 - (static_cast<f64>(global_stats.cache_miss_estimate) / 
                                              std::max(global_stats.total_allocations_ever, u64(1)));
        
    } catch (...) {
        // Fallback to placeholder data if memory tracker is not available
        LOG_WARN("Memory tracker not available, using placeholder memory data");
        
        memory_data_->blocks.clear();
        usize total_allocated = 0;
        for (usize i = 0; i < 20; ++i) {
            MemoryVisualizationData::AllocationBlock block;
            block.address = reinterpret_cast<void*>(0x10000000 + i * 1024);
            block.size = 256 + (i * 128);
            block.category = static_cast<memory::AllocationCategory>(i % 5);
            block.age = static_cast<f64>(i) * 0.5;
            block.is_active = (i % 10) != 9;
            block.is_hot = (i % 3) == 0;
            
            total_allocated += block.size;
            memory_data_->blocks.push_back(std::move(block));
        }
        
        memory_data_->total_allocated = total_allocated;
        memory_data_->peak_allocated = total_allocated * 1.2;
        memory_data_->fragmentation_ratio = 0.15;
        memory_data_->cache_hit_rate = 0.92;
    }
}

void VisualECSInspector::update_entity_browser_data() noexcept {
    if (!entity_browser_) return;
    
    // Generate example entity data
    entity_browser_->entities.clear();
    
    for (u32 i = 1; i <= 50; ++i) {
        EntityBrowserData::EntityEntry entry;
        entry.entity = i;
        entry.archetype_name = "Archetype_" + std::to_string((i % 5) + 1);
        
        // Add some example components
        entry.components.push_back("Transform");
        if (i % 2 == 0) entry.components.push_back("Renderer");
        if (i % 3 == 0) entry.components.push_back("Physics");
        if (i % 5 == 0) entry.components.push_back("Audio");
        
        entry.last_modified = core::get_time_seconds() - (i * 0.1);
        entry.matches_filter = true; // Apply filtering logic here
        
        entity_browser_->entities.push_back(std::move(entry));
    }
}

void VisualECSInspector::update_sparse_set_data() noexcept {
    if (!sparse_set_data_) return;
    
    try {
        // Get real sparse set data from the ECS registry
        auto& registry = ecs::get_registry();
        auto archetype_stats = registry.get_archetype_stats();
        
        sparse_set_data_->pools.clear();
        
        // Collect component pool information
        std::unordered_map<std::string, SparseSetVisualization::ComponentPool> pools_map;
        
        for (const auto& [signature, entity_count] : archetype_stats) {
            // For each component in the signature
            for (usize comp_id = 0; comp_id < signature.size() * 64; ++comp_id) {
                if (signature.has_component_id(comp_id)) {
                    std::string comp_name = "Component_" + std::to_string(comp_id);
                    
                    if (pools_map.find(comp_name) == pools_map.end()) {
                        SparseSetVisualization::ComponentPool pool;
                        pool.component_name = comp_name;
                        pool.dense_count = 0;
                        pool.sparse_size = 1024; // Default sparse array size
                        pool.capacity = 1024;
                        pools_map[comp_name] = pool;
                    }
                    
                    pools_map[comp_name].dense_count += entity_count;
                }
            }
        }
        
        // Convert map to vector and calculate metrics
        f64 total_efficiency = 0.0;
        f64 total_locality = 0.0;
        
        for (auto& [name, pool] : pools_map) {
            pool.utilization = static_cast<f64>(pool.dense_count) / pool.capacity;
            
            // Estimate access pattern score based on utilization and density
            pool.access_pattern_score = std::min(1.0, pool.utilization * 1.2);
            
            // Generate visualization data for dense/sparse arrays
            pool.dense_occupied.clear();
            pool.sparse_valid.clear();
            
            usize viz_size = std::min(pool.dense_count, usize(100)); // Limit for visualization
            pool.dense_occupied.resize(viz_size, true);
            pool.sparse_valid.resize(pool.sparse_size, false);
            
            // Mark valid sparse entries based on dense count
            for (usize i = 0; i < pool.dense_count && i < pool.sparse_size; ++i) {
                pool.sparse_valid[i] = true;
            }
            
            total_efficiency += pool.utilization;
            total_locality += pool.access_pattern_score;
            
            sparse_set_data_->pools.push_back(std::move(pool));
        }
        
        // Calculate overall metrics
        if (!sparse_set_data_->pools.empty()) {
            sparse_set_data_->overall_memory_efficiency = total_efficiency / sparse_set_data_->pools.size();
            sparse_set_data_->cache_locality_score = total_locality / sparse_set_data_->pools.size();
        } else {
            sparse_set_data_->overall_memory_efficiency = 0.0;
            sparse_set_data_->cache_locality_score = 0.0;
        }
        
    } catch (...) {
        // Fallback to placeholder data
        LOG_WARN("Registry not available, using placeholder sparse set data");
        
        sparse_set_data_->pools.clear();
        
        static const std::array<const char*, 4> component_names = {
            "Transform", "Renderer", "Physics", "Audio"
        };
        
        for (usize i = 0; i < component_names.size(); ++i) {
            SparseSetVisualization::ComponentPool pool;
            pool.component_name = component_names[i];
            pool.dense_count = 30 + (i * 10);
            pool.sparse_size = 100;
            pool.capacity = 150;
            pool.utilization = static_cast<f64>(pool.dense_count) / pool.capacity;
            pool.access_pattern_score = 0.8 + (i * 0.05);
            
            sparse_set_data_->pools.push_back(std::move(pool));
        }
        
        sparse_set_data_->overall_memory_efficiency = 0.85;
        sparse_set_data_->cache_locality_score = 0.9;
    }
}

void VisualECSInspector::update_performance_timeline() noexcept {
    if (!performance_timeline_) return;
    
    f64 current_time = core::get_time_seconds();
    
    if (current_time - performance_timeline_->last_sample_time >= performance_timeline_->sample_interval) {
        PerformanceTimeline::TimelineSample sample;
        sample.timestamp = current_time;
        sample.frame_time = 0.016 + (std::sin(current_time * 2.0) * 0.005); // Simulate varying frame time
        sample.system_time = sample.frame_time * 0.7;
        sample.memory_usage = 60.0 + (std::sin(current_time * 0.5) * 10.0);
        sample.entity_count = 100 + static_cast<u64>(std::sin(current_time * 0.1) * 20.0);
        sample.archetype_count = 5;
        
        performance_timeline_->add_sample(sample);
        performance_timeline_->last_sample_time = current_time;
    }
}

void VisualECSInspector::layout_archetype_nodes_force_directed() noexcept {
    if (archetype_nodes_.empty()) return;
    
    const f32 spring_strength = 0.01f;
    const f32 repulsion_strength = 5000.0f;
    const f32 damping = 0.9f;
    const f32 max_force = 10.0f;
    
    for (auto& node : archetype_nodes_) {
        ImVec2 force = ImVec2(0, 0);
        
        // Repulsion from other nodes
        for (const auto& other : archetype_nodes_) {
            if (&node == &other) continue;
            
            ImVec2 diff = ImVec2(node.position.x - other.position.x, node.position.y - other.position.y);
            f32 dist = std::sqrt(diff.x * diff.x + diff.y * diff.y);
            
            if (dist > 0 && dist < 300.0f) {
                f32 repulsion = repulsion_strength / (dist * dist);
                force.x += (diff.x / dist) * repulsion;
                force.y += (diff.y / dist) * repulsion;
            }
        }
        
        // Spring force from connections
        for (u32 connected_id : node.connected_archetypes) {
            auto connected_it = std::find_if(archetype_nodes_.begin(), archetype_nodes_.end(),
                [connected_id](const ArchetypeNode& n) { return n.archetype_id == connected_id; });
            
            if (connected_it != archetype_nodes_.end()) {
                ImVec2 diff = ImVec2(connected_it->position.x - node.position.x, 
                                    connected_it->position.y - node.position.y);
                f32 dist = std::sqrt(diff.x * diff.x + diff.y * diff.y);
                
                if (dist > 0) {
                    f32 spring_force = spring_strength * (dist - 150.0f); // Target distance: 150px
                    force.x += (diff.x / dist) * spring_force;
                    force.y += (diff.y / dist) * spring_force;
                }
            }
        }
        
        // Clamp force
        f32 force_mag = std::sqrt(force.x * force.x + force.y * force.y);
        if (force_mag > max_force) {
            force.x = (force.x / force_mag) * max_force;
            force.y = (force.y / force_mag) * max_force;
        }
        
        // Apply force
        if (!node.is_selected) { // Don't move selected nodes
            node.position.x += force.x * damping;
            node.position.y += force.y * damping;
        }
    }
}

ImU32 VisualECSInspector::calculate_archetype_node_color(const ArchetypeNode& node) const noexcept {
    if (node.is_selected) {
        return COLOR_ARCHETYPE_SELECTED;
    } else if (node.is_hot) {
        return COLOR_ARCHETYPE_HOT;
    } else {
        // Blend color based on entity count
        f32 intensity = std::clamp(static_cast<f32>(node.entity_count) / 100.0f, 0.0f, 1.0f);
        return heat_map_color(intensity);
    }
}

ImU32 VisualECSInspector::calculate_system_node_color(const SystemExecutionNode& node) const noexcept {
    if (node.is_over_budget) {
        return COLOR_SYSTEM_OVER_BUDGET;
    } else if (node.is_bottleneck) {
        return COLOR_SYSTEM_BOTTLENECK;
    } else {
        return COLOR_SYSTEM_NORMAL;
    }
}

std::string VisualECSInspector::format_memory_size(usize bytes) const noexcept {
    static const std::array<const char*, 5> units = {"B", "KB", "MB", "GB", "TB"};
    
    f64 size = static_cast<f64>(bytes);
    usize unit_index = 0;
    
    while (size >= 1024.0 && unit_index < units.size() - 1) {
        size /= 1024.0;
        ++unit_index;
    }
    
    std::ostringstream oss;
    oss.precision(2);
    oss << std::fixed << size << " " << units[unit_index];
    return oss.str();
}

std::string VisualECSInspector::format_time_duration(f64 seconds) const noexcept {
    if (seconds < 0.001) {
        return std::to_string(static_cast<int>(seconds * 1000000.0)) + " μs";
    } else if (seconds < 1.0) {
        return std::to_string(static_cast<int>(seconds * 1000.0)) + " ms";
    } else {
        return std::to_string(seconds) + " s";
    }
}

std::string VisualECSInspector::format_time_duration_us(f64 microseconds) const noexcept {
    if (microseconds < 1000.0) {
        return std::to_string(static_cast<int>(microseconds)) + " μs";
    } else if (microseconds < 1000000.0) {
        return std::to_string(static_cast<int>(microseconds / 1000.0)) + " ms";
    } else {
        return std::to_string(static_cast<int>(microseconds / 1000000.0)) + " s";
    }
}

std::string VisualECSInspector::format_percentage(f64 value) const noexcept {
    std::ostringstream oss;
    oss.precision(1);
    oss << std::fixed << (value * 100.0) << "%";
    return oss.str();
}

ImU32 VisualECSInspector::heat_map_color(f32 intensity) const noexcept {
    intensity = std::clamp(intensity, 0.0f, 1.0f);
    
    // Blue to red heat map
    if (intensity < 0.5f) {
        // Blue to green
        f32 t = intensity * 2.0f;
        u8 r = 0;
        u8 g = static_cast<u8>(255 * t);
        u8 b = static_cast<u8>(255 * (1.0f - t));
        return IM_COL32(r, g, b, 200);
    } else {
        // Green to red
        f32 t = (intensity - 0.5f) * 2.0f;
        u8 r = static_cast<u8>(255 * t);
        u8 g = static_cast<u8>(255 * (1.0f - t));
        u8 b = 0;
        return IM_COL32(r, g, b, 200);
    }
}

void VisualECSInspector::initialize_color_scheme() noexcept {
    current_color_scheme_.archetype_default = COLOR_ARCHETYPE_DEFAULT;
    current_color_scheme_.archetype_hot = COLOR_ARCHETYPE_HOT;
    current_color_scheme_.archetype_selected = COLOR_ARCHETYPE_SELECTED;
    current_color_scheme_.system_normal = COLOR_SYSTEM_NORMAL;
    current_color_scheme_.system_bottleneck = COLOR_SYSTEM_BOTTLENECK;
    current_color_scheme_.system_over_budget = COLOR_SYSTEM_OVER_BUDGET;
    current_color_scheme_.memory_low = IM_COL32(100, 255, 100, 200);
    current_color_scheme_.memory_medium = IM_COL32(255, 255, 100, 200);
    current_color_scheme_.memory_high = IM_COL32(255, 150, 100, 200);
    current_color_scheme_.memory_critical = IM_COL32(255, 100, 100, 200);
}

void VisualECSInspector::initialize_educational_content() noexcept {
    educational_tooltips_["archetype"] = "An archetype represents a unique combination of components. "
                                        "Entities with the same components belong to the same archetype, "
                                        "enabling efficient memory layout and system processing.";
    
    educational_tooltips_["sparse_set"] = "Sparse sets provide O(1) insertion, deletion, and lookup "
                                         "while maintaining dense arrays for cache-friendly iteration.";
    
    educational_tooltips_["cache_locality"] = "Cache locality refers to how memory access patterns "
                                             "affect CPU cache performance. Sequential access patterns "
                                             "are more cache-friendly than random access.";
    
    educational_tooltips_["system_dependency"] = "System dependencies define execution order. "
                                                "Systems with dependencies must wait for their "
                                                "dependencies to complete before executing.";
}

// Export functions
void VisualECSInspector::export_archetype_data(const std::string& filename) const noexcept {
    std::ofstream file(filename);
    if (!file.is_open()) {
        LOG_ERROR("Failed to open file for archetype export: " + filename);
        return;
    }
    
    file << "{\n";
    file << "  \"archetypes\": [\n";
    
    for (usize i = 0; i < archetype_nodes_.size(); ++i) {
        const auto& node = archetype_nodes_[i];
        file << "    {\n";
        file << "      \"id\": " << node.archetype_id << ",\n";
        file << "      \"signature\": \"" << node.signature_hash << "\",\n";
        file << "      \"entity_count\": " << node.entity_count << ",\n";
        file << "      \"memory_usage\": " << node.memory_usage << ",\n";
        file << "      \"creation_rate\": " << node.creation_rate << ",\n";
        file << "      \"destruction_rate\": " << node.destruction_rate << ",\n";
        file << "      \"access_frequency\": " << node.access_frequency << ",\n";
        file << "      \"is_hot\": " << (node.is_hot ? "true" : "false") << "\n";
        file << "    }";
        if (i < archetype_nodes_.size() - 1) file << ",";
        file << "\n";
    }
    
    file << "  ]\n";
    file << "}\n";
    
    LOG_INFO("Archetype data exported to: " + filename);
}

void VisualECSInspector::export_system_performance(const std::string& filename) const noexcept {
    std::ofstream file(filename);
    if (!file.is_open()) {
        LOG_ERROR("Failed to open file for system performance export: " + filename);
        return;
    }
    
    file << "system_name,phase,avg_execution_time_ms,time_budget_ms,budget_utilization,is_bottleneck\n";
    
    for (const auto& node : system_nodes_) {
        file << node.system_name << ","
             << static_cast<int>(node.phase) << ","
             << node.average_execution_time << ","
             << node.time_budget << ","
             << node.budget_utilization << ","
             << (node.is_bottleneck ? "true" : "false") << "\n";
    }
    
    LOG_INFO("System performance data exported to: " + filename);
}

void VisualECSInspector::export_memory_analysis(const std::string& filename) const noexcept {
    if (!memory_data_) return;
    
    std::ofstream file(filename);
    if (!file.is_open()) {
        LOG_ERROR("Failed to open file for memory analysis export: " + filename);
        return;
    }
    
    file << "{\n";
    file << "  \"summary\": {\n";
    file << "    \"total_allocated\": " << memory_data_->total_allocated << ",\n";
    file << "    \"peak_allocated\": " << memory_data_->peak_allocated << ",\n";
    file << "    \"fragmentation_ratio\": " << memory_data_->fragmentation_ratio << ",\n";
    file << "    \"cache_hit_rate\": " << memory_data_->cache_hit_rate << "\n";
    file << "  },\n";
    file << "  \"allocations\": [\n";
    
    for (usize i = 0; i < memory_data_->blocks.size(); ++i) {
        const auto& block = memory_data_->blocks[i];
        file << "    {\n";
        file << "      \"address\": \"" << block.address << "\",\n";
        file << "      \"size\": " << block.size << ",\n";
        file << "      \"category\": " << static_cast<int>(block.category) << ",\n";
        file << "      \"age\": " << block.age << ",\n";
        file << "      \"is_active\": " << (block.is_active ? "true" : "false") << ",\n";
        file << "      \"is_hot\": " << (block.is_hot ? "true" : "false") << "\n";
        file << "    }";
        if (i < memory_data_->blocks.size() - 1) file << ",";
        file << "\n";
    }
    
    file << "  ]\n";
    file << "}\n";
    
    LOG_INFO("Memory analysis data exported to: " + filename);
}

void VisualECSInspector::export_performance_timeline(const std::string& filename) const noexcept {
    if (!performance_timeline_) return;
    
    std::ofstream file(filename);
    if (!file.is_open()) {
        LOG_ERROR("Failed to open file for performance timeline export: " + filename);
        return;
    }
    
    file << "timestamp,frame_time_ms,system_time_ms,memory_usage_percent,entity_count,archetype_count\n";
    
    auto samples = performance_timeline_->get_recent_samples(500);
    for (const auto& sample : samples) {
        file << sample.timestamp << ","
             << (sample.frame_time * 1000.0) << ","
             << (sample.system_time * 1000.0) << ","
             << sample.memory_usage << ","
             << sample.entity_count << ","
             << sample.archetype_count << "\n";
    }
    
    LOG_INFO("Performance timeline data exported to: " + filename);
}

// Factory function
std::unique_ptr<VisualECSInspector> create_visual_ecs_inspector() {
    return std::make_unique<VisualECSInspector>();
}

// Integration functions
namespace visual_inspector_integration {
    
void register_inspector(ui::Overlay& overlay) {
    auto inspector = create_visual_ecs_inspector();
    inspector->show_archetype_graph(true);
    inspector->show_system_profiler(true);
    inspector->show_memory_visualizer(true);
    inspector->show_entity_browser(true);
    inspector->show_performance_timeline(true);
    
    overlay.add_panel(std::move(inspector));
    
    LOG_INFO("Visual ECS Inspector registered with UI overlay");
}

void update_from_registry(VisualECSInspector& inspector, const ecs::Registry& registry) {
    // Update archetype nodes from registry data
    auto archetype_stats = registry.get_archetype_stats();
    auto all_entities = registry.get_all_entities();
    
    // Update entity browser data
    if (inspector.entity_browser_) {
        inspector.entity_browser_->entities.clear();
        inspector.entity_browser_->entities.reserve(all_entities.size());
        
        for (const auto& entity : all_entities) {
            EntityBrowserData::EntityEntry entry;
            entry.entity = entity;
            
            // Find which archetype this entity belongs to
            for (usize i = 0; i < archetype_stats.size(); ++i) {
                const auto& [signature, count] = archetype_stats[i];
                // In a real implementation, we'd check if entity belongs to this archetype
                entry.archetype_name = "Archetype_" + std::to_string(i + 1);
                break;
            }
            
            entry.last_modified = core::get_time_seconds() - (entity * 0.1); // Placeholder
            entry.matches_filter = true;
            
            inspector.entity_browser_->entities.push_back(std::move(entry));
        }
    }
    
    LOG_DEBUG("Updated Visual ECS Inspector from registry with " + std::to_string(archetype_stats.size()) + 
              " archetypes and " + std::to_string(all_entities.size()) + " entities");
}

void update_from_system_manager(VisualECSInspector& inspector, const ecs::SystemManager& system_manager) {
    // Update system execution nodes from system manager
    auto all_system_stats = system_manager.get_all_system_stats();
    
    inspector.system_nodes_.clear();
    inspector.system_nodes_.reserve(all_system_stats.size());
    
    for (const auto& [system_name, stats] : all_system_stats) {
        SystemExecutionNode node;
        node.system_name = system_name;
        
        // Get system phase (would need API enhancement)
        node.phase = ecs::SystemPhase::Update; // Default
        
        // Map system stats to node data
        node.average_execution_time = stats.average_execution_time * 1000.0; // Convert to ms
        node.last_execution_time = stats.last_execution_time * 1000.0;
        node.time_budget = stats.allocated_time_budget * 1000.0;
        node.budget_utilization = stats.budget_utilization * 100.0;
        node.is_over_budget = stats.exceeded_budget;
        node.is_bottleneck = stats.exceeded_budget || node.budget_utilization > 80.0;
        
        // Position for visualization (simple layout)
        node.position = ImVec2(
            100.0f + (inspector.system_nodes_.size() % 5) * 120.0f,
            100.0f + (inspector.system_nodes_.size() / 5) * 80.0f
        );
        
        // Color based on performance
        if (node.is_over_budget) {
            node.color = IM_COL32(255, 100, 100, 200); // Red for over budget
        } else if (node.is_bottleneck) {
            node.color = IM_COL32(255, 200, 100, 200); // Orange for bottleneck
        } else {
            node.color = IM_COL32(100, 255, 100, 200); // Green for normal
        }
        
        inspector.system_nodes_.push_back(std::move(node));
    }
    
    LOG_DEBUG("Updated Visual ECS Inspector from system manager with " + std::to_string(all_system_stats.size()) + " systems");
}

void update_from_memory_tracker(VisualECSInspector& inspector, const memory::MemoryTracker& tracker) {
    if (!inspector.memory_data_) return;
    
    // Get comprehensive memory statistics
    auto global_stats = tracker.get_global_stats();
    auto category_stats = tracker.get_all_category_stats();
    auto active_allocations = tracker.get_active_allocations();
    auto memory_pressure = tracker.get_memory_pressure();
    auto timeline = tracker.get_allocation_timeline();
    
    // Update memory visualization blocks
    inspector.memory_data_->blocks.clear();
    inspector.memory_data_->blocks.reserve(std::min(active_allocations.size(), usize(200)));
    
    f64 current_time = core::get_time_seconds();
    usize block_count = 0;
    
    for (const auto& alloc : active_allocations) {
        if (block_count >= 200) break; // Limit for performance
        
        MemoryVisualizationData::AllocationBlock block;
        block.address = alloc.address;
        block.size = alloc.size;
        block.category = alloc.category;
        block.age = current_time - alloc.allocation_time;
        block.is_active = alloc.is_active;
        block.is_hot = alloc.is_hot;
        
        // Calculate visual position based on address (simplified memory map)
        uintptr_t addr_val = reinterpret_cast<uintptr_t>(alloc.address);
        block.position = ImVec2(
            static_cast<f32>((addr_val % 1000) / 10),
            static_cast<f32>((addr_val / 1000) % 100)
        );
        
        // Color based on category and age
        ImU32 base_color;
        switch (alloc.category) {
            case memory::AllocationCategory::ECS_Core:
                base_color = IM_COL32(100, 255, 100, 200);
                break;
            case memory::AllocationCategory::ECS_Components:
                base_color = IM_COL32(100, 100, 255, 200);
                break;
            case memory::AllocationCategory::Renderer_Meshes:
                base_color = IM_COL32(255, 100, 100, 200);
                break;
            case memory::AllocationCategory::Physics_Bodies:
                base_color = IM_COL32(255, 255, 100, 200);
                break;
            default:
                base_color = IM_COL32(150, 150, 150, 200);
                break;
        }
        
        // Modify color based on age (older = darker)
        f32 age_factor = std::clamp(static_cast<f32>(block.age / 10.0), 0.0f, 1.0f);
        f32 brightness = 1.0f - (age_factor * 0.5f);
        
        block.color = IM_COL32(
            static_cast<u8>((base_color & 0xFF) * brightness),
            static_cast<u8>(((base_color >> 8) & 0xFF) * brightness),
            static_cast<u8>(((base_color >> 16) & 0xFF) * brightness),
            (base_color >> 24) & 0xFF
        );
        
        inspector.memory_data_->blocks.push_back(std::move(block));
        ++block_count;
    }
    
    // Update summary statistics
    inspector.memory_data_->total_allocated = global_stats.total_allocated;
    inspector.memory_data_->peak_allocated = global_stats.peak_allocated;
    inspector.memory_data_->fragmentation_ratio = global_stats.fragmentation_ratio;
    inspector.memory_data_->cache_hit_rate = 1.0 - (static_cast<f64>(global_stats.cache_miss_estimate) / 
                                                    std::max(global_stats.total_allocations_ever, u64(1)));
    
    LOG_DEBUG("Updated Visual ECS Inspector memory data: " + std::to_string(active_allocations.size()) + 
              " allocations, " + std::to_string(global_stats.total_allocated / (1024.0 * 1024.0)) + " MB total, " +
              std::to_string(global_stats.fragmentation_ratio * 100.0) + "% fragmentation");
}

void enable_automatic_updates(VisualECSInspector& inspector, f64 frequency) {
    inspector.set_update_frequency(frequency);
    LOG_INFO("Enabled automatic updates for Visual ECS Inspector at " + std::to_string(frequency) + " Hz");
}

void disable_automatic_updates(VisualECSInspector& inspector) {
    inspector.set_update_frequency(0.0);
    LOG_INFO("Disabled automatic updates for Visual ECS Inspector");
}

} // namespace visual_inspector_integration

} // namespace ecscope::ui