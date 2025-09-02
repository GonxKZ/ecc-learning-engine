#include "panel_ecs_inspector.hpp"
#include "ecs/components/transform.hpp"
#include "core/time.hpp"
#include "core/log.hpp"
#include <algorithm>
#include <sstream>

#ifdef ECSCOPE_HAS_GRAPHICS
#include "imgui.h"
#endif

namespace ecscope::ui {

ECSInspectorPanel::ECSInspectorPanel() 
    : Panel("ECS Inspector", true) {
    
    cached_entities_.reserve(1000); // Pre-allocate for performance
}

void ECSInspectorPanel::render() {
#ifdef ECSCOPE_HAS_GRAPHICS
    if (!visible_) return;
    
    core::Timer render_timer;
    
    // Begin inspector window with docking support
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_None;
    if (ImGui::Begin(name_.c_str(), &visible_, window_flags)) {
        
        // Render control buttons
        render_controls();
        
        ImGui::Separator();
        
        // Create layout with splitter
        if (ImGui::BeginTable("ECSInspectorLayout", 2, 
                             ImGuiTableFlags_Resizable | ImGuiTableFlags_BordersInnerV)) {
            
            // Left column - Entity list and archetypes
            ImGui::TableSetupColumn("EntityList", ImGuiTableColumnFlags_WidthFixed, 300.0f);
            ImGui::TableSetupColumn("Details", ImGuiTableColumnFlags_WidthStretch);
            
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            
            // Entity list section
            if (show_entity_details_) {
                if (ImGui::CollapsingHeader("Entities", ImGuiTreeNodeFlags_DefaultOpen)) {
                    render_entity_list();
                }
            }
            
            // Archetype browser section
            if (show_archetype_list_) {
                if (ImGui::CollapsingHeader("Archetypes", ImGuiTreeNodeFlags_DefaultOpen)) {
                    render_archetype_browser();
                }
            }
            
            // Right column - Selected entity details
            ImGui::TableSetColumnIndex(1);
            
            if (entity_valid_ && show_component_inspector_) {
                render_entity_details();
                ImGui::Separator();
                render_component_inspector();
            } else {
                ImGui::TextDisabled("No entity selected");
            }
            
            ImGui::EndTable();
        }
        
        // Performance stats at bottom
        if (show_performance_stats_) {
            ImGui::Separator();
            render_performance_section();
        }
    }
    ImGui::End();
    
    stats_.render_time = render_timer.elapsed_milliseconds();
#endif
}

void ECSInspectorPanel::update(f64 delta_time) {
    cache_update_timer_ += delta_time;
    
    // Update cache periodically
    if (cache_update_timer_ >= CACHE_UPDATE_INTERVAL) {
        update_entity_cache();
        cache_update_timer_ = 0.0;
    }
    
    stats_.last_update_time = delta_time;
}

void ECSInspectorPanel::select_entity(ecs::Entity entity) {
    selected_entity_ = entity;
    entity_valid_ = ecs::get_registry().is_valid(entity);
    
    if (entity_valid_) {
        LOG_DEBUG("Selected entity: " + format_entity_name(entity));
    }
}

void ECSInspectorPanel::render_entity_list() {
#ifdef ECSCOPE_HAS_GRAPHICS
    // Search filter
    ImGui::SetNextItemWidth(-1);
    if (ImGui::InputTextWithHint("##EntitySearch", "Search entities...", 
                                entity_search_filter_.data(), entity_search_filter_.capacity())) {
        entity_search_filter_.resize(strlen(entity_search_filter_.data()));
    }
    
    // Entity list
    if (ImGui::BeginChild("EntityList", ImVec2(0, 200), true)) {
        auto& registry = ecs::get_registry();
        
        stats_.visible_entities = 0;
        stats_.filtered_entities = 0;
        
        for (const auto& cached : cached_entities_) {
            stats_.visible_entities++;
            
            if (!entity_matches_filter(cached)) {
                continue;
            }
            
            stats_.filtered_entities++;
            
            std::string entity_name = format_entity_name(cached.entity);
            bool is_selected = (cached.entity.index == selected_entity_.index && 
                               cached.entity.generation == selected_entity_.generation);
            
            if (ImGui::Selectable(entity_name.c_str(), is_selected)) {
                select_entity(cached.entity);
            }
            
            // Context menu
            if (ImGui::BeginPopupContextItem()) {
                render_entity_context_menu(cached.entity);
                ImGui::EndPopup();
            }
            
            // Show component count as hint
            if (ImGui::IsItemHovered()) {
                ImGui::BeginTooltip();
                ImGui::Text("Components: %zu", cached.component_count);
                ImGui::Text("Archetype: %s", cached.archetype_signature.c_str());
                ImGui::EndTooltip();
            }
        }
        
        if (stats_.filtered_entities == 0 && stats_.visible_entities > 0) {
            ImGui::TextDisabled("No entities match filter");
        } else if (stats_.visible_entities == 0) {
            ImGui::TextDisabled("No entities in registry");
        }
    }
    ImGui::EndChild();
    
    // Entity count info
    ImGui::Text("Showing %zu / %zu entities", stats_.filtered_entities, stats_.visible_entities);
#endif
}

void ECSInspectorPanel::render_entity_details() {
#ifdef ECSCOPE_HAS_GRAPHICS
    if (!entity_valid_) return;
    
    auto& registry = ecs::get_registry();
    
    ImGui::Text("Entity Details");
    ImGui::Separator();
    
    // Basic entity info
    ImGui::Text("ID: %u (Gen: %u)", selected_entity_.index, selected_entity_.generation);
    
    // Validity status
    bool is_valid = registry.is_valid(selected_entity_);
    if (is_valid) {
        ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), " Valid");
    } else {
        ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), " Invalid");
    }
    
    if (!is_valid) {
        ImGui::TextDisabled("Entity has been destroyed");
        entity_valid_ = false;
        return;
    }
    
    // Component count
    // TODO: Add method to registry to get component count for entity
    ImGui::Text("Components: %s", "N/A"); // Placeholder
    
    // Actions
    if (ImGui::Button("Destroy Entity")) {
        if (registry.destroy_entity(selected_entity_)) {
            entity_valid_ = false;
            LOG_INFO("Destroyed entity: " + format_entity_name(selected_entity_));
        }
    }
    
    ImGui::SameLine();
    if (ImGui::Button("Clone Entity")) {
        // TODO: Implement entity cloning
        LOG_WARN("Entity cloning not yet implemented");
    }
