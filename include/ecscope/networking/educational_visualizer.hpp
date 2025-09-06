#pragma once

/**
 * @file networking/educational_visualizer.hpp
 * @brief Educational Network Visualization and Interactive Learning System
 * 
 * This header provides comprehensive educational features for understanding
 * distributed systems and networking concepts through interactive visualization
 * and hands-on experimentation. Features include:
 * 
 * Core Educational Features:
 * - Real-time network protocol visualization
 * - Interactive network condition simulation
 * - Step-by-step distributed systems tutorials
 * - Comparative analysis of different networking approaches
 * - Visual representation of prediction and reconciliation
 * 
 * Interactive Learning Tools:
 * - Packet flow animation with educational annotations
 * - Bandwidth usage visualization and optimization guides
 * - Latency compensation demonstration
 * - Authority system visualization
 * - Performance impact analysis with different configurations
 * 
 * Simulation Features:
 * - Configurable network conditions (latency, packet loss, jitter)
 * - Client/server role switching for educational purposes
 * - Network topology visualization
 * - Real-time performance metrics with explanations
 * - A/B testing of different networking strategies
 * 
 * Advanced Features:
 * - Custom scenario creation for specific learning objectives
 * - Integration with ECScope performance benchmarking
 * - Export capabilities for educational reports
 * - Multi-language educational content support
 * - Progress tracking and learning achievement system
 * 
 * @author ECScope Educational ECS Framework - Advanced Networking Education
 * @date 2024
 */

#include "network_types.hpp"
#include "network_protocol.hpp"
#include "network_prediction.hpp"
#include "component_sync.hpp"
#include "../memory/arena.hpp"
#include "../performance/performance_benchmark.hpp"
#include <vector>
#include <string>
#include <unordered_map>
#include <functional>
#include <chrono>
#include <queue>
#include <memory>

namespace ecscope::networking::education {

//=============================================================================
// Educational Content System
//=============================================================================

/**
 * @brief Educational Content Types
 */
enum class ContentType : u8 {
    ConceptExplanation,    // Theoretical explanation
    InteractiveDemo,       // Hands-on demonstration
    ComparativeAnalysis,   // Side-by-side comparison
    PerformanceAnalysis,   // Performance impact analysis
    TroubleshootingGuide,  // Problem diagnosis and solution
    BestPracticesGuide,    // Industry best practices
    RealWorldExample       // Real-world use case
};

/**
 * @brief Educational Difficulty Level
 */
enum class DifficultyLevel : u8 {
    Beginner,      // Basic concepts
    Intermediate,  // Common scenarios
    Advanced,      // Complex optimization
    Expert         // Cutting-edge techniques
};

/**
 * @brief Educational Learning Objective
 */
struct LearningObjective {
    std::string title;
    std::string description;
    DifficultyLevel difficulty;
    std::vector<std::string> prerequisites;
    std::function<bool()> completion_check;
    bool completed{false};
    
    /** @brief Check if prerequisites are met */
    bool prerequisites_met(const std::unordered_map<std::string, bool>& completed_objectives) const {
        for (const auto& prereq : prerequisites) {
            auto it = completed_objectives.find(prereq);
            if (it == completed_objectives.end() || !it->second) {
                return false;
            }
        }
        return true;
    }
};

/**
 * @brief Educational Content Item
 */
struct EducationalContent {
    std::string id;
    std::string title;
    std::string content;
    ContentType type;
    DifficultyLevel difficulty;
    std::vector<std::string> tags;
    std::vector<LearningObjective> objectives;
    std::function<void()> interactive_demo;
    
    // Multimedia content
    std::vector<std::string> diagrams;
    std::vector<std::string> code_examples;
    std::vector<std::string> references;
    
    // Engagement metrics
    mutable u32 views{0};
    mutable u32 interactions{0};
    mutable f32 average_time_spent{0.0f};
    
    /** @brief Track content engagement */
    void track_engagement(f32 time_spent) const {
        views++;
        average_time_spent = (average_time_spent * (views - 1) + time_spent) / views;
    }
    
    /** @brief Track interaction */
    void track_interaction() const {
        interactions++;
    }
};

/**
 * @brief Educational Content Manager
 * 
 * Manages educational content delivery, progress tracking, and personalization
 * based on user learning patterns and preferences.
 */
class EducationalContentManager {
private:
    std::unordered_map<std::string, EducationalContent> content_library_;
    std::unordered_map<std::string, bool> completed_objectives_;
    std::unordered_map<DifficultyLevel, std::vector<std::string>> content_by_level_;
    
    // Personalization data
    DifficultyLevel user_level_{DifficultyLevel::Beginner};
    std::vector<std::string> user_interests_;
    f32 learning_speed_{1.0f};  // Multiplier for content pacing
    
