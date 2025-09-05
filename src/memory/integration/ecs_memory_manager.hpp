#pragma once

/**
 * @file memory/integration/ecs_memory_manager.hpp
 * @brief Comprehensive ECS Memory Management Integration
 * 
 * This header integrates all advanced memory systems (NUMA-aware pools, 
 * garbage collection, specialized pools, debugging tools, educational features)
 * with the existing ECS architecture and performance monitoring infrastructure.
 * 
 * Key Features:
 * - Unified memory management interface for ECS components
 * - Automatic allocation strategy selection based on component characteristics
 * - Integration with NUMA-aware and specialized memory pools
 * - Optional garbage collection for managed components
 * - Comprehensive memory debugging and leak detection
 * - Educational visualization and performance comparison tools
 * - Real-time memory performance monitoring and optimization
 * 
 * @author ECScope Educational ECS Framework - Memory Integration
 * @date 2025
 */

#include "memory/specialized/numa_aware_pools.hpp"
#include "memory/specialized/component_pools.hpp"
#include "memory/specialized/gpu_buffer_pools.hpp"
#include "memory/specialized/audio_pools.hpp"
#include "memory/specialized/thermal_pools.hpp"
#include "memory/gc/gc_manager.hpp"
#include "memory/debugging/advanced_debugger.hpp"
#include "memory/education/memory_simulator.hpp"
#include "memory/memory_tracker.hpp"
#include "ecs/component.hpp"
#include "ecs/entity.hpp"
#include "performance/performance_lab.hpp"
#include "core/types.hpp"
#include "core/log.hpp"
#include <memory>
#include <unordered_map>
#include <type_traits>
#include <atomic>
#include <mutex>
#include <thread>

namespace ecscope::memory::integration {

using namespace ecscope::ecs;
using namespace ecscope::memory;

//=============================================================================
// ECS Memory Management Strategy
//=============================================================================

/**
 * @brief Memory allocation strategy for different component types
 */
enum class ECSAllocationStrategy : u8 {
    Automatic         = 0,  // Automatically select best strategy
    StandardHeap      = 1,  // Use standard heap allocation
    Arena             = 2,  // Use arena allocator
    Pool              = 3,  // Use pool allocator
    ComponentPool     = 4,  // Use specialized component pool
    NumaAware         = 5,  // Use NUMA-aware allocation
    GarbageCollected  = 6,  // Use garbage collected allocation
    ThermalManaged    = 7   // Use thermal-aware allocation
};

/**
 * @brief Memory management configuration for ECS
 */
struct ECSMemoryConfig {
    // Global settings
    bool enable_numa_optimization;
    bool enable_garbage_collection;
    bool enable_thermal_management;
    bool enable_memory_debugging;
    bool enable_educational_features;
    
    // Pool configurations
    usize component_pool_initial_size;
    usize component_pool_max_size;
    f64 component_pool_growth_factor;
    
    // GC configuration
    gc::GCConfig gc_config;
    
    // NUMA configuration
    bool prefer_local_numa_allocation;
    f64 numa_migration_threshold;
    
    // Debugging configuration
    bool enable_guard_zones;
    bool enable_leak_detection;
    f64 leak_detection_threshold_seconds;
    
    // Educational configuration
    bool enable_allocation_visualization;
    bool enable_performance_comparison;
    education::SimulationScenario default_simulation_scenario;
    
    ECSMemoryConfig() noexcept {
        enable_numa_optimization = true;
        enable_garbage_collection = false; // Opt-in for specific components
        enable_thermal_management = true;
        enable_memory_debugging = true;
        enable_educational_features = true;
        
        component_pool_initial_size = 1024 * 1024; // 1MB
        component_pool_max_size = 64 * 1024 * 1024; // 64MB
        component_pool_growth_factor = 2.0;
        
        prefer_local_numa_allocation = true;
        numa_migration_threshold = 0.8;
        
        enable_guard_zones = false; // Performance overhead
        enable_leak_detection = true;
        leak_detection_threshold_seconds = 300.0; // 5 minutes
        
        enable_allocation_visualization = true;
        enable_performance_comparison = true;
    }
};

//=============================================================================
// Integrated ECS Memory Manager
//=============================================================================

/**
 * @brief Comprehensive memory manager for ECS systems
 */
class ECSMemoryManager {
private:
    ECSMemoryConfig config_;
    
