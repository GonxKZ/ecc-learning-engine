#pragma once

/**
 * @file networking/authority_system.hpp
 * @brief Distributed Authority System for ECScope ECS Networking
 * 
 * This header implements a comprehensive authority system that manages
 * distributed ownership and control of entities across network clients.
 * The system handles:
 * 
 * Core Authority Features:
 * - Dynamic authority assignment and transfer
 * - Authority conflict resolution and arbitration
 * - Ownership-based update permissions and validation
 * - Authority migration with seamless handoffs
 * - Hierarchical authority for complex entity relationships
 * 
 * Authority Distribution Strategies:
 * - Player-centric ownership (players own nearby entities)
 * - Server-authoritative critical systems (health, scoring)
 * - Load-balanced authority distribution
 * - Interest-based authority assignment
 * - Performance-aware authority migration
 * 
 * Educational Features:
 * - Visual authority mapping and conflict visualization
 * - Real-time authority transfer monitoring
 * - Interactive authority assignment experimentation
 * - Educational explanations of distributed ownership
 * - Authority performance impact analysis
 * 
 * Performance Optimizations:
 * - Efficient authority lookup data structures
 * - Batch authority transfer operations
 * - Predictive authority pre-assignment
 * - Cache-friendly authority storage
 * - Zero-allocation authority queries
 * 
 * @author ECScope Educational ECS Framework - Networking Layer
 * @date 2024
 */

#include "network_types.hpp"
#include "entity_replication.hpp"
#include "../entity.hpp"
#include "../core/types.hpp"
#include "../memory/arena.hpp"
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <optional>
#include <functional>
#include <algorithm>

namespace ecscope::networking::authority {

//=============================================================================
// Authority Types and Levels
//=============================================================================

/**
 * @brief Authority Level
 * 
 * Defines different levels of authority a client can have over an entity.
 * Higher authority levels override lower ones in conflict resolution.
 */
enum class AuthorityLevel : u8 {
    /** @brief No authority - cannot modify entity */
    None = 0,
    
    /** @brief Read-only access - can observe but not modify */
    Observer = 1,
    
    /** @brief Limited control - can modify specific components only */
    Limited = 2,
    
    /** @brief Full control - can modify all components except critical ones */
    Full = 3,
    
    /** @brief Complete control - can modify everything including critical components */
    Complete = 4,
    
    /** @brief Server authority - ultimate authority, cannot be overridden */
    Server = 255
};

/**
 * @brief Authority Scope
 * 
 * Defines what aspects of an entity the authority applies to.
 * Allows for fine-grained authority control.
 */
enum class AuthorityScope : u32 {
    /** @brief Authority over entity existence (create/destroy) */
    EntityLifecycle = 1 << 0,
    
    /** @brief Authority over transform components (position, rotation, scale) */
    Transform = 1 << 1,
    
    /** @brief Authority over physics components (velocity, forces) */
    Physics = 1 << 2,
    
    /** @brief Authority over rendering components (materials, animations) */
    Rendering = 1 << 3,
    
    /** @brief Authority over gameplay components (health, inventory) */
    Gameplay = 1 << 4,
    
    /** @brief Authority over audio components */
    Audio = 1 << 5,
    
    /** @brief Authority over AI components */
    AI = 1 << 6,
    
    /** @brief Authority over networking components */
    Networking = 1 << 7,
    
    /** @brief Authority over all components */
    All = 0xFFFFFFFF
};

inline AuthorityScope operator|(AuthorityScope a, AuthorityScope b) {
    return static_cast<AuthorityScope>(static_cast<u32>(a) | static_cast<u32>(b));
}

inline AuthorityScope operator&(AuthorityScope a, AuthorityScope b) {
    return static_cast<AuthorityScope>(static_cast<u32>(a) & static_cast<u32>(b));
}

inline bool has_authority_scope(AuthorityScope authority, AuthorityScope scope) {
    return (authority & scope) == scope;
}

/**
 * @brief Authority Assignment Strategy
 * 
 * Defines how authority should be assigned for different types of entities.
 */
enum class AuthorityStrategy : u8 {
    /** @brief Server maintains authority */
    ServerAuthoritative,
    