    // Progress tracking
    u32 total_objectives_completed_{0};
    NetworkTimestamp learning_session_start_{0};
    f32 total_learning_time_{0.0f};
    
public:
    /** @brief Initialize with default educational content */
    EducationalContentManager() {
        setup_default_content();
    }
    
    /** @brief Get content by ID */
    const EducationalContent* get_content(const std::string& id) const {
        auto it = content_library_.find(id);
        return it != content_library_.end() ? &it->second : nullptr;
    }
    
    /** @brief Get recommended content based on user progress */
    std::vector<std::string> get_recommended_content(usize max_items = 5) const {
        std::vector<std::string> recommendations;
        
        for (const auto& [id, content] : content_library_) {
            // Check if user level is appropriate
            if (static_cast<u8>(content.difficulty) > static_cast<u8>(user_level_) + 1) {
                continue; // Too advanced
            }
            
            // Check if prerequisites are met
            bool prerequisites_met = true;
            for (const auto& objective : content.objectives) {
                if (!objective.prerequisites_met(completed_objectives_)) {
                    prerequisites_met = false;
                    break;
                }
            }
            
            if (prerequisites_met) {
                recommendations.push_back(id);
            }
            
            if (recommendations.size() >= max_items) {
                break;
            }
        }
        
        return recommendations;
    }
    
    /** @brief Mark objective as completed */
    void complete_objective(const std::string& objective_id) {
        completed_objectives_[objective_id] = true;
        total_objectives_completed_++;
        
        // Update user level based on progress
        update_user_level();
    }
    
    /** @brief Start learning session tracking */
    void start_learning_session() {
        learning_session_start_ = timing::now();
    }
    
    /** @brief End learning session tracking */
    void end_learning_session() {
        if (learning_session_start_ > 0) {
            f32 session_time = (timing::now() - learning_session_start_) / 1000000.0f;
            total_learning_time_ += session_time;
            learning_session_start_ = 0;
        }
    }
    
    /** @brief Get learning statistics */
    struct LearningStats {
        u32 total_objectives_completed;
        f32 total_learning_time_hours;
        DifficultyLevel current_level;
        f32 progress_percentage;
        u32 content_items_viewed;
        f32 average_engagement_time;
    };
    
    LearningStats get_learning_stats() const {
        usize total_objectives = 0;
        usize total_views = 0;
        f32 total_engagement = 0.0f;
        
        for (const auto& [id, content] : content_library_) {
            total_objectives += content.objectives.size();
            total_views += content.views;
            total_engagement += content.average_time_spent * content.views;
        }
        
        f32 progress = total_objectives > 0 ? 
                      static_cast<f32>(total_objectives_completed_) / total_objectives * 100.0f : 0.0f;
        
        return LearningStats{
            .total_objectives_completed = total_objectives_completed_,
            .total_learning_time_hours = total_learning_time_ / 3600.0f,
            .current_level = user_level_,
            .progress_percentage = progress,
            .content_items_viewed = static_cast<u32>(total_views),
            .average_engagement_time = total_views > 0 ? total_engagement / total_views : 0.0f
        };
    }

private:
    /** @brief Setup default educational content */
    void setup_default_content() {
        // Beginner: Basic Networking Concepts
        add_content("network_basics", {
            .id = "network_basics",
            .title = "Network Programming Fundamentals",
            .content = "Learn the basics of network communication, including TCP vs UDP, "
                      "client-server architecture, and packet structure.",
            .type = ContentType::ConceptExplanation,
            .difficulty = DifficultyLevel::Beginner,
            .tags = {"basics", "tcp", "udp", "packets"},
            .objectives = {
                {
                    .title = "Understand TCP vs UDP",
                    .description = "Learn the differences between TCP and UDP protocols",
                    .difficulty = DifficultyLevel::Beginner,
                    .completion_check = []() { return true; } // Simplified
                }
            }
        });
        
        // Intermediate: Client-Side Prediction
        add_content("client_prediction", {
            .id = "client_prediction",
            .title = "Client-Side Prediction in Real-Time Games",
            .content = "Understand how client-side prediction reduces perceived latency "
                      "and provides responsive gameplay in networked applications.",
            .type = ContentType::InteractiveDemo,
            .difficulty = DifficultyLevel::Intermediate,
            .tags = {"prediction", "latency", "gaming", "responsiveness"},
            .objectives = {
                {
                    .title = "Implement Basic Prediction",
                    .description = "Create a simple prediction system for entity movement",
                    .difficulty = DifficultyLevel::Intermediate,
                    .prerequisites = {"network_basics"},
                    .completion_check = []() { return true; }
                }
            }
        });
        
        // Advanced: Delta Compression
        add_content("delta_compression", {
            .id = "delta_compression",
            .title = "Delta Compression for Bandwidth Optimization",
            .content = "Learn advanced techniques for minimizing bandwidth usage through "
                      "delta compression, bit packing, and smart synchronization.",
            .type = ContentType::PerformanceAnalysis,
            .difficulty = DifficultyLevel::Advanced,
            .tags = {"compression", "bandwidth", "optimization", "performance"},
            .objectives = {
                {
                    .title = "Implement Delta Compression",
                    .description = "Create a working delta compression system",
                    .difficulty = DifficultyLevel::Advanced,
                    .prerequisites = {"client_prediction"},
                    .completion_check = []() { return true; }
                }
            }
        });
        
        // Expert: Custom Reliability Protocols
        add_content("custom_reliability", {
            .id = "custom_reliability",
            .title = "Building Custom Reliability Protocols",
            .content = "Design and implement custom reliability layers on top of UDP, "
                      "including selective acknowledgments and adaptive retransmission.",
            .type = ContentType::RealWorldExample,
            .difficulty = DifficultyLevel::Expert,
            .tags = {"reliability", "protocols", "acknowledgments", "retransmission"},
            .objectives = {
                {
                    .title = "Create Custom Protocol",
                    .description = "Implement a production-ready reliability protocol",
                    .difficulty = DifficultyLevel::Expert,
                    .prerequisites = {"delta_compression"},
                    .completion_check = []() { return true; }
                }
            }
        });
    }
    