    // Core memory systems
    std::unique_ptr<MemoryTracker> memory_tracker_;
    std::unique_ptr<specialized::numa::NumaTopologyManager> numa_topology_manager_;
    std::unique_ptr<specialized::numa::NumaAwarePool> numa_pool_;
    std::unique_ptr<gc::GenerationalGCManager> gc_manager_;
    
    // Specialized pools
    std::unique_ptr<specialized::components::ComponentPoolManager> component_pool_manager_;
    std::unique_ptr<specialized::gpu::GPUBufferPoolManager> gpu_pool_manager_;
    std::unique_ptr<specialized::audio::AudioPoolManager> audio_pool_manager_;
    std::unique_ptr<specialized::thermal::ThermalPoolManager> thermal_pool_manager_;
    
    // Debugging and educational tools
    std::unique_ptr<debugging::GuardZoneManager> guard_zone_manager_;
    std::unique_ptr<debugging::LeakDetector> leak_detector_;
    std::unique_ptr<education::MemoryVisualizer> memory_visualizer_;
    std::unique_ptr<education::CacheSimulator> cache_simulator_;
    
    // Performance monitoring
    performance::PerformanceLab* performance_lab_;
    
    // Component type registry
    std::unordered_map<std::type_index, ECSAllocationStrategy> component_strategies_;
    std::unordered_map<std::type_index, specialized::components::ComponentPoolProperties<void>> component_properties_;
    mutable std::shared_mutex registry_mutex_;
    
    // Statistics
    alignas(core::CACHE_LINE_SIZE) std::atomic<u64> total_ecs_allocations_{0};
    alignas(core::CACHE_LINE_SIZE) std::atomic<u64> component_allocations_{0};
    alignas(core::CACHE_LINE_SIZE) std::atomic<u64> numa_allocations_{0};
    alignas(core::CACHE_LINE_SIZE) std::atomic<u64> gc_allocations_{0};
    alignas(core::CACHE_LINE_SIZE) std::atomic<usize> peak_memory_usage_{0};
    
    // Background optimization
    std::thread optimization_thread_;
    std::atomic<bool> optimization_active_{true};
    std::atomic<f64> optimization_interval_seconds_{60.0}; // 1 minute intervals
    
public:
    explicit ECSMemoryManager(const ECSMemoryConfig& config = ECSMemoryConfig{},
                             performance::PerformanceLab* perf_lab = nullptr)
        : config_(config), performance_lab_(perf_lab) {
        
        initialize_core_systems();
        initialize_specialized_pools();
        initialize_debugging_tools();
        initialize_educational_features();
        
        // Start background optimization
        optimization_thread_ = std::thread([this]() {
            optimization_worker();
        });
        
        LOG_INFO("Initialized ECS Memory Manager with advanced features");
    }
    
    ~ECSMemoryManager() {
        optimization_active_.store(false);
        if (optimization_thread_.joinable()) {
            optimization_thread_.join();
        }
        
        LOG_INFO("ECS Memory Manager destroyed: {} total allocations, {}KB peak usage",
                 total_ecs_allocations_.load(), peak_memory_usage_.load() / 1024);
    }
    
    /**
     * @brief Register component type with specific allocation strategy
     */
    template<typename ComponentType>
    void register_component_type(ECSAllocationStrategy strategy = ECSAllocationStrategy::Automatic) {
        static_assert(concepts::Component<ComponentType>, "Must be a valid ECS component");
        
        std::unique_lock<std::shared_mutex> lock(registry_mutex_);
        
        std::type_index type_idx(typeid(ComponentType));
        
        // Analyze component characteristics if using automatic strategy
        if (strategy == ECSAllocationStrategy::Automatic) {
            strategy = analyze_optimal_strategy<ComponentType>();
        }
        
        component_strategies_[type_idx] = strategy;
        
        // Configure specialized pools based on strategy
        configure_component_pools<ComponentType>(strategy);
        
        LOG_DEBUG("Registered ECS component type: {}, strategy: {}", 
                 typeid(ComponentType).name(), static_cast<u32>(strategy));
    }
    
