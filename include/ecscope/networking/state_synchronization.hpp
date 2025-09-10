#pragma once

#include "network_types.hpp"
#include "network_message.hpp"
#include "ecs_replication.hpp"
#include <vector>
#include <memory>
#include <unordered_map>
#include <chrono>
#include <functional>

namespace ecscope::networking {

/**
 * @brief World State Snapshot
 * 
 * Complete snapshot of the world state at a specific tick.
 */
class WorldSnapshot {
public:
    struct EntitySnapshot {
        NetworkEntityID network_entity_id = 0;
        ClientID owner_id = 0;
        
        struct ComponentData {
            ComponentTypeId type_id = 0;
            ComponentVersion version = 0;
            std::vector<uint8_t> data;
        };
        
        std::vector<ComponentData> components;
    };
    
    WorldSnapshot() = default;
    explicit WorldSnapshot(NetworkTick tick);
    
    NetworkTick get_tick() const { return tick_; }
    void set_tick(NetworkTick tick) { tick_ = tick; }
    
    // Entity management
    void add_entity(EntitySnapshot entity);
    void remove_entity(NetworkEntityID network_entity_id);
    const EntitySnapshot* get_entity(NetworkEntityID network_entity_id) const;
    
    const std::vector<EntitySnapshot>& get_entities() const { return entities_; }
    
    // Serialization
    std::vector<uint8_t> serialize() const;
    bool deserialize(const std::vector<uint8_t>& data);
    size_t get_serialized_size() const;
    
    // Comparison and delta generation
    std::vector<uint8_t> create_delta(const WorldSnapshot& previous) const;
    bool apply_delta(const WorldSnapshot& base, const std::vector<uint8_t>& delta);
    
    // Statistics
    size_t get_entity_count() const { return entities_.size(); }
    size_t get_total_component_count() const;

private:
    NetworkTick tick_ = 0;
    std::vector<EntitySnapshot> entities_;
    std::unordered_map<NetworkEntityID, size_t> entity_index_; // For fast lookup
};

/**
 * @brief Snapshot History Manager
 * 
 * Manages a rolling history of world snapshots for delta compression
 * and rollback/prediction systems.
 */
class SnapshotHistory {
public:
    explicit SnapshotHistory(size_t max_snapshots = 60); // 1 second at 60 Hz
    ~SnapshotHistory() = default;
    
    // Snapshot management
    void add_snapshot(std::unique_ptr<WorldSnapshot> snapshot);
    const WorldSnapshot* get_snapshot(NetworkTick tick) const;
    const WorldSnapshot* get_latest_snapshot() const;
    const WorldSnapshot* get_oldest_snapshot() const;
    
    // History queries
    std::vector<NetworkTick> get_available_ticks() const;
    NetworkTick get_latest_tick() const;
    NetworkTick get_oldest_tick() const;
    bool has_snapshot(NetworkTick tick) const;
    
    // Cleanup
    void cleanup_old_snapshots(NetworkTick before_tick);
    void clear();
    
    // Statistics
    size_t get_snapshot_count() const { return snapshots_.size(); }
    size_t get_max_snapshots() const { return max_snapshots_; }
    size_t get_memory_usage() const;

private:
    struct SnapshotEntry {
        NetworkTick tick;
        std::unique_ptr<WorldSnapshot> snapshot;
        std::chrono::steady_clock::time_point creation_time;
    };
    
    std::vector<SnapshotEntry> snapshots_;
    size_t max_snapshots_;
    mutable std::shared_mutex snapshots_mutex_;
    
    // Find snapshot index by tick (returns snapshots_.size() if not found)
    size_t find_snapshot_index(NetworkTick tick) const;
};

/**
 * @brief Delta Compression Engine
 * 
 * Handles delta compression between world snapshots to minimize
 * network bandwidth usage.
 */
class DeltaCompressionEngine {
public:
    struct CompressionStats {
        uint64_t full_snapshots_created = 0;
        uint64_t deltas_created = 0;
        uint64_t full_snapshot_bytes = 0;
        uint64_t delta_bytes = 0;
        double average_compression_ratio = 1.0;
        
