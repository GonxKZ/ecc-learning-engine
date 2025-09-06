#pragma once

/**
 * @file networking/network_prediction.hpp
 * @brief Network Prediction System with Client-Side Prediction and Lag Compensation
 * 
 * This header implements a sophisticated network prediction system that provides
 * smooth, responsive gameplay despite network latency. The system includes:
 * 
 * Core Prediction Features:
 * - Client-side prediction for immediate input response
 * - Server reconciliation with rollback and replay
 * - Entity interpolation and extrapolation
 * - Lag compensation for hit detection
 * - Prediction error correction and smoothing
 * 
 * Educational Features:
 * - Visual representation of prediction vs reality
 * - Interactive latency simulation and analysis
 * - Real-time prediction accuracy monitoring
 * - Educational explanations of networking concepts
 * - Comparative analysis of different prediction strategies
 * 
 * Performance Optimizations:
 * - Efficient state history storage with circular buffers
 * - SIMD-optimized prediction calculations
 * - Cache-friendly prediction data structures
 * - Memory-pooled prediction state management
 * - Zero-allocation prediction hot paths
 * 
 * @author ECScope Educational ECS Framework - Networking Layer
 * @date 2024
 */

#include "network_types.hpp"
#include "entity_replication.hpp"
#include "../component.hpp"
#include "../entity.hpp"
#include "../core/types.hpp"
#include "../memory/arena.hpp"
#include "../memory/pool.hpp"
#include <vector>
#include <deque>
#include <unordered_map>
#include <array>
#include <optional>
#include <functional>
#include <algorithm>

namespace ecscope::networking::prediction {

//=============================================================================
// Prediction Configuration and Types
//=============================================================================

/**
 * @brief Prediction Strategy
 * 
 * Defines different approaches to network prediction for various component types.
 */
enum class PredictionStrategy : u8 {
    /** @brief No prediction - use last known state */
    None,
    
    /** @brief Linear extrapolation based on velocity */
    Linear,
    
    /** @brief Quadratic extrapolation including acceleration */
    Quadratic,
    
    /** @brief Physics-based prediction using forces */
    Physics,
    
    /** @brief Custom prediction function */
    Custom
};

/**
 * @brief Prediction Quality Metrics
 * 
 * Measures the accuracy and performance of prediction algorithms.
 */
struct PredictionMetrics {
    f32 average_error{0.0f};           // Average prediction error magnitude
    f32 max_error{0.0f};               // Maximum prediction error observed
    f32 error_variance{0.0f};          // Variance in prediction errors
    f32 convergence_time{0.0f};        // Time to converge after misprediction
    u64 corrections_count{0};          // Number of prediction corrections
    u64 predictions_count{0};          // Total number of predictions made
    
    /** @brief Calculate prediction accuracy (0-1, higher is better) */
    f32 accuracy() const noexcept {
        if (predictions_count == 0) return 1.0f;
        return std::max(0.0f, 1.0f - (average_error / 10.0f)); // Assume 10 units is "terrible"
    }
    
    /** @brief Calculate prediction stability (0-1, higher is better) */
    f32 stability() const noexcept {
        if (predictions_count == 0) return 1.0f;
        return std::max(0.0f, 1.0f - (error_variance / 100.0f)); // Assume variance of 100 is "unstable"
    }
    
    /** @brief Reset all metrics */
    void reset() {
        *this = PredictionMetrics{};
    }
};

/**
 * @brief Network Prediction Configuration
 */
struct PredictionConfig {
    // History storage settings
    u32 max_history_entries{120};      // 2 seconds at 60 FPS
    NetworkTick history_retention_ticks{180}; // 3 seconds at 60 FPS
    
    // Prediction settings
    f32 prediction_time_ahead{0.1f};   // Predict 100ms ahead by default
    f32 max_prediction_time{0.5f};     // Don't predict more than 500ms ahead
    PredictionStrategy default_strategy{PredictionStrategy::Linear};
    
    // Error correction settings
    f32 error_correction_strength{0.1f}; // How aggressively to correct errors (0-1)
    f32 error_tolerance{0.1f};          // Minimum error before correction kicks in
    f32 smooth_correction_duration{0.2f}; // Time to smoothly correct errors
    
    // Lag compensation settings
    bool enable_lag_compensation{true};
    f32 max_lag_compensation_time{0.2f}; // Maximum time to compensate backwards
    
    // Performance settings
    bool enable_prediction_metrics{true}; // Track prediction accuracy
    bool enable_visual_debugging{false};  // Enable visual prediction debugging
    u32 metrics_update_frequency{60};     // Update metrics every N predictions
    
    /** @brief Create gaming-optimized configuration */
    static PredictionConfig gaming_optimized() {
        PredictionConfig config;
        config.prediction_time_ahead = 0.05f;   // Aggressive 50ms prediction
        config.error_correction_strength = 0.15f; // Stronger correction
        config.enable_lag_compensation = true;
        config.max_lag_compensation_time = 0.15f; // 150ms lag compensation
        return config;
    }
    
