#pragma once

/**
 * @file asset_loader.hpp
 * @brief Advanced Asynchronous Asset Loading System for ECScope Asset Pipeline
 * 
 * This system provides comprehensive asynchronous asset loading capabilities
 * with priority queues, progress tracking, loading screens, and educational
 * features for teaching concurrent programming and resource management.
 * 
 * Key Features:
 * - Multi-threaded asynchronous loading with priority queues
 * - Progress tracking and loading screen integration
 * - Memory-efficient streaming for large assets
 * - Educational visualization of loading operations
 * - Integration with asset registry and memory management
 * - Smart caching and preloading strategies
 * 
 * Educational Value:
 * - Demonstrates concurrent programming patterns
 * - Shows thread pool and work queue implementations
 * - Illustrates memory management during async operations
 * - Provides performance analysis tools
 * - Teaches optimization strategies for asset loading
 * 
 * @author ECScope Educational ECS Framework - Asset Pipeline
 * @date 2025
 */

#include "asset_pipeline.hpp"
#include "asset_hot_reload_manager.hpp"
#include "memory/memory_tracker.hpp"
#include "core/types.hpp"
#include "core/log.hpp"
#include <filesystem>
#include <unordered_map>
#include <vector>
#include <memory>
#include <atomic>
#include <shared_mutex>
#include <functional>
#include <future>
#include <chrono>
#include <queue>
#include <thread>
#include <condition_variable>
#include <optional>

namespace ecscope::assets {

//=============================================================================
// Forward Declarations
//=============================================================================

class AssetRegistry;
class AssetLoader;
class LoadingScreen;

//=============================================================================
// Loading Request System
//=============================================================================

/**
 * @brief Asset loading request with priority and configuration
 */
struct LoadingRequest {
    AssetID asset_id{INVALID_ASSET_ID};
    std::filesystem::path source_path;
    AssetType asset_type{AssetType::Unknown};
    LoadPriority priority{LoadPriority::Normal};
    
    // Loading configuration
    std::unique_ptr<ImportSettings> import_settings;
    bool force_reload{false};           // Force reload even if already loaded
    bool use_cache{true};              // Allow using cached versions
    bool track_dependencies{true};     // Track asset dependencies during load
    bool stream_data{false};           // Stream large assets in chunks
    
    // Memory management
    memory::MemoryTracker* memory_tracker{nullptr};
    usize memory_limit{0};             // 0 = no limit
    bool prefer_compressed{false};      // Prefer compressed versions to save memory
    
    // Callback configuration
    std::function<void(f32 progress)> progress_callback;
    std::function<void(const ImportResult&)> completion_callback;
    std::function<void(const std::string& error)> error_callback;
    
    // Educational settings
    bool generate_loading_report{false}; // Generate detailed loading report
    bool track_performance_metrics{true}; // Track performance for educational analysis
    std::string educational_context;     // Context for educational features
    
    // Request metadata
    std::string request_id;             // Unique identifier for this request
    std::chrono::high_resolution_clock::time_point created_time;
    std::string requester_name;         // Who requested this load (for debugging)
    
    LoadingRequest() = default;
    LoadingRequest(AssetID id, std::filesystem::path path, AssetType type, LoadPriority prio = LoadPriority::Normal)
        : asset_id(id), source_path(std::move(path)), asset_type(type), priority(prio)
        , created_time(std::chrono::high_resolution_clock::now()) {}
    
    f64 get_age_ms() const {
        auto now = std::chrono::high_resolution_clock::now();
        return std::chrono::duration<f64, std::milli>(now - created_time).count();
    }
    
    f32 get_priority_score() const {
        f32 base_score = static_cast<f32>(static_cast<u8>(LoadPriority::Background) - static_cast<u8>(priority));
        f32 age_score = static_cast<f32>(get_age_ms() / 1000.0); // Age in seconds
        return base_score * 10.0f + age_score * 0.1f; // Newer requests get slightly higher priority
    }
};

/**
 * @brief Loading operation result with comprehensive information
 */
struct LoadingResult {
    AssetID asset_id{INVALID_ASSET_ID};
    bool success{false};
    ImportResult import_result;
    
