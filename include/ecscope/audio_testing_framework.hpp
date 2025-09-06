#pragma once

/**
 * @file audio_testing_framework.hpp
 * @brief Comprehensive Audio Testing and Performance Validation Framework for ECScope
 * 
 * This comprehensive testing framework provides rigorous validation of audio systems,
 * performance benchmarking, quality assurance, and educational testing scenarios for
 * the ECScope spatial audio framework.
 * 
 * Key Features:
 * - Comprehensive unit and integration testing for all audio components
 * - Real-time performance benchmarking and profiling
 * - Audio quality validation and regression testing
 * - Educational scenario testing and validation
 * - Professional audio standards compliance testing
 * - Memory leak detection and performance analysis
 * - SIMD optimization validation and benchmarking
 * 
 * Testing Categories:
 * - Unit Tests: Individual component and algorithm testing
 * - Integration Tests: System interaction and data flow testing
 * - Performance Tests: Throughput, latency, and resource usage testing
 * - Quality Tests: Audio fidelity and standards compliance testing
 * - Educational Tests: Learning scenario validation and effectiveness
 * - Stress Tests: High-load and edge case testing
 * - Regression Tests: Preventing quality degradation over time
 * 
 * Educational Value:
 * - Demonstrates professional audio testing methodologies
 * - Shows performance optimization validation techniques
 * - Provides examples of comprehensive system testing
 * - Teaches about audio quality metrics and standards
 * - Interactive testing and debugging for educational purposes
 * - Real-world testing scenarios for audio engineering education
 * 
 * @author ECScope Educational ECS Framework - Audio Testing & Validation
 * @date 2025
 */

#include "spatial_audio_engine.hpp"
#include "audio_components.hpp"
#include "audio_systems.hpp"
#include "audio_processing_pipeline.hpp"
#include "audio_education_system.hpp"
#include "core/types.hpp"
#include "memory/memory_tracker.hpp"
#include <vector>
#include <string>
#include <memory>
#include <chrono>
#include <functional>
#include <unordered_map>
#include <random>

namespace ecscope::audio::testing {

// Import commonly used types
using namespace ecscope::audio;
using namespace ecscope::audio::components;
using namespace ecscope::audio::systems;
using namespace ecscope::audio::processing;
using namespace ecscope::audio::education;

//=============================================================================
// Testing Framework Core
//=============================================================================

/**
 * @brief Test result enumeration
 */
enum class TestResult : u8 {
    NotRun = 0,        ///< Test has not been executed
    Passed,            ///< Test passed successfully
    Failed,            ///< Test failed
    Skipped,           ///< Test was skipped
    Warning,           ///< Test passed with warnings
    Timeout,           ///< Test timed out
    Error              ///< Test encountered an error
};

/**
 * @brief Test case metadata and configuration
 */
struct TestCase {
    std::string name;
    std::string description;
    std::string category;
    f32 timeout_seconds{30.0f};
    bool is_enabled{true};
    std::vector<std::string> dependencies;
    std::vector<std::string> tags;
    
    // Educational metadata
    std::string educational_purpose;
    std::vector<std::string> learning_objectives;
    DifficultyLevel difficulty{DifficultyLevel::Intermediate};
    f32 educational_value{0.7f};
};

/**
 * @brief Test execution result with detailed metrics
 */
struct TestExecutionResult {
    TestResult result{TestResult::NotRun};
    f64 execution_time_ms{0.0};
    std::string failure_message;
    std::string warning_message;
    
    // Performance metrics
    f32 cpu_usage_percent{0.0f};
    usize memory_usage_bytes{0};
    f32 audio_processing_time_ms{0.0f};
    
    // Audio quality metrics
    f32 snr_db{0.0f};
    f32 thd_percent{0.0f};
    f32 frequency_response_flatness{0.0f};
    f32 latency_ms{0.0f};
    
    // Educational metrics
    f32 educational_effectiveness{0.0f};
    std::vector<std::string> concepts_validated;
    std::string educational_insights;
    
