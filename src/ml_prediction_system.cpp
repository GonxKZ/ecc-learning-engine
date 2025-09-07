/**
 * @file ml_prediction_system.cpp
 * @brief Implementation of core ML prediction system components
 * 
 * This file implements the foundational classes for the machine learning
 * prediction system, including dataset management, model implementations,
 * and feature extraction utilities.
 */

#include <ecscope/ml_prediction_system.hpp>
#include <ecscope/core/log.hpp>
#include <algorithm>
#include <numeric>
#include <random>
#include <fstream>
#include <sstream>
#include <cmath>
#include <iomanip>

namespace ecscope::ml {

// =============================================================================
// TrainingDataset Implementation
// =============================================================================

void TrainingDataset::normalize_features() {
    if (samples_.empty()) return;
    
    const usize feature_dim = feature_dimension();
    if (feature_dim == 0) return;
    
    // Calculate mean and standard deviation for each feature
    std::vector<f32> means(feature_dim, 0.0f);
    std::vector<f32> std_devs(feature_dim, 0.0f);
    
    // Calculate means
    for (const auto& sample : samples_) {
        for (usize i = 0; i < feature_dim; ++i) {
            means[i] += sample.features[i];
        }
    }
    
    for (auto& mean : means) {
        mean /= static_cast<f32>(samples_.size());
    }
    
    // Calculate standard deviations
    for (const auto& sample : samples_) {
        for (usize i = 0; i < feature_dim; ++i) {
            f32 diff = sample.features[i] - means[i];
            std_devs[i] += diff * diff;
        }
    }
    
    for (auto& std_dev : std_devs) {
        std_dev = std::sqrt(std_dev / static_cast<f32>(samples_.size()));
        if (std_dev < 1e-8f) std_dev = 1.0f; // Prevent division by zero
    }
    
    // Normalize features
    for (auto& sample : samples_) {
        for (usize i = 0; i < feature_dim; ++i) {
            sample.features[i] = (sample.features[i] - means[i]) / std_devs[i];
        }
    }
    
    if (enable_normalization_) {
        LOG_INFO("Normalized dataset '{}': {} features normalized", dataset_name_, feature_dim);
    }
}

void TrainingDataset::shuffle_samples() {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::shuffle(samples_.begin(), samples_.end(), gen);
}

void TrainingDataset::split_dataset(f32 train_ratio, TrainingDataset& train_set, TrainingDataset& test_set) const {
    if (train_ratio < 0.0f || train_ratio > 1.0f) {
        LOG_WARN("Invalid train ratio {:.2f}, using 0.8", train_ratio);
        train_ratio = 0.8f;
    }
    
    const usize train_size = static_cast<usize>(samples_.size() * train_ratio);
    
    train_set = TrainingDataset(dataset_name_ + "_train", max_samples_, enable_normalization_);
    test_set = TrainingDataset(dataset_name_ + "_test", max_samples_, enable_normalization_);
    
    for (usize i = 0; i < samples_.size(); ++i) {
        if (i < train_size) {
            train_set.add_sample(samples_[i]);
        } else {
            test_set.add_sample(samples_[i]);
        }
    }
    
    LOG_INFO("Split dataset '{}' into train ({}) and test ({}) sets", 
             dataset_name_, train_set.size(), test_set.size());
}

std::string TrainingDataset::get_dataset_summary() const {
    std::ostringstream oss;
    oss << "Dataset: " << dataset_name_ << "\n";
    oss << "  Samples: " << size() << " / " << max_samples_ << "\n";
    oss << "  Feature dimension: " << feature_dimension() << "\n";
    oss << "  Output dimension: " << output_dimension() << "\n";
    oss << "  Normalization: " << (enable_normalization_ ? "enabled" : "disabled") << "\n";
    
    if (!samples_.empty()) {
        // Calculate basic statistics
        const auto& first_sample = samples_.front();
        if (!first_sample.features.empty()) {
            std::vector<f32> feature_sums(first_sample.features.size(), 0.0f);
            for (const auto& sample : samples_) {
                for (usize i = 0; i < sample.features.size(); ++i) {
                    feature_sums[i] += sample.features[i];
                }
            }
            
            oss << "  Feature means: [";
            for (usize i = 0; i < feature_sums.size(); ++i) {
                oss << std::fixed << std::setprecision(3) << (feature_sums[i] / samples_.size());
                if (i < feature_sums.size() - 1) oss << ", ";
            }
            oss << "]\n";
        }
    }
    
    return oss.str();
}

void TrainingDataset::print_statistics() const {
    std::cout << get_dataset_summary() << std::endl;
}

// =============================================================================
// PredictionMetrics Implementation
// =============================================================================

void PredictionMetrics::update_from_prediction(const PredictionResult& predicted,
                                              const PredictionResult& actual,
                                              f32 threshold) {
    if (predicted.size() != actual.size()) {
        LOG_WARN("Prediction and actual result size mismatch: {} vs {}", predicted.size(), actual.size());
        return;
    }
    
    total_predictions++;
    
    // Calculate metrics based on prediction type
    if (predicted.size() == 1) {
        // Binary or regression case
        f32 pred_val = predicted[0];
        f32 actual_val = actual[0];
        
        // Mean absolute error
        mean_absolute_error = (mean_absolute_error * (total_predictions - 1) + 
                              std::abs(pred_val - actual_val)) / total_predictions;
        
        // Mean squared error
        f32 squared_error = (pred_val - actual_val) * (pred_val - actual_val);
        mean_squared_error = (mean_squared_error * (total_predictions - 1) + squared_error) / total_predictions;
        
        // Binary classification metrics
        if (actual_val == 0.0f || actual_val == 1.0f) {
            bool predicted_positive = pred_val >= threshold;
            bool actual_positive = actual_val >= 0.5f;
            
            if (predicted_positive && actual_positive) {
                correct_predictions++;
            } else if (!predicted_positive && !actual_positive) {
                correct_predictions++;
            }
        } else {
            // Regression: consider "correct" if within 10% of actual value
            if (std::abs(actual_val) > 1e-8f) {
                f32 relative_error = std::abs(pred_val - actual_val) / std::abs(actual_val);
                if (relative_error <= 0.1f) {
                    correct_predictions++;
                }
            }
        }
    } else {
        // Multi-class classification case
        usize predicted_class = std::distance(predicted.begin(), 
                                            std::max_element(predicted.begin(), predicted.end()));
        usize actual_class = std::distance(actual.begin(), 
                                         std::max_element(actual.begin(), actual.end()));
        
        if (predicted_class == actual_class) {
            correct_predictions++;
        }
        
        // Calculate MAE for multi-output case
        f32 total_error = 0.0f;
        for (usize i = 0; i < predicted.size(); ++i) {
            total_error += std::abs(predicted[i] - actual[i]);
        }
        mean_absolute_error = (mean_absolute_error * (total_predictions - 1) + 
                              total_error / predicted.size()) / total_predictions;
    }
    
    // Update derived metrics
    accuracy = static_cast<f32>(correct_predictions) / total_predictions;
    
    // Update confidence (use max probability as confidence measure)
    f32 max_confidence = *std::max_element(predicted.begin(), predicted.end());
    confidence = (confidence * (total_predictions - 1) + max_confidence) / total_predictions;
}

std::string PredictionMetrics::to_string() const {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(3);
    oss << "PredictionMetrics:\n";
    oss << "  Accuracy: " << accuracy << " (" << correct_predictions << "/" << total_predictions << ")\n";
    oss << "  Precision: " << precision << "\n";
    oss << "  Recall: " << recall << "\n";
    oss << "  F1 Score: " << f1_score << "\n";
    oss << "  MAE: " << mean_absolute_error << "\n";
    oss << "  MSE: " << mean_squared_error << "\n";
    oss << "  Confidence: " << confidence << "\n";
    return oss.str();
}

// =============================================================================
// MLModelBase Implementation
// =============================================================================

f32 MLModelBase::evaluate(const TrainingDataset& test_set) {
    if (test_set.empty()) {
        LOG_WARN("Cannot evaluate model on empty test set");
        return 0.0f;
    }
    
    if (!is_trained_) {
        LOG_WARN("Cannot evaluate untrained model");
        return 0.0f;
    }
    
    PredictionMetrics eval_metrics;
    
    for (const auto& sample : test_set.samples()) {
        try {
            auto prediction = predict(sample.features);
            eval_metrics.update_from_prediction(prediction, sample.expected_output);
        } catch (const std::exception& e) {
            LOG_WARN("Error during evaluation: {}", e.what());
        }
    }
    
    validation_metrics_ = eval_metrics;
    
    LOG_INFO("Model '{}' evaluation completed: {:.3f} accuracy on {} samples",
             config_.model_name, eval_metrics.accuracy, test_set.size());
    
    return eval_metrics.accuracy;
}

std::string MLModelBase::get_model_summary() const {
    std::ostringstream oss;
    oss << "Model: " << config_.model_name << " (" << model_type_ << ")\n";
    oss << "  Input dimension: " << config_.input_dimension << "\n";
    oss << "  Output dimension: " << config_.output_dimension << "\n";
    oss << "  Learning rate: " << config_.learning_rate << "\n";
    oss << "  Max epochs: " << config_.max_epochs << "\n";
    oss << "  Trained: " << (is_trained_ ? "yes" : "no") << "\n";
    
    if (is_trained_) {
        oss << "  Training accuracy: " << training_metrics_.accuracy << "\n";
        oss << "  Validation accuracy: " << validation_metrics_.accuracy << "\n";
        oss << "  Learning curve points: " << learning_curve_.size() << "\n";
    }
    
    return oss.str();
}

void MLModelBase::print_training_progress() const {
    if (learning_curve_.empty()) {
        std::cout << "No training progress available\n";
        return;
    }
    
    std::cout << "Training progress for " << config_.model_name << ":\n";
    std::cout << "Epoch\tLoss\n";
    std::cout << "-----\t----\n";
    
    const usize step = std::max(1ul, learning_curve_.size() / 10); // Show max 10 points
    for (usize i = 0; i < learning_curve_.size(); i += step) {
        std::cout << std::setw(5) << (i + 1) << "\t" 
                  << std::fixed << std::setprecision(4) << learning_curve_[i] << "\n";
    }
    
    if (learning_curve_.size() > 1) {
        f32 initial_loss = learning_curve_.front();
        f32 final_loss = learning_curve_.back();
        f32 improvement = initial_loss - final_loss;
        std::cout << "Improvement: " << std::fixed << std::setprecision(4) 
                  << improvement << " (final loss: " << final_loss << ")\n";
    }
}

// =============================================================================
// SimpleNeuralNetwork Implementation
// =============================================================================

SimpleNeuralNetwork::Layer::Layer(usize input_size, usize output_size) {
    weights.resize(output_size);
    biases.resize(output_size);
    activations.resize(output_size);
    gradients.resize(output_size);
    
    // Initialize weights with Xavier/Glorot initialization
    std::random_device rd;
    std::mt19937 gen(rd());
    f32 weight_range = std::sqrt(6.0f / (input_size + output_size));
    std::uniform_real_distribution<f32> weight_dist(-weight_range, weight_range);
    
    for (usize i = 0; i < output_size; ++i) {
        weights[i].resize(input_size);
        for (usize j = 0; j < input_size; ++j) {
            weights[i][j] = weight_dist(gen);
        }
        biases[i] = 0.0f; // Initialize biases to zero
    }
}

void SimpleNeuralNetwork::Layer::forward_pass(const std::vector<f32>& inputs) {
    for (usize i = 0; i < activations.size(); ++i) {
        f32 sum = biases[i];
        for (usize j = 0; j < inputs.size(); ++j) {
            sum += weights[i][j] * inputs[j];
        }
        activations[i] = activation_function(sum);
    }
}

f32 SimpleNeuralNetwork::Layer::activation_function(f32 x) const {
    // ReLU activation function
    return std::max(0.0f, x);
}

f32 SimpleNeuralNetwork::Layer::activation_derivative(f32 x) const {
    // ReLU derivative
    return x > 0.0f ? 1.0f : 0.0f;
}

SimpleNeuralNetwork::SimpleNeuralNetwork(const MLModelConfig& config, 
                                        const std::vector<usize>& hidden_layers)
    : MLModelBase(config, "SimpleNeuralNetwork") {
    
    // Build layer architecture
    layer_sizes_.push_back(config.input_dimension);
    for (usize size : hidden_layers) {
        layer_sizes_.push_back(size);
    }
    layer_sizes_.push_back(config.output_dimension);
    
    // Create layers
    for (usize i = 0; i < layer_sizes_.size() - 1; ++i) {
        layers_.emplace_back(layer_sizes_[i], layer_sizes_[i + 1]);
    }
    
    LOG_INFO("Created neural network with architecture: [{}]", 
             [&]() {
                 std::ostringstream oss;
                 for (usize i = 0; i < layer_sizes_.size(); ++i) {
                     oss << layer_sizes_[i];
                     if (i < layer_sizes_.size() - 1) oss << ", ";
                 }
                 return oss.str();
             }());
}

bool SimpleNeuralNetwork::train(const TrainingDataset& dataset) {
    if (dataset.empty()) {
        LOG_WARN("Cannot train on empty dataset");
        return false;
    }
    
    if (dataset.feature_dimension() != config_.input_dimension) {
        LOG_WARN("Dataset feature dimension ({}) doesn't match model input dimension ({})",
                dataset.feature_dimension(), config_.input_dimension);
        return false;
    }
    
    LOG_INFO("Starting training of neural network '{}' on {} samples",
             config_.model_name, dataset.size());
    
    auto train_start = std::chrono::high_resolution_clock::now();
    
    // Training loop
    for (usize epoch = 0; epoch < config_.max_epochs; ++epoch) {
        f32 epoch_loss = 0.0f;
        
        // Shuffle samples for each epoch
        auto samples = dataset.samples();
        static std::random_device rd;
        static std::mt19937 gen(rd());
        std::shuffle(samples.begin(), samples.end(), gen);
        
        // Train on each sample
        for (const auto& sample : samples) {
            // Forward pass
            forward_propagation(sample.features);
            
            // Calculate loss
            f32 sample_loss = mean_squared_error(layers_.back().activations, sample.expected_output);
            epoch_loss += sample_loss;
            
            // Backward pass
            backward_propagation(sample);
        }
        
        epoch_loss /= samples.size();
        add_learning_curve_point(epoch_loss);
        
        // Check for convergence
        if (epoch > 0 && std::abs(learning_curve_[epoch - 1] - epoch_loss) < config_.convergence_threshold) {
            LOG_INFO("Converged at epoch {} with loss {:.6f}", epoch + 1, epoch_loss);
            break;
        }
        
        // Log progress periodically
        if (config_.verbose_training && epoch % 100 == 0) {
            LOG_INFO("Epoch {}: loss = {:.6f}", epoch + 1, epoch_loss);
        }
    }
    
    auto train_end = std::chrono::high_resolution_clock::now();
    auto train_duration = std::chrono::duration_cast<std::chrono::milliseconds>(train_end - train_start);
    
    set_trained(true);
    
    LOG_INFO("Neural network training completed in {}ms. Final loss: {:.6f}",
             train_duration.count(), learning_curve_.back());
    
    return true;
}

PredictionResult SimpleNeuralNetwork::predict(const FeatureVector& features) const {
    if (!is_trained_) {
        LOG_WARN("Cannot make prediction with untrained model");
        return {};
    }
    
    if (features.size() != config_.input_dimension) {
        LOG_WARN("Feature vector size ({}) doesn't match model input dimension ({})",
                features.size(), config_.input_dimension);
        return {};
    }
    
    forward_propagation(features);
    return layers_.back().activations;
}

void SimpleNeuralNetwork::forward_propagation(const FeatureVector& input) const {
    std::vector<f32> layer_input = input;
    
    for (auto& layer : layers_) {
        layer.forward_pass(layer_input);
        layer_input = layer.activations;
    }
}

void SimpleNeuralNetwork::backward_propagation(const TrainingSample& sample) {
    // Calculate output layer gradients
    auto& output_layer = layers_.back();
    for (usize i = 0; i < output_layer.gradients.size(); ++i) {
        f32 error = output_layer.activations[i] - sample.expected_output[i];
        output_layer.gradients[i] = error * output_layer.activation_derivative(output_layer.activations[i]);
    }
    
    // Backpropagate gradients through hidden layers
    for (int layer_idx = static_cast<int>(layers_.size()) - 2; layer_idx >= 0; --layer_idx) {
        auto& current_layer = layers_[layer_idx];
        const auto& next_layer = layers_[layer_idx + 1];
        
        for (usize i = 0; i < current_layer.gradients.size(); ++i) {
            f32 error_sum = 0.0f;
            for (usize j = 0; j < next_layer.gradients.size(); ++j) {
                error_sum += next_layer.gradients[j] * next_layer.weights[j][i];
            }
            current_layer.gradients[i] = error_sum * current_layer.activation_derivative(current_layer.activations[i]);
        }
    }
    
    // Update weights and biases
    std::vector<f32> layer_input = sample.features;
    
    for (auto& layer : layers_) {
        layer.apply_gradients(config_.learning_rate);
        
        for (usize i = 0; i < layer.weights.size(); ++i) {
            for (usize j = 0; j < layer.weights[i].size(); ++j) {
                layer.weights[i][j] -= config_.learning_rate * layer.gradients[i] * layer_input[j];
            }
            layer.biases[i] -= config_.learning_rate * layer.gradients[i];
        }
        
        layer_input = layer.activations;
    }
}

void SimpleNeuralNetwork::Layer::apply_gradients(f32 learning_rate) {
    // Gradients are applied in backward_propagation method
    // This method could be used for additional gradient processing
}

f32 SimpleNeuralNetwork::mean_squared_error(const PredictionResult& predicted, 
                                           const PredictionResult& actual) const {
    if (predicted.size() != actual.size()) {
        return std::numeric_limits<f32>::max();
    }
    
    f32 mse = 0.0f;
    for (usize i = 0; i < predicted.size(); ++i) {
        f32 diff = predicted[i] - actual[i];
        mse += diff * diff;
    }
    
    return mse / predicted.size();
}

bool SimpleNeuralNetwork::save_model(const std::string& filepath) const {
    std::ofstream file(filepath, std::ios::binary);
    if (!file.is_open()) {
        LOG_WARN("Failed to open file for saving: {}", filepath);
        return false;
    }
    
    try {
        // Save model metadata
        usize layer_count = layers_.size();
        file.write(reinterpret_cast<const char*>(&layer_count), sizeof(layer_count));
        
        // Save layer sizes
        for (usize size : layer_sizes_) {
            file.write(reinterpret_cast<const char*>(&size), sizeof(size));
        }
        
        // Save layer weights and biases
        for (const auto& layer : layers_) {
            // Save weights
            for (const auto& neuron_weights : layer.weights) {
                usize weight_count = neuron_weights.size();
                file.write(reinterpret_cast<const char*>(&weight_count), sizeof(weight_count));
                file.write(reinterpret_cast<const char*>(neuron_weights.data()), 
                          weight_count * sizeof(f32));
            }
            
            // Save biases
            usize bias_count = layer.biases.size();
            file.write(reinterpret_cast<const char*>(&bias_count), sizeof(bias_count));
            file.write(reinterpret_cast<const char*>(layer.biases.data()), 
                      bias_count * sizeof(f32));
        }
        
        LOG_INFO("Neural network model saved to {}", filepath);
        return true;
        
    } catch (const std::exception& e) {
        LOG_WARN("Error saving model: {}", e.what());
        return false;
    }
}

bool SimpleNeuralNetwork::load_model(const std::string& filepath) {
    std::ifstream file(filepath, std::ios::binary);
    if (!file.is_open()) {
        LOG_WARN("Failed to open file for loading: {}", filepath);
        return false;
    }
    
    try {
        // Load model metadata
        usize layer_count;
        file.read(reinterpret_cast<char*>(&layer_count), sizeof(layer_count));
        
        // Load layer sizes
        layer_sizes_.clear();
        layer_sizes_.resize(layer_count + 1);
        for (usize& size : layer_sizes_) {
            file.read(reinterpret_cast<char*>(&size), sizeof(size));
        }
        
        // Recreate layers
        layers_.clear();
        for (usize i = 0; i < layer_count; ++i) {
            layers_.emplace_back(layer_sizes_[i], layer_sizes_[i + 1]);
        }
        
        // Load layer weights and biases
        for (auto& layer : layers_) {
            // Load weights
            for (auto& neuron_weights : layer.weights) {
                usize weight_count;
                file.read(reinterpret_cast<char*>(&weight_count), sizeof(weight_count));
                neuron_weights.resize(weight_count);
                file.read(reinterpret_cast<char*>(neuron_weights.data()), 
                         weight_count * sizeof(f32));
            }
            
            // Load biases
            usize bias_count;
            file.read(reinterpret_cast<char*>(&bias_count), sizeof(bias_count));
            layer.biases.resize(bias_count);
            file.read(reinterpret_cast<char*>(layer.biases.data()), 
                     bias_count * sizeof(f32));
        }
        
        set_trained(true);
        LOG_INFO("Neural network model loaded from {}", filepath);
        return true;
        
    } catch (const std::exception& e) {
        LOG_WARN("Error loading model: {}", e.what());
        return false;
    }
}

std::vector<f32> SimpleNeuralNetwork::get_feature_importance() const {
    if (!is_trained_ || layers_.empty()) {
        return {};
    }
    
    // Calculate feature importance based on first layer weights
    const auto& first_layer = layers_.front();
    std::vector<f32> importance(config_.input_dimension, 0.0f);
    
    for (usize feature = 0; feature < config_.input_dimension; ++feature) {
        for (usize neuron = 0; neuron < first_layer.weights.size(); ++neuron) {
            importance[feature] += std::abs(first_layer.weights[neuron][feature]);
        }
        importance[feature] /= first_layer.weights.size(); // Average across neurons
    }
    
    // Normalize to sum to 1
    f32 total_importance = std::accumulate(importance.begin(), importance.end(), 0.0f);
    if (total_importance > 1e-8f) {
        for (auto& imp : importance) {
            imp /= total_importance;
        }
    }
    
    return importance;
}

// =============================================================================
// MLModelFactory Implementation
// =============================================================================

std::unique_ptr<MLModelBase> MLModelFactory::create_model(ModelType type, const MLModelConfig& config) {
    switch (type) {
        case ModelType::LinearRegression:
            return create_linear_regression(config);
        case ModelType::NeuralNetwork:
            return create_neural_network(config);
        case ModelType::DecisionTree:
            return create_decision_tree(config);
        default:
            LOG_WARN("Unknown model type requested");
            return nullptr;
    }
}

std::unique_ptr<MLModelBase> MLModelFactory::create_linear_regression(const MLModelConfig& config) {
    return std::make_unique<LinearRegressionModel>(config);
}

std::unique_ptr<MLModelBase> MLModelFactory::create_neural_network(const MLModelConfig& config,
                                                                   const std::vector<usize>& hidden_layers) {
    return std::make_unique<SimpleNeuralNetwork>(config, hidden_layers);
}

std::unique_ptr<MLModelBase> MLModelFactory::create_decision_tree(const MLModelConfig& config,
                                                                  usize max_depth,
                                                                  usize min_samples_split) {
    return std::make_unique<DecisionTreeModel>(config, max_depth, min_samples_split);
}

std::string MLModelFactory::model_type_to_string(ModelType type) {
    switch (type) {
        case ModelType::LinearRegression: return "LinearRegression";
        case ModelType::NeuralNetwork: return "NeuralNetwork";
        case ModelType::DecisionTree: return "DecisionTree";
        default: return "Unknown";
    }
}

MLModelFactory::ModelType MLModelFactory::model_type_from_string(const std::string& type_str) {
    if (type_str == "LinearRegression") return ModelType::LinearRegression;
    if (type_str == "NeuralNetwork") return ModelType::NeuralNetwork;
    if (type_str == "DecisionTree") return ModelType::DecisionTree;
    return ModelType::LinearRegression; // Default
}

// =============================================================================
// PredictionContext Implementation
// =============================================================================

FeatureVector PredictionContext::to_feature_vector() const {
    FeatureVector features;
    features.reserve(20); // Estimated feature count
    
    // Entity features
    features.push_back(static_cast<f32>(entity.index()));
    features.push_back(static_cast<f32>(entity_signature.count()));
    features.push_back(static_cast<f32>(entity_age));
    
    // System state features
    features.push_back(frame_time);
    features.push_back(system_load);
    features.push_back(static_cast<f32>(active_entities));
    features.push_back(static_cast<f32>(total_components));
    
    // Memory features
    features.push_back(memory_usage);
    features.push_back(memory_pressure);
    features.push_back(static_cast<f32>(memory_allocations));
    
    // Performance features
    features.push_back(fps);
    features.push_back(frame_variance);
    features.push_back(system_efficiency);
    
    // Historical data (recent frames)
    for (f32 frame_time : recent_frame_times) {
        features.push_back(frame_time);
    }
    
    return features;
}

PredictionContext PredictionContext::from_registry(const ecs::Registry& registry, EntityID entity) {
    PredictionContext context;
    context.entity = entity;
    context.active_entities = registry.active_entities();
    
    // Get entity signature if valid entity
    if (registry.is_valid(entity)) {
        // Note: This would need access to entity signature from registry
        // Implementation depends on registry interface
        context.entity_age = 1; // Placeholder
    }
    
    // Get registry memory usage
    context.memory_usage = static_cast<f32>(registry.memory_usage()) / (1024.0f * 1024.0f); // MB
    
    // Additional context would be filled based on available registry information
    return context;
}

std::string PredictionContext::to_string() const {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2);
    oss << "PredictionContext:\n";
    oss << "  Entity: " << entity << " (age: " << entity_age << ")\n";
    oss << "  Frame time: " << frame_time << "ms\n";
    oss << "  Active entities: " << active_entities << "\n";
    oss << "  Memory usage: " << memory_usage << "MB\n";
    oss << "  Memory pressure: " << (memory_pressure * 100.0f) << "%\n";
    oss << "  FPS: " << fps << "\n";
    oss << "  System efficiency: " << (system_efficiency * 100.0f) << "%\n";
    return oss.str();
}