    // Timing information
    f64 total_time_ms{0.0};
    f64 import_time_ms{0.0};
    f64 memory_allocation_time_ms{0.0};
    f64 dependency_resolution_time_ms{0.0};
    f64 queue_wait_time_ms{0.0};
    
    // Resource usage
    usize memory_used{0};
    usize peak_memory_during_load{0};
    u32 dependencies_loaded{0};
    u32 cache_hits{0};
    u32 cache_misses{0};
    
    // Quality metrics
    f32 loading_efficiency{1.0f};       // How efficiently resources were used
    f32 cache_effectiveness{1.0f};      // How well caching worked
    std::vector<std::string> performance_warnings;
    std::vector<std::string> optimization_suggestions;
    
    // Educational information
    std::vector<std::string> loading_steps;
    std::string performance_analysis;
    std::string educational_summary;
    
    LoadingResult() = default;
    
    static LoadingResult success_result(AssetID id, ImportResult result) {
        LoadingResult loading_result;
        loading_result.asset_id = id;
        loading_result.success = true;
        loading_result.import_result = std::move(result);
        return loading_result;
    }
    
    static LoadingResult failure_result(AssetID id, const std::string& error) {
        LoadingResult loading_result;
        loading_result.asset_id = id;
        loading_result.success = false;
        loading_result.import_result = ImportResult::failure_result(error);
        return loading_result;
    }
};

//=============================================================================
// Loading Progress Tracking
//=============================================================================

/**
 * @brief Comprehensive progress tracking for loading operations
 */
class LoadingProgressTracker {
public:
    /**
     * @brief Progress information for a single loading operation
     */
    struct ProgressInfo {
        AssetID asset_id{INVALID_ASSET_ID};
        std::string asset_name;
        AssetType asset_type{AssetType::Unknown};
        
        // Progress metrics
        f32 overall_progress{0.0f};         // 0.0 to 1.0
        f32 import_progress{0.0f};          // Current import step progress
        f32 dependency_progress{0.0f};      // Dependency loading progress
        std::string current_step;           // Human-readable current step
        
        // Timing estimates
        f64 estimated_remaining_time_ms{0.0};
        f64 elapsed_time_ms{0.0};
        f64 estimated_total_time_ms{0.0};
        
        // Resource information
        usize current_memory_usage{0};
        usize estimated_final_memory{0};
        u32 dependencies_remaining{0};
        u32 total_dependencies{0};
        
        // Visual information for UI
        std::string status_text;
        std::string detail_text;
        f32 spinner_speed{1.0f};            // For animated loading indicators
        
        // Educational context
        std::vector<std::string> educational_notes;
        std::string learning_opportunity;
    };
    
private:
    std::unordered_map<AssetID, ProgressInfo> active_loads_;
    mutable std::shared_mutex progress_mutex_;
    
    // Global progress aggregation
    std::atomic<u32> total_active_loads_{0};
    std::atomic<f32> overall_system_progress_{1.0f}; // 1.0 when no loads active
    std::atomic<f64> estimated_remaining_time_{0.0};
    
    // History for performance prediction
    std::vector<f64> historical_load_times_;
    std::unordered_map<AssetType, std::vector<f64>> load_times_by_type_;
    mutable std::mutex history_mutex_;
    
public:
    LoadingProgressTracker() = default;
    ~LoadingProgressTracker() = default;
    
    // Progress tracking
    void start_tracking(AssetID asset_id, const std::string& name, AssetType type);
    void update_progress(AssetID asset_id, f32 progress, const std::string& step = "");
    void update_dependency_progress(AssetID asset_id, u32 loaded, u32 total);
    void finish_tracking(AssetID asset_id, f64 total_time_ms);
    void cancel_tracking(AssetID asset_id);
    
