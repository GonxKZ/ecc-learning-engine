/**
 * @file accessibility_screen_reader.hpp
 * @brief Screen Reader Support and Integration
 * 
 * Comprehensive screen reader support providing compatibility with NVDA, JAWS,
 * VoiceOver, and other assistive technologies. Implements ARIA-like semantics,
 * live regions, proper announcements, and accessible text generation.
 * 
 * Features:
 * - Cross-platform screen reader detection and integration
 * - ARIA-like role and property system
 * - Live region management with politeness levels
 * - Accessible text generation and formatting
 * - Speech synthesis integration
 * - Braille display support preparation
 * - Screen reader specific optimizations
 * - Announcement queue management
 * 
 * @author ECScope Development Team
 * @version 1.0.0
 */

#pragma once

#include "accessibility_core.hpp"
#include <memory>
#include <string>
#include <vector>
#include <queue>
#include <chrono>
#include <functional>
#include <atomic>
#include <mutex>

namespace ecscope::gui::accessibility {

// =============================================================================
// SCREEN READER INTEGRATION
// =============================================================================

/**
 * @brief Screen reader announcement priorities
 */
enum class AnnouncementPriority : uint8_t {
    Low,        // Informational, can be skipped
    Normal,     // Standard announcements
    Important,  // Should be heard but not interrupting
    Urgent,     // Interrupts current speech
    Emergency   // Immediate attention required
};

/**
 * @brief Speech synthesis parameters
 */
struct SpeechParameters {
    float rate = 1.0f;           // Speech rate (0.1 - 3.0)
    float pitch = 1.0f;          // Pitch (0.1 - 2.0) 
    float volume = 0.8f;         // Volume (0.0 - 1.0)
    std::string voice_name;      // Preferred voice name
    std::string language = "en-US";  // Language code
    bool spell_out = false;      // Spell out words letter by letter
    bool use_phonetics = false;  // Use phonetic alphabet for letters
};

/**
 * @brief Announcement structure
 */
struct Announcement {
    std::string message;
    AnnouncementPriority priority = AnnouncementPriority::Normal;
    bool interrupt_current = false;
    SpeechParameters speech_params;
    std::chrono::steady_clock::time_point timestamp;
    GuiID source_widget = 0;
    AccessibilityRole source_role = AccessibilityRole::None;
    
    // Metadata
    std::string context;         // Context where announcement originated
    bool is_focus_change = false;
    bool is_value_change = false;
    bool is_state_change = false;
    
    Announcement() : timestamp(std::chrono::steady_clock::now()) {}
    Announcement(const std::string& msg, AnnouncementPriority prio = AnnouncementPriority::Normal)
        : message(msg), priority(prio), timestamp(std::chrono::steady_clock::now()) {}
};

/**
 * @brief Live region update types
 */
enum class LiveRegionUpdate : uint8_t {
    Addition,      // New content added
    Removal,       // Content removed
    Text,          // Text content changed
    All            // All content changed
};

/**
 * @brief Live region information
 */
struct LiveRegion {
    GuiID region_id = 0;
    WidgetAccessibilityInfo::LiveRegionPoliteness politeness = WidgetAccessibilityInfo::LiveRegionPoliteness::Polite;
    bool atomic = false;       // Announce all content together
    bool busy = false;         // Region is being updated
    bool relevant_additions = true;
    bool relevant_removals = false;
    bool relevant_text = true;
    
    std::string current_content;
    std::string previous_content;
    std::chrono::steady_clock::time_point last_update;
    
    // Update tracking
    std::vector<LiveRegionUpdate> pending_updates;
    std::chrono::steady_clock::time_point next_announcement_time;
};

/**
 * @brief Screen reader specific formatting preferences
 */
struct ScreenReaderFormatting {
    // Punctuation handling
    enum class PunctuationLevel { None, Some, Most, All } punctuation_level = PunctuationLevel::Most;
    
    // Number handling
    bool announce_numbers_as_digits = false;
    bool announce_phone_numbers_as_digits = true;
    
    // Capitalization
    bool announce_capitalization = true;
    bool spell_capitalized_words = false;
    
    // Special characters
    bool announce_whitespace = false;
    bool announce_formatting = true;
    bool announce_colors = true;
    bool announce_fonts = false;
    