    /** @brief Add content to library */
    void add_content(const std::string& id, EducationalContent content) {
        content_library_[id] = std::move(content);
        content_by_level_[content.difficulty].push_back(id);
    }
    
    /** @brief Update user level based on progress */
    void update_user_level() {
        // Simple progression system based on completed objectives
        if (total_objectives_completed_ >= 10) {
            user_level_ = DifficultyLevel::Expert;
        } else if (total_objectives_completed_ >= 6) {
            user_level_ = DifficultyLevel::Advanced;
        } else if (total_objectives_completed_ >= 3) {
            user_level_ = DifficultyLevel::Intermediate;
        }
    }
};

//=============================================================================
// Network Visualization System
//=============================================================================

/**
 * @brief Visual Element Types
 */
enum class VisualElementType : u8 {
    Packet,           // Individual packet visualization
    Connection,       // Network connection line
    Entity,           // ECS entity representation
    PredictionGhost,  // Predicted entity position
    Authority,        // Authority ownership indicator
    Bandwidth,        // Bandwidth usage visualization
    Latency,          // Latency visualization
    Error             // Error or correction indicator
};

/**
 * @brief Visual Element
 */
struct VisualElement {
    VisualElementType type;
    NetworkTimestamp creation_time;
    NetworkTimestamp expiry_time{0};  // 0 = never expires
    
    // Position and movement
    std::array<f32, 2> position{0.0f, 0.0f};
    std::array<f32, 2> velocity{0.0f, 0.0f};
    std::array<f32, 2> target_position{0.0f, 0.0f};
    
    // Visual properties
    std::array<f32, 4> color{1.0f, 1.0f, 1.0f, 1.0f};  // RGBA
    f32 size{1.0f};
    f32 opacity{1.0f};
    
    // Educational annotations
    std::string label;
    std::string description;
    bool show_tooltip{false};
    
    // Animation state
    f32 animation_progress{0.0f};
    bool is_animating{false};
    
    /** @brief Check if element is expired */
    bool is_expired(NetworkTimestamp current_time) const {
        return expiry_time > 0 && current_time >= expiry_time;
    }
    
    /** @brief Update animation */
    void update_animation(f32 delta_time) {
        if (is_animating) {
            animation_progress += delta_time;
            
            // Animate position towards target
            f32 t = std::clamp(animation_progress * 2.0f, 0.0f, 1.0f);
            position[0] += (target_position[0] - position[0]) * t * delta_time;
            position[1] += (target_position[1] - position[1]) * t * delta_time;
            
            // Check if animation is complete
            if (animation_progress >= 1.0f) {
                position = target_position;
                is_animating = false;
            }
        }
    }
};

/**
 * @brief Network Packet Visualization
 */
struct PacketVisualization {
    u32 sequence_number;
    PacketType type;
    std::array<f32, 2> source_pos;
    std::array<f32, 2> dest_pos;
    f32 journey_progress{0.0f};
    f32 journey_speed{100.0f};  // pixels per second
    bool requires_ack{false};
    bool is_retransmission{false};
    NetworkTimestamp send_time;
    
    /** @brief Update packet journey */
    void update_journey(f32 delta_time) {
        journey_progress += journey_speed * delta_time;
        
        // Calculate current position along the path
        f32 t = std::clamp(journey_progress / 100.0f, 0.0f, 1.0f);
        // Could add bezier curve or other path types here
    }
    