void PredictionContext::print_summary() const {
    std::cout << to_string() << std::endl;
}

// =============================================================================
// FeatureExtractor Implementation
// =============================================================================

FeatureExtractor::FeatureExtractor(const ExtractionConfig& config) : config_(config) {
    initialize_feature_names();
}

void FeatureExtractor::initialize_feature_names() {
    feature_names_.clear();
    
    if (config_.include_entity_features) {
        feature_names_.push_back("entity_id");
        feature_names_.push_back("component_count");
        feature_names_.push_back("entity_age");
    }
    
    if (config_.include_system_features) {
        feature_names_.push_back("frame_time");
        feature_names_.push_back("system_load");
        feature_names_.push_back("active_entities");
        feature_names_.push_back("total_components");
    }
    
    if (config_.include_memory_features) {
        feature_names_.push_back("memory_usage");
        feature_names_.push_back("memory_pressure");
        feature_names_.push_back("memory_allocations");
    }
    
    if (config_.include_performance_features) {
        feature_names_.push_back("fps");
        feature_names_.push_back("frame_variance");
        feature_names_.push_back("system_efficiency");
    }
    
    if (config_.include_temporal_features) {
        for (usize i = 0; i < 10; ++i) {
            feature_names_.push_back("recent_frame_" + std::to_string(i));
        }
    }
    
    // Create feature indices map
    feature_indices_.clear();
    for (usize i = 0; i < feature_names_.size(); ++i) {
        feature_indices_[feature_names_[i]] = i;
    }
}

