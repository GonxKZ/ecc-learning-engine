#include "panel_visual_debugger.hpp"
#include "core/log.hpp"
#include "../imgui_utils.hpp"
#include <imgui.h>
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <cmath>

namespace ecscope::ui {

// VisualDebuggerPanel Implementation

VisualDebuggerPanel::VisualDebuggerPanel(std::shared_ptr<ecs::Registry> registry)
    : Panel("Visual ECS Debugger", true), registry_(registry) {
    
    // Initialize timeline
    timeline_start_time_ = std::chrono::duration<f64>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
    
    // Initialize performance metrics history
    performance_.frame_time_history.reserve(PerformanceMetrics::HISTORY_SIZE);
    performance_.entity_count_history.reserve(PerformanceMetrics::HISTORY_SIZE);
    performance_.memory_usage_history.reserve(PerformanceMetrics::HISTORY_SIZE);
    
    // Setup default breakpoints for educational purposes
    Breakpoint entity_creation_bp;
    entity_creation_bp.operation_type = ECSOperationType::EntityCreated;
    entity_creation_bp.enabled = false; // Start disabled
    entity_creation_bp.custom_message = "Entity created - observe memory allocation";
    add_breakpoint(entity_creation_bp);
    
    LOG_INFO("Visual ECS Debugger Panel initialized");
}

void VisualDebuggerPanel::render() {
    if (!visible_) return;
    
    ImGui::SetNextWindowSize(ImVec2(1000, 700), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSizeConstraints(ImVec2(MIN_PANEL_WIDTH, MIN_PANEL_HEIGHT), ImVec2(FLT_MAX, FLT_MAX));
    
    if (ImGui::Begin(name_.c_str(), &visible_, ImGuiWindowFlags_MenuBar)) {
        
        // Menu bar
        if (ImGui::BeginMenuBar()) {
            if (ImGui::BeginMenu("Debug")) {
                bool is_running = (current_state_ == DebuggerState::Running);
                bool is_paused = (current_state_ == DebuggerState::Paused);
                
                if (ImGui::MenuItem("Start", "F5", false, !is_running)) {
                    start_debugging();
                }
                if (ImGui::MenuItem("Pause", "F6", false, is_running)) {
                    pause_execution();
                }
                if (ImGui::MenuItem("Resume", "F7", false, is_paused)) {
                    resume_execution();
                }
                if (ImGui::MenuItem("Stop", "Shift+F5", false, is_running || is_paused)) {
                    stop_debugging();
                }
                
                ImGui::Separator();
                
                if (ImGui::MenuItem("Step Operation", "F10", false, is_paused)) {
                    step_single_operation();
                }
                if (ImGui::MenuItem("Step Frame", "F11", false, is_paused)) {
                    step_single_frame();
                }
                
                ImGui::Separator();
                
                ImGui::MenuItem("Recording", nullptr, &recording_enabled_);
                if (ImGui::MenuItem("Clear Recording", nullptr, false, !recorded_operations_.empty())) {
                    clear_recording();
                }
                
                ImGui::EndMenu();
            }
            
            if (ImGui::BeginMenu("View")) {
                if (ImGui::MenuItem("Main View", nullptr, active_panel_ == DebuggerPanel::MainView)) {
                    active_panel_ = DebuggerPanel::MainView;
                }
                if (ImGui::MenuItem("Timeline", nullptr, active_panel_ == DebuggerPanel::Timeline)) {
                    active_panel_ = DebuggerPanel::Timeline;
                }
                if (ImGui::MenuItem("Breakpoints", nullptr, active_panel_ == DebuggerPanel::Breakpoints)) {
                    active_panel_ = DebuggerPanel::Breakpoints;
                }
                if (ImGui::MenuItem("Entity Inspector", nullptr, active_panel_ == DebuggerPanel::EntityInspector)) {
                    active_panel_ = DebuggerPanel::EntityInspector;
                }
                if (ImGui::MenuItem("System Profiler", nullptr, active_panel_ == DebuggerPanel::SystemProfiler)) {
                    active_panel_ = DebuggerPanel::SystemProfiler;
                }
                if (ImGui::MenuItem("Memory Analyzer", nullptr, active_panel_ == DebuggerPanel::MemoryAnalyzer)) {
                    active_panel_ = DebuggerPanel::MemoryAnalyzer;
                }
                
                ImGui::Separator();
                ImGui::MenuItem("Show Side Panel", nullptr, &show_side_panel_);
                
                ImGui::EndMenu();
            }
            
            if (ImGui::BeginMenu("Visualization")) {
                ImGui::MenuItem("Show Entity IDs", nullptr, &viz_settings_.show_entity_ids);
                ImGui::MenuItem("Show Components", nullptr, &viz_settings_.show_component_types);
                ImGui::MenuItem("Show Connections", nullptr, &viz_settings_.show_archetype_connections);
                ImGui::MenuItem("Animate Operations", nullptr, &viz_settings_.animate_operations);
                
                ImGui::Separator();
                
                ImGui::SliderFloat("Entity Size", &viz_settings_.entity_size, 10.0f, 50.0f);
                ImGui::SliderFloat("Animation Speed", &viz_settings_.animation_speed, 0.5f, 3.0f);
                
                ImGui::Separator();
                
                ImGui::MenuItem("Performance Overlay", nullptr, &viz_settings_.show_performance_overlay);
                ImGui::MenuItem("Memory Usage", nullptr, &viz_settings_.show_memory_usage);
                ImGui::MenuItem("Frame Time Graph", nullptr, &viz_settings_.show_frame_time_graph);
                
                ImGui::EndMenu();
            }
            
            ImGui::EndMenuBar();
        }
        
        // Status bar at top
        render_control_toolbar();
        
        // Main content area
        if (show_side_panel_) {
            ImGui::Columns(2, "##debugger_layout", true);
            ImGui::SetColumnWidth(0, ImGui::GetWindowWidth() - side_panel_width_);
        }
        
        // Main panel content
        switch (active_panel_) {
            case DebuggerPanel::MainView:
                render_main_view();
                break;
            case DebuggerPanel::Timeline:
                render_timeline_panel();
                break;
            case DebuggerPanel::Breakpoints:
                render_breakpoints_panel();
                break;
            case DebuggerPanel::EntityInspector:
                render_entity_inspector_panel();
                break;
            case DebuggerPanel::SystemProfiler:
                render_system_profiler_panel();
                break;
            case DebuggerPanel::MemoryAnalyzer:
                render_memory_analyzer_panel();
                break;
            case DebuggerPanel::Settings:
                render_settings_panel();
                break;
        }
        
        if (show_side_panel_) {
            ImGui::NextColumn();
            
            // Side panel content - context-sensitive
            ImGui::Text("🔍 Debug Information");
            ImGui::Separator();
            
            // Current operation info
            if (current_state_ == DebuggerState::Breakpoint) {
                ImGui::Text("⏸️ Breakpoint Hit");
                ImGui::Text("Operation: %s", format_operation_description(current_breakpoint_operation_).c_str());
                
                if (current_breakpoint_operation_.target_entity != ecs::NULL_ENTITY) {
                    ImGui::Text("Entity: %s", format_entity_info(current_breakpoint_operation_.target_entity).c_str());
                }
            }
            
            // Selected entity info
            if (selected_entity_ != ecs::NULL_ENTITY && registry_) {
                ImGui::Text("📋 Selected Entity");
                ImGui::Text("ID: %s", format_entity_info(selected_entity_).c_str());
                
                // Component list - placeholder
                ImGui::Text("Components: (placeholder)");
                ImGui::BulletText("Transform");
                ImGui::BulletText("RigidBody");
            }
            
            // Performance summary
            ImGui::Text("📊 Performance");
            ImGui::Text("Frame Time: %.2f ms", performance_.frame_time_ms);
            ImGui::Text("Entities: %zu", performance_.entities_processed_per_frame);
            ImGui::Text("Memory: %.1f MB", performance_.memory_usage_mb);
            
            // Quick actions
            ImGui::Separator();
            ImGui::Text("⚡ Quick Actions");
            
            if (ImGui::Button("Clear History")) {
                clear_recording();
            }
            
            if (ImGui::Button("Export Session")) {
                export_debug_session("debug_session.json");
            }
            
            if (ImGui::Button("Reset View")) {
                timeline_position_ = 1.0f;
                timeline_zoom_ = 1.0f;
            }
            
            ImGui::Columns(1);
        }
        
        // Process pending actions
        while (!pending_debug_actions_.empty()) {
            pending_debug_actions_.front()();
            pending_debug_actions_.pop();
        }
    }
    ImGui::End();
}

void VisualDebuggerPanel::update(f64 delta_time) {
    if (!visible_) return;
    
    current_frame_number_++;
    
    // Update performance metrics
    update_performance_metrics(delta_time);
    
    // Update timeline if playing
    if (timeline_playing_ && current_state_ == DebuggerState::Running) {
        timeline_position_ += static_cast<f32>(delta_time * timeline_playback_speed_ / timeline_duration_);
        if (timeline_position_ > 1.0f) {
            timeline_position_ = 1.0f;
            timeline_playing_ = false;
        }
    }
    
    // Create periodic snapshots
    static f64 last_snapshot = 0.0;
    last_snapshot += delta_time;
    if (last_snapshot >= SNAPSHOT_FREQUENCY) {
        create_frame_snapshot();
        last_snapshot = 0.0;
    }
    
    // Update entity tracking
    for (ecs::Entity entity : tracked_entities_) {
        update_entity_snapshot(entity);
    }
    
    // Simulate some ECS operations for demonstration
    if (recording_enabled_ && current_state_ == DebuggerState::Running) {
        static f64 last_demo_operation = 0.0;
        last_demo_operation += delta_time;
        
        if (last_demo_operation >= 1.0) { // Demo operation every second
            // Create a fake operation for demonstration
            ECSOperation demo_op(ECSOperationType::EntityCreated);
            demo_op.frame_number = current_frame_number_;
            demo_op.operation_duration = 0.1; // 0.1ms
            demo_op.metadata["demo"] = "true";
            
            record_operation(demo_op);
            last_demo_operation = 0.0;
        }
    }
}

bool VisualDebuggerPanel::wants_keyboard_capture() const {
    return ImGui::IsWindowFocused() && (current_state_ == DebuggerState::Paused || current_state_ == DebuggerState::Breakpoint);
}

bool VisualDebuggerPanel::wants_mouse_capture() const {
    return ImGui::IsWindowHovered() || ImGui::IsWindowFocused();
}

void VisualDebuggerPanel::render_main_view() {
    // Main visualization area
    ImGui::BeginChild("##main_visualization", ImVec2(0, -150), true);
    
    // Entity visualization
    render_entity_visualization();
    
    // System execution overlay
    if (viz_settings_.show_performance_overlay) {
        render_system_execution_overlay();
    }
    
    ImGui::EndChild();
    
    // Bottom section with timeline and controls
    ImGui::Text("🕐 Timeline");
    render_timeline_scrubber();
    
    if (viz_settings_.show_frame_time_graph) {
        render_performance_graphs();
    }
}

void VisualDebuggerPanel::render_timeline_panel() {
    ImGui::Text("📅 Timeline View");
    ImGui::Separator();
    
    // Timeline controls
    render_playback_controls();
    ImGui::Separator();
    
    // Main timeline
    ImGui::BeginChild("##timeline_view", ImVec2(0, -100), true);
    render_timeline_scrubber();
    render_timeline_events();
    render_frame_markers();
    ImGui::EndChild();
    
    // Timeline info
    ImGui::Text("Position: %.1f%% | Duration: %.1f s | Operations: %zu", 
               timeline_position_ * 100.0f, timeline_duration_, recorded_operations_.size());
}

void VisualDebuggerPanel::render_breakpoints_panel() {
    ImGui::Text("🔴 Breakpoints");
    ImGui::Separator();
    
    // Breakpoint controls
    if (ImGui::Button("Add Breakpoint")) {
        Breakpoint new_bp;
        new_bp.operation_type = ECSOperationType::EntityCreated;
        new_bp.enabled = true;
        add_breakpoint(new_bp);
    }
    
    ImGui::SameLine();
    if (ImGui::Button("Clear All")) {
        clear_all_breakpoints();
    }
    
    ImGui::SameLine();
    ImGui::Checkbox("Enable Breakpoints", &breakpoints_enabled_);
    
    ImGui::Separator();
    
    // Breakpoint list
    ImGui::BeginChild("##breakpoint_list", ImVec2(0, 0), true);
    
    for (auto& [id, breakpoint] : breakpoints_) {
        ImGui::PushID(static_cast<int>(id));
        
        // Breakpoint enabled checkbox
        ImGui::Checkbox("##enabled", &breakpoint.enabled);
        ImGui::SameLine();
        
        // Breakpoint info
        std::string bp_desc = "BP" + std::to_string(id) + ": ";
        switch (breakpoint.operation_type) {
            case ECSOperationType::EntityCreated: bp_desc += "Entity Created"; break;
            case ECSOperationType::EntityDestroyed: bp_desc += "Entity Destroyed"; break;
            case ECSOperationType::ComponentAdded: bp_desc += "Component Added"; break;
            case ECSOperationType::ComponentRemoved: bp_desc += "Component Removed"; break;
            case ECSOperationType::SystemExecuted: bp_desc += "System Executed"; break;
            default: bp_desc += "Unknown"; break;
        }
        
        if (breakpoint.hit) {
            ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "%s (HIT %u times)", bp_desc.c_str(), breakpoint.hit_count);
        } else {
            ImGui::Text("%s", bp_desc.c_str());
        }
        
        // Breakpoint context menu
        if (ImGui::BeginPopupContextItem()) {
            if (ImGui::MenuItem("Edit Breakpoint")) {
                // Open breakpoint editor
            }
            if (ImGui::MenuItem("Delete Breakpoint")) {
                remove_breakpoint(id);
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }
        
        ImGui::PopID();
    }
    
    ImGui::EndChild();
}

void VisualDebuggerPanel::render_entity_inspector_panel() {
    ImGui::Text("🔍 Entity Inspector");
    ImGui::Separator();
    
    // Entity selection
    if (ImGui::BeginCombo("Select Entity", format_entity_info(selected_entity_).c_str())) {
        // List all entities - placeholder
        for (u32 i = 1; i <= 10; ++i) {
            ecs::Entity fake_entity{i, 0};
            bool selected = (selected_entity_.index == i);
            
            if (ImGui::Selectable(format_entity_info(fake_entity).c_str(), selected)) {
                select_entity(fake_entity);
            }
        }
        ImGui::EndCombo();
    }
    
    if (selected_entity_ != ecs::NULL_ENTITY) {
        ImGui::Separator();
        
        // Entity details
        ImGui::Text("Entity ID: %u.%u", selected_entity_.index, selected_entity_.generation);
        
        // Components section
        ImGui::Text("📦 Components:");
        ImGui::Indent();
        
        // Placeholder component list
        if (ImGui::CollapsingHeader("Transform##component", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::Text("Position: (100, 200)");
            ImGui::Text("Rotation: 0.5 rad");
            ImGui::Text("Scale: (1.0, 1.0)");
        }
        
        if (ImGui::CollapsingHeader("RigidBody##component")) {
            ImGui::Text("Mass: 1.0 kg");
            ImGui::Text("Velocity: (5.0, -2.0)");
            ImGui::Checkbox("Is Kinematic", nullptr);
        }
        
        ImGui::Unindent();
        
        // Entity history
        ImGui::Separator();
        ImGui::Text("📊 Entity History:");
        auto history_it = entity_history_.find(selected_entity_);
        if (history_it != entity_history_.end()) {
            ImGui::Text("Snapshots: %zu", history_it->second.size());
            ImGui::Text("Creation Time: %.2f s", history_it->second.empty() ? 0.0 : history_it->second[0].creation_time);
        } else {
            ImGui::Text("No history recorded");
        }
        
        // Quick actions
        ImGui::Separator();
        if (ImGui::Button("Track Entity")) {
            track_entity_lifecycle(selected_entity_);
        }
        ImGui::SameLine();
        if (ImGui::Button("Highlight Entity")) {
            highlight_entity(selected_entity_);
        }
    }
}

void VisualDebuggerPanel::render_system_profiler_panel() {
    ImGui::Text("⚙️ System Profiler");
    ImGui::Separator();
    
    // System execution summary
    ImGui::Text("Systems Executed: %zu", system_executions_.size());
    
    if (!system_executions_.empty()) {
        ImGui::Separator();
        
        // System list with performance data
        ImGui::BeginChild("##system_list", ImVec2(0, 0), true);
        
        for (const auto& execution : system_executions_) {
            ImGui::Text("🔧 %s", execution.system_name.c_str());
            ImGui::Indent();
            
            ImGui::Text("Duration: %.3f ms", execution.cpu_time * 1000.0);
            ImGui::Text("Entities Processed: %zu", execution.processed_entities.size());
            ImGui::Text("Memory Allocations: %zu", execution.memory_allocations);
            
            // Component access breakdown
            if (!execution.component_accesses.empty()) {
                ImGui::Text("Component Accesses:");
                ImGui::Indent();
                for (const auto& [component, count] : execution.component_accesses) {
                    ImGui::Text("  %s: %zu", component.c_str(), count);
                }
                ImGui::Unindent();
            }
            
            ImGui::Unindent();
            ImGui::Separator();
        }
        
        ImGui::EndChild();
    }
}

void VisualDebuggerPanel::render_memory_analyzer_panel() {
    ImGui::Text("💾 Memory Analyzer");
    ImGui::Separator();
    
    // Memory usage overview
    ImGui::Text("Current Memory Usage: %.1f MB", performance_.memory_usage_mb);
    ImGui::Text("Allocations This Frame: %zu", performance_.memory_allocations_per_frame);
    
    ImGui::Separator();
    
    // Memory history graph
    if (viz_settings_.show_memory_usage && !performance_.memory_usage_history.empty()) {
        ImGui::Text("Memory Usage History:");
        ImGui::PlotLines("##memory_history", 
                        performance_.memory_usage_history.data(), 
                        static_cast<int>(performance_.memory_usage_history.size()),
                        0, nullptr, 0.0f, FLT_MAX, 
                        ImVec2(0, 80));
    }
    
    // Memory allocation breakdown
    ImGui::Separator();
    ImGui::Text("Memory Allocation Breakdown:");
    ImGui::Indent();
    ImGui::Text("Entities: 25.6 MB");
    ImGui::Text("Components: 128.3 MB");
    ImGui::Text("Systems: 4.2 MB");
    ImGui::Text("Other: 12.1 MB");
    ImGui::Unindent();
}

void VisualDebuggerPanel::render_settings_panel() {
    ImGui::Text("⚙️ Debugger Settings");
    ImGui::Separator();
    
    // Recording settings
    ImGui::Checkbox("Enable Recording", &recording_enabled_);
    ImGui::SliderInt("Max Operations", reinterpret_cast<int*>(&max_recorded_operations_), 1000, 50000);
    
    ImGui::Separator();
    
    // Visualization settings
    ImGui::Text("Visualization:");
    ImGui::Checkbox("Show Entity IDs", &viz_settings_.show_entity_ids);
    ImGui::Checkbox("Show Component Types", &viz_settings_.show_component_types);
    ImGui::Checkbox("Show Archetype Connections", &viz_settings_.show_archetype_connections);
    ImGui::Checkbox("Animate Operations", &viz_settings_.animate_operations);
    
    ImGui::SliderFloat("Entity Size", &viz_settings_.entity_size, 5.0f, 100.0f);
    ImGui::SliderFloat("Animation Speed", &viz_settings_.animation_speed, 0.1f, 5.0f);
    
    ImGui::Separator();
    
    // Performance settings
    ImGui::Text("Performance:");
    ImGui::Checkbox("Show Performance Overlay", &viz_settings_.show_performance_overlay);
    ImGui::Checkbox("Show Memory Usage", &viz_settings_.show_memory_usage);
    ImGui::Checkbox("Show Frame Time Graph", &viz_settings_.show_frame_time_graph);
    ImGui::SliderFloat("Performance Graph Height", &viz_settings_.performance_graph_height, 50.0f, 200.0f);
    
    ImGui::Separator();
    
    // Timeline settings
    ImGui::Text("Timeline:");
    ImGui::SliderFloat("Timeline Duration", &timeline_duration_, 60.0f, 600.0f, "%.0f s");
    ImGui::SliderFloat("Playback Speed", &timeline_playback_speed_, 0.1f, 5.0f, "%.1fx");
}

// Control and visualization methods

void VisualDebuggerPanel::render_control_toolbar() {
    // Debugger state indicator
    const char* state_text = "Unknown";
    ImVec4 state_color = ImVec4(0.5f, 0.5f, 0.5f, 1.0f);
    
    switch (current_state_) {
        case DebuggerState::Running:
            state_text = "Running";
            state_color = ImVec4(0.2f, 0.8f, 0.2f, 1.0f);
            break;
        case DebuggerState::Paused:
            state_text = "Paused";
            state_color = ImVec4(1.0f, 0.8f, 0.2f, 1.0f);
            break;
        case DebuggerState::Breakpoint:
            state_text = "Breakpoint";
            state_color = ImVec4(1.0f, 0.3f, 0.3f, 1.0f);
            break;
        case DebuggerState::Stepping:
            state_text = "Stepping";
            state_color = ImVec4(0.2f, 0.7f, 1.0f, 1.0f);
            break;
    }
    
    ImGui::TextColored(state_color, "State: %s", state_text);
    ImGui::SameLine();
    
    // Recording indicator
    if (recording_enabled_) {
        ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "● REC");
    } else {
        ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "○ Not Recording");
    }
    
    ImGui::SameLine();
    ImGui::Text("| Frame: %u | Operations: %zu", current_frame_number_, recorded_operations_.size());
    
    ImGui::Separator();
}

void VisualDebuggerPanel::render_entity_visualization() {
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    ImVec2 canvas_pos = ImGui::GetCursorScreenPos();
    ImVec2 canvas_size = ImGui::GetContentRegionAvail();
    
    // Background
    draw_list->AddRectFilled(canvas_pos, 
                            ImVec2(canvas_pos.x + canvas_size.x, canvas_pos.y + canvas_size.y), 
                            IM_COL32(20, 20, 25, 255));
    
    // Demo entity visualization - draw some entities
    for (u32 i = 1; i <= 20; ++i) {
        f32 x = canvas_pos.x + (i % 5) * 100.0f + 50.0f;
        f32 y = canvas_pos.y + (i / 5) * 80.0f + 50.0f;
        f32 radius = viz_settings_.entity_size * 0.5f;
        
        ecs::Entity fake_entity{i, 0};
        ImU32 color = get_entity_color(fake_entity);
        
        // Entity circle
        draw_list->AddCircleFilled(ImVec2(x, y), radius, color);
        
        // Selection highlight
        if (selected_entity_.index == i) {
            draw_list->AddCircle(ImVec2(x, y), radius + 5.0f, IM_COL32(255, 255, 0, 255), 0, 3.0f);
        }
        
        // Entity ID
        if (viz_settings_.show_entity_ids) {
            std::string id_text = std::to_string(i);
            ImVec2 text_size = ImGui::CalcTextSize(id_text.c_str());
            draw_list->AddText(ImVec2(x - text_size.x * 0.5f, y - text_size.y * 0.5f), 
                              IM_COL32(255, 255, 255, 255), id_text.c_str());
        }
        
        // Component connections (demo)
        if (viz_settings_.show_archetype_connections && i > 1) {
            f32 prev_x = canvas_pos.x + ((i-1) % 5) * 100.0f + 50.0f;
            f32 prev_y = canvas_pos.y + ((i-1) / 5) * 80.0f + 50.0f;
            draw_list->AddLine(ImVec2(prev_x, prev_y), ImVec2(x, y), 
                              IM_COL32(100, 150, 255, 128), 2.0f);
        }
    }
    
    // Handle mouse interaction
    if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(0)) {
        ImVec2 mouse_pos = ImGui::GetMousePos();
        handle_entity_selection(mouse_pos.x - canvas_pos.x, mouse_pos.y - canvas_pos.y);
    }
    