    // Additional data
    std::unordered_map<std::string, f32> custom_metrics;
    std::vector<std::string> log_messages;
    std::chrono::high_resolution_clock::time_point execution_timestamp;
};

/**
 * @brief Base class for all audio tests
 */
class AudioTestBase {
protected:
    TestCase test_case_;
    TestExecutionResult result_;
    std::unique_ptr<memory::MemoryTracker> memory_tracker_;
    
    // Test utilities
    std::mt19937 random_generator_;
    std::unique_ptr<class AudioSignalGenerator> signal_generator_;
    std::unique_ptr<class AudioQualityAnalyzer> quality_analyzer_;
    
public:
    explicit AudioTestBase(const TestCase& test_case);
    virtual ~AudioTestBase() = default;
    
    // Test lifecycle
    virtual bool setup() = 0;
    virtual TestResult execute() = 0;
    virtual void cleanup() = 0;
    
    // Test execution wrapper
    TestExecutionResult run_test();
    
    // Accessors
    const TestCase& get_test_case() const { return test_case_; }
    const TestExecutionResult& get_result() const { return result_; }
    
    // Educational interface
    virtual std::string get_educational_explanation() const = 0;
    virtual std::vector<std::string> get_validated_concepts() const = 0;
    
protected:
    // Utility functions for tests
    void log_message(const std::string& message);
    void add_custom_metric(const std::string& name, f32 value);
    void set_failure_message(const std::string& message);
    void set_warning_message(const std::string& message);
    
    // Audio testing utilities
    std::vector<f32> generate_test_signal(u32 sample_rate, f32 duration_seconds, f32 frequency = 1000.0f);
    std::vector<f32> generate_noise_signal(u32 sample_rate, f32 duration_seconds, f32 amplitude = 0.1f);
    std::vector<f32> generate_chirp_signal(u32 sample_rate, f32 duration_seconds, f32 start_freq, f32 end_freq);
    f32 calculate_snr(const std::vector<f32>& signal, const std::vector<f32>& noise);
    f32 calculate_thd(const std::vector<f32>& signal, f32 fundamental_freq, u32 sample_rate);
    f32 calculate_rms(const std::vector<f32>& signal);
    
    // Performance measurement utilities
    class ScopedPerformanceTimer {
    private:
        AudioTestBase* test_;
        std::chrono::high_resolution_clock::time_point start_time_;
        std::string metric_name_;
        
    public:
        ScopedPerformanceTimer(AudioTestBase* test, const std::string& metric_name);
        ~ScopedPerformanceTimer();
    };
    
private:
    void initialize_test_utilities();
    void measure_system_resources();
};

//=============================================================================
// Unit Tests for Core Audio Components
//=============================================================================

/**
 * @brief HRTF Processing Unit Tests
 * 
 * Educational Context:
 * Tests the fundamental HRTF processing algorithms, demonstrating
 * how to validate complex audio DSP operations and ensure
 * mathematical correctness of spatial audio calculations.
 */
class HRTFProcessingTests : public AudioTestBase {
private:
    std::unique_ptr<HRTFProcessor> hrtf_processor_;
    u32 test_sample_rate_{48000};
    
public:
    HRTFProcessingTests();
    
    bool setup() override;
    TestResult execute() override;
    void cleanup() override;
    
    std::string get_educational_explanation() const override;
    std::vector<std::string> get_validated_concepts() const override;
    
private:
    TestResult test_hrtf_database_loading();
    TestResult test_spatial_positioning_accuracy();
    TestResult test_itd_calculation_correctness();
    TestResult test_ild_calculation_correctness();
    TestResult test_hrtf_interpolation_quality();
    TestResult test_hrtf_convolution_performance();
    TestResult test_binaural_rendering_quality();
};

/**
 * @brief Spatial Audio Engine Unit Tests
 */
class SpatialAudioEngineTests : public AudioTestBase {
private:
    std::unique_ptr<SpatialAudioEngine> audio_engine_;
    
public:
    SpatialAudioEngineTests();
    
