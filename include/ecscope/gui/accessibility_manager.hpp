/**
 * @file accessibility_manager.hpp
 * @brief Central Accessibility Manager and Integration Hub
 * 
 * Main accessibility management system that coordinates all accessibility
 * subsystems, provides unified configuration, handles system integration,
 * and ensures cohesive accessibility experience across the ECScope engine.
 * 
 * Features:
 * - Centralized accessibility system management
 * - Unified accessibility preferences and profiles
 * - System-wide accessibility state coordination
 * - Integration with platform accessibility APIs
 * - Accessibility event broadcasting and handling
 * - Performance monitoring and optimization
 * - Accessibility session management
 * 
 * @author ECScope Development Team
 * @version 1.0.0
 */

#pragma once

#include "accessibility_core.hpp"
#include "accessibility_keyboard.hpp"
#include "accessibility_screen_reader.hpp"
#include "accessibility_visual.hpp"
#include "accessibility_testing.hpp"
#include "accessibility_motor.hpp"
#include "gui_theme.hpp"
#include <memory>
#include <string>
#include <vector>
#include <functional>
#include <chrono>

namespace ecscope::gui::accessibility {

// =============================================================================
// ACCESSIBILITY MANAGER ENUMERATIONS
// =============================================================================

/**
 * @brief Accessibility system status
 */
enum class AccessibilityStatus : uint8_t {
    Uninitialized,
    Initializing,
    Active,
    Suspended,
    Error,
    ShuttingDown
};

/**
 * @brief Accessibility profile types
 */
enum class AccessibilityProfileType : uint8_t {
    Custom,
    Beginner,           // Basic accessibility features
    Intermediate,       // Standard accessibility setup
    Advanced,          // Full accessibility features
    ScreenReaderUser,  // Optimized for screen readers
    KeyboardOnly,      // Keyboard navigation focused
    MotorImpaired,     // Motor disability accommodations
    VisuallyImpaired,  // Visual accessibility focused
    CognitiveSupport,  // Cognitive accessibility aids
    Enterprise         // Enterprise/corporate settings
};

/**
 * @brief Integration levels with platform accessibility
 */
enum class PlatformIntegrationLevel : uint8_t {
    None,              // No platform integration
    Basic,             // Basic system notifications
    Standard,          // Standard accessibility API usage
    Full               // Full platform accessibility integration
};

// =============================================================================
// ACCESSIBILITY MANAGER STRUCTURES
// =============================================================================

/**
 * @brief Comprehensive accessibility configuration
 */
struct AccessibilityConfiguration {
    // General settings
    bool enabled = false;
    WCAGLevel target_compliance_level = WCAGLevel::AA;
    AccessibilityProfileType profile_type = AccessibilityProfileType::Intermediate;
    PlatformIntegrationLevel platform_integration = PlatformIntegrationLevel::Standard;
    
    // Feature enablement
    bool keyboard_navigation_enabled = true;
    bool screen_reader_support_enabled = true;
    bool visual_accessibility_enabled = true;
    bool motor_accommodations_enabled = true;
    bool testing_framework_enabled = false;
    
    // Performance settings
    bool optimize_for_performance = true;
    float update_frequency = 60.0f;         // Hz
    bool enable_async_processing = true;
    size_t max_concurrent_operations = 4;
    
    // Logging and diagnostics
    bool enable_accessibility_logging = false;
    bool enable_performance_monitoring = false;
    bool enable_usage_analytics = false;
    std::string log_file_path;
    
    // User preferences (loaded from system/user settings)
    AccessibilityPreferences user_preferences;
    
    // Advanced configuration
    std::unordered_map<std::string, std::string> advanced_settings;
};

/**
 * @brief Accessibility system statistics
 */
struct AccessibilitySystemStats {
    AccessibilityStatus status = AccessibilityStatus::Uninitialized;
    std::chrono::steady_clock::time_point initialization_time;
    std::chrono::duration<float> uptime{0};
    
    // Subsystem status
    bool keyboard_navigator_active = false;
    bool screen_reader_active = false;
    bool visual_manager_active = false;
    bool motor_manager_active = false;
    bool testing_framework_active = false;
    