    // Invisible button to capture input
    ImGui::InvisibleButton("##entity_canvas", canvas_size);
}

void VisualDebuggerPanel::render_system_execution_overlay() {
    // This would render system execution visualization over the entity view
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    ImVec2 canvas_pos = ImGui::GetCursorScreenPos();
    
    // Example: Draw system execution bars
    for (usize i = 0; i < std::min(system_executions_.size(), 5ul); ++i) {
        const auto& execution = system_executions_[i];
        f32 y = canvas_pos.y + 10.0f + i * 25.0f;
        f32 width = static_cast<f32>(execution.cpu_time * 1000.0); // Scale to pixels
        
        draw_list->AddRectFilled(ImVec2(canvas_pos.x + 10.0f, y), 
                                ImVec2(canvas_pos.x + 10.0f + width, y + 20.0f),
                                IM_COL32(255, 165, 0, 128));
        
        // System name
        draw_list->AddText(ImVec2(canvas_pos.x + 15.0f, y + 2.0f), 
                          IM_COL32(255, 255, 255, 255), 
                          execution.system_name.c_str());
    }
}

void VisualDebuggerPanel::render_timeline_scrubber() {
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    ImVec2 canvas_pos = ImGui::GetCursorScreenPos();
    ImVec2 canvas_size = ImVec2(ImGui::GetContentRegionAvail().x, TIMELINE_HEIGHT);
    
    // Timeline background
    draw_list->AddRectFilled(canvas_pos, 
                            ImVec2(canvas_pos.x + canvas_size.x, canvas_pos.y + canvas_size.y), 
                            IM_COL32(40, 40, 45, 255));
    
    // Timeline progress bar
    f32 progress_width = canvas_size.x * timeline_position_;
    draw_list->AddRectFilled(canvas_pos, 
                            ImVec2(canvas_pos.x + progress_width, canvas_pos.y + canvas_size.y), 
                            IM_COL32(70, 130, 180, 128));
    
    // Timeline markers for operations
    for (const auto& operation : recorded_operations_) {
        if (operation.frame_number == 0) continue;
        
        f32 op_position = static_cast<f32>(operation.frame_number) / static_cast<f32>(current_frame_number_);
        f32 x = canvas_pos.x + op_position * canvas_size.x;
        
        ImU32 marker_color = get_operation_color(operation.type);
        draw_list->AddLine(ImVec2(x, canvas_pos.y), 
                          ImVec2(x, canvas_pos.y + canvas_size.y), 
                          marker_color, 2.0f);
    }
    
    // Playhead
    f32 playhead_x = canvas_pos.x + timeline_position_ * canvas_size.x;
    draw_list->AddLine(ImVec2(playhead_x, canvas_pos.y), 
                      ImVec2(playhead_x, canvas_pos.y + canvas_size.y), 
                      IM_COL32(255, 255, 255, 255), 3.0f);
    
    // Handle timeline scrubbing
    ImGui::InvisibleButton("##timeline", canvas_size);
    if (ImGui::IsItemActive() && ImGui::IsMouseDragging(0)) {
        ImVec2 mouse_pos = ImGui::GetMousePos();
        f32 new_position = (mouse_pos.x - canvas_pos.x) / canvas_size.x;
        set_timeline_position(std::clamp(new_position, 0.0f, 1.0f));
    }
}

