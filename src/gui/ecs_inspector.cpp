/**
 * @file ecs_inspector.cpp
 * @brief Implementation of the comprehensive ECS Inspector
 */

#include "../../include/ecscope/gui/ecs_inspector.hpp"
#include "../../include/ecscope/core/log.hpp"

#include <algorithm>
#include <regex>
#include <fstream>
#include <sstream>
#include <iomanip>

#ifdef ECSCOPE_HAS_IMGUI
#include <imgui.h>
#include <imgui_internal.h>
#endif

namespace ecscope::gui {

// =============================================================================
// UTILITY FUNCTIONS
// =============================================================================

namespace {
    constexpr float PANEL_PADDING = 8.0f;
    constexpr float ITEM_SPACING = 4.0f;
    constexpr ImVec4 COLOR_SUCCESS = ImVec4(0.2f, 0.8f, 0.2f, 1.0f);
    constexpr ImVec4 COLOR_WARNING = ImVec4(1.0f, 0.8f, 0.2f, 1.0f);
    constexpr ImVec4 COLOR_ERROR = ImVec4(0.8f, 0.2f, 0.2f, 1.0f);
    constexpr ImVec4 COLOR_INFO = ImVec4(0.3f, 0.7f, 1.0f, 1.0f);
    
    std::string format_duration(std::chrono::duration<float, std::milli> duration) {
        float ms = duration.count();
        if (ms < 1.0f) {
            return std::format("{:.3f}μs", ms * 1000.0f);
        } else if (ms < 1000.0f) {
            return std::format("{:.3f}ms", ms);
        } else {
            return std::format("{:.3f}s", ms / 1000.0f);
        }
    }
    