    /** @brief Check if packet has arrived */
    bool has_arrived() const {
        return journey_progress >= 100.0f;
    }
};

/**
 * @brief Network Topology Visualization
 */
struct NetworkTopology {
    struct Node {
        ClientID client_id;
        std::array<f32, 2> position;
        ConnectionState state;
        std::string label;
        bool is_server{false};
    };
    
    struct Connection {
        ClientID from_client;
        ClientID to_client;
        f32 latency_ms;
        f32 packet_loss_rate;
        f32 bandwidth_utilization;
        std::array<f32, 4> color{0.5f, 0.5f, 1.0f, 1.0f};
    };
    
    std::vector<Node> nodes;
    std::vector<Connection> connections;
    
    /** @brief Add node to topology */
    void add_node(ClientID client_id, const std::array<f32, 2>& position, 
                  const std::string& label, bool is_server = false) {
        nodes.push_back({client_id, position, ConnectionState::Connected, label, is_server});
    }
    
    /** @brief Add connection between nodes */
    void add_connection(ClientID from, ClientID to, f32 latency_ms = 50.0f) {
        connections.push_back({from, to, latency_ms, 0.0f, 0.0f});
    }
    
    /** @brief Update connection statistics */
    void update_connection_stats(ClientID from, ClientID to, f32 latency, f32 loss, f32 bandwidth) {
        for (auto& conn : connections) {
            if (conn.from_client == from && conn.to_client == to) {
                conn.latency_ms = latency;
                conn.packet_loss_rate = loss;
                conn.bandwidth_utilization = bandwidth;
                
                // Update color based on connection quality
                f32 quality = 1.0f - std::max(loss, bandwidth);
                conn.color = {1.0f - quality, quality, 0.0f, 1.0f};
                break;
            }
        }
    }
};

/**
 * @brief Interactive Network Visualizer
 * 
 * Provides real-time visualization of network activity with educational
 * annotations and interactive exploration capabilities.
 */
class NetworkVisualizer {
private:
    // Visual elements
    std::vector<VisualElement> visual_elements_;
    std::vector<PacketVisualization> packet_visualizations_;
    NetworkTopology topology_;
    
    // Educational state
    bool educational_mode_{true};
    bool show_annotations_{true};
    bool show_performance_metrics_{true};
    DifficultyLevel detail_level_{DifficultyLevel::Beginner};
    
    // Animation and timing
    NetworkTimestamp last_update_time_{0};
    f32 animation_speed_multiplier_{1.0f};
    
    // Interactive state
    std::array<f32, 2> mouse_position_{0.0f, 0.0f};
    VisualElement* hovered_element_{nullptr};
    bool is_paused_{false};
    
    // Statistics for educational purposes
    u32 packets_visualized_{0};
    u32 prediction_corrections_shown_{0};
    u32 bandwidth_warnings_displayed_{0};
    
    // Memory management
    memory::Arena visualization_arena_;
    
public:
    /** @brief Initialize visualizer */
    explicit NetworkVisualizer(usize arena_size = 1024 * 1024)  // 1MB
        : visualization_arena_(arena_size) {}
    
    /** @brief Set educational mode */
    void set_educational_mode(bool enabled) {
        educational_mode_ = enabled;
    }
    
    /** @brief Set detail level for educational content */
    void set_detail_level(DifficultyLevel level) {
        detail_level_ = level;
    }
    
    /** @brief Update visualizer state */
    void update(f32 delta_time) {
        NetworkTimestamp current_time = timing::now();
        
        if (is_paused_) return;
        
        // Apply animation speed multiplier
        f32 adjusted_delta = delta_time * animation_speed_multiplier_;
        
        // Update visual elements
        for (auto& element : visual_elements_) {
            element.update_animation(adjusted_delta);
        }
        
        // Update packet visualizations
        for (auto& packet : packet_visualizations_) {
            packet.update_journey(adjusted_delta);
        }
        
        // Remove expired elements
        cleanup_expired_elements(current_time);
        
        last_update_time_ = current_time;
    }
    
    //-------------------------------------------------------------------------
    // Packet Visualization
    //-------------------------------------------------------------------------
    
