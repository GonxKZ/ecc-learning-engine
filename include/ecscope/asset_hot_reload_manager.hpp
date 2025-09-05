#pragma once

/**
 * @file asset_hot_reload_manager.hpp
 * @brief Advanced Asset Hot-Reload System for ECScope Asset Pipeline
 * 
 * This system provides comprehensive hot-reloading capabilities for all asset
 * types with dependency tracking, cascade updates, and educational features.
 * Integrates with the existing hot-reload system for script files.
 * 
 * Key Features:
 * - File system watching with minimal overhead
 * - Dependency tracking and cascade updates
 * - Asset-specific reload strategies
 * - Educational visualization of reload operations
 * - Integration with asset registry and memory management
 * - Performance impact monitoring
 * 
 * Educational Value:
 * - Demonstrates file system watching techniques
 * - Shows dependency graph algorithms
 * - Illustrates memory management during reloads
 * - Provides real-time development workflow insights
 * - Teaches optimization strategies for large asset sets
 * 
 * @author ECScope Educational ECS Framework - Asset Pipeline
 * @date 2025
 */

#include "asset_pipeline.hpp"
#include "hot_reload_system.hpp"
#include "core/types.hpp"
#include "core/log.hpp"
#include <filesystem>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <memory>
#include <atomic>
#include <shared_mutex>
#include <functional>
#include <chrono>
#include <queue>
#include <thread>
#include <condition_variable>

namespace ecscope::assets {

//=============================================================================
// Forward Declarations
//=============================================================================

class AssetRegistry;
class AssetHotReloadManager;

//=============================================================================
// Asset Change Event System
//=============================================================================

/**
 * @brief Types of asset change events
 */
enum class AssetChangeType : u8 {
    Modified = 1 << 0,    // Asset file was modified
    Created = 1 << 1,     // New asset file was created
    Deleted = 1 << 2,     // Asset file was deleted
    Moved = 1 << 3,       // Asset file was moved/renamed
    ImportSettingsChanged = 1 << 4, // Import settings were changed
    DependencyChanged = 1 << 5      // A dependency was changed
};

constexpr AssetChangeType operator|(AssetChangeType a, AssetChangeType b) {
    return static_cast<AssetChangeType>(static_cast<u8>(a) | static_cast<u8>(b));
}

constexpr bool operator&(AssetChangeType a, AssetChangeType b) {
    return static_cast<bool>(static_cast<u8>(a) & static_cast<u8>(b));
}

/**
 * @brief Asset change event with detailed information
 */
struct AssetChangeEvent {
    AssetID asset_id{INVALID_ASSET_ID};
    std::filesystem::path file_path;
    AssetChangeType change_type;
    AssetType asset_type;
    
    // Change details
    std::chrono::high_resolution_clock::time_point timestamp;
    usize file_size_before{0};
    usize file_size_after{0};
    std::string hash_before;
    std::string hash_after;
    
    // Dependency information
    std::vector<AssetID> affected_dependencies; // Assets that depend on this one
    std::vector<AssetID> dependency_chain;      // Full dependency chain
    
    // Educational metadata
    f64 detection_latency_ms{0.0};    // Time from file change to detection
    bool user_initiated{false};        // Whether change was user-initiated
    std::string change_description;     // Human-readable description
    
    AssetChangeEvent() = default;
    AssetChangeEvent(AssetID id, std::filesystem::path path, AssetChangeType type)
        : asset_id(id), file_path(std::move(path)), change_type(type)
        , timestamp(std::chrono::high_resolution_clock::now()) {}
    
    f64 get_age_milliseconds() const {
        auto now = std::chrono::high_resolution_clock::now();
        return std::chrono::duration<f64, std::milli>(now - timestamp).count();
    }
    