FeatureVector FeatureExtractor::extract_features(const PredictionContext& context) {
    auto features = context.to_feature_vector();
    
    if (config_.normalize_features) {
        normalize_features(features);
    }
    
    update_feature_statistics(features);
    return features;
}

FeatureVector FeatureExtractor::extract_features_from_registry(const ecs::Registry& registry, 
                                                              EntityID entity) {
    auto context = PredictionContext::from_registry(registry, entity);
    return extract_features(context);
}

void FeatureExtractor::normalize_features(FeatureVector& features) const {
    for (usize i = 0; i < features.size(); ++i) {
        std::string feature_name = get_feature_name(i);
        features[i] = normalize_value(features[i], feature_name);
    }
}

f32 FeatureExtractor::normalize_value(f32 value, const std::string& feature_name) const {
    // Simple min-max normalization to [0, 1] range
    auto mean_key = feature_name + "_mean";
    auto range_key = feature_name + "_range";
    
    auto mean_it = feature_stats_.find(mean_key);
    auto range_it = feature_stats_.find(range_key);
    
    if (mean_it != feature_stats_.end() && range_it != feature_stats_.end()) {
        f32 mean = mean_it->second;
        f32 range = range_it->second;
        if (range > 1e-8f) {
            return (value - mean + range / 2.0f) / range; // Center around 0.5
        }
    }
    
    return value; // Return original value if no normalization data
}

