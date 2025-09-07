#pragma once

#include "ml_prediction_system.hpp"
#include "entity.hpp"
#include "component.hpp"
#include "registry.hpp"
#include <vector>
#include <memory>
#include <string>
#include <chrono>
#include <unordered_map>
#include <queue>
#include <thread>
#include <atomic>
#include <mutex>
#include <functional>
#include <fstream>
#include <filesystem>
#include <condition_variable>

namespace ecscope::ml {

/**
 * @brief Types of training data that can be collected
 */
enum class DataCollectionType {
    EntityBehavior,      // Entity component changes and lifecycle
    ComponentUsage,      // Component allocation and usage patterns
    SystemPerformance,   // System execution times and bottlenecks
    MemoryAllocation,    // Memory allocation and deallocation patterns
    PerformanceMetrics,  // Frame rates, CPU usage, memory pressure
    UserInteraction,     // User input patterns and responses
    GameEvents,          // Game-specific events and state changes
    All                  // Collect all types of data
};

/**
 * @brief Raw training data point
 */
struct TrainingDataPoint {
    Timestamp timestamp;
    DataCollectionType data_type;
    std::string source_system;      // Which system generated this data
    std::string category;           // Sub-category of data
    
    // Raw data (flexible storage)
    std::unordered_map<std::string, f32> numeric_features;
    std::unordered_map<std::string, std::string> string_features;
    std::unordered_map<std::string, bool> boolean_features;
    
    // Context information
    EntityID associated_entity{ecs::null_entity()};
    std::string associated_component_type;
    usize frame_number{0};
    f32 frame_time{0.0f};
    
    // Metadata
    f32 data_quality_score{1.0f};   // Quality assessment (0-1)
    bool is_outlier{false};         // Whether this is an outlier
    f32 importance_weight{1.0f};    // Importance weight for training
    std::string collection_reason;  // Why this data was collected
    
    // Convert to feature vector for ML
    FeatureVector to_feature_vector(const std::vector<std::string>& feature_names) const;
    
    // Validation and quality checks
    bool is_valid() const;
    f32 calculate_completeness() const; // Percentage of expected features present
    
    std::string to_string() const;
    std::string to_csv_row(const std::vector<std::string>& headers) const;
};

/**
 * @brief Configuration for data collection
 */
struct DataCollectionConfig {
    // What to collect
    std::vector<DataCollectionType> enabled_types{DataCollectionType::All};
    
    // Collection frequency and limits
    std::chrono::milliseconds sampling_interval{16}; // 60 FPS
    usize max_samples_per_type{50000};    // Maximum samples per data type
    usize max_memory_usage{100 * 1024 * 1024}; // 100 MB limit
    
    // Quality control
    bool enable_outlier_detection{true};
    f32 outlier_threshold{3.0f};          // Standard deviations for outlier detection
    bool enable_data_validation{true};
    f32 min_data_quality_score{0.7f};     // Minimum quality to keep data
    
    // Storage settings
    std::string data_directory{"training_data"};
    std::string file_prefix{"ecs_training_"};
    bool enable_real_time_storage{true};
    bool compress_stored_data{true};
    usize storage_flush_interval{1000};   // Flush to disk every N samples
    
    // Performance settings
    bool enable_async_collection{true};
    usize collection_thread_count{2};
    bool enable_adaptive_sampling{true};  // Reduce sampling under high load
    f32 cpu_usage_threshold{0.8f};        // Reduce sampling if CPU > 80%
    
    // Educational features
    bool enable_collection_visualization{true};
    bool track_collection_efficiency{true};
    bool generate_collection_reports{true};
    
    // Feature engineering
    bool enable_automatic_feature_extraction{true};
    bool normalize_numeric_features{true};
    bool encode_categorical_features{true};
    usize max_categorical_unique_values{100};
};

/**
 * @brief Statistics for data collection
 */
struct DataCollectionStats {
    // Collection counts
    std::unordered_map<DataCollectionType, usize> samples_collected;
    std::unordered_map<DataCollectionType, usize> samples_discarded;
    std::unordered_map<DataCollectionType, f32> average_quality_scores;
    
    // Performance metrics
    f32 collection_overhead{0.0f};        // Performance overhead percentage
    f32 average_collection_time{0.0f};    // Average time per sample (microseconds)
    usize storage_writes{0};
    f32 storage_throughput{0.0f};         // MB/s storage throughput
    