    bool is_significant_change() const {
        // Ignore trivial file size changes or unchanged hashes
        if (file_size_before == file_size_after && hash_before == hash_after) {
            return false;
        }
        return true;
    }
};

//=============================================================================
// Asset Dependency Tracking
//=============================================================================

/**
 * @brief Advanced dependency tracker for assets
 */
class AssetDependencyTracker {
public:
    /**
     * @brief Dependency relationship types
     */
    enum class DependencyType : u8 {
        DirectReference,     // Asset directly references another (texture in material)
        Include,            // Shader includes another shader file
        Import,             // Asset imports data from another file
        Generation,         // Asset is generated from another (LOD from base mesh)
        Configuration       // Asset depends on configuration file
    };
    
    /**
     * @brief Dependency edge with metadata
     */
    struct DependencyEdge {
        AssetID from_asset;
        AssetID to_asset;
        DependencyType type;
        std::string dependency_path;    // Path/identifier of the dependency
        f32 dependency_strength{1.0f}; // How critical this dependency is (0-1)
        bool is_optional{false};        // Whether the dependency is optional
        
        // Educational metadata
        std::string description;        // Human-readable description
        std::string educational_note;   // Educational explanation
    };
    
private:
    // Dependency graph storage
    std::unordered_map<AssetID, std::vector<DependencyEdge>> forward_dependencies_;  // Asset -> its dependencies
    std::unordered_map<AssetID, std::vector<DependencyEdge>> reverse_dependencies_; // Asset -> what depends on it
    mutable std::shared_mutex dependencies_mutex_;
    
    // Circular dependency detection
    mutable std::unordered_set<AssetID> visiting_;
    mutable std::unordered_set<AssetID> visited_;
    mutable std::vector<std::vector<AssetID>> circular_dependencies_;
    
    // Performance tracking
    std::atomic<u64> dependency_checks_{0};
    std::atomic<u64> cycle_detections_{0};
    std::atomic<f64> total_analysis_time_{0.0};
    
public:
    AssetDependencyTracker() = default;
    ~AssetDependencyTracker() = default;
    
    // Dependency management
    void add_dependency(AssetID from_asset, AssetID to_asset, DependencyType type, 
                       const std::string& path = "", f32 strength = 1.0f);
    void remove_dependency(AssetID from_asset, AssetID to_asset);
    void remove_all_dependencies(AssetID asset_id);
    void update_dependency_strength(AssetID from_asset, AssetID to_asset, f32 strength);
    
    // Dependency queries
    std::vector<DependencyEdge> get_dependencies(AssetID asset_id) const;
    std::vector<DependencyEdge> get_dependents(AssetID asset_id) const;
    std::vector<AssetID> get_all_dependencies_recursive(AssetID asset_id) const;
    std::vector<AssetID> get_all_dependents_recursive(AssetID asset_id) const;
    
    // Dependency analysis
    bool has_dependency(AssetID from_asset, AssetID to_asset) const;
    bool has_circular_dependencies() const;
    std::vector<std::vector<AssetID>> find_circular_dependencies() const;
    u32 get_dependency_depth(AssetID asset_id) const;
    f32 calculate_dependency_complexity(AssetID asset_id) const;
    
    // Reload impact analysis
    struct ReloadImpactAnalysis {
        std::vector<AssetID> directly_affected;    // Assets that directly depend on changed asset
        std::vector<AssetID> indirectly_affected;  // Assets affected through dependency chain
        std::vector<AssetID> reload_order;         // Optimal order to reload assets
        f64 estimated_reload_time{0.0};            // Estimated time for full reload
        usize memory_impact_estimate{0};           // Estimated memory usage during reload
        std::vector<std::string> warnings;         // Potential issues during reload
    };
    
    ReloadImpactAnalysis analyze_reload_impact(const std::vector<AssetID>& changed_assets) const;
    std::vector<AssetID> calculate_optimal_reload_order(const std::vector<AssetID>& assets_to_reload) const;
    
