#pragma once

/**
 * @file ecs_performance_regression_tester.hpp
 * @brief ECS Performance Regression Testing Framework
 * 
 * This system provides automated regression testing capabilities for ECS
 * performance, ensuring that performance optimizations don't introduce
 * regressions and that the system maintains consistent performance
 * characteristics over time.
 * 
 * Key Features:
 * - Automated baseline establishment and comparison
 * - Regression detection with statistical significance testing
 * - Performance trend analysis over time
 * - Alert system for performance degradation
 * - Integration with CI/CD pipelines
 * - Historical performance tracking
 * - Detailed regression analysis reports
 * 
 * Educational Value:
 * - Demonstrates proper performance testing methodology
 * - Shows importance of continuous performance monitoring
 * - Teaches regression analysis and statistical validation
 * - Provides examples of performance quality assurance
 * 
 * @author ECScope Educational ECS Framework - Regression Testing
 * @date 2025
 */

#include "ecs_performance_benchmarker.hpp"
#include "core/types.hpp"
#include "core/log.hpp"
#include <vector>
#include <memory>
#include <string>
#include <chrono>
#include <unordered_map>
#include <functional>
#include <mutex>
#include <fstream>
#include <optional>

namespace ecscope::performance::regression {

//=============================================================================
// Regression Testing Data Structures
//=============================================================================

/**
 * @brief Performance baseline data
 */
struct PerformanceBaseline {
    std::string test_name;
    ecs::ECSArchitectureType architecture;
    u32 entity_count;
    
    // Statistical baseline data
    f64 baseline_mean_us;               // Baseline mean performance
    f64 baseline_std_dev_us;           // Baseline standard deviation
    f64 baseline_median_us;            // Baseline median
    f64 baseline_min_us;               // Baseline minimum
    f64 baseline_max_us;               // Baseline maximum
    
    // Quality metrics
    f64 baseline_consistency_score;     // Baseline consistency
    usize baseline_sample_count;       // Number of samples in baseline
    
    // Metadata
    std::string baseline_version;       // Code version when baseline was created
    std::string platform_info;         // Platform information
    std::chrono::system_clock::time_point created_time;
    bool is_valid;
    
    PerformanceBaseline()
        : entity_count(0), baseline_mean_us(0.0), baseline_std_dev_us(0.0),
          baseline_median_us(0.0), baseline_min_us(0.0), baseline_max_us(0.0),
          baseline_consistency_score(0.0), baseline_sample_count(0),
          is_valid(false) {}
};

/**
 * @brief Regression test result
 */
struct RegressionTestResult {
    enum class Status {
        Pass,           // No regression detected
        Warning,        // Minor performance change
        Regression,     // Performance regression detected
        Improvement,    // Performance improvement detected
        Invalid         // Test failed or invalid data
    };
    
    std::string test_name;
    ecs::ECSArchitectureType architecture;
    u32 entity_count;
    Status status;
    
    // Comparison metrics
    f64 current_mean_us;               // Current test mean
    f64 baseline_mean_us;              // Baseline mean for comparison
    f64 performance_change_percent;    // Percentage change from baseline
    f64 statistical_significance;      // P-value of statistical test
    
    // Regression analysis
    f64 regression_severity;           // Severity of regression (0.0-1.0)
    bool is_statistically_significant; // Whether change is significant
    std::string regression_cause;      // Suspected cause of regression
    
    // Recommendations
    std::vector<std::string> recommendations; // What to do about the regression
    std::string detailed_analysis;     // Detailed analysis text
    
    // Metadata
    std::chrono::system_clock::time_point test_time;
    std::string test_version;          // Code version when test was run
    
    RegressionTestResult()
        : entity_count(0), status(Status::Invalid), current_mean_us(0.0),
          baseline_mean_us(0.0), performance_change_percent(0.0),
          statistical_significance(1.0), regression_severity(0.0),
          is_statistically_significant(false) {}
};

/**
 * @brief Performance trend data over time
 */
struct PerformanceTrend {
    std::string test_name;
    ecs::ECSArchitectureType architecture;
    u32 entity_count;
    
