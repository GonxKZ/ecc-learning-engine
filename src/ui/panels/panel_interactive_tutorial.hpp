#pragma once

/**
 * @file panel_interactive_tutorial.hpp
 * @brief Interactive Tutorial Panel - Real-time guided learning interface
 * 
 * This panel provides an immersive, step-by-step learning experience for ECS concepts
 * with real-time guidance, interactive elements, visual cues, and adaptive feedback.
 * 
 * Features:
 * - Step-by-step guided tutorials with visual progression
 * - Interactive code examples with syntax highlighting
 * - Real-time validation and feedback
 * - Visual cues and highlighting of UI elements
 * - Adaptive difficulty and personalized hints
 * - Progress tracking with achievement system
 * - Integration with visual debugger and performance tools
 * 
 * Educational Design Principles:
 * - Scaffolded learning with progressive disclosure
 * - Just-in-time information delivery
 * - Multi-modal learning support (visual, kinesthetic, textual)
 * - Immediate feedback and validation
 * - Gamification elements for engagement
 * 
 * @author ECScope Educational Framework
 * @date 2024
 */

#include "../overlay.hpp"
#include "../../learning/tutorial_system.hpp"
#include "core/types.hpp"
#include <memory>
#include <string>
#include <vector>
#include <chrono>
#include <unordered_map>

namespace ecscope::ui {

/**
 * @brief Interactive Tutorial UI Panel
 */
class InteractiveTutorialPanel : public Panel {
private:
    // Tutorial system integration
    std::shared_ptr<learning::TutorialManager> tutorial_manager_;
    learning::Tutorial* current_tutorial_{nullptr};
    learning::TutorialStep* current_step_{nullptr};
    
    // UI State
    enum class PanelMode : u8 {
        TutorialSelection,  // Browse and select tutorials
        ActiveTutorial,     // Currently running tutorial
        StepExecution,      // Executing current step
        CodeEditor,         // Interactive code editing
        QuizMode,           // Quiz and assessment
        ProgressReview,     // Review progress and achievements
        HelpSystem          // Context-sensitive help
    };
    
    PanelMode current_mode_{PanelMode::TutorialSelection};
    bool tutorial_active_{false};
    bool step_validation_pending_{false};
    
    // Tutorial browser state
    struct TutorialBrowser {
        learning::TutorialCategory selected_category_{learning::TutorialCategory::BasicConcepts};
        learning::DifficultyLevel filter_difficulty_{learning::DifficultyLevel::Beginner};
        std::string search_query_;
        std::vector<learning::Tutorial*> filtered_tutorials_;
        int selected_tutorial_index_{-1};
        bool show_completed_{false};
        bool show_prerequisites_{true};
        
        // Display options
        bool show_category_tabs_{true};
        bool show_difficulty_filter_{true};
        bool show_progress_indicators_{true};
        bool show_time_estimates_{true};
    };
    
    TutorialBrowser browser_;
    
    // Step execution state
    struct StepExecution {
        std::chrono::steady_clock::time_point step_start_time_;
        f64 step_duration_{0.0};
        u32 validation_attempts_{0};
        bool show_hints_{false};
        bool show_detailed_explanation_{false};
        bool auto_advance_enabled_{true};
        f32 completion_animation_{0.0f}; // 0.0 to 1.0 for step completion animation
        
        // Visual cue rendering
        std::vector<learning::VisualCue> active_cues_;
        std::unordered_map<std::string, f32> cue_animations_; // element_id -> animation progress
        f64 cue_pulse_phase_{0.0};
        
        // Feedback display
        std::string last_feedback_message_;
        f32 feedback_display_timer_{0.0f};
        bool show_success_animation_{false};
        f32 success_animation_progress_{0.0f};
    };
    
    StepExecution step_execution_;
    
    // Code editor state
    struct CodeEditor {
        std::string current_code_;
        std::string original_template_;
        std::vector<std::string> available_hints_;
        u32 current_hint_level_{0};
        bool syntax_highlighting_enabled_{true};
        bool auto_completion_enabled_{true};
        bool show_line_numbers_{true};
        f32 editor_height_{300.0f};
        