    // Educational features
    std::string export_dependency_graph_dot() const;
    std::string generate_dependency_report(AssetID asset_id) const;
    std::vector<std::string> get_educational_insights() const;
    
    // Statistics
    struct DependencyStatistics {
        u32 total_assets_tracked{0};
        u32 total_dependencies{0};
        f32 average_dependencies_per_asset{0.0f};
        u32 circular_dependency_count{0};
        f32 average_dependency_depth{0.0f};
        f32 graph_density{0.0f};             // How connected the graph is
        AssetID most_dependent_asset{INVALID_ASSET_ID};   // Asset with most dependencies
        AssetID most_referenced_asset{INVALID_ASSET_ID};  // Most depended-upon asset
    };
    
    DependencyStatistics get_statistics() const;
    void validate_dependency_graph() const;
    
private:
    // Internal algorithms
    bool detect_cycle_dfs(AssetID current, std::vector<AssetID>& current_path) const;
    void topological_sort_dfs(AssetID asset, std::vector<AssetID>& result, 
                             std::unordered_set<AssetID>& visited) const;
    f32 calculate_reload_time_estimate(AssetID asset_id, const std::unordered_set<AssetID>& context) const;
};

//=============================================================================
// Asset Reload Strategies
//=============================================================================

/**
 * @brief Asset-specific reload strategies
 */
class AssetReloadStrategy {
public:
    virtual ~AssetReloadStrategy() = default;
    
    /**
     * @brief Reload result information
     */
    struct ReloadResult {
        bool success{false};
        AssetID reloaded_asset{INVALID_ASSET_ID};
        f64 reload_time_ms{0.0};
        usize memory_delta_bytes{0}; // Change in memory usage
        std::vector<std::string> warnings;
        std::string error_message;
        
        // Educational information
        std::vector<std::string> steps_taken;
        std::string performance_impact;
        std::string optimization_opportunities;
    };
    
    // Strategy interface
    virtual bool can_handle(AssetType type) const = 0;
    virtual ReloadResult reload_asset(AssetID asset_id, AssetRegistry* registry, 
                                    const AssetChangeEvent& change_event) = 0;
    virtual f64 estimate_reload_time(AssetID asset_id, AssetRegistry* registry) const = 0;
    virtual std::string get_strategy_description() const = 0;
    
    // Educational features
    virtual std::vector<std::string> get_educational_points() const = 0;
    virtual std::string explain_reload_process(AssetType type) const = 0;
};

/**
 * @brief Texture-specific reload strategy
 */
class TextureReloadStrategy : public AssetReloadStrategy {
public:
    bool can_handle(AssetType type) const override;
    ReloadResult reload_asset(AssetID asset_id, AssetRegistry* registry, 
                            const AssetChangeEvent& change_event) override;
    f64 estimate_reload_time(AssetID asset_id, AssetRegistry* registry) const override;
    std::string get_strategy_description() const override;
    
    std::vector<std::string> get_educational_points() const override;
    std::string explain_reload_process(AssetType type) const override;
    
private:
    bool try_incremental_reload(AssetID asset_id, AssetRegistry* registry) const;
    bool needs_full_reload(AssetID asset_id, AssetRegistry* registry, 
                          const AssetChangeEvent& change_event) const;
};

/**
 * @brief Shader-specific reload strategy with live compilation
 */
class ShaderReloadStrategy : public AssetReloadStrategy {
public:
    bool can_handle(AssetType type) const override;
    ReloadResult reload_asset(AssetID asset_id, AssetRegistry* registry, 
                            const AssetChangeEvent& change_event) override;
    f64 estimate_reload_time(AssetID asset_id, AssetRegistry* registry) const override;
    std::string get_strategy_description() const override;
    
