#include <iostream>
#include <memory>
#include <thread>
#include <chrono>
#include <random>
#include <vector>
#include <string>
#include <filesystem>

#ifdef ECSCOPE_HAS_GUI
#include "ecscope/gui/dashboard.hpp"
#include "ecscope/gui/asset_pipeline_ui.hpp"
#include "ecscope/gui/gui_manager.hpp"
#endif

class MockAssetSystem {
public:
    MockAssetSystem() : next_asset_id_(1) {
        rng_.seed(std::chrono::steady_clock::now().time_since_epoch().count());
    }

    void initialize(const std::string& project_root) {
#ifdef ECSCOPE_HAS_GUI
        project_root_ = project_root;
        
        // Create project directory structure if it doesn't exist
        create_project_structure();
        
        // Initialize asset pipeline manager
        ecscope::gui::AssetPipelineManager::instance().initialize(project_root);
        
        // Create some mock assets
        create_mock_assets();
#endif
    }

    void update(float delta_time) {
        update_time_ += delta_time;
        
        if (update_time_ >= 2.0f) { // Update every 2 seconds
            simulate_asset_changes();
            update_time_ = 0.0f;
        }
    }

private:
    void create_project_structure() {
        try {
            std::filesystem::create_directories(project_root_ + "/textures");
            std::filesystem::create_directories(project_root_ + "/models");
            std::filesystem::create_directories(project_root_ + "/audio");
            std::filesystem::create_directories(project_root_ + "/scripts");
            std::filesystem::create_directories(project_root_ + "/shaders");
            std::filesystem::create_directories(project_root_ + "/materials");
            std::filesystem::create_directories(project_root_ + "/scenes");
        } catch (const std::exception& e) {
            std::cerr << "Failed to create project structure: " << e.what() << std::endl;
        }
    }

    void create_mock_assets() {
#ifdef ECSCOPE_HAS_GUI
        // Create mock texture assets
        create_mock_asset("player_texture.png", ecscope::gui::AssetType::Texture, "textures/", 2048576);
        create_mock_asset("enemy_sprite.png", ecscope::gui::AssetType::Texture, "textures/", 1024768);
        create_mock_asset("background.jpg", ecscope::gui::AssetType::Texture, "textures/", 4194304);
        create_mock_asset("ui_elements.png", ecscope::gui::AssetType::Texture, "textures/", 512384);
        
        // Create mock model assets
        create_mock_asset("player_character.fbx", ecscope::gui::AssetType::Model, "models/", 8388608);
        create_mock_asset("environment_prop.obj", ecscope::gui::AssetType::Model, "models/", 2097152);
        create_mock_asset("vehicle.gltf", ecscope::gui::AssetType::Model, "models/", 16777216);
        
        // Create mock audio assets
        create_mock_asset("background_music.ogg", ecscope::gui::AssetType::Audio, "audio/", 10485760);
        create_mock_asset("footsteps.wav", ecscope::gui::AssetType::Audio, "audio/", 524288);
        create_mock_asset("explosion.mp3", ecscope::gui::AssetType::Audio, "audio/", 1048576);
        create_mock_asset("ambient_forest.ogg", ecscope::gui::AssetType::Audio, "audio/", 5242880);
        
        // Create mock script assets
        create_mock_asset("player_controller.cpp", ecscope::gui::AssetType::Script, "scripts/", 8192);
        create_mock_asset("game_logic.lua", ecscope::gui::AssetType::Script, "scripts/", 4096);
        create_mock_asset("ai_behavior.py", ecscope::gui::AssetType::Script, "scripts/", 6144);
        
        // Create mock shader assets
        create_mock_asset("basic_vertex.glsl", ecscope::gui::AssetType::Shader, "shaders/", 2048);
        create_mock_asset("pbr_fragment.glsl", ecscope::gui::AssetType::Shader, "shaders/", 4096);
        create_mock_asset("post_process.hlsl", ecscope::gui::AssetType::Shader, "shaders/", 3072);
        
        // Create mock material assets
        create_mock_asset("metal_material.mat", ecscope::gui::AssetType::Material, "materials/", 1024);
        create_mock_asset("wood_surface.material", ecscope::gui::AssetType::Material, "materials/", 1536);
        
        // Create mock scene assets
        create_mock_asset("main_menu.scene", ecscope::gui::AssetType::Scene, "scenes/", 16384);
        create_mock_asset("level_01.scene", ecscope::gui::AssetType::Scene, "scenes/", 32768);
        create_mock_asset("boss_arena.scene", ecscope::gui::AssetType::Scene, "scenes/", 24576);
        
        std::cout << "Created " << mock_assets_.size() << " mock assets" << std::endl;
#endif
    }

