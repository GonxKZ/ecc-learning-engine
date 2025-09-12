#pragma once

#include "../core/types.hpp"
#include <memory>
#include <unordered_map>
#include <vector>
#include <functional>
#include <chrono>
#include <mutex>
#include <atomic>
#include <optional>
#include <future>

namespace ecscope::gui::cache {

// Forward declarations
class CacheManager;
class AssetCache;
class LayoutCache;
class RenderCache;

// Cache entry metadata
template<typename T>
struct CacheEntry {
    T data;
    std::chrono::steady_clock::time_point last_access;
    std::chrono::steady_clock::time_point creation_time;
    size_t access_count = 0;
    size_t memory_size = 0;
    uint32_t version = 0;
    bool is_dirty = false;
    bool is_persistent = false;
};

// Cache statistics
struct CacheStats {
    size_t total_entries = 0;
    size_t memory_used = 0;
    size_t hit_count = 0;
    size_t miss_count = 0;
    size_t eviction_count = 0;
    float hit_rate = 0.0f;
    float avg_access_time_ms = 0.0f;
    std::chrono::steady_clock::time_point last_cleanup;
};

// Eviction policies
enum class EvictionPolicy {
    LRU,        // Least Recently Used
    LFU,        // Least Frequently Used
    FIFO,       // First In First Out
    SIZE,       // Largest items first
    TTL,        // Time To Live based
    ADAPTIVE    // Adaptive based on access patterns
};

// Multi-level cache with different storage tiers
template<typename Key, typename Value>
class MultiLevelCache {
public:
    struct Level {
        size_t max_entries;
        size_t max_memory;
        std::chrono::milliseconds ttl;
        EvictionPolicy policy;
    };
    
    MultiLevelCache(const std::vector<Level>& levels)
        : m_levels(levels) {
        m_caches.resize(levels.size());
    }
    
    std::optional<Value> Get(const Key& key) {
        auto start = std::chrono::high_resolution_clock::now();
        
        // Check each level
        for (size_t level = 0; level < m_caches.size(); ++level) {
            auto it = m_caches[level].find(key);
            if (it != m_caches[level].end()) {
                auto& entry = it->second;
                
                // Check TTL
                if (m_levels[level].ttl.count() > 0) {
                    auto age = std::chrono::steady_clock::now() - entry.creation_time;
                    if (age > m_levels[level].ttl) {
                        m_caches[level].erase(it);
                        continue;
                    }
                }
                
                // Update access info
                entry.last_access = std::chrono::steady_clock::now();
                entry.access_count++;
                
                // Promote to higher level if frequently accessed
                if (level > 0 && ShouldPromote(entry)) {
                    Promote(key, entry, level);
                }
                
                UpdateStats(true, start);
                return entry.data;
            }
        }
        
        UpdateStats(false, start);
        return std::nullopt;
    }
    
    void Put(const Key& key, const Value& value, size_t memory_size = sizeof(Value)) {
        // Add to L1 cache
        PutAtLevel(key, value, 0, memory_size);
    }
    
    void Invalidate(const Key& key) {
        for (auto& cache : m_caches) {
            cache.erase(key);
        }
    }
    
    void Clear() {
        for (auto& cache : m_caches) {
            cache.clear();
        }
        m_stats = CacheStats{};
    }
    
    const CacheStats& GetStats() const { return m_stats; }
    
    void SetEvictionPolicy(size_t level, EvictionPolicy policy) {
        if (level < m_levels.size()) {
            m_levels[level].policy = policy;
        }
    }
    
private:
    void PutAtLevel(const Key& key, const Value& value, size_t level, size_t memory_size) {
        if (level >= m_caches.size()) return;
        
        auto& cache = m_caches[level];
        auto& level_config = m_levels[level];
        
        // Check if we need to evict
        while (cache.size() >= level_config.max_entries ||
               GetLevelMemory(level) + memory_size > level_config.max_memory) {
            EvictFromLevel(level);
        }
        
        // Add entry
        CacheEntry<Value> entry;
        entry.data = value;
        entry.last_access = std::chrono::steady_clock::now();
        entry.creation_time = entry.last_access;
        entry.memory_size = memory_size;
        
        cache[key] = std::move(entry);
    }
    
