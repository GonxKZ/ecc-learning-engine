#pragma once

/**
 * @file networking/network_simulation.hpp
 * @brief Network Condition Simulation and Educational Tools for ECScope
 * 
 * This header provides comprehensive network simulation capabilities that allow
 * developers and students to understand networking concepts through interactive
 * experimentation. The system includes:
 * 
 * Core Simulation Features:
 * - Artificial latency injection with realistic distributions
 * - Packet loss simulation with various patterns
 * - Bandwidth throttling and congestion simulation
 * - Network jitter and out-of-order packet delivery
 * - Connection quality degradation scenarios
 * 
 * Educational Visualizations:
 * - Real-time network performance graphs
 * - Packet flow visualization and analysis
 * - Interactive network parameter tuning
 * - Comparative performance analysis
 * - Educational explanations of networking phenomena
 * 
 * Simulation Profiles:
 * - Real-world connection types (3G, 4G, WiFi, Ethernet)
 * - Stress testing scenarios (high latency, packet loss)
 * - Educational demonstration modes
 * - Custom simulation profiles for specific learning objectives
 * 
 * @author ECScope Educational ECS Framework - Networking Layer
 * @date 2024
 */

#include "network_types.hpp"
#include "udp_socket.hpp"
#include "../core/types.hpp"
#include "../memory/arena.hpp"
#include <queue>
#include <random>
#include <chrono>
#include <vector>
#include <unordered_map>
#include <functional>
#include <algorithm>

namespace ecscope::networking::simulation {

//=============================================================================
// Network Condition Simulation Parameters
//=============================================================================

/**
 * @brief Network Latency Model
 * 
 * Defines how artificial latency should be applied to network packets.
 * Different models simulate different real-world network conditions.
 */
enum class LatencyModel : u8 {
    /** @brief Constant latency (ideal for educational demonstrations) */
    Constant,
    
    /** @brief Uniform random latency within range */
    Uniform,
    
    /** @brief Normal distribution around mean latency */
    Normal,
    
    /** @brief Exponential distribution (models queuing delays) */
    Exponential,
    
    /** @brief Spike pattern (periodic latency spikes) */
    Spike,
    
    /** @brief Custom latency function */
    Custom
};

/**
 * @brief Packet Loss Model
 * 
 * Defines different patterns of packet loss for educational purposes.
 */
enum class PacketLossModel : u8 {
    /** @brief Random uniform packet loss */
    Random,
    
    /** @brief Burst loss (losing consecutive packets) */
    Burst,
    
    /** @brief Periodic loss pattern */
    Periodic,
    
    /** @brief Congestion-based loss (loss increases with load) */
    Congestion,
    
    /** @brief Gilbert-Elliott model (good/bad state transitions) */
    GilbertElliott
};

/**
 * @brief Bandwidth Throttling Model
 * 
 * Simulates different bandwidth limitations and their effects.
 */
enum class BandwidthModel : u8 {
    /** @brief Fixed bandwidth limit */
    Fixed,
    
    /** @brief Variable bandwidth (simulates network congestion) */
    Variable,
    
    /** @brief Burst capacity with sustained rate */
    TokenBucket,
    
    /** @brief Real-world connection profile */
    ConnectionProfile
};

/**
 * @brief Network Simulation Configuration
 * 
 * Comprehensive configuration for all aspects of network simulation.
 */
struct NetworkSimulationConfig {
    // Latency simulation
    LatencyModel latency_model{LatencyModel::Normal};
    f32 base_latency_ms{50.0f};        // Base network latency
    f32 latency_variance_ms{10.0f};    // Latency variation
    f32 spike_latency_ms{500.0f};      // Spike latency for spike model
    f32 spike_frequency{0.1f};         // Frequency of latency spikes (0-1)
    
    // Packet loss simulation  
    PacketLossModel loss_model{PacketLossModel::Random};
    f32 packet_loss_rate{0.01f};       // Base packet loss rate (0-1)
    u32 burst_loss_count{3};           // Consecutive packets lost in burst
    f32 burst_loss_probability{0.05f}; // Probability of burst loss event
    
    // Bandwidth throttling
    BandwidthModel bandwidth_model{BandwidthModel::Fixed};
    u32 bandwidth_kbps{1000};          // Available bandwidth in kbps
    u32 burst_capacity_kb{100};        // Burst capacity for token bucket
    f32 bandwidth_variance{0.2f};      // Bandwidth variation (0-1)
    
