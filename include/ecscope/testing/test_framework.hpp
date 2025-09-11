#pragma once

#include <string>
#include <vector>
#include <functional>
#include <memory>
#include <chrono>
#include <unordered_map>
#include <atomic>
#include <thread>
#include <future>
#include <exception>
#include <sstream>
#include <iomanip>
#include <typeinfo>
#include <cxxabi.h>

#ifdef _MSC_VER
    #include <windows.h>
    #include <psapi.h>
#elif defined(__linux__)
    #include <sys/resource.h>
    #include <unistd.h>
    #include <fstream>
#elif defined(__APPLE__)
    #include <mach/mach.h>
    #include <sys/resource.h>
#endif

namespace ecscope::testing {

// Forward declarations
class TestCase;
class TestSuite;
class TestRunner;
class TestReporter;

// Test result types
enum class TestResult {
    PASSED,
    FAILED,
    SKIPPED,
    ERROR
};

// Test categories for organization
enum class TestCategory {
    UNIT,
    INTEGRATION,
    PERFORMANCE,
    MEMORY,
    STRESS,
    REGRESSION,
    RENDERING,
    PHYSICS,
    AUDIO,
    NETWORKING,
    ASSET,
    ECS,
    MULTITHREADED
};

// Performance metrics structure
struct PerformanceMetrics {
    std::chrono::nanoseconds execution_time{0};
    size_t memory_used{0};
    size_t peak_memory{0};
    double cpu_usage{0.0};
    size_t allocations{0};
    size_t deallocations{0};
    std::unordered_map<std::string, double> custom_metrics;
};

// Test execution context
struct TestContext {
    std::string name;
    TestCategory category;
    bool is_parallel_safe{true};
    int timeout_seconds{30};
    std::vector<std::string> tags;
    std::unordered_map<std::string, std::string> metadata;
};

// Test assertion exception
class AssertionFailure : public std::exception {
public:
    AssertionFailure(const std::string& message, const std::string& file, int line)
        : message_(message), file_(file), line_(line) {
        full_message_ = file + ":" + std::to_string(line) + " - " + message;
    }

    const char* what() const noexcept override {
        return full_message_.c_str();
    }

    const std::string& message() const { return message_; }
    const std::string& file() const { return file_; }
    int line() const { return line_; }

private:
    std::string message_;
    std::string file_;
    int line_;
    std::string full_message_;
};

// Memory tracker for leak detection
class MemoryTracker {
public:
    struct AllocationInfo {
        size_t size;
        std::string file;
        int line;
        std::chrono::steady_clock::time_point timestamp;
    };

    static MemoryTracker& instance() {
        static MemoryTracker tracker;
        return tracker;
    }

    void record_allocation(void* ptr, size_t size, const std::string& file = "", int line = 0) {
        std::lock_guard<std::mutex> lock(mutex_);
        allocations_[ptr] = {size, file, line, std::chrono::steady_clock::now()};
        total_allocated_ += size;
        peak_memory_ = std::max(peak_memory_, current_memory_);
        current_memory_ += size;
    }

    void record_deallocation(void* ptr) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = allocations_.find(ptr);
        if (it != allocations_.end()) {
            current_memory_ -= it->second.size;
            allocations_.erase(it);
        }
    }

    void reset() {
        std::lock_guard<std::mutex> lock(mutex_);
        allocations_.clear();
        total_allocated_ = 0;
        current_memory_ = 0;
        peak_memory_ = 0;
    }

    PerformanceMetrics get_metrics() const {
        std::lock_guard<std::mutex> lock(mutex_);
        PerformanceMetrics metrics;
        metrics.memory_used = current_memory_;
        metrics.peak_memory = peak_memory_;
        metrics.allocations = allocations_.size();
        return metrics;
    }

    std::vector<AllocationInfo> get_leaks() const {
        std::lock_guard<std::mutex> lock(mutex_);
        std::vector<AllocationInfo> leaks;
        for (const auto& [ptr, info] : allocations_) {
            leaks.push_back(info);
        }
        return leaks;
    }

private:
    mutable std::mutex mutex_;
    std::unordered_map<void*, AllocationInfo> allocations_;
    std::atomic<size_t> total_allocated_{0};
    std::atomic<size_t> current_memory_{0};
    std::atomic<size_t> peak_memory_{0};
};

