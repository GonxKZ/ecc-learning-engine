#pragma once

#include "allocators.hpp"
#include "numa_support.hpp"
#include <array>
#include <chrono>
#include <algorithm>
#include <cmath>
#include <functional>

namespace ecscope::memory {

// ==== SIZE CLASS CONFIGURATION ====
// Optimized size classes based on common allocation patterns
class SizeClassConfig {
public:
    // Size classes: powers of 2 up to 64KB, then larger increments
    static constexpr std::array<size_t, 32> SIZE_CLASSES = {
        8, 16, 24, 32, 48, 64, 80, 96, 112, 128,           // Small objects
        160, 192, 224, 256, 320, 384, 448, 512,           // Medium objects  
        640, 768, 896, 1024, 1280, 1536, 1792, 2048,     // Large objects
        2560, 3072, 3584, 4096, 8192, 16384              // Very large objects
    };
    
    static constexpr size_t MAX_SMALL_SIZE = 256;
    static constexpr size_t MAX_MEDIUM_SIZE = 2048;
    static constexpr size_t MAX_LARGE_SIZE = 16384;
    
    [[nodiscard]] static constexpr size_t get_size_class_index(size_t size) noexcept {
        for (size_t i = 0; i < SIZE_CLASSES.size(); ++i) {
            if (size <= SIZE_CLASSES[i]) {
                return i;
            }
        }
        return SIZE_CLASSES.size() - 1; // Use largest class for oversized allocations
    }
    
    [[nodiscard]] static constexpr size_t get_size_class(size_t size) noexcept {
        return SIZE_CLASSES[get_size_class_index(size)];
    }
    
    [[nodiscard]] static constexpr bool is_small_object(size_t size) noexcept {
        return size <= MAX_SMALL_SIZE;
    }
    
    [[nodiscard]] static constexpr bool is_medium_object(size_t size) noexcept {
        return size > MAX_SMALL_SIZE && size <= MAX_MEDIUM_SIZE;
    }
    
    [[nodiscard]] static constexpr bool is_large_object(size_t size) noexcept {
        return size > MAX_MEDIUM_SIZE && size <= MAX_LARGE_SIZE;
    }
};

// ==== MEMORY PRESSURE DETECTOR ====
// Monitors system memory pressure and triggers cleanup
class MemoryPressureDetector {
public:
    enum class PressureLevel {
        LOW,       // < 50% memory used
        MODERATE,  // 50-75% memory used  
        HIGH,      // 75-90% memory used
        CRITICAL   // > 90% memory used
    };
    
    static MemoryPressureDetector& instance() {
        static MemoryPressureDetector detector;
        return detector;
    }
    
    [[nodiscard]] PressureLevel get_current_pressure() const {
        auto memory_info = get_system_memory_info();
        double usage_ratio = static_cast<double>(memory_info.used) / memory_info.total;
        
        if (usage_ratio < 0.5) return PressureLevel::LOW;
        if (usage_ratio < 0.75) return PressureLevel::MODERATE;
        if (usage_ratio < 0.9) return PressureLevel::HIGH;
        return PressureLevel::CRITICAL;
    }
    
    // Register callback for pressure level changes
    using PressureCallback = std::function<void(PressureLevel)>;
    void register_pressure_callback(PressureCallback callback) {
        std::lock_guard<std::mutex> lock(callback_mutex_);
        callbacks_.push_back(std::move(callback));
    }
    
