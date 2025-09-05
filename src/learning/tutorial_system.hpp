#pragma once

/**
 * @file tutorial_system.hpp
 * @brief Interactive Tutorial System for ECScope ECS Educational Platform
 * 
 * This system provides comprehensive educational tutorials with real-time guidance,
 * interactive code examples, and adaptive learning features specifically designed
 * for teaching ECS (Entity-Component-System) concepts.
 * 
 * Features:
 * - Step-by-step guided tutorials with visual cues
 * - Interactive code examples with real-time execution
 * - Progressive learning paths with difficulty adaptation
 * - Context-aware hints and explanations
 * - Integration with visual debugger and performance tools
 * - Progress tracking and achievement system
 * 
 * Educational Design:
 * - Scaffolded learning approach
 * - Just-in-time information delivery
 * - Immediate feedback and validation
 * - Multiple learning modalities (visual, kinesthetic, reading)
 * - Personalized learning paths
 * 
 * @author ECScope Educational Framework
 * @date 2024
 */

#include "core/types.hpp"
#include "core/result.hpp"
#include "ecs/registry.hpp"
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <memory>
#include <chrono>

namespace ecscope::learning {

// Forward declarations
class TutorialStep;
class Tutorial;
class InteractiveLearningSystem;

/**
 * @brief Types of tutorial interactions
 */
enum class InteractionType : u8 {
    ReadOnly,           // Just display information
    ClickTarget,        // Click on a specific UI element
    CodeEntry,          // Enter code in editor
    ValueAdjust,        // Adjust sliders or inputs
    EntityManipulation, // Create/modify entities
    ComponentEdit,      // Add/modify components
    SystemExecution,    // Run systems and observe
    PerformanceAnalysis, // Analyze performance metrics
    QuizQuestion        // Answer quiz questions
};

/**
 * @brief Learning difficulty levels
 */
enum class DifficultyLevel : u8 {
    Beginner,       // Basic concepts, lots of guidance
    Intermediate,   // Some independent work
    Advanced,       // Complex scenarios, minimal guidance
    Expert          // Open-ended challenges
};

/**
 * @brief Tutorial categories
 */
enum class TutorialCategory : u8 {
    BasicConcepts,      // ECS fundamentals
    EntityManagement,   // Creating and managing entities
    ComponentSystems,   // Components and their usage
    SystemDesign,       // Building and optimizing systems
    MemoryOptimization, // Memory layout and performance
    AdvancedPatterns,   // Complex ECS patterns
    RealWorldExamples,  // Practical applications
    PerformanceAnalysis // Profiling and optimization
};

/**
 * @brief Validation result for tutorial steps
 */
struct ValidationResult {
    bool is_valid{false};
    std::string feedback;
    std::vector<std::string> hints;
    f32 completion_score{0.0f}; // 0.0 to 1.0
    
    ValidationResult() = default;
    ValidationResult(bool valid, const std::string& msg) 
        : is_valid(valid), feedback(msg) {}
        
    static ValidationResult success(const std::string& msg = "Correct!") {
        ValidationResult result(true, msg);
        result.completion_score = 1.0f;
        return result;
    }
    
    static ValidationResult failure(const std::string& msg, const std::vector<std::string>& hint_list = {}) {
        ValidationResult result(false, msg);
        result.hints = hint_list;
        return result;
    }
    
    static ValidationResult partial(const std::string& msg, f32 score) {
        ValidationResult result(false, msg);
        result.completion_score = score;
        return result;
    }
};

/**
 * @brief Learning progress tracking
 */
struct LearningProgress {
    std::string learner_id;
    std::unordered_map<std::string, f32> tutorial_completion; // tutorial_id -> completion %
    std::unordered_map<std::string, u32> step_attempts;       // step_id -> attempt count
    std::unordered_map<std::string, f64> time_spent;          // tutorial_id -> total time in seconds
    std::vector<std::string> achievements_unlocked;
    DifficultyLevel current_level{DifficultyLevel::Beginner};
    f64 total_learning_time{0.0};
    u32 total_tutorials_completed{0};
    