    bool setup() override;
    TestResult execute() override;
    void cleanup() override;
    
    std::string get_educational_explanation() const override;
    std::vector<std::string> get_validated_concepts() const override;
    
private:
    TestResult test_distance_attenuation_models();
    TestResult test_doppler_effect_accuracy();
    TestResult test_3d_positioning_calculations();
    TestResult test_environmental_processing();
    TestResult test_multi_source_processing();
    TestResult test_listener_orientation_effects();
};

/**
 * @brief Audio Components Unit Tests
 */
class AudioComponentTests : public AudioTestBase {
public:
    AudioComponentTests();
    
    bool setup() override;
    TestResult execute() override;
    void cleanup() override;
    
    std::string get_educational_explanation() const override;
    std::vector<std::string> get_validated_concepts() const override;
    
private:
    TestResult test_audio_source_configuration();
    TestResult test_audio_listener_setup();
    TestResult test_audio_environment_parameters();
    TestResult test_component_serialization();
    TestResult test_component_validation();
    TestResult test_parameter_range_checking();
};

/**
 * @brief SIMD Optimization Unit Tests
 * 
 * Educational Context:
 * Validates SIMD optimization implementations, demonstrating
 * how to test performance optimizations and ensure they
 * produce correct results while improving performance.
 */
class SIMDOptimizationTests : public AudioTestBase {
private:
    std::unique_ptr<simd_ops::SIMDDispatcher> simd_dispatcher_;
    
public:
    SIMDOptimizationTests();
    
    bool setup() override;
    TestResult execute() override;
    void cleanup() override;
    
    std::string get_educational_explanation() const override;
    std::vector<std::string> get_validated_concepts() const override;
    
private:
    TestResult test_simd_detection();
    TestResult test_simd_audio_mixing_correctness();
    TestResult test_simd_volume_scaling_correctness();
    TestResult test_simd_convolution_correctness();
    TestResult test_simd_performance_gains();
    TestResult benchmark_simd_operations();
    
    // Utility for comparing SIMD vs scalar results
    bool compare_audio_buffers(const std::vector<f32>& simd_result, 
                              const std::vector<f32>& scalar_result, 
                              f32 tolerance = 1e-6f);
};

//=============================================================================
// Integration Tests for System Interactions
//=============================================================================

/**
 * @brief ECS Audio Systems Integration Tests
 * 
 * Educational Context:
 * Tests the interaction between multiple audio systems,
 * demonstrating how to validate complex system architectures
 * and ensure proper data flow between components.
 */
class AudioSystemsIntegrationTests : public AudioTestBase {
private:
    std::unique_ptr<World> test_world_;
    std::unique_ptr<AudioSystemManager> system_manager_;
    
public:
    AudioSystemsIntegrationTests();
    
    bool setup() override;
    TestResult execute() override;
    void cleanup() override;
    
    std::string get_educational_explanation() const override;
    std::vector<std::string> get_validated_concepts() const override;
    
private:
    TestResult test_system_initialization_order();
    TestResult test_component_system_interaction();
    TestResult test_multi_listener_processing();
    TestResult test_environmental_audio_integration();
    TestResult test_physics_audio_integration();
    TestResult test_memory_management_integration();
    TestResult test_system_performance_coordination();
    
    // Test utilities
    void create_test_audio_scene(u32 num_sources, u32 num_listeners, u32 num_environments);
    void validate_audio_processing_pipeline();
};

/**
 * @brief Audio-Physics Integration Tests
 */
class AudioPhysicsIntegrationTests : public AudioTestBase {
private:
    std::unique_ptr<World> test_world_;
    std::unique_ptr<AudioPhysicsIntegrationSystem> physics_audio_system_;
    
public:
    AudioPhysicsIntegrationTests();
    
    bool setup() override;
    TestResult execute() override;
    void cleanup() override;
    