    void start_monitoring() {
        monitoring_ = true;
        monitor_thread_ = std::thread([this]() {
            while (monitoring_) {
                auto current_pressure = get_current_pressure();
                if (current_pressure != last_pressure_) {
                    notify_pressure_change(current_pressure);
                    last_pressure_ = current_pressure;
                }
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
        });
    }
    
    void stop_monitoring() {
        monitoring_ = false;
        if (monitor_thread_.joinable()) {
            monitor_thread_.join();
        }
    }
    
private:
    struct MemoryInfo {
        size_t total;
        size_t used;
        size_t available;
    };
    
    [[nodiscard]] MemoryInfo get_system_memory_info() const {
#ifdef _WIN32
        MEMORYSTATUSEX status;
        status.dwLength = sizeof(status);
        GlobalMemoryStatusEx(&status);
        
        return {
            .total = static_cast<size_t>(status.ullTotalPhys),
            .used = static_cast<size_t>(status.ullTotalPhys - status.ullAvailPhys),
            .available = static_cast<size_t>(status.ullAvailPhys)
        };
#else
        long pages = sysconf(_SC_PHYS_PAGES);
        long avail_pages = sysconf(_SC_AVPHYS_PAGES);
        long page_size = sysconf(_SC_PAGE_SIZE);
        
        size_t total = pages * page_size;
        size_t available = avail_pages * page_size;
        
        return {
            .total = total,
            .used = total - available,
            .available = available
        };
#endif
    }
    
    void notify_pressure_change(PressureLevel new_level) {
        std::lock_guard<std::mutex> lock(callback_mutex_);
        for (const auto& callback : callbacks_) {
            callback(new_level);
        }
    }
    
    std::atomic<bool> monitoring_{false};
    std::thread monitor_thread_;
    PressureLevel last_pressure_{PressureLevel::LOW};
    
    std::mutex callback_mutex_;
    std::vector<PressureCallback> callbacks_;
};

// ==== DYNAMIC MEMORY POOL ====
// Self-managing pool that grows/shrinks based on demand
template<size_t SizeClass>
class DynamicPool {
    static constexpr size_t INITIAL_CAPACITY = 1024; // Initial number of objects
    static constexpr size_t GROWTH_FACTOR = 2;       // How much to grow by
    static constexpr size_t MAX_POOLS = 16;          // Maximum number of pools
    static constexpr size_t SHRINK_THRESHOLD = 4;    // Shrink if usage < 1/SHRINK_THRESHOLD
    
    using PoolType = ObjectPool<std::aligned_storage_t<SizeClass, alignof(std::max_align_t)>>;
    
public:
    DynamicPool() {
        // Start with one pool
        pools_.emplace_back(std::make_unique<PoolType>(INITIAL_CAPACITY));
        
        // Register for memory pressure notifications
        MemoryPressureDetector::instance().register_pressure_callback(
            [this](MemoryPressureDetector::PressureLevel level) {
                handle_memory_pressure(level);
            });
    }
    
    [[nodiscard]] void* allocate() {
        std::lock_guard<std::shared_mutex> lock(pools_mutex_);
        
        // Try to allocate from existing pools
        for (auto& pool : pools_) {
            if (auto ptr = pool->allocate()) {
                ++allocation_count_;
                update_usage_statistics();
                return ptr;
            }
        }
        
        // Need to grow - add new pool if possible
        if (pools_.size() < MAX_POOLS) {
            size_t new_capacity = INITIAL_CAPACITY * static_cast<size_t>(
                std::pow(GROWTH_FACTOR, pools_.size()));
            pools_.emplace_back(std::make_unique<PoolType>(new_capacity));
            
            auto ptr = pools_.back()->allocate();
            if (ptr) {
                ++allocation_count_;
                ++growth_events_;
                update_usage_statistics();
            }
            return ptr;
        }
        
        return nullptr; // Out of memory
    }
    
    void deallocate(void* ptr) {
        if (!ptr) return;
        
        std::shared_lock<std::shared_mutex> lock(pools_mutex_);
        
        for (auto& pool : pools_) {
            if (pool->owns(ptr)) {
                pool->deallocate(static_cast<typename PoolType::value_type*>(ptr));
                ++deallocation_count_;
                update_usage_statistics();
                return;
            }
        }
    }
    
    [[nodiscard]] bool owns(const void* ptr) const {
        std::shared_lock<std::shared_mutex> lock(pools_mutex_);
        
        for (const auto& pool : pools_) {
            if (pool->owns(ptr)) {
                return true;
            }
        }
        return false;
    }
    
