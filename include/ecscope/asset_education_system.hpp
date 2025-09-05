#pragma once

/**
 * @file asset_education_system.hpp
 * @brief Educational Visualization and Learning System for ECScope Asset Pipeline
 * 
 * This system provides comprehensive educational features for teaching asset
 * pipeline concepts, performance optimization, and best practices through
 * interactive visualizations and guided learning experiences.
 * 
 * Key Features:
 * - Interactive asset processing visualization
 * - Real-time performance metrics and analysis
 * - Guided tutorials and learning paths
 * - Asset optimization recommendations
 * - Integration with learning management system
 * - Performance profiling and educational insights
 * 
 * Educational Value:
 * - Demonstrates asset pipeline architecture
 * - Shows performance implications of different strategies
 * - Illustrates memory management techniques
 * - Provides hands-on optimization exercises
 * - Teaches industry best practices
 * 
 * @author ECScope Educational ECS Framework - Asset Pipeline
 * @date 2025
 */

#include "asset_pipeline.hpp"
#include "asset_loader.hpp"
#include "asset_hot_reload_manager.hpp"
#include "learning/tutorial_system.hpp"
#include "core/types.hpp"
#include <vector>
#include <string>
#include <memory>
#include <unordered_map>
#include <functional>
#include <chrono>
#include <optional>

namespace ecscope::assets::education {

//=============================================================================
// Forward Declarations
//=============================================================================

class AssetEducationSystem;
class PerformanceProfiler;
class OptimizationAnalyzer;
class InteractiveTutorialManager;

//=============================================================================
// Educational Metrics and Analytics
//=============================================================================

/**
 * @brief Comprehensive metrics for educational analysis
 */
struct EducationalMetrics {
    // Performance metrics
    struct PerformanceData {
        f64 total_import_time{0.0};
        f64 memory_allocation_time{0.0};
        f64 file_io_time{0.0};
        f64 processing_time{0.0};
        f64 optimization_time{0.0};
        
        usize peak_memory_usage{0};
        usize final_memory_usage{0};
        f32 memory_efficiency{1.0f};
        
        u32 cache_hits{0};
        u32 cache_misses{0};
        f32 cache_efficiency{1.0f};
        
        std::vector<std::pair<std::string, f64>> step_timings; // Step name, time
    } performance;
    
    // Quality metrics
    struct QualityAssessment {
        f32 import_quality_score{1.0f};      // 0.0 = poor, 1.0 = excellent
        f32 compression_efficiency{1.0f};
        f32 format_appropriateness{1.0f};
        
        std::vector<std::string> quality_issues;
        std::vector<std::string> optimization_opportunities;
        std::vector<std::string> best_practices_followed;
    } quality;
    
    // Learning metrics
    struct LearningData {
        std::vector<std::string> concepts_demonstrated;
        std::vector<std::string> techniques_used;
        std::string complexity_level; // "Beginner", "Intermediate", "Advanced"
        f32 educational_value{0.5f}; // 0.0 = low, 1.0 = high educational value
        
        std::vector<std::string> suggested_exercises;
        std::vector<std::string> related_topics;
        std::string learning_objective;
    } learning;
    
    // Asset-specific metrics
    std::unordered_map<std::string, f64> custom_metrics; // Flexible metrics storage
    
    // Context information
    AssetType asset_type{AssetType::Unknown};
    std::string asset_name;
    std::filesystem::path source_path;
    std::chrono::high_resolution_clock::time_point timestamp;
    std::string student_context; // Learning context for personalization
};

/**
 * @brief Performance profiler for educational analysis
 */
class PerformanceProfiler {
public:
    /**
     * @brief Profiling session for tracking operations
     */
    struct ProfilingSession {
        std::string session_id;
        AssetID asset_id{INVALID_ASSET_ID};
        AssetType asset_type{AssetType::Unknown};
        std::chrono::high_resolution_clock::time_point start_time;
        std::chrono::high_resolution_clock::time_point end_time;
        
        // Detailed timing data
        std::vector<std::pair<std::string, std::chrono::high_resolution_clock::time_point>> events;
        std::unordered_map<std::string, f64> phase_durations;
        
        // Memory tracking
        std::vector<std::pair<std::chrono::high_resolution_clock::time_point, usize>> memory_samples;
        usize peak_memory{0};
        