void VisualDebuggerPanel::render_performance_graphs() {
    if (performance_.frame_time_history.empty()) return;
    
    ImGui::Text("📊 Frame Time:");
    ImGui::PlotLines("##frame_time", 
                    performance_.frame_time_history.data(), 
                    static_cast<int>(performance_.frame_time_history.size()),
                    0, nullptr, 0.0f, 33.33f, // 0-33ms range (30-unlimited fps)
                    ImVec2(0, viz_settings_.performance_graph_height));
    
    ImGui::SameLine();
    ImGui::Text("%.2f ms", performance_.frame_time_ms);
}

// Control methods

void VisualDebuggerPanel::start_debugging() {
    current_state_ = DebuggerState::Running;
    recording_enabled_ = true;
    LOG_INFO("Visual debugger started");
}

void VisualDebuggerPanel::stop_debugging() {
    current_state_ = DebuggerState::Paused;
    timeline_playing_ = false;
    LOG_INFO("Visual debugger stopped");
}

void VisualDebuggerPanel::pause_execution() {
    if (current_state_ == DebuggerState::Running) {
        current_state_ = DebuggerState::Paused;
        timeline_playing_ = false;
        LOG_INFO("Execution paused");
    }
}

void VisualDebuggerPanel::resume_execution() {
    if (current_state_ == DebuggerState::Paused || current_state_ == DebuggerState::Breakpoint) {
        current_state_ = DebuggerState::Running;
        LOG_INFO("Execution resumed");
    }
}

