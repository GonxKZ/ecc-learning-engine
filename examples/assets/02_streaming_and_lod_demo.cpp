/**
 * ECScope Asset System Demo - Streaming and LOD
 * 
 * This demo demonstrates advanced asset streaming capabilities including:
 * - Asset streaming with distance-based LOD
 * - Quality level management
 * - Predictive asset streaming
 * - Memory budget management
 * - Real-time quality adjustment
 */

#include "ecscope/assets/assets.hpp"
#include "ecscope/assets/concrete_assets.hpp"
#include "ecscope/assets/asset_streaming.hpp"
#include "ecscope/core/application.hpp"
#include <iostream>
#include <random>

using namespace ecscope;
using namespace ecscope::assets;

class StreamingLODDemo : public core::Application {
public:
    StreamingLODDemo() : core::Application("Streaming and LOD Demo") {}

private:
    // Simulation state
    std::array<float, 3> m_camera_position = {0, 0, 0};
    std::array<float, 3> m_camera_velocity = {1, 0, 0};
    std::vector<AssetHandle> m_world_assets;
    
    // Asset streaming system
    std::unique_ptr<AssetStreamingSystem> m_streaming_system;
    std::unique_ptr<PredictiveStreamingSystem> m_predictive_streaming;
    
    bool initialize() override {
        std::cout << "=== ECScope Asset System - Streaming and LOD Demo ===\n\n";

        // Initialize asset system with streaming configuration
        AssetManagerConfig config;
        config.max_memory_mb = 512;
        config.worker_threads = 6;
        config.enable_streaming = true;
        config.enable_hot_reload = false; // Disable for performance
        config.asset_root = "assets/";

        if (!initialize_asset_system(config)) {
            std::cerr << "Failed to initialize asset system!\n";
            return false;
        }

        // Initialize streaming system
        m_streaming_system = std::make_unique<AssetStreamingSystem>(&get_asset_manager());
        if (!m_streaming_system->initialize()) {
            std::cerr << "Failed to initialize streaming system!\n";
            return false;
        }

        // Configure LOD settings
        LODConfiguration lod_config;
        lod_config.levels.clear();
        lod_config.levels.emplace_back(QualityLevel::LOW, 200.0f, 0.05f, "_low");
        lod_config.levels.emplace_back(QualityLevel::MEDIUM, 100.0f, 0.15f, "_med");
        lod_config.levels.emplace_back(QualityLevel::HIGH, 50.0f, 0.4f, "_high");
        lod_config.levels.emplace_back(QualityLevel::ULTRA, 25.0f, 1.0f, "_ultra");
        lod_config.hysteresis_factor = 0.15f;
        m_streaming_system->set_lod_configuration(lod_config);

        // Configure streaming budget
        auto& budget = m_streaming_system->get_budget_manager();
        budget.set_memory_budget_mb(256);
        budget.set_bandwidth_budget_mbps(50.0f);
        budget.set_time_budget_ms(3.0f);

        // Initialize predictive streaming
        m_predictive_streaming = std::make_unique<PredictiveStreamingSystem>(m_streaming_system.get());
        PredictiveStreamingSystem::PredictionConfig pred_config;
        pred_config.prediction_time_horizon = 3.0f;
        pred_config.confidence_threshold = 0.6f;
        pred_config.enable_movement_prediction = true;
        pred_config.enable_pattern_learning = true;
        m_predictive_streaming->set_prediction_config(pred_config);

        std::cout << "Streaming system initialized successfully\n";
        std::cout << "LOD Levels: " << lod_config.levels.size() << "\n";
        std::cout << "Memory Budget: " << budget.get_memory_budget() / 1024 / 1024 << " MB\n";
        std::cout << "Bandwidth Budget: " << budget.get_bandwidth_budget() / 1024 / 1024 << " MB/s\n\n";

        return true;
    }

    void run() override {
        setup_world_assets();
        demonstrate_streaming_basics();
        simulate_camera_movement();
        demonstrate_predictive_streaming();
        demonstrate_quality_management();
        demonstrate_memory_management();
        show_streaming_statistics();
    }

