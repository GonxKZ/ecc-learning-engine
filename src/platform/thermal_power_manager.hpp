#pragma once

/**
 * @file platform/thermal_power_manager.hpp
 * @brief Advanced Thermal and Power Management System for Mobile and Desktop Platforms
 * 
 * This system provides comprehensive thermal monitoring, power management, and
 * performance scaling capabilities across different platforms. It's especially
 * important for mobile devices and laptops where thermal throttling and battery
 * life are critical considerations for optimal performance.
 * 
 * Key Features:
 * - Real-time temperature monitoring (CPU, GPU, system)
 * - Power consumption tracking and analysis
 * - Thermal throttling detection and mitigation
 * - Battery life optimization strategies
 * - Performance scaling based on thermal/power constraints
 * - Platform-specific power management integration
 * - Educational thermal management demonstrations
 * 
 * Educational Value:
 * - Thermal management impact on performance
 * - Power consumption vs performance trade-offs
 * - Mobile platform optimization strategies
 * - Thermal throttling behavior analysis
 * - Battery optimization techniques
 * - Cooling system effectiveness analysis
 * 
 * @author ECScope Educational ECS Framework - Thermal/Power Management
 * @date 2025
 */

#include "hardware_detection.hpp"
#include "core/types.hpp"
#include <chrono>
#include <functional>
#include <vector>
#include <atomic>
#include <thread>
#include <mutex>
#include <optional>

// Platform-specific includes
#ifdef ECSCOPE_PLATFORM_WINDOWS
    #include <windows.h>
    #include <powerbase.h>
    #include <powrprof.h>
    #include <winnt.h>
#endif

#ifdef ECSCOPE_PLATFORM_LINUX
    #include <unistd.h>
    #include <fcntl.h>
    #include <sys/stat.h>
#endif

#ifdef ECSCOPE_PLATFORM_MACOS
    #include <IOKit/IOKitLib.h>
    #include <IOKit/ps/IOPowerSources.h>
    #include <IOKit/ps/IOPSKeys.h>
#endif

namespace ecscope::platform::thermal {

//=============================================================================
// Thermal and Power State Enumerations
//=============================================================================

/**
 * @brief System thermal states
 */
enum class ThermalState : u8 {
    Unknown,
    Cool,           // Normal operating temperature
    Nominal,        // Optimal temperature range
    Warm,           // Elevated but acceptable temperature
    Hot,            // High temperature, performance may be affected
    Critical,       // Critical temperature, emergency measures needed
    Throttled,      // Currently thermal throttling
    Emergency       // Emergency shutdown imminent
};

/**
 * @brief Power management states
 */
enum class PowerState : u8 {
    Unknown,
    Maximum_Performance,    // No power constraints
    Balanced,              // Balance between performance and power
    Power_Saver,           // Minimize power consumption
    Eco_Mode,              // Extreme power saving
    Gaming_Mode,           // Optimize for sustained performance
    Battery_Saver,         // Emergency power saving
    Thermal_Throttled      // Reduced performance due to thermal constraints
};

/**
 * @brief Battery charge states
 */
enum class BatteryState : u8 {
    Unknown,
    Charging,
    Discharging,
    Full,
    Low,
    Critical,
    Not_Present
};

/**
 * @brief Cooling system types
 */
enum class CoolingSystemType : u8 {
    Unknown,
    Passive,               // Heat sinks, thermal pads
    Active_Air,            // Fans
    Active_Liquid,         // Liquid cooling
    Hybrid,                // Multiple cooling methods
    Thermal_Throttling,    // Software-based thermal management
    None                   // No active cooling (typical mobile)
};

//=============================================================================
// Temperature Monitoring Structures
//=============================================================================

/**
 * @brief Temperature sensor information
 */
struct TemperatureSensor {
    std::string sensor_id;
    std::string sensor_name;
    std::string sensor_type;      // "CPU", "GPU", "System", "Battery", etc.
    std::string location;         // Physical location description
    
    f32 current_temperature_celsius = 0.0f;
    f32 max_temperature_celsius = 0.0f;
    f32 critical_temperature_celsius = 0.0f;
    f32 throttle_temperature_celsius = 0.0f;
    
    bool is_available = false;
    bool supports_alerts = false;
    std::chrono::steady_clock::time_point last_reading_time;
    
    // Temperature history for trend analysis
    static constexpr usize HISTORY_SIZE = 60; // Last 60 readings
    std::array<f32, HISTORY_SIZE> temperature_history{};
    usize history_index = 0;
    