        // Execution state
        bool can_execute_{false};
        bool is_executing_{false};
        std::string execution_output_;
        std::string expected_output_;
        bool show_expected_{false};
        
        // Validation
        learning::ValidationResult last_validation_;
        bool validation_in_progress_{false};
        
        // Visual feedback
        std::vector<std::pair<u32, std::string>> syntax_errors_; // line -> error message
        std::vector<u32> highlighted_lines_;
        f32 error_highlight_intensity_{0.0f};
    };
    
    CodeEditor code_editor_;
    
    // Progress display
    struct ProgressDisplay {
        f32 overall_progress_{0.0f};          // Current tutorial progress
        f32 step_progress_{0.0f};             // Current step progress
        u32 total_steps_{0};
        u32 completed_steps_{0};
        
        // Learning analytics
        f64 session_time_{0.0};
        u32 total_attempts_{0};
        u32 successful_validations_{0};
        f32 learning_velocity_{0.0f};         // Steps per minute
        
        // Achievement tracking
        std::vector<std::string> session_achievements_;
        bool show_achievement_popup_{false};
        std::string current_achievement_;
        f32 achievement_popup_timer_{0.0f};
        
        // Progress visualization
        bool show_progress_graph_{true};
        bool show_time_spent_{true};
        bool show_difficulty_adaptation_{true};
        std::vector<f32> progress_history_; // For graphing progress over time
    };
    
    ProgressDisplay progress_;
    
    // Help system
    struct HelpSystem {
        bool context_help_enabled_{true};
        bool smart_hints_enabled_{true};
        std::string current_help_topic_;
        std::vector<std::string> help_history_;
        
        // Adaptive help
        std::unordered_map<std::string, u32> help_request_count_; // topic -> count
        std::vector<std::string> frequently_needed_help_;
        bool show_proactive_hints_{true};
        f32 hint_relevance_threshold_{0.7f};
        
        // Help display
        bool show_help_sidebar_{false};
        f32 help_sidebar_width_{250.0f};
        std::string help_content_;
        bool help_content_expanded_{false};
    };
    
    HelpSystem help_system_;
    
    // Visual effects and animations
    struct VisualEffects {
        // Step transition animations
        f32 step_transition_progress_{0.0f};
        bool transitioning_steps_{false};
        learning::TutorialStep* previous_step_{nullptr};
        
        // Highlight animations
        f32 highlight_pulse_frequency_{2.0f};  // Hz
        f32 highlight_intensity_{0.8f};
        std::unordered_map<std::string, f32> element_highlights_; // element_id -> intensity
        
        // Success/failure animations
        f32 success_particle_life_{3.0f};      // seconds
        f32 error_shake_intensity_{5.0f};       // pixels
        std::vector<std::pair<f32, f32>> success_particles_; // x, y positions
        f32 error_shake_timer_{0.0f};
        
        // UI animations
        bool smooth_transitions_{true};
        f32 animation_speed_multiplier_{1.0f};
        bool reduce_motion_{false}; // Accessibility option
    };
    
    VisualEffects effects_;
    
    // Learner profile
    struct LearnerProfile {
        std::string learner_id_{"default_learner"};
        learning::DifficultyLevel preferred_difficulty_{learning::DifficultyLevel::Beginner};
        std::vector<learning::TutorialCategory> preferred_categories_;
        
        // Learning preferences
        bool prefer_visual_learning_{true};
        bool prefer_hands_on_practice_{true};
        bool prefer_detailed_explanations_{false};
        f32 preferred_pacing_speed_{1.0f}; // 1.0 = normal
        
        // Accessibility preferences
        bool high_contrast_mode_{false};
        bool large_text_mode_{false};
        bool screen_reader_mode_{false};
        f32 ui_scale_factor_{1.0f};
        
        // Adaptive learning data
        std::unordered_map<std::string, f32> concept_mastery_; // concept -> mastery level
        std::vector<std::string> struggling_concepts_;
        std::vector<std::string> mastered_concepts_;
    };
    
    LearnerProfile learner_;
    