    // Progress queries
    std::optional<ProgressInfo> get_progress(AssetID asset_id) const;
    std::vector<ProgressInfo> get_all_active_progress() const;
    f32 get_overall_progress() const;
    f64 get_estimated_remaining_time() const;
    u32 get_active_load_count() const;
    
    // Educational features
    std::string generate_loading_explanation(AssetID asset_id) const;
    std::vector<std::string> get_performance_insights() const;
    
    // Performance analysis
    struct LoadingStats {
        f64 average_load_time{0.0};
        f64 fastest_load_time{0.0};
        f64 slowest_load_time{0.0};
        std::unordered_map<AssetType, f64> average_by_type;
        u32 total_loads_tracked{0};
    };
    
    LoadingStats get_statistics() const;
    void record_historical_time(AssetType type, f64 time_ms);
    f64 estimate_load_time(AssetType type, usize file_size = 0) const;
    
private:
    void update_system_progress();
    f64 calculate_weighted_remaining_time() const;
};

//=============================================================================
// Asset Cache Management
//=============================================================================

/**
 * @brief Intelligent asset cache for improving loading performance
 */
class AssetCache {
public:
    /**
     * @brief Cache entry with metadata
     */
    struct CacheEntry {
        AssetID asset_id{INVALID_ASSET_ID};
        AssetData cached_data;
        AssetMetadata metadata;
        
        // Cache management
        std::chrono::high_resolution_clock::time_point creation_time;
        std::chrono::high_resolution_clock::time_point last_access_time;
        std::atomic<u32> access_count{0};
        f32 priority_score{1.0f};       // Higher = more important to keep
        
        // Validation
        std::string content_hash;
        bool is_valid{true};
        f64 validation_time_ms{0.0};
        
        CacheEntry() = default;
        CacheEntry(AssetID id, AssetData data, AssetMetadata meta)
            : asset_id(id), cached_data(std::move(data)), metadata(std::move(meta))
            , creation_time(std::chrono::high_resolution_clock::now())
            , last_access_time(creation_time) {}
        
        void record_access() {
            last_access_time = std::chrono::high_resolution_clock::now();
            access_count.fetch_add(1);
        }
        
        f64 get_age_seconds() const {
            auto now = std::chrono::high_resolution_clock::now();
            return std::chrono::duration<f64>(now - creation_time).count();
        }
        
        f64 get_time_since_access_seconds() const {
            auto now = std::chrono::high_resolution_clock::now();
            return std::chrono::duration<f64>(now - last_access_time).count();
        }
        
        f32 calculate_priority_score() const {
            f32 access_score = std::log(1.0f + access_count.load()) * 0.2f;
            f32 recency_score = 1.0f / (1.0f + static_cast<f32>(get_time_since_access_seconds() / 3600.0)); // Hours
            f32 size_penalty = 1.0f / (1.0f + static_cast<f32>(cached_data.size_bytes()) / (1024 * 1024)); // MB
            return access_score + recency_score + size_penalty;
        }
    };
    
    /**
     * @brief Cache configuration
     */
    struct CacheConfig {
        usize max_memory_bytes{256 * 1024 * 1024}; // 256 MB default
        u32 max_entries{1000};                     // Maximum number of cached assets
        f64 cleanup_interval_seconds{60.0};        // How often to run cleanup
        f32 eviction_threshold{0.9f};              // Start evicting at 90% capacity
        f32 eviction_target{0.7f};                 // Evict down to 70% capacity
        
        // Validation settings
        bool validate_on_access{true};
        f64 max_entry_age_seconds{3600.0};         // 1 hour max age
        bool check_file_modification{true};
        
        // Educational settings
        bool track_cache_performance{true};
        bool generate_cache_reports{true};
    };
    
private:
    std::unordered_map<AssetID, std::unique_ptr<CacheEntry>> cache_entries_;
    mutable std::shared_mutex cache_mutex_;
    
    CacheConfig config_;
    std::atomic<usize> current_memory_usage_{0};
    std::atomic<u32> current_entry_count_{0};
    
