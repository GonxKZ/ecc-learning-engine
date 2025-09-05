#pragma once

/**
 * @file panel_educational_features.hpp
 * @brief Educational Features Panel - Comprehensive learning and assessment system
 * 
 * This panel provides a complete educational ecosystem for learning ECS concepts,
 * featuring adaptive learning algorithms, interactive quizzes, progress tracking,
 * achievements, and personalized learning paths tailored to individual learning styles.
 * 
 * Features:
 * - Adaptive learning system that adjusts to learner's pace and comprehension
 * - Comprehensive quiz system with multiple question types
 * - Real-time progress tracking with detailed analytics
 * - Achievement and badge system for motivation
 * - Personalized learning recommendations
 * - Interactive code challenges and exercises
 * - Collaborative learning features (future)
 * - Accessibility support for diverse learning needs
 * 
 * Educational Principles:
 * - Mastery-based learning with spaced repetition
 * - Multi-modal content delivery (visual, auditory, kinesthetic)
 * - Immediate feedback and reinforcement
 * - Scaffolded learning with progressive complexity
 * - Personalized learning paths based on performance
 * 
 * @author ECScope Educational Framework
 * @date 2024
 */

#include "../overlay.hpp"
#include "../../learning/tutorial_system.hpp"
#include "core/types.hpp"
#include <memory>
#include <vector>
#include <unordered_map>
#include <string>
#include <chrono>
#include <functional>
#include <queue>

namespace ecscope::ui {

// Forward declarations
class EducationalFeaturesPanel;
class AdaptiveLearningEngine;
class QuizSystem;
class ProgressTracker;
class AchievementSystem;

/**
 * @brief Learning activity types
 */
enum class LearningActivityType : u8 {
    Tutorial,           // Step-by-step guided tutorial
    Quiz,               // Assessment quiz
    Exercise,           // Hands-on coding exercise
    Demonstration,      // Interactive demonstration
    Reflection,         // Self-reflection questions
    Challenge,          // Advanced problem-solving
    Review,             // Review of previous concepts
    Assessment          // Formal assessment
};

/**
 * @brief Question types for quizzes and assessments
 */
enum class QuestionType : u8 {
    MultipleChoice,     // Single correct answer from options
    MultipleSelect,     // Multiple correct answers from options
    TrueFalse,          // Binary true/false question
    FillInBlank,        // Fill in missing text
    CodeCompletion,     // Complete code snippet
    CodeExplanation,    // Explain what code does
    OrderingSequence,   // Put items in correct order
    Matching,           // Match items in two lists
    ShortAnswer,        // Brief text response
    CodeDebug,          // Find and fix code errors
    PerformanceAnalysis, // Analyze performance characteristics
    ConceptMapping      // Connect related concepts
};

/**
 * @brief Learning difficulty levels with adaptive progression
 */
enum class AdaptiveDifficulty : u8 {
    Novice,            // Complete beginner
    Beginner,          // Basic understanding
    Intermediate,      // Moderate proficiency
    Advanced,          // High proficiency
    Expert,            // Mastery level
    Adaptive           // AI-determined difficulty
};

/**
 * @brief Learning style preferences
 */
enum class LearningStyle : u8 {
    Visual,            // Learn through visual representations
    Auditory,          // Learn through explanations and discussion
    Kinesthetic,       // Learn through hands-on practice
    Reading,           // Learn through text and documentation
    Mixed,             // Combination of styles
    Adaptive           // AI-determined optimal style
};

/**
 * @brief Quiz question with multiple formats and validation
 */
struct QuizQuestion {
    std::string id;
    std::string title;
    std::string question_text;
    QuestionType type;
    AdaptiveDifficulty difficulty;
    
    // Question options and answers
    std::vector<std::string> options;           // For multiple choice/select
    std::vector<usize> correct_answers;         // Indices of correct options
    std::string correct_text_answer;            // For text-based questions
    std::string code_template;                  // For coding questions
    std::string expected_code_output;           // Expected result
    
    // Question metadata
    std::vector<std::string> topics;            // Related topics/concepts
    std::vector<std::string> prerequisites;     // Required prior knowledge
    f32 estimated_time_minutes{2.0f};          // Expected completion time
    u32 points_value{1};                       // Points awarded for correct answer
    