    // Color schemes and styling
    struct UITheme {
        // Tutorial step states
        static constexpr f32 step_pending_[4] = {0.6f, 0.6f, 0.6f, 1.0f};      // Gray
        static constexpr f32 step_active_[4] = {0.2f, 0.7f, 1.0f, 1.0f};       // Blue
        static constexpr f32 step_completed_[4] = {0.2f, 0.8f, 0.3f, 1.0f};    // Green
        static constexpr f32 step_error_[4] = {1.0f, 0.3f, 0.3f, 1.0f};        // Red
        
        // Interactive elements
        static constexpr f32 highlight_active_[4] = {1.0f, 0.8f, 0.2f, 0.8f};  // Bright yellow
        static constexpr f32 highlight_hover_[4] = {0.9f, 0.9f, 0.5f, 0.6f};   // Light yellow
        static constexpr f32 success_glow_[4] = {0.3f, 1.0f, 0.3f, 0.4f};      // Green glow
        static constexpr f32 error_glow_[4] = {1.0f, 0.2f, 0.2f, 0.4f};        // Red glow
        
        // Progress indicators
        static constexpr f32 progress_bg_[4] = {0.2f, 0.2f, 0.2f, 1.0f};       // Dark gray
        static constexpr f32 progress_fill_[4] = {0.2f, 0.8f, 0.4f, 1.0f};     // Green
        static constexpr f32 progress_text_[4] = {1.0f, 1.0f, 1.0f, 1.0f};     // White
        
        // Code editor
        static constexpr f32 code_bg_[4] = {0.1f, 0.1f, 0.1f, 1.0f};           // Very dark
        static constexpr f32 code_text_[4] = {0.9f, 0.9f, 0.9f, 1.0f};         // Light gray
        static constexpr f32 code_keyword_[4] = {0.5f, 0.8f, 1.0f, 1.0f};      // Light blue
        static constexpr f32 code_string_[4] = {0.8f, 0.6f, 0.8f, 1.0f};       // Light purple
        static constexpr f32 code_comment_[4] = {0.5f, 0.7f, 0.5f, 1.0f};      // Green
        static constexpr f32 code_error_[4] = {1.0f, 0.4f, 0.4f, 1.0f};        // Red
    };
    
    // Rendering methods
    void render_tutorial_selection();
    void render_active_tutorial();
    void render_step_execution();
    void render_code_editor();
    void render_quiz_mode();
    void render_progress_review();
    void render_help_system();
    
    // Tutorial browser methods
    void render_category_tabs();
    void render_difficulty_filter();
    void render_tutorial_list();
    void render_tutorial_preview(learning::Tutorial* tutorial);
    void update_filtered_tutorials();
    
    // Step execution methods
    void render_step_header();
    void render_step_content();
    void render_step_navigation();
    void render_visual_cues();
    void render_validation_feedback();
    void animate_step_transitions(f64 delta_time);
    
    // Code editor methods
    void render_code_editor_header();
    void render_code_input();
    void render_code_output();
    void render_execution_controls();
    void render_syntax_highlighting();
    void handle_code_validation();
    
    // Progress tracking methods
    void render_progress_header();
    void render_progress_indicators();
    void render_learning_analytics();
    void render_achievement_system();
    void update_progress_tracking(f64 delta_time);
    
    // Help system methods
    void render_context_help();
    void render_help_sidebar();
    void render_smart_hints();
    void update_adaptive_help();
    
    // Visual effects methods
    void update_animations(f64 delta_time);
    void render_highlight_effects();
    void render_particle_effects();
    void trigger_success_animation();
    void trigger_error_animation();
    
    // Input handling
    void handle_tutorial_selection_input();
    void handle_step_navigation_input();
    void handle_code_editor_input();
    void handle_keyboard_shortcuts();
    
    // Tutorial flow management
    void start_selected_tutorial();
    void advance_to_next_step();
    void return_to_previous_step();
    void complete_current_tutorial();
    void abandon_current_tutorial();
    
    // Validation and feedback
    void validate_current_step();
    void provide_step_feedback(const learning::ValidationResult& result);
    void show_hint_if_needed();
    void adapt_difficulty_if_needed();
    
