#pragma once

#include "ml_prediction_system.hpp"
#include "entity.hpp"
#include "component.hpp"
#include "signature.hpp"
#include "registry.hpp"
#include <unordered_map>
#include <queue>
#include <memory>
#include <chrono>
#include <functional>
#include <future>
#include <mutex>

namespace ecscope::ml {

/**
 * @brief Entity behavior pattern for tracking and prediction
 */
struct EntityBehaviorPattern {
    EntityID entity;
    std::vector<ecs::ComponentSignature> signature_history; // Component changes over time
    std::vector<Timestamp> signature_timestamps;
    std::vector<f32> activity_levels;     // How active the entity has been
    std::vector<f32> interaction_counts;  // Interactions with other entities
    f32 predictability_score{0.0f};      // How predictable this entity's behavior is
    f32 complexity_score{0.0f};          // How complex the behavior pattern is
    
    // Behavior classification
    enum class BehaviorType {
        Static,      // Rarely changes
        Dynamic,     // Changes frequently
        Periodic,    // Changes in cycles
        Random,      // Unpredictable changes
        Reactive     // Changes in response to other entities
    } behavior_type{BehaviorType::Static};
    
    // Add new observation
    void add_observation(const ecs::ComponentSignature& signature, f32 activity = 1.0f, f32 interactions = 0.0f);
    
    // Pattern analysis
    f32 calculate_predictability() const;
    f32 calculate_complexity() const;
    BehaviorType classify_behavior() const;
    
    // Prediction helpers
    ecs::ComponentSignature predict_next_signature() const;
    f32 get_activity_trend() const;
    
    // Educational features
    std::string behavior_type_to_string() const;
    std::string get_pattern_summary() const;
    void print_pattern_analysis() const;
};

/**
 * @brief Configuration for behavior prediction system
 */
struct BehaviorPredictionConfig {
    usize max_history_length{100};        // Maximum observations to keep per entity
    f32 observation_interval{1.0f / 60.0f}; // Seconds between observations (60 FPS)
    usize min_observations_for_prediction{10}; // Minimum data needed for predictions
    f32 prediction_confidence_threshold{0.7f}; // Minimum confidence for predictions
    bool enable_real_time_learning{true};     // Update models during runtime
    bool enable_behavior_classification{true}; // Classify entity behavior types
    bool enable_interaction_tracking{true};   // Track entity-entity interactions
    
    // Model configuration
    MLModelConfig behavior_model_config{
        .model_name = "BehaviorPredictor",
        .input_dimension = 20,  // Will be set based on feature extraction
        .output_dimension = 5,  // Predict next 5 component types likely to be added/removed
        .learning_rate = 0.01f,
        .max_epochs = 500,
        .enable_training_visualization = true
    };
    
    // Performance settings
    usize max_concurrent_predictions{10};
    bool enable_async_training{true};
    bool enable_prediction_caching{true};
    std::chrono::milliseconds cache_ttl{1000};
};

/**
 * @brief Prediction result for entity behavior
 */
struct BehaviorPrediction {
    EntityID entity;
    Timestamp prediction_time;
    f32 confidence{0.0f};
    
    // Component predictions
    std::vector<std::pair<std::string, f32>> likely_components_to_add;    // <component_name, probability>
    std::vector<std::pair<std::string, f32>> likely_components_to_remove; // <component_name, probability>
    ecs::ComponentSignature predicted_signature;                          // Most likely future signature
    
    // Behavior predictions
    f32 predicted_activity_level{0.0f};
    f32 predicted_interaction_count{0.0f};
    EntityBehaviorPattern::BehaviorType predicted_behavior_type{EntityBehaviorPattern::BehaviorType::Static};
    
    // Temporal predictions
    f32 time_to_next_change{0.0f};        // Seconds until next component change
    f32 stability_duration{0.0f};         // How long current state will likely last
    
    // Validation
    bool is_valid() const { return confidence >= 0.1f && entity != ecs::null_entity(); }
    bool is_high_confidence() const { return confidence >= 0.8f; }
    
    // Educational features
    std::string to_string() const;
    void print_prediction_summary() const;
};

/**
 * @brief Cache for storing and retrieving behavior predictions
 */
class BehaviorPredictionCache {
private:
    struct CacheEntry {
        BehaviorPrediction prediction;
        Timestamp creation_time;
        usize access_count{1};
        