    // Performance tracking
    std::atomic<u64> cache_hits_{0};
    std::atomic<u64> cache_misses_{0};
    std::atomic<u64> cache_invalidations_{0};
    std::atomic<u64> cache_evictions_{0};
    
    // Background maintenance
    std::thread cleanup_thread_;
    std::atomic<bool> shutdown_requested_{false};
    
public:
    explicit AssetCache(const CacheConfig& config = CacheConfig{});
    ~AssetCache();
    
    // Non-copyable
    AssetCache(const AssetCache&) = delete;
    AssetCache& operator=(const AssetCache&) = delete;
    
    // Cache operations
    bool store(AssetID asset_id, AssetData data, const AssetMetadata& metadata);
    std::optional<AssetData> retrieve(AssetID asset_id);
    bool remove(AssetID asset_id);
    void clear();
    
    // Cache queries
    bool contains(AssetID asset_id) const;
    bool is_valid(AssetID asset_id) const;
    usize get_memory_usage() const { return current_memory_usage_.load(); }
    u32 get_entry_count() const { return current_entry_count_.load(); }
    f32 get_memory_usage_percentage() const;
    
    // Cache management
    void invalidate(AssetID asset_id);
    void validate_all_entries();
    usize cleanup_expired_entries();
    usize evict_least_important_entries(usize target_memory);
    
    // Configuration
    void update_config(const CacheConfig& config);
    const CacheConfig& get_config() const { return config_; }
    
    // Statistics
    struct CacheStatistics {
        u64 cache_hits{0};
        u64 cache_misses{0};
        f64 hit_rate{1.0};
        u64 invalidations{0};
        u64 evictions{0};
        
        usize current_memory_usage{0};
        usize max_memory_limit{0};
        f32 memory_usage_percentage{0.0f};
        u32 current_entries{0};
        u32 max_entries{0};
        
        f64 average_entry_age{0.0};
        f64 average_access_frequency{0.0};
        std::unordered_map<AssetType, u32> entries_by_type;
    };
    
    CacheStatistics get_statistics() const;
    void reset_statistics();
    
    // Educational features
    std::string generate_cache_report() const;
    std::vector<std::string> get_optimization_suggestions() const;
    std::string explain_cache_behavior() const;
    
private:
    void cleanup_worker();
    bool validate_entry(CacheEntry& entry) const;
    void update_memory_usage();
    std::vector<AssetID> select_eviction_candidates(usize target_memory) const;
};

//=============================================================================
// Work Queue and Thread Pool
//=============================================================================

/**
 * @brief Thread pool for asynchronous asset loading operations
 */
class LoadingThreadPool {
public:
    /**
     * @brief Work item for the thread pool
     */
    struct WorkItem {
        std::function<LoadingResult()> work_function;
        LoadPriority priority{LoadPriority::Normal};
        std::string work_description;
        std::chrono::high_resolution_clock::time_point submit_time;
        std::promise<LoadingResult> result_promise;
        
        f32 get_priority_score() const {
            f32 base_score = static_cast<f32>(static_cast<u8>(LoadPriority::Background) - static_cast<u8>(priority));
            auto age = std::chrono::high_resolution_clock::now() - submit_time;
            f32 age_score = static_cast<f32>(std::chrono::duration<f64>(age).count()) * 0.1f;
            return base_score * 10.0f + age_score;
        }
        
        bool operator<(const WorkItem& other) const {
            return get_priority_score() < other.get_priority_score(); // Lower score = higher priority in priority_queue
        }
    };
    
private:
    std::priority_queue<WorkItem> work_queue_;
    std::mutex queue_mutex_;
    std::condition_variable work_condition_;
    std::condition_variable completion_condition_;
    
    std::vector<std::thread> worker_threads_;
    std::atomic<bool> shutdown_requested_{false};
    std::atomic<u32> active_workers_{0};
    std::atomic<u32> queued_items_{0};
    
