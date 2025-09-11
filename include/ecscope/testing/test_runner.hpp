#pragma once

#include "test_framework.hpp"
#include <iostream>
#include <fstream>
#include <chrono>
#include <thread>
#include <future>
#include <algorithm>
#include <regex>
#include <random>

namespace ecscope::testing {

// Test execution configuration
struct TestRunnerConfig {
    bool parallel_execution = true;
    int max_threads = std::thread::hardware_concurrency();
    bool shuffle_tests = false;
    int repeat_count = 1;
    std::vector<std::string> included_tags;
    std::vector<std::string> excluded_tags;
    std::vector<TestCategory> included_categories;
    std::vector<TestCategory> excluded_categories;
    std::string filter_pattern;
    bool stop_on_failure = false;
    bool verbose_output = false;
    int timeout_seconds = 300; // Global timeout
    std::string output_format = "console"; // console, xml, json, html
    std::string output_file;
};

// Test execution result
struct TestExecutionResult {
    TestCase* test;
    TestResult result;
    std::string error_message;
    PerformanceMetrics metrics;
    std::chrono::steady_clock::time_point start_time;
    std::chrono::steady_clock::time_point end_time;
};

// Test runner statistics
struct TestRunnerStats {
    int total_tests = 0;
    int passed_tests = 0;
    int failed_tests = 0;
    int skipped_tests = 0;
    int error_tests = 0;
    std::chrono::milliseconds total_time{0};
    std::vector<TestExecutionResult> results;
    
    double pass_rate() const {
        return total_tests > 0 ? static_cast<double>(passed_tests) / total_tests * 100.0 : 0.0;
    }
};

// Race condition detector
class RaceConditionDetector {
public:
    void register_access(const void* address, bool is_write, const std::string& location) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        auto now = std::chrono::steady_clock::now();
        auto& history = access_history_[address];
        
        // Check for potential race conditions
        for (const auto& access : history) {
            auto time_diff = std::chrono::duration_cast<std::chrono::microseconds>(now - access.timestamp);
            if (time_diff < std::chrono::microseconds(1000)) { // Within 1ms
                if (is_write || access.is_write) {
                    race_conditions_.push_back({
                        address, access.location, location, access.timestamp, now
                    });
                }
            }
        }
        
        history.push_back({is_write, location, now});
        
        // Cleanup old entries
        if (history.size() > 100) {
            history.erase(history.begin(), history.begin() + 50);
        }
    }

    struct RaceCondition {
        const void* address;
        std::string first_location;
        std::string second_location;
        std::chrono::steady_clock::time_point first_time;
        std::chrono::steady_clock::time_point second_time;
    };

    std::vector<RaceCondition> get_race_conditions() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return race_conditions_;
    }

    void reset() {
        std::lock_guard<std::mutex> lock(mutex_);
        access_history_.clear();
        race_conditions_.clear();
    }

private:
    struct MemoryAccess {
        bool is_write;
        std::string location;
        std::chrono::steady_clock::time_point timestamp;
    };

    mutable std::mutex mutex_;
    std::unordered_map<const void*, std::vector<MemoryAccess>> access_history_;
    std::vector<RaceCondition> race_conditions_;
};

// Thread safety validator
class ThreadSafetyValidator {
public:
    void register_thread_start(std::thread::id thread_id, const std::string& test_name) {
        std::lock_guard<std::mutex> lock(mutex_);
        active_threads_[thread_id] = test_name;
    }

    void register_thread_end(std::thread::id thread_id) {
        std::lock_guard<std::mutex> lock(mutex_);
        active_threads_.erase(thread_id);
    }

    bool has_thread_violations() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return !thread_violations_.empty();
    }

    std::vector<std::string> get_violations() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return thread_violations_;
    }

    void add_violation(const std::string& violation) {
        std::lock_guard<std::mutex> lock(mutex_);
        thread_violations_.push_back(violation);
    }

private:
    mutable std::mutex mutex_;
    std::unordered_map<std::thread::id, std::string> active_threads_;
    std::vector<std::string> thread_violations_;
};

// Performance regression detector
class RegressionDetector {
public:
    struct PerformanceBaseline {
        std::string test_name;
        std::chrono::nanoseconds baseline_time;
        size_t baseline_memory;
        std::chrono::steady_clock::time_point recorded_at;
    };

    void record_baseline(const std::string& test_name, const PerformanceMetrics& metrics) {
        baselines_[test_name] = {
            test_name,
            metrics.execution_time,
            metrics.peak_memory,
            std::chrono::steady_clock::now()
        };
    }

