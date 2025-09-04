#pragma once

/**
 * @file platform/system_integration.hpp
 * @brief System Integration Layer for Hardware-Aware ECScope Components
 * 
 * This integration layer connects the comprehensive hardware detection and
 * optimization system with existing ECScope components, enabling hardware-aware
 * optimizations across the entire engine. It provides seamless integration
 * with SIMD math, memory management, job system, and other core components.
 * 
 * Key Features:
 * - Automatic hardware-aware component initialization
 * - Dynamic optimization based on detected capabilities
 * - Integration with existing SIMD, memory, and threading systems
 * - Performance monitoring and adaptive optimization
 * - Cross-component optimization coordination
 * - Educational hardware impact demonstrations
 * 
 * Educational Value:
 * - Real-world hardware-aware engine design
 * - Component interaction optimization strategies
 * - Performance impact of hardware-aware design
 * - Adaptive system behavior demonstrations
 * - Cross-platform compatibility techniques
 * 
 * @author ECScope Educational ECS Framework - System Integration
 * @date 2025
 */

#include "hardware_detection.hpp"
#include "optimization_engine.hpp"
#include "thermal_power_manager.hpp"
#include "graphics_detection.hpp"
#include "performance_benchmark.hpp"
#include "../physics/simd_math.hpp"
#include "../memory/numa_manager.hpp"
#include "core/types.hpp"
#include <functional>
#include <memory>
#include <string>
#include <vector>
#include <unordered_map>

namespace ecscope::platform::integration {

//=============================================================================
// Hardware-Aware System Configuration
//=============================================================================

/**
 * @brief System-wide hardware configuration
 */
struct SystemHardwareConfig {
    // CPU configuration
    struct CpuConfig {
        u32 worker_thread_count = 0;           // 0 = auto-detect
        u32 numa_node_preference = UINT32_MAX;  // UINT32_MAX = auto
        bool enable_hyperthreading = true;
        bool enable_cpu_affinity = true;
        f32 cpu_utilization_target = 0.85f;    // Target CPU utilization
        
        // SIMD configuration
        std::string preferred_simd_level = "auto"; // "auto", "sse2", "avx2", "avx512", etc.
        bool enable_simd_fallback = true;
        bool enable_runtime_simd_detection = true;
    } cpu;
    
    // Memory configuration
    struct MemoryConfig {
        bool enable_numa_awareness = true;
        bool enable_large_pages = true;
        usize memory_pool_size_mb = 0;          // 0 = auto-size based on available memory
        f32 memory_usage_limit_percent = 80.0f; // Maximum memory usage
        
        // Memory layout optimization
        bool optimize_for_cache_locality = true;
        bool enable_memory_prefetching = true;
        std::string memory_layout_strategy = "auto"; // "aos", "soa", "auto"
    } memory;
    
    // Graphics configuration
    struct GraphicsConfig {
        bool enable_gpu_compute = true;
        bool prefer_discrete_gpu = true;
        std::string preferred_graphics_api = "auto"; // "auto", "vulkan", "opengl", "directx"
        bool enable_gpu_memory_management = true;
        f32 gpu_memory_usage_limit_percent = 90.0f;
    } graphics;
    
    // Thermal and power configuration
    struct ThermalPowerConfig {
        bool enable_thermal_monitoring = true;
        bool enable_adaptive_performance = true;
        f32 thermal_throttle_threshold = 85.0f;
        f32 power_usage_limit_percent = 95.0f;
        thermal::AdaptivePerformanceManager::ScalingStrategy scaling_strategy = 
            thermal::AdaptivePerformanceManager::ScalingStrategy::Balanced;
    } thermal_power;
    
    // Performance monitoring
    struct MonitoringConfig {
        bool enable_performance_monitoring = true;
        bool enable_hardware_counters = false; // Requires elevated privileges
        std::chrono::seconds monitoring_interval{1};
        bool save_performance_logs = false;
        std::string log_directory = "performance_logs";
    } monitoring;
    
    // Educational features
    struct EducationalConfig {
        bool enable_educational_mode = true;
        bool show_optimization_hints = true;
        bool demonstrate_hardware_impact = true;
        bool generate_performance_reports = true;
        std::string educational_level = "intermediate"; // "beginner", "intermediate", "advanced"
    } educational;
    