    void record_temperature(f32 temperature);
    f32 get_average_temperature(usize samples = HISTORY_SIZE) const;
    f32 get_temperature_trend() const; // Rate of change per second
    bool is_temperature_rising() const;
    bool is_overheating() const;
    std::string get_thermal_status() const;
};

/**
 * @brief Comprehensive thermal information
 */
struct ThermalInfo {
    std::vector<TemperatureSensor> sensors;
    
    // Primary temperature readings
    f32 cpu_temperature_celsius = 0.0f;
    f32 gpu_temperature_celsius = 0.0f;
    f32 system_temperature_celsius = 0.0f;
    f32 battery_temperature_celsius = 0.0f;
    
    // Thermal management state
    ThermalState current_state = ThermalState::Unknown;
    bool is_thermal_throttling = false;
    f32 throttling_factor = 1.0f; // 1.0 = no throttling, 0.5 = 50% throttling
    
    // Cooling system information
    CoolingSystemType cooling_type = CoolingSystemType::Unknown;
    std::vector<std::string> active_cooling_methods;
    f32 fan_speed_percent = 0.0f;
    bool cooling_system_active = false;
    
    // Thermal limits and thresholds
    f32 thermal_throttle_threshold = 85.0f;
    f32 critical_shutdown_threshold = 95.0f;
    f32 thermal_hysteresis = 5.0f; // Degrees for state transitions
    
    // Methods
    const TemperatureSensor* find_sensor(const std::string& type) const;
    f32 get_highest_temperature() const;
    ThermalState calculate_thermal_state() const;
    f32 get_thermal_headroom() const; // Degrees below throttling
    bool needs_cooling() const;
    std::string get_thermal_summary() const;
    std::vector<std::string> get_cooling_recommendations() const;
};

//=============================================================================
// Power Management Structures
//=============================================================================

/**
 * @brief Power consumption sensor
 */
struct PowerSensor {
    std::string sensor_id;
    std::string sensor_name;
    std::string component;        // "CPU", "GPU", "System", "Memory", etc.
    
    f32 current_power_watts = 0.0f;
    f32 average_power_watts = 0.0f;
    f32 peak_power_watts = 0.0f;
    f32 tdp_watts = 0.0f;         // Thermal Design Power
    
    bool is_available = false;
    std::chrono::steady_clock::time_point last_reading_time;
    
    // Power history for analysis
    static constexpr usize HISTORY_SIZE = 120; // 2 minutes of history
    std::array<f32, HISTORY_SIZE> power_history{};
    usize history_index = 0;
    
    void record_power(f32 power_watts);
    f32 get_average_power(usize samples = HISTORY_SIZE) const;
    f32 get_power_efficiency() const; // Performance per watt estimate
    std::string get_power_status() const;
};

/**
 * @brief Battery information
 */
struct BatteryInfo {
    std::string battery_name;
    std::string manufacturer;
    std::string chemistry;        // "Li-ion", "Li-Po", etc.
    
    BatteryState state = BatteryState::Unknown;
    f32 charge_level_percent = 0.0f;
    f32 capacity_wh = 0.0f;       // Total capacity in watt-hours
    f32 remaining_capacity_wh = 0.0f;
    f32 voltage_v = 0.0f;
    f32 current_ma = 0.0f;        // Positive = charging, negative = discharging
    f32 power_consumption_w = 0.0f;
    f32 temperature_celsius = 0.0f;
    
    // Battery health
    f32 design_capacity_wh = 0.0f;
    f32 health_percent = 100.0f;  // Based on capacity degradation
    u32 cycle_count = 0;
    
    // Time estimates
    std::chrono::minutes estimated_runtime{0};
    std::chrono::minutes estimated_charge_time{0};
    
    bool is_present = false;
    bool is_charging = false;
    bool is_critical = false;
    bool supports_fast_charging = false;
    
    // Methods
    f32 get_discharge_rate_w() const;
    f32 get_charge_rate_w() const;
    std::chrono::minutes calculate_remaining_runtime() const;
    std::chrono::minutes calculate_charge_time() const;
    std::string get_battery_status() const;
    std::vector<std::string> get_battery_optimization_tips() const;
};

/**
 * @brief Comprehensive power information
 */
struct PowerInfo {
    std::vector<PowerSensor> sensors;
    std::optional<BatteryInfo> battery;
    