    // Analytics
    std::vector<std::pair<std::string, f64>> learning_velocity; // concept -> learning rate
    std::unordered_map<std::string, u32> help_requests;        // topic -> help count
    std::vector<std::string> struggling_concepts;              // concepts needing reinforcement
};

/**
 * @brief Interactive code example with execution
 */
struct CodeExample {
    std::string code_template;      // Template with placeholders
    std::string expected_output;    // Expected result
    std::string current_code;       // User's current code
    std::vector<std::string> hints; // Progressive hints
    std::function<ValidationResult(const std::string&)> validator;
    bool supports_execution{true};
    bool show_expected_output{false};
    u32 hint_level{0}; // Current hint level shown
    
    CodeExample() = default;
    CodeExample(const std::string& tmpl, const std::string& expected = "")
        : code_template(tmpl), expected_output(expected), current_code(tmpl) {}
};

/**
 * @brief Visual cue for highlighting UI elements
 */
struct VisualCue {
    enum class CueType : u8 {
        Highlight,      // Highlight element with border
        Pulse,          // Pulsing animation
        Arrow,          // Arrow pointing to element
        Tooltip,        // Floating tooltip
        Overlay,        // Semi-transparent overlay
        Spotlight       // Dim everything except target
    };
    
    CueType type;
    std::string target_element_id;  // UI element to highlight
    std::string message;            // Associated message
    f32 duration{0.0f};            // 0 = permanent until dismissed
    bool auto_dismiss{false};
    f32 intensity{1.0f};           // Animation intensity
    
    VisualCue(CueType t, const std::string& target, const std::string& msg)
        : type(t), target_element_id(target), message(msg) {}
};

/**
 * @brief Individual tutorial step
 */
class TutorialStep {
private:
    std::string id_;
    std::string title_;
    std::string description_;
    std::string detailed_explanation_;
    
    InteractionType interaction_type_;
    std::vector<VisualCue> visual_cues_;
    std::unique_ptr<CodeExample> code_example_;
    
    // Validation
    std::function<ValidationResult()> validator_;
    bool is_completed_{false};
    ValidationResult last_validation_;
    
    // Adaptive features
    u32 attempt_count_{0};
    f64 time_spent_{0.0};
    std::chrono::steady_clock::time_point start_time_;
    bool has_started_{false};
    
    // Help system
    std::vector<std::string> contextual_hints_;
    u32 current_hint_level_{0};
    std::string help_topic_;
    
public:
    TutorialStep(const std::string& id, const std::string& title, const std::string& description);
    ~TutorialStep() = default;
    
    // Configuration
    TutorialStep& set_interaction_type(InteractionType type);
    TutorialStep& set_detailed_explanation(const std::string& explanation);
    TutorialStep& add_visual_cue(const VisualCue& cue);
    TutorialStep& set_code_example(std::unique_ptr<CodeExample> example);
    TutorialStep& set_validator(std::function<ValidationResult()> validator);
    TutorialStep& add_hint(const std::string& hint);
    TutorialStep& set_help_topic(const std::string& topic);
    
    // Execution
    void start();
    void reset();
    ValidationResult validate();
    void complete();
    
    // State queries
    const std::string& id() const { return id_; }
    const std::string& title() const { return title_; }
    const std::string& description() const { return description_; }
    const std::string& detailed_explanation() const { return detailed_explanation_; }
    InteractionType interaction_type() const { return interaction_type_; }
    const std::vector<VisualCue>& visual_cues() const { return visual_cues_; }
    CodeExample* code_example() const { return code_example_.get(); }
    bool is_completed() const { return is_completed_; }
    bool has_started() const { return has_started_; }
    
    // Progress tracking
    u32 attempt_count() const { return attempt_count_; }
    f64 time_spent() const;
    f32 completion_score() const { return last_validation_.completion_score; }
    
    // Help system
    std::string get_next_hint();
    void request_help();
    const std::string& help_topic() const { return help_topic_; }
    
    // Adaptive behavior
    bool needs_additional_help() const;
    DifficultyLevel suggested_difficulty() const;
};

/**
 * @brief Complete tutorial with multiple steps
 */
class Tutorial {
private:
    std::string id_;
    std::string title_;
    std::string description_;
    TutorialCategory category_;
    DifficultyLevel difficulty_;
    
