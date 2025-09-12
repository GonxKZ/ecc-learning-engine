/**
 * @file audio_effects_ui.hpp
 * @brief Audio Effects Chain Interface with Real-time Parameter Control
 * 
 * Professional effects editing interface featuring:
 * - Visual representation of audio processing pipeline
 * - Real-time parameter adjustment with audio feedback
 * - Spectrum analyzer and waveform visualization for effects
 * - Effects preset management and sharing
 * - Performance monitoring for audio effects processing
 * 
 * @author ECScope Development Team
 * @version 1.0.0
 */

#pragma once

#include <memory>
#include <vector>
#include <unordered_map>
#include <string>
#include <functional>
#include <atomic>
#include <mutex>
#include <queue>

#ifdef ECSCOPE_HAS_IMGUI
#include <imgui.h>
#include <imgui_internal.h>
#endif

#include "../audio/audio_effects.hpp"
#include "../audio/audio_pipeline.hpp"
#include "../core/math.hpp"

namespace ecscope::gui {

// =============================================================================
// EFFECT PARAMETER TYPES
// =============================================================================

/**
 * @brief Parameter value types for effects
 */
enum class EffectParameterType : uint8_t {
    Float,          ///< Floating point value
    Integer,        ///< Integer value
    Boolean,        ///< Boolean toggle
    Enum,           ///< Enumeration selection
    String,         ///< String value
    Color,          ///< Color picker
    Curve           ///< Curve editor
};

/**
 * @brief Effect parameter descriptor
 */
struct EffectParameter {
    std::string name;
    std::string display_name;
    std::string description;
    EffectParameterType type;
    
    // Value constraints
    float min_value = 0.0f;
    float max_value = 1.0f;
    float default_value = 0.5f;
    float current_value = 0.5f;
    
    // For enum types
    std::vector<std::string> enum_values;
    int current_enum_index = 0;
    
    // For curve types
    std::vector<ImVec2> curve_points;
    
    // UI properties
    bool is_automatable = true;
    bool show_in_compact_view = true;
    std::string unit_suffix;
    int decimal_places = 2;
    
    // Callback for parameter changes
    std::function<void(float)> change_callback;
};

/**
 * @brief Effect preset data
 */
struct EffectPreset {
    std::string name;
    std::string description;
    std::string author;
    std::string version;
    std::unordered_map<std::string, float> parameters;
    std::string preset_data; // JSON or binary data
    bool is_factory_preset = false;
    bool is_favorite = false;
};

/**
 * @brief Effect slot in the processing chain
 */
struct EffectSlot {
    uint32_t id;
    std::string effect_name;
    std::string display_name;
    bool is_enabled = true;
    bool is_bypassed = false;
    bool is_selected = false;
    bool is_solo = false;
    bool show_expanded = false;
    
    // Visual properties
    ImVec2 position{0.0f, 0.0f};
    ImVec2 size{150.0f, 100.0f};
    ImU32 color = IM_COL32(100, 150, 200, 255);
    
    // Effect instance and parameters
    std::shared_ptr<audio::AudioEffect> effect_instance;
    std::vector<EffectParameter> parameters;
    std::vector<EffectPreset> presets;
    
    // Performance metrics
    float processing_time_ms = 0.0f;
    float cpu_usage_percent = 0.0f;
    
    // Audio analysis data
    std::vector<float> input_spectrum;
    std::vector<float> output_spectrum;
    std::vector<float> input_waveform;
    std::vector<float> output_waveform;
    float input_level_rms = 0.0f;
    float output_level_rms = 0.0f;
    float input_level_peak = 0.0f;
    float output_level_peak = 0.0f;
};

/**
 * @brief Effect chain connection
 */
struct EffectConnection {
    uint32_t source_slot_id;
    uint32_t target_slot_id;
    std::string source_output_name = "output";
    std::string target_input_name = "input";
    bool is_active = true;
    ImU32 color = IM_COL32(255, 255, 255, 255);
};

/**
 * @brief Effect automation curve
 */
struct EffectAutomation {
    uint32_t slot_id;
    std::string parameter_name;
    std::vector<std::pair<float, float>> keyframes; // time, value pairs
    bool is_enabled = true;
    bool is_recording = false;
    float playback_time = 0.0f;
};

// =============================================================================
// EFFECTS CHAIN EDITOR CLASS
// =============================================================================

/**
 * @brief Professional Effects Chain Editor
 * 
 * Provides comprehensive interface for:
 * - Visual effects chain editing with drag-and-drop
 * - Real-time parameter adjustment
 * - Audio analysis and visualization
 * - Preset management and automation
 * - Performance monitoring
 */
class AudioEffectsChainEditor {
public:
    AudioEffectsChainEditor();
    ~AudioEffectsChainEditor();

