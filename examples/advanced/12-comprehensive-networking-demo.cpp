/**
 * @file examples/advanced/12-comprehensive-networking-demo.cpp
 * @brief Comprehensive Networking System Demonstration for ECScope
 * 
 * This advanced example demonstrates the complete networking system including:
 * - Client-server ECS synchronization with delta compression
 * - Network prediction with lag compensation
 * - Custom UDP protocol with reliability layers
 * - Authority system for distributed entity ownership
 * - Educational visualization and interactive learning
 * - Performance analysis and optimization guidance
 * 
 * The demo creates a multi-client networked simulation where users can:
 * - Connect as server or client
 * - See real-time network statistics and visualizations
 * - Experience different network conditions (latency, packet loss)
 * - Learn about distributed systems through interactive tutorials
 * - Analyze performance impact of different networking strategies
 * 
 * Educational Focus:
 * - Understanding client-server architecture
 * - Learning about prediction and reconciliation
 * - Exploring bandwidth optimization techniques
 * - Hands-on experience with network protocols
 * 
 * @author ECScope Educational Framework - Advanced Networking
 */

#include <ecscope/world.hpp>
#include <ecscope/registry.hpp>
#include <ecscope/components.hpp>
#include <ecscope/systems.hpp>
#include <ecscope/window.hpp>
#include <ecscope/renderer_2d.hpp>
#include <ecscope/debug_renderer_2d.hpp>
#include <ecscope/overlay.hpp>
#include <ecscope/panel_stats.hpp>
#include <ecscope/physics_system.hpp>
#include <ecscope/performance_benchmark.hpp>

// Networking headers
#include <ecscope/networking/ecs_networking_system.hpp>
#include <ecscope/networking/educational_visualizer.hpp>
#include <ecscope/networking/network_simulation.hpp>

#include <imgui.h>
#include <iostream>
#include <thread>
#include <chrono>
#include <random>
#include <memory>

using namespace ecscope;

//=============================================================================
// Demo Configuration and State
//=============================================================================

struct DemoConfig {
    bool is_server{false};
    std::string server_address{"127.0.0.1"};
    u16 server_port{7777};
    u32 max_clients{8};
    
    // Simulation parameters
    u32 num_entities{50};
    f32 world_size{800.0f};
    f32 entity_speed{100.0f};
    
    // Educational settings
    bool educational_mode{true};
    bool show_tutorials{true};
    bool enable_visualization{true};
    networking::education::DifficultyLevel detail_level{networking::education::DifficultyLevel::Intermediate};
    
    // Network simulation
    f32 artificial_latency_ms{50.0f};
    f32 packet_loss_rate{0.02f};  // 2%
    f32 jitter_ms{10.0f};
};

struct DemoStatistics {
    // Entity statistics
    u32 networked_entities{0};
    u32 local_entities{0};
    u32 predicted_entities{0};
    
    // Network statistics
    f32 average_ping_ms{0.0f};
    f32 bandwidth_usage_kbps{0.0f};
    u32 packets_sent{0};
    u32 packets_received{0};
    u32 prediction_corrections{0};
    
    // Educational statistics
    u32 tutorials_completed{0};
    f32 learning_time_minutes{0.0f};
    u32 concepts_explored{0};
};

//=============================================================================
// Demo-Specific Components
//=============================================================================

/**
 * @brief Networked Entity Component
 * 
 * Marks entities that should be synchronized across the network.
 * Contains network-specific metadata and state.
 */
struct NetworkedEntity {
    networking::NetworkEntityID network_id{0};
    networking::ClientID authority{0};
    bool is_predicted{false};
    f32 prediction_confidence{1.0f};
    std::chrono::high_resolution_clock::time_point last_sync_time;
    
    // Educational metadata
    std::string entity_type{"Generic"};
    u32 sync_priority{1};
    bool show_prediction_ghost{true};
};

/**
 * @brief Movement Component for Demo Entities
 * 
 * Simple movement component that demonstrates network synchronization
 * and prediction for constantly changing data.
 */
struct MovementComponent {
    math::Vec2 velocity{0.0f, 0.0f};
    f32 max_speed{100.0f};
    f32 acceleration{200.0f};
    f32 friction{0.95f};
    
    // Network optimization hints
    bool is_frequently_changing{true};
    f32 change_threshold{0.1f};  // Minimum change to trigger sync
};

/**
 * @brief Interactive Tutorial Entity
 * 
 * Special entities used in educational scenarios to demonstrate
 * specific networking concepts with visual feedback.
 */
struct TutorialEntity {
    std::string concept{"basic_sync"};
    std::string explanation;
    bool is_interactive{true};
    f32 highlight_intensity{0.0f};
    std::function<void()> on_interact;
};

