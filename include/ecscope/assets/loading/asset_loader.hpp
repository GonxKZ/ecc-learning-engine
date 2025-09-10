#pragma once

#include "../core/asset_types.hpp"
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <future>
#include <functional>
#include <atomic>
#include <fstream>

namespace ecscope::assets {

// =============================================================================
// Asset Load Request
// =============================================================================

struct AssetLoadRequest {
    AssetID asset_id;
    std::string path;
    AssetTypeID type_id;
    AssetLoadParams params;
    std::shared_ptr<Asset> asset;
    std::promise<AssetLoadResult> promise;
    std::chrono::steady_clock::time_point requested_time;
    
    AssetLoadRequest(AssetID id, const std::string& p, AssetTypeID tid, 
                    const AssetLoadParams& par, std::shared_ptr<Asset> a)
        : asset_id(id), path(p), type_id(tid), params(par), asset(std::move(a))
        , requested_time(std::chrono::steady_clock::now()) {}
    
    // Priority comparison for priority queue
    bool operator<(const AssetLoadRequest& other) const {
        if (params.priority != other.params.priority) {
            return params.priority > other.params.priority; // Higher priority = smaller value
        }
        return requested_time > other.requested_time; // Earlier requests first
    }
};

// =============================================================================
// Memory-Mapped File Reader
// =============================================================================

class MemoryMappedFile {
public:
    MemoryMappedFile() : data_(nullptr), size_(0) {}
    ~MemoryMappedFile() { close(); }
    
    // Non-copyable, movable
    MemoryMappedFile(const MemoryMappedFile&) = delete;
    MemoryMappedFile& operator=(const MemoryMappedFile&) = delete;
    
    MemoryMappedFile(MemoryMappedFile&& other) noexcept
        : data_(other.data_), size_(other.size_), file_handle_(other.file_handle_) {
        other.data_ = nullptr;
        other.size_ = 0;
        other.file_handle_ = nullptr;
    }
    
    bool open(const std::string& path);
    void close();
    
    const uint8_t* data() const { return data_; }
    size_t size() const { return size_; }
    bool isOpen() const { return data_ != nullptr; }
    
private:
    uint8_t* data_;
    size_t size_;
    void* file_handle_; // Platform-specific handle
};

// =============================================================================
// Asset Compression Support
// =============================================================================

class CompressionEngine {
public:
    virtual ~CompressionEngine() = default;
    
    virtual std::vector<uint8_t> compress(const uint8_t* data, size_t size) = 0;
    virtual std::vector<uint8_t> decompress(const uint8_t* data, size_t size, size_t expected_size = 0) = 0;
    virtual CompressionType getType() const = 0;
    virtual size_t getMaxCompressedSize(size_t input_size) const = 0;
};

class LZ4CompressionEngine : public CompressionEngine {
public:
    std::vector<uint8_t> compress(const uint8_t* data, size_t size) override;
    std::vector<uint8_t> decompress(const uint8_t* data, size_t size, size_t expected_size = 0) override;
    CompressionType getType() const override { return CompressionType::LZ4; }
    size_t getMaxCompressedSize(size_t input_size) const override;
};

class ZstdCompressionEngine : public CompressionEngine {
public:
    ZstdCompressionEngine(int compression_level = 3);
    
    std::vector<uint8_t> compress(const uint8_t* data, size_t size) override;
    std::vector<uint8_t> decompress(const uint8_t* data, size_t size, size_t expected_size = 0) override;
    CompressionType getType() const override { return CompressionType::Zstd; }
    size_t getMaxCompressedSize(size_t input_size) const override;
    
private:
    int compression_level_;
};

// =============================================================================
// Asset Streaming Support
// =============================================================================

class StreamingAsset {
public:
    virtual ~StreamingAsset() = default;
    
    // Stream management
    virtual bool startStreaming(AssetQuality quality) = 0;
    virtual void stopStreaming() = 0;
    virtual bool isStreaming() const = 0;
    
    // LOD management
    virtual void setTargetQuality(AssetQuality quality) = 0;
    virtual AssetQuality getCurrentQuality() const = 0;
    virtual AssetQuality getTargetQuality() const = 0;
    
    // Streaming progress
    virtual float getStreamingProgress() const = 0;
    virtual uint64_t getStreamedBytes() const = 0;
    virtual uint64_t getTotalBytes() const = 0;
};

// =============================================================================
// Asset Loader - Multi-threaded asset loading system
// =============================================================================

class AssetLoader {
public:
    AssetLoader(size_t num_threads = std::thread::hardware_concurrency());
    ~AssetLoader();
    