    // Pool management
    void try_shrink() {
        std::lock_guard<std::shared_mutex> lock(pools_mutex_);
        
        if (pools_.size() <= 1) return; // Keep at least one pool
        
        // Check if we can remove underutilized pools
        for (auto it = pools_.begin(); it != pools_.end();) {
            if ((*it)->utilization() < (1.0 / SHRINK_THRESHOLD) && pools_.size() > 1) {
                if ((*it)->empty()) {
                    it = pools_.erase(it);
                    ++shrink_events_;
                    continue;
                }
            }
            ++it;
        }
        
        update_usage_statistics();
    }
    
    // Statistics
    struct PoolStatistics {
        size_t size_class;
        size_t total_capacity;
        size_t total_used;
        size_t total_available;
        double utilization;
        size_t pool_count;
        size_t allocation_count;
        size_t deallocation_count;
        size_t growth_events;
        size_t shrink_events;
        double average_utilization;
    };
    
    [[nodiscard]] PoolStatistics get_statistics() const {
        std::shared_lock<std::shared_mutex> lock(pools_mutex_);
        
        PoolStatistics stats{};
        stats.size_class = SizeClass;
        stats.pool_count = pools_.size();
        stats.allocation_count = allocation_count_.load();
        stats.deallocation_count = deallocation_count_.load();
        stats.growth_events = growth_events_.load();
        stats.shrink_events = shrink_events_.load();
        
        for (const auto& pool : pools_) {
            stats.total_capacity += pool->capacity();
            stats.total_used += pool->used();
            stats.total_available += pool->available();
        }
        
        stats.utilization = stats.total_capacity > 0 ? 
            static_cast<double>(stats.total_used) / stats.total_capacity : 0.0;
        stats.average_utilization = recent_utilizations_.empty() ? 0.0 :
            std::accumulate(recent_utilizations_.begin(), recent_utilizations_.end(), 0.0) / 
            recent_utilizations_.size();
        
        return stats;
    }
    
    void defragment() {
        // For object pools, defragmentation is not applicable
        // This could be implemented for other allocator types
    }

private:
    void handle_memory_pressure(MemoryPressureDetector::PressureLevel level) {
        switch (level) {
            case MemoryPressureDetector::PressureLevel::HIGH:
            case MemoryPressureDetector::PressureLevel::CRITICAL:
                try_shrink();
                break;
            default:
                break;
        }
    }
    
    void update_usage_statistics() {
        // Keep running average of utilization
        if (pools_.empty()) return;
        
        size_t total_capacity = 0;
        size_t total_used = 0;
        
        for (const auto& pool : pools_) {
            total_capacity += pool->capacity();
            total_used += pool->used();
        }
        
        double current_utilization = total_capacity > 0 ? 
            static_cast<double>(total_used) / total_capacity : 0.0;
        
        recent_utilizations_.push_back(current_utilization);
        if (recent_utilizations_.size() > 100) { // Keep last 100 measurements
            recent_utilizations_.erase(recent_utilizations_.begin());
        }
    }
    
    mutable std::shared_mutex pools_mutex_;
    std::vector<std::unique_ptr<PoolType>> pools_;
    
    std::atomic<size_t> allocation_count_{0};
    std::atomic<size_t> deallocation_count_{0};
    std::atomic<size_t> growth_events_{0};
    std::atomic<size_t> shrink_events_{0};
    
    std::vector<double> recent_utilizations_;
};

// ==== ABSTRACT POOL INTERFACE ====
class AbstractPool {
public:
    virtual ~AbstractPool() = default;
    virtual void* allocate() = 0;
    virtual void deallocate(void* ptr) = 0;
    virtual bool owns(const void* ptr) const = 0;
    virtual void try_shrink() = 0;
    virtual void defragment() = 0;
    
    struct Statistics {
        size_t size_class;
        size_t total_capacity;
        size_t total_used;
        size_t total_available;
        double utilization;
        size_t pool_count;
        size_t allocation_count;
        size_t deallocation_count;
        size_t growth_events;
        size_t shrink_events;
    };
    