    void EvictFromLevel(size_t level) {
        auto& cache = m_caches[level];
        if (cache.empty()) return;
        
        Key evict_key;
        
        switch (m_levels[level].policy) {
            case EvictionPolicy::LRU:
                evict_key = FindLRU(cache);
                break;
            case EvictionPolicy::LFU:
                evict_key = FindLFU(cache);
                break;
            case EvictionPolicy::SIZE:
                evict_key = FindLargest(cache);
                break;
            default:
                evict_key = cache.begin()->first;
        }
        
        // Demote to lower level if possible
        if (level + 1 < m_caches.size()) {
            auto& entry = cache[evict_key];
            PutAtLevel(evict_key, entry.data, level + 1, entry.memory_size);
        }
        
        cache.erase(evict_key);
        m_stats.eviction_count++;
    }
    
    bool ShouldPromote(const CacheEntry<Value>& entry) const {
        // Promote if accessed more than threshold
        return entry.access_count > 3;
    }
    
    void Promote(const Key& key, CacheEntry<Value>& entry, size_t from_level) {
        if (from_level == 0) return;
        
        // Move to higher level
        PutAtLevel(key, entry.data, from_level - 1, entry.memory_size);
        m_caches[from_level].erase(key);
    }
    
    size_t GetLevelMemory(size_t level) const {
        size_t total = 0;
        for (const auto& [key, entry] : m_caches[level]) {
            total += entry.memory_size;
        }
        return total;
    }
    
    Key FindLRU(const std::unordered_map<Key, CacheEntry<Value>>& cache) const {
        auto oldest = cache.begin();
        for (auto it = cache.begin(); it != cache.end(); ++it) {
            if (it->second.last_access < oldest->second.last_access) {
                oldest = it;
            }
        }
        return oldest->first;
    }
    
    Key FindLFU(const std::unordered_map<Key, CacheEntry<Value>>& cache) const {
        auto least = cache.begin();
        for (auto it = cache.begin(); it != cache.end(); ++it) {
            if (it->second.access_count < least->second.access_count) {
                least = it;
            }
        }
        return least->first;
    }
    
    Key FindLargest(const std::unordered_map<Key, CacheEntry<Value>>& cache) const {
        auto largest = cache.begin();
        for (auto it = cache.begin(); it != cache.end(); ++it) {
            if (it->second.memory_size > largest->second.memory_size) {
                largest = it;
            }
        }
        return largest->first;
    }
    
    void UpdateStats(bool hit, std::chrono::high_resolution_clock::time_point start) {
        if (hit) {
            m_stats.hit_count++;
        } else {
            m_stats.miss_count++;
        }
        
        auto duration = std::chrono::high_resolution_clock::now() - start;
        auto ms = std::chrono::duration_cast<std::chrono::microseconds>(duration).count() / 1000.0f;
        
        // Update average access time
        float total_accesses = m_stats.hit_count + m_stats.miss_count;
        m_stats.avg_access_time_ms = 
            (m_stats.avg_access_time_ms * (total_accesses - 1) + ms) / total_accesses;
        
        m_stats.hit_rate = total_accesses > 0 ? 
            float(m_stats.hit_count) / total_accesses : 0.0f;
    }
    
    std::vector<Level> m_levels;
    std::vector<std::unordered_map<Key, CacheEntry<Value>>> m_caches;
    CacheStats m_stats;
    mutable std::mutex m_mutex;
};

// Lazy asset loader with async loading
template<typename T>
class LazyAsset {
public:
    using LoadFunc = std::function<std::shared_ptr<T>()>;
    using LoadAsyncFunc = std::function<std::future<std::shared_ptr<T>>()>;
    
    enum class LoadState {
        NOT_LOADED,
        LOADING,
        LOADED,
        FAILED
    };
    
    LazyAsset(LoadFunc loader) 
        : m_loader(loader), m_state(LoadState::NOT_LOADED) {}
    
    LazyAsset(LoadAsyncFunc async_loader)
        : m_async_loader(async_loader), m_state(LoadState::NOT_LOADED) {}
    