    // Educational content
    std::string explanation;                    // Detailed explanation of answer
    std::string learning_objective;             // What this question teaches
    std::vector<std::string> hints;            // Progressive hints
    std::string reference_material;             // Links to related content
    
    // Adaptive learning data
    f32 difficulty_score{0.5f};               // AI-computed difficulty (0-1)
    f32 discrimination_score{0.5f};           // How well it separates skill levels
    u32 times_asked{0};                       // Usage statistics
    u32 times_correct{0};                     // Success rate
    f64 average_time_taken{0.0};              // Average completion time
    
    // Feedback and validation
    std::function<bool(const std::string&)> custom_validator;
    std::unordered_map<std::string, std::string> wrong_answer_feedback; // Answer -> feedback
    bool requires_explanation{false};          // Require explanation of answer
    bool allows_partial_credit{false};         // Award partial points
    
    QuizQuestion() = default;
    QuizQuestion(const std::string& question_id, const std::string& text, QuestionType q_type)
        : id(question_id), question_text(text), type(q_type) {}
};

/**
 * @brief Quiz session with comprehensive tracking
 */
struct QuizSession {
    std::string session_id;
    std::string quiz_name;
    std::string learner_id;
    std::vector<QuizQuestion> questions;
    
    // Session state
    usize current_question_index{0};
    std::vector<std::string> user_answers;     // User's answers per question
    std::vector<f64> question_times;          // Time spent per question
    std::vector<bool> question_correct;        // Correctness per question
    std::vector<u32> hint_usage;              // Hints used per question
    
    // Session timing
    std::chrono::steady_clock::time_point start_time;
    std::chrono::steady_clock::time_point end_time;
    f64 total_time_seconds{0.0};
    f64 time_limit_seconds{0.0};              // 0 = no limit
    
    // Scoring and results
    u32 total_points{0};
    u32 earned_points{0};
    f32 percentage_score{0.0f};
    std::string grade;                         // Letter grade or pass/fail
    bool is_passing{false};
    f32 passing_threshold{0.7f};              // 70% default
    
    // Session configuration
    bool allow_review{true};                   // Can review answers
    bool show_correct_answers{true};           // Show correct answers at end
    bool randomize_questions{false};           // Randomize question order
    bool randomize_options{false};             // Randomize option order
    u32 max_attempts{1};                      // Retake limit
    u32 current_attempt{1};
    
    // Analytics and feedback
    std::vector<std::string> areas_of_strength;
    std::vector<std::string> areas_for_improvement;
    std::vector<std::string> recommended_study_topics;
    std::string overall_feedback;
    
    QuizSession() = default;
    QuizSession(const std::string& id, const std::string& name, const std::string& learner)
        : session_id(id), quiz_name(name), learner_id(learner) {}
};

/**
 * @brief Learning achievement with unlock conditions
 */
struct LearningAchievement {
    std::string id;
    std::string name;
    std::string description;
    std::string icon_path;                     // Path to achievement icon
    
    // Achievement criteria
    enum class UnlockType : u8 {
        TutorialsCompleted,                    // Complete N tutorials
        QuizzesPassedWithScore,                // Pass N quizzes with min score
        ConsecutiveCorrectAnswers,             // Answer N questions correctly in a row
        TimeSpentLearning,                     // Spend N minutes learning
        ConceptsMastered,                      // Master N different concepts
        PerfectQuizScore,                      // Get 100% on a quiz
        ImprovementStreak,                     // Show consistent improvement
        HelpOthers,                            // Help other learners (future)
        CustomCondition                        // Custom unlock condition
    } unlock_type;
    
    u32 required_count{1};                     // Required number/threshold
    f32 required_score{0.0f};                 // Required score threshold
    std::function<bool(const learning::LearningProgress&)> custom_condition;
    
    // Achievement metadata
    enum class Rarity : u8 {
        Common,       // Easy to achieve
        Uncommon,     // Moderate effort required
        Rare,         // Significant effort required
        Epic,         // Major accomplishment
        Legendary     // Exceptional achievement
    } rarity{Rarity::Common};
    
    u32 points_value{10};                      // Points awarded
    bool is_hidden{false};                     // Hidden until unlocked
    bool is_retroactive{true};                 // Can be unlocked from past progress
    
    // Display information
    std::string unlock_message;                // Message shown when unlocked
    std::string progress_format;               // Format string for progress display
    