    // Jitter and reordering
    bool enable_jitter{true};          // Enable packet timing jitter
    f32 jitter_variance_ms{5.0f};      // Jitter variance
    bool enable_reordering{false};     // Enable out-of-order delivery
    f32 reorder_probability{0.01f};    // Probability of packet reordering
    u32 reorder_distance{2};           // How far packets can be reordered
    
    // Connection quality degradation
    bool enable_quality_degradation{false}; // Gradually worsen conditions
    f32 degradation_rate{0.001f};      // Rate of quality degradation per second
    f32 recovery_rate{0.01f};          // Rate of quality recovery
    
    // Educational features
    bool enable_visualization{true};    // Enable real-time visualization
    bool log_dropped_packets{true};    // Log dropped packets for analysis
    bool detailed_statistics{true};    // Collect detailed performance metrics
    
    /** @brief Create mobile 3G simulation profile */
    static NetworkSimulationConfig mobile_3g() {
        NetworkSimulationConfig config;
        config.latency_model = LatencyModel::Normal;
        config.base_latency_ms = 200.0f;
        config.latency_variance_ms = 50.0f;
        config.packet_loss_rate = 0.02f;
        config.bandwidth_kbps = 384;  // 3G bandwidth
        config.enable_jitter = true;
        config.jitter_variance_ms = 30.0f;
        return config;
    }
    
    /** @brief Create mobile 4G simulation profile */
    static NetworkSimulationConfig mobile_4g() {
        NetworkSimulationConfig config;
        config.latency_model = LatencyModel::Normal;
        config.base_latency_ms = 50.0f;
        config.latency_variance_ms = 20.0f;
        config.packet_loss_rate = 0.005f;
        config.bandwidth_kbps = 10000; // 4G bandwidth
        config.enable_jitter = true;
        config.jitter_variance_ms = 10.0f;
        return config;
    }
    
    /** @brief Create WiFi simulation profile */
    static NetworkSimulationConfig wifi() {
        NetworkSimulationConfig config;
        config.latency_model = LatencyModel::Spike;
        config.base_latency_ms = 20.0f;
        config.latency_variance_ms = 5.0f;
        config.spike_latency_ms = 100.0f;
        config.spike_frequency = 0.05f;
        config.packet_loss_rate = 0.001f;
        config.bandwidth_kbps = 54000; // 802.11g bandwidth
        return config;
    }
    
    /** @brief Create high-loss stress test profile */
    static NetworkSimulationConfig stress_test_high_loss() {
        NetworkSimulationConfig config;
        config.latency_model = LatencyModel::Normal;
        config.base_latency_ms = 100.0f;
        config.latency_variance_ms = 30.0f;
        config.loss_model = PacketLossModel::Burst;
        config.packet_loss_rate = 0.1f; // 10% packet loss
        config.burst_loss_count = 5;
        config.burst_loss_probability = 0.2f;
        config.bandwidth_kbps = 500;
        return config;
    }
    
    /** @brief Create educational demonstration profile */
    static NetworkSimulationConfig educational_demo() {
        NetworkSimulationConfig config;
        config.latency_model = LatencyModel::Constant;
        config.base_latency_ms = 100.0f; // Visible latency for education
        config.packet_loss_rate = 0.05f; // Noticeable packet loss
        config.bandwidth_kbps = 1000;
        config.enable_visualization = true;
        config.detailed_statistics = true;
        config.log_dropped_packets = true;
        return config;
    }
};

//=============================================================================
// Delayed Packet Management
//=============================================================================

/**
 * @brief Delayed Packet Entry
 * 
 * Represents a packet that has been artificially delayed and is waiting
 * to be delivered according to the simulation parameters.
 */
struct DelayedPacket {
    std::vector<u8> packet_data;       // Packet contents
    NetworkAddress destination;        // Destination address  
    NetworkTimestamp original_send_time; // When packet was originally sent
    NetworkTimestamp delivery_time;    // When packet should be delivered
    u32 packet_id;                    // Unique packet identifier
    bool is_dropped{false};           // Whether packet should be dropped
    
