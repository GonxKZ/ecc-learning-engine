#include "panel_physics_debug.hpp"
#include "physics/world.hpp"
#include "physics/physics_system.hpp"
#include "ecs/components/transform.hpp"
#include "core/time.hpp"
#include "core/log.hpp"
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <cmath>

#ifdef ECSCOPE_HAS_GRAPHICS
#include "imgui.h"
#include "imgui_internal.h"
// For custom drawing
#include <implot.h>
#endif

namespace ecscope::ui {

//=============================================================================
// Constructor and Initialization
//=============================================================================

PhysicsDebugPanel::PhysicsDebugPanel() 
    : Panel("Physics Debug", true) {
    
    // Initialize learning content
    initialize_learning_content();
    
    // Set up initial UI state
    ui_state_.camera_zoom = 1.0f;
    ui_state_.camera_offset = physics::math::Vec2{0.0f, 0.0f};
    
    // Initialize performance history
    performance_.frame_times.fill(0.0f);
    performance_.physics_times.fill(0.0f);
    performance_.collision_times.fill(0.0f);
    performance_.integration_times.fill(0.0f);
    performance_.constraint_times.fill(0.0f);
    performance_.active_body_counts.fill(0);
    performance_.collision_check_counts.fill(0);
    performance_.contact_counts.fill(0);
    
    // Reserve cache space
    cached_entities_.reserve(1000);
    
    ECSCOPE_LOG_INFO("Physics Debug Panel initialized");
}

//=============================================================================
// Core Panel Interface Implementation
//=============================================================================

void PhysicsDebugPanel::render() {
#ifdef ECSCOPE_HAS_GRAPHICS
    if (!visible_ || !physics_world_) return;
    
    frame_timer_.reset();
    
    // Begin main physics debug window
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_None;
    if (ImGui::Begin(name_.c_str(), &visible_, window_flags)) {
        
        // Render tab bar
        if (ImGui::BeginTabBar("PhysicsDebugTabs", ImGuiTabBarFlags_Reorderable)) {
            
            // Visualization Tab
            if (ImGui::BeginTabItem("Visualization")) {
                active_tab_ = ActiveTab::Visualization;
                render_visualization_tab();
                ImGui::EndTabItem();
            }
            
            // Inspector Tab
            if (ImGui::BeginTabItem("Inspector")) {
                active_tab_ = ActiveTab::Inspector;
                render_inspector_tab();
                ImGui::EndTabItem();
            }
            
            // Performance Tab
            if (ImGui::BeginTabItem("Performance")) {
                active_tab_ = ActiveTab::Performance;
                render_performance_tab();
                ImGui::EndTabItem();
            }
            
            // Learning Tab
            if (ImGui::BeginTabItem("Learning")) {
                active_tab_ = ActiveTab::Learning;
                render_learning_tab();
                ImGui::EndTabItem();
            }
            
            // Controls Tab
            if (ImGui::BeginTabItem("Controls")) {
                active_tab_ = ActiveTab::Controls;
                render_controls_tab();
                ImGui::EndTabItem();
            }
            
            // Analysis Tab
            if (ImGui::BeginTabItem("Analysis")) {
                active_tab_ = ActiveTab::Analysis;
                render_analysis_tab();
                ImGui::EndTabItem();
            }
            
            ImGui::EndTabBar();
        }
    }
    ImGui::End();
    
    // Render physics overlay if visualization is active
    if (active_tab_ == ActiveTab::Visualization && visualization_.show_collision_shapes) {
        render_physics_overlay();
    }
    
#endif
}

void PhysicsDebugPanel::update(f64 delta_time) {
    if (!physics_world_) return;
    
    // Update cached entities if needed
    if (should_update_cache()) {
        update_cached_entities();
    }
    
    // Update performance metrics
    update_performance_metrics(delta_time);
    
    // Update analysis data
    update_analysis_data();
    
    // Handle simulation controls
    if (controls_.step_requested && controls_.is_paused) {
        controls_.single_step = true;
        controls_.step_requested = false;
    }
    
    // Update mouse world position for interaction
    #ifdef ECSCOPE_HAS_GRAPHICS
    ImGuiIO& io = ImGui::GetIO();
    ui_state_.world_mouse_pos = screen_to_world(physics::math::Vec2{io.MousePos.x, io.MousePos.y});
    #endif
}

//=============================================================================
// Tab Rendering Implementation
//=============================================================================

void PhysicsDebugPanel::render_visualization_tab() {
#ifdef ECSCOPE_HAS_GRAPHICS
    
    // Left panel with options
    if (ImGui::BeginChild("VisualizationOptions", ImVec2(ui_state_.left_panel_width, 0), true)) {
        render_visualization_options();
    }
    ImGui::EndChild();
    
    ImGui::SameLine();
    
    // Main visualization area
    if (ImGui::BeginChild("MainVisualization", ImVec2(0, 0), true)) {
        
        ImDrawList* draw_list = ImGui::GetWindowDrawList();
        ImVec2 canvas_pos = ImGui::GetCursorScreenPos();
        ImVec2 canvas_size = ImGui::GetContentRegionAvail();
        
        // Draw physics world visualization
        if (visualization_.show_collision_shapes) {
            render_collision_shapes();
        }
        
        if (visualization_.show_velocity_vectors) {
            render_motion_vectors();
        }
        
        if (visualization_.show_force_vectors) {
            render_force_visualization();
        }
        
        if (visualization_.show_constraint_connections) {
            render_constraint_connections();
        }
        
        if (visualization_.show_spatial_hash) {
            render_spatial_hash_grid();
        }
        
        if (visualization_.show_contact_points) {
            render_contact_points();
        }
        
        // Handle mouse interaction
        if (ImGui::IsWindowHovered() && ui_state_.mouse_interaction_enabled) {
            ImGuiIO& io = ImGui::GetIO();
            
            // Zoom with mouse wheel
            if (io.MouseWheel != 0.0f) {
                f32 zoom_factor = 1.1f;
                if (io.MouseWheel > 0) {
                    ui_state_.camera_zoom *= zoom_factor;
                } else {
                    ui_state_.camera_zoom /= zoom_factor;
                }
                ui_state_.camera_zoom = std::clamp(ui_state_.camera_zoom, 0.1f, 10.0f);
            }
            
            // Pan with middle mouse button
            if (ImGui::IsMouseDragging(ImGuiMouseButton_Middle)) {
                physics::math::Vec2 delta{io.MouseDelta.x, io.MouseDelta.y};
                ui_state_.camera_offset += delta / ui_state_.camera_zoom;
            }
            
            // Entity selection with left click
            if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && inspector_.auto_select_on_click) {
                // Find entity under cursor
                physics::math::Vec2 world_pos = screen_to_world(physics::math::Vec2{io.MousePos.x, io.MousePos.y});
                // TODO: Implement entity picking based on world position
            }
        }
        
        // Show controls overlay
        ImGui::SetCursorPos(ImVec2(10, 10));
        if (ImGui::BeginChild("ViewControls", ImVec2(200, 0), false, ImGuiWindowFlags_NoBackground)) {
            ImGui::Text("View Controls:");
            ImGui::Text("Zoom: %.2f", ui_state_.camera_zoom);
            ImGui::Text("Offset: (%.1f, %.1f)", ui_state_.camera_offset.x, ui_state_.camera_offset.y);
            
            if (ImGui::Button("Reset View")) {
                ui_state_.camera_zoom = 1.0f;
                ui_state_.camera_offset = physics::math::Vec2{0.0f, 0.0f};
            }
        }
        ImGui::EndChild();
        
    }
    ImGui::EndChild();
    
#endif
}

void PhysicsDebugPanel::render_inspector_tab() {
#ifdef ECSCOPE_HAS_GRAPHICS
    
    // Entity selector on the left
    if (ImGui::BeginChild("EntitySelector", ImVec2(ui_state_.left_panel_width, 0), true)) {
        render_entity_selector();
    }
    ImGui::EndChild();
    
    ImGui::SameLine();
    
    // Entity properties on the right
    if (ImGui::BeginChild("EntityProperties", ImVec2(0, 0), true)) {
        render_entity_properties();
    }
    ImGui::EndChild();
    
#endif
}

void PhysicsDebugPanel::render_performance_tab() {
#ifdef ECSCOPE_HAS_GRAPHICS
    
    // Performance graphs at the top
    if (ui_state_.show_performance_graphs) {
        if (ImGui::BeginChild("PerformanceGraphs", ImVec2(0, ui_state_.graph_height * 2), true)) {
            render_performance_graphs();
        }
        ImGui::EndChild();
    }
    
    // Performance details below
    if (ImGui::BeginChild("PerformanceDetails", ImVec2(0, 0), true)) {
        
        // Split into left (bottleneck analysis) and right (optimization advice)
        if (ImGui::BeginTable("PerformanceLayout", 2, ImGuiTableFlags_Resizable)) {
            
            ImGui::TableSetupColumn("Analysis", ImGuiTableColumnFlags_WidthFixed, 350.0f);
            ImGui::TableSetupColumn("Optimization", ImGuiTableColumnFlags_WidthStretch);
            
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            render_bottleneck_analysis();
            render_memory_usage();
            
            ImGui::TableSetColumnIndex(1);
            render_optimization_advice();
            
            ImGui::EndTable();
        }
    }
    ImGui::EndChild();
    
#endif
}

void PhysicsDebugPanel::render_learning_tab() {
#ifdef ECSCOPE_HAS_GRAPHICS
    
    // Tutorial selector on the left
    if (ImGui::BeginChild("TutorialSelector", ImVec2(ui_state_.left_panel_width, 0), true)) {
        render_tutorial_selector();
        
        ImGui::Separator();
        
        if (ImGui::CollapsingHeader("Concept Explanations", ImGuiTreeNodeFlags_DefaultOpen)) {
            render_concept_explanations();
        }
        
        if (ImGui::CollapsingHeader("Interactive Experiments")) {
            render_interactive_experiments();
        }
    }
    ImGui::EndChild();
    
    ImGui::SameLine();
    
    // Learning content on the right
    if (ImGui::BeginChild("LearningContent", ImVec2(0, 0), true)) {
        
        if (learning_.active_tutorial != LearningTools::Tutorial::None) {
            render_active_tutorial();
        } else {
            ImGui::TextWrapped("Select a tutorial from the left panel to begin learning about physics concepts.");
            ImGui::Separator();
            
            if (learning_.show_mathematical_details) {
                render_physics_formulas();
            }
            
            if (learning_.show_real_world_examples) {
                render_real_world_examples();
            }
        }
    }
    ImGui::EndChild();
    
#endif
}