        // Custom markers for educational insights
        std::vector<std::string> educational_markers;
        std::unordered_map<std::string, std::string> annotations;
        
        bool is_active{false};
    };
    
private:
    std::unordered_map<std::string, ProfilingSession> active_sessions_;
    std::mutex sessions_mutex_;
    
    // Global performance tracking
    std::vector<EducationalMetrics> historical_metrics_;
    mutable std::shared_mutex metrics_mutex_;
    constexpr static usize MAX_HISTORICAL_METRICS = 1000;
    
    // Memory tracking
    memory::MemoryTracker* memory_tracker_{nullptr};
    std::thread memory_sampling_thread_;
    std::atomic<bool> memory_sampling_active_{false};
    
public:
    explicit PerformanceProfiler(memory::MemoryTracker* tracker = nullptr);
    ~PerformanceProfiler();
    
    // Session management
    std::string start_profiling_session(AssetID asset_id, AssetType type, const std::string& context = "");
    void end_profiling_session(const std::string& session_id);
    void cancel_profiling_session(const std::string& session_id);
    
    // Event tracking
    void record_event(const std::string& session_id, const std::string& event_name);
    void record_phase_start(const std::string& session_id, const std::string& phase_name);
    void record_phase_end(const std::string& session_id, const std::string& phase_name);
    void add_educational_marker(const std::string& session_id, const std::string& marker);
    void add_annotation(const std::string& session_id, const std::string& key, const std::string& value);
    
    // Analysis
    EducationalMetrics generate_metrics(const std::string& session_id);
    std::vector<EducationalMetrics> get_historical_metrics(AssetType type = AssetType::Unknown, 
                                                          usize max_count = 100) const;
    
    // Educational insights
    std::string generate_performance_report(const std::string& session_id) const;
    std::vector<std::string> identify_performance_bottlenecks(const std::string& session_id) const;
    std::vector<std::string> suggest_optimizations(const std::string& session_id) const;
    
    // Visualization data
    struct VisualizationData {
        std::vector<std::string> phase_names;
        std::vector<f64> phase_durations;
        std::vector<std::pair<f64, usize>> memory_timeline; // Time, memory usage
        std::vector<std::string> critical_events;
        f64 total_duration{0.0};
    };
    
    VisualizationData get_visualization_data(const std::string& session_id) const;
    
    // Statistics
    struct ProfilerStatistics {
        u32 total_sessions{0};
        u32 active_sessions{0};
        f64 average_session_duration{0.0};
        usize total_metrics_collected{0};
        
        std::unordered_map<AssetType, u32> sessions_by_type;
        std::unordered_map<AssetType, f64> average_duration_by_type;
    };
    
    ProfilerStatistics get_statistics() const;
    void clear_historical_data();
    
private:
    void memory_sampling_worker();
    void sample_memory_for_session(ProfilingSession& session);
    std::string generate_session_id() const;
};

/**
 * @brief Optimization analyzer for educational recommendations
 */
class OptimizationAnalyzer {
public:
    /**
     * @brief Optimization recommendation
     */
    struct OptimizationRecommendation {
        enum class Priority {
            Low, Medium, High, Critical
        } priority{Priority::Medium};
        
        enum class Category {
            Performance, Memory, Quality, BestPractice, Educational
        } category{Category::Performance};
        
        std::string title;
        std::string description;
        std::string detailed_explanation;
        
        // Impact estimates
        f32 performance_impact{0.0f};   // Expected performance improvement (0-1)
        f32 memory_impact{0.0f};        // Expected memory reduction (0-1)
        f32 implementation_effort{0.5f}; // Implementation difficulty (0-1)
        
        // Educational value
        f32 learning_value{0.5f};       // Educational value of this optimization
        std::vector<std::string> concepts_taught;
        std::string tutorial_link;      // Link to relevant tutorial
        
        // Implementation guidance
        std::vector<std::string> implementation_steps;
        std::string code_example;
        std::vector<std::string> prerequisites;
        std::vector<std::string> resources;
    };
    
    /**
     * @brief Analysis result with recommendations
     */
    struct AnalysisResult {
        AssetID analyzed_asset{INVALID_ASSET_ID};
        AssetType asset_type{AssetType::Unknown};
        std::string asset_name;
        