    /** @brief Check if packet is ready for delivery */
    bool is_ready_for_delivery(NetworkTimestamp current_time) const {
        return !is_dropped && current_time >= delivery_time;
    }
    
    /** @brief Calculate actual delay experienced */
    u64 get_actual_delay_us(NetworkTimestamp current_time) const {
        return current_time - original_send_time;
    }
};

/**
 * @brief Packet Delay Queue
 * 
 * Priority queue that manages packets waiting to be delivered,
 * ordered by their scheduled delivery time.
 */
class PacketDelayQueue {
private:
    struct PacketComparator {
        bool operator()(const DelayedPacket& a, const DelayedPacket& b) const {
            return a.delivery_time > b.delivery_time; // Min-heap (earliest delivery first)
        }
    };
    
    std::priority_queue<DelayedPacket, std::vector<DelayedPacket>, PacketComparator> queue_;
    u32 next_packet_id_{1};
    
    // Statistics
    mutable u64 packets_queued_{0};
    mutable u64 packets_delivered_{0};
    mutable u64 packets_dropped_{0};
    mutable u64 total_delay_us_{0};
    
public:
    /** @brief Add packet to delay queue */
    void enqueue_packet(const std::vector<u8>& packet_data,
                       const NetworkAddress& destination,
                       NetworkTimestamp delivery_time) {
        DelayedPacket delayed_packet;
        delayed_packet.packet_data = packet_data;
        delayed_packet.destination = destination;
        delayed_packet.original_send_time = timing::now();
        delayed_packet.delivery_time = delivery_time;
        delayed_packet.packet_id = next_packet_id_++;
        
        queue_.push(delayed_packet);
        packets_queued_++;
    }
    
    /** @brief Mark packet as dropped (simulate packet loss) */
    void drop_packet(u32 packet_id) {
        // Note: Can't efficiently modify std::priority_queue
        // In real implementation, would use custom data structure
        packets_dropped_++;
    }
    
    /** @brief Get next ready packet */
    std::optional<DelayedPacket> get_next_ready_packet(NetworkTimestamp current_time) {
        while (!queue_.empty()) {
            const DelayedPacket& next_packet = queue_.top();
            
            if (next_packet.is_ready_for_delivery(current_time)) {
                DelayedPacket packet = next_packet;
                queue_.pop();
                
                if (!packet.is_dropped) {
                    packets_delivered_++;
                    total_delay_us_ += packet.get_actual_delay_us(current_time);
                    return packet;
                } else {
                    // Packet was dropped, continue to next
                    continue;
                }
            } else {
                // No more ready packets
                break;
            }
        }
        
        return std::nullopt;
    }
    
    /** @brief Get all ready packets */
    std::vector<DelayedPacket> get_all_ready_packets(NetworkTimestamp current_time) {
        std::vector<DelayedPacket> ready_packets;
        
        while (auto packet = get_next_ready_packet(current_time)) {
            ready_packets.push_back(*packet);
        }
        
        return ready_packets;
    }
    
    /** @brief Get queue size */
    usize size() const {
        return queue_.size();
    }
    
    /** @brief Check if queue is empty */
    bool empty() const {
        return queue_.empty();
    }
    
    /** @brief Get queue statistics */
    struct Statistics {
        u64 packets_queued;
        u64 packets_delivered;
        u64 packets_dropped;
        f64 average_delay_ms;
        f32 packet_loss_rate;
        usize current_queue_size;
    };
    
    Statistics get_statistics() const {
        f64 avg_delay_ms = packets_delivered_ > 0 ? 
                          static_cast<f64>(total_delay_us_) / packets_delivered_ / 1000.0 : 0.0;
        
        f32 loss_rate = packets_queued_ > 0 ?
                       static_cast<f32>(packets_dropped_) / packets_queued_ : 0.0f;
        
        return Statistics{
            .packets_queued = packets_queued_,
            .packets_delivered = packets_delivered_,
            .packets_dropped = packets_dropped_,
            .average_delay_ms = avg_delay_ms,
            .packet_loss_rate = loss_rate,
            .current_queue_size = queue_.size()
        };
    }
    
