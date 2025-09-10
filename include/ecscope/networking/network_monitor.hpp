#pragma once

#include "network_types.hpp"
#include <vector>
#include <memory>
#include <string>
#include <unordered_map>
#include <chrono>
#include <functional>
#include <atomic>
#include <thread>
#include <condition_variable>

namespace ecscope::networking {

/**
 * @brief Network Performance Metrics
 */
struct NetworkMetrics {
    // Latency metrics (microseconds)
    struct LatencyMetrics {
        uint64_t min_latency = UINT64_MAX;
        uint64_t max_latency = 0;
        uint64_t average_latency = 0;
        uint64_t current_latency = 0;
        uint64_t p95_latency = 0; // 95th percentile
        uint64_t p99_latency = 0; // 99th percentile
        uint64_t jitter = 0;      // Latency variation
        uint32_t sample_count = 0;
        
        void update(uint64_t new_latency) {
            if (new_latency < min_latency) min_latency = new_latency;
            if (new_latency > max_latency) max_latency = new_latency;
            
            // Simple moving average (could be improved with circular buffer)
            average_latency = (average_latency * sample_count + new_latency) / (sample_count + 1);
            current_latency = new_latency;
            sample_count++;
        }
    };
    
    // Throughput metrics (bytes per second)
    struct ThroughputMetrics {
        uint64_t bytes_sent_per_second = 0;
        uint64_t bytes_received_per_second = 0;
        uint64_t packets_sent_per_second = 0;
        uint64_t packets_received_per_second = 0;
        uint64_t peak_send_rate = 0;
        uint64_t peak_receive_rate = 0;
        double bandwidth_utilization = 0.0; // 0.0 to 1.0
        
        void update_bandwidth_utilization(uint64_t available_bandwidth) {
            if (available_bandwidth > 0) {
                bandwidth_utilization = static_cast<double>(bytes_sent_per_second + bytes_received_per_second) / 
                                      static_cast<double>(available_bandwidth);
            }
        }
    };
    
    // Packet loss metrics
    struct PacketLossMetrics {
        uint64_t packets_sent = 0;
        uint64_t packets_received = 0;
        uint64_t packets_lost = 0;
        uint64_t packets_duplicate = 0;
        uint64_t packets_out_of_order = 0;
        uint64_t packets_retransmitted = 0;
        double loss_rate = 0.0; // 0.0 to 1.0
        
        void update_loss_rate() {
            if (packets_sent > 0) {
                loss_rate = static_cast<double>(packets_lost) / static_cast<double>(packets_sent);
            }
        }
    };
    
    // Connection quality metrics
    struct QualityMetrics {
        double connection_stability = 1.0; // 0.0 to 1.0
        double data_integrity = 1.0;       // 0.0 to 1.0
        double overall_quality = 1.0;      // 0.0 to 1.0
        uint32_t disconnection_count = 0;
        uint32_t error_count = 0;
        
        void calculate_overall_quality(const LatencyMetrics& latency, 
                                     const PacketLossMetrics& packet_loss) {
            // Simple quality calculation (can be improved)
            double latency_factor = std::max(0.0, 1.0 - (latency.average_latency / 200000.0)); // 200ms baseline
            double loss_factor = std::max(0.0, 1.0 - packet_loss.loss_rate * 10.0);
            double stability_factor = connection_stability;
            
            overall_quality = (latency_factor * 0.4 + loss_factor * 0.4 + stability_factor * 0.2);
            overall_quality = std::clamp(overall_quality, 0.0, 1.0);
        }
    };
    
    LatencyMetrics latency;
    ThroughputMetrics throughput;
    PacketLossMetrics packet_loss;
    QualityMetrics quality;
    
    // Timestamps
    NetworkTimestamp first_measurement_time = 0;
    NetworkTimestamp last_update_time = 0;
    
    // Reset all metrics
    void reset() {
        *this = NetworkMetrics{};
    }
};

/**
 * @brief Network Event for monitoring
 */
struct NetworkMonitorEvent {
    enum Type : uint8_t {
        CONNECTION_ESTABLISHED = 0,
        CONNECTION_LOST = 1,
        HIGH_LATENCY = 2,
        PACKET_LOSS_THRESHOLD = 3,
        BANDWIDTH_SATURATION = 4,
        ERROR_OCCURRED = 5,
        QUALITY_DEGRADATION = 6,
        HEARTBEAT_TIMEOUT = 7,
        CUSTOM_EVENT = 255
    };
    