void FeatureExtractor::update_feature_statistics(const FeatureVector& features) {
    for (usize i = 0; i < features.size(); ++i) {
        std::string feature_name = get_feature_name(i);
        f32 value = features[i];
        
        // Update running statistics
        std::string mean_key = feature_name + "_mean";
        std::string min_key = feature_name + "_min";
        std::string max_key = feature_name + "_max";
        std::string count_key = feature_name + "_count";
        
        // Update count
        f32 count = feature_stats_[count_key]++;
        
        // Update mean
        f32 old_mean = feature_stats_[mean_key];
        f32 new_mean = old_mean + (value - old_mean) / (count + 1);
        feature_stats_[mean_key] = new_mean;
        
        // Update min/max
        feature_stats_[min_key] = std::min(feature_stats_[min_key], value);
        feature_stats_[max_key] = std::max(feature_stats_[max_key], value);
        
        // Update range
        feature_stats_[feature_name + "_range"] = 
            feature_stats_[max_key] - feature_stats_[min_key];
    }
}

std::string FeatureExtractor::get_feature_name(usize index) const {
    if (index < feature_names_.size()) {
        return feature_names_[index];
    }
    return "feature_" + std::to_string(index);
}

std::string FeatureExtractor::get_feature_description(usize index) const {
    std::string name = get_feature_name(index);
    
    // Provide descriptions for known features
    static const std::unordered_map<std::string, std::string> descriptions = {
        {"entity_id", "Unique identifier for the entity"},
        {"component_count", "Number of components attached to entity"},
        {"entity_age", "How long the entity has existed (frames)"},
        {"frame_time", "Current frame rendering time (ms)"},
        {"system_load", "Overall system computational load (0-1)"},
        {"active_entities", "Number of currently active entities"},
        {"total_components", "Total number of components in system"},
        {"memory_usage", "Current memory usage (MB)"},
        {"memory_pressure", "Memory pressure level (0-1)"},
        {"memory_allocations", "Number of memory allocations"},
        {"fps", "Frames per second"},
        {"frame_variance", "Variance in frame times"},
        {"system_efficiency", "Overall system efficiency (0-1)"}
    };
    
    auto it = descriptions.find(name);
    if (it != descriptions.end()) {
        return it->second;
    }
    
    return "Feature at index " + std::to_string(index);
}