void VisualDebuggerPanel::step_single_operation() {
    if (current_state_ == DebuggerState::Paused || current_state_ == DebuggerState::Breakpoint) {
        current_state_ = DebuggerState::Stepping;
        // Would advance by one operation
        LOG_INFO("Stepped single operation");
        current_state_ = DebuggerState::Paused;
    }
}

void VisualDebuggerPanel::step_single_frame() {
    if (current_state_ == DebuggerState::Paused || current_state_ == DebuggerState::Breakpoint) {
        current_state_ = DebuggerState::Stepping;
        // Would advance by one frame
        LOG_INFO("Stepped single frame");
        current_state_ = DebuggerState::Paused;
    }
}

// Recording and data management

void VisualDebuggerPanel::record_operation(const ECSOperation& operation) {
    if (!recording_enabled_) return;
    
    ECSOperation recorded_op = operation;
    recorded_op.operation_id = next_operation_id_++;
    recorded_op.frame_number = current_frame_number_;
    
    // Add to circular buffer
    if (recorded_operations_.size() < max_recorded_operations_) {
        recorded_operations_.push_back(recorded_op);
    } else {
        recorded_operations_[recording_head_] = recorded_op;
        recording_head_ = (recording_head_ + 1) % max_recorded_operations_;
    }
    
    // Update operation index
    operation_index_map_[recorded_op.operation_id] = 
        recorded_operations_.size() < max_recorded_operations_ ? 
        recorded_operations_.size() - 1 : recording_head_;
    
    // Check breakpoints
    check_breakpoints(recorded_op);
    
    // Update timeline
    f64 current_time = std::chrono::duration<f64>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
    
    // Add timeline event if new frame
    if (timeline_events_.empty() || timeline_events_.back().frame_number != current_frame_number_) {
        timeline_events_.emplace_back(current_time, current_frame_number_);
    }
    
    // Add operation to current timeline event
    if (!timeline_events_.empty()) {
        timeline_events_.back().operation_ids.push_back(recorded_op.operation_id);
    }
}