    Type event_type;
    ConnectionId connection_id;
    NetworkTimestamp timestamp;
    std::string description;
    std::unordered_map<std::string, double> parameters;
    
    NetworkMonitorEvent(Type type, ConnectionId conn_id, const std::string& desc = "") 
        : event_type(type), connection_id(conn_id), timestamp(timing::now()), description(desc) {}
};

/**
 * @brief Connection Monitor
 * 
 * Monitors a single network connection and collects performance metrics.
 */
class ConnectionMonitor {
public:
    struct MonitorConfig {
        std::chrono::milliseconds measurement_interval{100};  // How often to update metrics
        std::chrono::seconds history_retention{300};          // How long to keep history (5 minutes)
        
        // Threshold settings for alerts
        uint64_t high_latency_threshold_us = 100000;    // 100ms
        double packet_loss_threshold = 0.05;            // 5%
        double bandwidth_utilization_threshold = 0.9;   // 90%
        double quality_threshold = 0.7;                 // 70%
        
        bool enable_alerts = true;
        bool collect_detailed_history = true;
    };
    
    explicit ConnectionMonitor(ConnectionId connection_id, const MonitorConfig& config = {});
    ~ConnectionMonitor() = default;
    
    // Metric collection
    void record_packet_sent(size_t size, NetworkTimestamp timestamp = 0);
    void record_packet_received(size_t size, NetworkTimestamp timestamp = 0);
    void record_packet_lost(NetworkTimestamp timestamp = 0);
    void record_latency_sample(uint64_t latency_us, NetworkTimestamp timestamp = 0);
    void record_error(const std::string& error_description, NetworkTimestamp timestamp = 0);
    void record_disconnection(NetworkTimestamp timestamp = 0);
    
    // Metrics access
    const NetworkMetrics& get_current_metrics() const { return current_metrics_; }
    NetworkMetrics get_metrics_snapshot() const;
    
    // Historical data
    std::vector<NetworkMetrics> get_metrics_history(std::chrono::seconds duration) const;
    std::vector<NetworkMonitorEvent> get_event_history(std::chrono::seconds duration) const;
    
    // Configuration
    void set_config(const MonitorConfig& config) { config_ = config; }
    const MonitorConfig& get_config() const { return config_; }
    
    // Alert system
    using AlertCallback = std::function<void(const NetworkMonitorEvent&)>;
    void set_alert_callback(AlertCallback callback);
    
    // Control
    void start();
    void stop();
    void reset();

private:
    struct MetricsHistory {
        NetworkTimestamp timestamp;
        NetworkMetrics metrics;
    };
    
    ConnectionId connection_id_;
    MonitorConfig config_;
    
    // Current state
    NetworkMetrics current_metrics_;
    mutable std::shared_mutex metrics_mutex_;
    
    // Historical data
    std::vector<MetricsHistory> metrics_history_;
    std::vector<NetworkMonitorEvent> event_history_;
    mutable std::shared_mutex history_mutex_;
    
    // Alert system
    AlertCallback alert_callback_;
    std::mutex alert_mutex_;
    
    // Update timing
    std::chrono::steady_clock::time_point last_update_time_;
    std::chrono::steady_clock::time_point last_throughput_calculation_;
    uint64_t bytes_sent_last_second_ = 0;
    uint64_t bytes_received_last_second_ = 0;
    uint64_t packets_sent_last_second_ = 0;
    uint64_t packets_received_last_second_ = 0;
    
    // Helper functions
    void update_throughput_metrics();
    void check_alert_conditions();
    void cleanup_old_history();
    void trigger_alert(NetworkMonitorEvent::Type type, const std::string& description, 
                      const std::unordered_map<std::string, double>& parameters = {});
};

/**
 * @brief Network Monitor Manager
 * 
 * Manages monitoring for multiple connections and provides aggregated statistics.
 */
class NetworkMonitorManager {
public:
    struct GlobalConfig {
        std::chrono::milliseconds update_interval{1000};      // Global update frequency
        std::chrono::seconds statistics_retention{3600};     // 1 hour
        size_t max_monitored_connections = 1000;
        bool enable_global_alerts = true;
        bool auto_cleanup_disconnected = true;
        std::chrono::seconds cleanup_interval{60};           // Cleanup every minute
    };
    