//=============================================================================
// Demo Systems
//=============================================================================

/**
 * @brief Movement System
 * 
 * Handles entity movement with network-aware updates that demonstrate
 * prediction, authority, and synchronization concepts.
 */
class DemoMovementSystem : public System {
private:
    std::random_device rd_;
    std::mt19937 gen_;
    std::uniform_real_distribution<f32> direction_dist_;
    f32 world_size_;
    
public:
    explicit DemoMovementSystem(f32 world_size) 
        : gen_(rd_())
        , direction_dist_(-1.0f, 1.0f)
        , world_size_(world_size) {}
    
    void update(f32 delta_time) override {
        auto view = registry_->view<Transform, MovementComponent>();
        
        for (auto entity : view) {
            auto& transform = view.get<Transform>(entity);
            auto& movement = view.get<MovementComponent>(entity);
            
            // Apply movement
            transform.position += movement.velocity * delta_time;
            
            // Apply friction
            movement.velocity *= movement.friction;
            
            // Bounce off world boundaries
            if (transform.position.x < 0.0f || transform.position.x > world_size_) {
                movement.velocity.x *= -0.8f;
                transform.position.x = std::clamp(transform.position.x, 0.0f, world_size_);
            }
            
            if (transform.position.y < 0.0f || transform.position.y > world_size_) {
                movement.velocity.y *= -0.8f;
                transform.position.y = std::clamp(transform.position.y, 0.0f, world_size_);
            }
            
            // Add random movement for demonstration
            if (registry_->has_component<NetworkedEntity>(entity)) {
                auto& networked = registry_->get_component<NetworkedEntity>(entity);
                
                // Only entities with authority should generate movement
                // This demonstrates the authority system
                if (networked.authority == 1) {  // Local authority
                    math::Vec2 random_force = {
                        direction_dist_(gen_) * 50.0f,
                        direction_dist_(gen_) * 50.0f
                    };
                    
                    movement.velocity += random_force * delta_time;
                    
                    // Clamp to max speed
                    f32 speed = movement.velocity.length();
                    if (speed > movement.max_speed) {
                        movement.velocity = movement.velocity.normalized() * movement.max_speed;
                    }
                }
            }
        }
    }
};

/**
 * @brief Educational Overlay System
 * 
 * Manages the educational UI, tutorials, and interactive learning features.
 * Provides real-time insights into networking behavior.
 */
class EducationalOverlaySystem : public System {
private:
    networking::education::EducationalNetworkingSystem* edu_system_;
    DemoStatistics* demo_stats_;
    DemoConfig* demo_config_;
    bool show_detailed_stats_{false};
    bool show_packet_inspector_{false};
    bool show_prediction_analysis_{false};
    std::string selected_tutorial_;
    
public:
    EducationalOverlaySystem(networking::education::EducationalNetworkingSystem* edu_system,
                           DemoStatistics* stats, DemoConfig* config)
        : edu_system_(edu_system), demo_stats_(stats), demo_config_(config) {}
    
