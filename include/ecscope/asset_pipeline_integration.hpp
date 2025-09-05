#pragma once

/**
 * @file asset_pipeline_integration.hpp
 * @brief Complete Asset Pipeline Integration for ECScope Educational Platform
 * 
 * This header provides the main integration layer that ties together all
 * components of the asset pipeline system with the existing ECScope infrastructure.
 * It serves as the primary interface for using the asset pipeline in applications.
 * 
 * Key Features:
 * - Unified asset pipeline manager
 * - Integration with memory management systems
 * - Scene editor integration
 * - Educational system integration
 * - Performance monitoring and optimization
 * - Hot-reload system integration
 * 
 * Educational Value:
 * - Demonstrates system integration patterns
 * - Shows how to coordinate multiple subsystems
 * - Illustrates dependency injection and inversion of control
 * - Provides examples of factory patterns and service locators
 * - Teaches modular architecture design
 * 
 * @author ECScope Educational ECS Framework - Asset Pipeline
 * @date 2025
 */

#include "asset_pipeline.hpp"
#include "asset_loader.hpp"
#include "asset_hot_reload_manager.hpp"
#include "asset_education_system.hpp"
#include "texture_importer.hpp"
#include "model_importer.hpp"
#include "audio_importer.hpp"
#include "shader_importer.hpp"
#include "memory/memory_tracker.hpp"
#include "learning/tutorial_system.hpp"
#include "core/types.hpp"
#include "core/log.hpp"
#include <memory>
#include <vector>
#include <unordered_map>
#include <functional>
#include <atomic>
#include <string>

namespace ecscope::assets {

//=============================================================================
// Forward Declarations
//=============================================================================

class AssetPipelineManager;
class SceneEditorIntegration;
class MemorySystemIntegration;

//=============================================================================
// Asset Pipeline Configuration
//=============================================================================

/**
 * @brief Comprehensive configuration for the entire asset pipeline
 */
struct AssetPipelineConfig {
    // Core system settings
    bool enable_hot_reloading{true};
    bool enable_async_loading{true};
    bool enable_caching{true};
    bool enable_educational_features{true};
    
    // Memory management
    usize memory_budget_bytes{1024 * 1024 * 1024}; // 1 GB default
    usize cache_memory_limit{256 * 1024 * 1024};   // 256 MB for cache
    bool use_memory_pools{true};
    bool track_memory_usage{true};
    
    // Threading configuration
    u32 loader_thread_count{4};
    u32 import_thread_count{2};
    u32 max_concurrent_operations{8};
    
    // Hot-reload settings
    AssetHotReloadManager::Configuration hot_reload_config;
    
    // Loader settings
    AssetLoader::LoaderConfig loader_config;
    
    // Educational settings
    education::AssetEducationSystem::EducationConfig education_config;
    
    // Integration settings
    bool integrate_with_scene_editor{true};
    bool integrate_with_memory_tracker{true};
    bool integrate_with_learning_system{true};
    bool integrate_with_physics_materials{true};
    
    // Performance settings
    bool enable_performance_profiling{true};
    bool enable_optimization_analysis{true};
    f64 performance_update_interval_seconds{1.0};
    
    // Debug and validation
    bool enable_debug_validation{false};
    bool enable_asset_validation{true};
    bool log_all_operations{false};
    std::string debug_output_directory;
};

//=============================================================================
// Asset Pipeline Events
//=============================================================================

/**
 * @brief Event system for asset pipeline notifications
 */
namespace events {

/**
 * @brief Base class for asset pipeline events
 */
struct AssetPipelineEvent {
    std::chrono::high_resolution_clock::time_point timestamp;
    std::string event_id;
    AssetID asset_id{INVALID_ASSET_ID};
    