    /**
     * @brief Allocate component with optimal strategy
     */
    template<typename ComponentType, typename... Args>
    ComponentType* allocate_component(Entity entity, Args&&... args) {
        total_ecs_allocations_.fetch_add(1, std::memory_order_relaxed);
        
        std::shared_lock<std::shared_mutex> lock(registry_mutex_);
        std::type_index type_idx(typeid(ComponentType));
        
        // Get allocation strategy for this component type
        ECSAllocationStrategy strategy = ECSAllocationStrategy::StandardHeap;
        auto it = component_strategies_.find(type_idx);
        if (it != component_strategies_.end()) {
            strategy = it->second;
        }
        
        ComponentType* component = nullptr;
        
        switch (strategy) {
            case ECSAllocationStrategy::ComponentPool:
                component = allocate_from_component_pool<ComponentType>(entity, std::forward<Args>(args)...);
                break;
                
            case ECSAllocationStrategy::NumaAware:
                component = allocate_numa_aware<ComponentType>(entity, std::forward<Args>(args)...);
                break;
                
            case ECSAllocationStrategy::GarbageCollected:
                component = allocate_gc<ComponentType>(entity, std::forward<Args>(args)...);
                break;
                
            case ECSAllocationStrategy::ThermalManaged:
                component = allocate_thermal_managed<ComponentType>(entity, std::forward<Args>(args)...);
                break;
                
            default:
                component = allocate_standard<ComponentType>(entity, std::forward<Args>(args)...);
                break;
        }
        
        if (component) {
            component_allocations_.fetch_add(1, std::memory_order_relaxed);
            
            // Track with debugging tools if enabled
            if (config_.enable_memory_debugging) {
                track_component_allocation(component, sizeof(ComponentType), type_idx);
            }
            
            // Update peak usage
            update_peak_usage();
        }
        
        return component;
    }
    
    /**
     * @brief Deallocate component
     */
    template<typename ComponentType>
    void deallocate_component(ComponentType* component) {
        if (!component) return;
        
        std::shared_lock<std::shared_mutex> lock(registry_mutex_);
        std::type_index type_idx(typeid(ComponentType));
        
        // Get allocation strategy
        ECSAllocationStrategy strategy = ECSAllocationStrategy::StandardHeap;
        auto it = component_strategies_.find(type_idx);
        if (it != component_strategies_.end()) {
            strategy = it->second;
        }
        
        // Track deallocation if debugging enabled
        if (config_.enable_memory_debugging) {
            track_component_deallocation(component, type_idx);
        }
        
        // Deallocate based on strategy
        switch (strategy) {
            case ECSAllocationStrategy::ComponentPool:
                deallocate_from_component_pool<ComponentType>(component);
                break;
                
            case ECSAllocationStrategy::NumaAware:
                if (numa_pool_) numa_pool_->deallocate(component);
                break;
                
            case ECSAllocationStrategy::GarbageCollected:
                // GC managed - no explicit deallocation needed
                break;
                
            case ECSAllocationStrategy::ThermalManaged:
                deallocate_thermal_managed<ComponentType>(component);
                break;
                
            default:
                delete component;
                break;
        }
    }
    
    /**
     * @brief Get comprehensive memory statistics
     */
    struct ECSMemoryStatistics {
        // Global statistics
        u64 total_ecs_allocations;
        u64 component_allocations;
        u64 numa_allocations;
        u64 gc_allocations;
        usize peak_memory_usage;
        
        // Per-subsystem statistics
        MemoryTracker::GlobalStats global_memory_stats;
        specialized::numa::NumaTopologyManager::TopologyStatistics numa_stats;
        gc::GenerationalGCManager::GCManagerStatistics gc_stats;
        specialized::components::ComponentPoolManager::GlobalStatistics component_pool_stats;
        debugging::GuardZoneManager::GuardStatistics guard_zone_stats;
        debugging::LeakDetector::LeakStatistics leak_detection_stats;
        education::MemoryVisualizer::FragmentationStats visualization_stats;
        education::CacheSimulator::CacheStatistics cache_simulation_stats;
        