    bool check_regression(const std::string& test_name, const PerformanceMetrics& metrics,
                         double time_threshold = 1.2, double memory_threshold = 1.3) {
        auto it = baselines_.find(test_name);
        if (it == baselines_.end()) {
            record_baseline(test_name, metrics);
            return false;
        }

        const auto& baseline = it->second;
        
        double time_ratio = static_cast<double>(metrics.execution_time.count()) / 
                           baseline.baseline_time.count();
        double memory_ratio = static_cast<double>(metrics.peak_memory) / 
                             baseline.baseline_memory;

        if (time_ratio > time_threshold || memory_ratio > memory_threshold) {
            regressions_.push_back({
                test_name,
                baseline.baseline_time,
                metrics.execution_time,
                baseline.baseline_memory,
                metrics.peak_memory,
                time_ratio,
                memory_ratio
            });
            return true;
        }

        return false;
    }

    struct Regression {
        std::string test_name;
        std::chrono::nanoseconds baseline_time;
        std::chrono::nanoseconds current_time;
        size_t baseline_memory;
        size_t current_memory;
        double time_ratio;
        double memory_ratio;
    };

    const std::vector<Regression>& get_regressions() const { return regressions_; }

    void save_baselines(const std::string& filename) const {
        std::ofstream file(filename);
        if (file.is_open()) {
            for (const auto& [name, baseline] : baselines_) {
                file << name << "," << baseline.baseline_time.count() 
                     << "," << baseline.baseline_memory << "\n";
            }
        }
    }

    void load_baselines(const std::string& filename) {
        std::ifstream file(filename);
        if (file.is_open()) {
            std::string line;
            while (std::getline(file, line)) {
                std::istringstream iss(line);
                std::string name, time_str, memory_str;
                
                if (std::getline(iss, name, ',') &&
                    std::getline(iss, time_str, ',') &&
                    std::getline(iss, memory_str)) {
                    
                    baselines_[name] = {
                        name,
                        std::chrono::nanoseconds(std::stoull(time_str)),
                        std::stoull(memory_str),
                        std::chrono::steady_clock::now()
                    };
                }
            }
        }
    }

private:
    std::unordered_map<std::string, PerformanceBaseline> baselines_;
    std::vector<Regression> regressions_;
};

// Main test runner class
class TestRunner {
public:
    TestRunner(const TestRunnerConfig& config = TestRunnerConfig{})
        : config_(config) {
        race_detector_ = std::make_unique<RaceConditionDetector>();
        thread_validator_ = std::make_unique<ThreadSafetyValidator>();
        regression_detector_ = std::make_unique<RegressionDetector>();
    }

    TestRunnerStats run_all_tests() {
        auto& registry = TestRegistry::instance();
        
        std::vector<TestCase*> tests_to_run;
        
        // Collect all tests from registry
        for (const auto& test : registry.tests()) {
            tests_to_run.push_back(test.get());
        }
        
        // Collect tests from suites
        for (const auto& suite : registry.suites()) {
            for (const auto& test : suite->tests()) {
                tests_to_run.push_back(test.get());
            }
        }

        return run_tests(tests_to_run);
    }

    TestRunnerStats run_tests(const std::vector<TestCase*>& tests) {
        TestRunnerStats stats;
        
        // Filter tests
        auto filtered_tests = filter_tests(tests);
        
        if (config_.shuffle_tests) {
            shuffle_tests(filtered_tests);
        }

        stats.total_tests = filtered_tests.size();
        
        if (config_.verbose_output) {
            std::cout << "Running " << stats.total_tests << " tests...\n";
        }

        auto start_time = std::chrono::steady_clock::now();

        for (int repeat = 0; repeat < config_.repeat_count; ++repeat) {
            if (config_.repeat_count > 1 && config_.verbose_output) {
                std::cout << "Repeat " << (repeat + 1) << "/" << config_.repeat_count << "\n";
            }

            if (config_.parallel_execution) {
                run_tests_parallel(filtered_tests, stats);
            } else {
                run_tests_sequential(filtered_tests, stats);
            }

            if (config_.stop_on_failure && stats.failed_tests > 0) {
                break;
            }
        }

        auto end_time = std::chrono::steady_clock::now();
        stats.total_time = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

        // Check for regressions
        check_performance_regressions(stats);

        // Generate report
        generate_report(stats);

        return stats;
    }