void PhysicsDebugPanel::render_controls_tab() {
#ifdef ECSCOPE_HAS_GRAPHICS
    
    // Simulation controls
    if (ImGui::CollapsingHeader("Simulation Controls", ImGuiTreeNodeFlags_DefaultOpen)) {
        render_simulation_controls();
    }
    
    // World parameters
    if (ImGui::CollapsingHeader("World Parameters", ImGuiTreeNodeFlags_DefaultOpen)) {
        render_world_parameters();
    }
    
    // Creation tools
    if (ImGui::CollapsingHeader("Creation Tools")) {
        render_creation_tools();
    }
    
    // Scenario presets
    if (ImGui::CollapsingHeader("Scenario Presets")) {
        render_scenario_presets();
    }
    
    // Export/Import options
    if (ImGui::CollapsingHeader("Export/Import")) {
        render_export_import_options();
    }
    
#endif
}

void PhysicsDebugPanel::render_analysis_tab() {
#ifdef ECSCOPE_HAS_GRAPHICS
    
    // Analysis options
    if (ImGui::BeginChild("AnalysisOptions", ImVec2(ui_state_.left_panel_width, 0), true)) {
        
        ImGui::Text("Analysis Tools");
        ImGui::Separator();
        
        ImGui::Checkbox("Monitor Energy Conservation", &analysis_.monitor_energy_conservation);
        ImGui::Checkbox("Analyze Force Distribution", &analysis_.analyze_force_distribution);
        ImGui::Checkbox("Analyze Spatial Efficiency", &analysis_.analyze_spatial_efficiency);
        ImGui::Checkbox("Check Numerical Stability", &analysis_.check_numerical_stability);
        
        ImGui::Separator();
        
        if (ImGui::Button("Reset Analysis", ImVec2(-1, 0))) {
            // Reset all analysis data
            analysis_.energy_history.fill(0.0f);
            analysis_.force_contributors.clear();
            analysis_.collision_stats = {};
            analysis_.cell_occupancy.clear();
        }
        
    }
    ImGui::EndChild();
    
    ImGui::SameLine();
    
    // Analysis results
    if (ImGui::BeginChild("AnalysisResults", ImVec2(0, 0), true)) {
        
        if (analysis_.monitor_energy_conservation) {
            if (ImGui::CollapsingHeader("Energy Analysis", ImGuiTreeNodeFlags_DefaultOpen)) {
                render_energy_analysis();
            }
        }
        
        if (analysis_.analyze_force_distribution) {
            if (ImGui::CollapsingHeader("Force Analysis", ImGuiTreeNodeFlags_DefaultOpen)) {
                render_force_analysis();
            }
        }
        
        if (ImGui::CollapsingHeader("Collision Analysis", ImGuiTreeNodeFlags_DefaultOpen)) {
            render_collision_analysis();
        }
        
        if (analysis_.analyze_spatial_efficiency) {
            if (ImGui::CollapsingHeader("Spatial Analysis")) {
                render_spatial_analysis();
            }
        }
        
        if (analysis_.check_numerical_stability) {
            if (ImGui::CollapsingHeader("Stability Analysis")) {
                render_stability_analysis();
            }
        }
        
    }
    ImGui::EndChild();
    
#endif
}

//=============================================================================
// Visualization Rendering Implementation
//=============================================================================

void PhysicsDebugPanel::render_visualization_options() {
#ifdef ECSCOPE_HAS_GRAPHICS
    
    ImGui::Text("Visualization Options");
    ImGui::Separator();
    
    // Shape visualization
    if (ImGui::CollapsingHeader("Shapes & Collision", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Checkbox("Collision Shapes", &visualization_.show_collision_shapes);
        ImGui::Checkbox("AABB Bounds", &visualization_.show_aabb_bounds);
        ImGui::Checkbox("Compound Shapes", &visualization_.show_compound_shapes);
        ImGui::Checkbox("Contact Points", &visualization_.show_contact_points);
        ImGui::Checkbox("Collision Normals", &visualization_.show_collision_normals);
        ImGui::Checkbox("Trigger Bounds", &visualization_.show_trigger_bounds);
    }
    
    // Motion visualization
    if (ImGui::CollapsingHeader("Motion & Forces", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Checkbox("Velocity Vectors", &visualization_.show_velocity_vectors);
        ImGui::Checkbox("Acceleration Vectors", &visualization_.show_acceleration_vectors);
        ImGui::Checkbox("Force Vectors", &visualization_.show_force_vectors);
        ImGui::Checkbox("Center of Mass", &visualization_.show_center_of_mass);
        ImGui::Checkbox("Angular Motion", &visualization_.show_angular_motion);
        ImGui::Checkbox("Trajectory Trails", &visualization_.show_trajectory_trails);
        
        ImGui::SliderFloat("Vector Scale", &visualization_.vector_scale, 0.1f, 5.0f, "%.1f");
        
        if (visualization_.show_trajectory_trails) {
            ImGui::SliderFloat("Trail Length", &visualization_.trail_length, 0.5f, 10.0f, "%.1f s");
        }
    }
    
    // Advanced visualization
    if (ImGui::CollapsingHeader("Advanced")) {
        ImGui::Checkbox("Spatial Hash Grid", &visualization_.show_spatial_hash);
        ImGui::Checkbox("Constraint Connections", &visualization_.show_constraint_connections);
        ImGui::Checkbox("Sleeping Bodies", &visualization_.show_sleeping_bodies);
        ImGui::Checkbox("Energy Visualization", &visualization_.show_energy_visualization);
        
        ImGui::SliderFloat("Line Thickness", &visualization_.line_thickness, 1.0f, 5.0f, "%.1f");
        ImGui::SliderFloat("Transparency", &visualization_.transparency, 0.1f, 1.0f, "%.1f");
        
        ImGui::Checkbox("Use Physics Colors", &visualization_.use_physics_colors);
    }
    
    // Color configuration
    if (ImGui::CollapsingHeader("Colors")) {
        ImGui::ColorEdit3("Static Bodies", reinterpret_cast<f32*>(&visualization_.static_body_color));
        ImGui::ColorEdit3("Dynamic Bodies", reinterpret_cast<f32*>(&visualization_.dynamic_body_color));
        ImGui::ColorEdit3("Kinematic Bodies", reinterpret_cast<f32*>(&visualization_.kinematic_body_color));
        ImGui::ColorEdit3("Sleeping Bodies", reinterpret_cast<f32*>(&visualization_.sleeping_body_color));
        ImGui::ColorEdit3("Contact Points", reinterpret_cast<f32*>(&visualization_.contact_color));
        ImGui::ColorEdit3("Force Vectors", reinterpret_cast<f32*>(&visualization_.force_color));
        ImGui::ColorEdit3("Velocity Vectors", reinterpret_cast<f32*>(&visualization_.velocity_color));
        ImGui::ColorEdit3("Constraints", reinterpret_cast<f32*>(&visualization_.constraint_color));
    }
    
#endif
}

void PhysicsDebugPanel::render_physics_overlay() {
    // This would render the physics visualization directly onto the main viewport
    // Implementation would depend on the specific rendering backend
    // For now, we'll render debug information in a separate window
    
#ifdef ECSCOPE_HAS_GRAPHICS
    
    ImGuiWindowFlags overlay_flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | 
                                    ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar |
                                    ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_AlwaysAutoResize |
                                    ImGuiWindowFlags_NoBackground;
    
    ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_Always);
    if (ImGui::Begin("PhysicsOverlay", nullptr, overlay_flags)) {
        
        // Physics world stats
        if (physics_world_) {
            auto entities = get_physics_entities();
            u32 active_bodies = 0;
            u32 sleeping_bodies = 0;
            
            for (auto entity : entities) {
                if (auto* rb = get_rigidbody(entity)) {
                    if (rb->physics_flags.is_sleeping) {
                        sleeping_bodies++;
                    } else {
                        active_bodies++;
                    }
                }
            }
            
            ImGui::Text("Physics Bodies: %u active, %u sleeping", active_bodies, sleeping_bodies);
            ImGui::Text("Frame Time: %.2f ms", performance_.average_frame_time * 1000.0f);
            ImGui::Text("Physics CPU: %.1f%%", performance_.physics_cpu_percentage);
        }
        
        // Simulation controls
        if (controls_.is_paused) {
            ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "SIMULATION PAUSED");
        }
        
        if (controls_.time_scale != 1.0f) {
            ImGui::Text("Time Scale: %.2fx", controls_.time_scale);
        }
        
    }
    ImGui::End();
    
#endif
}

void PhysicsDebugPanel::render_collision_shapes() {
    // Render collision shapes for all physics entities
    for (const auto& cached_entity : cached_entities_) {
        u32 color = get_body_color(*get_rigidbody(cached_entity.entity));
        
        // Render based on shape type
        // Implementation would use immediate mode rendering or a debug renderer
        // This is a simplified example showing the logic structure
        
        physics::math::Vec2 screen_pos = world_to_screen(cached_entity.position);
        
        // Example: render circle shape
        if (std::holds_alternative<physics::math::Circle>(cached_entity.collision_shape)) {
            const auto& circle = std::get<physics::math::Circle>(cached_entity.collision_shape);
            f32 screen_radius = circle.radius * ui_state_.camera_zoom;
            
            #ifdef ECSCOPE_HAS_GRAPHICS
            ImDrawList* draw_list = ImGui::GetBackgroundDrawList();
            draw_list->AddCircle(
                ImVec2(screen_pos.x, screen_pos.y),
                screen_radius,
                color,
                0,
                visualization_.line_thickness
            );
            #endif
        }
        
        // Similar rendering for other shape types...
    }
}