    struct TrendPoint {
        f64 timestamp;          // Unix timestamp
        f64 performance_us;     // Performance measurement
        std::string version;    // Code version
        
        TrendPoint(f64 t = 0.0, f64 p = 0.0, const std::string& v = "")
            : timestamp(t), performance_us(p), version(v) {}
    };
    
    std::vector<TrendPoint> trend_points;
    
    // Trend analysis
    f64 trend_slope;                   // Performance trend slope
    f64 trend_correlation;             // Correlation coefficient
    std::string trend_direction;       // "improving", "degrading", "stable"
    
    PerformanceTrend()
        : entity_count(0), trend_slope(0.0), trend_correlation(0.0),
          trend_direction("stable") {}
};

/**
 * @brief Regression testing configuration
 */
struct RegressionTestConfig {
    // Thresholds
    f64 regression_threshold_percent{5.0};      // 5% performance degradation
    f64 warning_threshold_percent{2.0};         // 2% performance warning
    f64 significance_level{0.05};               // 5% statistical significance
    usize minimum_baseline_samples{10};         // Minimum samples for baseline
    
    // Test selection
    std::vector<std::string> tests_to_monitor;   // Specific tests to monitor
    std::vector<ecs::ECSArchitectureType> architectures_to_monitor;
    std::vector<u32> entity_counts_to_monitor;
    
    // Automation settings
    bool auto_update_baseline{false};           // Auto-update baseline on improvements
    bool fail_on_regression{true};              // Fail tests on regression
    bool generate_reports{true};                // Generate detailed reports
    
    // Notification settings
    bool enable_notifications{false};           // Enable regression notifications
    std::string notification_webhook;           // Webhook URL for notifications
    
    // Storage settings
    std::string baseline_storage_path{"baselines/"};
    std::string results_storage_path{"regression_results/"};
    usize max_historical_results{1000};         // Max results to keep
    
    static RegressionTestConfig create_default() {
        return RegressionTestConfig{};
    }
    
    static RegressionTestConfig create_strict() {
        RegressionTestConfig config;
        config.regression_threshold_percent = 2.0;
        config.warning_threshold_percent = 1.0;
        config.significance_level = 0.01;
        return config;
    }
};

//=============================================================================
// Baseline Management System
//=============================================================================

/**
 * @brief Performance baseline manager
 */
class PerformanceBaselineManager {
private:
    std::unordered_map<std::string, PerformanceBaseline> baselines_;
    std::string storage_path_;
    mutable std::mutex baselines_mutex_;
    
public:
    explicit PerformanceBaselineManager(const std::string& storage_path);
    
    // Baseline creation and management
    void create_baseline(const std::string& test_key, 
                        const std::vector<ecs::ECSBenchmarkResult>& results);
    bool has_baseline(const std::string& test_key) const;
    std::optional<PerformanceBaseline> get_baseline(const std::string& test_key) const;
    void update_baseline(const std::string& test_key,
                        const std::vector<ecs::ECSBenchmarkResult>& results);
    void remove_baseline(const std::string& test_key);
    
    // Baseline validation
    bool is_baseline_valid(const std::string& test_key) const;
    void validate_all_baselines();
    
    // Bulk operations
    void create_baselines_from_results(const std::vector<ecs::ECSBenchmarkResult>& results);
    std::vector<std::string> get_all_baseline_keys() const;
    usize get_baseline_count() const;
    
    // Persistence
    void save_baselines_to_disk();
    void load_baselines_from_disk();
    void export_baselines_json(const std::string& filename) const;
    void import_baselines_json(const std::string& filename);
    