    std::string get_educational_explanation() const override;
    std::vector<std::string> get_validated_concepts() const override;
    
private:
    TestResult test_occlusion_calculation_accuracy();
    TestResult test_doppler_effect_from_physics();
    TestResult test_collision_audio_generation();
    TestResult test_material_based_audio_response();
    TestResult test_physics_performance_impact();
};

//=============================================================================
// Performance and Benchmark Tests
//=============================================================================

/**
 * @brief Audio Performance Benchmark Suite
 * 
 * Educational Context:
 * Comprehensive performance testing demonstrating how to
 * benchmark audio systems, identify bottlenecks, and
 * validate performance optimization techniques.
 */
class AudioPerformanceBenchmarks : public AudioTestBase {
private:
    struct BenchmarkScenario {
        std::string name;
        u32 num_sources;
        u32 num_listeners;
        u32 num_environments;
        bool enable_hrtf;
        bool enable_environmental_effects;
        f32 expected_cpu_usage_percent;
        f32 expected_memory_usage_mb;
        f32 expected_latency_ms;
    };
    
    std::vector<BenchmarkScenario> benchmark_scenarios_;
    std::unique_ptr<AudioSystemManager> system_manager_;
    
public:
    AudioPerformanceBenchmarks();
    
    bool setup() override;
    TestResult execute() override;
    void cleanup() override;
    
    std::string get_educational_explanation() const override;
    std::vector<std::string> get_validated_concepts() const override;
    
private:
    TestResult benchmark_single_source_processing();
    TestResult benchmark_multi_source_scaling();
    TestResult benchmark_hrtf_processing_cost();
    TestResult benchmark_environmental_processing_cost();
    TestResult benchmark_simd_optimization_gains();
    TestResult benchmark_memory_allocation_performance();
    TestResult benchmark_real_time_constraints();
    
    // Benchmark execution utilities
    struct BenchmarkResult {
        f32 average_cpu_usage_percent;
        f32 peak_cpu_usage_percent;
        f32 average_memory_usage_mb;
        f32 peak_memory_usage_mb;
        f32 average_latency_ms;
        f32 worst_case_latency_ms;
        f32 throughput_samples_per_second;
        
        bool meets_real_time_constraints;
        std::string performance_rating; // "Excellent", "Good", "Fair", "Poor"
        std::vector<std::string> bottlenecks_identified;
    };
    
    BenchmarkResult run_benchmark_scenario(const BenchmarkScenario& scenario, f32 duration_seconds = 10.0f);
    void analyze_performance_results(const std::vector<BenchmarkResult>& results);
};

/**
 * @brief Memory Management Performance Tests
 */
class AudioMemoryPerformanceTests : public AudioTestBase {
private:
    std::unique_ptr<AudioMemorySystem> memory_system_;
    
public:
    AudioMemoryPerformanceTests();
    
    bool setup() override;
    TestResult execute() override;
    void cleanup() override;
    
    std::string get_educational_explanation() const override;
    std::vector<std::string> get_validated_concepts() const override;
    
private:
    TestResult test_memory_pool_efficiency();
    TestResult test_zero_allocation_audio_processing();
    TestResult test_memory_fragmentation_resistance();
    TestResult test_memory_leak_detection();
    TestResult test_cache_performance_optimization();
    TestResult benchmark_allocation_patterns();
};

//=============================================================================
// Audio Quality Validation Tests
//=============================================================================

/**
 * @brief Audio Quality Assurance Test Suite
 * 
 * Educational Context:
 * Professional-grade audio quality testing demonstrating
 * industry-standard quality metrics, measurement techniques,
 * and validation procedures used in audio engineering.
 */
class AudioQualityValidationTests : public AudioTestBase {
private:
    std::unique_ptr<AudioQualityDemo> quality_demo_;
    std::unique_ptr<RealtimeAudioAnalyzer> quality_analyzer_;
    
public:
    AudioQualityValidationTests();
    
    bool setup() override;
    TestResult execute() override;
    void cleanup() override;
    
