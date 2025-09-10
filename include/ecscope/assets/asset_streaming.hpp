#pragma once

#include "asset.hpp"
#include "asset_loader.hpp"
#include <queue>
#include <thread>
#include <condition_variable>
#include <functional>

namespace ecscope::assets {

    // Streaming priorities
    namespace StreamingPriority {
        constexpr int CRITICAL = 1000;
        constexpr int VISIBLE = 800;
        constexpr int NEARBY = 600;
        constexpr int BACKGROUND = 400;
        constexpr int DISTANT = 200;
        constexpr int PRELOAD = 100;
    }

    // Streaming request
    struct StreamingRequest {
        AssetID asset_id;
        QualityLevel target_quality;
        QualityLevel current_quality;
        int priority;
        float distance; // Distance from camera/player
        std::chrono::steady_clock::time_point request_time;
        std::function<void(AssetHandle, bool)> completion_callback;
        
        StreamingRequest() : asset_id(INVALID_ASSET_ID), 
                           target_quality(QualityLevel::MEDIUM),
                           current_quality(QualityLevel::LOW),
                           priority(StreamingPriority::BACKGROUND),
                           distance(0.0f),
                           request_time(std::chrono::steady_clock::now()) {}
        
        bool operator<(const StreamingRequest& other) const {
            // Higher priority first, then by distance (closer first)
            if (priority != other.priority) {
                return priority < other.priority;
            }
            return distance > other.distance;
        }
    };

    // Streaming statistics
    struct StreamingStatistics {
        std::atomic<uint64_t> requests_processed{0};
        std::atomic<uint64_t> bytes_streamed{0};
        std::atomic<uint64_t> cache_hits{0};
        std::atomic<uint64_t> cache_misses{0};
        std::atomic<uint64_t> quality_upgrades{0};
        std::atomic<uint64_t> quality_downgrades{0};
        std::atomic<float> average_streaming_time_ms{0.0f};
        
        void reset() {
            requests_processed = 0;
            bytes_streamed = 0;
            cache_hits = 0;
            cache_misses = 0;
            quality_upgrades = 0;
            quality_downgrades = 0;
            average_streaming_time_ms = 0.0f;
        }
    };

    // Level of Detail (LOD) configuration
    struct LODConfiguration {
        struct LODLevel {
            QualityLevel quality;
            float max_distance;
            float screen_size_threshold; // Percentage of screen
            std::string quality_suffix;  // e.g., "_low", "_med", "_high"
            
            LODLevel(QualityLevel q, float dist, float screen = 1.0f, const std::string& suffix = "")
                : quality(q), max_distance(dist), screen_size_threshold(screen), quality_suffix(suffix) {}
        };
        
        std::vector<LODLevel> levels;
        float hysteresis_factor = 0.1f; // Prevent LOD thrashing
        bool enable_temporal_upsampling = true;
        bool enable_quality_prediction = true;
        
        LODConfiguration() {
            // Default LOD levels
            levels.emplace_back(QualityLevel::LOW, 100.0f, 0.1f, "_low");
            levels.emplace_back(QualityLevel::MEDIUM, 50.0f, 0.25f, "_med");
            levels.emplace_back(QualityLevel::HIGH, 25.0f, 0.5f, "_high");
            levels.emplace_back(QualityLevel::ULTRA, 10.0f, 1.0f, "_ultra");
        }
        
        QualityLevel select_quality_for_distance(float distance) const;
        QualityLevel select_quality_for_screen_size(float screen_size) const;
    };

    // Streaming budget manager
    class StreamingBudgetManager {
    public:
        StreamingBudgetManager();
        
        // Budget configuration
        void set_memory_budget_mb(size_t budget_mb);
        void set_bandwidth_budget_mbps(float budget_mbps);
        void set_time_budget_ms(float budget_ms_per_frame);
        
        size_t get_memory_budget() const { return m_memory_budget_bytes; }
        float get_bandwidth_budget() const { return m_bandwidth_budget_bps; }
        float get_time_budget() const { return m_time_budget_ms; }
        
        // Budget tracking
        bool can_afford_request(const StreamingRequest& request, size_t estimated_bytes) const;
        void consume_budget(size_t bytes, float time_ms);
        void reset_frame_budget();
        
