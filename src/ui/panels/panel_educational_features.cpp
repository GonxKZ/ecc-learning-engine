#include "panel_educational_features.hpp"
#include "core/log.hpp"
#include "../imgui_utils.hpp"
#include <imgui.h>
#include <algorithm>
#include <numeric>
#include <sstream>
#include <iomanip>
#include <cmath>
#include <random>

namespace ecscope::ui {

// EducationalFeaturesPanel Implementation

EducationalFeaturesPanel::EducationalFeaturesPanel(std::shared_ptr<learning::TutorialManager> tutorial_mgr)
    : Panel("Educational Features", true), tutorial_manager_(tutorial_mgr) {
    
    // Initialize subsystems
    adaptive_engine_ = std::make_unique<AdaptiveLearningEngine>();
    quiz_system_ = std::make_unique<QuizSystem>();
    
    // Initialize current learner
    current_progress_ = DetailedLearningProgress(current_learner_id_);
    
    // Setup default achievements
    register_achievement(LearningAchievement("first_tutorial", "First Steps", 
                                            "Complete your first tutorial", 
                                            LearningAchievement::UnlockType::TutorialsCompleted));
    
    register_achievement(LearningAchievement("quiz_master", "Quiz Master", 
                                            "Pass 10 quizzes with 80% or higher", 
                                            LearningAchievement::UnlockType::QuizzesPassedWithScore));
    
    register_achievement(LearningAchievement("perfect_score", "Perfectionist", 
                                            "Get 100% on any quiz", 
                                            LearningAchievement::UnlockType::PerfectQuizScore));
    
    // Initialize default quiz questions
    std::vector<QuizQuestion> basic_ecs_questions;
    
    QuizQuestion q1("ecs_basics_1", "What does ECS stand for?", QuestionType::MultipleChoice);
    q1.options = {"Entity Component System", "Entity Control System", "Event Component System", "Extended Component System"};
    q1.correct_answers = {0};
    q1.explanation = "ECS stands for Entity-Component-System, a popular architectural pattern in game development.";
    q1.topics = {"ECS Basics"};
    basic_ecs_questions.push_back(q1);
    
    QuizQuestion q2("ecs_basics_2", "True or False: In ECS, entities contain both data and logic.", QuestionType::TrueFalse);
    q2.correct_text_answer = "false";
    q2.explanation = "False. In ECS, entities are just IDs. Components contain data, and systems contain logic.";
    q2.topics = {"ECS Basics", "Architecture"};
    basic_ecs_questions.push_back(q2);
    
    quiz_system_->add_quiz_bank("ECS Basics", basic_ecs_questions);
    
    // Initialize dashboard
    dashboard_.daily_tip = "Start with small, focused learning sessions for better retention!";
    dashboard_.motivational_quote = "The expert in anything was once a beginner.";
    
    LOG_INFO("Educational Features Panel initialized");
}

void EducationalFeaturesPanel::render() {
    if (!visible_) return;
    
    ImGui::SetNextWindowSize(ImVec2(1000, 700), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSizeConstraints(ImVec2(MIN_PANEL_WIDTH, MIN_PANEL_HEIGHT), ImVec2(FLT_MAX, FLT_MAX));
    
    if (ImGui::Begin(name_.c_str(), &visible_, ImGuiWindowFlags_MenuBar)) {
        
        // Menu bar
        if (ImGui::BeginMenuBar()) {
            if (ImGui::BeginMenu("Learning")) {
                if (ImGui::MenuItem("Dashboard", nullptr, current_mode_ == EducationMode::Dashboard)) {
                    set_education_mode(EducationMode::Dashboard);
                }
                if (ImGui::MenuItem("Tutorials", nullptr, current_mode_ == EducationMode::TutorialBrowser)) {
                    set_education_mode(EducationMode::TutorialBrowser);
                }
                if (ImGui::MenuItem("Quizzes", nullptr, current_mode_ == EducationMode::QuizCenter)) {
                    set_education_mode(EducationMode::QuizCenter);
                }
                if (ImGui::MenuItem("Learning Path", nullptr, current_mode_ == EducationMode::LearningPath)) {
                    set_education_mode(EducationMode::LearningPath);
                }
                
                ImGui::EndMenu();
            }
            
            if (ImGui::BeginMenu("Progress")) {
                if (ImGui::MenuItem("Analysis", nullptr, current_mode_ == EducationMode::ProgressAnalysis)) {
                    set_education_mode(EducationMode::ProgressAnalysis);
                }
                if (ImGui::MenuItem("Achievements", nullptr, current_mode_ == EducationMode::Achievements)) {
                    set_education_mode(EducationMode::Achievements);
                }
                
                ImGui::Separator();
                
                if (ImGui::MenuItem("Export Progress Report")) {
                    export_progress_report("learning_progress.json");
                }
                if (ImGui::MenuItem("Reset Progress")) {
                    reset_learning_progress();
                }
                
                ImGui::EndMenu();
            }
            
            if (ImGui::BeginMenu("Settings")) {
                if (ImGui::MenuItem("Preferences", nullptr, current_mode_ == EducationMode::Settings)) {
                    set_education_mode(EducationMode::Settings);
                }
                
                ImGui::Separator();
                
                ImGui::MenuItem("Adaptive Learning", nullptr, &settings_.enable_adaptive_learning);
                ImGui::MenuItem("Spaced Repetition", nullptr, &settings_.enable_spaced_repetition);
                ImGui::MenuItem("Daily Reminders", nullptr, &settings_.enable_daily_reminders);
                ImGui::MenuItem("Achievement Notifications", nullptr, &settings_.enable_achievement_notifications);
                
                ImGui::EndMenu();
            }
            
            if (ImGui::BeginMenu("Help")) {
                if (ImGui::MenuItem("Getting Started")) {
                    // Navigate to getting started tutorial
                    navigate_to_tutorial("getting_started");
                }
                if (ImGui::MenuItem("Learning Tips")) {
                    // Show learning tips dialog
                }
                if (ImGui::MenuItem("FAQ")) {
                    // Show FAQ
                }
                
                ImGui::EndMenu();
            }
            
            ImGui::EndMenuBar();
        }
        
        // Learner selection (top-right corner)
        ImGui::SameLine(ImGui::GetWindowWidth() - 200);
        ImGui::Text("Learner: ");
        ImGui::SameLine();
        if (ImGui::BeginCombo("##learner", current_learner_id_.c_str())) {
            // In a real implementation, this would list available learner profiles
            if (ImGui::Selectable("default_learner", current_learner_id_ == "default_learner")) {
                set_current_learner("default_learner");
            }
            if (ImGui::Selectable("guest_user", current_learner_id_ == "guest_user")) {
                set_current_learner("guest_user");
            }
            ImGui::EndCombo();
        }
        
        // Mode-specific content
        switch (current_mode_) {
            case EducationMode::Dashboard:
                render_dashboard();
                break;
            case EducationMode::TutorialBrowser:
                render_tutorial_browser();
                break;
            case EducationMode::QuizCenter:
                render_quiz_center();
                break;
            case EducationMode::ProgressAnalysis:
                render_progress_analysis();
                break;
            case EducationMode::Achievements:
                render_achievements();
                break;
            case EducationMode::Settings:
                render_settings();
                break;
            case EducationMode::LearningPath:
                render_learning_path();
                break;
            case EducationMode::StudyGroups:
                render_study_groups();
                break;
        }
        
        // Status bar
        ImGui::Separator();
        ImGui::Text("Learning Time Today: %s | Overall Progress: %.1f%% | Achievements: %u | Level: %s",
                   format_learning_time(dashboard_.today_learning_minutes).c_str(),
                   calculate_overall_progress() * 100.0f,
                   get_total_achievements_unlocked(),
                   current_progress_.current_rank.empty() ? "Beginner" : current_progress_.current_rank.c_str());
    }
    ImGui::End();
}

void EducationalFeaturesPanel::update(f64 delta_time) {
    if (!visible_) return;
    
    // Update learning session time
    dashboard_.today_learning_minutes += static_cast<f32>(delta_time / 60.0);
    
    // Update progress tracking
    last_progress_update_ += delta_time;
    if (last_progress_update_ >= 1.0 / PROGRESS_UPDATE_FREQUENCY) {
        calculate_mastery_levels();
        update_adaptive_parameters();
        last_progress_update_ = 0.0;
    }
    
    // Check for achievement unlocks
    last_achievement_check_ += delta_time;
    if (last_achievement_check_ >= 1.0 / ACHIEVEMENT_CHECK_FREQUENCY) {
        check_achievement_unlocks();
        last_achievement_check_ = 0.0;
    }
    
    // Auto-save progress
    last_auto_save_ += delta_time;
    if (last_auto_save_ >= AUTO_SAVE_INTERVAL) {
        save_progress_data();
        last_auto_save_ = 0.0;
    }
}

bool EducationalFeaturesPanel::wants_keyboard_capture() const {
    return has_active_quiz() && ImGui::IsWindowFocused();
}

bool EducationalFeaturesPanel::wants_mouse_capture() const {
    return ImGui::IsWindowHovered() || ImGui::IsWindowFocused();
}

void EducationalFeaturesPanel::render_dashboard() {
    ImGui::Text("🏠 Learning Dashboard");
    ImGui::Separator();
    
    // Top row - Quick stats
    render_quick_stats();
    
    ImGui::Separator();
    
    // Main content area - two columns
    ImGui::Columns(2, "##dashboard_layout", true);
    
    // Left column
    ImGui::Text("📈 Your Progress");
    render_daily_goals();
    
    ImGui::Spacing();
    render_learning_streak();
    
    ImGui::Spacing();
    render_recent_activity();
    
    ImGui::NextColumn();
    
    // Right column
    ImGui::Text("🎯 Recommended Activities");
    render_recommended_activities();
    
    ImGui::Spacing();
    render_motivational_elements();
    
    ImGui::Columns(1);
}

void EducationalFeaturesPanel::render_tutorial_browser() {
    ImGui::Text("📚 Tutorial Browser");
    ImGui::Separator();
    
    if (!tutorial_manager_) {
        ImGui::TextDisabled("Tutorial manager not available");
        return;
    }
    
    // Tutorial categories
    const std::vector<std::pair<learning::TutorialCategory, std::string>> categories = {
        {learning::TutorialCategory::BasicConcepts, "🎯 Basic Concepts"},
        {learning::TutorialCategory::EntityManagement, "🔧 Entity Management"},
        {learning::TutorialCategory::ComponentSystems, "⚙️ Component Systems"},
        {learning::TutorialCategory::SystemDesign, "🏗️ System Design"},
        {learning::TutorialCategory::MemoryOptimization, "💾 Memory Optimization"},
        {learning::TutorialCategory::AdvancedPatterns, "🚀 Advanced Patterns"}
    };
    
    static learning::TutorialCategory selected_category = learning::TutorialCategory::BasicConcepts;
    
    // Category tabs
    for (usize i = 0; i < categories.size(); ++i) {
        if (i > 0) ImGui::SameLine();
        
        const auto& [category, label] = categories[i];
        bool is_selected = (selected_category == category);
        
        if (is_selected) {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.7f, 1.0f, 1.0f));
        }
        
        if (ImGui::Button(label.c_str())) {
            selected_category = category;
        }
        
        if (is_selected) {
            ImGui::PopStyleColor();
        }
    }
    