    std::vector<std::unique_ptr<TutorialStep>> steps_;
    usize current_step_index_{0};
    
    // Prerequisites and recommendations
    std::vector<std::string> prerequisite_tutorials_;
    std::vector<std::string> recommended_next_;
    
    // Progress tracking
    bool is_started_{false};
    bool is_completed_{false};
    f64 total_time_spent_{0.0};
    std::chrono::steady_clock::time_point start_time_;
    
    // Learning objectives
    std::vector<std::string> learning_objectives_;
    std::unordered_map<std::string, bool> objectives_met_;
    
    // Resources
    std::vector<std::string> reference_links_;
    std::vector<std::string> additional_reading_;
    
public:
    Tutorial(const std::string& id, const std::string& title, TutorialCategory category, DifficultyLevel difficulty);
    ~Tutorial() = default;
    
    // Configuration
    Tutorial& set_description(const std::string& description);
    Tutorial& add_step(std::unique_ptr<TutorialStep> step);
    Tutorial& add_prerequisite(const std::string& tutorial_id);
    Tutorial& add_recommended_next(const std::string& tutorial_id);
    Tutorial& add_learning_objective(const std::string& objective);
    Tutorial& add_reference_link(const std::string& link);
    
    // Execution
    void start();
    void reset();
    bool advance_step();
    bool previous_step();
    void complete();
    
    // State queries
    const std::string& id() const { return id_; }
    const std::string& title() const { return title_; }
    const std::string& description() const { return description_; }
    TutorialCategory category() const { return category_; }
    DifficultyLevel difficulty() const { return difficulty_; }
    
    usize total_steps() const { return steps_.size(); }
    usize current_step_index() const { return current_step_index_; }
    TutorialStep* current_step() const;
    TutorialStep* get_step(usize index) const;
    
    bool is_started() const { return is_started_; }
    bool is_completed() const { return is_completed_; }
    f32 completion_percentage() const;
    f64 total_time_spent() const;
    
    // Prerequisites and flow
    const std::vector<std::string>& prerequisites() const { return prerequisite_tutorials_; }
    const std::vector<std::string>& recommended_next() const { return recommended_next_; }
    
    // Learning objectives
    const std::vector<std::string>& learning_objectives() const { return learning_objectives_; }
    bool is_objective_met(const std::string& objective) const;
    void mark_objective_met(const std::string& objective);
    f32 objectives_completion_rate() const;
    
    // Resources
    const std::vector<std::string>& reference_links() const { return reference_links_; }
    const std::vector<std::string>& additional_reading() const { return additional_reading_; }
    
    // Adaptive features
    DifficultyLevel calculate_effective_difficulty(const LearningProgress& progress) const;
    std::vector<std::string> get_struggling_concepts() const;
};

/**
 * @brief Tutorial manager and learning path coordinator
 */
class TutorialManager {
private:
    std::unordered_map<std::string, std::unique_ptr<Tutorial>> tutorials_;
    std::unordered_map<TutorialCategory, std::vector<std::string>> category_index_;
    std::unordered_map<DifficultyLevel, std::vector<std::string>> difficulty_index_;
    
    // Current state
    Tutorial* current_tutorial_{nullptr};
    std::vector<std::string> learning_path_;
    usize current_path_index_{0};
    
    // Adaptive system
    std::unordered_map<std::string, LearningProgress> learner_progress_;
    std::string current_learner_id_;
    
    // Content generation
    std::function<std::unique_ptr<CodeExample>(const std::string&)> code_generator_;
    
public:
    TutorialManager() = default;
    ~TutorialManager() = default;
    
    // Tutorial management
    void register_tutorial(std::unique_ptr<Tutorial> tutorial);
    Tutorial* get_tutorial(const std::string& id) const;
    std::vector<Tutorial*> get_tutorials_by_category(TutorialCategory category) const;
    std::vector<Tutorial*> get_tutorials_by_difficulty(DifficultyLevel difficulty) const;
    
