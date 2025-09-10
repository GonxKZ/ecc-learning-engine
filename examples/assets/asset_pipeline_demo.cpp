/**
 * @file asset_pipeline_demo.cpp
 * @brief Comprehensive demonstration of the ECScope asset pipeline system
 * 
 * This demo showcases:
 * - Multi-threaded asset loading with priority queues
 * - Asset processing pipeline (texture, model, audio)
 * - Hot-reload capabilities with file watching
 * - Asset management with dependency resolution
 * - ECS integration with asset components
 * - Memory management and streaming
 * - Performance benchmarking
 */

#include "ecscope/assets/core/asset_types.hpp"
#include "ecscope/assets/core/asset_handle.hpp"
#include "ecscope/assets/loading/asset_loader.hpp"
#include "ecscope/assets/processing/texture_processor.hpp"
#include "ecscope/assets/processing/model_processor.hpp"
#include "ecscope/assets/processing/audio_processor.hpp"
#include "ecscope/assets/management/asset_manager.hpp"
#include "ecscope/assets/integration/ecs_components.hpp"
#include "ecscope/assets/hotreload/file_watcher.hpp"

#include <iostream>
#include <chrono>
#include <thread>
#include <vector>
#include <memory>

using namespace ecscope::assets;

// =============================================================================
// Demo Asset Classes
// =============================================================================

class DemoTextureAsset : public TextureAsset {
public:
    AssetLoadResult load(const std::string& path, const AssetLoadParams& params = {}) override {
        std::cout << "Loading texture: " << path << std::endl;
        
        // Simulate texture loading
        auto start = std::chrono::steady_clock::now();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        auto end = std::chrono::steady_clock::now();
        
        // Create dummy texture data
        auto texture_data = std::make_unique<TextureData>();
        texture_data->width = 512;
        texture_data->height = 512;
        texture_data->format = TextureFormat::RGBA8;
        texture_data->data.resize(512 * 512 * 4, 255); // White texture
        
        texture_data_ = std::move(texture_data);
        setState(AssetState::Ready);
        
        AssetLoadResult result;
        result.success = true;
        result.bytes_loaded = getMemoryUsage();
        result.load_time = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        
        return result;
    }
};