    virtual Statistics get_statistics() const = 0;
};

// Wrapper to make DynamicPool compatible with AbstractPool
template<size_t SizeClass>
class DynamicPoolWrapper : public AbstractPool {
public:
    DynamicPoolWrapper() = default;
    
    void* allocate() override {
        return pool_.allocate();
    }
    
    void deallocate(void* ptr) override {
        pool_.deallocate(ptr);
    }
    
    bool owns(const void* ptr) const override {
        return pool_.owns(ptr);
    }
    
    void try_shrink() override {
        pool_.try_shrink();
    }
    
    void defragment() override {
        pool_.defragment();
    }
    
    Statistics get_statistics() const override {
        auto pool_stats = pool_.get_statistics();
        return {
            .size_class = pool_stats.size_class,
            .total_capacity = pool_stats.total_capacity,
            .total_used = pool_stats.total_used,
            .total_available = pool_stats.total_available,
            .utilization = pool_stats.utilization,
            .pool_count = pool_stats.pool_count,
            .allocation_count = pool_stats.allocation_count,
            .deallocation_count = pool_stats.deallocation_count,
            .growth_events = pool_stats.growth_events,
            .shrink_events = pool_stats.shrink_events
        };
    }

private:
    DynamicPool<SizeClass> pool_;
};

// ==== SIZE-CLASS SEGREGATED ALLOCATOR ====
// Main allocator that routes requests to appropriate pools
class SegregatedPoolAllocator {
public:
    SegregatedPoolAllocator() {
        // Initialize pools for each size class
        initialize_pools();
        
        // Start memory pressure monitoring
        MemoryPressureDetector::instance().start_monitoring();
        
        // Start background maintenance
        start_maintenance_thread();
    }
    
    ~SegregatedPoolAllocator() {
        stop_maintenance_thread();
        MemoryPressureDetector::instance().stop_monitoring();
    }
    
    [[nodiscard]] void* allocate(size_t size, size_t alignment = alignof(std::max_align_t)) {
        // Handle alignment requirements
        size = std::max(size, alignment);
        
        size_t class_index = SizeClassConfig::get_size_class_index(size);
        
        if (class_index < pools_.size() && pools_[class_index]) {
            if (auto ptr = pools_[class_index]->allocate()) {
                return ptr;
            }
        }
        
        // Fall back to large object allocator
        return large_object_allocator_.allocate(size, alignment);
    }
    
    void deallocate(void* ptr, size_t size) {
        if (!ptr) return;
        
        // Try size-class pools first
        size_t class_index = SizeClassConfig::get_size_class_index(size);
        if (class_index < pools_.size() && pools_[class_index] && 
            pools_[class_index]->owns(ptr)) {
            pools_[class_index]->deallocate(ptr);
            return;
        }
        
        // Check all pools (for cases where size is unknown)
        for (auto& pool : pools_) {
            if (pool && pool->owns(ptr)) {
                pool->deallocate(ptr);
                return;
            }
        }
        
        // Try large object allocator
        large_object_allocator_.deallocate(ptr, size);
    }
    
    [[nodiscard]] bool owns(const void* ptr) const {
        for (const auto& pool : pools_) {
            if (pool && pool->owns(ptr)) {
                return true;
            }
        }
        return large_object_allocator_.owns(ptr);
    }
    
    // Pool management operations
    void trigger_maintenance() {
        maintenance_needed_.store(true);
        maintenance_cv_.notify_one();
    }
    
    void force_defragmentation() {
        for (auto& pool : pools_) {
            if (pool) {
                pool->defragment();
            }
        }
    }
    
    void force_shrink() {
        for (auto& pool : pools_) {
            if (pool) {
                pool->try_shrink();
            }
        }
    }
    
    // Comprehensive statistics
    struct AllocatorStatistics {
        size_t total_capacity;
        size_t total_used;
        size_t total_available;
        double overall_utilization;
        size_t total_allocations;
        size_t total_deallocations;
        size_t total_growth_events;
        size_t total_shrink_events;
        size_t active_pools;
        MemoryPressureDetector::PressureLevel current_pressure;
        