        // Budget status
        size_t get_memory_used() const { return m_memory_used.load(); }
        float get_bandwidth_used() const { return m_bandwidth_used.load(); }
        float get_time_used() const { return m_time_used_this_frame.load(); }
        
        float get_memory_utilization() const;
        float get_bandwidth_utilization() const;
        float get_time_utilization() const;
        
    private:
        std::atomic<size_t> m_memory_budget_bytes{256 * 1024 * 1024}; // 256MB
        std::atomic<float> m_bandwidth_budget_bps{10.0f * 1024 * 1024}; // 10 MB/s
        std::atomic<float> m_time_budget_ms{2.0f}; // 2ms per frame
        
        std::atomic<size_t> m_memory_used{0};
        std::atomic<float> m_bandwidth_used{0.0f};
        std::atomic<float> m_time_used_this_frame{0.0f};
        
        std::chrono::steady_clock::time_point m_last_frame_time;
    };

    // Asset streaming system
    class AssetStreamingSystem {
    public:
        explicit AssetStreamingSystem(AssetManager* asset_manager);
        ~AssetStreamingSystem();
        
        // System control
        bool initialize();
        void shutdown();
        void update(float delta_time);
        
        // Streaming requests
        void request_asset(AssetID asset_id, 
                          QualityLevel target_quality = QualityLevel::MEDIUM,
                          int priority = StreamingPriority::BACKGROUND,
                          float distance = 0.0f,
                          std::function<void(AssetHandle, bool)> callback = nullptr);
        
        void cancel_request(AssetID asset_id);
        void change_priority(AssetID asset_id, int new_priority);
        void update_distance(AssetID asset_id, float distance);
        
        // Batch operations
        void request_assets_batch(const std::vector<std::pair<AssetID, QualityLevel>>& requests,
                                 int priority = StreamingPriority::BACKGROUND);
        void preload_area(const std::vector<AssetID>& asset_ids, float radius);
        
        // Quality management
        void set_global_quality_level(QualityLevel quality);
        QualityLevel get_global_quality_level() const { return m_global_quality; }
        
        void set_lod_configuration(const LODConfiguration& config);
        const LODConfiguration& get_lod_configuration() const { return m_lod_config; }
        
        // Budget management
        StreamingBudgetManager& get_budget_manager() { return m_budget_manager; }
        const StreamingBudgetManager& get_budget_manager() const { return m_budget_manager; }
        
        // Streaming state
        bool is_streaming_active(AssetID asset_id) const;
        float get_streaming_progress(AssetID asset_id) const;
        QualityLevel get_current_quality(AssetID asset_id) const;
        
        // Performance tuning
        void set_worker_thread_count(size_t count);
        size_t get_worker_thread_count() const { return m_worker_threads.size(); }
        
        void set_max_concurrent_requests(size_t max_requests);
        size_t get_max_concurrent_requests() const { return m_max_concurrent_requests; }
        
        // Statistics
        const StreamingStatistics& get_statistics() const { return m_statistics; }
        void reset_statistics() { m_statistics.reset(); }
        
        // Debugging
        void dump_streaming_state() const;
        std::vector<AssetID> get_active_requests() const;
        
    private:
        AssetManager* m_asset_manager;
        
        // Configuration
        QualityLevel m_global_quality = QualityLevel::MEDIUM;
        LODConfiguration m_lod_config;
        StreamingBudgetManager m_budget_manager;
        
        // Request management
        std::priority_queue<StreamingRequest> m_request_queue;
        std::unordered_map<AssetID, StreamingRequest> m_active_requests;
        std::unordered_map<AssetID, QualityLevel> m_current_qualities;
        
        // Threading
        std::vector<std::thread> m_worker_threads;
        std::atomic<bool> m_shutdown_requested{false};
        std::mutex m_queue_mutex;
        std::condition_variable m_queue_cv;
        
        // Performance settings
        std::atomic<size_t> m_max_concurrent_requests{4};
        std::atomic<size_t> m_active_request_count{0};
        
        // Statistics
        mutable StreamingStatistics m_statistics;
        
        // Thread safety
        mutable std::shared_mutex m_state_mutex;
        
        // Worker thread functions
        void worker_thread_main();
        void process_streaming_request(const StreamingRequest& request);
        
        // Quality selection
        QualityLevel select_optimal_quality(AssetID asset_id, float distance) const;
        bool should_upgrade_quality(AssetID asset_id, QualityLevel current, QualityLevel target) const;
        bool should_downgrade_quality(AssetID asset_id, QualityLevel current) const;
        
