#pragma once

#include "../asset.hpp"
#include "../../ecs/world.hpp"
#include "../../ecs/component.hpp"
#include "../../ecs/system.hpp"

namespace ecscope::assets::integrations {

    // Asset reference component for ECS
    struct AssetReferenceComponent : public ecs::Component<AssetReferenceComponent> {
        AssetHandle handle;
        LoadPriority priority = Priority::NORMAL;
        LoadFlags flags = LoadFlags::NONE;
        QualityLevel quality = QualityLevel::MEDIUM;
        
        // Loading state
        bool is_loading = false;
        bool load_requested = false;
        
        AssetReferenceComponent() = default;
        AssetReferenceComponent(AssetHandle h) : handle(std::move(h)) {}
        AssetReferenceComponent(const std::string& path, LoadPriority prio = Priority::NORMAL)
            : priority(prio), load_requested(true) {}
    };

    // Multiple asset references
    struct AssetCollectionComponent : public ecs::Component<AssetCollectionComponent> {
        std::vector<AssetHandle> assets;
        std::unordered_map<std::string, AssetHandle> named_assets;
        
        void add_asset(const std::string& name, AssetHandle handle);
        AssetHandle get_asset(const std::string& name) const;
        bool has_asset(const std::string& name) const;
        void remove_asset(const std::string& name);
        
        size_t get_asset_count() const { return assets.size() + named_assets.size(); }
        bool all_assets_loaded() const;
    };

    // Asset dependency component
    struct AssetDependencyComponent : public ecs::Component<AssetDependencyComponent> {
        std::vector<AssetID> dependencies;
        std::vector<AssetID> dependents;
        bool dependencies_loaded = false;
        
        void add_dependency(AssetID dependency);
        void remove_dependency(AssetID dependency);
        bool has_dependency(AssetID dependency) const;
        
        void add_dependent(AssetID dependent);
        void remove_dependent(AssetID dependent);
        
        float get_dependency_load_progress() const;
    };

    // Asset streaming component
    struct AssetStreamingComponent : public ecs::Component<AssetStreamingComponent> {
        QualityLevel target_quality = QualityLevel::MEDIUM;
        float distance_to_camera = 0.0f;
        float importance_factor = 1.0f;
        bool auto_adjust_quality = true;
        bool preload_enabled = false;
        
        // Streaming state
        bool is_streaming = false;
        float streaming_progress = 0.0f;
        QualityLevel current_quality = QualityLevel::LOW;
        
        // Quality thresholds based on distance
        struct QualityThreshold {
            float distance;
            QualityLevel quality;
            
            QualityThreshold(float d, QualityLevel q) : distance(d), quality(q) {}
        };
        
        std::vector<QualityThreshold> quality_thresholds = {
            {10.0f, QualityLevel::ULTRA},
            {25.0f, QualityLevel::HIGH},
            {50.0f, QualityLevel::MEDIUM},
            {100.0f, QualityLevel::LOW}
        };
        
        QualityLevel select_quality_for_distance(float distance) const;
    };

    // Asset loading system for ECS
    class ECSAssetLoadingSystem : public ecs::System {
    public:
        ECSAssetLoadingSystem();
        
        void update(float delta_time) override;
        
        // Configuration
        void set_max_loads_per_frame(size_t max_loads) { m_max_loads_per_frame = max_loads; }
        size_t get_max_loads_per_frame() const { return m_max_loads_per_frame; }
        
        void set_loading_budget_ms(float budget_ms) { m_loading_budget_ms = budget_ms; }
        float get_loading_budget_ms() const { return m_loading_budget_ms; }
        
        // Statistics
        size_t get_pending_loads() const { return m_pending_loads; }
        size_t get_active_loads() const { return m_active_loads; }
        size_t get_completed_loads() const { return m_completed_loads; }
        
    private:
        size_t m_max_loads_per_frame = 5;
        float m_loading_budget_ms = 2.0f;
        
        // Statistics
        size_t m_pending_loads = 0;
        size_t m_active_loads = 0;
        size_t m_completed_loads = 0;
        
        // Internal state
        std::chrono::steady_clock::time_point m_frame_start_time;
        float m_frame_time_used = 0.0f;
        
        void process_asset_loading_requests();
        void update_loading_progress();
        void handle_completed_loads();
        
        bool has_budget_remaining() const;
        void start_asset_load(ecs::Entity entity, AssetReferenceComponent& asset_ref);
    };

    // Asset streaming system for ECS
    class ECSAssetStreamingSystem : public ecs::System {
    public:
        ECSAssetStreamingSystem();
        
        void update(float delta_time) override;
        
        // Camera position for distance calculations
        void set_camera_position(float x, float y, float z);
        std::array<float, 3> get_camera_position() const { return m_camera_position; }
        
        // Streaming configuration
        void set_streaming_enabled(bool enabled) { m_streaming_enabled = enabled; }
        bool is_streaming_enabled() const { return m_streaming_enabled; }
        
        void set_max_streaming_distance(float distance) { m_max_streaming_distance = distance; }
        float get_max_streaming_distance() const { return m_max_streaming_distance; }
        
        // Statistics
        size_t get_streaming_asset_count() const { return m_streaming_assets; }
        size_t get_pending_quality_changes() const { return m_pending_quality_changes; }
        
    private:
        std::array<float, 3> m_camera_position = {0, 0, 0};
        bool m_streaming_enabled = true;
        float m_max_streaming_distance = 200.0f;
        
        // Statistics
        size_t m_streaming_assets = 0;
        size_t m_pending_quality_changes = 0;
        