    // System power consumption
    f32 total_system_power_w = 0.0f;
    f32 cpu_power_w = 0.0f;
    f32 gpu_power_w = 0.0f;
    f32 memory_power_w = 0.0f;
    f32 storage_power_w = 0.0f;
    f32 display_power_w = 0.0f;
    f32 other_power_w = 0.0f;
    
    // Power management state
    PowerState current_state = PowerState::Unknown;
    std::string power_plan = "Balanced";
    bool is_power_saving_enabled = false;
    bool is_on_battery_power = false;
    bool is_low_battery = false;
    
    // Power efficiency metrics
    f32 performance_per_watt = 0.0f;
    f32 power_efficiency_score = 0.0f;
    
    // Methods
    const PowerSensor* find_sensor(const std::string& component) const;
    f32 get_total_power_consumption() const;
    f32 calculate_power_efficiency() const;
    PowerState recommend_power_state() const;
    std::string get_power_summary() const;
    std::vector<std::string> get_power_optimization_recommendations() const;
};

//=============================================================================
// Thermal and Power Monitoring
//=============================================================================

/**
 * @brief Real-time thermal and power monitoring system
 */
class ThermalPowerMonitor {
private:
    // Monitoring configuration
    std::chrono::milliseconds monitoring_interval_{1000}; // 1 second default
    std::atomic<bool> monitoring_active_{false};
    std::thread monitoring_thread_;
    
    // Current state
    ThermalInfo current_thermal_info_;
    PowerInfo current_power_info_;
    mutable std::shared_mutex state_mutex_;
    
    // Historical data
    struct HistoryEntry {
        std::chrono::steady_clock::time_point timestamp;
        ThermalInfo thermal_info;
        PowerInfo power_info;
        f32 performance_impact; // 0-1, where 1 = no impact
    };
    
    static constexpr usize MAX_HISTORY_SIZE = 3600; // 1 hour of history
    std::vector<HistoryEntry> monitoring_history_;
    mutable std::mutex history_mutex_;
    
    // Alert system
    struct ThermalAlert {
        std::string alert_id;
        ThermalState trigger_state;
        std::function<void(const ThermalInfo&)> callback;
        bool is_enabled = true;
    };
    
    struct PowerAlert {
        std::string alert_id;
        f32 power_threshold_w;
        f32 battery_threshold_percent;
        std::function<void(const PowerInfo&)> callback;
        bool is_enabled = true;
    };
    
    std::vector<ThermalAlert> thermal_alerts_;
    std::vector<PowerAlert> power_alerts_;
    mutable std::mutex alert_mutex_;
    
public:
    ThermalPowerMonitor();
    ~ThermalPowerMonitor();
    
    // Non-copyable
    ThermalPowerMonitor(const ThermalPowerMonitor&) = delete;
    ThermalPowerMonitor& operator=(const ThermalPowerMonitor&) = delete;
    
    // Monitoring control
    bool initialize();
    void shutdown();
    void start_monitoring();
    void stop_monitoring();
    bool is_monitoring() const { return monitoring_active_.load(); }
    
    void set_monitoring_interval(std::chrono::milliseconds interval);
    std::chrono::milliseconds get_monitoring_interval() const;
    
    // Current state access
    ThermalInfo get_current_thermal_info() const;
    PowerInfo get_current_power_info() const;
    
    // Manual readings (for one-time queries)
    ThermalInfo read_thermal_sensors() const;
    PowerInfo read_power_sensors() const;
    
    // Alert system
    void register_thermal_alert(const std::string& alert_id, ThermalState trigger_state,
                               std::function<void(const ThermalInfo&)> callback);
    void register_power_alert(const std::string& alert_id, f32 power_threshold_w,
                             f32 battery_threshold_percent,
                             std::function<void(const PowerInfo&)> callback);
    void remove_alert(const std::string& alert_id);
    void enable_alert(const std::string& alert_id, bool enable);
    
    // Historical data access
    std::vector<HistoryEntry> get_history(std::chrono::minutes duration) const;
    std::vector<HistoryEntry> get_recent_history(usize count = 60) const;
    void clear_history();
    
    // Analysis methods
    f32 calculate_thermal_stability() const;
    f32 calculate_power_efficiency_trend() const;
    std::vector<std::string> analyze_thermal_patterns() const;
    std::vector<std::string> analyze_power_patterns() const;
    
