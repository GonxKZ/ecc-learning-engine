/**
 * @file asset_benchmarks.cpp
 * @brief Professional benchmarking suite for ECScope asset pipeline
 * 
 * Comprehensive benchmarks covering:
 * - Asset loading performance (single vs multi-threaded)
 * - Memory usage and allocation patterns
 * - Asset processing pipeline throughput
 * - Hot-reload system performance
 * - Cache efficiency and hit rates
 * - Large-scale asset catalog handling
 */

#include "ecscope/assets/core/asset_types.hpp"
#include "ecscope/assets/loading/asset_loader.hpp"
#include "ecscope/assets/processing/texture_processor.hpp"
#include "ecscope/assets/management/asset_manager.hpp"
#include "ecscope/assets/hotreload/file_watcher.hpp"

#include <iostream>
#include <chrono>
#include <vector>
#include <thread>
#include <random>
#include <algorithm>
#include <fstream>
#include <iomanip>

using namespace ecscope::assets;
using namespace std::chrono;

// =============================================================================
// Benchmark Infrastructure
// =============================================================================

class BenchmarkTimer {
public:
    void start() { start_time_ = steady_clock::now(); }
    void stop() { end_time_ = steady_clock::now(); }
    
    double getMilliseconds() const {
        return duration_cast<duration<double, std::milli>>(end_time_ - start_time_).count();
    }
    
    double getSeconds() const {
        return duration_cast<duration<double>>(end_time_ - start_time_).count();
    }
    
private:
    steady_clock::time_point start_time_, end_time_;
};

struct BenchmarkResult {
    std::string name;
    double time_ms;
    uint64_t operations;
    uint64_t bytes_processed;
    double operations_per_second;
    double throughput_mbps;
    
    void calculate() {
        operations_per_second = operations / (time_ms / 1000.0);
        throughput_mbps = (bytes_processed / (1024.0 * 1024.0)) / (time_ms / 1000.0);
    }
};

class BenchmarkSuite {
public:
    void addResult(const BenchmarkResult& result) {
        results_.push_back(result);
    }
    
    void printResults() const {
        std::cout << "\n" << std::string(80, '=') << "\n";
        std::cout << "BENCHMARK RESULTS SUMMARY\n";
        std::cout << std::string(80, '=') << "\n";
        
        std::cout << std::left << std::setw(30) << "Benchmark Name"
                  << std::setw(12) << "Time (ms)"
                  << std::setw(12) << "Ops/sec" 
                  << std::setw(15) << "Throughput MB/s"
                  << "Memory\n";
        std::cout << std::string(80, '-') << "\n";
        
        for (const auto& result : results_) {
            std::cout << std::left << std::setw(30) << result.name
                      << std::setw(12) << std::fixed << std::setprecision(2) << result.time_ms
                      << std::setw(12) << std::fixed << std::setprecision(0) << result.operations_per_second
                      << std::setw(15) << std::fixed << std::setprecision(2) << result.throughput_mbps
                      << (result.bytes_processed / (1024 * 1024)) << " MB\n";
        }
        
        std::cout << std::string(80, '=') << "\n";
    }
    
    void saveToFile(const std::string& filename) const {
        std::ofstream file(filename);
        file << "Benchmark,Time_ms,Operations,Bytes,Ops_per_sec,Throughput_MBps\n";
        
        for (const auto& result : results_) {
            file << result.name << "," << result.time_ms << "," 
                 << result.operations << "," << result.bytes_processed << ","
                 << result.operations_per_second << "," << result.throughput_mbps << "\n";
        }
    }
    
private:
    std::vector<BenchmarkResult> results_;
};

// =============================================================================
// Mock Assets for Benchmarking
// =============================================================================

class BenchmarkTextureAsset : public Asset {
public:
    ASSET_TYPE(BenchmarkTextureAsset, 2001)
    
    AssetLoadResult load(const std::string& path, const AssetLoadParams& params = {}) override {
        // Simulate variable loading time based on quality
        int load_time_ms = 10;
        switch (params.quality) {
            case AssetQuality::Ultra: load_time_ms = 50; break;
            case AssetQuality::High: load_time_ms = 30; break;
            case AssetQuality::Medium: load_time_ms = 20; break;
            case AssetQuality::Low: load_time_ms = 10; break;
            default: break;
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(load_time_ms));
        
        // Simulate texture data
        size_t texture_size = 1024 * 1024 * 4; // 1MB RGBA texture
        texture_data_.resize(texture_size);
        
        setState(AssetState::Ready);
        
        AssetLoadResult result;
        result.success = true;
        result.bytes_loaded = texture_size;
        result.load_time = std::chrono::milliseconds(load_time_ms);
        
        return result;
    }
    
