#pragma once

/**
 * @file progress_storage.hpp
 * @brief Progress Tracking System with Persistent Storage
 * 
 * This system provides comprehensive progress tracking and persistent storage
 * for all learning activities in the Interactive Learning System.
 * 
 * Features:
 * - JSON-based progress serialization
 * - Automatic backup and recovery
 * - Cross-session progress persistence
 * - Learning analytics storage
 * - Multi-user profile management
 * 
 * @author ECScope Educational Framework
 * @date 2024
 */

#include "core/types.hpp"
#include <string>
#include <vector>
#include <unordered_map>
#include <chrono>
#include <fstream>

namespace ecscope::learning {

/**
 * @brief Progress tracking and storage system
 */
class ProgressStorage {
public:
    struct LearnerProgressData {
        std::string learner_id;
        std::string display_name;
        std::chrono::steady_clock::time_point created_at;
        std::chrono::steady_clock::time_point last_active;
        
        // Tutorial progress
        std::unordered_map<std::string, f32> tutorial_completion; // tutorial_id -> progress (0-1)
        std::unordered_map<std::string, f64> tutorial_time_spent; // tutorial_id -> minutes
        std::unordered_map<std::string, u32> tutorial_attempts;   // tutorial_id -> attempt count
        
        // Quiz results
        struct QuizResult {
            std::string quiz_id;
            f32 score{0.0f};
            f64 time_taken{0.0};
            u32 attempt_number{1};
            std::chrono::steady_clock::time_point completed_at;
            bool passed{false};
            
            QuizResult() = default;
            QuizResult(const std::string& id, f32 s, f64 time, bool p)
                : quiz_id(id), score(s), time_taken(time), passed(p) {
                completed_at = std::chrono::steady_clock::now();
            }
        };
        
        std::vector<QuizResult> quiz_history;
        std::unordered_map<std::string, f32> quiz_best_scores; // quiz_id -> best score
        
        // Achievement tracking
        std::vector<std::string> unlocked_achievements;
        std::unordered_map<std::string, std::chrono::steady_clock::time_point> achievement_unlock_times;
        u32 total_achievement_points{0};
        
        // Learning patterns and preferences
        std::unordered_map<std::string, f32> topic_mastery;     // topic -> mastery level (0-1)
        std::unordered_map<std::string, u32> topic_time_spent;  // topic -> minutes
        std::unordered_map<std::string, u32> help_requests;     // topic -> help count
        std::vector<std::string> preferred_learning_styles;
        
        // Session tracking
        struct LearningSession {
            std::string session_id;
            std::chrono::steady_clock::time_point start_time;
            std::chrono::steady_clock::time_point end_time;
            f64 duration_minutes{0.0};
            std::string primary_activity; // "tutorial", "quiz", "practice", etc.
            std::vector<std::string> activities_completed;
            f32 engagement_score{0.0f}; // 0-1 based on interaction patterns
            
            LearningSession() = default;
            LearningSession(const std::string& id, const std::string& activity)
                : session_id(id), primary_activity(activity) {
                start_time = std::chrono::steady_clock::now();
            }
        };
        
        std::vector<LearningSession> session_history;
        
        // Statistics
        f64 total_learning_time_hours{0.0};
        u32 total_sessions{0};
        u32 consecutive_days_streak{0};
        std::chrono::steady_clock::time_point last_streak_day;
        
        LearnerProgressData() = default;
        LearnerProgressData(const std::string& id, const std::string& name = "")
            : learner_id(id), display_name(name.empty() ? id : name) {
            created_at = std::chrono::steady_clock::now();
            last_active = created_at;
        }
    };
    
private:
    std::string storage_directory_;
    std::unordered_map<std::string, LearnerProgressData> learner_data_;
    std::string current_learner_id_;
    
    // Auto-save settings
    bool auto_save_enabled_{true};
    f64 auto_save_interval_minutes_{5.0};
    std::chrono::steady_clock::time_point last_auto_save_;
    
    // Backup settings
    bool backup_enabled_{true};
    u32 max_backups_{10};
    
public:
    ProgressStorage() = default;
    explicit ProgressStorage(const std::string& storage_dir) : storage_directory_(storage_dir) {}
    
    // Initialization and configuration
    void initialize(const std::string& storage_directory);
    void shutdown();
    void set_auto_save_interval(f64 minutes) { auto_save_interval_minutes_ = minutes; }
    void enable_auto_save(bool enabled) { auto_save_enabled_ = enabled; }
    void enable_backups(bool enabled) { backup_enabled_ = enabled; }
    void set_max_backups(u32 max_backups) { max_backups_ = max_backups; }
    
    // Learner profile management
    void create_learner_profile(const std::string& learner_id, const std::string& display_name = "");
    void delete_learner_profile(const std::string& learner_id);
    void set_current_learner(const std::string& learner_id);
    std::string get_current_learner() const { return current_learner_id_; }
    std::vector<std::string> get_all_learner_ids() const;
    bool learner_exists(const std::string& learner_id) const;
    
    // Progress tracking
    void record_tutorial_progress(const std::string& tutorial_id, f32 completion, f64 time_spent_minutes);
    void record_tutorial_completion(const std::string& tutorial_id, f64 total_time_minutes);
    void record_quiz_result(const std::string& quiz_id, f32 score, f64 time_taken, bool passed, u32 attempt_number = 1);
    void record_achievement_unlock(const std::string& achievement_id, u32 points = 10);
    void record_topic_mastery(const std::string& topic, f32 mastery_level);
    void record_help_request(const std::string& topic);
    