    TestRunnerStats run_tests_by_category(TestCategory category) {
        auto& registry = TestRegistry::instance();
        auto tests = registry.find_tests_by_category(category);
        return run_tests(tests);
    }

    TestRunnerStats run_tests_by_tag(const std::string& tag) {
        auto& registry = TestRegistry::instance();
        auto tests = registry.find_tests_by_tag(tag);
        return run_tests(tests);
    }

    void set_regression_baseline_file(const std::string& filename) {
        regression_detector_->load_baselines(filename);
        baseline_file_ = filename;
    }

private:
    TestRunnerConfig config_;
    std::unique_ptr<RaceConditionDetector> race_detector_;
    std::unique_ptr<ThreadSafetyValidator> thread_validator_;
    std::unique_ptr<RegressionDetector> regression_detector_;
    std::string baseline_file_;

    std::vector<TestCase*> filter_tests(const std::vector<TestCase*>& tests) {
        std::vector<TestCase*> filtered;

        for (auto* test : tests) {
            if (!should_include_test(test)) {
                continue;
            }

            // Apply pattern filter
            if (!config_.filter_pattern.empty()) {
                std::regex pattern(config_.filter_pattern);
                if (!std::regex_search(test->context().name, pattern)) {
                    continue;
                }
            }

            filtered.push_back(test);
        }

        return filtered;
    }

    bool should_include_test(TestCase* test) {
        const auto& context = test->context();

        // Check category filters
        if (!config_.included_categories.empty()) {
            if (std::find(config_.included_categories.begin(), 
                         config_.included_categories.end(), 
                         context.category) == config_.included_categories.end()) {
                return false;
            }
        }

        if (!config_.excluded_categories.empty()) {
            if (std::find(config_.excluded_categories.begin(),
                         config_.excluded_categories.end(),
                         context.category) != config_.excluded_categories.end()) {
                return false;
            }
        }

        // Check tag filters
        if (!config_.included_tags.empty()) {
            bool has_included_tag = false;
            for (const auto& tag : context.tags) {
                if (std::find(config_.included_tags.begin(),
                             config_.included_tags.end(),
                             tag) != config_.included_tags.end()) {
                    has_included_tag = true;
                    break;
                }
            }
            if (!has_included_tag) {
                return false;
            }
        }

        if (!config_.excluded_tags.empty()) {
            for (const auto& tag : context.tags) {
                if (std::find(config_.excluded_tags.begin(),
                             config_.excluded_tags.end(),
                             tag) != config_.excluded_tags.end()) {
                    return false;
                }
            }
        }

        return true;
    }

    void shuffle_tests(std::vector<TestCase*>& tests) {
        std::random_device rd;
        std::mt19937 g(rd());
        std::shuffle(tests.begin(), tests.end(), g);
    }

    void run_tests_sequential(const std::vector<TestCase*>& tests, TestRunnerStats& stats) {
        for (auto* test : tests) {
            auto result = execute_test(test);
            update_stats(result, stats);
            
            if (config_.verbose_output) {
                print_test_result(result);
            }

            if (config_.stop_on_failure && result.result == TestResult::FAILED) {
                break;
            }
        }
    }

    void run_tests_parallel(const std::vector<TestCase*>& tests, TestRunnerStats& stats) {
        std::vector<std::future<TestExecutionResult>> futures;
        std::vector<TestCase*> sequential_tests;

        // Separate parallel-safe and sequential tests
        for (auto* test : tests) {
            if (test->context().is_parallel_safe) {
                futures.emplace_back(std::async(std::launch::async, [this, test]() {
                    return execute_test(test);
                }));
            } else {
                sequential_tests.push_back(test);
            }
        }

        // Run sequential tests
        for (auto* test : sequential_tests) {
            auto result = execute_test(test);
            update_stats(result, stats);
            
            if (config_.verbose_output) {
                print_test_result(result);
            }
        }

        // Collect results from parallel tests
        for (auto& future : futures) {
            auto result = future.get();
            update_stats(result, stats);
            
            if (config_.verbose_output) {
                print_test_result(result);
            }
        }
    }