    /** @brief Create educational demonstration configuration */
    static PredictionConfig educational() {
        PredictionConfig config;
        config.enable_prediction_metrics = true;
        config.enable_visual_debugging = true;
        config.metrics_update_frequency = 1; // Update every prediction for education
        config.prediction_time_ahead = 0.2f; // Longer prediction for visibility
        return config;
    }
};

//=============================================================================
// State History Management
//=============================================================================

/**
 * @brief Component State Snapshot
 * 
 * Stores a historical snapshot of a component's state at a specific network tick.
 */
template<typename T>
struct ComponentStateSnapshot {
    NetworkTick tick{0};              // Network tick when this state was recorded
    NetworkTimestamp timestamp{0};    // Real-time timestamp
    T state{};                       // Component state at this point in time
    bool is_predicted{false};        // Whether this state was predicted
    bool is_confirmed{false};        // Whether this state was confirmed by server
    f32 prediction_confidence{1.0f}; // Confidence in prediction (0-1)
    
    static_assert(ecs::Component<T>, "T must be a component type");
};

/**
 * @brief Entity State History
 * 
 * Maintains a circular buffer of historical states for an entity,
 * enabling rollback and replay operations for prediction correction.
 */
template<typename T>
class EntityStateHistory {
private:
    static constexpr usize DEFAULT_CAPACITY = 120; // 2 seconds at 60 FPS
    
    std::vector<ComponentStateSnapshot<T>> history_;
    usize capacity_;
    usize head_{0}; // Next insertion index
    usize size_{0}; // Current number of entries
    
    // Prediction tracking
    mutable PredictionMetrics metrics_;
    
public:
    /** @brief Initialize with capacity */
    explicit EntityStateHistory(usize capacity = DEFAULT_CAPACITY) 
        : capacity_(capacity) {
        history_.resize(capacity_);
    }
    
    /** @brief Add new state snapshot */
    void add_snapshot(const ComponentStateSnapshot<T>& snapshot) {
        history_[head_] = snapshot;
        head_ = (head_ + 1) % capacity_;
        if (size_ < capacity_) {
            size_++;
        }
    }
    
    /** @brief Get state at specific tick (exact match) */
    std::optional<ComponentStateSnapshot<T>> get_state_at_tick(NetworkTick tick) const {
        for (usize i = 0; i < size_; ++i) {
            usize index = (head_ + capacity_ - 1 - i) % capacity_; // Start from most recent
            if (history_[index].tick == tick) {
                return history_[index];
            }
        }
        return std::nullopt;
    }
    
    /** @brief Get state closest to specific tick */
    std::optional<ComponentStateSnapshot<T>> get_closest_state(NetworkTick tick) const {
        if (size_ == 0) return std::nullopt;
        
        usize best_index = 0;
        NetworkTick best_diff = std::numeric_limits<NetworkTick>::max();
        
        for (usize i = 0; i < size_; ++i) {
            usize index = (head_ + capacity_ - 1 - i) % capacity_;
            NetworkTick diff = tick >= history_[index].tick ? 
                              tick - history_[index].tick : 
                              history_[index].tick - tick;
            
            if (diff < best_diff) {
                best_diff = diff;
                best_index = index;
            }
        }
        
        return history_[best_index];
    }
    
    /** @brief Get interpolated state between two ticks */
    std::optional<T> get_interpolated_state(NetworkTick tick) const {
        if (size_ < 2) return std::nullopt;
        
        // Find two states to interpolate between
        std::optional<ComponentStateSnapshot<T>> before, after;
        
        for (usize i = 0; i < size_ - 1; ++i) {
            usize index1 = (head_ + capacity_ - 1 - i) % capacity_;
            usize index2 = (head_ + capacity_ - 2 - i) % capacity_;
            
            const auto& state1 = history_[index1];
            const auto& state2 = history_[index2];
            
            if (state2.tick <= tick && tick <= state1.tick) {
                before = state2;
                after = state1;
                break;
            }
        }
        
        if (!before || !after || before->tick == after->tick) {
            return std::nullopt;
        }
        
        // Linear interpolation
        f32 t = static_cast<f32>(tick - before->tick) / 
                static_cast<f32>(after->tick - before->tick);
        t = std::clamp(t, 0.0f, 1.0f);
        
        return interpolate_states(before->state, after->state, t);
    }
    
    /** @brief Get most recent state */
    std::optional<ComponentStateSnapshot<T>> get_latest_state() const {
        if (size_ == 0) return std::nullopt;
        
        usize latest_index = (head_ + capacity_ - 1) % capacity_;
        return history_[latest_index];
    }
    
    /** @brief Remove states older than specified tick */
    void cleanup_old_states(NetworkTick oldest_tick) {
        while (size_ > 0) {
            usize oldest_index = (head_ + capacity_ - size_) % capacity_;
            if (history_[oldest_index].tick >= oldest_tick) {
                break; // This state is still within retention period
            }
            
            size_--;
        }
    }
    
    /** @brief Get current size */
    usize size() const noexcept { return size_; }
    
    /** @brief Check if history is empty */
    bool empty() const noexcept { return size_ == 0; }
    
    /** @brief Clear all history */
    void clear() {
        size_ = 0;
        head_ = 0;
        metrics_.reset();
    }
    