    void shutdown() override {
        m_predictive_streaming.reset();
        m_streaming_system->shutdown();
        m_streaming_system.reset();
        shutdown_asset_system();
        std::cout << "\nStreaming system shut down successfully\n";
    }

private:
    void setup_world_assets() {
        std::cout << "=== Setting Up World Assets ===\n";

        // Create a virtual world with assets at various distances
        std::vector<std::pair<std::string, std::array<float, 3>>> world_objects = {
            {"models/building_01.fbx", {10, 0, 0}},
            {"models/building_02.fbx", {30, 0, 0}},
            {"models/building_03.fbx", {60, 0, 0}},
            {"models/tree_01.fbx", {15, 0, 5}},
            {"models/tree_02.fbx", {45, 0, -8}},
            {"models/car_01.fbx", {25, 0, 3}},
            {"models/car_02.fbx", {75, 0, -5}},
            {"models/streetlight.fbx", {20, 0, 0}},
            {"models/streetlight.fbx", {40, 0, 0}},
            {"models/streetlight.fbx", {80, 0, 0}},
        };

        for (const auto& [asset_path, position] : world_objects) {
            // Request asset with initial streaming configuration
            float distance = calculate_distance(position);
            QualityLevel initial_quality = select_quality_for_distance(distance);
            int priority = calculate_priority(distance);
            
            m_streaming_system->request_asset(
                path_to_asset_id(asset_path),
                initial_quality,
                priority,
                distance,
                [this, asset_path](AssetHandle handle, bool success) {
                    if (success) {
                        m_world_assets.push_back(handle);
                        std::cout << "✓ Streamed: " << asset_path << "\n";
                    } else {
                        std::cout << "✗ Failed to stream: " << asset_path << "\n";
                    }
                }
            );
        }

        std::cout << "Requested streaming for " << world_objects.size() << " world assets\n\n";
    }

    void demonstrate_streaming_basics() {
        std::cout << "=== Basic Streaming Operations ===\n";

        // Show initial streaming state
        std::cout << "Initial streaming requests: " 
                  << m_streaming_system->get_active_requests().size() << "\n";

        // Wait for some assets to load
        for (int i = 0; i < 20; ++i) {
            m_streaming_system->update(0.1f);
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            
            if (i % 5 == 0) {
                auto& stats = m_streaming_system->get_statistics();
                std::cout << "Frame " << i << ": Processed " 
                          << stats.requests_processed.load() << " requests, "
                          << (stats.bytes_streamed.load() / 1024) << " KB streamed\n";
            }
        }

        std::cout << "Assets loaded: " << m_world_assets.size() << "\n";
        
        // Show quality levels
        for (const auto& handle : m_world_assets) {
            if (handle) {
                AssetID id = handle.get_id();
                QualityLevel quality = m_streaming_system->get_current_quality(id);
                float progress = m_streaming_system->get_streaming_progress(id);
                std::cout << "  Asset " << id << ": Quality " 
                          << static_cast<int>(quality) << ", Progress " 
                          << (progress * 100) << "%\n";
            }
        }

        std::cout << "\n";
    }