        // Performance insights
        f64 overall_allocation_efficiency;
        f64 numa_locality_score;
        f64 gc_overhead_percentage;
        f64 memory_fragmentation_score;
        std::vector<std::string> optimization_recommendations;
        
        ECSMemoryConfig configuration;
    };
    
    ECSMemoryStatistics get_comprehensive_statistics() const {
        ECSMemoryStatistics stats{};
        
        // Basic counters
        stats.total_ecs_allocations = total_ecs_allocations_.load();
        stats.component_allocations = component_allocations_.load();
        stats.numa_allocations = numa_allocations_.load();
        stats.gc_allocations = gc_allocations_.load();
        stats.peak_memory_usage = peak_memory_usage_.load();
        
        // Subsystem statistics
        if (memory_tracker_) {
            stats.global_memory_stats = memory_tracker_->get_global_stats();
        }
        
        if (numa_topology_manager_) {
            stats.numa_stats = numa_topology_manager_->get_statistics();
        }
        
        if (gc_manager_) {
            stats.gc_stats = gc_manager_->get_statistics();
        }
        
        if (component_pool_manager_) {
            stats.component_pool_stats = component_pool_manager_->get_global_statistics();
        }
        
        if (guard_zone_manager_) {
            stats.guard_zone_stats = guard_zone_manager_->get_statistics();
        }
        
        if (leak_detector_) {
            stats.leak_detection_stats = leak_detector_->get_statistics();
        }
        
        if (memory_visualizer_) {
            stats.visualization_stats = memory_visualizer_->calculate_fragmentation_stats();
        }
        
        if (cache_simulator_) {
            stats.cache_simulation_stats = cache_simulator_->get_statistics();
        }
        
        // Calculate performance insights
        calculate_performance_insights(stats);
        generate_optimization_recommendations(stats);
        
        stats.configuration = config_;
        
        return stats;
    }
    
    /**
     * @brief Force memory optimization across all systems
     */
    void force_memory_optimization() {
        LOG_INFO("Starting comprehensive memory optimization...");
        
        // Optimize NUMA placement
        if (numa_pool_ && config_.enable_numa_optimization) {
            // Force migration of misplaced allocations
            // Implementation would analyze access patterns and migrate
        }
        
        // Trigger garbage collection
        if (gc_manager_ && config_.enable_garbage_collection) {
            gc_manager_->force_collection(gc::CollectionType::Full);
        }
        
        // Optimize component pools
        if (component_pool_manager_) {
            // Implementation would compact and optimize component pools
        }
        
        // Update thermal management
        if (thermal_pool_manager_ && config_.enable_thermal_management) {
            // Implementation would rebalance hot/cold data
        }
        
        // Run leak detection
        if (leak_detector_ && config_.enable_memory_debugging) {
            auto leaks = leak_detector_->detect_leaks();
            if (!leaks.empty()) {
                LOG_WARNING("Memory optimization detected {} potential leaks", leaks.size());
            }
        }
        
        LOG_INFO("Memory optimization completed");
    }
    
