#pragma once

#include "ml_prediction_system.hpp"
#include "ecs_behavior_predictor.hpp"
#include "entity.hpp"
#include "component.hpp"
#include "signature.hpp"
#include "registry.hpp"
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <memory>
#include <string>
#include <chrono>
#include <functional>
#include <future>
#include <queue>
#include <atomic>
#include <mutex>

namespace ecscope::ml {

/**
 * @brief Component need prediction for entities
 */
struct ComponentNeedPrediction {
    EntityID entity;
    std::string component_type_name;
    f32 probability{0.0f};           // Probability that entity will need this component
    f32 urgency{0.0f};              // How soon the component will be needed (0-1, 1 = immediately)
    f32 confidence{0.0f};           // Model confidence in this prediction
    Timestamp predicted_need_time;   // When the component will likely be needed
    f32 estimated_lifetime{0.0f};   // How long the component will likely be used
    
    // Reasoning for educational purposes
    std::string prediction_reason;   // Why this component is predicted to be needed
    std::vector<std::string> contributing_factors; // What factors led to this prediction
    
    // Validation
    bool is_valid() const { return probability > 0.1f && confidence > 0.1f; }
    bool is_high_priority() const { return probability > 0.7f && urgency > 0.7f; }
    bool is_immediate_need() const { return urgency > 0.9f; }
    
    // Educational features
    std::string to_string() const;
    void print_prediction_details() const;
};

/**
 * @brief Component allocation strategy based on predictions
 */
struct ComponentAllocationStrategy {
    enum class Strategy {
        Reactive,    // Allocate components only when requested
        Predictive,  // Pre-allocate based on predictions
        Hybrid,      // Mix of reactive and predictive
        Conservative, // Only allocate high-confidence predictions
        Aggressive   // Allocate even low-probability predictions
    } strategy{Strategy::Hybrid};
    
    f32 probability_threshold{0.6f};  // Minimum probability to pre-allocate
    f32 confidence_threshold{0.7f};   // Minimum confidence to pre-allocate
    f32 urgency_threshold{0.5f};      // Minimum urgency to pre-allocate
    usize max_preallocation_count{100}; // Maximum components to pre-allocate
    
    // Resource management
    f32 memory_usage_limit{0.8f};     // Don't pre-allocate if memory usage > 80%
    usize max_memory_per_component{1024}; // Maximum memory per component instance
    
    // Educational settings
    bool enable_allocation_logging{true};
    bool track_allocation_efficiency{true};
    bool enable_waste_analysis{true};
    
    std::string strategy_to_string() const;
    bool should_preallocate(const ComponentNeedPrediction& prediction) const;
};

/**
 * @brief Statistics for component prediction and allocation
 */
struct ComponentPredictionStats {
    // Prediction accuracy
    usize total_predictions{0};
    usize correct_predictions{0};
    usize false_positives{0};
    usize false_negatives{0};
    f32 precision{0.0f};
    f32 recall{0.0f};
    f32 f1_score{0.0f};
    
    // Allocation efficiency
    usize components_preallocated{0};
    usize preallocations_used{0};
    usize preallocations_wasted{0};
    f32 allocation_efficiency{0.0f};
    f32 memory_savings{0.0f};
    f32 time_savings{0.0f};
    
    // Component type statistics
    std::unordered_map<std::string, usize> predictions_per_type;
    std::unordered_map<std::string, f32> accuracy_per_type;
    std::unordered_map<std::string, f32> usage_per_type;
    
    void reset();
    void update_prediction_accuracy(const ComponentNeedPrediction& prediction, bool was_correct);
    void update_allocation_efficiency(const std::string& component_type, bool was_used);
    std::string to_string() const;
};

/**
 * @brief Pre-allocated component pool for specific component type
 */
template<typename ComponentT>
requires ecs::Component<ComponentT>
class PredictiveComponentPool {
private:
    std::queue<std::unique_ptr<ComponentT>> available_components_;
    std::unordered_map<EntityID, std::unique_ptr<ComponentT>> allocated_components_;
    std::string component_name_;
    usize max_pool_size_;
    usize preallocation_count_;
    
    // Statistics
    std::atomic<usize> total_allocations_{0};
    std::atomic<usize> pool_hits_{0};
    std::atomic<usize> pool_misses_{0};
    std::atomic<usize> waste_count_{0};
    
    mutable std::mutex pool_mutex_;
    
public:
    explicit PredictiveComponentPool(const std::string& name, 
                                   usize max_size = 1000, 
                                   usize prealloc_count = 100)
        : component_name_(name), max_pool_size_(max_size), preallocation_count_(prealloc_count) {
        preallocate_components();
    }
    
    ~PredictiveComponentPool() {
        clear_pool();
    }
    