    std::vector<std::string> get_educational_points() const override;
    std::string explain_reload_process(AssetType type) const override;
    
private:
    bool validate_shader_before_reload(AssetID asset_id, AssetRegistry* registry) const;
    void rollback_shader_on_failure(AssetID asset_id, AssetRegistry* registry) const;
};

/**
 * @brief Model-specific reload strategy
 */
class ModelReloadStrategy : public AssetReloadStrategy {
public:
    bool can_handle(AssetType type) const override;
    ReloadResult reload_asset(AssetID asset_id, AssetRegistry* registry, 
                            const AssetChangeEvent& change_event) override;
    f64 estimate_reload_time(AssetID asset_id, AssetRegistry* registry) const override;
    std::string get_strategy_description() const override;
    
    std::vector<std::string> get_educational_points() const override;
    std::string explain_reload_process(AssetType type) const override;
    
private:
    bool can_reload_incrementally(AssetID asset_id, const AssetChangeEvent& change_event) const;
};

/**
 * @brief Audio-specific reload strategy
 */
class AudioReloadStrategy : public AssetReloadStrategy {
public:
    bool can_handle(AssetType type) const override;
    ReloadResult reload_asset(AssetID asset_id, AssetRegistry* registry, 
                            const AssetChangeEvent& change_event) override;
    f64 estimate_reload_time(AssetID asset_id, AssetRegistry* registry) const override;
    std::string get_strategy_description() const override;
    
    std::vector<std::string> get_educational_points() const override;
    std::string explain_reload_process(AssetType type) const override;
};

//=============================================================================
// Main Asset Hot-Reload Manager
//=============================================================================

/**
 * @brief Comprehensive asset hot-reload manager
 */
class AssetHotReloadManager {
public:
    /**
     * @brief Hot-reload configuration
     */
    struct Configuration {
        bool enabled{true};                      // Enable/disable hot reloading
        f64 file_check_interval_ms{100.0};      // File system polling interval
        f64 batch_delay_ms{200.0};              // Delay before processing batched changes
        u32 max_concurrent_reloads{4};          // Maximum simultaneous reloads
        bool enable_dependency_tracking{true};   // Track asset dependencies
        bool enable_cascade_reloads{true};      // Automatically reload dependent assets
        
        // Educational settings
        bool log_reload_operations{true};       // Log all reload operations for learning
        bool generate_reload_reports{true};     // Generate detailed reload reports
        bool track_performance_metrics{true};   // Track performance impact
        
        // Safety settings
        f64 max_reload_time_ms{5000.0};         // Maximum time for a single reload
        u32 max_retry_attempts{3};              // Retry failed reloads
        bool validate_before_reload{true};      // Validate assets before reloading
        bool backup_before_reload{false};       // Create backup before risky reloads
        
        // Filtering
        std::vector<std::string> watched_extensions; // File extensions to watch
        std::vector<std::string> ignored_paths;      // Paths to ignore
        std::vector<std::string> priority_paths;     // High-priority paths for faster processing
    };
    
    /**
     * @brief Reload operation result
     */
    struct ReloadOperation {
        AssetID asset_id;
        AssetChangeEvent change_event;
        AssetReloadStrategy::ReloadResult result;
        f64 total_time_ms{0.0};
        u32 retry_count{0};
        bool completed{false};
        std::vector<AssetID> cascade_reloads; // Assets that were reloaded as a result
        
        // Educational information
        std::string operation_description;
        std::vector<std::string> learning_points;
        std::string performance_analysis;
    };
    
private:
    // Core components
    AssetRegistry* asset_registry_;
    std::unique_ptr<AssetDependencyTracker> dependency_tracker_;
    std::unique_ptr<ecscope::scripting::hotreload::FileWatcher> file_watcher_;
    
    // Reload strategies
    std::vector<std::unique_ptr<AssetReloadStrategy>> reload_strategies_;
    std::unordered_map<AssetType, AssetReloadStrategy*> strategy_map_;
    
    // Configuration and state
    Configuration config_;
    std::atomic<bool> is_running_{false};
    std::atomic<bool> shutdown_requested_{false};
    
