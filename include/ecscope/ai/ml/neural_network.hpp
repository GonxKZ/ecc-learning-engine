#pragma once

#include "../core/ai_types.hpp"
#include <vector>
#include <memory>
#include <random>
#include <functional>
#include <fstream>
#include <cmath>
#include <algorithm>

namespace ecscope::ai {

/**
 * @brief Neural Network Framework - Production-grade ML for AI
 * 
 * Provides a complete neural network implementation with multiple
 * activation functions, loss functions, optimizers, and training
 * algorithms. Designed for both inference and training in game AI.
 */

// Forward declarations
class Layer;
class DenseLayer;
class NeuralNetwork;

/**
 * @brief Matrix operations for neural networks
 */
class Matrix {
public:
    Matrix() : rows_(0), cols_(0) {}
    Matrix(usize rows, usize cols) : rows_(rows), cols_(cols), data_(rows * cols, 0.0f) {}
    Matrix(usize rows, usize cols, f32 initial_value) 
        : rows_(rows), cols_(cols), data_(rows * cols, initial_value) {}
    
    // Access operations
    f32& operator()(usize row, usize col) {
        return data_[row * cols_ + col];
    }
    
    const f32& operator()(usize row, usize col) const {
        return data_[row * cols_ + col];
    }
    
    // Basic operations
    Matrix operator+(const Matrix& other) const {
        Matrix result(rows_, cols_);
        for (usize i = 0; i < data_.size(); ++i) {
            result.data_[i] = data_[i] + other.data_[i];
        }
        return result;
    }
    
    Matrix operator-(const Matrix& other) const {
        Matrix result(rows_, cols_);
        for (usize i = 0; i < data_.size(); ++i) {
            result.data_[i] = data_[i] - other.data_[i];
        }
        return result;
    }
    
    Matrix operator*(const Matrix& other) const {
        Matrix result(rows_, other.cols_);
        for (usize i = 0; i < rows_; ++i) {
            for (usize j = 0; j < other.cols_; ++j) {
                f32 sum = 0.0f;
                for (usize k = 0; k < cols_; ++k) {
                    sum += (*this)(i, k) * other(k, j);
                }
                result(i, j) = sum;
            }
        }
        return result;
    }
    
    Matrix operator*(f32 scalar) const {
        Matrix result(rows_, cols_);
        for (usize i = 0; i < data_.size(); ++i) {
            result.data_[i] = data_[i] * scalar;
        }
        return result;
    }
    
    // In-place operations
    Matrix& operator+=(const Matrix& other) {
        for (usize i = 0; i < data_.size(); ++i) {
            data_[i] += other.data_[i];
        }
        return *this;
    }
    
    Matrix& operator-=(const Matrix& other) {
        for (usize i = 0; i < data_.size(); ++i) {
            data_[i] -= other.data_[i];
        }
        return *this;
    }
    
    Matrix& operator*=(f32 scalar) {
        for (f32& val : data_) {
            val *= scalar;
        }
        return *this;
    }
    
    // Transpose
    Matrix transpose() const {
        Matrix result(cols_, rows_);
        for (usize i = 0; i < rows_; ++i) {
            for (usize j = 0; j < cols_; ++j) {
                result(j, i) = (*this)(i, j);
            }
        }
        return result;
    }
    
    // Element-wise operations
    Matrix hadamard(const Matrix& other) const {
        Matrix result(rows_, cols_);
        for (usize i = 0; i < data_.size(); ++i) {
            result.data_[i] = data_[i] * other.data_[i];
        }
        return result;
    }
    
    void apply_function(std::function<f32(f32)> func) {
        for (f32& val : data_) {
            val = func(val);
        }
    }
    
    // Utilities
    void zero() {
        std::fill(data_.begin(), data_.end(), 0.0f);
    }
    
    void random_fill(f32 min_val = -1.0f, f32 max_val = 1.0f) {
        static thread_local std::random_device rd;
        static thread_local std::mt19937 gen(rd());
        std::uniform_real_distribution<f32> dis(min_val, max_val);
        
        for (f32& val : data_) {
            val = dis(gen);
        }
    }
    