    /** @brief Client who created entity has authority */
    CreatorOwned,
    
    /** @brief Closest client to entity has authority */
    ProximityBased,
    
    /** @brief Client with best network connection has authority */
    NetworkOptimal,
    
    /** @brief Client with lowest CPU load has authority */
    LoadBalanced,
    
    /** @brief Player who interacted with entity last has authority */
    InteractionBased,
    
    /** @brief Manually assigned authority */
    Manual
};

//=============================================================================
// Authority Record and Tracking
//=============================================================================

/**
 * @brief Authority Record
 * 
 * Comprehensive record of authority assignment for an entity,
 * including history and conflict resolution information.
 */
struct AuthorityRecord {
    NetworkEntityID entity_id{0};         // Entity this authority record applies to
    ClientID current_authority{0};         // Client currently holding authority
    AuthorityLevel authority_level{AuthorityLevel::None}; // Level of authority
    AuthorityScope authority_scope{AuthorityScope::All};  // Scope of authority
    AuthorityStrategy assignment_strategy{AuthorityStrategy::ServerAuthoritative}; // How authority was assigned
    
    // Authority history
    NetworkTimestamp authority_acquired{0}; // When current authority was acquired
    NetworkTimestamp last_authority_use{0}; // Last time authority was exercised
    ClientID previous_authority{0};         // Previous authority holder
    u32 authority_transfer_count{0};        // Number of authority transfers
    
    // Conflict resolution
    std::vector<std::pair<ClientID, NetworkTimestamp>> authority_claims; // Pending authority claims
    bool has_conflicts{false};              // Whether there are unresolved conflicts
    
    // Performance tracking
    f32 authority_utilization{0.0f};        // How actively authority is being used (0-1)
    f32 network_performance_score{1.0f};    // Network performance of authority holder (0-1)
    
    /** @brief Check if client has authority */
    bool has_authority(ClientID client_id) const noexcept {
        return current_authority == client_id && authority_level > AuthorityLevel::None;
    }
    
    /** @brief Check if client has authority for specific scope */
    bool has_authority_for_scope(ClientID client_id, AuthorityScope scope) const noexcept {
        return has_authority(client_id) && has_authority_scope(authority_scope, scope);
    }
    
    /** @brief Check if authority has been used recently */
    bool is_authority_active(NetworkTimestamp current_time, u64 timeout_us = 5000000) const noexcept { // 5 second default
        return (current_time - last_authority_use) <= timeout_us;
    }
    
    /** @brief Add authority claim */
    void add_authority_claim(ClientID client_id, NetworkTimestamp timestamp) {
        // Remove existing claim from same client
        authority_claims.erase(
            std::remove_if(authority_claims.begin(), authority_claims.end(),
                          [client_id](const auto& claim) { return claim.first == client_id; }),
            authority_claims.end());
        
        authority_claims.emplace_back(client_id, timestamp);
        has_conflicts = authority_claims.size() > 1;
    }
    
    /** @brief Clear authority claims */
    void clear_authority_claims() {
        authority_claims.clear();
        has_conflicts = false;
    }
    
    /** @brief Transfer authority to new client */
    void transfer_authority(ClientID new_authority, AuthorityLevel level, 
                           AuthorityScope scope, NetworkTimestamp timestamp) {
        previous_authority = current_authority;
        current_authority = new_authority;
        authority_level = level;
        authority_scope = scope;
        authority_acquired = timestamp;
        authority_transfer_count++;
        clear_authority_claims();
    }
};

//=============================================================================
// Authority Manager
//=============================================================================

/**
 * @brief Authority System Configuration
 */
struct AuthorityConfig {
    // Authority timeout and cleanup
    u64 authority_timeout_us{30000000};     // 30 seconds - authority expires if unused
    u64 claim_timeout_us{5000000};          // 5 seconds - claims expire
    u64 conflict_resolution_timeout_us{1000000}; // 1 second - time to resolve conflicts
    