    // Data quality metrics
    f32 overall_data_quality{1.0f};
    usize outliers_detected{0};
    usize validation_failures{0};
    f32 completeness_ratio{1.0f};         // Ratio of complete vs partial samples
    
    // Storage statistics
    usize total_storage_used{0};          // Bytes used for storage
    usize files_created{0};
    f32 compression_ratio{1.0f};          // Original size / compressed size
    
    void reset();
    void update_collection_stats(DataCollectionType type, const TrainingDataPoint& data_point);
    void update_performance_stats(f32 collection_time, f32 cpu_overhead);
    std::string to_string() const;
};

/**
 * @brief Data collection buffer for efficient batch processing
 */
template<typename DataType>
class DataCollectionBuffer {
private:
    std::vector<DataType> buffer_;
    usize max_size_;
    std::atomic<usize> write_index_{0};
    std::atomic<usize> read_index_{0};
    mutable std::mutex buffer_mutex_;
    std::condition_variable buffer_cv_;
    
public:
    explicit DataCollectionBuffer(usize max_size = 10000)
        : max_size_(max_size) {
        buffer_.resize(max_size);
    }
    
    bool push(const DataType& data) {
        std::lock_guard<std::mutex> lock(buffer_mutex_);
        usize next_write = (write_index_.load() + 1) % max_size_;
        
        if (next_write == read_index_.load()) {
            return false; // Buffer full
        }
        
        buffer_[write_index_.load()] = data;
        write_index_.store(next_write);
        buffer_cv_.notify_one();
        return true;
    }
    
    bool pop(DataType& data) {
        std::unique_lock<std::mutex> lock(buffer_mutex_);
        
        if (read_index_.load() == write_index_.load()) {
            return false; // Buffer empty
        }
        
        data = buffer_[read_index_.load()];
        read_index_.store((read_index_.load() + 1) % max_size_);
        return true;
    }
    
    std::vector<DataType> pop_batch(usize max_count = 100) {
        std::lock_guard<std::mutex> lock(buffer_mutex_);
        std::vector<DataType> batch;
        
        usize available = (write_index_.load() + max_size_ - read_index_.load()) % max_size_;
        usize to_pop = std::min(max_count, available);
        
        batch.reserve(to_pop);
        for (usize i = 0; i < to_pop; ++i) {
            batch.push_back(buffer_[read_index_.load()]);
            read_index_.store((read_index_.load() + 1) % max_size_);
        }
        
        return batch;
    }
    
    bool wait_for_data(std::chrono::milliseconds timeout = std::chrono::milliseconds{100}) {
        std::unique_lock<std::mutex> lock(buffer_mutex_);
        return buffer_cv_.wait_for(lock, timeout, [this]() {
            return read_index_.load() != write_index_.load();
        });
    }
    
    usize size() const {
        return (write_index_.load() + max_size_ - read_index_.load()) % max_size_;
    }
    
    bool empty() const { return read_index_.load() == write_index_.load(); }
    bool full() const { return ((write_index_.load() + 1) % max_size_) == read_index_.load(); }
    
    void clear() {
        std::lock_guard<std::mutex> lock(buffer_mutex_);
        read_index_.store(0);
        write_index_.store(0);
    }
};

/**
 * @brief Main training data collector for ECS ML systems
 * 
 * This system continuously collects training data from ECS operations in real-time,
 * processes it for quality, and stores it for machine learning model training.
 * It's designed to be efficient and educational.
 */
class MLTrainingDataCollector {
private:
    DataCollectionConfig config_;
    DataCollectionStats collection_stats_;
    
    // Data buffers for different types
    std::unordered_map<DataCollectionType, 
                      std::unique_ptr<DataCollectionBuffer<TrainingDataPoint>>> data_buffers_;
    
    // Feature extraction and processing
    std::unordered_map<DataCollectionType, std::vector<std::string>> feature_schemas_;
    std::unordered_map<std::string, f32> feature_statistics_; // For normalization
    std::unique_ptr<FeatureExtractor> feature_extractor_;
    
    // Background processing
    std::vector<std::unique_ptr<std::thread>> collection_threads_;
    std::unique_ptr<std::thread> storage_thread_;
    std::atomic<bool> should_stop_threads_{false};
    