void VisualDebuggerPanel::clear_recording() {
    recorded_operations_.clear();
    operation_index_map_.clear();
    timeline_events_.clear();
    frame_snapshots_.clear();
    entity_history_.clear();
    system_executions_.clear();
    
    recording_head_ = 0;
    next_operation_id_ = 1;
    current_frame_number_ = 0;
    
    LOG_INFO("Recording cleared");
}

// Breakpoint management

u64 VisualDebuggerPanel::add_breakpoint(const Breakpoint& breakpoint) {
    Breakpoint new_bp = breakpoint;
    new_bp.breakpoint_id = next_breakpoint_id_++;
    
    u64 id = new_bp.breakpoint_id;
    breakpoints_[id] = new_bp;
    
    LOG_INFO("Added breakpoint " + std::to_string(id));
    return id;
}

void VisualDebuggerPanel::remove_breakpoint(u64 breakpoint_id) {
    auto it = breakpoints_.find(breakpoint_id);
    if (it != breakpoints_.end()) {
        breakpoints_.erase(it);
        LOG_INFO("Removed breakpoint " + std::to_string(breakpoint_id));
    }
}

void VisualDebuggerPanel::clear_all_breakpoints() {
    breakpoints_.clear();
    LOG_INFO("Cleared all breakpoints");
}