    // Auto-configuration based on detected hardware
    void auto_configure(const HardwareDetector& detector);
    std::string get_configuration_summary() const;
    void validate_configuration(const HardwareDetector& detector);
};

//=============================================================================
// Component Integration Interfaces
//=============================================================================

/**
 * @brief Interface for hardware-aware components
 */
class IHardwareAwareComponent {
public:
    virtual ~IHardwareAwareComponent() = default;
    
    // Component lifecycle
    virtual bool initialize(const HardwareDetector& detector) = 0;
    virtual void shutdown() = 0;
    virtual bool is_initialized() const = 0;
    
    // Hardware adaptation
    virtual void adapt_to_hardware(const HardwareDetector& detector) = 0;
    virtual void handle_thermal_event(thermal::ThermalState state) = 0;
    virtual void handle_power_event(thermal::PowerState state) = 0;
    
    // Performance optimization
    virtual std::vector<OptimizationRecommendation> get_optimization_recommendations(
        const HardwareDetector& detector) const = 0;
    virtual void apply_optimizations(const std::vector<std::string>& optimization_ids) = 0;
    
    // Monitoring and reporting
    virtual std::string get_component_name() const = 0;
    virtual std::string get_performance_status() const = 0;
    virtual std::unordered_map<std::string, f64> get_performance_metrics() const = 0;
};

/**
 * @brief Hardware-aware SIMD math integration
 */
class HardwareAwareSimdMath : public IHardwareAwareComponent {
private:
    std::unique_ptr<physics::simd::performance::AutoTuner> auto_tuner_;
    std::string current_simd_level_;
    bool fallback_enabled_ = true;
    
public:
    HardwareAwareSimdMath();
    ~HardwareAwareSimdMath() override;
    
    // IHardwareAwareComponent implementation
    bool initialize(const HardwareDetector& detector) override;
    void shutdown() override;
    bool is_initialized() const override;
    
    void adapt_to_hardware(const HardwareDetector& detector) override;
    void handle_thermal_event(thermal::ThermalState state) override;
    void handle_power_event(thermal::PowerState state) override;
    
    std::vector<OptimizationRecommendation> get_optimization_recommendations(
        const HardwareDetector& detector) const override;
    void apply_optimizations(const std::vector<std::string>& optimization_ids) override;
    
    std::string get_component_name() const override { return "SIMD Math System"; }
    std::string get_performance_status() const override;
    std::unordered_map<std::string, f64> get_performance_metrics() const override;
    
    // SIMD-specific methods
    const std::string& get_current_simd_level() const { return current_simd_level_; }
    void force_simd_level(const std::string& level);
    void benchmark_simd_performance();
    std::string get_simd_optimization_report() const;
    
private:
    std::string select_optimal_simd_level(const HardwareDetector& detector) const;
    void configure_simd_operations(const std::string& simd_level);
    void update_thermal_scaling(f32 scaling_factor);
};

/**
 * @brief Hardware-aware memory management integration
 */
class HardwareAwareMemoryManager : public IHardwareAwareComponent {
private:
    std::unique_ptr<memory::numa::NumaManager> numa_manager_;
    bool numa_enabled_ = false;
    bool large_pages_enabled_ = false;
    f32 current_memory_pressure_ = 0.0f;
    
public:
    HardwareAwareMemoryManager();
    ~HardwareAwareMemoryManager() override;
    
    // IHardwareAwareComponent implementation
    bool initialize(const HardwareDetector& detector) override;
    void shutdown() override;
    bool is_initialized() const override;
    
    void adapt_to_hardware(const HardwareDetector& detector) override;
    void handle_thermal_event(thermal::ThermalState state) override;
    void handle_power_event(thermal::PowerState state) override;
    
    std::vector<OptimizationRecommendation> get_optimization_recommendations(
        const HardwareDetector& detector) const override;
    void apply_optimizations(const std::vector<std::string>& optimization_ids) override;
    
    std::string get_component_name() const override { return "Memory Management System"; }
    std::string get_performance_status() const override;
    std::unordered_map<std::string, f64> get_performance_metrics() const override;
    
    // Memory-specific methods
    bool is_numa_enabled() const { return numa_enabled_; }
    bool is_large_pages_enabled() const { return large_pages_enabled_; }
    memory::numa::NumaManager* get_numa_manager() const { return numa_manager_.get(); }
    
