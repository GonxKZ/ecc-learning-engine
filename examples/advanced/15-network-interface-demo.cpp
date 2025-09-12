#include <iostream>
#include <memory>
#include <thread>
#include <chrono>
#include <random>
#include <vector>
#include <string>

#ifdef ECSCOPE_HAS_GUI
#include "ecscope/gui/dashboard.hpp"
#include "ecscope/gui/network_ui.hpp"
#include "ecscope/gui/gui_manager.hpp"
#endif

class MockNetworkSystem {
public:
    MockNetworkSystem() : next_connection_id_(1), next_packet_id_(1) {
        rng_.seed(std::chrono::steady_clock::now().time_since_epoch().count());
    }

    void initialize() {
#ifdef ECSCOPE_HAS_GUI
        ecscope::gui::NetworkManager::instance().initialize();
        
        // Create some mock connections
        create_mock_connection("Game Server", "gameserver.example.com", 8080, 
                             ecscope::gui::NetworkProtocol::TCP, true);
        create_mock_connection("Voice Chat", "voice.example.com", 9001, 
                             ecscope::gui::NetworkProtocol::UDP, false);
        create_mock_connection("Web API", "api.example.com", 443, 
                             ecscope::gui::NetworkProtocol::WebSocket, false);
        create_mock_connection("File Server", "files.example.com", 21, 
                             ecscope::gui::NetworkProtocol::TCP, false);
#endif
    }

    void update(float delta_time) {
        update_time_ += delta_time;
        
        if (update_time_ >= 0.1f) { // Update 10 times per second
            update_connections();
            generate_packets();
            update_statistics();
            update_time_ = 0.0f;
        }
    }

private:
    void create_mock_connection(const std::string& name, const std::string& address, 
                               uint16_t port, ecscope::gui::NetworkProtocol protocol, bool is_server) {
#ifdef ECSCOPE_HAS_GUI
        ecscope::gui::NetworkConnection conn;
        conn.id = next_connection_id_++;
        conn.name = name;
        conn.address = address;
        conn.port = port;
        conn.protocol = protocol;
        conn.state = ecscope::gui::ConnectionState::Connected;
        conn.ping_ms = 20.0f + uniform_dist_(rng_) * 80.0f; // 20-100ms ping
        conn.bytes_sent = uniform_dist_(rng_) * 1000000; // Random bytes sent
        conn.bytes_received = uniform_dist_(rng_) * 1000000; // Random bytes received
        conn.packets_sent = static_cast<uint32_t>(uniform_dist_(rng_) * 10000);
        conn.packets_received = static_cast<uint32_t>(uniform_dist_(rng_) * 10000);
        conn.packets_lost = static_cast<uint32_t>(uniform_dist_(rng_) * 100);
        conn.packet_loss_rate = conn.packets_lost / static_cast<float>(conn.packets_received + conn.packets_lost);
        conn.last_activity = std::chrono::steady_clock::now();
        conn.is_server = is_server;
        
        connections_.push_back(conn);
        
        ecscope::gui::NetworkManager::instance().notify_connection_changed(conn);
#endif
    }

    void update_connections() {
#ifdef ECSCOPE_HAS_GUI
        for (auto& conn : connections_) {
            // Simulate ping variations
            conn.ping_ms += (uniform_dist_(rng_) - 0.5f) * 10.0f;
            conn.ping_ms = std::max(5.0f, std::min(200.0f, conn.ping_ms));
            
            // Simulate data transfer
            uint64_t bytes_increment = static_cast<uint64_t>(uniform_dist_(rng_) * 1000);
            conn.bytes_sent += bytes_increment;
            conn.bytes_received += bytes_increment * 0.8f; // Slightly less received
            
            // Update packet counts
            uint32_t packets_increment = static_cast<uint32_t>(uniform_dist_(rng_) * 10);
            conn.packets_sent += packets_increment;
            conn.packets_received += packets_increment;
            
            // Occasionally lose a packet
            if (uniform_dist_(rng_) < 0.05f) {
                conn.packets_lost++;
            }
            
            // Update packet loss rate
            conn.packet_loss_rate = conn.packets_lost / static_cast<float>(conn.packets_received + conn.packets_lost);
            
            conn.last_activity = std::chrono::steady_clock::now();
            
            ecscope::gui::NetworkManager::instance().notify_connection_changed(conn);
        }
#endif
    }