        void update_compression_ratio() {
            if (full_snapshot_bytes > 0) {
                average_compression_ratio = static_cast<double>(delta_bytes) / 
                                          static_cast<double>(full_snapshot_bytes);
            }
        }
    };
    
    DeltaCompressionEngine() = default;
    ~DeltaCompressionEngine() = default;
    
    // Delta creation
    std::vector<uint8_t> create_world_delta(const WorldSnapshot& current, 
                                           const WorldSnapshot& previous) const;
    
    std::vector<uint8_t> create_entity_delta(const WorldSnapshot::EntitySnapshot& current,
                                            const WorldSnapshot::EntitySnapshot& previous) const;
    
    std::vector<uint8_t> create_component_delta(ComponentTypeId component_type,
                                              const std::vector<uint8_t>& current,
                                              const std::vector<uint8_t>& previous) const;
    
    // Delta application
    bool apply_world_delta(WorldSnapshot& target, const WorldSnapshot& base,
                          const std::vector<uint8_t>& delta) const;
    
    bool apply_entity_delta(WorldSnapshot::EntitySnapshot& target,
                           const WorldSnapshot::EntitySnapshot& base,
                           const std::vector<uint8_t>& delta) const;
    
    bool apply_component_delta(ComponentTypeId component_type,
                              std::vector<uint8_t>& target,
                              const std::vector<uint8_t>& base,
                              const std::vector<uint8_t>& delta) const;
    
    // Configuration
    void set_compression_threshold(size_t threshold) { compression_threshold_ = threshold; }
    size_t get_compression_threshold() const { return compression_threshold_; }
    
    // Statistics
    CompressionStats get_statistics() const;
    void reset_statistics();

private:
    size_t compression_threshold_ = 100; // Don't compress if delta is larger than this % of original
    mutable CompressionStats statistics_;
    mutable std::mutex statistics_mutex_;
    
    // Binary diff utilities
    std::vector<uint8_t> create_binary_delta(const std::vector<uint8_t>& current,
                                            const std::vector<uint8_t>& previous) const;
    bool apply_binary_delta(std::vector<uint8_t>& target,
                           const std::vector<uint8_t>& base,
                           const std::vector<uint8_t>& delta) const;
};

/**
 * @brief State Synchronization Message
 * 
 * Network message containing synchronized state data (snapshots or deltas).
 */
class StateSynchronizationMessage : public NetworkMessage {
public:
    static constexpr uint16_t MESSAGE_TYPE = 110;
    
    enum Type : uint8_t {
        FULL_SNAPSHOT = 0,
        DELTA_SNAPSHOT = 1,
        SNAPSHOT_ACK = 2
    };
    
    StateSynchronizationMessage();
    StateSynchronizationMessage(Type type, NetworkTick tick);
    
    Type get_sync_type() const { return sync_type_; }
    void set_sync_type(Type type) { sync_type_ = type; }
    
    NetworkTick get_target_tick() const { return target_tick_; }
    void set_target_tick(NetworkTick tick) { target_tick_ = tick; }
    
    NetworkTick get_base_tick() const { return base_tick_; }
    void set_base_tick(NetworkTick tick) { base_tick_ = tick; }
    
    const std::vector<uint8_t>& get_data() const { return data_; }
    void set_data(std::vector<uint8_t> data) { data_ = std::move(data); }
    