    /** @brief Visualize packet transmission */
    void visualize_packet_transmission(u32 sequence_number, PacketType type,
                                      ClientID from_client, ClientID to_client,
                                      bool requires_ack = false, bool is_retransmission = false) {
        auto from_pos = get_client_position(from_client);
        auto to_pos = get_client_position(to_client);
        
        PacketVisualization packet_viz;
        packet_viz.sequence_number = sequence_number;
        packet_viz.type = type;
        packet_viz.source_pos = from_pos;
        packet_viz.dest_pos = to_pos;
        packet_viz.requires_ack = requires_ack;
        packet_viz.is_retransmission = is_retransmission;
        packet_viz.send_time = timing::now();
        
        packet_visualizations_.push_back(packet_viz);
        packets_visualized_++;
        
        if (educational_mode_) {
            add_educational_annotation(from_pos, 
                "Packet " + std::to_string(sequence_number) + " sent (" + 
                packet_type_to_string(type) + ")", 2000);
        }
    }
    
    /** @brief Visualize packet acknowledgment */
    void visualize_acknowledgment(u32 ack_sequence, ClientID from_client, ClientID to_client) {
        visualize_packet_transmission(ack_sequence, PacketType::Acknowledgment, 
                                    from_client, to_client, false, false);
        
        if (educational_mode_) {
            auto pos = get_client_position(to_client);
            add_educational_annotation(pos, 
                "ACK received for packet " + std::to_string(ack_sequence), 1500);
        }
    }
    
    /** @brief Visualize packet loss */
    void visualize_packet_loss(u32 sequence_number, ClientID from_client, ClientID to_client) {
        auto midpoint = calculate_midpoint(get_client_position(from_client), 
                                          get_client_position(to_client));
        
        // Create error indicator
        VisualElement error_element;
        error_element.type = VisualElementType::Error;
        error_element.position = midpoint;
        error_element.color = {1.0f, 0.0f, 0.0f, 1.0f};  // Red
        error_element.size = 2.0f;
        error_element.label = "LOST";
        error_element.description = "Packet " + std::to_string(sequence_number) + " was lost";
        error_element.creation_time = timing::now();
        error_element.expiry_time = error_element.creation_time + 3000000;  // 3 seconds
        
        visual_elements_.push_back(error_element);
        
        if (educational_mode_) {
            add_educational_annotation(midpoint, 
                "Packet loss detected! This will trigger retransmission.", 3000);
        }
    }
    
    //-------------------------------------------------------------------------
    // Prediction Visualization
    //-------------------------------------------------------------------------
    
    /** @brief Visualize client-side prediction */
    void visualize_prediction(NetworkEntityID entity_id, 
                             const std::array<f32, 2>& predicted_pos,
                             const std::array<f32, 2>& actual_pos,
                             f32 confidence) {
        // Create prediction ghost
        VisualElement prediction_ghost;
        prediction_ghost.type = VisualElementType::PredictionGhost;
        prediction_ghost.position = predicted_pos;
        prediction_ghost.color = {0.0f, 1.0f, 0.0f, 0.3f};  // Semi-transparent green
        prediction_ghost.size = 1.0f;
        prediction_ghost.label = "PREDICTED";
        prediction_ghost.creation_time = timing::now();
        prediction_ghost.expiry_time = prediction_ghost.creation_time + 1000000;  // 1 second
        
        visual_elements_.push_back(prediction_ghost);
        
        // Calculate prediction error
        f32 error = std::sqrt((predicted_pos[0] - actual_pos[0]) * (predicted_pos[0] - actual_pos[0]) +
                             (predicted_pos[1] - actual_pos[1]) * (predicted_pos[1] - actual_pos[1]));
        
        if (educational_mode_ && error > 10.0f) {  // Significant prediction error
            add_educational_annotation(actual_pos,
                "Prediction error: " + std::to_string(error) + " units. "
                "This will trigger correction.", 2500);
            prediction_corrections_shown_++;
        }
    }
    
    /** @brief Visualize prediction correction */
    void visualize_prediction_correction(NetworkEntityID entity_id,
                                        const std::array<f32, 2>& from_pos,
                                        const std::array<f32, 2>& to_pos) {
        // Create correction arrow
        VisualElement correction;
        correction.type = VisualElementType::Error;  // Reuse error type with different color
        correction.position = from_pos;
        correction.target_position = to_pos;
        correction.color = {1.0f, 0.5f, 0.0f, 0.8f};  // Orange
        correction.size = 1.5f;
        correction.label = "CORRECTION";
        correction.is_animating = true;
        correction.creation_time = timing::now();
        correction.expiry_time = correction.creation_time + 1500000;  // 1.5 seconds
        
        visual_elements_.push_back(correction);
    }
    
    //-------------------------------------------------------------------------
    // Authority Visualization
    //-------------------------------------------------------------------------
    
    /** @brief Visualize entity authority */
    void visualize_entity_authority(NetworkEntityID entity_id, ClientID authority_client,
                                   const std::array<f32, 2>& entity_pos) {
        VisualElement authority_indicator;
        authority_indicator.type = VisualElementType::Authority;
        authority_indicator.position = entity_pos;
        authority_indicator.color = get_client_color(authority_client);
        authority_indicator.size = 0.8f;
        authority_indicator.label = "AUTH:" + std::to_string(authority_client);
        authority_indicator.creation_time = timing::now();
        
        visual_elements_.push_back(authority_indicator);
    }
    