    // Component allocation
    std::unique_ptr<ComponentT> allocate_component(EntityID entity) {
        std::lock_guard<std::mutex> lock(pool_mutex_);
        ++total_allocations_;
        
        if (!available_components_.empty()) {
            auto component = std::move(available_components_.front());
            available_components_.pop();
            allocated_components_[entity] = std::move(component);
            ++pool_hits_;
            return std::make_unique<ComponentT>(*allocated_components_[entity]);
        } else {
            ++pool_misses_;
            auto component = std::make_unique<ComponentT>();
            allocated_components_[entity] = std::make_unique<ComponentT>(*component);
            return component;
        }
    }
    
    // Component deallocation (return to pool)
    void deallocate_component(EntityID entity) {
        std::lock_guard<std::mutex> lock(pool_mutex_);
        auto it = allocated_components_.find(entity);
        if (it != allocated_components_.end()) {
            if (available_components_.size() < max_pool_size_) {
                available_components_.push(std::move(it->second));
            } else {
                ++waste_count_;
            }
            allocated_components_.erase(it);
        }
    }
    
    // Preallocation based on predictions
    void preallocate_for_predictions(const std::vector<ComponentNeedPrediction>& predictions) {
        std::lock_guard<std::mutex> lock(pool_mutex_);
        
        usize needed_components = 0;
        for (const auto& prediction : predictions) {
            if (prediction.component_type_name == component_name_ && prediction.is_high_priority()) {
                ++needed_components;
            }
        }
        
        // Ensure we have enough preallocated components
        while (available_components_.size() < needed_components && 
               available_components_.size() < max_pool_size_) {
            available_components_.push(std::make_unique<ComponentT>());
        }
    }
    
    // Pool management
    void resize_pool(usize new_max_size) {
        std::lock_guard<std::mutex> lock(pool_mutex_);
        max_pool_size_ = new_max_size;
        
        // Shrink pool if necessary
        while (available_components_.size() > max_pool_size_) {
            available_components_.pop();
            ++waste_count_;
        }
    }
    
    void clear_pool() {
        std::lock_guard<std::mutex> lock(pool_mutex_);
        while (!available_components_.empty()) {
            available_components_.pop();
        }
        allocated_components_.clear();
    }
    
    // Statistics
    f32 hit_rate() const {
        usize total = total_allocations_.load();
        return total > 0 ? static_cast<f32>(pool_hits_.load()) / total : 0.0f;
    }
    
    usize pool_size() const {
        std::lock_guard<std::mutex> lock(pool_mutex_);
        return available_components_.size();
    }
    
    usize allocated_count() const {
        std::lock_guard<std::mutex> lock(pool_mutex_);
        return allocated_components_.size();
    }
    
    usize total_allocations() const { return total_allocations_.load(); }
    usize waste_count() const { return waste_count_.load(); }
    
    std::string get_statistics() const {
        return fmt::format("Pool '{}': Hit Rate: {:.2f}%, Size: {}/{}, Allocated: {}, Waste: {}",
                          component_name_, hit_rate() * 100.0f, pool_size(), max_pool_size_,
                          allocated_count(), waste_count());
    }
    
private:
    void preallocate_components() {
        for (usize i = 0; i < preallocation_count_ && i < max_pool_size_; ++i) {
            available_components_.push(std::make_unique<ComponentT>());
        }
    }
};

/**
 * @brief Configuration for predictive component system
 */
struct PredictiveComponentConfig {
    ComponentAllocationStrategy allocation_strategy;
    
    // Prediction settings
    f32 prediction_horizon{5.0f};          // How far ahead to predict (seconds)
    usize max_predictions_per_entity{10};  // Maximum component predictions per entity
    f32 min_prediction_confidence{0.5f};   // Minimum confidence for making predictions
    
    // Model configuration
    MLModelConfig component_model_config{
        .model_name = "ComponentPredictor",
        .input_dimension = 25,  // Will be adjusted based on features
        .output_dimension = 1,  // Probability of needing component
        .learning_rate = 0.005f,
        .max_epochs = 1000,
        .enable_training_visualization = true
    };
    
    // Pool management
    bool enable_component_pooling{true};
    usize default_pool_size{500};
    usize max_pools{50};                    // Maximum number of component pools
    f32 pool_shrink_threshold{0.1f};        // Shrink pool if hit rate < 10%
    f32 pool_grow_threshold{0.9f};          // Grow pool if hit rate > 90%
    
    // Performance settings
    bool enable_async_prediction{true};
    usize max_concurrent_predictions{20};
    std::chrono::milliseconds prediction_cache_ttl{2000};
    