    // Performance metrics
    float average_update_time_ms = 0.0f;
    float peak_update_time_ms = 0.0f;
    size_t total_events_processed = 0;
    size_t accessibility_violations_detected = 0;
    
    // User interaction metrics
    size_t focus_changes = 0;
    size_t screen_reader_announcements = 0;
    size_t keyboard_shortcuts_used = 0;
    size_t accessibility_features_used = 0;
    
    // Resource usage
    size_t memory_usage_kb = 0;
    float cpu_usage_percent = 0.0f;
};

/**
 * @brief Accessibility event types
 */
enum class AccessibilityEvent : uint16_t {
    // System events
    SystemInitialized,
    SystemShutdown,
    ConfigurationChanged,
    ProfileChanged,
    
    // Focus events
    FocusChanged,
    FocusEntered,
    FocusLeft,
    
    // Screen reader events
    AnnouncementMade,
    ScreenReaderStatusChanged,
    LiveRegionUpdated,
    
    // Visual events
    HighContrastToggled,
    FontScaleChanged,
    ColorBlindnessChanged,
    MotionPreferenceChanged,
    
    // Motor events
    AccommodationEnabled,
    AccommodationDisabled,
    SwitchAccessActivated,
    VoiceCommandRecognized,
    
    // Testing events
    ValidationStarted,
    ValidationCompleted,
    IssueDetected,
    IssueResolved,
    
    // User events
    UserPreferencesChanged,
    AccessibilityFeatureUsed,
    ShortcutActivated,
    
    // Error events
    AccessibilityError,
    CompatibilityIssue,
    ResourceExhaustion
};

/**
 * @brief Accessibility event data
 */
struct AccessibilityEventData {
    AccessibilityEvent event_type;
    std::chrono::steady_clock::time_point timestamp;
    std::string source_component;
    GuiID widget_id = 0;
    
    // Event-specific data
    std::unordered_map<std::string, std::string> string_data;
    std::unordered_map<std::string, float> numeric_data;
    std::unordered_map<std::string, bool> boolean_data;
    
    // Context information
    std::string context_path;
    std::string user_action;
    WCAGLevel compliance_level = WCAGLevel::A;
    
    AccessibilityEventData(AccessibilityEvent type, const std::string& source)
        : event_type(type), timestamp(std::chrono::steady_clock::now()), source_component(source) {}
};

// =============================================================================
// CENTRAL ACCESSIBILITY MANAGER
// =============================================================================

/**
 * @brief Central accessibility system manager
 */
class AccessibilityManager {
public:
    AccessibilityManager();
    ~AccessibilityManager();

    // Non-copyable and non-movable
    AccessibilityManager(const AccessibilityManager&) = delete;
    AccessibilityManager& operator=(const AccessibilityManager&) = delete;
    AccessibilityManager(AccessibilityManager&&) = delete;
    AccessibilityManager& operator=(AccessibilityManager&&) = delete;

    // =============================================================================
    // SYSTEM LIFECYCLE
    // =============================================================================

    /**
     * @brief Initialize the accessibility system
     * @param config Accessibility configuration
     * @param theme_manager Pointer to theme manager (optional)
     * @param input_system Pointer to input system (optional)
     * @return true if successful
     */
    bool initialize(const AccessibilityConfiguration& config = AccessibilityConfiguration{},
                   ThemeManager* theme_manager = nullptr,
                   InputSystem* input_system = nullptr);

    /**
     * @brief Shutdown the accessibility system
     */
    void shutdown();

    /**
     * @brief Update the accessibility system (call each frame)
     * @param delta_time Time since last update
     */
    void update(float delta_time);

    /**
     * @brief Suspend accessibility system (for performance)
     */
    void suspend();

    /**
     * @brief Resume accessibility system
     */
    void resume();

    /**
     * @brief Check if accessibility system is initialized and running
     */
    bool is_active() const;

    /**
     * @brief Get current system status
     */
    AccessibilityStatus get_status() const;

    // =============================================================================
    // CONFIGURATION MANAGEMENT
    // =============================================================================

