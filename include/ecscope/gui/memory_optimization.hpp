#pragma once

#include "../core/types.hpp"
#include "../memory/allocators.hpp"
#include <memory>
#include <atomic>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <functional>
#include <chrono>

namespace ecscope::gui::memory {

// Forward declarations
class MemoryPool;
class ObjectPool;
class StringInterner;
class TextureAtlas;
class ResourceCache;

// Memory allocation strategies
enum class AllocationStrategy {
    IMMEDIATE,      // Allocate immediately
    LAZY,          // Allocate on first use
    PREALLOC,      // Pre-allocate at initialization
    POOL,          // Use memory pool
    STACK,         // Use stack allocator
    RING           // Use ring buffer
};

// Memory priority levels
enum class MemoryPriority {
    CRITICAL,      // Never release (UI framework core)
    HIGH,          // Release only under extreme pressure
    NORMAL,        // Standard priority
    LOW,           // Release under moderate pressure
    CACHE          // Release first (can be regenerated)
};

// Smart memory pool for UI objects
template<typename T>
class UIObjectPool {
public:
    static constexpr size_t DEFAULT_POOL_SIZE = 1024;
    static constexpr size_t GROW_FACTOR = 2;
    
    UIObjectPool(size_t initial_size = DEFAULT_POOL_SIZE)
        : m_pool_size(initial_size) {
        m_pool.reserve(initial_size);
        for (size_t i = 0; i < initial_size; ++i) {
            m_available.push_back(m_pool.emplace_back(std::make_unique<T>()).get());
        }
    }
    
    template<typename... Args>
    T* Acquire(Args&&... args) {
        std::lock_guard<std::mutex> lock(m_mutex);
        
        if (m_available.empty()) {
            Grow();
        }
        
        T* obj = m_available.back();
        m_available.pop_back();
        
        // Reconstruct object in-place
        new (obj) T(std::forward<Args>(args)...);
        
        m_in_use.insert(obj);
        m_acquisitions++;
        
        return obj;
    }
    
    void Release(T* obj) {
        if (!obj) return;
        
        std::lock_guard<std::mutex> lock(m_mutex);
        
        auto it = m_in_use.find(obj);
        if (it != m_in_use.end()) {
            m_in_use.erase(it);
            
            // Call destructor but keep memory
            obj->~T();
            
            m_available.push_back(obj);
            m_releases++;
        }
    }
    
    size_t GetPoolSize() const { return m_pool_size; }
    size_t GetInUseCount() const { return m_in_use.size(); }
    size_t GetAvailableCount() const { return m_available.size(); }
    float GetUtilization() const { 
        return m_pool_size > 0 ? float(m_in_use.size()) / m_pool_size : 0.0f;
    }
    
    void Shrink() {
        std::lock_guard<std::mutex> lock(m_mutex);
        
        // Remove excess available objects
        size_t target_size = m_in_use.size() * GROW_FACTOR;
        while (m_pool.size() > target_size && !m_available.empty()) {
            T* obj = m_available.back();
            m_available.pop_back();
            
            // Find and remove from pool
            auto it = std::find_if(m_pool.begin(), m_pool.end(),
                [obj](const auto& ptr) { return ptr.get() == obj; });
            if (it != m_pool.end()) {
                m_pool.erase(it);
                m_pool_size--;
            }
        }
    }
    
private:
    void Grow() {
        size_t new_size = m_pool_size * GROW_FACTOR;
        m_pool.reserve(new_size);
        
        for (size_t i = m_pool_size; i < new_size; ++i) {
            m_available.push_back(m_pool.emplace_back(std::make_unique<T>()).get());
        }
        
        m_pool_size = new_size;
        m_grows++;
    }
    
    std::vector<std::unique_ptr<T>> m_pool;
    std::vector<T*> m_available;
    std::unordered_set<T*> m_in_use;
    size_t m_pool_size;
    
    // Statistics
    std::atomic<size_t> m_acquisitions{0};
    std::atomic<size_t> m_releases{0};
    std::atomic<size_t> m_grows{0};
    
    mutable std::mutex m_mutex;
};

// String interning for UI text
class StringInterner {
public:
    using StringId = uint32_t;
    
    StringInterner();
    
    StringId Intern(const std::string& str);
    StringId InternView(std::string_view str);
    
    const std::string& GetString(StringId id) const;
    std::string_view GetStringView(StringId id) const;
    
    size_t GetInternedCount() const { return m_strings.size(); }
    size_t GetMemoryUsage() const;
    
