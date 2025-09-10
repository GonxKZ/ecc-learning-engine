#pragma once

/**
 * @file advanced.hpp
 * @brief Advanced Memory Management System - Complete Header
 * 
 * ECScope Advanced Memory Management System
 * ========================================
 * 
 * This header provides access to the complete world-class memory management system
 * that rivals commercial game engines. It includes:
 * 
 * CORE ALLOCATORS:
 * - LinearAllocator: Ultra-fast bump pointer allocation (< 10ns)
 * - StackAllocator: LIFO allocation with marker-based unwinding
 * - ObjectPool<T>: Zero-fragmentation fixed-size allocation
 * - FreeListAllocator: General-purpose with coalescing and splitting
 * 
 * ADVANCED ALLOCATORS:
 * - NUMAAllocator: NUMA-aware allocation for multi-CPU systems
 * - ThreadSafeAllocator: Thread-safe with per-thread caching
 * - LockFreeAllocator<Size>: Lock-free for hot allocation paths
 * - SegregatedPoolAllocator: Size-class segregated with auto-scaling
 * 
 * MEMORY TRACKING & PROFILING:
 * - MemoryLeakDetector: Real-time leak detection with stack traces
 * - AllocationStatistics: Comprehensive allocation metrics
 * - MemoryBandwidthMonitor: Memory bandwidth utilization tracking
 * - HotspotAnalysis: Allocation pattern analysis and optimization
 * 
 * ADVANCED FEATURES:
 * - SIMD-optimized memory operations (copy, set, compare)
 * - Custom memory alignment with SIMD support
 * - Memory protection and guard pages
 * - Memory encryption for sensitive data
 * - Copy-on-write memory management
 * - Memory compression for inactive data
 * - NUMA topology detection and optimization
 * 
 * MEMORY MANAGER:
 * - Unified memory management interface
 * - Automatic allocation strategy selection
 * - Cross-pool allocation strategies
 * - Memory pressure detection and auto-cleanup
 * - Real-time performance monitoring
 * - Health reporting and diagnostics
 * 
 * PERFORMANCE TARGETS:
 * - < 10ns allocation time for pool allocators
 * - 99.9%+ memory efficiency (minimal overhead)
 * - TB-scale memory management support
 * - Sub-microsecond memory tracking overhead
 * - NUMA-aware allocation with 2x+ performance boost
 * - SIMD memory operations with significant speedup
 * 
 * USAGE EXAMPLES:
 * 
 * Basic Usage:
 * ```cpp
 * // Initialize memory manager
 * MemoryManager::instance().initialize();
 * 
 * // Allocate with strategy
 * MemoryPolicy policy;
 * policy.strategy = AllocationStrategy::SIZE_SEGREGATED;
 * void* ptr = MemoryManager::instance().allocate(1024, policy);
 * 
 * // Smart pointers
 * auto smart_ptr = make_unique_memory_ptr<MyClass>(args...);
 * ```
 * 
 * ECS Integration:
 * ```cpp
 * // Component with custom allocation
 * struct MyComponent {
 *     static void* operator new(size_t size) {
 *         MemoryPolicy policy;
 *         policy.strategy = AllocationStrategy::NUMA_AWARE;
 *         return MemoryManager::instance().allocate(size, policy);
 *     }
 * };
 * ```
 * 
 * Performance Monitoring:
 * ```cpp
 * auto metrics = MemoryManager::instance().get_performance_metrics();
 * auto health = MemoryManager::instance().generate_health_report();
 * ```
 * 
 * @author ECScope Memory Management Team
 * @version 2.0
 * @date 2024
 */

// Core memory management headers
#include "allocators.hpp"          // Core allocator implementations
#include "numa_support.hpp"        // NUMA-aware allocation
#include "memory_pools.hpp"        // Dynamic pool management  
#include "memory_tracker.hpp"      // Advanced tracking and profiling
#include "memory_utils.hpp"        // Memory utilities and advanced features
#include "memory_manager.hpp"      // Central memory management

namespace ecscope::memory {

/**
 * @brief Memory Management System Version Information
 */
struct Version {
    static constexpr int MAJOR = 2;
    static constexpr int MINOR = 0;
    static constexpr int PATCH = 0;
    static constexpr const char* STRING = "2.0.0";
    static constexpr const char* NAME = "ECScope Advanced Memory Management System";
    static constexpr const char* DESCRIPTION = "World-class memory management rivaling commercial game engines";
};

/**
 * @brief System capabilities detection
 */
struct SystemCapabilities {
    bool numa_available = false;
    bool sse2_available = false;
    bool avx2_available = false;
    bool avx512_available = false;
    size_t hardware_threads = 0;
    size_t cache_line_size = 0;
    size_t numa_nodes = 0;
    