void VisualDebuggerPanel::check_breakpoints(const ECSOperation& operation) {
    if (!breakpoints_enabled_) return;
    
    for (auto& [id, breakpoint] : breakpoints_) {
        if (!breakpoint.enabled) continue;
        
        // Check operation type match
        if (breakpoint.operation_type != operation.type) continue;
        
        // Check specific entity filter
        if (breakpoint.specific_entity != ecs::NULL_ENTITY && 
            breakpoint.specific_entity != operation.target_entity) continue;
        
        // Check component type filter
        if (!breakpoint.component_type_filter.empty() && 
            breakpoint.component_type_filter != operation.component_type_name) continue;
        
        // Check system name filter
        if (!breakpoint.system_name_filter.empty() && 
            breakpoint.system_name_filter != operation.system_name) continue;
        
        // Evaluate custom condition if present
        if (breakpoint.condition_evaluator && 
            !breakpoint.condition_evaluator(operation)) continue;
        
        // Check hit count condition
        breakpoint.hit_count++;
        bool should_break = false;
        
        switch (breakpoint.hit_condition) {
            case Breakpoint::HitCondition::Always:
                should_break = true;
                break;
            case Breakpoint::HitCondition::HitCountEquals:
                should_break = (breakpoint.hit_count == breakpoint.hit_condition_value);
                break;
            case Breakpoint::HitCondition::HitCountMultiple:
                should_break = (breakpoint.hit_count % breakpoint.hit_condition_value == 0);
                break;
            case Breakpoint::HitCondition::HitCountGreater:
                should_break = (breakpoint.hit_count > breakpoint.hit_condition_value);
                break;
        }
        
        if (should_break) {
            trigger_breakpoint(breakpoint, operation);
        }
    }
}

