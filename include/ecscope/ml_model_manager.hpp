#pragma once

#include "ml_prediction_system.hpp"
#include "ml_training_data_collector.hpp"
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
#include <filesystem>
#include <future>
#include <condition_variable>

namespace ecscope::ml {

/**
 * @brief Model training status
 */
enum class ModelTrainingStatus {
    NotStarted,    // Training hasn't begun
    InProgress,    // Currently training
    Completed,     // Training finished successfully
    Failed,        // Training failed
    Cancelled,     // Training was cancelled
    Paused         // Training is paused
};

/**
 * @brief Model validation metrics
 */
struct ModelValidationResult {
    std::string model_name;
    f32 accuracy{0.0f};
    f32 precision{0.0f};
    f32 recall{0.0f};
    f32 f1_score{0.0f};
    f32 mean_absolute_error{0.0f};
    f32 mean_squared_error{0.0f};
    f32 r_squared{0.0f};
    f32 validation_loss{0.0f};
    
    // Cross-validation results
    std::vector<f32> cv_scores;
    f32 cv_mean{0.0f};
    f32 cv_std{0.0f};
    
    // Training metrics
    std::vector<f32> training_losses;
    std::vector<f32> validation_losses;
    usize training_epochs{0};
    std::chrono::milliseconds training_time{0};
    
    // Model complexity
    usize parameter_count{0};
    usize model_size_bytes{0};
    f32 inference_time_ms{0.0f};
    
    bool is_acceptable_quality(f32 min_accuracy = 0.7f) const { return accuracy >= min_accuracy; }
    bool shows_overfitting(f32 threshold = 0.1f) const;
    
    std::string to_string() const;
    void print_detailed_results() const;
};

/**
 * @brief Training progress information
 */
struct TrainingProgress {
    std::string model_name;
    ModelTrainingStatus status{ModelTrainingStatus::NotStarted};
    usize current_epoch{0};
    usize total_epochs{0};
    f32 current_loss{0.0f};
    f32 best_loss{std::numeric_limits<f32>::max()};
    f32 progress_percentage{0.0f};
    
    std::chrono::high_resolution_clock::time_point start_time;
    std::chrono::high_resolution_clock::time_point estimated_completion_time;
    std::chrono::milliseconds elapsed_time{0};
    std::chrono::milliseconds estimated_remaining_time{0};
    
    // Training details
    usize training_samples{0};
    usize validation_samples{0};
    f32 learning_rate{0.0f};
    std::string optimizer_type;
    
    // Early stopping information
    bool early_stopping_enabled{false};
    usize patience_counter{0};
    usize max_patience{50};
    f32 early_stopping_threshold{0.001f};
    
    // Recent performance
    std::vector<f32> recent_losses;
    bool is_converging() const;
    bool should_stop_early() const;
    
    std::string status_to_string() const;
    std::string to_string() const;
};

/**
 * @brief Model management configuration
 */
struct ModelManagerConfig {
    // Training settings
    bool enable_automatic_training{true};
    std::chrono::hours retraining_interval{24};  // Retrain models every 24 hours
    usize min_samples_for_training{100};         // Minimum samples before training
    f32 validation_split{0.2f};                  // 20% of data for validation
    
    // Model storage
    std::string model_directory{"models"};
    std::string model_file_extension{".ecml"};   // ECScope ML format
    bool enable_model_versioning{true};
    usize max_model_versions{10};                // Keep last 10 versions
    bool auto_save_best_models{true};
    
    // Training optimization
    bool enable_parallel_training{true};
    usize max_concurrent_trainings{4};
    bool enable_early_stopping{true};
    bool enable_learning_rate_scheduling{true};
    bool enable_hyperparameter_optimization{true};
    
    // Validation and testing
    bool enable_cross_validation{true};
    usize cv_folds{5};                           // 5-fold cross-validation
    bool enable_holdout_testing{true};
    f32 test_split{0.15f};                       // 15% of data for testing
    
