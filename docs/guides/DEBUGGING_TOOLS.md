# ECScope Debugging Tools

**Comprehensive debugging and visualization capabilities for understanding system behavior and optimizing performance**

## Table of Contents

1. [Debugging System Overview](#debugging-system-overview)
2. [Interactive Debug Panels](#interactive-debug-panels)
3. [Visual Debugging Tools](#visual-debugging-tools)
4. [Performance Analysis Tools](#performance-analysis-tools)
5. [Memory Debugging](#memory-debugging)
6. [Physics Debugging](#physics-debugging)
7. [Rendering Debugging](#rendering-debugging)
8. [Advanced Debugging Techniques](#advanced-debugging-techniques)

## Debugging System Overview

### Philosophy

ECScope's debugging system is built on the principle of "making the invisible visible." Complex system behaviors, memory patterns, and performance characteristics are visualized in real-time, enabling deep understanding and effective optimization.

### Debug Architecture

```cpp
class DebugSystem {
    // Core debugging infrastructure
    std::unique_ptr<UIOverlay> ui_overlay_;
    std::vector<std::unique_ptr<DebugPanel>> debug_panels_;
    std::unique_ptr<VisualDebugRenderer> visual_renderer_;
    
    // Debug configuration
    DebugConfig config_;
    
    // Performance monitoring
    std::unique_ptr<PerformanceProfiler> profiler_;
    std::unique_ptr<MemoryTracker> memory_tracker_;
    
public:
    void initialize(Registry& registry) {
        // Initialize UI overlay
        ui_overlay_ = std::make_unique<UIOverlay>();
        ui_overlay_->initialize();
        
        // Create debug panels
        create_debug_panels(registry);
        
        // Initialize visual debugging
        visual_renderer_ = std::make_unique<VisualDebugRenderer>();
        visual_renderer_->initialize();
        
        // Setup performance monitoring
        setup_performance_monitoring();
        
        log_info("Debug system initialized with {} panels", debug_panels_.size());
    }
    
    void update(Registry& registry, f32 delta_time) {
        // Update performance monitoring
        profiler_->update(delta_time);
        memory_tracker_->update();
        
        // Update debug panels
        for (auto& panel : debug_panels_) {
            panel->update(registry, delta_time);
        }
        
        // Update visual debugging
        visual_renderer_->update(registry, delta_time);
    }
    
    void render(Registry& registry) {
        ui_overlay_->begin_frame();
        
        // Render debug panels
        for (auto& panel : debug_panels_) {
            panel->render(registry);
        }
        
        // Render performance overlay
        render_performance_overlay();
        
        ui_overlay_->end_frame();
        
        // Render visual debug elements
        visual_renderer_->render(registry);
    }
    
private:
    void create_debug_panels(Registry& registry) {
        // ECS inspection
        debug_panels_.push_back(std::make_unique<ECSInspectorPanel>());
        debug_panels_.push_back(std::make_unique<EntityBrowserPanel>());
        debug_panels_.push_back(std::make_unique<ArchetypeAnalyzerPanel>());
        
        // Memory analysis
        debug_panels_.push_back(std::make_unique<MemoryAnalysisPanel>());
        debug_panels_.push_back(std::make_unique<AllocatorComparisonPanel>());
        debug_panels_.push_back(std::make_unique<CacheBehaviorPanel>());
        
        // Performance monitoring
        debug_panels_.push_back(std::make_unique<PerformanceLabPanel>());
        debug_panels_.push_back(std::make_unique<SystemProfilingPanel>());
        debug_panels_.push_back(std::make_unique<BenchmarkPanel>());
        
        // Physics debugging
        debug_panels_.push_back(std::make_unique<PhysicsDebugPanel>());
        debug_panels_.push_back(std::make_unique<CollisionVisualizationPanel>());
        debug_panels_.push_back(std::make_unique<ConstraintAnalysisPanel>());
        
        // Rendering debugging
        debug_panels_.push_back(std::make_unique<RenderingDebugPanel>());
        debug_panels_.push_back(std::make_unique<BatchAnalysisPanel>());
        debug_panels_.push_back(std::make_unique<GPUMemoryPanel>());
    }
};
```

## Interactive Debug Panels

### 1. ECS Inspector Panel

**Purpose**: Real-time inspection and modification of ECS state.

```cpp
class ECSInspectorPanel : public DebugPanel {
    EntityID selected_entity_{EntityID::INVALID};
    std::string entity_search_filter_;
    bool show_component_details_{true};
    
public:
    void render(Registry& registry) override {
        if (ImGui::Begin("ECS Inspector")) {
            render_entity_list(registry);
            ImGui::Separator();
            render_selected_entity_details(registry);
            ImGui::Separator();
            render_archetype_information(registry);
        }
        ImGui::End();
    }
    
private:
    void render_entity_list(Registry& registry) {
        ImGui::Text("Entities (%zu active)", registry.get_entity_count());
        
        // Search filter
        ImGui::InputText("Filter", entity_search_filter_.data(), entity_search_filter_.size());
        
        // Entity list with component summary
        if (ImGui::BeginListBox("##entities", ImVec2(-1, 200))) {
            auto all_entities = registry.get_all_entities();
            
            for (const EntityID entity : all_entities) {
                // Apply search filter
                const std::string entity_name = std::format("Entity {}", entity.value);
                if (!entity_search_filter_.empty() && 
                    entity_name.find(entity_search_filter_) == std::string::npos) {
                    continue;
                }
                
                // Show entity with component summary
                const auto component_mask = registry.get_component_signature(entity);
                const std::string component_summary = format_component_mask(component_mask);
                
                const std::string display_text = std::format("{} [{}]", entity_name, component_summary);
                
                if (ImGui::Selectable(display_text.c_str(), selected_entity_ == entity)) {
                    selected_entity_ = entity;
                }
                
                // Context menu for entity operations
                if (ImGui::BeginPopupContextItem()) {
                    if (ImGui::MenuItem("Delete Entity")) {
                        registry.destroy_entity(entity);
                        if (selected_entity_ == entity) {
                            selected_entity_ = EntityID::INVALID;
                        }
                    }
                    
                    if (ImGui::MenuItem("Clone Entity")) {
                        clone_entity(registry, entity);
                    }
                    
                    ImGui::EndPopup();
                }
            }
            
            ImGui::EndListBox();
        }
        
        // Entity creation tools
        if (ImGui::Button("Create Empty Entity")) {
            selected_entity_ = registry.create_entity();
        }
        
        ImGui::SameLine();
        if (ImGui::Button("Create Test Entity")) {
            create_test_entity(registry);
        }
    }
    
    void render_selected_entity_details(Registry& registry) {
        if (selected_entity_ == EntityID::INVALID) {
            ImGui::Text("No entity selected");
            return;
        }
        
        if (!registry.is_entity_valid(selected_entity_)) {
            ImGui::TextColored({1,0,0,1}, "Selected entity is invalid!");
            selected_entity_ = EntityID::INVALID;
            return;
        }
        
        ImGui::Text("Entity %u Details:", selected_entity_.value);
        
        // Component management
        render_component_list(registry, selected_entity_);
        
        // Add component interface
        ImGui::Separator();
        render_add_component_ui(registry, selected_entity_);
    }
    
    void render_component_list(Registry& registry, EntityID entity) {
        // Transform component (if present)
        if (auto* transform = registry.get_component<Transform>(entity)) {
            if (ImGui::CollapsingHeader("Transform")) {
                bool modified = false;
                modified |= ImGui::DragFloat2("Position", &transform->position.x, 0.1f);
                modified |= ImGui::DragFloat("Rotation", &transform->rotation, 0.01f);
                modified |= ImGui::DragFloat2("Scale", &transform->scale.x, 0.1f);
                
                if (modified) {
                    log_debug("Transform component modified for entity {}", entity.value);
                }
                
                // Remove component button
                if (ImGui::Button("Remove Transform")) {
                    registry.remove_component<Transform>(entity);
                }
            }
        }
        
        // RigidBody component (if present)
        if (auto* rigidbody = registry.get_component<RigidBody>(entity)) {
            if (ImGui::CollapsingHeader("RigidBody")) {
                ImGui::DragFloat2("Velocity", &rigidbody->velocity.x, 0.1f);
                ImGui::DragFloat("Angular Velocity", &rigidbody->angular_velocity, 0.01f);
                ImGui::DragFloat("Mass", &rigidbody->mass, 0.1f, 0.1f, 100.0f);
                ImGui::SliderFloat("Restitution", &rigidbody->restitution, 0.0f, 1.0f);
                ImGui::SliderFloat("Friction", &rigidbody->friction, 0.0f, 1.0f);
                
                ImGui::Checkbox("Is Static", &rigidbody->is_static);
                ImGui::Checkbox("Enable Gravity", &rigidbody->enable_gravity);
                
                // Real-time physics properties
                ImGui::Separator();
                ImGui::Text("Derived Properties:");
                ImGui::Text("  Speed: %.2f m/s", rigidbody->velocity.magnitude());
                ImGui::Text("  Kinetic Energy: %.2f J", rigidbody->get_kinetic_energy());
                ImGui::Text("  Inv Mass: %.4f", rigidbody->inv_mass);
                
                if (ImGui::Button("Remove RigidBody")) {
                    registry.remove_component<RigidBody>(entity);
                }
            }
        }
        
        // Add other component types...
        render_collider_component(registry, entity);
        render_sprite_component(registry, entity);
        render_custom_components(registry, entity);
    }
    
    void render_add_component_ui(Registry& registry, EntityID entity) {
        ImGui::Text("Add Component:");
        
        if (ImGui::Button("Add Transform") && !registry.has_component<Transform>(entity)) {
            registry.add_component<Transform>(entity, Transform{});
        }
        
        ImGui::SameLine();
        if (ImGui::Button("Add RigidBody") && !registry.has_component<RigidBody>(entity)) {
            registry.add_component<RigidBody>(entity, RigidBody{});
        }
        
        ImGui::SameLine();
        if (ImGui::Button("Add CircleCollider") && !registry.has_component<CircleCollider>(entity)) {
            registry.add_component<CircleCollider>(entity, CircleCollider{1.0f});
        }
        
        // Component templates
        ImGui::Separator();
        ImGui::Text("Component Templates:");
        
        if (ImGui::Button("Physics Ball")) {
            add_physics_ball_template(registry, entity);
        }
        
        ImGui::SameLine();
        if (ImGui::Button("Static Platform")) {
            add_static_platform_template(registry, entity);
        }
    }
};
```

### 2. Memory Analysis Panel

**Purpose**: Real-time memory usage monitoring and optimization guidance.

```cpp
class MemoryAnalysisPanel : public DebugPanel {
    MemorySnapshot previous_snapshot_;
    f32 update_interval_{0.1f};
    f32 time_since_update_{0.0f};
    
public:
    void render(Registry& registry) override {
        if (ImGui::Begin("Memory Analysis")) {
            render_memory_overview(registry);
            ImGui::Separator();
            render_allocator_breakdown(registry);
            ImGui::Separator();
            render_memory_trends();
            ImGui::Separator();
            render_optimization_recommendations(registry);
        }
        ImGui::End();
    }
    
    void update(Registry& registry, f32 delta_time) override {
        time_since_update_ += delta_time;
        
        if (time_since_update_ >= update_interval_) {
            update_memory_snapshot(registry);
            time_since_update_ = 0.0f;
        }
    }
    
private:
    void render_memory_overview(Registry& registry) {
        const auto current_snapshot = capture_memory_snapshot(registry);
        
        ImGui::Text("Memory Overview");
        ImGui::Text("  Total Allocated: %.2f MB", 
                   current_snapshot.total_allocated / (1024.0f * 1024.0f));
        ImGui::Text("  Total Used: %.2f MB", 
                   current_snapshot.total_used / (1024.0f * 1024.0f));
        ImGui::Text("  Fragmentation: %.1f%%", current_snapshot.fragmentation_percentage);
        ImGui::Text("  Active Entities: %zu", current_snapshot.entity_count);
        ImGui::Text("  Component Arrays: %zu", current_snapshot.component_array_count);
        
        // Memory efficiency indicators
        const f32 efficiency = (current_snapshot.total_used / current_snapshot.total_allocated) * 100.0f;
        if (efficiency > 90.0f) {
            ImGui::TextColored({0,1,0,1}, "  Efficiency: %.1f%% (Excellent)", efficiency);
        } else if (efficiency > 70.0f) {
            ImGui::TextColored({1,1,0,1}, "  Efficiency: %.1f%% (Good)", efficiency);
        } else {
            ImGui::TextColored({1,0,0,1}, "  Efficiency: %.1f%% (Poor)", efficiency);
        }
    }
    
    void render_allocator_breakdown(Registry& registry) {
        ImGui::Text("Allocator Breakdown");
        
        const auto allocator_stats = registry.get_allocator_statistics();
        
        // Create pie chart data
        std::vector<f32> allocator_usage;
        std::vector<const char*> allocator_names;
        std::vector<ImU32> allocator_colors;
        
        for (const auto& [name, stats] : allocator_stats) {
            allocator_usage.push_back(static_cast<f32>(stats.bytes_allocated));
            allocator_names.push_back(name.c_str());
            allocator_colors.push_back(hash_to_color(std::hash<std::string>{}(name)));
        }
        
        // Custom pie chart rendering
        render_memory_pie_chart(allocator_usage, allocator_names, allocator_colors);
        
        // Detailed breakdown table
        if (ImGui::BeginTable("AllocatorDetails", 4, ImGuiTableFlags_Borders)) {
            ImGui::TableSetupColumn("Allocator");
            ImGui::TableSetupColumn("Allocated");
            ImGui::TableSetupColumn("Used");
            ImGui::TableSetupColumn("Efficiency");
            ImGui::TableHeadersRow();
            
            for (const auto& [name, stats] : allocator_stats) {
                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::Text("%s", name.c_str());
                
                ImGui::TableNextColumn();
                ImGui::Text("%.2f MB", stats.bytes_allocated / (1024.0f * 1024.0f));
                
                ImGui::TableNextColumn();
                ImGui::Text("%.2f MB", stats.bytes_used / (1024.0f * 1024.0f));
                
                ImGui::TableNextColumn();
                const f32 efficiency = (stats.bytes_used / stats.bytes_allocated) * 100.0f;
                if (efficiency > 80.0f) {
                    ImGui::TextColored({0,1,0,1}, "%.1f%%", efficiency);
                } else if (efficiency > 60.0f) {
                    ImGui::TextColored({1,1,0,1}, "%.1f%%", efficiency);
                } else {
                    ImGui::TextColored({1,0,0,1}, "%.1f%%", efficiency);
                }
            }
            
            ImGui::EndTable();
        }
    }
    
    void render_memory_trends() {
        ImGui::Text("Memory Trends");
        
        // Get historical data
        const auto& memory_history = memory_tracker_->get_history();
        
        std::vector<f32> total_usage;
        std::vector<f32> fragmentation;
        std::vector<f32> allocation_rate;
        
        for (const auto& snapshot : memory_history) {
            total_usage.push_back(snapshot.total_used / (1024.0f * 1024.0f));
            fragmentation.push_back(snapshot.fragmentation_percentage);
            allocation_rate.push_back(snapshot.allocations_per_second);
        }
        
        // Memory usage over time
        ImGui::PlotLines("Memory Usage (MB)", total_usage.data(), total_usage.size(),
                        0, nullptr, 0.0f, 0.0f, ImVec2(0, 80));
        
        // Fragmentation over time
        ImGui::PlotLines("Fragmentation (%)", fragmentation.data(), fragmentation.size(),
                        0, nullptr, 0.0f, 100.0f, ImVec2(0, 80));
        
        // Allocation rate
        ImGui::PlotLines("Allocs/sec", allocation_rate.data(), allocation_rate.size(),
                        0, nullptr, 0.0f, 0.0f, ImVec2(0, 80));
    }
    
    void render_optimization_recommendations(Registry& registry) {
        ImGui::Text("Optimization Recommendations");
        
        const auto current_snapshot = capture_memory_snapshot(registry);
        
        if (current_snapshot.fragmentation_percentage > 20.0f) {
            ImGui::TextColored({1,0.5f,0,1}, "• High fragmentation detected");
            ImGui::Text("  Consider using arena allocators for frequently allocated objects");
            
            if (ImGui::Button("Apply Arena Optimization")) {
                apply_arena_optimization_suggestion(registry);
            }
        }
        
        if (current_snapshot.total_allocated > current_snapshot.total_used * 2) {
            ImGui::TextColored({1,0.5f,0,1}, "• Excessive memory overhead detected");
            ImGui::Text("  Review allocator sizing and consider pool allocators");
            
            if (ImGui::Button("Analyze Pool Opportunities")) {
                analyze_pool_allocator_opportunities(registry);
            }
        }
        
        const f32 cache_efficiency = estimate_cache_efficiency(registry);
        if (cache_efficiency < 0.7f) {
            ImGui::TextColored({1,0.5f,0,1}, "• Poor cache efficiency detected");
            ImGui::Text("  Consider reorganizing component access patterns");
            
            if (ImGui::Button("Suggest Access Optimizations")) {
                suggest_access_pattern_optimizations(registry);
            }
        }
        
        // Positive feedback
        if (current_snapshot.fragmentation_percentage < 5.0f && 
            cache_efficiency > 0.85f) {
            ImGui::TextColored({0,1,0,1}, "• Excellent memory optimization!");
            ImGui::Text("  Current configuration demonstrates best practices");
        }
    }
};
```

### 3. Physics Debug Panel

**Purpose**: Interactive physics system debugging and parameter modification.

```cpp
class PhysicsDebugPanel : public DebugPanel {
    bool enable_collision_visualization_{true};
    bool enable_constraint_visualization_{true};
    bool enable_velocity_vectors_{false};
    bool enable_force_vectors_{false};
    bool step_by_step_mode_{false};
    
    EntityID debug_focus_entity_{EntityID::INVALID};
    
public:
    void render(Registry& registry) override {
        if (ImGui::Begin("Physics Debug")) {
            render_simulation_controls(registry);
            ImGui::Separator();
            render_visualization_options(registry);
            ImGui::Separator();
            render_physics_statistics(registry);
            ImGui::Separator();
            render_entity_focus_tools(registry);
        }
        ImGui::End();
    }
    
private:
    void render_simulation_controls(Registry& registry) {
        ImGui::Text("Simulation Controls");
        
        auto* physics_world = registry.get_system<PhysicsSystem>()->get_world();
        
        // Gravity control
        Vec2 gravity = physics_world->get_gravity();
        if (ImGui::DragFloat2("Gravity", &gravity.x, 0.1f, -50.0f, 50.0f)) {
            physics_world->set_gravity(gravity);
        }
        
        // Time scale
        f32 time_scale = physics_world->get_time_scale();
        if (ImGui::SliderFloat("Time Scale", &time_scale, 0.0f, 2.0f)) {
            physics_world->set_time_scale(time_scale);
        }
        
        // Iteration counts
        u32 velocity_iterations = physics_world->get_velocity_iterations();
        if (ImGui::SliderInt("Velocity Iterations", reinterpret_cast<int*>(&velocity_iterations), 1, 20)) {
            physics_world->set_velocity_iterations(velocity_iterations);
        }
        
        u32 position_iterations = physics_world->get_position_iterations();
        if (ImGui::SliderInt("Position Iterations", reinterpret_cast<int*>(&position_iterations), 1, 10)) {
            physics_world->set_position_iterations(position_iterations);
        }
        
        // Step-by-step debugging
        ImGui::Checkbox("Step-by-Step Mode", &step_by_step_mode_);
        physics_world->set_step_by_step_mode(step_by_step_mode_);
        
        if (step_by_step_mode_) {
            if (ImGui::Button("Next Step")) {
                physics_world->execute_single_step();
            }
            
            ImGui::SameLine();
            ImGui::Text("Current Phase: %s", physics_world->get_current_phase_name());
        }
    }
    
    void render_visualization_options(Registry& registry) {
        ImGui::Text("Visualization Options");
        
        ImGui::Checkbox("Collision Shapes", &enable_collision_visualization_);
        ImGui::Checkbox("Constraints/Joints", &enable_constraint_visualization_);
        ImGui::Checkbox("Velocity Vectors", &enable_velocity_vectors_);
        ImGui::Checkbox("Force Vectors", &enable_force_vectors_);
        
        // Apply visualization settings
        auto* debug_renderer = registry.get_system<PhysicsSystem>()->get_debug_renderer();
        debug_renderer->set_collision_visualization(enable_collision_visualization_);
        debug_renderer->set_constraint_visualization(enable_constraint_visualization_);
        debug_renderer->set_velocity_visualization(enable_velocity_vectors_);
        debug_renderer->set_force_visualization(enable_force_vectors_);
        
        // Visualization colors and styles
        ImGui::Separator();
        static Vec4 collision_color{0.0f, 1.0f, 0.0f, 0.5f};
        static Vec4 contact_color{1.0f, 0.0f, 0.0f, 1.0f};
        static Vec4 constraint_color{1.0f, 1.0f, 0.0f, 1.0f};
        
        ImGui::ColorEdit4("Collision Color", &collision_color.x);
        ImGui::ColorEdit4("Contact Color", &contact_color.x);
        ImGui::ColorEdit4("Constraint Color", &constraint_color.x);
        
        debug_renderer->set_collision_color(collision_color);
        debug_renderer->set_contact_color(contact_color);
        debug_renderer->set_constraint_color(constraint_color);
    }
    
    void render_physics_statistics(Registry& registry) {
        ImGui::Text("Physics Statistics");
        
        const auto* physics_system = registry.get_system<PhysicsSystem>();
        const auto stats = physics_system->get_statistics();
        
        ImGui::Text("Performance:");
        ImGui::Text("  Update Time: %.3f ms", stats.last_update_time_ms);
        ImGui::Text("  Broad Phase: %.3f ms", stats.broad_phase_time_ms);
        ImGui::Text("  Narrow Phase: %.3f ms", stats.narrow_phase_time_ms);
        ImGui::Text("  Constraint Solving: %.3f ms", stats.constraint_solve_time_ms);
        ImGui::Text("  Integration: %.3f ms", stats.integration_time_ms);
        
        ImGui::Text("Simulation:");
        ImGui::Text("  Active Bodies: %u", stats.active_rigidbodies);
        ImGui::Text("  Active Colliders: %u", stats.active_colliders);
        ImGui::Text("  Collision Pairs Tested: %u", stats.collision_pairs_tested);
        ImGui::Text("  Active Constraints: %u", stats.active_constraints);
        ImGui::Text("  Spatial Grid Cells: %u", stats.spatial_grid_active_cells);
        
        // Performance indicators
        ImGui::Separator();
        ImGui::Text("Performance Indicators:");
        
        const f32 target_frame_time = 16.67f; // 60 FPS
        if (stats.last_update_time_ms > target_frame_time) {
            ImGui::TextColored({1,0,0,1}, "• Physics exceeding frame budget!");
        } else if (stats.last_update_time_ms > target_frame_time * 0.5f) {
            ImGui::TextColored({1,1,0,1}, "• Physics using significant frame time");
        } else {
            ImGui::TextColored({0,1,0,1}, "• Physics performance excellent");
        }
        
        // Optimization suggestions
        if (stats.collision_pairs_tested > stats.active_rigidbodies * stats.active_rigidbodies * 0.1f) {
            ImGui::TextColored({1,0.5f,0,1}, "• Consider spatial optimization improvements");
        }
        
        if (stats.active_constraints > stats.active_rigidbodies * 5) {
            ImGui::TextColored({1,0.5f,0,1}, "• High constraint density - review iteration counts");
        }
    }
    
    void render_entity_focus_tools(Registry& registry) {
        ImGui::Text("Entity Focus Tools");
        
        // Entity selection
        const auto physics_entities = registry.view<Transform, RigidBody>().entities();
        
        if (ImGui::BeginCombo("Focus Entity", 
                             (debug_focus_entity_ != EntityID::INVALID) ? 
                             std::format("Entity {}", debug_focus_entity_.value).c_str() : 
                             "None")) {
            
            if (ImGui::Selectable("None", debug_focus_entity_ == EntityID::INVALID)) {
                debug_focus_entity_ = EntityID::INVALID;
            }
            
            for (const EntityID entity : physics_entities) {
                const bool selected = (debug_focus_entity_ == entity);
                const std::string entity_name = std::format("Entity {}", entity.value);
                
                if (ImGui::Selectable(entity_name.c_str(), selected)) {
                    debug_focus_entity_ = entity;
                }
            }
            
            ImGui::EndCombo();
        }
        
        // Focus entity details
        if (debug_focus_entity_ != EntityID::INVALID) {
            auto* transform = registry.get_component<Transform>(debug_focus_entity_);
            auto* rigidbody = registry.get_component<RigidBody>(debug_focus_entity_);
            
            if (transform && rigidbody) {
                ImGui::Separator();
                ImGui::Text("Focused Entity Details:");
                
                ImGui::Text("  Position: (%.2f, %.2f)", transform->position.x, transform->position.y);
                ImGui::Text("  Velocity: (%.2f, %.2f)", rigidbody->velocity.x, rigidbody->velocity.y);
                ImGui::Text("  Speed: %.2f m/s", rigidbody->velocity.magnitude());
                ImGui::Text("  Angular Velocity: %.2f rad/s", rigidbody->angular_velocity);
                ImGui::Text("  Kinetic Energy: %.2f J", rigidbody->get_kinetic_energy());
                
                // Interactive force application
                ImGui::Separator();
                ImGui::Text("Apply Forces:");
                
                static Vec2 force_to_apply{0.0f, 0.0f};
                ImGui::DragFloat2("Force Vector", &force_to_apply.x, 0.1f, -100.0f, 100.0f);
                
                if (ImGui::Button("Apply Force")) {
                    rigidbody->apply_force(force_to_apply);
                    log_info("Applied force ({:.2f}, {:.2f}) to entity {}", 
                            force_to_apply.x, force_to_apply.y, debug_focus_entity_.value);
                }
                
                ImGui::SameLine();
                if (ImGui::Button("Apply Impulse")) {
                    rigidbody->apply_impulse(force_to_apply);
                    log_info("Applied impulse ({:.2f}, {:.2f}) to entity {}", 
                            force_to_apply.x, force_to_apply.y, debug_focus_entity_.value);
                }
                
                // Reset entity
                if (ImGui::Button("Reset Entity")) {
                    rigidbody->velocity = Vec2{0.0f, 0.0f};
                    rigidbody->angular_velocity = 0.0f;
                    rigidbody->force = Vec2{0.0f, 0.0f};
                    rigidbody->torque = 0.0f;
                    transform->position = Vec2{0.0f, 5.0f}; // Reset to starting position
                }
            }
        }
    }
};
```

## Visual Debugging Tools

### Collision Visualization

```cpp
class CollisionVisualizer {
    std::vector<CollisionInfo> active_collisions_;
    std::vector<ContactPoint> contact_points_;
    bool show_collision_normals_{true};
    bool show_penetration_depth_{true};
    bool show_contact_points_{true};
    
public:
    void update(Registry& registry) {
        active_collisions_.clear();
        contact_points_.clear();
        
        // Collect collision information from physics system
        auto* physics_system = registry.get_system<PhysicsSystem>();
        const auto& collision_results = physics_system->get_collision_results();
        
        for (const auto& collision : collision_results) {
            if (collision.is_colliding) {
                active_collisions_.push_back(collision);
                
                // Create contact points for visualization
                for (const auto& contact : collision.contact_points) {
                    contact_points_.push_back(contact);
                }
            }
        }
    }
    
    void render_visual_debug(DebugRenderer& debug_renderer) {
        // Render collision shapes
        render_collision_shapes(debug_renderer);
        
        // Render collision information
        if (show_collision_normals_) {
            render_collision_normals(debug_renderer);
        }
        
        if (show_penetration_depth_) {
            render_penetration_visualization(debug_renderer);
        }
        
        if (show_contact_points_) {
            render_contact_points(debug_renderer);
        }
    }
    
private:
    void render_collision_normals(DebugRenderer& debug_renderer) {
        for (const auto& collision : active_collisions_) {
            const Vec2 normal_start = collision.contact_point;
            const Vec2 normal_end = normal_start + collision.normal * 2.0f;
            
            // Draw normal vector
            debug_renderer.draw_arrow(normal_start, normal_end, Color::RED, 0.1f);
            
            // Draw normal label
            const std::string normal_text = std::format("N({:.2f},{:.2f})", 
                                                       collision.normal.x, collision.normal.y);
            debug_renderer.draw_text(normal_end + Vec2{0.2f, 0.2f}, normal_text, Color::WHITE);
        }
    }
    
    void render_penetration_visualization(DebugRenderer& debug_renderer) {
        for (const auto& collision : active_collisions_) {
            if (collision.penetration_depth > 0.001f) {
                const Vec2 penetration_start = collision.contact_point;
                const Vec2 penetration_end = penetration_start + 
                                           collision.normal * collision.penetration_depth;
                
                // Draw penetration vector
                debug_renderer.draw_line(penetration_start, penetration_end, 
                                       Color::ORANGE, 3.0f);
                
                // Draw penetration depth text
                const std::string depth_text = std::format("Pen: {:.3f}", 
                                                          collision.penetration_depth);
                debug_renderer.draw_text(penetration_end, depth_text, Color::ORANGE);
            }
        }
    }
    
    void render_contact_points(DebugRenderer& debug_renderer) {
        for (const auto& contact : contact_points_) {
            // Draw contact point
            debug_renderer.draw_circle(contact.position, 0.1f, Color::YELLOW, true);
            
            // Draw contact impulse if significant
            if (contact.normal_impulse > 0.1f) {
                const Vec2 impulse_end = contact.position + 
                                       contact.normal * contact.normal_impulse * 0.5f;
                debug_renderer.draw_arrow(contact.position, impulse_end, Color::CYAN, 0.05f);
                
                const std::string impulse_text = std::format("I: {:.2f}", contact.normal_impulse);
                debug_renderer.draw_text(impulse_end + Vec2{0.1f, 0.1f}, impulse_text, Color::CYAN);
            }
        }
    }
};
```

### Memory Access Pattern Visualization

```cpp
class MemoryAccessVisualizer {
    struct AccessEvent {
        void* address;
        usize size;
        f32 timestamp;
        AccessType type; // Read, Write, Allocate, Deallocate
        const char* source_location;
    };
    
    CircularBuffer<AccessEvent, 10000> access_history_;
    bool recording_enabled_{false};
    f32 visualization_time_window_{1.0f}; // Show last 1 second
    
public:
    void record_access(void* address, usize size, AccessType type, 
                      const char* source_location = __builtin_FUNCTION()) {
        if (!recording_enabled_) return;
        
        access_history_.push({
            .address = address,
            .size = size,
            .timestamp = Time::get_time(),
            .type = type,
            .source_location = source_location
        });
    }
    
    void render_access_visualization() {
        if (ImGui::Begin("Memory Access Pattern Visualization")) {
            render_access_timeline();
            ImGui::Separator();
            render_address_space_map();
            ImGui::Separator();
            render_access_statistics();
        }
        ImGui::End();
    }
    
private:
    void render_access_timeline() {
        ImGui::Text("Memory Access Timeline (Last %.1fs)", visualization_time_window_);
        
        const f32 current_time = Time::get_time();
        const f32 time_range_start = current_time - visualization_time_window_;
        
        // Filter recent accesses
        std::vector<AccessEvent> recent_accesses;
        for (const auto& access : access_history_) {
            if (access.timestamp >= time_range_start) {
                recent_accesses.push_back(access);
            }
        }
        
        if (recent_accesses.empty()) {
            ImGui::Text("No recent memory accesses");
            return;
        }
        
        // Render timeline
        const ImVec2 canvas_pos = ImGui::GetCursorScreenPos();
        const ImVec2 canvas_size = ImGui::GetContentRegionAvail();
        const ImU32 timeline_bg_color = IM_COL32(50, 50, 50, 255);
        
        ImDrawList* draw_list = ImGui::GetWindowDrawList();
        draw_list->AddRectFilled(canvas_pos, 
                               ImVec2(canvas_pos.x + canvas_size.x, canvas_pos.y + 100), 
                               timeline_bg_color);
        
        // Draw access events
        for (const auto& access : recent_accesses) {
            const f32 time_offset = access.timestamp - time_range_start;
            const f32 x_pos = canvas_pos.x + (time_offset / visualization_time_window_) * canvas_size.x;
            
            ImU32 color;
            switch (access.type) {
                case AccessType::Read:       color = IM_COL32(0, 255, 0, 255); break;
                case AccessType::Write:      color = IM_COL32(255, 255, 0, 255); break;
                case AccessType::Allocate:   color = IM_COL32(0, 0, 255, 255); break;
                case AccessType::Deallocate: color = IM_COL32(255, 0, 0, 255); break;
            }
            
            draw_list->AddLine(ImVec2(x_pos, canvas_pos.y), 
                             ImVec2(x_pos, canvas_pos.y + 100), 
                             color, 2.0f);
        }
        
        // Timeline labels
        ImGui::SetCursorScreenPos(ImVec2(canvas_pos.x, canvas_pos.y + 110));
        ImGui::Text("%.2fs ago", visualization_time_window_);
        ImGui::SameLine();
        ImGui::SetCursorPosX(canvas_size.x - 50);
        ImGui::Text("Now");
        
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 20);
    }
    
    void render_address_space_map() {
        ImGui::Text("Address Space Map");
        
        // Group accesses by address ranges
        std::map<uintptr_t, std::vector<AccessEvent>> address_groups;
        
        const f32 current_time = Time::get_time();
        const f32 time_threshold = current_time - visualization_time_window_;
        
        for (const auto& access : access_history_) {
            if (access.timestamp >= time_threshold) {
                const uintptr_t aligned_address = reinterpret_cast<uintptr_t>(access.address) & 
                                                ~(64 - 1); // Align to cache line
                address_groups[aligned_address].push_back(access);
            }
        }
        
        // Render address map
        ImGui::Text("Active Memory Regions (%zu cache lines):", address_groups.size());
        
        if (ImGui::BeginTable("AddressMap", 4, ImGuiTableFlags_Borders)) {
            ImGui::TableSetupColumn("Address Range");
            ImGui::TableSetupColumn("Access Count");
            ImGui::TableSetupColumn("Primary Type");
            ImGui::TableSetupColumn("Hot/Cold");
            ImGui::TableHeadersRow();
            
            for (const auto& [base_address, accesses] : address_groups) {
                ImGui::TableNextRow();
                
                ImGui::TableNextColumn();
                ImGui::Text("0x%016" PRIxPTR, base_address);
                
                ImGui::TableNextColumn();
                ImGui::Text("%zu", accesses.size());
                
                ImGui::TableNextColumn();
                const AccessType primary_type = get_primary_access_type(accesses);
                ImGui::Text("%s", access_type_name(primary_type));
                
                ImGui::TableNextColumn();
                const f32 access_frequency = calculate_access_frequency(accesses);
                if (access_frequency > 10.0f) {
                    ImGui::TextColored({1,0,0,1}, "HOT");
                } else if (access_frequency > 1.0f) {
                    ImGui::TextColored({1,1,0,1}, "WARM");
                } else {
                    ImGui::TextColored({0,0,1,1}, "COLD");
                }
            }
            
            ImGui::EndTable();
        }
    }
};
```

## Performance Analysis Tools

### Real-Time Profiler

```cpp
class RealTimeProfiler {
    struct ProfileScope {
        std::string name;
        f32 start_time;
        f32 end_time;
        f32 duration;
        u32 call_count;
        ProfileScope* parent;
        std::vector<std::unique_ptr<ProfileScope>> children;
    };
    
    std::unique_ptr<ProfileScope> root_scope_;
    ProfileScope* current_scope_{nullptr};
    
    // Frame-based statistics
    CircularBuffer<f32, 300> frame_times_; // 5 seconds at 60 FPS
    std::unordered_map<std::string, CircularBuffer<f32, 300>> scope_times_;
    
public:
    class ScopeTimer {
        RealTimeProfiler& profiler_;
        std::string scope_name_;
        f32 start_time_;
        
    public:
        ScopeTimer(RealTimeProfiler& profiler, std::string name) 
            : profiler_(profiler), scope_name_(std::move(name)) {
            start_time_ = Time::get_time();
            profiler_.enter_scope(scope_name_);
        }
        
        ~ScopeTimer() {
            const f32 duration = Time::get_time() - start_time_;
            profiler_.exit_scope(scope_name_, duration);
        }
    };
    
    void enter_scope(const std::string& name) {
        auto new_scope = std::make_unique<ProfileScope>();
        new_scope->name = name;
        new_scope->start_time = Time::get_time();
        new_scope->parent = current_scope_;
        
        if (current_scope_) {
            current_scope_->children.push_back(std::move(new_scope));
            current_scope_ = current_scope_->children.back().get();
        } else {
            root_scope_ = std::move(new_scope);
            current_scope_ = root_scope_.get();
        }
    }
    
    void exit_scope(const std::string& name, f32 duration) {
        if (current_scope_ && current_scope_->name == name) {
            current_scope_->end_time = Time::get_time();
            current_scope_->duration = duration;
            ++current_scope_->call_count;
            
            // Update historical data
            scope_times_[name].push(duration);
            
            current_scope_ = current_scope_->parent;
        }
    }
    
    void render_profiler_ui() {
        if (ImGui::Begin("Real-Time Profiler")) {
            render_frame_timing();
            ImGui::Separator();
            render_scope_hierarchy();
            ImGui::Separator();
            render_performance_graphs();
        }
        ImGui::End();
    }
    
private:
    void render_scope_hierarchy() {
        ImGui::Text("Current Frame Profile:");
        
        if (root_scope_) {
            render_scope_tree(root_scope_.get(), 0);
        } else {
            ImGui::Text("No profiling data available");
        }
    }
    
    void render_scope_tree(ProfileScope* scope, u32 depth) {
        const std::string indent(depth * 2, ' ');
        const f32 percentage = (scope->duration / get_frame_time()) * 100.0f;
        
        // Color code by performance impact
        ImVec4 color = {1, 1, 1, 1}; // White
        if (percentage > 50.0f) {
            color = {1, 0, 0, 1}; // Red - high impact
        } else if (percentage > 20.0f) {
            color = {1, 1, 0, 1}; // Yellow - medium impact
        } else if (percentage > 5.0f) {
            color = {0, 1, 0, 1}; // Green - low impact
        }
        
        ImGui::TextColored(color, "%s%s: %.3f ms (%.1f%%)", 
                          indent.c_str(), scope->name.c_str(), 
                          scope->duration, percentage);
        
        // Render children
        for (const auto& child : scope->children) {
            render_scope_tree(child.get(), depth + 1);
        }
    }
    
    void render_performance_graphs() {
        ImGui::Text("Performance History:");
        
        // Frame time graph
        std::vector<f32> frame_time_data;
        for (const f32 frame_time : frame_times_) {
            frame_time_data.push_back(frame_time);
        }
        
        ImGui::PlotLines("Frame Time (ms)", frame_time_data.data(), frame_time_data.size(),
                        0, nullptr, 0.0f, 33.33f, ImVec2(0, 80));
        
        // Individual scope graphs
        for (const auto& [scope_name, scope_history] : scope_times_) {
            if (scope_history.size() < 10) continue; // Skip scopes with little data
            
            std::vector<f32> scope_data;
            for (const f32 scope_time : scope_history) {
                scope_data.push_back(scope_time);
            }
            
            const std::string graph_label = scope_name + " (ms)";
            ImGui::PlotLines(graph_label.c_str(), scope_data.data(), scope_data.size(),
                           0, nullptr, 0.0f, 0.0f, ImVec2(0, 60));
        }
    }
};

// RAII macro for easy profiling
#define PROFILE_SCOPE(profiler, name) \
    RealTimeProfiler::ScopeTimer _timer(profiler, name)

// Usage example:
void PhysicsSystem::update(Registry& registry, f32 delta_time) {
    PROFILE_SCOPE(debug_profiler_, "PhysicsSystem::update");
    
    {
        PROFILE_SCOPE(debug_profiler_, "Broad Phase");
        broad_phase_collision_detection(registry);
    }
    
    {
        PROFILE_SCOPE(debug_profiler_, "Narrow Phase");
        narrow_phase_collision_detection(registry);
    }
    
    {
        PROFILE_SCOPE(debug_profiler_, "Constraint Solving");
        solve_constraints(registry, delta_time);
    }
    
    {
        PROFILE_SCOPE(debug_profiler_, "Integration");
        integrate_motion(registry, delta_time);
    }
}
```

## Advanced Debugging Techniques

### Memory Leak Detection

```cpp
class MemoryLeakDetector {
    struct AllocationInfo {
        void* address;
        usize size;
        f32 timestamp;
        std::string source_location;
        std::string call_stack;
    };
    
    std::unordered_map<void*, AllocationInfo> active_allocations_;
    std::vector<AllocationInfo> leaked_allocations_;
    usize total_allocations_{0};
    usize total_deallocations_{0};
    
public:
    void record_allocation(void* address, usize size, const char* source_location) {
        AllocationInfo info{};
        info.address = address;
        info.size = size;
        info.timestamp = Time::get_time();
        info.source_location = source_location;
        info.call_stack = capture_call_stack();
        
        active_allocations_[address] = std::move(info);
        ++total_allocations_;
    }
    
    void record_deallocation(void* address) {
        if (const auto it = active_allocations_.find(address); it != active_allocations_.end()) {
            active_allocations_.erase(it);
            ++total_deallocations_;
        } else {
            log_warning("Attempted to deallocate unknown address: {}", address);
        }
    }
    
    void detect_leaks() {
        leaked_allocations_.clear();
        
        const f32 current_time = Time::get_time();
        const f32 leak_threshold = 30.0f; // 30 seconds
        
        for (const auto& [address, info] : active_allocations_) {
            if (current_time - info.timestamp > leak_threshold) {
                leaked_allocations_.push_back(info);
            }
        }
        
        if (!leaked_allocations_.empty()) {
            log_warning("Detected {} potential memory leaks", leaked_allocations_.size());
        }
    }
    
    void render_leak_detector_ui() {
        if (ImGui::Begin("Memory Leak Detector")) {
            ImGui::Text("Allocation Summary:");
            ImGui::Text("  Total Allocations: %zu", total_allocations_);
            ImGui::Text("  Total Deallocations: %zu", total_deallocations_);
            ImGui::Text("  Active Allocations: %zu", active_allocations_.size());
            ImGui::Text("  Potential Leaks: %zu", leaked_allocations_.size());
            
            if (ImGui::Button("Scan for Leaks")) {
                detect_leaks();
            }
            
            ImGui::Separator();
            
            // Display potential leaks
            if (!leaked_allocations_.empty()) {
                ImGui::TextColored({1,0,0,1}, "Potential Memory Leaks:");
                
                if (ImGui::BeginTable("MemoryLeaks", 4, ImGuiTableFlags_Borders)) {
                    ImGui::TableSetupColumn("Address");
                    ImGui::TableSetupColumn("Size");
                    ImGui::TableSetupColumn("Age (s)");
                    ImGui::TableSetupColumn("Source");
                    ImGui::TableHeadersRow();
                    
                    const f32 current_time = Time::get_time();
                    
                    for (const auto& leak : leaked_allocations_) {
                        ImGui::TableNextRow();
                        
                        ImGui::TableNextColumn();
                        ImGui::Text("%p", leak.address);
                        
                        ImGui::TableNextColumn();
                        ImGui::Text("%zu bytes", leak.size);
                        
                        ImGui::TableNextColumn();
                        ImGui::Text("%.1f", current_time - leak.timestamp);
                        
                        ImGui::TableNextColumn();
                        ImGui::Text("%s", leak.source_location.c_str());
                        
                        // Show call stack on hover
                        if (ImGui::IsItemHovered()) {
                            ImGui::SetTooltip("Call Stack:\n%s", leak.call_stack.c_str());
                        }
                    }
                    
                    ImGui::EndTable();
                }
            }
        }
        ImGui::End();
    }
    
private:
    std::string capture_call_stack() {
        // Platform-specific call stack capture
        #ifdef _WIN32
        // Windows implementation using StackWalk64
        #elif defined(__linux__)
        // Linux implementation using backtrace
        #else
        return "Call stack capture not implemented for this platform";
        #endif
    }
};
```

### Performance Bottleneck Analyzer

```cpp
class BottleneckAnalyzer {
    struct SystemProfile {
        std::string system_name;
        f32 average_time_ms;
        f32 max_time_ms;
        f32 min_time_ms;
        f32 time_percentage;
        u32 call_count;
        
        // Bottleneck indicators
        bool is_cpu_bound;
        bool is_memory_bound;
        bool is_cache_bound;
        bool has_contention;
    };
    
    std::vector<SystemProfile> system_profiles_;
    f32 analysis_window_{5.0f}; // Analyze last 5 seconds
    
public:
    void analyze_performance(const std::vector<System*>& systems) {
        system_profiles_.clear();
        
        const f32 total_frame_time = calculate_total_frame_time();
        
        for (System* system : systems) {
            SystemProfile profile{};
            profile.system_name = system->get_name();
            
            const auto timing_data = system->get_timing_history(analysis_window_);
            profile.average_time_ms = calculate_average(timing_data);
            profile.max_time_ms = *std::max_element(timing_data.begin(), timing_data.end());
            profile.min_time_ms = *std::min_element(timing_data.begin(), timing_data.end());
            profile.time_percentage = (profile.average_time_ms / total_frame_time) * 100.0f;
            profile.call_count = timing_data.size();
            
            // Analyze bottleneck characteristics
            analyze_bottleneck_type(profile, system);
            
            system_profiles_.push_back(profile);
        }
        
        // Sort by time percentage (worst first)
        std::sort(system_profiles_.begin(), system_profiles_.end(),
                 [](const SystemProfile& a, const SystemProfile& b) {
                     return a.time_percentage > b.time_percentage;
                 });
    }
    
    void render_bottleneck_analysis() {
        if (ImGui::Begin("Bottleneck Analysis")) {
            ImGui::Text("System Performance Analysis (Last %.1fs)", analysis_window_);
            
            if (ImGui::BeginTable("SystemBottlenecks", 6, ImGuiTableFlags_Borders | ImGuiTableFlags_Sortable)) {
                ImGui::TableSetupColumn("System", ImGuiTableColumnFlags_DefaultSort);
                ImGui::TableSetupColumn("Avg Time (ms)");
                ImGui::TableSetupColumn("% of Frame");
                ImGui::TableSetupColumn("Max Time (ms)");
                ImGui::TableSetupColumn("Bottleneck Type");
                ImGui::TableSetupColumn("Optimization");
                ImGui::TableHeadersRow();
                
                for (const auto& profile : system_profiles_) {
                    ImGui::TableNextRow();
                    
                    ImGui::TableNextColumn();
                    // Color-code by performance impact
                    ImVec4 name_color = {1, 1, 1, 1};
                    if (profile.time_percentage > 30.0f) {
                        name_color = {1, 0, 0, 1}; // Red - major bottleneck
                    } else if (profile.time_percentage > 10.0f) {
                        name_color = {1, 1, 0, 1}; // Yellow - minor bottleneck
                    }
                    ImGui::TextColored(name_color, "%s", profile.system_name.c_str());
                    
                    ImGui::TableNextColumn();
                    ImGui::Text("%.3f", profile.average_time_ms);
                    
                    ImGui::TableNextColumn();
                    ImGui::Text("%.1f%%", profile.time_percentage);
                    
                    ImGui::TableNextColumn();
                    ImGui::Text("%.3f", profile.max_time_ms);
                    
                    ImGui::TableNextColumn();
                    ImGui::Text("%s", get_bottleneck_type_string(profile));
                    
                    ImGui::TableNextColumn();
                    if (ImGui::Button(std::format("Optimize##{}", profile.system_name).c_str())) {
                        suggest_optimization_strategy(profile);
                    }
                }
                
                ImGui::EndTable();
            }
            
            // Bottleneck visualization
            ImGui::Separator();
            render_bottleneck_visualization();
        }
        ImGui::End();
    }
    
private:
    void analyze_bottleneck_type(SystemProfile& profile, System* system) {
        // CPU bound analysis
        const f32 variance = calculate_variance(system->get_timing_history(analysis_window_));
        profile.is_cpu_bound = (variance < profile.average_time_ms * 0.1f); // Low variance indicates CPU bound
        
        // Memory bound analysis
        const auto memory_stats = system->get_memory_statistics();
        profile.is_memory_bound = (memory_stats.cache_miss_ratio > 0.1f);
        
        // Cache bound analysis
        profile.is_cache_bound = (memory_stats.cache_line_utilization < 0.5f);
        
        // Contention analysis (for multi-threaded systems)
        profile.has_contention = (system->get_contention_ratio() > 0.05f);
    }
    
    const char* get_bottleneck_type_string(const SystemProfile& profile) {
        if (profile.is_cpu_bound) return "CPU Bound";
        if (profile.is_memory_bound) return "Memory Bound";
        if (profile.is_cache_bound) return "Cache Bound";
        if (profile.has_contention) return "Contention";
        return "Optimal";
    }
    
    void suggest_optimization_strategy(const SystemProfile& profile) {
        std::vector<std::string> suggestions;
        
        if (profile.is_cpu_bound) {
            suggestions.push_back("Consider algorithmic optimizations");
            suggestions.push_back("Look for vectorization opportunities");
            suggestions.push_back("Review computational complexity");
        }
        
        if (profile.is_memory_bound) {
            suggestions.push_back("Optimize memory access patterns");
            suggestions.push_back("Consider data structure reorganization");
            suggestions.push_back("Implement prefetching strategies");
        }
        
        if (profile.is_cache_bound) {
            suggestions.push_back("Improve spatial locality");
            suggestions.push_back("Reduce working set size");
            suggestions.push_back("Consider SoA layout optimization");
        }
        
        if (profile.has_contention) {
            suggestions.push_back("Reduce shared data access");
            suggestions.push_back("Consider lock-free alternatives");
            suggestions.push_back("Implement work-stealing patterns");
        }
        
        // Display suggestions in popup
        ImGui::OpenPopup(std::format("Optimization Suggestions - {}", profile.system_name).c_str());
        
        if (ImGui::BeginPopupModal(std::format("Optimization Suggestions - {}", profile.system_name).c_str())) {
            ImGui::Text("Optimization suggestions for %s:", profile.system_name.c_str());
            ImGui::Separator();
            
            for (const auto& suggestion : suggestions) {
                ImGui::BulletText("%s", suggestion.c_str());
            }
            
            ImGui::Separator();
            if (ImGui::Button("Close")) {
                ImGui::CloseCurrentPopup();
            }
            
            ImGui::EndPopup();
        }
    }
};
```

### GPU Debug Tools

```cpp
class GPUDebugger {
    // GPU timing queries
    std::array<GLuint, 8> timer_queries_;
    usize current_query_index_{0};
    bool gpu_timing_available_{false};
    
    // GPU memory tracking
    struct GPUMemoryInfo {
        i32 total_memory_kb;
        i32 available_memory_kb;
        i32 current_available_memory_kb;
        i32 evicted_memory_kb;
        i32 evicted_count;
    };
    
    CircularBuffer<GPUMemoryInfo, 300> gpu_memory_history_;
    
public:
    void initialize() {
        // Check for timer query support
        if (GLAD_GL_ARB_timer_query) {
            glGenQueries(static_cast<GLsizei>(timer_queries_.size()), timer_queries_.data());
            gpu_timing_available_ = true;
            log_info("GPU timing queries available");
        }
        
        // Check for memory info extension
        if (GLAD_GL_NVX_gpu_memory_info) {
            log_info("NVIDIA GPU memory info available");
        } else if (GLAD_GL_ATI_meminfo) {
            log_info("AMD GPU memory info available");
        }
    }
    
    void begin_gpu_timing(const std::string& label) {
        if (!gpu_timing_available_) return;
        
        const GLuint query = timer_queries_[current_query_index_];
        glBeginQuery(GL_TIME_ELAPSED, query);
        
        // Store label for later retrieval
        gpu_timing_labels_[current_query_index_] = label;
    }
    
    void end_gpu_timing() {
        if (!gpu_timing_available_) return;
        
        glEndQuery(GL_TIME_ELAPSED);
        current_query_index_ = (current_query_index_ + 1) % timer_queries_.size();
    }
    
    void update_gpu_memory_info() {
        GPUMemoryInfo info{};
        
        #ifdef GL_GPU_MEMORY_INFO_TOTAL_AVAILABLE_MEMORY_NVX
        glGetIntegerv(GL_GPU_MEMORY_INFO_TOTAL_AVAILABLE_MEMORY_NVX, &info.total_memory_kb);
        glGetIntegerv(GL_GPU_MEMORY_INFO_CURRENT_AVAILABLE_VIDMEM_NVX, &info.current_available_memory_kb);
        glGetIntegerv(GL_GPU_MEMORY_INFO_EVICTED_MEMORY_NVX, &info.evicted_memory_kb);
        glGetIntegerv(GL_GPU_MEMORY_INFO_EVICTION_COUNT_NVX, &info.evicted_count);
        #endif
        
        gpu_memory_history_.push(info);
    }
    
    void render_gpu_debug_ui() {
        if (ImGui::Begin("GPU Debug")) {
            render_gpu_timing_results();
            ImGui::Separator();
            render_gpu_memory_analysis();
            ImGui::Separator();
            render_gpu_state_inspector();
        }
        ImGui::End();
    }
    
private:
    void render_gpu_timing_results() {
        ImGui::Text("GPU Timing Results:");
        
        if (!gpu_timing_available_) {
            ImGui::TextColored({1,0,0,1}, "GPU timing not available on this system");
            return;
        }
        
        // Retrieve completed queries
        for (usize i = 0; i < timer_queries_.size(); ++i) {
            GLint query_available = 0;
            glGetQueryObjectiv(timer_queries_[i], GL_QUERY_RESULT_AVAILABLE, &query_available);
            
            if (query_available) {
                GLuint64 gpu_time_ns = 0;
                glGetQueryObjectui64v(timer_queries_[i], GL_QUERY_RESULT, &gpu_time_ns);
                
                const f32 gpu_time_ms = static_cast<f32>(gpu_time_ns) / 1000000.0f;
                const std::string& label = gpu_timing_labels_[i];
                
                if (!label.empty()) {
                    ImGui::Text("  %s: %.3f ms", label.c_str(), gpu_time_ms);
                    
                    // Performance indicators
                    if (gpu_time_ms > 5.0f) {
                        ImGui::SameLine();
                        ImGui::TextColored({1,0,0,1}, "(Slow)");
                    } else if (gpu_time_ms > 1.0f) {
                        ImGui::SameLine();
                        ImGui::TextColored({1,1,0,1}, "(Moderate)");
                    }
                }
            }
        }
    }
    
    void render_gpu_memory_analysis() {
        if (gpu_memory_history_.empty()) {
            ImGui::Text("GPU memory info not available");
            return;
        }
        
        const auto& latest = gpu_memory_history_.back();
        
        ImGui::Text("GPU Memory Status:");
        ImGui::Text("  Total VRAM: %.1f MB", latest.total_memory_kb / 1024.0f);
        ImGui::Text("  Available: %.1f MB", latest.current_available_memory_kb / 1024.0f);
        ImGui::Text("  Used: %.1f MB", 
                   (latest.total_memory_kb - latest.current_available_memory_kb) / 1024.0f);
        
        if (latest.evicted_memory_kb > 0) {
            ImGui::TextColored({1,0,0,1}, "  Evicted: %.1f MB (%d times)", 
                              latest.evicted_memory_kb / 1024.0f, latest.evicted_count);
        }
        
        // Memory usage graph
        std::vector<f32> memory_usage;
        for (const auto& info : gpu_memory_history_) {
            const f32 used_mb = (info.total_memory_kb - info.current_available_memory_kb) / 1024.0f;
            memory_usage.push_back(used_mb);
        }
        
        ImGui::PlotLines("VRAM Usage (MB)", memory_usage.data(), memory_usage.size(),
                        0, nullptr, 0.0f, latest.total_memory_kb / 1024.0f, ImVec2(0, 80));
    }
    
    void render_gpu_state_inspector() {
        ImGui::Text("OpenGL State Inspector:");
        
        // Viewport state
        GLint viewport[4];
        glGetIntegerv(GL_VIEWPORT, viewport);
        ImGui::Text("  Viewport: (%d, %d, %d, %d)", viewport[0], viewport[1], viewport[2], viewport[3]);
        
        // Blend state
        GLboolean blend_enabled = glIsEnabled(GL_BLEND);
        ImGui::Text("  Blending: %s", blend_enabled ? "Enabled" : "Disabled");
        
        if (blend_enabled) {
            GLint src_factor, dst_factor;
            glGetIntegerv(GL_BLEND_SRC, &src_factor);
            glGetIntegerv(GL_BLEND_DST, &dst_factor);
            ImGui::Text("    Blend Func: %s, %s", 
                       blend_factor_name(src_factor), blend_factor_name(dst_factor));
        }
        
        // Texture state
        GLint active_texture;
        glGetIntegerv(GL_ACTIVE_TEXTURE, &active_texture);
        ImGui::Text("  Active Texture Unit: %d", active_texture - GL_TEXTURE0);
        
        GLint bound_texture;
        glGetIntegerv(GL_TEXTURE_BINDING_2D, &bound_texture);
        ImGui::Text("  Bound 2D Texture: %d", bound_texture);
        
        // Shader state
        GLint current_program;
        glGetIntegerv(GL_CURRENT_PROGRAM, &current_program);
        ImGui::Text("  Current Shader Program: %d", current_program);
        
        // Buffer state
        GLint array_buffer, element_buffer;
        glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &array_buffer);
        glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING, &element_buffer);
        ImGui::Text("  Array Buffer: %d", array_buffer);
        ImGui::Text("  Element Buffer: %d", element_buffer);
        
        // VAO state
        GLint vertex_array;
        glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &vertex_array);
        ImGui::Text("  Vertex Array: %d", vertex_array);
    }
};
```

---

**ECScope Debugging Tools: Making complex systems transparent, performance bottlenecks visible, and optimization opportunities clear.**

*The debugging system transforms abstract concepts into concrete, visual understanding, enabling effective learning and optimization of modern game engine systems.*