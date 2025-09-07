#pragma once

#include "core/types.hpp"
#include "core/log.hpp"
#include "entity.hpp"
#include "component.hpp"
#include "signature.hpp"
#include "registry.hpp"
#include <vector>
#include <memory>
#include <unordered_map>
#include <string>
#include <functional>
#include <chrono>
#include <numeric>
#include <algorithm>
#include <concepts>
#include <ranges>
#include <span>
#include <array>
#include <optional>

namespace ecscope::ml {

// Forward declarations
class MLModelBase;
class PredictionEngine;
class TrainingDataCollector;
class ModelTrainer;

/**
 * @brief Core data types for ML predictions in ECS
 */
using FeatureVector = std::vector<f32>;
using PredictionResult = std::vector<f32>;
using EntityID = ecs::Entity;
using Timestamp = std::chrono::high_resolution_clock::time_point;

/**
 * @brief Training sample containing input features and expected output
 */
struct TrainingSample {
    FeatureVector features;
    PredictionResult expected_output;
    f32 weight{1.0f};  // Sample importance weight
    Timestamp timestamp{std::chrono::high_resolution_clock::now()};
    
    TrainingSample() = default;
    TrainingSample(const FeatureVector& f, const PredictionResult& out, f32 w = 1.0f)
        : features(f), expected_output(out), weight(w) {}
};

/**
 * @brief Training dataset with educational features
 */
class TrainingDataset {
private:
    std::vector<TrainingSample> samples_;
    std::string dataset_name_;
    usize max_samples_;
    bool enable_normalization_;
    
public:
    explicit TrainingDataset(const std::string& name = "Unnamed", 
                           usize max_samples = 10000, 
                           bool normalize = true)
        : dataset_name_(name), max_samples_(max_samples), enable_normalization_(normalize) {
        samples_.reserve(max_samples);
    }
    
    // Add training sample
    void add_sample(const TrainingSample& sample) {
        if (samples_.size() >= max_samples_) {
            // Remove oldest sample to make room (FIFO)
            samples_.erase(samples_.begin());
        }
        samples_.push_back(sample);
    }
    
    // Add sample with convenience constructor
    void add_sample(const FeatureVector& features, const PredictionResult& output, f32 weight = 1.0f) {
        add_sample(TrainingSample{features, output, weight});
    }
    
    // Get all samples
    const std::vector<TrainingSample>& samples() const { return samples_; }
    std::vector<TrainingSample>& samples() { return samples_; }
    
    // Dataset statistics
    usize size() const { return samples_.size(); }
    bool empty() const { return samples_.empty(); }
    usize feature_dimension() const { 
        return samples_.empty() ? 0 : samples_.front().features.size(); 
    }
    usize output_dimension() const {
        return samples_.empty() ? 0 : samples_.front().expected_output.size();
    }
    
    // Data preprocessing
    void normalize_features();
    void shuffle_samples();
    void split_dataset(f32 train_ratio, TrainingDataset& train_set, TrainingDataset& test_set) const;
    
    // Educational insights
    std::string get_dataset_summary() const;
    void print_statistics() const;
    
    const std::string& name() const { return dataset_name_; }
};

/**
 * @brief Prediction metrics for model evaluation
 */
struct PredictionMetrics {
    f32 accuracy{0.0f};
    f32 precision{0.0f};
    f32 recall{0.0f};
    f32 f1_score{0.0f};
    f32 mean_absolute_error{0.0f};
    f32 mean_squared_error{0.0f};
    f32 confidence{0.0f};
    usize total_predictions{0};
    usize correct_predictions{0};
    
    void reset() {
        *this = PredictionMetrics{};
    }
    
    void update_from_prediction(const PredictionResult& predicted, 
                               const PredictionResult& actual,
                               f32 threshold = 0.5f);
                               