    LearningAchievement() = default;
    LearningAchievement(const std::string& achievement_id, const std::string& achievement_name, 
                       const std::string& desc, UnlockType type)
        : id(achievement_id), name(achievement_name), description(desc), unlock_type(type) {}
};

/**
 * @brief Comprehensive learning progress tracking
 */
struct DetailedLearningProgress {
    std::string learner_id;
    std::chrono::steady_clock::time_point first_session;
    std::chrono::steady_clock::time_point last_session;
    
    // Overall statistics
    f64 total_learning_time_hours{0.0};
    u32 total_sessions{0};
    u32 tutorials_completed{0};
    u32 quizzes_taken{0};
    u32 quizzes_passed{0};
    f32 overall_quiz_average{0.0f};
    
    // Progress by topic/concept
    std::unordered_map<std::string, f32> topic_mastery;      // topic -> mastery level (0-1)
    std::unordered_map<std::string, u32> topic_time_spent;   // topic -> minutes
    std::unordered_map<std::string, u32> topic_attempts;     // topic -> attempt count
    
    // Learning velocity and patterns
    std::vector<std::pair<std::chrono::steady_clock::time_point, f32>> progress_timeline;
    f32 current_learning_velocity{0.0f};      // Concepts mastered per hour
    std::vector<std::string> preferred_learning_times; // Times of day for peak performance
    
    // Adaptive learning data
    AdaptiveDifficulty current_difficulty{AdaptiveDifficulty::Beginner};
    LearningStyle preferred_style{LearningStyle::Mixed};
    f32 attention_span_minutes{20.0f};        // Estimated optimal session length
    f32 retention_rate{0.7f};                 // How well concepts are retained
    
    // Achievement tracking
    std::vector<std::string> unlocked_achievements;
    u32 total_achievement_points{0};
    std::string current_rank;                 // e.g., "Novice", "Apprentice", etc.
    
    // Behavioral patterns
    std::unordered_map<std::string, u32> common_mistakes;    // mistake -> frequency
    std::vector<std::string> strong_concepts;               // Well-understood topics
    std::vector<std::string> weak_concepts;                 // Topics needing work
    std::vector<std::string> abandoned_concepts;            // Topics given up on
    
    // Recommendations and interventions
    std::vector<std::string> recommended_next_topics;
    std::vector<std::string> suggested_review_topics;
    bool needs_motivational_intervention{false};
    bool shows_signs_of_struggle{false};
    f32 engagement_score{0.8f};               // 0-1 engagement level
    
    DetailedLearningProgress() = default;
    DetailedLearningProgress(const std::string& learner) : learner_id(learner) {
        first_session = std::chrono::steady_clock::now();
        last_session = first_session;
    }
};

/**
 * @brief Educational Features Panel - Main UI component
 */
class EducationalFeaturesPanel : public Panel {
private:
    // Core systems
    std::shared_ptr<learning::TutorialManager> tutorial_manager_;
    std::unique_ptr<AdaptiveLearningEngine> adaptive_engine_;
    std::unique_ptr<QuizSystem> quiz_system_;
    std::unique_ptr<ProgressTracker> progress_tracker_;
    std::unique_ptr<AchievementSystem> achievement_system_;
    
    // Panel modes
    enum class EducationMode : u8 {
        Dashboard,          // Main learning dashboard
        TutorialBrowser,    // Browse and select tutorials  
        QuizCenter,         // Quiz selection and taking
        ProgressAnalysis,   // Detailed progress analysis
        Achievements,       // Achievement gallery and tracking
        Settings,           // Educational preferences and settings
        LearningPath,       // Personalized learning path
        StudyGroups         // Collaborative learning (future)
    };
    
    EducationMode current_mode_{EducationMode::Dashboard};
    
    // Current learner state
    std::string current_learner_id_{"default_learner"};
    DetailedLearningProgress current_progress_;
    
    // Dashboard state
    struct DashboardState {
        // Quick stats
        f32 today_learning_minutes{0.0f};
        u32 this_week_quizzes_taken{0};
        f32 current_streak_days{0.0f};
        std::string next_recommended_activity;
        
        // Recent activity
        std::vector<std::string> recent_tutorials;
        std::vector<std::string> recent_quiz_results;
        std::vector<std::string> recent_achievements;
        
        // Goals and targets
        f32 daily_learning_goal_minutes{30.0f};
        u32 weekly_quiz_goal{3};
        f32 target_mastery_level{0.8f};
        