    void Clear();
    void Compact(); // Remove unused strings
    
private:
    std::vector<std::string> m_strings;
    std::unordered_map<std::string_view, StringId> m_string_map;
    mutable std::mutex m_mutex;
    
    // Memory tracking
    std::atomic<size_t> m_total_memory{0};
};

// Texture atlas for efficient GPU memory usage
class TextureAtlas {
public:
    struct AtlasRegion {
        uint32_t x, y, width, height;
        uint32_t texture_id;
        float u0, v0, u1, v1; // UV coordinates
    };
    
    TextureAtlas(uint32_t width, uint32_t height, uint32_t format);
    ~TextureAtlas();
    
    AtlasRegion* AddTexture(const void* data, uint32_t width, uint32_t height);
    bool RemoveTexture(AtlasRegion* region);
    
    void Defragment();
    float GetUtilization() const;
    size_t GetMemoryUsage() const;
    
    uint32_t GetTextureID() const { return m_texture_id; }
    
private:
    struct Node {
        uint32_t x, y, width, height;
        bool used;
        std::unique_ptr<Node> left;
        std::unique_ptr<Node> right;
    };
    
    std::unique_ptr<Node> Insert(std::unique_ptr<Node> node, uint32_t width, uint32_t height);
    
    uint32_t m_width, m_height, m_format;
    uint32_t m_texture_id;
    std::unique_ptr<Node> m_root;
    std::vector<std::unique_ptr<AtlasRegion>> m_regions;
    
    mutable std::mutex m_mutex;
};

// Intelligent resource cache with LRU eviction
template<typename Key, typename Resource>
class LRUResourceCache {
public:
    using ResourcePtr = std::shared_ptr<Resource>;
    using LoadFunc = std::function<ResourcePtr(const Key&)>;
    
    LRUResourceCache(size_t max_size, size_t max_memory = 0)
        : m_max_size(max_size), m_max_memory(max_memory) {}
    
    void SetLoader(LoadFunc loader) {
        m_loader = loader;
    }
    
    ResourcePtr Get(const Key& key) {
        std::lock_guard<std::mutex> lock(m_mutex);
        
        auto it = m_cache.find(key);
        if (it != m_cache.end()) {
            // Move to front (most recently used)
            m_lru_list.splice(m_lru_list.begin(), m_lru_list, it->second.lru_it);
            m_hits++;
            return it->second.resource;
        }
        
        m_misses++;
        
        // Load resource
        if (!m_loader) return nullptr;
        
        ResourcePtr resource = m_loader(key);
        if (!resource) return nullptr;
        
        // Add to cache
        Add(key, resource);
        
        return resource;
    }
    
    void Preload(const std::vector<Key>& keys) {
        for (const auto& key : keys) {
            Get(key);
        }
    }
    
    void Clear() {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_cache.clear();
        m_lru_list.clear();
        m_current_memory = 0;
    }
    
    void SetMaxSize(size_t size) { m_max_size = size; }
    void SetMaxMemory(size_t memory) { m_max_memory = memory; }
    
    float GetHitRate() const {
        size_t total = m_hits + m_misses;
        return total > 0 ? float(m_hits) / total : 0.0f;
    }
    
    size_t GetCacheSize() const { return m_cache.size(); }
    size_t GetMemoryUsage() const { return m_current_memory; }
    
private:
    struct CacheEntry {
        ResourcePtr resource;
        typename std::list<Key>::iterator lru_it;
        size_t memory_size;
        std::chrono::steady_clock::time_point last_access;
    };
    
    void Add(const Key& key, ResourcePtr resource) {
        size_t memory_size = GetResourceSize(resource);
        
        // Evict if necessary
        while ((m_cache.size() >= m_max_size) ||
               (m_max_memory > 0 && m_current_memory + memory_size > m_max_memory)) {
            if (m_lru_list.empty()) break;
            
            Evict();
        }
        
        // Add new entry
        m_lru_list.push_front(key);
        m_cache[key] = {
            resource,
            m_lru_list.begin(),
            memory_size,
            std::chrono::steady_clock::now()
        };
        
        m_current_memory += memory_size;
    }
    
    void Evict() {
        if (m_lru_list.empty()) return;
        
        Key key = m_lru_list.back();
        m_lru_list.pop_back();
        
        auto it = m_cache.find(key);
        if (it != m_cache.end()) {
            m_current_memory -= it->second.memory_size;
            m_cache.erase(it);
            m_evictions++;
        }
    }
    
    size_t GetResourceSize(const ResourcePtr& resource) const {
        // Override this for specific resource types
        return sizeof(Resource);
    }
    