#endif
}

void ECSInspectorPanel::render_archetype_browser() {
#ifdef ECSCOPE_HAS_GRAPHICS
    auto& registry = ecs::get_registry();
    auto archetype_stats = registry.get_archetype_stats();
    
    // Archetype list
    if (ImGui::BeginChild("ArchetypeList", ImVec2(0, 150), true)) {
        for (usize i = 0; i < archetype_stats.size(); ++i) {
            const auto& [signature, entity_count] = archetype_stats[i];
            
            // Skip empty archetypes if filtered
            if (!show_empty_archetypes_ && entity_count == 0) {
                continue;
            }
            
            std::string archetype_name = format_archetype_signature(signature);
            std::string display_name = archetype_name + " (" + std::to_string(entity_count) + ")";
            
            if (ImGui::TreeNode(display_name.c_str())) {
                ImGui::Text("Components: %zu", signature.count());
                ImGui::Text("Entities: %zu", entity_count);
                ImGui::Text("Signature: %s", signature.to_string().c_str());
                
                // Context menu for archetype
                if (ImGui::BeginPopupContextItem()) {
                    render_archetype_context_menu(i);
                    ImGui::EndPopup();
                }
                
                ImGui::TreePop();
            }
        }
        
        if (archetype_stats.empty()) {
            ImGui::TextDisabled("No archetypes found");
        }
    }
    ImGui::EndChild();
    
    // Options
    ImGui::Checkbox("Show empty archetypes", &show_empty_archetypes_);
#endif
}