    std::string format_memory_size(size_t bytes) {
        const char* units[] = {"B", "KB", "MB", "GB"};
        size_t unit = 0;
        double size = static_cast<double>(bytes);
        
        while (size >= 1024.0 && unit < 3) {
            size /= 1024.0;
            unit++;
        }
        
        return std::format("{:.2f} {}", size, units[unit]);
    }
}

// =============================================================================
// ENTITY FILTER IMPLEMENTATION
// =============================================================================

bool EntityFilter::matches(const EntityInfo& entity) const {
    // Name pattern matching
    if (!name_pattern.empty()) {
        std::regex pattern(name_pattern, std::regex_constants::icase);
        if (!std::regex_search(entity.name, pattern)) {
            return false;
        }
    }
    
    // Tag pattern matching
    if (!tag_pattern.empty()) {
        std::regex pattern(tag_pattern, std::regex_constants::icase);
        if (!std::regex_search(entity.tag, pattern)) {
            return false;
        }
    }
    
    // Component requirements
    for (auto required : required_components) {
        if (std::find(entity.components.begin(), entity.components.end(), required) == entity.components.end()) {
            return false;
        }
    }
    
    // Component exclusions
    for (auto excluded : excluded_components) {
        if (std::find(entity.components.begin(), entity.components.end(), excluded) != entity.components.end()) {
            return false;
        }
    }
    
    // State filters
    if (only_enabled && !entity.enabled) return false;
    if (only_selected && !entity.selected) return false;
    
    return true;
}

std::string EntityFilter::to_string() const {
    std::ostringstream oss;
    
    if (!name_pattern.empty()) oss << "name:\"" << name_pattern << "\" ";
    if (!tag_pattern.empty()) oss << "tag:\"" << tag_pattern << "\" ";
    if (!required_components.empty()) oss << "has:" << required_components.size() << " ";
    if (!excluded_components.empty()) oss << "!has:" << excluded_components.size() << " ";
    if (only_enabled) oss << "enabled ";
    if (only_selected) oss << "selected ";
    
    std::string result = oss.str();
    if (!result.empty() && result.back() == ' ') {
        result.pop_back();
    }
    
    return result.empty() ? "no filter" : result;
}

// =============================================================================
// SELECTION STATE IMPLEMENTATION
// =============================================================================

void SelectionState::select(EntityID entity, bool multi) {
    if (!multi) {
        selected_entities.clear();
    }
    
    selected_entities.insert(entity);
    primary_selection = entity;
    selection_time = std::chrono::steady_clock::now();
}

void SelectionState::deselect(EntityID entity) {
    selected_entities.erase(entity);
    if (primary_selection == entity) {
        primary_selection = selected_entities.empty() ? std::nullopt : std::make_optional(*selected_entities.begin());
    }
}

bool SelectionState::is_selected(EntityID entity) const {
    return selected_entities.find(entity) != selected_entities.end();
}

// =============================================================================
// SYSTEM STATS IMPLEMENTATION
// =============================================================================

void SystemStats::record_execution(std::chrono::duration<float, std::milli> duration, uint64_t processed) {
    last_execution_time = duration;
    entities_processed = processed;
    execution_count++;
    
    // Update min/max
    if (duration < min_execution_time) min_execution_time = duration;
    if (duration > max_execution_time) max_execution_time = duration;
    
    // Update running average
    float alpha = 0.1f; // Exponential moving average factor
    average_execution_time = std::chrono::duration<float, std::milli>(
        average_execution_time.count() * (1.0f - alpha) + duration.count() * alpha
    );
    
    // Update history for graphing
    execution_history.push(duration.count());
    while (execution_history.size() > MAX_HISTORY) {
        execution_history.pop();
    }
}

void SystemStats::reset_stats() {
    last_execution_time = std::chrono::duration<float, std::milli>{0};
    average_execution_time = std::chrono::duration<float, std::milli>{0};
    max_execution_time = std::chrono::duration<float, std::milli>{0};
    min_execution_time = std::chrono::duration<float, std::milli>::max();
    execution_count = 0;
    entities_processed = 0;
    
    // Clear history
    while (!execution_history.empty()) {
        execution_history.pop();
    }
}

// =============================================================================
// ARCHETYPE INFO IMPLEMENTATION
// =============================================================================

std::string ECSInspector::ArchetypeInfo::to_string() const {
    std::ostringstream oss;
    oss << "Archetype[" << components.size() << " components, " << entity_count << " entities]";
    return oss.str();
}

// =============================================================================
// QUERY SPEC IMPLEMENTATION
// =============================================================================

std::string ECSInspector::QuerySpec::to_string() const {
    std::ostringstream oss;
    oss << name << " (";
    
    if (!required_components.empty()) {
        oss << "requires:" << required_components.size();
    }
    
    if (!excluded_components.empty()) {
        if (!required_components.empty()) oss << ", ";
        oss << "excludes:" << excluded_components.size();
    }
    
    oss << ")";
    return oss.str();
}

// =============================================================================
// MAIN ECS INSPECTOR IMPLEMENTATION
// =============================================================================

ECSInspector::ECSInspector(ecs::Registry* registry, const InspectorConfig& config)
    : registry_(registry)
    , dashboard_(nullptr)
    , config_(config)
    , history_position_(0)
    , last_entity_update_(std::chrono::steady_clock::now())
    , last_component_update_(std::chrono::steady_clock::now())
    , last_system_update_(std::chrono::steady_clock::now())
    , last_memory_update_(std::chrono::steady_clock::now())
{
    if (!registry_) {
        LOG_ERROR("ECSInspector: Registry cannot be null");
        return;
    }
    
    // Initialize with current time
    auto now = std::chrono::steady_clock::now();
    metrics_.last_measurement = now;
}

ECSInspector::~ECSInspector() {
    shutdown();
}

bool ECSInspector::initialize() {
    if (initialized_.load()) {
        return true;
    }
    
    if (!registry_) {
        LOG_ERROR("ECSInspector: Cannot initialize without valid registry");
        return false;
    }
    
    LOG_INFO("Initializing ECS Inspector...");
    
    // Initialize entity tracking
    update_entity_tracking();
    
    // Set up default component metadata for common types
    // This would be expanded based on the actual component types in the system
    
    initialized_.store(true);
    LOG_INFO("ECS Inspector initialized successfully");
    
    return true;
}

void ECSInspector::shutdown() {
    if (!initialized_.load()) {
        return;
    }
    
    shutdown_requested_.store(true);
    
    LOG_INFO("Shutting down ECS Inspector...");
    
    // Clear all tracked data
    {
        std::lock_guard<std::mutex> lock(entity_mutex_);
        entity_info_.clear();
        selection_state_.clear();
    }
    
    {
        std::lock_guard<std::mutex> lock(component_mutex_);
        change_history_.clear();
        component_templates_.clear();
    }
    
    {
        std::lock_guard<std::mutex> lock(system_mutex_);
        system_graph_.systems.clear();
    }
    
    initialized_.store(false);
    LOG_INFO("ECS Inspector shutdown complete");
}

void ECSInspector::update(float delta_time) {
    if (!initialized_.load() || shutdown_requested_.load()) {
        return;
    }
    
    auto start_time = std::chrono::high_resolution_clock::now();
    auto now = std::chrono::steady_clock::now();
    
    // Update entity tracking based on refresh rate
    if (std::chrono::duration_cast<std::chrono::milliseconds>(now - last_entity_update_).count() >= config_.entity_refresh_rate) {
        update_entity_tracking();
        last_entity_update_ = now;
    }
    
    // Update component tracking
    if (std::chrono::duration_cast<std::chrono::milliseconds>(now - last_component_update_).count() >= config_.component_refresh_rate) {
        update_component_tracking();
        last_component_update_ = now;
    }
    
    // Update system monitoring
    if (std::chrono::duration_cast<std::chrono::milliseconds>(now - last_system_update_).count() >= config_.system_refresh_rate) {
        update_system_monitoring();
        last_system_update_ = now;
    }
    
    // Periodic cleanup
    if (frame_counter_.load() % 300 == 0) { // Every 5 seconds at 60fps
        cleanup_stale_data();
    }
    
    frame_counter_.fetch_add(1);
    
    auto end_time = std::chrono::high_resolution_clock::now();
    metrics_.last_update_time_ms = std::chrono::duration<float, std::milli>(end_time - start_time).count();
}

void ECSInspector::render() {
    if (!initialized_.load()) {
        return;
    }
    
#ifdef ECSCOPE_HAS_IMGUI
    auto start_time = std::chrono::high_resolution_clock::now();
    
    render_main_inspector_window();
    
    auto end_time = std::chrono::high_resolution_clock::now();
    metrics_.last_render_time_ms = std::chrono::duration<float, std::milli>(end_time - start_time).count();
#endif
}

void ECSInspector::render_as_dashboard_panel(const std::string& panel_name) {
#ifdef ECSCOPE_HAS_IMGUI
    if (ImGui::Begin(panel_name.c_str())) {
        render_main_inspector_window();
    }
    ImGui::End();
#endif
}

void ECSInspector::register_with_dashboard(Dashboard* dashboard) {
    dashboard_ = dashboard;
    
    if (dashboard_) {
        // Register as a feature in the dashboard
        FeatureInfo inspector_feature;
        inspector_feature.id = "ecs_inspector";
        inspector_feature.name = "ECS Inspector";
        inspector_feature.description = "Comprehensive ECS debugging and development tool";
        inspector_feature.category = FeatureCategory::Debugging;
        inspector_feature.icon = "🔍";
        inspector_feature.launch_callback = [this]() {
            show_entity_hierarchy_ = true;
            show_component_inspector_ = true;
        };
        inspector_feature.status_callback = [this]() {
            return initialized_.load();
        };
        
        dashboard_->register_feature(inspector_feature);
        LOG_INFO("ECS Inspector registered with dashboard");
    }
}

// =============================================================================
// ENTITY MANAGEMENT IMPLEMENTATION
// =============================================================================

const EntityInfo* ECSInspector::get_entity_info(EntityID entity) const {
    std::lock_guard<std::mutex> lock(entity_mutex_);
    auto it = entity_info_.find(entity);
    return (it != entity_info_.end()) ? it->second.get() : nullptr;
}

EntityID ECSInspector::create_entity(const std::string& name, const std::string& tag) {
    if (!registry_) {
        return EntityID{}; // Invalid entity
    }
    
    EntityID entity = registry_->create_entity();
    
    if (entity != EntityID{}) {
        std::lock_guard<std::mutex> lock(entity_mutex_);
        auto info = std::make_unique<EntityInfo>(entity);
        info->name = name.empty() ? std::format("Entity_{}", static_cast<uint32_t>(entity)) : name;
        info->tag = tag;
        entity_info_[entity] = std::move(info);
        
        LOG_DEBUG("Created entity {} with name '{}'", static_cast<uint32_t>(entity), name);
    }
    
    return entity;
}

bool ECSInspector::destroy_entities(const std::vector<EntityID>& entities) {
    if (!registry_) {
        return false;
    }
    
    bool all_success = true;
    
    for (EntityID entity : entities) {
        bool success = registry_->destroy_entity(entity);
        if (success) {
            std::lock_guard<std::mutex> lock(entity_mutex_);
            entity_info_.erase(entity);
            selection_state_.deselect(entity);
        } else {
            all_success = false;
        }
    }
    
    return all_success;
}

EntityID ECSInspector::clone_entity(EntityID source, const std::string& name) {
    if (!registry_ || !registry_->is_valid(source)) {
        return EntityID{}; // Invalid entity
    }
    
    // For now, create a new entity and manually copy components
    // This would need to be expanded based on the actual component system
    EntityID cloned = create_entity(name.empty() ? "Clone" : name, "");
    
    // TODO: Copy all components from source to cloned entity
    // This requires iterating through all component types and copying them
    
    LOG_DEBUG("Cloned entity {} to {}", static_cast<uint32_t>(source), static_cast<uint32_t>(cloned));
    return cloned;
}

std::vector<EntityID> ECSInspector::search_entities(const EntityFilter& filter) const {
    std::vector<EntityID> results;
    
    std::lock_guard<std::mutex> lock(entity_mutex_);
    for (const auto& [entity, info] : entity_info_) {
        if (filter.matches(*info)) {
            results.push_back(entity);
        }
    }
    
    return results;
}

std::vector<EntityID> ECSInspector::get_all_entities() const {
    std::vector<EntityID> entities;
    
    std::lock_guard<std::mutex> lock(entity_mutex_);
    entities.reserve(entity_info_.size());
    
    for (const auto& [entity, info] : entity_info_) {
        entities.push_back(entity);
    }
    
    return entities;
}

void ECSInspector::set_entity_name(EntityID entity, const std::string& name) {
    std::lock_guard<std::mutex> lock(entity_mutex_);
    auto it = entity_info_.find(entity);
    if (it != entity_info_.end()) {
        it->second->name = name;
        it->second->last_modified = std::chrono::steady_clock::now();
    }
}

void ECSInspector::set_entity_tag(EntityID entity, const std::string& tag) {
    std::lock_guard<std::mutex> lock(entity_mutex_);
    auto it = entity_info_.find(entity);
    if (it != entity_info_.end()) {
        it->second->tag = tag;
        it->second->last_modified = std::chrono::steady_clock::now();
    }
}

// =============================================================================
// SELECTION SYSTEM IMPLEMENTATION
// =============================================================================

void ECSInspector::select_entity(EntityID entity, bool multi_select) {
    selection_state_.select(entity, multi_select);
    
    // Update entity info selection state
    std::lock_guard<std::mutex> lock(entity_mutex_);
    for (auto& [ent, info] : entity_info_) {
        info->selected = selection_state_.is_selected(ent);
    }
}

void ECSInspector::deselect_entity(EntityID entity) {
    selection_state_.deselect(entity);
    
    // Update entity info
    std::lock_guard<std::mutex> lock(entity_mutex_);
    auto it = entity_info_.find(entity);
    if (it != entity_info_.end()) {
        it->second->selected = false;
    }
}

void ECSInspector::clear_selection() {
    selection_state_.clear();
    
    // Update all entity info
    std::lock_guard<std::mutex> lock(entity_mutex_);
    for (auto& [entity, info] : entity_info_) {
        info->selected = false;
    }
}

bool ECSInspector::is_entity_selected(EntityID entity) const {
    return selection_state_.is_selected(entity);
}

std::vector<EntityID> ECSInspector::get_selected_entities() const {
    std::vector<EntityID> selected;
    selected.reserve(selection_state_.count());
    
    for (EntityID entity : selection_state_.selected_entities) {
        selected.push_back(entity);
    }
    
    return selected;
}

// =============================================================================
// COMPONENT SYSTEM IMPLEMENTATION
// =============================================================================

void ECSInspector::register_component_metadata(const ComponentMetadata& metadata) {
    std::lock_guard<std::mutex> lock(component_mutex_);
    component_metadata_[metadata.type] = metadata;
    LOG_DEBUG("Registered component metadata for: {}", metadata.name);
}

const ComponentMetadata* ECSInspector::get_component_metadata(ComponentTypeInfo type) const {
    std::lock_guard<std::mutex> lock(component_mutex_);
    auto it = component_metadata_.find(type);
    return (it != component_metadata_.end()) ? &it->second : nullptr;
}

std::vector<ComponentMetadata> ECSInspector::get_all_component_metadata() const {
    std::vector<ComponentMetadata> metadata;
    
    std::lock_guard<std::mutex> lock(component_mutex_);
    metadata.reserve(component_metadata_.size());
    
    for (const auto& [type, meta] : component_metadata_) {
        metadata.push_back(meta);
    }
    
    return metadata;
}

void ECSInspector::record_change(const ComponentChange& change) {
    if (!config_.enable_undo_redo) {
        return;
    }
    
    std::lock_guard<std::mutex> lock(component_mutex_);
    
    // Remove any changes after current position (when creating new branch)
    if (history_position_ < change_history_.size()) {
        change_history_.erase(change_history_.begin() + history_position_, change_history_.end());
    }
    
    // Add new change
    change_history_.push_back(change);
    history_position_ = change_history_.size();
    
    // Limit history size
    while (change_history_.size() > config_.max_history_entries) {
        change_history_.erase(change_history_.begin());
        if (history_position_ > 0) history_position_--;
    }
}

bool ECSInspector::can_undo() const {
    std::lock_guard<std::mutex> lock(component_mutex_);
    return history_position_ > 0;
}

bool ECSInspector::can_redo() const {
    std::lock_guard<std::mutex> lock(component_mutex_);
    return history_position_ < change_history_.size();
}

// =============================================================================
// SYSTEM MONITORING IMPLEMENTATION
// =============================================================================

void ECSInspector::register_system(const SystemStats& system) {
    std::lock_guard<std::mutex> lock(system_mutex_);
    system_graph_.systems[system.system_id] = system;
    LOG_DEBUG("Registered system: {}", system.name);
}

void ECSInspector::update_system_stats(const SystemID& system_id, std::chrono::duration<float, std::milli> execution_time, uint64_t entities_processed) {
    std::lock_guard<std::mutex> lock(system_mutex_);
    auto it = system_graph_.systems.find(system_id);
    if (it != system_graph_.systems.end()) {
        it->second.record_execution(execution_time, entities_processed);
    }
}

const SystemStats* ECSInspector::get_system_stats(const SystemID& system_id) const {
    std::lock_guard<std::mutex> lock(system_mutex_);
    auto it = system_graph_.systems.find(system_id);
    return (it != system_graph_.systems.end()) ? &it->second : nullptr;
}

std::vector<SystemID> ECSInspector::get_all_systems() const {
    std::vector<SystemID> systems;
    
    std::lock_guard<std::mutex> lock(system_mutex_);
    systems.reserve(system_graph_.systems.size());
    
    for (const auto& [id, stats] : system_graph_.systems) {
        systems.push_back(id);
    }
    
    return systems;
}

// =============================================================================
// PRIVATE UPDATE METHODS
// =============================================================================

void ECSInspector::update_entity_tracking() {
    if (!registry_) {
        return;
    }
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    // Get all entities from registry
    auto all_entities = registry_->get_all_entities();
    
    std::lock_guard<std::mutex> lock(entity_mutex_);
    
    // Remove entities that no longer exist
    std::vector<EntityID> to_remove;
    for (const auto& [entity, info] : entity_info_) {
        if (std::find(all_entities.begin(), all_entities.end(), entity) == all_entities.end()) {
            to_remove.push_back(entity);
        }
    }
    
    for (EntityID entity : to_remove) {
        entity_info_.erase(entity);
        selection_state_.deselect(entity);
    }
    
    // Add new entities
    for (EntityID entity : all_entities) {
        if (entity_info_.find(entity) == entity_info_.end()) {
            auto info = std::make_unique<EntityInfo>(entity);
            info->name = std::format("Entity_{}", static_cast<uint32_t>(entity));
            entity_info_[entity] = std::move(info);
        }
    }
    
    // Update metrics
    metrics_.entities_tracked = entity_info_.size();
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration<float, std::milli>(end_time - start_time).count();
    
    // Check performance limits
    if (duration > config_.max_update_time_ms) {
        LOG_WARN("Entity tracking update took {:.2f}ms (limit: {:.2f}ms)", duration, config_.max_update_time_ms);
    }
}

void ECSInspector::update_component_tracking() {
    // Update component information for tracked entities
    std::lock_guard<std::mutex> lock(entity_mutex_);
    
    for (auto& [entity, info] : entity_info_) {
        if (registry_ && registry_->is_valid(entity)) {
            // This would need to be expanded to track actual component types
            // For now, we just update the timestamp
            info->last_modified = std::chrono::steady_clock::now();
        }
    }
    
    metrics_.components_tracked = component_metadata_.size();
}

void ECSInspector::update_system_monitoring() {
    std::lock_guard<std::mutex> lock(system_mutex_);
    metrics_.systems_tracked = system_graph_.systems.size();
}

void ECSInspector::cleanup_stale_data() {
    // Remove old history entries if they exceed limits
    std::lock_guard<std::mutex> lock(component_mutex_);
    
    auto now = std::chrono::steady_clock::now();
    constexpr auto MAX_AGE = std::chrono::hours(1);
    
    // Clean old change history
    auto cutoff = now - MAX_AGE;
    change_history_.erase(
        std::remove_if(change_history_.begin(), change_history_.end(),
            [cutoff](const ComponentChange& change) {
                return change.timestamp < cutoff;
            }
        ),
        change_history_.end()
    );
    
    // Adjust history position if needed
    if (history_position_ > change_history_.size()) {
        history_position_ = change_history_.size();
    }
}

// =============================================================================
// RENDERING IMPLEMENTATION
// =============================================================================

void ECSInspector::render_main_inspector_window() {
#ifdef ECSCOPE_HAS_IMGUI
    ImGui::Begin("ECS Inspector");
    
    // Main toolbar
    if (ImGui::BeginMenuBar()) {
        if (ImGui::BeginMenu("View")) {
            ImGui::MenuItem("Entity Hierarchy", nullptr, &show_entity_hierarchy_);
            ImGui::MenuItem("Component Inspector", nullptr, &show_component_inspector_);
            ImGui::MenuItem("System Monitor", nullptr, &show_system_monitor_);
            ImGui::MenuItem("Archetype Analyzer", nullptr, &show_archetype_analyzer_);
            ImGui::Separator();
            ImGui::MenuItem("Query Builder", nullptr, &show_query_builder_);
            ImGui::MenuItem("History", nullptr, &show_history_panel_);
            ImGui::MenuItem("Templates", nullptr, &show_templates_panel_);
            ImGui::MenuItem("Settings", nullptr, &show_settings_panel_);
            ImGui::MenuItem("Metrics", nullptr, &show_metrics_panel_);
            ImGui::EndMenu();
        }
        
        if (ImGui::BeginMenu("Tools")) {
            if (ImGui::MenuItem("Clear Selection")) {
                clear_selection();
            }
            if (ImGui::MenuItem("Refresh All")) {
                update_entity_tracking();
                update_component_tracking();
                update_system_monitoring();
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Export State...")) {
                // TODO: Implement state export
            }
            if (ImGui::MenuItem("Import State...")) {
                // TODO: Implement state import
            }
            ImGui::EndMenu();
        }
        
        // Status bar in menu bar
        ImGui::Separator();
        ImGui::Text("Entities: %zu | Selected: %zu | Systems: %zu", 
                   metrics_.entities_tracked, 
                   selection_state_.count(),
                   metrics_.systems_tracked);
        
        ImGui::EndMenuBar();
    }
    
    // Search and filter bar
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4, 4));
    ImGui::SetNextItemWidth(300);
    if (ImGui::InputTextWithHint("##search", "Search entities...", &current_search_filter_[0], current_search_filter_.capacity())) {
        // Update active filter based on search
        active_filter_.name_pattern = current_search_filter_;
    }
    ImGui::SameLine();
    if (ImGui::Button("Clear Filter")) {
        current_search_filter_.clear();
        active_filter_ = EntityFilter{};
    }
    ImGui::PopStyleVar();
    
    // Main content area with docking
    ImGuiID dockspace_id = ImGui::GetID("InspectorDockSpace");
    ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_None);
    
    // Render individual panels
    if (show_entity_hierarchy_) {
        render_entity_hierarchy_panel();
    }
    
    if (show_component_inspector_) {
        render_component_inspector_panel();
    }
    
    if (show_system_monitor_) {
        render_system_monitor_panel();
    }
    
    if (show_archetype_analyzer_) {
        render_archetype_analyzer_panel();
    }
    
    if (show_query_builder_) {
        render_query_builder_panel();
    }
    
    if (show_history_panel_) {
        render_history_panel();
    }
    
    if (show_templates_panel_) {
        render_templates_panel();
    }
    
    if (show_settings_panel_) {
        render_settings_panel();
    }
    
    if (show_metrics_panel_) {
        render_metrics_panel();
    }
    
    ImGui::End();