        // Overall assessment
        f32 optimization_score{1.0f};   // How well optimized (0 = poor, 1 = excellent)
        std::string performance_grade{"A"}; // Letter grade A-F
        std::string summary;
        
        // Recommendations by priority
        std::vector<OptimizationRecommendation> critical_recommendations;
        std::vector<OptimizationRecommendation> high_priority_recommendations;
        std::vector<OptimizationRecommendation> medium_priority_recommendations;
        std::vector<OptimizationRecommendation> low_priority_recommendations;
        
        // Educational insights
        std::vector<std::string> positive_aspects;  // What was done well
        std::vector<std::string> learning_opportunities;
        std::string next_learning_steps;
        
        std::chrono::high_resolution_clock::time_point analysis_time;
    };
    
private:
    // Analysis rules and patterns
    std::vector<std::function<std::vector<OptimizationRecommendation>(const EducationalMetrics&)>> analysis_rules_;
    
    // Knowledge base for educational content
    std::unordered_map<std::string, std::string> concept_explanations_;
    std::unordered_map<std::string, std::vector<std::string>> concept_tutorials_;
    
    // Historical analysis for learning
    std::vector<AnalysisResult> historical_analyses_;
    mutable std::shared_mutex analyses_mutex_;
    
public:
    OptimizationAnalyzer();
    ~OptimizationAnalyzer() = default;
    
    // Analysis operations
    AnalysisResult analyze_asset(const EducationalMetrics& metrics);
    AnalysisResult analyze_asset_comprehensive(AssetID asset_id, const AssetMetadata& metadata,
                                             const EducationalMetrics& metrics);
    
    // Batch analysis
    std::vector<AnalysisResult> analyze_multiple_assets(const std::vector<EducationalMetrics>& metrics);
    
    // Recommendation system
    std::vector<OptimizationRecommendation> get_recommendations_by_category(
        const EducationalMetrics& metrics, 
        OptimizationRecommendation::Category category
    );
    
    std::vector<OptimizationRecommendation> get_recommendations_for_learning_level(
        const EducationalMetrics& metrics,
        const std::string& learning_level
    );
    
    // Educational features
    std::string generate_optimization_tutorial(const AnalysisResult& result) const;
    std::string explain_recommendation(const OptimizationRecommendation& recommendation) const;
    std::vector<std::string> suggest_learning_path(const AnalysisResult& result) const;
    
    // Knowledge base management
    void add_concept_explanation(const std::string& concept, const std::string& explanation);
    void add_tutorial_link(const std::string& concept, const std::string& tutorial_link);
    void update_knowledge_base_from_file(const std::filesystem::path& knowledge_file);
    
    // Historical analysis
    std::vector<AnalysisResult> get_historical_analyses(AssetType type = AssetType::Unknown,
                                                       usize max_count = 50) const;
    std::string generate_improvement_trends_report() const;
    
private:
    void initialize_analysis_rules();
    void initialize_knowledge_base();
    
    // Specific analyzers
    std::vector<OptimizationRecommendation> analyze_texture_performance(const EducationalMetrics& metrics);
    std::vector<OptimizationRecommendation> analyze_model_optimization(const EducationalMetrics& metrics);
    std::vector<OptimizationRecommendation> analyze_audio_efficiency(const EducationalMetrics& metrics);
    std::vector<OptimizationRecommendation> analyze_shader_performance(const EducationalMetrics& metrics);
    std::vector<OptimizationRecommendation> analyze_memory_usage(const EducationalMetrics& metrics);
    std::vector<OptimizationRecommendation> analyze_loading_patterns(const EducationalMetrics& metrics);
    
    // Utility functions
    f32 calculate_optimization_score(const EducationalMetrics& metrics);
    std::string determine_performance_grade(f32 score);
    std::string generate_summary(const std::vector<OptimizationRecommendation>& recommendations);
};

//=============================================================================
// Interactive Tutorial System
//=============================================================================

/**
 * @brief Interactive tutorial manager for asset pipeline education
 */
class InteractiveTutorialManager {
public:
    /**
     * @brief Interactive exercise for hands-on learning
     */
    struct InteractiveExercise {
        std::string id;
        std::string title;
        std::string description;
        std::string objective;
        