void PhysicsDebugPanel::render_motion_vectors() {
    if (!visualization_.show_velocity_vectors && !visualization_.show_acceleration_vectors) {
        return;
    }
    
    #ifdef ECSCOPE_HAS_GRAPHICS
    ImDrawList* draw_list = ImGui::GetBackgroundDrawList();
    
    for (const auto& cached_entity : cached_entities_) {
        physics::math::Vec2 screen_pos = world_to_screen(cached_entity.position);
        
        // Velocity vectors
        if (visualization_.show_velocity_vectors && cached_entity.velocity.length() > 0.01f) {
            physics::math::Vec2 velocity_end = cached_entity.position + 
                cached_entity.velocity * visualization_.vector_scale;
            physics::math::Vec2 screen_end = world_to_screen(velocity_end);
            
            draw_list->AddLine(
                ImVec2(screen_pos.x, screen_pos.y),
                ImVec2(screen_end.x, screen_end.y),
                visualization_.velocity_color,
                visualization_.line_thickness
            );
            
            // Arrow head
            physics::math::Vec2 direction = cached_entity.velocity.normalized();
            f32 arrow_size = 5.0f * ui_state_.camera_zoom;
            physics::math::Vec2 arrow_base = screen_end - direction * arrow_size;
            physics::math::Vec2 arrow_left = arrow_base + physics::math::Vec2(-direction.y, direction.x) * arrow_size * 0.5f;
            physics::math::Vec2 arrow_right = arrow_base + physics::math::Vec2(direction.y, -direction.x) * arrow_size * 0.5f;
            
            draw_list->AddTriangleFilled(
                ImVec2(screen_end.x, screen_end.y),
                ImVec2(arrow_left.x, arrow_left.y),
                ImVec2(arrow_right.x, arrow_right.y),
                visualization_.velocity_color
            );
        }
    }
    #endif
}

void PhysicsDebugPanel::render_force_visualization() {
    if (!visualization_.show_force_vectors) return;
    
    // Similar to velocity vectors but for forces
    // Would query ForceAccumulator components for force data
    
    #ifdef ECSCOPE_HAS_GRAPHICS
    ImDrawList* draw_list = ImGui::GetBackgroundDrawList();
    
    for (const auto& cached_entity : cached_entities_) {
        if (auto* forces = get_force_accumulator(cached_entity.entity)) {
            physics::math::Vec2 net_force;
            f32 net_torque;
            forces->get_net_forces(net_force, net_torque);
            
            if (net_force.length() > 0.01f) {
                physics::math::Vec2 screen_pos = world_to_screen(cached_entity.position);
                physics::math::Vec2 force_end = cached_entity.position + 
                    net_force * visualization_.vector_scale * 0.1f; // Scale down forces
                physics::math::Vec2 screen_end = world_to_screen(force_end);
                
                draw_list->AddLine(
                    ImVec2(screen_pos.x, screen_pos.y),
                    ImVec2(screen_end.x, screen_end.y),
                    visualization_.force_color,
                    visualization_.line_thickness
                );
            }
        }
    }
    #endif
}

void PhysicsDebugPanel::render_constraint_connections() {
    // Render lines between entities connected by constraints
    // Would iterate through Constraint2D components
    
    #ifdef ECSCOPE_HAS_GRAPHICS
    if (!visualization_.show_constraint_connections) return;
    
    ImDrawList* draw_list = ImGui::GetBackgroundDrawList();
    
    // TODO: Query constraint components and render connections
    // This would require iterating through all Constraint2D components
    // and drawing lines between the connected entities
    #endif
}

void PhysicsDebugPanel::render_spatial_hash_grid() {
    // Render the spatial hash grid if available from the physics world
    
    #ifdef ECSCOPE_HAS_GRAPHICS
    if (!visualization_.show_spatial_hash) return;
    
    ImDrawList* draw_list = ImGui::GetBackgroundDrawList();
    
    // TODO: Query spatial hash grid from physics world
    // This would require access to the broad-phase collision detection system
    // to get grid cell information
    #endif
}

void PhysicsDebugPanel::render_contact_points() {
    // Render contact points from collision detection
    
    #ifdef ECSCOPE_HAS_GRAPHICS
    if (!visualization_.show_contact_points) return;
    
    ImDrawList* draw_list = ImGui::GetBackgroundDrawList();
    
    // TODO: Query contact manifolds from collision world
    // Render contact points and normals
    #endif
}

void PhysicsDebugPanel::render_debug_annotations() {
    // Render text annotations with physics information
    
    #ifdef ECSCOPE_HAS_GRAPHICS
    ImDrawList* draw_list = ImGui::GetBackgroundDrawList();
    
    for (const auto& cached_entity : cached_entities_) {
        if (!cached_entity.debug_name.empty()) {
            physics::math::Vec2 screen_pos = world_to_screen(cached_entity.position);
            screen_pos.y -= 20.0f; // Offset above the object
            
            draw_list->AddText(
                ImVec2(screen_pos.x, screen_pos.y),
                IM_COL32(255, 255, 255, 255),
                cached_entity.debug_name.c_str()
            );
        }
    }
    #endif
}

//=============================================================================
// Inspector Rendering Implementation
//=============================================================================

void PhysicsDebugPanel::render_entity_selector() {
#ifdef ECSCOPE_HAS_GRAPHICS
    
    ImGui::Text("Physics Entities");
    ImGui::Separator();
    
    // Search filter
    ImGui::InputText("Search", ui_state_.entity_search_filter.data(), 
                     ui_state_.entity_search_filter.capacity());
    
    // Filter options
    ImGui::Checkbox("Active Bodies Only", &ui_state_.show_only_active_bodies);
    ImGui::Checkbox("Colliding Bodies Only", &ui_state_.show_only_colliding_bodies);
    
    ImGui::Separator();
    
    // Entity list
    auto entities = get_physics_entities();
    for (auto entity : entities) {
        auto* rb = get_rigidbody(entity);
        auto* transform = get_transform(entity);
        
        if (!rb || !transform) continue;
        
        // Apply filters
        if (ui_state_.show_only_active_bodies && rb->physics_flags.is_sleeping) continue;
        
        // Format entity name
        std::string entity_name = format("Entity %u", static_cast<u32>(entity));
        if (!ui_state_.entity_search_filter.empty() && 
            entity_name.find(ui_state_.entity_search_filter) == std::string::npos) {
            continue;
        }
        
        // Entity selection
        bool is_selected = (inspector_.selected_entity == entity);
        if (ImGui::Selectable(entity_name.c_str(), is_selected)) {
            select_entity(entity);
        }
        
        // Show additional info
        if (ImGui::IsItemHovered()) {
            ImGui::BeginTooltip();
            ImGui::Text("Position: (%.2f, %.2f)", transform->position.x, transform->position.y);
            ImGui::Text("Velocity: (%.2f, %.2f)", rb->velocity.x, rb->velocity.y);
            ImGui::Text("Mass: %.2f kg", rb->mass);
            ImGui::Text("Type: %s", get_body_type_string(*rb).c_str());
            ImGui::EndTooltip();
        }
        
        // Right-click context menu
        if (ImGui::BeginPopupContextItem()) {
            if (ImGui::MenuItem("Select")) {
                select_entity(entity);
            }
            if (ImGui::MenuItem("Follow")) {
                inspector_.follow_selected = true;
                select_entity(entity);
            }
            if (ImGui::MenuItem("Wake Up") && rb->physics_flags.is_sleeping) {
                rb->wake_up();
            }
            if (ImGui::MenuItem("Put to Sleep") && !rb->physics_flags.is_sleeping) {
                rb->put_to_sleep();
            }
            ImGui::EndPopup();
        }
    }
    
#endif
}

void PhysicsDebugPanel::render_entity_properties() {
#ifdef ECSCOPE_HAS_GRAPHICS
    
    if (!inspector_.entity_valid || !is_valid_entity(inspector_.selected_entity)) {
        ImGui::TextWrapped("No entity selected. Choose an entity from the list on the left.");
        return;
    }
    
    ImGui::Text("Entity %u Properties", static_cast<u32>(inspector_.selected_entity));
    ImGui::Separator();
    
    // Entity-wide controls
    if (ImGui::Button("Wake Up")) {
        if (auto* rb = get_rigidbody(inspector_.selected_entity)) {
            rb->wake_up();
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("Stop Motion")) {
        if (auto* rb = get_rigidbody(inspector_.selected_entity)) {
            rb->stop();
        }
    }
    ImGui::SameLine();
    ImGui::Checkbox("Follow", &inspector_.follow_selected);
    
    ImGui::Separator();
    
    // Component inspectors
    if (inspector_.show_transform_details) {
        if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen)) {
            render_transform_inspector();
        }
    }
    
    if (inspector_.show_rigidbody_details) {
        if (ImGui::CollapsingHeader("Rigid Body", ImGuiTreeNodeFlags_DefaultOpen)) {
            render_rigidbody_inspector();
        }
    }
    
    if (inspector_.show_collider_details) {
        if (ImGui::CollapsingHeader("Collider", ImGuiTreeNodeFlags_DefaultOpen)) {
            render_collider_inspector();
        }
    }
    
    if (inspector_.show_forces_details) {
        if (ImGui::CollapsingHeader("Forces", ImGuiTreeNodeFlags_DefaultOpen)) {
            render_forces_inspector();
        }
    }
    
    if (inspector_.show_constraints_details) {
        if (ImGui::CollapsingHeader("Constraints")) {
            render_constraints_inspector();
        }
    }
    
    if (inspector_.show_performance_details) {
        if (ImGui::CollapsingHeader("Performance")) {
            // Show entity-specific performance data
            ImGui::Text("Performance data for this entity would go here");
        }
    }
    