    // Performance monitoring
    bool enable_training_visualization{true};
    bool track_model_performance_over_time{true};
    bool enable_model_drift_detection{true};
    f32 model_drift_threshold{0.1f};             // Accuracy drop threshold
    
    // Educational features
    bool enable_detailed_logging{true};
    bool generate_training_reports{true};
    bool explain_model_decisions{true};
};

/**
 * @brief Model registry entry
 */
struct ModelRegistryEntry {
    std::string model_name;
    std::string model_type;                      // "neural_network", "decision_tree", etc.
    std::unique_ptr<MLModelBase> model;
    MLModelConfig model_config;
    
    // Status and metrics
    bool is_trained{false};
    ModelValidationResult latest_validation;
    TrainingProgress training_progress;
    
    // Version management
    usize current_version{1};
    std::vector<ModelValidationResult> version_history;
    std::string model_file_path;
    
    // Usage statistics
    std::atomic<usize> prediction_count{0};
    std::atomic<usize> correct_predictions{0};
    f32 runtime_accuracy{0.0f};
    std::chrono::high_resolution_clock::time_point last_used;
    std::chrono::high_resolution_clock::time_point last_trained;
    
    // Dependencies and data
    std::string associated_data_collector;
    DataCollectionType required_data_type;
    usize min_training_samples{100};
    
    f32 get_runtime_accuracy() const {
        usize total = prediction_count.load();
        return total > 0 ? static_cast<f32>(correct_predictions.load()) / total : 0.0f;
    }
    
    bool needs_retraining(std::chrono::hours interval) const;
    bool has_sufficient_data(usize available_samples) const;
    
    std::string to_string() const;
};

/**
 * @brief Training job for asynchronous model training
 */
struct TrainingJob {
    std::string job_id;
    std::string model_name;
    TrainingDataset training_data;
    TrainingDataset validation_data;
    std::optional<TrainingDataset> test_data;
    
    // Training configuration
    MLModelConfig model_config;
    std::function<void(const TrainingProgress&)> progress_callback;
    std::function<void(const ModelValidationResult&)> completion_callback;
    
    // Job status
    std::atomic<ModelTrainingStatus> status{ModelTrainingStatus::NotStarted};
    std::shared_ptr<TrainingProgress> progress;
    std::future<ModelValidationResult> training_future;
    
    // Priority and scheduling
    usize priority{5};  // 1-10, higher = more important
    std::chrono::high_resolution_clock::time_point scheduled_time;
    std::chrono::high_resolution_clock::time_point started_time;
    
    std::string to_string() const;
};

/**
 * @brief Main ML model management system
 * 
 * This system manages the lifecycle of machine learning models used in the ECS,
 * including training, validation, versioning, and deployment. It provides educational
 * insights into ML model management in game engines.
 */
class MLModelManager {
private:
    ModelManagerConfig config_;
    
    // Model registry
    std::unordered_map<std::string, std::unique_ptr<ModelRegistryEntry>> model_registry_;
    mutable std::mutex registry_mutex_;
    
    // Training infrastructure
    std::queue<std::unique_ptr<TrainingJob>> training_queue_;
    std::vector<std::unique_ptr<TrainingJob>> active_jobs_;
    std::mutex training_mutex_;
    std::condition_variable training_cv_;
    
    // Background threads
    std::vector<std::unique_ptr<std::thread>> training_threads_;
    std::unique_ptr<std::thread> maintenance_thread_;
    std::atomic<bool> should_stop_threads_{false};
    
    // Data integration
    std::unique_ptr<MLTrainingDataCollector> data_collector_;
    std::unordered_map<std::string, TrainingDataset> cached_datasets_;
    
    // Performance tracking
    std::unordered_map<std::string, std::vector<f32>> model_accuracy_history_;
    std::unordered_map<std::string, f32> model_drift_scores_;
    
    // Statistics
    std::atomic<usize> total_models_trained_{0};
    std::atomic<usize> successful_trainings_{0};
    std::atomic<usize> failed_trainings_{0};
    std::atomic<usize> total_predictions_served_{0};
    