    std::string get_educational_explanation() const override;
    std::vector<std::string> get_validated_concepts() const override;
    
private:
    TestResult test_frequency_response_accuracy();
    TestResult test_thd_plus_noise_levels();
    TestResult test_signal_to_noise_ratio();
    TestResult test_dynamic_range_preservation();
    TestResult test_phase_response_linearity();
    TestResult test_loudness_standards_compliance();
    TestResult test_spatial_audio_quality();
    TestResult test_audio_processing_transparency();
    
    // Quality measurement utilities
    struct QualityMetrics {
        f32 frequency_response_flatness_db;    // Deviation from flat response
        f32 thd_plus_noise_percent;            // Total harmonic distortion + noise
        f32 signal_to_noise_ratio_db;          // SNR measurement
        f32 dynamic_range_db;                  // Dynamic range preservation
        f32 phase_linearity_deviation;         // Phase response deviation
        f32 loudness_lufs;                     // EBU R128 loudness
        f32 spatial_localization_accuracy;     // 3D positioning accuracy
        
        bool meets_broadcast_standards;
        bool meets_mastering_standards;
        std::string overall_quality_rating;
        std::vector<std::string> quality_issues;
    };
    
    QualityMetrics measure_audio_quality(const std::vector<f32>& input_signal, 
                                        const std::vector<f32>& output_signal,
                                        u32 sample_rate);
};

/**
 * @brief Spatial Audio Quality Tests
 */
class SpatialAudioQualityTests : public AudioTestBase {
private:
    std::unique_ptr<SpatialAudioDemo> spatial_demo_;
    
public:
    SpatialAudioQualityTests();
    
    bool setup() override;
    TestResult execute() override;
    void cleanup() override;
    
    std::string get_educational_explanation() const override;
    std::vector<std::string> get_validated_concepts() const override;
    
private:
    TestResult test_3d_localization_accuracy();
    TestResult test_distance_perception_accuracy();
    TestResult test_hrtf_processing_quality();
    TestResult test_binaural_rendering_fidelity();
    TestResult test_environmental_realism();
    TestResult test_multi_source_separation();
};

//=============================================================================
// Educational Testing Framework
//=============================================================================

/**
 * @brief Educational Scenario Validation Tests
 * 
 * Educational Context:
 * Tests the effectiveness of educational features, validating
 * that learning objectives are met and educational content
 * is accurate and engaging.
 */
class EducationalScenarioTests : public AudioTestBase {
private:
    std::unique_ptr<AudioEducationSystem> education_system_;
    std::string test_student_id_;
    
public:
    EducationalScenarioTests();
    
    bool setup() override;
    TestResult execute() override;
    void cleanup() override;
    
    std::string get_educational_explanation() const override;
    std::vector<std::string> get_validated_concepts() const override;
    
private:
    TestResult test_demonstration_functionality();
    TestResult test_learning_path_progression();
    TestResult test_educational_content_accuracy();
    TestResult test_interactive_tutorial_effectiveness();
    TestResult test_progress_tracking_accuracy();
    TestResult test_adaptive_difficulty_adjustment();
    TestResult validate_audio_concept_explanations();
    
    // Educational effectiveness metrics
    struct EducationalEffectiveness {
        f32 concept_comprehension_score;       // How well concepts are explained
        f32 engagement_level;                  // Student engagement metrics
        f32 learning_objective_completion;     // Percentage of objectives met
        f32 practical_application_score;       // Real-world applicability
        f32 retention_likelihood;              // Predicted knowledge retention
        
        std::vector<std::string> strengths;
        std::vector<std::string> areas_for_improvement;
        std::string overall_effectiveness_rating;
    };
    