    /** @brief Visualize authority transfer */
    void visualize_authority_transfer(NetworkEntityID entity_id,
                                     ClientID from_client, ClientID to_client,
                                     const std::array<f32, 2>& entity_pos) {
        if (educational_mode_) {
            add_educational_annotation(entity_pos,
                "Authority transferred from client " + std::to_string(from_client) +
                " to client " + std::to_string(to_client), 3000);
        }
    }
    
    //-------------------------------------------------------------------------
    // Performance Visualization
    //-------------------------------------------------------------------------
    
    /** @brief Visualize bandwidth usage */
    void visualize_bandwidth_usage(ClientID client_id, f32 usage_percentage, f32 limit_kbps) {
        auto client_pos = get_client_position(client_id);
        
        // Offset position for bandwidth indicator
        std::array<f32, 2> indicator_pos = {client_pos[0] + 50.0f, client_pos[1] - 30.0f};
        
        VisualElement bandwidth_indicator;
        bandwidth_indicator.type = VisualElementType::Bandwidth;
        bandwidth_indicator.position = indicator_pos;
        bandwidth_indicator.size = usage_percentage;  // Size represents usage
        bandwidth_indicator.label = std::to_string(static_cast<u32>(usage_percentage)) + "%";
        bandwidth_indicator.creation_time = timing::now();
        
        // Color coding: Green -> Yellow -> Red
        if (usage_percentage < 50.0f) {
            bandwidth_indicator.color = {0.0f, 1.0f, 0.0f, 0.7f};  // Green
        } else if (usage_percentage < 80.0f) {
            bandwidth_indicator.color = {1.0f, 1.0f, 0.0f, 0.7f};  // Yellow
        } else {
            bandwidth_indicator.color = {1.0f, 0.0f, 0.0f, 0.7f};  // Red
            
            if (educational_mode_) {
                add_educational_annotation(indicator_pos,
                    "High bandwidth usage detected! Consider optimizing data transmission.", 4000);
                bandwidth_warnings_displayed_++;
            }
        }
        
        visual_elements_.push_back(bandwidth_indicator);
    }
    
    /** @brief Visualize network latency */
    void visualize_latency(ClientID from_client, ClientID to_client, f32 latency_ms) {
        auto from_pos = get_client_position(from_client);
        auto to_pos = get_client_position(to_client);
        auto midpoint = calculate_midpoint(from_pos, to_pos);
        
        VisualElement latency_indicator;
        latency_indicator.type = VisualElementType::Latency;
        latency_indicator.position = midpoint;
        latency_indicator.label = std::to_string(static_cast<u32>(latency_ms)) + "ms";
        latency_indicator.size = std::clamp(latency_ms / 100.0f, 0.5f, 3.0f);
        latency_indicator.creation_time = timing::now();
        latency_indicator.expiry_time = latency_indicator.creation_time + 2000000;  // 2 seconds
        
        // Color based on latency
        if (latency_ms < 50.0f) {
            latency_indicator.color = {0.0f, 1.0f, 0.0f, 0.6f};  // Green
        } else if (latency_ms < 150.0f) {
            latency_indicator.color = {1.0f, 1.0f, 0.0f, 0.6f};  // Yellow
        } else {
            latency_indicator.color = {1.0f, 0.0f, 0.0f, 0.6f};  // Red
        }
        
        visual_elements_.push_back(latency_indicator);
    }
    
    //-------------------------------------------------------------------------
    // Interactive Features
    //-------------------------------------------------------------------------
    
    /** @brief Handle mouse movement for interactivity */
    void handle_mouse_move(f32 x, f32 y) {
        mouse_position_ = {x, y};
        
        // Find hovered element
        hovered_element_ = nullptr;
        for (auto& element : visual_elements_) {
            f32 dx = x - element.position[0];
            f32 dy = y - element.position[1];
            f32 distance = std::sqrt(dx * dx + dy * dy);
            
            if (distance <= element.size * 10.0f) {  // Hit test radius
                hovered_element_ = &element;
                element.show_tooltip = true;
                break;
            } else {
                element.show_tooltip = false;
            }
        }
    }
    
    /** @brief Toggle pause state */
    void toggle_pause() {
        is_paused_ = !is_paused_;
    }
    
    /** @brief Set animation speed */
    void set_animation_speed(f32 multiplier) {
        animation_speed_multiplier_ = std::clamp(multiplier, 0.1f, 5.0f);
    }
    