        // Exercise configuration
        AssetType target_asset_type{AssetType::Unknown};
        std::filesystem::path sample_asset_path;
        std::vector<std::string> required_tools;
        
        // Steps and validation
        std::vector<learning::TutorialStep> steps;
        std::function<bool(const EducationalMetrics&)> success_validator;
        std::function<std::string(const EducationalMetrics&)> feedback_generator;
        
        // Educational context
        std::vector<std::string> prerequisites;
        std::vector<std::string> learning_outcomes;
        std::string difficulty_level;
        f64 estimated_duration_minutes{30.0};
        
        // Scoring and assessment
        u32 max_score{100};
        std::function<u32(const EducationalMetrics&)> score_calculator;
    };
    
    /**
     * @brief Learning path for progressive skill development
     */
    struct LearningPath {
        std::string path_id;
        std::string title;
        std::string description;
        std::vector<std::string> exercise_ids; // Ordered sequence of exercises
        
        // Progress tracking
        std::unordered_map<std::string, bool> exercise_completion;
        std::unordered_map<std::string, u32> exercise_scores;
        f32 overall_progress{0.0f};
        
        // Adaptive features
        std::string current_skill_level;
        std::vector<std::string> mastered_concepts;
        std::vector<std::string> struggling_areas;
        
        bool is_completed() const {
            for (const auto& exercise_id : exercise_ids) {
                if (!exercise_completion.count(exercise_id) || !exercise_completion.at(exercise_id)) {
                    return false;
                }
            }
            return true;
        }
        
        f32 calculate_progress() const {
            if (exercise_ids.empty()) return 1.0f;
            u32 completed = 0;
            for (const auto& exercise_id : exercise_ids) {
                if (exercise_completion.count(exercise_id) && exercise_completion.at(exercise_id)) {
                    ++completed;
                }
            }
            return static_cast<f32>(completed) / exercise_ids.size();
        }
    };
    
private:
    // Exercise registry
    std::unordered_map<std::string, InteractiveExercise> exercises_;
    std::unordered_map<std::string, LearningPath> learning_paths_;
    
    // Student progress tracking
    std::unordered_map<std::string, LearningPath> student_progress_; // student_id -> progress
    
    // Integration with other systems
    AssetLoader* asset_loader_{nullptr};
    PerformanceProfiler* profiler_{nullptr};
    OptimizationAnalyzer* analyzer_{nullptr};
    
    // Active sessions
    std::unordered_map<std::string, std::string> active_sessions_; // session_id -> exercise_id
    
public:
    InteractiveTutorialManager() = default;
    ~InteractiveTutorialManager() = default;
    
    // System integration
    void set_asset_loader(AssetLoader* loader) { asset_loader_ = loader; }
    void set_performance_profiler(PerformanceProfiler* profiler) { profiler_ = profiler; }
    void set_optimization_analyzer(OptimizationAnalyzer* analyzer) { analyzer_ = analyzer; }
    
    // Exercise management
    void register_exercise(const InteractiveExercise& exercise);
    void register_learning_path(const LearningPath& path);
    std::vector<std::string> get_available_exercises() const;
    std::vector<std::string> get_available_learning_paths() const;
    
    // Tutorial execution
    std::string start_exercise(const std::string& exercise_id, const std::string& student_id);
    bool complete_exercise_step(const std::string& session_id, const std::string& step_id);
    void submit_exercise_result(const std::string& session_id, const EducationalMetrics& metrics);
    void end_exercise_session(const std::string& session_id);
    
    // Progress tracking
    LearningPath get_student_progress(const std::string& student_id, const std::string& path_id);
    void update_student_progress(const std::string& student_id, const std::string& path_id,
                                const std::string& exercise_id, bool completed, u32 score);
    
    // Adaptive learning
    std::vector<std::string> recommend_exercises(const std::string& student_id);
    std::string recommend_next_exercise(const std::string& student_id, const std::string& path_id);
    void adjust_difficulty_based_on_performance(const std::string& student_id, const EducationalMetrics& metrics);
    
    // Content generation
    std::string generate_exercise_report(const std::string& session_id) const;
    std::string generate_progress_report(const std::string& student_id) const;
    std::vector<std::string> generate_personalized_tips(const std::string& student_id);
    