    std::unordered_map<Key, CacheEntry> m_cache;
    std::list<Key> m_lru_list;
    LoadFunc m_loader;
    
    size_t m_max_size;
    size_t m_max_memory;
    std::atomic<size_t> m_current_memory{0};
    
    // Statistics
    std::atomic<size_t> m_hits{0};
    std::atomic<size_t> m_misses{0};
    std::atomic<size_t> m_evictions{0};
    
    mutable std::mutex m_mutex;
};

// Memory pressure handler
class MemoryPressureHandler {
public:
    enum class PressureLevel {
        NONE,
        LOW,
        MEDIUM,
        HIGH,
        CRITICAL
    };
    
    using CleanupCallback = std::function<size_t(PressureLevel)>;
    
    void RegisterCleanupCallback(const std::string& name, 
                                 MemoryPriority priority,
                                 CleanupCallback callback);
    
    void UnregisterCleanupCallback(const std::string& name);
    
    size_t HandleMemoryPressure(PressureLevel level);
    
    PressureLevel GetCurrentPressureLevel() const;
    void UpdatePressureLevel();
    
private:
    struct CleanupEntry {
        std::string name;
        MemoryPriority priority;
        CleanupCallback callback;
    };
    
    std::vector<CleanupEntry> m_cleanup_callbacks;
    std::atomic<PressureLevel> m_current_level{PressureLevel::NONE};
    mutable std::mutex m_mutex;
};

// Memory budget tracker
class MemoryBudget {
public:
    struct Budget {
        size_t total_mb = 512;
        size_t ui_mb = 128;
        size_t texture_mb = 256;
        size_t cache_mb = 64;
        size_t buffer_mb = 64;
    };
    
    void SetBudget(const Budget& budget) { m_budget = budget; }
    const Budget& GetBudget() const { return m_budget; }
    
    void TrackAllocation(const std::string& category, size_t bytes);
    void TrackDeallocation(const std::string& category, size_t bytes);
    
    bool IsWithinBudget(const std::string& category) const;
    float GetUtilization(const std::string& category) const;
    
    std::unordered_map<std::string, size_t> GetCurrentUsage() const;
    
private:
    Budget m_budget;
    std::unordered_map<std::string, std::atomic<size_t>> m_usage;
    mutable std::mutex m_mutex;
};

// Lazy loader for deferred resource loading
template<typename T>
class LazyLoader {
public:
    using LoadFunc = std::function<std::unique_ptr<T>()>;
    
    LazyLoader(LoadFunc loader) : m_loader(loader) {}
    
    T* Get() {
        std::call_once(m_load_flag, [this]() {
            m_resource = m_loader();
        });
        return m_resource.get();
    }
    
    const T* Get() const {
        return const_cast<LazyLoader*>(this)->Get();
    }
    
    T* operator->() { return Get(); }
    const T* operator->() const { return Get(); }
    
    T& operator*() { return *Get(); }
    const T& operator*() const { return *Get(); }
    
    bool IsLoaded() const { return m_resource != nullptr; }
    
    void Reset() {
        m_resource.reset();
        m_load_flag = std::once_flag{};
    }
    
private:
    LoadFunc m_loader;
    mutable std::unique_ptr<T> m_resource;
    mutable std::once_flag m_load_flag;
};

// Global memory optimization manager
class MemoryOptimizer {
public:
    static MemoryOptimizer& Instance();
    
    // Pool management
    template<typename T>
    UIObjectPool<T>& GetObjectPool() {
        static UIObjectPool<T> pool;
        return pool;
    }
    
    // String interning
    StringInterner& GetStringInterner() { return *m_string_interner; }
    
    // Memory pressure
    MemoryPressureHandler& GetPressureHandler() { return *m_pressure_handler; }
    
    // Memory budget
    MemoryBudget& GetMemoryBudget() { return *m_memory_budget; }
    
    // Optimization strategies
    void EnableAggressiveCaching(bool enable);
    void EnableMemoryCompaction(bool enable);
    void SetMemoryLimit(size_t limit_mb);
    
    // Cleanup operations
    size_t PerformGarbageCollection();
    size_t CompactMemory();
    size_t FlushCaches();
    
private:
    MemoryOptimizer();
    
    std::unique_ptr<StringInterner> m_string_interner;
    std::unique_ptr<MemoryPressureHandler> m_pressure_handler;
    std::unique_ptr<MemoryBudget> m_memory_budget;
    
    std::atomic<bool> m_aggressive_caching{false};
    std::atomic<bool> m_memory_compaction{false};
    std::atomic<size_t> m_memory_limit{0};
};

} // namespace ecscope::gui::memory