        struct SizeClassStats {
            size_t size_class;
            size_t capacity;
            size_t used;
            double utilization;
            size_t pool_count;
        };
        std::vector<SizeClassStats> size_class_stats;
    };
    
    [[nodiscard]] AllocatorStatistics get_statistics() const {
        AllocatorStatistics stats{};
        stats.current_pressure = MemoryPressureDetector::instance().get_current_pressure();
        
        for (size_t i = 0; i < pools_.size(); ++i) {
            if (pools_[i]) {
                auto pool_stats = pools_[i]->get_statistics();
                
                stats.total_capacity += pool_stats.total_capacity;
                stats.total_used += pool_stats.total_used;
                stats.total_available += pool_stats.total_available;
                stats.total_allocations += pool_stats.allocation_count;
                stats.total_deallocations += pool_stats.deallocation_count;
                stats.total_growth_events += pool_stats.growth_events;
                stats.total_shrink_events += pool_stats.shrink_events;
                
                if (pool_stats.total_capacity > 0) {
                    ++stats.active_pools;
                    
                    AllocatorStatistics::SizeClassStats class_stats{};
                    class_stats.size_class = pool_stats.size_class;
                    class_stats.capacity = pool_stats.total_capacity;
                    class_stats.used = pool_stats.total_used;
                    class_stats.utilization = pool_stats.utilization;
                    class_stats.pool_count = pool_stats.pool_count;
                    
                    stats.size_class_stats.push_back(class_stats);
                }
            }
        }
        
        stats.overall_utilization = stats.total_capacity > 0 ?
            static_cast<double>(stats.total_used) / stats.total_capacity : 0.0;
        
        return stats;
    }

private:
    void initialize_pools() {
        pools_.resize(SizeClassConfig::SIZE_CLASSES.size());
        
        // Create pools for each size class using template specialization
        create_pool<0>();
    }
    
    template<size_t Index>
    void create_pool() {
        if constexpr (Index < SizeClassConfig::SIZE_CLASSES.size()) {
            constexpr size_t size_class = SizeClassConfig::SIZE_CLASSES[Index];
            pools_[Index] = std::make_unique<DynamicPoolWrapper<size_class>>();
            create_pool<Index + 1>();
        }
    }
    
    void start_maintenance_thread() {
        maintenance_running_ = true;
        maintenance_thread_ = std::thread([this]() {
            std::unique_lock<std::mutex> lock(maintenance_mutex_);
            
            while (maintenance_running_) {
                // Wait for maintenance signal or timeout
                if (maintenance_cv_.wait_for(lock, std::chrono::minutes(5), 
                    [this] { return maintenance_needed_.load() || !maintenance_running_; })) {
                    
                    if (!maintenance_running_) break;
                    
                    // Perform maintenance
                    perform_maintenance();
                    maintenance_needed_.store(false);
                }
            }
        });
    }
    
    void stop_maintenance_thread() {
        maintenance_running_ = false;
        maintenance_cv_.notify_one();
        if (maintenance_thread_.joinable()) {
            maintenance_thread_.join();
        }
    }
    
    void perform_maintenance() {
        // Shrink underutilized pools
        auto pressure = MemoryPressureDetector::instance().get_current_pressure();
        if (pressure >= MemoryPressureDetector::PressureLevel::MODERATE) {
            for (auto& pool : pools_) {
                if (pool) {
                    pool->try_shrink();
                }
            }
        }
        
        // Additional maintenance tasks could include:
        // - Defragmentation
        // - Compaction
        // - Statistics collection
        // - Performance optimization
    }
    
    std::vector<std::unique_ptr<AbstractPool>> pools_; // Type-erased pool pointers
    NUMAAllocator large_object_allocator_{256 * 1024 * 1024}; // 256MB for large objects
    
    // Maintenance thread
    std::atomic<bool> maintenance_running_{false};
    std::atomic<bool> maintenance_needed_{false};
    std::thread maintenance_thread_;
    std::mutex maintenance_mutex_;
    std::condition_variable maintenance_cv_;
};

} // namespace ecscope::memory