    void set_configuration(const AccessibilityConfiguration& config);
    const AccessibilityConfiguration& get_configuration() const;
    
    void apply_accessibility_profile(AccessibilityProfileType profile_type);
    AccessibilityProfileType get_current_profile_type() const;
    
    void save_configuration_to_file(const std::string& filename) const;
    bool load_configuration_from_file(const std::string& filename);

    // =============================================================================
    // SUBSYSTEM ACCESS
    // =============================================================================

    AccessibilityContext* get_accessibility_context() { return accessibility_context_.get(); }
    AdvancedKeyboardNavigator* get_keyboard_navigator() { return keyboard_navigator_.get(); }
    ScreenReaderManager* get_screen_reader_manager() { return screen_reader_manager_.get(); }
    VisualAccessibilityManager* get_visual_manager() { return visual_manager_.get(); }
    MotorAccessibilityManager* get_motor_manager() { return motor_manager_.get(); }
    AccessibilityTestFramework* get_test_framework() { return test_framework_.get(); }

    // =============================================================================
    // UNIFIED ACCESSIBILITY INTERFACE
    // =============================================================================

    /**
     * @brief Enable or disable accessibility system
     */
    void set_accessibility_enabled(bool enabled);
    bool is_accessibility_enabled() const;

    /**
     * @brief Quick accessibility feature toggles
     */
    void toggle_screen_reader_support();
    void toggle_high_contrast_mode();
    void toggle_keyboard_navigation();
    void toggle_motor_accommodations();
    
    bool is_screen_reader_active() const;
    bool is_high_contrast_active() const;
    bool is_keyboard_navigation_active() const;
    bool are_motor_accommodations_active() const;

    /**
     * @brief Set accessibility preferences
     */
    void set_user_preferences(const AccessibilityPreferences& preferences);
    const AccessibilityPreferences& get_user_preferences() const;

    // =============================================================================
    // PLATFORM INTEGRATION
    // =============================================================================

    void set_platform_integration_level(PlatformIntegrationLevel level);
    PlatformIntegrationLevel get_platform_integration_level() const;
    
    void detect_system_accessibility_settings();
    void apply_system_accessibility_settings();
    void sync_with_system_changes();
    
    bool register_with_platform_accessibility();
    void unregister_from_platform_accessibility();

    // =============================================================================
    // EVENT SYSTEM
    // =============================================================================

    using AccessibilityEventHandler = std::function<void(const AccessibilityEventData& event)>;
    
    void add_event_handler(AccessibilityEvent event_type, AccessibilityEventHandler handler);
    void remove_event_handler(AccessibilityEvent event_type);
    void broadcast_event(const AccessibilityEventData& event);
    
    std::vector<AccessibilityEventData> get_recent_events(size_t max_count = 100) const;
    void clear_event_history();

    // =============================================================================
    // VALIDATION AND COMPLIANCE
    // =============================================================================

    /**
     * @brief Run quick accessibility validation
     */
    AccessibilityTestSuiteResult validate_current_interface();

    /**
     * @brief Run full WCAG compliance test
     */
    AccessibilityTestSuiteResult run_compliance_audit(WCAGLevel target_level = WCAGLevel::AA);

    /**
     * @brief Check if interface meets accessibility standards
     */
    bool meets_accessibility_standards(WCAGLevel level = WCAGLevel::AA) const;

    /**
     * @brief Get accessibility issues summary
     */
    std::vector<std::string> get_accessibility_issues() const;

    // =============================================================================
    // PERFORMANCE AND MONITORING
    // =============================================================================

    AccessibilitySystemStats get_system_stats() const;
    void reset_performance_counters();
    
    void enable_performance_monitoring(bool enable);
    bool is_performance_monitoring_enabled() const;
    
    void set_update_frequency(float frequency_hz);
    float get_update_frequency() const;

    // =============================================================================
    // SESSION MANAGEMENT
    // =============================================================================

    struct AccessibilitySession {
        std::string session_id;
        std::chrono::steady_clock::time_point start_time;
        AccessibilityProfileType profile_type;
        std::vector<AccessibilityFeature> features_used;
        size_t interactions_count = 0;
        std::vector<std::string> issues_encountered;
        float total_usage_time = 0.0f;
        
