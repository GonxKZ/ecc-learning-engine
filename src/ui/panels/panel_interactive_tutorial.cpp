#include "panel_interactive_tutorial.hpp"
#include "core/log.hpp"
#include "../imgui_utils.hpp"
#include <imgui.h>
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <cmath>

namespace ecscope::ui {

// InteractiveTutorialPanel Implementation

InteractiveTutorialPanel::InteractiveTutorialPanel(std::shared_ptr<learning::TutorialManager> manager)
    : Panel("Interactive Tutorial", true), tutorial_manager_(manager) {
    
    // Initialize browser state
    update_filtered_tutorials();
    
    // Initialize learner preferences
    load_learner_preferences();
    
    // Initialize progress tracking
    progress_.session_time_ = 0.0;
    progress_.total_attempts_ = 0;
    progress_.successful_validations_ = 0;
    
    LOG_INFO("Interactive Tutorial Panel initialized");
}

void InteractiveTutorialPanel::render() {
    if (!visible_) return;
    
    ImGui::SetNextWindowSize(ImVec2(800, 600), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSizeConstraints(ImVec2(MIN_PANEL_WIDTH, MIN_PANEL_HEIGHT), ImVec2(FLT_MAX, FLT_MAX));
    
    if (ImGui::Begin(name_.c_str(), &visible_, ImGuiWindowFlags_MenuBar)) {
        
        // Menu bar
        if (ImGui::BeginMenuBar()) {
            if (ImGui::BeginMenu("Tutorial")) {
                if (ImGui::MenuItem("Browse Tutorials", nullptr, current_mode_ == PanelMode::TutorialSelection)) {
                    set_panel_mode(PanelMode::TutorialSelection);
                }
                if (ImGui::MenuItem("Resume Tutorial", nullptr, current_mode_ == PanelMode::ActiveTutorial, tutorial_active_)) {
                    set_panel_mode(PanelMode::ActiveTutorial);
                }
                ImGui::Separator();
                if (ImGui::MenuItem("Reset Current Tutorial", nullptr, false, tutorial_active_)) {
                    reset_current_tutorial();
                }
                if (ImGui::MenuItem("Abandon Tutorial", nullptr, false, tutorial_active_)) {
                    abandon_current_tutorial();
                }
                ImGui::EndMenu();
            }
            
            if (ImGui::BeginMenu("View")) {
                if (ImGui::MenuItem("Show Progress", nullptr, current_mode_ == PanelMode::ProgressReview)) {
                    set_panel_mode(PanelMode::ProgressReview);
                }
                if (ImGui::MenuItem("Show Help", nullptr, current_mode_ == PanelMode::HelpSystem)) {
                    set_panel_mode(PanelMode::HelpSystem);
                }
                ImGui::Separator();
                ImGui::MenuItem("Visual Cues", nullptr, &effects_.smooth_transitions_);
                ImGui::MenuItem("Smart Hints", nullptr, &help_system_.smart_hints_enabled_);
                ImGui::MenuItem("Context Help", nullptr, &help_system_.context_help_enabled_);
                ImGui::EndMenu();
            }
            
            if (ImGui::BeginMenu("Settings")) {
                ImGui::MenuItem("High Contrast", nullptr, &learner_.high_contrast_mode_);
                ImGui::MenuItem("Large Text", nullptr, &learner_.large_text_mode_);
                ImGui::MenuItem("Reduce Motion", nullptr, &effects_.reduce_motion_);
                ImGui::Separator();
                ImGui::SliderFloat("UI Scale", &learner_.ui_scale_factor_, 0.8f, 2.0f);
                ImGui::SliderFloat("Animation Speed", &effects_.animation_speed_multiplier_, 0.5f, 2.0f);
                ImGui::EndMenu();
            }
            
            ImGui::EndMenuBar();
        }
        
        // Render current mode
        switch (current_mode_) {
            case PanelMode::TutorialSelection:
                render_tutorial_selection();
                break;
            case PanelMode::ActiveTutorial:
                render_active_tutorial();
                break;
            case PanelMode::StepExecution:
                render_step_execution();
                break;
            case PanelMode::CodeEditor:
                render_code_editor();
                break;
            case PanelMode::QuizMode:
                render_quiz_mode();
                break;
            case PanelMode::ProgressReview:
                render_progress_review();
                break;
            case PanelMode::HelpSystem:
                render_help_system();
                break;
        }
        
        // Render visual effects overlay
        if (effects_.smooth_transitions_) {
            render_highlight_effects();
            render_particle_effects();
        }
        
        // Context help popup
        if (help_system_.context_help_enabled_ && !help_system_.current_help_topic_.empty()) {
            render_context_help();
        }
        
        // Achievement popup
        if (progress_.show_achievement_popup_) {
            ImGui::SetNextWindowPos(ImVec2(ImGui::GetWindowPos().x + ImGui::GetWindowWidth() / 2 - 150, 
                                          ImGui::GetWindowPos().y + 100));
            ImGui::SetNextWindowSize(ImVec2(300, 80));
            if (ImGui::Begin("Achievement Unlocked!", nullptr, 
                           ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | 
                           ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoDocking)) {
                
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.8f, 0.2f, 1.0f));
                ImGui::Text("🏆 %s", progress_.current_achievement_.c_str());
                ImGui::PopStyleColor();
                
                ImGui::ProgressBar(1.0f - (progress_.achievement_popup_timer_ / 3.0f), 
                                  ImVec2(-1.0f, 0.0f), "");
            }
            ImGui::End();
        }
        
    }
    ImGui::End();
}