    EducationalEffectiveness evaluate_educational_effectiveness(const std::string& demonstration_id);
};

//=============================================================================
// Stress and Load Testing
//=============================================================================

/**
 * @brief Audio System Stress Tests
 * 
 * Educational Context:
 * Demonstrates how to test audio systems under extreme
 * conditions, validating robustness and identifying
 * failure modes and performance limits.
 */
class AudioStressTests : public AudioTestBase {
private:
    struct StressTestScenario {
        std::string name;
        u32 max_audio_sources;
        u32 max_listeners;
        f32 duration_minutes;
        bool enable_rapid_parameter_changes;
        bool enable_memory_pressure;
        bool enable_cpu_pressure;
    };
    
    std::vector<StressTestScenario> stress_scenarios_;
    
public:
    AudioStressTests();
    
    bool setup() override;
    TestResult execute() override;
    void cleanup() override;
    
    std::string get_educational_explanation() const override;
    std::vector<std::string> get_validated_concepts() const override;
    
private:
    TestResult test_maximum_source_count();
    TestResult test_memory_exhaustion_handling();
    TestResult test_cpu_overload_recovery();
    TestResult test_rapid_parameter_changes();
    TestResult test_edge_case_input_values();
    TestResult test_system_stability_over_time();
    TestResult test_graceful_degradation();
    
    // Stress test result analysis
    struct StressTestResults {
        u32 max_stable_sources;
        f32 failure_point_cpu_percent;
        usize failure_point_memory_mb;
        bool graceful_degradation_observed;
        std::vector<std::string> failure_modes;
        std::string stability_rating;
    };
    
    StressTestResults analyze_stress_test_results();
};

//=============================================================================
// Regression Testing Framework
//=============================================================================

/**
 * @brief Audio Regression Test Suite
 * 
 * Educational Context:
 * Demonstrates regression testing methodologies for
 * maintaining audio quality and performance over time,
 * showing how to prevent quality degradation in iterative development.
 */
class AudioRegressionTests : public AudioTestBase {
private:
    struct RegressionBaseline {
        std::string version;
        std::chrono::system_clock::time_point baseline_date;
        
        // Performance baselines
        f32 baseline_cpu_usage_percent;
        usize baseline_memory_usage_bytes;
        f32 baseline_latency_ms;
        
        // Quality baselines
        f32 baseline_snr_db;
        f32 baseline_thd_percent;
        f32 baseline_frequency_response_flatness;
        
        // Feature baselines
        std::unordered_map<std::string, f32> feature_performance_baselines;
        std::unordered_map<std::string, f32> feature_quality_baselines;
    };
    
    RegressionBaseline current_baseline_;
    std::vector<RegressionBaseline> historical_baselines_;
    
public:
    AudioRegressionTests();
    
    bool setup() override;
    TestResult execute() override;
    void cleanup() override;
    
    std::string get_educational_explanation() const override;
    std::vector<std::string> get_validated_concepts() const override;
    
private:
    TestResult test_performance_regression();
    TestResult test_quality_regression();
    TestResult test_feature_regression();
    TestResult test_api_compatibility();
    TestResult validate_against_baseline();
    
    // Regression analysis utilities
    struct RegressionAnalysis {
        bool performance_regression_detected;
        bool quality_regression_detected;
        bool feature_regression_detected;
        
        std::vector<std::string> performance_regressions;
        std::vector<std::string> quality_regressions;
        std::vector<std::string> feature_regressions;
        std::vector<std::string> improvements_detected;
        
        f32 overall_regression_risk_score;  // 0-1, higher = more risk
        std::string regression_assessment;
    };
    
    RegressionAnalysis perform_regression_analysis();
    bool load_baseline_data(const std::string& version);
    void save_current_as_baseline();
};

//=============================================================================
// Test Suite Manager and Runner
//=============================================================================

/**
 * @brief Comprehensive test suite manager and runner
 * 
 * Educational Context:
 * Demonstrates how to organize and execute comprehensive
 * test suites, providing insights into professional testing
 * practices and quality assurance methodologies.
 */
class AudioTestSuiteRunner {
private:
    std::vector<std::unique_ptr<AudioTestBase>> all_tests_;
    std::unordered_map<std::string, std::vector<std::string>> test_categories_;
    
