/**
 * ECScope Asset System Demo - Basic Asset Loading
 * 
 * This demo demonstrates the fundamental asset loading capabilities
 * of the ECScope asset system including:
 * - Loading different asset types (textures, models, audio, shaders)
 * - Synchronous and asynchronous loading
 * - Asset handle management
 * - Basic error handling
 */

#include "ecscope/assets/assets.hpp"
#include "ecscope/assets/concrete_assets.hpp"
#include "ecscope/core/application.hpp"
#include <iostream>
#include <thread>
#include <chrono>

using namespace ecscope;
using namespace ecscope::assets;

class BasicAssetLoadingDemo : public core::Application {
public:
    BasicAssetLoadingDemo() : core::Application("Basic Asset Loading Demo") {}

private:
    bool initialize() override {
        std::cout << "=== ECScope Asset System - Basic Loading Demo ===\n\n";

        // Initialize asset system
        AssetManagerConfig config;
        config.max_memory_mb = 256;
        config.worker_threads = 4;
        config.enable_hot_reload = true;
        config.asset_root = "assets/";

        if (!initialize_asset_system(config)) {
            std::cerr << "Failed to initialize asset system!\n";
            return false;
        }

        std::cout << "Asset system initialized successfully\n";
        std::cout << "Configuration:\n";
        std::cout << "  Memory Budget: " << config.max_memory_mb << " MB\n";
        std::cout << "  Worker Threads: " << config.worker_threads << "\n";
        std::cout << "  Asset Root: " << config.asset_root << "\n\n";

        return true;
    }

    void run() override {
        demonstrate_basic_loading();
        demonstrate_async_loading();
        demonstrate_batch_loading();
        demonstrate_asset_types();
        demonstrate_error_handling();
        show_system_statistics();
    }

    void shutdown() override {
        shutdown_asset_system();
        std::cout << "\nAsset system shut down successfully\n";
    }

private:
    void demonstrate_basic_loading() {
        std::cout << "=== Basic Asset Loading ===\n";

        // Load a texture synchronously
        auto texture = load_asset<TextureAsset>("textures/test_texture.png");
        if (texture) {
            std::cout << "✓ Texture loaded: " << texture->get_path() << "\n";
            std::cout << "  Dimensions: " << texture->get_width() << "x" << texture->get_height() << "\n";
            std::cout << "  Channels: " << texture->get_channels() << "\n";
            std::cout << "  Size: " << (texture->get_memory_usage() / 1024) << " KB\n";
        } else {
            std::cout << "✗ Failed to load texture\n";
        }

        // Load a model
        auto model = load_asset<ModelAsset>("models/test_model.obj");
        if (model) {
            std::cout << "✓ Model loaded: " << model->get_path() << "\n";
            std::cout << "  Meshes: " << model->get_mesh_count() << "\n";
            std::cout << "  Vertices: " << model->get_vertex_count() << "\n";
            std::cout << "  Triangles: " << model->get_triangle_count() << "\n";
            std::cout << "  Size: " << (model->get_memory_usage() / 1024) << " KB\n";
        } else {
            std::cout << "✗ Failed to load model\n";
        }

        std::cout << "\n";
    }

    void demonstrate_async_loading() {
        std::cout << "=== Asynchronous Asset Loading ===\n";

        // Start async loads
        auto texture_future = load_asset_async<TextureAsset>("textures/large_texture.png");
        auto audio_future = load_asset_async<AudioAsset>("audio/background_music.ogg");
        auto model_future = load_asset_async<ModelAsset>("models/complex_model.fbx");

        std::cout << "Started 3 async loading operations...\n";

        // Show loading progress simulation
        for (int i = 0; i < 10; ++i) {
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
            std::cout << "Loading... " << (i + 1) * 10 << "%\r" << std::flush;
        }
        std::cout << "\n";

        // Wait for completion and check results
        try {
            auto texture = texture_future.get();
            if (texture) {
                std::cout << "✓ Async texture loaded: " 
                         << texture->get_width() << "x" << texture->get_height() << "\n";
            }

            auto audio = audio_future.get();
            if (audio) {
                std::cout << "✓ Async audio loaded: " 
                         << audio->get_duration() << "s, " 
                         << audio->get_sample_rate() << "Hz\n";
            }

            auto model = model_future.get();
            if (model) {
                std::cout << "✓ Async model loaded: " 
                         << model->get_mesh_count() << " meshes\n";
            }
        } catch (const std::exception& e) {
            std::cout << "✗ Async loading error: " << e.what() << "\n";
        }

        std::cout << "\n";
    }

    void demonstrate_batch_loading() {
        std::cout << "=== Batch Asset Loading ===\n";

        // Prepare batch of asset paths
        std::vector<std::string> asset_paths = {
            "textures/ui_button.png",
            "textures/ui_background.png", 
            "textures/ui_icon_health.png",
            "textures/ui_icon_ammo.png",
            "audio/ui_click.wav",
            "audio/ui_hover.wav"
        };

        auto start_time = std::chrono::steady_clock::now();

        // Load all assets in batch
        auto handles = load_assets_batch<Asset>(asset_paths);

        auto end_time = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
            end_time - start_time).count();