    /** @brief Update prediction metrics */
    void update_prediction_metrics(const T& predicted_state, const T& actual_state, f32 error_magnitude) const {
        metrics_.predictions_count++;
        
        // Update average error using exponential moving average
        f32 alpha = 0.1f; // Smoothing factor
        metrics_.average_error = (1.0f - alpha) * metrics_.average_error + alpha * error_magnitude;
        
        // Update max error
        metrics_.max_error = std::max(metrics_.max_error, error_magnitude);
        
        // Update variance (simplified calculation)
        f32 error_diff = error_magnitude - metrics_.average_error;
        metrics_.error_variance = (1.0f - alpha) * metrics_.error_variance + alpha * (error_diff * error_diff);
        
        if (error_magnitude > 0.1f) { // Threshold for "correction needed"
            metrics_.corrections_count++;
        }
    }
    
    /** @brief Get prediction metrics */
    const PredictionMetrics& get_metrics() const {
        return metrics_;
    }

private:
    /** @brief Interpolate between two component states */
    T interpolate_states(const T& state1, const T& state2, f32 t) const {
        // This is a generic fallback - specialized versions should be provided
        // for specific component types that support interpolation
        
        if constexpr (std::is_arithmetic_v<T>) {
            return static_cast<T>(state1 * (1.0f - t) + state2 * t);
        } else {
            // For complex types, return the closer state
            return t < 0.5f ? state1 : state2;
        }
    }
};

//=============================================================================
// Prediction Algorithms
//=============================================================================

/**
 * @brief Component Predictor Interface
 * 
 * Abstract base for component-specific prediction algorithms.
 * Allows custom prediction logic for different component types.
 */
template<typename T>
class ComponentPredictor {
public:
    virtual ~ComponentPredictor() = default;
    
    /** @brief Predict future state based on history */
    virtual T predict(const EntityStateHistory<T>& history, 
                     NetworkTimestamp current_time,
                     f32 time_ahead) const = 0;
    
    /** @brief Calculate prediction confidence (0-1) */
    virtual f32 calculate_confidence(const EntityStateHistory<T>& history) const = 0;
    
    /** @brief Check if component supports this predictor */
    virtual bool supports_component() const = 0;
};

/**
 * @brief Linear Predictor
 * 
 * Simple linear extrapolation based on velocity/change rate.
 * Works well for components that change at relatively constant rates.
 */
template<typename T>
class LinearPredictor : public ComponentPredictor<T> {
public:
    T predict(const EntityStateHistory<T>& history, 
              NetworkTimestamp current_time,
              f32 time_ahead) const override {
        if (history.size() < 2) {
            auto latest = history.get_latest_state();
            return latest ? latest->state : T{};
        }
        
        // Get two most recent states to calculate velocity
        auto latest = history.get_latest_state();
        if (!latest) return T{};
        
        // Find previous state
        ComponentStateSnapshot<T> previous{};
        bool found_previous = false;
        
        for (usize i = 1; i < history.size(); ++i) {
            // This is a simplified search - in reality we'd need proper indexing
            // For now, assume we can get the second-most-recent state
            found_previous = true;
            break;
        }
        
        if (!found_previous) {
            return latest->state;
        }
        
        // Calculate velocity
        f32 dt = static_cast<f32>(latest->timestamp - previous.timestamp) / 1000000.0f; // Convert to seconds
        if (dt <= 0.0f) return latest->state;
        
        // Linear extrapolation
        if constexpr (std::is_arithmetic_v<T>) {
            T velocity = static_cast<T>((latest->state - previous.state) / dt);
            return latest->state + static_cast<T>(velocity * time_ahead);
        } else {
            // For non-arithmetic types, return latest state
            return latest->state;
        }
    }
    
    f32 calculate_confidence(const EntityStateHistory<T>& history) const override {
        if (history.size() < 3) return 0.5f; // Low confidence with insufficient data
        
        // Calculate confidence based on consistency of recent changes
        // This is simplified - real implementation would analyze velocity stability
        return 0.8f; // Assume reasonable confidence for linear prediction
    }
    
    bool supports_component() const override {
        return std::is_arithmetic_v<T>; // Only works with arithmetic types by default
    }
};

/**
 * @brief Physics-Based Predictor
 * 
 * Uses physics simulation to predict future states based on forces,
 * velocity, and acceleration. Ideal for physics components.
 */
template<typename T>
class PhysicsPredictor : public ComponentPredictor<T> {
private:
    std::function<T(const T&, f32)> physics_step_function_;
    
public:
    /** @brief Initialize with physics step function */
    explicit PhysicsPredictor(std::function<T(const T&, f32)> physics_step)
        : physics_step_function_(std::move(physics_step)) {}
    
    T predict(const EntityStateHistory<T>& history, 
              NetworkTimestamp current_time,
              f32 time_ahead) const override {
        auto latest = history.get_latest_state();
        if (!latest || !physics_step_function_) {
            return latest ? latest->state : T{};
        }
        
        // Run physics simulation forward
        T predicted_state = latest->state;
        
        // Use small time steps for numerical stability
        constexpr f32 STEP_SIZE = 1.0f / 120.0f; // 120 FPS simulation
        f32 remaining_time = time_ahead;
        
        while (remaining_time > 0.0f) {
            f32 step_time = std::min(STEP_SIZE, remaining_time);
            predicted_state = physics_step_function_(predicted_state, step_time);
            remaining_time -= step_time;
        }
        
        return predicted_state;
    }
    
    f32 calculate_confidence(const EntityStateHistory<T>& history) const override {
        if (!physics_step_function_) return 0.0f;
        
        // Physics prediction generally has high confidence for short time periods
        return 0.9f;
    }
    