class DemoModelAsset : public ModelAsset {
public:
    AssetLoadResult load(const std::string& path, const AssetLoadParams& params = {}) override {
        std::cout << "Loading model: " << path << std::endl;
        
        auto start = std::chrono::steady_clock::now();
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        auto end = std::chrono::steady_clock::now();
        
        // Create dummy model data
        auto model_data = std::make_unique<ModelData>();
        model_data->name = "Demo Model";
        
        // Create a simple triangle mesh
        Mesh mesh;
        mesh.name = "Triangle";
        
        SubMesh submesh;
        submesh.vertices = {
            {{-0.5f, -0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},
            {{ 0.5f, -0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f}},
            {{ 0.0f,  0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.5f, 1.0f}}
        };
        submesh.indices = {0, 1, 2};
        submesh.calculateBounds();
        
        mesh.sub_meshes.push_back(std::move(submesh));
        mesh.calculateBounds();
        
        model_data->meshes.push_back(std::move(mesh));
        model_data->calculateBounds();
        
        model_data_ = std::move(model_data);
        setState(AssetState::Ready);
        
        AssetLoadResult result;
        result.success = true;
        result.bytes_loaded = getMemoryUsage();
        result.load_time = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        
        return result;
    }
};

class DemoAudioAsset : public AudioAsset {
public:
    AssetLoadResult load(const std::string& path, const AssetLoadParams& params = {}) override {
        std::cout << "Loading audio: " << path << std::endl;
        
        auto start = std::chrono::steady_clock::now();
        std::this_thread::sleep_for(std::chrono::milliseconds(150));
        auto end = std::chrono::steady_clock::now();
        
        // Create dummy audio data
        auto audio_data = std::make_unique<AudioData>();
        audio_data->format = AudioFormat::PCM_S16;
        audio_data->sample_rate = 44100;
        audio_data->channels = AudioChannelLayout::Stereo;
        audio_data->frame_count = 44100; // 1 second of audio
        audio_data->data.resize(44100 * 2 * 2, 0); // 1 second stereo 16-bit
        
        audio_data_ = std::move(audio_data);
        setState(AssetState::Ready);
        
        AssetLoadResult result;
        result.success = true;
        result.bytes_loaded = getMemoryUsage();
        result.load_time = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        
        return result;
    }
};

// =============================================================================
// Demo Functions
// =============================================================================

void demonstrateAssetTypes() {
    std::cout << "\n=== Asset Type Registration Demo ===\n";
    
    auto& registry = AssetTypeRegistry::instance();
    
    // Register demo asset types
    registry.registerType(
        DemoTextureAsset::ASSET_TYPE_ID, 
        "DemoTexture",
        []() { return std::make_unique<DemoTextureAsset>(); },
        [](const std::string& path, Asset& asset, const AssetLoadParams& params) {
            return static_cast<DemoTextureAsset&>(asset).load(path, params);
        },
        {".png", ".jpg", ".dds"}
    );
    
    registry.registerType(
        DemoModelAsset::ASSET_TYPE_ID,
        "DemoModel", 
        []() { return std::make_unique<DemoModelAsset>(); },
        [](const std::string& path, Asset& asset, const AssetLoadParams& params) {
            return static_cast<DemoModelAsset&>(asset).load(path, params);
        },
        {".obj", ".fbx", ".gltf"}
    );
    
    registry.registerType(
        DemoAudioAsset::ASSET_TYPE_ID,
        "DemoAudio",
        []() { return std::make_unique<DemoAudioAsset>(); },
        [](const std::string& path, Asset& asset, const AssetLoadParams& params) {
            return static_cast<DemoAudioAsset&>(asset).load(path, params);
        },
        {".wav", ".ogg", ".mp3"}
    );
    
    std::cout << "Registered asset types:\n";
    std::cout << "- Texture (.png, .jpg, .dds)\n";
    std::cout << "- Model (.obj, .fbx, .gltf)\n"; 
    std::cout << "- Audio (.wav, .ogg, .mp3)\n";
    
    // Test type detection by extension
    auto texture_type = registry.getTypeIDByExtension(".png");
    auto model_type = registry.getTypeIDByExtension(".obj");
    auto audio_type = registry.getTypeIDByExtension(".wav");
    
    std::cout << "Type IDs: Texture=" << texture_type 
              << ", Model=" << model_type 
              << ", Audio=" << audio_type << std::endl;
}

void demonstrateMultithreadedLoading() {
    std::cout << "\n=== Multi-threaded Asset Loading Demo ===\n";
    
    AssetLoader loader(4); // 4 worker threads
    
    // Create test assets
    auto texture_asset = std::make_shared<DemoTextureAsset>();
    auto model_asset = std::make_shared<DemoModelAsset>();
    auto audio_asset = std::make_shared<DemoAudioAsset>();
    
    std::vector<std::future<AssetLoadResult>> futures;
    auto start_time = std::chrono::steady_clock::now();
    
    // Load assets with different priorities
    AssetLoadParams high_priority;
    high_priority.priority = AssetPriority::High;
    
    AssetLoadParams normal_priority;
    normal_priority.priority = AssetPriority::Normal;
    
    AssetLoadParams low_priority;
    low_priority.priority = AssetPriority::Low;
    
    // Submit multiple load requests
    futures.push_back(loader.loadAsync(1, "texture1.png", DemoTextureAsset::ASSET_TYPE_ID, texture_asset, high_priority));
    futures.push_back(loader.loadAsync(2, "model1.obj", DemoModelAsset::ASSET_TYPE_ID, model_asset, normal_priority));
    futures.push_back(loader.loadAsync(3, "audio1.wav", DemoAudioAsset::ASSET_TYPE_ID, audio_asset, low_priority));
    
    // Wait for all assets to load
    std::cout << "Loading assets in parallel...\n";
    
    for (auto& future : futures) {
        auto result = future.get();
        std::cout << "Asset loaded: " << (result.success ? "SUCCESS" : "FAILED") 
                  << ", bytes=" << result.bytes_loaded 
                  << ", time=" << result.load_time.count() << "ms\n";
    }
    
    auto end_time = std::chrono::steady_clock::now();
    auto total_time = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    std::cout << "Total parallel loading time: " << total_time.count() << "ms\n";
    
    // Show loader statistics
    auto stats = loader.getStatistics();
    std::cout << "Loader statistics:\n";
    std::cout << "- Total loads: " << stats.total_assets << "\n";
    std::cout << "- Successful loads: " << stats.loaded_assets << "\n";
    std::cout << "- Memory used: " << stats.memory_used << " bytes\n";
    std::cout << "- Average load time: " << stats.average_load_time.count() << "ms\n";
}

void demonstrateAssetProcessing() {
    std::cout << "\n=== Asset Processing Pipeline Demo ===\n";
    
    // Texture processing demo
    TextureProcessor texture_processor;
    TextureProcessingOptions texture_options;
    texture_options.generate_mipmaps = true;
    texture_options.compress = true;
    texture_options.target_quality = AssetQuality::High;
    
    std::cout << "Texture processing options:\n";
    std::cout << "- Generate mipmaps: " << (texture_options.generate_mipmaps ? "YES" : "NO") << "\n";
    std::cout << "- Compression: " << (texture_options.compress ? "YES" : "NO") << "\n";
    std::cout << "- Target quality: HIGH\n";
    
    // Model processing demo
    ModelProcessor model_processor;
    ModelProcessingOptions model_options;
    model_options.optimize_vertices = true;
    model_options.generate_lods = true;
    model_options.calculate_tangents = true;
    
    std::cout << "\nModel processing options:\n";
    std::cout << "- Optimize vertices: " << (model_options.optimize_vertices ? "YES" : "NO") << "\n";
    std::cout << "- Generate LODs: " << (model_options.generate_lods ? "YES" : "NO") << "\n";
    std::cout << "- Calculate tangents: " << (model_options.calculate_tangents ? "YES" : "NO") << "\n";
    
    // Audio processing demo
    AudioProcessor audio_processor;
    AudioProcessingOptions audio_options;
    audio_options.normalize = true;
    audio_options.target_sample_rate = 44100;
    audio_options.target_channels = AudioChannelLayout::Stereo;
    
    std::cout << "\nAudio processing options:\n";
    std::cout << "- Normalize: " << (audio_options.normalize ? "YES" : "NO") << "\n";
    std::cout << "- Target sample rate: " << audio_options.target_sample_rate << "Hz\n";
    std::cout << "- Target channels: STEREO\n";
}

void demonstrateHotReload() {
    std::cout << "\n=== Hot Reload System Demo ===\n";
    
    HotReloadManager hot_reload;
    
    // Set up hot reload callback
    hot_reload.setReloadCallback([](AssetID asset_id, const std::string& path) {
        std::cout << "Hot reload triggered for asset " << asset_id << " (" << path << ")\n";
    });
    
    // Configure hot reload
    auto config = hot_reload.getConfig();
    config.auto_reload = true;
    config.reload_delay = std::chrono::milliseconds(100);
    config.reload_dependencies = true;
    hot_reload.setConfig(config);
    
    std::cout << "Hot reload configuration:\n";
    std::cout << "- Auto reload: " << (config.auto_reload ? "ENABLED" : "DISABLED") << "\n";
    std::cout << "- Reload delay: " << config.reload_delay.count() << "ms\n";
    std::cout << "- Reload dependencies: " << (config.reload_dependencies ? "YES" : "NO") << "\n";
    
    // Add watch directory (in a real application)
    // hot_reload.addWatchDirectory("assets/", true);
    
    std::cout << "Hot reload system ready (would watch 'assets/' directory in real use)\n";
    
    // Demonstrate manual reload trigger
    hot_reload.registerAsset(1, "assets/texture.png");
    hot_reload.triggerReload(1);
    
    auto stats = hot_reload.getStatistics();
    std::cout << "Hot reload statistics:\n";
    std::cout << "- Assets reloaded: " << stats.assets_reloaded << "\n";
    std::cout << "- Reload failures: " << stats.reload_failures << "\n";
}

void demonstrateECSIntegration() {
    std::cout << "\n=== ECS Integration Demo ===\n";
    
    // Create asset components
    TextureComponent texture_comp("assets/player_texture.png");
    ModelComponent model_comp("assets/player_model.obj");
    AudioComponent audio_comp("assets/footstep.wav");
    
    texture_comp.auto_load = true;
    texture_comp.load_priority = AssetPriority::High;
    
    model_comp.visible = true;
    model_comp.cast_shadows = true;
    model_comp.setLODDistance(100.0f);
    
    auto& audio_playback = audio_comp.getPlaybackState();
    audio_playback.volume = 0.8f;
    audio_playback.looping = false;
    
    auto& spatial = audio_comp.getSpatialProperties();
    spatial.enabled = true;
    spatial.min_distance = 1.0f;
    spatial.max_distance = 50.0f;
    
    std::cout << "Created ECS components:\n";
    std::cout << "- TextureComponent: " << texture_comp.getTexturePath() << " (priority: HIGH)\n";
    std::cout << "- ModelComponent: " << model_comp.getModelPath() << " (LOD distance: 100m)\n";
    std::cout << "- AudioComponent: " << audio_comp.getAudioPath() << " (3D audio enabled)\n";
    
    // Demonstrate asset collection
    AssetCollectionComponent collection;
    // collection.addAsset("texture", texture_comp.getTextureHandle());
    // collection.addAsset("model", model_comp.getModelHandle());
    // collection.addAsset("audio", audio_comp.getAudioHandle());
    
    std::cout << "Created asset collection with " << collection.getAssetCount() << " assets\n";
    
    // Demonstrate streaming component
    AssetStreamingComponent streaming;
    auto streaming_config = streaming.getStreamingConfig();
    streaming_config.lod_distances = {0.0f, 25.0f, 50.0f, 100.0f};
    streaming_config.lod_qualities = {AssetQuality::Ultra, AssetQuality::High, AssetQuality::Medium, AssetQuality::Low};
    streaming.setStreamingConfig(streaming_config);
    
    std::cout << "Streaming component configured with " << streaming_config.lod_distances.size() << " LOD levels\n";
}

void demonstrateMemoryManagement() {
    std::cout << "\n=== Memory Management Demo ===\n";
    
    AssetReferenceManager ref_manager;
    
    // Set memory budget (256MB)
    ref_manager.setMemoryBudget(256 * 1024 * 1024);
    
    std::cout << "Memory budget: " << (ref_manager.getMemoryBudget() / (1024 * 1024)) << "MB\n";
    
    // Simulate asset references
    for (AssetID id = 1; id <= 10; ++id) {
        ref_manager.addReference(id);
        ref_manager.recordAccess(id);
        
        if (id % 3 == 0) {
            ref_manager.addReference(id); // Add extra reference
        }
    }
    
    std::cout << "Simulated 10 assets with references\n";
    
    // Show reference counts
    for (AssetID id = 1; id <= 5; ++id) {
        uint32_t ref_count = ref_manager.getReferenceCount(id);
        std::cout << "Asset " << id << ": " << ref_count << " references\n";
    }
    
    // Get unload candidates
    auto unload_candidates = ref_manager.getUnloadCandidates();
    std::cout << "Assets eligible for unloading: " << unload_candidates.size() << "\n";
    
    // Get LRU candidates
    auto lru_candidates = ref_manager.getLeastRecentlyUsed(3);
    std::cout << "Least recently used assets (top 3): ";
    for (auto id : lru_candidates) {
        std::cout << id << " ";
    }
    std::cout << "\n";
}

void benchmarkAssetPipeline() {
    std::cout << "\n=== Asset Pipeline Benchmark ===\n";
    
    constexpr int NUM_ASSETS = 100;
    constexpr int NUM_THREADS = 4;
    
    AssetLoader loader(NUM_THREADS);
    
    std::vector<std::shared_ptr<DemoTextureAsset>> texture_assets;
    std::vector<std::future<AssetLoadResult>> futures;
    
    // Prepare assets
    for (int i = 0; i < NUM_ASSETS; ++i) {
        texture_assets.push_back(std::make_shared<DemoTextureAsset>());
    }
    
    std::cout << "Benchmarking " << NUM_ASSETS << " texture loads with " << NUM_THREADS << " threads...\n";
    
    auto start_time = std::chrono::steady_clock::now();
    
    // Submit all load requests
    for (int i = 0; i < NUM_ASSETS; ++i) {
        std::string path = "benchmark_texture_" + std::to_string(i) + ".png";
        futures.push_back(loader.loadAsync(i + 1, path, DemoTextureAsset::ASSET_TYPE_ID, texture_assets[i]));
    }
    
    // Wait for all to complete
    int successful_loads = 0;
    uint64_t total_bytes = 0;
    
    for (auto& future : futures) {
        auto result = future.get();
        if (result.success) {
            successful_loads++;
            total_bytes += result.bytes_loaded;
        }
    }
    
    auto end_time = std::chrono::steady_clock::now();
    auto total_time = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    std::cout << "Benchmark Results:\n";
    std::cout << "- Total time: " << total_time.count() << "ms\n";
    std::cout << "- Successful loads: " << successful_loads << "/" << NUM_ASSETS << "\n";
    std::cout << "- Total data loaded: " << (total_bytes / (1024 * 1024)) << "MB\n";
    std::cout << "- Average load time: " << (total_time.count() / NUM_ASSETS) << "ms per asset\n";
    std::cout << "- Throughput: " << (NUM_ASSETS * 1000 / total_time.count()) << " assets/second\n";
    
    // Show final statistics
    auto stats = loader.getStatistics();
    std::cout << "Final loader statistics:\n";
    std::cout << "- Memory used: " << (stats.memory_used / (1024 * 1024)) << "MB\n";
    std::cout << "- Total load time: " << stats.total_load_time.count() << "ms\n";
}

// =============================================================================
// Main Demo Function
// =============================================================================

int main() {
    std::cout << "ECScope Asset Pipeline Comprehensive Demo\n";
    std::cout << "========================================\n";
    
    try {
        demonstrateAssetTypes();
        demonstrateMultithreadedLoading();
        demonstrateAssetProcessing();
        demonstrateHotReload();
        demonstrateECSIntegration();
        demonstrateMemoryManagement();
        benchmarkAssetPipeline();
        
        std::cout << "\n=== Demo Complete ===\n";
        std::cout << "All asset pipeline features demonstrated successfully!\n";
        
    } catch (const std::exception& e) {
        std::cerr << "Demo failed with exception: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}