void InteractiveTutorialPanel::update(f64 delta_time) {
    if (!visible_) return;
    
    // Update session time
    progress_.session_time_ += delta_time;
    
    // Update animations
    if (effects_.smooth_transitions_) {
        update_animations(delta_time);
    }
    
    // Update progress tracking
    update_progress_tracking(delta_time);
    
    // Update adaptive help
    if (help_system_.smart_hints_enabled_) {
        update_adaptive_help();
    }
    
    // Update achievement popup
    if (progress_.show_achievement_popup_) {
        progress_.achievement_popup_timer_ -= static_cast<f32>(delta_time);
        if (progress_.achievement_popup_timer_ <= 0.0f) {
            progress_.show_achievement_popup_ = false;
        }
    }
    
    // Auto-save progress periodically
    static f64 last_save_time = 0.0;
    last_save_time += delta_time;
    if (last_save_time >= 30.0) { // Save every 30 seconds
        save_learner_progress();
        last_save_time = 0.0;
    }
}

bool InteractiveTutorialPanel::wants_keyboard_capture() const {
    return current_mode_ == PanelMode::CodeEditor && code_editor_.is_executing_;
}

bool InteractiveTutorialPanel::wants_mouse_capture() const {
    return ImGui::IsWindowHovered() || ImGui::IsWindowFocused();
}