    std::shared_ptr<T> Get() {
        if (m_state == LoadState::LOADED) {
            return m_asset;
        }
        
        if (m_state == LoadState::NOT_LOADED) {
            Load();
        }
        
        if (m_state == LoadState::LOADING && m_future.valid()) {
            // Wait for async load
            if (m_future.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready) {
                m_asset = m_future.get();
                m_state = m_asset ? LoadState::LOADED : LoadState::FAILED;
            }
        }
        
        return m_asset;
    }
    
    std::shared_ptr<T> GetAsync() {
        if (m_state == LoadState::NOT_LOADED) {
            LoadAsync();
        }
        return m_asset;
    }
    
    bool IsLoaded() const { return m_state == LoadState::LOADED; }
    bool IsLoading() const { return m_state == LoadState::LOADING; }
    LoadState GetState() const { return m_state; }
    
    void Unload() {
        m_asset.reset();
        m_state = LoadState::NOT_LOADED;
    }
    
    void Reload() {
        Unload();
        Load();
    }
    
private:
    void Load() {
        if (m_loader) {
            m_state = LoadState::LOADING;
            m_asset = m_loader();
            m_state = m_asset ? LoadState::LOADED : LoadState::FAILED;
        } else if (m_async_loader) {
            LoadAsync();
        }
    }
    
    void LoadAsync() {
        if (m_async_loader) {
            m_state = LoadState::LOADING;
            m_future = m_async_loader();
        }
    }
    
    LoadFunc m_loader;
    LoadAsyncFunc m_async_loader;
    std::shared_ptr<T> m_asset;
    std::future<std::shared_ptr<T>> m_future;
    std::atomic<LoadState> m_state;
};

// Layout cache for UI element layouts
class LayoutCache {
public:
    struct LayoutKey {
        uint32_t element_id;
        float container_width;
        float container_height;
        uint32_t constraints_hash;
        
        bool operator==(const LayoutKey& other) const {
            return element_id == other.element_id &&
                   std::abs(container_width - other.container_width) < 0.01f &&
                   std::abs(container_height - other.container_height) < 0.01f &&
                   constraints_hash == other.constraints_hash;
        }
    };
    
    struct LayoutResult {
        float x, y, width, height;
        std::vector<LayoutResult> children;
    };
    
    struct LayoutKeyHash {
        size_t operator()(const LayoutKey& key) const {
            size_t h1 = std::hash<uint32_t>()(key.element_id);
            size_t h2 = std::hash<float>()(key.container_width);
            size_t h3 = std::hash<float>()(key.container_height);
            size_t h4 = std::hash<uint32_t>()(key.constraints_hash);
            return h1 ^ (h2 << 1) ^ (h3 << 2) ^ (h4 << 3);
        }
    };
    
    LayoutCache(size_t max_entries = 10000);
    
    std::optional<LayoutResult> Get(const LayoutKey& key);
    void Put(const LayoutKey& key, const LayoutResult& result);
    void InvalidateElement(uint32_t element_id);
    void Clear();
    
    const CacheStats& GetStats() const { return m_cache.GetStats(); }
    
private:
    MultiLevelCache<LayoutKey, LayoutResult> m_cache;
};

// Render cache for computed render data
class RenderCache {
public:
    struct RenderData {
        std::vector<float> vertices;
        std::vector<uint16_t> indices;
        uint32_t texture_id;
        uint32_t shader_id;
        float transform[16];
    };
    
    RenderCache(size_t max_memory_mb = 128);
    
    std::shared_ptr<RenderData> GetOrCompute(
        uint32_t element_id,
        uint32_t version,
        std::function<std::shared_ptr<RenderData>()> compute);
    
    void Invalidate(uint32_t element_id);
    void SetMaxMemory(size_t memory_mb);
    void Compact();
    
private:
    struct CacheKey {
        uint32_t element_id;
        uint32_t version;
        
        bool operator==(const CacheKey& other) const {
            return element_id == other.element_id && version == other.version;
        }
    };
    
    struct CacheKeyHash {
        size_t operator()(const CacheKey& key) const {
            return std::hash<uint64_t>()((uint64_t(key.element_id) << 32) | key.version);
        }
    };
    