    // Navigation aids
    bool announce_headings_level = true;
    bool announce_list_info = true;
    bool announce_table_info = true;
    bool announce_link_info = true;
    
    // Verbosity
    enum class VerbosityLevel { Brief, Normal, Verbose } verbosity = VerbosityLevel::Normal;
    bool include_help_text = true;
    bool include_type_info = true;
    bool include_state_info = true;
    bool include_position_info = false;
};

// =============================================================================
// SCREEN READER MANAGER
// =============================================================================

/**
 * @brief Central screen reader integration manager
 */
class ScreenReaderManager {
public:
    ScreenReaderManager();
    ~ScreenReaderManager();

    // =============================================================================
    // INITIALIZATION & LIFECYCLE
    // =============================================================================

    bool initialize(AccessibilityContext* accessibility_context);
    void shutdown();
    void update(float delta_time);

    // =============================================================================
    // SCREEN READER DETECTION
    // =============================================================================

    void detect_screen_readers();
    ScreenReaderType get_detected_screen_reader() const;
    bool is_screen_reader_active() const;
    std::string get_screen_reader_name() const;
    std::string get_screen_reader_version() const;

    // =============================================================================
    // ANNOUNCEMENT SYSTEM
    // =============================================================================

    void announce(const std::string& message, 
                  AnnouncementPriority priority = AnnouncementPriority::Normal,
                  bool interrupt = false);
    
    void announce(const Announcement& announcement);
    
    void announce_widget(GuiID widget_id, bool include_value = true, bool include_state = true);
    void announce_focus_change(GuiID old_focus, GuiID new_focus);
    void announce_value_change(GuiID widget_id, const std::string& old_value, const std::string& new_value);
    void announce_state_change(GuiID widget_id, const AccessibilityState& old_state, const AccessibilityState& new_state);
    void announce_selection_change(GuiID widget_id, const std::string& selection_info);

    // =============================================================================
    // LIVE REGIONS
    // =============================================================================

    void create_live_region(GuiID region_id, 
                           WidgetAccessibilityInfo::LiveRegionPoliteness politeness = WidgetAccessibilityInfo::LiveRegionPoliteness::Polite,
                           bool atomic = false);
    
    void update_live_region(GuiID region_id, const std::string& content, LiveRegionUpdate update_type = LiveRegionUpdate::All);
    void remove_live_region(GuiID region_id);
    void set_live_region_busy(GuiID region_id, bool busy);
    
    std::vector<GuiID> get_live_regions() const;
    const LiveRegion* get_live_region_info(GuiID region_id) const;

    // =============================================================================
    // ACCESSIBLE TEXT GENERATION
    // =============================================================================

    std::string generate_accessible_name(GuiID widget_id) const;
    std::string generate_accessible_description(GuiID widget_id) const;
    std::string generate_full_description(GuiID widget_id, bool include_position = false) const;
    std::string generate_state_description(const AccessibilityState& state) const;
    std::string generate_role_description(AccessibilityRole role) const;
    std::string generate_value_description(GuiID widget_id) const;
    
    // Context-specific descriptions
    std::string generate_button_description(GuiID widget_id) const;
    std::string generate_input_description(GuiID widget_id) const;
    std::string generate_list_description(GuiID widget_id) const;
    std::string generate_table_description(GuiID widget_id) const;
    std::string generate_menu_description(GuiID widget_id) const;

    // =============================================================================
    // SPEECH SYNTHESIS
    // =============================================================================

    void set_speech_parameters(const SpeechParameters& params);
    const SpeechParameters& get_speech_parameters() const;
    
    bool is_speech_available() const;
    bool is_speaking() const;
    void stop_speech();
    void pause_speech();
    void resume_speech();
    
    std::vector<std::string> get_available_voices() const;
    void set_preferred_voice(const std::string& voice_name);

    // =============================================================================
    // FORMATTING & VERBOSITY
    // =============================================================================

    void set_formatting_preferences(const ScreenReaderFormatting& formatting);
    const ScreenReaderFormatting& get_formatting_preferences() const;
    
    void set_verbosity_level(ScreenReaderFormatting::VerbosityLevel level);
    ScreenReaderFormatting::VerbosityLevel get_verbosity_level() const;
    