    void create_mock_asset(const std::string& name, ecscope::gui::AssetType type, 
                          const std::string& subfolder, size_t file_size) {
#ifdef ECSCOPE_HAS_GUI
        ecscope::gui::AssetMetadata metadata;
        metadata.id = "asset_" + std::to_string(next_asset_id_++);
        metadata.name = name;
        metadata.path = project_root_ + "/" + subfolder + name;
        metadata.source_path = metadata.path;
        metadata.type = type;
        metadata.status = ecscope::gui::AssetStatus::Loaded;
        metadata.file_size = file_size;
        metadata.created_time = std::chrono::system_clock::now() - 
                               std::chrono::hours(static_cast<int>(uniform_dist_(rng_) * 720)); // Random creation time within 30 days
        metadata.modified_time = metadata.created_time + 
                                std::chrono::hours(static_cast<int>(uniform_dist_(rng_) * 24)); // Random modification time
        metadata.last_accessed = std::chrono::system_clock::now() - 
                                std::chrono::minutes(static_cast<int>(uniform_dist_(rng_) * 1440)); // Random access time within 24 hours
        
        // Add some type-specific properties
        switch (type) {
            case ecscope::gui::AssetType::Texture:
                metadata.properties["width"] = std::to_string(static_cast<int>(256 + uniform_dist_(rng_) * 1792)); // 256-2048
                metadata.properties["height"] = std::to_string(static_cast<int>(256 + uniform_dist_(rng_) * 1792));
                metadata.properties["format"] = (uniform_dist_(rng_) < 0.5f) ? "RGBA8" : "RGB8";
                metadata.properties["compression"] = (uniform_dist_(rng_) < 0.3f) ? "DXT5" : "None";
                break;
                
            case ecscope::gui::AssetType::Model:
                metadata.properties["vertices"] = std::to_string(static_cast<int>(1000 + uniform_dist_(rng_) * 49000)); // 1K-50K vertices
                metadata.properties["triangles"] = std::to_string(static_cast<int>(500 + uniform_dist_(rng_) * 24500)); // 0.5K-25K triangles
                metadata.properties["materials"] = std::to_string(static_cast<int>(1 + uniform_dist_(rng_) * 9)); // 1-10 materials
                metadata.properties["has_animations"] = (uniform_dist_(rng_) < 0.4f) ? "true" : "false";
                break;
                
            case ecscope::gui::AssetType::Audio:
                metadata.properties["duration"] = std::to_string(uniform_dist_(rng_) * 300.0f); // 0-300 seconds
                metadata.properties["sample_rate"] = (uniform_dist_(rng_) < 0.7f) ? "44100" : "48000";
                metadata.properties["channels"] = (uniform_dist_(rng_) < 0.8f) ? "2" : "1";
                metadata.properties["bitrate"] = std::to_string(static_cast<int>(128 + uniform_dist_(rng_) * 192)); // 128-320 kbps
                break;
                
            case ecscope::gui::AssetType::Script:
                metadata.properties["lines"] = std::to_string(static_cast<int>(50 + uniform_dist_(rng_) * 950)); // 50-1000 lines
                metadata.properties["language"] = name.substr(name.find_last_of('.') + 1);
                break;
                
            default:
                break;
        }
        
        // Add some random dependencies
        if (uniform_dist_(rng_) < 0.3f) {
            metadata.dependencies.push_back("dependency_" + std::to_string(static_cast<int>(uniform_dist_(rng_) * 10)));
        }
        
        metadata.has_preview = (type == ecscope::gui::AssetType::Texture || 
                               type == ecscope::gui::AssetType::Model) && uniform_dist_(rng_) < 0.7f;
        
        if (metadata.has_preview) {
            metadata.preview_path = metadata.path + ".preview.png";
            metadata.preview_texture_id = next_asset_id_; // Mock texture ID
        }
        
        mock_assets_[metadata.id] = metadata;
        
        // Notify the asset pipeline manager
        ecscope::gui::AssetPipelineManager::instance().notify_asset_changed(metadata);
#endif
    }

    void simulate_asset_changes() {
#ifdef ECSCOPE_HAS_GUI
        if (mock_assets_.empty()) return;
        
        // Randomly modify some assets
        if (uniform_dist_(rng_) < 0.1f) { // 10% chance each update
            // Pick a random asset
            auto it = mock_assets_.begin();
            std::advance(it, static_cast<size_t>(uniform_dist_(rng_) * mock_assets_.size()));
            
            // Simulate a modification
            it->second.modified_time = std::chrono::system_clock::now();
            it->second.status = ecscope::gui::AssetStatus::Modified;
            
            std::cout << "Simulated modification of asset: " << it->second.name << std::endl;
            
            // Notify the asset pipeline manager
            ecscope::gui::AssetPipelineManager::instance().notify_asset_changed(it->second);
        }
        
        // Occasionally simulate new asset import
        if (uniform_dist_(rng_) < 0.05f) { // 5% chance each update
            std::vector<std::string> new_asset_names = {
                "imported_texture.png", "new_model.fbx", "sound_effect.wav",
                "utility_script.cpp", "custom_shader.glsl", "imported_scene.scene"
            };
            
            std::vector<ecscope::gui::AssetType> types = {
                ecscope::gui::AssetType::Texture, ecscope::gui::AssetType::Model, 
                ecscope::gui::AssetType::Audio, ecscope::gui::AssetType::Script,
                ecscope::gui::AssetType::Shader, ecscope::gui::AssetType::Scene
            };
            
            size_t idx = static_cast<size_t>(uniform_dist_(rng_) * new_asset_names.size());
            std::string name = "imported_" + std::to_string(next_asset_id_) + "_" + new_asset_names[idx];
            
            create_mock_asset(name, types[idx], "imported/", 
                            static_cast<size_t>(1024 + uniform_dist_(rng_) * 1048576));
            
            std::cout << "Simulated import of new asset: " << name << std::endl;
        }
#endif
    }