    //-------------------------------------------------------------------------
    // Topology Management
    //-------------------------------------------------------------------------
    
    /** @brief Setup network topology */
    void setup_topology(const std::vector<ClientID>& clients, ClientID server_id) {
        topology_ = NetworkTopology{};
        
        // Add server at center
        topology_.add_node(server_id, {400.0f, 300.0f}, "SERVER", true);
        
        // Add clients in a circle around server
        f32 radius = 200.0f;
        f32 angle_step = 2.0f * 3.14159f / clients.size();
        
        for (usize i = 0; i < clients.size(); ++i) {
            f32 angle = i * angle_step;
            f32 x = 400.0f + radius * std::cos(angle);
            f32 y = 300.0f + radius * std::sin(angle);
            
            std::string label = "CLIENT " + std::to_string(clients[i]);
            topology_.add_node(clients[i], {x, y}, label);
            topology_.add_connection(clients[i], server_id);
        }
    }
    
    /** @brief Get visualization statistics for educational purposes */
    struct VisualizationStats {
        u32 packets_visualized;
        u32 prediction_corrections_shown;
        u32 bandwidth_warnings_displayed;
        usize active_visual_elements;
        f32 animation_speed_multiplier;
        bool is_paused;
    };
    
    VisualizationStats get_visualization_stats() const {
        return VisualizationStats{
            .packets_visualized = packets_visualized_,
            .prediction_corrections_shown = prediction_corrections_shown_,
            .bandwidth_warnings_displayed = bandwidth_warnings_displayed_,
            .active_visual_elements = visual_elements_.size(),
            .animation_speed_multiplier = animation_speed_multiplier_,
            .is_paused = is_paused_
        };
    }

private:
    /** @brief Get client position in topology */
    std::array<f32, 2> get_client_position(ClientID client_id) const {
        for (const auto& node : topology_.nodes) {
            if (node.client_id == client_id) {
                return node.position;
            }
        }
        return {0.0f, 0.0f};  // Default position
    }
    
    /** @brief Get color associated with client */
    std::array<f32, 4> get_client_color(ClientID client_id) const {
        // Generate consistent color based on client ID
        f32 hue = (client_id * 137.508f) / 360.0f;  // Golden angle
        return {hue, 1.0f, 1.0f, 1.0f};  // HSV to be converted to RGB
    }
    
    /** @brief Calculate midpoint between two positions */
    std::array<f32, 2> calculate_midpoint(const std::array<f32, 2>& a, 
                                         const std::array<f32, 2>& b) const {
        return {(a[0] + b[0]) * 0.5f, (a[1] + b[1]) * 0.5f};
    }
    
    /** @brief Add educational annotation */
    void add_educational_annotation(const std::array<f32, 2>& position,
                                   const std::string& text, u32 duration_ms) {
        if (!educational_mode_) return;
        
        VisualElement annotation;
        annotation.type = VisualElementType::Entity;  // Reuse for text
        annotation.position = position;
        annotation.color = {1.0f, 1.0f, 1.0f, 0.9f};
        annotation.label = text;
        annotation.creation_time = timing::now();
        annotation.expiry_time = annotation.creation_time + duration_ms * 1000;
        
        visual_elements_.push_back(annotation);
    }
    
    /** @brief Remove expired visual elements */
    void cleanup_expired_elements(NetworkTimestamp current_time) {
        visual_elements_.erase(
            std::remove_if(visual_elements_.begin(), visual_elements_.end(),
                          [current_time](const VisualElement& element) {
                              return element.is_expired(current_time);
                          }),
            visual_elements_.end()
        );
        
        packet_visualizations_.erase(
            std::remove_if(packet_visualizations_.begin(), packet_visualizations_.end(),
                          [](const PacketVisualization& packet) {
                              return packet.has_arrived();
                          }),
            packet_visualizations_.end()
        );
    }
    
    /** @brief Convert packet type to string */
    std::string packet_type_to_string(PacketType type) const {
        switch (type) {
            case PacketType::Data: return "Data";
            case PacketType::Acknowledgment: return "ACK";
            case PacketType::ConnectRequest: return "Connect";
            case PacketType::Heartbeat: return "Heartbeat";
            case PacketType::Fragment: return "Fragment";
            default: return "Unknown";
        }
    }
};

//=============================================================================
// Complete Educational System Integration
//=============================================================================

/**
 * @brief Educational Networking System
 * 
 * Combines content management, visualization, and interactive learning
 * into a comprehensive educational experience for distributed systems.
 */
class EducationalNetworkingSystem {
private:
    EducationalContentManager content_manager_;
    NetworkVisualizer visualizer_;
    
    // Learning session state
    bool learning_session_active_{false};
    std::string current_lesson_;
    NetworkTimestamp session_start_time_{0};
    
