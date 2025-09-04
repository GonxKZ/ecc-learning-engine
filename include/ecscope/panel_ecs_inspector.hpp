#pragma once

#include "../overlay.hpp"
#include "ecs/registry.hpp"
#include "core/types.hpp"
#include <vector>
#include <string>

namespace ecscope::ui {

class ECSInspectorPanel : public Panel {
private:
    // Inspector state
    ecs::Entity selected_entity_{};
    bool entity_valid_{false};
    
    // Filters and search
    std::string entity_search_filter_;
    std::string component_search_filter_;
    bool show_empty_archetypes_{false};
    
    // Display options
    bool show_entity_details_{true};
    bool show_archetype_list_{true};
    bool show_component_inspector_{true};
    bool show_performance_stats_{true};
    
    // Cached data for performance
    struct CachedEntityInfo {
        ecs::Entity entity;
        usize archetype_index;
        usize component_count;
        std::string archetype_signature;
    };
    
    std::vector<CachedEntityInfo> cached_entities_;
    f64 cache_update_timer_{0.0};
    static constexpr f64 CACHE_UPDATE_INTERVAL = 0.5; // 500ms
    
    // Stats tracking
    struct Stats {
        f64 last_update_time{0.0};
        f64 render_time{0.0};
        usize visible_entities{0};
        usize filtered_entities{0};
    } stats_;
    
public:
    ECSInspectorPanel();
    
    void render() override;
    void update(f64 delta_time) override;
    
    // Entity selection
    void select_entity(ecs::Entity entity);
    ecs::Entity selected_entity() const { return selected_entity_; }
    
    // UI state
    void set_show_entity_details(bool show) { show_entity_details_ = show; }
    void set_show_archetype_list(bool show) { show_archetype_list_ = show; }
    void set_show_component_inspector(bool show) { show_component_inspector_ = show; }
    
private:
    void render_entity_list();
    void render_entity_details();
    void render_archetype_browser();
    void render_component_inspector();
    void render_performance_section();
    void render_controls();
    
    void update_entity_cache();
    bool entity_matches_filter(const CachedEntityInfo& info) const;
    
    void render_entity_context_menu(ecs::Entity entity);
    void render_archetype_context_menu(usize archetype_index);
    
    // Utility functions
    std::string format_entity_name(ecs::Entity entity) const;
    std::string format_archetype_signature(const ecs::ComponentSignature& signature) const;
    const char* get_component_type_name(core::ComponentID id) const;
};

} // namespace ecscope::ui