    void optimize_memory_layout();
    void trigger_memory_cleanup();
    std::string get_memory_optimization_report() const;
    
private:
    void configure_numa_settings(const HardwareDetector& detector);
    void configure_memory_pools(const HardwareDetector& detector);
    void adjust_memory_pressure(f32 pressure);
};

/**
 * @brief Hardware-aware job system integration  
 */
class HardwareAwareJobSystem : public IHardwareAwareComponent {
private:
    u32 worker_thread_count_ = 0;
    u32 optimal_thread_count_ = 0;
    bool thread_affinity_enabled_ = false;
    std::vector<u32> thread_to_core_mapping_;
    f32 thermal_scaling_factor_ = 1.0f;
    
public:
    HardwareAwareJobSystem();
    ~HardwareAwareJobSystem() override;
    
    // IHardwareAwareComponent implementation
    bool initialize(const HardwareDetector& detector) override;
    void shutdown() override;
    bool is_initialized() const override;
    
    void adapt_to_hardware(const HardwareDetector& detector) override;
    void handle_thermal_event(thermal::ThermalState state) override;
    void handle_power_event(thermal::PowerState state) override;
    
    std::vector<OptimizationRecommendation> get_optimization_recommendations(
        const HardwareDetector& detector) const override;
    void apply_optimizations(const std::vector<std::string>& optimization_ids) override;
    
    std::string get_component_name() const override { return "Job System"; }
    std::string get_performance_status() const override;
    std::unordered_map<std::string, f64> get_performance_metrics() const override;
    
    // Job system specific methods
    u32 get_worker_thread_count() const { return worker_thread_count_; }
    u32 get_optimal_thread_count() const { return optimal_thread_count_; }
    bool is_thread_affinity_enabled() const { return thread_affinity_enabled_; }
    
    void set_worker_thread_count(u32 count);
    void enable_thread_affinity(bool enable);
    void benchmark_thread_scalability();
    std::string get_threading_optimization_report() const;
    
private:
    u32 calculate_optimal_thread_count(const HardwareDetector& detector) const;
    void configure_thread_affinity(const HardwareDetector& detector);
    void apply_thermal_scaling(f32 scaling_factor);
    void balance_thread_workload();
};

//=============================================================================
// System Integration Manager
//=============================================================================

/**
 * @brief Central manager for hardware-aware system integration
 */
class SystemIntegrationManager {
private:
    // Core detection systems
    std::unique_ptr<HardwareDetector> hardware_detector_;
    std::unique_ptr<graphics::GraphicsDetector> graphics_detector_;
    std::unique_ptr<thermal::ThermalPowerMonitor> thermal_monitor_;
    std::unique_ptr<OptimizationEngine> optimization_engine_;
    std::unique_ptr<benchmark::BenchmarkExecutor> benchmark_executor_;
    
    // Integrated components
    std::vector<std::unique_ptr<IHardwareAwareComponent>> components_;
    std::unordered_map<std::string, IHardwareAwareComponent*> component_registry_;
    
    // System configuration
    SystemHardwareConfig system_config_;
    bool is_initialized_ = false;
    
    // Performance monitoring
    std::atomic<bool> monitoring_active_{false};
    std::thread monitoring_thread_;
    std::chrono::steady_clock::time_point last_optimization_check_;
    
    // Event handling
    std::vector<std::function<void(thermal::ThermalState)>> thermal_event_handlers_;
    std::vector<std::function<void(thermal::PowerState)>> power_event_handlers_;
    std::mutex event_handler_mutex_;
    
public:
    SystemIntegrationManager();
    ~SystemIntegrationManager();
    
    // Non-copyable
    SystemIntegrationManager(const SystemIntegrationManager&) = delete;
    SystemIntegrationManager& operator=(const SystemIntegrationManager&) = delete;
    
    // System lifecycle
    bool initialize(const SystemHardwareConfig& config = {});
    void shutdown();
    bool is_initialized() const { return is_initialized_; }
    
    // Component management
    void register_component(std::unique_ptr<IHardwareAwareComponent> component);
    template<typename T>
    T* get_component(const std::string& name) const {
        auto it = component_registry_.find(name);
        return (it != component_registry_.end()) ? dynamic_cast<T*>(it->second) : nullptr;
    }
    
    std::vector<std::string> get_registered_components() const;
    
    // Hardware detection access
    const HardwareDetector& get_hardware_detector() const { return *hardware_detector_; }
    const graphics::GraphicsDetector& get_graphics_detector() const { return *graphics_detector_; }
    const thermal::ThermalPowerMonitor& get_thermal_monitor() const { return *thermal_monitor_; }
    const OptimizationEngine& get_optimization_engine() const { return *optimization_engine_; }
    const benchmark::BenchmarkExecutor& get_benchmark_executor() const { return *benchmark_executor_; }
    