    // Performance impact analysis
    f32 estimate_thermal_performance_impact() const;
    f32 estimate_power_performance_impact() const;
    std::string get_performance_optimization_suggestions() const;
    
private:
    // Monitoring worker
    void monitoring_worker();
    void update_thermal_info();
    void update_power_info();
    void check_alerts();
    void record_history_entry();
    
    // Platform-specific implementations
    ThermalInfo detect_thermal_info() const;
    PowerInfo detect_power_info() const;
    
    // Platform-specific sensor reading
#ifdef ECSCOPE_PLATFORM_WINDOWS
    std::vector<TemperatureSensor> read_windows_temperature_sensors() const;
    std::vector<PowerSensor> read_windows_power_sensors() const;
    std::optional<BatteryInfo> read_windows_battery_info() const;
#endif
    
#ifdef ECSCOPE_PLATFORM_LINUX
    std::vector<TemperatureSensor> read_linux_temperature_sensors() const;
    std::vector<PowerSensor> read_linux_power_sensors() const;
    std::optional<BatteryInfo> read_linux_battery_info() const;
    f32 read_thermal_zone_temperature(const std::string& zone_path) const;
    f32 read_power_supply_value(const std::string& supply_path, const std::string& property) const;
#endif
    
#ifdef ECSCOPE_PLATFORM_MACOS
    std::vector<TemperatureSensor> read_macos_temperature_sensors() const;
    std::vector<PowerSensor> read_macos_power_sensors() const;
    std::optional<BatteryInfo> read_macos_battery_info() const;
#endif
    
    // Utility functions
    ThermalState calculate_thermal_state(const ThermalInfo& thermal_info) const;
    PowerState calculate_power_state(const PowerInfo& power_info) const;
    f32 calculate_performance_impact(const ThermalInfo& thermal, const PowerInfo& power) const;
};

//=============================================================================
// Adaptive Performance Management
//=============================================================================

/**
 * @brief Adaptive performance management based on thermal/power constraints
 */
class AdaptivePerformanceManager {
public:
    /**
     * @brief Performance scaling strategy
     */
    enum class ScalingStrategy : u8 {
        Conservative,          // Prioritize thermal/power safety
        Balanced,             // Balance performance and constraints
        Performance,          // Maximize performance within limits
        Aggressive,           // Push limits for maximum performance
        Battery_Optimized,    // Optimize for battery life
        Thermal_Aware         // Dynamically adjust based on temperature
    };
    
    /**
     * @brief Performance scaling recommendation
     */
    struct PerformanceRecommendation {
        std::string component;           // "CPU", "GPU", "Memory", etc.
        f32 recommended_scale_factor;    // 0.0-1.0, where 1.0 = maximum
        std::string reasoning;           // Human-readable explanation
        f32 estimated_power_reduction;   // Watts saved
        f32 estimated_temp_reduction;    // Degrees reduced
        f32 estimated_performance_loss;  // Performance impact (0-1)
        u32 priority;                    // Recommendation priority (1-10)
    };
    
private:
    ThermalPowerMonitor& monitor_;
    ScalingStrategy current_strategy_;
    std::atomic<bool> adaptive_scaling_enabled_{false};
    
    // Scaling parameters
    struct ScalingParameters {
        f32 thermal_throttle_start_temp = 80.0f;
        f32 thermal_throttle_critical_temp = 90.0f;
        f32 power_throttle_threshold_percent = 85.0f;
        f32 battery_saver_threshold_percent = 20.0f;
        f32 minimum_performance_scale = 0.25f; // Never scale below 25%
        std::chrono::seconds response_time{5}; // How quickly to respond to changes
    };
    
    ScalingParameters scaling_params_;
    
    // Current scaling state
    struct ComponentScaling {
        f32 current_scale_factor = 1.0f;
        f32 target_scale_factor = 1.0f;
        std::chrono::steady_clock::time_point last_update;
        std::string last_reason;
    };
    
    std::unordered_map<std::string, ComponentScaling> component_scaling_;
    mutable std::mutex scaling_mutex_;
    
public:
    explicit AdaptivePerformanceManager(ThermalPowerMonitor& monitor);
    ~AdaptivePerformanceManager() = default;
    
    // Configuration
    void set_scaling_strategy(ScalingStrategy strategy);
    ScalingStrategy get_scaling_strategy() const { return current_strategy_; }
    
    void set_scaling_parameters(const ScalingParameters& params);
    const ScalingParameters& get_scaling_parameters() const { return scaling_params_; }
    