    // Authority assignment preferences
    AuthorityStrategy default_strategy{AuthorityStrategy::ProximityBased};
    AuthorityLevel default_level{AuthorityLevel::Full};
    AuthorityScope default_scope{AuthorityScope::All};
    
    // Performance thresholds
    f32 min_network_performance{0.5f};      // Minimum network performance to hold authority
    f32 min_authority_utilization{0.1f};    // Minimum utilization before considering transfer
    u32 max_entities_per_client{100};       // Maximum entities one client can have authority over
    
    // Proximity-based authority settings
    f32 proximity_radius{50.0f};            // Radius for proximity-based authority
    f32 proximity_hysteresis{10.0f};        // Hysteresis to prevent authority thrashing
    
    // Server authority overrides
    bool server_override_conflicts{true};    // Server can override authority conflicts
    bool server_force_critical_authority{true}; // Server forces authority for critical entities
    
    /** @brief Create server-authoritative configuration */
    static AuthorityConfig server_authoritative() {
        AuthorityConfig config;
        config.default_strategy = AuthorityStrategy::ServerAuthoritative;
        config.default_level = AuthorityLevel::Complete;
        config.server_force_critical_authority = true;
        return config;
    }
    
    /** @brief Create client-distributed configuration */
    static AuthorityConfig client_distributed() {
        AuthorityConfig config;
        config.default_strategy = AuthorityStrategy::ProximityBased;
        config.default_level = AuthorityLevel::Full;
        config.max_entities_per_client = 50;
        return config;
    }
};

/**
 * @brief Client Connection Information
 * 
 * Information about connected clients used for authority assignment decisions.
 */
struct ClientInfo {
    ClientID client_id{0};
    std::array<f32, 3> position{0, 0, 0};   // Client's position in world space
    f32 network_latency{0.0f};              // Network latency in seconds
    f32 packet_loss_rate{0.0f};             // Packet loss rate (0-1)
    f32 cpu_load{0.0f};                     // CPU load (0-1)
    u32 entities_owned{0};                  // Number of entities currently owned
    NetworkTimestamp last_seen{0};          // Last time we heard from this client
    bool is_server{false};                  // Whether this is the server
    
    /** @brief Calculate network performance score */
    f32 network_performance() const noexcept {
        f32 latency_score = std::max(0.0f, 1.0f - (network_latency / 0.5f)); // 500ms baseline
        f32 loss_score = 1.0f - packet_loss_rate;
        return (latency_score * 0.7f) + (loss_score * 0.3f);
    }
    
    /** @brief Calculate load score (lower is better) */
    f32 load_score() const noexcept {
        f32 cpu_score = 1.0f - cpu_load;
        f32 entity_score = entities_owned < 50 ? 1.0f : std::max(0.0f, 1.0f - ((entities_owned - 50) / 50.0f));
        return (cpu_score * 0.6f) + (entity_score * 0.4f);
    }
};

/**
 * @brief Authority System Manager
 * 
 * Main class that manages all aspects of the distributed authority system.
 * Handles authority assignment, transfer, conflict resolution, and optimization.
 */
class AuthorityManager {
private:
    AuthorityConfig config_;
    ClientID local_client_id_{0};
    bool is_server_{false};
    
    // Authority tracking
    std::unordered_map<NetworkEntityID, AuthorityRecord> entity_authorities_;
    std::unordered_map<ClientID, ClientInfo> connected_clients_;
    std::unordered_map<NetworkEntityID, std::array<f32, 3>> entity_positions_; // For proximity calculations
    
    // Authority assignment callbacks
    std::function<void(NetworkEntityID, ClientID, ClientID)> authority_transfer_callback_;
    std::function<void(NetworkEntityID, ClientID)> authority_conflict_callback_;
    
    // Statistics
    mutable u64 authority_transfers_{0};
    mutable u64 authority_conflicts_{0};
    mutable u64 authority_timeouts_{0};
    mutable u64 authority_assignments_{0};
    
public:
    /** @brief Initialize authority manager */
    explicit AuthorityManager(ClientID client_id, bool is_server = false, 
                             const AuthorityConfig& config = AuthorityConfig{})
        : config_(config), local_client_id_(client_id), is_server_(is_server) {}
    