    // System optimization
    void trigger_system_optimization();
    void apply_optimization_recommendations();
    std::vector<OptimizationRecommendation> get_system_wide_recommendations() const;
    
    // Performance monitoring
    void start_performance_monitoring();
    void stop_performance_monitoring();
    bool is_monitoring_active() const { return monitoring_active_.load(); }
    
    // Event handling
    void register_thermal_event_handler(std::function<void(thermal::ThermalState)> handler);
    void register_power_event_handler(std::function<void(thermal::PowerState)> handler);
    
    // Configuration
    void update_system_configuration(const SystemHardwareConfig& config);
    const SystemHardwareConfig& get_system_configuration() const { return system_config_; }
    
    // Reporting and analysis
    std::string generate_system_report() const;
    std::string generate_optimization_report() const;
    std::string generate_performance_analysis() const;
    
    // Benchmarking
    void run_system_benchmarks();
    void run_component_benchmarks(const std::string& component_name);
    std::string get_benchmark_results() const;
    
    // Educational features
    void demonstrate_hardware_integration();
    void show_optimization_impact();
    void compare_hardware_configurations();
    std::string get_educational_analysis() const;
    
private:
    // Initialization helpers
    bool initialize_detection_systems();
    bool initialize_optimization_systems();
    bool initialize_components();
    
    // Monitoring implementation
    void monitoring_worker();
    void check_for_hardware_changes();
    void update_component_optimizations();
    void handle_thermal_events();
    void handle_power_events();
    
    // Event dispatch
    void dispatch_thermal_event(thermal::ThermalState state);
    void dispatch_power_event(thermal::PowerState state);
    
    // Auto-configuration
    void auto_configure_system();
    void validate_system_configuration();
    
    // Performance analysis
    void analyze_component_performance();
    void identify_system_bottlenecks();
    void suggest_hardware_upgrades();
};

//=============================================================================
// Integration Utilities
//=============================================================================

/**
 * @brief Utility functions for system integration
 */
namespace utils {
    
    /**
     * @brief Hardware-aware thread configuration
     */
    struct ThreadConfiguration {
        u32 total_threads;
        u32 worker_threads;
        u32 io_threads;
        std::vector<u32> core_affinity;
        bool hyperthreading_beneficial;
        
        static ThreadConfiguration calculate_optimal(const HardwareDetector& detector);
        std::string get_description() const;
    };
    
    /**
     * @brief Memory layout recommendation
     */
    struct MemoryLayoutRecommendation {
        std::string layout_type;        // "AoS", "SoA", "Hybrid"
        usize cache_line_alignment;
        bool use_numa_awareness;
        bool use_large_pages;
        f32 expected_improvement;
        
        static MemoryLayoutRecommendation analyze(const HardwareDetector& detector,
                                                 const std::string& use_case);
        std::string get_implementation_guide() const;
    };
    
    /**
     * @brief SIMD optimization recommendation
     */
    struct SimdOptimizationRecommendation {
        std::string instruction_set;    // "SSE2", "AVX2", "AVX512", etc.
        std::vector<std::string> optimization_techniques;
        f32 expected_speedup;
        std::string code_example;
        
        static SimdOptimizationRecommendation analyze(const HardwareDetector& detector,
                                                     const std::string& operation_type);
        std::string get_optimization_guide() const;
    };
    
    // Utility functions
    std::string format_hardware_summary(const HardwareDetector& detector);
    std::vector<std::string> identify_performance_bottlenecks(
        const HardwareDetector& detector,
        const std::unordered_map<std::string, f64>& performance_metrics);
    f32 calculate_hardware_score(const HardwareDetector& detector);
    std::string generate_optimization_checklist(const HardwareDetector& detector);
    
} // namespace utils

//=============================================================================
// Global System Integration
//=============================================================================

/**
 * @brief Initialize the global system integration
 */
void initialize_system_integration(const SystemHardwareConfig& config = {});

/**
 * @brief Get the global system integration manager
 */
SystemIntegrationManager& get_system_integration_manager();

/**
 * @brief Shutdown the global system integration
 */
void shutdown_system_integration();

/**
 * @brief Quick system integration helpers
 */
namespace quick_integration {
    bool is_system_optimized();
    f32 get_system_performance_score();
    std::string get_optimization_status();
    std::vector<std::string> get_quick_optimization_tips();
    void apply_safe_optimizations();
    void benchmark_current_configuration();
    std::string get_hardware_compatibility_report();
}

} // namespace ecscope::platform::integration