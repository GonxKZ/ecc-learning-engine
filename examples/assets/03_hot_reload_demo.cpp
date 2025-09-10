/**
 * ECScope Asset System Demo - Hot Reload
 * 
 * This demo demonstrates the hot-reload capabilities including:
 * - File system watching and change detection
 * - Live asset reloading without application restart
 * - Dependency tracking and cascaded reloads
 * - Network-based hot reload for team development
 * - Asset validation and rollback on errors
 */

#include "ecscope/assets/assets.hpp"
#include "ecscope/assets/concrete_assets.hpp"
#include "ecscope/assets/hot_reload.hpp"
#include "ecscope/core/application.hpp"
#include <iostream>
#include <fstream>
#include <filesystem>

using namespace ecscope;
using namespace ecscope::assets;

class HotReloadDemo : public core::Application {
public:
    HotReloadDemo() : core::Application("Hot Reload Demo") {}

private:
    std::unique_ptr<HotReloadSystem> m_hot_reload;
    std::vector<AssetHandle> m_watched_assets;
    std::string m_test_assets_dir;
    int m_reload_count = 0;
    
    bool initialize() override {
        std::cout << "=== ECScope Asset System - Hot Reload Demo ===\n\n";

        // Initialize asset system
        AssetManagerConfig config;
        config.max_memory_mb = 256;
        config.worker_threads = 2;
        config.enable_hot_reload = true;
        config.asset_root = "assets/";

        if (!initialize_asset_system(config)) {
            std::cerr << "Failed to initialize asset system!\n";
            return false;
        }

        // Setup test assets directory
        m_test_assets_dir = "assets/hot_reload_test/";
        std::filesystem::create_directories(m_test_assets_dir);

        // Initialize hot reload system
        auto& asset_manager = get_asset_manager();
        m_hot_reload = create_hot_reload_system(&asset_manager.get_registry(), 
                                               HotReloadConfig{});
        
        if (!m_hot_reload->initialize(m_test_assets_dir)) {
            std::cerr << "Failed to initialize hot reload system!\n";
            return false;
        }

        // Set up reload callback
        m_hot_reload->set_reload_callback([this](AssetID id, const std::string& path) {
            on_asset_reloaded(id, path);
        });

        std::cout << "Hot reload system initialized\n";
        std::cout << "Watching directory: " << m_test_assets_dir << "\n";
        std::cout << "Debounce time: 100ms\n\n";

        return true;
    }

    void run() override {
        create_test_assets();
        demonstrate_basic_hot_reload();
        demonstrate_dependency_reloading();
        demonstrate_error_handling();
        demonstrate_batch_reloading();
        show_hot_reload_statistics();
        interactive_session();
    }

    void shutdown() override {
        m_hot_reload->shutdown();
        m_hot_reload.reset();
        cleanup_test_assets();
        shutdown_asset_system();
        std::cout << "\nHot reload system shut down successfully\n";
    }

private:
    void create_test_assets() {
        std::cout << "=== Creating Test Assets ===\n";

        // Create test configuration file
        create_test_config("test_config.json", R"({
    "game_title": "Hot Reload Test",
    "version": "1.0.0",
    "debug_mode": true,
    "player_speed": 5.0
})");

        // Create test shader
        create_test_shader("test_shader.glsl", R"(#version 330 core

layout(location = 0) in vec3 position;
layout(location = 1) in vec2 texcoord;

uniform mat4 mvp_matrix;
uniform float time;

out vec2 v_texcoord;

void main() {
    vec3 pos = position;
    pos.y += sin(time + position.x) * 0.1;
    
    gl_Position = mvp_matrix * vec4(pos, 1.0);
    v_texcoord = texcoord;
}
)");