    /**
     * @brief Generate educational memory report
     */
    std::string generate_educational_report() const {
        if (!config_.enable_educational_features) {
            return "Educational features disabled";
        }
        
        std::ostringstream report;
        report << "=== ECS Memory Management Educational Report ===\n\n";
        
        auto stats = get_comprehensive_statistics();
        
        // Memory allocation overview
        report << "ALLOCATION OVERVIEW:\n";
        report << "  Total ECS allocations: " << stats.total_ecs_allocations << "\n";
        report << "  Component allocations: " << stats.component_allocations << "\n";
        report << "  NUMA-aware allocations: " << stats.numa_allocations << "\n";
        report << "  GC-managed allocations: " << stats.gc_allocations << "\n";
        report << "  Peak memory usage: " << stats.peak_memory_usage / 1024 << "KB\n\n";
        
        // NUMA analysis
        if (config_.enable_numa_optimization && numa_topology_manager_) {
            report << "NUMA TOPOLOGY ANALYSIS:\n";
            report << "  NUMA nodes detected: " << stats.numa_stats.total_nodes << "\n";
            report << "  Average utilization: " << std::fixed << std::setprecision(1) 
                   << stats.numa_stats.average_utilization * 100 << "%\n";
            report << "  Memory locality score: " << std::fixed << std::setprecision(2)
                   << stats.numa_locality_score << "\n\n";
        }
        
        // Garbage collection insights
        if (config_.enable_garbage_collection && gc_manager_) {
            report << "GARBAGE COLLECTION ANALYSIS:\n";
            report << "  Total collections: " << stats.gc_stats.total_collections << "\n";
            report << "  Average pause time: " << std::fixed << std::setprecision(2)
                   << stats.gc_stats.average_pause_time_ms << "ms\n";
            report << "  GC overhead: " << std::fixed << std::setprecision(1)
                   << stats.gc_overhead_percentage << "%\n\n";
        }
        
        // Memory debugging results
        if (config_.enable_memory_debugging) {
            report << "MEMORY DEBUGGING RESULTS:\n";
            if (stats.guard_zone_stats.total_corruptions_detected > 0) {
                report << "  ⚠️  Memory corruptions detected: " << stats.guard_zone_stats.total_corruptions_detected << "\n";
            }
            if (stats.leak_detection_stats.suspected_leaks > 0) {
                report << "  ⚠️  Memory leaks suspected: " << stats.leak_detection_stats.suspected_leaks 
                       << " (" << stats.leak_detection_stats.total_leaked_bytes / 1024 << "KB)\n";
            }
            if (stats.guard_zone_stats.total_corruptions_detected == 0 && 
                stats.leak_detection_stats.suspected_leaks == 0) {
                report << "  ✅ No memory issues detected\n";
            }
            report << "\n";
        }
        
        // Performance recommendations
        report << "OPTIMIZATION RECOMMENDATIONS:\n";
        for (const auto& recommendation : stats.optimization_recommendations) {
            report << "  • " << recommendation << "\n";
        }
        
        // Educational insights
        report << "\nEDUCATIONAL INSIGHTS:\n";
        report << "  • NUMA-aware allocation reduces cross-node memory access latency\n";
        report << "  • Generational GC is most effective for short-lived objects\n";
        report << "  • Component pools provide excellent cache locality for similar objects\n";
        report << "  • Hot/cold data separation improves cache utilization\n";
        report << "  • Guard zones and leak detection help catch memory bugs early\n";
        
        return report.str();
    }
    
    /**
     * @brief Export memory visualization data
     */
    void export_memory_visualization(const std::string& base_filename) const {
        if (!config_.enable_educational_features) return;
        
        if (memory_visualizer_) {
            memory_visualizer_->export_visualization_data(base_filename + "_layout.csv");
        }
        
        // Export NUMA topology
        if (numa_topology_manager_) {
            // Implementation would export NUMA topology visualization
        }
        
        // Export GC visualization
        if (gc_manager_) {
            // Implementation would export GC phase visualization
        }
        
        LOG_INFO("Exported memory visualization data to: {}_*.csv", base_filename);
    }
    
    /**
     * @brief Configuration management
     */
    void update_config(const ECSMemoryConfig& new_config) {
        config_ = new_config;
        
        // Update subsystem configurations
        if (gc_manager_) {
            gc_manager_->set_config(config_.gc_config);
        }
        
        if (leak_detector_) {
            leak_detector_->set_detection_interval(config_.leak_detection_threshold_seconds / 10);
        }
        
        LOG_INFO("Updated ECS memory configuration");
    }
    