    // Data storage
    std::unordered_map<DataCollectionType, std::ofstream> data_files_;
    std::queue<std::pair<DataCollectionType, std::string>> pending_writes_;
    std::mutex storage_mutex_;
    std::condition_variable storage_cv_;
    
    // Performance monitoring
    std::chrono::high_resolution_clock::time_point last_collection_time_;
    std::atomic<f32> current_cpu_overhead_{0.0f};
    std::atomic<usize> total_samples_collected_{0};
    
    // Quality control
    std::unordered_map<DataCollectionType, std::vector<TrainingDataPoint>> outlier_examples_;
    std::unordered_map<std::string, f32> feature_ranges_; // For outlier detection
    
public:
    explicit MLTrainingDataCollector(const DataCollectionConfig& config = DataCollectionConfig{});
    ~MLTrainingDataCollector();
    
    // Non-copyable but movable
    MLTrainingDataCollector(const MLTrainingDataCollector&) = delete;
    MLTrainingDataCollector& operator=(const MLTrainingDataCollector&) = delete;
    MLTrainingDataCollector(MLTrainingDataCollector&&) = default;
    MLTrainingDataCollector& operator=(MLTrainingDataCollector&&) = default;
    
    // Collection control
    void start_collection();
    void stop_collection();
    void pause_collection();
    void resume_collection();
    bool is_collecting() const { return !should_stop_threads_.load(); }
    
    // Manual data collection
    void collect_entity_behavior_data(EntityID entity, const ecs::Registry& registry);
    void collect_component_usage_data(EntityID entity, const std::string& component_type,
                                     const ecs::Registry& registry);
    void collect_system_performance_data(const std::string& system_name, f32 execution_time,
                                        const ecs::Registry& registry);
    void collect_memory_allocation_data(void* address, usize size, const std::string& allocator_type);
    void collect_performance_metrics_data(f32 frame_time, f32 cpu_usage, f32 memory_usage);
    void collect_custom_data(const TrainingDataPoint& data_point);
    
    // Batch data collection
    void collect_all_entity_data(const ecs::Registry& registry);
    void collect_frame_performance_data(const ecs::Registry& registry);
    void collect_memory_state_data(const ecs::Registry& registry);
    
    // Data retrieval and export
    std::vector<TrainingDataPoint> get_collected_data(DataCollectionType type, usize max_samples = 1000);
    TrainingDataset create_training_dataset(DataCollectionType type, const std::string& dataset_name = "");
    void export_data_to_csv(DataCollectionType type, const std::string& filename);
    void export_all_data(const std::string& directory = "");
    
    // Data processing and quality control
    void process_collected_data();
    void validate_data_quality();
    std::vector<TrainingDataPoint> detect_outliers(DataCollectionType type);
    void remove_low_quality_data(f32 min_quality_threshold = 0.5f);
    
    // Feature management
    void register_feature_schema(DataCollectionType type, const std::vector<std::string>& features);
    std::vector<std::string> get_feature_schema(DataCollectionType type) const;
    void update_feature_statistics(const TrainingDataPoint& data_point);
    TrainingDataPoint normalize_data_point(const TrainingDataPoint& data_point) const;
    
    // Statistics and monitoring
    const DataCollectionStats& get_collection_statistics() const { return collection_stats_; }
    f32 get_current_collection_rate() const; // Samples per second
    f32 get_storage_usage() const;           // Bytes used for storage
    usize get_total_samples_collected() const { return total_samples_collected_.load(); }
    
    // Configuration management
    const DataCollectionConfig& config() const { return config_; }
    void update_config(const DataCollectionConfig& new_config);
    
    // Educational features
    std::string generate_collection_report() const;
    void print_collection_summary() const;
    std::string visualize_data_distribution(DataCollectionType type) const;
    std::string get_data_quality_analysis() const;
    
    // Advanced features
    std::vector<std::string> suggest_additional_features(DataCollectionType type) const;
    f32 estimate_model_training_readiness(DataCollectionType type) const;
    std::unordered_map<DataCollectionType, usize> recommend_sample_sizes() const;
    
    // Integration with other ML systems
    void setup_automatic_collection_for_behavior_predictor();
    void setup_automatic_collection_for_performance_predictor();
    void setup_automatic_collection_for_memory_predictor();
    