    ImGui::Separator();
    
    // Tutorial list for selected category
    auto tutorials = tutorial_manager_->get_tutorials_by_category(selected_category);
    
    ImGui::BeginChild("##tutorial_list", ImVec2(0, 400), true);
    
    for (auto* tutorial : tutorials) {
        if (!tutorial) continue;
        
        // Tutorial card
        ImGui::PushID(tutorial->id().c_str());
        
        bool is_completed = current_progress_.topic_mastery[tutorial->id()] >= MASTERY_THRESHOLD;
        ImVec4 title_color = is_completed ? ImVec4(0.2f, 0.8f, 0.3f, 1.0f) : ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
        
        ImGui::TextColored(title_color, "%s %s", is_completed ? "✅" : "📖", tutorial->title().c_str());
        
        ImGui::Text("Difficulty: %s | Steps: %zu", 
                   get_difficulty_display_name(tutorial->difficulty()).c_str(),
                   tutorial->total_steps());
        
        ImGui::Text("%s", tutorial->description().c_str());
        
        // Progress bar if started
        auto mastery_it = current_progress_.topic_mastery.find(tutorial->id());
        if (mastery_it != current_progress_.topic_mastery.end() && mastery_it->second > 0.0f) {
            ImGui::ProgressBar(mastery_it->second, ImVec2(200, 0), 
                              (std::to_string(static_cast<int>(mastery_it->second * 100)) + "% mastered").c_str());
        }
        
        if (ImGui::Button(is_completed ? "Review" : "Start Tutorial")) {
            navigate_to_tutorial(tutorial->id());
        }
        
        ImGui::Separator();
        ImGui::PopID();
    }
    