    void generate_packets() {
#ifdef ECSCOPE_HAS_GUI
        // Generate some random packets for demonstration
        for (int i = 0; i < 3; ++i) {
            if (uniform_dist_(rng_) < 0.3f && !connections_.empty()) {
                ecscope::gui::NetworkPacket packet;
                packet.id = next_packet_id_++;
                
                // Random packet type
                int type_index = static_cast<int>(uniform_dist_(rng_) * 8);
                packet.type = static_cast<ecscope::gui::PacketType>(type_index);
                
                // Random connection
                size_t conn_index = static_cast<size_t>(uniform_dist_(rng_) * connections_.size());
                packet.connection_id = connections_[conn_index].id;
                
                packet.size = static_cast<size_t>(uniform_dist_(rng_) * 1024) + 64;
                packet.timestamp = std::chrono::steady_clock::now();
                packet.is_outgoing = uniform_dist_(rng_) < 0.5f;
                
                // Generate description based on packet type
                switch (packet.type) {
                    case ecscope::gui::PacketType::Handshake:
                        packet.description = "Handshake " + std::to_string(packet.id);
                        break;
                    case ecscope::gui::PacketType::GameData:
                        packet.description = "Game Data " + std::to_string(packet.id);
                        break;
                    case ecscope::gui::PacketType::PlayerInput:
                        packet.description = "Player Input " + std::to_string(packet.id);
                        break;
                    case ecscope::gui::PacketType::WorldSync:
                        packet.description = "World Sync " + std::to_string(packet.id);
                        break;
                    case ecscope::gui::PacketType::Chat:
                        packet.description = "Chat Message " + std::to_string(packet.id);
                        break;
                    case ecscope::gui::PacketType::Voice:
                        packet.description = "Voice Data " + std::to_string(packet.id);
                        break;
                    case ecscope::gui::PacketType::File:
                        packet.description = "File Transfer " + std::to_string(packet.id);
                        break;
                    default:
                        packet.description = "Custom Packet " + std::to_string(packet.id);
                        break;
                }
                
                // Generate some mock data
                std::vector<uint8_t> data(packet.size);
                for (size_t j = 0; j < data.size(); ++j) {
                    data[j] = static_cast<uint8_t>(uniform_dist_(rng_) * 256);
                }
                
                ecscope::gui::NetworkManager::instance().notify_packet_received(packet, data);
            }
        }
#endif
    }

    void update_statistics() {
#ifdef ECSCOPE_HAS_GUI
        ecscope::gui::NetworkStatistics stats;
        
        stats.total_connections = static_cast<uint32_t>(connections_.size());
        stats.active_connections = 0;
        stats.total_bytes_sent = 0;
        stats.total_bytes_received = 0;
        float total_ping = 0.0f;
        float total_packet_loss = 0.0f;
        
        for (const auto& conn : connections_) {
            if (conn.state == ecscope::gui::ConnectionState::Connected) {
                stats.active_connections++;
                total_ping += conn.ping_ms;
                total_packet_loss += conn.packet_loss_rate;
            }
            stats.total_bytes_sent += conn.bytes_sent;
            stats.total_bytes_received += conn.bytes_received;
        }
        
        if (stats.active_connections > 0) {
            stats.average_ping = total_ping / stats.active_connections;
            stats.total_packet_loss = total_packet_loss / stats.active_connections;
        }
        
        stats.packets_per_second = static_cast<uint32_t>(uniform_dist_(rng_) * 100) + 50;
        stats.bandwidth_usage = uniform_dist_(rng_) * 50.0f + 10.0f; // 10-60 Mbps
        
        // Add to history
        stats.ping_history.push_back(stats.average_ping);
        if (stats.ping_history.size() > 100) {
            stats.ping_history.erase(stats.ping_history.begin());
        }
        
        stats.bandwidth_history.push_back(stats.bandwidth_usage);
        if (stats.bandwidth_history.size() > 100) {
            stats.bandwidth_history.erase(stats.bandwidth_history.begin());
        }
        
        ecscope::gui::NetworkManager::instance().notify_statistics_updated(stats);
#endif
    }