void VisualDebuggerPanel::trigger_breakpoint(const Breakpoint& breakpoint, const ECSOperation& operation) {
    if (breakpoint.pause_execution) {
        current_state_ = DebuggerState::Breakpoint;
        current_breakpoint_operation_ = operation;
        timeline_playing_ = false;
    }
    
    if (breakpoint.highlight_entity && operation.target_entity != ecs::NULL_ENTITY) {
        highlight_entity(operation.target_entity);
    }
    
    if (breakpoint.log_operation) {
        LOG_INFO("Breakpoint hit: " + format_operation_description(operation));
    }
    
    if (breakpoint_hit_callback_) {
        breakpoint_hit_callback_(breakpoint);
    }
}

// Utility methods

void VisualDebuggerPanel::update_performance_metrics(f64 delta_time) {
    performance_.frame_time_ms = delta_time * 1000.0;
    
    // Update history arrays
    performance_.frame_time_history.push_back(static_cast<f32>(performance_.frame_time_ms));
    if (performance_.frame_time_history.size() > PerformanceMetrics::HISTORY_SIZE) {
        performance_.frame_time_history.erase(performance_.frame_time_history.begin());
    }
    
    // Simulate some metrics for demo
    performance_.entities_processed_per_frame = tracked_entities_.size();
    performance_.memory_usage_mb = 150.0f + std::sin(current_frame_number_ * 0.01f) * 20.0f;
    
    performance_.memory_usage_history.push_back(performance_.memory_usage_mb);
    if (performance_.memory_usage_history.size() > PerformanceMetrics::HISTORY_SIZE) {
        performance_.memory_usage_history.erase(performance_.memory_usage_history.begin());
    }
}

void VisualDebuggerPanel::create_frame_snapshot() {
    FrameSnapshot snapshot(current_frame_number_, 
                          std::chrono::duration<f64>(std::chrono::steady_clock::now().time_since_epoch()).count());
    
    // Capture current entity states
    for (ecs::Entity entity : tracked_entities_) {
        snapshot.entity_states[entity] = create_entity_snapshot(entity);
    }
    
    // Capture system executions
    snapshot.system_executions = system_executions_;
    
    // Capture metrics
    snapshot.total_entities = tracked_entities_.size();
    snapshot.memory_usage = static_cast<usize>(performance_.memory_usage_mb * 1024 * 1024);
    
    // Add to circular buffer
    if (frame_snapshots_.size() < max_frame_snapshots_) {
        frame_snapshots_.push_back(snapshot);
    } else {
        frame_snapshots_[snapshot_head_] = snapshot;
        snapshot_head_ = (snapshot_head_ + 1) % max_frame_snapshots_;
    }
}

EntitySnapshot VisualDebuggerPanel::create_entity_snapshot(ecs::Entity entity) const {
    EntitySnapshot snapshot(entity);
    snapshot.creation_time = std::chrono::duration<f64>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
    
    // Would populate with actual component data from registry
    snapshot.archetype_signature = "Transform,RigidBody"; // Placeholder
    snapshot.is_alive = true;
    
    return snapshot;
}