#endif
}

void ECSInspector::render_entity_hierarchy_panel() {
#ifdef ECSCOPE_HAS_IMGUI
    if (ImGui::Begin("Entity Hierarchy", &show_entity_hierarchy_)) {
        // Entity creation controls
        if (ImGui::Button("Create Entity")) {
            create_entity();
        }
        ImGui::SameLine();
        if (ImGui::Button("Delete Selected") && !selection_state_.selected_entities.empty()) {
            std::vector<EntityID> to_delete(selection_state_.selected_entities.begin(), 
                                           selection_state_.selected_entities.end());
            destroy_entities(to_delete);
        }
        ImGui::SameLine();
        if (ImGui::Button("Clone Selected") && selection_state_.primary_selection.has_value()) {
            clone_entity(selection_state_.primary_selection.value());
        }
        
        ImGui::Separator();
        
        // Entity tree
        if (ImGui::BeginChild("EntityTree", ImVec2(0, 0), true)) {
            std::lock_guard<std::mutex> lock(entity_mutex_);
            
            for (const auto& [entity, info] : entity_info_) {
                if (!active_filter_.matches(*info)) {
                    continue;
                }
                
                render_entity_tree_node(entity, *info);
            }
        }
        ImGui::EndChild();
    }
    ImGui::End();
#endif
}