    // Utility methods
    void load_learner_preferences();
    void save_learner_progress();
    void apply_accessibility_settings();
    void update_tutorial_recommendations();
    std::string format_time_duration(f64 seconds) const;
    std::string get_difficulty_display_name(learning::DifficultyLevel level) const;
    std::string get_category_display_name(learning::TutorialCategory category) const;
    f32 calculate_estimated_time(learning::Tutorial* tutorial) const;
    
    // Event handlers
    void on_tutorial_started(const std::string& tutorial_id);
    void on_tutorial_completed(const std::string& tutorial_id);
    void on_step_completed(const std::string& step_id);
    void on_validation_failed(const learning::ValidationResult& result);
    void on_achievement_unlocked(const std::string& achievement);
    
public:
    explicit InteractiveTutorialPanel(std::shared_ptr<learning::TutorialManager> manager);
    ~InteractiveTutorialPanel() override = default;
    
    // Panel interface
    void render() override;
    void update(f64 delta_time) override;
    bool wants_keyboard_capture() const override;
    bool wants_mouse_capture() const override;
    
    // Tutorial control
    void set_tutorial_manager(std::shared_ptr<learning::TutorialManager> manager);
    void start_tutorial(const std::string& tutorial_id);
    void pause_current_tutorial();
    void resume_current_tutorial();
    void reset_current_tutorial();
    
    // Panel configuration
    void set_learner_id(const std::string& learner_id);
    void set_panel_mode(PanelMode mode);
    void enable_visual_cues(bool enabled);
    void enable_smart_hints(bool enabled);
    void set_auto_advance(bool enabled);
    
    // Accessibility
    void set_high_contrast_mode(bool enabled);
    void set_large_text_mode(bool enabled);
    void set_ui_scale_factor(f32 scale);
    void set_reduce_motion(bool reduce);
    
    // Integration with other panels
    void highlight_ui_element(const std::string& element_id, f32 duration = 0.0f);
    void remove_ui_highlight(const std::string& element_id);
    void trigger_visual_cue(const learning::VisualCue& cue);
    void show_contextual_help(const std::string& topic);
    
    // Data export and analysis
    void export_learning_progress();
    void export_session_analytics();
    learning::TutorialManager::LearningAnalytics get_current_analytics() const;
    
private:
    // Constants
    static constexpr f32 MIN_PANEL_WIDTH = 500.0f;
    static constexpr f32 MIN_PANEL_HEIGHT = 400.0f;
    static constexpr f32 DEFAULT_CODE_EDITOR_HEIGHT = 300.0f;
    static constexpr f32 HELP_SIDEBAR_WIDTH = 250.0f;
    static constexpr f32 ANIMATION_SPEED = 2.0f;
    static constexpr f64 STEP_TIMEOUT_SECONDS = 300.0; // 5 minutes
    static constexpr f32 HINT_DELAY_SECONDS = 15.0f;
    static constexpr u32 MAX_VALIDATION_ATTEMPTS = 5;
    static constexpr f32 SUCCESS_ANIMATION_DURATION = 2.0f;
    static constexpr f32 ERROR_ANIMATION_DURATION = 0.5f;
    
    // Update frequencies
    static constexpr f64 PROGRESS_UPDATE_FREQUENCY = 0.5; // 2 Hz
    static constexpr f64 ANALYTICS_UPDATE_FREQUENCY = 1.0; // 1 Hz
    static constexpr f64 HELP_UPDATE_FREQUENCY = 2.0;     // 0.5 Hz
    
    f64 last_progress_update_{0.0};
    f64 last_analytics_update_{0.0};
    f64 last_help_update_{0.0};
};

/**
 * @brief Specialized code highlighting widget for tutorials
 */
class TutorialCodeHighlighter {
private:
    struct SyntaxRule {
        std::string pattern;
        const f32* color;
        bool is_regex;
        
        SyntaxRule(const std::string& pat, const f32* col, bool regex = false)
            : pattern(pat), color(col), is_regex(regex) {}
    };
    
    std::vector<SyntaxRule> cpp_syntax_rules_;
    std::vector<SyntaxRule> ecs_syntax_rules_;
    bool rules_initialized_{false};
    