    // Educational features
    bool enable_prediction_logging{true};
    bool track_component_lifecycle{true};
    bool enable_efficiency_analysis{true};
};

/**
 * @brief Main predictive component system
 * 
 * This system uses machine learning to predict which components entities will need
 * in the future, allowing for pre-allocation and optimization of component management.
 * It integrates with the ECS behavior predictor and provides educational insights.
 */
class PredictiveComponentSystem {
private:
    PredictiveComponentConfig config_;
    std::unique_ptr<MLModelBase> component_model_;
    std::unique_ptr<FeatureExtractor> feature_extractor_;
    std::unique_ptr<ECSBehaviorPredictor> behavior_predictor_;
    
    // Component pools (type-erased)
    std::unordered_map<std::string, std::unique_ptr<void>> component_pools_;
    std::unordered_map<std::string, std::function<void*(EntityID)>> pool_allocators_;
    std::unordered_map<std::string, std::function<void(EntityID, void*)>> pool_deallocators_;
    
    // Prediction cache
    std::unordered_map<EntityID, std::vector<ComponentNeedPrediction>> prediction_cache_;
    std::unordered_map<EntityID, Timestamp> cache_timestamps_;
    mutable std::mutex cache_mutex_;
    
    // Training data
    TrainingDataset component_dataset_;
    std::unordered_map<std::string, TrainingDataset> type_specific_datasets_;
    
    // Statistics and metrics
    ComponentPredictionStats prediction_stats_;
    std::vector<f32> prediction_accuracy_history_;
    std::unordered_map<std::string, f32> component_type_popularity_;
    
    // Background processing
    std::unique_ptr<std::thread> prediction_thread_;
    std::unique_ptr<std::thread> training_thread_;
    std::atomic<bool> should_stop_threads_{false};
    std::queue<EntityID> prediction_request_queue_;
    mutable std::mutex queue_mutex_;
    
    // Performance tracking
    std::atomic<usize> total_predictions_made_{0};
    std::atomic<usize> successful_predictions_{0};
    std::atomic<usize> components_preallocated_{0};
    std::atomic<usize> preallocations_used_{0};
    
public:
    explicit PredictiveComponentSystem(const PredictiveComponentConfig& config = PredictiveComponentConfig{});
    ~PredictiveComponentSystem();
    
    // Non-copyable but movable
    PredictiveComponentSystem(const PredictiveComponentSystem&) = delete;
    PredictiveComponentSystem& operator=(const PredictiveComponentSystem&) = delete;
    PredictiveComponentSystem(PredictiveComponentSystem&&) = default;
    PredictiveComponentSystem& operator=(PredictiveComponentSystem&&) = default;
    
    // Component type registration
    template<typename ComponentT>
    requires ecs::Component<ComponentT>
    void register_component_type(const std::string& type_name, usize pool_size = 0) {
        if (pool_size == 0) {
            pool_size = config_.default_pool_size;
        }
        
        auto pool = std::make_unique<PredictiveComponentPool<ComponentT>>(type_name, pool_size);
        
        // Store type-erased pool
        component_pools_[type_name] = std::move(pool);
        
        // Store type-erased allocator/deallocator functions
        pool_allocators_[type_name] = [this, type_name](EntityID entity) -> void* {
            auto pool_ptr = static_cast<PredictiveComponentPool<ComponentT>*>(
                component_pools_[type_name].get());
            return pool_ptr->allocate_component(entity).release();
        };
        
        pool_deallocators_[type_name] = [this, type_name](EntityID entity, void* component) {
            auto pool_ptr = static_cast<PredictiveComponentPool<ComponentT>*>(
                component_pools_[type_name].get());
            pool_ptr->deallocate_component(entity);
            delete static_cast<ComponentT*>(component);
        };
        
        // Create type-specific dataset
        type_specific_datasets_[type_name] = TrainingDataset(type_name + "_Dataset");
        
        LOG_INFO("Registered component type '{}' with pool size {}", type_name, pool_size);
    }
    
    // Prediction methods
    std::vector<ComponentNeedPrediction> predict_component_needs(EntityID entity, 
                                                                const ecs::Registry& registry);
    std::vector<ComponentNeedPrediction> predict_all_component_needs(const ecs::Registry& registry);
    ComponentNeedPrediction predict_specific_component_need(EntityID entity,
                                                           const std::string& component_type,
                                                           const ecs::Registry& registry);
    
    // Async prediction
    std::future<std::vector<ComponentNeedPrediction>> predict_component_needs_async(
        EntityID entity, const ecs::Registry& registry);
    void request_prediction_for_entity(EntityID entity);
    
    // Component allocation based on predictions
    template<typename ComponentT>
    requires ecs::Component<ComponentT>
    std::unique_ptr<ComponentT> allocate_predicted_component(EntityID entity, 
                                                            const std::string& type_name) {
        if (auto it = pool_allocators_.find(type_name); it != pool_allocators_.end()) {
            ++components_preallocated_;
            void* component_ptr = it->second(entity);
            return std::unique_ptr<ComponentT>(static_cast<ComponentT*>(component_ptr));
        }
        
        // Fallback to regular allocation
        return std::make_unique<ComponentT>();
    }
    