    std::string to_string() const;
};

/**
 * @brief Configuration for ML model behavior
 */
struct MLModelConfig {
    std::string model_name{"UnnamedModel"};
    usize input_dimension{0};
    usize output_dimension{0};
    f32 learning_rate{0.001f};
    usize max_epochs{1000};
    f32 convergence_threshold{0.001f};
    bool enable_regularization{true};
    f32 regularization_strength{0.01f};
    bool enable_early_stopping{true};
    usize early_stopping_patience{50};
    bool verbose_training{false};
    
    // Educational features
    bool enable_training_visualization{true};
    bool track_learning_curve{true};
    usize validation_frequency{10}; // Every N epochs
};

/**
 * @brief Base class for all ML models in ECS prediction system
 * 
 * This abstract base class provides the common interface for different
 * types of machine learning models used in ECS prediction. It includes
 * educational features for understanding model behavior and performance.
 */
class MLModelBase {
protected:
    MLModelConfig config_;
    PredictionMetrics training_metrics_;
    PredictionMetrics validation_metrics_;
    std::vector<f32> learning_curve_;
    bool is_trained_;
    Timestamp last_training_time_;
    std::string model_type_;
    
public:
    explicit MLModelBase(const MLModelConfig& config, const std::string& type)
        : config_(config), is_trained_(false), model_type_(type) {}
    
    virtual ~MLModelBase() = default;
    
    // Pure virtual methods that must be implemented by derived classes
    virtual bool train(const TrainingDataset& dataset) = 0;
    virtual PredictionResult predict(const FeatureVector& features) const = 0;
    virtual bool save_model(const std::string& filepath) const = 0;
    virtual bool load_model(const std::string& filepath) = 0;
    
    // Optional methods with default implementations
    virtual f32 evaluate(const TrainingDataset& test_set);
    virtual std::vector<f32> get_feature_importance() const { return {}; }
    virtual void reset_model() { 
        is_trained_ = false; 
        training_metrics_.reset();
        validation_metrics_.reset();
        learning_curve_.clear();
    }
    
    // Model information
    bool is_trained() const { return is_trained_; }
    const MLModelConfig& config() const { return config_; }
    const PredictionMetrics& training_metrics() const { return training_metrics_; }
    const PredictionMetrics& validation_metrics() const { return validation_metrics_; }
    const std::vector<f32>& learning_curve() const { return learning_curve_; }
    const std::string& model_type() const { return model_type_; }
    
    // Educational features
    std::string get_model_summary() const;
    void print_training_progress() const;
    
protected:
    void set_trained(bool trained) { 
        is_trained_ = trained; 
        if (trained) {
            last_training_time_ = std::chrono::high_resolution_clock::now();
        }
    }
    
    void add_learning_curve_point(f32 loss) {
        learning_curve_.push_back(loss);
    }
    
    void update_training_metrics(const PredictionMetrics& metrics) {
        training_metrics_ = metrics;
    }
    
    void update_validation_metrics(const PredictionMetrics& metrics) {
        validation_metrics_ = metrics;
    }
};

/**
 * @brief Simple neural network implementation for ECS predictions
 * 
 * Educational implementation of a basic feedforward neural network
 * suitable for learning ECS behavior patterns. This is designed to be
 * understandable and educational rather than highly optimized.
 */
class SimpleNeuralNetwork : public MLModelBase {
public:
    struct Layer {
        std::vector<std::vector<f32>> weights;  // weights[neuron][input]
        std::vector<f32> biases;                // biases[neuron]
        std::vector<f32> activations;           // activations[neuron]
        std::vector<f32> gradients;             // gradients[neuron]
        
        Layer(usize input_size, usize output_size);
        void forward_pass(const std::vector<f32>& inputs);
        void backward_pass(const std::vector<f32>& next_layer_gradients);
        void apply_gradients(f32 learning_rate);
        f32 activation_function(f32 x) const; // ReLU
        f32 activation_derivative(f32 x) const;
    };
    
private:
    std::vector<Layer> layers_;
    std::vector<usize> layer_sizes_;
    
public:
    explicit SimpleNeuralNetwork(const MLModelConfig& config, 
                                const std::vector<usize>& hidden_layers = {64, 32});
    