    void enable_context_help(bool enable);
    bool is_context_help_enabled() const;

    // =============================================================================
    // NAVIGATION ANNOUNCEMENTS
    // =============================================================================

    void announce_page_title(const std::string& title);
    void announce_window_change(const std::string& window_name);
    void announce_dialog_open(const std::string& dialog_title, const std::string& dialog_type = "Dialog");
    void announce_dialog_close();
    void announce_menu_open(const std::string& menu_name);
    void announce_menu_close();
    void announce_context_menu_open(const std::string& context);
    
    void announce_navigation_landmark(const std::string& landmark_type, const std::string& landmark_name = "");
    void announce_heading(const std::string& text, int level);
    void announce_list_start(const std::string& list_type, int item_count);
    void announce_list_end();
    void announce_table_start(int rows, int columns);
    void announce_table_cell(int row, int column, const std::string& content, bool is_header = false);

    // =============================================================================
    // ERROR AND STATUS ANNOUNCEMENTS
    // =============================================================================

    void announce_error(const std::string& error_message, GuiID widget_id = 0);
    void announce_warning(const std::string& warning_message, GuiID widget_id = 0);
    void announce_success(const std::string& success_message, GuiID widget_id = 0);
    void announce_status(const std::string& status_message, GuiID widget_id = 0);
    
    void announce_validation_error(GuiID widget_id, const std::string& error_message);
    void announce_loading_start(const std::string& operation = "");
    void announce_loading_progress(float progress, const std::string& operation = "");
    void announce_loading_complete(const std::string& operation = "");

    // =============================================================================
    // BRAILLE DISPLAY SUPPORT
    // =============================================================================

    void enable_braille_output(bool enable);
    bool is_braille_enabled() const;
    void update_braille_display(GuiID focused_widget);
    std::string generate_braille_text(GuiID widget_id) const;

    // =============================================================================
    // DEBUGGING & DIAGNOSTICS
    // =============================================================================

    struct ScreenReaderStats {
        ScreenReaderType detected_type = ScreenReaderType::None;
        bool active = false;
        size_t announcements_queued = 0;
        size_t announcements_sent = 0;
        size_t live_regions = 0;
        bool speech_available = false;
        bool speaking = false;
        float speech_rate = 1.0f;
        std::string current_voice;
        ScreenReaderFormatting::VerbosityLevel verbosity = ScreenReaderFormatting::VerbosityLevel::Normal;
    };

    ScreenReaderStats get_stats() const;
    void render_debug_overlay(DrawList* draw_list);
    std::string get_announcement_queue_status() const;

    // =============================================================================
    // EVENT CALLBACKS
    // =============================================================================

    using AnnouncementCallback = std::function<void(const Announcement& announcement)>;
    using ScreenReaderStatusCallback = std::function<void(bool active, ScreenReaderType type)>;

    void set_announcement_callback(AnnouncementCallback callback);
    void set_screen_reader_status_callback(ScreenReaderStatusCallback callback);

private:
    // Core components
    AccessibilityContext* accessibility_context_ = nullptr;
    
    // Screen reader detection
    ScreenReaderType detected_screen_reader_ = ScreenReaderType::None;
    bool screen_reader_active_ = false;
    std::string screen_reader_name_;
    std::string screen_reader_version_;
    
    // Announcement system
    std::queue<Announcement> announcement_queue_;
    std::mutex announcement_mutex_;
    std::atomic<bool> is_speaking_{false};
    std::chrono::steady_clock::time_point last_announcement_time_;
    
    // Live regions
    std::unordered_map<GuiID, LiveRegion> live_regions_;
    std::chrono::steady_clock::time_point last_live_region_check_;
    
    // Speech synthesis
    SpeechParameters speech_parameters_;
    bool speech_available_ = false;
    std::vector<std::string> available_voices_;
    
    // Formatting
    ScreenReaderFormatting formatting_preferences_;
    bool context_help_enabled_ = true;
    
    // Braille support
    bool braille_enabled_ = false;
    std::string current_braille_text_;
    
    // Platform-specific handles
    void* platform_screen_reader_handle_ = nullptr;
    void* platform_speech_handle_ = nullptr;
    
    // Callbacks
    AnnouncementCallback announcement_callback_;
    ScreenReaderStatusCallback screen_reader_status_callback_;
    