        bool is_expired(std::chrono::milliseconds ttl) const {
            auto now = std::chrono::high_resolution_clock::now();
            return (now - creation_time) > ttl;
        }
    };
    
    std::unordered_map<EntityID, CacheEntry> cache_;
    std::chrono::milliseconds ttl_;
    mutable std::mutex cache_mutex_;
    
    // Statistics
    mutable usize cache_hits_{0};
    mutable usize cache_misses_{0};
    mutable usize cache_evictions_{0};
    
public:
    explicit BehaviorPredictionCache(std::chrono::milliseconds ttl = std::chrono::milliseconds{1000})
        : ttl_(ttl) {}
    
    // Cache operations
    void store_prediction(const BehaviorPrediction& prediction);
    std::optional<BehaviorPrediction> get_prediction(EntityID entity) const;
    bool has_valid_prediction(EntityID entity) const;
    void invalidate_entity(EntityID entity);
    void clear_expired_entries();
    void clear_all();
    
    // Statistics
    f32 hit_rate() const;
    usize total_entries() const;
    std::string get_cache_statistics() const;
};

/**
 * @brief Main class for predicting entity behavior patterns
 * 
 * This system observes entity behavior over time and uses machine learning
 * to predict future component changes, activity levels, and behavior patterns.
 * It's designed to be educational and help understand how AI can enhance ECS.
 */
class ECSBehaviorPredictor {
private:
    BehaviorPredictionConfig config_;
    std::unique_ptr<MLModelBase> behavior_model_;
    std::unique_ptr<FeatureExtractor> feature_extractor_;
    BehaviorPredictionCache prediction_cache_;
    
    // Entity tracking
    std::unordered_map<EntityID, EntityBehaviorPattern> entity_patterns_;
    std::queue<std::pair<EntityID, Timestamp>> observation_queue_;
    
    // Training data
    TrainingDataset behavior_dataset_;
    bool model_needs_retraining_;
    Timestamp last_training_time_;
    
    // Performance tracking
    PredictionMetrics prediction_metrics_;
    std::vector<f32> prediction_accuracy_history_;
    
    // Async processing
    std::unique_ptr<std::thread> observation_thread_;
    std::unique_ptr<std::thread> training_thread_;
    std::atomic<bool> should_stop_threads_{false};
    mutable std::mutex entity_patterns_mutex_;
    mutable std::mutex dataset_mutex_;
    
    // Statistics
    std::atomic<usize> total_predictions_made_{0};
    std::atomic<usize> successful_predictions_{0};
    std::atomic<usize> entities_observed_{0};
    
public:
    explicit ECSBehaviorPredictor(const BehaviorPredictionConfig& config = BehaviorPredictionConfig{});
    ~ECSBehaviorPredictor();
    
    // Non-copyable but movable
    ECSBehaviorPredictor(const ECSBehaviorPredictor&) = delete;
    ECSBehaviorPredictor& operator=(const ECSBehaviorPredictor&) = delete;
    ECSBehaviorPredictor(ECSBehaviorPredictor&&) = default;
    ECSBehaviorPredictor& operator=(ECSBehaviorPredictor&&) = default;
    
    // Observation and training
    void observe_entity(EntityID entity, const ecs::Registry& registry);
    void observe_all_entities(const ecs::Registry& registry);
    void start_continuous_observation(const ecs::Registry& registry);
    void stop_continuous_observation();
    
    // Prediction
    BehaviorPrediction predict_entity_behavior(EntityID entity, const ecs::Registry& registry);
    std::vector<BehaviorPrediction> predict_all_entity_behaviors(const ecs::Registry& registry);
    BehaviorPrediction predict_with_context(EntityID entity, const PredictionContext& context);
    
    // Batch predictions for performance
    std::vector<BehaviorPrediction> predict_batch(const std::vector<EntityID>& entities,
                                                 const ecs::Registry& registry);
    
    // Model management
    bool train_model();
    bool train_model_async();
    void retrain_if_needed();
    bool save_model(const std::string& filepath) const;
    bool load_model(const std::string& filepath);
    void reset_model();
    
