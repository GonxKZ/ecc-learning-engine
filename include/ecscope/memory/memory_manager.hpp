#pragma once

#include "allocators.hpp"
#include "numa_support.hpp"
#include "memory_pools.hpp"
#include "memory_tracker.hpp"
#include "memory_utils.hpp"
#include <unordered_map>
#include <string_view>
#include <optional>
#include <functional>

namespace ecscope::memory {

// ==== ALLOCATION STRATEGY ====
enum class AllocationStrategy {
    FASTEST,           // Prioritize speed over memory efficiency
    MOST_EFFICIENT,    // Prioritize memory efficiency over speed
    BALANCED,          // Balance between speed and efficiency
    NUMA_AWARE,        // Prioritize NUMA locality
    THREAD_LOCAL,      // Use thread-local pools
    SIZE_SEGREGATED    // Use size-class segregated pools
};

// ==== MEMORY POLICY ====
struct MemoryPolicy {
    AllocationStrategy strategy = AllocationStrategy::BALANCED;
    bool enable_tracking = true;
    bool enable_leak_detection = true;
    bool enable_stack_traces = false; // Expensive, enable only for debugging
    bool enable_memory_encryption = false;
    bool enable_guard_pages = false;
    size_t alignment = alignof(std::max_align_t);
    std::string allocation_tag;
    
    // Pressure handling
    bool enable_automatic_cleanup = true;
    double cleanup_pressure_threshold = 0.8; // Cleanup when 80% memory used
    
    // Performance tuning
    bool prefer_simd_operations = true;
    bool enable_compression_for_inactive = false;
    size_t compression_threshold_kb = 1024; // Compress allocations > 1MB when inactive
};

// ==== CENTRAL MEMORY MANAGER ====
// World-class memory management system that coordinates all allocators
class MemoryManager {
public:
    static MemoryManager& instance() {
        static MemoryManager manager;
        return manager;
    }
    
    // Initialize with custom configuration
    void initialize(const MemoryPolicy& default_policy = {}) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        if (initialized_) return;
        
        default_policy_ = default_policy;
        
        // Initialize core allocators based on policy
        linear_allocator_ = std::make_unique<LinearAllocator>(64 * 1024 * 1024); // 64MB
        stack_allocator_ = std::make_unique<StackAllocator>(32 * 1024 * 1024);   // 32MB
        numa_allocator_ = std::make_unique<NUMAAllocator>(256 * 1024 * 1024);    // 256MB per node
        thread_safe_allocator_ = std::make_unique<ThreadSafeAllocator>(512 * 1024 * 1024); // 512MB
        segregated_allocator_ = std::make_unique<SegregatedPoolAllocator>();
        
        // Initialize tracking if enabled
        if (default_policy_.enable_leak_detection) {
            leak_detector_ = std::make_unique<MemoryLeakDetector>(
                default_policy_.enable_stack_traces);
        }
        
        if (default_policy_.enable_tracking) {
            bandwidth_monitor_ = std::make_unique<MemoryBandwidthMonitor>();
        }
        
        // Register for memory pressure notifications
        if (default_policy_.enable_automatic_cleanup) {
            MemoryPressureDetector::instance().register_pressure_callback(
                [this](MemoryPressureDetector::PressureLevel level) {
                    handle_memory_pressure(level);
                });
        }
        
        initialized_ = true;
    }
    
    // Smart allocation with automatic strategy selection
    [[nodiscard]] void* allocate(size_t size, 
                                const MemoryPolicy& policy = {}) {
        ensure_initialized();
        
        auto effective_policy = merge_policies(default_policy_, policy);
        void* ptr = allocate_with_strategy(size, effective_policy);
        
        if (ptr && effective_policy.enable_tracking) {
            track_allocation(ptr, size, effective_policy);
        }
        
        return ptr;
    }
    