    // Statistics
    mutable ScreenReaderStats stats_;
    size_t total_announcements_sent_ = 0;
    
    // Helper methods
    void process_announcement_queue();
    void send_announcement_to_screen_reader(const Announcement& announcement);
    void process_live_region_updates();
    
    bool should_announce_live_region(const LiveRegion& region) const;
    std::string format_message_for_screen_reader(const std::string& message, const SpeechParameters& params) const;
    
    void detect_nvda();
    void detect_jaws();
    void detect_voiceover();
    void detect_orca();
    
    bool send_to_nvda(const std::string& message, bool interrupt);
    bool send_to_jaws(const std::string& message, bool interrupt);
    bool send_to_voiceover(const std::string& message, bool interrupt);
    bool send_to_generic(const std::string& message, bool interrupt);
    
    void initialize_speech_synthesis();
    void cleanup_speech_synthesis();
    bool speak_text(const std::string& text, const SpeechParameters& params);
    
    std::string apply_formatting_rules(const std::string& text) const;
    std::string add_punctuation_pauses(const std::string& text) const;
    std::string handle_capitalization(const std::string& text) const;
    std::string handle_numbers(const std::string& text) const;
    
    bool initialized_ = false;
};

// =============================================================================
// ACCESSIBLE TEXT UTILITIES
// =============================================================================

/**
 * @brief Utilities for generating accessible text descriptions
 */
namespace accessible_text {
    /**
     * @brief Generate accessible name following ARIA naming computation
     */
    std::string compute_accessible_name(const WidgetAccessibilityInfo& widget_info,
                                       const std::unordered_map<GuiID, WidgetAccessibilityInfo>& all_widgets);
    
    /**
     * @brief Generate accessible description following ARIA description computation
     */
    std::string compute_accessible_description(const WidgetAccessibilityInfo& widget_info,
                                              const std::unordered_map<GuiID, WidgetAccessibilityInfo>& all_widgets);
    
    /**
     * @brief Format text for screen reader output
     */
    std::string format_for_screen_reader(const std::string& text, const ScreenReaderFormatting& formatting);
    
    /**
     * @brief Generate position information text
     */
    std::string generate_position_info(const WidgetAccessibilityInfo& widget_info,
                                      const std::unordered_map<GuiID, WidgetAccessibilityInfo>& all_widgets);
    
    /**
     * @brief Generate help text for widget
     */
    std::string generate_help_text(AccessibilityRole role, const std::string& context = "");
    
    /**
     * @brief Convert role to human-readable string
     */
    std::string role_to_string(AccessibilityRole role);
    
    /**
     * @brief Convert state to human-readable string
     */
    std::string state_to_string(const AccessibilityState& state);
    
    /**
     * @brief Generate phonetic spelling for text
     */
    std::string generate_phonetic_spelling(const std::string& text);
}

// =============================================================================
// SCREEN READER TESTING UTILITIES
// =============================================================================

/**
 * @brief Screen reader testing and simulation utilities
 */
class ScreenReaderTester {
public:
    struct TestResult {
        bool accessible_name_present = false;
        bool accessible_description_appropriate = false;
        bool role_appropriate = false;
        bool states_announced = false;
        bool keyboard_accessible = false;
        bool focus_management_correct = false;
        std::vector<std::string> issues;
        std::vector<std::string> suggestions;
    };
    
    static TestResult test_widget_accessibility(GuiID widget_id, const AccessibilityContext& context);
    static TestResult test_live_region(GuiID region_id, const ScreenReaderManager& manager);
    static TestResult test_focus_flow(const std::vector<GuiID>& focus_order, const AccessibilityContext& context);
    
    static void simulate_screen_reader_navigation(const AccessibilityContext& context, ScreenReaderManager& manager);
    static std::string generate_accessibility_report(const AccessibilityContext& context);
};

// =============================================================================
// GLOBAL SCREEN READER MANAGER
// =============================================================================

/**
 * @brief Get the global screen reader manager
 */
ScreenReaderManager* get_screen_reader_manager();

/**
 * @brief Initialize global screen reader support
 */
bool initialize_screen_reader_support();

/**
 * @brief Shutdown global screen reader support
 */
void shutdown_screen_reader_support();

} // namespace ecscope::gui::accessibility