    /** @brief Clear all packets and reset statistics */
    void clear() {
        while (!queue_.empty()) {
            queue_.pop();
        }
        packets_queued_ = 0;
        packets_delivered_ = 0;
        packets_dropped_ = 0;
        total_delay_us_ = 0;
        next_packet_id_ = 1;
    }
};

//=============================================================================
// Network Condition Simulators
//=============================================================================

/**
 * @brief Latency Simulator
 * 
 * Generates realistic network latency values based on configured models.
 */
class LatencySimulator {
private:
    NetworkSimulationConfig config_;
    mutable std::random_device rd_;
    mutable std::mt19937 gen_;
    mutable std::normal_distribution<f32> normal_dist_;
    mutable std::uniform_real_distribution<f32> uniform_dist_;
    mutable std::exponential_distribution<f32> exponential_dist_;
    
    // Spike simulation state
    mutable NetworkTimestamp last_spike_time_{0};
    mutable f32 spike_phase_{0.0f};
    
public:
    /** @brief Initialize with configuration */
    explicit LatencySimulator(const NetworkSimulationConfig& config)
        : config_(config), gen_(rd_()) {
        update_distributions();
    }
    
    /** @brief Generate latency value in microseconds */
    u64 generate_latency_us() const {
        f32 latency_ms = 0.0f;
        
        switch (config_.latency_model) {
            case LatencyModel::Constant:
                latency_ms = config_.base_latency_ms;
                break;
                
            case LatencyModel::Uniform: {
                f32 min_latency = config_.base_latency_ms - config_.latency_variance_ms;
                f32 max_latency = config_.base_latency_ms + config_.latency_variance_ms;
                std::uniform_real_distribution<f32> dist(min_latency, max_latency);
                latency_ms = dist(gen_);
                break;
            }
            
            case LatencyModel::Normal: {
                std::normal_distribution<f32> dist(config_.base_latency_ms, config_.latency_variance_ms);
                latency_ms = std::max(0.0f, dist(gen_)); // Clamp to positive values
                break;
            }
            
            case LatencyModel::Exponential: {
                std::exponential_distribution<f32> dist(1.0f / config_.base_latency_ms);
                latency_ms = dist(gen_);
                break;
            }
            
            case LatencyModel::Spike: {
                NetworkTimestamp current_time = timing::now();
                f32 time_since_spike = (current_time - last_spike_time_) / 1000000.0f; // Convert to seconds
                
                // Check if we should generate a spike
                if (uniform_dist_(gen_) < config_.spike_frequency * 0.016f) { // Assuming ~60 FPS
                    last_spike_time_ = current_time;
                    latency_ms = config_.spike_latency_ms;
                } else {
                    latency_ms = config_.base_latency_ms + 
                               normal_dist_(gen_) * config_.latency_variance_ms;
                }
                break;
            }
            
            case LatencyModel::Custom:
                // Would call custom function here
                latency_ms = config_.base_latency_ms;
                break;
        }
        
        return static_cast<u64>(std::max(0.0f, latency_ms) * 1000.0f); // Convert to microseconds
    }
    
    /** @brief Update configuration */
    void update_config(const NetworkSimulationConfig& config) {
        config_ = config;
        update_distributions();
    }

private:
    /** @brief Update random distributions based on config */
    void update_distributions() {
        normal_dist_ = std::normal_distribution<f32>(0.0f, 1.0f); // Normalized distribution
        uniform_dist_ = std::uniform_real_distribution<f32>(0.0f, 1.0f);
        exponential_dist_ = std::exponential_distribution<f32>(1.0f);
    }
};

/**
 * @brief Packet Loss Simulator
 * 
 * Determines whether packets should be dropped based on configured loss models.
 */
class PacketLossSimulator {
private:
    NetworkSimulationConfig config_;
    mutable std::random_device rd_;
    mutable std::mt19937 gen_;
    mutable std::uniform_real_distribution<f32> uniform_dist_;
    
    // Burst loss state
    mutable u32 burst_packets_remaining_{0};
    
    // Gilbert-Elliott state
    mutable bool in_good_state_{true};
    mutable NetworkTimestamp last_state_change_{0};
    
public:
    /** @brief Initialize with configuration */
    explicit PacketLossSimulator(const NetworkSimulationConfig& config)
        : config_(config), gen_(rd_()), uniform_dist_(0.0f, 1.0f) {}
    