    // Adaptive scaling control
    void enable_adaptive_scaling(bool enable);
    bool is_adaptive_scaling_enabled() const { return adaptive_scaling_enabled_.load(); }
    
    // Performance recommendations
    std::vector<PerformanceRecommendation> get_current_recommendations() const;
    std::vector<PerformanceRecommendation> get_thermal_recommendations() const;
    std::vector<PerformanceRecommendation> get_power_recommendations() const;
    std::vector<PerformanceRecommendation> get_battery_recommendations() const;
    
    // Manual scaling control
    void set_component_scale_factor(const std::string& component, f32 scale_factor);
    f32 get_component_scale_factor(const std::string& component) const;
    void reset_all_scaling();
    
    // Analysis and reporting
    std::string get_current_scaling_status() const;
    std::string get_performance_analysis() const;
    f32 get_overall_performance_scale() const;
    f32 estimate_power_savings() const;
    f32 estimate_thermal_improvement() const;
    
private:
    // Recommendation generation
    std::vector<PerformanceRecommendation> analyze_thermal_constraints() const;
    std::vector<PerformanceRecommendation> analyze_power_constraints() const;
    std::vector<PerformanceRecommendation> analyze_battery_constraints() const;
    
    // Scaling calculation
    f32 calculate_thermal_scale_factor(f32 current_temp, f32 throttle_temp, f32 critical_temp) const;
    f32 calculate_power_scale_factor(f32 current_power, f32 max_power) const;
    f32 calculate_battery_scale_factor(f32 battery_percent) const;
    
    // Strategy-specific scaling
    std::vector<PerformanceRecommendation> apply_conservative_strategy() const;
    std::vector<PerformanceRecommendation> apply_balanced_strategy() const;
    std::vector<PerformanceRecommendation> apply_performance_strategy() const;
    std::vector<PerformanceRecommendation> apply_battery_optimized_strategy() const;
    std::vector<PerformanceRecommendation> apply_thermal_aware_strategy() const;
};

//=============================================================================
// Educational Thermal Management Demonstrations
//=============================================================================

/**
 * @brief Educational demonstrations of thermal and power management concepts
 */
class ThermalEducationSuite {
private:
    ThermalPowerMonitor& monitor_;
    AdaptivePerformanceManager& performance_manager_;
    
public:
    ThermalEducationSuite(ThermalPowerMonitor& monitor, 
                         AdaptivePerformanceManager& manager);
    
    // Educational demonstrations
    void demonstrate_thermal_throttling();
    void demonstrate_power_scaling();
    void demonstrate_battery_optimization();
    void demonstrate_cooling_effectiveness();
    void demonstrate_performance_vs_power_tradeoffs();
    
    // Interactive tutorials
    void interactive_thermal_management_tutorial();
    void interactive_power_optimization_tutorial();
    void interactive_mobile_performance_tutorial();
    
    // Analysis and visualization
    std::string analyze_thermal_behavior() const;
    std::string analyze_power_efficiency() const;
    void visualize_thermal_power_relationship() const;
    void compare_power_profiles() const;
    
    // Educational reports
    std::string generate_thermal_analysis_report() const;
    std::string generate_power_optimization_report() const;
    std::string generate_mobile_performance_guide() const;
    
private:
    // Demonstration helpers
    void run_thermal_stress_test();
    void simulate_different_workloads();
    void measure_cooling_response();
    void analyze_battery_drain_patterns();
};

//=============================================================================
// Global Thermal/Power Management
//=============================================================================

/**
 * @brief Initialize the global thermal/power management system
 */
void initialize_thermal_power_management();

/**
 * @brief Get the global thermal/power monitor
 */
ThermalPowerMonitor& get_thermal_power_monitor();

/**
 * @brief Get the global adaptive performance manager
 */
AdaptivePerformanceManager& get_adaptive_performance_manager();

/**
 * @brief Shutdown the global thermal/power management system
 */
void shutdown_thermal_power_management();

/**
 * @brief Quick thermal/power status helpers
 */
namespace quick_thermal {
    ThermalState get_current_thermal_state();
    PowerState get_current_power_state();
    f32 get_cpu_temperature();
    f32 get_battery_level();
    bool is_thermal_throttling();
    bool is_on_battery_power();
    std::string get_thermal_summary();
    std::string get_power_summary();
    std::vector<std::string> get_optimization_tips();
}

} // namespace ecscope::platform::thermal