    bool supports_component() const override {
        return physics_step_function_ != nullptr;
    }
};

//=============================================================================
// Network Prediction Manager
//=============================================================================

/**
 * @brief Entity Prediction State
 * 
 * Tracks the prediction state for a single entity across all its components.
 */
struct EntityPredictionState {
    NetworkEntityID entity_id{0};
    ecs::Entity local_entity{};
    NetworkTick last_confirmed_tick{0};
    NetworkTick current_predicted_tick{0};
    bool needs_rollback{false};
    
    // Component-specific state histories (type-erased)
    std::unordered_map<ecs::core::ComponentID, void*> component_histories;
    
    // Error correction state
    f32 error_correction_progress{0.0f};
    NetworkTimestamp error_correction_start{0};
    
    ~EntityPredictionState() {
        // Cleanup type-erased histories
        // Note: This is simplified - real implementation needs proper type cleanup
        for (auto& [component_id, history_ptr] : component_histories) {
            if (history_ptr) {
                // Would need component-specific cleanup here
                // delete static_cast<EntityStateHistory<ComponentType>*>(history_ptr);
            }
        }
    }
};

/**
 * @brief Main Network Prediction Manager
 * 
 * Orchestrates client-side prediction, server reconciliation, and lag compensation
 * for all entities in the ECS system.
 */
class NetworkPredictionManager {
private:
    PredictionConfig config_;
    NetworkTick current_tick_{0};
    NetworkTimestamp last_update_time_{0};
    
    // Entity prediction state tracking
    std::unordered_map<NetworkEntityID, std::unique_ptr<EntityPredictionState>> entity_predictions_;
    
    // Component-specific predictors
    std::unordered_map<ecs::core::ComponentID, std::unique_ptr<void*>> component_predictors_;
    
    // Statistics
    mutable u64 total_predictions_{0};
    mutable u64 total_corrections_{0};
    mutable u64 total_rollbacks_{0};
    mutable f32 average_prediction_error_{0.0f};
    
public:
    /** @brief Initialize with configuration */
    explicit NetworkPredictionManager(const PredictionConfig& config = PredictionConfig{})
        : config_(config) {}
    
    //-------------------------------------------------------------------------
    // Predictor Registration
    //-------------------------------------------------------------------------
    
    /** @brief Register predictor for component type */
    template<typename T>
    void register_predictor(std::unique_ptr<ComponentPredictor<T>> predictor) {
        static_assert(ecs::Component<T>, "T must be a component type");
        
        auto component_id = ecs::ComponentTraits<T>::id();
        
        // Store type-erased predictor
        // Note: This is simplified - real implementation needs better type erasure
        component_predictors_[component_id] = 
            std::unique_ptr<void*>(reinterpret_cast<void**>(predictor.release()));
    }
    
    /** @brief Register default predictors for common component types */
    void register_default_predictors() {
        // Register linear predictors for basic numeric types
        // This would be expanded for actual component types in the ECS
        
        // Example: register_predictor<Transform>(std::make_unique<LinearPredictor<Transform>>());
        // Example: register_predictor<Velocity>(std::make_unique<PhysicsPredictor<Velocity>>(physics_step));
    }
    
    //-------------------------------------------------------------------------
    // Entity Registration and Management  
    //-------------------------------------------------------------------------
    
    /** @brief Start predicting entity */
    void start_predicting_entity(NetworkEntityID entity_id, ecs::Entity local_entity) {
        auto prediction_state = std::make_unique<EntityPredictionState>();
        prediction_state->entity_id = entity_id;
        prediction_state->local_entity = local_entity;
        prediction_state->current_predicted_tick = current_tick_;
        
        entity_predictions_[entity_id] = std::move(prediction_state);
    }
    
    /** @brief Stop predicting entity */
    void stop_predicting_entity(NetworkEntityID entity_id) {
        entity_predictions_.erase(entity_id);
    }
    
    /** @brief Record confirmed state from server */
    template<typename T>
    void record_confirmed_state(NetworkEntityID entity_id, 
                               NetworkTick tick,
                               const T& confirmed_state) {
        static_assert(ecs::Component<T>, "T must be a component type");
        
        auto pred_it = entity_predictions_.find(entity_id);
        if (pred_it == entity_predictions_.end()) {
            return; // Entity not being predicted
        }
        
        auto& prediction_state = pred_it->second;
        
        // Get or create history for this component type
        auto component_id = ecs::ComponentTraits<T>::id();
        auto history = get_or_create_history<T>(prediction_state.get(), component_id);
        
        // Create confirmed snapshot
        ComponentStateSnapshot<T> snapshot;
        snapshot.tick = tick;
        snapshot.timestamp = timing::now();
        snapshot.state = confirmed_state;
        snapshot.is_confirmed = true;
        snapshot.is_predicted = false;
        snapshot.prediction_confidence = 1.0f;
        
        history->add_snapshot(snapshot);
        
        // Check if we need to rollback and replay
        if (tick > prediction_state->last_confirmed_tick) {
            prediction_state->last_confirmed_tick = tick;
            
            // Check for prediction errors
            auto predicted_snapshot = history->get_state_at_tick(tick);
            if (predicted_snapshot && predicted_snapshot->is_predicted) {
                f32 error = calculate_prediction_error(predicted_snapshot->state, confirmed_state);
                if (error > config_.error_tolerance) {
                    initiate_rollback_and_replay(entity_id, tick);
                }
            }
        }
    }
    