    AssetPipelineEvent() : timestamp(std::chrono::high_resolution_clock::now()) {}
    virtual ~AssetPipelineEvent() = default;
};

/**
 * @brief Asset loading events
 */
struct AssetLoadedEvent : public AssetPipelineEvent {
    LoadingResult result;
    f64 load_time_ms{0.0};
    std::string loader_thread_id;
};

struct AssetLoadFailedEvent : public AssetPipelineEvent {
    std::string error_message;
    u32 retry_count{0};
    bool will_retry{false};
};

/**
 * @brief Hot-reload events
 */
struct AssetReloadedEvent : public AssetPipelineEvent {
    AssetChangeEvent change_event;
    std::vector<AssetID> cascade_reloaded_assets;
    f64 reload_time_ms{0.0};
};

/**
 * @brief Educational events
 */
struct EducationalAnalysisEvent : public AssetPipelineEvent {
    education::EducationalMetrics metrics;
    education::OptimizationAnalyzer::AnalysisResult analysis;
    std::string student_context;
};

/**
 * @brief Memory management events
 */
struct MemoryThresholdEvent : public AssetPipelineEvent {
    usize current_usage{0};
    usize threshold{0};
    f32 usage_percentage{0.0f};
    std::string action_taken;
};

} // namespace events

//=============================================================================
// Scene Editor Integration
//=============================================================================

/**
 * @brief Integration layer for scene editor functionality
 */
class SceneEditorIntegration {
public:
    /**
     * @brief Asset preview data for scene editor
     */
    struct AssetPreviewData {
        AssetID asset_id{INVALID_ASSET_ID};
        AssetType type{AssetType::Unknown};
        std::string name;
        std::string thumbnail_path;
        
        // Type-specific preview data
        std::variant<
            std::monostate,
            renderer::resources::TextureData,    // Texture preview
            // Add other preview types as needed
            std::vector<u8>                      // Generic binary preview
        > preview_data;
        
        // Metadata for editor display
        std::unordered_map<std::string, std::string> display_properties;
        f32 preview_scale{1.0f};
        bool is_valid{false};
    };
    
    /**
     * @brief Asset drag-and-drop data
     */
    struct DragDropData {
        std::vector<std::filesystem::path> file_paths;
        std::vector<AssetType> detected_types;
        std::string operation{"import"}; // "import", "reference", "embed"
        std::unordered_map<AssetType, std::unique_ptr<ImportSettings>> import_settings;
    };
    
private:
    AssetPipelineManager* pipeline_manager_{nullptr};
    
    // Preview cache
    std::unordered_map<AssetID, AssetPreviewData> preview_cache_;
    mutable std::shared_mutex preview_cache_mutex_;
    
    // Drag and drop state
    std::optional<DragDropData> current_drag_data_;
    
public:
    explicit SceneEditorIntegration(AssetPipelineManager* manager);
    ~SceneEditorIntegration() = default;
    
    // Preview generation
    std::future<AssetPreviewData> generate_asset_preview(AssetID asset_id, 
                                                        u32 thumbnail_size = 256);
    AssetPreviewData get_cached_preview(AssetID asset_id) const;
    void invalidate_preview(AssetID asset_id);
    
    // Drag and drop support
    bool start_drag_operation(const std::vector<std::filesystem::path>& files);
    bool can_drop_files(const std::vector<std::filesystem::path>& files) const;
    std::vector<AssetID> drop_files(const std::vector<std::filesystem::path>& files,
                                   const std::unordered_map<AssetType, std::unique_ptr<ImportSettings>>& settings = {});
    
    // Asset browser integration
    std::vector<AssetID> get_assets_in_directory(const std::filesystem::path& directory) const;
    std::vector<AssetID> search_assets(const std::string& query) const;
    std::vector<AssetID> filter_assets_by_type(const std::vector<AssetID>& assets, AssetType type) const;
    
    // Property editor integration
    std::unordered_map<std::string, std::string> get_asset_properties(AssetID asset_id) const;
    bool update_asset_property(AssetID asset_id, const std::string& property, const std::string& value);
    std::unique_ptr<ImportSettings> get_import_settings_for_editing(AssetID asset_id) const;
    bool apply_import_settings(AssetID asset_id, const ImportSettings& settings);
    