        std::string generate_summary() const;
    };

    void start_accessibility_session(const std::string& session_id = "");
    void end_accessibility_session();
    const AccessibilitySession* get_current_session() const;
    std::vector<AccessibilitySession> get_session_history() const;

    // =============================================================================
    // USER ASSISTANCE
    // =============================================================================

    /**
     * @brief Get accessibility help for current context
     */
    std::string get_contextual_accessibility_help() const;

    /**
     * @brief Get available accessibility shortcuts
     */
    std::vector<std::string> get_accessibility_shortcuts() const;

    /**
     * @brief Show accessibility onboarding
     */
    void show_accessibility_tutorial();

    /**
     * @brief Get accessibility feature recommendations
     */
    std::vector<std::string> get_accessibility_recommendations() const;

    // =============================================================================
    // DEBUGGING AND DIAGNOSTICS
    // =============================================================================

    void render_accessibility_debug_overlay(DrawList* draw_list);
    void render_accessibility_status_panel(DrawList* draw_list);
    
    void enable_accessibility_logging(bool enable, const std::string& log_file = "");
    void log_accessibility_event(const std::string& message, const std::string& level = "INFO");
    
    std::string generate_accessibility_report() const;
    void export_accessibility_diagnostics(const std::string& filename) const;

    // =============================================================================
    // GLOBAL ACCESS HELPERS
    // =============================================================================

    /**
     * @brief Quick access to common functionality
     */
    void announce_to_screen_reader(const std::string& message, bool interrupt = false);
    void set_widget_accessible_name(GuiID widget_id, const std::string& name);
    void set_widget_accessible_description(GuiID widget_id, const std::string& description);
    void set_widget_role(GuiID widget_id, AccessibilityRole role);
    void focus_widget(GuiID widget_id);
    
    /**
     * @brief Convenience macros implementation
     */
    void label_widget(GuiID widget_id, const std::string& label);
    void describe_widget(GuiID widget_id, const std::string& description);
    void set_widget_role_direct(GuiID widget_id, AccessibilityRole role);

private:
    // Configuration
    AccessibilityConfiguration config_;
    AccessibilityStatus status_ = AccessibilityStatus::Uninitialized;
    AccessibilityProfileType current_profile_type_ = AccessibilityProfileType::Intermediate;
    
    // Core subsystems
    std::unique_ptr<AccessibilityContext> accessibility_context_;
    std::unique_ptr<AdvancedKeyboardNavigator> keyboard_navigator_;
    std::unique_ptr<ScreenReaderManager> screen_reader_manager_;
    std::unique_ptr<VisualAccessibilityManager> visual_manager_;
    std::unique_ptr<MotorAccessibilityManager> motor_manager_;
    std::unique_ptr<AccessibilityTestFramework> test_framework_;
    
    // External system references
    ThemeManager* theme_manager_ = nullptr;
    InputSystem* input_system_ = nullptr;
    
    // Platform integration
    PlatformIntegrationLevel platform_integration_level_ = PlatformIntegrationLevel::Standard;
    void* platform_accessibility_handle_ = nullptr;
    
    // Event system
    std::unordered_map<AccessibilityEvent, std::vector<AccessibilityEventHandler>> event_handlers_;
    std::vector<AccessibilityEventData> event_history_;
    mutable std::mutex event_mutex_;
    
    // Session management
    std::unique_ptr<AccessibilitySession> current_session_;
    std::vector<AccessibilitySession> session_history_;
    
    // Performance monitoring
    std::chrono::steady_clock::time_point last_update_time_;
    std::chrono::steady_clock::time_point initialization_time_;
    float update_frequency_ = 60.0f;
    bool performance_monitoring_enabled_ = false;
    
    // Statistics and monitoring
    mutable AccessibilitySystemStats system_stats_;
    std::chrono::steady_clock::time_point stats_last_update_;
    
    // Logging
    bool logging_enabled_ = false;
    std::string log_file_path_;
    std::unique_ptr<std::ofstream> log_file_;
    