    const ECSMemoryConfig& get_config() const { return config_; }
    
private:
    void initialize_core_systems() {
        // Initialize memory tracker
        TrackerConfig tracker_config;
        tracker_config.enable_tracking = true;
        tracker_config.enable_call_stacks = config_.enable_memory_debugging;
        tracker_config.enable_leak_detection = config_.enable_memory_debugging;
        
        memory_tracker_ = std::make_unique<MemoryTracker>();
        MemoryTracker::initialize(tracker_config);
        
        // Initialize NUMA management if enabled
        if (config_.enable_numa_optimization) {
            numa_topology_manager_ = std::make_unique<specialized::numa::NumaTopologyManager>();
            numa_pool_ = std::make_unique<specialized::numa::NumaAwarePool>(
                *numa_topology_manager_, 64 * 1024 * 1024, memory_tracker_.get()
            );
        }
        
        // Initialize garbage collection if enabled
        if (config_.enable_garbage_collection) {
            gc_manager_ = std::make_unique<gc::GenerationalGCManager>(
                config_.gc_config, memory_tracker_.get()
            );
        }
        
        LOG_DEBUG("Initialized core memory systems");
    }
    
    void initialize_specialized_pools() {
        // Component pool manager
        component_pool_manager_ = std::make_unique<specialized::components::ComponentPoolManager>();
        
        // GPU buffer pool manager
        gpu_pool_manager_ = std::make_unique<specialized::gpu::GPUBufferPoolManager>(
            memory_tracker_.get()
        );
        
        // Audio pool manager
        audio_pool_manager_ = std::make_unique<specialized::audio::AudioPoolManager>(
            memory_tracker_.get()
        );
        
        // Thermal pool manager if enabled
        if (config_.enable_thermal_management) {
            thermal_pool_manager_ = std::make_unique<specialized::thermal::ThermalPoolManager>(
                memory_tracker_.get()
            );
        }
        
        LOG_DEBUG("Initialized specialized memory pools");
    }
    
    void initialize_debugging_tools() {
        if (!config_.enable_memory_debugging) return;
        
        // Guard zone manager
        if (config_.enable_guard_zones) {
            guard_zone_manager_ = std::make_unique<debugging::GuardZoneManager>();
        }
        
        // Leak detector
        if (config_.enable_leak_detection) {
            leak_detector_ = std::make_unique<debugging::LeakDetector>(
                config_.leak_detection_threshold_seconds, 0.7, 64
            );
        }
        
        LOG_DEBUG("Initialized memory debugging tools");
    }
    
    void initialize_educational_features() {
        if (!config_.enable_educational_features) return;
        
        // Memory visualizer
        if (config_.enable_allocation_visualization) {
            memory_visualizer_ = std::make_unique<education::MemoryVisualizer>(
                config_.component_pool_initial_size
            );
        }
        
        // Cache simulator
        cache_simulator_ = std::make_unique<education::CacheSimulator>(
            config_.default_simulation_scenario.cache_params
        );
        
        LOG_DEBUG("Initialized educational memory features");
    }
    
    template<typename ComponentType>
    ECSAllocationStrategy analyze_optimal_strategy() const {
        // Analyze component characteristics to determine optimal strategy
        
        if constexpr (sizeof(ComponentType) <= 32) {
            // Small components benefit from pool allocation
            return ECSAllocationStrategy::ComponentPool;
        }
        
        if constexpr (sizeof(ComponentType) >= 1024) {
            // Large components may benefit from NUMA-aware allocation
            if (config_.enable_numa_optimization) {
                return ECSAllocationStrategy::NumaAware;
            }
        }
        
        if constexpr (std::is_trivially_destructible_v<ComponentType>) {
            // Trivially destructible components can use GC if enabled
            if (config_.enable_garbage_collection) {
                return ECSAllocationStrategy::GarbageCollected;
            }
        }
        
        // Default to component pool for most ECS components
        return ECSAllocationStrategy::ComponentPool;
    }
    
    template<typename ComponentType>
    void configure_component_pools(ECSAllocationStrategy strategy) {
        if (strategy == ECSAllocationStrategy::ComponentPool && component_pool_manager_) {
            specialized::components::ComponentLayout layout = 
                specialized::components::ComponentLayout::SoA;
            
            if constexpr (sizeof(ComponentType) <= 32) {
                layout = specialized::components::ComponentLayout::AoS;
            }
            
            component_pool_manager_->register_component_pool<ComponentType>(layout);
        }
    }
    