    std::vector<ecscope::gui::NetworkConnection> connections_;
    uint32_t next_connection_id_;
    uint32_t next_packet_id_;
    float update_time_ = 0.0f;
    std::mt19937 rng_;
    std::uniform_real_distribution<float> uniform_dist_{0.0f, 1.0f};
};

int main() {
    std::cout << "═══════════════════════════════════════════════════════\n";
    std::cout << "  ECScope Network Interface Demo\n";
    std::cout << "═══════════════════════════════════════════════════════\n\n";

#ifdef ECSCOPE_HAS_GUI
    try {
        // Initialize GUI manager
        auto gui_manager = std::make_unique<ecscope::gui::GuiManager>();
        if (!gui_manager->initialize("ECScope Network Interface Demo", 1400, 900)) {
            std::cerr << "Failed to initialize GUI manager\n";
            return 1;
        }

        // Initialize dashboard
        auto dashboard = std::make_unique<ecscope::gui::Dashboard>();
        if (!dashboard->initialize()) {
            std::cerr << "Failed to initialize dashboard\n";
            return 1;
        }

        // Initialize network UI
        auto network_ui = std::make_unique<ecscope::gui::NetworkUI>();
        if (!network_ui->initialize()) {
            std::cerr << "Failed to initialize network UI\n";
            return 1;
        }

        // Initialize mock network system
        auto network_system = std::make_unique<MockNetworkSystem>();
        network_system->initialize();

        // Set up network callbacks
        network_ui->set_connection_callback([](const std::string& address, uint16_t port, 
                                              ecscope::gui::NetworkProtocol protocol) {
            std::cout << "Connection request: " << address << ":" << port << " (" 
                     << static_cast<int>(protocol) << ")\n";
        });

        network_ui->set_disconnect_callback([](uint32_t connection_id) {
            std::cout << "Disconnect request: Connection " << connection_id << "\n";
        });

        network_ui->set_server_start_callback([](const ecscope::gui::ServerConfiguration& config) {
            std::cout << "Server start request: " << config.name << " on port " << config.port << "\n";
        });

        network_ui->set_server_stop_callback([]() {
            std::cout << "Server stop request\n";
        });

        std::cout << "Network Interface Demo Controls:\n";
        std::cout << "• Connection Manager: Add/remove connections\n";
        std::cout << "• Server Controls: Start/stop game server\n";
        std::cout << "• Packet Monitor: View network traffic\n";
        std::cout << "• Statistics: Real-time network metrics\n";
        std::cout << "• Visualizer: Network topology visualization\n";
        std::cout << "• Close window to exit\n\n";

        // Main loop
        auto last_time = std::chrono::high_resolution_clock::now();
        
        while (!gui_manager->should_close()) {
            auto current_time = std::chrono::high_resolution_clock::now();
            float delta_time = std::chrono::duration<float>(current_time - last_time).count();
            last_time = current_time;

            // Update systems
            network_system->update(delta_time);
            network_ui->update(delta_time);

            // Render frame
            gui_manager->begin_frame();
            
            // Render dashboard with network UI as a feature
            dashboard->add_feature("Network Interface", "Comprehensive networking controls and monitoring", [&]() {
                network_ui->render();
            }, true);
            
            dashboard->render();

            gui_manager->end_frame();
            
            // Small delay to prevent excessive CPU usage
            std::this_thread::sleep_for(std::chrono::milliseconds(16)); // ~60 FPS
        }

        // Cleanup
        network_ui->shutdown();
        dashboard->shutdown();
        gui_manager->shutdown();

        std::cout << "Network Interface Demo completed successfully!\n";
        return 0;

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
#else
    std::cout << "❌ GUI system not available\n";
    std::cout << "This demo requires GLFW, OpenGL, and Dear ImGui\n";
    std::cout << "Please build with -DECSCOPE_BUILD_GUI=ON\n";
    return 1;
#endif
}