        // Create test material definition
        create_test_config("test_material.json", R"({
    "name": "test_material",
    "shader": "test_shader.glsl",
    "parameters": {
        "base_color": [1.0, 1.0, 1.0, 1.0],
        "metallic": 0.0,
        "roughness": 0.8,
        "emissive": [0.0, 0.0, 0.0]
    },
    "textures": {
        "albedo": "test_texture.png",
        "normal": "test_normal.png"
    }
})");

        std::cout << "Created test assets in " << m_test_assets_dir << "\n\n";
    }

    void demonstrate_basic_hot_reload() {
        std::cout << "=== Basic Hot Reload ===\n";

        // Load and register assets for hot reload
        std::string config_path = m_test_assets_dir + "test_config.json";
        auto config_asset = load_asset<ConfigAsset>(config_path);
        if (config_asset) {
            m_watched_assets.push_back(config_asset);
            m_hot_reload->register_asset_path(config_asset.get_id(), config_path);
            std::cout << "✓ Registered config asset for hot reload\n";
            std::cout << "  Original content preview: " 
                      << config_asset->get_config_data().substr(0, 50) << "...\n";
        }

        std::string shader_path = m_test_assets_dir + "test_shader.glsl";
        auto shader_asset = load_asset<ShaderAsset>(shader_path);
        if (shader_asset) {
            m_watched_assets.push_back(shader_asset);
            m_hot_reload->register_asset_path(shader_asset.get_id(), shader_path);
            std::cout << "✓ Registered shader asset for hot reload\n";
        }

        // Wait a bit, then modify the config file
        std::cout << "\nModifying config file in 2 seconds...\n";
        std::this_thread::sleep_for(std::chrono::seconds(2));

        // Modify config
        modify_test_config("test_config.json", R"({
    "game_title": "Hot Reload Test - UPDATED!",
    "version": "1.1.0",
    "debug_mode": false,
    "player_speed": 7.5,
    "new_feature": "hot_reload_works"
})");

        std::cout << "Modified config file, waiting for hot reload...\n";

        // Wait for hot reload to trigger
        std::this_thread::sleep_for(std::chrono::seconds(2));

        // Check if asset was reloaded
        if (config_asset && config_asset.is_loaded()) {
            std::cout << "✓ Config asset reloaded successfully\n";
            std::cout << "  Updated content preview: " 
                      << config_asset->get_config_data().substr(0, 50) << "...\n";
        }

        std::cout << "\n";
    }

    void demonstrate_dependency_reloading() {
        std::cout << "=== Dependency-Based Reloading ===\n";

        // Load material that depends on shader and textures
        std::string material_path = m_test_assets_dir + "test_material.json";
        auto material_asset = load_asset<MaterialAsset>(material_path);
        
        if (material_asset) {
            m_watched_assets.push_back(material_asset);
            m_hot_reload->register_asset_path(material_asset.get_id(), material_path);

            // Register dependencies
            std::vector<std::string> dependencies = {
                m_test_assets_dir + "test_shader.glsl",
                m_test_assets_dir + "test_texture.png",
                m_test_assets_dir + "test_normal.png"
            };
            m_hot_reload->register_dependency(material_path, dependencies);

            std::cout << "✓ Registered material with " << dependencies.size() 
                      << " dependencies\n";
        }

        // Modify the shader (dependency)
        std::cout << "\nModifying dependency (shader) in 2 seconds...\n";
        std::this_thread::sleep_for(std::chrono::seconds(2));

        modify_test_shader("test_shader.glsl", R"(#version 330 core

layout(location = 0) in vec3 position;
layout(location = 1) in vec2 texcoord;

uniform mat4 mvp_matrix;
uniform float time;
uniform vec3 wave_params; // NEW PARAMETER

out vec2 v_texcoord;

void main() {
    vec3 pos = position;
    // Updated wave calculation
    pos.y += sin(time * wave_params.x + position.x * wave_params.y) * wave_params.z;
    
    gl_Position = mvp_matrix * vec4(pos, 1.0);
    v_texcoord = texcoord;
}
)");

        std::cout << "Modified shader dependency, waiting for cascaded reload...\n";
        std::this_thread::sleep_for(std::chrono::seconds(3));

        std::cout << "Dependencies reloaded: " << m_reload_count << " assets affected\n\n";
    }

    void demonstrate_error_handling() {
        std::cout << "=== Error Handling and Rollback ===\n";

        // Create backup of current shader
        m_hot_reload->create_backup(m_watched_assets[1].get_id()); // shader asset
        std::cout << "✓ Created backup of shader asset\n";

        // Introduce syntax error
        std::cout << "\nIntroducing syntax error in 2 seconds...\n";
        std::this_thread::sleep_for(std::chrono::seconds(2));

        create_test_shader("test_shader.glsl", R"(#version 330 core

layout(location = 0) in vec3 position;
layout(location = 1) in vec2 texcoord;

uniform mat4 mvp_matrix;
uniform float time;

out vec2 v_texcoord;

void main() {
    vec3 pos = position;
    pos.y += sin(time + position.x * 0.1; // SYNTAX ERROR: missing closing parenthesis
    
    gl_Position = mvp_matrix * vec4(pos, 1.0);
    v_texcoord = texcoord;
}
)");

        std::cout << "Introduced syntax error, system should detect and handle it...\n";
        std::this_thread::sleep_for(std::chrono::seconds(3));

        // Try to restore backup
        std::cout << "Attempting to restore from backup...\n";
        if (m_hot_reload->restore_backup(m_watched_assets[1].get_id())) {
            std::cout << "✓ Successfully restored shader from backup\n";
        } else {
            std::cout << "✗ Failed to restore shader from backup\n";
        }

        std::cout << "\n";
    }

    void demonstrate_batch_reloading() {
        std::cout << "=== Batch Reloading ===\n";

        // Modify multiple files simultaneously
        std::cout << "Modifying multiple files simultaneously...\n";

        // Modify config
        modify_test_config("test_config.json", R"({
    "game_title": "Batch Reload Test",
    "version": "2.0.0",
    "debug_mode": true,
    "player_speed": 10.0,
    "batch_update": true
})");

        // Modify material
        modify_test_config("test_material.json", R"({
    "name": "test_material_v2",
    "shader": "test_shader.glsl",
    "parameters": {
        "base_color": [0.8, 0.9, 1.0, 1.0],
        "metallic": 0.2,
        "roughness": 0.4,
        "emissive": [0.1, 0.1, 0.2]
    },
    "textures": {
        "albedo": "test_texture.png",
        "normal": "test_normal.png"
    },
    "new_features": ["batch_reload", "improved_lighting"]
})");

        std::cout << "Modified 2 files, waiting for batch reload...\n";
        
        // Hot reload system should batch these changes
        std::this_thread::sleep_for(std::chrono::seconds(3));
        
        std::cout << "Batch reload completed\n\n";
    }

    void show_hot_reload_statistics() {
        std::cout << "=== Hot Reload Statistics ===\n";

        const auto& stats = m_hot_reload->get_statistics();
        std::cout << "Files watched: " << stats.files_watched.load() << "\n";
        std::cout << "Reload events: " << stats.reload_events.load() << "\n";
        std::cout << "Successful reloads: " << stats.successful_reloads.load() << "\n";
        std::cout << "Failed reloads: " << stats.failed_reloads.load() << "\n";
        std::cout << "Ignored events: " << stats.ignored_events.load() << "\n";
        
        float success_rate = stats.reload_events.load() > 0 ?
            (static_cast<float>(stats.successful_reloads.load()) / stats.reload_events.load()) * 100.0f : 0.0f;
        std::cout << "Success rate: " << success_rate << "%\n";
        
        std::cout << "\nWatched paths:\n";
        auto watch_paths = m_hot_reload->get_watch_paths();
        for (const auto& path : watch_paths) {
            std::cout << "  " << path << "\n";
        }
        
        std::cout << "\n";
    }

    void interactive_session() {
        std::cout << "=== Interactive Hot Reload Session ===\n";
        std::cout << "The system is now watching for file changes.\n";
        std::cout << "You can manually edit files in: " << m_test_assets_dir << "\n";
        std::cout << "Supported commands:\n";
        std::cout << "  'q' or 'quit' - Exit interactive session\n";
        std::cout << "  'stats' - Show current statistics\n";
        std::cout << "  'list' - List watched assets\n";
        std::cout << "  'reload <asset_id>' - Force reload specific asset\n";
        std::cout << "  'backup' - Create backups of all watched assets\n";
        std::cout << "  'clear' - Clear all backups\n\n";

        std::string input;
        while (true) {
            std::cout << "hot-reload> ";
            std::getline(std::cin, input);
            
            if (input == "q" || input == "quit") {
                break;
            } else if (input == "stats") {
                show_hot_reload_statistics();
            } else if (input == "list") {
                list_watched_assets();
            } else if (input.starts_with("reload ")) {
                try {
                    AssetID id = std::stoull(input.substr(7));
                    m_hot_reload->force_reload(id);
                    std::cout << "Forced reload of asset " << id << "\n";
                } catch (...) {
                    std::cout << "Invalid asset ID\n";
                }
            } else if (input == "backup") {
                for (const auto& asset : m_watched_assets) {
                    if (asset) {
                        m_hot_reload->create_backup(asset.get_id());
                    }
                }
                std::cout << "Created backups for all watched assets\n";
            } else if (input == "clear") {
                m_hot_reload->clear_backups();
                std::cout << "Cleared all backups\n";
            } else if (!input.empty()) {
                std::cout << "Unknown command: " << input << "\n";
            }
        }
    }

    // Callback methods
    void on_asset_reloaded(AssetID id, const std::string& path) {
        m_reload_count++;
        std::cout << "[HOT RELOAD] Asset " << id << " reloaded from " << path << "\n";
        
        // Find the asset in our watched list
        for (const auto& asset : m_watched_assets) {
            if (asset && asset.get_id() == id) {
                if (asset.is_loaded()) {
                    std::cout << "  ✓ Reload successful, asset is ready\n";
                } else {
                    std::cout << "  ⚠ Asset reloaded but not ready yet\n";
                }
                break;
            }
        }
    }

    // Helper methods
    void create_test_config(const std::string& filename, const std::string& content) {
        std::ofstream file(m_test_assets_dir + filename);
        file << content;
    }

    void modify_test_config(const std::string& filename, const std::string& content) {
        create_test_config(filename, content);
    }

    void create_test_shader(const std::string& filename, const std::string& content) {
        std::ofstream file(m_test_assets_dir + filename);
        file << content;
    }

    void modify_test_shader(const std::string& filename, const std::string& content) {
        create_test_shader(filename, content);
    }

    void list_watched_assets() {
        std::cout << "Watched assets:\n";
        for (size_t i = 0; i < m_watched_assets.size(); ++i) {
            const auto& asset = m_watched_assets[i];
            if (asset) {
                std::cout << "  [" << i << "] ID: " << asset.get_id() 
                          << ", Path: " << asset->get_path()
                          << ", State: " << asset_state_to_string(asset->get_state()) << "\n";
            }
        }
    }

    void cleanup_test_assets() {
        try {
            std::filesystem::remove_all(m_test_assets_dir);
            std::cout << "Cleaned up test assets directory\n";
        } catch (const std::exception& e) {
            std::cout << "Warning: Failed to clean up test assets: " << e.what() << "\n";
        }
    }
};

int main() {
    try {
        HotReloadDemo demo;
        return demo.execute();
    } catch (const std::exception& e) {
        std::cerr << "Demo failed with exception: " << e.what() << "\n";
        return -1;
    }
}