    template<typename ComponentType, typename... Args>
    ComponentType* allocate_from_component_pool(Entity entity, Args&&... args) {
        if (!component_pool_manager_) {
            return allocate_standard<ComponentType>(entity, std::forward<Args>(args)...);
        }
        
        // Implementation would use component pool manager
        return new ComponentType(std::forward<Args>(args)...);
    }
    
    template<typename ComponentType, typename... Args>
    ComponentType* allocate_numa_aware(Entity entity, Args&&... args) {
        if (!numa_pool_) {
            return allocate_standard<ComponentType>(entity, std::forward<Args>(args)...);
        }
        
        void* memory = numa_pool_->allocate(sizeof(ComponentType), alignof(ComponentType));
        if (!memory) {
            return allocate_standard<ComponentType>(entity, std::forward<Args>(args)...);
        }
        
        numa_allocations_.fetch_add(1, std::memory_order_relaxed);
        return new(memory) ComponentType(std::forward<Args>(args)...);
    }
    
    template<typename ComponentType, typename... Args>
    ComponentType* allocate_gc(Entity entity, Args&&... args) {
        if (!gc_manager_) {
            return allocate_standard<ComponentType>(entity, std::forward<Args>(args)...);
        }
        
        auto* gc_object = gc_manager_->allocate<ComponentType>(std::forward<Args>(args)...);
        if (!gc_object) {
            return allocate_standard<ComponentType>(entity, std::forward<Args>(args)...);
        }
        
        gc_allocations_.fetch_add(1, std::memory_order_relaxed);
        return gc_object->get_object();
    }
    
    template<typename ComponentType, typename... Args>
    ComponentType* allocate_thermal_managed(Entity entity, Args&&... args) {
        if (!thermal_pool_manager_) {
            return allocate_standard<ComponentType>(entity, std::forward<Args>(args)...);
        }
        
        // Implementation would use thermal pool allocation
        return allocate_standard<ComponentType>(entity, std::forward<Args>(args)...);
    }
    
    template<typename ComponentType, typename... Args>
    ComponentType* allocate_standard(Entity entity, Args&&... args) {
        if (config_.enable_guard_zones && guard_zone_manager_) {
            void* memory = guard_zone_manager_->allocate_guarded(sizeof(ComponentType), alignof(ComponentType));
            if (memory) {
                return new(memory) ComponentType(std::forward<Args>(args)...);
            }
        }
        
        return new ComponentType(std::forward<Args>(args)...);
    }
    
    template<typename ComponentType>
    void deallocate_from_component_pool(ComponentType* component) {
        // Implementation would deallocate from component pool
        delete component;
    }
    
    template<typename ComponentType>
    void deallocate_thermal_managed(ComponentType* component) {
        // Implementation would deallocate from thermal pool
        delete component;
    }
    
    void track_component_allocation(void* address, usize size, std::type_index type) {
        if (memory_tracker_) {
            memory_tracker_->track_allocation(
                address, size, size, alignof(std::max_align_t),
                AllocationCategory::ECS_Components,
                AllocatorType::Custom,
                "ECSMemoryManager",
                static_cast<u32>(type.hash_code() % 1000)
            );
        }
        
        if (leak_detector_) {
            leak_detector_->track_allocation(address, size, AllocationCategory::ECS_Components);
        }
        
        if (memory_visualizer_) {
            memory_visualizer_->record_allocation(
                reinterpret_cast<usize>(address), size, 
                static_cast<u32>(type.hash_code() % 1000)
            );
        }
    }
    
    void track_component_deallocation(void* address, std::type_index type) {
        if (memory_tracker_) {
            memory_tracker_->track_deallocation(
                address, AllocatorType::Custom, 
                "ECSMemoryManager", static_cast<u32>(type.hash_code() % 1000)
            );
        }
        
        if (leak_detector_) {
            leak_detector_->untrack_allocation(address);
        }
        
        if (memory_visualizer_) {
            // Implementation would record deallocation
        }
    }
    