    void unload() override {
        texture_data_.clear();
        setState(AssetState::NotLoaded);
    }
    
    bool isLoaded() const override {
        return getState() == AssetState::Ready;
    }
    
    uint64_t getMemoryUsage() const override {
        return texture_data_.size();
    }
    
private:
    std::vector<uint8_t> texture_data_;
};

// =============================================================================
// Loading Performance Benchmarks
// =============================================================================

BenchmarkResult benchmarkSingleThreadedLoading() {
    std::cout << "Running single-threaded loading benchmark...\n";
    
    const int NUM_ASSETS = 100;
    AssetLoader loader(1); // Single thread
    
    std::vector<std::shared_ptr<BenchmarkTextureAsset>> assets;
    std::vector<std::future<AssetLoadResult>> futures;
    
    for (int i = 0; i < NUM_ASSETS; ++i) {
        assets.push_back(std::make_shared<BenchmarkTextureAsset>());
    }
    
    BenchmarkTimer timer;
    timer.start();
    
    for (int i = 0; i < NUM_ASSETS; ++i) {
        std::string path = "benchmark_" + std::to_string(i) + ".png";
        futures.push_back(loader.loadAsync(i + 1, path, BenchmarkTextureAsset::ASSET_TYPE_ID, assets[i]));
    }
    
    uint64_t total_bytes = 0;
    for (auto& future : futures) {
        auto result = future.get();
        total_bytes += result.bytes_loaded;
    }
    
    timer.stop();
    
    BenchmarkResult result;
    result.name = "Single-threaded Loading";
    result.time_ms = timer.getMilliseconds();
    result.operations = NUM_ASSETS;
    result.bytes_processed = total_bytes;
    result.calculate();
    
    return result;
}

BenchmarkResult benchmarkMultiThreadedLoading() {
    std::cout << "Running multi-threaded loading benchmark...\n";
    
    const int NUM_ASSETS = 100;
    const int NUM_THREADS = std::thread::hardware_concurrency();
    AssetLoader loader(NUM_THREADS);
    
    std::vector<std::shared_ptr<BenchmarkTextureAsset>> assets;
    std::vector<std::future<AssetLoadResult>> futures;
    
    for (int i = 0; i < NUM_ASSETS; ++i) {
        assets.push_back(std::make_shared<BenchmarkTextureAsset>());
    }
    
    BenchmarkTimer timer;
    timer.start();
    
    for (int i = 0; i < NUM_ASSETS; ++i) {
        std::string path = "benchmark_" + std::to_string(i) + ".png";
        futures.push_back(loader.loadAsync(i + 1, path, BenchmarkTextureAsset::ASSET_TYPE_ID, assets[i]));
    }
    
    uint64_t total_bytes = 0;
    for (auto& future : futures) {
        auto result = future.get();
        total_bytes += result.bytes_loaded;
    }
    
    timer.stop();
    
    BenchmarkResult result;
    result.name = "Multi-threaded Loading (" + std::to_string(NUM_THREADS) + " threads)";
    result.time_ms = timer.getMilliseconds();
    result.operations = NUM_ASSETS;
    result.bytes_processed = total_bytes;
    result.calculate();
    
    return result;
}

BenchmarkResult benchmarkPriorityLoading() {
    std::cout << "Running priority-based loading benchmark...\n";
    
    const int NUM_ASSETS = 50;
    AssetLoader loader(4);
    
    std::vector<std::shared_ptr<BenchmarkTextureAsset>> assets;
    std::vector<std::future<AssetLoadResult>> futures;
    std::vector<AssetPriority> priorities = {
        AssetPriority::Critical, AssetPriority::High, AssetPriority::Normal, 
        AssetPriority::Low, AssetPriority::Background
    };
    
    for (int i = 0; i < NUM_ASSETS; ++i) {
        assets.push_back(std::make_shared<BenchmarkTextureAsset>());
    }
    
    BenchmarkTimer timer;
    timer.start();
    
    // Mix priorities randomly
    std::random_device rd;
    std::mt19937 gen(rd());
    
    for (int i = 0; i < NUM_ASSETS; ++i) {
        AssetLoadParams params;
        params.priority = priorities[gen() % priorities.size()];
        
        std::string path = "priority_benchmark_" + std::to_string(i) + ".png";
        futures.push_back(loader.loadAsync(i + 1, path, BenchmarkTextureAsset::ASSET_TYPE_ID, assets[i], params));
    }
    
    uint64_t total_bytes = 0;
    for (auto& future : futures) {
        auto result = future.get();
        total_bytes += result.bytes_loaded;
    }
    
    timer.stop();
    
    BenchmarkResult result;
    result.name = "Priority Loading";
    result.time_ms = timer.getMilliseconds();
    result.operations = NUM_ASSETS;
    result.bytes_processed = total_bytes;
    result.calculate();
    
    return result;
}