    size_t get_serialized_size() const override;

protected:
    void serialize_payload(std::vector<uint8_t>& buffer) const override;
    bool deserialize_payload(const uint8_t* data, size_t size) override;

private:
    Type sync_type_ = FULL_SNAPSHOT;
    NetworkTick target_tick_ = 0;
    NetworkTick base_tick_ = 0; // For delta snapshots
    std::vector<uint8_t> data_;
};

/**
 * @brief Client Prediction and Rollback System
 * 
 * Handles client-side prediction and server reconciliation for smooth gameplay.
 */
class PredictionSystem {
public:
    struct PredictionConfig {
        bool enable_prediction = true;
        bool enable_rollback = true;
        size_t max_prediction_frames = 10;
        double prediction_smoothing = 0.1; // For interpolation
        double rollback_threshold = 0.1;   // Rollback if error exceeds this
    };
    
    explicit PredictionSystem(const PredictionConfig& config = {});
    ~PredictionSystem() = default;
    
    // Prediction management
    void start_prediction(NetworkTick from_tick);
    void apply_server_update(const WorldSnapshot& authoritative_state);
    void rollback_to_tick(NetworkTick tick);
    
    // Input handling
    void record_input(NetworkTick tick, const std::vector<uint8_t>& input_data);
    void replay_inputs_from_tick(NetworkTick from_tick);
    
    // State management
    void save_predicted_state(NetworkTick tick, const WorldSnapshot& state);
    const WorldSnapshot* get_predicted_state(NetworkTick tick) const;
    
    // Configuration
    void set_config(const PredictionConfig& config) { config_ = config; }
    const PredictionConfig& get_config() const { return config_; }
    
    // Statistics
    struct PredictionStats {
        uint64_t predictions_made = 0;
        uint64_t rollbacks_performed = 0;
        uint64_t inputs_replayed = 0;
        double average_prediction_error = 0.0;
        double max_prediction_error = 0.0;
    };
    
    PredictionStats get_statistics() const;
    void reset_statistics();

private:
    struct InputRecord {
        NetworkTick tick;
        std::vector<uint8_t> input_data;
    };
    
    PredictionConfig config_;
    
    // Prediction state
    std::unique_ptr<SnapshotHistory> predicted_states_;
    std::vector<InputRecord> input_history_;
    NetworkTick last_server_tick_ = 0;
    
    // Statistics
    mutable PredictionStats statistics_;
    mutable std::mutex statistics_mutex_;
    
    // Helper functions
    void calculate_prediction_error(const WorldSnapshot& predicted, 
                                  const WorldSnapshot& authoritative);
    bool should_rollback(const WorldSnapshot& predicted, 
                        const WorldSnapshot& authoritative) const;
};

/**
 * @brief Interpolation and Extrapolation System
 * 
 * Handles smooth interpolation between network updates for visual smoothness.
 */
class InterpolationSystem {
public:
    struct InterpolationConfig {
        std::chrono::milliseconds interpolation_delay{100}; // Buffer for smooth interpolation
        double extrapolation_limit = 0.5; // Max seconds to extrapolate
        bool enable_extrapolation = true;
        bool enable_smoothing = true;
        double smoothing_factor = 0.2;
    };
    
    explicit InterpolationSystem(const InterpolationConfig& config = {});
    ~InterpolationSystem() = default;
    
    // Add network states for interpolation
    void add_network_state(NetworkTick tick, const WorldSnapshot& state);
    
    // Get interpolated state at current time
    std::unique_ptr<WorldSnapshot> get_interpolated_state(NetworkTimestamp current_time) const;
    
    // Get interpolated state at specific time
    std::unique_ptr<WorldSnapshot> get_interpolated_state_at_time(NetworkTimestamp target_time) const;
    
    // Configuration
    void set_config(const InterpolationConfig& config) { config_ = config; }
    const InterpolationConfig& get_config() const { return config_; }
    
    // Cleanup old states
    void cleanup_old_states(NetworkTimestamp before_time);

private:
    struct NetworkState {
        NetworkTick tick;
        NetworkTimestamp timestamp;
        std::unique_ptr<WorldSnapshot> snapshot;
    };
    
