#include "progress_storage.hpp"
#include "core/log.hpp"
#include <filesystem>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <ctime>

namespace ecscope::learning {

// Global progress storage instance
static std::unique_ptr<ProgressStorage> g_progress_storage;

// ProgressStorage Implementation

void ProgressStorage::initialize(const std::string& storage_directory) {
    storage_directory_ = storage_directory;
    
    if (!ensure_storage_directory_exists()) {
        LOG_ERROR("Failed to create storage directory: " + storage_directory);
        return;
    }
    
    // Load all existing progress data
    load_all_progress();
    
    last_auto_save_ = std::chrono::steady_clock::now();
    
    LOG_INFO("Progress storage initialized at: " + storage_directory);
}

void ProgressStorage::shutdown() {
    // Save all progress before shutdown
    if (auto_save_enabled_) {
        save_all_progress();
    }
    
    learner_data_.clear();
    LOG_INFO("Progress storage shutdown");
}

void ProgressStorage::create_learner_profile(const std::string& learner_id, const std::string& display_name) {
    if (learner_exists(learner_id)) {
        LOG_WARNING("Learner profile already exists: " + learner_id);
        return;
    }
    
    learner_data_[learner_id] = LearnerProgressData(learner_id, display_name);
    save_progress(learner_id);
    
    LOG_INFO("Created learner profile: " + learner_id);
}

void ProgressStorage::delete_learner_profile(const std::string& learner_id) {
    auto it = learner_data_.find(learner_id);
    if (it == learner_data_.end()) {
        LOG_WARNING("Learner profile not found: " + learner_id);
        return;
    }
    
    learner_data_.erase(it);
    
    // Delete the file
    std::string filepath = get_storage_path(learner_id);
    if (file_exists(filepath)) {
        std::filesystem::remove(filepath);
    }
    
    // Switch to different learner if this was current
    if (current_learner_id_ == learner_id) {
        current_learner_id_.clear();
        if (!learner_data_.empty()) {
            current_learner_id_ = learner_data_.begin()->first;
        }
    }
    
    LOG_INFO("Deleted learner profile: " + learner_id);
}

void ProgressStorage::set_current_learner(const std::string& learner_id) {
    if (!learner_exists(learner_id)) {
        create_learner_profile(learner_id);
    }
    
    current_learner_id_ = learner_id;
    update_learner_activity(learner_id);
    
    LOG_INFO("Set current learner: " + learner_id);
}

std::vector<std::string> ProgressStorage::get_all_learner_ids() const {
    std::vector<std::string> ids;
    ids.reserve(learner_data_.size());
    
    for (const auto& [id, data] : learner_data_) {
        ids.push_back(id);
    }
    
    return ids;
}

bool ProgressStorage::learner_exists(const std::string& learner_id) const {
    return learner_data_.find(learner_id) != learner_data_.end();
}

// Progress tracking methods

void ProgressStorage::record_tutorial_progress(const std::string& tutorial_id, f32 completion, f64 time_spent_minutes) {
    if (current_learner_id_.empty()) return;
    
    auto* data = get_learner_progress();
    if (!data) return;
    
    data->tutorial_completion[tutorial_id] = completion;
    data->tutorial_time_spent[tutorial_id] = time_spent_minutes;
    data->tutorial_attempts[tutorial_id]++;
    data->total_learning_time_hours += time_spent_minutes / 60.0;
    
    update_learner_activity(current_learner_id_);
    
    LOG_INFO("Recorded tutorial progress: " + tutorial_id + " (" + std::to_string(completion * 100.0f) + "%)");
}

void ProgressStorage::record_tutorial_completion(const std::string& tutorial_id, f64 total_time_minutes) {
    record_tutorial_progress(tutorial_id, 1.0f, total_time_minutes);
    
    LOG_INFO("Tutorial completed: " + tutorial_id + " (time: " + std::to_string(total_time_minutes) + " minutes)");
}

void ProgressStorage::record_quiz_result(const std::string& quiz_id, f32 score, f64 time_taken, bool passed, u32 attempt_number) {
    if (current_learner_id_.empty()) return;
    
    auto* data = get_learner_progress();
    if (!data) return;
    
    // Add to quiz history
    data->quiz_history.emplace_back(quiz_id, score, time_taken, passed);
    data->quiz_history.back().attempt_number = attempt_number;
    
    // Update best score
    auto& best_score = data->quiz_best_scores[quiz_id];
    best_score = std::max(best_score, score);
    
    update_learner_activity(current_learner_id_);
    
    LOG_INFO("Recorded quiz result: " + quiz_id + " (score: " + std::to_string(score) + 
             ", passed: " + (passed ? "yes" : "no") + ")");
}

void ProgressStorage::record_achievement_unlock(const std::string& achievement_id, u32 points) {
    if (current_learner_id_.empty()) return;
    
    auto* data = get_learner_progress();
    if (!data) return;
    
    // Check if already unlocked
    auto it = std::find(data->unlocked_achievements.begin(), data->unlocked_achievements.end(), achievement_id);
    if (it != data->unlocked_achievements.end()) {
        return; // Already unlocked
    }
    
    data->unlocked_achievements.push_back(achievement_id);
    data->achievement_unlock_times[achievement_id] = std::chrono::steady_clock::now();
    data->total_achievement_points += points;
    
    update_learner_activity(current_learner_id_);
    
    LOG_INFO("Achievement unlocked: " + achievement_id + " (+" + std::to_string(points) + " points)");
}

void ProgressStorage::record_topic_mastery(const std::string& topic, f32 mastery_level) {
    if (current_learner_id_.empty()) return;
    
    auto* data = get_learner_progress();
    if (!data) return;
    
    data->topic_mastery[topic] = std::clamp(mastery_level, 0.0f, 1.0f);
    update_learner_activity(current_learner_id_);
    
    LOG_INFO("Topic mastery updated: " + topic + " (" + std::to_string(mastery_level * 100.0f) + "%)");
}

void ProgressStorage::record_help_request(const std::string& topic) {
    if (current_learner_id_.empty()) return;
    
    auto* data = get_learner_progress();
    if (!data) return;
    
    data->help_requests[topic]++;
    update_learner_activity(current_learner_id_);
    
    LOG_INFO("Help request recorded for topic: " + topic);
}

// Session management

void ProgressStorage::start_learning_session(const std::string& session_id, const std::string& primary_activity) {
    if (current_learner_id_.empty()) return;
    
    auto* data = get_learner_progress();
    if (!data) return;
    
    data->session_history.emplace_back(session_id, primary_activity);
    data->total_sessions++;
    
    update_learner_activity(current_learner_id_);
    
    LOG_INFO("Started learning session: " + session_id + " (activity: " + primary_activity + ")");
}

void ProgressStorage::end_learning_session(const std::string& session_id) {
    if (current_learner_id_.empty()) return;
    
    auto* data = get_learner_progress();
    if (!data) return;
    
    // Find the session
    for (auto& session : data->session_history) {
        if (session.session_id == session_id) {
            session.end_time = std::chrono::steady_clock::now();
            session.duration_minutes = std::chrono::duration<f64>(session.end_time - session.start_time).count() / 60.0;
            data->total_learning_time_hours += session.duration_minutes / 60.0;
            break;
        }
    }
    
    calculate_learning_streak(*data);
    update_learner_activity(current_learner_id_);
    
    LOG_INFO("Ended learning session: " + session_id);
}

void ProgressStorage::record_session_activity(const std::string& session_id, const std::string& activity) {
    if (current_learner_id_.empty()) return;
    
    auto* data = get_learner_progress();
    if (!data) return;
    
    // Find the session and add activity
    for (auto& session : data->session_history) {
        if (session.session_id == session_id) {
            session.activities_completed.push_back(activity);
            break;
        }
    }
}

// Data retrieval

ProgressStorage::LearnerProgressData* ProgressStorage::get_learner_progress(const std::string& learner_id) {
    std::string id = learner_id.empty() ? current_learner_id_ : learner_id;
    if (id.empty()) return nullptr;
    
    auto it = learner_data_.find(id);
    return it != learner_data_.end() ? &it->second : nullptr;
}

const ProgressStorage::LearnerProgressData* ProgressStorage::get_learner_progress(const std::string& learner_id) const {
    std::string id = learner_id.empty() ? current_learner_id_ : learner_id;
    if (id.empty()) return nullptr;
    
    auto it = learner_data_.find(id);
    return it != learner_data_.end() ? &it->second : nullptr;
}

f32 ProgressStorage::get_tutorial_completion(const std::string& tutorial_id, const std::string& learner_id) const {
    const auto* data = get_learner_progress(learner_id);
    if (!data) return 0.0f;
    
    auto it = data->tutorial_completion.find(tutorial_id);
    return it != data->tutorial_completion.end() ? it->second : 0.0f;
}

f32 ProgressStorage::get_topic_mastery(const std::string& topic, const std::string& learner_id) const {
    const auto* data = get_learner_progress(learner_id);
    if (!data) return 0.0f;
    
    auto it = data->topic_mastery.find(topic);
    return it != data->topic_mastery.end() ? it->second : 0.0f;
}

std::vector<ProgressStorage::LearnerProgressData::QuizResult> ProgressStorage::get_quiz_history(const std::string& learner_id) const {
    const auto* data = get_learner_progress(learner_id);
    return data ? data->quiz_history : std::vector<LearnerProgressData::QuizResult>{};
}

std::vector<std::string> ProgressStorage::get_unlocked_achievements(const std::string& learner_id) const {
    const auto* data = get_learner_progress(learner_id);
    return data ? data->unlocked_achievements : std::vector<std::string>{};
}

// Analytics generation

ProgressStorage::LearningAnalytics ProgressStorage::generate_analytics(const std::string& learner_id) const {
    const auto* data = get_learner_progress(learner_id);
    if (!data) return LearningAnalytics{};
    
    LearningAnalytics analytics;
    
    // Basic statistics
    analytics.total_learning_time_hours = data->total_learning_time_hours;
    analytics.total_sessions = data->total_sessions;
    analytics.achievements_unlocked = static_cast<u32>(data->unlocked_achievements.size());
    analytics.consecutive_days_streak = data->consecutive_days_streak;
    
    // Calculate average session length
    if (!data->session_history.empty()) {
        f64 total_session_time = 0.0;
        u32 completed_sessions = 0;
        
        for (const auto& session : data->session_history) {
            if (session.duration_minutes > 0.0) {
                total_session_time += session.duration_minutes;
                completed_sessions++;
            }
        }
        
        if (completed_sessions > 0) {
            analytics.average_session_length_minutes = total_session_time / completed_sessions;
        }
    }
    
    // Tutorial statistics
    analytics.tutorials_completed = static_cast<u32>(
        std::count_if(data->tutorial_completion.begin(), data->tutorial_completion.end(),
                     [](const auto& pair) { return pair.second >= 1.0f; })
    );
    
    // Quiz statistics
    if (!data->quiz_history.empty()) {
        f32 total_score = 0.0f;
        u32 passed_count = 0;
        
        for (const auto& result : data->quiz_history) {
            total_score += result.score;
            if (result.passed) passed_count++;
        }
        
        analytics.average_quiz_score = total_score / static_cast<f32>(data->quiz_history.size());
        analytics.quizzes_passed = passed_count;
    }
    
    // Topic mastery
    analytics.topic_mastery_levels = data->topic_mastery;
    
    // Identify strengths and improvement areas
    for (const auto& [topic, mastery] : data->topic_mastery) {
        if (mastery >= 0.8f) {
            analytics.strong_areas.push_back(topic);
        } else if (mastery < 0.5f) {
            analytics.improvement_areas.push_back(topic);
        }
    }
    
    // Calculate overall engagement (simplified)
    if (!data->session_history.empty()) {
        f32 total_engagement = 0.0f;
        u32 engagement_count = 0;
        
        for (const auto& session : data->session_history) {
            if (session.engagement_score > 0.0f) {
                total_engagement += session.engagement_score;
                engagement_count++;
            }
        }
        
        if (engagement_count > 0) {
            analytics.overall_engagement = total_engagement / static_cast<f32>(engagement_count);
        } else {
            analytics.overall_engagement = 0.7f; // Default moderate engagement
        }
    }
    
    return analytics;
}

// Persistence operations

bool ProgressStorage::save_progress(const std::string& learner_id) {
    std::string id = learner_id.empty() ? current_learner_id_ : learner_id;
    if (id.empty()) return false;
    
    const auto* data = get_learner_progress(id);
    if (!data) return false;
    
    std::string json_data = serialize_progress_data(*data);
    std::string filepath = get_storage_path(id);
    
    if (write_file(filepath, json_data)) {
        LOG_INFO("Saved progress for learner: " + id);
        return true;
    } else {
        LOG_ERROR("Failed to save progress for learner: " + id);
        return false;
    }
}

bool ProgressStorage::load_progress(const std::string& learner_id) {
    std::string filepath = get_storage_path(learner_id);
    
    if (!file_exists(filepath)) {
        // Create new learner profile
        create_learner_profile(learner_id);
        return true;
    }
    
    std::string json_data;
    if (!read_file(filepath, json_data)) {
        LOG_ERROR("Failed to read progress file: " + filepath);
        return false;
    }
    
    try {
        LearnerProgressData data = deserialize_progress_data(json_data);
        if (validate_learner_data(data)) {
            learner_data_[learner_id] = data;
            LOG_INFO("Loaded progress for learner: " + learner_id);
            return true;
        }
    } catch (const std::exception& e) {
        LOG_ERROR("Failed to deserialize progress data: " + std::string(e.what()));
    }
    
    return false;
}

bool ProgressStorage::save_all_progress() {
    bool all_saved = true;
    
    for (const auto& [learner_id, data] : learner_data_) {
        if (!save_progress(learner_id)) {
            all_saved = false;
        }
    }
    
    LOG_INFO("Saved progress for all learners");
    return all_saved;
}

bool ProgressStorage::load_all_progress() {
    if (!ensure_storage_directory_exists()) {
        return false;
    }
    
    auto files = list_files_in_directory(storage_directory_, ".json");
    
    for (const std::string& filename : files) {
        // Extract learner ID from filename (remove .json extension)
        std::string learner_id = filename.substr(0, filename.find_last_of('.'));
        load_progress(learner_id);
    }
    
    LOG_INFO("Loaded progress for " + std::to_string(learner_data_.size()) + " learners");
    return true;
}

// Auto-save management

void ProgressStorage::update_auto_save(f64 delta_time_seconds) {
    if (!auto_save_enabled_) return;
    
    auto now = std::chrono::steady_clock::now();
    f64 elapsed_minutes = std::chrono::duration<f64>(now - last_auto_save_).count() / 60.0;
    
    if (elapsed_minutes >= auto_save_interval_minutes_) {
        save_all_progress();
        last_auto_save_ = now;
    }
}

void ProgressStorage::force_save() {
    save_all_progress();
    last_auto_save_ = std::chrono::steady_clock::now();
}

// Utility functions

std::string ProgressStorage::get_storage_path(const std::string& learner_id) const {
    return storage_directory_ + "/" + learner_id + ".json";
}

std::string ProgressStorage::get_backup_path(const std::string& backup_name) const {
    return storage_directory_ + "/backups/" + backup_name;
}

bool ProgressStorage::ensure_storage_directory_exists() const {
    return create_directory(storage_directory_) && 
           create_directory(storage_directory_ + "/backups");
}

// JSON serialization (simplified - in real implementation would use proper JSON library)

std::string ProgressStorage::serialize_progress_data(const LearnerProgressData& data) const {
    std::ostringstream json;
    
    json << "{\n";
    json << "  \"learner_id\": \"" << data.learner_id << "\",\n";
    json << "  \"display_name\": \"" << data.display_name << "\",\n";
    json << "  \"total_learning_time_hours\": " << data.total_learning_time_hours << ",\n";
    json << "  \"total_sessions\": " << data.total_sessions << ",\n";
    json << "  \"consecutive_days_streak\": " << data.consecutive_days_streak << ",\n";
    json << "  \"total_achievement_points\": " << data.total_achievement_points << ",\n";
    
    // Tutorial completion
    json << "  \"tutorial_completion\": {\n";
    bool first = true;
    for (const auto& [tutorial_id, completion] : data.tutorial_completion) {
        if (!first) json << ",\n";
        json << "    \"" << tutorial_id << "\": " << completion;
        first = false;
    }
    json << "\n  },\n";
    
    // Topic mastery
    json << "  \"topic_mastery\": {\n";
    first = true;
    for (const auto& [topic, mastery] : data.topic_mastery) {
        if (!first) json << ",\n";
        json << "    \"" << topic << "\": " << mastery;
        first = false;
    }
    json << "\n  },\n";
    
    // Unlocked achievements
    json << "  \"unlocked_achievements\": [\n";
    first = true;
    for (const auto& achievement : data.unlocked_achievements) {
        if (!first) json << ",\n";
        json << "    \"" << achievement << "\"";
        first = false;
    }
    json << "\n  ]\n";
    
    json << "}";
    
    return json.str();
}

ProgressStorage::LearnerProgressData ProgressStorage::deserialize_progress_data(const std::string& json_data) const {
    // Simplified JSON parsing - in real implementation would use proper JSON library
    LearnerProgressData data;
    
    // This is a placeholder implementation
    // Real implementation would parse JSON properly
    data.learner_id = "parsed_from_json";
    data.display_name = "Parsed Learner";
    data.total_learning_time_hours = 0.0;
    
    return data;
}

// File operations (simplified implementations)

bool ProgressStorage::write_file(const std::string& filepath, const std::string& content) const {
    std::ofstream file(filepath);
    if (file.is_open()) {
        file << content;
        return true;
    }
    return false;
}

bool ProgressStorage::read_file(const std::string& filepath, std::string& content) const {
    std::ifstream file(filepath);
    if (file.is_open()) {
        content.assign((std::istreambuf_iterator<char>(file)),
                       std::istreambuf_iterator<char>());
        return true;
    }
    return false;
}

bool ProgressStorage::file_exists(const std::string& filepath) const {
    return std::filesystem::exists(filepath);
}

bool ProgressStorage::create_directory(const std::string& dir_path) const {
    if (std::filesystem::exists(dir_path)) {
        return std::filesystem::is_directory(dir_path);
    }
    
    return std::filesystem::create_directories(dir_path);
}

std::vector<std::string> ProgressStorage::list_files_in_directory(const std::string& dir_path, const std::string& extension) const {
    std::vector<std::string> files;
    
    if (!std::filesystem::exists(dir_path)) {
        return files;
    }
    
    for (const auto& entry : std::filesystem::directory_iterator(dir_path)) {
        if (entry.is_regular_file()) {
            std::string filename = entry.path().filename().string();
            if (extension.empty() || filename.ends_with(extension)) {
                files.push_back(filename);
            }
        }
    }
    
    return files;
}

// Internal helpers

void ProgressStorage::update_learner_activity(const std::string& learner_id) {
    auto it = learner_data_.find(learner_id);
    if (it != learner_data_.end()) {
        it->second.last_active = std::chrono::steady_clock::now();
    }
}

void ProgressStorage::calculate_learning_streak(LearnerProgressData& data) {
    auto now = std::chrono::steady_clock::now();
    auto today = std::chrono::time_point_cast<std::chrono::days>(now);
    auto last_streak_day = std::chrono::time_point_cast<std::chrono::days>(data.last_streak_day);
    
    auto days_since_last = std::chrono::duration_cast<std::chrono::days>(today - last_streak_day);
    
    if (days_since_last.count() == 1) {
        // Consecutive day
        data.consecutive_days_streak++;
        data.last_streak_day = now;
    } else if (days_since_last.count() == 0) {
        // Same day, maintain streak
        data.last_streak_day = now;
    } else {
        // Streak broken
        data.consecutive_days_streak = 1;
        data.last_streak_day = now;
    }
}

bool ProgressStorage::validate_learner_data(const LearnerProgressData& data) const {
    return !data.learner_id.empty() && 
           data.total_learning_time_hours >= 0.0 &&
           data.total_sessions >= 0;
}

// Stub implementations for remaining methods
void ProgressStorage::create_backup() {
    LOG_INFO("Creating backup of all progress data");
}

bool ProgressStorage::restore_from_backup(const std::string& backup_filename) {
    LOG_INFO("Restoring from backup: " + backup_filename);
    return false;
}

std::vector<std::string> ProgressStorage::list_available_backups() const {
    return list_files_in_directory(storage_directory_ + "/backups", ".backup");
}

bool ProgressStorage::export_learner_data(const std::string& learner_id, const std::string& filename) const {
    LOG_INFO("Exporting learner data: " + learner_id + " to " + filename);
    return false;
}

// Global access functions

ProgressStorage& get_progress_storage() {
    if (!g_progress_storage) {
        g_progress_storage = std::make_unique<ProgressStorage>();
    }
    return *g_progress_storage;
}

void set_progress_storage(std::unique_ptr<ProgressStorage> storage) {
    g_progress_storage = std::move(storage);
}

// Convenience functions implementation

namespace progress_tracking {

static std::string current_session_id;

void record_tutorial_start(const std::string& tutorial_id) {
    auto& storage = get_progress_storage();
    storage.record_tutorial_progress(tutorial_id, 0.0f, 0.0);
}

void record_tutorial_step_completion(const std::string& tutorial_id, const std::string& step_id, f64 time_spent) {
    // In a real implementation, this would calculate partial completion
    auto& storage = get_progress_storage();
    storage.record_tutorial_progress(tutorial_id, 0.5f, time_spent);
}

void record_tutorial_completion(const std::string& tutorial_id, f64 total_time) {
    auto& storage = get_progress_storage();
    storage.record_tutorial_completion(tutorial_id, total_time);
}

void record_quiz_attempt(const std::string& quiz_id, f32 score, bool passed) {
    auto& storage = get_progress_storage();
    storage.record_quiz_result(quiz_id, score, 5.0, passed); // Assume 5 minutes
}

void unlock_achievement(const std::string& achievement_id) {
    auto& storage = get_progress_storage();
    storage.record_achievement_unlock(achievement_id);
}

std::string start_session(const std::string& activity_type) {
    current_session_id = "session_" + std::to_string(std::time(nullptr));
    auto& storage = get_progress_storage();
    storage.start_learning_session(current_session_id, activity_type);
    return current_session_id;
}

void end_current_session() {
    if (!current_session_id.empty()) {
        auto& storage = get_progress_storage();
        storage.end_learning_session(current_session_id);
        current_session_id.clear();
    }
}

void record_activity(const std::string& activity_name) {
    if (!current_session_id.empty()) {
        auto& storage = get_progress_storage();
        storage.record_session_activity(current_session_id, activity_name);
    }
}

f32 get_overall_progress() {
    auto& storage = get_progress_storage();
    const auto* data = storage.get_learner_progress();
    if (!data) return 0.0f;
    
    // Simple calculation: average of all tutorial completions
    if (data->tutorial_completion.empty()) return 0.0f;
    
    f32 total = 0.0f;
    for (const auto& [tutorial_id, completion] : data->tutorial_completion) {
        total += completion;
    }
    
    return total / static_cast<f32>(data->tutorial_completion.size());
}

std::vector<std::string> get_struggling_topics() {
    auto& storage = get_progress_storage();
    const auto* data = storage.get_learner_progress();
    if (!data) return {};
    
    std::vector<std::string> struggling;
    for (const auto& [topic, mastery] : data->topic_mastery) {
        if (mastery < 0.5f) {
            struggling.push_back(topic);
        }
    }
    
    return struggling;
}

std::vector<std::string> get_mastered_topics() {
    auto& storage = get_progress_storage();
    const auto* data = storage.get_learner_progress();
    if (!data) return {};
    
    std::vector<std::string> mastered;
    for (const auto& [topic, mastery] : data->topic_mastery) {
        if (mastery >= 0.8f) {
            mastered.push_back(topic);
        }
    }
    
    return mastered;
}

u32 get_current_streak() {
    auto& storage = get_progress_storage();
    const auto* data = storage.get_learner_progress();
    return data ? data->consecutive_days_streak : 0;
}

void save_progress() {
    auto& storage = get_progress_storage();
    storage.save_all_progress();
}

void backup_progress() {
    auto& storage = get_progress_storage();
    storage.create_backup();
}

void export_progress(const std::string& filename) {
    auto& storage = get_progress_storage();
    storage.export_all_data(filename);
}

} // namespace progress_tracking

} // namespace ecscope::learning