#endif
}

void PhysicsDebugPanel::render_transform_inspector() {
#ifdef ECSCOPE_HAS_GRAPHICS
    
    auto* transform = get_transform(inspector_.selected_entity);
    if (!transform) return;
    
    // Position
    if (inspector_.enable_live_editing) {
        if (ImGui::DragFloat2("Position", &transform->position.x, 0.1f, -1000.0f, 1000.0f, "%.2f")) {
            // Position changed
        }
        if (ImGui::DragFloat("Rotation", &transform->rotation, 0.01f, -360.0f, 360.0f, "%.2f°")) {
            // Rotation changed
        }
        if (ImGui::DragFloat2("Scale", &transform->scale.x, 0.01f, 0.1f, 10.0f, "%.2f")) {
            // Scale changed
        }
    } else {
        ImGui::Text("Position: (%.2f, %.2f)", transform->position.x, transform->position.y);
        ImGui::Text("Rotation: %.2f°", transform->rotation);
        ImGui::Text("Scale: (%.2f, %.2f)", transform->scale.x, transform->scale.y);
    }
    
    // Transform matrix info (read-only)
    if (inspector_.show_advanced_properties) {
        ImGui::Separator();
        ImGui::Text("Advanced Transform Data:");
        // Show matrix components, world space info, etc.
    }
    
#endif
}

void PhysicsDebugPanel::render_rigidbody_inspector() {
#ifdef ECSCOPE_HAS_GRAPHICS
    
    auto* rb = get_rigidbody(inspector_.selected_entity);
    if (!rb) return;
    
    // Body type
    ImGui::Text("Type: %s", get_body_type_string(*rb).c_str());
    
    // Mass properties
    if (inspector_.enable_live_editing) {
        if (ImGui::DragFloat("Mass", &rb->mass, 0.1f, 0.1f, 1000.0f, "%.2f kg")) {
            rb->set_mass(rb->mass);
        }
        if (ImGui::DragFloat("Moment of Inertia", &rb->moment_of_inertia, 0.1f, 0.1f, 1000.0f, "%.2f kg⋅m²")) {
            rb->set_moment_of_inertia(rb->moment_of_inertia);
        }
    } else {
        ImGui::Text("Mass: %.2f kg", rb->mass);
        ImGui::Text("Moment of Inertia: %.2f kg⋅m²", rb->moment_of_inertia);
    }
    
    ImGui::Text("Inverse Mass: %.4f", rb->inverse_mass);
    ImGui::Text("Inverse Inertia: %.4f", rb->inverse_moment_of_inertia);
    
    // Motion state
    ImGui::Separator();
    ImGui::Text("Motion State:");
    
    if (inspector_.enable_live_editing) {
        if (ImGui::DragFloat2("Velocity", &rb->velocity.x, 0.1f, -100.0f, 100.0f, "%.2f m/s")) {
            rb->set_velocity(rb->velocity);
        }
        if (ImGui::DragFloat("Angular Velocity", &rb->angular_velocity, 0.1f, -50.0f, 50.0f, "%.2f rad/s")) {
            rb->set_angular_velocity(rb->angular_velocity);
        }
    } else {
        ImGui::Text("Velocity: %s", format_vector(rb->velocity, "m/s").c_str());
        ImGui::Text("Angular Velocity: %.2f rad/s", rb->angular_velocity);
    }
    
    ImGui::Text("Speed: %.2f m/s", rb->velocity.length());
    ImGui::Text("Acceleration: %s", format_vector(rb->acceleration, "m/s²").c_str());
    ImGui::Text("Angular Acceleration: %.2f rad/s²", rb->angular_acceleration);
    
    // Energy information
    ImGui::Separator();
    f32 kinetic_energy = rb->calculate_kinetic_energy();
    ImGui::Text("Kinetic Energy: %.2f J", kinetic_energy);
    ImGui::Text("Linear KE: %.2f J", 0.5f * rb->mass * rb->velocity.length_squared());
    ImGui::Text("Angular KE: %.2f J", 0.5f * rb->moment_of_inertia * rb->angular_velocity * rb->angular_velocity);
    
    // Physics flags
    ImGui::Separator();
    ImGui::Text("Physics Flags:");
    ImGui::Text("Sleeping: %s", rb->physics_flags.is_sleeping ? "Yes" : "No");
    ImGui::Text("Static: %s", rb->physics_flags.is_static ? "Yes" : "No");
    ImGui::Text("Kinematic: %s", rb->physics_flags.is_kinematic ? "Yes" : "No");
    
    if (inspector_.enable_live_editing) {
        bool freeze_rotation = rb->physics_flags.freeze_rotation;
        if (ImGui::Checkbox("Freeze Rotation", &freeze_rotation)) {
            rb->physics_flags.freeze_rotation = freeze_rotation;
        }
        
        bool ignore_gravity = rb->physics_flags.ignore_gravity;
        if (ImGui::Checkbox("Ignore Gravity", &ignore_gravity)) {
            rb->physics_flags.ignore_gravity = ignore_gravity;
        }
    }
    
    // Damping and limits
    if (inspector_.show_advanced_properties) {
        ImGui::Separator();
        ImGui::Text("Advanced Properties:");
        
        if (inspector_.enable_live_editing) {
            render_property_editor("Linear Damping", rb->linear_damping, 0.0f, 1.0f);
            render_property_editor("Angular Damping", rb->angular_damping, 0.0f, 1.0f);
            render_property_editor("Gravity Scale", rb->gravity_scale, -2.0f, 2.0f);
            render_property_editor("Max Velocity", rb->max_velocity, 0.0f, 200.0f);
            render_property_editor("Max Angular Velocity", rb->max_angular_velocity, 0.0f, 100.0f);
        } else {
            ImGui::Text("Linear Damping: %.3f", rb->linear_damping);
            ImGui::Text("Angular Damping: %.3f", rb->angular_damping);
            ImGui::Text("Gravity Scale: %.2f", rb->gravity_scale);
            ImGui::Text("Max Velocity: %.1f m/s", rb->max_velocity);
            ImGui::Text("Max Angular Velocity: %.1f rad/s", rb->max_angular_velocity);
        }
    }
    
#endif
}

void PhysicsDebugPanel::render_collider_inspector() {
#ifdef ECSCOPE_HAS_GRAPHICS
    
    auto* collider = get_collider(inspector_.selected_entity);
    if (!collider) return;
    
    // Shape information
    ImGui::Text("Shape: %s", collider->get_shape_name());
    ImGui::Text("Shape Count: %zu", collider->get_shape_count());
    
    // Shape details based on type
    auto shape_info = collider->get_shape_info();
    ImGui::Text("Area: %.2f m²", shape_info.area);
    ImGui::Text("Perimeter: %.2f m", shape_info.perimeter);
    ImGui::Text("Centroid: %s", format_vector(shape_info.centroid, "m").c_str());
    ImGui::Text("Complexity: %u/10", shape_info.complexity_score);
    
    // Material properties
    ImGui::Separator();
    ImGui::Text("Material Properties:");
    
    if (inspector_.enable_live_editing) {
        render_material_editor(collider->material);
    } else {
        ImGui::Text("Restitution: %.2f", collider->material.restitution);
        ImGui::Text("Static Friction: %.2f", collider->material.static_friction);
        ImGui::Text("Kinetic Friction: %.2f", collider->material.kinetic_friction);
        ImGui::Text("Density: %.1f kg/m³", collider->material.density);
    }
    
    // Collision layers and filtering
    ImGui::Separator();
    ImGui::Text("Collision Filtering:");
    
    if (inspector_.enable_live_editing) {
        // Layer editing would go here
        ImGui::Text("Layers: 0x%08X", collider->collision_layers);
        ImGui::Text("Mask: 0x%08X", collider->collision_mask);
    } else {
        ImGui::Text("Collision Layers: 0x%08X", collider->collision_layers);
        ImGui::Text("Collision Mask: 0x%08X", collider->collision_mask);
    }
    
    // Collision flags
    ImGui::Text("Is Trigger: %s", collider->collision_flags.is_trigger ? "Yes" : "No");
    ImGui::Text("Is Sensor: %s", collider->collision_flags.is_sensor ? "Yes" : "No");
    ImGui::Text("Generate Events: %s", collider->collision_flags.generate_events ? "Yes" : "No");
    
    // Performance information
    if (inspector_.show_advanced_properties) {
        ImGui::Separator();
        ImGui::Text("Performance Metrics:");
        ImGui::Text("Collision Checks: %u", collider->performance_info.collision_checks_count);
        ImGui::Text("Last Check Duration: %.4f ms", collider->performance_info.last_check_duration * 1000.0f);
        ImGui::Text("Cache Hits: %u", collider->performance_info.cache_hits);
        ImGui::Text("Cache Misses: %u", collider->performance_info.cache_misses);
        
        f32 hit_ratio = collider->performance_info.cache_hits + collider->performance_info.cache_misses > 0 ?
            static_cast<f32>(collider->performance_info.cache_hits) / 
            (collider->performance_info.cache_hits + collider->performance_info.cache_misses) : 0.0f;
        ImGui::Text("Cache Hit Ratio: %.1f%%", hit_ratio * 100.0f);
    }
    
#endif
}