    TestExecutionResult execute_test(TestCase* test) {
        TestExecutionResult result;
        result.test = test;
        result.start_time = std::chrono::steady_clock::now();

        // Set up thread safety monitoring
        thread_validator_->register_thread_start(std::this_thread::get_id(), test->context().name);

        try {
            // Execute with timeout
            auto future = std::async(std::launch::async, [test]() {
                test->execute();
            });

            auto timeout = std::chrono::seconds(std::min(config_.timeout_seconds, test->context().timeout_seconds));
            
            if (future.wait_for(timeout) == std::future_status::timeout) {
                result.result = TestResult::ERROR;
                result.error_message = "Test timed out after " + std::to_string(timeout.count()) + " seconds";
            } else {
                future.get(); // May throw
                result.result = test->result();
                result.error_message = test->error_message();
                result.metrics = test->metrics();
            }
        } catch (const std::exception& e) {
            result.result = TestResult::ERROR;
            result.error_message = std::string("Exception during test execution: ") + e.what();
        } catch (...) {
            result.result = TestResult::ERROR;
            result.error_message = "Unknown exception during test execution";
        }

        result.end_time = std::chrono::steady_clock::now();
        thread_validator_->register_thread_end(std::this_thread::get_id());

        return result;
    }

    void update_stats(const TestExecutionResult& result, TestRunnerStats& stats) {
        stats.results.push_back(result);
        
        switch (result.result) {
            case TestResult::PASSED:
                stats.passed_tests++;
                break;
            case TestResult::FAILED:
                stats.failed_tests++;
                break;
            case TestResult::SKIPPED:
                stats.skipped_tests++;
                break;
            case TestResult::ERROR:
                stats.error_tests++;
                break;
        }
    }

    void print_test_result(const TestExecutionResult& result) {
        std::string status;
        switch (result.result) {
            case TestResult::PASSED:
                status = "PASS";
                break;
            case TestResult::FAILED:
                status = "FAIL";
                break;
            case TestResult::SKIPPED:
                status = "SKIP";
                break;
            case TestResult::ERROR:
                status = "ERROR";
                break;
        }

        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
            result.end_time - result.start_time);

        std::cout << "[" << status << "] " << result.test->context().name 
                  << " (" << duration.count() << "ms)";
        
        if (!result.error_message.empty()) {
            std::cout << " - " << result.error_message;
        }
        