        // Motivational elements
        std::string daily_tip;
        std::string motivational_quote;
        f32 overall_completion_percentage{0.0f};
        
        DashboardState() = default;
    };
    
    DashboardState dashboard_;
    
    // Quiz center state
    struct QuizCenterState {
        std::vector<std::string> available_quiz_categories;
        std::string selected_category;
        std::vector<QuizSession> available_quizzes;
        std::unique_ptr<QuizSession> current_quiz_session;
        
        // Quiz history
        std::vector<QuizSession> completed_quizzes;
        
        // Quiz creation (for educators)
        bool quiz_creation_mode{false};
        QuizQuestion current_editing_question;
        
        QuizCenterState() = default;
    };
    
    QuizCenterState quiz_center_;
    
    // Progress analysis state
    struct ProgressAnalysisState {
        // Time range for analysis
        enum class TimeRange : u8 {
            LastWeek,
            LastMonth,
            LastQuarter,
            AllTime,
            Custom
        } selected_time_range{TimeRange::LastMonth};
        
        std::chrono::steady_clock::time_point custom_start_time;
        std::chrono::steady_clock::time_point custom_end_time;
        
        // Analysis focus
        enum class AnalysisFocus : u8 {
            Overall,
            ByTopic,
            ByDifficulty,
            ByLearningStyle,
            Comparative
        } analysis_focus{AnalysisFocus::Overall};
        
        // Chart and visualization options
        bool show_trend_lines{true};
        bool show_averages{true};
        bool show_predictions{false};
        f32 chart_time_scale{1.0f};
        
        ProgressAnalysisState() = default;
    };
    
    ProgressAnalysisState progress_analysis_;
    
    // Settings state
    struct EducationSettings {
        // Learning preferences
        AdaptiveDifficulty preferred_difficulty{AdaptiveDifficulty::Adaptive};
        LearningStyle preferred_style{LearningStyle::Adaptive};
        bool enable_adaptive_learning{true};
        bool enable_spaced_repetition{true};
        
        // Notification and reminder settings
        bool enable_daily_reminders{true};
        bool enable_achievement_notifications{true};
        bool enable_progress_alerts{false};
        u32 reminder_hour{18}; // 6 PM default
        
        // Accessibility settings
        bool high_contrast_mode{false};
        bool large_text_mode{false};
        bool screen_reader_support{false};
        bool reduced_motion{false};
        f32 ui_scale{1.0f};
        
        // Privacy settings
        bool share_progress_with_instructors{true};
        bool allow_anonymous_analytics{true};
        bool enable_peer_comparison{false};
        
        // Performance settings
        bool preload_content{true};
        bool cache_quiz_data{true};
        u32 max_undo_levels{10};
        
        EducationSettings() = default;
    };
    
    EducationSettings settings_;
    
    // Rendering methods
    void render_dashboard();
    void render_tutorial_browser();
    void render_quiz_center();
    void render_progress_analysis();
    void render_achievements();
    void render_settings();
    void render_learning_path();
    void render_study_groups();
    
    // Dashboard components
    void render_quick_stats();
    void render_daily_goals();
    void render_recent_activity();
    void render_recommended_activities();
    void render_motivational_elements();
    void render_learning_streak();
    
    // Quiz center components
    void render_quiz_selection();
    void render_active_quiz();
    void render_quiz_results();
    void render_quiz_history();
    void render_quiz_creation_tools();
    
    // Progress analysis components
    void render_progress_overview();
    void render_mastery_heatmap();
    void render_learning_velocity_chart();
    void render_topic_breakdown();
    void render_difficulty_progression();
    void render_time_spent_analysis();
    void render_comparative_analysis();
    
    // Achievement components
    void render_achievement_gallery();
    void render_achievement_progress();
    void render_leaderboard();
    void render_badge_showcase();
    
    // Learning path components
    void render_path_overview();
    void render_next_steps();
    void render_prerequisite_checker();
    void render_custom_path_builder();
    
    // Interactive elements
    void render_concept_map();
    void render_skill_tree();
    void render_learning_calendar();
    void render_study_planner();
    
    // Data visualization helpers
    void render_progress_chart(const std::vector<f32>& data, const std::string& title, f32 height);
    void render_mastery_bar(const std::string& topic, f32 mastery_level, f32 width);
    void render_achievement_card(const LearningAchievement& achievement, bool unlocked);
    void render_quiz_question(const QuizQuestion& question, const std::string& user_answer = "");
    void render_learning_timeline(const std::vector<std::pair<std::chrono::steady_clock::time_point, std::string>>& events);
    