    // Educational integration
    std::string get_asset_educational_info(AssetID asset_id) const;
    std::vector<std::string> get_asset_optimization_suggestions(AssetID asset_id) const;
    
private:
    AssetType detect_asset_type_from_extension(const std::filesystem::path& file_path) const;
    std::string generate_thumbnail_path(AssetID asset_id, u32 size) const;
};

//=============================================================================
// Memory System Integration
//=============================================================================

/**
 * @brief Integration with ECScope memory management systems
 */
class MemorySystemIntegration {
public:
    /**
     * @brief Memory allocation strategy for assets
     */
    enum class AllocationStrategy {
        Default,              // Use default allocators
        PooledByType,        // Use type-specific memory pools
        LargeAssetPool,      // Use specialized pools for large assets
        GPUOptimized,        // Optimize for GPU upload
        Educational         // Use educational memory tracking
    };
    
    /**
     * @brief Memory usage breakdown
     */
    struct MemoryUsageBreakdown {
        std::unordered_map<AssetType, usize> usage_by_type;
        std::unordered_map<std::string, usize> usage_by_category; // "cache", "active", "loading"
        usize total_usage{0};
        usize peak_usage{0};
        f32 fragmentation_ratio{0.0f};
        u32 allocation_count{0};
    };
    
private:
    memory::MemoryTracker* memory_tracker_{nullptr};
    AllocationStrategy strategy_{AllocationStrategy::Default};
    
    // Memory pools for different asset types
    std::unordered_map<AssetType, std::unique_ptr<memory::PoolAllocator>> type_pools_;
    
    // Memory monitoring
    std::atomic<usize> total_asset_memory_{0};
    std::atomic<usize> peak_asset_memory_{0};
    std::thread memory_monitor_thread_;
    std::atomic<bool> monitoring_active_{false};
    
    // Educational memory tracking
    std::vector<std::pair<std::chrono::high_resolution_clock::time_point, usize>> memory_timeline_;
    mutable std::mutex timeline_mutex_;
    
public:
    explicit MemorySystemIntegration(memory::MemoryTracker* tracker);
    ~MemorySystemIntegration();
    
    // Memory management
    void set_allocation_strategy(AllocationStrategy strategy);
    AllocationStrategy get_allocation_strategy() const { return strategy_; }
    
    // Memory allocation for assets
    void* allocate_for_asset(AssetID asset_id, usize size, usize alignment, AssetType type);
    void deallocate_for_asset(void* ptr, AssetID asset_id, AssetType type);
    
    // Memory monitoring
    void start_monitoring();
    void stop_monitoring();
    MemoryUsageBreakdown get_memory_breakdown() const;
    
    // Memory optimization
    usize cleanup_unused_memory();
    usize defragment_memory();
    bool is_memory_pressure_high() const;
    
    // Educational features
    std::vector<std::pair<f64, usize>> get_memory_timeline() const; // Time, usage
    std::string generate_memory_usage_report() const;
    std::vector<std::string> get_memory_optimization_suggestions() const;
    
private:
    void initialize_memory_pools();
    void memory_monitoring_worker();
    void record_memory_usage();
    memory::PoolAllocator* get_pool_for_type(AssetType type);
};

//=============================================================================
// Main Asset Pipeline Manager
//=============================================================================

/**
 * @brief Central manager for the entire asset pipeline system
 */
class AssetPipelineManager {
public:
    /**
     * @brief System status information
     */
    struct SystemStatus {
        bool is_initialized{false};
        bool is_running{false};
        u32 active_operations{0};
        u32 queued_operations{0};
        
        // Component status
        bool registry_healthy{true};
        bool loader_healthy{true};
        bool hot_reload_healthy{true};
        bool education_system_healthy{true};
        
        // Performance metrics
        f64 average_load_time{0.0};
        f32 cache_hit_rate{0.0f};
        usize memory_usage{0};
        f32 cpu_usage{0.0f};
        