    // Typed allocation with automatic alignment
    template<typename T, typename... Args>
    [[nodiscard]] T* allocate_object(const MemoryPolicy& policy = {}, Args&&... args) {
        auto effective_policy = merge_policies(default_policy_, policy);
        effective_policy.alignment = std::max(effective_policy.alignment, alignof(T));
        
        void* ptr = allocate(sizeof(T), effective_policy);
        if (!ptr) return nullptr;
        
        try {
            return new(ptr) T(std::forward<Args>(args)...);
        } catch (...) {
            deallocate(ptr, sizeof(T), effective_policy);
            throw;
        }
    }
    
    // Array allocation
    template<typename T>
    [[nodiscard]] T* allocate_array(size_t count, const MemoryPolicy& policy = {}) {
        auto effective_policy = merge_policies(default_policy_, policy);
        effective_policy.alignment = std::max(effective_policy.alignment, alignof(T));
        
        size_t total_size = sizeof(T) * count;
        void* ptr = allocate(total_size, effective_policy);
        
        return static_cast<T*>(ptr);
    }
    
    // Smart deallocation
    void deallocate(void* ptr, size_t size = 0, 
                   const MemoryPolicy& policy = {}) {
        if (!ptr) return;
        
        auto effective_policy = merge_policies(default_policy_, policy);
        
        if (effective_policy.enable_tracking) {
            track_deallocation(ptr, size);
        }
        
        deallocate_with_strategy(ptr, size, effective_policy);
    }
    
    // Typed deallocation
    template<typename T>
    void deallocate_object(T* ptr, const MemoryPolicy& policy = {}) {
        if (!ptr) return;
        
        ptr->~T();
        deallocate(ptr, sizeof(T), policy);
    }
    
    // Array deallocation
    template<typename T>
    void deallocate_array(T* ptr, size_t count, const MemoryPolicy& policy = {}) {
        if (!ptr) return;
        
        // Call destructors for non-trivial types
        if constexpr (!std::is_trivially_destructible_v<T>) {
            for (size_t i = 0; i < count; ++i) {
                ptr[i].~T();
            }
        }
        
        deallocate(ptr, sizeof(T) * count, policy);
    }
    
    // Memory pool management
    void register_custom_pool(const std::string& name, 
                             std::unique_ptr<AbstractPool> pool) {
        std::lock_guard<std::mutex> lock(mutex_);
        custom_pools_[name] = std::move(pool);
    }
    
    [[nodiscard]] void* allocate_from_pool(const std::string& pool_name, 
                                         size_t size,
                                         const MemoryPolicy& policy = {}) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        auto it = custom_pools_.find(pool_name);
        if (it != custom_pools_.end()) {
            void* ptr = it->second->allocate();
            
            if (ptr && policy.enable_tracking) {
                track_allocation(ptr, size, policy);
            }
            
            return ptr;
        }
        
