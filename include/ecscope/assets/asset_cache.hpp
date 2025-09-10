#pragma once

#include "asset.hpp"
#include <unordered_map>
#include <list>
#include <mutex>
#include <atomic>
#include <fstream>

namespace ecscope::assets {

    // Cache eviction policies
    enum class EvictionPolicy {
        LRU,    // Least Recently Used
        LFU,    // Least Frequently Used
        FIFO,   // First In, First Out
        RANDOM, // Random eviction
        SIZE    // Evict largest assets first
    };

    // Cache statistics
    struct CacheStatistics {
        std::atomic<uint64_t> hits{0};
        std::atomic<uint64_t> misses{0};
        std::atomic<uint64_t> evictions{0};
        std::atomic<uint64_t> insertions{0};
        std::atomic<uint64_t> bytes_stored{0};
        std::atomic<uint64_t> bytes_evicted{0};
        
        double get_hit_rate() const {
            uint64_t total = hits + misses;
            return total > 0 ? static_cast<double>(hits) / total : 0.0;
        }
        
        void reset() {
            hits = 0;
            misses = 0;
            evictions = 0;
            insertions = 0;
            bytes_stored = 0;
            bytes_evicted = 0;
        }
    };

    // Cache entry
    struct CacheEntry {
        AssetID id;
        std::vector<uint8_t> data;
        size_t size;
        std::chrono::steady_clock::time_point last_access;
        std::chrono::steady_clock::time_point creation_time;
        uint32_t access_count;
        AssetType type;
        bool is_compressed;
        
        CacheEntry() : id(INVALID_ASSET_ID), size(0), access_count(0), 
                      type(AssetType::UNKNOWN), is_compressed(false) {}
        
        CacheEntry(AssetID asset_id, std::vector<uint8_t> asset_data, AssetType asset_type)
            : id(asset_id), data(std::move(asset_data)), size(data.size()), 
              last_access(std::chrono::steady_clock::now()),
              creation_time(std::chrono::steady_clock::now()),
              access_count(1), type(asset_type), is_compressed(false) {}
    };

    // Memory cache for frequently accessed assets
    class AssetCache {
    public:
        explicit AssetCache(size_t max_size_bytes = 128 * 1024 * 1024, // 128MB default
                           EvictionPolicy policy = EvictionPolicy::LRU);
        ~AssetCache();
        
        // Cache operations
        bool put(AssetID id, const std::vector<uint8_t>& data, AssetType type);
        bool put_compressed(AssetID id, const std::vector<uint8_t>& compressed_data, AssetType type);
        bool get(AssetID id, std::vector<uint8_t>& data);
        bool has(AssetID id) const;
        bool remove(AssetID id);
        void clear();
        
        // Cache management
        void set_max_size(size_t max_size_bytes);
        size_t get_max_size() const { return m_max_size_bytes; }
        size_t get_current_size() const { return m_current_size_bytes.load(); }
        size_t get_entry_count() const;
        
        // Eviction policy
        void set_eviction_policy(EvictionPolicy policy) { m_eviction_policy = policy; }
        EvictionPolicy get_eviction_policy() const { return m_eviction_policy; }
        
        // Cache warming and preloading
        void warm_cache(const std::vector<AssetID>& asset_ids);
        void preload_from_disk();
        
        // Statistics
        const CacheStatistics& get_statistics() const { return m_statistics; }
        void reset_statistics() { m_statistics.reset(); }
        
        // Cache persistence
        bool save_to_disk(const std::string& cache_file) const;
        bool load_from_disk(const std::string& cache_file);
        void set_persistent_cache_path(const std::string& path) { m_persistent_cache_path = path; }
        
        // Memory management
        void trim_to_size(size_t target_size);
        void collect_garbage();
        std::vector<AssetID> get_cached_assets() const;
        
        // Configuration
        void set_compression_enabled(bool enabled) { m_compression_enabled = enabled; }
        bool is_compression_enabled() const { return m_compression_enabled; }
        
        void set_auto_eviction_enabled(bool enabled) { m_auto_eviction_enabled = enabled; }
        bool is_auto_eviction_enabled() const { return m_auto_eviction_enabled; }
        
        // Debugging
        void dump_cache_info() const;
        std::vector<std::pair<AssetID, size_t>> get_entries_by_size() const;
        std::vector<std::pair<AssetID, uint32_t>> get_entries_by_access_count() const;
        
    private:
        // Cache storage
        std::unordered_map<AssetID, std::unique_ptr<CacheEntry>> m_entries;
        
        // LRU list for eviction
        std::list<AssetID> m_lru_list;
        std::unordered_map<AssetID, std::list<AssetID>::iterator> m_lru_iterators;
        
        // Configuration
        std::atomic<size_t> m_max_size_bytes;
        std::atomic<size_t> m_current_size_bytes{0};
        EvictionPolicy m_eviction_policy;
        bool m_compression_enabled;
        bool m_auto_eviction_enabled;
        std::string m_persistent_cache_path;
        