        std::vector<std::string> warnings;
        std::vector<std::string> errors;
    };
    
private:
    // Core components
    std::unique_ptr<AssetRegistry> asset_registry_;
    std::unique_ptr<AssetLoader> asset_loader_;
    std::unique_ptr<AssetHotReloadManager> hot_reload_manager_;
    std::unique_ptr<education::AssetEducationSystem> education_system_;
    
    // Integration layers
    std::unique_ptr<SceneEditorIntegration> scene_editor_integration_;
    std::unique_ptr<MemorySystemIntegration> memory_integration_;
    
    // External system references
    memory::MemoryTracker* memory_tracker_{nullptr};
    learning::TutorialManager* tutorial_manager_{nullptr};
    
    // Configuration and state
    AssetPipelineConfig config_;
    std::atomic<bool> is_initialized_{false};
    std::atomic<bool> is_running_{false};
    
    // Event system
    std::vector<std::function<void(const events::AssetPipelineEvent&)>> event_listeners_;
    std::mutex event_listeners_mutex_;
    
    // Performance monitoring
    std::thread performance_monitor_thread_;
    std::atomic<bool> performance_monitoring_active_{false};
    SystemStatus cached_status_;
    mutable std::shared_mutex status_mutex_;
    
    // Statistics
    std::atomic<u64> total_operations_{0};
    std::atomic<u64> successful_operations_{0};
    std::atomic<f64> total_operation_time_{0.0};
    
public:
    explicit AssetPipelineManager(const AssetPipelineConfig& config = AssetPipelineConfig{});
    ~AssetPipelineManager();
    
    // Non-copyable
    AssetPipelineManager(const AssetPipelineManager&) = delete;
    AssetPipelineManager& operator=(const AssetPipelineManager&) = delete;
    
    // System lifecycle
    bool initialize(memory::MemoryTracker* memory_tracker = nullptr,
                   learning::TutorialManager* tutorial_manager = nullptr);
    void start();
    void stop();
    void shutdown();
    
    // Configuration
    void update_config(const AssetPipelineConfig& config);
    const AssetPipelineConfig& get_config() const { return config_; }
    
    // Asset operations (high-level interface)
    std::future<LoadingResult> load_asset_async(const std::filesystem::path& file_path,
                                               LoadPriority priority = LoadPriority::Normal);
    LoadingResult load_asset_sync(const std::filesystem::path& file_path);
    bool unload_asset(AssetID asset_id);
    bool reload_asset(AssetID asset_id);
    
    // Batch operations
    std::vector<std::future<LoadingResult>> load_assets_batch(const std::vector<std::filesystem::path>& files);
    
    // Asset queries
    AssetID find_asset(const std::filesystem::path& file_path) const;
    std::vector<AssetID> find_assets_by_type(AssetType type) const;
    std::vector<AssetID> search_assets(const std::string& query) const;
    bool is_asset_loaded(AssetID asset_id) const;
    std::optional<AssetMetadata> get_asset_metadata(AssetID asset_id) const;
    
    // Hot-reload control
    void enable_hot_reload(bool enabled);
    bool is_hot_reload_enabled() const;
    void watch_directory(const std::filesystem::path& directory, bool recursive = true);
    void unwatch_directory(const std::filesystem::path& directory);
    
    // Educational features
    std::string start_educational_session(AssetID asset_id, const std::string& student_id);
    void end_educational_session(const std::string& session_id);
    std::vector<std::string> get_available_tutorials() const;
    std::string get_asset_learning_content(AssetID asset_id) const;
    
    // System status and monitoring
    SystemStatus get_system_status() const;
    void start_performance_monitoring();
    void stop_performance_monitoring();
    
    // Component access
    AssetRegistry* get_asset_registry() const { return asset_registry_.get(); }
    AssetLoader* get_asset_loader() const { return asset_loader_.get(); }
    AssetHotReloadManager* get_hot_reload_manager() const { return hot_reload_manager_.get(); }
    education::AssetEducationSystem* get_education_system() const { return education_system_.get(); }
    SceneEditorIntegration* get_scene_editor_integration() const { return scene_editor_integration_.get(); }
    MemorySystemIntegration* get_memory_integration() const { return memory_integration_.get(); }
    