    // MLModelBase implementation
    bool train(const TrainingDataset& dataset) override;
    PredictionResult predict(const FeatureVector& features) const override;
    bool save_model(const std::string& filepath) const override;
    bool load_model(const std::string& filepath) override;
    std::vector<f32> get_feature_importance() const override;
    
    // Neural network specific methods
    void add_layer(usize size);
    usize layer_count() const { return layers_.size(); }
    const std::vector<usize>& layer_sizes() const { return layer_sizes_; }
    
private:
    void initialize_weights();
    f32 compute_loss(const std::vector<TrainingSample>& samples) const;
    void forward_propagation(const FeatureVector& input) const;
    void backward_propagation(const TrainingSample& sample);
    f32 mean_squared_error(const PredictionResult& predicted, const PredictionResult& actual) const;
};

/**
 * @brief Linear regression model for simple ECS predictions
 * 
 * Educational implementation of linear regression with regularization.
 * Useful for understanding basic ML concepts and simple prediction tasks.
 */
class LinearRegressionModel : public MLModelBase {
private:
    std::vector<f32> weights_;
    f32 bias_;
    std::vector<f32> feature_importance_;
    
public:
    explicit LinearRegressionModel(const MLModelConfig& config);
    
    // MLModelBase implementation
    bool train(const TrainingDataset& dataset) override;
    PredictionResult predict(const FeatureVector& features) const override;
    bool save_model(const std::string& filepath) const override;
    bool load_model(const std::string& filepath) override;
    std::vector<f32> get_feature_importance() const override { return feature_importance_; }
    
    // Linear regression specific methods
    const std::vector<f32>& weights() const { return weights_; }
    f32 bias() const { return bias_; }
    
private:
    void initialize_weights(usize feature_count);
    void gradient_descent(const TrainingDataset& dataset);
    f32 compute_cost(const TrainingDataset& dataset) const;
    void update_feature_importance();
};

/**
 * @brief Decision tree model for ECS behavior classification
 * 
 * Simple decision tree implementation for understanding rule-based
 * predictions in ECS systems. Educational focus on interpretability.
 */
class DecisionTreeModel : public MLModelBase {
public:
    struct TreeNode {
        bool is_leaf{false};
        usize feature_index{0};     // Which feature to split on
        f32 threshold{0.0f};        // Split threshold
        PredictionResult prediction; // For leaf nodes
        std::unique_ptr<TreeNode> left_child;
        std::unique_ptr<TreeNode> right_child;
        usize samples_count{0};
        f32 impurity{0.0f};         // Gini impurity or entropy
        
        TreeNode() = default;
        bool is_valid() const { return is_leaf || (left_child && right_child); }
    };
    
private:
    std::unique_ptr<TreeNode> root_;
    usize max_depth_;
    usize min_samples_split_;
    f32 min_impurity_decrease_;
    std::vector<f32> feature_importance_;
    
public:
    explicit DecisionTreeModel(const MLModelConfig& config, 
                              usize max_depth = 10,
                              usize min_samples_split = 2);
    
    // MLModelBase implementation
    bool train(const TrainingDataset& dataset) override;
    PredictionResult predict(const FeatureVector& features) const override;
    bool save_model(const std::string& filepath) const override;
    bool load_model(const std::string& filepath) override;
    std::vector<f32> get_feature_importance() const override { return feature_importance_; }
    
    // Decision tree specific methods
    std::string visualize_tree() const;
    usize tree_depth() const;
    usize node_count() const;
    
private:
    std::unique_ptr<TreeNode> build_tree(const std::vector<usize>& sample_indices,
                                        const TrainingDataset& dataset,
                                        usize current_depth);
    f32 calculate_impurity(const std::vector<usize>& sample_indices,
                          const TrainingDataset& dataset) const;
    std::pair<usize, f32> find_best_split(const std::vector<usize>& sample_indices,
                                         const TrainingDataset& dataset) const;
    PredictionResult predict_node(const TreeNode& node, const FeatureVector& features) const;
    std::string visualize_node(const TreeNode& node, usize depth = 0) const;
    usize count_nodes(const TreeNode& node) const;
    usize calculate_depth(const TreeNode& node) const;
    void calculate_feature_importance();
};

/**
 * @brief Model factory for creating different types of ML models
 */
class MLModelFactory {
public:
    enum class ModelType {
        LinearRegression,
        NeuralNetwork,
        DecisionTree
    };
    