    // Built-in exercises
    void initialize_default_exercises();
    
private:
    void create_texture_optimization_exercises();
    void create_model_processing_exercises();
    void create_audio_analysis_exercises();
    void create_shader_compilation_exercises();
    void create_performance_profiling_exercises();
    
    // Exercise helpers
    std::string generate_session_id() const;
    InteractiveExercise* get_exercise(const std::string& exercise_id);
    bool validate_exercise_prerequisites(const InteractiveExercise& exercise, const std::string& student_id);
};

//=============================================================================
// Main Educational System
//=============================================================================

/**
 * @brief Comprehensive educational system for asset pipeline learning
 */
class AssetEducationSystem {
public:
    /**
     * @brief Educational system configuration
     */
    struct EducationConfig {
        bool enable_performance_profiling{true};
        bool enable_optimization_analysis{true};
        bool enable_interactive_tutorials{true};
        bool enable_real_time_feedback{true};
        
        // Visualization settings
        bool enable_visual_debugging{true};
        bool enable_performance_graphs{true};
        bool enable_memory_visualization{true};
        
        // Learning management
        bool track_student_progress{true};
        bool personalize_content{true};
        bool generate_reports{true};
        f64 analysis_update_interval_seconds{1.0};
        
        // Integration settings
        std::string learning_management_system_url;
        std::string student_database_path;
        bool export_metrics_to_lms{false};
    };
    
private:
    // Core components
    std::unique_ptr<PerformanceProfiler> profiler_;
    std::unique_ptr<OptimizationAnalyzer> analyzer_;
    std::unique_ptr<InteractiveTutorialManager> tutorial_manager_;
    
    // System integration
    AssetRegistry* asset_registry_{nullptr};
    AssetLoader* asset_loader_{nullptr};
    AssetHotReloadManager* hot_reload_manager_{nullptr};
    learning::TutorialManager* learning_system_{nullptr};
    
    // Configuration
    EducationConfig config_;
    
    // Real-time monitoring
    std::atomic<bool> monitoring_active_{false};
    std::thread monitoring_thread_;
    std::vector<std::function<void(const EducationalMetrics&)>> metrics_observers_;
    
    // Student session management
    std::unordered_map<std::string, std::string> active_student_sessions_; // student_id -> session_id
    std::mutex sessions_mutex_;
    
public:
    explicit AssetEducationSystem(const EducationConfig& config = EducationConfig{});
    ~AssetEducationSystem();
    
    // System integration
    void integrate_with_asset_registry(AssetRegistry* registry);
    void integrate_with_asset_loader(AssetLoader* loader);
    void integrate_with_hot_reload_manager(AssetHotReloadManager* hot_reload);
    void integrate_with_learning_system(learning::TutorialManager* learning);
    
    // Initialization
    bool initialize();
    void shutdown();
    
    // Educational monitoring
    void start_monitoring();
    void stop_monitoring();
    std::string start_educational_session(AssetID asset_id, const std::string& student_id,
                                        const std::string& learning_context = "");
    void end_educational_session(const std::string& session_id);
    
    // Real-time analysis
    void analyze_asset_operation(AssetID asset_id, const std::string& operation_type);
    EducationalMetrics get_current_metrics(AssetID asset_id) const;
    OptimizationAnalyzer::AnalysisResult get_current_analysis(AssetID asset_id) const;
    
    // Interactive learning
    std::vector<std::string> get_available_exercises() const;
    std::string start_interactive_exercise(const std::string& exercise_id, const std::string& student_id);
    void submit_exercise_result(const std::string& session_id, const EducationalMetrics& metrics);
    
    // Visualization and reporting
    std::string generate_comprehensive_report(AssetID asset_id) const;
    std::string generate_student_progress_report(const std::string& student_id) const;
    std::string generate_system_usage_analytics() const;
    
    // Educational content
    std::vector<std::string> get_learning_recommendations(const std::string& student_id) const;
    std::string explain_asset_operation(AssetID asset_id, const std::string& operation) const;
    std::vector<std::string> get_optimization_suggestions(AssetID asset_id) const;
    
    // Observer pattern for real-time updates
    void add_metrics_observer(std::function<void(const EducationalMetrics&)> observer);
    void remove_all_observers();
    
