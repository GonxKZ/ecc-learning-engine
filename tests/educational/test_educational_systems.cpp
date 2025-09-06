#include "../framework/ecscope_test_framework.hpp"

#include <ecscope/learning_system.hpp>
#include <ecscope/tutorial_system.hpp>
#include <ecscope/interactive_visualization.hpp>
#include <ecscope/progress_tracking.hpp>
#include <ecscope/adaptive_difficulty.hpp>
#include <ecscope/educational_analytics.hpp>

namespace ECScope::Testing {

class EducationalSystemTest : public ECScopeTestFixture {
protected:
    void SetUp() override {
        ECScopeTestFixture::SetUp();
        
        // Initialize learning system
        learning_system_ = std::make_unique<Education::LearningSystem>();
        learning_system_->initialize();
        
        // Initialize tutorial system
        tutorial_system_ = std::make_unique<Education::TutorialSystem>();
        tutorial_system_->initialize(*world_);
        
        // Initialize visualization system
        visualization_ = std::make_unique<Education::InteractiveVisualization>();
        visualization_->initialize();
        
        // Initialize progress tracking
        progress_tracker_ = std::make_unique<Education::ProgressTracker>();
        progress_tracker_->initialize("test_student_data");
        
        // Initialize adaptive difficulty system
        adaptive_difficulty_ = std::make_unique<Education::AdaptiveDifficulty>();
        adaptive_difficulty_->initialize();
        
        // Initialize analytics
        analytics_ = std::make_unique<Education::EducationalAnalytics>();
        analytics_->initialize("test_analytics_db");
        
        // Create test student profile
        test_student_id_ = "test_student_001";
        Education::StudentProfile profile;
        profile.student_id = test_student_id_;
        profile.name = "Test Student";
        profile.skill_level = Education::SkillLevel::Beginner;
        profile.learning_style = Education::LearningStyle::Visual;
        profile.preferred_pace = Education::LearningPace::Medium;
        
        progress_tracker_->create_student_profile(profile);
    }

    void TearDown() override {
        analytics_.reset();
        adaptive_difficulty_.reset();
        progress_tracker_.reset();
        visualization_.reset();
        tutorial_system_.reset();
        learning_system_.reset();
        
        ECScopeTestFixture::TearDown();
    }

    // Helper to create educational content
    Education::LearningModule create_test_module(const std::string& name, 
                                               Education::DifficultyLevel difficulty = Education::DifficultyLevel::Beginner) {
        Education::LearningModule module;
        module.name = name;
        module.description = "Test module for " + name;
        module.difficulty_level = difficulty;
        module.estimated_duration = 300.0f; // 5 minutes
        
        // Add learning objectives
        module.objectives.push_back("Understand basic concepts");
        module.objectives.push_back("Apply knowledge in practice");
        module.objectives.push_back("Demonstrate mastery");
        
        // Add prerequisites if not beginner
        if (difficulty != Education::DifficultyLevel::Beginner) {
            module.prerequisites.push_back("BasicModule");
        }
        
        return module;
    }

    // Helper to simulate student interaction
    void simulate_student_interaction(Education::SessionID session_id, 
                                    int interaction_count = 10,
                                    bool successful = true) {
        for (int i = 0; i < interaction_count; ++i) {
            Education::LearningEvent event;
            event.type = successful ? Education::LearningEventType::CorrectAnswer : 
                                    Education::LearningEventType::IncorrectAnswer;
            event.description = "Simulated interaction " + std::to_string(i);
            event.timestamp = static_cast<double>(i);
            event.metadata["score"] = successful ? "100" : "25";
            
            learning_system_->record_learning_event(session_id, event);
        }
    }

protected:
    std::unique_ptr<Education::LearningSystem> learning_system_;
    std::unique_ptr<Education::TutorialSystem> tutorial_system_;
    std::unique_ptr<Education::InteractiveVisualization> visualization_;
    std::unique_ptr<Education::ProgressTracker> progress_tracker_;
    std::unique_ptr<Education::AdaptiveDifficulty> adaptive_difficulty_;
    std::unique_ptr<Education::EducationalAnalytics> analytics_;
    