        std::cout << "Batch loaded " << handles.size() << " assets in " 
                  << duration << "ms\n";

        size_t successful = 0;
        size_t total_size = 0;
        
        for (const auto& handle : handles) {
            if (handle && handle.is_loaded()) {
                successful++;
                total_size += handle->get_memory_usage();
            }
        }

        std::cout << "Success rate: " << successful << "/" << handles.size() 
                  << " (" << (successful * 100 / handles.size()) << "%)\n";
        std::cout << "Total memory used: " << (total_size / 1024) << " KB\n\n";
    }

    void demonstrate_asset_types() {
        std::cout << "=== Different Asset Types ===\n";

        // Texture asset
        std::cout << "Loading texture assets...\n";
        auto diffuse = load_asset<TextureAsset>("textures/diffuse.jpg");
        auto normal = load_asset<TextureAsset>("textures/normal.png");
        auto specular = load_asset<TextureAsset>("textures/specular.tga");
        
        if (diffuse && normal && specular) {
            std::cout << "✓ Material texture set loaded\n";
            std::cout << "  Diffuse: " << diffuse->get_width() << "x" << diffuse->get_height() << "\n";
            std::cout << "  Normal: " << (normal->has_alpha() ? "with alpha" : "no alpha") << "\n";
            std::cout << "  Specular: " << (specular->is_srgb() ? "sRGB" : "linear") << "\n";
        }

        // Audio assets
        std::cout << "\nLoading audio assets...\n";
        auto sfx = load_asset<AudioAsset>("audio/explosion.wav");
        auto music = load_asset<AudioAsset>("audio/ambient.ogg");
        
        if (sfx && music) {
            std::cout << "✓ Audio assets loaded\n";
            std::cout << "  SFX: " << sfx->get_duration() << "s, " 
                     << (sfx->is_3d_audio() ? "3D" : "2D") << "\n";
            std::cout << "  Music: " << music->get_channel_count() << " channels, "
                     << (music->is_music() ? "music" : "sound effect") << "\n";
        }

        // Shader assets
        std::cout << "\nLoading shader assets...\n";
        auto vertex_shader = load_asset<ShaderAsset>("shaders/basic.vert");
        auto fragment_shader = load_asset<ShaderAsset>("shaders/basic.frag");
        
        if (vertex_shader && fragment_shader) {
            std::cout << "✓ Shader program loaded\n";
            std::cout << "  Vertex shader uniforms: " 
                     << vertex_shader->get_uniforms().size() << "\n";
            std::cout << "  Fragment shader textures: " 
                     << fragment_shader->get_textures().size() << "\n";
        }

        std::cout << "\n";
    }

    void demonstrate_error_handling() {
        std::cout << "=== Error Handling ===\n";

        // Try to load non-existent asset
        auto missing_texture = load_asset<TextureAsset>("textures/does_not_exist.png");
        if (!missing_texture) {
            std::cout << "✓ Properly handled missing texture\n";
        }

        // Try to load corrupted asset
        auto corrupted_model = load_asset<ModelAsset>("models/corrupted.obj");
        if (!corrupted_model || corrupted_model->has_error()) {
            std::cout << "✓ Properly handled corrupted model\n";
        }

        // Try to load asset with wrong type
        auto wrong_type = load_asset<AudioAsset>("textures/test_texture.png");
        if (!wrong_type) {
            std::cout << "✓ Properly handled wrong asset type\n";
        }

        std::cout << "\n";
    }

    void show_system_statistics() {
        std::cout << "=== System Statistics ===\n";

        auto stats = get_asset_system_statistics();
        
        std::cout << "Load Statistics:\n";
        std::cout << "  Total assets loaded: " << stats.total_assets_loaded << "\n";
        std::cout << "  Cache hits: " << stats.total_cache_hits << "\n";
        std::cout << "  Cache misses: " << stats.total_cache_misses << "\n";
        std::cout << "  Cache hit rate: " 
                  << (stats.total_cache_hits * 100.0f / 
                     (stats.total_cache_hits + stats.total_cache_misses)) << "%\n";
        
        std::cout << "\nMemory Usage:\n";
        std::cout << "  Total memory used: " << (stats.total_memory_used / 1024 / 1024) << " MB\n";
        std::cout << "  Average load time: " << stats.average_load_time_ms << " ms\n";
        
        std::cout << "\nAsset Manager:\n";
        auto& manager = get_asset_manager();
        std::cout << "  Active assets: " << manager.get_asset_count() << "\n";
        std::cout << "  Memory usage: " << (manager.get_memory_usage() / 1024 / 1024) << " MB\n";
        
        const auto& load_stats = manager.get_load_statistics();
        std::cout << "  Successful loads: " << load_stats.successful_loads.load() << "\n";
        std::cout << "  Failed loads: " << load_stats.failed_loads.load() << "\n";
        std::cout << "  Total requests: " << load_stats.total_requests.load() << "\n";
    }
};

int main() {
    try {
        BasicAssetLoadingDemo demo;
        return demo.execute();
    } catch (const std::exception& e) {
        std::cerr << "Demo failed with exception: " << e.what() << "\n";
        return -1;
    }
}