// =============================================================================
// Memory Management Benchmarks
// =============================================================================

BenchmarkResult benchmarkMemoryAllocation() {
    std::cout << "Running memory allocation benchmark...\n";
    
    const int NUM_ALLOCATIONS = 1000;
    std::vector<std::unique_ptr<BenchmarkTextureAsset>> assets;
    
    BenchmarkTimer timer;
    timer.start();
    
    uint64_t total_bytes = 0;
    for (int i = 0; i < NUM_ALLOCATIONS; ++i) {
        auto asset = std::make_unique<BenchmarkTextureAsset>();
        AssetLoadParams params;
        asset->load("mem_benchmark_" + std::to_string(i) + ".png", params);
        total_bytes += asset->getMemoryUsage();
        assets.push_back(std::move(asset));
    }
    
    timer.stop();
    
    BenchmarkResult result;
    result.name = "Memory Allocation";
    result.time_ms = timer.getMilliseconds();
    result.operations = NUM_ALLOCATIONS;
    result.bytes_processed = total_bytes;
    result.calculate();
    
    return result;
}

BenchmarkResult benchmarkMemoryDeallocation() {
    std::cout << "Running memory deallocation benchmark...\n";
    
    const int NUM_ASSETS = 1000;
    std::vector<std::unique_ptr<BenchmarkTextureAsset>> assets;
    uint64_t total_bytes = 0;
    
    // First allocate all assets
    for (int i = 0; i < NUM_ASSETS; ++i) {
        auto asset = std::make_unique<BenchmarkTextureAsset>();
        AssetLoadParams params;
        asset->load("dealloc_benchmark_" + std::to_string(i) + ".png", params);
        total_bytes += asset->getMemoryUsage();
        assets.push_back(std::move(asset));
    }
    
    BenchmarkTimer timer;
    timer.start();
    
    // Now deallocate all
    for (auto& asset : assets) {
        asset->unload();
    }
    assets.clear();
    
    timer.stop();
    
    BenchmarkResult result;
    result.name = "Memory Deallocation";
    result.time_ms = timer.getMilliseconds();
    result.operations = NUM_ASSETS;
    result.bytes_processed = total_bytes;
    result.calculate();
    
    return result;
}

// =============================================================================
// Asset Processing Benchmarks  
// =============================================================================

BenchmarkResult benchmarkTextureProcessing() {
    std::cout << "Running texture processing benchmark...\n";
    
    const int NUM_TEXTURES = 50;
    TextureProcessor processor;
    
    BenchmarkTimer timer;
    timer.start();
    
    uint64_t total_bytes = 0;
    for (int i = 0; i < NUM_TEXTURES; ++i) {
        // Create dummy texture data
        TextureData texture_data;
        texture_data.width = 512;
        texture_data.height = 512;
        texture_data.format = TextureFormat::RGBA8;
        texture_data.data.resize(512 * 512 * 4);
        
        // Process for different quality levels
        auto processed = processor.processForQuality(texture_data, AssetQuality::High);
        if (processed) {
            total_bytes += processed->getTotalSize();
        }
    }
    
    timer.stop();
    
    BenchmarkResult result;
    result.name = "Texture Processing";
    result.time_ms = timer.getMilliseconds();
    result.operations = NUM_TEXTURES;
    result.bytes_processed = total_bytes;
    result.calculate();
    
    return result;
}

// =============================================================================
// Cache Performance Benchmarks
// =============================================================================

BenchmarkResult benchmarkCachePerformance() {
    std::cout << "Running cache performance benchmark...\n";
    
    const int NUM_OPERATIONS = 10000;
    const int NUM_UNIQUE_ASSETS = 100;
    
    // Simulate cache operations
    std::unordered_map<AssetID, std::vector<uint8_t>> cache;
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(1, NUM_UNIQUE_ASSETS);
    
    BenchmarkTimer timer;
    timer.start();
    
    uint64_t hits = 0;
    uint64_t misses = 0;
    uint64_t total_bytes = 0;
    
    for (int i = 0; i < NUM_OPERATIONS; ++i) {
        AssetID id = dis(gen);
        
        auto it = cache.find(id);
        if (it != cache.end()) {
            // Cache hit
            hits++;
            total_bytes += it->second.size();
        } else {
            // Cache miss - simulate loading
            misses++;
            std::vector<uint8_t> data(1024 * 1024); // 1MB asset
            cache[id] = std::move(data);
            total_bytes += 1024 * 1024;
        }
    }
    
    timer.stop();
    
    std::cout << "Cache stats: " << hits << " hits, " << misses << " misses, " 
              << std::fixed << std::setprecision(2) << (100.0 * hits / NUM_OPERATIONS) << "% hit rate\n";
    
    BenchmarkResult result;
    result.name = "Cache Performance";
    result.time_ms = timer.getMilliseconds();
    result.operations = NUM_OPERATIONS;
    result.bytes_processed = total_bytes;
    result.calculate();
    
    return result;
}