void ECSInspector::render_component_inspector_panel() {
#ifdef ECSCOPE_HAS_IMGUI
    if (ImGui::Begin("Component Inspector", &show_component_inspector_)) {
        if (selection_state_.primary_selection.has_value()) {
            EntityID selected = selection_state_.primary_selection.value();
            
            ImGui::Text("Entity: %u", static_cast<uint32_t>(selected));
            
            const EntityInfo* info = get_entity_info(selected);
            if (info) {
                // Entity name and tag editing
                static char name_buffer[256] = {};
                static char tag_buffer[256] = {};
                
                strncpy(name_buffer, info->name.c_str(), sizeof(name_buffer) - 1);
                strncpy(tag_buffer, info->tag.c_str(), sizeof(tag_buffer) - 1);
                
                if (ImGui::InputText("Name", name_buffer, sizeof(name_buffer))) {
                    set_entity_name(selected, std::string(name_buffer));
                }
                
                if (ImGui::InputText("Tag", tag_buffer, sizeof(tag_buffer))) {
                    set_entity_tag(selected, std::string(tag_buffer));
                }
                
                ImGui::Separator();
                
                // Component list and editing
                render_component_list(selected);
            }
        } else if (selection_state_.count() > 1) {
            ImGui::Text("Multi-selection: %zu entities", selection_state_.count());
            render_multi_entity_operations();
        } else {
            ImGui::Text("Select an entity to inspect its components");
        }
    }
    ImGui::End();
#endif
}