void InteractiveTutorialPanel::render_tutorial_selection() {
    ImGui::Text("📚 Choose a Tutorial to Begin Your Learning Journey");
    ImGui::Separator();
    
    // Top controls row
    ImGui::BeginChild("##tutorial_controls", ImVec2(0, 80), true);
    
    // Category tabs
    render_category_tabs();
    
    // Search and filters
    ImGui::Columns(3, "##tutorial_filters", false);
    
    // Search box
    ImGui::Text("🔍 Search:");
    char search_buf[256];
    strncpy(search_buf, browser_.search_query_.c_str(), sizeof(search_buf));
    if (ImGui::InputText("##search", search_buf, sizeof(search_buf))) {
        browser_.search_query_ = search_buf;
        update_filtered_tutorials();
    }
    
    ImGui::NextColumn();
    
    // Difficulty filter
    ImGui::Text("📊 Difficulty:");
    const char* difficulty_items[] = { "Beginner", "Intermediate", "Advanced", "Expert", "All" };
    int current_difficulty = static_cast<int>(browser_.filter_difficulty_);
    if (current_difficulty == 4) current_difficulty = 4; // "All" option
    
    if (ImGui::Combo("##difficulty", &current_difficulty, difficulty_items, 5)) {
        if (current_difficulty == 4) {
            browser_.filter_difficulty_ = learning::DifficultyLevel::Beginner; // Will be ignored in filter
        } else {
            browser_.filter_difficulty_ = static_cast<learning::DifficultyLevel>(current_difficulty);
        }
        update_filtered_tutorials();
    }
    
    ImGui::NextColumn();
    
    // Display options
    ImGui::Checkbox("Show Completed", &browser_.show_completed_);
    ImGui::Checkbox("Show Prerequisites", &browser_.show_prerequisites_);
    
    ImGui::Columns(1);
    ImGui::EndChild();
    
    // Tutorial list
    ImGui::BeginChild("##tutorial_list", ImVec2(0, -50));
    render_tutorial_list();
    ImGui::EndChild();
    
    // Bottom controls
    ImGui::Separator();
    if (browser_.selected_tutorial_index_ >= 0 && 
        browser_.selected_tutorial_index_ < static_cast<int>(browser_.filtered_tutorials_.size())) {
        
        learning::Tutorial* selected = browser_.filtered_tutorials_[browser_.selected_tutorial_index_];
        
        ImGui::Text("Selected: %s", selected->title().c_str());
        ImGui::SameLine();
        
        if (ImGui::Button("Start Tutorial")) {
            start_selected_tutorial();
        }
        
        ImGui::SameLine();
        if (ImGui::Button("Preview")) {
            // Show tutorial preview popup
            ImGui::OpenPopup("Tutorial Preview");
        }
    } else {
        ImGui::TextDisabled("Select a tutorial to begin");
    }
    
    // Tutorial preview popup
    if (ImGui::BeginPopupModal("Tutorial Preview", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        if (browser_.selected_tutorial_index_ >= 0) {
            learning::Tutorial* tutorial = browser_.filtered_tutorials_[browser_.selected_tutorial_index_];
            render_tutorial_preview(tutorial);
        }
        
        if (ImGui::Button("Start Tutorial")) {
            start_selected_tutorial();
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel")) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}

void InteractiveTutorialPanel::render_active_tutorial() {
    if (!current_tutorial_) {
        ImGui::Text("No active tutorial");
        if (ImGui::Button("Browse Tutorials")) {
            set_panel_mode(PanelMode::TutorialSelection);
        }
        return;
    }
    
    // Tutorial header
    ImGui::Text("📖 %s", current_tutorial_->title().c_str());
    ImGui::SameLine();
    
    // Difficulty badge
    const char* difficulty_text = get_difficulty_display_name(current_tutorial_->difficulty()).c_str();
    ImGui::TextColored(ImVec4(0.7f, 0.7f, 1.0f, 1.0f), "[%s]", difficulty_text);
    
    ImGui::Text("%s", current_tutorial_->description().c_str());
    
    // Progress bar
    f32 progress = current_tutorial_->completion_percentage();
    ImGui::ProgressBar(progress, ImVec2(-1.0f, 0.0f), 
                      (std::to_string(static_cast<int>(progress * 100)) + "%").c_str());
    
    ImGui::Separator();
    
    // Current step info
    if (current_step_) {
        render_step_header();
        ImGui::Separator();
        render_step_content();
        ImGui::Separator();
        render_step_navigation();
    } else {
        ImGui::Text("Tutorial completed! 🎉");
        if (ImGui::Button("View Results")) {
            set_panel_mode(PanelMode::ProgressReview);
        }
    }
    
    // Visual cues overlay
    if (!step_execution_.active_cues_.empty()) {
        render_visual_cues();
    }
}

void InteractiveTutorialPanel::render_step_execution() {
    // This is handled within render_active_tutorial for better integration
    render_active_tutorial();
}

void InteractiveTutorialPanel::render_code_editor() {
    if (!current_step_ || !current_step_->code_example()) {
        ImGui::Text("No code example available");
        return;
    }
    
    render_code_editor_header();
    ImGui::Separator();
    
    // Split view: code input and output
    ImGui::Columns(2, "##code_editor", true);
    
    // Code input section
    ImGui::Text("📝 Code Editor");
    render_code_input();
    
    ImGui::NextColumn();
    
    // Output section
    ImGui::Text("📤 Output");
    render_code_output();
    
    ImGui::Columns(1);
    
    ImGui::Separator();
    render_execution_controls();
}

void InteractiveTutorialPanel::render_quiz_mode() {
    ImGui::Text("🧠 Quiz & Assessment");
    ImGui::Separator();
    
    // This would integrate with the TutorialQuizWidget
    // For now, show a placeholder
    ImGui::Text("Quiz functionality will be integrated here");
    ImGui::Text("This will include multiple choice, code completion,");
    ImGui::Text("and interactive assessment questions.");
    
    if (ImGui::Button("Return to Tutorial")) {
        set_panel_mode(PanelMode::ActiveTutorial);
    }
}

void InteractiveTutorialPanel::render_progress_review() {
    ImGui::Text("📊 Learning Progress & Analytics");
    ImGui::Separator();
    
    render_progress_header();
    ImGui::Separator();
    render_progress_indicators();
    ImGui::Separator();
    render_learning_analytics();
    ImGui::Separator();
    render_achievement_system();
}

void InteractiveTutorialPanel::render_help_system() {
    ImGui::Text("❓ Help & Support");
    ImGui::Separator();
    
    render_context_help();
    
    if (help_system_.show_help_sidebar_) {
        render_help_sidebar();
    }
    
    render_smart_hints();
}

// Helper method implementations

void InteractiveTutorialPanel::render_category_tabs() {
    const std::vector<std::pair<learning::TutorialCategory, std::string>> categories = {
        {learning::TutorialCategory::BasicConcepts, "🎯 Basics"},
        {learning::TutorialCategory::EntityManagement, "🔧 Entities"},
        {learning::TutorialCategory::ComponentSystems, "⚙️ Components"},
        {learning::TutorialCategory::SystemDesign, "🏗️ Systems"},
        {learning::TutorialCategory::MemoryOptimization, "💾 Memory"},
        {learning::TutorialCategory::AdvancedPatterns, "🚀 Advanced"},
        {learning::TutorialCategory::PerformanceAnalysis, "📈 Performance"},
        {learning::TutorialCategory::RealWorldExamples, "🌍 Examples"}
    };
    
    for (usize i = 0; i < categories.size(); ++i) {
        if (i > 0) ImGui::SameLine();
        
        const auto& [category, label] = categories[i];
        bool selected = (browser_.selected_category_ == category);
        
        if (selected) {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.7f, 1.0f, 1.0f));
        }
        
        if (ImGui::Button(label.c_str())) {
            browser_.selected_category_ = category;
            update_filtered_tutorials();
        }
        
        if (selected) {
            ImGui::PopStyleColor();
        }
    }
}

void InteractiveTutorialPanel::render_tutorial_list() {
    if (browser_.filtered_tutorials_.empty()) {
        ImGui::TextDisabled("No tutorials match your criteria");
        return;
    }
    
    for (usize i = 0; i < browser_.filtered_tutorials_.size(); ++i) {
        learning::Tutorial* tutorial = browser_.filtered_tutorials_[i];
        
        bool selected = (static_cast<int>(i) == browser_.selected_tutorial_index_);
        
        // Tutorial item
        if (ImGui::Selectable(("##tutorial_" + std::to_string(i)).c_str(), selected, 0, ImVec2(0, 60))) {
            browser_.selected_tutorial_index_ = static_cast<int>(i);
        }
        
        if (ImGui::IsItemHovered()) {
            ImGui::BeginTooltip();
            ImGui::Text("%s", tutorial->description().c_str());
            ImGui::Text("Estimated time: %.0f minutes", calculate_estimated_time(tutorial));
            ImGui::EndTooltip();
        }
        
        // Tutorial info overlay
        ImGui::SameLine();
        ImGui::BeginGroup();
        
        // Title and difficulty
        ImGui::Text("%s", tutorial->title().c_str());
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 1.0f, 1.0f), 
                          "[%s]", get_difficulty_display_name(tutorial->difficulty()).c_str());
        
        // Description (truncated)
        std::string description = tutorial->description();
        if (description.length() > 80) {
            description = description.substr(0, 77) + "...";
        }
        ImGui::TextWrapped("%s", description.c_str());
        
        // Progress indicator (if started)
        if (tutorial_manager_) {
            const auto& progress = tutorial_manager_->get_learner_progress(learner_.learner_id_);
            auto completion_it = progress.tutorial_completion.find(tutorial->id());
            if (completion_it != progress.tutorial_completion.end()) {
                f32 completion = completion_it->second;
                if (completion > 0.0f) {
                    ImGui::ProgressBar(completion, ImVec2(200, 0), 
                                     (std::to_string(static_cast<int>(completion * 100)) + "% complete").c_str());
                }
            }
        }
        
        ImGui::EndGroup();
    }
}