    std::string test_student_id_;
};

// =============================================================================
// Learning System Tests
// =============================================================================

TEST_F(EducationalSystemTest, LearningModuleManagement) {
    // Create test modules
    auto basic_module = create_test_module("BasicECS", Education::DifficultyLevel::Beginner);
    auto intermediate_module = create_test_module("IntermediateECS", Education::DifficultyLevel::Intermediate);
    auto advanced_module = create_test_module("AdvancedECS", Education::DifficultyLevel::Advanced);
    
    // Add modules to learning system
    EXPECT_TRUE(learning_system_->add_module(basic_module));
    EXPECT_TRUE(learning_system_->add_module(intermediate_module));
    EXPECT_TRUE(learning_system_->add_module(advanced_module));
    
    // Test module retrieval
    auto retrieved_basic = learning_system_->get_module("BasicECS");
    EXPECT_NE(retrieved_basic, nullptr);
    EXPECT_EQ(retrieved_basic->name, "BasicECS");
    EXPECT_EQ(retrieved_basic->difficulty_level, Education::DifficultyLevel::Beginner);
    
    // Test module listing
    auto all_modules = learning_system_->get_all_modules();
    EXPECT_EQ(all_modules.size(), 3);
    
    // Test module filtering by difficulty
    auto beginner_modules = learning_system_->get_modules_by_difficulty(Education::DifficultyLevel::Beginner);
    EXPECT_EQ(beginner_modules.size(), 1);
    EXPECT_EQ(beginner_modules[0].name, "BasicECS");
    
    // Test prerequisite validation
    auto available_for_beginner = learning_system_->get_available_modules(test_student_id_);
    EXPECT_GE(available_for_beginner.size(), 1); // At least basic module should be available
    
    // Should not include modules with unmet prerequisites
    bool has_advanced = std::any_of(available_for_beginner.begin(), available_for_beginner.end(),
                                   [](const Education::LearningModule& m) { return m.name == "AdvancedECS"; });
    EXPECT_FALSE(has_advanced); // Advanced module should not be available initially
}

TEST_F(EducationalSystemTest, LearningSessionManagement) {
    // Add test module
    auto test_module = create_test_module("SessionTestModule");
    learning_system_->add_module(test_module);
    
    // Start learning session
    auto session_id = learning_system_->start_learning_session("SessionTestModule", test_student_id_);
    EXPECT_NE(session_id, Education::INVALID_SESSION_ID);
    
    // Verify session is active
    EXPECT_TRUE(learning_system_->is_session_active(session_id));
    
    // Get session info
    auto session_info = learning_system_->get_session_info(session_id);
    EXPECT_NE(session_info, nullptr);
    EXPECT_EQ(session_info->module_name, "SessionTestModule");
    EXPECT_EQ(session_info->student_id, test_student_id_);
    
    // Simulate learning activities
    simulate_student_interaction(session_id, 5, true);
    
    // Check session progress
    auto progress = learning_system_->get_session_progress(session_id);
    EXPECT_GT(progress.events_recorded, 0);
    EXPECT_GT(progress.session_duration, 0.0f);
    
    // End session
    learning_system_->end_learning_session(session_id);
    EXPECT_FALSE(learning_system_->is_session_active(session_id));
    
    // Get final results
    auto results = learning_system_->get_session_results(session_id);
    EXPECT_GT(results.completion_percentage, 0.0f);
    EXPECT_GT(results.events_recorded, 0);
}

TEST_F(EducationalSystemTest, LearningEventTracking) {
    auto test_module = create_test_module("EventTrackingModule");
    learning_system_->add_module(test_module);
    
    auto session_id = learning_system_->start_learning_session("EventTrackingModule", test_student_id_);
    
    // Record different types of events
    std::vector<Education::LearningEventType> event_types = {
        Education::LearningEventType::ModuleStarted,
        Education::LearningEventType::ConceptIntroduced,
        Education::LearningEventType::InteractionCompleted,
        Education::LearningEventType::CorrectAnswer,
        Education::LearningEventType::IncorrectAnswer,
        Education::LearningEventType::HintRequested,
        Education::LearningEventType::ObjectiveCompleted,
        Education::LearningEventType::ModuleCompleted
    };
    
    for (size_t i = 0; i < event_types.size(); ++i) {
        Education::LearningEvent event;
        event.type = event_types[i];
        event.description = "Test event " + std::to_string(i);
        event.timestamp = static_cast<double>(i);
        event.metadata["test_data"] = "value_" + std::to_string(i);
        
        learning_system_->record_learning_event(session_id, event);
    }
    
    // Verify events were recorded
    auto session_events = learning_system_->get_session_events(session_id);
    EXPECT_EQ(session_events.size(), event_types.size());
    
    // Check event ordering
    for (size_t i = 1; i < session_events.size(); ++i) {
        EXPECT_GE(session_events[i].timestamp, session_events[i-1].timestamp);
    }
    
    // Check event filtering
    auto correct_answer_events = learning_system_->get_session_events_by_type(
        session_id, Education::LearningEventType::CorrectAnswer);
    EXPECT_EQ(correct_answer_events.size(), 1);
    
    learning_system_->end_learning_session(session_id);
}

// =============================================================================
// Tutorial System Tests
// =============================================================================

TEST_F(EducationalSystemTest, TutorialCreationAndExecution) {
    // Create tutorial steps
    Education::TutorialStep step1;
    step1.id = "step_1";
    step1.title = "Create Your First Entity";
    step1.description = "Learn how to create entities in ECScope";
    step1.instruction = "Click the 'Create Entity' button";
    step1.expected_action = Education::TutorialAction::CreateEntity;
    
    Education::TutorialStep step2;
    step2.id = "step_2";
    step2.title = "Add a Component";
    step2.description = "Add a Transform component to your entity";
    step2.instruction = "Select your entity and add a Transform component";
    step2.expected_action = Education::TutorialAction::AddComponent;
    step2.component_type = "Transform3D";
    
    Education::TutorialStep step3;
    step3.id = "step_3";
    step3.title = "Modify Component";
    step3.description = "Change the entity's position";
    step3.instruction = "Set the position to (1, 2, 3)";
    step3.expected_action = Education::TutorialAction::ModifyComponent;
    
    // Create tutorial
    Education::Tutorial tutorial;
    tutorial.id = "basic_entity_tutorial";
    tutorial.name = "Basic Entity Tutorial";
    tutorial.description = "Learn the basics of entity creation and modification";
    tutorial.steps = {step1, step2, step3};
    
    // Add tutorial to system
    EXPECT_TRUE(tutorial_system_->add_tutorial(tutorial));
    
    // Start tutorial
    auto tutorial_session = tutorial_system_->start_tutorial("basic_entity_tutorial", test_student_id_);
    EXPECT_NE(tutorial_session, Education::INVALID_TUTORIAL_SESSION);
    
    // Verify tutorial is active
    EXPECT_TRUE(tutorial_system_->is_tutorial_active(tutorial_session));
    
    // Get current step
    auto current_step = tutorial_system_->get_current_step(tutorial_session);
    EXPECT_NE(current_step, nullptr);
    EXPECT_EQ(current_step->id, "step_1");
    
    // Simulate completing first step
    auto created_entity = world_->create_entity();
    tutorial_system_->notify_action_completed(tutorial_session, 
                                           Education::TutorialAction::CreateEntity,
                                           {{"entity_id", std::to_string(static_cast<uint32_t>(created_entity))}});
    
    // Should advance to next step
    current_step = tutorial_system_->get_current_step(tutorial_session);
    EXPECT_NE(current_step, nullptr);
    EXPECT_EQ(current_step->id, "step_2");
    
    // Complete second step
    world_->add_component<Transform3D>(created_entity, Vec3(0, 0, 0));
    tutorial_system_->notify_action_completed(tutorial_session,
                                           Education::TutorialAction::AddComponent,
                                           {{"component_type", "Transform3D"}});
    
    // Complete third step
    auto& transform = world_->get_component<Transform3D>(created_entity);
    transform.position = Vec3(1, 2, 3);
    tutorial_system_->notify_action_completed(tutorial_session,
                                           Education::TutorialAction::ModifyComponent);
    
    // Tutorial should be complete
    EXPECT_TRUE(tutorial_system_->is_tutorial_complete(tutorial_session));
    
    // Get tutorial results
    auto results = tutorial_system_->get_tutorial_results(tutorial_session);
    EXPECT_EQ(results.steps_completed, 3);
    EXPECT_GT(results.completion_time, 0.0f);
    EXPECT_EQ(results.hints_used, 0);
}

TEST_F(EducationalSystemTest, TutorialHintSystem) {
    // Create tutorial with hints
    Education::TutorialStep step_with_hints;
    step_with_hints.id = "hint_test_step";
    step_with_hints.title = "Test Hints";
    step_with_hints.description = "A step to test the hint system";
    step_with_hints.instruction = "Perform a complex action";
    step_with_hints.expected_action = Education::TutorialAction::CreateEntity;
    
    // Add multiple hints with increasing specificity
    step_with_hints.hints.push_back("Look for buttons on the toolbar");
    step_with_hints.hints.push_back("The button you need is labeled 'Create Entity'");
    step_with_hints.hints.push_back("Click the 'Create Entity' button in the top-left toolbar");
    
    Education::Tutorial hint_tutorial;
    hint_tutorial.id = "hint_tutorial";
    hint_tutorial.name = "Hint System Tutorial";
    hint_tutorial.steps = {step_with_hints};
    
    tutorial_system_->add_tutorial(hint_tutorial);
    auto session = tutorial_system_->start_tutorial("hint_tutorial", test_student_id_);
    
    // Test hint progression
    auto hint = tutorial_system_->get_next_hint(session);
    EXPECT_FALSE(hint.empty());
    EXPECT_EQ(hint, "Look for buttons on the toolbar");
    
    hint = tutorial_system_->get_next_hint(session);
    EXPECT_EQ(hint, "The button you need is labeled 'Create Entity'");
    
    hint = tutorial_system_->get_next_hint(session);
    EXPECT_EQ(hint, "Click the 'Create Entity' button in the top-left toolbar");
    
    // No more hints available
    hint = tutorial_system_->get_next_hint(session);
    EXPECT_TRUE(hint.empty());
    
    // Check hint usage tracking
    auto results = tutorial_system_->get_tutorial_results(session);
    EXPECT_EQ(results.hints_used, 3);
}

// =============================================================================
// Interactive Visualization Tests
// =============================================================================

TEST_F(EducationalSystemTest, ConceptVisualization) {
    // Test entity-component visualization
    auto entity = world_->create_entity();
    world_->add_component<Transform3D>(entity, Vec3(1, 2, 3));
    world_->add_component<TestVelocity>(entity, TestVelocity{4, 5, 6});
    world_->add_component<TestHealth>(entity, TestHealth{75, 100});
    
    // Create visualization for this entity
    auto visualization_id = visualization_->create_entity_visualization(entity, *world_);
    EXPECT_NE(visualization_id, Education::INVALID_VISUALIZATION_ID);
    
    // Get visualization data
    auto viz_data = visualization_->get_visualization_data(visualization_id);
    EXPECT_NE(viz_data, nullptr);
    EXPECT_EQ(viz_data->entity_id, entity);
    EXPECT_GT(viz_data->components.size(), 0);
    
    // Test component highlighting
    visualization_->highlight_component(visualization_id, "Transform3D");
    EXPECT_TRUE(visualization_->is_component_highlighted(visualization_id, "Transform3D"));
    
    visualization_->unhighlight_component(visualization_id, "Transform3D");
    EXPECT_FALSE(visualization_->is_component_highlighted(visualization_id, "Transform3D"));
    
    // Test animation system
    Education::VisualizationAnimation anim;
    anim.type = Education::AnimationType::ComponentAddition;
    anim.target_component = "TestTag";
    anim.duration = 2.0f;
    anim.easing = Education::EasingType::EaseInOut;
    
    visualization_->start_animation(visualization_id, anim);
    EXPECT_TRUE(visualization_->is_animation_playing(visualization_id));
    
    // Update animation
    for (int i = 0; i < 120; ++i) { // 2 seconds at 60 FPS
        visualization_->update(1.0f / 60.0f);
    }
    
    EXPECT_FALSE(visualization_->is_animation_playing(visualization_id));
}

TEST_F(EducationalSystemTest, MemoryVisualization) {
    // Create entities to visualize memory layout
    constexpr int entity_count = 100;
    EntityFactory factory(*world_);
    
    std::vector<Entity> entities;
    for (int i = 0; i < entity_count; ++i) {
        entities.push_back(factory.create_full_entity());
    }
    
    // Create memory visualization
    auto memory_viz = visualization_->create_memory_visualization(*world_);
    EXPECT_NE(memory_viz, Education::INVALID_VISUALIZATION_ID);
    
    // Get memory layout data
    auto memory_data = visualization_->get_memory_visualization_data(memory_viz);
    EXPECT_NE(memory_data, nullptr);
    EXPECT_GT(memory_data->archetypes.size(), 0);
    
    // Verify archetype information
    for (const auto& archetype_info : memory_data->archetypes) {
        EXPECT_GT(archetype_info.entity_count, 0);
        EXPECT_GT(archetype_info.component_types.size(), 0);
        EXPECT_GT(archetype_info.memory_usage, 0);
    }
    
    // Test memory hotspot detection
    auto hotspots = visualization_->detect_memory_hotspots(memory_viz);
    EXPECT_GE(hotspots.size(), 0); // May or may not have hotspots
    
    // Test cache visualization
    auto cache_viz = visualization_->create_cache_visualization(*world_);
    auto cache_data = visualization_->get_cache_visualization_data(cache_viz);
    
    EXPECT_NE(cache_data, nullptr);
    EXPECT_GE(cache_data->cache_lines.size(), 0);
}

// =============================================================================
// Progress Tracking Tests
// =============================================================================

TEST_F(EducationalSystemTest, StudentProgressTracking) {
    // Create learning modules with progression
    auto basic_module = create_test_module("BasicModule", Education::DifficultyLevel::Beginner);
    auto intermediate_module = create_test_module("IntermediateModule", Education::DifficultyLevel::Intermediate);
    
    learning_system_->add_module(basic_module);
    learning_system_->add_module(intermediate_module);
    
    // Start and complete basic module
    auto basic_session = learning_system_->start_learning_session("BasicModule", test_student_id_);
    simulate_student_interaction(basic_session, 10, true); // Successful completion
    learning_system_->end_learning_session(basic_session);
    
    // Update progress tracker
    progress_tracker_->update_module_progress(test_student_id_, "BasicModule", 100.0f);
    
    // Check overall progress
    auto overall_progress = progress_tracker_->get_overall_progress(test_student_id_);
    EXPECT_GT(overall_progress.modules_completed, 0);
    EXPECT_GT(overall_progress.total_time_spent, 0.0f);
    EXPECT_GT(overall_progress.skill_points_earned, 0);
    
    // Check specific module progress
    auto module_progress = progress_tracker_->get_module_progress(test_student_id_, "BasicModule");
    EXPECT_EQ(module_progress.completion_percentage, 100.0f);
    EXPECT_TRUE(module_progress.is_completed);
    
    // Test skill assessment
    auto skill_assessment = progress_tracker_->assess_student_skills(test_student_id_);
    EXPECT_GT(skill_assessment.conceptual_understanding, 0.0f);
    EXPECT_GT(skill_assessment.practical_application, 0.0f);
    EXPECT_LE(skill_assessment.conceptual_understanding, 1.0f);
    EXPECT_LE(skill_assessment.practical_application, 1.0f);
    
    // Test learning path recommendations
    auto recommendations = progress_tracker_->get_learning_recommendations(test_student_id_);
    EXPECT_GT(recommendations.size(), 0);
    
    // Should recommend intermediate module after completing basic
    bool recommends_intermediate = std::any_of(recommendations.begin(), recommendations.end(),
        [](const Education::LearningRecommendation& rec) {
            return rec.module_name == "IntermediateModule";
        });
    EXPECT_TRUE(recommends_intermediate);
}

TEST_F(EducationalSystemTest, AchievementSystem) {
    // Define achievements
    Education::Achievement first_entity_achievement;
    first_entity_achievement.id = "first_entity";
    first_entity_achievement.name = "First Steps";
    first_entity_achievement.description = "Create your first entity";
    first_entity_achievement.points = 10;
    first_entity_achievement.criteria.push_back({"entities_created", "1"});
    
    Education::Achievement speed_learner_achievement;
    speed_learner_achievement.id = "speed_learner";
    speed_learner_achievement.name = "Speed Learner";
    speed_learner_achievement.description = "Complete a module in under 5 minutes";
    speed_learner_achievement.points = 25;
    speed_learner_achievement.criteria.push_back({"module_completion_time", "<300"});
    
    // Add achievements to progress tracker
    progress_tracker_->add_achievement(first_entity_achievement);
    progress_tracker_->add_achievement(speed_learner_achievement);
    
    // Simulate activities that trigger achievements
    progress_tracker_->track_student_action(test_student_id_, "entity_created", {});
    
    // Check if achievement was unlocked
    auto unlocked_achievements = progress_tracker_->get_unlocked_achievements(test_student_id_);
    
    bool has_first_entity = std::any_of(unlocked_achievements.begin(), unlocked_achievements.end(),
        [](const Education::Achievement& ach) { return ach.id == "first_entity"; });
    EXPECT_TRUE(has_first_entity);
    
    // Test achievement progress
    auto achievement_progress = progress_tracker_->get_achievement_progress(test_student_id_, "speed_learner");
    EXPECT_GE(achievement_progress, 0.0f);
    EXPECT_LE(achievement_progress, 1.0f);
}

// =============================================================================
// Adaptive Difficulty Tests
// =============================================================================

TEST_F(EducationalSystemTest, AdaptiveDifficultyAdjustment) {
    // Set up initial difficulty assessment
    Education::DifficultyAssessment initial_assessment;
    initial_assessment.student_id = test_student_id_;
    initial_assessment.current_level = Education::DifficultyLevel::Beginner;
    initial_assessment.confidence_score = 0.7f;
    initial_assessment.accuracy_rate = 0.8f;
    initial_assessment.completion_time_factor = 1.2f; // Slightly slower than average
    
    adaptive_difficulty_->initialize_student_assessment(initial_assessment);
    
    // Simulate learning performance data
    std::vector<Education::PerformanceData> performance_history;
    
    // Good performance - should increase difficulty
    for (int i = 0; i < 5; ++i) {
        Education::PerformanceData good_performance;
        good_performance.accuracy = 0.95f;
        good_performance.completion_time = 180.0f; // Fast completion
        good_performance.hints_used = 0;
        good_performance.attempts_required = 1;
        good_performance.timestamp = static_cast<double>(i * 60);
        
        performance_history.push_back(good_performance);
    }
    
    // Update difficulty based on performance
    auto new_assessment = adaptive_difficulty_->update_difficulty(test_student_id_, performance_history);
    
    EXPECT_GE(new_assessment.current_level, initial_assessment.current_level);
    EXPECT_GT(new_assessment.confidence_score, initial_assessment.confidence_score);
    
    // Test difficulty adjustment recommendations
    auto adjustment_recommendation = adaptive_difficulty_->recommend_difficulty_adjustment(test_student_id_);
    
    if (adjustment_recommendation.adjustment_type == Education::DifficultyAdjustment::Increase) {
        EXPECT_GT(adjustment_recommendation.new_level, adjustment_recommendation.current_level);
    }
    
    // Test poor performance scenario
    performance_history.clear();
    
    for (int i = 0; i < 5; ++i) {
        Education::PerformanceData poor_performance;
        poor_performance.accuracy = 0.4f;
        poor_performance.completion_time = 600.0f; // Slow completion
        poor_performance.hints_used = 3;
        poor_performance.attempts_required = 4;
        poor_performance.timestamp = static_cast<double>(i * 60 + 300);
        
        performance_history.push_back(poor_performance);
    }
    
    new_assessment = adaptive_difficulty_->update_difficulty(test_student_id_, performance_history);
    adjustment_recommendation = adaptive_difficulty_->recommend_difficulty_adjustment(test_student_id_);
    
    // Should recommend decreasing difficulty after poor performance
    EXPECT_EQ(adjustment_recommendation.adjustment_type, Education::DifficultyAdjustment::Decrease);
}

TEST_F(EducationalSystemTest, PersonalizedLearningPath) {
    // Create multiple modules with different characteristics
    auto visual_module = create_test_module("VisualLearning", Education::DifficultyLevel::Beginner);
    visual_module.learning_style_preference = Education::LearningStyle::Visual;
    
    auto kinesthetic_module = create_test_module("HandsOnLearning", Education::DifficultyLevel::Beginner);
    kinesthetic_module.learning_style_preference = Education::LearningStyle::Kinesthetic;
    
    auto theoretical_module = create_test_module("TheoreticalLearning", Education::DifficultyLevel::Beginner);
    theoretical_module.learning_style_preference = Education::LearningStyle::Theoretical;
    
    learning_system_->add_module(visual_module);
    learning_system_->add_module(kinesthetic_module);
    learning_system_->add_module(theoretical_module);
    
    // Get personalized learning path for visual learner
    auto learning_path = adaptive_difficulty_->generate_personalized_path(test_student_id_);
    
    EXPECT_GT(learning_path.recommended_modules.size(), 0);
    
    // Visual learner should get visual modules prioritized
    if (!learning_path.recommended_modules.empty()) {
        auto first_recommendation = learning_path.recommended_modules[0];
        // While we can't guarantee it's the visual module first, 
        // we can check that the path considers learning style
        EXPECT_NE(first_recommendation.reasoning.find("learning style"), std::string::npos);
    }
    
    // Test path adaptation based on progress
    progress_tracker_->update_module_progress(test_student_id_, "VisualLearning", 100.0f);
    
    auto updated_path = adaptive_difficulty_->generate_personalized_path(test_student_id_);
    EXPECT_NE(updated_path.path_id, learning_path.path_id); // Should be different path
}

// =============================================================================
// Educational Analytics Tests
// =============================================================================

TEST_F(EducationalSystemTest, LearningAnalytics) {
    // Generate learning data
    auto test_module = create_test_module("AnalyticsTestModule");
    learning_system_->add_module(test_module);
    
    // Simulate multiple learning sessions
    for (int session_num = 0; session_num < 5; ++session_num) {
        auto session_id = learning_system_->start_learning_session("AnalyticsTestModule", test_student_id_);
        
        // Vary performance across sessions
        bool good_performance = (session_num % 2 == 0);
        simulate_student_interaction(session_id, 8, good_performance);
        
        learning_system_->end_learning_session(session_id);
        
        // Record analytics data
        Education::AnalyticsEvent analytics_event;
        analytics_event.student_id = test_student_id_;
        analytics_event.module_name = "AnalyticsTestModule";
        analytics_event.session_id = session_id;
        analytics_event.event_type = "session_completed";
        analytics_event.performance_score = good_performance ? 0.85f : 0.45f;
        analytics_event.timestamp = std::chrono::system_clock::now();
        
        analytics_->record_event(analytics_event);
    }
    
    // Generate analytics reports
    auto student_report = analytics_->generate_student_report(test_student_id_);
    EXPECT_NE(student_report, nullptr);
    EXPECT_GT(student_report->total_sessions, 0);
    EXPECT_GT(student_report->total_time_spent, 0.0f);
    
    // Check learning trends
    auto learning_trends = analytics_->analyze_learning_trends(test_student_id_);
    EXPECT_GT(learning_trends.data_points.size(), 0);
    
    // Test cohort analysis (comparing with other students)
    // Add another test student for comparison
    std::string test_student_2 = "test_student_002";
    Education::StudentProfile profile2;
    profile2.student_id = test_student_2;
    profile2.name = "Test Student 2";
    profile2.skill_level = Education::SkillLevel::Intermediate;
    progress_tracker_->create_student_profile(profile2);
    
    // Add some data for second student
    for (int i = 0; i < 3; ++i) {
        Education::AnalyticsEvent event;
        event.student_id = test_student_2;
        event.module_name = "AnalyticsTestModule";
        event.event_type = "session_completed";
        event.performance_score = 0.9f; // High performer
        analytics_->record_event(event);
    }
    
    // Generate cohort analysis
    std::vector<std::string> cohort = {test_student_id_, test_student_2};
    auto cohort_analysis = analytics_->analyze_cohort_performance(cohort);
    
    EXPECT_EQ(cohort_analysis.student_count, 2);
    EXPECT_GT(cohort_analysis.average_performance, 0.0f);
    EXPECT_LT(cohort_analysis.average_performance, 1.0f);
    
    // Test predictive analytics
    auto predictions = analytics_->predict_student_outcomes(test_student_id_);
    EXPECT_GE(predictions.completion_probability, 0.0f);
    EXPECT_LE(predictions.completion_probability, 1.0f);
    EXPECT_GT(predictions.estimated_completion_time, 0.0f);
}

TEST_F(EducationalSystemTest, ContentEffectivenessAnalysis) {
    // Create modules with different success rates
    auto effective_module = create_test_module("EffectiveModule");
    auto challenging_module = create_test_module("ChallengingModule");
    
    learning_system_->add_module(effective_module);
    learning_system_->add_module(challenging_module);
    
    // Simulate high success rate for effective module
    for (int i = 0; i < 10; ++i) {
        std::string student_id = "student_" + std::to_string(i);
        
        // Create student profile
        Education::StudentProfile profile;
        profile.student_id = student_id;
        profile.name = "Student " + std::to_string(i);
        progress_tracker_->create_student_profile(profile);
        
        // High success for effective module
        Education::AnalyticsEvent event;
        event.student_id = student_id;
        event.module_name = "EffectiveModule";
        event.event_type = "module_completed";
        event.performance_score = 0.85f + (static_cast<float>(rand()) / RAND_MAX) * 0.10f; // 85-95%
        event.completion_time = 240.0f + (static_cast<float>(rand()) / RAND_MAX) * 60.0f; // 4-5 minutes
        analytics_->record_event(event);
        
        // Lower success for challenging module
        event.module_name = "ChallengingModule";
        event.performance_score = 0.45f + (static_cast<float>(rand()) / RAND_MAX) * 0.20f; // 45-65%
        event.completion_time = 480.0f + (static_cast<float>(rand()) / RAND_MAX) * 120.0f; // 8-10 minutes
        analytics_->record_event(event);
    }
    
    // Analyze content effectiveness
    auto effective_analysis = analytics_->analyze_content_effectiveness("EffectiveModule");
    auto challenging_analysis = analytics_->analyze_content_effectiveness("ChallengingModule");
    
    EXPECT_GT(effective_analysis.average_performance, challenging_analysis.average_performance);
    EXPECT_LT(effective_analysis.average_completion_time, challenging_analysis.average_completion_time);
    EXPECT_GT(effective_analysis.completion_rate, challenging_analysis.completion_rate);
    
    // Test improvement recommendations
    auto recommendations = analytics_->generate_content_improvement_recommendations("ChallengingModule");
    EXPECT_GT(recommendations.size(), 0);
    
    // Should recommend improvements for challenging content
    bool has_difficulty_recommendation = std::any_of(recommendations.begin(), recommendations.end(),
        [](const Education::ImprovementRecommendation& rec) {
            return rec.type == Education::RecommendationType::ReduceDifficulty ||
                   rec.type == Education::RecommendationType::AddMoreExamples ||
                   rec.type == Education::RecommendationType::ImproveExplanation;
        });
    EXPECT_TRUE(has_difficulty_recommendation);
}

// =============================================================================
// Integration Test: Complete Educational Workflow
// =============================================================================

TEST_F(EducationalSystemTest, CompleteEducationalWorkflow) {
    // This test simulates a complete educational workflow from start to finish
    
    // 1. Set up educational content
    auto intro_module = create_test_module("IntroToECS", Education::DifficultyLevel::Beginner);
    auto basic_module = create_test_module("BasicECSOperations", Education::DifficultyLevel::Beginner);
    auto intermediate_module = create_test_module("AdvancedECS", Education::DifficultyLevel::Intermediate);
    
    learning_system_->add_module(intro_module);
    learning_system_->add_module(basic_module);
    learning_system_->add_module(intermediate_module);
    
    // Create tutorial for hands-on learning
    Education::TutorialStep create_step;
    create_step.id = "create_entities";
    create_step.title = "Create Entities";
    create_step.description = "Learn to create entities";
    create_step.instruction = "Create 3 entities";
    create_step.expected_action = Education::TutorialAction::CreateEntity;
    
    Education::TutorialStep component_step;
    component_step.id = "add_components";
    component_step.title = "Add Components";
    component_step.description = "Add components to entities";
    component_step.instruction = "Add Transform and Velocity components";
    component_step.expected_action = Education::TutorialAction::AddComponent;
    
    Education::Tutorial hands_on_tutorial;
    hands_on_tutorial.id = "hands_on_ecs";
    hands_on_tutorial.name = "Hands-On ECS Tutorial";
    hands_on_tutorial.steps = {create_step, component_step};
    
    tutorial_system_->add_tutorial(hands_on_tutorial);
    
    // 2. Start student learning journey
    
    // Begin with intro module
    auto intro_session = learning_system_->start_learning_session("IntroToECS", test_student_id_);
    
    // Create visualization for concepts
    auto concept_viz = visualization_->create_concept_visualization("Entity-Component-System");
    visualization_->highlight_concept_part(concept_viz, "Entity");
    
    // Simulate learning the intro module
    simulate_student_interaction(intro_session, 12, true);
    learning_system_->end_learning_session(intro_session);
    
    // Update progress
    progress_tracker_->update_module_progress(test_student_id_, "IntroToECS", 100.0f);
    
    // 3. Hands-on tutorial
    auto tutorial_session = tutorial_system_->start_tutorial("hands_on_ecs", test_student_id_);
    
    // Simulate tutorial completion
    auto entity1 = world_->create_entity();
    auto entity2 = world_->create_entity();
    auto entity3 = world_->create_entity();
    
    tutorial_system_->notify_action_completed(tutorial_session, Education::TutorialAction::CreateEntity);
    
    world_->add_component<Transform3D>(entity1, Vec3(1, 0, 0));
    world_->add_component<TestVelocity>(entity1, TestVelocity{1, 0, 0});
    world_->add_component<Transform3D>(entity2, Vec3(0, 1, 0));
    world_->add_component<TestVelocity>(entity2, TestVelocity{0, 1, 0});
    
    tutorial_system_->notify_action_completed(tutorial_session, Education::TutorialAction::AddComponent);
    
    // 4. Move to basic operations module
    auto basic_session = learning_system_->start_learning_session("BasicECSOperations", test_student_id_);
    
    // Create memory visualization to show layout
    auto memory_viz = visualization_->create_memory_visualization(*world_);
    
    // Simulate learning with some challenges
    simulate_student_interaction(basic_session, 8, false); // Some failures initially
    simulate_student_interaction(basic_session, 6, true);  // Then success
    
    learning_system_->end_learning_session(basic_session);
    progress_tracker_->update_module_progress(test_student_id_, "BasicECSOperations", 90.0f);
    
    // 5. Adaptive difficulty assessment
    std::vector<Education::PerformanceData> performance_data;
    Education::PerformanceData recent_performance;
    recent_performance.accuracy = 0.75f;
    recent_performance.completion_time = 400.0f;
    recent_performance.hints_used = 2;
    recent_performance.attempts_required = 2;
    performance_data.push_back(recent_performance);
    
    auto difficulty_update = adaptive_difficulty_->update_difficulty(test_student_id_, performance_data);
    auto learning_path = adaptive_difficulty_->generate_personalized_path(test_student_id_);
    
    // 6. Analytics and reporting
    Education::AnalyticsEvent completion_event;
    completion_event.student_id = test_student_id_;
    completion_event.module_name = "BasicECSOperations";
    completion_event.event_type = "workflow_completed";
    completion_event.performance_score = 0.75f;
    completion_event.completion_time = 800.0f; // Total time for all activities
    analytics_->record_event(completion_event);
    
    // Generate final reports
    auto student_report = analytics_->generate_student_report(test_student_id_);
    auto progress_summary = progress_tracker_->get_overall_progress(test_student_id_);
    
    // 7. Validate complete workflow
    
    // Should have completed multiple modules
    EXPECT_GE(progress_summary.modules_completed, 2);
    EXPECT_GT(progress_summary.total_time_spent, 0.0f);
    
    // Should have tutorial completion
    auto tutorial_results = tutorial_system_->get_tutorial_results(tutorial_session);
    EXPECT_EQ(tutorial_results.steps_completed, 2);
    
    // Should have visualization data
    auto viz_data = visualization_->get_visualization_data(concept_viz);
    EXPECT_NE(viz_data, nullptr);
    
    // Should have analytics data
    EXPECT_NE(student_report, nullptr);
    EXPECT_GT(student_report->total_sessions, 2);
    
    // Should have personalized recommendations
    EXPECT_GT(learning_path.recommended_modules.size(), 0);
    
    // Verify no memory leaks throughout the educational workflow
    EXPECT_NO_MEMORY_LEAKS();
    
    std::cout << "Educational Workflow Results:\n";
    std::cout << "  Modules Completed: " << progress_summary.modules_completed << "\n";
    std::cout << "  Total Learning Time: " << progress_summary.total_time_spent << "s\n";
    std::cout << "  Tutorial Steps Completed: " << tutorial_results.steps_completed << "\n";
    std::cout << "  Final Skill Level: " << static_cast<int>(difficulty_update.current_level) << "\n";
    std::cout << "  Analytics Sessions Recorded: " << student_report->total_sessions << "\n";
}

} // namespace ECScope::Testing