    void update(f32 delta_time) override {
        render_main_educational_panel();
        render_network_statistics_panel();
        render_tutorial_panel();
        render_visualization_controls();
        
        if (show_detailed_stats_) {
            render_detailed_statistics();
        }
        
        if (show_packet_inspector_) {
            render_packet_inspector();
        }
        
        if (show_prediction_analysis_) {
            render_prediction_analysis();
        }
    }
    
private:
    void render_main_educational_panel() {
        if (!ImGui::Begin("🎓 Network Education Center", nullptr, 
                         ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::End();
            return;
        }
        
        // Learning progress
        auto learning_stats = edu_system_->get_content_manager().get_learning_stats();
        
        ImGui::Text("📊 Learning Progress");
        ImGui::Separator();
        
        ImGui::Text("Objectives Completed: %u", learning_stats.total_objectives_completed);
        ImGui::Text("Learning Time: %.1f hours", learning_stats.total_learning_time_hours);
        ImGui::Text("Current Level: %s", difficulty_level_to_string(learning_stats.current_level));
        
        // Progress bar
        f32 progress = learning_stats.progress_percentage / 100.0f;
        ImGui::ProgressBar(progress, ImVec2(200, 0), "Progress");
        
        ImGui::Spacing();
        
        // Quick learning actions
        ImGui::Text("🚀 Quick Learning");
        ImGui::Separator();
        
        if (ImGui::Button("Start Basic Networking Tutorial")) {
            edu_system_->start_tutorial("networking_basics");
            selected_tutorial_ = "networking_basics";
        }
        
        if (ImGui::Button("Explore Client Prediction")) {
            edu_system_->start_tutorial("client_prediction");
            selected_tutorial_ = "client_prediction";
        }
        
        if (ImGui::Button("Advanced: Protocol Design")) {
            edu_system_->start_tutorial("custom_reliability");
            selected_tutorial_ = "custom_reliability";
        }
        
        ImGui::Spacing();
        
        // Educational settings
        ImGui::Text("⚙️ Educational Settings");
        ImGui::Separator();
        
        ImGui::Checkbox("Show Tutorials", &demo_config_->show_tutorials);
        ImGui::Checkbox("Enable Visualization", &demo_config_->enable_visualization);
        
        // Detail level selection
        const char* levels[] = {"Beginner", "Intermediate", "Advanced", "Expert"};
        int current_level = static_cast<int>(demo_config_->detail_level);
        if (ImGui::Combo("Detail Level", &current_level, levels, 4)) {
            demo_config_->detail_level = static_cast<networking::education::DifficultyLevel>(current_level);
            edu_system_->get_visualizer().set_detail_level(demo_config_->detail_level);
        }
        
        ImGui::End();
    }
    
    void render_network_statistics_panel() {
        if (!ImGui::Begin("📡 Network Statistics", nullptr, 
                         ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::End();
            return;
        }
        
        // Real-time network metrics
        ImGui::Text("🌐 Connection Status");
        ImGui::Separator();
        
        ImGui::Text("Role: %s", demo_config_->is_server ? "Server" : "Client");
        ImGui::Text("Address: %s:%u", demo_config_->server_address.c_str(), demo_config_->server_port);
        
        // Network performance
        ImGui::Spacing();
        ImGui::Text("📈 Performance Metrics");
        ImGui::Separator();
        
        ImGui::Text("Ping: %.1f ms", demo_stats_->average_ping_ms);
        ImGui::Text("Bandwidth: %.1f KB/s", demo_stats_->bandwidth_usage_kbps);
        ImGui::Text("Packets Sent: %u", demo_stats_->packets_sent);
        ImGui::Text("Packets Received: %u", demo_stats_->packets_received);
        
        // Entity synchronization
        ImGui::Spacing();
        ImGui::Text("🔄 Entity Synchronization");
        ImGui::Separator();
        
        ImGui::Text("Networked Entities: %u", demo_stats_->networked_entities);
        ImGui::Text("Predicted Entities: %u", demo_stats_->predicted_entities);
        ImGui::Text("Prediction Corrections: %u", demo_stats_->prediction_corrections);
        
        // Control buttons
        ImGui::Spacing();
        if (ImGui::Button("Show Detailed Stats")) {
            show_detailed_stats_ = !show_detailed_stats_;
        }
        
        ImGui::SameLine();
        if (ImGui::Button("Packet Inspector")) {
            show_packet_inspector_ = !show_packet_inspector_;
        }
        
        if (ImGui::Button("Prediction Analysis")) {
            show_prediction_analysis_ = !show_prediction_analysis_;
        }
        
        ImGui::End();
    }
    
    void render_tutorial_panel() {
        if (!demo_config_->show_tutorials) return;
        
        if (!ImGui::Begin("📚 Interactive Tutorial", nullptr, 
                         ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::End();
            return;
        }
        
        if (selected_tutorial_.empty()) {
            ImGui::Text("Select a tutorial from the Education Center to begin learning!");
            ImGui::Text("Each tutorial provides hands-on experience with networking concepts.");
        } else {
            render_active_tutorial();
        }
        
        ImGui::End();
    }
    
    void render_active_tutorial() {
        ImGui::Text("Active Tutorial: %s", selected_tutorial_.c_str());
        ImGui::Separator();
        
        // This would show the current tutorial step and interactive elements
        if (selected_tutorial_ == "networking_basics") {
            render_networking_basics_tutorial();
        } else if (selected_tutorial_ == "client_prediction") {
            render_client_prediction_tutorial();
        } else if (selected_tutorial_ == "custom_reliability") {
            render_custom_reliability_tutorial();
        }
    }
    
    void render_networking_basics_tutorial() {
        ImGui::TextColored(ImVec4(0.4f, 0.8f, 0.4f, 1.0f), "Step 1: Understanding Network Protocols");
        ImGui::Text("Watch the network visualization to see packets being sent between clients.");
        ImGui::Text("Notice how different packet types (Data, ACK, Heartbeat) are color-coded.");
        
        ImGui::Spacing();
        ImGui::TextWrapped("💡 TCP vs UDP: TCP guarantees delivery and order but has higher latency. "
                          "UDP is faster but packets can be lost or arrive out of order. "
                          "Our custom protocol adds reliability on top of UDP for the best of both worlds!");
        
        if (ImGui::Button("I understand - Next Step")) {
            // Progress tutorial
        }
    }
    
    void render_client_prediction_tutorial() {
        ImGui::TextColored(ImVec4(0.8f, 0.4f, 0.8f, 1.0f), "Client-Side Prediction Deep Dive");
        ImGui::Text("Watch entities move smoothly despite network latency.");
        ImGui::Text("Green ghosts show where the client predicts entities will be.");
        
        ImGui::Spacing();
        ImGui::TextWrapped("🔮 Prediction reduces perceived latency by guessing where entities will be. "
                          "When the server sends corrections, the client smoothly adjusts. "
                          "This creates responsive gameplay even with network delays!");
        
        // Interactive controls
        if (ImGui::SliderFloat("Artificial Latency", &demo_config_->artificial_latency_ms, 0.0f, 500.0f)) {
            // Update network simulation
        }
    }
    
    void render_custom_reliability_tutorial() {
        ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.4f, 1.0f), "Advanced: Building Reliability Protocols");
        ImGui::Text("Learn how acknowledgments and retransmissions work.");
        
        ImGui::TextWrapped("🔧 Our protocol implements selective acknowledgments, allowing the receiver "
                          "to indicate which packets were received out of a range. This is more "
                          "efficient than acknowledging every packet individually.");
    }
    
    void render_visualization_controls() {
        if (!demo_config_->enable_visualization) return;
        
        if (!ImGui::Begin("🎨 Visualization Controls", nullptr, 
                         ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::End();
            return;
        }
        
        auto& visualizer = edu_system_->get_visualizer();
        auto viz_stats = visualizer.get_visualization_stats();
        
        ImGui::Text("Active Visual Elements: %zu", viz_stats.active_visual_elements);
        ImGui::Text("Packets Visualized: %u", viz_stats.packets_visualized);
        ImGui::Text("Corrections Shown: %u", viz_stats.prediction_corrections_shown);
        
        ImGui::Spacing();
        
        // Animation controls
        f32 speed = viz_stats.animation_speed_multiplier;
        if (ImGui::SliderFloat("Animation Speed", &speed, 0.1f, 5.0f)) {
            visualizer.set_animation_speed(speed);
        }
        
        if (ImGui::Button(viz_stats.is_paused ? "▶️ Resume" : "⏸️ Pause")) {
            visualizer.toggle_pause();
        }
        
        ImGui::Spacing();
        
        // Network condition simulation
        ImGui::Text("Network Condition Simulation");
        ImGui::Separator();
        
        ImGui::SliderFloat("Latency (ms)", &demo_config_->artificial_latency_ms, 0.0f, 500.0f);
        ImGui::SliderFloat("Packet Loss (%)", &demo_config_->packet_loss_rate, 0.0f, 0.2f, "%.1f%%");
        ImGui::SliderFloat("Jitter (ms)", &demo_config_->jitter_ms, 0.0f, 100.0f);
        
        ImGui::End();
    }
    
    void render_detailed_statistics() {
        if (!ImGui::Begin("Detailed Network Statistics", &show_detailed_stats_)) {
            ImGui::End();
            return;
        }
        
        // This would show comprehensive networking statistics
        ImGui::Text("Comprehensive network analysis would be displayed here");
        ImGui::Text("Including bandwidth breakdown, latency histograms, etc.");
        
        ImGui::End();
    }
    
    void render_packet_inspector() {
        if (!ImGui::Begin("Packet Inspector", &show_packet_inspector_)) {
            ImGui::End();
            return;
        }
        
        ImGui::Text("Real-time packet analysis would be shown here");
        ImGui::Text("Including packet headers, payloads, and flow analysis");
        
        ImGui::End();
    }
    
    void render_prediction_analysis() {
        if (!ImGui::Begin("Prediction Analysis", &show_prediction_analysis_)) {
            ImGui::End();
            return;
        }
        
        ImGui::Text("Prediction accuracy metrics and analysis would be displayed");
        ImGui::Text("Including error rates, correction frequencies, etc.");
        
        ImGui::End();
    }
    
    const char* difficulty_level_to_string(networking::education::DifficultyLevel level) {
        switch (level) {
            case networking::education::DifficultyLevel::Beginner: return "Beginner";
            case networking::education::DifficultyLevel::Intermediate: return "Intermediate";
            case networking::education::DifficultyLevel::Advanced: return "Advanced";
            case networking::education::DifficultyLevel::Expert: return "Expert";
        }
        return "Unknown";
    }
};

//=============================================================================
// Main Demo Application
//=============================================================================

class NetworkingDemo {
private:
    // Core ECScope systems
    std::unique_ptr<Window> window_;
    std::unique_ptr<World> world_;
    std::unique_ptr<Renderer2D> renderer_;
    std::unique_ptr<DebugRenderer2D> debug_renderer_;
    std::unique_ptr<Overlay> overlay_;
    
    // Demo-specific systems
    std::unique_ptr<networking::ECSNetworkingSystem> networking_system_;
    std::unique_ptr<networking::education::EducationalNetworkingSystem> educational_system_;
    std::unique_ptr<DemoMovementSystem> movement_system_;
    std::unique_ptr<EducationalOverlaySystem> edu_overlay_system_;
    
    // Configuration and state
    DemoConfig config_;
    DemoStatistics stats_;
    bool running_{true};
    bool show_connection_dialog_{true};
    
    // Performance tracking
    performance::PerformanceBenchmark benchmark_;
    
public:
    NetworkingDemo() : benchmark_("NetworkingDemo") {
        initialize();
    }
    
    ~NetworkingDemo() {
        shutdown();
    }
    
    void run() {
        auto& registry = world_->registry();
        
        // Show initial connection dialog
        if (show_connection_dialog_) {
            show_connection_setup();
        }
        
        // Create demo entities
        create_demo_entities(registry);
        
        // Start educational session
        educational_system_->start_learning_session("comprehensive_networking_demo");
        
        // Main loop
        while (running_ && window_->is_open()) {
            benchmark_.begin_frame();
            
            window_->poll_events();
            
            if (window_->should_close()) {
                running_ = false;
                break;
            }
            
            f32 delta_time = window_->delta_time();
            
            // Update systems
            update_systems(delta_time);
            
            // Render
            render();
            
            // Update statistics
            update_statistics();
            
            benchmark_.end_frame();
            
            // Handle window events
            if (window_->key_pressed(Key::Escape)) {
                running_ = false;
            }
            
            if (window_->key_pressed(Key::F1)) {
                config_.show_tutorials = !config_.show_tutorials;
            }
        }
        
        // End educational session
        educational_system_->end_learning_session();
        
        // Show final learning report
        show_learning_report();
    }

private:
    void initialize() {
        // Initialize ECScope systems
        window_ = std::make_unique<Window>(1400, 900, "ECScope Advanced Networking Demo");
        world_ = std::make_unique<World>();
        renderer_ = std::make_unique<Renderer2D>();
        debug_renderer_ = std::make_unique<DebugRenderer2D>();
        overlay_ = std::make_unique<Overlay>(*window_);
        
        // Initialize networking systems
        auto& registry = world_->registry();
        networking_system_ = std::make_unique<networking::ECSNetworkingSystem>(registry);
        educational_system_ = std::make_unique<networking::education::EducationalNetworkingSystem>();
        
        // Initialize demo systems
        movement_system_ = std::make_unique<DemoMovementSystem>(config_.world_size);
        edu_overlay_system_ = std::make_unique<EducationalOverlaySystem>(
            educational_system_.get(), &stats_, &config_);
        
        // Register systems with world
        world_->register_system<DemoMovementSystem>(movement_system_.get());
        world_->register_system<networking::ECSNetworkingSystem>(networking_system_.get());
        world_->register_system<EducationalOverlaySystem>(edu_overlay_system_.get());
        
        // Setup educational system
        educational_system_->get_visualizer().set_educational_mode(config_.educational_mode);
        educational_system_->get_visualizer().set_detail_level(config_.detail_level);
        
        std::cout << "🎓 ECScope Advanced Networking Demo Initialized\n";
        std::cout << "Features: ECS Sync, Prediction, Custom Protocols, Educational Tools\n";
        std::cout << "Press F1 to toggle tutorials, ESC to exit\n\n";
    }
    
    void show_connection_setup() {
        ImGui::Begin("🌐 Network Connection Setup", &show_connection_dialog_, 
                    ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoCollapse);
        
        ImGui::Text("Choose your role in the networked simulation:");
        ImGui::Spacing();
        
        // Server/Client selection
        ImGui::RadioButton("Server (Host)", &config_.is_server, true);
        ImGui::RadioButton("Client (Connect)", &config_.is_server, false);
        
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
        
        // Connection settings
        if (config_.is_server) {
            ImGui::Text("Server Configuration:");
            ImGui::SliderInt("Max Clients", reinterpret_cast<int*>(&config_.max_clients), 1, 16);
        } else {
            ImGui::Text("Client Configuration:");
            static char address_buffer[64] = "127.0.0.1";
            ImGui::InputText("Server Address", address_buffer, sizeof(address_buffer));
            config_.server_address = std::string(address_buffer);
        }
        
        ImGui::InputScalar("Port", ImGuiDataType_U16, &config_.server_port);
        
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
        
        // Educational settings
        ImGui::Text("Educational Settings:");
        ImGui::Checkbox("Enable Educational Mode", &config_.educational_mode);
        ImGui::Checkbox("Show Interactive Tutorials", &config_.show_tutorials);
        ImGui::Checkbox("Network Visualization", &config_.enable_visualization);
        
        ImGui::Spacing();
        
        // Connect/Start button
        if (ImGui::Button(config_.is_server ? "🚀 Start Server" : "🔌 Connect to Server", ImVec2(200, 40))) {
            if (start_networking()) {
                show_connection_dialog_ = false;
            }
        }
        
        ImGui::SameLine();
        if (ImGui::Button("Demo Mode (No Network)", ImVec2(150, 40))) {
            show_connection_dialog_ = false;
            std::cout << "Running in demo mode without networking\n";
        }
        
        ImGui::End();
    }
    
    bool start_networking() {
        try {
            if (config_.is_server) {
                if (networking_system_->start_server()) {
                    std::cout << "✅ Server started on port " << config_.server_port << "\n";
                    std::cout << "Waiting for clients to connect...\n";
                    return true;
                }
            } else {
                if (networking_system_->start_client()) {
                    std::cout << "✅ Connected to server at " << config_.server_address 
                             << ":" << config_.server_port << "\n";
                    return true;
                }
            }
        } catch (const std::exception& e) {
            std::cerr << "❌ Network error: " << e.what() << "\n";
        }
        
        return false;
    }
    
    void create_demo_entities(ecs::Registry& registry) {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<f32> pos_dist(50.0f, config_.world_size - 50.0f);
        std::uniform_real_distribution<f32> vel_dist(-config_.entity_speed, config_.entity_speed);
        std::uniform_real_distribution<f32> color_dist(0.3f, 1.0f);
        
        for (u32 i = 0; i < config_.num_entities; ++i) {
            auto entity = registry.create();
            
            // Transform component
            Transform transform;
            transform.position = {pos_dist(gen), pos_dist(gen)};
            transform.scale = {10.0f, 10.0f};
            registry.add_component(entity, transform);
            
            // Movement component
            MovementComponent movement;
            movement.velocity = {vel_dist(gen), vel_dist(gen)};
            movement.max_speed = config_.entity_speed;
            registry.add_component(entity, movement);
            
            // Visual component
            RenderComponent render_comp;
            render_comp.color = {color_dist(gen), color_dist(gen), color_dist(gen), 1.0f};
            render_comp.primitive = RenderPrimitive::Circle;
            registry.add_component(entity, render_comp);
            
            // Network component (for some entities)
            if (i < config_.num_entities / 2) {
                NetworkedEntity networked;
                networked.network_id = networking_system_->register_entity(entity);
                networked.authority = 1;  // Local authority for now
                networked.entity_type = "MovingCircle";
                networked.last_sync_time = std::chrono::high_resolution_clock::now();
                registry.add_component(entity, networked);
                
                stats_.networked_entities++;
            } else {
                stats_.local_entities++;
            }
        }
        
        // Create tutorial entities for educational demonstrations
        create_tutorial_entities(registry);
        
        std::cout << "Created " << config_.num_entities << " demo entities\n";
        std::cout << "  - Networked: " << stats_.networked_entities << "\n";
        std::cout << "  - Local: " << stats_.local_entities << "\n";
    }
    
    void create_tutorial_entities(ecs::Registry& registry) {
        // Create special entities for educational demonstrations
        
        // Authority demonstration entity
        auto authority_demo = registry.create();
        Transform auth_transform;
        auth_transform.position = {100.0f, 100.0f};
        auth_transform.scale = {20.0f, 20.0f};
        registry.add_component(authority_demo, auth_transform);
        
        RenderComponent auth_render;
        auth_render.color = {1.0f, 0.8f, 0.2f, 1.0f};  // Gold color
        auth_render.primitive = RenderPrimitive::Square;
        registry.add_component(authority_demo, auth_render);
        
        TutorialEntity auth_tutorial;
        auth_tutorial.concept = "authority_system";
        auth_tutorial.explanation = "This entity demonstrates authority ownership. "
                                   "Only the client with authority can modify it.";
        auth_tutorial.is_interactive = true;
        registry.add_component(authority_demo, auth_tutorial);
        
        // Prediction demonstration entity
        auto prediction_demo = registry.create();
        Transform pred_transform;
        pred_transform.position = {200.0f, 100.0f};
        pred_transform.scale = {15.0f, 15.0f};
        registry.add_component(prediction_demo, pred_transform);
        
        RenderComponent pred_render;
        pred_render.color = {0.2f, 1.0f, 0.2f, 1.0f};  // Green
        pred_render.primitive = RenderPrimitive::Triangle;
        registry.add_component(prediction_demo, pred_render);
        
        MovementComponent pred_movement;
        pred_movement.velocity = {50.0f, 30.0f};
        pred_movement.max_speed = 80.0f;
        registry.add_component(prediction_demo, pred_movement);
        
        TutorialEntity pred_tutorial;
        pred_tutorial.concept = "client_prediction";
        pred_tutorial.explanation = "Watch this entity's prediction ghost to see how "
                                   "the client predicts movement between server updates.";
        registry.add_component(prediction_demo, pred_tutorial);
    }
    
    void update_systems(f32 delta_time) {
        // Update ECScope world (includes all registered systems)
        world_->update(delta_time);
        
        // Update educational system
        educational_system_->update(delta_time);
        
        // Update visualizations if enabled
        if (config_.enable_visualization) {
            update_network_visualization();
        }
    }
    
    void update_network_visualization() {
        auto& visualizer = educational_system_->get_visualizer();
        
        // Visualize entity synchronization
        auto& registry = world_->registry();
        auto networked_view = registry.view<NetworkedEntity, Transform>();
        
        for (auto entity : networked_view) {
            auto& networked = networked_view.get<NetworkedEntity>(entity);
            auto& transform = networked_view.get<Transform>(entity);
            
            // Show authority ownership
            std::array<f32, 2> pos = {transform.position.x, transform.position.y};
            visualizer.visualize_entity_authority(networked.network_id, networked.authority, pos);
            
            // Show prediction if enabled
            if (networked.is_predicted && networked.show_prediction_ghost) {
                // This would get actual predicted position from prediction system
                std::array<f32, 2> predicted_pos = pos;  // Simplified
                visualizer.visualize_prediction(networked.network_id, predicted_pos, pos, 
                                               networked.prediction_confidence);
            }
        }
        
        // Visualize network statistics
        if (networking_system_->is_running()) {
            auto net_stats = networking_system_->get_network_stats();
            stats_.average_ping_ms = net_stats.ping_current / 1000.0f;  // Convert to ms
            stats_.bandwidth_usage_kbps = net_stats.bytes_sent_per_sec / 1024.0f;
            
            // Show bandwidth usage for local client
            f32 bandwidth_percentage = (stats_.bandwidth_usage_kbps / 100.0f) * 100.0f;  // Assume 100 KB/s limit
            visualizer.visualize_bandwidth_usage(networking_system_->get_local_client_id(), 
                                                bandwidth_percentage, 100.0f);
        }
    }
    
    void render() {
        renderer_->begin_frame(window_->width(), window_->height());
        renderer_->clear({0.1f, 0.1f, 0.2f, 1.0f});
        
        // Render entities
        auto& registry = world_->registry();
        auto render_view = registry.view<Transform, RenderComponent>();
        
        for (auto entity : render_view) {
            auto& transform = render_view.get<Transform>(entity);
            auto& render_comp = render_view.get<RenderComponent>(entity);
            
            math::Vec2 position = transform.position;
            math::Vec2 size = transform.scale;
            
            switch (render_comp.primitive) {
                case RenderPrimitive::Circle:
                    renderer_->draw_circle(position, size.x * 0.5f, render_comp.color);
                    break;
                case RenderPrimitive::Square:
                    renderer_->draw_rect(position - size * 0.5f, size, render_comp.color);
                    break;
                case RenderPrimitive::Triangle:
                    // Simplified triangle rendering
                    renderer_->draw_circle(position, size.x * 0.5f, render_comp.color);
                    break;
            }
            
            // Render educational annotations for tutorial entities
            if (registry.has_component<TutorialEntity>(entity)) {
                auto& tutorial = registry.get_component<TutorialEntity>(entity);
                if (tutorial.is_interactive) {
                    // Highlight interactive entities
                    f32 highlight = 0.5f + 0.5f * std::sin(glfwGetTime() * 3.0f);
                    math::Color highlight_color = {1.0f, 1.0f, 1.0f, highlight * 0.3f};
                    renderer_->draw_circle(position, size.x * 0.7f, highlight_color);
                }
            }
        }
        
        // Render network visualization
        if (config_.enable_visualization) {
            render_network_visualization_overlay();
        }
        
        // Render debug information
        render_debug_overlay();
        
        renderer_->end_frame();
        
        // Render UI
        overlay_->begin_frame();
        world_->debug_render();  // This calls all system debug_render methods
        overlay_->end_frame();
        
        window_->present();
    }
    
    void render_network_visualization_overlay() {
        // This would integrate with the NetworkVisualizer to render
        // packets, connections, and educational annotations
        
        auto& visualizer = educational_system_->get_visualizer();
        // visualizer.render(*renderer_);  // Would need renderer integration
        
        // For now, render simple network topology
        if (networking_system_->is_server()) {
            // Render server node
            math::Vec2 server_pos = {config_.world_size * 0.5f, 50.0f};
            renderer_->draw_rect(server_pos - math::Vec2{15.0f, 15.0f}, 
                               {30.0f, 30.0f}, {0.2f, 0.8f, 1.0f, 0.8f});
            
            // Render connections to clients
            auto clients = networking_system_->get_connected_clients();
            for (usize i = 0; i < clients.size(); ++i) {
                f32 angle = (i * 2.0f * 3.14159f) / clients.size();
                math::Vec2 client_pos = server_pos + math::Vec2{
                    std::cos(angle) * 100.0f, 
                    std::sin(angle) * 100.0f + 100.0f
                };
                
                // Draw connection line
                // renderer_->draw_line(server_pos, client_pos, {0.5f, 0.5f, 1.0f, 0.6f});
                
                // Draw client node
                renderer_->draw_circle(client_pos, 8.0f, {0.8f, 0.8f, 0.2f, 0.8f});
            }
        }
    }
    
    void render_debug_overlay() {
        // Show basic debug information
        std::string fps_text = "FPS: " + std::to_string(static_cast<int>(1.0f / window_->delta_time()));
        std::string entity_text = "Entities: " + std::to_string(world_->registry().alive());
        std::string network_text = "Network: " + (networking_system_->is_running() ? "Active" : "Inactive");
        
        // This would use debug renderer to show text
        // debug_renderer_->draw_text({10, 10}, fps_text, {1, 1, 1, 1});
        // debug_renderer_->draw_text({10, 30}, entity_text, {1, 1, 1, 1});
        // debug_renderer_->draw_text({10, 50}, network_text, {1, 1, 1, 1});
    }
    
    void update_statistics() {
        if (networking_system_->is_running()) {
            auto net_stats = networking_system_->get_network_stats();
            auto entity_stats = networking_system_->get_entity_stats();
            
            stats_.packets_sent = static_cast<u32>(net_stats.packets_sent);
            stats_.packets_received = static_cast<u32>(net_stats.packets_received);
            
            // Update educational statistics
            auto learning_stats = educational_system_->get_content_manager().get_learning_stats();
            stats_.tutorials_completed = learning_stats.total_objectives_completed;
            stats_.learning_time_minutes = learning_stats.total_learning_time_hours * 60.0f;
        }
    }
    
    void show_learning_report() {
        auto report = educational_system_->generate_learning_report();
        
        std::cout << "\n🎓 Learning Session Report\n";
        std::cout << "========================\n";
        std::cout << "Session Duration: " << report.session_duration_hours << " hours\n";
        std::cout << "Objectives Completed: " << report.content_stats.total_objectives_completed << "\n";
        std::cout << "Tutorials Completed: " << report.tutorials_completed << "\n";
        std::cout << "Packets Visualized: " << report.visualization_stats.packets_visualized << "\n";
        std::cout << "Prediction Corrections Observed: " << report.visualization_stats.prediction_corrections_shown << "\n";
        
        if (!report.achievements.empty()) {
            std::cout << "\n🏆 Achievements Unlocked:\n";
            for (const auto& achievement : report.achievements) {
                std::cout << "  - " << achievement << "\n";
            }
        }
        
        std::cout << "\nThank you for using ECScope's Advanced Networking Demo!\n";
        std::cout << "Continue exploring to deepen your understanding of distributed systems.\n\n";
    }
    
    void shutdown() {
        if (networking_system_) {
            networking_system_->shutdown();
        }
        
        std::cout << "Demo shutting down. Thanks for learning!\n";
    }
};

//=============================================================================
// Program Entry Point
//=============================================================================

int main(int argc, char* argv[]) {
    std::cout << "🎮 ECScope Advanced Networking Demo\n";
    std::cout << "=====================================\n";
    std::cout << "Educational distributed systems with ECS synchronization\n\n";
    
    try {
        NetworkingDemo demo;
        demo.run();
    } catch (const std::exception& e) {
        std::cerr << "❌ Demo error: " << e.what() << std::endl;
        return -1;
    }
    
    return 0;
}