    // Quiz management
    void start_quiz(const std::string& quiz_name);
    void submit_quiz_answer(const std::string& answer);
    void finish_quiz();
    void review_quiz_results();
    void retake_quiz();
    
    // Progress tracking
    void update_learning_progress(const std::string& activity_type, const std::string& content_id, f32 score = 1.0f);
    void log_learning_session(f64 duration_minutes, const std::string& activity);
    void calculate_mastery_levels();
    void update_adaptive_parameters();
    
    // Achievement processing
    void check_achievement_unlocks();
    void unlock_achievement(const std::string& achievement_id);
    void calculate_achievement_progress();
    
    // Adaptive learning
    void adjust_difficulty_based_on_performance();
    void recommend_next_activities();
    void identify_knowledge_gaps();
    void schedule_spaced_repetition();
    
    // Data persistence
    void save_progress_data();
    void load_progress_data();
    void export_learning_analytics();
    void import_quiz_bank();
    
    // Utility methods
    std::string format_learning_time(f64 minutes) const;
    std::string format_mastery_level(f32 mastery) const;
    std::string get_difficulty_display_name(AdaptiveDifficulty difficulty) const;
    std::string get_learning_style_name(LearningStyle style) const;
    f32 calculate_overall_progress() const;
    std::vector<std::string> get_struggling_topics() const;
    std::vector<std::string> get_recommended_study_topics() const;
    
    // Event handlers
    void on_tutorial_completed(const std::string& tutorial_id);
    void on_quiz_completed(const QuizSession& session);
    void on_achievement_unlocked(const std::string& achievement_id);
    void on_mastery_level_increased(const std::string& topic, f32 new_level);
    void on_learning_goal_achieved(const std::string& goal_type);
    
public:
    explicit EducationalFeaturesPanel(std::shared_ptr<learning::TutorialManager> tutorial_mgr);
    ~EducationalFeaturesPanel() override = default;
    
    // Panel interface
    void render() override;
    void update(f64 delta_time) override;
    bool wants_keyboard_capture() const override;
    bool wants_mouse_capture() const override;
    
    // Learner management
    void set_current_learner(const std::string& learner_id);
    void create_new_learner_profile(const std::string& learner_id, const std::string& name = "");
    void switch_learner_profile(const std::string& learner_id);
    
    // Educational mode control
    void set_education_mode(EducationMode mode);
    void navigate_to_tutorial(const std::string& tutorial_id);
    void navigate_to_quiz(const std::string& quiz_id);
    void show_achievement_details(const std::string& achievement_id);
    
    // Quiz system integration
    void add_quiz(const std::string& quiz_name, const std::vector<QuizQuestion>& questions);
    void remove_quiz(const std::string& quiz_name);
    QuizSession create_quiz_session(const std::string& quiz_name);
    
    // Achievement system integration
    void register_achievement(const LearningAchievement& achievement);
    void remove_achievement(const std::string& achievement_id);
    std::vector<LearningAchievement> get_available_achievements() const;
    std::vector<std::string> get_unlocked_achievements() const;
    
    // Progress tracking integration
    void record_learning_activity(LearningActivityType type, const std::string& content_id, 
                                 f64 duration_minutes, f32 success_score = 1.0f);
    DetailedLearningProgress get_learning_progress() const { return current_progress_; }
    void reset_learning_progress();
    
    // Settings and preferences
    void apply_education_settings(const EducationSettings& settings);
    EducationSettings get_education_settings() const { return settings_; }
    void reset_settings_to_defaults();
    
    // Data import/export
    void export_progress_report(const std::string& filename, const std::string& format = "json");
    void import_quiz_questions(const std::string& filename);
    void backup_learner_data(const std::string& filename);
    void restore_learner_data(const std::string& filename);
    
    // Adaptive learning controls
    void enable_adaptive_difficulty(bool enabled);
    void set_learning_style_preference(LearningStyle style);
    void calibrate_learning_parameters();
    void reset_adaptive_parameters();
    
    // Integration with other panels
    void integrate_tutorial_panel(class InteractiveTutorialPanel* tutorial_panel);
    void integrate_debugger_panel(class VisualDebuggerPanel* debugger_panel);
    void integrate_performance_panel(class PerformanceComparisonPanel* performance_panel);
    