    // Utility
    static std::string generate_test_key(const ecs::ECSBenchmarkResult& result);
    static std::string generate_test_key(const std::string& test_name,
                                        ecs::ECSArchitectureType architecture,
                                        u32 entity_count);
    
private:
    void calculate_baseline_statistics(PerformanceBaseline& baseline,
                                     const std::vector<f64>& performance_data);
};

//=============================================================================
// Statistical Analysis System
//=============================================================================

/**
 * @brief Statistical analysis for regression detection
 */
class RegressionStatisticalAnalyzer {
public:
    /** @brief Perform t-test between baseline and current results */
    static f64 perform_t_test(const std::vector<f64>& baseline_samples,
                             const std::vector<f64>& current_samples);
    
    /** @brief Calculate effect size (Cohen's d) */
    static f64 calculate_effect_size(const std::vector<f64>& baseline_samples,
                                    const std::vector<f64>& current_samples);
    
    /** @brief Detect outliers using IQR method */
    static std::vector<f64> remove_outliers(const std::vector<f64>& samples);
    
    /** @brief Calculate confidence interval */
    static std::pair<f64, f64> calculate_confidence_interval(
        const std::vector<f64>& samples, f64 confidence_level);
    
    /** @brief Perform Mann-Whitney U test (non-parametric) */
    static f64 perform_mann_whitney_test(const std::vector<f64>& baseline_samples,
                                        const std::vector<f64>& current_samples);
    
    /** @brief Calculate statistical power of test */
    static f64 calculate_statistical_power(usize sample_size, f64 effect_size,
                                          f64 significance_level);
    
    /** @brief Recommend minimum sample size */
    static usize recommend_sample_size(f64 desired_power, f64 expected_effect_size,
                                      f64 significance_level);
};

//=============================================================================
// Main Regression Testing System
//=============================================================================

/**
 * @brief Comprehensive ECS performance regression testing system
 */
class ECSPerformanceRegressionTester {
private:
    RegressionTestConfig config_;
    std::unique_ptr<PerformanceBaselineManager> baseline_manager_;
    std::unique_ptr<ecs::ECSPerformanceBenchmarker> benchmarker_;
    
    // Results storage
    std::vector<RegressionTestResult> recent_results_;
    std::unordered_map<std::string, PerformanceTrend> performance_trends_;
    mutable std::mutex results_mutex_;
    
    // Notification system
    std::function<void(const RegressionTestResult&)> notification_callback_;
    
public:
    explicit ECSPerformanceRegressionTester(const RegressionTestConfig& config);
    ~ECSPerformanceRegressionTester();
    
    // System setup
    void initialize();
    void shutdown();
    
    // Baseline management
    void establish_baseline();
    void establish_baseline_for_tests(const std::vector<std::string>& test_names);
    void update_baseline(const std::string& test_name);
    bool has_valid_baselines() const;
    
    // Regression testing
    std::vector<RegressionTestResult> run_regression_tests();
    std::vector<RegressionTestResult> run_regression_tests_for(
        const std::vector<std::string>& test_names);
    RegressionTestResult run_single_regression_test(const std::string& test_key);
    
    // Continuous monitoring
    void start_continuous_monitoring(std::chrono::minutes interval);
    void stop_continuous_monitoring();
    
    // Trend analysis
    void update_performance_trends(const std::vector<ecs::ECSBenchmarkResult>& results);
    std::vector<PerformanceTrend> get_performance_trends() const;
    PerformanceTrend get_trend_for_test(const std::string& test_key) const;
    
    // Results and reporting
    std::vector<RegressionTestResult> get_recent_results(usize count = 100) const;
    std::vector<RegressionTestResult> get_failing_tests() const;
    std::string generate_regression_report() const;
    std::string generate_trend_analysis_report() const;
    
    // Configuration
    void set_config(const RegressionTestConfig& config);
    const RegressionTestConfig& get_config() const { return config_; }
    
    // Notification system
    void set_notification_callback(std::function<void(const RegressionTestResult&)> callback);
    void send_regression_alert(const RegressionTestResult& result);
    
    // Export/Import
    void export_results_csv(const std::string& filename) const;
    void export_trends_json(const std::string& filename) const;
    void export_comprehensive_report(const std::string& filename) const;
    