    //-------------------------------------------------------------------------
    // Authority Assignment and Management
    //-------------------------------------------------------------------------
    
    /** @brief Assign authority for entity */
    void assign_authority(NetworkEntityID entity_id, ClientID client_id, 
                         AuthorityLevel level = AuthorityLevel::Full,
                         AuthorityScope scope = AuthorityScope::All,
                         AuthorityStrategy strategy = AuthorityStrategy::Manual) {
        NetworkTimestamp current_time = timing::now();
        
        auto& record = entity_authorities_[entity_id];
        record.entity_id = entity_id;
        record.transfer_authority(client_id, level, scope, current_time);
        record.assignment_strategy = strategy;
        
        // Update client info
        if (auto client_it = connected_clients_.find(client_id); client_it != connected_clients_.end()) {
            client_it->second.entities_owned++;
        }
        
        authority_assignments_++;
        
        // Notify callback
        if (authority_transfer_callback_) {
            authority_transfer_callback_(entity_id, 0, client_id); // 0 = no previous authority
        }
    }
    
    /** @brief Request authority transfer */
    bool request_authority_transfer(NetworkEntityID entity_id, ClientID requesting_client) {
        auto auth_it = entity_authorities_.find(entity_id);
        if (auth_it == entity_authorities_.end()) {
            // No existing authority - assign to requester
            assign_authority(entity_id, requesting_client);
            return true;
        }
        
        auto& record = auth_it->second;
        NetworkTimestamp current_time = timing::now();
        
        // Add claim
        record.add_authority_claim(requesting_client, current_time);
        
        // Evaluate if transfer should happen
        if (should_transfer_authority(record, requesting_client, current_time)) {
            transfer_authority_internal(record, requesting_client, current_time);
            return true;
        }
        
        return false;
    }
    
    /** @brief Check if client has authority over entity */
    bool has_authority(NetworkEntityID entity_id, ClientID client_id, 
                      AuthorityScope scope = AuthorityScope::All) const {
        auto it = entity_authorities_.find(entity_id);
        if (it == entity_authorities_.end()) {
            return false;
        }
        
        return it->second.has_authority_for_scope(client_id, scope);
    }
    
    /** @brief Get authority holder for entity */
    std::optional<ClientID> get_authority(NetworkEntityID entity_id) const {
        auto it = entity_authorities_.find(entity_id);
        if (it == entity_authorities_.end() || it->second.authority_level == AuthorityLevel::None) {
            return std::nullopt;
        }
        
        return it->second.current_authority;
    }
    
    /** @brief Remove entity from authority tracking */
    void remove_entity(NetworkEntityID entity_id) {
        auto it = entity_authorities_.find(entity_id);
        if (it != entity_authorities_.end()) {
            // Update client entity count
            if (auto client_it = connected_clients_.find(it->second.current_authority); 
                client_it != connected_clients_.end() && client_it->second.entities_owned > 0) {
                client_it->second.entities_owned--;
            }
            
            entity_authorities_.erase(it);
        }
        
        entity_positions_.erase(entity_id);
    }
    
    //-------------------------------------------------------------------------
    // Client Management
    //-------------------------------------------------------------------------
    
    /** @brief Update client information */
    void update_client_info(const ClientInfo& client_info) {
        connected_clients_[client_info.client_id] = client_info;
    }
    
    /** @brief Remove client and transfer its authorities */
    void remove_client(ClientID client_id) {
        // Transfer all authorities held by this client
        for (auto& [entity_id, record] : entity_authorities_) {
            if (record.current_authority == client_id) {
                // Find best alternative client
                auto best_client = find_best_authority_client(entity_id, {client_id});
                if (best_client) {
                    NetworkTimestamp current_time = timing::now();
                    transfer_authority_internal(record, *best_client, current_time);
                }
            }
        }
        
        connected_clients_.erase(client_id);
    }
    