    // =============================================================================
    // INITIALIZATION & LIFECYCLE
    // =============================================================================

    /**
     * @brief Initialize the effects editor
     * @param audio_pipeline Reference to audio pipeline
     * @return true on success
     */
    bool initialize(audio::AudioPipeline* audio_pipeline);

    /**
     * @brief Shutdown and cleanup
     */
    void shutdown();

    /**
     * @brief Check if editor is initialized
     */
    bool is_initialized() const { return initialized_; }

    // =============================================================================
    // MAIN INTERFACE
    // =============================================================================

    /**
     * @brief Render the effects chain editor
     */
    void render();

    /**
     * @brief Update editor state and audio analysis
     * @param delta_time Time since last update
     */
    void update(float delta_time);

    // =============================================================================
    // EFFECT SLOT MANAGEMENT
    // =============================================================================

    /**
     * @brief Add effect to chain
     * @param effect_name Name of effect to add
     * @param position Position in chain (0 = beginning, -1 = end)
     * @return ID of created effect slot
     */
    uint32_t add_effect(const std::string& effect_name, int position = -1);

    /**
     * @brief Remove effect from chain
     */
    void remove_effect(uint32_t slot_id);

    /**
     * @brief Move effect to new position
     */
    void move_effect(uint32_t slot_id, int new_position);

    /**
     * @brief Duplicate effect slot
     */
    uint32_t duplicate_effect(uint32_t slot_id);

    /**
     * @brief Get effect slot by ID
     */
    EffectSlot* get_effect_slot(uint32_t slot_id);

    /**
     * @brief Get all effect slots
     */
    const std::vector<EffectSlot>& get_effect_slots() const { return effect_slots_; }

    // =============================================================================
    // EFFECT PARAMETER CONTROL
    // =============================================================================

    /**
     * @brief Set effect parameter value
     */
    void set_parameter_value(uint32_t slot_id, const std::string& param_name, float value);

    /**
     * @brief Get effect parameter value
     */
    float get_parameter_value(uint32_t slot_id, const std::string& param_name) const;

    /**
     * @brief Set parameter automation
     */
    void set_parameter_automation(uint32_t slot_id, const std::string& param_name, 
                                 const EffectAutomation& automation);

    /**
     * @brief Start/stop parameter automation recording
     */
    void start_automation_recording(uint32_t slot_id, const std::string& param_name);
    void stop_automation_recording(uint32_t slot_id, const std::string& param_name);

    // =============================================================================
    // PRESET MANAGEMENT
    // =============================================================================

    /**
     * @brief Load preset for effect
     */
    bool load_preset(uint32_t slot_id, const std::string& preset_name);

    /**
     * @brief Save current settings as preset
     */
    bool save_preset(uint32_t slot_id, const std::string& preset_name, 
                    const std::string& description = "");

    /**
     * @brief Import presets from file
     */
    bool import_presets(const std::string& filepath);

    /**
     * @brief Export presets to file
     */
    bool export_presets(const std::string& filepath) const;

    // =============================================================================
    // AUDIO ANALYSIS
    // =============================================================================

    /**
     * @brief Enable/disable real-time audio analysis
     */
    void enable_audio_analysis(bool enable) { audio_analysis_enabled_ = enable; }

    /**
     * @brief Update audio analysis data for effect
     */
    void update_audio_analysis(uint32_t slot_id, const float* input_buffer, 
                              const float* output_buffer, size_t buffer_size);

    // =============================================================================
    // CHAIN OPERATIONS
    // =============================================================================

    /**
     * @brief Enable/disable entire effects chain
     */
    void enable_effects_chain(bool enable);

    /**
     * @brief Bypass entire effects chain
     */
    void bypass_effects_chain(bool bypass);