    /** @brief Determine if packet should be dropped */
    bool should_drop_packet() const {
        switch (config_.loss_model) {
            case PacketLossModel::Random:
                return uniform_dist_(gen_) < config_.packet_loss_rate;
                
            case PacketLossModel::Burst: {
                if (burst_packets_remaining_ > 0) {
                    burst_packets_remaining_--;
                    return true;
                }
                
                // Check if we should start a new burst
                if (uniform_dist_(gen_) < config_.burst_loss_probability) {
                    burst_packets_remaining_ = config_.burst_loss_count - 1; // -1 because we're dropping this one
                    return true;
                }
                
                return false;
            }
            
            case PacketLossModel::Periodic: {
                // Simple periodic loss based on packet count
                static u64 packet_count = 0;
                packet_count++;
                u32 period = static_cast<u32>(1.0f / config_.packet_loss_rate);
                return (packet_count % period) == 0;
            }
            
            case PacketLossModel::Congestion: {
                // Loss rate increases with simulated network load
                // This would need integration with bandwidth simulation
                f32 congestion_factor = 1.0f; // Simplified
                f32 adjusted_loss_rate = config_.packet_loss_rate * congestion_factor;
                return uniform_dist_(gen_) < adjusted_loss_rate;
            }
            
            case PacketLossModel::GilbertElliott: {
                NetworkTimestamp current_time = timing::now();
                
                // Simple two-state model
                f32 good_to_bad_rate = config_.packet_loss_rate * 10.0f;  // Transition rate
                f32 bad_to_good_rate = 1.0f - config_.packet_loss_rate;   // Recovery rate
                
                // Check for state transitions
                if (in_good_state_) {
                    if (uniform_dist_(gen_) < good_to_bad_rate * 0.016f) { // Assuming ~60 FPS
                        in_good_state_ = false;
                        last_state_change_ = current_time;
                    }
                    return false; // No loss in good state
                } else {
                    if (uniform_dist_(gen_) < bad_to_good_rate * 0.016f) { // Assuming ~60 FPS
                        in_good_state_ = true;
                        last_state_change_ = current_time;
                        return false;
                    }
                    return uniform_dist_(gen_) < 0.5f; // High loss rate in bad state
                }
            }
        }
        
        return false;
    }
    
    /** @brief Update configuration */
    void update_config(const NetworkSimulationConfig& config) {
        config_ = config;
    }
};

//=============================================================================
// Main Network Simulator
//=============================================================================

/**
 * @brief Comprehensive Network Simulator
 * 
 * Main class that orchestrates all network condition simulation and provides
 * educational visualization and analysis capabilities.
 */
class NetworkSimulator {
private:
    NetworkSimulationConfig config_;
    LatencySimulator latency_simulator_;
    PacketLossSimulator loss_simulator_;
    PacketDelayQueue delay_queue_;
    
    // Bandwidth throttling
    struct BandwidthThrottle {
        u32 available_bytes{0};         // Available bytes in current window
        NetworkTimestamp last_refill{0}; // Last time bytes were refilled
        u32 bytes_per_ms{0};           // Bytes available per millisecond
    };
    BandwidthThrottle bandwidth_throttle_;
    
    // Statistics and monitoring
    struct SimulationStatistics {
        u64 total_packets_processed{0};
        u64 packets_delayed{0};
        u64 packets_dropped{0};
        u64 total_latency_added_us{0};
        f64 average_latency_ms{0.0};
        f32 effective_packet_loss_rate{0.0f};
        u32 current_bandwidth_usage{0};
        NetworkTimestamp simulation_start_time{0};
    };
    mutable SimulationStatistics stats_;
    
    // Visualization data
    std::vector<std::pair<NetworkTimestamp, f32>> latency_history_;
    std::vector<std::pair<NetworkTimestamp, f32>> loss_rate_history_;
    std::vector<std::pair<NetworkTimestamp, u32>> bandwidth_history_;
    
public:
    /** @brief Initialize network simulator */
    explicit NetworkSimulator(const NetworkSimulationConfig& config = NetworkSimulationConfig{})
        : config_(config),
          latency_simulator_(config),
          loss_simulator_(config) {
        
        update_bandwidth_throttle();
        stats_.simulation_start_time = timing::now();
    }
    
    //-------------------------------------------------------------------------
    // Packet Processing
    //-------------------------------------------------------------------------
    