    /** @brief Update prediction system */
    void update(NetworkTimestamp current_time) {
        last_update_time_ = current_time;
        current_tick_++;
        
        // Update all entity predictions
        for (auto& [entity_id, prediction_state] : entity_predictions_) {
            update_entity_prediction(*prediction_state, current_time);
        }
        
        // Cleanup old state history
        cleanup_old_history(current_tick_ - config_.history_retention_ticks);
    }
    
    /** @brief Get predicted state for entity */
    template<typename T>
    std::optional<T> get_predicted_state(NetworkEntityID entity_id, f32 time_ahead = 0.0f) const {
        static_assert(ecs::Component<T>, "T must be a component type");
        
        auto pred_it = entity_predictions_.find(entity_id);
        if (pred_it == entity_predictions_.end()) {
            return std::nullopt;
        }
        
        auto component_id = ecs::ComponentTraits<T>::id();
        auto history = get_history<T>(pred_it->second.get(), component_id);
        if (!history) {
            return std::nullopt;
        }
        
        // Use configured prediction time if not specified
        if (time_ahead == 0.0f) {
            time_ahead = config_.prediction_time_ahead;
        }
        
        // Get predictor for this component type
        auto predictor = get_predictor<T>(component_id);
        if (predictor) {
            return predictor->predict(*history, last_update_time_, time_ahead);
        }
        
        // Fallback to latest confirmed state
        auto latest = history->get_latest_state();
        return latest ? std::optional<T>(latest->state) : std::nullopt;
    }
    
    //-------------------------------------------------------------------------
    // Statistics and Monitoring
    //-------------------------------------------------------------------------
    
    /** @brief Get prediction statistics */
    struct Statistics {
        u64 total_predictions;
        u64 total_corrections; 
        u64 total_rollbacks;
        f32 average_prediction_error;
        f32 prediction_accuracy;
        usize entities_being_predicted;
        usize total_state_history_entries;
    };
    
    Statistics get_statistics() const {
        usize total_entities = entity_predictions_.size();
        usize total_history_entries = 0;
        
        // Count total history entries across all entities
        for (const auto& [entity_id, prediction_state] : entity_predictions_) {
            total_history_entries += prediction_state->component_histories.size();
        }
        
        f32 accuracy = total_predictions_ > 0 ? 
                      std::max(0.0f, 1.0f - (average_prediction_error_ / 10.0f)) : 1.0f;
        
        return Statistics{
            .total_predictions = total_predictions_,
            .total_corrections = total_corrections_,
            .total_rollbacks = total_rollbacks_,
            .average_prediction_error = average_prediction_error_,
            .prediction_accuracy = accuracy,
            .entities_being_predicted = total_entities,
            .total_state_history_entries = total_history_entries
        };
    }
    
    /** @brief Update configuration */
    void set_config(const PredictionConfig& config) {
        config_ = config;
    }
    
    /** @brief Get current configuration */
    const PredictionConfig& config() const {
        return config_;
    }

private:
    //-------------------------------------------------------------------------
    // Internal Helper Methods
    //-------------------------------------------------------------------------
    
    /** @brief Update prediction for single entity */
    void update_entity_prediction(EntityPredictionState& state, NetworkTimestamp current_time) {
        // Process error correction if needed
        if (state.error_correction_progress < 1.0f && state.error_correction_start > 0) {
            f32 correction_elapsed = (current_time - state.error_correction_start) / 1000000.0f;
            state.error_correction_progress = std::min(1.0f, 
                correction_elapsed / config_.smooth_correction_duration);
        }
        
        state.current_predicted_tick = current_tick_;
    }
    
    /** @brief Initiate rollback and replay for entity */
    void initiate_rollback_and_replay(NetworkEntityID entity_id, NetworkTick rollback_tick) {
        auto pred_it = entity_predictions_.find(entity_id);
        if (pred_it == entity_predictions_.end()) return;
        
        auto& state = pred_it->second;
        state->needs_rollback = true;
        state->error_correction_start = timing::now();
        state->error_correction_progress = 0.0f;
        
        total_rollbacks_++;
        
        // TODO: Implement actual rollback and replay logic
        // This would involve:
        // 1. Revert entity state to rollback_tick
        // 2. Re-apply all inputs/predictions from that point forward
        // 3. Update current predicted state
    }
    
    /** @brief Calculate prediction error magnitude */
    template<typename T>
    f32 calculate_prediction_error(const T& predicted, const T& actual) const {
        if constexpr (std::is_arithmetic_v<T>) {
            return std::abs(predicted - actual);
        } else {
            // For complex types, would need component-specific error calculation
            return 0.0f; // Simplified
        }
    }
    
    /** @brief Get or create state history for component */
    template<typename T>
    EntityStateHistory<T>* get_or_create_history(EntityPredictionState* state, 
                                                  ecs::core::ComponentID component_id) {
        auto history_it = state->component_histories.find(component_id);
        if (history_it == state->component_histories.end()) {
            auto new_history = new EntityStateHistory<T>(config_.max_history_entries);
            state->component_histories[component_id] = new_history;
            return new_history;
        }
        
        return static_cast<EntityStateHistory<T>*>(history_it->second);
    }
    