    void xavier_init() {
        f32 limit = std::sqrt(6.0f / (rows_ + cols_));
        random_fill(-limit, limit);
    }
    
    void he_init() {
        f32 std_dev = std::sqrt(2.0f / rows_);
        static thread_local std::random_device rd;
        static thread_local std::mt19937 gen(rd());
        std::normal_distribution<f32> dis(0.0f, std_dev);
        
        for (f32& val : data_) {
            val = dis(gen);
        }
    }
    
    // Properties
    usize rows() const { return rows_; }
    usize cols() const { return cols_; }
    usize size() const { return data_.size(); }
    const std::vector<f32>& data() const { return data_; }
    std::vector<f32>& data() { return data_; }
    
    // Statistics
    f32 sum() const {
        f32 total = 0.0f;
        for (f32 val : data_) {
            total += val;
        }
        return total;
    }
    
    f32 mean() const {
        return data_.empty() ? 0.0f : sum() / data_.size();
    }
    
    f32 max() const {
        return data_.empty() ? 0.0f : *std::max_element(data_.begin(), data_.end());
    }
    
    f32 min() const {
        return data_.empty() ? 0.0f : *std::min_element(data_.begin(), data_.end());
    }
    
private:
    usize rows_, cols_;
    std::vector<f32> data_;
};

/**
 * @brief Activation Functions
 */
class ActivationFunctions {
public:
    static f32 sigmoid(f32 x) {
        return 1.0f / (1.0f + std::exp(-std::clamp(x, -500.0f, 500.0f)));
    }
    
    static f32 sigmoid_derivative(f32 x) {
        f32 s = sigmoid(x);
        return s * (1.0f - s);
    }
    
    static f32 tanh_func(f32 x) {
        return std::tanh(x);
    }
    
    static f32 tanh_derivative(f32 x) {
        f32 t = tanh_func(x);
        return 1.0f - t * t;
    }
    
    static f32 relu(f32 x) {
        return std::max(0.0f, x);
    }
    
    static f32 relu_derivative(f32 x) {
        return x > 0.0f ? 1.0f : 0.0f;
    }
    
    static f32 leaky_relu(f32 x, f32 alpha = 0.01f) {
        return x > 0.0f ? x : alpha * x;
    }
    
    static f32 leaky_relu_derivative(f32 x, f32 alpha = 0.01f) {
        return x > 0.0f ? 1.0f : alpha;
    }
    
    static f32 linear(f32 x) {
        return x;
    }
    
    static f32 linear_derivative(f32 x) {
        return 1.0f;
    }
    
    // Softmax (for output layer)
    static Matrix softmax(const Matrix& input) {
        Matrix result(input.rows(), input.cols());
        
        for (usize row = 0; row < input.rows(); ++row) {
            f32 max_val = input(row, 0);
            for (usize col = 1; col < input.cols(); ++col) {
                max_val = std::max(max_val, input(row, col));
            }
            
            f32 sum = 0.0f;
            for (usize col = 0; col < input.cols(); ++col) {
                result(row, col) = std::exp(input(row, col) - max_val);
                sum += result(row, col);
            }
            
            for (usize col = 0; col < input.cols(); ++col) {
                result(row, col) /= sum;
            }
        }
        
        return result;
    }
    
    static std::function<f32(f32)> get_activation(ActivationFunction type) {
        switch (type) {
            case ActivationFunction::SIGMOID: return sigmoid;
            case ActivationFunction::TANH: return tanh_func;
            case ActivationFunction::RELU: return relu;
            case ActivationFunction::LEAKY_RELU: return [](f32 x) { return leaky_relu(x); };
            case ActivationFunction::LINEAR: return linear;
            default: return linear;
        }
    }
    