    // Performance tracking
    std::atomic<u64> completed_tasks_{0};
    std::atomic<f64> total_work_time_{0.0};
    std::atomic<f64> total_queue_time_{0.0};
    
public:
    explicit LoadingThreadPool(u32 thread_count = std::thread::hardware_concurrency());
    ~LoadingThreadPool();
    
    // Non-copyable
    LoadingThreadPool(const LoadingThreadPool&) = delete;
    LoadingThreadPool& operator=(const LoadingThreadPool&) = delete;
    
    // Task submission
    std::future<LoadingResult> submit_work(std::function<LoadingResult()> work,
                                          LoadPriority priority = LoadPriority::Normal,
                                          const std::string& description = "");
    
    // Queue management
    u32 get_queue_size() const { return queued_items_.load(); }
    u32 get_active_worker_count() const { return active_workers_.load(); }
    u32 get_total_worker_count() const { return static_cast<u32>(worker_threads_.size()); }
    
    // Control
    void shutdown();
    void wait_for_completion();
    bool is_idle() const;
    
    // Statistics
    struct ThreadPoolStatistics {
        u32 total_threads{0};
        u32 active_threads{0};
        u32 queued_tasks{0};
        u64 completed_tasks{0};
        f64 average_work_time{0.0};
        f64 average_queue_time{0.0};
        f64 throughput_per_second{0.0};
        f64 thread_utilization{0.0}; // Percentage of threads actively working
    };
    
    ThreadPoolStatistics get_statistics() const;
    void reset_statistics();
    
private:
    void worker_thread_main();
    WorkItem get_next_work_item();
};

//=============================================================================
// Main Asset Loader
//=============================================================================

/**
 * @brief Comprehensive asynchronous asset loading system
 */
class AssetLoader {
public:
    /**
     * @brief Loader configuration
     */
    struct LoaderConfig {
        u32 worker_thread_count{4};                // Number of loading threads
        usize memory_budget_bytes{512 * 1024 * 1024}; // 512 MB loading budget
        u32 max_concurrent_loads{8};               // Max simultaneous loads
        
        // Cache configuration
        AssetCache::CacheConfig cache_config;
        
        // Progress tracking
        bool enable_progress_tracking{true};
        f64 progress_update_interval_ms{100.0};
        
        // Educational settings
        bool generate_loading_reports{true};
        bool track_educational_metrics{true};
        bool enable_loading_visualization{true};
        
        // Performance settings
        bool enable_preloading{true};
        bool enable_dependency_preloading{true};
        f64 preload_prediction_threshold{0.7f};     // Confidence threshold for preloading
        u32 max_preload_queue_size{20};
        
        // Error handling
        u32 max_retry_attempts{3};
        f64 retry_delay_ms{1000.0};
        bool fail_fast_on_critical_errors{true};
    };
    
private:
    // Core components
    AssetRegistry* asset_registry_;
    std::unique_ptr<LoadingThreadPool> thread_pool_;
    std::unique_ptr<AssetCache> cache_;
    std::unique_ptr<LoadingProgressTracker> progress_tracker_;
    
    // Importer registry
    std::unordered_map<AssetType, std::vector<std::unique_ptr<AssetImporter>>> importers_;
    
    // Configuration and state
    LoaderConfig config_;
    memory::MemoryTracker* memory_tracker_;
    std::atomic<bool> is_running_{false};
    
    // Active loading tracking
    std::unordered_map<AssetID, std::future<LoadingResult>> active_loads_;
    std::shared_mutex active_loads_mutex_;
    
    // Preloading system
    std::queue<LoadingRequest> preload_queue_;
    std::mutex preload_queue_mutex_;
    std::thread preload_thread_;
    std::atomic<bool> preload_enabled_{true};
    
    // Statistics
    std::atomic<u64> total_loads_requested_{0};
    std::atomic<u64> successful_loads_{0};
    std::atomic<u64> failed_loads_{0};
    std::atomic<f64> total_loading_time_{0.0};
    