void InteractiveTutorialPanel::render_step_header() {
    if (!current_step_) return;
    
    ImGui::Text("Step %zu of %zu: %s", 
               current_tutorial_->current_step_index() + 1,
               current_tutorial_->total_steps(),
               current_step_->title().c_str());
    
    // Step progress
    f32 step_progress = current_step_->completion_score();
    if (step_progress > 0.0f) {
        ImGui::ProgressBar(step_progress, ImVec2(-1.0f, 0.0f));
    }
    
    // Time spent on step
    f64 step_time = step_execution_.step_duration_;
    ImGui::Text("Time on step: %s", format_time_duration(step_time).c_str());
}

void InteractiveTutorialPanel::render_step_content() {
    if (!current_step_) return;
    
    // Step description
    ImGui::TextWrapped("%s", current_step_->description().c_str());
    
    // Detailed explanation (expandable)
    if (!current_step_->detailed_explanation().empty()) {
        if (ImGui::CollapsingHeader("Detailed Explanation")) {
            ImGui::TextWrapped("%s", current_step_->detailed_explanation().c_str());
        }
    }
    
    // Code example
    if (current_step_->code_example()) {
        ImGui::Separator();
        if (ImGui::Button("Open Code Editor")) {
            set_panel_mode(PanelMode::CodeEditor);
        }
    }
    
    // Hints (if available and requested)
    if (step_execution_.show_hints_ && !current_step_->help_topic().empty()) {
        ImGui::Separator();
        ImGui::Text("💡 Hint:");
        ImGui::TextWrapped("%s", current_step_->get_next_hint().c_str());
    }
}