    std::unordered_map<CacheKey, std::shared_ptr<RenderData>, CacheKeyHash> m_cache;
    size_t m_max_memory;
    std::atomic<size_t> m_current_memory{0};
    mutable std::mutex m_mutex;
};

// Font glyph cache
class GlyphCache {
public:
    struct Glyph {
        uint32_t texture_id;
        float u0, v0, u1, v1;
        float advance;
        float bearing_x, bearing_y;
        float width, height;
    };
    
    GlyphCache(size_t atlas_size = 2048);
    
    const Glyph* GetGlyph(uint32_t font_id, uint32_t codepoint, float size);
    void PreloadGlyphs(uint32_t font_id, const std::vector<uint32_t>& codepoints, float size);
    
    void ClearFont(uint32_t font_id);
    void Clear();
    
private:
    struct GlyphKey {
        uint32_t font_id;
        uint32_t codepoint;
        uint16_t size; // Fixed point
        
        bool operator==(const GlyphKey& other) const {
            return font_id == other.font_id && 
                   codepoint == other.codepoint && 
                   size == other.size;
        }
    };
    
    struct GlyphKeyHash {
        size_t operator()(const GlyphKey& key) const {
            size_t h1 = std::hash<uint32_t>()(key.font_id);
            size_t h2 = std::hash<uint32_t>()(key.codepoint);
            size_t h3 = std::hash<uint16_t>()(key.size);
            return h1 ^ (h2 << 1) ^ (h3 << 2);
        }
    };
    
    std::unordered_map<GlyphKey, Glyph, GlyphKeyHash> m_glyphs;
    uint32_t m_atlas_texture;
    size_t m_atlas_size;
    mutable std::mutex m_mutex;
};

// Computed style cache
class StyleCache {
public:
    struct ComputedStyle {
        // Layout
        float x, y, width, height;
        float padding[4];
        float margin[4];
        
        // Visual
        uint32_t background_color;
        uint32_t border_color;
        float border_width;
        float border_radius;
        
        // Text
        uint32_t font_id;
        float font_size;
        uint32_t text_color;
        uint32_t text_align;
        
        // Effects
        float opacity;
        float transform[16];
        uint32_t filter;
    };
    
    StyleCache(size_t max_entries = 10000);
    
    std::shared_ptr<ComputedStyle> GetOrCompute(
        uint32_t element_id,
        uint32_t parent_style_version,
        std::function<std::shared_ptr<ComputedStyle>()> compute);
    
    void InvalidateElement(uint32_t element_id);
    void InvalidateAll();
    
private:
    struct StyleKey {
        uint32_t element_id;
        uint32_t parent_version;
        
        bool operator==(const StyleKey& other) const {
            return element_id == other.element_id && 
                   parent_version == other.parent_version;
        }
    };
    
    struct StyleKeyHash {
        size_t operator()(const StyleKey& key) const {
            return std::hash<uint64_t>()((uint64_t(key.element_id) << 32) | key.parent_version);
        }
    };
    
    std::unordered_map<StyleKey, std::shared_ptr<ComputedStyle>, StyleKeyHash> m_cache;
    size_t m_max_entries;
    mutable std::mutex m_mutex;
};

// Global cache manager
class CacheManager {
public:
    static CacheManager& Instance();
    
    LayoutCache& GetLayoutCache() { return *m_layout_cache; }
    RenderCache& GetRenderCache() { return *m_render_cache; }
    GlyphCache& GetGlyphCache() { return *m_glyph_cache; }
    StyleCache& GetStyleCache() { return *m_style_cache; }
    
    // Cache control
    void SetMemoryBudget(size_t total_mb);
    void ClearAll();
    void Compact();
    
    // Performance monitoring
    void EnableProfiling(bool enable) { m_profiling_enabled = enable; }
    CacheStats GetGlobalStats() const;
    
    // Cache warming
    void WarmCache(const std::vector<uint32_t>& element_ids);
    
private:
    CacheManager();
    
    std::unique_ptr<LayoutCache> m_layout_cache;
    std::unique_ptr<RenderCache> m_render_cache;
    std::unique_ptr<GlyphCache> m_glyph_cache;
    std::unique_ptr<StyleCache> m_style_cache;
    
    std::atomic<bool> m_profiling_enabled{false};
    std::atomic<size_t> m_memory_budget{512 * 1024 * 1024}; // 512MB default
};

} // namespace ecscope::gui::cache