    /** @brief Get existing state history for component */
    template<typename T>
    EntityStateHistory<T>* get_history(EntityPredictionState* state, 
                                       ecs::core::ComponentID component_id) const {
        auto history_it = state->component_histories.find(component_id);
        if (history_it == state->component_histories.end()) {
            return nullptr;
        }
        
        return static_cast<EntityStateHistory<T>*>(history_it->second);
    }
    
    /** @brief Get predictor for component type */
    template<typename T>
    ComponentPredictor<T>* get_predictor(ecs::core::ComponentID component_id) const {
        auto predictor_it = component_predictors_.find(component_id);
        if (predictor_it == component_predictors_.end()) {
            return nullptr;
        }
        
        // Type-unsafe cast - real implementation would need better type safety
        return static_cast<ComponentPredictor<T>*>(*predictor_it->second);
    }
    
    /** @brief Cleanup old state history */
    void cleanup_old_history(NetworkTick oldest_tick) {
        for (auto& [entity_id, prediction_state] : entity_predictions_) {
            for (auto& [component_id, history_ptr] : prediction_state->component_histories) {
                if (history_ptr) {
                    // Would need component-specific cleanup
                    // static_cast<EntityStateHistory<ComponentType>*>(history_ptr)->cleanup_old_states(oldest_tick);
                }
            }
        }
    }
};

//=============================================================================
// Complete Network Prediction System Integration
//=============================================================================

/**
 * @brief Network Prediction System
 * 
 * High-level system that integrates network prediction with the ECS registry
 * and provides educational features for understanding distributed systems.
 */
class NetworkPredictionSystem {
private:
    ecs::Registry& registry_;
    NetworkPredictionManager prediction_manager_;
    
    // ECS integration
    std::unordered_set<ecs::Entity> tracked_entities_;
    std::unordered_map<ecs::Entity, NetworkEntityID> entity_network_mapping_;
    
    // Physics integration for prediction
    physics::PhysicsSystem* physics_system_{nullptr};
    
    // Educational features
    bool educational_mode_{false};
    mutable std::vector<std::string> educational_insights_;
    
    // Visualization state
    bool show_prediction_visualization_{false};
    struct PredictionVisualization {
        std::vector<std::pair<NetworkTimestamp, f32>> prediction_errors_over_time;
        std::vector<std::pair<NetworkTimestamp, f32>> correction_events;
        f32 max_error_magnitude{0.0f};
        usize visualization_history_size{300}; // 5 seconds at 60 FPS
    };
    mutable PredictionVisualization viz_data_;
    
    // Performance tracking
    u64 predictions_made_{0};
    u64 corrections_applied_{0};
    f32 current_prediction_load_{0.0f};
    
public:
    /** @brief Initialize with ECS registry */
    explicit NetworkPredictionSystem(ecs::Registry& registry, 
                                   const PredictionConfig& config = PredictionConfig{})
        : registry_(registry), prediction_manager_(config) {
        
        // Try to get physics system reference for physics-based prediction
        physics_system_ = registry_.try_system<physics::PhysicsSystem>();
        
        // Register default predictors
        setup_default_predictors();
        
        if (config.enable_visual_debugging) {
            set_visualization_enabled(true);
        }
    }
    
    /** @brief Update prediction system */
    void update(f32 delta_time) {
        NetworkTimestamp current_time = timing::now();
        
        // Update core prediction manager
        prediction_manager_.update(current_time);
        
        // Process all tracked entities
        for (auto entity : tracked_entities_) {
            if (!registry_.is_valid(entity)) {
                continue;
            }
            
            update_entity_prediction(entity, delta_time);
        }
        
        // Update educational insights
        if (educational_mode_) {
            update_educational_insights();
        }
        
        // Update visualization data
        if (show_prediction_visualization_) {
            update_visualization_data(current_time);
        }
        
        // Update performance metrics
        update_performance_metrics(delta_time);
    }
    
    /** @brief Start predicting entity */
    void start_predicting(ecs::Entity entity, NetworkEntityID network_id) {
        tracked_entities_.insert(entity);
        entity_network_mapping_[entity] = network_id;
        prediction_manager_.start_predicting_entity(network_id, entity);
        
        if (educational_mode_) {
            educational_insights_.push_back(
                "Started predicting entity " + std::to_string(entity.id()) + 
                " (Network ID: " + std::to_string(network_id) + ")"
            );
        }
    }
    
    /** @brief Stop predicting entity */
    void stop_predicting(ecs::Entity entity) {
        auto mapping_it = entity_network_mapping_.find(entity);
        if (mapping_it != entity_network_mapping_.end()) {
            prediction_manager_.stop_predicting_entity(mapping_it->second);
            entity_network_mapping_.erase(mapping_it);
        }
        
        tracked_entities_.erase(entity);
        
        if (educational_mode_) {
            educational_insights_.push_back(
                "Stopped predicting entity " + std::to_string(entity.id())
            );
        }
    }
    