void InteractiveTutorialPanel::render_step_navigation() {
    // Previous step button
    bool can_go_back = current_tutorial_ && current_tutorial_->current_step_index() > 0;
    if (!can_go_back) {
        ImGui::BeginDisabled();
    }
    
    if (ImGui::Button("⬅️ Previous")) {
        return_to_previous_step();
    }
    
    if (!can_go_back) {
        ImGui::EndDisabled();
    }
    
    ImGui::SameLine();
    
    // Validate/Next step button
    if (current_step_ && !current_step_->is_completed()) {
        if (ImGui::Button("✅ Validate")) {
            validate_current_step();
        }
        
        // Show hint button
        ImGui::SameLine();
        if (ImGui::Button("💡 Hint")) {
            step_execution_.show_hints_ = true;
        }
    } else {
        if (ImGui::Button("➡️ Next")) {
            advance_to_next_step();
        }
    }
    
    // Validation feedback
    if (step_validation_pending_) {
        ImGui::SameLine();
        ImGui::Text("Validating...");
    } else if (!step_execution_.last_feedback_message_.empty()) {
        ImGui::SameLine();
        bool is_success = current_step_ && current_step_->is_completed();
        ImVec4 color = is_success ? ImVec4(0.2f, 0.8f, 0.3f, 1.0f) : ImVec4(1.0f, 0.3f, 0.3f, 1.0f);
        ImGui::TextColored(color, "%s", step_execution_.last_feedback_message_.c_str());
    }
}