    // Callbacks for real-time integration
    using DataCollectionCallback = std::function<void(const TrainingDataPoint&)>;
    using QualityIssueCallback = std::function<void(const std::string&)>;
    
    void set_data_collection_callback(DataCollectionCallback callback) { data_callback_ = callback; }
    void set_quality_issue_callback(QualityIssueCallback callback) { quality_callback_ = callback; }
    
private:
    // Internal implementation
    void initialize_data_buffers();
    void initialize_storage_files();
    void cleanup_storage_files();
    void start_background_threads();
    void stop_background_threads();
    
    // Data collection implementation
    TrainingDataPoint create_entity_behavior_data_point(EntityID entity, const ecs::Registry& registry);
    TrainingDataPoint create_component_usage_data_point(EntityID entity, const std::string& component_type,
                                                        const ecs::Registry& registry);
    TrainingDataPoint create_system_performance_data_point(const std::string& system_name,
                                                           f32 execution_time, const ecs::Registry& registry);
    TrainingDataPoint create_memory_allocation_data_point(void* address, usize size,
                                                          const std::string& allocator_type);
    TrainingDataPoint create_performance_metrics_data_point(f32 frame_time, f32 cpu_usage, f32 memory_usage);
    
    // Background thread functions
    void collection_thread_function(DataCollectionType type);
    void storage_thread_function();
    
    // Data processing
    bool validate_data_point(const TrainingDataPoint& data_point) const;
    f32 calculate_data_point_quality(const TrainingDataPoint& data_point) const;
    bool is_outlier(const TrainingDataPoint& data_point) const;
    void update_outlier_detection_parameters(const TrainingDataPoint& data_point);
    
    // Storage implementation
    void write_data_point_to_file(DataCollectionType type, const TrainingDataPoint& data_point);
    void flush_pending_writes();
    std::string generate_filename(DataCollectionType type) const;
    std::string data_type_to_string(DataCollectionType type) const;
    
    // Performance monitoring
    void update_performance_metrics();
    f32 calculate_cpu_overhead() const;
    void adapt_sampling_rate_based_on_performance();
    
    // Feature schema management
    void initialize_default_feature_schemas();
    std::vector<std::string> extract_features_from_data_point(const TrainingDataPoint& data_point) const;
    
    // Educational content generation
    std::string explain_data_collection_purpose(DataCollectionType type) const;
    std::string generate_feature_importance_analysis(DataCollectionType type) const;
    
    // Callbacks
    DataCollectionCallback data_callback_;
    QualityIssueCallback quality_callback_;
};

/**
 * @brief Utility functions for training data collection
 */
namespace training_data_utils {

// Data conversion utilities
TrainingDataset convert_data_points_to_dataset(const std::vector<TrainingDataPoint>& data_points,
                                               const std::vector<std::string>& feature_names,
                                               const std::string& dataset_name = "");

// Quality assessment
f32 assess_dataset_quality(const std::vector<TrainingDataPoint>& data_points);
std::vector<std::string> identify_quality_issues(const std::vector<TrainingDataPoint>& data_points);
std::vector<TrainingDataPoint> remove_outliers(const std::vector<TrainingDataPoint>& data_points,
                                               f32 threshold = 3.0f);

// Feature analysis
std::unordered_map<std::string, f32> calculate_feature_importance(const std::vector<TrainingDataPoint>& data_points,
                                                                 const std::vector<std::string>& feature_names);
std::vector<std::string> identify_correlated_features(const std::vector<TrainingDataPoint>& data_points,
                                                     f32 correlation_threshold = 0.9f);

// Educational utilities
std::string visualize_data_point_timeline(const std::vector<TrainingDataPoint>& data_points);
std::string create_feature_distribution_chart(const std::vector<TrainingDataPoint>& data_points,
                                              const std::string& feature_name);
std::string explain_data_collection_best_practices();

// Export utilities
void export_training_dataset_to_csv(const TrainingDataset& dataset, const std::string& filename);
void export_training_dataset_to_json(const TrainingDataset& dataset, const std::string& filename);
bool load_training_dataset_from_csv(const std::string& filename, TrainingDataset& dataset);

} // namespace training_data_utils

} // namespace ecscope::ml