    std::string project_root_;
    std::unordered_map<std::string, ecscope::gui::AssetMetadata> mock_assets_;
    uint32_t next_asset_id_;
    float update_time_ = 0.0f;
    std::mt19937 rng_;
    std::uniform_real_distribution<float> uniform_dist_{0.0f, 1.0f};
};

int main() {
    std::cout << "═══════════════════════════════════════════════════════\n";
    std::cout << "  ECScope Asset Pipeline Demo\n";
    std::cout << "═══════════════════════════════════════════════════════\n\n";

#ifdef ECSCOPE_HAS_GUI
    try {
        // Set up project directory
        std::string project_root = "./demo_project";
        
        // Initialize GUI manager
        auto gui_manager = std::make_unique<ecscope::gui::GuiManager>();
        if (!gui_manager->initialize("ECScope Asset Pipeline Demo", 1600, 1000)) {
            std::cerr << "Failed to initialize GUI manager\n";
            return 1;
        }

        // Initialize dashboard
        auto dashboard = std::make_unique<ecscope::gui::Dashboard>();
        if (!dashboard->initialize()) {
            std::cerr << "Failed to initialize dashboard\n";
            return 1;
        }

        // Initialize asset pipeline UI
        auto asset_pipeline_ui = std::make_unique<ecscope::gui::AssetPipelineUI>();
        if (!asset_pipeline_ui->initialize(project_root)) {
            std::cerr << "Failed to initialize asset pipeline UI\n";
            return 1;
        }

        // Initialize mock asset system
        auto asset_system = std::make_unique<MockAssetSystem>();
        asset_system->initialize(project_root);

        // Set up asset pipeline callbacks
        asset_pipeline_ui->set_asset_loaded_callback([](const std::string& asset_id) {
            std::cout << "Asset loaded: " << asset_id << std::endl;
        });

        asset_pipeline_ui->set_asset_modified_callback([](const std::string& asset_id) {
            std::cout << "Asset modified: " << asset_id << std::endl;
        });

        asset_pipeline_ui->set_import_completed_callback([](const std::string& asset_id, bool success) {
            std::cout << "Asset import " << (success ? "completed" : "failed") << ": " << asset_id << std::endl;
        });

        std::cout << "Asset Pipeline Demo Features:\n";
        std::cout << "• Asset Browser: Navigate and manage project assets\n";
        std::cout << "• Asset Inspector: View detailed asset properties and metadata\n";
        std::cout << "• Import Queue: Monitor asset import progress\n";
        std::cout << "• Collections: Organize assets into logical groups\n";
        std::cout << "• Search: Find assets quickly with text search\n";
        std::cout << "• Drag & Drop: Import assets by dragging files\n";
        std::cout << "• Preview System: Generate and view asset previews\n";
        std::cout << "• Real-time Updates: Watch for file system changes\n";
        std::cout << "• Close window to exit\n\n";

        // Main loop
        auto last_time = std::chrono::high_resolution_clock::now();
        
        while (!gui_manager->should_close()) {
            auto current_time = std::chrono::high_resolution_clock::now();
            float delta_time = std::chrono::duration<float>(current_time - last_time).count();
            last_time = current_time;

            // Update systems
            asset_system->update(delta_time);
            asset_pipeline_ui->update(delta_time);

            // Render frame
            gui_manager->begin_frame();
            
            // Render dashboard with asset pipeline UI as a feature
            dashboard->add_feature("Asset Pipeline", "Comprehensive asset management and import system", [&]() {
                asset_pipeline_ui->render();
            }, true);
            
            dashboard->render();

            gui_manager->end_frame();
            
            // Small delay to prevent excessive CPU usage
            std::this_thread::sleep_for(std::chrono::milliseconds(16)); // ~60 FPS
        }

        // Cleanup
        asset_pipeline_ui->shutdown();
        dashboard->shutdown();
        gui_manager->shutdown();

        std::cout << "Asset Pipeline Demo completed successfully!\n";
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