    // Learning path management
    void create_learning_path(const std::string& learner_id, 
                             const std::vector<TutorialCategory>& preferred_categories = {},
                             DifficultyLevel starting_difficulty = DifficultyLevel::Beginner);
    void set_custom_learning_path(const std::vector<std::string>& tutorial_ids);
    std::vector<std::string> generate_adaptive_path(const std::string& learner_id);
    
    // Current tutorial control
    bool start_tutorial(const std::string& tutorial_id, const std::string& learner_id);
    bool advance_current_tutorial();
    bool previous_step_current_tutorial();
    void reset_current_tutorial();
    void complete_current_tutorial();
    
    // Progress tracking
    void set_current_learner(const std::string& learner_id);
    LearningProgress& get_learner_progress(const std::string& learner_id);
    const LearningProgress& get_learner_progress(const std::string& learner_id) const;
    void save_progress(const std::string& filename) const;
    void load_progress(const std::string& filename);
    
    // Recommendations
    std::vector<std::string> get_recommended_tutorials(const std::string& learner_id) const;
    std::vector<std::string> get_review_tutorials(const std::string& learner_id) const;
    DifficultyLevel recommend_difficulty(const std::string& learner_id) const;
    
    // Analytics
    struct LearningAnalytics {
        f64 average_completion_time{0.0};
        std::unordered_map<std::string, f32> concept_mastery; // concept -> mastery level
        std::vector<std::string> strengths;
        std::vector<std::string> areas_for_improvement;
        f32 overall_progress{0.0f};
        u32 total_attempts{0};
        u32 successful_completions{0};
    };
    
    LearningAnalytics generate_analytics(const std::string& learner_id) const;
    
    // Content customization
    void set_code_generator(std::function<std::unique_ptr<CodeExample>(const std::string&)> generator);
    void customize_content_for_learner(const std::string& learner_id);
    
    // State queries
    Tutorial* current_tutorial() const { return current_tutorial_; }
    const std::vector<std::string>& current_learning_path() const { return learning_path_; }
    usize total_tutorials() const { return tutorials_.size(); }
    
    // Utility
    std::vector<std::string> search_tutorials(const std::string& query) const;
    std::vector<Tutorial*> get_all_tutorials() const;
    void clear_all_progress();
};

/**
 * @brief Factory for creating common tutorial patterns
 */
class TutorialFactory {
public:
    // Basic ECS tutorials
    static std::unique_ptr<Tutorial> create_basic_ecs_intro();
    static std::unique_ptr<Tutorial> create_entity_management_tutorial();
    static std::unique_ptr<Tutorial> create_component_systems_tutorial();
    static std::unique_ptr<Tutorial> create_system_design_tutorial();
    
    // Advanced tutorials
    static std::unique_ptr<Tutorial> create_memory_optimization_tutorial();
    static std::unique_ptr<Tutorial> create_performance_analysis_tutorial();
    static std::unique_ptr<Tutorial> create_sparse_set_tutorial();
    static std::unique_ptr<Tutorial> create_archetype_tutorial();
    
    // Interactive examples
    static std::unique_ptr<Tutorial> create_physics_simulation_tutorial();
    static std::unique_ptr<Tutorial> create_rendering_pipeline_tutorial();
    static std::unique_ptr<Tutorial> create_job_system_tutorial();
    
    // Custom tutorial builders
    static std::unique_ptr<Tutorial> create_custom_tutorial(
        const std::string& id,
        const std::string& title,
        TutorialCategory category,
        DifficultyLevel difficulty,
        const std::vector<std::string>& step_descriptions
    );
    
    // Code example generators
    static std::unique_ptr<CodeExample> create_entity_creation_example();
    static std::unique_ptr<CodeExample> create_component_addition_example();
    static std::unique_ptr<CodeExample> create_system_iteration_example();
    static std::unique_ptr<CodeExample> create_performance_measurement_example();
    
private:
    static std::unique_ptr<TutorialStep> create_step(
        const std::string& id,
        const std::string& title,
        const std::string& description,
        InteractionType type
    );
};

} // namespace ecscope::learning