    void initialize_cpp_syntax_rules();
    void initialize_ecs_syntax_rules();
    
public:
    TutorialCodeHighlighter();
    
    void highlight_code(const std::string& code, const std::string& language = "cpp");
    void render_highlighted_text(const std::string& text, f32 wrap_width = 0.0f);
    
    // Error highlighting
    void highlight_error_lines(const std::vector<std::pair<u32, std::string>>& errors);
    void highlight_success_lines(const std::vector<u32>& lines);
    
    // Interactive elements
    void render_clickable_identifier(const std::string& identifier, 
                                   const std::string& tooltip = "",
                                   std::function<void()> on_click = nullptr);
    void render_editable_region(const std::string& content, 
                               std::function<void(const std::string&)> on_change = nullptr);
    
    // Configuration
    void set_font_size(f32 size);
    void set_line_height(f32 height);
    void enable_line_numbers(bool enable);
    void set_tab_size(u32 spaces);
};

/**
 * @brief Interactive quiz widget for tutorial assessments
 */
class TutorialQuizWidget {
public:
    enum class QuestionType : u8 {
        MultipleChoice,
        TrueFalse,
        CodeCompletion,
        DragAndDrop,
        Ordering,
        FillInBlank,
        CodeExplanation
    };
    
    struct QuizQuestion {
        std::string id;
        std::string question_text;
        QuestionType type;
        std::vector<std::string> options;
        std::vector<usize> correct_answers; // Indices of correct options
        std::string explanation;
        std::string code_context; // For code-related questions
        u32 points_value{1};
        f32 time_limit{0.0f}; // 0 = no limit
        
        // Feedback
        std::string correct_feedback;
        std::string incorrect_feedback;
        std::vector<std::string> option_feedback; // Per-option feedback
    };
    
private:
    std::vector<QuizQuestion> questions_;
    usize current_question_index_{0};
    std::vector<std::vector<usize>> user_answers_; // Per question, selected options
    std::vector<bool> questions_answered_;
    
    // Quiz state
    bool quiz_active_{false};
    bool quiz_completed_{false};
    std::chrono::steady_clock::time_point quiz_start_time_;
    std::chrono::steady_clock::time_point question_start_time_;
    
    // Scoring
    u32 total_points_{0};
    u32 earned_points_{0};
    f32 completion_percentage_{0.0f};
    
    // Visual feedback
    bool show_immediate_feedback_{true};
    bool show_explanations_after_answer_{false};
    f32 feedback_display_time_{3.0f};
    
public:
    TutorialQuizWidget() = default;
    
    // Quiz management
    void add_question(const QuizQuestion& question);
    void start_quiz();
    void reset_quiz();
    void submit_current_answer();
    void next_question();
    void previous_question();
    void complete_quiz();
    
    // Rendering
    void render();
    void render_question_navigation();
    void render_current_question();
    void render_quiz_results();
    
    // State queries
    bool is_quiz_active() const { return quiz_active_; }
    bool is_quiz_completed() const { return quiz_completed_; }
    usize current_question() const { return current_question_index_; }
    usize total_questions() const { return questions_.size(); }
    f32 completion_percentage() const { return completion_percentage_; }
    u32 score() const { return earned_points_; }
    u32 max_score() const { return total_points_; }
    
    // Configuration
    void enable_immediate_feedback(bool enable) { show_immediate_feedback_ = enable; }
    void enable_explanations_after_answer(bool enable) { show_explanations_after_answer_ = enable; }
    void set_feedback_display_time(f32 time) { feedback_display_time_ = time; }
    
private:
    void render_multiple_choice_question(const QuizQuestion& question);
    void render_true_false_question(const QuizQuestion& question);
    void render_code_completion_question(const QuizQuestion& question);
    void render_drag_and_drop_question(const QuizQuestion& question);
    void render_ordering_question(const QuizQuestion& question);
    void render_fill_in_blank_question(const QuizQuestion& question);
    void render_code_explanation_question(const QuizQuestion& question);
    
    bool is_answer_correct(usize question_index, const std::vector<usize>& user_answer) const;
    void calculate_score();
    void show_question_feedback(usize question_index);
};

} // namespace ecscope::ui