    // Educational features
    std::vector<LoadingResult> recent_results_;
    mutable std::mutex recent_results_mutex_;
    constexpr static usize MAX_RECENT_RESULTS = 50;
    
public:
    explicit AssetLoader(AssetRegistry* registry, const LoaderConfig& config = LoaderConfig{},
                        memory::MemoryTracker* tracker = nullptr);
    ~AssetLoader();
    
    // Non-copyable
    AssetLoader(const AssetLoader&) = delete;
    AssetLoader& operator=(const AssetLoader&) = delete;
    
    // Initialization
    bool initialize();
    void shutdown();
    
    // Importer management
    void register_importer(std::unique_ptr<AssetImporter> importer);
    AssetImporter* get_importer_for_type(AssetType type);
    AssetImporter* get_importer_for_file(const std::filesystem::path& file_path);
    
    // Synchronous loading
    LoadingResult load_asset_sync(const LoadingRequest& request);
    LoadingResult load_asset_from_file_sync(const std::filesystem::path& file_path,
                                           AssetType type = AssetType::Unknown,
                                           const ImportSettings* settings = nullptr);
    
    // Asynchronous loading
    std::future<LoadingResult> load_asset_async(const LoadingRequest& request);
    std::future<LoadingResult> load_asset_from_file_async(const std::filesystem::path& file_path,
                                                         AssetType type = AssetType::Unknown,
                                                         const ImportSettings* settings = nullptr,
                                                         LoadPriority priority = LoadPriority::Normal);
    
    // Batch loading
    std::vector<std::future<LoadingResult>> load_assets_batch(const std::vector<LoadingRequest>& requests);
    std::future<std::vector<LoadingResult>> load_assets_batch_combined(const std::vector<LoadingRequest>& requests);
    
    // Preloading
    void enable_preloading(bool enabled) { preload_enabled_.store(enabled); }
    void queue_preload(const LoadingRequest& request);
    void queue_preload_directory(const std::filesystem::path& directory, bool recursive = true);
    void predict_and_preload_assets(); // ML-based preloading prediction
    
    // Loading status
    bool is_loading(AssetID asset_id) const;
    std::optional<LoadingProgressTracker::ProgressInfo> get_loading_progress(AssetID asset_id) const;
    std::vector<LoadingProgressTracker::ProgressInfo> get_all_active_progress() const;
    u32 get_active_load_count() const;
    
    // Cache management
    bool is_cached(AssetID asset_id) const;
    void invalidate_cache(AssetID asset_id);
    void clear_cache();
    AssetCache::CacheStatistics get_cache_statistics() const;
    
    // Configuration
    void update_config(const LoaderConfig& config);
    const LoaderConfig& get_config() const { return config_; }
    
    // Statistics and monitoring
    struct LoaderStatistics {
        u64 total_loads_requested{0};
        u64 successful_loads{0};
        u64 failed_loads{0};
        f64 success_rate{1.0};
        f64 average_loading_time{0.0};
        
        // Thread pool statistics
        LoadingThreadPool::ThreadPoolStatistics thread_pool_stats;
        
        // Cache statistics
        AssetCache::CacheStatistics cache_stats;
        
        // Current state
        u32 active_loads{0};
        u32 queued_preloads{0};
        usize memory_usage{0};
        f32 memory_usage_percentage{0.0f};
        
        // Performance metrics
        f64 cache_hit_rate{0.0};
        f64 average_queue_time{0.0};
        f64 loads_per_second{0.0};
        
        // Educational metrics
        u32 educational_reports_generated{0};
        f64 total_analysis_time{0.0};
    };
    
    LoaderStatistics get_statistics() const;
    void reset_statistics();
    
    // Educational features
    std::vector<LoadingResult> get_recent_results() const;
    std::string generate_loading_analysis_report() const;
    std::string generate_performance_optimization_guide() const;
    std::string generate_concurrency_tutorial() const;
    
    // Debugging and diagnostics
    std::string diagnose_loading_issues(AssetID asset_id) const;
    std::vector<std::string> get_performance_warnings() const;
    bool validate_loader_state() const;
    