    // Execution configuration
    struct TestConfiguration {
        bool run_unit_tests{true};
        bool run_integration_tests{true};
        bool run_performance_tests{true};
        bool run_quality_tests{true};
        bool run_educational_tests{true};
        bool run_stress_tests{false};       // Only run when specifically requested
        bool run_regression_tests{true};
        
        bool stop_on_first_failure{false};
        bool generate_detailed_reports{true};
        bool enable_educational_analysis{true};
        f32 individual_test_timeout_seconds{60.0f};
        f32 total_suite_timeout_minutes{30.0f};
    } config_;
    
    // Execution state
    struct ExecutionState {
        std::chrono::high_resolution_clock::time_point start_time;
        u32 tests_run{0};
        u32 tests_passed{0};
        u32 tests_failed{0};
        u32 tests_skipped{0};
        u32 tests_timeout{0};
        
        f32 total_execution_time_minutes{0.0f};
        std::vector<TestExecutionResult> all_results;
        std::string current_test_category;
    } execution_state_;
    
public:
    AudioTestSuiteRunner();
    ~AudioTestSuiteRunner();
    
    // Test registration and management
    void register_test(std::unique_ptr<AudioTestBase> test);
    void register_all_standard_tests();
    
    // Configuration
    void set_test_configuration(const TestConfiguration& config);
    const TestConfiguration& get_configuration() const { return config_; }
    
    // Test execution
    bool run_all_tests();
    bool run_tests_by_category(const std::string& category);
    bool run_specific_test(const std::string& test_name);
    bool run_educational_validation_suite();
    bool run_performance_benchmark_suite();
    
    // Results and reporting
    struct TestSuiteResults {
        u32 total_tests_run;
        u32 tests_passed;
        u32 tests_failed;
        u32 tests_skipped;
        f32 success_rate_percent;
        f32 total_execution_time_minutes;
        
        // Performance summary
        f32 average_cpu_usage_percent;
        usize peak_memory_usage_mb;
        f32 audio_quality_score;        // 0-1 overall quality score
        
        // Educational effectiveness
        f32 educational_value_score;    // 0-1 educational effectiveness
        std::vector<std::string> validated_concepts;
        std::string educational_insights_summary;
        
        // Detailed results
        std::vector<TestExecutionResult> individual_results;
        std::unordered_map<std::string, u32> results_by_category;
        
        // Issues and recommendations
        std::vector<std::string> critical_failures;
        std::vector<std::string> performance_issues;
        std::vector<std::string> quality_issues;
        std::vector<std::string> recommendations;
    };
    
    TestSuiteResults get_test_results() const;
    
    // Reporting and analysis
    void generate_html_report(const std::string& output_path) const;
    void generate_json_report(const std::string& output_path) const;
    void generate_educational_report(const std::string& output_path) const;
    void generate_performance_analysis_report(const std::string& output_path) const;
    
    // Educational interface
    std::string generate_testing_methodology_tutorial() const;
    std::string generate_audio_quality_testing_guide() const;
    std::string generate_performance_testing_best_practices() const;
    std::vector<std::string> get_educational_testing_concepts() const;
    
    // Continuous integration support
    int get_exit_code() const;  // 0 = success, non-zero = failure for CI systems
    void print_summary_to_console() const;
    bool save_baseline_for_regression_testing() const;
    
private:
    // Test execution utilities
    void initialize_test_environment();
    void cleanup_test_environment();
    bool execute_single_test(AudioTestBase* test);
    void update_execution_statistics(const TestExecutionResult& result);
    
    // Analysis and reporting utilities
    void analyze_test_results();
    void generate_performance_summary();
    void generate_quality_assessment();
    void generate_educational_effectiveness_analysis();
    void identify_critical_issues();
    void generate_improvement_recommendations();
    
    // Educational content generation
    void generate_testing_insights();
    std::string create_concept_validation_summary() const;
    std::string create_learning_effectiveness_report() const;
};

} // namespace ecscope::audio::testing