        // Fall back to regular allocation
        return allocate(size, policy);
    }
    
    void deallocate_to_pool(const std::string& pool_name, void* ptr) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        auto it = custom_pools_.find(pool_name);
        if (it != custom_pools_.end() && it->second->owns(ptr)) {
            it->second->deallocate(ptr);
            
            if (default_policy_.enable_tracking && leak_detector_) {
                leak_detector_->record_deallocation(ptr);
            }
        }
    }
    
    // Memory operations using best available SIMD
    void copy_memory(void* dest, const void* src, size_t size) {
        if (default_policy_.prefer_simd_operations) {
            SIMDMemoryOps::fast_copy(dest, src, size);
        } else {
            std::memcpy(dest, src, size);
        }
        
        if (bandwidth_monitor_) {
            bandwidth_monitor_->record_read(size);
            bandwidth_monitor_->record_write(size);
        }
    }
    
    void set_memory(void* dest, uint8_t value, size_t size) {
        if (default_policy_.prefer_simd_operations) {
            SIMDMemoryOps::fast_set(dest, value, size);
        } else {
            std::memset(dest, value, size);
        }
        
        if (bandwidth_monitor_) {
            bandwidth_monitor_->record_write(size);
        }
    }
    
    void zero_memory(void* dest, size_t size) {
        set_memory(dest, 0, size);
    }
    
    int compare_memory(const void* ptr1, const void* ptr2, size_t size) {
        if (default_policy_.prefer_simd_operations) {
            return SIMDMemoryOps::fast_compare(ptr1, ptr2, size);
        }
        return std::memcmp(ptr1, ptr2, size);
    }
    
    // Advanced memory utilities
    [[nodiscard]] std::unique_ptr<AlignmentUtils::AlignedMemory<64>>
    create_cache_aligned_memory(size_t size) {
        return std::make_unique<AlignmentUtils::AlignedMemory<64>>(size);
    }
    
    [[nodiscard]] std::unique_ptr<MemoryProtection::GuardedMemory>
    create_guarded_memory(size_t size) {
        return std::make_unique<MemoryProtection::GuardedMemory>(size);
    }
    
    [[nodiscard]] std::unique_ptr<MemoryEncryption::EncryptedMemory>
    create_encrypted_memory(size_t size) {
        return std::make_unique<MemoryEncryption::EncryptedMemory>(size);
    }
    
    // Performance and diagnostics
    struct PerformanceMetrics {
        // Allocation performance
        double average_allocation_time_ns = 0.0;
        double peak_allocation_time_ns = 0.0;
        size_t total_allocations = 0;
        size_t failed_allocations = 0;
        
        // Memory utilization
        size_t total_allocated_bytes = 0;
        size_t peak_allocated_bytes = 0;
        size_t current_allocated_bytes = 0;
        double memory_efficiency = 0.0;
        
        // Bandwidth utilization
        double current_read_bandwidth_mbps = 0.0;
        double current_write_bandwidth_mbps = 0.0;
        double peak_bandwidth_mbps = 0.0;
        
        // Pool statistics
        size_t active_pools = 0;
        double average_pool_utilization = 0.0;
        
        // NUMA statistics
        std::unordered_map<uint32_t, double> numa_node_utilization;
        
        // Memory pressure
        MemoryPressureDetector::PressureLevel current_pressure;
    };
    
    [[nodiscard]] PerformanceMetrics get_performance_metrics() const {
        PerformanceMetrics metrics{};
        
        // Get basic statistics
        if (leak_detector_) {
            auto stats = leak_detector_->get_statistics();
            metrics.total_allocations = stats.total_allocations;
            metrics.current_allocated_bytes = stats.current_bytes;
            metrics.peak_allocated_bytes = stats.peak_bytes;
            metrics.total_allocated_bytes = stats.total_bytes_allocated;
            
            if (stats.total_bytes_allocated > 0) {
                metrics.memory_efficiency = 
                    static_cast<double>(stats.current_bytes) / stats.total_bytes_allocated;
            }
        }
        
        // Get bandwidth statistics
        if (bandwidth_monitor_) {
            auto bw_stats = bandwidth_monitor_->get_statistics();
            metrics.current_read_bandwidth_mbps = bw_stats.read_bandwidth_mbps;
            metrics.current_write_bandwidth_mbps = bw_stats.write_bandwidth_mbps;
            metrics.peak_bandwidth_mbps = std::max(
                bw_stats.peak_read_bandwidth_mbps, 
                bw_stats.peak_write_bandwidth_mbps);
        }
        
        // Get pool statistics
        if (segregated_allocator_) {
            auto pool_stats = segregated_allocator_->get_statistics();
            metrics.active_pools = pool_stats.active_pools;
            metrics.average_pool_utilization = pool_stats.overall_utilization;
        }
        
        // Get NUMA statistics
        if (numa_allocator_) {
            auto numa_stats = numa_allocator_->get_node_statistics();
            for (const auto& [node_id, stats] : numa_stats) {
                metrics.numa_node_utilization[node_id] = stats.utilization;
            }
        }
        
        // Get current memory pressure
        metrics.current_pressure = MemoryPressureDetector::instance().get_current_pressure();
        
        return metrics;
    }
    
    // Memory health check
    struct HealthReport {
        bool has_memory_leaks = false;
        bool has_memory_corruption = false;
        bool has_performance_issues = false;
        size_t leaked_bytes = 0;
        size_t leaked_allocations = 0;
        std::vector<std::string> recommendations;
        std::vector<std::string> warnings;
    };
    
    [[nodiscard]] HealthReport generate_health_report() const {
        HealthReport report{};
        
        if (leak_detector_) {
            auto leak_report = leak_detector_->generate_leak_report();
            report.has_memory_leaks = leak_report.leaked_allocation_count > 0;
            report.leaked_bytes = leak_report.total_leaked_bytes;
            report.leaked_allocations = leak_report.leaked_allocation_count;
            
            auto corruption_report = leak_detector_->generate_corruption_report();
            report.has_memory_corruption = 
                corruption_report.double_frees > 0 || corruption_report.invalid_frees > 0;
        }
        
        // Performance analysis
        auto metrics = get_performance_metrics();
        
        if (metrics.memory_efficiency < 0.5) {
            report.has_performance_issues = true;
            report.recommendations.push_back(
                "Memory efficiency is low. Consider using size-segregated pools.");
        }
        
        if (metrics.current_pressure == MemoryPressureDetector::PressureLevel::HIGH ||
            metrics.current_pressure == MemoryPressureDetector::PressureLevel::CRITICAL) {
            report.warnings.push_back("System is under high memory pressure.");
            report.recommendations.push_back("Enable automatic cleanup or increase available memory.");
        }
        
        if (metrics.average_pool_utilization < 0.3) {
            report.recommendations.push_back(
                "Pool utilization is low. Consider reducing pool sizes or enabling automatic shrinking.");
        }
        
        return report;
    }
    
    // Debug and profiling
    void export_allocation_profile(const std::string& filename) const {
        if (leak_detector_) {
            leak_detector_->export_leak_report(filename);
        }
    }
    
    void reset_statistics() {
        if (leak_detector_) {
            leak_detector_->reset();
        }
    }
    
    // Memory policy management
    void set_default_policy(const MemoryPolicy& policy) {
        std::lock_guard<std::mutex> lock(mutex_);
        default_policy_ = policy;
    }
    
    [[nodiscard]] const MemoryPolicy& get_default_policy() const {
        return default_policy_;
    }