    /** @brief Process outgoing packet through simulation */
    bool process_outgoing_packet(const std::vector<u8>& packet_data,
                                const NetworkAddress& destination) {
        NetworkTimestamp current_time = timing::now();
        stats_.total_packets_processed++;
        
        // Check bandwidth throttling
        if (!check_bandwidth_allowance(packet_data.size(), current_time)) {
            // Packet would exceed bandwidth - could queue or drop
            stats_.packets_dropped++;
            return false; // Packet dropped due to bandwidth
        }
        
        // Check packet loss
        if (loss_simulator_.should_drop_packet()) {
            stats_.packets_dropped++;
            if (config_.log_dropped_packets) {
                // Would log packet drop for analysis
            }
            return false; // Packet dropped
        }
        
        // Apply latency
        u64 latency_us = latency_simulator_.generate_latency_us();
        NetworkTimestamp delivery_time = current_time + latency_us;
        
        // Add to delay queue
        delay_queue_.enqueue_packet(packet_data, destination, delivery_time);
        
        stats_.packets_delayed++;
        stats_.total_latency_added_us += latency_us;
        
        // Update statistics
        update_simulation_statistics(current_time);
        
        return true; // Packet accepted for transmission
    }
    
    /** @brief Get packets ready for actual transmission */
    std::vector<DelayedPacket> get_ready_packets() {
        NetworkTimestamp current_time = timing::now();
        return delay_queue_.get_all_ready_packets(current_time);
    }
    
    /** @brief Update simulation (call regularly) */
    void update() {
        NetworkTimestamp current_time = timing::now();
        
        // Update bandwidth throttle
        update_bandwidth_throttle();
        
        // Update quality degradation if enabled
        if (config_.enable_quality_degradation) {
            update_quality_degradation(current_time);
        }
        
        // Update visualization data
        if (config_.enable_visualization) {
            update_visualization_data(current_time);
        }
        
        // Update statistics
        update_simulation_statistics(current_time);
    }
    
    //-------------------------------------------------------------------------
    // Configuration and Control
    //-------------------------------------------------------------------------
    
    /** @brief Update simulation configuration */
    void set_config(const NetworkSimulationConfig& config) {
        config_ = config;
        latency_simulator_.update_config(config);
        loss_simulator_.update_config(config);
        update_bandwidth_throttle();
    }
    
    /** @brief Get current configuration */
    const NetworkSimulationConfig& config() const {
        return config_;
    }
    
    /** @brief Reset simulation state */
    void reset() {
        delay_queue_.clear();
        stats_ = SimulationStatistics{};
        stats_.simulation_start_time = timing::now();
        latency_history_.clear();
        loss_rate_history_.clear();
        bandwidth_history_.clear();
    }
    
    //-------------------------------------------------------------------------
    // Statistics and Monitoring
    //-------------------------------------------------------------------------
    
    /** @brief Get comprehensive simulation statistics */
    struct Statistics {
        u64 total_packets_processed;
        u64 packets_delayed;
        u64 packets_dropped;
        f64 average_latency_ms;
        f32 effective_packet_loss_rate;
        u32 current_bandwidth_usage_kbps;
        f64 simulation_duration_seconds;
        PacketDelayQueue::Statistics queue_stats;
    };
    
    Statistics get_statistics() const {
        NetworkTimestamp current_time = timing::now();
        f64 simulation_duration = (current_time - stats_.simulation_start_time) / 1000000.0; // Convert to seconds
        
        return Statistics{
            .total_packets_processed = stats_.total_packets_processed,
            .packets_delayed = stats_.packets_delayed,
            .packets_dropped = stats_.packets_dropped,
            .average_latency_ms = stats_.average_latency_ms,
            .effective_packet_loss_rate = stats_.effective_packet_loss_rate,
            .current_bandwidth_usage_kbps = stats_.current_bandwidth_usage,
            .simulation_duration_seconds = simulation_duration,
            .queue_stats = delay_queue_.get_statistics()
        };
    }
    
    /** @brief Get latency history for visualization */
    const std::vector<std::pair<NetworkTimestamp, f32>>& get_latency_history() const {
        return latency_history_;
    }
    
    /** @brief Get packet loss history for visualization */
    const std::vector<std::pair<NetworkTimestamp, f32>>& get_loss_history() const {
        return loss_rate_history_;
    }
    