void PhysicsDebugPanel::render_forces_inspector() {
#ifdef ECSCOPE_HAS_GRAPHICS
    
    auto* forces = get_force_accumulator(inspector_.selected_entity);
    if (!forces) return;
    
    // Current accumulated forces
    physics::math::Vec2 net_force;
    f32 net_torque;
    forces->get_net_forces(net_force, net_torque);
    
    ImGui::Text("Net Force: %s", format_vector(net_force, "N").c_str());
    ImGui::Text("Net Torque: %.2f N⋅m", net_torque);
    ImGui::Text("Force Magnitude: %.2f N", net_force.length());
    
    // Impulses
    physics::math::Vec2 impulse;
    f32 angular_impulse;
    forces->get_impulses(impulse, angular_impulse);
    
    if (impulse.length() > 0.01f || std::abs(angular_impulse) > 0.01f) {
        ImGui::Separator();
        ImGui::Text("Impulses:");
        ImGui::Text("Linear Impulse: %s", format_vector(impulse, "N⋅s").c_str());
        ImGui::Text("Angular Impulse: %.2f N⋅m⋅s", angular_impulse);
    }
    
    // Force history
    auto force_history = forces->get_force_history();
    if (!force_history.empty()) {
        ImGui::Separator();
        ImGui::Text("Force Contributors (%zu):", force_history.size());
        
        for (const auto& record : force_history) {
            const char* type_names[] = {
                "Unknown", "Gravity", "Spring", "Damping", "Contact",
                "User", "Motor", "Friction", "Magnetic", "Wind"
            };
            
            u8 type_index = static_cast<u8>(record.type);
            const char* type_name = (type_index < std::size(type_names)) ? 
                type_names[type_index] : "Unknown";
            
            ImGui::Text("  %s: %s (%.2f N⋅m torque)", 
                       record.source_name ? record.source_name : "Unknown",
                       format_vector(record.force, "N").c_str(),
                       record.torque_contribution);
            ImGui::SameLine();
            ImGui::TextDisabled("(%s)", type_name);
        }
    }
    
    // Persistent forces
    auto persistent_forces = forces->get_persistent_forces();
    if (!persistent_forces.empty()) {
        ImGui::Separator();
        ImGui::Text("Persistent Forces (%zu):", persistent_forces.size());
        
        for (const auto& persistent : persistent_forces) {
            if (persistent.is_active) {
                ImGui::Text("  %s: %s/s", 
                           persistent.name ? persistent.name : "Unknown",
                           format_vector(persistent.force_per_second, "N").c_str());
                
                if (persistent.duration > 0) {
                    ImGui::SameLine();
                    ImGui::TextDisabled("(%.1fs remaining)", persistent.remaining_time);
                }
            }
        }
    }
    
    // Force analysis
    if (inspector_.show_advanced_properties) {
        ImGui::Separator();
        auto analysis = forces->get_force_analysis();
        ImGui::Text("Force Analysis:");
        ImGui::Text("Contributors: %u", analysis.force_contributors);
        ImGui::Text("Center of Pressure: %s", format_vector(analysis.center_of_pressure, "m").c_str());
        ImGui::Text("Largest Force: %.2f N", analysis.largest_force_mag);
    }
    
#endif
}

void PhysicsDebugPanel::render_constraints_inspector() {
#ifdef ECSCOPE_HAS_GRAPHICS
    
    // TODO: Query Constraint2D components for the selected entity
    // This would show all constraints attached to the entity
    
    ImGui::Text("Constraint information would be displayed here");
    ImGui::Text("This requires querying Constraint2D components that reference this entity");
    
#endif
}

//=============================================================================
// Performance Analysis Implementation
//=============================================================================

void PhysicsDebugPanel::render_performance_graphs() {
#ifdef ECSCOPE_HAS_GRAPHICS
    
    // Frame time graph
    if (ImPlot::BeginPlot("Frame Time", ImVec2(-1, ui_state_.graph_height))) {
        ImPlot::SetupAxes("Frame", "Time (ms)", ImPlotAxisFlags_AutoFit, ImPlotAxisFlags_AutoFit);
        
        // Convert to milliseconds for display
        std::array<f32, PerformanceMonitoring::HISTORY_SIZE> frame_times_ms;
        std::array<f32, PerformanceMonitoring::HISTORY_SIZE> physics_times_ms;
        
        for (usize i = 0; i < PerformanceMonitoring::HISTORY_SIZE; ++i) {
            frame_times_ms[i] = performance_.frame_times[i] * 1000.0f;
            physics_times_ms[i] = performance_.physics_times[i] * 1000.0f;
        }
        
        ImPlot::PlotLine("Total Frame", frame_times_ms.data(), PerformanceMonitoring::HISTORY_SIZE);
        ImPlot::PlotLine("Physics", physics_times_ms.data(), PerformanceMonitoring::HISTORY_SIZE);
        
        // Add target frame time reference line
        f32 target_frame_time = 1000.0f / controls_.target_fps;
        ImPlot::PlotHLines("Target", &target_frame_time, 1);
        
        ImPlot::EndPlot();
    }
    
    // Object count graph
    if (ImPlot::BeginPlot("Object Counts", ImVec2(-1, ui_state_.graph_height))) {
        ImPlot::SetupAxes("Frame", "Count", ImPlotAxisFlags_AutoFit, ImPlotAxisFlags_AutoFit);
        
        // Convert to floats for plotting
        std::array<f32, PerformanceMonitoring::HISTORY_SIZE> active_bodies_float;
        std::array<f32, PerformanceMonitoring::HISTORY_SIZE> collision_checks_float;
        std::array<f32, PerformanceMonitoring::HISTORY_SIZE> contacts_float;
        
        for (usize i = 0; i < PerformanceMonitoring::HISTORY_SIZE; ++i) {
            active_bodies_float[i] = static_cast<f32>(performance_.active_body_counts[i]);
            collision_checks_float[i] = static_cast<f32>(performance_.collision_check_counts[i]);
            contacts_float[i] = static_cast<f32>(performance_.contact_counts[i]);
        }
        
        ImPlot::PlotLine("Active Bodies", active_bodies_float.data(), PerformanceMonitoring::HISTORY_SIZE);
        ImPlot::PlotLine("Collision Checks", collision_checks_float.data(), PerformanceMonitoring::HISTORY_SIZE);
        ImPlot::PlotLine("Contacts", contacts_float.data(), PerformanceMonitoring::HISTORY_SIZE);
        
        ImPlot::EndPlot();
    }
    
#endif
}

void PhysicsDebugPanel::render_bottleneck_analysis() {
#ifdef ECSCOPE_HAS_GRAPHICS
    
    ImGui::Text("Performance Analysis");
    ImGui::Separator();
    
    // Overall rating
    ImGui::Text("Performance Rating: %s", performance_.performance_rating);
    
    // Frame time breakdown
    f32 frame_time_ms = performance_.average_frame_time * 1000.0f;
    f32 physics_percentage = performance_.physics_cpu_percentage;
    
    ImGui::Text("Average Frame Time: %.2f ms", frame_time_ms);
    ImGui::Text("Physics CPU Usage: %.1f%%", physics_percentage);
    ImGui::Text("Target FPS: %.0f (%.2f ms)", controls_.target_fps, 1000.0f / controls_.target_fps);
    
    // Progress bars for time breakdown
    ImGui::Text("Time Breakdown:");
    ImGui::ProgressBar(physics_percentage / 100.0f, ImVec2(-1, 0), 
                      format("Physics: %.1f%%", physics_percentage).c_str());
    
    f32 other_percentage = 100.0f - physics_percentage;
    ImGui::ProgressBar(other_percentage / 100.0f, ImVec2(-1, 0), 
                      format("Other: %.1f%%", other_percentage).c_str());
    
    // Bottleneck identification
    ImGui::Separator();
    ImGui::Text("Primary Bottleneck: %s", performance_.primary_bottleneck);
    
    // Detailed breakdown if available
    if (performance_.show_advanced_metrics) {
        ImGui::Separator();
        ImGui::Text("Detailed Breakdown:");
        // Add more detailed metrics here
    }
    
#endif
}

void PhysicsDebugPanel::render_memory_usage() {
#ifdef ECSCOPE_HAS_GRAPHICS
    
    ImGui::Separator();
    ImGui::Text("Memory Usage");
    ImGui::Separator();
    
    // Memory breakdown
    f32 total_mb = static_cast<f32>(performance_.total_physics_memory) / (1024.0f * 1024.0f);
    f32 rigidbody_mb = static_cast<f32>(performance_.rigid_body_memory) / (1024.0f * 1024.0f);
    f32 collider_mb = static_cast<f32>(performance_.collider_memory) / (1024.0f * 1024.0f);
    f32 constraint_mb = static_cast<f32>(performance_.constraint_memory) / (1024.0f * 1024.0f);
    
    ImGui::Text("Total Physics Memory: %.2f MB", total_mb);
    ImGui::Text("Rigid Bodies: %.2f MB", rigidbody_mb);
    ImGui::Text("Colliders: %.2f MB", collider_mb);
    ImGui::Text("Constraints: %.2f MB", constraint_mb);
    
    // Allocation statistics
    ImGui::Text("Allocations: %u", performance_.allocation_count);
    
    // Memory efficiency
    if (total_mb > 0.0f) {
        f32 rigidbody_percent = (rigidbody_mb / total_mb) * 100.0f;
        f32 collider_percent = (collider_mb / total_mb) * 100.0f;
        f32 constraint_percent = (constraint_mb / total_mb) * 100.0f;
        
        ImGui::ProgressBar(rigidbody_percent / 100.0f, ImVec2(-1, 0), 
                          format("RigidBodies: %.1f%%", rigidbody_percent).c_str());
        ImGui::ProgressBar(collider_percent / 100.0f, ImVec2(-1, 0), 
                          format("Colliders: %.1f%%", collider_percent).c_str());
        ImGui::ProgressBar(constraint_percent / 100.0f, ImVec2(-1, 0), 
                          format("Constraints: %.1f%%", constraint_percent).c_str());
    }
    
#endif
}

void PhysicsDebugPanel::render_optimization_advice() {
#ifdef ECSCOPE_HAS_GRAPHICS
    
    ImGui::Text("Optimization Advice");
    ImGui::Separator();
    
    if (!performance_.optimization_advice.empty()) {
        ImGui::TextWrapped("%s", performance_.optimization_advice.c_str());
    }
    
    if (!performance_.optimization_suggestions.empty()) {
        ImGui::Separator();
        ImGui::Text("Suggestions:");
        
        for (usize i = 0; i < performance_.optimization_suggestions.size(); ++i) {
            ImGui::BulletText("%s", performance_.optimization_suggestions[i].c_str());
        }
    }
    
    // Performance tips based on current state
    ImGui::Separator();
    ImGui::Text("General Tips:");
    ImGui::BulletText("Use sleeping system for inactive objects");
    ImGui::BulletText("Optimize collision shapes (prefer circles over polygons)");
    ImGui::BulletText("Use spatial partitioning for large numbers of objects");
    ImGui::BulletText("Consider object pooling for frequently created/destroyed objects");
    ImGui::BulletText("Profile with different time step values");
    
#endif
}