// Test fixture base class
class TestFixture {
public:
    virtual ~TestFixture() = default;
    virtual void setup() {}
    virtual void teardown() {}

protected:
    MemoryTracker& memory_tracker_ = MemoryTracker::instance();
};

// Mock object base class
class MockObject {
public:
    virtual ~MockObject() = default;
    
    template<typename T>
    void expect_call(const std::string& method_name, T&& return_value) {
        expectations_[method_name] = std::forward<T>(return_value);
    }

    template<typename... Args>
    void verify_call(const std::string& method_name, Args&&... args) {
        call_history_[method_name].push_back({std::forward<Args>(args)...});
    }

    bool was_called(const std::string& method_name) const {
        return call_history_.find(method_name) != call_history_.end();
    }

    size_t call_count(const std::string& method_name) const {
        auto it = call_history_.find(method_name);
        return it != call_history_.end() ? it->second.size() : 0;
    }

private:
    std::unordered_map<std::string, std::any> expectations_;
    std::unordered_map<std::string, std::vector<std::vector<std::any>>> call_history_;
};

// Test case implementation
class TestCase {
public:
    TestCase(const std::string& name, TestCategory category = TestCategory::UNIT)
        : context_{name, category} {}

    virtual ~TestCase() = default;

    // Core test execution
    virtual void run() = 0;
    virtual void setup() {}
    virtual void teardown() {}

    // Test configuration
    TestCase& with_timeout(int seconds) {
        context_.timeout_seconds = seconds;
        return *this;
    }

    TestCase& with_tag(const std::string& tag) {
        context_.tags.push_back(tag);
        return *this;
    }

    TestCase& with_metadata(const std::string& key, const std::string& value) {
        context_.metadata[key] = value;
        return *this;
    }

    TestCase& parallel_unsafe() {
        context_.is_parallel_safe = false;
        return *this;
    }

    // Getters
    const TestContext& context() const { return context_; }
    const PerformanceMetrics& metrics() const { return metrics_; }
    TestResult result() const { return result_; }
    const std::string& error_message() const { return error_message_; }

    // Test execution with metrics
    void execute() {
        auto start_time = std::chrono::high_resolution_clock::now();
        memory_tracker_.reset();
        
        try {
            setup();
            run();
            teardown();
            result_ = TestResult::PASSED;
        } catch (const AssertionFailure& e) {
            error_message_ = e.what();
            result_ = TestResult::FAILED;
        } catch (const std::exception& e) {
            error_message_ = std::string("Exception: ") + e.what();
            result_ = TestResult::ERROR;
        } catch (...) {
            error_message_ = "Unknown exception occurred";
            result_ = TestResult::ERROR;
        }

        auto end_time = std::chrono::high_resolution_clock::now();
        metrics_.execution_time = std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time);
        
        auto memory_metrics = memory_tracker_.get_metrics();
        metrics_.memory_used = memory_metrics.memory_used;
        metrics_.peak_memory = memory_metrics.peak_memory;
        metrics_.allocations = memory_metrics.allocations;
    }

protected:
    TestContext context_;
    PerformanceMetrics metrics_;
    TestResult result_{TestResult::PASSED};
    std::string error_message_;
    MemoryTracker& memory_tracker_ = MemoryTracker::instance();
};

// Benchmark test case for performance testing
class BenchmarkTest : public TestCase {
public:
    BenchmarkTest(const std::string& name, int iterations = 1000)
        : TestCase(name, TestCategory::PERFORMANCE), iterations_(iterations) {}

    void run() override {
        std::vector<std::chrono::nanoseconds> times;
        times.reserve(iterations_);

        for (int i = 0; i < iterations_; ++i) {
            auto start = std::chrono::high_resolution_clock::now();
            benchmark();
            auto end = std::chrono::high_resolution_clock::now();
            times.push_back(std::chrono::duration_cast<std::chrono::nanoseconds>(end - start));
        }

        // Calculate statistics
        calculate_statistics(times);
    }