void FeatureExtractor::print_feature_summary() const {
    std::cout << "Feature Extractor Summary:\n";
    std::cout << "  Total features: " << feature_names_.size() << "\n";
    std::cout << "  Entity features: " << (config_.include_entity_features ? "enabled" : "disabled") << "\n";
    std::cout << "  System features: " << (config_.include_system_features ? "enabled" : "disabled") << "\n";
    std::cout << "  Memory features: " << (config_.include_memory_features ? "enabled" : "disabled") << "\n";
    std::cout << "  Performance features: " << (config_.include_performance_features ? "enabled" : "disabled") << "\n";
    std::cout << "  Temporal features: " << (config_.include_temporal_features ? "enabled" : "disabled") << "\n";
    std::cout << "  Normalization: " << (config_.normalize_features ? "enabled" : "disabled") << "\n";
    
    if (!feature_names_.empty()) {
        std::cout << "\nFeatures:\n";
        for (usize i = 0; i < std::min(static_cast<usize>(10), feature_names_.size()); ++i) {
            std::cout << "  " << (i + 1) << ". " << feature_names_[i] << " - " 
                      << get_feature_description(i) << "\n";
        }
        if (feature_names_.size() > 10) {
            std::cout << "  ... and " << (feature_names_.size() - 10) << " more features\n";
        }
    }
}