//=============================================================================
// Learning Tools Implementation
//=============================================================================

void PhysicsDebugPanel::render_tutorial_selector() {
#ifdef ECSCOPE_HAS_GRAPHICS
    
    ImGui::Text("Physics Tutorials");
    ImGui::Separator();
    
    const char* tutorial_names[] = {
        "No Tutorial",
        "Basic Physics Concepts",
        "Collision Detection",
        "Force Analysis",
        "Energy Conservation", 
        "Constraint Physics",
        "Optimization Techniques"
    };
    
    i32 current_tutorial = static_cast<i32>(learning_.active_tutorial);
    if (ImGui::Combo("Active Tutorial", &current_tutorial, tutorial_names, std::size(tutorial_names))) {
        learning_.active_tutorial = static_cast<LearningTools::Tutorial>(current_tutorial);
        if (learning_.active_tutorial != LearningTools::Tutorial::None) {
            start_tutorial(learning_.active_tutorial);
        }
    }
    
    // Tutorial options
    ImGui::Separator();
    ImGui::Checkbox("Show Mathematical Details", &learning_.show_mathematical_details);
    ImGui::Checkbox("Show Algorithm Breakdown", &learning_.show_algorithm_breakdown);
    ImGui::Checkbox("Interactive Examples", &learning_.interactive_examples);
    ImGui::Checkbox("Show Formulas", &learning_.show_formulas);
    ImGui::Checkbox("Show Real World Examples", &learning_.show_real_world_examples);
    
#endif
}

void PhysicsDebugPanel::render_active_tutorial() {
#ifdef ECSCOPE_HAS_GRAPHICS
    
    const char* tutorial_titles[] = {
        "No Tutorial",
        "Basic Physics Concepts",
        "Collision Detection Deep Dive",
        "Understanding Forces",
        "Energy and Momentum",
        "Constraints and Joints",
        "Physics Optimization"
    };
    
    u8 tutorial_index = static_cast<u8>(learning_.active_tutorial);
    if (tutorial_index < std::size(tutorial_titles)) {
        ImGui::Text("%s - Step %zu", tutorial_titles[tutorial_index], learning_.tutorial_step + 1);
        ImGui::Separator();
    }
    
    // Tutorial content based on active tutorial and step
    switch (learning_.active_tutorial) {
        case LearningTools::Tutorial::BasicPhysics:
            render_basic_physics_tutorial();
            break;
            
        case LearningTools::Tutorial::CollisionDetection:
            render_collision_detection_tutorial();
            break;
            
        case LearningTools::Tutorial::ForceAnalysis:
            render_force_analysis_tutorial();
            break;
            
        case LearningTools::Tutorial::EnergyConservation:
            render_energy_conservation_tutorial();
            break;
            
        case LearningTools::Tutorial::ConstraintPhysics:
            render_constraint_physics_tutorial();
            break;
            
        case LearningTools::Tutorial::OptimizationTech:
            render_optimization_tutorial();
            break;
            
        default:
            ImGui::Text("No tutorial selected");
            break;
    }
    
    // Tutorial navigation
    ImGui::Separator();
    if (ImGui::Button("Previous Step") && learning_.tutorial_step > 0) {
        learning_.tutorial_step--;
    }
    ImGui::SameLine();
    if (ImGui::Button("Next Step")) {
        advance_tutorial_step();
    }
    ImGui::SameLine();
    if (ImGui::Button("Reset Tutorial")) {
        learning_.tutorial_step = 0;
    }
    
#endif
}

void PhysicsDebugPanel::render_concept_explanations() {
#ifdef ECSCOPE_HAS_GRAPHICS
    
    // Searchable list of physics concepts
    ImGui::InputText("Search Concepts", ui_state_.concept_search_filter.data(), 
                     ui_state_.concept_search_filter.capacity());
    
    const char* concepts[] = {
        "Newton's Laws",
        "Force and Acceleration", 
        "Mass and Inertia",
        "Collision Detection",
        "Contact Resolution",
        "Energy Conservation",
        "Momentum Conservation",
        "Friction",
        "Restitution",
        "Constraints and Joints",
        "Spatial Partitioning",
        "Integration Methods"
    };
    
    for (const char* concept : concepts) {
        if (!ui_state_.concept_search_filter.empty() && 
            std::string(concept).find(ui_state_.concept_search_filter) == std::string::npos) {
            continue;
        }
        
        if (ImGui::Selectable(concept, learning_.selected_concept == concept)) {
            learning_.selected_concept = concept;
        }
        
        if (ImGui::IsItemHovered()) {
            ImGui::BeginTooltip();
            std::string explanation = get_concept_explanation(concept);
            if (explanation.length() > 100) {
                explanation = explanation.substr(0, 100) + "...";
            }
            ImGui::TextWrapped("%s", explanation.c_str());
            ImGui::EndTooltip();
        }
    }
    
#endif
}

void PhysicsDebugPanel::render_interactive_experiments() {
#ifdef ECSCOPE_HAS_GRAPHICS
    
    ImGui::Text("Interactive Experiments");
    ImGui::Separator();
    
    for (usize i = 0; i < learning_.available_experiments.size(); ++i) {
        const auto& experiment = learning_.available_experiments[i];
        
        bool is_active = (learning_.current_experiment == static_cast<i32>(i));
        if (ImGui::Selectable(experiment.name.c_str(), is_active)) {
            learning_.current_experiment = static_cast<i32>(i);
            setup_experiment(experiment.name);
        }
        
        if (ImGui::IsItemHovered()) {
            ImGui::BeginTooltip();
            ImGui::TextWrapped("%s", experiment.description.c_str());
            ImGui::EndTooltip();
        }
    }
    
    if (learning_.current_experiment >= 0) {
        ImGui::Separator();
        if (ImGui::Button("Start Experiment")) {
            auto& experiment = learning_.available_experiments[learning_.current_experiment];
            if (experiment.setup_function) {
                experiment.setup_function();
                experiment.is_active = true;
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("Stop Experiment")) {
            if (learning_.current_experiment >= 0) {
                learning_.available_experiments[learning_.current_experiment].is_active = false;
            }
        }
    }
    
#endif
}

//=============================================================================
// Utility Function Implementations
//=============================================================================

std::vector<ecs::Entity> PhysicsDebugPanel::get_physics_entities() const {
    std::vector<ecs::Entity> entities;
    
    if (!physics_world_) return entities;
    
    // TODO: Query all entities with physics components from the ECS registry
    // This would require access to the ECS registry through the physics world
    
    return entities;
}

physics::components::RigidBody2D* PhysicsDebugPanel::get_rigidbody(ecs::Entity entity) const {
    if (!physics_world_) return nullptr;
    
    // TODO: Query RigidBody2D component for entity
    // This would require access to the ECS registry
    
    return nullptr;
}

physics::components::Collider2D* PhysicsDebugPanel::get_collider(ecs::Entity entity) const {
    if (!physics_world_) return nullptr;
    
    // TODO: Query Collider2D component for entity
    
    return nullptr;
}

physics::components::ForceAccumulator* PhysicsDebugPanel::get_force_accumulator(ecs::Entity entity) const {
    if (!physics_world_) return nullptr;
    
    // TODO: Query ForceAccumulator component for entity
    
    return nullptr;
}

ecs::components::Transform* PhysicsDebugPanel::get_transform(ecs::Entity entity) const {
    if (!physics_world_) return nullptr;
    
    // TODO: Query Transform component for entity
    
    return nullptr;
}

physics::math::Vec2 PhysicsDebugPanel::world_to_screen(const physics::math::Vec2& world_pos) const {
    // Apply camera transformation
    physics::math::Vec2 camera_relative = world_pos - ui_state_.camera_offset;
    physics::math::Vec2 screen_pos = camera_relative * ui_state_.camera_zoom;
    
    // TODO: Add proper viewport transformation
    return screen_pos;
}

physics::math::Vec2 PhysicsDebugPanel::screen_to_world(const physics::math::Vec2& screen_pos) const {
    // Inverse of world_to_screen
    physics::math::Vec2 camera_relative = screen_pos / ui_state_.camera_zoom;
    physics::math::Vec2 world_pos = camera_relative + ui_state_.camera_offset;
    
    return world_pos;
}

u32 PhysicsDebugPanel::get_body_color(const physics::components::RigidBody2D& body) const {
    if (body.physics_flags.is_sleeping) {
        return visualization_.sleeping_body_color;
    } else if (body.physics_flags.is_static) {
        return visualization_.static_body_color;
    } else if (body.physics_flags.is_kinematic) {
        return visualization_.kinematic_body_color;
    } else {
        return visualization_.dynamic_body_color;
    }
}

std::string PhysicsDebugPanel::format_physics_value(f32 value, const char* unit, u32 decimal_places) const {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(decimal_places) << value;
    if (unit && unit[0] != '\0') {
        oss << " " << unit;
    }
    return oss.str();
}

std::string PhysicsDebugPanel::format_vector(const physics::math::Vec2& vec, const char* unit) const {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2) << "(" << vec.x << ", " << vec.y << ")";
    if (unit && unit[0] != '\0') {
        oss << " " << unit;
    }
    return oss.str();
}

std::string PhysicsDebugPanel::get_body_type_string(const physics::components::RigidBody2D& body) const {
    if (body.physics_flags.is_static) return "Static";
    if (body.physics_flags.is_kinematic) return "Kinematic";
    return "Dynamic";
}

void PhysicsDebugPanel::select_entity(ecs::Entity entity) {
    inspector_.selected_entity = entity;
    inspector_.entity_valid = is_valid_entity(entity);
}