    static std::unique_ptr<MLModelBase> create_model(ModelType type, 
                                                    const MLModelConfig& config);
    
    static std::unique_ptr<MLModelBase> create_linear_regression(const MLModelConfig& config);
    static std::unique_ptr<MLModelBase> create_neural_network(const MLModelConfig& config,
                                                             const std::vector<usize>& hidden_layers = {64, 32});
    static std::unique_ptr<MLModelBase> create_decision_tree(const MLModelConfig& config,
                                                            usize max_depth = 10,
                                                            usize min_samples_split = 2);
    
    static std::string model_type_to_string(ModelType type);
    static ModelType model_type_from_string(const std::string& type_str);
};

/**
 * @brief Prediction context containing ECS state information
 * 
 * This structure captures the current state of the ECS system
 * for use as input features to ML models.
 */
struct PredictionContext {
    // Entity information
    EntityID entity{ecs::null_entity()};
    ecs::ComponentSignature entity_signature;
    usize entity_age{0};  // How long entity has existed
    
    // System state
    f32 frame_time{0.0f};
    f32 system_load{0.0f};
    usize active_entities{0};
    usize total_components{0};
    
    // Memory information
    f32 memory_usage{0.0f};
    f32 memory_pressure{0.0f};
    usize memory_allocations{0};
    
    // Performance metrics
    f32 fps{0.0f};
    f32 frame_variance{0.0f};
    f32 system_efficiency{0.0f};
    
    // Historical data (recent frames)
    std::array<f32, 10> recent_frame_times{};
    std::array<f32, 10> recent_memory_usage{};
    
    // Convert to feature vector for ML models
    FeatureVector to_feature_vector() const;
    
    // Create context from ECS registry
    static PredictionContext from_registry(const ecs::Registry& registry, 
                                         EntityID entity = ecs::null_entity());
    
    // Educational features
    std::string to_string() const;
    void print_summary() const;
};

/**
 * @brief Feature extractor for converting ECS state to ML features
 * 
 * This class is responsible for converting ECS system state into
 * feature vectors that can be consumed by ML models.
 */
class FeatureExtractor {
public:
    struct ExtractionConfig {
        bool include_entity_features{true};
        bool include_system_features{true};
        bool include_memory_features{true};
        bool include_performance_features{true};
        bool include_temporal_features{true};
        bool normalize_features{true};
        f32 normalization_range{1.0f};
    };
    
private:
    ExtractionConfig config_;
    std::unordered_map<std::string, f32> feature_stats_; // For normalization
    std::unordered_map<std::string, usize> feature_indices_;
    std::vector<std::string> feature_names_;
    
public:
    explicit FeatureExtractor(const ExtractionConfig& config = ExtractionConfig{});
    
    // Extract features from prediction context
    FeatureVector extract_features(const PredictionContext& context);
    
    // Extract features directly from registry
    FeatureVector extract_features_from_registry(const ecs::Registry& registry, 
                                                 EntityID entity = ecs::null_entity());
    
    // Feature information
    usize feature_dimension() const { return feature_names_.size(); }
    const std::vector<std::string>& feature_names() const { return feature_names_; }
    std::string get_feature_name(usize index) const;
    
    // Statistics and normalization
    void update_feature_statistics(const FeatureVector& features);
    void normalize_features(FeatureVector& features) const;
    
    // Educational features
    std::string get_feature_description(usize index) const;
    void print_feature_summary() const;
    std::string visualize_feature_importance(const std::vector<f32>& importance) const;
    
private:
    void initialize_feature_names();
    f32 normalize_value(f32 value, const std::string& feature_name) const;
};

} // namespace ecscope::ml