    ImGui::EndChild();
    
    // Tutorial statistics
    ImGui::Separator();
    ImGui::Text("📊 Tutorial Statistics:");
    ImGui::Text("Total Available: %zu | Completed: %u | In Progress: %u", 
               tutorials.size(), 
               current_progress_.tutorials_completed, 
               static_cast<u32>(current_progress_.topic_mastery.size()) - current_progress_.tutorials_completed);
}

void EducationalFeaturesPanel::render_quiz_center() {
    ImGui::Text("🧠 Quiz Center");
    ImGui::Separator();
    
    // Check if there's an active quiz
    if (has_active_quiz()) {
        render_active_quiz();
        return;
    }
    
    // Quiz selection interface
    render_quiz_selection();
    
    ImGui::Separator();
    
    // Quiz history
    render_quiz_history();
}

void EducationalFeaturesPanel::render_progress_analysis() {
    ImGui::Text("📊 Learning Progress Analysis");
    ImGui::Separator();
    
    // Time range selector
    const char* time_ranges[] = {"Last Week", "Last Month", "Last Quarter", "All Time", "Custom"};
    int current_range = static_cast<int>(progress_analysis_.selected_time_range);
    
    ImGui::Text("Time Range: ");
    ImGui::SameLine();
    if (ImGui::Combo("##time_range", &current_range, time_ranges, 5)) {
        progress_analysis_.selected_time_range = static_cast<ProgressAnalysisState::TimeRange>(current_range);
    }
    
    ImGui::Separator();
    
    // Progress overview
    render_progress_overview();
    
    ImGui::Separator();
    
    // Detailed analysis tabs
    ImGui::BeginTabBar("##analysis_tabs");
    
    if (ImGui::BeginTabItem("Overall Progress")) {
        render_mastery_heatmap();
        ImGui::Spacing();
        render_learning_velocity_chart();
        ImGui::EndTabItem();
    }
    
    if (ImGui::BeginTabItem("Topic Breakdown")) {
        render_topic_breakdown();
        ImGui::EndTabItem();
    }
    
    if (ImGui::BeginTabItem("Time Analysis")) {
        render_time_spent_analysis();
        ImGui::EndTabItem();
    }
    
    if (ImGui::BeginTabItem("Difficulty Progression")) {
        render_difficulty_progression();
        ImGui::EndTabItem();
    }
    
    ImGui::EndTabBar();
}

void EducationalFeaturesPanel::render_achievements() {
    ImGui::Text("🏆 Achievements & Badges");
    ImGui::Separator();
    
    // Achievement stats
    u32 total_achievements = 10; // Would get from achievement system
    u32 unlocked_achievements = get_total_achievements_unlocked();
    f32 completion_percentage = static_cast<f32>(unlocked_achievements) / static_cast<f32>(total_achievements);
    
    ImGui::Text("Achievement Progress: %u / %u (%.1f%%)", 
               unlocked_achievements, total_achievements, completion_percentage * 100.0f);
    ImGui::ProgressBar(completion_percentage, ImVec2(-1.0f, 0.0f));
    
    ImGui::Text("Total Achievement Points: %u", current_progress_.total_achievement_points);
    
    ImGui::Separator();
    
    // Achievement gallery
    render_achievement_gallery();
    
    ImGui::Separator();
    
    // Achievement progress
    render_achievement_progress();
}

void EducationalFeaturesPanel::render_settings() {
    ImGui::Text("⚙️ Educational Preferences");
    ImGui::Separator();
    
    // Learning preferences
    ImGui::Text("Learning Preferences:");
    
    const char* difficulty_items[] = {"Novice", "Beginner", "Intermediate", "Advanced", "Expert", "Adaptive"};
    int current_difficulty = static_cast<int>(settings_.preferred_difficulty);
    if (ImGui::Combo("Preferred Difficulty", &current_difficulty, difficulty_items, 6)) {
        settings_.preferred_difficulty = static_cast<AdaptiveDifficulty>(current_difficulty);
    }
    
    const char* style_items[] = {"Visual", "Auditory", "Kinesthetic", "Reading", "Mixed", "Adaptive"};
    int current_style = static_cast<int>(settings_.preferred_style);
    if (ImGui::Combo("Learning Style", &current_style, style_items, 6)) {
        settings_.preferred_style = static_cast<LearningStyle>(current_style);
    }
    
    ImGui::Checkbox("Enable Adaptive Learning", &settings_.enable_adaptive_learning);
    ImGui::Checkbox("Enable Spaced Repetition", &settings_.enable_spaced_repetition);
    
    ImGui::Separator();
    
    // Notification settings
    ImGui::Text("Notifications & Reminders:");
    ImGui::Checkbox("Daily Learning Reminders", &settings_.enable_daily_reminders);
    
    if (settings_.enable_daily_reminders) {
        int reminder_hour = static_cast<int>(settings_.reminder_hour);
        if (ImGui::SliderInt("Reminder Time (24h)", &reminder_hour, 0, 23)) {
            settings_.reminder_hour = static_cast<u32>(reminder_hour);
        }
    }
    
    ImGui::Checkbox("Achievement Notifications", &settings_.enable_achievement_notifications);
    ImGui::Checkbox("Progress Alerts", &settings_.enable_progress_alerts);
    
    ImGui::Separator();
    
    // Accessibility settings
    ImGui::Text("Accessibility:");
    ImGui::Checkbox("High Contrast Mode", &settings_.high_contrast_mode);
    ImGui::Checkbox("Large Text Mode", &settings_.large_text_mode);
    ImGui::Checkbox("Screen Reader Support", &settings_.screen_reader_support);
    ImGui::Checkbox("Reduced Motion", &settings_.reduced_motion);
    
    ImGui::SliderFloat("UI Scale", &settings_.ui_scale, 0.8f, 2.0f);
    
    ImGui::Separator();
    
    // Privacy settings
    ImGui::Text("Privacy:");
    ImGui::Checkbox("Share Progress with Instructors", &settings_.share_progress_with_instructors);
    ImGui::Checkbox("Allow Anonymous Analytics", &settings_.allow_anonymous_analytics);
    ImGui::Checkbox("Enable Peer Comparison", &settings_.enable_peer_comparison);
    
    ImGui::Separator();
    
    // Action buttons
    if (ImGui::Button("Apply Settings")) {
        apply_education_settings(settings_);
        LOG_INFO("Applied educational settings");
    }
    
    ImGui::SameLine();
    if (ImGui::Button("Reset to Defaults")) {
        reset_settings_to_defaults();
    }
    
    ImGui::SameLine();
    if (ImGui::Button("Calibrate Learning Parameters")) {
        calibrate_learning_parameters();
    }
}