        // Request management
        void add_request_internal(const StreamingRequest& request);
        void remove_request_internal(AssetID asset_id);
        void update_request_priority(AssetID asset_id, int new_priority);
        
        // Memory management
        void enforce_memory_budget();
        void evict_low_priority_assets();
        
        // Performance monitoring
        void update_statistics(const StreamingRequest& request, bool success, 
                              std::chrono::milliseconds processing_time);
    };

    // Predictive streaming system
    class PredictiveStreamingSystem {
    public:
        explicit PredictiveStreamingSystem(AssetStreamingSystem* streaming_system);
        ~PredictiveStreamingSystem();
        
        // Prediction configuration
        struct PredictionConfig {
            float prediction_time_horizon = 2.0f; // seconds
            float confidence_threshold = 0.7f;
            int max_predictions_per_frame = 10;
            bool enable_movement_prediction = true;
            bool enable_interaction_prediction = true;
            bool enable_pattern_learning = true;
        };
        
        void set_prediction_config(const PredictionConfig& config);
        const PredictionConfig& get_prediction_config() const { return m_config; }
        
        // Position and movement tracking
        void update_camera_position(float x, float y, float z);
        void update_camera_velocity(float vx, float vy, float vz);
        void update_camera_direction(float dx, float dy, float dz);
        
        void update_player_position(float x, float y, float z);
        void update_player_velocity(float vx, float vy, float vz);
        
        // User interaction tracking
        void on_asset_accessed(AssetID asset_id);
        void on_area_entered(const std::string& area_name, const std::vector<AssetID>& assets);
        void on_level_loaded(const std::string& level_name);
        
        // Prediction updates
        void update_predictions(float delta_time);
        std::vector<AssetID> get_predicted_assets() const;
        
        // Pattern learning
        void enable_learning(bool enable) { m_learning_enabled = enable; }
        bool is_learning_enabled() const { return m_learning_enabled; }
        
        void save_learned_patterns(const std::string& file_path) const;
        bool load_learned_patterns(const std::string& file_path);
        
    private:
        AssetStreamingSystem* m_streaming_system;
        PredictionConfig m_config;
        
        // Position tracking
        struct PositionState {
            std::array<float, 3> position = {0, 0, 0};
            std::array<float, 3> velocity = {0, 0, 0};
            std::array<float, 3> direction = {0, 0, 1}; // Forward direction
            std::chrono::steady_clock::time_point last_update;
        } m_camera_state, m_player_state;
        
        // Access patterns
        struct AccessPattern {
            AssetID asset_id;
            std::chrono::steady_clock::time_point last_access;
            std::vector<std::chrono::steady_clock::time_point> access_history;
            float access_frequency = 0.0f;
            std::array<float, 3> typical_access_position = {0, 0, 0};
        };
        
        std::unordered_map<AssetID, AccessPattern> m_access_patterns;
        
        // Area patterns
        struct AreaPattern {
            std::string area_name;
            std::vector<AssetID> associated_assets;
            float visit_frequency = 0.0f;
            std::chrono::steady_clock::time_point last_visit;
        };
        
        std::unordered_map<std::string, AreaPattern> m_area_patterns;
        
        // Prediction state
        std::vector<AssetID> m_predicted_assets;
        std::chrono::steady_clock::time_point m_last_prediction_update;
        
        // Learning
        bool m_learning_enabled = true;
        
        // Thread safety
        mutable std::shared_mutex m_patterns_mutex;
        
        // Prediction algorithms
        std::vector<AssetID> predict_movement_based_assets() const;
        std::vector<AssetID> predict_pattern_based_assets() const;
        std::vector<AssetID> predict_interaction_based_assets() const;
        
        // Pattern learning
        void update_access_pattern(AssetID asset_id);
        void update_area_pattern(const std::string& area_name);
        void decay_patterns(float delta_time);
    };

    // Global streaming system accessors
    AssetStreamingSystem& get_streaming_system();
    void set_streaming_system(std::unique_ptr<AssetStreamingSystem> system);

    PredictiveStreamingSystem& get_predictive_streaming();
    void set_predictive_streaming(std::unique_ptr<PredictiveStreamingSystem> system);

} // namespace ecscope::assets