    //-------------------------------------------------------------------------
    // Automatic Authority Management
    //-------------------------------------------------------------------------
    
    /** @brief Update entity position for proximity-based authority */
    void update_entity_position(NetworkEntityID entity_id, const std::array<f32, 3>& position) {
        entity_positions_[entity_id] = position;
        
        // Check if authority should be transferred based on proximity
        auto auth_it = entity_authorities_.find(entity_id);
        if (auth_it != entity_authorities_.end() && 
            auth_it->second.assignment_strategy == AuthorityStrategy::ProximityBased) {
            
            auto best_client = find_closest_client(position);
            if (best_client && *best_client != auth_it->second.current_authority) {
                // Check if transfer is warranted (with hysteresis)
                f32 current_distance = calculate_client_distance(auth_it->second.current_authority, position);
                f32 new_distance = calculate_client_distance(*best_client, position);
                
                if (new_distance + config_.proximity_hysteresis < current_distance) {
                    NetworkTimestamp current_time = timing::now();
                    transfer_authority_internal(auth_it->second, *best_client, current_time);
                }
            }
        }
    }
    
    /** @brief Process periodic authority maintenance */
    void update_authority_system() {
        NetworkTimestamp current_time = timing::now();
        
        // Process timeouts and conflicts
        std::vector<NetworkEntityID> entities_to_reassign;
        
        for (auto& [entity_id, record] : entity_authorities_) {
            // Handle authority timeouts
            if (record.is_authority_active(current_time, config_.authority_timeout_us) == false &&
                record.current_authority != 0) {
                entities_to_reassign.push_back(entity_id);
                authority_timeouts_++;
            }
            
            // Handle authority conflicts
            if (record.has_conflicts) {
                resolve_authority_conflict(record, current_time);
            }
            
            // Clean up old claims
            auto old_claim_cutoff = current_time - config_.claim_timeout_us;
            record.authority_claims.erase(
                std::remove_if(record.authority_claims.begin(), record.authority_claims.end(),
                              [old_claim_cutoff](const auto& claim) {
                                  return claim.second < old_claim_cutoff;
                              }),
                record.authority_claims.end());
            
            record.has_conflicts = record.authority_claims.size() > 1;
        }
        
        // Reassign timed out authorities
        for (auto entity_id : entities_to_reassign) {
            auto& record = entity_authorities_[entity_id];
            auto best_client = find_best_authority_client(entity_id, {record.current_authority});
            if (best_client) {
                transfer_authority_internal(record, *best_client, current_time);
            }
        }
    }
    
    //-------------------------------------------------------------------------
    // Configuration and Callbacks
    //-------------------------------------------------------------------------
    
    /** @brief Set authority transfer callback */
    void set_authority_transfer_callback(std::function<void(NetworkEntityID, ClientID, ClientID)> callback) {
        authority_transfer_callback_ = std::move(callback);
    }
    
    /** @brief Set authority conflict callback */
    void set_authority_conflict_callback(std::function<void(NetworkEntityID, ClientID)> callback) {
        authority_conflict_callback_ = std::move(callback);
    }
    
    /** @brief Update configuration */
    void set_config(const AuthorityConfig& config) {
        config_ = config;
    }
    
    /** @brief Get current configuration */
    const AuthorityConfig& config() const {
        return config_;
    }
    
    //-------------------------------------------------------------------------
    // Statistics and Monitoring
    //-------------------------------------------------------------------------
    
    /** @brief Get authority statistics */
    struct Statistics {
        usize total_entities_tracked;
        usize entities_with_authority;
        usize entities_with_conflicts;
        u64 total_authority_transfers;
        u64 total_authority_conflicts;
        u64 total_authority_timeouts;
        u64 total_authority_assignments;
        
        // Per-client statistics
        std::unordered_map<ClientID, u32> entities_per_client;
        f32 authority_distribution_balance; // 0 = perfectly balanced, 1 = very unbalanced
    };
    