    explicit NetworkMonitorManager(const GlobalConfig& config = {});
    ~NetworkMonitorManager();
    
    // Connection management
    void add_connection(ConnectionId connection_id, const ConnectionMonitor::MonitorConfig& config = {});
    void remove_connection(ConnectionId connection_id);
    bool has_connection(ConnectionId connection_id) const;
    
    // Metric collection (delegates to individual connection monitors)
    void record_packet_sent(ConnectionId connection_id, size_t size, NetworkTimestamp timestamp = 0);
    void record_packet_received(ConnectionId connection_id, size_t size, NetworkTimestamp timestamp = 0);
    void record_packet_lost(ConnectionId connection_id, NetworkTimestamp timestamp = 0);
    void record_latency_sample(ConnectionId connection_id, uint64_t latency_us, NetworkTimestamp timestamp = 0);
    void record_error(ConnectionId connection_id, const std::string& error_description, NetworkTimestamp timestamp = 0);
    void record_disconnection(ConnectionId connection_id, NetworkTimestamp timestamp = 0);
    
    // Individual connection access
    std::shared_ptr<ConnectionMonitor> get_connection_monitor(ConnectionId connection_id) const;
    NetworkMetrics get_connection_metrics(ConnectionId connection_id) const;
    
    // Aggregated statistics
    struct AggregatedMetrics {
        NetworkMetrics combined_metrics;
        size_t active_connections = 0;
        double average_quality = 0.0;
        uint64_t total_bytes_transferred = 0;
        uint64_t total_packets_transferred = 0;
        ConnectionId best_connection = INVALID_CONNECTION_ID;
        ConnectionId worst_connection = INVALID_CONNECTION_ID;
    };
    
    AggregatedMetrics get_aggregated_metrics() const;
    
    // Connection queries
    std::vector<ConnectionId> get_monitored_connections() const;
    std::vector<ConnectionId> get_connections_with_quality_below(double threshold) const;
    std::vector<ConnectionId> get_connections_with_high_latency(uint64_t threshold_us) const;
    std::vector<ConnectionId> get_connections_with_packet_loss_above(double threshold) const;
    
    // Global alert system
    using GlobalAlertCallback = std::function<void(const NetworkMonitorEvent&)>;
    void set_global_alert_callback(GlobalAlertCallback callback);
    
    // Configuration
    void set_global_config(const GlobalConfig& config) { config_ = config; }
    const GlobalConfig& get_global_config() const { return config_; }
    
    // Control
    void start();
    void stop();
    void reset_all_statistics();

private:
    GlobalConfig config_;
    
    // Connection monitors
    std::unordered_map<ConnectionId, std::shared_ptr<ConnectionMonitor>> connection_monitors_;
    mutable std::shared_mutex monitors_mutex_;
    
    // Global alert system
    GlobalAlertCallback global_alert_callback_;
    std::mutex alert_mutex_;
    
    // Update thread
    std::thread update_thread_;
    std::thread cleanup_thread_;
    std::atomic<bool> should_stop_{false};
    std::condition_variable update_cv_;
    std::mutex update_mutex_;
    
    // Threading functions
    void update_thread_function();
    void cleanup_thread_function();
    void handle_connection_alert(const NetworkMonitorEvent& event);
    void cleanup_disconnected_connections();
    
    // Aggregation helpers
    void calculate_aggregated_metrics(AggregatedMetrics& result) const;
};

/**
 * @brief Network Profiler
 * 
 * Advanced profiling tool for detailed network analysis and debugging.
 */
class NetworkProfiler {
public:
    struct ProfileConfig {
        bool enable_packet_capture = false;      // Capture raw packet data
        bool enable_timing_analysis = true;      // Detailed timing measurements
        bool enable_bandwidth_analysis = true;   // Bandwidth usage patterns
        bool enable_protocol_analysis = true;    // Protocol-specific analysis
        size_t max_captured_packets = 10000;     // Memory limit for packet capture
        std::chrono::seconds analysis_window{60}; // Analysis time window
    };
    
    struct PacketInfo {
        NetworkTimestamp timestamp;
        ConnectionId connection_id;
        size_t size;
        bool is_outbound;
        uint16_t message_type;
        std::vector<uint8_t> raw_data; // Only if packet capture enabled
        
