#include "ecscope/gui/network_ui.hpp"
#include <algorithm>
#include <iomanip>
#include <sstream>
#include <cstring>

#ifdef ECSCOPE_HAS_IMGUI
#include <imgui.h>
#include <imgui_internal.h>
#endif

namespace ecscope {
namespace gui {

NetworkUI::NetworkUI() 
    : show_window_(true)
    , show_connection_manager_(true)
    , show_server_controls_(true)
    , show_packet_monitor_(false)
    , show_statistics_(true)
    , show_visualizer_(false)
    , show_bandwidth_monitor_(false)
    , show_security_panel_(false)
    , display_mode_(0)
    , auto_refresh_(true)
    , refresh_rate_(1.0f)
    , last_refresh_time_(0.0f)
    , selected_connection_id_(0)
    , server_running_(false)
    , connection_port_(8080)
    , connection_protocol_(0) {
    
    std::strcpy(connection_address_, "localhost");
    
    server_config_.name = "ECScope Game Server";
    server_config_.port = 8080;
    server_config_.max_connections = 32;
    server_config_.auto_start = false;
    server_config_.tick_rate = 60.0f;
    server_config_.enable_compression = true;
    server_config_.enable_encryption = false;
    server_config_.password = "";
}

NetworkUI::~NetworkUI() {
    shutdown();
}

bool NetworkUI::initialize() {
#ifdef ECSCOPE_HAS_IMGUI
    visualizer_ = std::make_unique<NetworkVisualizer>();
    visualizer_->initialize();
    
    packet_inspector_ = std::make_unique<NetworkPacketInspector>();
    packet_inspector_->initialize();
    
    NetworkManager::instance().register_network_ui(this);
    return true;
#else
    return false;
#endif
}

void NetworkUI::render() {
#ifdef ECSCOPE_HAS_IMGUI
    if (!show_window_) return;

    ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar;
    
    if (ImGui::Begin("Network Interface", &show_window_, window_flags)) {
        if (ImGui::BeginMenuBar()) {
            if (ImGui::BeginMenu("View")) {
                ImGui::MenuItem("Connection Manager", nullptr, &show_connection_manager_);
                ImGui::MenuItem("Server Controls", nullptr, &show_server_controls_);
                ImGui::MenuItem("Packet Monitor", nullptr, &show_packet_monitor_);
                ImGui::MenuItem("Statistics", nullptr, &show_statistics_);
                ImGui::MenuItem("Network Visualizer", nullptr, &show_visualizer_);
                ImGui::MenuItem("Bandwidth Monitor", nullptr, &show_bandwidth_monitor_);
                ImGui::MenuItem("Security Panel", nullptr, &show_security_panel_);
                ImGui::EndMenu();
            }
            
            if (ImGui::BeginMenu("Settings")) {
                ImGui::Checkbox("Auto Refresh", &auto_refresh_);
                ImGui::SliderFloat("Refresh Rate", &refresh_rate_, 0.1f, 10.0f, "%.1f Hz");
                
                ImGui::Separator();
                ImGui::Text("Display Mode:");
                ImGui::RadioButton("Compact", &display_mode_, 0);
                ImGui::SameLine();
                ImGui::RadioButton("Detailed", &display_mode_, 1);
                ImGui::SameLine();
                ImGui::RadioButton("Professional", &display_mode_, 2);
                
                ImGui::EndMenu();
            }
            
            if (ImGui::BeginMenu("Tools")) {
                if (ImGui::MenuItem("Connection Test")) {
                }
                if (ImGui::MenuItem("Ping All Connections")) {
                }
                if (ImGui::MenuItem("Export Statistics")) {
                }
                if (ImGui::MenuItem("Clear Packet History")) {
                    packet_inspector_->clear_packet_history();
                }
                ImGui::EndMenu();
            }
            
            ImGui::EndMenuBar();
        }

        ImGui::Columns(2, "NetworkColumns", true);
        ImGui::SetColumnWidth(0, 400);

        if (show_connection_manager_) {
            render_connection_manager();
        }

        if (show_server_controls_) {
            render_server_controls();
        }

        ImGui::NextColumn();

        if (show_statistics_) {
            render_statistics_panel();
        }

        if (show_packet_monitor_) {
            render_packet_monitor();
        }

        ImGui::Columns(1);

        if (show_visualizer_) {
            ImGui::Separator();
            if (ImGui::CollapsingHeader("Network Visualization", ImGuiTreeNodeFlags_DefaultOpen)) {
                visualizer_->render_connection_graph();
                visualizer_->render_packet_flow();
            }
        }

        if (show_bandwidth_monitor_) {
            render_bandwidth_monitor();
        }

        if (show_security_panel_) {
            render_security_panel();
        }
    }
    ImGui::End();

    if (selected_connection_id_ > 0) {
        render_connection_details();
    }
#endif
}

void NetworkUI::render_connection_manager() {
#ifdef ECSCOPE_HAS_IMGUI
    if (ImGui::CollapsingHeader("Connection Manager", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Text("New Connection:");
        ImGui::InputText("Address", connection_address_, sizeof(connection_address_));
        ImGui::InputInt("Port", &connection_port_);
        
        const char* protocols[] = {"TCP", "UDP", "WebSocket", "HTTP", "Custom"};
        ImGui::Combo("Protocol", &connection_protocol_, protocols, IM_ARRAYSIZE(protocols));
        
        ImGui::SameLine();
        if (ImGui::Button("Connect")) {
            if (connection_callback_) {
                connection_callback_(connection_address_, static_cast<uint16_t>(connection_port_), 
                                   static_cast<NetworkProtocol>(connection_protocol_));
            }
        }
        
        ImGui::Separator();
        ImGui::Text("Active Connections (%zu):", connections_.size());
        
        if (ImGui::BeginTable("ConnectionsTable", 6, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | 
                             ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollY, ImVec2(0, 200))) {
            ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthFixed, 100);
            ImGui::TableSetupColumn("Address", ImGuiTableColumnFlags_WidthFixed, 120);
            ImGui::TableSetupColumn("Protocol", ImGuiTableColumnFlags_WidthFixed, 80);
            ImGui::TableSetupColumn("Status", ImGuiTableColumnFlags_WidthFixed, 80);
            ImGui::TableSetupColumn("Ping", ImGuiTableColumnFlags_WidthFixed, 60);
            ImGui::TableSetupColumn("Actions", ImGuiTableColumnFlags_WidthFixed, 80);
            ImGui::TableHeadersRow();

            std::lock_guard<std::mutex> lock(connections_mutex_);
            for (const auto& conn : connections_) {
                ImGui::TableNextRow();
                
                ImGui::TableNextColumn();
                if (ImGui::Selectable(conn.name.c_str(), selected_connection_id_ == conn.id, 
                                     ImGuiSelectableFlags_SpanAllColumns)) {
                    selected_connection_id_ = conn.id;
                }
                
                ImGui::TableNextColumn();
                ImGui::Text("%s:%d", conn.address.c_str(), conn.port);
                
                ImGui::TableNextColumn();
                const char* protocol_names[] = {"TCP", "UDP", "WS", "HTTP", "Custom"};
                ImGui::Text("%s", protocol_names[static_cast<int>(conn.protocol)]);
                
                ImGui::TableNextColumn();
                ImVec4 status_color;
                const char* status_text;
                switch (conn.state) {
                    case ConnectionState::Connected:
                        status_color = ImVec4(0.2f, 0.8f, 0.2f, 1.0f);
                        status_text = "Connected";
                        break;
                    case ConnectionState::Connecting:
                        status_color = ImVec4(0.8f, 0.8f, 0.2f, 1.0f);
                        status_text = "Connecting";
                        break;
                    case ConnectionState::Disconnected:
                        status_color = ImVec4(0.6f, 0.6f, 0.6f, 1.0f);
                        status_text = "Disconnected";
                        break;
                    case ConnectionState::Failed:
                        status_color = ImVec4(0.8f, 0.2f, 0.2f, 1.0f);
                        status_text = "Failed";
                        break;
                    default:
                        status_color = ImVec4(0.8f, 0.4f, 0.2f, 1.0f);
                        status_text = "Unknown";
                        break;
                }
                ImGui::TextColored(status_color, "%s", status_text);
                
                ImGui::TableNextColumn();
                if (conn.state == ConnectionState::Connected) {
                    ImGui::Text("%.1fms", conn.ping_ms);
                } else {
                    ImGui::Text("--");
                }
                
                ImGui::TableNextColumn();
                ImGui::PushID(conn.id);
                if (ImGui::SmallButton("Info")) {
                    selected_connection_id_ = conn.id;
                }
                ImGui::SameLine();
                if (ImGui::SmallButton("X")) {
                    if (disconnect_callback_) {
                        disconnect_callback_(conn.id);
                    }
                }
                ImGui::PopID();
            }
            ImGui::EndTable();
        }
    }
#endif
}

void NetworkUI::render_server_controls() {
#ifdef ECSCOPE_HAS_IMGUI
    if (ImGui::CollapsingHeader("Server Controls", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::InputText("Server Name", server_config_.name.data(), server_config_.name.capacity());
        ImGui::InputInt("Port", reinterpret_cast<int*>(&server_config_.port));
        ImGui::InputInt("Max Connections", reinterpret_cast<int*>(&server_config_.max_connections));
        ImGui::SliderFloat("Tick Rate", &server_config_.tick_rate, 10.0f, 120.0f, "%.1f Hz");
        
        ImGui::Checkbox("Auto Start", &server_config_.auto_start);
        ImGui::SameLine();
        ImGui::Checkbox("Compression", &server_config_.enable_compression);
        ImGui::SameLine();
        ImGui::Checkbox("Encryption", &server_config_.enable_encryption);
        
        if (server_config_.enable_encryption) {
            ImGui::InputText("Password", server_config_.password.data(), 
                           server_config_.password.capacity(), ImGuiInputTextFlags_Password);
        }
        
        ImGui::Separator();
        
        if (!server_running_) {
            if (ImGui::Button("Start Server", ImVec2(100, 30))) {
                server_running_ = true;
                if (server_start_callback_) {
                    server_start_callback_(server_config_);
                }
            }
        } else {
            if (ImGui::Button("Stop Server", ImVec2(100, 30))) {
                server_running_ = false;
                if (server_stop_callback_) {
                    server_stop_callback_();
                }
            }
        }
        
        ImGui::SameLine();
        ImGui::Text("Status: %s", server_running_ ? "Running" : "Stopped");
        
        if (server_running_) {
            std::lock_guard<std::mutex> lock(connections_mutex_);
            ImGui::Text("Active Connections: %zu / %u", connections_.size(), server_config_.max_connections);
        }
    }
#endif
}

void NetworkUI::render_packet_monitor() {
#ifdef ECSCOPE_HAS_IMGUI
    if (ImGui::CollapsingHeader("Packet Monitor", ImGuiTreeNodeFlags_DefaultOpen)) {
        packet_inspector_->render();
    }
#endif
}

void NetworkUI::render_statistics_panel() {
#ifdef ECSCOPE_HAS_IMGUI
    if (ImGui::CollapsingHeader("Network Statistics", ImGuiTreeNodeFlags_DefaultOpen)) {
        std::lock_guard<std::mutex> lock(statistics_mutex_);
        
        ImGui::Text("Connections: %u active / %u total", statistics_.active_connections, statistics_.total_connections);
        ImGui::Text("Data Sent: %.2f MB", statistics_.total_bytes_sent / (1024.0f * 1024.0f));
        ImGui::Text("Data Received: %.2f MB", statistics_.total_bytes_received / (1024.0f * 1024.0f));
        ImGui::Text("Average Ping: %.1f ms", statistics_.average_ping);
        ImGui::Text("Packet Loss: %.2f%%", statistics_.total_packet_loss * 100.0f);
        ImGui::Text("Packets/sec: %u", statistics_.packets_per_second);
        ImGui::Text("Bandwidth Usage: %.2f Mbps", statistics_.bandwidth_usage);
        
        if (!statistics_.ping_history.empty()) {
            ImGui::Separator();
            ImGui::Text("Ping History:");
            ImGui::PlotLines("##PingHistory", statistics_.ping_history.data(), 
                           static_cast<int>(statistics_.ping_history.size()), 
                           0, nullptr, 0.0f, 500.0f, ImVec2(0, 60));
        }
        
        if (!statistics_.bandwidth_history.empty()) {
            ImGui::Text("Bandwidth History:");
            ImGui::PlotLines("##BandwidthHistory", statistics_.bandwidth_history.data(),
                           static_cast<int>(statistics_.bandwidth_history.size()),
                           0, nullptr, 0.0f, 100.0f, ImVec2(0, 60));
        }
    }
#endif
}

void NetworkUI::render_connection_details() {
#ifdef ECSCOPE_HAS_IMGUI
    auto it = std::find_if(connections_.begin(), connections_.end(),
                          [this](const NetworkConnection& conn) { return conn.id == selected_connection_id_; });
    
    if (it == connections_.end()) {
        selected_connection_id_ = 0;
        return;
    }

    const NetworkConnection& conn = *it;
    
    ImGui::SetNextWindowSize(ImVec2(400, 300), ImGuiCond_FirstUseEver);
    std::string window_title = "Connection Details - " + conn.name;
    
    if (ImGui::Begin(window_title.c_str(), nullptr, ImGuiWindowFlags_NoDocking)) {
        ImGui::Text("Connection ID: %u", conn.id);
        ImGui::Text("Name: %s", conn.name.c_str());
        ImGui::Text("Address: %s:%d", conn.address.c_str(), conn.port);
        ImGui::Text("Protocol: %s", conn.protocol == NetworkProtocol::TCP ? "TCP" :
                                   conn.protocol == NetworkProtocol::UDP ? "UDP" :
                                   conn.protocol == NetworkProtocol::WebSocket ? "WebSocket" :
                                   conn.protocol == NetworkProtocol::HTTP ? "HTTP" : "Custom");
        
        ImGui::Separator();
        
        ImGui::Text("Status: %s", conn.state == ConnectionState::Connected ? "Connected" :
                                 conn.state == ConnectionState::Connecting ? "Connecting" :
                                 conn.state == ConnectionState::Disconnected ? "Disconnected" :
                                 conn.state == ConnectionState::Failed ? "Failed" : "Unknown");
        
        if (conn.state == ConnectionState::Connected) {
            ImGui::Text("Ping: %.1f ms", conn.ping_ms);
            ImGui::Text("Bytes Sent: %lu", conn.bytes_sent);
            ImGui::Text("Bytes Received: %lu", conn.bytes_received);
            ImGui::Text("Packets Sent: %u", conn.packets_sent);
            ImGui::Text("Packets Received: %u", conn.packets_received);
            ImGui::Text("Packets Lost: %u", conn.packets_lost);
            ImGui::Text("Packet Loss Rate: %.2f%%", conn.packet_loss_rate * 100.0f);
        }
        
        ImGui::Separator();
        
        if (ImGui::Button("Close")) {
            selected_connection_id_ = 0;
        }
    }
    ImGui::End();
#endif
}

void NetworkUI::render_network_settings() {
#ifdef ECSCOPE_HAS_IMGUI
    if (ImGui::CollapsingHeader("Network Settings")) {
        ImGui::Text("Advanced configuration options would go here");
    }
#endif
}

void NetworkUI::render_bandwidth_monitor() {
#ifdef ECSCOPE_HAS_IMGUI
    if (ImGui::CollapsingHeader("Bandwidth Monitor")) {
        ImGui::Text("Real-time bandwidth monitoring visualization");
    }
#endif
}

void NetworkUI::render_security_panel() {
#ifdef ECSCOPE_HAS_IMGUI
    if (ImGui::CollapsingHeader("Security Panel")) {
        ImGui::Text("Security settings and monitoring tools");
    }
#endif
}

void NetworkUI::update(float delta_time) {
    last_refresh_time_ += delta_time;
    
    if (auto_refresh_ && last_refresh_time_ >= (1.0f / refresh_rate_)) {
        calculate_statistics();
        update_ping_history();
        last_refresh_time_ = 0.0f;
    }
}

void NetworkUI::shutdown() {
    NetworkManager::instance().unregister_network_ui(this);
}

void NetworkUI::add_connection(const NetworkConnection& connection) {
    std::lock_guard<std::mutex> lock(connections_mutex_);
    connections_.push_back(connection);
    update_connection_colors();
}

void NetworkUI::update_connection(uint32_t connection_id, const NetworkConnection& connection) {
    std::lock_guard<std::mutex> lock(connections_mutex_);
    auto it = std::find_if(connections_.begin(), connections_.end(),
                          [connection_id](const NetworkConnection& conn) { return conn.id == connection_id; });
    
    if (it != connections_.end()) {
        *it = connection;
    }
}

void NetworkUI::remove_connection(uint32_t connection_id) {
    std::lock_guard<std::mutex> lock(connections_mutex_);
    connections_.erase(
        std::remove_if(connections_.begin(), connections_.end(),
                      [connection_id](const NetworkConnection& conn) { return conn.id == connection_id; }),
        connections_.end());
}

void NetworkUI::add_packet(const NetworkPacket& packet, const std::vector<uint8_t>& data) {
    if (packet_inspector_) {
        packet_inspector_->add_packet(packet, data);
    }
}

void NetworkUI::update_statistics(const NetworkStatistics& stats) {
    std::lock_guard<std::mutex> lock(statistics_mutex_);
    statistics_ = stats;
}

void NetworkUI::update_connection_colors() {
    for (size_t i = 0; i < connections_.size(); ++i) {
        connections_[i].connection_color = ImGui::ColorConvertFloat4ToU32(connection_colors_[i % 6]);
    }
}

void NetworkUI::update_ping_history() {
    if (!connections_.empty()) {
        float total_ping = 0.0f;
        int connected_count = 0;
        
        for (const auto& conn : connections_) {
            if (conn.state == ConnectionState::Connected) {
                total_ping += conn.ping_ms;
                connected_count++;
            }
        }
        
        if (connected_count > 0) {
            float avg_ping = total_ping / connected_count;
            
            std::lock_guard<std::mutex> lock(statistics_mutex_);
            statistics_.ping_history.push_back(avg_ping);
            if (statistics_.ping_history.size() > 100) {
                statistics_.ping_history.erase(statistics_.ping_history.begin());
            }
        }
    }
}

void NetworkUI::calculate_statistics() {
    std::lock_guard<std::mutex> lock1(connections_mutex_);
    std::lock_guard<std::mutex> lock2(statistics_mutex_);
    
    statistics_.total_connections = static_cast<uint32_t>(connections_.size());
    statistics_.active_connections = 0;
    
    float total_ping = 0.0f;
    uint64_t total_sent = 0;
    uint64_t total_received = 0;
    float total_loss = 0.0f;
    
    for (const auto& conn : connections_) {
        if (conn.state == ConnectionState::Connected) {
            statistics_.active_connections++;
            total_ping += conn.ping_ms;
            total_loss += conn.packet_loss_rate;
        }
        total_sent += conn.bytes_sent;
        total_received += conn.bytes_received;
    }
    
    statistics_.total_bytes_sent = total_sent;
    statistics_.total_bytes_received = total_received;
    
    if (statistics_.active_connections > 0) {
        statistics_.average_ping = total_ping / statistics_.active_connections;
        statistics_.total_packet_loss = total_loss / statistics_.active_connections;
    } else {
        statistics_.average_ping = 0.0f;
        statistics_.total_packet_loss = 0.0f;
    }
}

void NetworkUI::set_connection_callback(std::function<void(const std::string&, uint16_t, NetworkProtocol)> callback) {
    connection_callback_ = callback;
}

void NetworkUI::set_disconnect_callback(std::function<void(uint32_t)> callback) {
    disconnect_callback_ = callback;
}

void NetworkUI::set_server_start_callback(std::function<void(const ServerConfiguration&)> callback) {
    server_start_callback_ = callback;
}

void NetworkUI::set_server_stop_callback(std::function<void()> callback) {
    server_stop_callback_ = callback;
}

void NetworkVisualizer::initialize() {
    auto_layout_ = true;
    node_separation_ = 100.0f;
    spring_strength_ = 0.1f;
    damping_ = 0.9f;
}

void NetworkVisualizer::render_connection_graph() {
#ifdef ECSCOPE_HAS_IMGUI
    if (ImGui::BeginChild("ConnectionGraph", ImVec2(0, 200), true)) {
        ImDrawList* draw_list = ImGui::GetWindowDrawList();
        ImVec2 canvas_pos = ImGui::GetCursorScreenPos();
        ImVec2 canvas_size = ImGui::GetContentRegionAvail();
        
        ImGui::InvisibleButton("ConnectionGraphCanvas", canvas_size);
        
        ImVec2 center = ImVec2(canvas_pos.x + canvas_size.x * 0.5f, canvas_pos.y + canvas_size.y * 0.5f);
        
        for (const auto& [id, node] : connection_nodes_) {
            ImVec2 pos = ImVec2(center.x + node.position.x, center.y + node.position.y);
            draw_list->AddCircleFilled(pos, node.radius, node.color);
            draw_list->AddCircle(pos, node.radius, IM_COL32(255, 255, 255, 100), 0, 2.0f);
        }
        
        for (const auto& [from_id, to_id] : connection_links_) {
            auto from_it = connection_nodes_.find(from_id);
            auto to_it = connection_nodes_.find(to_id);
            if (from_it != connection_nodes_.end() && to_it != connection_nodes_.end()) {
                ImVec2 from_pos = ImVec2(center.x + from_it->second.position.x, center.y + from_it->second.position.y);
                ImVec2 to_pos = ImVec2(center.x + to_it->second.position.x, center.y + to_it->second.position.y);
                draw_list->AddLine(from_pos, to_pos, IM_COL32(128, 128, 128, 200), 2.0f);
            }
        }
    }
    ImGui::EndChild();
#endif
}

void NetworkVisualizer::render_packet_flow() {
#ifdef ECSCOPE_HAS_IMGUI
    ImGui::Text("Packet flow visualization would be implemented here");
#endif
}

void NetworkVisualizer::render_network_topology() {
#ifdef ECSCOPE_HAS_IMGUI
    ImGui::Text("Network topology visualization would be implemented here");
#endif
}

void NetworkVisualizer::update_visualization_data(const std::vector<NetworkConnection>& connections,
                                                 const std::vector<NetworkPacket>& recent_packets) {
    connection_nodes_.clear();
    connection_links_.clear();
    
    for (const auto& conn : connections) {
        ConnectionNode node;
        node.position = ImVec2(0, 0); // Would be calculated based on layout algorithm
        node.radius = 15.0f;
        node.color = conn.connection_color;
        node.is_selected = false;
        connection_nodes_[conn.id] = node;
    }
}

void NetworkPacketInspector::initialize() {
    max_history_size_ = 1000;
    auto_scroll_ = true;
    
    packet_filters_[PacketType::Handshake] = true;
    packet_filters_[PacketType::GameData] = true;
    packet_filters_[PacketType::PlayerInput] = true;
    packet_filters_[PacketType::WorldSync] = true;
    packet_filters_[PacketType::Chat] = true;
    packet_filters_[PacketType::Voice] = false;
    packet_filters_[PacketType::File] = true;
    packet_filters_[PacketType::Custom] = true;
}

void NetworkPacketInspector::render() {
#ifdef ECSCOPE_HAS_IMGUI
    ImGui::Text("Packet Inspector");
    
    ImGui::Text("Filters:");
    ImGui::SameLine();
    if (ImGui::Checkbox("Handshake", &packet_filters_[PacketType::Handshake])) {}
    ImGui::SameLine();
    if (ImGui::Checkbox("Game Data", &packet_filters_[PacketType::GameData])) {}
    ImGui::SameLine();
    if (ImGui::Checkbox("Input", &packet_filters_[PacketType::PlayerInput])) {}
    
    if (ImGui::BeginChild("PacketHistory", ImVec2(0, 150), true)) {
        for (auto& entry : packet_history_) {
            if (!packet_filters_[entry.packet.type]) continue;
            
            ImGui::PushID(entry.packet.id);
            if (ImGui::TreeNode(entry.packet.description.c_str())) {
                ImGui::Text("Type: %d", static_cast<int>(entry.packet.type));
                ImGui::Text("Size: %zu bytes", entry.packet.size);
                ImGui::Text("Connection: %u", entry.packet.connection_id);
                ImGui::Text("Direction: %s", entry.packet.is_outgoing ? "Outgoing" : "Incoming");
                ImGui::TreePop();
            }
            ImGui::PopID();
        }
    }
    ImGui::EndChild();
#endif
}

void NetworkPacketInspector::add_packet(const NetworkPacket& packet, const std::vector<uint8_t>& data) {
    PacketEntry entry;
    entry.packet = packet;
    entry.data = data;
    entry.is_expanded = false;
    
    packet_history_.push_back(entry);
    
    if (packet_history_.size() > max_history_size_) {
        packet_history_.erase(packet_history_.begin());
    }
}

void NetworkPacketInspector::clear_packet_history() {
    packet_history_.clear();
}

void NetworkManager::initialize() {
}

void NetworkManager::shutdown() {
    std::lock_guard<std::mutex> lock(ui_mutex_);
    registered_uis_.clear();
}

void NetworkManager::register_network_ui(NetworkUI* ui) {
    std::lock_guard<std::mutex> lock(ui_mutex_);
    registered_uis_.push_back(ui);
}

void NetworkManager::unregister_network_ui(NetworkUI* ui) {
    std::lock_guard<std::mutex> lock(ui_mutex_);
    registered_uis_.erase(
        std::remove(registered_uis_.begin(), registered_uis_.end(), ui),
        registered_uis_.end());
}

void NetworkManager::notify_connection_changed(const NetworkConnection& connection) {
    std::lock_guard<std::mutex> lock(ui_mutex_);
    for (auto* ui : registered_uis_) {
        ui->update_connection(connection.id, connection);
    }
}

void NetworkManager::notify_packet_received(const NetworkPacket& packet, const std::vector<uint8_t>& data) {
    std::lock_guard<std::mutex> lock(ui_mutex_);
    for (auto* ui : registered_uis_) {
        ui->add_packet(packet, data);
    }
}

void NetworkManager::notify_statistics_updated(const NetworkStatistics& stats) {
    std::lock_guard<std::mutex> lock(ui_mutex_);
    for (auto* ui : registered_uis_) {
        ui->update_statistics(stats);
    }
}

}} // namespace ecscope::gui