    static std::function<f32(f32)> get_activation_derivative(ActivationFunction type) {
        switch (type) {
            case ActivationFunction::SIGMOID: return sigmoid_derivative;
            case ActivationFunction::TANH: return tanh_derivative;
            case ActivationFunction::RELU: return relu_derivative;
            case ActivationFunction::LEAKY_RELU: return [](f32 x) { return leaky_relu_derivative(x); };
            case ActivationFunction::LINEAR: return linear_derivative;
            default: return linear_derivative;
        }
    }
};

/**
 * @brief Loss Functions
 */
class LossFunctions {
public:
    // Mean Squared Error
    static f32 mse(const Matrix& predicted, const Matrix& actual) {
        f32 sum = 0.0f;
        usize count = 0;
        
        for (usize i = 0; i < predicted.rows(); ++i) {
            for (usize j = 0; j < predicted.cols(); ++j) {
                f32 diff = predicted(i, j) - actual(i, j);
                sum += diff * diff;
                count++;
            }
        }
        
        return count > 0 ? sum / count : 0.0f;
    }
    
    static Matrix mse_derivative(const Matrix& predicted, const Matrix& actual) {
        Matrix result(predicted.rows(), predicted.cols());
        f32 scale = 2.0f / (predicted.rows() * predicted.cols());
        
        for (usize i = 0; i < predicted.rows(); ++i) {
            for (usize j = 0; j < predicted.cols(); ++j) {
                result(i, j) = scale * (predicted(i, j) - actual(i, j));
            }
        }
        
        return result;
    }
    
    // Cross Entropy Loss
    static f32 cross_entropy(const Matrix& predicted, const Matrix& actual) {
        f32 loss = 0.0f;
        usize count = 0;
        
        for (usize i = 0; i < predicted.rows(); ++i) {
            for (usize j = 0; j < predicted.cols(); ++j) {
                f32 p = std::clamp(predicted(i, j), 1e-7f, 1.0f - 1e-7f);
                loss -= actual(i, j) * std::log(p);
                count++;
            }
        }
        
        return count > 0 ? loss / count : 0.0f;
    }
    
    static Matrix cross_entropy_derivative(const Matrix& predicted, const Matrix& actual) {
        Matrix result(predicted.rows(), predicted.cols());
        
        for (usize i = 0; i < predicted.rows(); ++i) {
            for (usize j = 0; j < predicted.cols(); ++j) {
                f32 p = std::clamp(predicted(i, j), 1e-7f, 1.0f - 1e-7f);
                result(i, j) = -actual(i, j) / p;
            }
        }
        
        return result;
    }
};

/**
 * @brief Dense (Fully Connected) Layer
 */
class DenseLayer {
public:
    DenseLayer(usize input_size, usize output_size, ActivationFunction activation)
        : input_size_(input_size)
        , output_size_(output_size)
        , activation_(activation)
        , weights_(output_size, input_size)
        , biases_(output_size, 1, 0.0f)
        , last_input_(1, 1)
        , last_output_(1, 1)
        , last_weighted_sum_(1, 1) {
        
        // Initialize weights using He initialization for ReLU, Xavier for others
        if (activation == ActivationFunction::RELU || activation == ActivationFunction::LEAKY_RELU) {
            weights_.he_init();
        } else {
            weights_.xavier_init();
        }
        
        // Initialize biases to small positive values
        biases_.random_fill(0.0f, 0.1f);
    }
    
    Matrix forward(const Matrix& input) {
        last_input_ = input;
        
        // Weighted sum: W * x + b
        last_weighted_sum_ = weights_ * input + biases_;
        
        // Apply activation function
        last_output_ = last_weighted_sum_;
        
        auto activation_func = ActivationFunctions::get_activation(activation_);
        last_output_.apply_function(activation_func);
        
        return last_output_;
    }
    
    Matrix backward(const Matrix& output_gradient, f32 learning_rate) {
        // Compute gradient with respect to activation
        Matrix activation_gradient = last_weighted_sum_;
        auto activation_derivative = ActivationFunctions::get_activation_derivative(activation_);
        activation_gradient.apply_function(activation_derivative);
        
        // Combine with output gradient
        Matrix delta = output_gradient.hadamard(activation_gradient);
        
        // Compute weight gradients
        Matrix weight_gradient = delta * last_input_.transpose();
        
        // Compute input gradient (for previous layer)
        Matrix input_gradient = weights_.transpose() * delta;
        
        // Update weights and biases
        weights_ -= weight_gradient * learning_rate;
        biases_ -= delta * learning_rate;
        
        return input_gradient;
    }
    
