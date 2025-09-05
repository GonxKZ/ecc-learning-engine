#include "interactive_learning_integration.hpp"
#include "core/log.hpp"
#include <algorithm>
#include <sstream>
#include <fstream>

namespace ecscope::learning {

// Global integration instance
static std::unique_ptr<InteractiveLearningIntegration> g_learning_integration;

// InteractiveLearningIntegration Implementation

void InteractiveLearningIntegration::initialize(ui::UIOverlay& ui_overlay, std::shared_ptr<ecs::Registry> registry) {
    ui_overlay_ = &ui_overlay;
    ecs_registry_ = registry;
    
    // Initialize tutorial manager if not already set
    if (!tutorial_manager_) {
        tutorial_manager_ = std::make_shared<TutorialManager>();
        
        // Create default tutorials using factory
        tutorial_manager_->register_tutorial(
            IntegratedLearningExperienceFactory::create_ecs_basics_with_debugging());
        tutorial_manager_->register_tutorial(
            IntegratedLearningExperienceFactory::create_performance_optimization_masterclass());
        tutorial_manager_->register_tutorial(
            IntegratedLearningExperienceFactory::create_memory_layout_exploration());
    }
    
    // Register all learning panels with the UI overlay
    register_all_learning_panels();
    
    LOG_INFO("Interactive Learning Integration initialized");
}

void InteractiveLearningIntegration::shutdown() {
    if (current_session_) {
        end_learning_session();
    }
    
    // Cleanup would go here
    ui_overlay_ = nullptr;
    tutorial_panel_ = nullptr;
    debugger_panel_ = nullptr;
    performance_panel_ = nullptr;
    education_panel_ = nullptr;
    
    LOG_INFO("Interactive Learning Integration shutdown");
}

void InteractiveLearningIntegration::set_performance_lab(std::shared_ptr<performance::PerformanceLab> lab) {
    performance_lab_ = lab;
    
    // Update performance panel if it exists
    if (performance_panel_) {
        performance_panel_->set_performance_lab(lab);
    }
}

void InteractiveLearningIntegration::set_tutorial_manager(std::shared_ptr<TutorialManager> manager) {
    tutorial_manager_ = manager;
    
    // Update tutorial panel if it exists
    if (tutorial_panel_) {
        tutorial_panel_->set_tutorial_manager(manager);
    }
    
    // Update education panel if it exists
    if (education_panel_) {
        // Education panel would be updated here
    }
}

void InteractiveLearningIntegration::register_all_learning_panels() {
    if (!ui_overlay_) {
        LOG_ERROR("UI overlay not available for panel registration");
        return;
    }
    
    // Register Interactive Tutorial Panel
    if (!tutorial_panel_ && tutorial_manager_) {
        tutorial_panel_ = ui_overlay_->add_panel<ui::InteractiveTutorialPanel>(tutorial_manager_);
        tutorial_panel_->set_learner_id(current_learner_id_);
        
        // Set up tutorial callbacks
        // In a real implementation, these would be properly connected
        LOG_INFO("Registered Interactive Tutorial Panel");
    }
    
    // Register Visual Debugger Panel
    if (!debugger_panel_ && ecs_registry_) {
        debugger_panel_ = ui_overlay_->add_panel<ui::VisualDebuggerPanel>(ecs_registry_);
        
        // Set up debugger callbacks for educational integration
        // In a real implementation, these would be properly connected
        LOG_INFO("Registered Visual Debugger Panel");
    }
    
    // Register Performance Comparison Panel
    if (!performance_panel_ && performance_lab_) {
        performance_panel_ = ui_overlay_->add_panel<ui::PerformanceComparisonPanel>(performance_lab_);
        
        // Set up performance callbacks
        // In a real implementation, these would be properly connected
        LOG_INFO("Registered Performance Comparison Panel");
    }
    
    // Register Educational Features Panel
    if (!education_panel_ && tutorial_manager_) {
        education_panel_ = ui_overlay_->add_panel<ui::EducationalFeaturesPanel>(tutorial_manager_);
        education_panel_->set_current_learner(current_learner_id_);
        
        // Integrate with other panels
        if (tutorial_panel_) {
            education_panel_->integrate_tutorial_panel(tutorial_panel_);
        }
        if (debugger_panel_) {
            education_panel_->integrate_debugger_panel(debugger_panel_);
        }
        if (performance_panel_) {
            education_panel_->integrate_performance_panel(performance_panel_);
        }
        
        LOG_INFO("Registered Educational Features Panel");
    }
}

void InteractiveLearningIntegration::show_learning_panels(bool show) {
    if (tutorial_panel_) tutorial_panel_->set_visible(show);
    if (debugger_panel_) debugger_panel_->set_visible(show);
    if (performance_panel_) performance_panel_->set_visible(show);
    if (education_panel_) education_panel_->set_visible(show);
    
    learning_mode_active_ = show;
    
    LOG_INFO(show ? "Activated learning mode - all panels visible" : "Deactivated learning mode - panels hidden");
}

void InteractiveLearningIntegration::hide_learning_panels() {
    show_learning_panels(false);
}

// Learning session management

void InteractiveLearningIntegration::start_learning_session(const std::string& activity_type) {
    if (current_session_) {
        end_learning_session();
    }
    
    std::string session_id = "session_" + std::to_string(std::time(nullptr));
    current_session_ = std::make_unique<LearningSession>(session_id);
    current_session_->current_activity_type = activity_type;
    
    // Show relevant panels based on activity type
    if (activity_type == "tutorial" || activity_type == "general") {
        show_learning_panels(true);
    } else if (activity_type == "quiz") {
        if (education_panel_) education_panel_->set_visible(true);
    } else if (activity_type == "debug") {
        if (debugger_panel_) debugger_panel_->set_visible(true);
        if (tutorial_panel_) tutorial_panel_->set_visible(true);
    } else if (activity_type == "benchmark") {
        if (performance_panel_) performance_panel_->set_visible(true);
        if (education_panel_) education_panel_->set_visible(true);
    }
    
    // Enable cross-panel synchronization for integrated learning
    if (cross_panel_sync_enabled_) {
        enable_cross_panel_communication(true);
    }
    
    LOG_INFO("Started learning session: " + session_id + " (activity: " + activity_type + ")");
}

void InteractiveLearningIntegration::end_learning_session() {
    if (!current_session_) return;
    
    // Calculate session duration
    auto now = std::chrono::steady_clock::now();
    current_session_->total_duration_seconds = 
        std::chrono::duration<f64>(now - current_session_->start_time).count();
    
    // Record session data to education panel
    if (education_panel_) {
        education_panel_->record_learning_activity(
            learning::ui::LearningActivityType::Tutorial, // Simplified mapping
            current_session_->session_id,
            current_session_->total_duration_seconds / 60.0, // Convert to minutes
            1.0f // Assume successful completion
        );
    }
    
    std::string session_summary = "Learning session completed - Duration: " + 
                                 std::to_string(current_session_->total_duration_seconds / 60.0) + " minutes";
    
    LOG_INFO(session_summary);
    current_session_.reset();
}

// Cross-panel coordination

void InteractiveLearningIntegration::synchronize_tutorial_with_debugger(const std::string& tutorial_id) {
    if (!tutorial_panel_ || !debugger_panel_ || !cross_panel_sync_enabled_) return;
    
    // Start tutorial in tutorial panel
    tutorial_panel_->start_tutorial(tutorial_id);
    
    // Configure debugger to follow tutorial progress
    debugger_panel_->start_debugging(); // Start in educational mode
    
    if (current_session_) {
        current_session_->current_tutorial_id = tutorial_id;
        current_session_->debugger_following_tutorial = true;
        current_session_->synchronized_panels.push_back("tutorial");
        current_session_->synchronized_panels.push_back("debugger");
    }
    
    LOG_INFO("Synchronized tutorial '" + tutorial_id + "' with visual debugger");
}

void InteractiveLearningIntegration::link_performance_analysis_with_tutorial() {
    if (!performance_panel_ || !tutorial_panel_ || !cross_panel_sync_enabled_) return;
    
    // Enable performance tracking
    if (current_session_) {
        current_session_->performance_tracking_enabled = true;
        current_session_->synchronized_panels.push_back("performance");
    }
    
    // Configure performance panel for educational mode
    performance_panel_->start_guided_learning();
    
    LOG_INFO("Linked performance analysis with tutorial system");
}

void InteractiveLearningIntegration::coordinate_quiz_with_debugging_practice() {
    if (!education_panel_ || !debugger_panel_ || !cross_panel_sync_enabled_) return;
    
    // Set education panel to quiz mode
    education_panel_->set_education_mode(ui::EducationalFeaturesPanel::EducationMode::QuizCenter);
    
    // Configure debugger for practice scenarios
    debugger_panel_->start_debugging();
    
    LOG_INFO("Coordinated quiz system with debugging practice");
}

// Learning workflow orchestration

void InteractiveLearningIntegration::start_guided_ecs_introduction() {
    start_learning_session("tutorial");
    
    if (tutorial_manager_) {
        // Create comprehensive ECS introduction workflow
        IntegratedLearningExperienceFactory::create_comprehensive_ecs_journey(*this);
    }
    
    // Navigate to first tutorial
    navigate_to_tutorial("ecs_basics_introduction");
    
    LOG_INFO("Started guided ECS introduction workflow");
}

void InteractiveLearningIntegration::start_performance_optimization_workflow() {
    start_learning_session("benchmark");
    
    // Show relevant panels
    if (performance_panel_) performance_panel_->set_visible(true);
    if (tutorial_panel_) tutorial_panel_->set_visible(true);
    if (education_panel_) education_panel_->set_visible(true);
    
    // Start performance tutorial
    navigate_to_tutorial("performance_optimization_masterclass");
    
    // Enable performance tracking
    link_performance_analysis_with_tutorial();
    
    LOG_INFO("Started performance optimization workflow");
}

void InteractiveLearningIntegration::start_debugging_mastery_path() {
    start_learning_session("debug");
    
    // Show debugging-focused panels
    if (debugger_panel_) debugger_panel_->set_visible(true);
    if (tutorial_panel_) tutorial_panel_->set_visible(true);
    
    // Start debugging tutorial with live practice
    synchronize_tutorial_with_debugger("visual_debugging_mastery");
    
    LOG_INFO("Started debugging mastery learning path");
}

void InteractiveLearningIntegration::start_advanced_patterns_exploration() {
    start_learning_session("tutorial");
    
    show_learning_panels(true);
    
    // Navigate to advanced patterns tutorial
    navigate_to_tutorial("advanced_ecs_patterns");
    
    LOG_INFO("Started advanced patterns exploration");
}

// Learner management

void InteractiveLearningIntegration::set_current_learner(const std::string& learner_id) {
    if (current_learner_id_ == learner_id) return;
    
    current_learner_id_ = learner_id;
    
    // Update all panels with new learner
    if (tutorial_panel_) tutorial_panel_->set_learner_id(learner_id);
    if (education_panel_) education_panel_->set_current_learner(learner_id);
    
    // Load progress for new learner
    learning_integration::load_learning_progress_for_user(learner_id);
    
    LOG_INFO("Switched to learner: " + learner_id);
}

void InteractiveLearningIntegration::switch_learner_profile(const std::string& learner_id) {
    // Save current progress before switching
    learning_integration::save_all_learning_progress();
    
    set_current_learner(learner_id);
}

// Tutorial integration

void InteractiveLearningIntegration::navigate_to_tutorial(const std::string& tutorial_id) {
    if (tutorial_panel_) {
        tutorial_panel_->start_tutorial(tutorial_id);
        
        // Show tutorial panel if not visible
        if (!tutorial_panel_->is_visible()) {
            tutorial_panel_->set_visible(true);
        }
        
        // Update current session
        if (current_session_) {
            current_session_->current_tutorial_id = tutorial_id;
        }
        
        LOG_INFO("Navigated to tutorial: " + tutorial_id);
    }
}

void InteractiveLearningIntegration::create_tutorial_from_debugging_session() {
    if (!debugger_panel_) return;
    
    // Create tutorial based on current debugging session
    // This would analyze the debugging session and create educational content
    LOG_INFO("Creating tutorial from current debugging session");
}

// Quiz and assessment integration

void InteractiveLearningIntegration::start_adaptive_assessment() {
    if (education_panel_) {
        education_panel_->set_education_mode(ui::EducationalFeaturesPanel::EducationMode::QuizCenter);
        education_panel_->set_visible(true);
        
        LOG_INFO("Started adaptive assessment");
    }
}

void InteractiveLearningIntegration::create_quiz_from_tutorial_content(const std::string& tutorial_id) {
    // Generate quiz questions based on tutorial content
    LOG_INFO("Creating quiz from tutorial: " + tutorial_id);
}

// Performance learning integration

void InteractiveLearningIntegration::start_performance_comparison_tutorial() {
    if (performance_panel_) {
        performance_panel_->start_guided_learning();
        performance_panel_->set_visible(true);
        
        // Link with tutorial system
        link_performance_analysis_with_tutorial();
        
        LOG_INFO("Started performance comparison tutorial");
    }
}

void InteractiveLearningIntegration::create_benchmark_challenge(const std::string& challenge_id) {
    LOG_INFO("Creating benchmark challenge: " + challenge_id);
}

// Visual debugging integration

void InteractiveLearningIntegration::start_visual_debugging_tutorial() {
    if (debugger_panel_) {
        debugger_panel_->start_debugging();
        debugger_panel_->set_visible(true);
        
        // Start educational debugging tour
        debugger_panel_->start_guided_debugging_tour();
        
        LOG_INFO("Started visual debugging tutorial");
    }
}

void InteractiveLearningIntegration::create_debugging_scenario(const std::string& scenario_id) {
    LOG_INFO("Creating debugging scenario: " + scenario_id);
}

// Analytics and reporting

InteractiveLearningIntegration::LearningAnalytics InteractiveLearningIntegration::generate_comprehensive_analytics() const {
    LearningAnalytics analytics;
    analytics.learner_id = current_learner_id_;
    
    // Gather data from education panel
    if (education_panel_) {
        auto progress = education_panel_->get_learning_progress();
        analytics.total_learning_time_hours = progress.total_learning_time_hours;
        analytics.tutorials_completed = progress.tutorials_completed;
        analytics.quizzes_passed = progress.quizzes_passed;
        analytics.average_quiz_score = progress.overall_quiz_average;
        analytics.overall_progress = education_panel_->get_overall_progress();
        
        // Determine skill level based on progress
        if (analytics.overall_progress >= 0.8f) {
            analytics.current_skill_level = "Advanced";
        } else if (analytics.overall_progress >= 0.5f) {
            analytics.current_skill_level = "Intermediate";
        } else {
            analytics.current_skill_level = "Beginner";
        }
    }
    
    // Add cross-panel insights
    if (tutorial_panel_ && debugger_panel_) {
        // Calculate correlation between tutorial progress and debugging ability
        analytics.tutorial_debugger_correlation = 0.75f; // Placeholder calculation
    }
    
    if (performance_panel_) {
        // Assess understanding of performance concepts
        analytics.performance_understanding = 0.6f; // Placeholder calculation
    }
    
    return analytics;
}

void InteractiveLearningIntegration::export_learning_analytics(const std::string& filename) const {
    auto analytics = generate_comprehensive_analytics();
    
    // Export analytics to JSON file
    std::ofstream file(filename);
    if (file.is_open()) {
        file << "{\n";
        file << "  \"learner_id\": \"" << analytics.learner_id << "\",\n";
        file << "  \"total_learning_time_hours\": " << analytics.total_learning_time_hours << ",\n";
        file << "  \"tutorials_completed\": " << analytics.tutorials_completed << ",\n";
        file << "  \"quizzes_passed\": " << analytics.quizzes_passed << ",\n";
        file << "  \"average_quiz_score\": " << analytics.average_quiz_score << ",\n";
        file << "  \"overall_progress\": " << analytics.overall_progress << ",\n";
        file << "  \"current_skill_level\": \"" << analytics.current_skill_level << "\",\n";
        file << "  \"tutorial_debugger_correlation\": " << analytics.tutorial_debugger_correlation << ",\n";
        file << "  \"performance_understanding\": " << analytics.performance_understanding << "\n";
        file << "}\n";
        file.close();
        
        LOG_INFO("Exported learning analytics to: " + filename);
    } else {
        LOG_ERROR("Failed to export learning analytics to: " + filename);
    }
}

// Event callback registration

void InteractiveLearningIntegration::set_tutorial_started_callback(std::function<void(const std::string&)> callback) {
    on_tutorial_started_callback_ = callback;
}

void InteractiveLearningIntegration::set_tutorial_completed_callback(std::function<void(const std::string&)> callback) {
    on_tutorial_completed_callback_ = callback;
}

void InteractiveLearningIntegration::set_quiz_completed_callback(std::function<void(const std::string&, f32)> callback) {
    on_quiz_completed_callback_ = callback;
}

void InteractiveLearningIntegration::set_achievement_unlocked_callback(std::function<void(const std::string&)> callback) {
    on_achievement_unlocked_callback_ = callback;
}

// Integration utilities

void InteractiveLearningIntegration::highlight_ui_element_across_panels(const std::string& element_id) {
    if (tutorial_panel_) tutorial_panel_->highlight_ui_element(element_id);
    if (debugger_panel_) debugger_panel_->highlight_concept(element_id);
    if (performance_panel_) performance_panel_->highlight_concept(element_id);
}

void InteractiveLearningIntegration::show_contextual_help(const std::string& topic) {
    if (education_panel_) {
        education_panel_->show_explanation(topic);
    }
    if (tutorial_panel_) {
        tutorial_panel_->show_contextual_help(topic);
    }
}

void InteractiveLearningIntegration::trigger_cross_panel_notification(const std::string& message) {
    LOG_INFO("Cross-panel notification: " + message);
    // Would trigger notifications across all visible panels
}

// State queries

std::vector<std::string> InteractiveLearningIntegration::get_active_learning_panels() const {
    std::vector<std::string> active_panels;
    
    if (tutorial_panel_ && tutorial_panel_->is_visible()) active_panels.push_back("tutorial");
    if (debugger_panel_ && debugger_panel_->is_visible()) active_panels.push_back("debugger");
    if (performance_panel_ && performance_panel_->is_visible()) active_panels.push_back("performance");
    if (education_panel_ && education_panel_->is_visible()) active_panels.push_back("education");
    
    return active_panels;
}

// IntegratedLearningExperienceFactory Implementation

std::unique_ptr<Tutorial> IntegratedLearningExperienceFactory::create_ecs_basics_with_debugging() {
    auto tutorial = std::make_unique<Tutorial>("ecs_basics_with_debugging", 
                                               "ECS Basics with Live Debugging", 
                                               TutorialCategory::BasicConcepts, 
                                               DifficultyLevel::Beginner);
    
    tutorial->set_description("Learn ECS fundamentals with hands-on debugging practice");
    tutorial->add_learning_objective("Understand Entity-Component-System architecture");
    tutorial->add_learning_objective("Practice debugging ECS operations");
    tutorial->add_learning_objective("Visualize ECS memory layout");
    
    // Add integrated steps that combine tutorial content with debugging practice
    auto step1 = create_debugger_integration_step("create_entity", "Entity Creation");
    tutorial->add_step(std::move(step1));
    
    auto step2 = create_debugger_integration_step("add_component", "Component Addition");
    tutorial->add_step(std::move(step2));
    
    auto step3 = create_debugger_integration_step("system_execution", "System Processing");
    tutorial->add_step(std::move(step3));
    
    return tutorial;
}

std::unique_ptr<Tutorial> IntegratedLearningExperienceFactory::create_performance_optimization_masterclass() {
    auto tutorial = std::make_unique<Tutorial>("performance_optimization_masterclass",
                                               "Performance Optimization Masterclass",
                                               TutorialCategory::MemoryOptimization,
                                               DifficultyLevel::Advanced);
    
    tutorial->set_description("Master ECS performance optimization with real-time analysis");
    tutorial->add_learning_objective("Understand cache-friendly data structures");
    tutorial->add_learning_objective("Measure and analyze performance bottlenecks");
    tutorial->add_learning_objective("Implement optimization strategies");
    
    // Add performance measurement steps
    auto step1 = create_performance_measurement_step("memory_layout_comparison", "SoA vs AoS Performance");
    tutorial->add_step(std::move(step1));
    
    auto step2 = create_performance_measurement_step("cache_optimization", "Cache Behavior Analysis");
    tutorial->add_step(std::move(step2));
    
    return tutorial;
}

std::unique_ptr<Tutorial> IntegratedLearningExperienceFactory::create_memory_layout_exploration() {
    auto tutorial = std::make_unique<Tutorial>("memory_layout_exploration",
                                               "Memory Layout Deep Dive",
                                               TutorialCategory::MemoryOptimization,
                                               DifficultyLevel::Intermediate);
    
    tutorial->set_description("Explore different memory layouts and their performance implications");
    tutorial->add_learning_objective("Understand memory layout strategies");
    tutorial->add_learning_objective("Compare SoA and AoS performance");
    tutorial->add_learning_objective("Optimize for cache efficiency");
    
    return tutorial;
}

std::unique_ptr<Tutorial> IntegratedLearningExperienceFactory::create_system_design_workshop() {
    auto tutorial = std::make_unique<Tutorial>("system_design_workshop",
                                               "ECS System Design Workshop",
                                               TutorialCategory::SystemDesign,
                                               DifficultyLevel::Intermediate);
    
    tutorial->set_description("Learn to design efficient and maintainable ECS systems");
    tutorial->add_learning_objective("Design system dependencies");
    tutorial->add_learning_objective("Optimize system execution order");
    tutorial->add_learning_objective("Handle system interactions");
    
    return tutorial;
}

void IntegratedLearningExperienceFactory::create_comprehensive_ecs_journey(InteractiveLearningIntegration& integration) {
    // Create a comprehensive learning journey that uses all panels
    integration.start_learning_session("comprehensive");
    integration.show_learning_panels(true);
    
    LOG_INFO("Created comprehensive ECS learning journey");
}

std::unique_ptr<TutorialStep> IntegratedLearningExperienceFactory::create_debugger_integration_step(
    const std::string& step_id, const std::string& debug_operation) {
    
    auto step = std::make_unique<TutorialStep>(step_id, 
                                               "Debug: " + debug_operation, 
                                               "Practice " + debug_operation + " with visual debugger");
    
    step->set_interaction_type(InteractionType::EntityManipulation);
    step->add_hint("Use the visual debugger to observe the operation");
    step->add_hint("Set breakpoints to pause execution");
    step->add_hint("Examine entity state changes");
    
    return step;
}

std::unique_ptr<TutorialStep> IntegratedLearningExperienceFactory::create_performance_measurement_step(
    const std::string& step_id, const std::string& benchmark_type) {
    
    auto step = std::make_unique<TutorialStep>(step_id,
                                               "Measure: " + benchmark_type,
                                               "Run performance benchmarks for " + benchmark_type);
    
    step->set_interaction_type(InteractionType::PerformanceAnalysis);
    step->add_hint("Use the performance comparison panel");
    step->add_hint("Compare different implementations");
    step->add_hint("Analyze the results and explanations");
    
    return step;
}

// Global access functions

InteractiveLearningIntegration& get_learning_integration() {
    if (!g_learning_integration) {
        g_learning_integration = std::make_unique<InteractiveLearningIntegration>();
    }
    return *g_learning_integration;
}

void set_learning_integration(std::unique_ptr<InteractiveLearningIntegration> integration) {
    g_learning_integration = std::move(integration);
}

// Convenience functions implementation

namespace learning_integration {

void quick_start_ecs_tutorial() {
    auto& integration = get_learning_integration();
    integration.start_guided_ecs_introduction();
}

void quick_start_performance_analysis() {
    auto& integration = get_learning_integration();
    integration.start_performance_optimization_workflow();
}

void quick_start_debugging_practice() {
    auto& integration = get_learning_integration();
    integration.start_debugging_mastery_path();
}

void quick_start_adaptive_quiz(const std::string& topic) {
    auto& integration = get_learning_integration();
    integration.start_adaptive_assessment();
}

void save_all_learning_progress() {
    LOG_INFO("Saving all learning progress");
    // Implementation would save progress from all panels
}

void load_learning_progress_for_user(const std::string& learner_id) {
    LOG_INFO("Loading learning progress for user: " + learner_id);
    // Implementation would load user-specific progress
}

void backup_learning_data(const std::string& backup_path) {
    LOG_INFO("Backing up learning data to: " + backup_path);
    // Implementation would create backup of all learning data
}

void restore_learning_data(const std::string& backup_path) {
    LOG_INFO("Restoring learning data from: " + backup_path);
    // Implementation would restore from backup
}

void highlight_ecs_concept(const std::string& concept_name) {
    auto& integration = get_learning_integration();
    integration.highlight_ui_element_across_panels(concept_name);
}

void demonstrate_performance_impact(const std::string& operation) {
    auto& integration = get_learning_integration();
    integration.show_contextual_help("performance_impact_" + operation);
}

void show_debugging_technique(const std::string& technique_name) {
    auto& integration = get_learning_integration();
    integration.show_contextual_help("debugging_technique_" + technique_name);
}

void explain_memory_layout_difference(const std::string& layout_type) {
    auto& integration = get_learning_integration();
    integration.show_contextual_help("memory_layout_" + layout_type);
}

void create_learning_snapshot() {
    LOG_INFO("Creating learning snapshot");
    // Implementation would save current state across all panels
}

void replay_learning_session(const std::string& session_id) {
    LOG_INFO("Replaying learning session: " + session_id);
    // Implementation would replay recorded session
}

void generate_personalized_study_plan(const std::string& learner_id) {
    LOG_INFO("Generating personalized study plan for: " + learner_id);
    // Implementation would create AI-generated study plan
}

void export_comprehensive_learning_report(const std::string& output_path) {
    auto& integration = get_learning_integration();
    integration.export_learning_analytics(output_path);
}

} // namespace learning_integration

} // namespace ecscope::learning