void EducationalFeaturesPanel::render_learning_path() {
    ImGui::Text("🗺️ Personalized Learning Path");
    ImGui::Separator();
    
    render_path_overview();
    
    ImGui::Separator();
    
    // Two-column layout
    ImGui::Columns(2, "##path_layout", true);
    
    // Left column - current progress
    ImGui::Text("📍 Current Progress");
    render_next_steps();
    
    ImGui::NextColumn();
    
    // Right column - prerequisites and recommendations
    ImGui::Text("🎯 Recommendations");
    render_prerequisite_checker();
    
    ImGui::Columns(1);
    
    ImGui::Separator();
    
    // Custom path builder
    render_custom_path_builder();
}

void EducationalFeaturesPanel::render_study_groups() {
    ImGui::Text("👥 Study Groups (Coming Soon)");
    ImGui::Separator();
    
    ImGui::TextDisabled("Collaborative learning features will be available in a future update.");
    ImGui::TextWrapped("Features will include:");
    ImGui::BulletText("Join study groups with other learners");
    ImGui::BulletText("Share progress and achievements");
    ImGui::BulletText("Collaborative problem solving");
    ImGui::BulletText("Peer tutoring system");
    ImGui::BulletText("Group challenges and competitions");
}

// Dashboard component implementations

void EducationalFeaturesPanel::render_quick_stats() {
    // Four-card layout
    f32 card_width = (ImGui::GetContentRegionAvail().x - 30.0f) / 4.0f;
    
    // Learning time today
    ImGui::BeginChild("##stat1", ImVec2(card_width, DASHBOARD_CARD_HEIGHT), true, ImGuiWindowFlags_NoScrollbar);
    ImGui::Text("⏱️ Today's Learning");
    ImGui::Separator();
    ImGui::Text("%.0f minutes", dashboard_.today_learning_minutes);
    f32 goal_progress = dashboard_.today_learning_minutes / dashboard_.daily_learning_goal_minutes;
    ImGui::ProgressBar(std::min(goal_progress, 1.0f), ImVec2(-1, 0), 
                      goal_progress >= 1.0f ? "Goal Achieved!" : "");
    ImGui::EndChild();
    
    ImGui::SameLine();
    
    // Quiz performance this week
    ImGui::BeginChild("##stat2", ImVec2(card_width, DASHBOARD_CARD_HEIGHT), true, ImGuiWindowFlags_NoScrollbar);
    ImGui::Text("🧠 Weekly Quizzes");
    ImGui::Separator();
    ImGui::Text("%u / %u taken", dashboard_.this_week_quizzes_taken, dashboard_.weekly_quiz_goal);
    ImGui::Text("Avg: %.1f%%", current_progress_.overall_quiz_average * 100.0f);
    ImGui::EndChild();
    
    ImGui::SameLine();
    
    // Current streak
    ImGui::BeginChild("##stat3", ImVec2(card_width, DASHBOARD_CARD_HEIGHT), true, ImGuiWindowFlags_NoScrollbar);
    ImGui::Text("🔥 Learning Streak");
    ImGui::Separator();
    ImGui::Text("%.0f days", dashboard_.current_streak_days);
    ImGui::Text("Keep it up!");
    ImGui::EndChild();
    
    ImGui::SameLine();
    
    // Overall progress
    ImGui::BeginChild("##stat4", ImVec2(card_width, DASHBOARD_CARD_HEIGHT), true, ImGuiWindowFlags_NoScrollbar);
    ImGui::Text("📈 Overall Progress");
    ImGui::Separator();
    f32 overall = calculate_overall_progress();
    ImGui::Text("%.1f%% Complete", overall * 100.0f);
    ImGui::ProgressBar(overall, ImVec2(-1, 0));
    ImGui::EndChild();
}

void EducationalFeaturesPanel::render_daily_goals() {
    ImGui::Text("🎯 Daily Goals");
    
    // Learning time goal
    f32 time_progress = dashboard_.today_learning_minutes / dashboard_.daily_learning_goal_minutes;
    ImGui::Text("Learning Time: %s / %s", 
               format_learning_time(dashboard_.today_learning_minutes).c_str(),
               format_learning_time(dashboard_.daily_learning_goal_minutes).c_str());
    ImGui::ProgressBar(std::min(time_progress, 1.0f), ImVec2(-1, 0));
    
    if (time_progress >= 1.0f) {
        ImGui::TextColored(ImVec4(0.2f, 0.8f, 0.3f, 1.0f), "✅ Daily learning goal achieved!");
    }
    
    // Quiz goal (if any)
    if (dashboard_.weekly_quiz_goal > 0) {
        ImGui::Text("Weekly Quiz Goal: %u / %u", 
                   dashboard_.this_week_quizzes_taken, dashboard_.weekly_quiz_goal);
        f32 quiz_progress = static_cast<f32>(dashboard_.this_week_quizzes_taken) / 
                           static_cast<f32>(dashboard_.weekly_quiz_goal);
        ImGui::ProgressBar(std::min(quiz_progress, 1.0f), ImVec2(-1, 0));
    }
}

void EducationalFeaturesPanel::render_recent_activity() {
    ImGui::Text("📝 Recent Activity");
    
    ImGui::BeginChild("##recent_activity", ImVec2(0, 150), true);
    
    // Show recent tutorials
    for (const std::string& tutorial : dashboard_.recent_tutorials) {
        ImGui::Text("📖 Completed: %s", tutorial.c_str());
    }
    
    // Show recent quiz results
    for (const std::string& result : dashboard_.recent_quiz_results) {
        ImGui::Text("🧠 Quiz: %s", result.c_str());
    }
    
    // Show recent achievements
    for (const std::string& achievement : dashboard_.recent_achievements) {
        ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "🏆 Unlocked: %s", achievement.c_str());
    }
    
    if (dashboard_.recent_tutorials.empty() && dashboard_.recent_quiz_results.empty() && 
        dashboard_.recent_achievements.empty()) {
        ImGui::TextDisabled("No recent activity. Start learning to see your progress here!");
    }
    
    ImGui::EndChild();
}