// =============================================================================
// Hot Reload Benchmarks
// =============================================================================

BenchmarkResult benchmarkHotReloadDetection() {
    std::cout << "Running hot reload detection benchmark...\n";
    
    const int NUM_CHANGES = 1000;
    HotReloadManager hot_reload;
    
    std::atomic<int> reload_count{0};
    hot_reload.setReloadCallback([&reload_count](AssetID, const std::string&) {
        reload_count++;
    });
    
    // Register assets
    for (int i = 1; i <= NUM_CHANGES; ++i) {
        hot_reload.registerAsset(i, "benchmark_asset_" + std::to_string(i) + ".png");
    }
    
    BenchmarkTimer timer;
    timer.start();
    
    // Trigger reloads
    for (int i = 1; i <= NUM_CHANGES; ++i) {
        hot_reload.triggerReload(i);
    }
    
    // Wait for all reloads to process
    while (reload_count < NUM_CHANGES) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    
    timer.stop();
    
    BenchmarkResult result;
    result.name = "Hot Reload Detection";
    result.time_ms = timer.getMilliseconds();
    result.operations = NUM_CHANGES;
    result.bytes_processed = 0; // No actual bytes processed
    result.calculate();
    
    return result;
}

// =============================================================================
// Scalability Benchmarks
// =============================================================================

BenchmarkResult benchmarkLargeAssetCatalog() {
    std::cout << "Running large asset catalog benchmark...\n";
    
    const int CATALOG_SIZE = 10000;
    AssetReferenceManager ref_manager;
    
    BenchmarkTimer timer;
    timer.start();
    
    // Simulate large asset catalog operations
    for (AssetID id = 1; id <= CATALOG_SIZE; ++id) {
        ref_manager.addReference(id);
        ref_manager.recordAccess(id);
        
        if (id % 100 == 0) {
            // Occasionally remove references
            ref_manager.removeReference(id);
        }
    }
    
    // Query operations
    auto unload_candidates = ref_manager.getUnloadCandidates();
    auto lru_candidates = ref_manager.getLeastRecentlyUsed(100);
    
    timer.stop();
    
    std::cout << "Catalog stats: " << CATALOG_SIZE << " assets, " 
              << unload_candidates.size() << " unload candidates, "
              << lru_candidates.size() << " LRU candidates\n";
    
    BenchmarkResult result;
    result.name = "Large Asset Catalog";
    result.time_ms = timer.getMilliseconds();
    result.operations = CATALOG_SIZE;
    result.bytes_processed = 0;
    result.calculate();
    
    return result;
}

// =============================================================================
// Main Benchmark Runner
// =============================================================================

int main() {
    std::cout << "ECScope Asset Pipeline Professional Benchmarks\n";
    std::cout << "==============================================\n";
    std::cout << "Hardware: " << std::thread::hardware_concurrency() << " CPU cores\n";
    std::cout << "Compiler: " << __VERSION__ << "\n";
    std::cout << "Build: " << (__OPTIMIZE__ ? "Optimized" : "Debug") << "\n\n";
    
    BenchmarkSuite suite;
    
    try {
        // Register benchmark asset type
        auto& registry = AssetTypeRegistry::instance();
        registry.registerType(
            BenchmarkTextureAsset::ASSET_TYPE_ID,
            "BenchmarkTexture",
            []() { return std::make_unique<BenchmarkTextureAsset>(); },
            [](const std::string& path, Asset& asset, const AssetLoadParams& params) {
                return static_cast<BenchmarkTextureAsset&>(asset).load(path, params);
            }
        );
        
        // Run loading benchmarks
        suite.addResult(benchmarkSingleThreadedLoading());
        suite.addResult(benchmarkMultiThreadedLoading());
        suite.addResult(benchmarkPriorityLoading());
        
        // Run memory benchmarks
        suite.addResult(benchmarkMemoryAllocation());
        suite.addResult(benchmarkMemoryDeallocation());
        
        // Run processing benchmarks
        suite.addResult(benchmarkTextureProcessing());
        
        // Run system benchmarks
        suite.addResult(benchmarkCachePerformance());
        suite.addResult(benchmarkHotReloadDetection());
        suite.addResult(benchmarkLargeAssetCatalog());
        
        // Display results
        suite.printResults();
        
        // Save to file
        suite.saveToFile("asset_benchmark_results.csv");
        std::cout << "\nResults saved to: asset_benchmark_results.csv\n";
        
    } catch (const std::exception& e) {
        std::cerr << "Benchmark failed: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}