    // Event system
    void add_event_listener(std::function<void(const events::AssetPipelineEvent&)> listener);
    void remove_all_event_listeners();
    
    // Statistics
    struct PipelineStatistics {
        // Operations
        u64 total_operations{0};
        u64 successful_operations{0};
        f64 success_rate{1.0};
        f64 average_operation_time{0.0};
        
        // Component statistics
        AssetRegistry::RegistryStatistics registry_stats;
        AssetLoader::LoaderStatistics loader_stats;
        AssetHotReloadManager::HotReloadStatistics hot_reload_stats;
        education::AssetEducationSystem::EducationStatistics education_stats;
        
        // System health
        SystemStatus current_status;
    };
    
    PipelineStatistics get_comprehensive_statistics() const;
    void reset_statistics();
    
    // Diagnostics and debugging
    std::vector<std::string> diagnose_system_issues() const;
    std::string generate_system_report() const;
    bool validate_system_integrity() const;
    
    // Factory methods for common configurations
    static AssetPipelineConfig create_development_config();
    static AssetPipelineConfig create_production_config();
    static AssetPipelineConfig create_educational_config();
    static AssetPipelineConfig create_minimal_config();
    
private:
    // Internal initialization
    bool initialize_core_systems();
    bool initialize_integration_layers();
    bool initialize_importers();
    void setup_default_importers();
    
    // Event handling
    void emit_event(std::unique_ptr<events::AssetPipelineEvent> event);
    void setup_internal_event_handlers();
    
    // Performance monitoring
    void performance_monitoring_worker();
    void update_system_status();
    
    // Component health checking
    bool check_component_health() const;
    std::vector<std::string> collect_system_warnings() const;
    
    // Resource management
    void cleanup_resources();
    void emergency_shutdown();
};

//=============================================================================
// Global Asset Pipeline Access
//=============================================================================

/**
 * @brief Global access point for the asset pipeline (singleton pattern)
 */
class GlobalAssetPipeline {
private:
    static std::unique_ptr<AssetPipelineManager> instance_;
    static std::mutex instance_mutex_;
    
public:
    // Initialize global instance
    static bool initialize(const AssetPipelineConfig& config = AssetPipelineConfig{},
                          memory::MemoryTracker* memory_tracker = nullptr,
                          learning::TutorialManager* tutorial_manager = nullptr);
    
    // Get global instance
    static AssetPipelineManager* get_instance();
    
    // Cleanup global instance
    static void shutdown();
    
    // Convenience methods
    static std::future<LoadingResult> load_asset(const std::filesystem::path& file_path);
    static AssetID find_asset(const std::filesystem::path& file_path);
    static bool is_asset_loaded(AssetID asset_id);
    
private:
    GlobalAssetPipeline() = delete;
};

//=============================================================================
// Utility Functions
//=============================================================================

namespace utils {

// Asset type detection
AssetType detect_asset_type(const std::filesystem::path& file_path);
std::vector<std::string> get_supported_extensions();
bool is_asset_file(const std::filesystem::path& file_path);

// Path utilities
std::filesystem::path normalize_asset_path(const std::filesystem::path& path);
std::string generate_asset_id_from_path(const std::filesystem::path& path);
std::filesystem::path get_cache_path_for_asset(const std::filesystem::path& source_path);

// Configuration helpers
AssetPipelineConfig merge_configs(const AssetPipelineConfig& base, const AssetPipelineConfig& override);
bool validate_config(const AssetPipelineConfig& config, std::vector<std::string>& issues);
AssetPipelineConfig load_config_from_file(const std::filesystem::path& config_file);

// Performance utilities
std::string format_memory_size(usize bytes);
std::string format_duration(f64 milliseconds);
f32 calculate_efficiency_score(f64 actual_time, f64 estimated_time);

} // namespace utils

} // namespace ecscope::assets