    // State queries
    EducationMode get_current_mode() const { return current_mode_; }
    std::string get_current_learner_id() const { return current_learner_id_; }
    bool has_active_quiz() const { return quiz_center_.current_quiz_session != nullptr; }
    f32 get_overall_progress() const { return calculate_overall_progress(); }
    u32 get_total_achievements_unlocked() const { return static_cast<u32>(current_progress_.unlocked_achievements.size()); }
    
private:
    // Constants
    static constexpr f32 MIN_PANEL_WIDTH = 800.0f;
    static constexpr f32 MIN_PANEL_HEIGHT = 600.0f;
    static constexpr f32 DASHBOARD_CARD_HEIGHT = 120.0f;
    static constexpr f32 PROGRESS_BAR_HEIGHT = 20.0f;
    static constexpr f32 ACHIEVEMENT_CARD_SIZE = 80.0f;
    static constexpr f32 QUIZ_QUESTION_MIN_HEIGHT = 100.0f;
    static constexpr u32 MAX_RECENT_ACTIVITIES = 10;
    static constexpr u32 MAX_QUIZ_HISTORY_DISPLAY = 20;
    static constexpr f64 AUTO_SAVE_INTERVAL = 300.0; // 5 minutes
    static constexpr f32 MASTERY_THRESHOLD = 0.8f;
    static constexpr f32 STRUGGLING_THRESHOLD = 0.3f;
    static constexpr u32 ACHIEVEMENT_UNLOCK_ANIMATION_DURATION = 3000; // ms
    
    // Update frequencies
    static constexpr f64 PROGRESS_UPDATE_FREQUENCY = 1.0;  // 1 Hz
    static constexpr f64 ACHIEVEMENT_CHECK_FREQUENCY = 0.5; // 0.5 Hz
    static constexpr f64 ADAPTIVE_UPDATE_FREQUENCY = 0.1;   // 0.1 Hz
    
    f64 last_progress_update_{0.0};
    f64 last_achievement_check_{0.0};
    f64 last_adaptive_update_{0.0};
    f64 last_auto_save_{0.0};
};

/**
 * @brief Adaptive Learning Engine - AI-powered learning optimization
 */
class AdaptiveLearningEngine {
private:
    struct LearnerModel {
        std::string learner_id;
        
        // Cognitive model parameters
        f32 processing_speed{1.0f};        // How quickly concepts are learned
        f32 working_memory_capacity{1.0f}; // How much complexity can be handled
        f32 attention_span{1.0f};          // Optimal session length multiplier
        f32 retention_rate{0.8f};          // How well information is retained
        
        // Learning preferences (inferred from behavior)
        std::unordered_map<LearningStyle, f32> style_affinities;
        std::unordered_map<QuestionType, f32> question_type_performance;
        std::unordered_map<std::string, f32> topic_affinities;
        
        // Performance predictors
        f32 quiz_performance_predictor{0.5f};
        f32 difficulty_tolerance{0.5f};
        f32 hint_dependency{0.3f};
        f32 time_pressure_sensitivity{0.2f};
        
        LearnerModel(const std::string& id) : learner_id(id) {
            // Initialize default style affinities
            style_affinities[LearningStyle::Visual] = 0.25f;
            style_affinities[LearningStyle::Auditory] = 0.25f;
            style_affinities[LearningStyle::Kinesthetic] = 0.25f;
            style_affinities[LearningStyle::Reading] = 0.25f;
        }
    };
    
    std::unordered_map<std::string, LearnerModel> learner_models_;
    
    // Machine learning components (simplified)
    struct MLModel {
        std::vector<f32> weights;
        f32 learning_rate{0.01f};
        f32 momentum{0.9f};
        
        MLModel(usize input_size) : weights(input_size, 0.0f) {}
    };
    
    MLModel difficulty_predictor_{10}; // Predicts appropriate difficulty
    MLModel performance_predictor_{15}; // Predicts quiz performance
    MLModel engagement_predictor_{8};   // Predicts learner engagement
    
public:
    explicit AdaptiveLearningEngine() = default;
    
    // Learner modeling
    void update_learner_model(const std::string& learner_id, const QuizSession& quiz_session);
    void update_learner_model(const std::string& learner_id, const std::string& tutorial_id, f64 completion_time, f32 success_rate);
    LearnerModel get_learner_model(const std::string& learner_id) const;
    