void EducationalFeaturesPanel::render_recommended_activities() {
    ImGui::Text("Recommended for you:");
    
    if (!dashboard_.next_recommended_activity.empty()) {
        ImGui::Text("🎯 Next: %s", dashboard_.next_recommended_activity.c_str());
        
        if (ImGui::Button("Start Recommended Activity")) {
            // Navigate to recommended activity
            LOG_INFO("Starting recommended activity: " + dashboard_.next_recommended_activity);
        }
    }
    
    // Show struggling topics that need review
    auto struggling_topics = get_struggling_topics();
    if (!struggling_topics.empty()) {
        ImGui::Text("📚 Topics to Review:");
        for (const std::string& topic : struggling_topics) {
            ImGui::BulletText("%s", topic.c_str());
        }
    }
    
    // Show recommended study topics
    auto study_topics = get_recommended_study_topics();
    if (!study_topics.empty()) {
        ImGui::Text("📖 Suggested Study:");
        for (const std::string& topic : study_topics) {
            ImGui::BulletText("%s", topic.c_str());
        }
    }
}

void EducationalFeaturesPanel::render_motivational_elements() {
    ImGui::Text("💡 Daily Tip");
    ImGui::TextWrapped("%s", dashboard_.daily_tip.c_str());
    
    ImGui::Spacing();
    
    ImGui::Text("✨ Motivation");
    ImGui::TextWrapped("\"%s\"", dashboard_.motivational_quote.c_str());
}

void EducationalFeaturesPanel::render_learning_streak() {
    ImGui::Text("🔥 Learning Streak: %.0f days", dashboard_.current_streak_days);
    
    if (dashboard_.current_streak_days > 0) {
        ImGui::Text("Great consistency! Keep up the daily learning habit.");
    } else {
        ImGui::Text("Start your learning streak today!");
    }
}

// Quiz center component implementations

void EducationalFeaturesPanel::render_quiz_selection() {
    ImGui::Text("Available Quizzes:");
    
    auto quiz_banks = quiz_system_->get_available_quiz_banks();
    
    for (const std::string& bank_name : quiz_banks) {
        ImGui::PushID(bank_name.c_str());
        
        ImGui::Text("📚 %s", bank_name.c_str());
        
        // Quiz info would be displayed here
        ImGui::Text("Questions: 10 | Estimated time: 5-10 minutes");
        
        if (ImGui::Button("Take Quiz")) {
            start_quiz(bank_name);
        }
        
        ImGui::SameLine();
        if (ImGui::Button("Practice Mode")) {
            // Start practice mode (no scoring)
            start_quiz(bank_name);
        }
        
        ImGui::Separator();
        ImGui::PopID();
    }
    
    if (quiz_banks.empty()) {
        ImGui::TextDisabled("No quizzes available. Check back later!");
    }
}

void EducationalFeaturesPanel::render_active_quiz() {
    if (!quiz_center_.current_quiz_session) {
        ImGui::Text("No active quiz session");
        return;
    }
    
    auto& session = *quiz_center_.current_quiz_session;
    
    ImGui::Text("📝 Quiz: %s", session.quiz_name.c_str());
    ImGui::Separator();
    
    // Progress
    ImGui::Text("Question %zu of %zu", 
               session.current_question_index + 1, 
               session.questions.size());
    
    f32 progress = static_cast<f32>(session.current_question_index) / static_cast<f32>(session.questions.size());
    ImGui::ProgressBar(progress, ImVec2(-1, 0));
    
    // Current question
    if (session.current_question_index < session.questions.size()) {
        const auto& question = session.questions[session.current_question_index];
        std::string user_answer = session.current_question_index < session.user_answers.size() ? 
                                 session.user_answers[session.current_question_index] : "";
        
        render_quiz_question(question, user_answer);
        
        // Navigation buttons
        if (ImGui::Button("Submit Answer")) {
            submit_quiz_answer("user_selected_answer"); // Placeholder
        }
        
        if (session.current_question_index > 0) {
            ImGui::SameLine();
            if (ImGui::Button("Previous")) {
                // Go to previous question
                if (session.current_question_index > 0) {
                    session.current_question_index--;
                }
            }
        }
        
        if (session.current_question_index < session.questions.size() - 1) {
            ImGui::SameLine();
            if (ImGui::Button("Next")) {
                // Go to next question (if answered)
                session.current_question_index++;
            }
        }
    } else {
        // Quiz completed
        if (ImGui::Button("Finish Quiz")) {
            finish_quiz();
        }
    }
    
    ImGui::SameLine();
    if (ImGui::Button("Exit Quiz")) {
        // Exit current quiz
        quiz_center_.current_quiz_session.reset();
    }
}

void EducationalFeaturesPanel::render_quiz_results() {
    ImGui::Text("📊 Quiz Results");
    ImGui::Separator();
    
    if (!quiz_center_.current_quiz_session) {
        ImGui::TextDisabled("No quiz results to display");
        return;
    }
    
    auto& session = *quiz_center_.current_quiz_session;
    
    ImGui::Text("Quiz: %s", session.quiz_name.c_str());
    ImGui::Text("Score: %u / %u (%.1f%%)", 
               session.earned_points, session.total_points, 
               session.percentage_score * 100.0f);
    ImGui::Text("Grade: %s", session.grade.c_str());
    ImGui::Text("Time Taken: %.1f minutes", session.total_time_seconds / 60.0);
    
    // Results breakdown would go here
    ImGui::Separator();
    
    if (ImGui::Button("Review Answers")) {
        review_quiz_results();
    }
    
    if (session.max_attempts > session.current_attempt && !session.is_passing) {
        ImGui::SameLine();
        if (ImGui::Button("Retake Quiz")) {
            retake_quiz();
        }
    }
    
    ImGui::SameLine();
    if (ImGui::Button("Take Another Quiz")) {
        quiz_center_.current_quiz_session.reset();
    }
}