    // Event processing
    std::queue<AssetChangeEvent> pending_events_;
    std::mutex pending_events_mutex_;
    std::condition_variable event_condition_;
    
    // Background processing
    std::vector<std::thread> worker_threads_;
    std::queue<ReloadOperation> active_operations_;
    std::mutex operations_mutex_;
    
    // Performance tracking
    std::atomic<u64> total_reloads_{0};
    std::atomic<f64> total_reload_time_{0.0};
    std::atomic<u64> successful_reloads_{0};
    std::atomic<u64> failed_reloads_{0};
    
    // Educational features
    mutable std::vector<ReloadOperation> recent_operations_;
    mutable std::mutex recent_operations_mutex_;
    constexpr static usize MAX_RECENT_OPERATIONS = 100;
    
    // Event callbacks
    std::vector<std::function<void(const AssetChangeEvent&)>> change_listeners_;
    std::vector<std::function<void(const ReloadOperation&)>> reload_listeners_;
    
public:
    explicit AssetHotReloadManager(AssetRegistry* registry, const Configuration& config = Configuration{});
    ~AssetHotReloadManager();
    
    // Non-copyable
    AssetHotReloadManager(const AssetHotReloadManager&) = delete;
    AssetHotReloadManager& operator=(const AssetHotReloadManager&) = delete;
    
    // Core functionality
    bool initialize();
    void start();
    void stop();
    void update(); // Call once per frame
    
    // Asset watching
    bool watch_asset(AssetID asset_id, const std::filesystem::path& file_path);
    bool unwatch_asset(AssetID asset_id);
    void watch_directory(const std::filesystem::path& directory, bool recursive = true);
    void unwatch_directory(const std::filesystem::path& directory);
    
    // Manual reload triggering
    bool trigger_reload(AssetID asset_id, bool force = false);
    bool trigger_reload_cascade(AssetID asset_id, bool force = false);
    void trigger_full_asset_refresh();
    
    // Dependency management
    void register_dependency(AssetID from_asset, AssetID to_asset, 
                           AssetDependencyTracker::DependencyType type,
                           const std::string& path = "", f32 strength = 1.0f);
    void unregister_dependency(AssetID from_asset, AssetID to_asset);
    void analyze_asset_dependencies(AssetID asset_id);
    
    // Configuration
    void update_configuration(const Configuration& config);
    const Configuration& get_configuration() const { return config_; }
    void set_enabled(bool enabled);
    bool is_enabled() const { return config_.enabled; }
    
    // Event handling
    void add_change_listener(std::function<void(const AssetChangeEvent&)> listener);
    void add_reload_listener(std::function<void(const ReloadOperation&)> listener);
    void remove_all_listeners();
    
    // Statistics and monitoring
    struct HotReloadStatistics {
        u64 total_reloads{0};
        u64 successful_reloads{0};
        u64 failed_reloads{0};
        f64 success_rate{1.0};
        f64 average_reload_time{0.0};
        
        u32 files_watched{0};
        u32 directories_watched{0};
        u32 dependencies_tracked{0};
        
        // Per-asset-type statistics
        std::unordered_map<AssetType, u32> reloads_by_type;
        std::unordered_map<AssetType, f64> average_time_by_type;
        
        // Recent performance
        f64 recent_reload_frequency{0.0}; // Reloads per minute
        f64 peak_reload_time{0.0};        // Longest reload time
        u32 cascade_reload_count{0};      // Number of cascade reloads
        
        // Educational metrics
        u32 learning_opportunities_generated{0};
        f64 total_educational_content_time{0.0};
    };
    
    HotReloadStatistics get_statistics() const;
    void reset_statistics();
    
    // Educational features
    std::vector<ReloadOperation> get_recent_operations() const;
    std::string generate_reload_report() const;
    std::string generate_dependency_visualization() const;
    std::string generate_performance_analysis() const;
    