        // Statistics
        mutable CacheStatistics m_statistics;
        
        // Thread safety
        mutable std::shared_mutex m_mutex;
        
        // Internal methods
        void evict_if_needed(size_t incoming_size);
        AssetID select_eviction_candidate();
        void evict_entry(AssetID id);
        void update_lru(AssetID id);
        void remove_from_lru(AssetID id);
        
        // Persistence helpers
        struct CacheHeader {
            uint32_t version;
            uint32_t entry_count;
            uint64_t total_size;
            uint32_t checksum;
        };
        
        bool write_cache_header(std::ofstream& file, const CacheHeader& header) const;
        bool read_cache_header(std::ifstream& file, CacheHeader& header) const;
        uint32_t calculate_checksum(const std::vector<uint8_t>& data) const;
    };

    // Disk cache for long-term asset storage
    class DiskCache {
    public:
        explicit DiskCache(const std::string& cache_directory = "cache/");
        ~DiskCache();
        
        // Cache operations
        bool put(AssetID id, const std::vector<uint8_t>& data, AssetType type);
        bool get(AssetID id, std::vector<uint8_t>& data);
        bool has(AssetID id) const;
        bool remove(AssetID id);
        void clear();
        
        // Cache management
        void set_cache_directory(const std::string& directory);
        const std::string& get_cache_directory() const { return m_cache_directory; }
        
        size_t get_cache_size() const;
        size_t get_file_count() const;
        
        // Cache maintenance
        void cleanup_old_files(std::chrono::hours max_age = std::chrono::hours{24 * 7}); // 7 days
        void optimize_cache();
        
        // Validation
        bool validate_cache() const;
        std::vector<AssetID> find_corrupted_files() const;
        
        // Statistics
        struct DiskCacheStats {
            size_t total_files;
            size_t total_size_bytes;
            size_t corrupted_files;
            std::chrono::system_clock::time_point last_cleanup;
        };
        
        DiskCacheStats get_statistics() const;
        
    private:
        std::string m_cache_directory;
        mutable std::mutex m_mutex;
        
        std::string get_cache_file_path(AssetID id) const;
        std::string get_metadata_file_path(AssetID id) const;
        
        struct FileMetadata {
            AssetID id;
            AssetType type;
            size_t size;
            std::chrono::system_clock::time_point creation_time;
            uint32_t checksum;
        };
        
        bool write_metadata(AssetID id, const FileMetadata& metadata) const;
        bool read_metadata(AssetID id, FileMetadata& metadata) const;
        bool ensure_cache_directory() const;
    };

    // Multi-level cache system
    class MultiLevelCache {
    public:
        MultiLevelCache(std::unique_ptr<AssetCache> memory_cache,
                       std::unique_ptr<DiskCache> disk_cache);
        ~MultiLevelCache();
        
        // Cache operations
        bool put(AssetID id, const std::vector<uint8_t>& data, AssetType type);
        bool get(AssetID id, std::vector<uint8_t>& data);
        bool has(AssetID id) const;
        bool remove(AssetID id);
        void clear();
        
        // Level-specific operations
        bool get_from_memory(AssetID id, std::vector<uint8_t>& data);
        bool get_from_disk(AssetID id, std::vector<uint8_t>& data);
        bool promote_to_memory(AssetID id);
        bool demote_to_disk(AssetID id);
        
        // Cache management
        AssetCache* get_memory_cache() { return m_memory_cache.get(); }
        DiskCache* get_disk_cache() { return m_disk_cache.get(); }
        
        // Statistics
        struct MultiLevelStats {
            CacheStatistics memory_stats;
            size_t disk_files;
            size_t disk_size_bytes;
            uint64_t memory_hits;
            uint64_t disk_hits;
            uint64_t total_misses;
        };
        
        MultiLevelStats get_combined_statistics() const;
        void reset_statistics();
        
        // Cache warming
        void warm_memory_cache(const std::vector<AssetID>& priority_assets);
        
    private:
        std::unique_ptr<AssetCache> m_memory_cache;
        std::unique_ptr<DiskCache> m_disk_cache;
        
        mutable std::mutex m_mutex;
        mutable std::atomic<uint64_t> m_memory_hits{0};
        mutable std::atomic<uint64_t> m_disk_hits{0};
        mutable std::atomic<uint64_t> m_total_misses{0};
    };

    // Cache configuration
    struct CacheConfig {
        size_t memory_cache_size_mb = 128;
        EvictionPolicy eviction_policy = EvictionPolicy::LRU;
        bool enable_compression = true;
        bool enable_disk_cache = true;
        std::string disk_cache_directory = "cache/";
        bool enable_persistent_cache = true;
        std::chrono::hours disk_cache_max_age{24 * 7}; // 7 days
    };

    // Cache factory
    std::unique_ptr<MultiLevelCache> create_multi_level_cache(const CacheConfig& config = CacheConfig{});

} // namespace ecscope::assets