    virtual void benchmark() = 0;

protected:
    int iterations_;

private:
    void calculate_statistics(const std::vector<std::chrono::nanoseconds>& times) {
        auto total = std::chrono::nanoseconds{0};
        auto min_time = times.front();
        auto max_time = times.front();

        for (const auto& time : times) {
            total += time;
            min_time = std::min(min_time, time);
            max_time = std::max(max_time, time);
        }

        auto mean = total / times.size();
        metrics_.execution_time = mean;
        
        // Calculate variance and standard deviation
        double variance = 0.0;
        for (const auto& time : times) {
            auto diff = time.count() - mean.count();
            variance += diff * diff;
        }
        variance /= times.size();
        auto std_dev = std::sqrt(variance);

        metrics_.custom_metrics["min_time_ns"] = min_time.count();
        metrics_.custom_metrics["max_time_ns"] = max_time.count();
        metrics_.custom_metrics["mean_time_ns"] = mean.count();
        metrics_.custom_metrics["std_dev_ns"] = std_dev;
        metrics_.custom_metrics["iterations"] = iterations_;
    }
};

// Parameterized test case
template<typename T>
class ParameterizedTest : public TestCase {
public:
    ParameterizedTest(const std::string& name, const std::vector<T>& parameters)
        : TestCase(name), parameters_(parameters) {}

    void run() override {
        for (size_t i = 0; i < parameters_.size(); ++i) {
            try {
                run_with_parameter(parameters_[i], i);
            } catch (const AssertionFailure& e) {
                std::stringstream ss;
                ss << "Parameter " << i << ": " << e.what();
                throw AssertionFailure(ss.str(), e.file(), e.line());
            }
        }
    }

    virtual void run_with_parameter(const T& parameter, size_t index) = 0;

protected:
    std::vector<T> parameters_;
};

// Test suite for organizing related tests
class TestSuite {
public:
    TestSuite(const std::string& name) : name_(name) {}

    void add_test(std::unique_ptr<TestCase> test) {
        tests_.push_back(std::move(test));
    }

    template<typename TestType, typename... Args>
    void add_test(Args&&... args) {
        tests_.push_back(std::make_unique<TestType>(std::forward<Args>(args)...));
    }

    void run_sequential() {
        for (auto& test : tests_) {
            test->execute();
        }
    }

    void run_parallel() {
        std::vector<std::future<void>> futures;
        
        for (auto& test : tests_) {
            if (test->context().is_parallel_safe) {
                futures.emplace_back(std::async(std::launch::async, [&test]() {
                    test->execute();
                }));
            } else {
                test->execute();
            }
        }

        for (auto& future : futures) {
            future.wait();
        }
    }

    const std::string& name() const { return name_; }
    const std::vector<std::unique_ptr<TestCase>>& tests() const { return tests_; }

private:
    std::string name_;
    std::vector<std::unique_ptr<TestCase>> tests_;
};

// Test discovery and registration
class TestRegistry {
public:
    static TestRegistry& instance() {
        static TestRegistry registry;
        return registry;
    }

    void register_test(std::unique_ptr<TestCase> test) {
        tests_.push_back(std::move(test));
    }

    void register_suite(std::unique_ptr<TestSuite> suite) {
        suites_.push_back(std::move(suite));
    }

    const std::vector<std::unique_ptr<TestCase>>& tests() const { return tests_; }
    const std::vector<std::unique_ptr<TestSuite>>& suites() const { return suites_; }

    std::vector<TestCase*> find_tests_by_category(TestCategory category) const {
        std::vector<TestCase*> result;
        for (const auto& test : tests_) {
            if (test->context().category == category) {
                result.push_back(test.get());
            }
        }
        return result;
    }

    std::vector<TestCase*> find_tests_by_tag(const std::string& tag) const {
        std::vector<TestCase*> result;
        for (const auto& test : tests_) {
            const auto& tags = test->context().tags;
            if (std::find(tags.begin(), tags.end(), tag) != tags.end()) {
                result.push_back(test.get());
            }
        }
        return result;
    }

private:
    std::vector<std::unique_ptr<TestCase>> tests_;
    std::vector<std::unique_ptr<TestSuite>> suites_;
};

// Macro for automatic test registration
#define REGISTER_TEST(TestClass, ...) \
    namespace { \
        static auto registered_##TestClass = []() { \
            ecscope::testing::TestRegistry::instance().register_test( \
                std::make_unique<TestClass>(__VA_ARGS__)); \
            return true; \
        }(); \
    }