    // Asset dependency queries
    AssetDependencyTracker::ReloadImpactAnalysis analyze_reload_impact(AssetID asset_id) const;
    std::vector<AssetID> get_asset_dependencies(AssetID asset_id) const;
    std::vector<AssetID> get_asset_dependents(AssetID asset_id) const;
    
    // Debugging and validation
    bool validate_watch_system() const;
    std::vector<std::string> get_watched_paths() const;
    std::string diagnose_reload_issues(AssetID asset_id) const;
    
private:
    // Initialization
    void initialize_reload_strategies();
    void initialize_file_watcher();
    void initialize_worker_threads();
    
    // Event processing
    void process_file_change_event(const ecscope::scripting::hotreload::FileEvent& file_event);
    void enqueue_asset_change_event(const AssetChangeEvent& event);
    void process_pending_events();
    void batch_process_events(const std::vector<AssetChangeEvent>& events);
    
    // Reload execution
    void worker_thread_main();
    bool execute_reload_operation(ReloadOperation& operation);
    void handle_reload_failure(ReloadOperation& operation);
    void execute_cascade_reloads(const ReloadOperation& primary_operation);
    
    // Strategy management
    AssetReloadStrategy* get_strategy_for_asset(AssetType type);
    void register_strategy(std::unique_ptr<AssetReloadStrategy> strategy);
    
    // Asset identification
    AssetID find_asset_by_path(const std::filesystem::path& path) const;
    AssetType detect_asset_type(const std::filesystem::path& path) const;
    
    // Educational content generation
    void generate_educational_content(ReloadOperation& operation);
    std::string analyze_reload_performance(const ReloadOperation& operation) const;
    std::vector<std::string> extract_learning_points(const ReloadOperation& operation) const;
    
    // Utility functions
    bool is_file_worth_watching(const std::filesystem::path& path) const;
    f64 get_current_time_ms() const;
    void cleanup_old_operations();
    
    // Event notification
    void notify_change_listeners(const AssetChangeEvent& event);
    void notify_reload_listeners(const ReloadOperation& operation);
};

//=============================================================================
// Educational Visualization
//=============================================================================

/**
 * @brief Educational tools for visualizing hot-reload operations
 */
class HotReloadEducationVisualizer {
private:
    const AssetHotReloadManager& hot_reload_manager_;
    
public:
    explicit HotReloadEducationVisualizer(const AssetHotReloadManager& manager)
        : hot_reload_manager_(manager) {}
    
    // Dependency visualization
    std::string generate_dependency_graph_dot() const;
    std::string generate_dependency_tree_ascii(AssetID root_asset) const;
    std::string generate_reload_timeline_visualization() const;
    
    // Performance visualization
    struct PerformanceVisualization {
        std::vector<f64> reload_times;
        std::vector<std::string> asset_names;
        std::vector<AssetType> asset_types;
        f64 total_time{0.0};
        f64 average_time{0.0};
        f64 peak_time{0.0};
    };
    
    PerformanceVisualization generate_performance_data() const;
    
    // Educational reports
    std::string generate_hot_reload_tutorial() const;
    std::string generate_dependency_management_guide() const;
    std::string generate_optimization_recommendations() const;
    
    // Interactive demonstrations
    struct InteractiveDemo {
        std::string title;
        std::string description;
        std::vector<std::string> steps;
        std::function<void()> execute_step;
        std::string expected_outcome;
    };
    
    std::vector<InteractiveDemo> generate_interactive_demos() const;
    
    // Real-time monitoring data for UI
    struct RealtimeData {
        u32 active_watchers{0};
        u32 pending_reloads{0};
        f64 current_reload_rate{0.0};
        std::vector<std::string> recent_activity;
        bool system_healthy{true};
    };
    
    RealtimeData get_realtime_data() const;
};

} // namespace ecscope::assets