void ECSInspector::render_system_monitor_panel() {
#ifdef ECSCOPE_HAS_IMGUI
    if (ImGui::Begin("System Monitor", &show_system_monitor_)) {
        if (ImGui::Button("Reset All Stats")) {
            std::lock_guard<std::mutex> lock(system_mutex_);
            for (auto& [id, stats] : system_graph_.systems) {
                stats.reset_stats();
            }
        }
        
        ImGui::Separator();
        
        // System list table
        if (ImGui::BeginTable("SystemsTable", 6, ImGuiTableFlags_Resizable | ImGuiTableFlags_Sortable | ImGuiTableFlags_ScrollY)) {
            ImGui::TableSetupColumn("System", ImGuiTableColumnFlags_WidthFixed, 200.0f);
            ImGui::TableSetupColumn("Status", ImGuiTableColumnFlags_WidthFixed, 80.0f);
            ImGui::TableSetupColumn("Last Exec", ImGuiTableColumnFlags_WidthFixed, 100.0f);
            ImGui::TableSetupColumn("Avg Exec", ImGuiTableColumnFlags_WidthFixed, 100.0f);
            ImGui::TableSetupColumn("Entities", ImGuiTableColumnFlags_WidthFixed, 80.0f);
            ImGui::TableSetupColumn("Graph", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableHeadersRow();
            
            std::lock_guard<std::mutex> lock(system_mutex_);
            for (const auto& [id, stats] : system_graph_.systems) {
                ImGui::TableNextRow();
                
                ImGui::TableNextColumn();
                ImGui::Text("%s", stats.name.c_str());
                
                ImGui::TableNextColumn();
                if (stats.is_enabled) {
                    ImGui::TextColored(COLOR_SUCCESS, "Enabled");
                } else {
                    ImGui::TextColored(COLOR_ERROR, "Disabled");
                }
                
                ImGui::TableNextColumn();
                ImGui::Text("%s", format_duration(stats.last_execution_time).c_str());
                
                ImGui::TableNextColumn();
                ImGui::Text("%s", format_duration(stats.average_execution_time).c_str());
                
                ImGui::TableNextColumn();
                ImGui::Text("%llu", stats.entities_processed);
                
                ImGui::TableNextColumn();
                // Mini performance graph would go here
                ImGui::Text("Graph");
            }
            
            ImGui::EndTable();
        }
    }
    ImGui::End();
#endif
}

void ECSInspector::render_archetype_analyzer_panel() {
#ifdef ECSCOPE_HAS_IMGUI
    if (ImGui::Begin("Archetype Analyzer", &show_archetype_analyzer_)) {
        if (!registry_) {
            ImGui::Text("No registry available");
            ImGui::End();
            return;
        }
        
        // Get archetype statistics from registry
        auto archetype_stats = registry_->get_archetype_stats();
        
        ImGui::Text("Total Archetypes: %zu", archetype_stats.size());
        
        if (ImGui::Button("Refresh Analysis")) {
            refresh_archetype_analysis();
        }
        
        ImGui::Separator();
        
        // Archetype table
        if (ImGui::BeginTable("ArchetypesTable", 4, ImGuiTableFlags_Resizable | ImGuiTableFlags_Sortable | ImGuiTableFlags_ScrollY)) {
            ImGui::TableSetupColumn("Signature", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("Entities", ImGuiTableColumnFlags_WidthFixed, 80.0f);
            ImGui::TableSetupColumn("Memory", ImGuiTableColumnFlags_WidthFixed, 100.0f);
            ImGui::TableSetupColumn("Components", ImGuiTableColumnFlags_WidthFixed, 100.0f);
            ImGui::TableHeadersRow();
            
            for (const auto& [signature, entity_count] : archetype_stats) {
                ImGui::TableNextRow();
                
                ImGui::TableNextColumn();
                ImGui::Text("%s", signature.to_string().c_str());
                
                ImGui::TableNextColumn();
                ImGui::Text("%zu", entity_count);
                
                ImGui::TableNextColumn();
                ImGui::Text("Unknown"); // Would need to calculate actual memory usage
                
                ImGui::TableNextColumn();
                ImGui::Text("%zu", signature.count());
            }
            
            ImGui::EndTable();
        }
    }
    ImGui::End();
#endif
}

void ECSInspector::render_entity_tree_node(EntityID entity, const EntityInfo& info) {
#ifdef ECSCOPE_HAS_IMGUI
    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
    
    if (info.selected) {
        flags |= ImGuiTreeNodeFlags_Selected;
    }
    
    if (info.children.empty()) {
        flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
    }
    
    std::string label = std::format("{}##{}", info.name, static_cast<uint32_t>(entity));
    if (!info.tag.empty()) {
        label += std::format(" [{}]", info.tag);
    }
    
    bool node_open = ImGui::TreeNodeEx(label.c_str(), flags);
    
    // Handle selection
    if (ImGui::IsItemClicked()) {
        bool multi_select = ImGui::GetIO().KeyCtrl;
        select_entity(entity, multi_select);
    }
    
    // Context menu
    if (ImGui::BeginPopupContextItem()) {
        render_entity_context_menu(entity);
        ImGui::EndPopup();
    }
    
    // Children
    if (node_open && !info.children.empty()) {
        for (EntityID child : info.children) {
            const EntityInfo* child_info = get_entity_info(child);
            if (child_info && active_filter_.matches(*child_info)) {
                render_entity_tree_node(child, *child_info);
            }
        }
    }
    
    if (node_open && !info.children.empty()) {
        ImGui::TreePop();
    }
#endif
}

void ECSInspector::render_entity_context_menu(EntityID entity) {
#ifdef ECSCOPE_HAS_IMGUI
    if (ImGui::MenuItem("Select")) {
        select_entity(entity, false);
    }
    if (ImGui::MenuItem("Add to Selection")) {
        select_entity(entity, true);
    }
    ImGui::Separator();
    if (ImGui::MenuItem("Clone")) {
        clone_entity(entity);
    }
    if (ImGui::MenuItem("Delete")) {
        destroy_entities({entity});
    }
    ImGui::Separator();
    if (ImGui::MenuItem("Focus in Inspector")) {
        select_entity(entity, false);
        show_component_inspector_ = true;
    }
#endif
}

void ECSInspector::render_component_list(EntityID entity) {
#ifdef ECSCOPE_HAS_IMGUI
    if (ImGui::Button("Add Component")) {
        ImGui::OpenPopup("AddComponentPopup");
    }
    
    if (ImGui::BeginPopup("AddComponentPopup")) {
        render_add_component_dialog(entity);
        ImGui::EndPopup();
    }
    
    ImGui::Separator();
    
    // List existing components
    const EntityInfo* info = get_entity_info(entity);
    if (info) {
        for (ComponentTypeInfo comp_type : info->components) {
            const ComponentMetadata* metadata = get_component_metadata(comp_type);
            std::string comp_name = metadata ? metadata->name : "Unknown Component";
            
            if (ImGui::CollapsingHeader(comp_name.c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {
                render_component_editor(entity, comp_type);
            }
        }
    }
#endif
}

void ECSInspector::render_component_editor(EntityID entity, ComponentTypeInfo type) {
#ifdef ECSCOPE_HAS_IMGUI
    const ComponentMetadata* metadata = get_component_metadata(type);
    
    if (!metadata) {
        ImGui::Text("No metadata available for this component");
        return;
    }
    
    if (!metadata->is_editable) {
        ImGui::Text("This component is not editable");
        return;
    }
    
    // For now, show placeholder editor
    ImGui::Text("Component Editor for: %s", metadata->name.c_str());
    ImGui::Text("Size: %zu bytes", metadata->size);
    
    if (ImGui::Button("Remove##Component")) {
        // TODO: Remove component from entity
    }
#endif
}

void ECSInspector::render_add_component_dialog(EntityID entity) {
#ifdef ECSCOPE_HAS_IMGUI
    ImGui::Text("Add Component to Entity %u", static_cast<uint32_t>(entity));
    
    std::lock_guard<std::mutex> lock(component_mutex_);
    for (const auto& [type, metadata] : component_metadata_) {
        if (ImGui::Selectable(metadata.name.c_str())) {
            // TODO: Add component to entity with default values
            ImGui::CloseCurrentPopup();
        }
    }
#endif
}

void ECSInspector::render_multi_entity_operations() {
#ifdef ECSCOPE_HAS_IMGUI
    ImGui::Text("Batch Operations");
    
    if (ImGui::Button("Delete All Selected")) {
        std::vector<EntityID> to_delete(selection_state_.selected_entities.begin(), 
                                       selection_state_.selected_entities.end());
        destroy_entities(to_delete);
    }
    
    ImGui::Separator();
    
    // Show component intersection/union
    ImGui::Text("Common Operations:");
    if (ImGui::Button("Add Component to All")) {
        ImGui::OpenPopup("BatchAddComponent");
    }
    
    if (ImGui::BeginPopup("BatchAddComponent")) {
        ImGui::Text("Select component to add to all selected entities:");
        // Component selection would go here
        ImGui::EndPopup();
    }
#endif
}

void ECSInspector::render_query_builder_panel() {
#ifdef ECSCOPE_HAS_IMGUI
    if (ImGui::Begin("Query Builder", &show_query_builder_)) {
        ImGui::Text("ECS Query Builder");
        ImGui::Separator();
        
        // TODO: Implement query builder UI
        ImGui::Text("Query builder implementation coming soon...");
    }
    ImGui::End();
#endif
}

void ECSInspector::render_history_panel() {
#ifdef ECSCOPE_HAS_IMGUI
    if (ImGui::Begin("Change History", &show_history_panel_)) {
        if (ImGui::Button("Undo") && can_undo()) {
            undo_last_change();
        }
        ImGui::SameLine();
        if (ImGui::Button("Redo") && can_redo()) {
            redo_change();
        }
        ImGui::SameLine();
        if (ImGui::Button("Clear History")) {
            clear_history();
        }
        
        ImGui::Separator();
        
        // History list
        std::lock_guard<std::mutex> lock(component_mutex_);
        for (size_t i = 0; i < change_history_.size(); ++i) {
            const auto& change = change_history_[i];
            
            bool is_current = (i == history_position_ - 1);
            if (is_current) {
                ImGui::PushStyleColor(ImGuiCol_Text, COLOR_INFO);
            }
            
            ImGui::Text("[%u] %s - %s", 
                       static_cast<uint32_t>(change.entity),
                       change.description.c_str(),
                       format_duration(std::chrono::duration<float, std::milli>(
                           std::chrono::duration_cast<std::chrono::milliseconds>(
                               std::chrono::steady_clock::now() - change.timestamp
                           ).count()
                       )).c_str());
            
            if (is_current) {
                ImGui::PopStyleColor();
            }
        }
    }
    ImGui::End();
#endif
}

void ECSInspector::render_templates_panel() {
#ifdef ECSCOPE_HAS_IMGUI
    if (ImGui::Begin("Component Templates", &show_templates_panel_)) {
        ImGui::Text("Component Templates");
        ImGui::Separator();
        
        // TODO: Implement component templates UI
        ImGui::Text("Component templates implementation coming soon...");
    }
    ImGui::End();
#endif
}

void ECSInspector::render_settings_panel() {
#ifdef ECSCOPE_HAS_IMGUI
    if (ImGui::Begin("Inspector Settings", &show_settings_panel_)) {
        ImGui::Text("ECS Inspector Configuration");
        ImGui::Separator();
        
        // Update rates
        ImGui::SliderFloat("Entity Refresh Rate (ms)", &config_.entity_refresh_rate, 8.0f, 100.0f);
        ImGui::SliderFloat("Component Refresh Rate (ms)", &config_.component_refresh_rate, 16.0f, 200.0f);
        ImGui::SliderFloat("System Refresh Rate (ms)", &config_.system_refresh_rate, 50.0f, 1000.0f);
        
        ImGui::Separator();
        
        // Feature toggles
        ImGui::Checkbox("Enable Undo/Redo", &config_.enable_undo_redo);
        ImGui::Checkbox("Enable Component Validation", &config_.enable_component_validation);
        ImGui::Checkbox("Enable Realtime Updates", &config_.enable_realtime_updates);
        ImGui::Checkbox("Enable Advanced Filtering", &config_.enable_advanced_filtering);
        ImGui::Checkbox("Enable Batch Operations", &config_.enable_batch_operations);
        
        ImGui::Separator();
        
        // Performance limits
        ImGui::InputScalar("Max Entities Displayed", ImGuiDataType_U64, &config_.max_entities_displayed);
        ImGui::InputScalar("Max History Entries", ImGuiDataType_U64, &config_.max_history_entries);
        ImGui::SliderFloat("Max Update Time (ms)", &config_.max_update_time_ms, 1.0f, 50.0f);
        
        ImGui::Separator();
        
        // Preset buttons
        if (ImGui::Button("Performance Focused")) {
            config_ = InspectorConfig::create_performance_focused();
        }
        ImGui::SameLine();
        if (ImGui::Button("Debugging Focused")) {
            config_ = InspectorConfig::create_debugging_focused();
        }
    }
    ImGui::End();
#endif
}

void ECSInspector::render_metrics_panel() {
#ifdef ECSCOPE_HAS_IMGUI
    if (ImGui::Begin("Inspector Metrics", &show_metrics_panel_)) {
        ImGui::Text("Performance Metrics");
        ImGui::Separator();
        
        ImGui::Text("Last Update Time: %.3f ms", metrics_.last_update_time_ms);
        ImGui::Text("Last Render Time: %.3f ms", metrics_.last_render_time_ms);
        ImGui::Text("Entities Tracked: %zu", metrics_.entities_tracked);
        ImGui::Text("Components Tracked: %zu", metrics_.components_tracked);
        ImGui::Text("Systems Tracked: %zu", metrics_.systems_tracked);
        ImGui::Text("Memory Usage: %s", format_memory_size(metrics_.memory_usage_bytes).c_str());
        
        ImGui::Separator();
        
        ImGui::Text("Frame: %llu", frame_counter_.load());
        
        // Performance warning thresholds
        if (metrics_.last_update_time_ms > config_.max_update_time_ms) {
            ImGui::TextColored(COLOR_WARNING, "Warning: Update time exceeds threshold!");
        }
        
        if (metrics_.entities_tracked > config_.max_entities_displayed) {
            ImGui::TextColored(COLOR_WARNING, "Warning: Too many entities being tracked!");
        }
    }
    ImGui::End();
#endif
}

void ECSInspector::refresh_archetype_analysis() {
    // This would refresh archetype analysis data
    // Implementation depends on the specific ECS registry capabilities
}

void ECSInspector::clear_history() {
    std::lock_guard<std::mutex> lock(component_mutex_);
    change_history_.clear();
    history_position_ = 0;
}

bool ECSInspector::undo_last_change() {
    std::lock_guard<std::mutex> lock(component_mutex_);
    if (history_position_ == 0) {
        return false;
    }
    
    // TODO: Implement actual undo logic
    history_position_--;
    return true;
}

bool ECSInspector::redo_change() {
    std::lock_guard<std::mutex> lock(component_mutex_);
    if (history_position_ >= change_history_.size()) {
        return false;
    }
    
    // TODO: Implement actual redo logic
    history_position_++;
    return true;
}

} // namespace ecscope::gui