    /**
     * @brief Clear entire effects chain
     */
    void clear_effects_chain();

    /**
     * @brief Save effects chain configuration
     */
    bool save_chain_configuration(const std::string& filepath) const;

    /**
     * @brief Load effects chain configuration
     */
    bool load_chain_configuration(const std::string& filepath);

    // =============================================================================
    // PERFORMANCE MONITORING
    // =============================================================================

    /**
     * @brief Get total processing time for chain
     */
    float get_total_processing_time() const;

    /**
     * @brief Get CPU usage percentage for chain
     */
    float get_cpu_usage_percentage() const;

    /**
     * @brief Enable/disable performance monitoring
     */
    void enable_performance_monitoring(bool enable) { performance_monitoring_enabled_ = enable; }

private:
    // =============================================================================
    // PRIVATE RENDERING METHODS
    // =============================================================================

    void render_chain_overview();
    void render_effect_rack();
    void render_parameter_panel();
    void render_preset_browser();
    void render_audio_analysis_panel();
    void render_automation_panel();
    void render_performance_panel();

    // Effect slot rendering
    void render_effect_slot(EffectSlot& slot);
    void render_effect_connections();
    void render_parameter_controls(EffectSlot& slot);
    void render_effect_analyzer(const EffectSlot& slot);

    // Parameter control widgets
    void render_float_parameter(EffectParameter& param);
    void render_integer_parameter(EffectParameter& param);
    void render_boolean_parameter(EffectParameter& param);
    void render_enum_parameter(EffectParameter& param);
    void render_curve_parameter(EffectParameter& param);

    // Visualization
    void render_spectrum_comparison(const std::vector<float>& input, 
                                   const std::vector<float>& output);
    void render_waveform_comparison(const std::vector<float>& input, 
                                   const std::vector<float>& output);
    void render_level_meters(float input_rms, float input_peak, 
                           float output_rms, float output_peak);

    // =============================================================================
    // PRIVATE UTILITY METHODS
    // =============================================================================

    void update_effect_chain_audio_processing();
    void update_performance_metrics();
    void update_automation_playback(float delta_time);
    
    // Parameter management
    void initialize_effect_parameters(EffectSlot& slot);
    void apply_parameter_changes(EffectSlot& slot);
    void load_factory_presets(EffectSlot& slot);
    
    // Audio analysis
    void perform_spectrum_analysis(const float* buffer, size_t size, std::vector<float>& spectrum);
    void calculate_audio_levels(const float* buffer, size_t size, float& rms, float& peak);
    
    // Chain management
    uint32_t generate_slot_id();
    void rebuild_effect_chain();
    void update_effect_connections();
    
    // UI helpers
    void handle_drag_and_drop();
    void handle_effect_selection();
    ImVec2 get_effect_slot_screen_position(uint32_t slot_id) const;
    bool is_point_in_slot(const ImVec2& point, const EffectSlot& slot) const;

    // =============================================================================
    // MEMBER VARIABLES
    // =============================================================================

    // Core state
    bool initialized_ = false;
    audio::AudioPipeline* audio_pipeline_ = nullptr;
    
    // Effect chain data
    std::vector<EffectSlot> effect_slots_;
    std::vector<EffectConnection> effect_connections_;
    std::vector<EffectAutomation> effect_automations_;
    uint32_t next_slot_id_ = 1;
    
    // UI state
    uint32_t selected_slot_id_ = 0;
    bool show_parameter_panel_ = true;
    bool show_preset_browser_ = false;
    bool show_audio_analysis_ = true;
    bool show_automation_panel_ = false;
    bool show_performance_panel_ = true;
    
    // Editor modes
    enum class EditorMode {
        Overview,       ///< Full chain overview
        RackView,       ///< Traditional rack view
        GraphView,      ///< Node graph view
        MixerView       ///< Mixer-style view
    } current_mode_ = EditorMode::Overview;
    
    // Drag and drop state
    bool dragging_effect_ = false;
    uint32_t dragged_slot_id_ = 0;
    ImVec2 drag_offset_{0.0f, 0.0f};
    
    // Audio analysis
    bool audio_analysis_enabled_ = true;
    std::mutex analysis_mutex_;
    static constexpr size_t SPECTRUM_SIZE = 512;
    static constexpr size_t WAVEFORM_SIZE = 1024;
    