    // Interactive tutorials
    struct Tutorial {
        std::string id;
        std::string title;
        std::vector<std::string> steps;
        usize current_step{0};
        bool completed{false};
        std::function<bool()> step_completion_check;
    };
    
    std::unordered_map<std::string, Tutorial> tutorials_;
    std::string active_tutorial_;
    
public:
    /** @brief Initialize educational system */
    EducationalNetworkingSystem() {
        setup_tutorials();
    }
    
    /** @brief Start learning session */
    void start_learning_session(const std::string& lesson_id = "") {
        learning_session_active_ = true;
        current_lesson_ = lesson_id;
        session_start_time_ = timing::now();
        
        content_manager_.start_learning_session();
        visualizer_.set_educational_mode(true);
    }
    
    /** @brief End learning session */
    void end_learning_session() {
        learning_session_active_ = false;
        content_manager_.end_learning_session();
    }
    
    /** @brief Update educational system */
    void update(f32 delta_time) {
        visualizer_.update(delta_time);
        
        // Update active tutorial
        if (!active_tutorial_.empty()) {
            update_active_tutorial();
        }
    }
    
    /** @brief Get content manager */
    EducationalContentManager& get_content_manager() {
        return content_manager_;
    }
    
    /** @brief Get visualizer */
    NetworkVisualizer& get_visualizer() {
        return visualizer_;
    }
    
    /** @brief Start interactive tutorial */
    void start_tutorial(const std::string& tutorial_id) {
        auto it = tutorials_.find(tutorial_id);
        if (it != tutorials_.end()) {
            active_tutorial_ = tutorial_id;
            it->second.current_step = 0;
            it->second.completed = false;
        }
    }
    
    /** @brief Get comprehensive learning report */
    struct LearningReport {
        EducationalContentManager::LearningStats content_stats;
        NetworkVisualizer::VisualizationStats visualization_stats;
        u32 tutorials_completed;
        f32 session_duration_hours;
        std::vector<std::string> achievements;
    };
    
    LearningReport generate_learning_report() const {
        auto content_stats = content_manager_.get_learning_stats();
        auto viz_stats = visualizer_.get_visualization_stats();
        
        // Count completed tutorials
        u32 completed_tutorials = 0;
        for (const auto& [id, tutorial] : tutorials_) {
            if (tutorial.completed) {
                completed_tutorials++;
            }
        }
        
        // Calculate session duration
        f32 session_duration = learning_session_active_ ? 
            (timing::now() - session_start_time_) / 3600000000.0f : 0.0f;
        
        // Generate achievements
        std::vector<std::string> achievements;
        if (content_stats.total_objectives_completed >= 5) {
            achievements.push_back("Network Apprentice");
        }
        if (content_stats.total_objectives_completed >= 10) {
            achievements.push_back("Distributed Systems Expert");
        }
        if (viz_stats.packets_visualized >= 100) {
            achievements.push_back("Packet Inspector");
        }
        
        return LearningReport{
            .content_stats = content_stats,
            .visualization_stats = viz_stats,
            .tutorials_completed = completed_tutorials,
            .session_duration_hours = session_duration,
            .achievements = achievements
        };
    }

private:
    /** @brief Setup interactive tutorials */
    void setup_tutorials() {
        // Basic networking tutorial
        tutorials_["networking_basics"] = Tutorial{
            .id = "networking_basics",
            .title = "Network Programming Fundamentals",
            .steps = {
                "Understand the difference between TCP and UDP",
                "Learn about client-server architecture",
                "Explore packet structure and headers",
                "Practice with basic connection handling"
            },
            .step_completion_check = []() { return true; }  // Simplified
        };
        
        // Client-side prediction tutorial
        tutorials_["client_prediction"] = Tutorial{
            .id = "client_prediction",
            .title = "Client-Side Prediction Deep Dive",
            .steps = {
                "Understand why prediction is necessary",
                "Implement basic linear prediction",
                "Handle prediction corrections",
                "Optimize for different network conditions"
            },
            .step_completion_check = []() { return true; }
        };
    }
    
    /** @brief Update active tutorial progress */
    void update_active_tutorial() {
        auto it = tutorials_.find(active_tutorial_);
        if (it == tutorials_.end()) return;
        
        auto& tutorial = it->second;
        
        // Check if current step is completed
        if (tutorial.step_completion_check && tutorial.step_completion_check()) {
            tutorial.current_step++;
            
            // Check if tutorial is completed
            if (tutorial.current_step >= tutorial.steps.size()) {
                tutorial.completed = true;
                active_tutorial_.clear();
                
                // Mark related objectives as completed
                content_manager_.complete_objective(tutorial.id);
            }
        }
    }
};

} // namespace ecscope::networking::education