bool PhysicsDebugPanel::is_valid_entity(ecs::Entity entity) const {
    // TODO: Check if entity exists in ECS registry
    return entity != ecs::Entity{};
}

bool PhysicsDebugPanel::has_physics_components(ecs::Entity entity) const {
    // TODO: Check if entity has required physics components
    return get_rigidbody(entity) != nullptr || get_collider(entity) != nullptr;
}

void PhysicsDebugPanel::update_cached_entities() {
    cached_entities_.clear();
    
    auto entities = get_physics_entities();
    for (auto entity : entities) {
        cached_entities_.push_back(create_cached_entity_data(entity));
    }
    
    cache_update_timer_ = 0.0;
}

PhysicsDebugPanel::CachedEntityData PhysicsDebugPanel::create_cached_entity_data(ecs::Entity entity) {
    CachedEntityData data{};
    data.entity = entity;
    
    // TODO: Fill in data from actual components
    // This is a placeholder implementation
    
    return data;
}

bool PhysicsDebugPanel::should_update_cache() const {
    return cache_update_timer_ >= CACHE_UPDATE_INTERVAL;
}

void PhysicsDebugPanel::update_performance_metrics(f64 delta_time) {
    // Update performance history
    usize next_index = (performance_.history_index + 1) % PerformanceMonitoring::HISTORY_SIZE;
    
    // TODO: Get actual timing data from physics world
    performance_.frame_times[next_index] = static_cast<f32>(delta_time);
    performance_.physics_times[next_index] = 0.0f; // Placeholder
    
    performance_.history_index = next_index;
    
    // Calculate averages
    f32 total_frame_time = 0.0f;
    for (f32 time : performance_.frame_times) {
        total_frame_time += time;
    }
    performance_.average_frame_time = total_frame_time / PerformanceMonitoring::HISTORY_SIZE;
}

void PhysicsDebugPanel::update_analysis_data() {
    if (!physics_world_) return;
    
    // TODO: Update analysis data from physics world
    // This would include energy calculations, force analysis, etc.
}

void PhysicsDebugPanel::initialize_learning_content() {
    // Initialize concept explanations
    learning_.concept_explanations["Newton's Laws"] = 
        "Newton's three laws of motion form the foundation of classical mechanics:\n"
        "1. An object at rest stays at rest, and an object in motion stays in motion, unless acted upon by a force.\n"
        "2. F = ma - Force equals mass times acceleration.\n"
        "3. For every action, there is an equal and opposite reaction.";
        
    learning_.concept_explanations["Force and Acceleration"] =
        "Force is a push or pull that can change an object's motion. Acceleration is the rate of change of velocity.\n"
        "The relationship F = ma shows that force is directly proportional to acceleration and mass.";
        
    // TODO: Add more concept explanations
    
    // Initialize experiments
    learning_.available_experiments = {
        {"Gravity Demo", "Drop objects with different masses to see gravity effects", nullptr, false},
        {"Collision Types", "Compare elastic vs inelastic collisions", nullptr, false},
        {"Spring Forces", "Experiment with Hooke's law and spring constants", nullptr, false},
        {"Friction Effects", "Compare motion with different friction coefficients", nullptr, false}
    };
}

const std::string& PhysicsDebugPanel::get_concept_explanation(const std::string& concept) const {
    auto it = learning_.concept_explanations.find(concept);
    if (it != learning_.concept_explanations.end()) {
        return it->second;
    }
    
    static const std::string empty_explanation = "No explanation available for this concept.";
    return empty_explanation;
}

void PhysicsDebugPanel::start_tutorial(LearningTools::Tutorial tutorial) {
    learning_.active_tutorial = tutorial;
    learning_.tutorial_step = 0;
    
    // TODO: Initialize tutorial-specific content
}

void PhysicsDebugPanel::advance_tutorial_step() {
    learning_.tutorial_step++;
    
    // TODO: Handle tutorial step progression
}

//=============================================================================
// Placeholder Tutorial Rendering Functions
//=============================================================================

void PhysicsDebugPanel::render_basic_physics_tutorial() {
#ifdef ECSCOPE_HAS_GRAPHICS
    ImGui::TextWrapped("Basic Physics Tutorial content would go here, covering fundamental concepts like mass, force, acceleration, and Newton's laws.");
#endif
}

void PhysicsDebugPanel::render_collision_detection_tutorial() {
#ifdef ECSCOPE_HAS_GRAPHICS
    ImGui::TextWrapped("Collision Detection Tutorial content would cover broad-phase and narrow-phase collision detection, spatial partitioning, and collision response.");
#endif
}

void PhysicsDebugPanel::render_force_analysis_tutorial() {
#ifdef ECSCOPE_HAS_GRAPHICS
    ImGui::TextWrapped("Force Analysis Tutorial would explain different types of forces, force accumulation, and how forces affect motion.");
#endif
}

void PhysicsDebugPanel::render_energy_conservation_tutorial() {
#ifdef ECSCOPE_HAS_GRAPHICS
    ImGui::TextWrapped("Energy Conservation Tutorial would cover kinetic and potential energy, energy conservation in collisions, and energy loss mechanisms.");
#endif
}

void PhysicsDebugPanel::render_constraint_physics_tutorial() {
#ifdef ECSCOPE_HAS_GRAPHICS
    ImGui::TextWrapped("Constraint Physics Tutorial would explain joints, springs, motors, and constraint solving techniques.");
#endif
}

void PhysicsDebugPanel::render_optimization_tutorial() {
#ifdef ECSCOPE_HAS_GRAPHICS
    ImGui::TextWrapped("Optimization Tutorial would cover performance optimization techniques, profiling, and best practices for physics simulation.");
#endif
}

//=============================================================================
// Additional Placeholder Implementations
//=============================================================================

void PhysicsDebugPanel::render_frame_time_graph() {
    // Already implemented in render_performance_graphs()
}

void PhysicsDebugPanel::render_collision_stats_graph() {
    // Already implemented in render_performance_graphs()
}

void PhysicsDebugPanel::render_cpu_usage_breakdown() {
    // Already implemented in render_bottleneck_analysis()
}

void PhysicsDebugPanel::render_simulation_controls() {
#ifdef ECSCOPE_HAS_GRAPHICS
    ImGui::Text("Simulation Controls");
    ImGui::Separator();
    
    // Play/Pause controls
    if (controls_.is_paused) {
        if (ImGui::Button("Play")) {
            controls_.is_paused = false;
        }
    } else {
        if (ImGui::Button("Pause")) {
            controls_.is_paused = true;
        }
    }
    
    ImGui::SameLine();
    if (ImGui::Button("Step")) {
        request_single_step();
    }
    
    // Time scale
    ImGui::SliderFloat("Time Scale", &controls_.time_scale, 0.01f, 5.0f, "%.2fx");
    
    // Target FPS
    ImGui::SliderFloat("Target FPS", &controls_.target_fps, 10.0f, 240.0f, "%.0f");
    
    // Fixed timestep option
    ImGui::Checkbox("Fixed Timestep", &controls_.fixed_timestep);
    if (controls_.fixed_timestep) {
        ImGui::SliderFloat("Timestep", &controls_.custom_timestep, 1.0f/240.0f, 1.0f/10.0f, "%.4f s");
    }
#endif
}

void PhysicsDebugPanel::render_world_parameters() {
#ifdef ECSCOPE_HAS_GRAPHICS
    ImGui::Text("World Parameters");
    ImGui::Separator();
    
    // TODO: Get actual world parameters from physics world
    static physics::math::Vec2 gravity{0.0f, -9.81f};
    ImGui::DragFloat2("Gravity", &gravity.x, 0.1f, -50.0f, 50.0f, "%.2f m/s²");
    
    static f32 linear_damping = 0.01f;
    ImGui::SliderFloat("Global Linear Damping", &linear_damping, 0.0f, 1.0f, "%.3f");
    
    static f32 angular_damping = 0.01f;
    ImGui::SliderFloat("Global Angular Damping", &angular_damping, 0.0f, 1.0f, "%.3f");
#endif
}

void PhysicsDebugPanel::render_creation_tools() {
#ifdef ECSCOPE_HAS_GRAPHICS
    ImGui::Text("Object Creation Tools");
    ImGui::Separator();
    
    ImGui::Checkbox("Creation Mode", &controls_.creation_mode);
    
    if (controls_.creation_mode) {
        // Shape selection
        const char* shape_names[] = {"Circle", "Box", "Polygon"};
        i32 shape_index = static_cast<i32>(controls_.shape_to_create);
        ImGui::Combo("Shape", &shape_index, shape_names, std::size(shape_names));
        controls_.shape_to_create = static_cast<physics::components::CollisionShape::Type>(shape_index);
        
        // Basic properties
        ImGui::DragFloat("Mass", &controls_.creation_mass, 0.1f, 0.1f, 100.0f, "%.2f kg");
        ImGui::Checkbox("Static", &controls_.creation_is_static);
        
        // Material properties
        render_material_editor(controls_.creation_material);
        
        ImGui::Text("Click in the visualization area to create objects");
    }
#endif
}

void PhysicsDebugPanel::render_scenario_presets() {
#ifdef ECSCOPE_HAS_GRAPHICS
    ImGui::Text("Scenario Presets");
    ImGui::Separator();
    
    if (ImGui::Button("Basic Stack", ImVec2(-1, 0))) {
        // TODO: Create basic stack scenario
    }
    
    if (ImGui::Button("Pendulum", ImVec2(-1, 0))) {
        // TODO: Create pendulum scenario
    }
    
    if (ImGui::Button("Domino Chain", ImVec2(-1, 0))) {
        // TODO: Create domino chain scenario
    }
    
    if (ImGui::Button("Spring System", ImVec2(-1, 0))) {
        // TODO: Create spring system scenario
    }
    
    if (ImGui::Button("Clear All", ImVec2(-1, 0))) {
        // TODO: Clear all physics objects
    }
#endif
}