void EducationalFeaturesPanel::render_quiz_history() {
    ImGui::Text("📈 Quiz History");
    
    ImGui::BeginChild("##quiz_history", ImVec2(0, 200), true);
    
    // Show completed quizzes
    for (const auto& session : quiz_center_.completed_quizzes) {
        ImGui::Text("📝 %s - %.1f%% (%s)", 
                   session.quiz_name.c_str(),
                   session.percentage_score * 100.0f,
                   session.is_passing ? "PASS" : "FAIL");
    }
    
    if (quiz_center_.completed_quizzes.empty()) {
        ImGui::TextDisabled("No quiz history yet. Take your first quiz!");
    }
    
    ImGui::EndChild();
}

// Progress analysis component implementations

void EducationalFeaturesPanel::render_progress_overview() {
    ImGui::Text("Progress Summary:");
    
    ImGui::Text("Total Learning Time: %.1f hours", current_progress_.total_learning_time_hours);
    ImGui::Text("Sessions Completed: %u", current_progress_.total_sessions);
    ImGui::Text("Average Quiz Score: %.1f%%", current_progress_.overall_quiz_average * 100.0f);
    ImGui::Text("Topics Mastered: %zu", current_progress_.topic_mastery.size());
    
    // Overall progress bar
    f32 overall_progress = calculate_overall_progress();
    ImGui::Text("Overall Progress: %.1f%%", overall_progress * 100.0f);
    ImGui::ProgressBar(overall_progress, ImVec2(-1, 0));
}

void EducationalFeaturesPanel::render_mastery_heatmap() {
    ImGui::Text("📊 Topic Mastery Levels");
    
    // Simple mastery display
    for (const auto& [topic, mastery] : current_progress_.topic_mastery) {
        render_mastery_bar(topic, mastery, 300.0f);
    }
    
    if (current_progress_.topic_mastery.empty()) {
        ImGui::TextDisabled("No mastery data available yet. Complete some tutorials to see progress!");
    }
}

void EducationalFeaturesPanel::render_learning_velocity_chart() {
    ImGui::Text("📈 Learning Velocity");
    
    ImGui::Text("Current velocity: %.2f concepts/hour", current_progress_.current_learning_velocity);
    
    // Would show velocity chart here
    ImGui::TextDisabled("Velocity chart visualization coming soon");
}

void EducationalFeaturesPanel::render_topic_breakdown() {
    ImGui::Text("📚 Topic Breakdown");
    
    for (const auto& [topic, time_spent] : current_progress_.topic_time_spent) {
        ImGui::Text("%s: %u minutes", topic.c_str(), time_spent);
        
        // Show mastery level for this topic
        auto mastery_it = current_progress_.topic_mastery.find(topic);
        if (mastery_it != current_progress_.topic_mastery.end()) {
            ImGui::SameLine();
            ImGui::Text("(%.1f%% mastered)", mastery_it->second * 100.0f);
        }
    }
}

// Achievement component implementations

void EducationalFeaturesPanel::render_achievement_gallery() {
    ImGui::Text("Achievement Gallery:");
    
    // Mock achievements for display
    std::vector<std::pair<std::string, bool>> mock_achievements = {
        {"First Steps", true},
        {"Quiz Master", false},
        {"Perfectionist", false},
        {"Speed Learner", false},
        {"Consistent Learner", true},
        {"Helper", false}
    };
    
    ImGui::BeginChild("##achievement_gallery", ImVec2(0, 200), true);
    
    for (const auto& [name, unlocked] : mock_achievements) {
        LearningAchievement achievement;
        achievement.name = name;
        achievement.description = "Achievement description goes here";
        render_achievement_card(achievement, unlocked);
    }
    
    ImGui::EndChild();
}

void EducationalFeaturesPanel::render_achievement_progress() {
    ImGui::Text("Achievement Progress:");
    
    // Show progress towards locked achievements
    ImGui::Text("🎯 Quiz Master: %u / 10 quizzes passed with 80%+", current_progress_.quizzes_passed);
    ImGui::ProgressBar(static_cast<f32>(current_progress_.quizzes_passed) / 10.0f, ImVec2(-1, 0));
    
    ImGui::Text("📚 Scholar: %u / 20 tutorials completed", current_progress_.tutorials_completed);
    ImGui::ProgressBar(static_cast<f32>(current_progress_.tutorials_completed) / 20.0f, ImVec2(-1, 0));
}

// Helper rendering methods

void EducationalFeaturesPanel::render_mastery_bar(const std::string& topic, f32 mastery_level, f32 width) {
    ImGui::Text("%s", topic.c_str());
    
    // Color-coded progress bar
    ImVec4 color = mastery_level >= MASTERY_THRESHOLD ? 
                   ImVec4(0.2f, 0.8f, 0.3f, 1.0f) :  // Green for mastered
                   mastery_level >= 0.5f ?
                   ImVec4(1.0f, 0.8f, 0.2f, 1.0f) :  // Yellow for moderate
                   ImVec4(1.0f, 0.3f, 0.3f, 1.0f);   // Red for struggling
    
    ImGui::PushStyleColor(ImGuiCol_PlotHistogram, color);
    ImGui::ProgressBar(mastery_level, ImVec2(width, 0), 
                      (format_mastery_level(mastery_level) + " mastered").c_str());
    ImGui::PopStyleColor();
}

void EducationalFeaturesPanel::render_achievement_card(const LearningAchievement& achievement, bool unlocked) {
    ImVec4 card_color = unlocked ? ImVec4(1.0f, 0.8f, 0.2f, 0.3f) : ImVec4(0.3f, 0.3f, 0.3f, 0.3f);
    ImGui::PushStyleColor(ImGuiCol_ChildBg, card_color);
    
    ImGui::BeginChild(achievement.id.c_str(), ImVec2(ACHIEVEMENT_CARD_SIZE, ACHIEVEMENT_CARD_SIZE), true);
    
    ImGui::Text("%s", unlocked ? "🏆" : "🔒");
    ImGui::Text("%s", achievement.name.c_str());
    
    if (ImGui::IsItemHovered()) {
        ImGui::BeginTooltip();
        ImGui::Text("%s", achievement.description.c_str());
        if (!unlocked) {
            ImGui::Text("Keep learning to unlock this achievement!");
        }
        ImGui::EndTooltip();
    }
    
    ImGui::EndChild();
    ImGui::PopStyleColor();
    
    ImGui::SameLine();
}