        struct TimingInfo {
            NetworkTimestamp send_time = 0;
            NetworkTimestamp receive_time = 0;
            NetworkTimestamp process_time = 0;
            uint64_t serialization_time_us = 0;
            uint64_t compression_time_us = 0;
            uint64_t encryption_time_us = 0;
        } timing;
    };
    
    struct BandwidthAnalysis {
        std::vector<std::pair<NetworkTimestamp, uint64_t>> bandwidth_over_time;
        uint64_t peak_bandwidth = 0;
        uint64_t average_bandwidth = 0;
        uint64_t total_bytes = 0;
        std::chrono::seconds analysis_duration{0};
        
        struct ProtocolBreakdown {
            uint16_t message_type;
            uint64_t bytes_sent = 0;
            uint32_t packet_count = 0;
            double percentage = 0.0;
        };
        std::vector<ProtocolBreakdown> protocol_breakdown;
    };
    
    explicit NetworkProfiler(const ProfileConfig& config = {});
    ~NetworkProfiler() = default;
    
    // Packet recording
    void record_packet_sent(ConnectionId connection_id, const void* data, size_t size, 
                           uint16_t message_type, NetworkTimestamp timestamp = 0);
    void record_packet_received(ConnectionId connection_id, const void* data, size_t size,
                               uint16_t message_type, NetworkTimestamp timestamp = 0);
    
    // Timing measurements
    void record_serialization_time(ConnectionId connection_id, uint64_t time_us);
    void record_compression_time(ConnectionId connection_id, uint64_t time_us);
    void record_encryption_time(ConnectionId connection_id, uint64_t time_us);
    void record_processing_time(ConnectionId connection_id, uint64_t time_us);
    
    // Analysis results
    BandwidthAnalysis get_bandwidth_analysis(ConnectionId connection_id = INVALID_CONNECTION_ID) const;
    std::vector<PacketInfo> get_packet_history(ConnectionId connection_id, 
                                              std::chrono::seconds duration) const;
    
    // Protocol analysis
    struct ProtocolStats {
        uint16_t message_type;
        std::string type_name;
        uint32_t sent_count = 0;
        uint32_t received_count = 0;
        uint64_t sent_bytes = 0;
        uint64_t received_bytes = 0;
        double average_size = 0.0;
        uint64_t min_latency_us = UINT64_MAX;
        uint64_t max_latency_us = 0;
        uint64_t average_latency_us = 0;
    };
    
    std::vector<ProtocolStats> get_protocol_statistics() const;
    
    // Performance bottleneck detection
    struct BottleneckAnalysis {
        enum BottleneckType {
            SERIALIZATION = 0,
            COMPRESSION = 1,
            ENCRYPTION = 2,
            NETWORK_IO = 3,
            PROCESSING = 4
        };
        
        BottleneckType type;
        ConnectionId connection_id;
        double impact_percentage;
        uint64_t average_time_us;
        uint32_t occurrence_count;
        std::string description;
    };
    
    std::vector<BottleneckAnalysis> detect_performance_bottlenecks() const;
    
    // Export and reporting
    std::string generate_performance_report() const;
    bool export_packet_capture(const std::string& filename) const;
    bool export_bandwidth_data(const std::string& filename) const;
    
    // Configuration
    void set_config(const ProfileConfig& config) { config_ = config; }
    const ProfileConfig& get_config() const { return config_; }
    
    // Control
    void start_profiling();
    void stop_profiling();
    void clear_data();
    bool is_profiling() const { return profiling_active_.load(); }

private:
    ProfileConfig config_;
    std::atomic<bool> profiling_active_{false};
    
    // Packet data
    std::vector<PacketInfo> packet_history_;
    mutable std::shared_mutex packet_history_mutex_;
    
    // Analysis data
    std::unordered_map<uint16_t, ProtocolStats> protocol_stats_;
    mutable std::shared_mutex protocol_stats_mutex_;
    
    // Per-connection timing data
    struct ConnectionTimingData {
        std::vector<uint64_t> serialization_times;
        std::vector<uint64_t> compression_times;
        std::vector<uint64_t> encryption_times;
        std::vector<uint64_t> processing_times;
        NetworkTimestamp last_activity;
    };
    
    std::unordered_map<ConnectionId, ConnectionTimingData> connection_timing_;
    mutable std::shared_mutex timing_mutex_;
    