void InteractiveTutorialPanel::update_filtered_tutorials() {
    browser_.filtered_tutorials_.clear();
    
    if (!tutorial_manager_) return;
    
    auto all_tutorials = tutorial_manager_->get_all_tutorials();
    
    for (learning::Tutorial* tutorial : all_tutorials) {
        // Category filter
        if (tutorial->category() != browser_.selected_category_) {
            continue;
        }
        
        // Search filter
        if (!browser_.search_query_.empty()) {
            std::string query_lower = browser_.search_query_;
            std::transform(query_lower.begin(), query_lower.end(), query_lower.begin(), ::tolower);
            
            std::string title_lower = tutorial->title();
            std::string desc_lower = tutorial->description();
            std::transform(title_lower.begin(), title_lower.end(), title_lower.begin(), ::tolower);
            std::transform(desc_lower.begin(), desc_lower.end(), desc_lower.begin(), ::tolower);
            
            if (title_lower.find(query_lower) == std::string::npos && 
                desc_lower.find(query_lower) == std::string::npos) {
                continue;
            }
        }
        
        browser_.filtered_tutorials_.push_back(tutorial);
    }
    
    // Reset selection if out of bounds
    if (browser_.selected_tutorial_index_ >= static_cast<int>(browser_.filtered_tutorials_.size())) {
        browser_.selected_tutorial_index_ = -1;
    }
}

void InteractiveTutorialPanel::start_selected_tutorial() {
    if (browser_.selected_tutorial_index_ < 0 || 
        browser_.selected_tutorial_index_ >= static_cast<int>(browser_.filtered_tutorials_.size())) {
        return;
    }
    
    learning::Tutorial* selected = browser_.filtered_tutorials_[browser_.selected_tutorial_index_];
    start_tutorial(selected->id());
}

void InteractiveTutorialPanel::start_tutorial(const std::string& tutorial_id) {
    if (!tutorial_manager_) return;
    
    if (tutorial_manager_->start_tutorial(tutorial_id, learner_.learner_id_)) {
        current_tutorial_ = tutorial_manager_->current_tutorial();
        current_step_ = current_tutorial_->current_step();
        tutorial_active_ = true;
        
        // Initialize step execution
        step_execution_.step_start_time_ = std::chrono::steady_clock::now();
        step_execution_.step_duration_ = 0.0;
        step_execution_.validation_attempts_ = 0;
        step_execution_.show_hints_ = false;
        step_execution_.active_cues_.clear();
        
        // Update visual cues
        if (current_step_) {
            step_execution_.active_cues_ = current_step_->visual_cues();
        }
        
        set_panel_mode(PanelMode::ActiveTutorial);
        on_tutorial_started(tutorial_id);
    }
}

void InteractiveTutorialPanel::validate_current_step() {
    if (!current_step_) return;
    
    step_validation_pending_ = true;
    step_execution_.validation_attempts_++;
    
    learning::ValidationResult result = current_step_->validate();
    provide_step_feedback(result);
    
    step_validation_pending_ = false;
    
    if (result.is_valid) {
        trigger_success_animation();
        if (step_execution_.auto_advance_enabled_) {
            advance_to_next_step();
        }
    } else {
        trigger_error_animation();
        show_hint_if_needed();
    }
}

void InteractiveTutorialPanel::provide_step_feedback(const learning::ValidationResult& result) {
    step_execution_.last_feedback_message_ = result.feedback;
    step_execution_.feedback_display_timer_ = 3.0f; // Show for 3 seconds
    
    if (result.is_valid) {
        progress_.successful_validations_++;
        on_step_completed(current_step_->id());
    } else {
        // Add hints to help system if available
        for (const std::string& hint : result.hints) {
            // Could add to smart hints system
        }
    }
}

void InteractiveTutorialPanel::advance_to_next_step() {
    if (!tutorial_manager_) return;
    
    bool advanced = tutorial_manager_->advance_current_tutorial();
    if (advanced) {
        current_step_ = current_tutorial_->current_step();
        
        // Reset step execution state
        step_execution_.step_start_time_ = std::chrono::steady_clock::now();
        step_execution_.step_duration_ = 0.0;
        step_execution_.validation_attempts_ = 0;
        step_execution_.show_hints_ = false;
        step_execution_.last_feedback_message_.clear();
        
        // Update visual cues
        if (current_step_) {
            step_execution_.active_cues_ = current_step_->visual_cues();
        }
    } else {
        // Tutorial completed
        complete_current_tutorial();
    }
}