void ECSInspectorPanel::render_component_inspector() {
#ifdef ECSCOPE_HAS_GRAPHICS
    if (!entity_valid_) return;
    
    auto& registry = ecs::get_registry();
    
    ImGui::Text("Components");
    ImGui::Separator();
    
    // Check for Transform component (most common)
    if (auto* transform = registry.get_component<ecs::components::Transform>(selected_entity_)) {
        if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen)) {
            // Position
            f32 pos[2] = {transform->position.x, transform->position.y};
            if (ImGui::DragFloat2("Position", pos, 0.1f)) {
                transform->position.x = pos[0];
                transform->position.y = pos[1];
            }
            
            // Rotation (convert to degrees for UI)
            f32 rotation_degrees = transform->rotation * 180.0f / 3.14159f;
            if (ImGui::DragFloat("Rotation", &rotation_degrees, 1.0f)) {
                transform->rotation = rotation_degrees * 3.14159f / 180.0f;
            }
            
            // Scale
            f32 scale[2] = {transform->scale.x, transform->scale.y};
            if (ImGui::DragFloat2("Scale", scale, 0.01f, 0.01f, 10.0f)) {
                transform->scale.x = scale[0];
                transform->scale.y = scale[1];
            }
            
            // Utility buttons
            if (ImGui::Button("Reset")) {
                *transform = ecs::components::Transform::identity();
            }
            
            ImGui::SameLine();
            if (ImGui::Button("Normalize Scale")) {
                f32 avg_scale = (transform->scale.x + transform->scale.y) * 0.5f;
                transform->scale = ecs::components::Vec2{avg_scale, avg_scale};
            }
        }
    }
    
    // Check for Vec2 component (velocity, etc.)
    if (auto* vec2 = registry.get_component<ecs::components::Vec2>(selected_entity_)) {
        if (ImGui::CollapsingHeader("Vec2", ImGuiTreeNodeFlags_DefaultOpen)) {
            f32 values[2] = {vec2->x, vec2->y};
            if (ImGui::DragFloat2("Value", values, 0.1f)) {
                vec2->x = values[0];
                vec2->y = values[1];
            }
            
            // Show magnitude
            f32 magnitude = vec2->length();
            ImGui::Text("Magnitude: %.3f", magnitude);
            
            if (magnitude > 0.0f && ImGui::Button("Normalize")) {
                *vec2 = vec2->normalized();
            }
        }
    }
    
    // Add component section
    if (ImGui::Button("Add Component")) {
        ImGui::OpenPopup("AddComponentPopup");
    }
    
    if (ImGui::BeginPopup("AddComponentPopup")) {
        if (ImGui::MenuItem("Transform")) {
            registry.add_component<ecs::components::Transform>(selected_entity_, 
                ecs::components::Transform::identity());
        }
        if (ImGui::MenuItem("Vec2")) {
            registry.add_component<ecs::components::Vec2>(selected_entity_, 
                ecs::components::Vec2::zero());
        }
        ImGui::EndPopup();
    }
#endif
}

void ECSInspectorPanel::render_performance_section() {
#ifdef ECSCOPE_HAS_GRAPHICS
    auto& registry = ecs::get_registry();
    
    ImGui::Text("Performance Stats");
    ImGui::Separator();
    
    // Registry stats
    ImGui::Text("Total Entities: %zu", registry.total_entities_created());
    ImGui::Text("Active Entities: %zu", registry.active_entities());
    ImGui::Text("Archetypes: %zu", registry.archetype_count());
    
    // Memory usage
    f64 memory_mb = registry.memory_usage() / (1024.0 * 1024.0);
    ImGui::Text("Memory Usage: %.2f MB", memory_mb);
    
    // Panel performance
    ImGui::Text("Render Time: %.3f ms", stats_.render_time);
    ImGui::Text("Update Time: %.3f ms", stats_.last_update_time * 1000.0);
    
    // Progress bar for memory usage (example - need actual limits)
    f32 memory_fraction = static_cast<f32>(registry.memory_usage()) / (10.0f * 1024.0f * 1024.0f); // 10MB max
    memory_fraction = std::clamp(memory_fraction, 0.0f, 1.0f);
    imgui_utils::progress_bar_animated(memory_fraction, 
                                      ("Memory: " + imgui_utils::format_bytes(registry.memory_usage())).c_str());
#endif
}