    // Getters
    usize input_size() const { return input_size_; }
    usize output_size() const { return output_size_; }
    const Matrix& weights() const { return weights_; }
    const Matrix& biases() const { return biases_; }
    
    // Parameter count
    usize parameter_count() const {
        return weights_.size() + biases_.size();
    }
    
private:
    usize input_size_;
    usize output_size_;
    ActivationFunction activation_;
    
    Matrix weights_;
    Matrix biases_;
    
    // Cached values for backpropagation
    Matrix last_input_;
    Matrix last_output_;
    Matrix last_weighted_sum_;
};

/**
 * @brief Neural Network
 */
class NeuralNetwork {
public:
    explicit NeuralNetwork(const NeuralNetworkConfig& config = {})
        : config_(config), is_trained_(false), training_epochs_(0) {
        
        // Build layers from configuration
        if (!config.layer_sizes.empty()) {
            build_from_config();
        }
    }
    
    // Network construction
    void add_dense_layer(usize input_size, usize output_size, ActivationFunction activation) {
        layers_.emplace_back(std::make_unique<DenseLayer>(input_size, output_size, activation));
    }
    
    void build_from_config() {
        layers_.clear();
        
        if (config_.layer_sizes.size() < 2) {
            return; // Need at least input and output layer
        }
        
        for (usize i = 0; i < config_.layer_sizes.size() - 1; ++i) {
            ActivationFunction activation = ActivationFunction::RELU; // Default
            if (i < config_.activations.size()) {
                activation = config_.activations[i];
            }
            
            add_dense_layer(config_.layer_sizes[i], config_.layer_sizes[i + 1], activation);
        }
    }
    
    // Forward pass
    std::vector<f32> predict(const std::vector<f32>& input) {
        if (layers_.empty() || input.size() != layers_[0]->input_size()) {
            return {};
        }
        
        // Convert input to matrix
        Matrix current_input(input.size(), 1);
        for (usize i = 0; i < input.size(); ++i) {
            current_input(i, 0) = input[i];
        }
        
        // Forward pass through all layers
        for (auto& layer : layers_) {
            current_input = layer->forward(current_input);
        }
        
        // Convert output back to vector
        std::vector<f32> output(current_input.rows());
        for (usize i = 0; i < current_input.rows(); ++i) {
            output[i] = current_input(i, 0);
        }
        
        return output;
    }
    
    // Batch prediction
    std::vector<std::vector<f32>> predict_batch(const std::vector<std::vector<f32>>& inputs) {
        std::vector<std::vector<f32>> outputs;
        outputs.reserve(inputs.size());
        
        for (const auto& input : inputs) {
            outputs.push_back(predict(input));
        }
        
        return outputs;
    }
    
    // Training
    struct TrainingResult {
        bool success = false;
        u32 epochs_trained = 0;
        f32 final_loss = 0.0f;
        f32 best_loss = std::numeric_limits<f32>::max();
        f32 final_accuracy = 0.0f;
        f64 training_time_seconds = 0.0;
    };
    