    Statistics get_statistics() const {
        Statistics stats;
        stats.total_entities_tracked = entity_authorities_.size();
        stats.total_authority_transfers = authority_transfers_;
        stats.total_authority_conflicts = authority_conflicts_;
        stats.total_authority_timeouts = authority_timeouts_;
        stats.total_authority_assignments = authority_assignments_;
        
        usize entities_with_authority = 0;
        usize entities_with_conflicts = 0;
        
        for (const auto& [entity_id, record] : entity_authorities_) {
            if (record.authority_level > AuthorityLevel::None) {
                entities_with_authority++;
                stats.entities_per_client[record.current_authority]++;
            }
            if (record.has_conflicts) {
                entities_with_conflicts++;
            }
        }
        
        stats.entities_with_authority = entities_with_authority;
        stats.entities_with_conflicts = entities_with_conflicts;
        
        // Calculate balance (standard deviation of entity distribution)
        if (connected_clients_.size() > 1) {
            f32 mean_entities = static_cast<f32>(entities_with_authority) / connected_clients_.size();
            f32 variance = 0.0f;
            
            for (const auto& [client_id, client_info] : connected_clients_) {
                f32 diff = static_cast<f32>(client_info.entities_owned) - mean_entities;
                variance += diff * diff;
            }
            
            variance /= connected_clients_.size();
            stats.authority_distribution_balance = std::sqrt(variance) / (mean_entities + 1.0f);
        } else {
            stats.authority_distribution_balance = 0.0f;
        }
        
        return stats;
    }
    
    /** @brief Get entities with authority held by specific client */
    std::vector<NetworkEntityID> get_client_entities(ClientID client_id) const {
        std::vector<NetworkEntityID> entities;
        
        for (const auto& [entity_id, record] : entity_authorities_) {
            if (record.has_authority(client_id)) {
                entities.push_back(entity_id);
            }
        }
        
        return entities;
    }

private:
    //-------------------------------------------------------------------------
    // Internal Authority Management
    //-------------------------------------------------------------------------
    
    /** @brief Internal authority transfer implementation */
    void transfer_authority_internal(AuthorityRecord& record, ClientID new_client, NetworkTimestamp current_time) {
        ClientID old_client = record.current_authority;
        
        // Update client entity counts
        if (auto old_client_it = connected_clients_.find(old_client); 
            old_client_it != connected_clients_.end() && old_client_it->second.entities_owned > 0) {
            old_client_it->second.entities_owned--;
        }
        
        if (auto new_client_it = connected_clients_.find(new_client); 
            new_client_it != connected_clients_.end()) {
            new_client_it->second.entities_owned++;
        }
        
        // Transfer authority
        record.transfer_authority(new_client, record.authority_level, record.authority_scope, current_time);
        
        authority_transfers_++;
        
        // Notify callback
        if (authority_transfer_callback_) {
            authority_transfer_callback_(record.entity_id, old_client, new_client);
        }
    }
    
    /** @brief Check if authority should be transferred */
    bool should_transfer_authority(const AuthorityRecord& record, ClientID requesting_client, NetworkTimestamp current_time) const {
        // Server always has ultimate authority
        if (is_server_ && local_client_id_ == requesting_client) {
            return true;
        }
        
        // Don't transfer from server unless specifically configured
        if (record.current_authority == 0 || // 0 = server
            (connected_clients_.count(record.current_authority) > 0 && 
             connected_clients_.at(record.current_authority).is_server)) {
            return config_.server_override_conflicts && is_server_;
        }
        
        // Check if current authority holder is inactive
        if (!record.is_authority_active(current_time, config_.authority_timeout_us)) {
            return true;
        }
        
        // Check network performance
        auto current_client_it = connected_clients_.find(record.current_authority);
        auto requesting_client_it = connected_clients_.find(requesting_client);
        
        if (current_client_it != connected_clients_.end() && 
            requesting_client_it != connected_clients_.end()) {
            
            f32 current_performance = current_client_it->second.network_performance();
            f32 requesting_performance = requesting_client_it->second.network_performance();
            
            // Transfer if requesting client has significantly better performance
            if (requesting_performance > current_performance + 0.2f) { // 20% better
                return true;
            }
        }
        
        return false;
    }
    