    // Pattern analysis
    const EntityBehaviorPattern* get_entity_pattern(EntityID entity) const;
    std::vector<EntityID> get_entities_by_behavior_type(EntityBehaviorPattern::BehaviorType type) const;
    std::vector<EntityID> get_most_predictable_entities(usize count = 10) const;
    std::vector<EntityID> get_least_predictable_entities(usize count = 10) const;
    
    // Validation and evaluation
    f32 validate_predictions(const ecs::Registry& registry);
    void evaluate_prediction_accuracy(const BehaviorPrediction& prediction, 
                                     const ecs::Registry& registry, 
                                     f32 time_elapsed);
    PredictionMetrics get_prediction_metrics() const { return prediction_metrics_; }
    
    // Configuration and statistics
    const BehaviorPredictionConfig& config() const { return config_; }
    void update_config(const BehaviorPredictionConfig& new_config);
    
    usize total_entities_observed() const { return entities_observed_; }
    usize total_predictions_made() const { return total_predictions_made_; }
    f32 prediction_success_rate() const;
    
    // Educational features
    std::string generate_behavior_report() const;
    void print_prediction_statistics() const;
    std::string visualize_entity_patterns(EntityID entity) const;
    std::string get_model_insights() const;
    
    // Advanced features
    std::vector<EntityID> find_similar_entities(EntityID reference_entity, usize count = 5) const;
    f32 calculate_entity_similarity(EntityID entity1, EntityID entity2) const;
    std::unordered_map<std::string, f32> analyze_component_usage_patterns() const;
    
    // Callbacks for real-time integration
    using PredictionCallback = std::function<void(const BehaviorPrediction&)>;
    using PatternChangeCallback = std::function<void(EntityID, const EntityBehaviorPattern&)>;
    
    void set_prediction_callback(PredictionCallback callback) { prediction_callback_ = callback; }
    void set_pattern_change_callback(PatternChangeCallback callback) { pattern_change_callback_ = callback; }
    
private:
    // Internal methods
    void initialize_model();
    void initialize_feature_extractor();
    void start_background_threads();
    void stop_background_threads();
    
    // Data processing
    void process_observation_queue();
    void update_entity_pattern(EntityID entity, const ecs::Registry& registry);
    TrainingSample create_training_sample(const EntityBehaviorPattern& pattern, 
                                         const PredictionContext& context);
    
    // Prediction implementation
    BehaviorPrediction make_prediction_internal(EntityID entity, 
                                               const EntityBehaviorPattern& pattern,
                                               const PredictionContext& context);
    f32 calculate_prediction_confidence(const PredictionResult& model_output,
                                       const EntityBehaviorPattern& pattern) const;
    
    // Model training
    void collect_training_data();
    void train_model_internal();
    bool should_retrain_model() const;
    
    // Utilities
    f32 calculate_entity_activity(EntityID entity, const ecs::Registry& registry) const;
    f32 calculate_entity_interactions(EntityID entity, const ecs::Registry& registry) const;
    void update_prediction_metrics(const BehaviorPrediction& prediction, bool was_correct);
    
    // Callbacks
    PredictionCallback prediction_callback_;
    PatternChangeCallback pattern_change_callback_;
    
    // Background thread management
    void observation_thread_function(const ecs::Registry& registry);
    void training_thread_function();
};

/**
 * @brief Utility functions for behavior prediction
 */
namespace behavior_utils {

// Component signature analysis
f32 signature_similarity(const ecs::ComponentSignature& sig1, const ecs::ComponentSignature& sig2);
std::vector<std::string> signature_diff(const ecs::ComponentSignature& from, const ecs::ComponentSignature& to);
f32 signature_complexity(const ecs::ComponentSignature& signature);

// Pattern analysis
f32 pattern_stability(const std::vector<ecs::ComponentSignature>& signature_history);
f32 pattern_periodicity(const std::vector<ecs::ComponentSignature>& signature_history,
                       const std::vector<Timestamp>& timestamps);
f32 pattern_trend_analysis(const std::vector<f32>& values);

// Educational utilities
std::string visualize_signature_changes(const std::vector<ecs::ComponentSignature>& history,
                                       const std::vector<Timestamp>& timestamps);
std::string create_behavior_timeline(const EntityBehaviorPattern& pattern);
std::string explain_prediction(const BehaviorPrediction& prediction);

} // namespace behavior_utils

} // namespace ecscope::ml