void PhysicsDebugPanel::render_export_import_options() {
#ifdef ECSCOPE_HAS_GRAPHICS
    ImGui::Text("Export/Import Options");
    ImGui::Separator();
    
    if (ImGui::Button("Export Scene", ImVec2(-1, 0))) {
        // TODO: Export current physics scene
    }
    
    if (ImGui::Button("Import Scene", ImVec2(-1, 0))) {
        // TODO: Import physics scene from file
    }
    
    if (ImGui::Button("Export Performance Data", ImVec2(-1, 0))) {
        // TODO: Export performance metrics
    }
#endif
}

void PhysicsDebugPanel::render_energy_analysis() {
#ifdef ECSCOPE_HAS_GRAPHICS
    ImGui::Text("Energy Analysis");
    ImGui::Separator();
    
    ImGui::Text("Total Kinetic Energy: %.2f J", analysis_.total_kinetic_energy);
    ImGui::Text("Total Potential Energy: %.2f J", analysis_.total_potential_energy);
    ImGui::Text("Total System Energy: %.2f J", analysis_.total_kinetic_energy + analysis_.total_potential_energy);
    ImGui::Text("Energy Conservation Error: %.4f%%", analysis_.energy_conservation_error * 100.0f);
    
    // Energy history graph would go here
#endif
}

void PhysicsDebugPanel::render_force_analysis() {
#ifdef ECSCOPE_HAS_GRAPHICS
    ImGui::Text("Force Analysis");
    ImGui::Separator();
    
    ImGui::Text("Net System Force: %s", format_vector(analysis_.net_force, "N").c_str());
    ImGui::Text("Total Force Magnitude: %.2f N", analysis_.total_force_magnitude);
    
    if (!analysis_.force_contributors.empty()) {
        ImGui::Text("Top Force Contributors:");
        for (usize i = 0; i < std::min(analysis_.force_contributors.size(), usize{5}); ++i) {
            ImGui::Text("  Entity %u: %.2f N", 
                       static_cast<u32>(analysis_.force_contributors[i].first),
                       analysis_.force_contributors[i].second);
        }
    }
#endif
}

void PhysicsDebugPanel::render_collision_analysis() {
#ifdef ECSCOPE_HAS_GRAPHICS
    ImGui::Text("Collision Analysis");
    ImGui::Separator();
    
    ImGui::Text("Total Collision Checks: %u", analysis_.collision_stats.total_checks);
    ImGui::Text("Broad Phase Culled: %u", analysis_.collision_stats.broad_phase_culled);
    ImGui::Text("Narrow Phase Contacts: %u", analysis_.collision_stats.narrow_phase_contacts);
    ImGui::Text("Average Contact Depth: %.3f m", analysis_.collision_stats.average_contact_depth);
    ImGui::Text("Max Contact Force: %.2f N", analysis_.collision_stats.max_contact_force);
    
    if (analysis_.collision_stats.total_checks > 0) {
        f32 efficiency = static_cast<f32>(analysis_.collision_stats.broad_phase_culled) / 
                        analysis_.collision_stats.total_checks * 100.0f;
        ImGui::Text("Broad Phase Efficiency: %.1f%%", efficiency);
    }
#endif
}

void PhysicsDebugPanel::render_spatial_analysis() {
#ifdef ECSCOPE_HAS_GRAPHICS
    ImGui::Text("Spatial Partitioning Analysis");
    ImGui::Separator();
    
    ImGui::Text("Hash Load Factor: %.2f", analysis_.spatial_hash_load_factor);
    ImGui::Text("Average Objects per Cell: %u", analysis_.average_objects_per_cell);
    ImGui::Text("Max Objects per Cell: %u", analysis_.max_objects_per_cell);
    
    // Cell occupancy distribution would be shown here
#endif
}

void PhysicsDebugPanel::render_stability_analysis() {
#ifdef ECSCOPE_HAS_GRAPHICS
    ImGui::Text("Numerical Stability Analysis");
    ImGui::Separator();
    
    ImGui::Text("Max Velocity: %.2f m/s", analysis_.max_velocity_magnitude);
    ImGui::Text("Max Acceleration: %.2f m/s²", analysis_.max_acceleration_magnitude);
    ImGui::Text("NaN Values Detected: %s", analysis_.has_nan_values ? "Yes" : "No");
    ImGui::Text("Infinite Values Detected: %s", analysis_.has_infinite_values ? "Yes" : "No");
    ImGui::Text("Unstable Objects: %u", analysis_.unstable_object_count);
    
    if (analysis_.has_nan_values || analysis_.has_infinite_values || analysis_.unstable_object_count > 0) {
        ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "Warning: Numerical instabilities detected!");
    }
#endif
}

void PhysicsDebugPanel::render_property_editor(const std::string& property_name, f32& value, f32 min_val, f32 max_val) {
#ifdef ECSCOPE_HAS_GRAPHICS
    ImGui::SliderFloat(property_name.c_str(), &value, min_val, max_val, "%.3f");
#endif
}

void PhysicsDebugPanel::render_material_editor(physics::components::PhysicsMaterial& material) {
#ifdef ECSCOPE_HAS_GRAPHICS
    ImGui::Text("Material Properties:");
    ImGui::SliderFloat("Restitution", &material.restitution, 0.0f, 1.0f, "%.2f");
    ImGui::SliderFloat("Static Friction", &material.static_friction, 0.0f, 2.0f, "%.2f");
    ImGui::SliderFloat("Kinetic Friction", &material.kinetic_friction, 0.0f, 2.0f, "%.2f");
    ImGui::SliderFloat("Density", &material.density, 100.0f, 10000.0f, "%.0f kg/m³");
    
    // Material presets
    if (ImGui::Button("Steel")) material = physics::components::PhysicsMaterial::steel();
    ImGui::SameLine();
    if (ImGui::Button("Rubber")) material = physics::components::PhysicsMaterial::rubber();
    ImGui::SameLine();
    if (ImGui::Button("Ice")) material = physics::components::PhysicsMaterial::ice();
    ImGui::SameLine();
    if (ImGui::Button("Wood")) material = physics::components::PhysicsMaterial::wood();
#endif
}

void PhysicsDebugPanel::render_physics_formulas() {
#ifdef ECSCOPE_HAS_GRAPHICS
    ImGui::Text("Physics Formulas and Equations");
    ImGui::Separator();
    
    ImGui::Text("Newton's Second Law:");
    ImGui::Text("  F = ma");
    ImGui::Text("  Force = mass × acceleration");
    
    ImGui::Text("Kinetic Energy:");
    ImGui::Text("  KE = ½mv² + ½Iω²");
    ImGui::Text("  Linear KE + Rotational KE");
    
    ImGui::Text("Collision Response:");
    ImGui::Text("  v' = v + (1+e)(vrel⋅n)n/m");
    ImGui::Text("  Where e is restitution coefficient");
    
    // More formulas would be added here
#endif
}

void PhysicsDebugPanel::render_real_world_examples() {
#ifdef ECSCOPE_HAS_GRAPHICS
    ImGui::Text("Real World Physics Examples");
    ImGui::Separator();
    
    ImGui::TextWrapped("Bouncing Ball: A rubber ball dropped from height demonstrates restitution coefficient. "
                      "The ratio of bounce height to drop height equals the square of the restitution coefficient.");
    
    ImGui::TextWrapped("Car Braking: When a car brakes, friction between tires and road provides the stopping force. "
                      "The friction coefficient determines how quickly the car can stop.");
    
    ImGui::TextWrapped("Pendulum Motion: A pendulum converts between kinetic and potential energy, "
                      "demonstrating conservation of energy in the absence of friction.");
    
    // More examples would be added here
#endif
}

void PhysicsDebugPanel::setup_experiment(const std::string& experiment_name) {
    // TODO: Set up the specified physics experiment
    ECSCOPE_LOG_INFO("Setting up experiment: %s", experiment_name.c_str());
}

void PhysicsDebugPanel::validate_ui_state() {
    // Clamp values to reasonable ranges
    ui_state_.camera_zoom = std::clamp(ui_state_.camera_zoom, 0.1f, 10.0f);
    controls_.time_scale = std::clamp(controls_.time_scale, 0.01f, 10.0f);
    controls_.target_fps = std::clamp(controls_.target_fps, 1.0f, 240.0f);
    
    // Validate entity selection
    if (!is_valid_entity(inspector_.selected_entity)) {
        inspector_.entity_valid = false;
    }
}

void PhysicsDebugPanel::clear_cache() noexcept {
    cached_entities_.clear();
    cache_update_timer_ = 0.0;
}

u32 PhysicsDebugPanel::lerp_color(u32 color_a, u32 color_b, f32 t) const {
    t = std::clamp(t, 0.0f, 1.0f);
    
    u8 r_a = (color_a >> 24) & 0xFF;
    u8 g_a = (color_a >> 16) & 0xFF;
    u8 b_a = (color_a >> 8) & 0xFF;
    u8 a_a = color_a & 0xFF;
    
    u8 r_b = (color_b >> 24) & 0xFF;
    u8 g_b = (color_b >> 16) & 0xFF;
    u8 b_b = (color_b >> 8) & 0xFF;
    u8 a_b = color_b & 0xFF;
    
    u8 r = static_cast<u8>(r_a + t * (r_b - r_a));
    u8 g = static_cast<u8>(g_a + t * (g_b - g_a));
    u8 b = static_cast<u8>(b_a + t * (b_b - b_a));
    u8 a = static_cast<u8>(a_a + t * (a_b - a_a));
    
    return (r << 24) | (g << 16) | (b << 8) | a;
}

std::string PhysicsDebugPanel::format_performance_rating(f32 frame_time) const {
    f32 target_time = 1.0f / controls_.target_fps;
    
    if (frame_time < target_time * 0.8f) {
        return "Excellent";
    } else if (frame_time < target_time * 1.2f) {
        return "Good";
    } else if (frame_time < target_time * 1.5f) {
        return "Fair";
    } else {
        return "Poor";
    }
}

} // namespace ecscope::ui