    static SystemCapabilities detect() {
        SystemCapabilities caps;
        
        // NUMA detection
        auto& topology = NUMATopology::instance();
        caps.numa_available = topology.is_numa_available();
        caps.numa_nodes = topology.get_num_nodes();
        
        // SIMD detection
        caps.sse2_available = SIMDMemoryOps::has_sse2();
        caps.avx2_available = SIMDMemoryOps::has_avx2();
        caps.avx512_available = SIMDMemoryOps::has_avx512();
        
        // System info
        caps.hardware_threads = std::thread::hardware_concurrency();
        caps.cache_line_size = get_cache_line_size();
        
        return caps;
    }
    
    void print() const {
        std::cout << "System Capabilities:\n";
        std::cout << "  Hardware threads: " << hardware_threads << "\n";
        std::cout << "  Cache line size: " << cache_line_size << " bytes\n";
        std::cout << "  NUMA nodes: " << numa_nodes << (numa_available ? " (Available)" : " (Simulated)") << "\n";
        std::cout << "  SIMD support: ";
        if (avx512_available) std::cout << "AVX-512";
        else if (avx2_available) std::cout << "AVX2";
        else if (sse2_available) std::cout << "SSE2";
        else std::cout << "None";
        std::cout << "\n";
    }
};

/**
 * @brief Quick-start memory management initialization
 * 
 * Provides one-line initialization for common use cases:
 * - Gaming: Optimized for game engine usage
 * - ServerApplication: Optimized for server applications
 * - EmbeddedSystem: Optimized for embedded systems
 * - DevelopmentMode: Full tracking and debugging enabled
 */
class QuickStart {
public:
    // Gaming configuration: Maximum performance, minimal tracking
    static void initialize_for_gaming() {
        MemoryPolicy policy;
        policy.strategy = AllocationStrategy::SIZE_SEGREGATED;
        policy.enable_tracking = false;
        policy.enable_leak_detection = false;
        policy.prefer_simd_operations = true;
        policy.enable_automatic_cleanup = true;
        
        MemoryManager::instance().initialize(policy);
    }
    
    // Server application: Balanced performance and reliability
    static void initialize_for_server() {
        MemoryPolicy policy;
        policy.strategy = AllocationStrategy::NUMA_AWARE;
        policy.enable_tracking = true;
        policy.enable_leak_detection = true;
        policy.enable_stack_traces = false;
        policy.enable_automatic_cleanup = true;
        
        MemoryManager::instance().initialize(policy);
    }
    
    // Embedded system: Minimal memory footprint
    static void initialize_for_embedded() {
        MemoryPolicy policy;
        policy.strategy = AllocationStrategy::MOST_EFFICIENT;
        policy.enable_tracking = false;
        policy.enable_leak_detection = false;
        policy.prefer_simd_operations = false;
        policy.enable_automatic_cleanup = true;
        
        MemoryManager::instance().initialize(policy);
    }
    
    // Development mode: Full debugging and profiling
    static void initialize_for_development() {
        MemoryPolicy policy;
        policy.strategy = AllocationStrategy::BALANCED;
        policy.enable_tracking = true;
        policy.enable_leak_detection = true;
        policy.enable_stack_traces = true;
        policy.enable_guard_pages = true;
        policy.enable_automatic_cleanup = true;
        
        MemoryManager::instance().initialize(policy);
        
        // Start memory pressure monitoring
        MemoryPressureDetector::instance().start_monitoring();
    }
};

/**
 * @brief Memory management benchmarking utilities
 */
class Benchmark {
public:
    struct BenchmarkResults {
        double allocation_time_ns = 0.0;
        double deallocation_time_ns = 0.0;
        double memory_bandwidth_gbps = 0.0;
        double efficiency_percentage = 0.0;
        size_t successful_allocations = 0;
        size_t failed_allocations = 0;
    };
    
    // Benchmark allocation speed
    static BenchmarkResults benchmark_allocation(AllocationStrategy strategy, 
                                                size_t allocation_size,
                                                size_t num_iterations = 10000) {
        MemoryPolicy policy;
        policy.strategy = strategy;
        policy.enable_tracking = false;
        
        auto& manager = MemoryManager::instance();
        std::vector<void*> ptrs;
        ptrs.reserve(num_iterations);
        
        // Benchmark allocation
        auto start = std::chrono::high_resolution_clock::now();
        
        for (size_t i = 0; i < num_iterations; ++i) {
            void* ptr = manager.allocate(allocation_size, policy);
            if (ptr) {
                ptrs.push_back(ptr);
            }
        }
        
        auto alloc_end = std::chrono::high_resolution_clock::now();
        
        // Benchmark deallocation
        for (void* ptr : ptrs) {
            manager.deallocate(ptr, allocation_size, policy);
        }
        
        auto dealloc_end = std::chrono::high_resolution_clock::now();
        
        // Calculate results
        BenchmarkResults results;
        
        auto alloc_duration = std::chrono::duration_cast<std::chrono::nanoseconds>(
            alloc_end - start).count();
        auto dealloc_duration = std::chrono::duration_cast<std::chrono::nanoseconds>(
            dealloc_end - alloc_end).count();
        
        results.allocation_time_ns = static_cast<double>(alloc_duration) / num_iterations;
        results.deallocation_time_ns = static_cast<double>(dealloc_duration) / num_iterations;
        results.successful_allocations = ptrs.size();
        results.failed_allocations = num_iterations - ptrs.size();
        results.efficiency_percentage = 
            (static_cast<double>(ptrs.size()) / num_iterations) * 100.0;
        
        return results;
    }
    