private:
    MemoryManager() = default;
    ~MemoryManager() = default;
    
    // Non-copyable, non-movable
    MemoryManager(const MemoryManager&) = delete;
    MemoryManager& operator=(const MemoryManager&) = delete;
    
    void ensure_initialized() {
        if (!initialized_) {
            initialize();
        }
    }
    
    MemoryPolicy merge_policies(const MemoryPolicy& base, const MemoryPolicy& override) {
        // Override takes precedence over base
        MemoryPolicy result = base;
        
        if (override.strategy != AllocationStrategy::BALANCED || 
            base.strategy == AllocationStrategy::BALANCED) {
            result.strategy = override.strategy;
        }
        
        if (!override.allocation_tag.empty()) {
            result.allocation_tag = override.allocation_tag;
        }
        
        // Override boolean flags if explicitly set
        if (override.enable_tracking != base.enable_tracking) {
            result.enable_tracking = override.enable_tracking;
        }
        
        if (override.alignment != alignof(std::max_align_t)) {
            result.alignment = override.alignment;
        }
        
        return result;
    }
    
    void* allocate_with_strategy(size_t size, const MemoryPolicy& policy) {
        void* ptr = nullptr;
        
        switch (policy.strategy) {
            case AllocationStrategy::FASTEST:
                // Try linear allocator first, then pools
                if (linear_allocator_) {
                    ptr = linear_allocator_->allocate(size, policy.alignment);
                }
                if (!ptr && segregated_allocator_) {
                    ptr = segregated_allocator_->allocate(size, policy.alignment);
                }
                break;
                
            case AllocationStrategy::MOST_EFFICIENT:
                // Use size-segregated pools for best fit
                if (segregated_allocator_) {
                    ptr = segregated_allocator_->allocate(size, policy.alignment);
                }
                break;
                
            case AllocationStrategy::NUMA_AWARE:
                if (numa_allocator_) {
                    ptr = numa_allocator_->allocate(size, policy.alignment);
                }
                break;
                
            case AllocationStrategy::THREAD_LOCAL:
                if (thread_safe_allocator_) {
                    ptr = thread_safe_allocator_->allocate(size, policy.alignment);
                }
                break;
                
            case AllocationStrategy::SIZE_SEGREGATED:
                if (segregated_allocator_) {
                    ptr = segregated_allocator_->allocate(size, policy.alignment);
                }
                break;
                
            case AllocationStrategy::BALANCED:
            default:
                // Try most appropriate allocator based on size
                if (SizeClassConfig::is_small_object(size) && segregated_allocator_) {
                    ptr = segregated_allocator_->allocate(size, policy.alignment);
                } else if (thread_safe_allocator_) {
                    ptr = thread_safe_allocator_->allocate(size, policy.alignment);
                }
                break;
        }
        
        // Fallback to system allocator if all else fails
        if (!ptr) {
            ptr = AlignmentUtils::aligned_alloc<alignof(std::max_align_t)>(size);
        }
        
        // Apply additional policies
        if (ptr) {
            if (policy.enable_guard_pages && size >= 4096) {
                // For large allocations, consider using guarded memory
                // This would require refactoring the allocation interface
            }
            
            if (policy.enable_memory_encryption) {
                // Encrypt the memory region
                auto key = MemoryEncryption::generate_key();
                MemoryEncryption::encrypt_inplace(ptr, size, key);
            }
        }
        
        return ptr;
    }
    
    void deallocate_with_strategy(void* ptr, size_t size, const MemoryPolicy& policy) {
        // Try to find the owning allocator
        bool deallocated = false;
        
        if (linear_allocator_ && linear_allocator_->owns(ptr)) {
            linear_allocator_->deallocate(ptr, size);
            deallocated = true;
        } else if (stack_allocator_ && stack_allocator_->owns(ptr)) {
            stack_allocator_->deallocate(ptr, size);
            deallocated = true;
        } else if (numa_allocator_ && numa_allocator_->owns(ptr)) {
            numa_allocator_->deallocate(ptr, size);
            deallocated = true;
        } else if (thread_safe_allocator_ && thread_safe_allocator_->owns(ptr)) {
            thread_safe_allocator_->deallocate(ptr, size);
            deallocated = true;
        } else if (segregated_allocator_ && segregated_allocator_->owns(ptr)) {
            segregated_allocator_->deallocate(ptr, size);
            deallocated = true;
        }
        
        // Check custom pools
        if (!deallocated) {
            for (auto& [name, pool] : custom_pools_) {
                if (pool->owns(ptr)) {
                    pool->deallocate(ptr);
                    deallocated = true;
                    break;
                }
            }
        }
        
        // Fallback to system deallocation
        if (!deallocated) {
            AlignmentUtils::aligned_free(ptr);
        }
    }
    
    void track_allocation(void* ptr, size_t size, const MemoryPolicy& policy) {
        if (leak_detector_) {
            leak_detector_->record_allocation(ptr, size, policy.alignment, policy.allocation_tag);
        }
    }
    
    void track_deallocation(void* ptr, size_t size) {
        if (leak_detector_) {
            leak_detector_->record_deallocation(ptr);
        }
    }
    
    void handle_memory_pressure(MemoryPressureDetector::PressureLevel level) {
        switch (level) {
            case MemoryPressureDetector::PressureLevel::MODERATE:
                // Trigger maintenance on pools
                if (segregated_allocator_) {
                    segregated_allocator_->trigger_maintenance();
                }
                break;
                
            case MemoryPressureDetector::PressureLevel::HIGH:
                // Force shrink pools
                if (segregated_allocator_) {
                    segregated_allocator_->force_shrink();
                }
                // Reset thread caches
                if (thread_safe_allocator_) {
                    thread_safe_allocator_->collect_unused_caches();
                }
                break;
                
            case MemoryPressureDetector::PressureLevel::CRITICAL:
                // Aggressive cleanup
                if (linear_allocator_) {
                    linear_allocator_->reset();
                }
                if (stack_allocator_) {
                    stack_allocator_->reset();
                }
                if (segregated_allocator_) {
                    segregated_allocator_->force_shrink();
                    segregated_allocator_->force_defragmentation();
                }
                break;
                
            default:
                break;
        }
    }
    
    // Core allocators
    std::unique_ptr<LinearAllocator> linear_allocator_;
    std::unique_ptr<StackAllocator> stack_allocator_;
    std::unique_ptr<NUMAAllocator> numa_allocator_;
    std::unique_ptr<ThreadSafeAllocator> thread_safe_allocator_;
    std::unique_ptr<SegregatedPoolAllocator> segregated_allocator_;
    
    // Custom pools
    std::unordered_map<std::string, std::unique_ptr<AbstractPool>> custom_pools_;
    
    // Tracking and monitoring
    std::unique_ptr<MemoryLeakDetector> leak_detector_;
    std::unique_ptr<MemoryBandwidthMonitor> bandwidth_monitor_;
    
    // Configuration
    MemoryPolicy default_policy_;
    bool initialized_ = false;
    
    mutable std::mutex mutex_;
};

