#pragma once

/**
 * @file interactive_learning_integration.hpp
 * @brief Integration layer for the Interactive Learning System with ECScope UI
 * 
 * This file provides the integration layer that connects the comprehensive
 * Interactive Learning System with ECScope's existing UI overlay and core systems.
 * It manages the lifecycle and coordination of all learning-related panels.
 * 
 * Integration Features:
 * - Seamless integration with existing ECScope UI panels
 * - Cross-panel communication and state synchronization
 * - Unified learning session management
 * - Coordinated tutorial and debugging workflows
 * - Shared progress tracking across all learning activities
 * 
 * @author ECScope Educational Framework
 * @date 2024
 */

#include "tutorial_system.hpp"
#include "../ui/panels/panel_interactive_tutorial.hpp"
#include "../ui/panels/panel_visual_debugger.hpp"
#include "../ui/panels/panel_performance_comparison.hpp"
#include "../ui/panels/panel_educational_features.hpp"
#include "../ui/overlay.hpp"
#include "ecs/registry.hpp"
#include "performance/performance_lab.hpp"
#include "core/types.hpp"
#include <memory>
#include <string>
#include <functional>

namespace ecscope::learning {

/**
 * @brief Interactive Learning System Integration Manager
 * 
 * This class orchestrates all learning-related panels and systems,
 * providing a unified interface for educational functionality within ECScope.
 */
class InteractiveLearningIntegration {
private:
    // Core systems
    std::shared_ptr<TutorialManager> tutorial_manager_;
    std::shared_ptr<ecs::Registry> ecs_registry_;
    std::shared_ptr<performance::PerformanceLab> performance_lab_;
    ui::UIOverlay* ui_overlay_{nullptr};
    
    // Learning panels
    ui::InteractiveTutorialPanel* tutorial_panel_{nullptr};
    ui::VisualDebuggerPanel* debugger_panel_{nullptr};
    ui::PerformanceComparisonPanel* performance_panel_{nullptr};
    ui::EducationalFeaturesPanel* education_panel_{nullptr};
    
    // Integration state
    std::string current_learner_id_{"default_learner"};
    bool learning_mode_active_{false};
    bool cross_panel_sync_enabled_{true};
    
    // Learning session coordination
    struct LearningSession {
        std::string session_id;
        std::string current_tutorial_id;
        std::string current_activity_type; // "tutorial", "quiz", "debug", "benchmark"
        std::chrono::steady_clock::time_point start_time;
        f64 total_duration_seconds{0.0};
        
        // Cross-panel state
        bool debugger_following_tutorial{false};
        bool performance_tracking_enabled{false};
        std::vector<std::string> synchronized_panels;
        
        LearningSession() = default;
        LearningSession(const std::string& id) : session_id(id) {
            start_time = std::chrono::steady_clock::now();
        }
    };
    
    std::unique_ptr<LearningSession> current_session_;
    
    // Event handlers and callbacks
    std::function<void(const std::string&)> on_tutorial_started_callback_;
    std::function<void(const std::string&)> on_tutorial_completed_callback_;
    std::function<void(const std::string&, f32)> on_quiz_completed_callback_;
    std::function<void(const std::string&)> on_achievement_unlocked_callback_;
    
public:
    InteractiveLearningIntegration() = default;
    ~InteractiveLearningIntegration() = default;
    
    // Initialization and setup
    void initialize(ui::UIOverlay& ui_overlay, std::shared_ptr<ecs::Registry> registry);
    void shutdown();
    
    // System integration
    void set_performance_lab(std::shared_ptr<performance::PerformanceLab> lab);
    void set_tutorial_manager(std::shared_ptr<TutorialManager> manager);
    
    // Panel management
    void register_all_learning_panels();
    void show_learning_panels(bool show = true);
    void hide_learning_panels();
    
    // Learning session management
    void start_learning_session(const std::string& activity_type = "general");
    void end_learning_session();
    bool has_active_session() const { return current_session_ != nullptr; }
    const LearningSession* get_current_session() const { return current_session_.get(); }
    
    // Cross-panel coordination
    void synchronize_tutorial_with_debugger(const std::string& tutorial_id);
    void link_performance_analysis_with_tutorial();
    void coordinate_quiz_with_debugging_practice();
    void enable_cross_panel_communication(bool enabled) { cross_panel_sync_enabled_ = enabled; }
    
    // Learning workflow orchestration
    void start_guided_ecs_introduction();
    void start_performance_optimization_workflow();
    void start_debugging_mastery_path();
    void start_advanced_patterns_exploration();
    
    // Learner management
    void set_current_learner(const std::string& learner_id);
    std::string get_current_learner() const { return current_learner_id_; }
    void switch_learner_profile(const std::string& learner_id);
    
    // Tutorial integration
    void navigate_to_tutorial(const std::string& tutorial_id);
    void create_tutorial_from_debugging_session();
    void export_tutorial_progress_report();
    
    // Quiz and assessment integration
    void start_adaptive_assessment();
    void create_quiz_from_tutorial_content(const std::string& tutorial_id);
    void schedule_spaced_repetition_quiz(const std::string& topic);
    
    // Performance learning integration
    void start_performance_comparison_tutorial();
    void create_benchmark_challenge(const std::string& challenge_id);
    void analyze_code_performance_with_explanation();
    
    // Visual debugging integration
    void start_visual_debugging_tutorial();
    void create_debugging_scenario(const std::string& scenario_id);
    void record_debugging_session_for_replay();
    