        std::cout << std::endl;
    }

    void check_performance_regressions(TestRunnerStats& stats) {
        for (auto& result : stats.results) {
            if (result.result == TestResult::PASSED) {
                regression_detector_->check_regression(
                    result.test->context().name, result.metrics);
            }
        }

        if (!baseline_file_.empty()) {
            regression_detector_->save_baselines(baseline_file_);
        }
    }

    void generate_report(const TestRunnerStats& stats) {
        if (config_.output_format == "console" || config_.output_file.empty()) {
            print_console_report(stats);
        }
        
        if (!config_.output_file.empty()) {
            if (config_.output_format == "xml") {
                generate_xml_report(stats);
            } else if (config_.output_format == "json") {
                generate_json_report(stats);
            } else if (config_.output_format == "html") {
                generate_html_report(stats);
            }
        }
    }

    void print_console_report(const TestRunnerStats& stats) {
        std::cout << "\n" << std::string(50, '=') << "\n";
        std::cout << "Test Results Summary\n";
        std::cout << std::string(50, '=') << "\n";
        std::cout << "Total Tests: " << stats.total_tests << "\n";
        std::cout << "Passed: " << stats.passed_tests << "\n";
        std::cout << "Failed: " << stats.failed_tests << "\n";
        std::cout << "Errors: " << stats.error_tests << "\n";
        std::cout << "Skipped: " << stats.skipped_tests << "\n";
        std::cout << "Pass Rate: " << std::fixed << std::setprecision(2) << stats.pass_rate() << "%\n";
        std::cout << "Total Time: " << stats.total_time.count() << "ms\n";

        // Report race conditions
        auto race_conditions = race_detector_->get_race_conditions();
        if (!race_conditions.empty()) {
            std::cout << "\nRace Conditions Detected: " << race_conditions.size() << "\n";
        }

        // Report thread safety violations
        if (thread_validator_->has_thread_violations()) {
            std::cout << "\nThread Safety Violations:\n";
            for (const auto& violation : thread_validator_->get_violations()) {
                std::cout << "  " << violation << "\n";
            }
        }

        // Report performance regressions
        auto regressions = regression_detector_->get_regressions();
        if (!regressions.empty()) {
            std::cout << "\nPerformance Regressions Detected: " << regressions.size() << "\n";
            for (const auto& regression : regressions) {
                std::cout << "  " << regression.test_name 
                          << " - Time: " << std::fixed << std::setprecision(2) << regression.time_ratio << "x"
                          << ", Memory: " << regression.memory_ratio << "x\n";
            }
        }

        std::cout << std::string(50, '=') << "\n";
    }

    void generate_xml_report(const TestRunnerStats& stats) {
        // Implementation for XML report generation
        std::ofstream file(config_.output_file);
        if (file.is_open()) {
            file << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
            file << "<testsuites>\n";
            file << "  <testsuite name=\"ECScope Tests\" tests=\"" << stats.total_tests 
                 << "\" failures=\"" << stats.failed_tests << "\" errors=\"" << stats.error_tests 
                 << "\" time=\"" << stats.total_time.count() / 1000.0 << "\">\n";
            
            for (const auto& result : stats.results) {
                file << "    <testcase name=\"" << result.test->context().name 
                     << "\" time=\"" << result.metrics.execution_time.count() / 1000000000.0 << "\"";
                
                if (result.result == TestResult::FAILED) {
                    file << ">\n      <failure message=\"" << result.error_message << "\"/>\n    </testcase>\n";
                } else if (result.result == TestResult::ERROR) {
                    file << ">\n      <error message=\"" << result.error_message << "\"/>\n    </testcase>\n";
                } else {
                    file << "/>\n";
                }
            }
            
            file << "  </testsuite>\n";
            file << "</testsuites>\n";
        }
    }

    void generate_json_report(const TestRunnerStats& stats) {
        // Implementation for JSON report generation
        std::ofstream file(config_.output_file);
        if (file.is_open()) {
            file << "{\n";
            file << "  \"summary\": {\n";
            file << "    \"total\": " << stats.total_tests << ",\n";
            file << "    \"passed\": " << stats.passed_tests << ",\n";
            file << "    \"failed\": " << stats.failed_tests << ",\n";
            file << "    \"errors\": " << stats.error_tests << ",\n";
            file << "    \"skipped\": " << stats.skipped_tests << ",\n";
            file << "    \"pass_rate\": " << stats.pass_rate() << ",\n";
            file << "    \"total_time_ms\": " << stats.total_time.count() << "\n";
            file << "  },\n";
            file << "  \"tests\": [\n";
            
            for (size_t i = 0; i < stats.results.size(); ++i) {
                const auto& result = stats.results[i];
                file << "    {\n";
                file << "      \"name\": \"" << result.test->context().name << "\",\n";
                file << "      \"result\": \"";
                switch (result.result) {
                    case TestResult::PASSED: file << "passed"; break;
                    case TestResult::FAILED: file << "failed"; break;
                    case TestResult::ERROR: file << "error"; break;
                    case TestResult::SKIPPED: file << "skipped"; break;
                }
                file << "\",\n";
                file << "      \"time_ns\": " << result.metrics.execution_time.count() << ",\n";
                file << "      \"memory_bytes\": " << result.metrics.memory_used << "\n";
                if (!result.error_message.empty()) {
                    file << "      \"error\": \"" << result.error_message << "\",\n";
                }
                file << "    }";
                if (i < stats.results.size() - 1) file << ",";
                file << "\n";
            }
            
            file << "  ]\n";
            file << "}\n";
        }
    }

    void generate_html_report(const TestRunnerStats& stats) {
        // Implementation for HTML report generation with charts and visualizations
        std::ofstream file(config_.output_file);
        if (file.is_open()) {
            file << "<!DOCTYPE html>\n<html>\n<head>\n";
            file << "<title>ECScope Test Report</title>\n";
            file << "<style>\nbody { font-family: Arial, sans-serif; margin: 20px; }\n";
            file << ".summary { background: #f0f0f0; padding: 15px; border-radius: 5px; }\n";
            file << ".passed { color: green; } .failed { color: red; } .error { color: orange; }\n";
            file << "</style>\n</head>\n<body>\n";
            file << "<h1>ECScope Test Report</h1>\n";
            file << "<div class=\"summary\">\n";
            file << "<h2>Summary</h2>\n";
            file << "<p>Total Tests: " << stats.total_tests << "</p>\n";
            file << "<p>Passed: <span class=\"passed\">" << stats.passed_tests << "</span></p>\n";
            file << "<p>Failed: <span class=\"failed\">" << stats.failed_tests << "</span></p>\n";
            file << "<p>Errors: <span class=\"error\">" << stats.error_tests << "</span></p>\n";
            file << "<p>Pass Rate: " << std::fixed << std::setprecision(2) << stats.pass_rate() << "%</p>\n";
            file << "</div>\n";
            file << "</body>\n</html>\n";
        }
    }
};

} // namespace ecscope::testing