// ==== CONVENIENT GLOBAL FUNCTIONS ====
// Global convenience functions for easy memory management

template<typename T, typename... Args>
[[nodiscard]] T* make_unique_memory(Args&&... args) {
    return MemoryManager::instance().allocate_object<T>(MemoryPolicy{}, std::forward<Args>(args)...);
}

template<typename T>
[[nodiscard]] T* make_array_memory(size_t count) {
    return MemoryManager::instance().allocate_array<T>(count);
}

template<typename T>
void free_memory(T* ptr) {
    MemoryManager::instance().deallocate_object(ptr);
}

template<typename T>
void free_array_memory(T* ptr, size_t count) {
    MemoryManager::instance().deallocate_array(ptr, count);
}

// RAII memory wrapper for automatic cleanup
template<typename T>
class unique_memory_ptr {
public:
    unique_memory_ptr() = default;
    explicit unique_memory_ptr(T* ptr) : ptr_(ptr) {}
    
    ~unique_memory_ptr() {
        if (ptr_) {
            free_memory(ptr_);
        }
    }
    
    // Non-copyable, movable
    unique_memory_ptr(const unique_memory_ptr&) = delete;
    unique_memory_ptr& operator=(const unique_memory_ptr&) = delete;
    
    unique_memory_ptr(unique_memory_ptr&& other) noexcept 
        : ptr_(std::exchange(other.ptr_, nullptr)) {}
    
    unique_memory_ptr& operator=(unique_memory_ptr&& other) noexcept {
        if (this != &other) {
            if (ptr_) free_memory(ptr_);
            ptr_ = std::exchange(other.ptr_, nullptr);
        }
        return *this;
    }
    
    [[nodiscard]] T* get() const noexcept { return ptr_; }
    [[nodiscard]] T* operator->() const noexcept { return ptr_; }
    [[nodiscard]] T& operator*() const noexcept { return *ptr_; }
    
    [[nodiscard]] T* release() noexcept {
        return std::exchange(ptr_, nullptr);
    }
    
    void reset(T* ptr = nullptr) noexcept {
        if (ptr_) free_memory(ptr_);
        ptr_ = ptr;
    }
    
    [[nodiscard]] explicit operator bool() const noexcept {
        return ptr_ != nullptr;
    }

private:
    T* ptr_ = nullptr;
};

template<typename T, typename... Args>
[[nodiscard]] unique_memory_ptr<T> make_unique_memory_ptr(Args&&... args) {
    return unique_memory_ptr<T>{make_unique_memory<T>(std::forward<Args>(args)...)};
}

} // namespace ecscope::memory