std::string FeatureExtractor::visualize_feature_importance(const std::vector<f32>& importance) const {
    if (importance.size() != feature_names_.size()) {
        return "Feature importance vector size mismatch";
    }
    
    std::ostringstream oss;
    oss << "Feature Importance Visualization:\n";
    oss << std::string(50, '=') << "\n";
    
    // Find max importance for scaling
    f32 max_importance = *std::max_element(importance.begin(), importance.end());
    if (max_importance < 1e-8f) max_importance = 1.0f;
    
    // Create sorted indices by importance
    std::vector<usize> indices(importance.size());
    std::iota(indices.begin(), indices.end(), 0);
    std::sort(indices.begin(), indices.end(), [&](usize a, usize b) {
        return importance[a] > importance[b];
    });
    
    // Display top 10 most important features
    for (usize i = 0; i < std::min(static_cast<usize>(10), indices.size()); ++i) {
        usize idx = indices[i];
        f32 imp = importance[idx];
        std::string name = feature_names_[idx];
        
        // Create bar visualization
        usize bar_length = static_cast<usize>((imp / max_importance) * 30);
        std::string bar(bar_length, '█');
        bar += std::string(30 - bar_length, '░');
        
        oss << std::setw(3) << (i + 1) << ". " 
            << std::setw(20) << std::left << name.substr(0, 19)
            << " |" << bar << "| " 
            << std::fixed << std::setprecision(3) << imp << "\n";
    }
    
    return oss.str();
}

} // namespace ecscope::ml