    // Component deallocation
    template<typename ComponentT>
    requires ecs::Component<ComponentT>
    void deallocate_component(EntityID entity, const std::string& type_name, 
                             std::unique_ptr<ComponentT> component) {
        if (auto it = pool_deallocators_.find(type_name); it != pool_deallocators_.end()) {
            it->second(entity, component.release());
        }
        // If no pool, component will be automatically deleted by unique_ptr
    }
    
    // Model training and management
    bool train_component_models();
    bool train_type_specific_model(const std::string& component_type);
    void collect_training_data(const ecs::Registry& registry);
    void observe_component_usage(EntityID entity, const std::string& component_type,
                                bool was_needed, const ecs::Registry& registry);
    
    // Pool management
    void optimize_pool_sizes();
    void shrink_unused_pools();
    void preload_predicted_components(const std::vector<ComponentNeedPrediction>& predictions);
    
    // Statistics and analysis
    const ComponentPredictionStats& get_prediction_statistics() const { return prediction_stats_; }
    f32 get_overall_prediction_accuracy() const;
    f32 get_allocation_efficiency() const;
    std::unordered_map<std::string, f32> get_component_type_accuracies() const;
    
    // Configuration
    const PredictiveComponentConfig& config() const { return config_; }
    void update_config(const PredictiveComponentConfig& new_config);
    
    // Educational features
    std::string generate_prediction_report() const;
    std::string generate_efficiency_report() const;
    void print_component_usage_analysis() const;
    std::string visualize_component_predictions(EntityID entity) const;
    std::string explain_component_prediction(const ComponentNeedPrediction& prediction) const;
    
    // Integration with behavior predictor
    void set_behavior_predictor(std::unique_ptr<ECSBehaviorPredictor> predictor);
    const ECSBehaviorPredictor* get_behavior_predictor() const { return behavior_predictor_.get(); }
    
    // Callback for prediction events
    using ComponentPredictionCallback = std::function<void(const ComponentNeedPrediction&)>;
    void set_prediction_callback(ComponentPredictionCallback callback) { 
        prediction_callback_ = callback; 
    }
    
    // Validation and testing
    void validate_predictions_against_reality(const ecs::Registry& registry);
    f32 test_prediction_accuracy(const std::vector<EntityID>& test_entities,
                                const ecs::Registry& registry);
    
private:
    // Internal prediction implementation
    ComponentNeedPrediction make_component_prediction_internal(EntityID entity,
                                                              const std::string& component_type,
                                                              const PredictionContext& context);
    
    // Feature extraction for component prediction
    FeatureVector extract_component_features(EntityID entity,
                                            const std::string& component_type,
                                            const ecs::Registry& registry);
    
    // Training data creation
    TrainingSample create_component_training_sample(EntityID entity,
                                                   const std::string& component_type,
                                                   bool was_needed,
                                                   const PredictionContext& context);
    
    // Model management
    void initialize_models();
    void train_models_if_needed();
    
    // Background thread functions
    void prediction_thread_function();
    void training_thread_function();
    void start_background_threads();
    void stop_background_threads();
    
    // Cache management
    void update_prediction_cache(EntityID entity, const std::vector<ComponentNeedPrediction>& predictions);
    std::optional<std::vector<ComponentNeedPrediction>> get_cached_predictions(EntityID entity) const;
    void clear_expired_cache_entries();
    
    // Statistics updates
    void update_prediction_statistics(const ComponentNeedPrediction& prediction, bool was_correct);
    void update_component_type_popularity(const std::string& component_type);
    
    // Callback
    ComponentPredictionCallback prediction_callback_;
};

/**
 * @brief Utility functions for component prediction
 */
namespace component_prediction_utils {

// Component analysis
std::vector<std::string> analyze_component_dependencies(const std::string& component_type,
                                                       const ecs::Registry& registry);
f32 calculate_component_usage_frequency(const std::string& component_type,
                                       const ecs::Registry& registry);
f32 calculate_component_lifetime_average(const std::string& component_type,
                                        const std::vector<EntityID>& entities);

// Prediction validation
bool validate_component_prediction(const ComponentNeedPrediction& prediction,
                                  EntityID entity,
                                  const ecs::Registry& registry,
                                  f32 time_elapsed);

// Educational utilities
std::string create_component_usage_timeline(EntityID entity,
                                           const std::vector<std::string>& component_types);
std::string explain_component_relationships(const std::string& component_type);
std::string visualize_component_prediction_accuracy(const ComponentPredictionStats& stats);

} // namespace component_prediction_utils

} // namespace ecscope::ml