void InteractiveTutorialPanel::complete_current_tutorial() {
    if (!tutorial_manager_ || !current_tutorial_) return;
    
    tutorial_manager_->complete_current_tutorial();
    on_tutorial_completed(current_tutorial_->id());
    
    // Show completion celebration
    progress_.show_achievement_popup_ = true;
    progress_.current_achievement_ = "Completed " + current_tutorial_->title();
    progress_.achievement_popup_timer_ = 3.0f;
    
    tutorial_active_ = false;
    set_panel_mode(PanelMode::ProgressReview);
}

// Utility method implementations

std::string InteractiveTutorialPanel::format_time_duration(f64 seconds) const {
    int minutes = static_cast<int>(seconds) / 60;
    int secs = static_cast<int>(seconds) % 60;
    
    std::ostringstream oss;
    oss << minutes << "m " << secs << "s";
    return oss.str();
}

std::string InteractiveTutorialPanel::get_difficulty_display_name(learning::DifficultyLevel level) const {
    switch (level) {
        case learning::DifficultyLevel::Beginner: return "Beginner";
        case learning::DifficultyLevel::Intermediate: return "Intermediate";
        case learning::DifficultyLevel::Advanced: return "Advanced";
        case learning::DifficultyLevel::Expert: return "Expert";
        default: return "Unknown";
    }
}

std::string InteractiveTutorialPanel::get_category_display_name(learning::TutorialCategory category) const {
    switch (category) {
        case learning::TutorialCategory::BasicConcepts: return "Basic Concepts";
        case learning::TutorialCategory::EntityManagement: return "Entity Management";
        case learning::TutorialCategory::ComponentSystems: return "Component Systems";
        case learning::TutorialCategory::SystemDesign: return "System Design";
        case learning::TutorialCategory::MemoryOptimization: return "Memory Optimization";
        case learning::TutorialCategory::AdvancedPatterns: return "Advanced Patterns";
        case learning::TutorialCategory::RealWorldExamples: return "Real World Examples";
        case learning::TutorialCategory::PerformanceAnalysis: return "Performance Analysis";
        default: return "Unknown";
    }
}

f32 InteractiveTutorialPanel::calculate_estimated_time(learning::Tutorial* tutorial) const {
    if (!tutorial) return 0.0f;
    
    // Rough estimate: 3-5 minutes per step depending on difficulty
    f32 minutes_per_step = 3.0f;
    switch (tutorial->difficulty()) {
        case learning::DifficultyLevel::Beginner: minutes_per_step = 3.0f; break;
        case learning::DifficultyLevel::Intermediate: minutes_per_step = 4.0f; break;
        case learning::DifficultyLevel::Advanced: minutes_per_step = 6.0f; break;
        case learning::DifficultyLevel::Expert: minutes_per_step = 8.0f; break;
    }
    
    return static_cast<f32>(tutorial->total_steps()) * minutes_per_step;
}

// Placeholder implementations for remaining methods
void InteractiveTutorialPanel::render_tutorial_preview(learning::Tutorial* tutorial) {
    if (!tutorial) return;
    
    ImGui::Text("📖 %s", tutorial->title().c_str());
    ImGui::Text("📊 Difficulty: %s", get_difficulty_display_name(tutorial->difficulty()).c_str());
    ImGui::Text("⏱️ Estimated time: %.0f minutes", calculate_estimated_time(tutorial));
    ImGui::Text("📝 Steps: %zu", tutorial->total_steps());
    
    ImGui::Separator();
    ImGui::TextWrapped("%s", tutorial->description().c_str());
    
    if (!tutorial->learning_objectives().empty()) {
        ImGui::Separator();
        ImGui::Text("🎯 Learning Objectives:");
        for (const std::string& objective : tutorial->learning_objectives()) {
            ImGui::BulletText("%s", objective.c_str());
        }
    }
}

// Additional placeholder implementations for complex rendering methods
void InteractiveTutorialPanel::update_animations(f64 delta_time) {
    // Update step execution timing
    if (current_step_ && current_step_->has_started()) {
        auto now = std::chrono::steady_clock::now();
        step_execution_.step_duration_ = std::chrono::duration<f64>(
            now - step_execution_.step_start_time_).count();
    }
    
    // Update visual effect animations
    effects_.cue_pulse_phase_ += delta_time * effects_.highlight_pulse_frequency_ * 2.0 * M_PI;
    if (effects_.cue_pulse_phase_ > 2.0 * M_PI) {
        effects_.cue_pulse_phase_ -= 2.0 * M_PI;
    }
}