    // Non-copyable, non-movable
    AssetLoader(const AssetLoader&) = delete;
    AssetLoader& operator=(const AssetLoader&) = delete;
    
    // Loading interface
    std::future<AssetLoadResult> loadAsync(AssetID asset_id, const std::string& path, 
                                          AssetTypeID type_id, std::shared_ptr<Asset> asset,
                                          const AssetLoadParams& params = {});
    
    AssetLoadResult loadSync(const std::string& path, AssetTypeID type_id, 
                           Asset& asset, const AssetLoadParams& params = {});
    
    // Queue management
    void cancelLoad(AssetID asset_id);
    void cancelAllLoads();
    void setPaused(bool paused);
    bool isPaused() const { return paused_; }
    
    // Priority queue management
    void updatePriority(AssetID asset_id, AssetPriority new_priority);
    size_t getQueueSize() const;
    
    // Statistics
    AssetStats getStatistics() const;
    void resetStatistics();
    
    // Compression support
    void registerCompressionEngine(std::unique_ptr<CompressionEngine> engine);
    CompressionEngine* getCompressionEngine(CompressionType type);
    
    // Memory budget
    void setMemoryBudget(uint64_t bytes) { memory_budget_ = bytes; }
    uint64_t getMemoryBudget() const { return memory_budget_; }
    uint64_t getMemoryUsed() const { return memory_used_; }
    
    // Streaming support
    void setStreamingEnabled(bool enabled) { streaming_enabled_ = enabled; }
    bool isStreamingEnabled() const { return streaming_enabled_; }
    
private:
    void workerThread(size_t thread_id);
    AssetLoadResult processLoadRequest(const AssetLoadRequest& request);
    
    bool loadFromFile(const std::string& path, Asset& asset, const AssetLoadParams& params);
    bool loadFromMemoryMappedFile(const std::string& path, Asset& asset, const AssetLoadParams& params);
    bool loadFromCompressedFile(const std::string& path, Asset& asset, const AssetLoadParams& params);
    
    void updateStatistics(const AssetLoadResult& result, std::chrono::milliseconds load_time);
    
    // Thread management
    std::vector<std::thread> worker_threads_;
    std::atomic<bool> shutdown_;
    std::atomic<bool> paused_;
    
    // Request queue
    std::priority_queue<AssetLoadRequest> load_queue_;
    mutable std::mutex queue_mutex_;
    std::condition_variable queue_condition_;
    
    // Active requests (for cancellation)
    std::unordered_map<AssetID, std::shared_ptr<std::atomic<bool>>> active_requests_;
    std::mutex active_requests_mutex_;
    
    // Compression engines
    std::unordered_map<CompressionType, std::unique_ptr<CompressionEngine>> compression_engines_;
    
    // Statistics
    mutable std::mutex stats_mutex_;
    std::atomic<uint64_t> total_loads_;
    std::atomic<uint64_t> successful_loads_;
    std::atomic<uint64_t> failed_loads_;
    std::atomic<uint64_t> bytes_loaded_;
    std::atomic<uint64_t> total_load_time_ms_;
    
    // Memory management
    std::atomic<uint64_t> memory_budget_;
    std::atomic<uint64_t> memory_used_;
    
    // Streaming
    std::atomic<bool> streaming_enabled_;
    
    // File I/O optimization
    static constexpr size_t MMAP_THRESHOLD = 64 * 1024; // 64KB threshold for memory mapping
    static constexpr size_t READ_BUFFER_SIZE = 8 * 1024; // 8KB read buffer
};

// =============================================================================
// Asset Bundle Loader
// =============================================================================

class AssetBundleLoader {
public:
    AssetBundleLoader(AssetLoader& asset_loader);
    
    // Bundle loading
    std::future<bool> loadBundleAsync(const std::string& bundle_path);
    bool loadBundleSync(const std::string& bundle_path);
    
    // Bundle management
    void unloadBundle(const std::string& bundle_name);
    bool isBundleLoaded(const std::string& bundle_name) const;
    
    // Bundle information
    std::vector<std::string> getLoadedBundles() const;
    AssetBundleInfo getBundleInfo(const std::string& bundle_name) const;
    
private:
    struct BundleData {
        AssetBundleInfo info;
        std::vector<uint8_t> data;
        bool loaded = false;
    };
    
    bool loadBundleInternal(const std::string& bundle_path);
    bool extractAssetsFromBundle(const BundleData& bundle);
    
    AssetLoader& asset_loader_;
    std::unordered_map<std::string, BundleData> loaded_bundles_;
    mutable std::mutex bundles_mutex_;
};

} // namespace ecscope::assets