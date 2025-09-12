#pragma once

#include <vector>
#include <string>
#include <memory>
#include <unordered_map>
#include <functional>
#include <chrono>
#include <mutex>

#ifdef ECSCOPE_HAS_IMGUI
#include <imgui.h>
#endif

namespace ecscope {
namespace gui {

enum class ConnectionState {
    Disconnected,
    Connecting,
    Connected,
    Reconnecting,
    Failed,
    Timeout
};

enum class NetworkProtocol {
    TCP,
    UDP,
    WebSocket,
    HTTP,
    Custom
};

enum class PacketType {
    Handshake,
    GameData,
    PlayerInput,
    WorldSync,
    Chat,
    Voice,
    File,
    Custom
};

struct NetworkConnection {
    uint32_t id;
    std::string name;
    std::string address;
    uint16_t port;
    NetworkProtocol protocol;
    ConnectionState state;
    float ping_ms;
    uint64_t bytes_sent;
    uint64_t bytes_received;
    uint32_t packets_sent;
    uint32_t packets_received;
    uint32_t packets_lost;
    float packet_loss_rate;
    std::chrono::steady_clock::time_point last_activity;
    bool is_server;
    ImU32 connection_color;
};

struct NetworkPacket {
    uint32_t id;
    PacketType type;
    uint32_t connection_id;
    size_t size;
    std::chrono::steady_clock::time_point timestamp;
    bool is_outgoing;
    std::string description;
};

struct NetworkStatistics {
    uint64_t total_bytes_sent;
    uint64_t total_bytes_received;
    uint32_t total_connections;
    uint32_t active_connections;
    float average_ping;
    float total_packet_loss;
    uint32_t packets_per_second;
    float bandwidth_usage;
    std::vector<float> ping_history;
    std::vector<float> bandwidth_history;
};

struct ServerConfiguration {
    std::string name;
    uint16_t port;
    uint32_t max_connections;
    bool auto_start;
    float tick_rate;
    bool enable_compression;
    bool enable_encryption;
    std::string password;
    std::vector<std::string> allowed_ips;
    std::unordered_map<std::string, std::string> custom_settings;
};

class NetworkVisualizer {
public:
    NetworkVisualizer() = default;
    ~NetworkVisualizer() = default;

    void initialize();
    void render_connection_graph();
    void render_packet_flow();
    void render_network_topology();
    void update_visualization_data(const std::vector<NetworkConnection>& connections,
                                 const std::vector<NetworkPacket>& recent_packets);

private:
    struct ConnectionNode {
        ImVec2 position;
        ImVec2 velocity;
        float radius;
        ImU32 color;
        bool is_selected;
    };

    std::unordered_map<uint32_t, ConnectionNode> connection_nodes_;
    std::vector<std::pair<uint32_t, uint32_t>> connection_links_;
    bool auto_layout_;
    float node_separation_;
    float spring_strength_;
    float damping_;
};

class NetworkPacketInspector {
public:
    NetworkPacketInspector() = default;
    ~NetworkPacketInspector() = default;

    void initialize();
    void render();
    void add_packet(const NetworkPacket& packet, const std::vector<uint8_t>& data);
    void set_packet_filter(PacketType type, bool show);
    void clear_packet_history();

private:
    struct PacketEntry {
        NetworkPacket packet;
        std::vector<uint8_t> data;
        bool is_expanded;
    };

    std::vector<PacketEntry> packet_history_;
    std::unordered_map<PacketType, bool> packet_filters_;
    size_t max_history_size_;
    bool auto_scroll_;
    std::string search_filter_;
};

class NetworkUI {
public:
    NetworkUI();
    ~NetworkUI();

    bool initialize();
    void render();
    void update(float delta_time);
    void shutdown();

    void add_connection(const NetworkConnection& connection);
    void update_connection(uint32_t connection_id, const NetworkConnection& connection);
    void remove_connection(uint32_t connection_id);
    void add_packet(const NetworkPacket& packet, const std::vector<uint8_t>& data);
    void update_statistics(const NetworkStatistics& stats);

    void set_server_configuration(const ServerConfiguration& config);
    ServerConfiguration get_server_configuration() const { return server_config_; }

    void set_connection_callback(std::function<void(const std::string&, uint16_t, NetworkProtocol)> callback);
    void set_disconnect_callback(std::function<void(uint32_t)> callback);
    void set_server_start_callback(std::function<void(const ServerConfiguration&)> callback);
    void set_server_stop_callback(std::function<void()> callback);

    void set_display_mode(int mode) { display_mode_ = mode; }
    void enable_auto_refresh(bool enable) { auto_refresh_ = enable; }
    void set_refresh_rate(float rate) { refresh_rate_ = rate; }

    bool is_window_open() const { return show_window_; }
    void set_window_open(bool open) { show_window_ = open; }

private:
    void render_connection_manager();
    void render_server_controls();
    void render_packet_monitor();
    void render_statistics_panel();
    void render_connection_details();
    void render_network_settings();
    void render_bandwidth_monitor();
    void render_security_panel();

    void update_connection_colors();
    void update_ping_history();
    void calculate_statistics();

    std::vector<NetworkConnection> connections_;
    NetworkStatistics statistics_;
    ServerConfiguration server_config_;
    
    std::unique_ptr<NetworkVisualizer> visualizer_;
    std::unique_ptr<NetworkPacketInspector> packet_inspector_;

    std::function<void(const std::string&, uint16_t, NetworkProtocol)> connection_callback_;
    std::function<void(uint32_t)> disconnect_callback_;
    std::function<void(const ServerConfiguration&)> server_start_callback_;
    std::function<void()> server_stop_callback_;

    bool show_window_;
    bool show_connection_manager_;
    bool show_server_controls_;
    bool show_packet_monitor_;
    bool show_statistics_;
    bool show_visualizer_;
    bool show_bandwidth_monitor_;
    bool show_security_panel_;

    int display_mode_;
    bool auto_refresh_;
    float refresh_rate_;
    float last_refresh_time_;

    uint32_t selected_connection_id_;
    bool server_running_;
    
    char connection_address_[256];
    int connection_port_;
    int connection_protocol_;
    
    std::mutex connections_mutex_;
    std::mutex statistics_mutex_;

    ImVec4 connection_colors_[6] = {
        {0.2f, 0.8f, 0.2f, 1.0f},
        {0.8f, 0.8f, 0.2f, 1.0f}, 
        {0.8f, 0.2f, 0.2f, 1.0f},
        {0.2f, 0.6f, 0.8f, 1.0f},
        {0.8f, 0.4f, 0.8f, 1.0f},
        {0.6f, 0.6f, 0.6f, 1.0f}
    };
};

class NetworkManager {
public:
    static NetworkManager& instance() {
        static NetworkManager instance;
        return instance;
    }

    void initialize();
    void shutdown();
    void update(float delta_time);

    void register_network_ui(NetworkUI* ui);
    void unregister_network_ui(NetworkUI* ui);

    void notify_connection_changed(const NetworkConnection& connection);
    void notify_packet_received(const NetworkPacket& packet, const std::vector<uint8_t>& data);
    void notify_statistics_updated(const NetworkStatistics& stats);

private:
    NetworkManager() = default;
    ~NetworkManager() = default;

    std::vector<NetworkUI*> registered_uis_;
    std::mutex ui_mutex_;
};

}} // namespace ecscope::gui