    // Benchmark SIMD operations
    static double benchmark_simd_copy(size_t buffer_size, size_t iterations = 1000) {
        void* src = std::aligned_alloc(32, buffer_size);
        void* dst = std::aligned_alloc(32, buffer_size);
        
        auto start = std::chrono::high_resolution_clock::now();
        
        for (size_t i = 0; i < iterations; ++i) {
            SIMDMemoryOps::fast_copy(dst, src, buffer_size);
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        
        std::free(src);
        std::free(dst);
        
        auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
        double total_bytes = buffer_size * iterations * 2.0; // read + write
        double seconds = duration / 1e9;
        
        return (total_bytes / seconds) / (1024.0 * 1024.0 * 1024.0); // GB/s
    }
};

/**
 * @brief Memory health monitoring
 */
class HealthMonitor {
public:
    static void start_continuous_monitoring(std::chrono::seconds interval = std::chrono::seconds(30)) {
        std::thread monitor_thread([interval]() {
            while (monitoring_active_) {
                auto health = MemoryManager::instance().generate_health_report();
                
                if (health.has_memory_leaks || health.has_memory_corruption || health.has_performance_issues) {
                    std::cout << "⚠️  Memory Health Alert:\n";
                    
                    if (health.has_memory_leaks) {
                        std::cout << "  - Memory leaks: " << health.leaked_allocations 
                                 << " (" << health.leaked_bytes << " bytes)\n";
                    }
                    
                    if (health.has_memory_corruption) {
                        std::cout << "  - Memory corruption detected\n";
                    }
                    
                    if (health.has_performance_issues) {
                        std::cout << "  - Performance issues detected\n";
                    }
                    
                    for (const auto& warning : health.warnings) {
                        std::cout << "  - " << warning << "\n";
                    }
                }
                
                std::this_thread::sleep_for(interval);
            }
        });
        
        monitor_thread.detach();
    }
    
    static void stop_continuous_monitoring() {
        monitoring_active_ = false;
    }

private:
    static inline std::atomic<bool> monitoring_active_{true};
};

/**
 * @brief Print comprehensive system information
 */
inline void print_system_information() {
    std::cout << Version::NAME << " v" << Version::STRING << "\n";
    std::cout << Version::DESCRIPTION << "\n";
    std::cout << std::string(60, '=') << "\n\n";
    
    auto caps = SystemCapabilities::detect();
    caps.print();
    
    std::cout << "\nFeatures:\n";
    std::cout << "  ✓ Ultra-fast linear and pool allocators\n";
    std::cout << "  ✓ NUMA-aware allocation\n";
    std::cout << "  ✓ Thread-safe allocators with per-thread caching\n";
    std::cout << "  ✓ Size-class segregated pools with auto-scaling\n";
    std::cout << "  ✓ Real-time leak detection and profiling\n";
    std::cout << "  ✓ SIMD-optimized memory operations\n";
    std::cout << "  ✓ Memory protection and encryption\n";
    std::cout << "  ✓ Automatic memory pressure handling\n";
    std::cout << "  ✓ Comprehensive performance monitoring\n";
    std::cout << "\n";
}

} // namespace ecscope::memory

/**
 * @brief Convenience macros for common operations
 */

// Quick memory manager access
#define MEMORY_MANAGER ecscope::memory::MemoryManager::instance()

// Allocation with automatic tag
#define ALLOCATE_TAGGED(size, tag) \
    [&]() { \
        ecscope::memory::MemoryPolicy policy; \
        policy.allocation_tag = tag; \
        return MEMORY_MANAGER.allocate(size, policy); \
    }()

// RAII memory allocation
#define SCOPED_ALLOCATION(var, type, ...) \
    auto var = ecscope::memory::make_unique_memory_ptr<type>(__VA_ARGS__)

// Performance measurement
#define MEASURE_MEMORY_OP(name, code) \
    do { \
        auto start = std::chrono::high_resolution_clock::now(); \
        code; \
        auto end = std::chrono::high_resolution_clock::now(); \
        auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count(); \
        std::cout << name << " took " << duration << " ns\n"; \
    } while(0)

// Memory health check
#define CHECK_MEMORY_HEALTH() \
    do { \
        auto health = MEMORY_MANAGER.generate_health_report(); \
        if (health.has_memory_leaks || health.has_memory_corruption || health.has_performance_issues) { \
            std::cerr << "Memory health issues detected!\n"; \
            for (const auto& warning : health.warnings) { \
                std::cerr << "  - " << warning << "\n"; \
            } \
        } \
    } while(0)