    // Job management
    std::atomic<usize> next_job_id_{1};
    
public:
    explicit MLModelManager(const ModelManagerConfig& config = ModelManagerConfig{});
    ~MLModelManager();
    
    // Non-copyable but movable
    MLModelManager(const MLModelManager&) = delete;
    MLModelManager& operator=(const MLModelManager&) = delete;
    MLModelManager(MLModelManager&&) = default;
    MLModelManager& operator=(MLModelManager&&) = default;
    
    // Model registration and lifecycle
    void register_model(const std::string& name, 
                       std::unique_ptr<MLModelBase> model,
                       const MLModelConfig& config,
                       DataCollectionType required_data_type);
    void unregister_model(const std::string& name);
    MLModelBase* get_model(const std::string& name);
    const MLModelBase* get_model(const std::string& name) const;
    
    // Model training
    std::string train_model(const std::string& model_name, const TrainingDataset& dataset);
    std::string train_model_async(const std::string& model_name, const TrainingDataset& dataset);
    void train_all_models();
    void schedule_training_job(std::unique_ptr<TrainingJob> job);
    
    // Model validation and testing
    ModelValidationResult validate_model(const std::string& model_name, const TrainingDataset& test_data);
    ModelValidationResult cross_validate_model(const std::string& model_name, const TrainingDataset& dataset);
    void validate_all_models();
    
    // Model persistence
    bool save_model(const std::string& model_name, const std::string& filepath = "");
    bool load_model(const std::string& model_name, const std::string& filepath);
    void save_all_models();
    void auto_save_models();
    
    // Model versioning
    void create_model_snapshot(const std::string& model_name);
    bool restore_model_version(const std::string& model_name, usize version);
    std::vector<usize> get_model_versions(const std::string& model_name) const;
    void cleanup_old_versions(const std::string& model_name);
    
    // Data integration
    void set_data_collector(std::unique_ptr<MLTrainingDataCollector> collector);
    void update_training_data(const std::string& model_name);
    TrainingDataset get_training_data(const std::string& model_name);
    
    // Performance monitoring
    void track_prediction_result(const std::string& model_name, bool was_correct);
    f32 calculate_model_drift(const std::string& model_name);
    std::vector<std::string> detect_underperforming_models() const;
    void monitor_model_performance();
    
    // Training job management
    std::vector<std::string> get_active_training_jobs() const;
    TrainingProgress get_training_progress(const std::string& job_id) const;
    bool cancel_training_job(const std::string& job_id);
    void pause_training_job(const std::string& job_id);
    void resume_training_job(const std::string& job_id);
    
    // Model management operations
    void start_model_manager();
    void stop_model_manager();
    void retrain_stale_models();
    void optimize_model_performance();
    
    // Information and statistics
    std::vector<std::string> list_registered_models() const;
    ModelRegistryEntry* get_model_info(const std::string& model_name);
    const ModelRegistryEntry* get_model_info(const std::string& model_name) const;
    
    // Configuration
    const ModelManagerConfig& config() const { return config_; }
    void update_config(const ModelManagerConfig& new_config);
    
    // Educational features
    std::string generate_model_management_report() const;
    std::string explain_training_process(const std::string& model_name) const;
    void print_model_status_summary() const;
    std::string visualize_model_performance(const std::string& model_name) const;
    std::string get_model_optimization_suggestions(const std::string& model_name) const;
    
    // Advanced features
    void perform_hyperparameter_optimization(const std::string& model_name);
    std::vector<std::string> suggest_model_ensemble_candidates() const;
    void benchmark_model_inference_speed(const std::string& model_name);
    
    // Callbacks for integration
    using TrainingCompleteCallback = std::function<void(const std::string&, const ModelValidationResult&)>;
    using ModelDriftCallback = std::function<void(const std::string&, f32)>;
    using TrainingProgressCallback = std::function<void(const std::string&, const TrainingProgress&)>;
    