    // Helper functions
    void cleanup_old_data();
    void update_protocol_stats(uint16_t message_type, size_t size, bool is_outbound);
    std::string get_message_type_name(uint16_t message_type) const;
    void analyze_bandwidth_patterns(BandwidthAnalysis& analysis, ConnectionId connection_id) const;
};

/**
 * @brief Network Debug Tools
 * 
 * Collection of debugging utilities for network troubleshooting.
 */
namespace debug_tools {
    
    /**
     * @brief Connection Diagnostics
     */
    struct ConnectionDiagnostics {
        ConnectionId connection_id;
        NetworkAddress local_address;
        NetworkAddress remote_address;
        ConnectionState connection_state;
        NetworkTimestamp connection_start_time;
        NetworkTimestamp last_activity_time;
        
        // Socket information
        bool socket_valid = false;
        int socket_error_code = 0;
        std::string socket_error_message;
        
        // Protocol information
        TransportProtocol protocol;
        bool is_encrypted = false;
        bool is_compressed = false;
        
        // Buffer states
        size_t send_buffer_used = 0;
        size_t send_buffer_size = 0;
        size_t receive_buffer_used = 0;
        size_t receive_buffer_size = 0;
        
        // Recent errors
        std::vector<std::pair<NetworkTimestamp, std::string>> recent_errors;
    };
    
    ConnectionDiagnostics diagnose_connection(ConnectionId connection_id);
    
    /**
     * @brief Network Trace Logger
     */
    class NetworkTraceLogger {
    public:
        enum LogLevel {
            TRACE = 0,
            DEBUG = 1,
            INFO = 2,
            WARN = 3,
            ERROR = 4
        };
        
        static NetworkTraceLogger& instance();
        
        void log(LogLevel level, ConnectionId connection_id, const std::string& message);
        void log_packet(ConnectionId connection_id, bool outbound, const void* data, size_t size);
        
        void set_log_level(LogLevel level) { log_level_ = level; }
        void set_output_file(const std::string& filename);
        void enable_console_output(bool enable) { console_output_ = enable; }
        
        std::vector<std::string> get_recent_logs(size_t count = 100) const;
        
    private:
        NetworkTraceLogger() = default;
        LogLevel log_level_ = INFO;
        bool console_output_ = true;
        std::string log_file_;
        std::vector<std::string> log_buffer_;
        mutable std::mutex log_mutex_;
        
        std::string format_log_message(LogLevel level, ConnectionId connection_id, 
                                     const std::string& message) const;
    };
    
    // Convenience macros for logging
    #define NET_TRACE(conn_id, msg) debug_tools::NetworkTraceLogger::instance().log(debug_tools::NetworkTraceLogger::TRACE, conn_id, msg)
    #define NET_DEBUG(conn_id, msg) debug_tools::NetworkTraceLogger::instance().log(debug_tools::NetworkTraceLogger::DEBUG, conn_id, msg)
    #define NET_INFO(conn_id, msg) debug_tools::NetworkTraceLogger::instance().log(debug_tools::NetworkTraceLogger::INFO, conn_id, msg)
    #define NET_WARN(conn_id, msg) debug_tools::NetworkTraceLogger::instance().log(debug_tools::NetworkTraceLogger::WARN, conn_id, msg)
    #define NET_ERROR(conn_id, msg) debug_tools::NetworkTraceLogger::instance().log(debug_tools::NetworkTraceLogger::ERROR, conn_id, msg)
    
    /**
     * @brief Network Simulator for Testing
     */
    class NetworkSimulator {
    public:
        struct SimulationConfig {
            uint64_t base_latency_us = 50000;    // 50ms base latency
            uint64_t latency_variance_us = 10000; // ±10ms variance
            double packet_loss_rate = 0.01;       // 1% packet loss
            uint64_t bandwidth_limit_bps = 0;     // 0 = unlimited
            bool enable_jitter = true;
            bool enable_congestion = false;
        };
        
        explicit NetworkSimulator(const SimulationConfig& config = {});
        
        // Packet processing simulation
        bool should_drop_packet() const;
        uint64_t calculate_transmission_delay(size_t packet_size) const;
        uint64_t calculate_latency() const;
        
        void set_config(const SimulationConfig& config) { config_ = config; }
        const SimulationConfig& get_config() const { return config_; }
        
    private:
        SimulationConfig config_;
        mutable std::mt19937 random_generator_;
    };
    
} // namespace debug_tools

} // namespace ecscope::networking