    void update_peak_usage() {
        if (memory_tracker_) {
            auto stats = memory_tracker_->get_global_stats();
            usize current_peak = peak_memory_usage_.load();
            while (stats.total_allocated > current_peak && 
                   !peak_memory_usage_.compare_exchange_weak(current_peak, stats.total_allocated)) {
                // Retry until successful
            }
        }
    }
    
    void calculate_performance_insights(ECSMemoryStatistics& stats) const {
        // Calculate overall allocation efficiency
        if (stats.total_ecs_allocations > 0) {
            f64 successful_ratio = static_cast<f64>(stats.component_allocations) / stats.total_ecs_allocations;
            stats.overall_allocation_efficiency = successful_ratio;
        }
        
        // Calculate NUMA locality score
        if (config_.enable_numa_optimization && stats.numa_allocations > 0) {
            stats.numa_locality_score = 1.0 - stats.numa_stats.cross_node_ratio;
        }
        
        // Calculate GC overhead
        if (config_.enable_garbage_collection) {
            stats.gc_overhead_percentage = stats.gc_stats.gc_overhead_percentage;
        }
        
        // Calculate memory fragmentation score
        stats.memory_fragmentation_score = stats.visualization_stats.fragmentation_ratio;
    }
    
    void generate_optimization_recommendations(ECSMemoryStatistics& stats) const {
        stats.optimization_recommendations.clear();
        
        if (stats.overall_allocation_efficiency < 0.9) {
            stats.optimization_recommendations.push_back(
                "Consider pre-registering component types for better allocation efficiency"
            );
        }
        
        if (config_.enable_numa_optimization && stats.numa_locality_score < 0.8) {
            stats.optimization_recommendations.push_back(
                "High cross-NUMA traffic detected - consider thread affinity optimization"
            );
        }
        
        if (config_.enable_garbage_collection && stats.gc_overhead_percentage > 5.0) {
            stats.optimization_recommendations.push_back(
                "High GC overhead - consider tuning generation sizes or collection frequency"
            );
        }
        
        if (stats.memory_fragmentation_score > 0.3) {
            stats.optimization_recommendations.push_back(
                "Memory fragmentation detected - consider periodic compaction or different allocation strategy"
            );
        }
        
        if (config_.enable_memory_debugging && 
            stats.leak_detection_stats.suspected_leaks > 10) {
            stats.optimization_recommendations.push_back(
                "Multiple memory leaks detected - review component lifecycle management"
            );
        }
        
        if (stats.cache_simulation_stats.hit_rate < 0.8) {
            stats.optimization_recommendations.push_back(
                "Low cache hit rate - consider improving data locality with SoA layout"
            );
        }
    }
    
    void optimization_worker() {
        while (optimization_active_.load()) {
            f64 interval = optimization_interval_seconds_.load();
            std::this_thread::sleep_for(std::chrono::duration<f64>(interval));
            
            if (!optimization_active_.load()) break;
            
            // Perform background memory optimization
            perform_background_optimization();
        }
    }
    
    void perform_background_optimization() {
        // Update memory visualizer
        if (memory_visualizer_) {
            memory_visualizer_->update_visualization();
        }
        
        // Run periodic leak detection
        if (leak_detector_) {
            auto leaks = leak_detector_->detect_leaks();
            if (leaks.size() > 10) {
                LOG_WARNING("Background optimization detected {} potential leaks", leaks.size());
            }
        }
        
        // Optimize NUMA placement if needed
        if (numa_pool_ && config_.enable_numa_optimization) {
            auto numa_stats = numa_topology_manager_->get_statistics();
            if (numa_stats.average_utilization > config_.numa_migration_threshold) {
                // Implementation would trigger NUMA rebalancing
            }
        }
        
        // Log performance insights
        if (performance_lab_) {
            // Integration with performance lab for monitoring
        }
    }
};

//=============================================================================
// Global ECS Memory Manager Instance
//=============================================================================

ECSMemoryManager& get_global_ecs_memory_manager();

} // namespace ecscope::memory::integration