    // Configuration
    void update_config(const EducationConfig& config);
    const EducationConfig& get_config() const { return config_; }
    
    // Statistics
    struct EducationStatistics {
        u32 active_sessions{0};
        u32 total_students{0};
        u32 exercises_completed{0};
        u32 analyses_performed{0};
        f64 average_session_duration{0.0};
        
        std::unordered_map<AssetType, u32> operations_by_type;
        std::unordered_map<std::string, u32> concepts_taught;
        std::unordered_map<std::string, f32> student_progress_distribution;
        
        f64 system_effectiveness_score{0.0}; // How well the system is teaching
    };
    
    EducationStatistics get_statistics() const;
    
    // Component access
    PerformanceProfiler* get_profiler() const { return profiler_.get(); }
    OptimizationAnalyzer* get_analyzer() const { return analyzer_.get(); }
    InteractiveTutorialManager* get_tutorial_manager() const { return tutorial_manager_.get(); }
    
private:
    void monitoring_worker();
    void process_asset_operation_for_education(AssetID asset_id, const std::string& operation);
    void notify_metrics_observers(const EducationalMetrics& metrics);
    std::string generate_session_id() const;
    
    // Educational content integration
    void integrate_with_existing_tutorials();
    void create_asset_pipeline_tutorials();
};

//=============================================================================
// Educational Visualization Components
//=============================================================================

/**
 * @brief Visualization components for educational UI integration
 */
namespace visualization {

/**
 * @brief Performance timeline visualization data
 */
struct PerformanceTimelineData {
    std::vector<std::string> phase_names;
    std::vector<f64> phase_start_times;
    std::vector<f64> phase_durations;
    std::vector<std::string> phase_descriptions;
    f64 total_duration{0.0};
};

/**
 * @brief Memory usage visualization data
 */
struct MemoryUsageData {
    std::vector<f64> timeline;              // Time points
    std::vector<usize> memory_usage;        // Memory usage at each time point
    std::vector<std::string> events;        // Memory events (allocations, etc.)
    std::vector<f64> event_times;          // When events occurred
    usize peak_memory{0};
    f64 peak_time{0.0};
};

/**
 * @brief Asset dependency graph data
 */
struct DependencyGraphData {
    struct Node {
        AssetID id;
        std::string name;
        AssetType type;
        f32 x, y; // Layout coordinates
        f32 importance_score{1.0f};
    };
    
    struct Edge {
        AssetID from_asset;
        AssetID to_asset;
        f32 strength{1.0f};
        std::string relationship_type;
    };
    
    std::vector<Node> nodes;
    std::vector<Edge> edges;
    std::string layout_algorithm{"force_directed"};
};

/**
 * @brief Optimization recommendations visualization
 */
struct OptimizationVisualizationData {
    struct Recommendation {
        std::string title;
        std::string category;
        f32 priority_score{0.5f};
        f32 impact_estimate{0.5f};
        std::string difficulty;
        std::vector<std::string> tags;
    };
    
    std::vector<Recommendation> recommendations;
    std::string overall_grade;
    f32 optimization_score{1.0f};
    std::vector<std::string> positive_aspects;
};

} // namespace visualization

//=============================================================================
// Factory Functions for Educational Content
//=============================================================================

/**
 * @brief Factory functions for creating educational content
 */
namespace factory {

// Create standard educational exercises
std::vector<InteractiveTutorialManager::InteractiveExercise> create_texture_exercises();
std::vector<InteractiveTutorialManager::InteractiveExercise> create_model_exercises();
std::vector<InteractiveTutorialManager::InteractiveExercise> create_audio_exercises();
std::vector<InteractiveTutorialManager::InteractiveExercise> create_shader_exercises();
std::vector<InteractiveTutorialManager::InteractiveExercise> create_performance_exercises();

// Create learning paths
InteractiveTutorialManager::LearningPath create_beginner_path();
InteractiveTutorialManager::LearningPath create_intermediate_path();
InteractiveTutorialManager::LearningPath create_advanced_path();

// Create educational content for integration with existing tutorial system
std::vector<std::unique_ptr<learning::Tutorial>> create_asset_pipeline_tutorials();

} // namespace factory

} // namespace ecscope::assets::education