void EducationalFeaturesPanel::render_quiz_question(const QuizQuestion& question, const std::string& user_answer) {
    ImGui::BeginChild("##quiz_question", ImVec2(0, QUIZ_QUESTION_MIN_HEIGHT), true);
    
    ImGui::Text("Question: %s", question.question_text.c_str());
    
    switch (question.type) {
        case QuestionType::MultipleChoice:
        case QuestionType::MultipleSelect:
            for (usize i = 0; i < question.options.size(); ++i) {
                bool selected = user_answer == std::to_string(i);
                if (ImGui::RadioButton(question.options[i].c_str(), selected)) {
                    // Update user answer
                }
            }
            break;
            
        case QuestionType::TrueFalse:
            if (ImGui::RadioButton("True", user_answer == "true")) {
                // Set answer to true
            }
            ImGui::SameLine();
            if (ImGui::RadioButton("False", user_answer == "false")) {
                // Set answer to false
            }
            break;
            
        case QuestionType::FillInBlank:
        case QuestionType::ShortAnswer:
            {
                char answer_buf[256];
                strncpy(answer_buf, user_answer.c_str(), sizeof(answer_buf));
                if (ImGui::InputText("Answer", answer_buf, sizeof(answer_buf))) {
                    // Update user answer
                }
            }
            break;
            
        default:
            ImGui::TextDisabled("Question type not yet implemented in UI");
            break;
    }
    
    ImGui::EndChild();
}

// Control methods

void EducationalFeaturesPanel::set_education_mode(EducationMode mode) {
    current_mode_ = mode;
    LOG_INFO("Switched to education mode: " + std::to_string(static_cast<int>(mode)));
}

void EducationalFeaturesPanel::set_current_learner(const std::string& learner_id) {
    if (current_learner_id_ != learner_id) {
        // Save current progress
        save_progress_data();
        
        // Switch learner
        current_learner_id_ = learner_id;
        current_progress_ = DetailedLearningProgress(learner_id);
        
        // Load new learner's progress
        load_progress_data();
        
        LOG_INFO("Switched to learner: " + learner_id);
    }
}

void EducationalFeaturesPanel::start_quiz(const std::string& quiz_name) {
    quiz_center_.current_quiz_session = std::make_unique<QuizSession>(
        quiz_system_->create_session("session_" + std::to_string(std::time(nullptr)), 
                                    quiz_name, current_learner_id_));
    
    LOG_INFO("Started quiz: " + quiz_name);
}

void EducationalFeaturesPanel::register_achievement(const LearningAchievement& achievement) {
    // In a real implementation, this would add to achievement system
    LOG_INFO("Registered achievement: " + achievement.name);
}

// Progress tracking methods

void EducationalFeaturesPanel::record_learning_activity(LearningActivityType type, const std::string& content_id, 
                                                       f64 duration_minutes, f32 success_score) {
    // Update progress tracking
    current_progress_.total_learning_time_hours += duration_minutes / 60.0;
    current_progress_.total_sessions++;
    
    // Update topic-specific progress
    current_progress_.topic_time_spent[content_id] += static_cast<u32>(duration_minutes);
    
    // Update mastery level based on success
    if (current_progress_.topic_mastery.find(content_id) == current_progress_.topic_mastery.end()) {
        current_progress_.topic_mastery[content_id] = 0.0f;
    }
    
    // Simple mastery calculation (would be more sophisticated in practice)
    f32 current_mastery = current_progress_.topic_mastery[content_id];
    f32 learning_rate = 0.1f; // How quickly mastery increases
    current_progress_.topic_mastery[content_id] = std::min(1.0f, 
        current_mastery + success_score * learning_rate);
    
    // Update last session time
    current_progress_.last_session = std::chrono::steady_clock::now();
    
    LOG_INFO("Recorded learning activity: " + content_id + " (score: " + std::to_string(success_score) + ")");
}

void EducationalFeaturesPanel::check_achievement_unlocks() {
    // Check if any achievements should be unlocked
    // This is a simplified version - real implementation would be more comprehensive
    
    if (current_progress_.tutorials_completed >= 1 && 
        std::find(current_progress_.unlocked_achievements.begin(), 
                 current_progress_.unlocked_achievements.end(), 
                 "first_tutorial") == current_progress_.unlocked_achievements.end()) {
        unlock_achievement("first_tutorial");
    }
    
    if (current_progress_.quizzes_passed >= 10 && 
        current_progress_.overall_quiz_average >= 0.8f &&
        std::find(current_progress_.unlocked_achievements.begin(), 
                 current_progress_.unlocked_achievements.end(), 
                 "quiz_master") == current_progress_.unlocked_achievements.end()) {
        unlock_achievement("quiz_master");
    }
}

void EducationalFeaturesPanel::unlock_achievement(const std::string& achievement_id) {
    current_progress_.unlocked_achievements.push_back(achievement_id);
    current_progress_.total_achievement_points += 10; // Default points
    
    // Add to recent achievements for dashboard
    dashboard_.recent_achievements.push_back(achievement_id);
    if (dashboard_.recent_achievements.size() > MAX_RECENT_ACTIVITIES) {
        dashboard_.recent_achievements.erase(dashboard_.recent_achievements.begin());
    }
    
    on_achievement_unlocked(achievement_id);
    LOG_INFO("Achievement unlocked: " + achievement_id);
}

f32 EducationalFeaturesPanel::calculate_overall_progress() const {
    if (current_progress_.topic_mastery.empty()) {
        return 0.0f;
    }
    
    f32 total_mastery = 0.0f;
    for (const auto& [topic, mastery] : current_progress_.topic_mastery) {
        total_mastery += mastery;
    }
    
    return total_mastery / static_cast<f32>(current_progress_.topic_mastery.size());
}

std::vector<std::string> EducationalFeaturesPanel::get_struggling_topics() const {
    std::vector<std::string> struggling;
    
    for (const auto& [topic, mastery] : current_progress_.topic_mastery) {
        if (mastery < STRUGGLING_THRESHOLD) {
            struggling.push_back(topic);
        }
    }
    
    return struggling;
}

std::vector<std::string> EducationalFeaturesPanel::get_recommended_study_topics() const {
    // Simple recommendation: topics with moderate mastery that could be improved
    std::vector<std::string> recommendations;
    
    for (const auto& [topic, mastery] : current_progress_.topic_mastery) {
        if (mastery >= STRUGGLING_THRESHOLD && mastery < MASTERY_THRESHOLD) {
            recommendations.push_back(topic);
        }
    }
    
    return recommendations;
}

// Utility methods