// Assertion macros
#define ASSERT_TRUE(condition) \
    do { \
        if (!(condition)) { \
            throw ecscope::testing::AssertionFailure( \
                "Expected true but got false: " #condition, __FILE__, __LINE__); \
        } \
    } while(0)

#define ASSERT_FALSE(condition) \
    do { \
        if (condition) { \
            throw ecscope::testing::AssertionFailure( \
                "Expected false but got true: " #condition, __FILE__, __LINE__); \
        } \
    } while(0)

#define ASSERT_EQ(expected, actual) \
    do { \
        if ((expected) != (actual)) { \
            std::stringstream ss; \
            ss << "Expected: " << (expected) << ", Actual: " << (actual); \
            throw ecscope::testing::AssertionFailure(ss.str(), __FILE__, __LINE__); \
        } \
    } while(0)

#define ASSERT_NE(expected, actual) \
    do { \
        if ((expected) == (actual)) { \
            std::stringstream ss; \
            ss << "Expected not equal, but both were: " << (expected); \
            throw ecscope::testing::AssertionFailure(ss.str(), __FILE__, __LINE__); \
        } \
    } while(0)

#define ASSERT_LT(left, right) \
    do { \
        if (!((left) < (right))) { \
            std::stringstream ss; \
            ss << "Expected " << (left) << " < " << (right); \
            throw ecscope::testing::AssertionFailure(ss.str(), __FILE__, __LINE__); \
        } \
    } while(0)

#define ASSERT_LE(left, right) \
    do { \
        if (!((left) <= (right))) { \
            std::stringstream ss; \
            ss << "Expected " << (left) << " <= " << (right); \
            throw ecscope::testing::AssertionFailure(ss.str(), __FILE__, __LINE__); \
        } \
    } while(0)

#define ASSERT_GT(left, right) \
    do { \
        if (!((left) > (right))) { \
            std::stringstream ss; \
            ss << "Expected " << (left) << " > " << (right); \
            throw ecscope::testing::AssertionFailure(ss.str(), __FILE__, __LINE__); \
        } \
    } while(0)

#define ASSERT_GE(left, right) \
    do { \
        if (!((left) >= (right))) { \
            std::stringstream ss; \
            ss << "Expected " << (left) << " >= " << (right); \
            throw ecscope::testing::AssertionFailure(ss.str(), __FILE__, __LINE__); \
        } \
    } while(0)

#define ASSERT_NEAR(expected, actual, tolerance) \
    do { \
        auto diff = std::abs((expected) - (actual)); \
        if (diff > (tolerance)) { \
            std::stringstream ss; \
            ss << "Expected " << (expected) << " ± " << (tolerance) \
               << ", but got " << (actual) << " (difference: " << diff << ")"; \
            throw ecscope::testing::AssertionFailure(ss.str(), __FILE__, __LINE__); \
        } \
    } while(0)

#define ASSERT_THROW(statement, exception_type) \
    do { \
        bool caught = false; \
        try { \
            statement; \
        } catch (const exception_type&) { \
            caught = true; \
        } catch (...) { \
            throw ecscope::testing::AssertionFailure( \
                "Expected " #exception_type " but caught different exception", \
                __FILE__, __LINE__); \
        } \
        if (!caught) { \
            throw ecscope::testing::AssertionFailure( \
                "Expected " #exception_type " but no exception was thrown", \
                __FILE__, __LINE__); \
        } \
    } while(0)

#define ASSERT_NO_THROW(statement) \
    do { \
        try { \
            statement; \
        } catch (...) { \
            throw ecscope::testing::AssertionFailure( \
                "Expected no exception but one was thrown", __FILE__, __LINE__); \
        } \
    } while(0)

// Expect macros (non-fatal)
#define EXPECT_TRUE(condition) \
    do { \
        if (!(condition)) { \
            std::cerr << __FILE__ << ":" << __LINE__ \
                      << " - Expected true but got false: " #condition << std::endl; \
        } \
    } while(0)

#define EXPECT_EQ(expected, actual) \
    do { \
        if ((expected) != (actual)) { \
            std::cerr << __FILE__ << ":" << __LINE__ \
                      << " - Expected: " << (expected) << ", Actual: " << (actual) << std::endl; \
        } \
    } while(0)

} // namespace ecscope::testing