    void set_training_complete_callback(TrainingCompleteCallback callback) { training_complete_callback_ = callback; }
    void set_model_drift_callback(ModelDriftCallback callback) { model_drift_callback_ = callback; }
    void set_training_progress_callback(TrainingProgressCallback callback) { training_progress_callback_ = callback; }
    
private:
    // Internal implementation
    void initialize_model_manager();
    void cleanup_model_manager();
    void start_background_threads();
    void stop_background_threads();
    
    // Training infrastructure
    void training_thread_function();
    void maintenance_thread_function();
    ModelValidationResult train_model_internal(TrainingJob& job);
    
    // Model validation implementation
    ModelValidationResult validate_model_internal(const std::string& model_name,
                                                 const TrainingDataset& test_data);
    ModelValidationResult cross_validate_internal(const std::string& model_name,
                                                 const TrainingDataset& dataset);
    
    // Data management
    void prepare_training_data(const std::string& model_name, TrainingJob& job);
    void split_dataset(const TrainingDataset& dataset, 
                      TrainingDataset& train_set,
                      TrainingDataset& val_set,
                      std::optional<TrainingDataset>& test_set);
    
    // File management
    std::string get_model_filepath(const std::string& model_name, usize version = 0) const;
    std::string get_model_directory(const std::string& model_name) const;
    void ensure_model_directory_exists(const std::string& model_name);
    
    // Performance monitoring implementation
    void update_model_drift_scores();
    bool should_retrain_model(const std::string& model_name) const;
    void schedule_automatic_retraining();
    
    // Job management implementation
    std::string generate_job_id();
    void cleanup_completed_jobs();
    void update_job_progress(TrainingJob& job, const TrainingProgress& progress);
    
    // Educational content generation
    std::string explain_model_type(const std::string& model_type) const;
    std::string generate_training_recommendations(const std::string& model_name) const;
    
    // Utilities
    ModelValidationResult calculate_validation_metrics(const std::string& model_name,
                                                       const std::vector<FeatureVector>& inputs,
                                                       const std::vector<PredictionResult>& expected_outputs,
                                                       const std::vector<PredictionResult>& predicted_outputs);
    
    // Callbacks
    TrainingCompleteCallback training_complete_callback_;
    ModelDriftCallback model_drift_callback_;
    TrainingProgressCallback training_progress_callback_;
};

/**
 * @brief Utility functions for model management
 */
namespace model_management_utils {

// Model evaluation utilities
ModelValidationResult calculate_classification_metrics(const std::vector<PredictionResult>& predicted,
                                                      const std::vector<PredictionResult>& actual);
ModelValidationResult calculate_regression_metrics(const std::vector<PredictionResult>& predicted,
                                                   const std::vector<PredictionResult>& actual);

// Training utilities
std::pair<TrainingDataset, TrainingDataset> split_training_dataset(const TrainingDataset& dataset,
                                                                   f32 train_ratio = 0.8f);
TrainingDataset merge_training_datasets(const std::vector<TrainingDataset>& datasets);

// Model comparison
struct ModelComparisonResult {
    std::string model1_name;
    std::string model2_name;
    f32 accuracy_difference{0.0f};
    f32 speed_difference{0.0f};
    f32 memory_difference{0.0f};
    std::string recommendation;
};

ModelComparisonResult compare_models(const ModelRegistryEntry& model1,
                                    const ModelRegistryEntry& model2,
                                    const TrainingDataset& test_data);

// Educational utilities
std::string visualize_training_progress(const std::vector<f32>& losses);
std::string explain_validation_metrics(const ModelValidationResult& result);
std::string create_model_performance_timeline(const std::vector<f32>& accuracy_history);

// Hyperparameter optimization
std::vector<MLModelConfig> generate_hyperparameter_candidates(const MLModelConfig& base_config);
MLModelConfig optimize_hyperparameters(const std::string& model_name,
                                       const TrainingDataset& dataset,
                                       const std::vector<MLModelConfig>& candidates);

} // namespace model_management_utils

} // namespace ecscope::ml