void InteractiveTutorialPanel::trigger_success_animation() {
    effects_.show_success_animation_ = true;
    effects_.success_animation_progress_ = 0.0f;
}

void InteractiveTutorialPanel::trigger_error_animation() {
    effects_.error_shake_timer_ = ERROR_ANIMATION_DURATION;
}

// Event handler implementations
void InteractiveTutorialPanel::on_tutorial_started(const std::string& tutorial_id) {
    LOG_INFO("Started tutorial: " + tutorial_id);
}

void InteractiveTutorialPanel::on_tutorial_completed(const std::string& tutorial_id) {
    LOG_INFO("Completed tutorial: " + tutorial_id);
}

void InteractiveTutorialPanel::on_step_completed(const std::string& step_id) {
    LOG_INFO("Completed step: " + step_id);
}

void InteractiveTutorialPanel::on_validation_failed(const learning::ValidationResult& result) {
    LOG_INFO("Validation failed: " + result.feedback);
}

void InteractiveTutorialPanel::on_achievement_unlocked(const std::string& achievement) {
    LOG_INFO("Achievement unlocked: " + achievement);
    progress_.session_achievements_.push_back(achievement);
}

// Configuration methods
void InteractiveTutorialPanel::set_panel_mode(PanelMode mode) {
    current_mode_ = mode;
}

void InteractiveTutorialPanel::set_learner_id(const std::string& learner_id) {
    learner_.learner_id_ = learner_id;
    if (tutorial_manager_) {
        tutorial_manager_->set_current_learner(learner_id);
    }
}

// Placeholder implementations for remaining complex methods
void InteractiveTutorialPanel::render_visual_cues() {
    // Implementation would render overlay effects for UI highlighting
}

void InteractiveTutorialPanel::render_highlight_effects() {
    // Implementation would render highlight animations
}

void InteractiveTutorialPanel::render_particle_effects() {
    // Implementation would render success/celebration particles
}

void InteractiveTutorialPanel::render_progress_header() {
    ImGui::Text("📈 Learning Progress Overview");
}

void InteractiveTutorialPanel::render_progress_indicators() {
    ImGui::Text("Progress indicators will be implemented here");
}

void InteractiveTutorialPanel::render_learning_analytics() {
    ImGui::Text("Learning analytics will be implemented here");
}

void InteractiveTutorialPanel::render_achievement_system() {
    ImGui::Text("Achievement system will be implemented here");
}

void InteractiveTutorialPanel::render_context_help() {
    ImGui::Text("Context help will be implemented here");
}

void InteractiveTutorialPanel::render_help_sidebar() {
    ImGui::Text("Help sidebar will be implemented here");
}

void InteractiveTutorialPanel::render_smart_hints() {
    ImGui::Text("Smart hints will be implemented here");
}

void InteractiveTutorialPanel::render_code_editor_header() {
    ImGui::Text("Code Editor");
}

void InteractiveTutorialPanel::render_code_input() {
    ImGui::Text("Code input will be implemented here");
}

void InteractiveTutorialPanel::render_code_output() {
    ImGui::Text("Code output will be implemented here");
}

void InteractiveTutorialPanel::render_execution_controls() {
    ImGui::Text("Execution controls will be implemented here");
}

// Stub implementations for remaining methods
void InteractiveTutorialPanel::update_progress_tracking(f64 delta_time) { }
void InteractiveTutorialPanel::update_adaptive_help() { }
void InteractiveTutorialPanel::show_hint_if_needed() { }
void InteractiveTutorialPanel::load_learner_preferences() { }
void InteractiveTutorialPanel::save_learner_progress() { }
void InteractiveTutorialPanel::reset_current_tutorial() { }
void InteractiveTutorialPanel::abandon_current_tutorial() { }
void InteractiveTutorialPanel::return_to_previous_step() { }

} // namespace ecscope::ui