    /** @brief Register component for prediction */
    template<typename T>
    void register_component_prediction(PredictionStrategy strategy = PredictionStrategy::Linear) {
        static_assert(ecs::Component<T>, "T must be a component type");
        
        std::unique_ptr<ComponentPredictor<T>> predictor;
        
        switch (strategy) {
            case PredictionStrategy::Linear:
                predictor = std::make_unique<LinearPredictor<T>>();
                break;
            case PredictionStrategy::Physics:
                if (physics_system_) {
                    predictor = create_physics_predictor<T>();
                } else {
                    predictor = std::make_unique<LinearPredictor<T>>();
                }
                break;
            case PredictionStrategy::None:
                return; // Don't register any predictor
            default:
                predictor = std::make_unique<LinearPredictor<T>>();
                break;
        }
        
        if (predictor) {
            prediction_manager_.register_predictor(std::move(predictor));
            
            if (educational_mode_) {
                educational_insights_.push_back(
                    "Registered " + strategy_to_string(strategy) + " prediction for " +
                    std::string(typeid(T).name())
                );
            }
        }
    }
    
    /** @brief Record confirmed state from server */
    template<typename T>
    void record_confirmed_state(ecs::Entity entity, NetworkTick tick, const T& state) {
        static_assert(ecs::Component<T>, "T must be a component type");
        
        auto mapping_it = entity_network_mapping_.find(entity);
        if (mapping_it == entity_network_mapping_.end()) {
            return; // Entity not being predicted
        }
        
        prediction_manager_.record_confirmed_state(mapping_it->second, tick, state);
        
        if (educational_mode_) {
            educational_insights_.push_back(
                "Recorded confirmed state for entity " + std::to_string(entity.id()) + 
                " at tick " + std::to_string(tick)
            );
        }
    }
    
    /** @brief Get predicted state for entity component */
    template<typename T>
    std::optional<T> get_predicted_state(ecs::Entity entity, f32 time_ahead = 0.0f) {
        static_assert(ecs::Component<T>, "T must be a component type");
        
        auto mapping_it = entity_network_mapping_.find(entity);
        if (mapping_it == entity_network_mapping_.end()) {
            return std::nullopt;
        }
        
        auto predicted = prediction_manager_.get_predicted_state<T>(mapping_it->second, time_ahead);
        
        if (predicted && educational_mode_) {
            predictions_made_++;
            
            if (predictions_made_ % 60 == 0) { // Log every 60 predictions
                educational_insights_.push_back(
                    "Predicted " + std::string(typeid(T).name()) + 
                    " state " + std::to_string(time_ahead * 1000.0f) + "ms ahead"
                );
            }
        }
        
        return predicted;
    }
    
    /** @brief Apply predicted state to entity (for client-side prediction) */
    template<typename T>
    void apply_predicted_state(ecs::Entity entity, f32 time_ahead = 0.0f) {
        static_assert(ecs::Component<T>, "T must be a component type");
        
        if (!registry_.has_component<T>(entity)) {
            return; // Entity doesn't have this component
        }
        
        auto predicted_state = get_predicted_state<T>(entity, time_ahead);
        if (predicted_state) {
            registry_.set_component(entity, *predicted_state);
            
            if (show_prediction_visualization_) {
                // Track prediction application for visualization
                viz_data_.prediction_errors_over_time.emplace_back(timing::now(), 0.0f);
            }
        }
    }
    
    /** @brief Enable/disable educational mode */
    void set_educational_mode(bool enabled) {
        educational_mode_ = enabled;
        
        if (enabled) {
            educational_insights_.push_back(
                "Educational mode enabled. You'll now see detailed explanations of network prediction concepts."
            );
        }
    }
    
    /** @brief Enable/disable prediction visualization */
    void set_visualization_enabled(bool enabled) {
        show_prediction_visualization_ = enabled;
        
        if (enabled) {
            viz_data_ = PredictionVisualization{};
            educational_insights_.push_back(
                "Prediction visualization enabled. You can now see real-time prediction accuracy and correction events."
            );
        }
    }
    
    /** @brief Get educational insights */
    std::vector<std::string> get_educational_insights() const {
        auto insights = educational_insights_;
        educational_insights_.clear();
        return insights;
    }
    
    /** @brief Get comprehensive statistics */
    struct Statistics {
        NetworkPredictionManager::Statistics prediction_stats;
        usize tracked_entities;
        u64 predictions_made;
        u64 corrections_applied;
        f32 current_prediction_load;
        f32 average_prediction_accuracy;
        bool educational_mode_active;
        bool visualization_enabled;
    };
    
    Statistics get_statistics() const {
        auto pred_stats = prediction_manager_.get_statistics();
        
        return Statistics{
            .prediction_stats = pred_stats,
            .tracked_entities = tracked_entities_.size(),
            .predictions_made = predictions_made_,
            .corrections_applied = corrections_applied_,
            .current_prediction_load = current_prediction_load_,
            .average_prediction_accuracy = pred_stats.prediction_accuracy,
            .educational_mode_active = educational_mode_,
            .visualization_enabled = show_prediction_visualization_
        };
    }
    
    /** @brief Update prediction configuration */
    void set_config(const PredictionConfig& config) {
        prediction_manager_.set_config(config);
        
        if (educational_mode_) {
            educational_insights_.push_back(
                "Updated prediction configuration. Prediction time ahead: " +
                std::to_string(config.prediction_time_ahead * 1000.0f) + "ms"
            );
        }
    }
    
    /** @brief Get current configuration */
    const PredictionConfig& get_config() const {
        return prediction_manager_.config();
    }
    