    // Event callbacks
    using LoadCompletionCallback = std::function<void(const LoadingResult&)>;
    using LoadProgressCallback = std::function<void(AssetID, f32 progress, const std::string& step)>;
    using LoadErrorCallback = std::function<void(AssetID, const std::string& error)>;
    
    void set_completion_callback(LoadCompletionCallback callback) { completion_callback_ = std::move(callback); }
    void set_progress_callback(LoadProgressCallback callback) { progress_callback_ = std::move(callback); }
    void set_error_callback(LoadErrorCallback callback) { error_callback_ = std::move(callback); }
    
private:
    // Loading implementation
    LoadingResult execute_loading_request(const LoadingRequest& request);
    LoadingResult import_asset_with_importer(const LoadingRequest& request, AssetImporter* importer);
    
    // Preloading system
    void preload_worker_thread();
    void process_preload_queue();
    
    // Asset type detection
    AssetType detect_asset_type(const std::filesystem::path& file_path) const;
    
    // Educational analysis
    void analyze_loading_result(LoadingResult& result) const;
    void generate_educational_content(LoadingResult& result) const;
    std::vector<std::string> extract_performance_insights(const LoadingResult& result) const;
    
    // Error handling
    LoadingResult handle_loading_error(const LoadingRequest& request, const std::string& error);
    bool should_retry_load(const LoadingRequest& request, u32 attempt_count);
    
    // Memory management
    void track_memory_usage(const LoadingRequest& request, usize memory_used);
    void cleanup_memory_if_needed();
    
    // Utility functions
    std::string generate_request_id() const;
    void record_loading_result(const LoadingResult& result);
    void notify_callbacks(const LoadingResult& result);
    
    // Callback storage
    LoadCompletionCallback completion_callback_;
    LoadProgressCallback progress_callback_;
    LoadErrorCallback error_callback_;
};

//=============================================================================
// Loading Screen Integration
//=============================================================================

/**
 * @brief Loading screen system with educational visualization
 */
class LoadingScreen {
public:
    /**
     * @brief Loading screen configuration
     */
    struct ScreenConfig {
        bool show_progress_bars{true};
        bool show_asset_names{true};
        bool show_educational_tips{true};
        bool show_performance_metrics{false};
        bool animate_elements{true};
        
        // Visual settings
        f32 update_frequency_hz{30.0f};
        std::string background_asset_path;
        std::string loading_animation_path;
        
        // Educational settings
        bool explain_loading_process{true};
        bool show_optimization_tips{true};
        bool display_asset_statistics{true};
        f64 tip_rotation_interval_seconds{5.0};
    };
    
private:
    AssetLoader* asset_loader_;
    ScreenConfig config_;
    
    // Display state
    std::atomic<bool> is_visible_{false};
    std::vector<std::string> educational_tips_;
    usize current_tip_index_{0};
    std::chrono::steady_clock::time_point last_tip_change_;
    
    // Performance visualization
    std::vector<f32> loading_time_history_;
    std::vector<std::string> recent_asset_names_;
    
public:
    explicit LoadingScreen(AssetLoader* loader, const ScreenConfig& config = ScreenConfig{});
    
    // Control
    void show();
    void hide();
    bool is_visible() const { return is_visible_.load(); }
    
    // Update (call once per frame while visible)
    void update(f32 delta_time);
    
    // Display data for UI rendering
    struct DisplayData {
        f32 overall_progress{0.0f};
        std::vector<LoadingProgressTracker::ProgressInfo> active_loads;
        std::string current_tip;
        std::string performance_summary;
        std::vector<f32> performance_graph_data;
        bool show_details{false};
    };
    
    DisplayData get_display_data() const;
    
    // Configuration
    void update_config(const ScreenConfig& config) { config_ = config; }
    const ScreenConfig& get_config() const { return config_; }
    
private:
    void initialize_educational_tips();
    void update_educational_tip();
    void update_performance_visualization();
};

} // namespace ecscope::assets