std::string EducationalFeaturesPanel::format_learning_time(f64 minutes) const {
    if (minutes < 60.0) {
        return std::to_string(static_cast<int>(minutes)) + " min";
    } else {
        int hours = static_cast<int>(minutes / 60.0);
        int mins = static_cast<int>(minutes) % 60;
        return std::to_string(hours) + "h " + std::to_string(mins) + "m";
    }
}

std::string EducationalFeaturesPanel::format_mastery_level(f32 mastery) const {
    return std::to_string(static_cast<int>(mastery * 100)) + "%";
}

std::string EducationalFeaturesPanel::get_difficulty_display_name(AdaptiveDifficulty difficulty) const {
    switch (difficulty) {
        case AdaptiveDifficulty::Novice: return "Novice";
        case AdaptiveDifficulty::Beginner: return "Beginner";
        case AdaptiveDifficulty::Intermediate: return "Intermediate";
        case AdaptiveDifficulty::Advanced: return "Advanced";
        case AdaptiveDifficulty::Expert: return "Expert";
        case AdaptiveDifficulty::Adaptive: return "Adaptive";
        default: return "Unknown";
    }
}

// Event handlers

void EducationalFeaturesPanel::on_tutorial_completed(const std::string& tutorial_id) {
    current_progress_.tutorials_completed++;
    record_learning_activity(LearningActivityType::Tutorial, tutorial_id, 30.0f, 1.0f);
    
    // Add to recent activities
    dashboard_.recent_tutorials.push_back(tutorial_id);
    if (dashboard_.recent_tutorials.size() > MAX_RECENT_ACTIVITIES) {
        dashboard_.recent_tutorials.erase(dashboard_.recent_tutorials.begin());
    }
    
    LOG_INFO("Tutorial completed: " + tutorial_id);
}

void EducationalFeaturesPanel::on_quiz_completed(const QuizSession& session) {
    current_progress_.quizzes_taken++;
    if (session.is_passing) {
        current_progress_.quizzes_passed++;
    }
    
    // Update overall quiz average
    f32 old_average = current_progress_.overall_quiz_average;
    f32 new_average = (old_average * (current_progress_.quizzes_taken - 1) + session.percentage_score) / 
                     static_cast<f32>(current_progress_.quizzes_taken);
    current_progress_.overall_quiz_average = new_average;
    
    record_learning_activity(LearningActivityType::Quiz, session.quiz_name, 
                            session.total_time_seconds / 60.0, session.percentage_score);
    
    // Add to recent activities
    std::string result_text = session.quiz_name + " (" + 
                             std::to_string(static_cast<int>(session.percentage_score * 100)) + "%)";
    dashboard_.recent_quiz_results.push_back(result_text);
    if (dashboard_.recent_quiz_results.size() > MAX_RECENT_ACTIVITIES) {
        dashboard_.recent_quiz_results.erase(dashboard_.recent_quiz_results.begin());
    }
    
    LOG_INFO("Quiz completed: " + session.quiz_name + " with score " + std::to_string(session.percentage_score));
}

void EducationalFeaturesPanel::on_achievement_unlocked(const std::string& achievement_id) {
    // Could trigger notification or animation here
    LOG_INFO("Achievement unlocked notification: " + achievement_id);
}

// Stub implementations for remaining methods
void EducationalFeaturesPanel::navigate_to_tutorial(const std::string& tutorial_id) {
    LOG_INFO("Navigating to tutorial: " + tutorial_id);
}

void EducationalFeaturesPanel::submit_quiz_answer(const std::string& answer) {
    LOG_INFO("Quiz answer submitted: " + answer);
}

void EducationalFeaturesPanel::finish_quiz() {
    if (quiz_center_.current_quiz_session) {
        on_quiz_completed(*quiz_center_.current_quiz_session);
        quiz_center_.completed_quizzes.push_back(*quiz_center_.current_quiz_session);
        quiz_center_.current_quiz_session.reset();
    }
}

void EducationalFeaturesPanel::calculate_mastery_levels() {
    // Would implement sophisticated mastery calculation
}

void EducationalFeaturesPanel::update_adaptive_parameters() {
    // Would update adaptive learning parameters
}

void EducationalFeaturesPanel::save_progress_data() {
    LOG_INFO("Saving progress data for learner: " + current_learner_id_);
}

void EducationalFeaturesPanel::load_progress_data() {
    LOG_INFO("Loading progress data for learner: " + current_learner_id_);
}

void EducationalFeaturesPanel::reset_learning_progress() {
    current_progress_ = DetailedLearningProgress(current_learner_id_);
    LOG_INFO("Reset learning progress for: " + current_learner_id_);
}

void EducationalFeaturesPanel::export_progress_report(const std::string& filename, const std::string& format) {
    LOG_INFO("Exporting progress report to: " + filename + " (format: " + format + ")");
}

void EducationalFeaturesPanel::apply_education_settings(const EducationSettings& settings) {
    settings_ = settings;
    LOG_INFO("Applied educational settings");
}

void EducationalFeaturesPanel::reset_settings_to_defaults() {
    settings_ = EducationSettings();
    LOG_INFO("Reset settings to defaults");
}

void EducationalFeaturesPanel::calibrate_learning_parameters() {
    LOG_INFO("Calibrating learning parameters");
}

// Additional stub implementations for remaining render methods
void EducationalFeaturesPanel::render_path_overview() {
    ImGui::Text("Learning path overview will be implemented here");
}

void EducationalFeaturesPanel::render_next_steps() {
    ImGui::Text("Next steps recommendations will be implemented here");
}

void EducationalFeaturesPanel::render_prerequisite_checker() {
    ImGui::Text("Prerequisite checker will be implemented here");
}

void EducationalFeaturesPanel::render_custom_path_builder() {
    ImGui::Text("Custom path builder will be implemented here");
}

void EducationalFeaturesPanel::render_time_spent_analysis() {
    ImGui::Text("Time spent analysis will be implemented here");
}

void EducationalFeaturesPanel::render_difficulty_progression() {
    ImGui::Text("Difficulty progression analysis will be implemented here");
}

void EducationalFeaturesPanel::render_comparative_analysis() {
    ImGui::Text("Comparative analysis will be implemented here");
}

void EducationalFeaturesPanel::review_quiz_results() {
    LOG_INFO("Reviewing quiz results");
}

void EducationalFeaturesPanel::retake_quiz() {
    LOG_INFO("Retaking quiz");
}

} // namespace ecscope::ui