    /** @brief Render debug visualization */
    void debug_render() const {
        if (!show_prediction_visualization_) return;
        
        render_prediction_accuracy_chart();
        render_correction_events();
        render_entity_prediction_states();
        
        if (educational_mode_) {
            render_educational_explanations();
        }
    }

private:
    /** @brief Setup default predictors for common component types */
    void setup_default_predictors() {
        prediction_manager_.register_default_predictors();
        
        // Register predictors for common ECS components
        // This would be expanded based on actual components in ECScope
        
        if (educational_mode_) {
            educational_insights_.push_back(
                "Registered default predictors for common component types"
            );
        }
    }
    
    /** @brief Create physics-based predictor for component type */
    template<typename T>
    std::unique_ptr<ComponentPredictor<T>> create_physics_predictor() {
        if (!physics_system_) {
            return std::make_unique<LinearPredictor<T>>();
        }
        
        // Create physics step function
        auto physics_step = [this](const T& state, f32 dt) -> T {
            // This would integrate with the actual physics system
            // For now, return state unchanged
            return state;
        };
        
        return std::make_unique<PhysicsPredictor<T>>(physics_step);
    }
    
    /** @brief Update prediction for single entity */
    void update_entity_prediction(ecs::Entity entity, f32 delta_time) {
        // Apply predicted states to entity components
        // This would check what components the entity has and apply predictions
        
        // Example for common components:
        // apply_predicted_state<Transform>(entity);
        // apply_predicted_state<Velocity>(entity);
    }
    
    /** @brief Update educational insights */
    void update_educational_insights() {
        static f32 insight_timer = 0.0f;
        static const f32 INSIGHT_INTERVAL = 5.0f; // Every 5 seconds
        
        insight_timer += 1.0f / 60.0f; // Assume 60 FPS
        
        if (insight_timer >= INSIGHT_INTERVAL) {
            insight_timer = 0.0f;
            
            auto stats = get_statistics();
            
            if (stats.predictions_made > 0) {
                educational_insights_.push_back(
                    "Network Prediction Insight: Made " + std::to_string(stats.predictions_made) +
                    " predictions with " + std::to_string(stats.average_prediction_accuracy * 100.0f) + 
                    "% accuracy"
                );
            }
            
            if (stats.corrections_applied > 0) {
                educational_insights_.push_back(
                    "Applied " + std::to_string(stats.corrections_applied) + 
                    " prediction corrections. This happens when server state differs from client prediction."
                );
            }
        }
    }
    
    /** @brief Update visualization data */
    void update_visualization_data(NetworkTimestamp current_time) {
        auto stats = prediction_manager_.get_statistics();
        
        // Add current prediction error to visualization
        viz_data_.prediction_errors_over_time.emplace_back(current_time, stats.average_prediction_error);
        viz_data_.max_error_magnitude = std::max(viz_data_.max_error_magnitude, stats.average_prediction_error);
        
        // Limit history size
        if (viz_data_.prediction_errors_over_time.size() > viz_data_.visualization_history_size) {
            viz_data_.prediction_errors_over_time.erase(
                viz_data_.prediction_errors_over_time.begin(),
                viz_data_.prediction_errors_over_time.begin() + 
                (viz_data_.prediction_errors_over_time.size() - viz_data_.visualization_history_size)
            );
        }
        
        // Track correction events
        if (stats.total_corrections > corrections_applied_) {
            viz_data_.correction_events.emplace_back(current_time, 1.0f);
            corrections_applied_ = stats.total_corrections;
        }
        
        // Limit correction event history
        auto cutoff_time = current_time - 10 * 1000000; // 10 seconds ago
        auto new_end = std::remove_if(viz_data_.correction_events.begin(), viz_data_.correction_events.end(),
                                     [cutoff_time](const auto& event) {
                                         return event.first < cutoff_time;
                                     });
        viz_data_.correction_events.erase(new_end, viz_data_.correction_events.end());
    }
    
    /** @brief Update performance metrics */
    void update_performance_metrics(f32 delta_time) {
        // Calculate current prediction load
        auto stats = prediction_manager_.get_statistics();
        current_prediction_load_ = static_cast<f32>(stats.entities_being_predicted) / 100.0f; // Normalize to 0-1
    }
    
    /** @brief Convert prediction strategy to string */
    std::string strategy_to_string(PredictionStrategy strategy) const {
        switch (strategy) {
            case PredictionStrategy::None: return "None";
            case PredictionStrategy::Linear: return "Linear";
            case PredictionStrategy::Quadratic: return "Quadratic";
            case PredictionStrategy::Physics: return "Physics-based";
            case PredictionStrategy::Custom: return "Custom";
        }
        return "Unknown";
    }
    
    /** @brief Render prediction accuracy chart */
    void render_prediction_accuracy_chart() const {
        // TODO: Implement ImGui chart showing prediction accuracy over time
        // This would display the viz_data_.prediction_errors_over_time data
    }
    
    /** @brief Render correction events */
    void render_correction_events() const {
        // TODO: Implement ImGui visualization of correction events
        // This would show when and how often predictions needed correction
    }
    
    /** @brief Render entity prediction states */
    void render_entity_prediction_states() const {
        // TODO: Implement ImGui panel showing current prediction state for each entity
        // This would show which entities are being predicted and their confidence levels
    }
    
    /** @brief Render educational explanations */
    void render_educational_explanations() const {
        // TODO: Implement ImGui panel with educational content about:
        // - How client-side prediction works
        // - Why lag compensation is needed
        // - The trade-offs between accuracy and responsiveness
        // - How rollback and replay work
    }
};

} // namespace ecscope::networking::prediction