    /** @brief Resolve authority conflicts */
    void resolve_authority_conflict(AuthorityRecord& record, NetworkTimestamp current_time) {
        if (record.authority_claims.empty()) {
            record.has_conflicts = false;
            return;
        }
        
        authority_conflicts_++;
        
        // Server override
        if (config_.server_override_conflicts && is_server_) {
            // Check if server has a claim
            for (const auto& [client_id, timestamp] : record.authority_claims) {
                if (client_id == local_client_id_) {
                    transfer_authority_internal(record, local_client_id_, current_time);
                    return;
                }
            }
        }
        
        // Find best client among claimants
        ClientID best_client = 0;
        f32 best_score = -1.0f;
        
        for (const auto& [client_id, timestamp] : record.authority_claims) {
            auto client_it = connected_clients_.find(client_id);
            if (client_it == connected_clients_.end()) continue;
            
            f32 score = calculate_authority_score(client_it->second, record.entity_id);
            if (score > best_score) {
                best_score = score;
                best_client = client_id;
            }
        }
        
        if (best_client != 0) {
            transfer_authority_internal(record, best_client, current_time);
        }
        
        // Notify conflict callback
        if (authority_conflict_callback_) {
            authority_conflict_callback_(record.entity_id, best_client);
        }
    }
    
    /** @brief Calculate authority assignment score for client */
    f32 calculate_authority_score(const ClientInfo& client, NetworkEntityID entity_id) const {
        f32 score = 0.0f;
        
        // Network performance (40% of score)
        score += client.network_performance() * 0.4f;
        
        // Load balance (30% of score)
        score += client.load_score() * 0.3f;
        
        // Proximity (30% of score)
        auto pos_it = entity_positions_.find(entity_id);
        if (pos_it != entity_positions_.end()) {
            f32 distance = calculate_client_distance(client.client_id, pos_it->second);
            f32 proximity_score = std::max(0.0f, 1.0f - (distance / config_.proximity_radius));
            score += proximity_score * 0.3f;
        }
        
        return score;
    }
    
    /** @brief Find best client for authority assignment */
    std::optional<ClientID> find_best_authority_client(NetworkEntityID entity_id, 
                                                      const std::unordered_set<ClientID>& exclude = {}) const {
        ClientID best_client = 0;
        f32 best_score = -1.0f;
        
        for (const auto& [client_id, client_info] : connected_clients_) {
            if (exclude.count(client_id) > 0) continue;
            
            // Check entity limit
            if (client_info.entities_owned >= config_.max_entities_per_client) continue;
            
            f32 score = calculate_authority_score(client_info, entity_id);
            if (score > best_score) {
                best_score = score;
                best_client = client_id;
            }
        }
        
        return best_client != 0 ? std::optional<ClientID>(best_client) : std::nullopt;
    }
    
    /** @brief Find closest client to position */
    std::optional<ClientID> find_closest_client(const std::array<f32, 3>& position) const {
        ClientID closest_client = 0;
        f32 closest_distance = std::numeric_limits<f32>::max();
        
        for (const auto& [client_id, client_info] : connected_clients_) {
            f32 distance = calculate_client_distance(client_id, position);
            if (distance < closest_distance) {
                closest_distance = distance;
                closest_client = client_id;
            }
        }
        
        return closest_client != 0 ? std::optional<ClientID>(closest_client) : std::nullopt;
    }
    
    /** @brief Calculate distance between client and position */
    f32 calculate_client_distance(ClientID client_id, const std::array<f32, 3>& position) const {
        auto client_it = connected_clients_.find(client_id);
        if (client_it == connected_clients_.end()) {
            return std::numeric_limits<f32>::max();
        }
        
        const auto& client_pos = client_it->second.position;
        f32 dx = position[0] - client_pos[0];
        f32 dy = position[1] - client_pos[1];
        f32 dz = position[2] - client_pos[2];
        
        return std::sqrt(dx*dx + dy*dy + dz*dz);
    }
};

} // namespace ecscope::networking::authority