    // Adaptive recommendations
    AdaptiveDifficulty recommend_difficulty(const std::string& learner_id, const std::string& topic) const;
    LearningStyle recommend_learning_style(const std::string& learner_id) const;
    std::vector<std::string> recommend_next_topics(const std::string& learner_id) const;
    std::vector<QuizQuestion> adapt_quiz_questions(const std::string& learner_id, const std::vector<QuizQuestion>& questions) const;
    
    // Performance prediction
    f32 predict_quiz_success_probability(const std::string& learner_id, const std::string& quiz_topic) const;
    f64 estimate_completion_time(const std::string& learner_id, const std::string& content_id) const;
    f32 predict_engagement_level(const std::string& learner_id, LearningActivityType activity_type) const;
    
    // Spaced repetition optimization
    std::chrono::steady_clock::time_point calculate_optimal_review_time(const std::string& learner_id, const std::string& topic, f32 mastery_level) const;
    std::vector<std::string> get_topics_due_for_review(const std::string& learner_id) const;
    
    // Learning path optimization
    std::vector<std::string> generate_optimal_learning_path(const std::string& learner_id, const std::vector<std::string>& available_topics) const;
    void reorder_learning_activities(const std::string& learner_id, std::vector<std::string>& activities) const;
    
private:
    void train_difficulty_predictor(const std::vector<std::pair<std::vector<f32>, f32>>& training_data);
    void train_performance_predictor(const std::vector<std::pair<std::vector<f32>, f32>>& training_data);
    std::vector<f32> extract_learner_features(const std::string& learner_id) const;
    f32 sigmoid(f32 x) const { return 1.0f / (1.0f + std::exp(-x)); }
};

/**
 * @brief Comprehensive Quiz System with adaptive questioning
 */
class QuizSystem {
private:
    std::unordered_map<std::string, std::vector<QuizQuestion>> quiz_banks_;
    std::unordered_map<std::string, QuizSession> active_sessions_;
    
    // Question generation and adaptation
    std::function<QuizQuestion(const std::string& topic, AdaptiveDifficulty difficulty)> question_generator_;
    
public:
    QuizSystem() = default;
    
    // Quiz management
    void add_quiz_bank(const std::string& bank_name, const std::vector<QuizQuestion>& questions);
    void remove_quiz_bank(const std::string& bank_name);
    std::vector<std::string> get_available_quiz_banks() const;
    
    // Question management
    void add_question_to_bank(const std::string& bank_name, const QuizQuestion& question);
    void remove_question_from_bank(const std::string& bank_name, const std::string& question_id);
    void update_question_statistics(const std::string& bank_name, const std::string& question_id, bool correct, f64 time_taken);
    
    // Quiz session management
    QuizSession create_session(const std::string& session_id, const std::string& bank_name, const std::string& learner_id, usize num_questions = 0);
    void start_session(const std::string& session_id);
    void submit_answer(const std::string& session_id, const std::string& answer);
    void use_hint(const std::string& session_id);
    void finish_session(const std::string& session_id);
    QuizSession get_session(const std::string& session_id) const;
    
    // Adaptive questioning
    void set_question_generator(std::function<QuizQuestion(const std::string&, AdaptiveDifficulty)> generator);
    std::vector<QuizQuestion> generate_adaptive_quiz(const std::string& learner_id, const std::string& topic, usize num_questions, AdaptiveDifficulty difficulty);
    QuizQuestion generate_follow_up_question(const QuizQuestion& previous_question, bool was_correct);
    
    // Analytics and reporting
    struct QuizAnalytics {
        f32 average_score{0.0f};
        f64 average_completion_time{0.0};
        std::unordered_map<std::string, f32> topic_performance;
        std::unordered_map<QuestionType, f32> question_type_performance;
        std::vector<std::string> commonly_missed_questions;
        f32 improvement_trend{0.0f};
    };
    
    QuizAnalytics generate_analytics(const std::string& learner_id) const;
    std::vector<QuizSession> get_learner_quiz_history(const std::string& learner_id) const;
    
private:
    void score_quiz_session(QuizSession& session);
    void generate_feedback(QuizSession& session);
    std::vector<QuizQuestion> select_questions_for_session(const std::string& bank_name, const std::string& learner_id, usize count);
};

} // namespace ecscope::ui