    // Session management
    void start_learning_session(const std::string& session_id, const std::string& primary_activity);
    void end_learning_session(const std::string& session_id);
    void record_session_activity(const std::string& session_id, const std::string& activity);
    void update_engagement_score(const std::string& session_id, f32 engagement);
    
    // Data retrieval
    LearnerProgressData* get_learner_progress(const std::string& learner_id = "");
    const LearnerProgressData* get_learner_progress(const std::string& learner_id = "") const;
    f32 get_tutorial_completion(const std::string& tutorial_id, const std::string& learner_id = "") const;
    f32 get_topic_mastery(const std::string& topic, const std::string& learner_id = "") const;
    std::vector<LearnerProgressData::QuizResult> get_quiz_history(const std::string& learner_id = "") const;
    std::vector<std::string> get_unlocked_achievements(const std::string& learner_id = "") const;
    
    // Analytics and statistics
    struct LearningAnalytics {
        f64 total_learning_time_hours{0.0};
        f64 average_session_length_minutes{0.0};
        u32 total_sessions{0};
        u32 tutorials_completed{0};
        u32 quizzes_passed{0};
        f32 average_quiz_score{0.0f};
        u32 achievements_unlocked{0};
        u32 consecutive_days_streak{0};
        f32 overall_engagement{0.0f};
        std::unordered_map<std::string, f32> topic_mastery_levels;
        std::vector<std::string> strong_areas;
        std::vector<std::string> improvement_areas;
        
        // Time-based patterns
        std::vector<std::pair<u32, f64>> daily_activity; // hour -> average minutes
        std::vector<std::pair<std::string, f64>> weekly_pattern; // day -> minutes
    };
    
    LearningAnalytics generate_analytics(const std::string& learner_id = "") const;
    LearningAnalytics generate_comparative_analytics(const std::vector<std::string>& learner_ids) const;
    
    // Persistence operations
    bool save_progress(const std::string& learner_id = "");
    bool load_progress(const std::string& learner_id);
    bool save_all_progress();
    bool load_all_progress();
    
    // Backup operations
    void create_backup();
    bool restore_from_backup(const std::string& backup_filename);
    std::vector<std::string> list_available_backups() const;
    void cleanup_old_backups();
    
    // Import/export operations
    bool export_learner_data(const std::string& learner_id, const std::string& filename) const;
    bool import_learner_data(const std::string& filename);
    bool export_all_data(const std::string& filename) const;
    void clear_all_data();
    
    // Auto-save management
    void update_auto_save(f64 delta_time_seconds);
    void force_save();
    
    // Utility functions
    std::string get_storage_path(const std::string& learner_id) const;
    std::string get_backup_path(const std::string& backup_name) const;
    bool ensure_storage_directory_exists() const;
    
private:
    // JSON serialization helpers
    std::string serialize_progress_data(const LearnerProgressData& data) const;
    LearnerProgressData deserialize_progress_data(const std::string& json_data) const;
    std::string serialize_time_point(const std::chrono::steady_clock::time_point& time_point) const;
    std::chrono::steady_clock::time_point deserialize_time_point(const std::string& time_string) const;
    
    // File operations
    bool write_file(const std::string& filepath, const std::string& content) const;
    bool read_file(const std::string& filepath, std::string& content) const;
    bool file_exists(const std::string& filepath) const;
    bool create_directory(const std::string& dir_path) const;
    std::vector<std::string> list_files_in_directory(const std::string& dir_path, const std::string& extension = ".json") const;
    
    // Data validation
    bool validate_learner_data(const LearnerProgressData& data) const;
    void sanitize_learner_data(LearnerProgressData& data) const;
    
    // Internal helpers
    void update_learner_activity(const std::string& learner_id);
    void calculate_learning_streak(LearnerProgressData& data);
    f32 calculate_overall_progress(const LearnerProgressData& data) const;
    void merge_session_data(LearnerProgressData& data);
};

/**
 * @brief Global progress storage instance
 */
ProgressStorage& get_progress_storage();
void set_progress_storage(std::unique_ptr<ProgressStorage> storage);

/**
 * @brief Convenience functions for progress tracking
 */
namespace progress_tracking {
    
    // Quick progress recording
    void record_tutorial_start(const std::string& tutorial_id);
    void record_tutorial_step_completion(const std::string& tutorial_id, const std::string& step_id, f64 time_spent);
    void record_tutorial_completion(const std::string& tutorial_id, f64 total_time);
    void record_quiz_attempt(const std::string& quiz_id, f32 score, bool passed);
    void unlock_achievement(const std::string& achievement_id);
    
    // Session management shortcuts
    std::string start_session(const std::string& activity_type);
    void end_current_session();
    void record_activity(const std::string& activity_name);
    
    // Analytics shortcuts
    f32 get_overall_progress();
    std::vector<std::string> get_struggling_topics();
    std::vector<std::string> get_mastered_topics();
    u32 get_current_streak();
    
    // Data management shortcuts
    void save_progress();
    void backup_progress();
    void export_progress(const std::string& filename);
}

} // namespace ecscope::learning