void VisualDebuggerPanel::handle_entity_selection(f32 mouse_x, f32 mouse_y) {
    // Simple entity selection based on mouse position
    // In a real implementation, this would check actual entity positions
    
    for (u32 i = 1; i <= 20; ++i) {
        f32 entity_x = (i % 5) * 100.0f + 50.0f;
        f32 entity_y = (i / 5) * 80.0f + 50.0f;
        f32 radius = viz_settings_.entity_size * 0.5f;
        
        f32 dist = std::sqrt((mouse_x - entity_x) * (mouse_x - entity_x) + 
                           (mouse_y - entity_y) * (mouse_y - entity_y));
        
        if (dist <= radius) {
            select_entity(ecs::Entity{i, 0});
            return;
        }
    }
}

void VisualDebuggerPanel::select_entity(ecs::Entity entity) {
    selected_entity_ = entity;
    LOG_INFO("Selected entity: " + format_entity_info(entity));
}

void VisualDebuggerPanel::highlight_entity(ecs::Entity entity, bool highlight) {
    if (highlight) {
        highlighted_entities_.insert(entity);
    } else {
        highlighted_entities_.erase(entity);
    }
}

void VisualDebuggerPanel::set_timeline_position(f32 position) {
    timeline_position_ = std::clamp(position, 0.0f, 1.0f);
    
    // Would trigger timeline scrubbing to restore state at this position
    if (timeline_position_ < 1.0f) {
        current_state_ = DebuggerState::Rewinding;
    }
}

// Formatting methods

std::string VisualDebuggerPanel::format_operation_description(const ECSOperation& operation) const {
    std::ostringstream oss;
    
    switch (operation.type) {
        case ECSOperationType::EntityCreated:
            oss << "Entity " << operation.target_entity.index << " created";
            break;
        case ECSOperationType::EntityDestroyed:
            oss << "Entity " << operation.target_entity.index << " destroyed";
            break;
        case ECSOperationType::ComponentAdded:
            oss << "Component " << operation.component_type_name << " added to entity " << operation.target_entity.index;
            break;
        case ECSOperationType::ComponentRemoved:
            oss << "Component " << operation.component_type_name << " removed from entity " << operation.target_entity.index;
            break;
        case ECSOperationType::SystemExecuted:
            oss << "System " << operation.system_name << " executed";
            break;
        default:
            oss << "Unknown operation";
            break;
    }
    
    return oss.str();
}

std::string VisualDebuggerPanel::format_entity_info(ecs::Entity entity) const {
    if (entity == ecs::NULL_ENTITY) {
        return "No Entity";
    }
    
    return "Entity " + std::to_string(entity.index) + "." + std::to_string(entity.generation);
}

ImU32 VisualDebuggerPanel::get_operation_color(ECSOperationType type) const {
    switch (type) {
        case ECSOperationType::EntityCreated: return IM_COL32(76, 175, 80, 255);   // Green
        case ECSOperationType::EntityDestroyed: return IM_COL32(244, 67, 54, 255); // Red  
        case ECSOperationType::ComponentAdded: return IM_COL32(33, 150, 243, 255); // Blue
        case ECSOperationType::ComponentRemoved: return IM_COL32(255, 152, 0, 255); // Orange
        case ECSOperationType::SystemExecuted: return IM_COL32(156, 39, 176, 255);  // Purple
        default: return IM_COL32(158, 158, 158, 255); // Gray
    }
}

ImU32 VisualDebuggerPanel::get_entity_color(ecs::Entity entity) const {
    // Simple hash-based coloring
    u32 hash = entity.index * 2654435761u; // Knuth's multiplicative hash
    return IM_COL32((hash >> 16) & 0xFF, (hash >> 8) & 0xFF, hash & 0xFF, 255);
}

// Stub implementations for remaining methods
void VisualDebuggerPanel::track_entity_lifecycle(ecs::Entity entity) {
    tracked_entities_.insert(entity);
    LOG_INFO("Tracking entity lifecycle: " + format_entity_info(entity));
}

void VisualDebuggerPanel::untrack_entity_lifecycle(ecs::Entity entity) {
    tracked_entities_.erase(entity);
    LOG_INFO("Stopped tracking entity: " + format_entity_info(entity));
}

void VisualDebuggerPanel::update_entity_snapshot(ecs::Entity entity) {
    EntitySnapshot snapshot = create_entity_snapshot(entity);
    entity_history_[entity].push_back(snapshot);
    
    // Limit history size
    if (entity_history_[entity].size() > 300) { // 5 minutes at 60fps
        entity_history_[entity].erase(entity_history_[entity].begin());
    }
}

void VisualDebuggerPanel::export_debug_session(const std::string& filename) {
    LOG_INFO("Exporting debug session to: " + filename);
    // Would implement JSON export of recorded operations and state
}

// Additional stub implementations
void VisualDebuggerPanel::render_playback_controls() { }
void VisualDebuggerPanel::render_timeline_events() { }
void VisualDebuggerPanel::render_frame_markers() { }
void VisualDebuggerPanel::start_recording() { recording_enabled_ = true; }
void VisualDebuggerPanel::stop_recording() { recording_enabled_ = false; }
void VisualDebuggerPanel::save_recording(const std::string& filename) { }
void VisualDebuggerPanel::load_recording(const std::string& filename) { }
void VisualDebuggerPanel::set_timeline_zoom(f32 zoom) { timeline_zoom_ = zoom; }
void VisualDebuggerPanel::play_timeline() { timeline_playing_ = true; }
void VisualDebuggerPanel::pause_timeline() { timeline_playing_ = false; }
void VisualDebuggerPanel::reset_timeline() { timeline_position_ = 0.0f; }

} // namespace ecscope::ui