    // Utility functions
    bool is_test_monitored(const std::string& test_name) const;
    f64 calculate_overall_health_score() const;
    std::vector<std::string> get_performance_recommendations() const;
    
private:
    // Implementation details
    RegressionTestResult analyze_single_test(const std::string& test_key,
                                            const ecs::ECSBenchmarkResult& current_result);
    void classify_regression_result(RegressionTestResult& result);
    void generate_regression_recommendations(RegressionTestResult& result);
    void update_single_trend(const std::string& test_key, 
                           const ecs::ECSBenchmarkResult& result);
    
    // Statistical analysis
    bool is_performance_regression(const PerformanceBaseline& baseline,
                                  const ecs::ECSBenchmarkResult& current);
    f64 calculate_regression_severity(const PerformanceBaseline& baseline,
                                     const ecs::ECSBenchmarkResult& current);
    
    // Trend analysis
    void analyze_trend_direction(PerformanceTrend& trend);
    
    // Continuous monitoring
    std::atomic<bool> monitoring_active_{false};
    std::unique_ptr<std::thread> monitoring_thread_;
    std::chrono::minutes monitoring_interval_{60};
    void monitoring_loop();
};

//=============================================================================
// Integration with CI/CD Systems
//=============================================================================

/**
 * @brief CI/CD integration helper for regression testing
 */
class CICDIntegration {
public:
    struct CIResult {
        bool tests_passed;
        usize total_tests;
        usize failed_tests;
        usize warning_tests;
        std::vector<std::string> failure_messages;
        std::string summary_report;
    };
    
    /** @brief Run regression tests for CI/CD pipeline */
    static CIResult run_ci_regression_tests(const RegressionTestConfig& config);
    
    /** @brief Generate JUnit XML report for CI systems */
    static void generate_junit_report(const std::vector<RegressionTestResult>& results,
                                     const std::string& filename);
    
    /** @brief Generate GitHub Actions summary */
    static std::string generate_github_actions_summary(
        const std::vector<RegressionTestResult>& results);
    
    /** @brief Check if performance is acceptable for deployment */
    static bool is_performance_acceptable(const std::vector<RegressionTestResult>& results,
                                         const RegressionTestConfig& config);
    
    /** @brief Generate performance badge data */
    static std::string generate_performance_badge(f64 overall_health_score);
};

//=============================================================================
// Educational Regression Testing Tutorial
//=============================================================================

/**
 * @brief Educational system for learning regression testing concepts
 */
class RegressionTestingTutorial {
public:
    /** @brief Explain regression testing concepts */
    static std::string explain_regression_testing();
    
    /** @brief Explain statistical significance in performance testing */
    static std::string explain_statistical_significance();
    
    /** @brief Demonstrate proper baseline establishment */
    static void demonstrate_baseline_establishment();
    
    /** @brief Show how to interpret regression results */
    static std::string interpret_regression_result(const RegressionTestResult& result);
    
    /** @brief Provide guidance on fixing performance regressions */
    static std::vector<std::string> get_regression_fixing_guide(
        const RegressionTestResult& result);
    
    /** @brief Generate interactive regression testing exercise */
    static std::string generate_interactive_exercise();
};

//=============================================================================
// Utility Functions
//=============================================================================

namespace regression_utils {
    /** @brief Calculate percentage change */
    f64 calculate_percentage_change(f64 baseline, f64 current);
    
    /** @brief Format regression status for display */
    std::string format_regression_status(RegressionTestResult::Status status);
    
    /** @brief Get severity color for UI display */
    u32 get_severity_color(f64 severity);
    
    /** @brief Generate regression summary statistics */
    struct SummaryStats {
        usize total_tests;
        usize passed_tests;
        usize warning_tests;
        usize failed_tests;
        f64 average_performance_change;
        f64 worst_regression_percent;
    };
    
    SummaryStats calculate_summary_stats(const std::vector<RegressionTestResult>& results);
    
    /** @brief Create regression test report template */
    std::string create_report_template();
}

} // namespace ecscope::performance::regression