    /** @brief Get bandwidth usage history for visualization */
    const std::vector<std::pair<NetworkTimestamp, u32>>& get_bandwidth_history() const {
        return bandwidth_history_;
    }

private:
    //-------------------------------------------------------------------------
    // Internal Implementation
    //-------------------------------------------------------------------------
    
    /** @brief Check if packet fits within bandwidth allowance */
    bool check_bandwidth_allowance(usize packet_size, NetworkTimestamp current_time) {
        // Refill bandwidth tokens
        u64 time_delta_ms = (current_time - bandwidth_throttle_.last_refill) / 1000;
        if (time_delta_ms > 0) {
            u32 bytes_to_add = static_cast<u32>(time_delta_ms * bandwidth_throttle_.bytes_per_ms);
            bandwidth_throttle_.available_bytes = std::min(
                bandwidth_throttle_.available_bytes + bytes_to_add,
                config_.burst_capacity_kb * 1024
            );
            bandwidth_throttle_.last_refill = current_time;
        }
        
        // Check if packet fits
        if (packet_size <= bandwidth_throttle_.available_bytes) {
            bandwidth_throttle_.available_bytes -= static_cast<u32>(packet_size);
            return true;
        }
        
        return false;
    }
    
    /** @brief Update bandwidth throttle parameters */
    void update_bandwidth_throttle() {
        bandwidth_throttle_.bytes_per_ms = config_.bandwidth_kbps * 1024 / 1000; // Convert kbps to bytes/ms
        
        if (bandwidth_throttle_.last_refill == 0) {
            bandwidth_throttle_.last_refill = timing::now();
            bandwidth_throttle_.available_bytes = config_.burst_capacity_kb * 1024;
        }
    }
    
    /** @brief Update quality degradation simulation */
    void update_quality_degradation(NetworkTimestamp current_time) {
        // Simplified quality degradation - would be more sophisticated in practice
        static f32 quality_factor = 1.0f;
        
        // Gradually degrade quality
        quality_factor -= config_.degradation_rate * 0.016f; // Assuming ~60 FPS
        quality_factor = std::max(0.1f, quality_factor);
        
        // Apply degradation to simulation parameters
        // This would modify config_ values based on quality_factor
    }
    
    /** @brief Update visualization data */
    void update_visualization_data(NetworkTimestamp current_time) {
        constexpr usize MAX_HISTORY_ENTRIES = 1000; // Keep last 1000 entries
        
        // Sample current metrics
        f32 current_latency = config_.base_latency_ms;
        f32 current_loss_rate = stats_.effective_packet_loss_rate;
        u32 current_bandwidth = stats_.current_bandwidth_usage;
        
        // Add to history
        latency_history_.emplace_back(current_time, current_latency);
        loss_rate_history_.emplace_back(current_time, current_loss_rate);
        bandwidth_history_.emplace_back(current_time, current_bandwidth);
        
        // Trim old entries
        if (latency_history_.size() > MAX_HISTORY_ENTRIES) {
            latency_history_.erase(latency_history_.begin());
        }
        if (loss_rate_history_.size() > MAX_HISTORY_ENTRIES) {
            loss_rate_history_.erase(loss_rate_history_.begin());
        }
        if (bandwidth_history_.size() > MAX_HISTORY_ENTRIES) {
            bandwidth_history_.erase(bandwidth_history_.begin());
        }
    }
    
    /** @brief Update simulation statistics */
    void update_simulation_statistics(NetworkTimestamp current_time) {
        // Update average latency
        if (stats_.packets_delayed > 0) {
            stats_.average_latency_ms = static_cast<f64>(stats_.total_latency_added_us) / 
                                       stats_.packets_delayed / 1000.0; // Convert to ms
        }
        
        // Update effective packet loss rate
        if (stats_.total_packets_processed > 0) {
            stats_.effective_packet_loss_rate = static_cast<f32>(stats_.packets_dropped) / 
                                               stats_.total_packets_processed;
        }
        
        // Update current bandwidth usage (simplified)
        stats_.current_bandwidth_usage = config_.bandwidth_kbps - 
            (bandwidth_throttle_.available_bytes * 8 / 1024); // Convert to kbps
    }
};

} // namespace ecscope::networking::simulation