    // Educational analytics and reporting
    struct LearningAnalytics {
        std::string learner_id;
        f64 total_learning_time_hours{0.0};
        u32 tutorials_completed{0};
        u32 quizzes_passed{0};
        f32 average_quiz_score{0.0f};
        u32 debugging_sessions{0};
        u32 performance_analyses{0};
        f32 overall_progress{0.0f};
        std::vector<std::string> mastered_topics;
        std::vector<std::string> struggling_areas;
        std::string current_skill_level;
        
        // Cross-panel insights
        f32 tutorial_debugger_correlation{0.0f}; // How well tutorial knowledge transfers to debugging
        f32 performance_understanding{0.0f};     // Understanding of performance concepts
        f32 practical_application_score{0.0f};   // Ability to apply knowledge practically
    };
    
    LearningAnalytics generate_comprehensive_analytics() const;
    void export_learning_analytics(const std::string& filename) const;
    
    // Event callback registration
    void set_tutorial_started_callback(std::function<void(const std::string&)> callback);
    void set_tutorial_completed_callback(std::function<void(const std::string&)> callback);
    void set_quiz_completed_callback(std::function<void(const std::string&, f32)> callback);
    void set_achievement_unlocked_callback(std::function<void(const std::string&)> callback);
    
    // Integration utilities
    void highlight_ui_element_across_panels(const std::string& element_id);
    void show_contextual_help(const std::string& topic);
    void trigger_cross_panel_notification(const std::string& message);
    
    // Advanced learning features
    void enable_ai_tutor_mode(bool enabled);
    void start_collaborative_learning_session();
    void create_custom_learning_path(const std::vector<std::string>& topics);
    
    // State queries
    bool is_learning_mode_active() const { return learning_mode_active_; }
    bool are_panels_synchronized() const { return cross_panel_sync_enabled_; }
    std::vector<std::string> get_active_learning_panels() const;
    
private:
    // Panel coordination methods
    void sync_tutorial_progress_to_debugger();
    void sync_debugger_insights_to_tutorial();
    void sync_performance_data_to_education_panel();
    void update_cross_panel_state();
    
    // Event handling
    void on_tutorial_step_completed(const std::string& tutorial_id, const std::string& step_id);
    void on_debugger_breakpoint_hit(const std::string& operation_type);
    void on_performance_benchmark_completed(const std::string& benchmark_name, f64 result);
    void on_quiz_question_answered(const std::string& question_id, bool correct);
    
    // Learning session tracking
    void update_session_progress(f64 delta_time);
    void record_cross_panel_interaction(const std::string& source_panel, const std::string& target_panel, const std::string& action);
    
    // Workflow management
    void execute_learning_workflow(const std::string& workflow_id);
    void advance_workflow_step();
    bool validate_workflow_prerequisites(const std::string& workflow_id);
};

/**
 * @brief Factory for creating integrated learning experiences
 */
class IntegratedLearningExperienceFactory {
public:
    // Pre-built learning experiences
    static std::unique_ptr<Tutorial> create_ecs_basics_with_debugging();
    static std::unique_ptr<Tutorial> create_performance_optimization_masterclass();
    static std::unique_ptr<Tutorial> create_memory_layout_exploration();
    static std::unique_ptr<Tutorial> create_system_design_workshop();
    
    // Interactive scenario builders
    static std::unique_ptr<Tutorial> create_debugging_scenario_tutorial(const std::string& scenario_type);
    static std::unique_ptr<Tutorial> create_performance_challenge_tutorial(const std::string& challenge_type);
    static std::unique_ptr<Tutorial> create_adaptive_quiz_tutorial(const std::string& topic, learning::DifficultyLevel difficulty);
    
    // Cross-panel experience creators
    static void create_tutorial_with_live_debugging(InteractiveLearningIntegration& integration, const std::string& topic);
    static void create_performance_analysis_with_explanation(InteractiveLearningIntegration& integration, const std::string& benchmark_type);
    static void create_comprehensive_ecs_journey(InteractiveLearningIntegration& integration);
    
private:
    static std::unique_ptr<TutorialStep> create_debugger_integration_step(const std::string& step_id, const std::string& debug_operation);
    static std::unique_ptr<TutorialStep> create_performance_measurement_step(const std::string& step_id, const std::string& benchmark_type);
    static std::unique_ptr<TutorialStep> create_quiz_validation_step(const std::string& step_id, const std::vector<QuizQuestion>& questions);
};

/**
 * @brief Global access point for the learning integration system
 */
InteractiveLearningIntegration& get_learning_integration();
void set_learning_integration(std::unique_ptr<InteractiveLearningIntegration> integration);

/**
 * @brief Convenience functions for common learning operations
 */
namespace learning_integration {
    
    // Quick start functions
    void quick_start_ecs_tutorial();
    void quick_start_performance_analysis();
    void quick_start_debugging_practice();
    void quick_start_adaptive_quiz(const std::string& topic);
    
    // Learning state management
    void save_all_learning_progress();
    void load_learning_progress_for_user(const std::string& learner_id);
    void backup_learning_data(const std::string& backup_path);
    void restore_learning_data(const std::string& backup_path);
    
    // Cross-panel utilities
    void highlight_ecs_concept(const std::string& concept_name);
    void demonstrate_performance_impact(const std::string& operation);
    void show_debugging_technique(const std::string& technique_name);
    void explain_memory_layout_difference(const std::string& layout_type);
    
    // Advanced integration features
    void create_learning_snapshot(); // Save current state across all panels
    void replay_learning_session(const std::string& session_id);
    void generate_personalized_study_plan(const std::string& learner_id);
    void export_comprehensive_learning_report(const std::string& output_path);
}

} // namespace ecscope::learning