void ECSInspectorPanel::render_controls() {
#ifdef ECSCOPE_HAS_GRAPHICS
    // View options
    if (ImGui::Button("Refresh")) {
        update_entity_cache();
    }
    
    ImGui::SameLine();
    if (ImGui::Button("Clear Selection")) {
        selected_entity_ = {};
        entity_valid_ = false;
    }
    
    ImGui::SameLine();
    if (ImGui::Button("Settings")) {
        ImGui::OpenPopup("InspectorSettings");
    }
    
    // Settings popup
    if (ImGui::BeginPopup("InspectorSettings")) {
        ImGui::Checkbox("Show Entity Details", &show_entity_details_);
        ImGui::Checkbox("Show Archetype List", &show_archetype_list_);
        ImGui::Checkbox("Show Component Inspector", &show_component_inspector_);
        ImGui::Checkbox("Show Performance Stats", &show_performance_stats_);
        ImGui::EndPopup();
    }
#endif
}

void ECSInspectorPanel::update_entity_cache() {
    auto& registry = ecs::get_registry();
    cached_entities_.clear();
    
    // Get all entities
    auto all_entities = registry.get_all_entities();
    
    for (const auto& entity : all_entities) {
        CachedEntityInfo info;
        info.entity = entity;
        info.archetype_index = 0; // TODO: Get actual archetype index
        info.component_count = 0; // TODO: Get actual component count
        
        // Build signature string for display
        // TODO: Get actual component signature for entity
        info.archetype_signature = "Unknown";
        
        cached_entities_.push_back(info);
    }
    
    // Sort entities by ID for consistent display
    std::sort(cached_entities_.begin(), cached_entities_.end(),
        [](const CachedEntityInfo& a, const CachedEntityInfo& b) {
            return a.entity.index < b.entity.index;
        });
}

bool ECSInspectorPanel::entity_matches_filter(const CachedEntityInfo& info) const {
    if (entity_search_filter_.empty()) {
        return true;
    }
    
    std::string entity_name = format_entity_name(info.entity);
    return entity_name.find(entity_search_filter_) != std::string::npos;
}

void ECSInspectorPanel::render_entity_context_menu(ecs::Entity entity) {
#ifdef ECSCOPE_HAS_GRAPHICS
    if (ImGui::MenuItem("Select")) {
        select_entity(entity);
    }
    
    if (ImGui::MenuItem("Destroy")) {
        auto& registry = ecs::get_registry();
        if (registry.destroy_entity(entity)) {
            LOG_INFO("Destroyed entity: " + format_entity_name(entity));
            if (selected_entity_.index == entity.index && 
                selected_entity_.generation == entity.generation) {
                entity_valid_ = false;
            }
        }
    }
    
    if (ImGui::MenuItem("Clone")) {
        LOG_WARN("Entity cloning not yet implemented");
    }
#endif
}

void ECSInspectorPanel::render_archetype_context_menu(usize archetype_index) {
#ifdef ECSCOPE_HAS_GRAPHICS
    if (ImGui::MenuItem("Show Details")) {
        LOG_INFO("Showing archetype details for index: " + std::to_string(archetype_index));
    }
    
    if (ImGui::MenuItem("Export Data")) {
        LOG_WARN("Archetype export not yet implemented");
    }
#endif
}

std::string ECSInspectorPanel::format_entity_name(ecs::Entity entity) const {
    std::ostringstream oss;
    oss << "Entity " << entity.index << " (Gen " << entity.generation << ")";
    return oss.str();
}

std::string ECSInspectorPanel::format_archetype_signature(const ecs::ComponentSignature& signature) const {
    if (signature.empty()) {
        return "Empty";
    }
    
    std::ostringstream oss;
    auto component_ids = signature.to_component_ids();
    
    for (usize i = 0; i < component_ids.size(); ++i) {
        if (i > 0) oss << ", ";
        oss << get_component_type_name(component_ids[i]);
    }
    
    return oss.str();
}

const char* ECSInspectorPanel::get_component_type_name(core::ComponentID id) const {
    // TODO: Implement proper component type name registry
    // For now, use hardcoded mappings for known components
    if (id.value() == 0) return "Transform";
    if (id.value() == 1) return "Vec2";
    
    static std::string unknown_name = "Component" + std::to_string(id.value());
    return unknown_name.c_str();
}

} // namespace ecscope::ui