    TrainingResult train(const TrainingData& data) {
        TrainingResult result;
        auto start_time = std::chrono::high_resolution_clock::now();
        
        if (layers_.empty() || data.inputs.empty() || data.inputs.size() != data.targets.size()) {
            return result;
        }
        
        // Split data into training and validation
        usize validation_size = static_cast<usize>(data.inputs.size() * data.validation_split);
        usize training_size = data.inputs.size() - validation_size;
        
        std::vector<usize> indices(data.inputs.size());
        std::iota(indices.begin(), indices.end(), 0);
        std::shuffle(indices.begin(), indices.end(), std::mt19937{std::random_device{}()});
        
        // Training loop
        u32 epochs_without_improvement = 0;
        f32 best_validation_loss = std::numeric_limits<f32>::max();
        
        for (u32 epoch = 0; epoch < data.max_epochs; ++epoch) {
            f32 total_loss = 0.0f;
            u32 batch_count = 0;
            
            // Train on batches
            for (usize batch_start = 0; batch_start < training_size; batch_start += data.batch_size) {
                usize batch_end = std::min(batch_start + data.batch_size, training_size);
                f32 batch_loss = 0.0f;
                
                // Process batch
                for (usize i = batch_start; i < batch_end; ++i) {
                    usize idx = indices[i];
                    batch_loss += train_single_sample(data.inputs[idx], data.targets[idx]);
                }
                
                batch_loss /= (batch_end - batch_start);
                total_loss += batch_loss;
                batch_count++;
            }
            
            total_loss /= batch_count;
            
            // Validation
            f32 validation_loss = 0.0f;
            if (validation_size > 0) {
                for (usize i = training_size; i < data.inputs.size(); ++i) {
                    usize idx = indices[i];
                    auto predicted = predict(data.inputs[idx]);
                    validation_loss += calculate_loss(predicted, data.targets[idx]);
                }
                validation_loss /= validation_size;
                
                // Early stopping check
                if (validation_loss < best_validation_loss) {
                    best_validation_loss = validation_loss;
                    epochs_without_improvement = 0;
                } else {
                    epochs_without_improvement++;
                    if (epochs_without_improvement >= data.early_stopping_patience) {
                        break; // Early stopping
                    }
                }
            }
            
            result.epochs_trained = epoch + 1;
            result.final_loss = total_loss;
            if (total_loss < result.best_loss) {
                result.best_loss = total_loss;
            }
        }
        
        // Calculate final accuracy
        result.final_accuracy = calculate_accuracy(data);
        
        auto end_time = std::chrono::high_resolution_clock::now();
        result.training_time_seconds = std::chrono::duration<f64>(end_time - start_time).count();
        
        is_trained_ = true;
        training_epochs_ = result.epochs_trained;
        result.success = true;
        
        return result;
    }
    
    // Network information
    usize parameter_count() const {
        usize total = 0;
        for (const auto& layer : layers_) {
            total += layer->parameter_count();
        }
        return total;
    }
    
    usize layer_count() const {
        return layers_.size();
    }
    
    std::vector<usize> get_layer_sizes() const {
        std::vector<usize> sizes;
        if (!layers_.empty()) {
            sizes.push_back(layers_[0]->input_size());
            for (const auto& layer : layers_) {
                sizes.push_back(layer->output_size());
            }
        }
        return sizes;
    }
    
    // Serialization
    bool save_to_file(const std::string& filepath) const {
        std::ofstream file(filepath, std::ios::binary);
        if (!file.is_open()) {
            return false;
        }
        
        // Save network structure
        usize layer_count = layers_.size();
        file.write(reinterpret_cast<const char*>(&layer_count), sizeof(layer_count));
        
        for (const auto& layer : layers_) {
            // Save layer dimensions
            usize input_size = layer->input_size();
            usize output_size = layer->output_size();
            file.write(reinterpret_cast<const char*>(&input_size), sizeof(input_size));
            file.write(reinterpret_cast<const char*>(&output_size), sizeof(output_size));
            
            // Save weights
            const auto& weights = layer->weights();
            file.write(reinterpret_cast<const char*>(weights.data().data()), 
                      weights.size() * sizeof(f32));
            
            // Save biases
            const auto& biases = layer->biases();
            file.write(reinterpret_cast<const char*>(biases.data().data()),
                      biases.size() * sizeof(f32));
        }
        
        return true;
    }
    
