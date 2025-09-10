#pragma once

#include "asset.hpp"
#include "asset_database.hpp"
#include <unordered_map>
#include <shared_mutex>
#include <functional>

namespace ecscope::assets {

    // Asset dependency graph
    class DependencyGraph {
    public:
        void add_dependency(AssetID asset, AssetID dependency);
        void remove_dependency(AssetID asset, AssetID dependency);
        void remove_asset(AssetID asset);
        
        std::vector<AssetID> get_dependencies(AssetID asset) const;
        std::vector<AssetID> get_dependents(AssetID asset) const;
        std::vector<AssetID> get_load_order(const std::vector<AssetID>& assets) const;
        
        bool has_circular_dependency(AssetID asset) const;
        std::vector<AssetID> find_circular_dependencies() const;
        
        void clear();
        size_t get_asset_count() const;
        
    private:
        std::unordered_map<AssetID, std::vector<AssetID>> m_dependencies;
        std::unordered_map<AssetID, std::vector<AssetID>> m_dependents;
        mutable std::shared_mutex m_mutex;
        
        bool has_circular_dependency_recursive(AssetID asset, std::unordered_set<AssetID>& visited, 
                                              std::unordered_set<AssetID>& in_stack) const;
    };

    // Asset registry - central repository for asset management
    class AssetRegistry {
    public:
        AssetRegistry();
        ~AssetRegistry();
        
        // Asset registration
        AssetID register_asset(std::shared_ptr<Asset> asset);
        AssetID register_asset(const std::string& path, AssetType type);
        bool unregister_asset(AssetID id);
        bool unregister_asset(const std::string& path);
        
        // Asset lookup
        std::shared_ptr<Asset> get_asset(AssetID id) const;
        std::shared_ptr<Asset> get_asset(const std::string& path) const;
        AssetID get_asset_id(const std::string& path) const;
        std::string get_asset_path(AssetID id) const;
        
        // Asset queries
        bool has_asset(AssetID id) const;
        bool has_asset(const std::string& path) const;
        bool is_asset_loaded(AssetID id) const;
        bool is_asset_loaded(const std::string& path) const;
        
        // Asset collections
        std::vector<AssetID> get_all_assets() const;
        std::vector<AssetID> get_assets_by_type(AssetType type) const;
        std::vector<AssetID> get_assets_by_state(AssetState state) const;
        std::vector<AssetID> find_assets(const std::string& pattern) const;
        
        // Asset metadata
        bool set_asset_metadata(AssetID id, const AssetMetadata& metadata);
        AssetMetadata get_asset_metadata(AssetID id) const;
        bool update_asset_metadata(AssetID id, const std::function<void(AssetMetadata&)>& updater);
        
        // Dependency management
        void add_dependency(AssetID asset, AssetID dependency);
        void remove_dependency(AssetID asset, AssetID dependency);
        std::vector<AssetID> get_dependencies(AssetID asset) const;
        std::vector<AssetID> get_dependents(AssetID asset) const;
        std::vector<AssetID> get_load_order(const std::vector<AssetID>& assets) const;
        
        // Reference counting
        uint32_t get_reference_count(AssetID id) const;
        void add_reference(AssetID id);
        void remove_reference(AssetID id);
        std::vector<AssetID> get_unreferenced_assets() const;
        
        // Memory management
        size_t get_memory_usage() const;
        size_t get_asset_count() const;
        void collect_garbage();
        std::vector<AssetID> find_assets_for_cleanup() const;
        
        // Asset versioning
        AssetVersion get_asset_version(AssetID id) const;
        void increment_asset_version(AssetID id);
        bool is_asset_outdated(AssetID id) const;
        
        // Asset state management
        void set_asset_state(AssetID id, AssetState state);
        AssetState get_asset_state(AssetID id) const;
        void mark_asset_dirty(AssetID id);
        std::vector<AssetID> get_dirty_assets() const;
        
        // Database integration
        void set_database(std::shared_ptr<AssetDatabase> database);
        std::shared_ptr<AssetDatabase> get_database() const { return m_database; }
        bool save_to_database();
        bool load_from_database();
        
        // Event system
        using AssetEventCallback = std::function<void(AssetID, AssetState, AssetState)>;
        void register_state_change_callback(AssetEventCallback callback);
        void unregister_state_change_callback(AssetEventCallback callback);
        
        // Debugging and profiling
        void dump_registry_info() const;
        void dump_dependency_graph() const;
        std::vector<std::pair<AssetID, size_t>> get_memory_usage_by_asset() const;
        
        // Thread safety
        void lock_shared() const { m_mutex.lock_shared(); }
        void unlock_shared() const { m_mutex.unlock_shared(); }
        void lock() const { m_mutex.lock(); }
        void unlock() const { m_mutex.unlock(); }
        
    private:
        // Asset storage
        std::unordered_map<AssetID, std::shared_ptr<Asset>> m_assets_by_id;
        std::unordered_map<std::string, AssetID> m_path_to_id;
        std::unordered_map<AssetID, AssetMetadata> m_metadata;
        
        // Reference counting
        std::unordered_map<AssetID, uint32_t> m_reference_counts;
        
        // Dependency management
        std::unique_ptr<DependencyGraph> m_dependency_graph;
        
        // State tracking
        std::unordered_set<AssetID> m_dirty_assets;
        
        // Database
        std::shared_ptr<AssetDatabase> m_database;
        
        // Event callbacks
        std::vector<AssetEventCallback> m_state_change_callbacks;
        
        // Thread safety
        mutable std::shared_mutex m_mutex;
        mutable std::mutex m_callbacks_mutex;
        
        // Internal methods
        void notify_state_change(AssetID id, AssetState old_state, AssetState new_state);
        AssetID generate_unique_id();
        void cleanup_asset_internal(AssetID id);
    };

    // Asset registry configuration
    struct AssetRegistryConfig {
        size_t initial_capacity = 1000;
        bool enable_reference_counting = true;
        bool enable_dependency_tracking = true;
        bool enable_versioning = true;
        bool enable_state_tracking = true;
        bool auto_cleanup_unreferenced = true;
        std::chrono::seconds cleanup_interval{60};
    };

    // Scoped asset reference - RAII wrapper for asset references
    class ScopedAssetReference {
    public:
        ScopedAssetReference(AssetRegistry& registry, AssetID id);
        ~ScopedAssetReference();
        
        ScopedAssetReference(const ScopedAssetReference&) = delete;
        ScopedAssetReference& operator=(const ScopedAssetReference&) = delete;
        
        ScopedAssetReference(ScopedAssetReference&& other) noexcept;
        ScopedAssetReference& operator=(ScopedAssetReference&& other) noexcept;
        
        AssetID get_id() const { return m_id; }
        std::shared_ptr<Asset> get_asset() const;
        
        void release();
        
    private:
        AssetRegistry* m_registry;
        AssetID m_id;
        bool m_active;
    };

    // Asset registry factory
    std::unique_ptr<AssetRegistry> create_asset_registry(const AssetRegistryConfig& config = AssetRegistryConfig{});

} // namespace ecscope::assets