    // Helper methods
    bool initialize_subsystems();
    void shutdown_subsystems();
    void update_subsystems(float delta_time);
    void update_system_stats();
    void update_performance_metrics(float delta_time);
    
    void apply_profile_configuration(AccessibilityProfileType profile_type);
    AccessibilityConfiguration create_profile_configuration(AccessibilityProfileType profile_type) const;
    
    void setup_default_event_handlers();
    void handle_focus_change_event(GuiID old_focus, GuiID new_focus);
    void handle_screen_reader_event(const std::string& message);
    void handle_accessibility_error(const std::string& error_message);
    
    void detect_windows_accessibility_settings();
    void detect_macos_accessibility_settings();
    void detect_linux_accessibility_settings();
    
    void log_internal(const std::string& message, const std::string& level);
    std::string format_log_message(const std::string& message, const std::string& level) const;
    
    static AccessibilityManager* global_instance_;
    friend AccessibilityManager* get_global_accessibility_manager();
};

// =============================================================================
// ACCESSIBILITY PROFILES
// =============================================================================

/**
 * @brief Predefined accessibility profiles
 */
namespace accessibility_profiles {
    AccessibilityConfiguration create_beginner_profile();
    AccessibilityConfiguration create_intermediate_profile();
    AccessibilityConfiguration create_advanced_profile();
    AccessibilityConfiguration create_screen_reader_profile();
    AccessibilityConfiguration create_keyboard_only_profile();
    AccessibilityConfiguration create_motor_impaired_profile();
    AccessibilityConfiguration create_visually_impaired_profile();
    AccessibilityConfiguration create_enterprise_profile();
}

// =============================================================================
// GLOBAL ACCESSIBILITY MANAGER
// =============================================================================

/**
 * @brief Get the global accessibility manager instance
 */
AccessibilityManager* get_global_accessibility_manager();

/**
 * @brief Initialize global accessibility system
 */
bool initialize_global_accessibility(const AccessibilityConfiguration& config = AccessibilityConfiguration{},
                                    ThemeManager* theme_manager = nullptr,
                                    InputSystem* input_system = nullptr);

/**
 * @brief Shutdown global accessibility system
 */
void shutdown_global_accessibility();

/**
 * @brief Quick global accessibility functions
 */
void enable_accessibility(bool enable = true);
bool is_accessibility_enabled();
void announce(const std::string& message, bool interrupt = false);
void set_accessible_label(GuiID widget_id, const std::string& label);
void set_accessible_description(GuiID widget_id, const std::string& description);

// =============================================================================
// ACCESSIBILITY INTEGRATION MACROS
// =============================================================================

// Enhanced versions of the convenience macros that work with the manager
#define ECSCOPE_ACCESSIBILITY_LABEL(widget_id, label) \
    do { \
        if (auto* manager = get_global_accessibility_manager()) { \
            manager->set_widget_accessible_name(widget_id, label); \
        } \
    } while(0)

#define ECSCOPE_ACCESSIBILITY_DESCRIPTION(widget_id, description) \
    do { \
        if (auto* manager = get_global_accessibility_manager()) { \
            manager->set_widget_accessible_description(widget_id, description); \
        } \
    } while(0)

#define ECSCOPE_ACCESSIBILITY_ROLE(widget_id, role) \
    do { \
        if (auto* manager = get_global_accessibility_manager()) { \
            manager->set_widget_role(widget_id, role); \
        } \
    } while(0)

#define ECSCOPE_ANNOUNCE(message) \
    do { \
        if (auto* manager = get_global_accessibility_manager()) { \
            manager->announce_to_screen_reader(message); \
        } \
    } while(0)

#define ECSCOPE_ANNOUNCE_URGENT(message) \
    do { \
        if (auto* manager = get_global_accessibility_manager()) { \
            manager->announce_to_screen_reader(message, true); \
        } \
    } while(0)

#define ECSCOPE_FOCUS_WIDGET(widget_id) \
    do { \
        if (auto* manager = get_global_accessibility_manager()) { \
            manager->focus_widget(widget_id); \
        } \
    } while(0)

} // namespace ecscope::gui::accessibility