    void simulate_camera_movement() {
        std::cout << "=== Simulating Camera Movement ===\n";
        
        const float simulation_time = 10.0f; // 10 seconds
        const float dt = 0.1f;
        
        for (float time = 0; time < simulation_time; time += dt) {
            // Update camera position (simple forward movement with some variation)
            m_camera_position[0] += m_camera_velocity[0] * dt;
            m_camera_position[1] += 0.5f * sin(time) * dt; // Some vertical movement
            m_camera_position[2] += 0.3f * cos(time * 0.7f) * dt; // Some side movement
            
            // Update predictive streaming with camera movement
            m_predictive_streaming->update_camera_position(
                m_camera_position[0], m_camera_position[1], m_camera_position[2]);
            m_predictive_streaming->update_camera_velocity(
                m_camera_velocity[0], m_camera_velocity[1], m_camera_velocity[2]);
            
            // Update streaming system
            m_streaming_system->update(dt);
            m_predictive_streaming->update_predictions(dt);
            
            // Update asset distances and qualities
            update_asset_streaming();
            
            // Show progress periodically
            if (static_cast<int>(time / dt) % 20 == 0) {
                std::cout << "Time: " << time << "s, Camera: ("
                          << m_camera_position[0] << ", " 
                          << m_camera_position[1] << ", "
                          << m_camera_position[2] << ")\n";
                          
                show_current_qualities();
            }
            
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
        
        std::cout << "\n";
    }

    void demonstrate_predictive_streaming() {
        std::cout << "=== Predictive Streaming ===\n";
        
        // Simulate player interactions
        simulate_area_transitions();
        simulate_asset_access_patterns();
        
        // Get predictions
        auto predicted_assets = m_predictive_streaming->get_predicted_assets();
        std::cout << "Predicted assets for preloading: " << predicted_assets.size() << "\n";
        
        for (AssetID asset_id : predicted_assets) {
            std::cout << "  Predicting need for asset " << asset_id << "\n";
            
            // Preload predicted assets at low quality
            m_streaming_system->request_asset(
                asset_id, 
                QualityLevel::LOW,
                StreamingPriority::PRELOAD,
                0.0f // Unknown distance, use prediction
            );
        }
        
        std::cout << "\n";
    }

    void demonstrate_quality_management() {
        std::cout << "=== Quality Management ===\n";
        
        // Test quality level changes
        std::cout << "Testing global quality changes...\n";
        
        QualityLevel levels[] = {
            QualityLevel::LOW, 
            QualityLevel::MEDIUM, 
            QualityLevel::HIGH, 
            QualityLevel::ULTRA
        };
        
        for (QualityLevel level : levels) {
            std::cout << "Setting global quality to " << static_cast<int>(level) << "\n";
            m_streaming_system->set_global_quality_level(level);
            
            // Process for a bit to see changes
            for (int i = 0; i < 10; ++i) {
                m_streaming_system->update(0.1f);
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
            
            show_quality_distribution();
        }
        
        // Reset to automatic quality selection
        m_streaming_system->set_global_quality_level(QualityLevel::MEDIUM);
        std::cout << "Reset to automatic quality selection\n\n";
    }

    void demonstrate_memory_management() {
        std::cout << "=== Memory Management ===\n";
        
        auto& budget = m_streaming_system->get_budget_manager();
        
        std::cout << "Current budget usage:\n";
        std::cout << "  Memory: " << (budget.get_memory_used() / 1024 / 1024) 
                  << "/" << (budget.get_memory_budget() / 1024 / 1024) << " MB ("
                  << (budget.get_memory_utilization() * 100) << "%)\n";
        std::cout << "  Bandwidth: " << (budget.get_bandwidth_used() / 1024 / 1024)
                  << "/" << (budget.get_bandwidth_budget() / 1024 / 1024) << " MB/s ("
                  << (budget.get_bandwidth_utilization() * 100) << "%)\n";
        std::cout << "  Time: " << budget.get_time_used()
                  << "/" << budget.get_time_budget() << " ms ("
                  << (budget.get_time_utilization() * 100) << "%)\n";
        
        // Test budget limits by requesting many assets
        std::cout << "\nTesting budget limits...\n";
        for (int i = 0; i < 20; ++i) {
            AssetID fake_id = generate_asset_id();
            m_streaming_system->request_asset(
                fake_id,
                QualityLevel::HIGH,
                StreamingPriority::BACKGROUND,
                100.0f + i * 10
            );
        }
        
        // Process and show how budget affects loading
        for (int frame = 0; frame < 30; ++frame) {
            budget.reset_frame_budget();
            m_streaming_system->update(0.033f); // ~30 FPS
            
            if (frame % 10 == 0) {
                std::cout << "Frame " << frame << " budget usage: "
                          << (budget.get_time_utilization() * 100) << "% time, "
                          << (budget.get_memory_utilization() * 100) << "% memory\n";
            }
        }
        
        std::cout << "\n";
    }

    void show_streaming_statistics() {
        std::cout << "=== Final Streaming Statistics ===\n";
        
        const auto& stats = m_streaming_system->get_statistics();
        
        std::cout << "Streaming Performance:\n";
        std::cout << "  Total requests processed: " << stats.requests_processed.load() << "\n";
        std::cout << "  Total data streamed: " << (stats.bytes_streamed.load() / 1024 / 1024) << " MB\n";
        std::cout << "  Cache hit rate: " 
                  << (stats.cache_hits.load() * 100.0f / 
                     (stats.cache_hits.load() + stats.cache_misses.load())) << "%\n";
        std::cout << "  Quality upgrades: " << stats.quality_upgrades.load() << "\n";
        std::cout << "  Quality downgrades: " << stats.quality_downgrades.load() << "\n";
        std::cout << "  Average streaming time: " << stats.average_streaming_time_ms.load() << " ms\n";
        
        std::cout << "\nSystem Statistics:\n";
        auto system_stats = get_asset_system_statistics();
        std::cout << "  Total assets in memory: " << system_stats.total_assets_loaded << "\n";
        std::cout << "  Memory usage: " << (system_stats.total_memory_used / 1024 / 1024) << " MB\n";
        
        auto& budget = m_streaming_system->get_budget_manager();
        std::cout << "  Final memory utilization: " << (budget.get_memory_utilization() * 100) << "%\n";
        std::cout << "  Final bandwidth utilization: " << (budget.get_bandwidth_utilization() * 100) << "%\n";
    }

    // Helper methods
    float calculate_distance(const std::array<float, 3>& position) const {
        float dx = position[0] - m_camera_position[0];
        float dy = position[1] - m_camera_position[1];
        float dz = position[2] - m_camera_position[2];
        return sqrt(dx*dx + dy*dy + dz*dz);
    }

    QualityLevel select_quality_for_distance(float distance) const {
        const auto& lod_config = m_streaming_system->get_lod_configuration();
        return lod_config.select_quality_for_distance(distance);
    }

    int calculate_priority(float distance) const {
        if (distance < 25.0f) return StreamingPriority::VISIBLE;
        if (distance < 50.0f) return StreamingPriority::NEARBY;
        if (distance < 100.0f) return StreamingPriority::BACKGROUND;
        return StreamingPriority::DISTANT;
    }

    void update_asset_streaming() {
        // Update distances and request quality changes for existing assets
        for (const auto& handle : m_world_assets) {
            if (handle) {
                AssetID id = handle.get_id();
                // Note: In real implementation, would get actual asset position
                float distance = 50.0f; // Placeholder
                m_streaming_system->update_distance(id, distance);
            }
        }
    }

    void show_current_qualities() {
        std::unordered_map<QualityLevel, int> quality_counts;
        
        for (const auto& handle : m_world_assets) {
            if (handle) {
                QualityLevel quality = m_streaming_system->get_current_quality(handle.get_id());
                quality_counts[quality]++;
            }
        }
        
        std::cout << "  Quality distribution: ";
        for (const auto& [quality, count] : quality_counts) {
            std::cout << "L" << static_cast<int>(quality) << ":" << count << " ";
        }
        std::cout << "\n";
    }

    void show_quality_distribution() {
        show_current_qualities();
    }

    void simulate_area_transitions() {
        // Simulate moving between different game areas
        std::vector<std::string> areas = {"forest", "city", "desert", "underground"};
        
        for (const auto& area : areas) {
            std::cout << "Entering area: " << area << "\n";
            
            // Create fake asset list for area
            std::vector<AssetID> area_assets;
            for (int i = 0; i < 5; ++i) {
                area_assets.push_back(generate_asset_id());
            }
            
            m_predictive_streaming->on_area_entered(area, area_assets);
            
            // Simulate some time in the area
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
        }
    }

    void simulate_asset_access_patterns() {
        // Simulate accessing specific assets repeatedly
        for (int i = 0; i < 10; ++i) {
            for (const auto& handle : m_world_assets) {
                if (handle && i % 3 == 0) { // Access every 3rd iteration
                    m_predictive_streaming->on_asset_accessed(handle.get_id());
                }
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }
};

int main() {
    try {
        StreamingLODDemo demo;
        return demo.execute();
    } catch (const std::exception& e) {
        std::cerr << "Demo failed with exception: " << e.what() << "\n";
        return -1;
    }
}