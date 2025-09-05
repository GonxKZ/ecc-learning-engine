#include "tutorial_system.hpp"
#include "core/log.hpp"
#include <algorithm>
#include <numeric>
#include <sstream>
#include <random>

namespace ecscope::learning {

// TutorialStep Implementation

TutorialStep::TutorialStep(const std::string& id, const std::string& title, const std::string& description)
    : id_(id), title_(title), description_(description), interaction_type_(InteractionType::ReadOnly) {
}

TutorialStep& TutorialStep::set_interaction_type(InteractionType type) {
    interaction_type_ = type;
    return *this;
}

TutorialStep& TutorialStep::set_detailed_explanation(const std::string& explanation) {
    detailed_explanation_ = explanation;
    return *this;
}

TutorialStep& TutorialStep::add_visual_cue(const VisualCue& cue) {
    visual_cues_.push_back(cue);
    return *this;
}

TutorialStep& TutorialStep::set_code_example(std::unique_ptr<CodeExample> example) {
    code_example_ = std::move(example);
    return *this;
}

TutorialStep& TutorialStep::set_validator(std::function<ValidationResult()> validator) {
    validator_ = std::move(validator);
    return *this;
}

TutorialStep& TutorialStep::add_hint(const std::string& hint) {
    contextual_hints_.push_back(hint);
    return *this;
}

TutorialStep& TutorialStep::set_help_topic(const std::string& topic) {
    help_topic_ = topic;
    return *this;
}

void TutorialStep::start() {
    if (!has_started_) {
        has_started_ = true;
        start_time_ = std::chrono::steady_clock::now();
        current_hint_level_ = 0;
        LOG_INFO("Started tutorial step: " + title_);
    }
}

void TutorialStep::reset() {
    is_completed_ = false;
    has_started_ = false;
    attempt_count_ = 0;
    time_spent_ = 0.0;
    current_hint_level_ = 0;
    last_validation_ = ValidationResult();
    
    if (code_example_) {
        code_example_->current_code = code_example_->code_template;
        code_example_->hint_level = 0;
    }
}

ValidationResult TutorialStep::validate() {
    if (!validator_) {
        return ValidationResult::success("Step completed");
    }
    
    attempt_count_++;
    last_validation_ = validator_();
    
    if (last_validation_.is_valid) {
        complete();
    }
    
    return last_validation_;
}

void TutorialStep::complete() {
    if (!is_completed_) {
        is_completed_ = true;
        auto now = std::chrono::steady_clock::now();
        if (has_started_) {
            time_spent_ += std::chrono::duration<f64>(now - start_time_).count();
        }
        LOG_INFO("Completed tutorial step: " + title_);
    }
}

f64 TutorialStep::time_spent() const {
    if (!has_started_) return time_spent_;
    
    auto now = std::chrono::steady_clock::now();
    return time_spent_ + std::chrono::duration<f64>(now - start_time_).count();
}

std::string TutorialStep::get_next_hint() {
    if (current_hint_level_ >= contextual_hints_.size()) {
        return "No more hints available. Try reviewing the explanation or asking for help.";
    }
    
    return contextual_hints_[current_hint_level_++];
}

void TutorialStep::request_help() {
    // This could trigger additional help UI or logging
    LOG_INFO("Help requested for step: " + title_);
}

bool TutorialStep::needs_additional_help() const {
    return attempt_count_ > 3 || time_spent() > 300.0; // 5 minutes
}

DifficultyLevel TutorialStep::suggested_difficulty() const {
    if (attempt_count_ <= 1 && time_spent() < 60.0) {
        return DifficultyLevel::Advanced;
    } else if (attempt_count_ <= 3 && time_spent() < 180.0) {
        return DifficultyLevel::Intermediate;
    } else {
        return DifficultyLevel::Beginner;
    }
}

// Tutorial Implementation

Tutorial::Tutorial(const std::string& id, const std::string& title, TutorialCategory category, DifficultyLevel difficulty)
    : id_(id), title_(title), category_(category), difficulty_(difficulty) {
}

Tutorial& Tutorial::set_description(const std::string& description) {
    description_ = description;
    return *this;
}

Tutorial& Tutorial::add_step(std::unique_ptr<TutorialStep> step) {
    steps_.push_back(std::move(step));
    return *this;
}

Tutorial& Tutorial::add_prerequisite(const std::string& tutorial_id) {
    prerequisite_tutorials_.push_back(tutorial_id);
    return *this;
}

Tutorial& Tutorial::add_recommended_next(const std::string& tutorial_id) {
    recommended_next_.push_back(tutorial_id);
    return *this;
}

Tutorial& Tutorial::add_learning_objective(const std::string& objective) {
    learning_objectives_.push_back(objective);
    objectives_met_[objective] = false;
    return *this;
}

Tutorial& Tutorial::add_reference_link(const std::string& link) {
    reference_links_.push_back(link);
    return *this;
}

void Tutorial::start() {
    if (!is_started_) {
        is_started_ = true;
        start_time_ = std::chrono::steady_clock::now();
        current_step_index_ = 0;
        
        if (!steps_.empty()) {
            steps_[0]->start();
        }
        
        LOG_INFO("Started tutorial: " + title_);
    }
}

void Tutorial::reset() {
    is_started_ = false;
    is_completed_ = false;
    current_step_index_ = 0;
    total_time_spent_ = 0.0;
    
    // Reset all objectives
    for (auto& [objective, met] : objectives_met_) {
        met = false;
    }
    
    // Reset all steps
    for (auto& step : steps_) {
        step->reset();
    }
}

bool Tutorial::advance_step() {
    if (!is_started_ || is_completed_) {
        return false;
    }
    
    // Validate current step
    if (current_step_index_ < steps_.size()) {
        auto& current = steps_[current_step_index_];
        ValidationResult result = current->validate();
        
        if (!result.is_valid) {
            return false; // Cannot advance without completing current step
        }
    }
    
    current_step_index_++;
    
    if (current_step_index_ >= steps_.size()) {
        complete();
        return false;
    }
    
    // Start next step
    steps_[current_step_index_]->start();
    return true;
}

bool Tutorial::previous_step() {
    if (current_step_index_ == 0) {
        return false;
    }
    
    current_step_index_--;
    return true;
}

void Tutorial::complete() {
    if (!is_completed_) {
        is_completed_ = true;
        auto now = std::chrono::steady_clock::now();
        if (is_started_) {
            total_time_spent_ += std::chrono::duration<f64>(now - start_time_).count();
        }
        
        LOG_INFO("Completed tutorial: " + title_);
    }
}

TutorialStep* Tutorial::current_step() const {
    if (current_step_index_ >= steps_.size()) {
        return nullptr;
    }
    return steps_[current_step_index_].get();
}

TutorialStep* Tutorial::get_step(usize index) const {
    if (index >= steps_.size()) {
        return nullptr;
    }
    return steps_[index].get();
}

f32 Tutorial::completion_percentage() const {
    if (steps_.empty()) return 0.0f;
    
    usize completed_steps = 0;
    for (const auto& step : steps_) {
        if (step->is_completed()) {
            completed_steps++;
        }
    }
    
    return static_cast<f32>(completed_steps) / static_cast<f32>(steps_.size());
}

f64 Tutorial::total_time_spent() const {
    if (!is_started_) return total_time_spent_;
    
    auto now = std::chrono::steady_clock::now();
    f64 current_session = std::chrono::duration<f64>(now - start_time_).count();
    return total_time_spent_ + current_session;
}

bool Tutorial::is_objective_met(const std::string& objective) const {
    auto it = objectives_met_.find(objective);
    return it != objectives_met_.end() ? it->second : false;
}

void Tutorial::mark_objective_met(const std::string& objective) {
    objectives_met_[objective] = true;
}

f32 Tutorial::objectives_completion_rate() const {
    if (objectives_met_.empty()) return 1.0f;
    
    usize met_count = std::count_if(objectives_met_.begin(), objectives_met_.end(),
                                   [](const auto& pair) { return pair.second; });
    
    return static_cast<f32>(met_count) / static_cast<f32>(objectives_met_.size());
}

DifficultyLevel Tutorial::calculate_effective_difficulty(const LearningProgress& progress) const {
    // Adaptive difficulty based on learner's progress
    auto it = progress.tutorial_completion.find(id_);
    if (it != progress.tutorial_completion.end()) {
        f32 completion = it->second;
        if (completion < 0.3f) {
            // Struggling, reduce difficulty
            return static_cast<DifficultyLevel>(std::max(0, static_cast<int>(difficulty_) - 1));
        } else if (completion > 0.8f) {
            // Doing well, can increase difficulty
            return static_cast<DifficultyLevel>(std::min(3, static_cast<int>(difficulty_) + 1));
        }
    }
    
    return difficulty_;
}

std::vector<std::string> Tutorial::get_struggling_concepts() const {
    std::vector<std::string> struggling;
    
    for (const auto& step : steps_) {
        if (step->attempt_count() > 3 || step->needs_additional_help()) {
            struggling.push_back(step->help_topic());
        }
    }
    
    return struggling;
}

// TutorialManager Implementation

void TutorialManager::register_tutorial(std::unique_ptr<Tutorial> tutorial) {
    if (!tutorial) return;
    
    std::string id = tutorial->id();
    TutorialCategory category = tutorial->category();
    DifficultyLevel difficulty = tutorial->difficulty();
    
    // Add to main collection
    tutorials_[id] = std::move(tutorial);
    
    // Update indices
    category_index_[category].push_back(id);
    difficulty_index_[difficulty].push_back(id);
    
    LOG_INFO("Registered tutorial: " + id);
}

Tutorial* TutorialManager::get_tutorial(const std::string& id) const {
    auto it = tutorials_.find(id);
    return it != tutorials_.end() ? it->second.get() : nullptr;
}

std::vector<Tutorial*> TutorialManager::get_tutorials_by_category(TutorialCategory category) const {
    std::vector<Tutorial*> result;
    
    auto it = category_index_.find(category);
    if (it != category_index_.end()) {
        for (const std::string& id : it->second) {
            if (Tutorial* tutorial = get_tutorial(id)) {
                result.push_back(tutorial);
            }
        }
    }
    
    return result;
}

std::vector<Tutorial*> TutorialManager::get_tutorials_by_difficulty(DifficultyLevel difficulty) const {
    std::vector<Tutorial*> result;
    
    auto it = difficulty_index_.find(difficulty);
    if (it != difficulty_index_.end()) {
        for (const std::string& id : it->second) {
            if (Tutorial* tutorial = get_tutorial(id)) {
                result.push_back(tutorial);
            }
        }
    }
    
    return result;
}

void TutorialManager::create_learning_path(const std::string& learner_id, 
                                          const std::vector<TutorialCategory>& preferred_categories,
                                          DifficultyLevel starting_difficulty) {
    set_current_learner(learner_id);
    learning_path_.clear();
    current_path_index_ = 0;
    
    // If no preferences given, use default progression
    std::vector<TutorialCategory> categories = preferred_categories;
    if (categories.empty()) {
        categories = {
            TutorialCategory::BasicConcepts,
            TutorialCategory::EntityManagement,
            TutorialCategory::ComponentSystems,
            TutorialCategory::SystemDesign,
            TutorialCategory::MemoryOptimization,
            TutorialCategory::AdvancedPatterns,
            TutorialCategory::PerformanceAnalysis,
            TutorialCategory::RealWorldExamples
        };
    }
    
    // Build path based on categories and difficulty progression
    for (TutorialCategory category : categories) {
        auto tutorials = get_tutorials_by_category(category);
        
        // Sort by difficulty
        std::sort(tutorials.begin(), tutorials.end(),
                 [](Tutorial* a, Tutorial* b) {
                     return static_cast<int>(a->difficulty()) < static_cast<int>(b->difficulty());
                 });
        
        // Add tutorials at appropriate difficulty level
        for (Tutorial* tutorial : tutorials) {
            if (static_cast<int>(tutorial->difficulty()) >= static_cast<int>(starting_difficulty)) {
                learning_path_.push_back(tutorial->id());
            }
        }
    }
    
    LOG_INFO("Created learning path with " + std::to_string(learning_path_.size()) + " tutorials for learner: " + learner_id);
}

void TutorialManager::set_custom_learning_path(const std::vector<std::string>& tutorial_ids) {
    learning_path_ = tutorial_ids;
    current_path_index_ = 0;
}

std::vector<std::string> TutorialManager::generate_adaptive_path(const std::string& learner_id) {
    LearningProgress& progress = get_learner_progress(learner_id);
    
    std::vector<std::string> adaptive_path;
    
    // Analyze learner's strengths and weaknesses
    std::unordered_map<TutorialCategory, f32> category_performance;
    
    for (const auto& [tutorial_id, completion] : progress.tutorial_completion) {
        if (Tutorial* tutorial = get_tutorial(tutorial_id)) {
            TutorialCategory category = tutorial->category();
            category_performance[category] = std::max(category_performance[category], completion);
        }
    }
    
    // Generate path focusing on areas needing improvement
    std::vector<std::pair<TutorialCategory, f32>> sorted_categories;
    for (const auto& [category, performance] : category_performance) {
        sorted_categories.emplace_back(category, performance);
    }
    
    std::sort(sorted_categories.begin(), sorted_categories.end(),
             [](const auto& a, const auto& b) { return a.second < b.second; });
    
    // Add tutorials for weakest categories first
    for (const auto& [category, performance] : sorted_categories) {
        if (performance < 0.8f) { // Focus on areas below 80% mastery
            auto tutorials = get_tutorials_by_category(category);
            for (Tutorial* tutorial : tutorials) {
                // Skip already completed tutorials
                auto it = progress.tutorial_completion.find(tutorial->id());
                if (it == progress.tutorial_completion.end() || it->second < 1.0f) {
                    adaptive_path.push_back(tutorial->id());
                }
            }
        }
    }
    
    return adaptive_path;
}

bool TutorialManager::start_tutorial(const std::string& tutorial_id, const std::string& learner_id) {
    Tutorial* tutorial = get_tutorial(tutorial_id);
    if (!tutorial) {
        LOG_ERROR("Tutorial not found: " + tutorial_id);
        return false;
    }
    
    set_current_learner(learner_id);
    current_tutorial_ = tutorial;
    tutorial->start();
    
    return true;
}

bool TutorialManager::advance_current_tutorial() {
    if (!current_tutorial_) return false;
    
    bool advanced = current_tutorial_->advance_step();
    
    // Update progress
    if (!current_learner_id_.empty()) {
        LearningProgress& progress = get_learner_progress(current_learner_id_);
        progress.tutorial_completion[current_tutorial_->id()] = current_tutorial_->completion_percentage();
        
        if (current_tutorial_->is_completed()) {
            progress.total_tutorials_completed++;
        }
    }
    
    return advanced;
}

bool TutorialManager::previous_step_current_tutorial() {
    if (!current_tutorial_) return false;
    return current_tutorial_->previous_step();
}

void TutorialManager::reset_current_tutorial() {
    if (current_tutorial_) {
        current_tutorial_->reset();
    }
}

void TutorialManager::complete_current_tutorial() {
    if (!current_tutorial_) return;
    
    current_tutorial_->complete();
    
    // Update progress
    if (!current_learner_id_.empty()) {
        LearningProgress& progress = get_learner_progress(current_learner_id_);
        progress.tutorial_completion[current_tutorial_->id()] = 1.0f;
        progress.total_tutorials_completed++;
        progress.total_learning_time += current_tutorial_->total_time_spent();
    }
}

void TutorialManager::set_current_learner(const std::string& learner_id) {
    current_learner_id_ = learner_id;
    
    // Ensure learner exists in progress map
    if (learner_progress_.find(learner_id) == learner_progress_.end()) {
        learner_progress_[learner_id] = LearningProgress{};
        learner_progress_[learner_id].learner_id = learner_id;
    }
}

LearningProgress& TutorialManager::get_learner_progress(const std::string& learner_id) {
    auto it = learner_progress_.find(learner_id);
    if (it == learner_progress_.end()) {
        learner_progress_[learner_id] = LearningProgress{};
        learner_progress_[learner_id].learner_id = learner_id;
        it = learner_progress_.find(learner_id);
    }
    return it->second;
}

const LearningProgress& TutorialManager::get_learner_progress(const std::string& learner_id) const {
    static LearningProgress empty_progress;
    auto it = learner_progress_.find(learner_id);
    return it != learner_progress_.end() ? it->second : empty_progress;
}

std::vector<std::string> TutorialManager::get_recommended_tutorials(const std::string& learner_id) const {
    const LearningProgress& progress = get_learner_progress(learner_id);
    std::vector<std::string> recommendations;
    
    // Find tutorials that match current level and haven't been completed
    for (const auto& [id, tutorial] : tutorials_) {
        auto completion_it = progress.tutorial_completion.find(id);
        bool is_completed = (completion_it != progress.tutorial_completion.end() && 
                           completion_it->second >= 1.0f);
        
        if (!is_completed) {
            // Check if prerequisites are met
            bool prerequisites_met = true;
            for (const std::string& prereq_id : tutorial->prerequisites()) {
                auto prereq_it = progress.tutorial_completion.find(prereq_id);
                if (prereq_it == progress.tutorial_completion.end() || 
                    prereq_it->second < 0.8f) {
                    prerequisites_met = false;
                    break;
                }
            }
            
            if (prerequisites_met) {
                recommendations.push_back(id);
            }
        }
    }
    
    return recommendations;
}

std::vector<std::string> TutorialManager::search_tutorials(const std::string& query) const {
    std::vector<std::string> results;
    
    std::string lower_query = query;
    std::transform(lower_query.begin(), lower_query.end(), lower_query.begin(), ::tolower);
    
    for (const auto& [id, tutorial] : tutorials_) {
        std::string lower_title = tutorial->title();
        std::string lower_description = tutorial->description();
        std::transform(lower_title.begin(), lower_title.end(), lower_title.begin(), ::tolower);
        std::transform(lower_description.begin(), lower_description.end(), lower_description.begin(), ::tolower);
        
        if (lower_title.find(lower_query) != std::string::npos ||
            lower_description.find(lower_query) != std::string::npos) {
            results.push_back(id);
        }
    }
    
    return results;
}

std::vector<Tutorial*> TutorialManager::get_all_tutorials() const {
    std::vector<Tutorial*> result;
    result.reserve(tutorials_.size());
    
    for (const auto& [id, tutorial] : tutorials_) {
        result.push_back(tutorial.get());
    }
    
    return result;
}

void TutorialManager::clear_all_progress() {
    learner_progress_.clear();
    current_learner_id_.clear();
    current_tutorial_ = nullptr;
}

TutorialManager::LearningAnalytics TutorialManager::generate_analytics(const std::string& learner_id) const {
    const LearningProgress& progress = get_learner_progress(learner_id);
    LearningAnalytics analytics;
    
    // Calculate average completion time
    if (progress.time_spent.empty()) {
        analytics.average_completion_time = 0.0;
    } else {
        f64 total_time = std::accumulate(progress.time_spent.begin(), progress.time_spent.end(), 0.0,
                                       [](f64 sum, const auto& pair) { return sum + pair.second; });
        analytics.average_completion_time = total_time / progress.time_spent.size();
    }
    
    // Overall progress
    if (!progress.tutorial_completion.empty()) {
        f32 total_completion = std::accumulate(progress.tutorial_completion.begin(), 
                                             progress.tutorial_completion.end(), 0.0f,
                                             [](f32 sum, const auto& pair) { 
                                                 return sum + pair.second; 
                                             });
        analytics.overall_progress = total_completion / progress.tutorial_completion.size();
    }
    
    // Success rate
    analytics.total_attempts = std::accumulate(progress.step_attempts.begin(), 
                                             progress.step_attempts.end(), 0u,
                                             [](u32 sum, const auto& pair) { 
                                                 return sum + pair.second; 
                                             });
    
    analytics.successful_completions = static_cast<u32>(
        std::count_if(progress.tutorial_completion.begin(), 
                     progress.tutorial_completion.end(),
                     [](const auto& pair) { return pair.second >= 1.0f; })
    );
    
    return analytics;
}

} // namespace ecscope::learning