    // Performance monitoring
    bool performance_monitoring_enabled_ = true;
    std::queue<float> processing_time_history_;
    std::queue<float> cpu_usage_history_;
    static constexpr size_t MAX_PERFORMANCE_HISTORY = 300;
    
    // Automation
    float automation_time_ = 0.0f;
    bool automation_playing_ = false;
    std::mutex automation_mutex_;
    
    // Available effects registry
    std::vector<std::string> available_effects_;
    std::unordered_map<std::string, std::function<std::shared_ptr<audio::AudioEffect>()>> effect_factories_;
    
    // Preset management
    std::unordered_map<std::string, std::vector<EffectPreset>> effect_presets_;
    std::string preset_search_filter_;
    
    // Configuration
    std::string config_file_path_ = "effects_chain.json";
    
    // Thread safety
    mutable std::mutex slots_mutex_;
};

// =============================================================================
// SPECIALIZED EFFECT WIDGETS
// =============================================================================

/**
 * @brief EQ Curve Editor Widget
 */
class EQCurveEditor {
public:
    EQCurveEditor();
    ~EQCurveEditor();
    
    void render(const std::string& label, std::vector<ImVec2>& curve_points, 
               const ImVec2& size = ImVec2(400, 200));
    void set_frequency_range(float min_freq, float max_freq);
    void set_gain_range(float min_gain, float max_gain);
    
private:
    float min_frequency_ = 20.0f;
    float max_frequency_ = 20000.0f;
    float min_gain_ = -24.0f;
    float max_gain_ = 24.0f;
    int selected_point_ = -1;
};

/**
 * @brief Compressor Visualization Widget
 */
class CompressorVisualizer {
public:
    CompressorVisualizer();
    ~CompressorVisualizer();
    
    void render(float threshold, float ratio, float attack, float release, 
               float makeup_gain, const ImVec2& size = ImVec2(200, 200));
    
private:
    std::vector<float> gain_reduction_history_;
    std::vector<float> input_level_history_;
    std::vector<float> output_level_history_;
};

/**
 * @brief Reverb Visualization Widget
 */
class ReverbVisualizer {
public:
    ReverbVisualizer();
    ~ReverbVisualizer();
    
    void render(float room_size, float damping, float wet_level, float dry_level,
               const ImVec2& size = ImVec2(300, 200));
    
private:
    std::vector<ImVec2> impulse_response_;
    float visualization_time_ = 0.0f;
};

/**
 * @brief Delay Tap Editor Widget
 */
class DelayTapEditor {
public:
    struct DelayTap {
        float delay_time = 0.0f;
        float feedback = 0.0f;
        float level = 1.0f;
        float pan = 0.0f;
        bool enabled = true;
    };
    
    DelayTapEditor();
    ~DelayTapEditor();
    
    void render(std::vector<DelayTap>& taps, float max_delay_time = 2.0f,
               const ImVec2& size = ImVec2(400, 150));
    
private:
    int selected_tap_ = -1;
    bool dragging_tap_ = false;
};

// =============================================================================
// UTILITY FUNCTIONS
// =============================================================================

/**
 * @brief Create default effect parameter descriptor
 */
EffectParameter create_float_parameter(const std::string& name, const std::string& display_name,
                                     float min_val, float max_val, float default_val,
                                     const std::string& unit = "");

/**
 * @brief Create boolean parameter descriptor
 */
EffectParameter create_boolean_parameter(const std::string& name, const std::string& display_name,
                                       bool default_val);

/**
 * @brief Create enum parameter descriptor
 */
EffectParameter create_enum_parameter(const std::string& name, const std::string& display_name,
                                    const std::vector<std::string>& options, int default_index = 0);

/**
 * @brief Convert decibel value to linear gain
 */
float db_to_linear(float db);

/**
 * @brief Convert linear gain to decibel value
 */
float linear_to_db(float linear);

/**
 * @brief Format frequency for display
 */
std::string format_frequency_display(float frequency);

/**
 * @brief Format time for display
 */
std::string format_time_display(float time_seconds);

/**
 * @brief Format gain/level for display
 */
std::string format_gain_display(float gain_db);

} // namespace ecscope::gui