    InterpolationConfig config_;
    std::vector<NetworkState> network_states_;
    mutable std::shared_mutex states_mutex_;
    
    // Interpolation helpers
    std::unique_ptr<WorldSnapshot> interpolate_between_snapshots(
        const WorldSnapshot& earlier, const WorldSnapshot& later, double factor) const;
    
    std::unique_ptr<WorldSnapshot> extrapolate_from_snapshots(
        const WorldSnapshot& base, const WorldSnapshot& previous, double factor) const;
    
    double calculate_interpolation_factor(NetworkTimestamp target_time,
                                        NetworkTimestamp earlier_time,
                                        NetworkTimestamp later_time) const;
};

/**
 * @brief State Synchronization Manager
 * 
 * High-level manager that coordinates all state synchronization systems.
 */
class StateSynchronizationManager {
public:
    struct SyncConfig {
        std::chrono::milliseconds snapshot_interval{16}; // 60 Hz
        size_t max_snapshots_in_history = 60;
        bool enable_delta_compression = true;
        bool enable_prediction = true;
        bool enable_interpolation = true;
        DeltaCompressionEngine::CompressionStats compression_config;
        PredictionSystem::PredictionConfig prediction_config;
        InterpolationSystem::InterpolationConfig interpolation_config;
    };
    
    explicit StateSynchronizationManager(const SyncConfig& config = {});
    ~StateSynchronizationManager() = default;
    
    // Core update loop
    void update(NetworkTimestamp current_time);
    
    // Snapshot management
    void take_snapshot(NetworkTick tick, const WorldSnapshot& snapshot);
    const WorldSnapshot* get_current_display_state() const;
    
    // Network message handling
    void handle_state_sync_message(const StateSynchronizationMessage& message, ClientID sender);
    std::vector<std::unique_ptr<StateSynchronizationMessage>> generate_sync_messages_for_client(
        ClientID client_id, NetworkTick current_tick);
    
    // Client tracking
    void add_client(ClientID client_id);
    void remove_client(ClientID client_id);
    void set_client_last_ack_tick(ClientID client_id, NetworkTick tick);
    NetworkTick get_client_last_ack_tick(ClientID client_id) const;
    
    // Configuration
    void set_config(const SyncConfig& config) { config_ = config; }
    const SyncConfig& get_config() const { return config_; }
    
    // Statistics and debugging
    struct SyncStats {
        uint64_t snapshots_taken = 0;
        uint64_t sync_messages_sent = 0;
        uint64_t sync_messages_received = 0;
        DeltaCompressionEngine::CompressionStats compression_stats;
        PredictionSystem::PredictionStats prediction_stats;
    };
    
    SyncStats get_statistics() const;
    void reset_statistics();

private:
    SyncConfig config_;
    
    // Core systems
    std::unique_ptr<SnapshotHistory> snapshot_history_;
    std::unique_ptr<DeltaCompressionEngine> delta_engine_;
    std::unique_ptr<PredictionSystem> prediction_system_;
    std::unique_ptr<InterpolationSystem> interpolation_system_;
    
    // Client state tracking
    struct ClientSyncState {
        ClientID client_id = 0;
        NetworkTick last_ack_tick = 0;
        NetworkTimestamp last_sync_time = 0;
        bool needs_full_snapshot = true;
    };
    
    std::unordered_map<ClientID, ClientSyncState> client_states_;
    mutable std::shared_mutex client_states_mutex_;
    
    // Timing
    NetworkTimestamp last_snapshot_time_ = 0;
    NetworkTick current_tick_ = 0;
    
    // Statistics
    mutable SyncStats statistics_;
    mutable std::mutex statistics_mutex_;
    
    // Helper functions
    bool should_send_snapshot_to_client(ClientID client_id, NetworkTimestamp current_time) const;
    NetworkTick find_best_base_tick_for_client(ClientID client_id) const;
};

} // namespace ecscope::networking