        void update_asset_distances();
        void adjust_asset_qualities();
        void process_streaming_requests();
        
        float calculate_distance(const std::array<float, 3>& position) const;
    };

    // Asset dependency system for ECS
    class ECSAssetDependencySystem : public ecs::System {
    public:
        ECSAssetDependencySystem();
        
        void update(float delta_time) override;
        
        // Dependency management
        void add_asset_dependency(ecs::Entity entity, AssetID dependency);
        void remove_asset_dependency(ecs::Entity entity, AssetID dependency);
        void resolve_dependencies(ecs::Entity entity);
        
        // Statistics
        size_t get_dependency_count() const { return m_total_dependencies; }
        size_t get_resolved_dependencies() const { return m_resolved_dependencies; }
        float get_resolution_progress() const;
        
    private:
        size_t m_total_dependencies = 0;
        size_t m_resolved_dependencies = 0;
        
        void check_dependency_resolution();
        void update_dependent_entities(AssetID dependency);
        
        // Dependency graph for circular dependency detection
        std::unordered_map<AssetID, std::vector<AssetID>> m_dependency_graph;
        bool has_circular_dependency(AssetID asset_id) const;
    };

    // Asset cleanup system for ECS
    class ECSAssetCleanupSystem : public ecs::System {
    public:
        ECSAssetCleanupSystem();
        
        void update(float delta_time) override;
        
        // Cleanup configuration
        void set_cleanup_interval(std::chrono::seconds interval) { m_cleanup_interval = interval; }
        std::chrono::seconds get_cleanup_interval() const { return m_cleanup_interval; }
        
        void set_unused_threshold(std::chrono::seconds threshold) { m_unused_threshold = threshold; }
        std::chrono::seconds get_unused_threshold() const { return m_unused_threshold; }
        
        // Manual cleanup
        void force_cleanup();
        size_t cleanup_unused_assets();
        
        // Statistics
        size_t get_cleaned_up_assets() const { return m_cleaned_up_assets; }
        size_t get_memory_freed_mb() const { return m_memory_freed_bytes / 1024 / 1024; }
        
    private:
        std::chrono::seconds m_cleanup_interval{30}; // 30 seconds
        std::chrono::seconds m_unused_threshold{60}; // 1 minute
        std::chrono::steady_clock::time_point m_last_cleanup;
        
        // Statistics
        size_t m_cleaned_up_assets = 0;
        size_t m_memory_freed_bytes = 0;
        
        void perform_cleanup();
        bool is_asset_unused(const AssetHandle& handle) const;
        std::chrono::steady_clock::time_point get_last_access_time(const AssetHandle& handle) const;
    };

    // Utility functions for ECS integration
    namespace utils {
        
        // Entity creation with assets
        ecs::Entity create_entity_with_asset(ecs::World& world, 
                                            const std::string& asset_path,
                                            LoadPriority priority = Priority::NORMAL);
        
        ecs::Entity create_entity_with_assets(ecs::World& world,
                                             const std::vector<std::string>& asset_paths,
                                             LoadPriority priority = Priority::NORMAL);
        
        // Asset preloading for entities
        void preload_entity_assets(ecs::World& world, ecs::Entity entity);
        void preload_scene_assets(ecs::World& world, const std::vector<ecs::Entity>& entities);
        
        // Asset state queries
        bool are_entity_assets_loaded(ecs::World& world, ecs::Entity entity);
        float get_entity_asset_load_progress(ecs::World& world, ecs::Entity entity);
        
        // Bulk operations
        void set_entity_asset_quality(ecs::World& world, ecs::Entity entity, QualityLevel quality);
        void enable_entity_asset_streaming(ecs::World& world, ecs::Entity entity, bool enable);
        
        // Asset replacement
        void replace_entity_asset(ecs::World& world, ecs::Entity entity, 
                                const std::string& old_asset_path, 
                                const std::string& new_asset_path);
        
        // Statistics collection
        struct ECSAssetStatistics {
            size_t entities_with_assets = 0;
            size_t total_asset_references = 0;
            size_t loaded_assets = 0;
            size_t loading_assets = 0;
            size_t streaming_assets = 0;
            size_t memory_usage_mb = 0;
        };
        
        ECSAssetStatistics collect_ecs_asset_statistics(const ecs::World& world);
    }

    // Asset component factory
    class ECSAssetComponentFactory {
    public:
        // Component creation
        static AssetReferenceComponent create_texture_reference(const std::string& path, 
                                                               QualityLevel quality = QualityLevel::MEDIUM);
        static AssetReferenceComponent create_model_reference(const std::string& path,
                                                             QualityLevel quality = QualityLevel::MEDIUM);
        static AssetReferenceComponent create_audio_reference(const std::string& path,
                                                             QualityLevel quality = QualityLevel::MEDIUM);
        static AssetReferenceComponent create_shader_reference(const std::string& path);
        
        // Collection creation
        static AssetCollectionComponent create_material_collection(const std::string& material_path);
        static AssetCollectionComponent create_character_collection(const std::string& base_path);
        static AssetCollectionComponent create_level_collection(const std::vector<std::string>& asset_paths);
        
        // Streaming configuration
        static AssetStreamingComponent create_streaming_config(QualityLevel base_quality,
                                                             float max_distance,
                                                             bool auto_adjust = true);
        
        // Dependency setup
        static AssetDependencyComponent create_dependency_chain(const std::vector<AssetID>& dependencies);
    };

} // namespace ecscope::assets::integrations