    bool load_from_file(const std::string& filepath) {
        std::ifstream file(filepath, std::ios::binary);
        if (!file.is_open()) {
            return false;
        }
        
        // Load network structure
        usize layer_count;
        file.read(reinterpret_cast<char*>(&layer_count), sizeof(layer_count));
        
        layers_.clear();
        layers_.reserve(layer_count);
        
        for (usize i = 0; i < layer_count; ++i) {
            usize input_size, output_size;
            file.read(reinterpret_cast<char*>(&input_size), sizeof(input_size));
            file.read(reinterpret_cast<char*>(&output_size), sizeof(output_size));
            
            // Create layer (using default activation for loading)
            auto layer = std::make_unique<DenseLayer>(input_size, output_size, ActivationFunction::RELU);
            
            // Load weights
            auto& weights = const_cast<Matrix&>(layer->weights());
            file.read(reinterpret_cast<char*>(weights.data().data()),
                     weights.size() * sizeof(f32));
            
            // Load biases
            auto& biases = const_cast<Matrix&>(layer->biases());
            file.read(reinterpret_cast<char*>(biases.data().data()),
                     biases.size() * sizeof(f32));
            
            layers_.push_back(std::move(layer));
        }
        
        return true;
    }
    
    // Status
    bool is_trained() const { return is_trained_; }
    u32 training_epochs() const { return training_epochs_; }
    
private:
    NeuralNetworkConfig config_;
    std::vector<std::unique_ptr<DenseLayer>> layers_;
    bool is_trained_;
    u32 training_epochs_;
    
    f32 train_single_sample(const std::vector<f32>& input, const std::vector<f32>& target) {
        // Forward pass
        auto predicted = predict(input);
        
        // Calculate loss
        f32 loss = calculate_loss(predicted, target);
        
        // Backward pass
        Matrix target_matrix(target.size(), 1);
        Matrix predicted_matrix(predicted.size(), 1);
        for (usize i = 0; i < target.size(); ++i) {
            target_matrix(i, 0) = target[i];
            predicted_matrix(i, 0) = predicted[i];
        }
        
        // Compute output gradient based on loss function
        Matrix output_gradient;
        switch (config_.loss_function) {
            case LossFunction::MSE:
                output_gradient = LossFunctions::mse_derivative(predicted_matrix, target_matrix);
                break;
            case LossFunction::CROSS_ENTROPY:
                output_gradient = LossFunctions::cross_entropy_derivative(predicted_matrix, target_matrix);
                break;
            default:
                output_gradient = LossFunctions::mse_derivative(predicted_matrix, target_matrix);
                break;
        }
        
        // Backpropagate through layers
        for (auto it = layers_.rbegin(); it != layers_.rend(); ++it) {
            output_gradient = (*it)->backward(output_gradient, config_.learning_rate);
        }
        
        return loss;
    }
    
    f32 calculate_loss(const std::vector<f32>& predicted, const std::vector<f32>& target) const {
        if (predicted.size() != target.size()) {
            return std::numeric_limits<f32>::max();
        }
        
        Matrix predicted_matrix(predicted.size(), 1);
        Matrix target_matrix(target.size(), 1);
        for (usize i = 0; i < predicted.size(); ++i) {
            predicted_matrix(i, 0) = predicted[i];
            target_matrix(i, 0) = target[i];
        }
        
        switch (config_.loss_function) {
            case LossFunction::MSE:
                return LossFunctions::mse(predicted_matrix, target_matrix);
            case LossFunction::CROSS_ENTROPY:
                return LossFunctions::cross_entropy(predicted_matrix, target_matrix);
            default:
                return LossFunctions::mse(predicted_matrix, target_matrix);
        }
    }
    
    f32 calculate_accuracy(const TrainingData& data) const {
        if (data.inputs.empty()) {
            return 0.0f;
        }
        
        u32 correct = 0;
        for (usize i = 0; i < data.inputs.size(); ++i) {
            auto predicted = predict(data.inputs[i]);
            
            // For classification, find max element
            usize predicted_class = 0;
            usize actual_class = 0;
            
            for (usize j = 1; j < predicted.size(); ++j) {
                if (predicted[j] > predicted[predicted_class]) {
                    predicted_class = j;
                }
                if (data.targets[i][j] > data.targets[i][actual_class]) {
                    actual_class = j;
                }
            }
            
            if (predicted_class == actual_class) {
                correct++;
            }
        }
        
        return static_cast<f32>(correct) / data.inputs.size();
    }
};

} // namespace ecscope::ai