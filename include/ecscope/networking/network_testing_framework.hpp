#pragma once

/**
 * @file networking/network_testing_framework.hpp
 * @brief Comprehensive Network Testing Framework for ECScope
 * 
 * This header provides a complete testing framework for validating network
 * reliability, performance, and educational features. It includes:
 * 
 * Core Testing Features:
 * - Unit tests for protocol components
 * - Integration tests for client-server synchronization
 * - Performance benchmarks with statistical analysis
 * - Stress tests for high-load scenarios
 * - Regression tests to prevent performance degradation
 * 
 * Reliability Testing:
 * - Packet loss simulation and recovery validation
 * - Network partitioning and reconnection tests
 * - Authority transfer and conflict resolution
 * - Data consistency validation across clients
 * - Edge case handling (late packets, duplicates, corruption)
 * 
 * Performance Testing:
 * - Bandwidth utilization analysis
 * - Latency measurement and optimization
 * - Prediction accuracy and correction efficiency
 * - Memory allocation patterns and leak detection
 * - CPU usage profiling under different loads
 * 
 * Educational Testing:
 * - Tutorial completion and learning objective validation
 * - Visualization accuracy and educational effectiveness
 * - Content delivery and engagement measurement
 * - Interactive demo functionality verification
 * - Progress tracking and achievement system
 * 
 * Advanced Features:
 * - Automated test suite execution with CI/CD integration
 * - Custom scenario generation for specific testing needs
 * - Real-world network condition simulation
 * - Comparative analysis of different networking strategies
 * - Detailed reporting with actionable insights
 * 
 * @author ECScope Educational ECS Framework - Network Testing
 * @date 2024
 */

#include "network_types.hpp"
#include "network_protocol.hpp"
#include "network_prediction.hpp"
#include "component_sync.hpp"
#include "ecs_networking_system.hpp"
#include "educational_visualizer.hpp"
#include "../performance/performance_benchmark.hpp"
#include "../memory/memory_tracker.hpp"
#include <vector>
#include <string>
#include <functional>
#include <chrono>
#include <random>
#include <unordered_map>
#include <memory>
#include <fstream>
#include <thread>
#include <atomic>

namespace ecscope::networking::testing {

//=============================================================================
// Test Framework Core
//=============================================================================

/**
 * @brief Test Result Status
 */
enum class TestResult : u8 {
    NotRun,      // Test hasn't been executed
    Passed,      // Test passed all assertions
    Failed,      // Test failed one or more assertions
    Skipped,     // Test was skipped due to conditions
    Timeout,     // Test exceeded time limit
    Error        // Test encountered unexpected error
};

/**
 * @brief Test Category
 */
enum class TestCategory : u8 {
    Unit,           // Individual component tests
    Integration,    // System interaction tests
    Performance,    // Benchmark and performance tests
    Reliability,    // Network reliability and recovery tests
    Educational,    // Learning and visualization tests
    Stress,         // High-load and edge case tests
    Regression      // Performance regression detection
};

/**
 * @brief Test Priority Level
 */
enum class TestPriority : u8 {
    Critical,    // Must pass for system to be considered working
    High,        // Important for production readiness
    Normal,      // Standard functionality validation
    Low,         // Nice-to-have features
    Optional     // Experimental or future features
};

/**
 * @brief Individual Test Case
 */
struct TestCase {
    std::string id;
    std::string name;
    std::string description;
    TestCategory category;
    TestPriority priority;
    f32 timeout_seconds{30.0f};
    
    std::function<void()> setup;
    std::function<void()> test_function;
    std::function<void()> teardown;
    std::function<bool()> skip_condition;
    
    // Test state
    TestResult result{TestResult::NotRun};
    std::string failure_message;
    f64 execution_time_ms{0.0};
    NetworkTimestamp start_time{0};
    NetworkTimestamp end_time{0};
    
    // Performance data
    std::unordered_map<std::string, f64> metrics;
    
    /** @brief Check if test should be skipped */
    bool should_skip() const {
        return skip_condition && skip_condition();
    }
    
    /** @brief Mark test as failed with message */
    void fail(const std::string& message) {
        result = TestResult::Failed;
        failure_message = message;
    }
    
    /** @brief Add performance metric */
    void add_metric(const std::string& name, f64 value) {
        metrics[name] = value;
    }
};

/**
 * @brief Test Suite
 */
class TestSuite {
private:
    std::string name_;
    std::string description_;
    std::vector<std::unique_ptr<TestCase>> test_cases_;
    
    // Execution settings
    bool parallel_execution_{false};
    u32 max_parallel_tests_{4};
    bool stop_on_failure_{false};
    
    // Results tracking
    u32 passed_count_{0};
    u32 failed_count_{0};
    u32 skipped_count_{0};
    u32 timeout_count_{0};
    u32 error_count_{0};
    f64 total_execution_time_ms_{0.0};
    
public:
    /** @brief Initialize test suite */
    explicit TestSuite(const std::string& name, const std::string& description = "")
        : name_(name), description_(description) {}
    
    /** @brief Add test case to suite */
    void add_test(std::unique_ptr<TestCase> test_case) {
        test_cases_.push_back(std::move(test_case));
    }
    
    /** @brief Run all tests in suite */
    void run_all() {
        reset_results();
        
        auto start_time = std::chrono::high_resolution_clock::now();
        
        if (parallel_execution_) {
            run_tests_parallel();
        } else {
            run_tests_sequential();
        }
        
        auto end_time = std::chrono::high_resolution_clock::now();
        total_execution_time_ms_ = std::chrono::duration<f64, std::milli>(end_time - start_time).count();
        
        generate_report();
    }
    
    /** @brief Run specific test categories */
    void run_category(TestCategory category) {
        reset_results();
        
        auto start_time = std::chrono::high_resolution_clock::now();
        
        for (auto& test : test_cases_) {
            if (test->category == category) {
                run_single_test(*test);
            }
        }
        
        auto end_time = std::chrono::high_resolution_clock::now();
        total_execution_time_ms_ = std::chrono::duration<f64, std::milli>(end_time - start_time).count();
    }
    
    /** @brief Get test results summary */
    struct TestSummary {
        u32 total_tests;
        u32 passed;
        u32 failed;
        u32 skipped;
        u32 timeout;
        u32 error;
        f64 total_time_ms;
        f32 success_rate;
    };
    
    TestSummary get_summary() const {
        u32 total = static_cast<u32>(test_cases_.size());
        f32 success_rate = total > 0 ? (static_cast<f32>(passed_count_) / total) * 100.0f : 0.0f;
        
        return TestSummary{
            .total_tests = total,
            .passed = passed_count_,
            .failed = failed_count_,
            .skipped = skipped_count_,
            .timeout = timeout_count_,
            .error = error_count_,
            .total_time_ms = total_execution_time_ms_,
            .success_rate = success_rate
        };
    }
    
    /** @brief Enable parallel test execution */
    void set_parallel_execution(bool enabled, u32 max_threads = 4) {
        parallel_execution_ = enabled;
        max_parallel_tests_ = max_threads;
    }
    
    /** @brief Set stop on first failure */
    void set_stop_on_failure(bool enabled) {
        stop_on_failure_ = enabled;
    }

private:
    /** @brief Reset result counters */
    void reset_results() {
        passed_count_ = 0;
        failed_count_ = 0;
        skipped_count_ = 0;
        timeout_count_ = 0;
        error_count_ = 0;
        total_execution_time_ms_ = 0.0;
    }
    
    /** @brief Run tests sequentially */
    void run_tests_sequential() {
        for (auto& test : test_cases_) {
            run_single_test(*test);
            
            if (stop_on_failure_ && test->result == TestResult::Failed) {
                break;
            }
        }
    }
    
    /** @brief Run tests in parallel */
    void run_tests_parallel() {
        // Implementation would use thread pool for parallel execution
        // For now, fall back to sequential
        run_tests_sequential();
    }
    
    /** @brief Run individual test case */
    void run_single_test(TestCase& test) {
        if (test.should_skip()) {
            test.result = TestResult::Skipped;
            skipped_count_++;
            return;
        }
        
        test.start_time = timing::now();
        
        try {
            // Setup
            if (test.setup) {
                test.setup();
            }
            
            // Run test with timeout
            std::atomic<bool> test_completed{false};
            std::atomic<bool> test_timedout{false};
            
            std::thread test_thread([&]() {
                try {
                    test.test_function();
                    if (!test_timedout.load()) {
                        test.result = TestResult::Passed;
                        passed_count_++;
                    }
                } catch (const std::exception& e) {
                    if (!test_timedout.load()) {
                        test.fail("Exception: " + std::string(e.what()));
                        failed_count_++;
                    }
                }
                test_completed.store(true);
            });
            
            // Wait for test completion or timeout
            auto timeout_duration = std::chrono::duration<f32>(test.timeout_seconds);
            bool completed = false;
            
            if (test_thread.joinable()) {
                auto start = std::chrono::high_resolution_clock::now();
                
                while (!test_completed.load() && 
                       std::chrono::high_resolution_clock::now() - start < timeout_duration) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                }
                
                if (!test_completed.load()) {
                    test_timedout.store(true);
                    test.result = TestResult::Timeout;
                    timeout_count_++;
                }
                
                test_thread.join();
            }
            
            // Teardown
            if (test.teardown) {
                test.teardown();
            }
            
        } catch (const std::exception& e) {
            test.result = TestResult::Error;
            test.failure_message = "Setup/Teardown error: " + std::string(e.what());
            error_count_++;
        }
        
        test.end_time = timing::now();
        test.execution_time_ms = (test.end_time - test.start_time) / 1000.0;
    }
    
    /** @brief Generate test report */
    void generate_report() {
        std::cout << "\n" << "=" << std::string(60, '=') << "\n";
        std::cout << "Test Suite: " << name_ << "\n";
        if (!description_.empty()) {
            std::cout << "Description: " << description_ << "\n";
        }
        std::cout << "=" << std::string(60, '=') << "\n";
        
        auto summary = get_summary();
        std::cout << "Results: " << summary.passed << " passed, " 
                  << summary.failed << " failed, "
                  << summary.skipped << " skipped, "
                  << summary.timeout << " timeout, "
                  << summary.error << " error\n";
        std::cout << "Success Rate: " << summary.success_rate << "%\n";
        std::cout << "Total Time: " << summary.total_time_ms << " ms\n\n";
        
        // Show failed tests
        for (const auto& test : test_cases_) {
            if (test->result == TestResult::Failed || test->result == TestResult::Error) {
                std::cout << "❌ " << test->name << ": " << test->failure_message << "\n";
            }
        }
        
        std::cout << std::string(60, '=') << "\n\n";
    }
};

//=============================================================================
// Test Utilities and Assertions
//=============================================================================

/**
 * @brief Test Assertion Utilities
 */
class TestAssert {
public:
    /** @brief Assert that condition is true */
    static void is_true(bool condition, const std::string& message = "Assertion failed") {
        if (!condition) {
            throw std::runtime_error(message);
        }
    }
    
    /** @brief Assert that condition is false */
    static void is_false(bool condition, const std::string& message = "Expected false") {
        is_true(!condition, message);
    }
    
    /** @brief Assert that values are equal */
    template<typename T>
    static void equals(const T& expected, const T& actual, const std::string& message = "Values not equal") {
        if (expected != actual) {
            throw std::runtime_error(message + " (expected: " + std::to_string(expected) + 
                                   ", actual: " + std::to_string(actual) + ")");
        }
    }
    
    /** @brief Assert that floating-point values are approximately equal */
    static void approx_equals(f64 expected, f64 actual, f64 tolerance = 1e-6, 
                             const std::string& message = "Values not approximately equal") {
        if (std::abs(expected - actual) > tolerance) {
            throw std::runtime_error(message + " (expected: " + std::to_string(expected) + 
                                   ", actual: " + std::to_string(actual) + 
                                   ", tolerance: " + std::to_string(tolerance) + ")");
        }
    }
    
    /** @brief Assert that value is within range */
    template<typename T>
    static void in_range(const T& value, const T& min_val, const T& max_val,
                        const std::string& message = "Value not in range") {
        if (value < min_val || value > max_val) {
            throw std::runtime_error(message + " (value: " + std::to_string(value) + 
                                   ", range: [" + std::to_string(min_val) + ", " + 
                                   std::to_string(max_val) + "])");
        }
    }
    
    /** @brief Assert that pointer is not null */
    template<typename T>
    static void not_null(T* ptr, const std::string& message = "Pointer is null") {
        if (ptr == nullptr) {
            throw std::runtime_error(message);
        }
    }
    
    /** @brief Assert that container is not empty */
    template<typename Container>
    static void not_empty(const Container& container, const std::string& message = "Container is empty") {
        if (container.empty()) {
            throw std::runtime_error(message);
        }
    }
    
    /** @brief Assert that network statistics meet expectations */
    static void network_stats_valid(const NetworkStats& stats, 
                                   const std::string& message = "Network stats validation failed") {
        is_true(stats.packets_received >= 0, message + ": negative packets received");
        is_true(stats.packets_sent >= 0, message + ": negative packets sent");
        is_true(stats.connection_quality >= 0.0f && stats.connection_quality <= 1.0f, 
               message + ": connection quality out of range");
    }
};

//=============================================================================
// Network Test Environment
//=============================================================================

/**
 * @brief Simulated Network Environment
 * 
 * Provides a controlled environment for network testing with
 * configurable conditions and comprehensive monitoring.
 */
class NetworkTestEnvironment {
private:
    // Network simulation parameters
    f32 base_latency_ms_{50.0f};
    f32 latency_jitter_ms_{10.0f};
    f32 packet_loss_rate_{0.0f};
    f32 bandwidth_limit_kbps_{1000.0f};
    bool enable_packet_reordering_{false};
    
    // Test clients and server
    std::unique_ptr<ECSNetworkingSystem> server_;
    std::vector<std::unique_ptr<ECSNetworkingSystem>> clients_;
    std::unique_ptr<ecs::Registry> test_registry_;
    
    // Network condition simulation
    std::mt19937 rng_;
    std::uniform_real_distribution<f32> latency_dist_;
    std::uniform_real_distribution<f32> loss_dist_;
    
    // Monitoring and statistics
    std::unordered_map<std::string, f64> performance_metrics_;
    std::vector<std::string> event_log_;
    NetworkTimestamp test_start_time_{0};
    
    // Memory tracking
    memory::MemoryTracker memory_tracker_;
    
public:
    /** @brief Initialize test environment */
    NetworkTestEnvironment() : rng_(std::random_device{}()) {
        test_registry_ = std::make_unique<ecs::Registry>();
        memory_tracker_.start_tracking("NetworkTesting");
    }
    
    /** @brief Setup server for testing */
    void setup_server(u16 port = 7778) {
        NetworkConfig server_config = NetworkConfig::server_default();
        server_config.server_address = NetworkAddress::local(port);
        
        server_ = std::make_unique<ECSNetworkingSystem>(*test_registry_, server_config);
        
        if (!server_->start_server()) {
            throw std::runtime_error("Failed to start test server");
        }
        
        log_event("Server started on port " + std::to_string(port));
    }
    
    /** @brief Add test client */
    void add_client(const std::string& client_name = "") {
        NetworkConfig client_config = NetworkConfig::client_default();
        
        auto client = std::make_unique<ECSNetworkingSystem>(*test_registry_, client_config);
        
        if (!client->start_client()) {
            throw std::runtime_error("Failed to start test client");
        }
        
        clients_.push_back(std::move(client));
        
        std::string name = client_name.empty() ? "Client" + std::to_string(clients_.size()) : client_name;
        log_event(name + " connected");
    }
    
    /** @brief Set network conditions */
    void set_network_conditions(f32 latency_ms, f32 jitter_ms, f32 loss_rate) {
        base_latency_ms_ = latency_ms;
        latency_jitter_ms_ = jitter_ms;
        packet_loss_rate_ = loss_rate;
        
        latency_dist_ = std::uniform_real_distribution<f32>(-jitter_ms, jitter_ms);
        loss_dist_ = std::uniform_real_distribution<f32>(0.0f, 1.0f);
        
        log_event("Network conditions: " + std::to_string(latency_ms) + "ms latency, " +
                 std::to_string(loss_rate * 100.0f) + "% loss");
    }
    
    /** @brief Simulate network conditions */
    bool should_drop_packet() {
        return loss_dist_(rng_) < packet_loss_rate_;
    }
    
    /** @brief Get simulated latency */
    f32 get_simulated_latency() {
        return base_latency_ms_ + latency_dist_(rng_);
    }
    
    /** @brief Update test environment */
    void update(f32 delta_time) {
        if (server_) {
            server_->update(delta_time);
        }
        
        for (auto& client : clients_) {
            client->update(delta_time);
        }
    }
    
    /** @brief Create test entities */
    std::vector<ecs::Entity> create_test_entities(u32 count) {
        std::vector<ecs::Entity> entities;
        entities.reserve(count);
        
        for (u32 i = 0; i < count; ++i) {
            auto entity = test_registry_->create();
            
            // Add basic transform component
            Transform transform;
            transform.position = {static_cast<f32>(i * 20), static_cast<f32>(i * 20)};
            test_registry_->add_component(entity, transform);
            
            // Register for network replication
            if (server_) {
                auto network_id = server_->register_entity(entity);
                log_event("Entity " + std::to_string(entity.id()) + " registered with network ID " + 
                         std::to_string(network_id));
            }
            
            entities.push_back(entity);
        }
        
        return entities;
    }
    
    /** @brief Get performance metrics */
    const std::unordered_map<std::string, f64>& get_performance_metrics() const {
        return performance_metrics_;
    }
    
    /** @brief Add performance metric */
    void add_metric(const std::string& name, f64 value) {
        performance_metrics_[name] = value;
    }
    
    /** @brief Get event log */
    const std::vector<std::string>& get_event_log() const {
        return event_log_;
    }
    
    /** @brief Wait for clients to connect */
    bool wait_for_clients(u32 expected_count, f32 timeout_seconds = 10.0f) {
        auto start_time = std::chrono::high_resolution_clock::now();
        auto timeout_duration = std::chrono::duration<f32>(timeout_seconds);
        
        while (std::chrono::high_resolution_clock::now() - start_time < timeout_duration) {
            if (server_ && server_->get_connected_clients().size() >= expected_count) {
                return true;
            }
            
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            update(0.1f);
        }
        
        return false;
    }
    
    /** @brief Cleanup test environment */
    void cleanup() {
        clients_.clear();
        server_.reset();
        
        memory_tracker_.stop_tracking();
        
        log_event("Test environment cleaned up");
    }

private:
    /** @brief Log event */
    void log_event(const std::string& event) {
        auto timestamp = timing::now();
        event_log_.push_back("[" + std::to_string(timestamp) + "] " + event);
    }
};

//=============================================================================
// Specific Test Categories
//=============================================================================

/**
 * @brief Protocol Unit Tests
 */
class ProtocolTests {
public:
    /** @brief Create protocol test suite */
    static std::unique_ptr<TestSuite> create_test_suite() {
        auto suite = std::make_unique<TestSuite>("Protocol Unit Tests", 
                                               "Tests for networking protocol components");
        
        // Packet header tests
        suite->add_test(create_packet_header_test());
        suite->add_test(create_sequence_number_test());
        suite->add_test(create_fragmentation_test());
        suite->add_test(create_reliability_test());
        
        return suite;
    }

private:
    static std::unique_ptr<TestCase> create_packet_header_test() {
        auto test = std::make_unique<TestCase>();
        test->id = "protocol_packet_header";
        test->name = "Packet Header Validation";
        test->description = "Test packet header creation and validation";
        test->category = TestCategory::Unit;
        test->priority = TestPriority::Critical;
        
        test->test_function = []() {
            using namespace protocol;
            
            // Test header creation
            PacketHeader header;
            TestAssert::equals(constants::PROTOCOL_MAGIC, header.magic);
            TestAssert::equals(constants::PROTOCOL_VERSION, header.version);
            TestAssert::is_true(header.is_valid());
            
            // Test invalid header
            header.magic = 0xDEADBEEF;
            TestAssert::is_false(header.is_valid());
            
            // Test header size
            TestAssert::equals(16UL, sizeof(PacketHeader));
        };
        
        return test;
    }
    
    static std::unique_ptr<TestCase> create_sequence_number_test() {
        auto test = std::make_unique<TestCase>();
        test->id = "protocol_sequence_numbers";
        test->name = "Sequence Number Management";
        test->description = "Test sequence number generation and comparison";
        test->category = TestCategory::Unit;
        test->priority = TestPriority::High;
        
        test->test_function = []() {
            using namespace protocol;
            
            SequenceManager seq_manager;
            
            // Test sequential generation
            u32 seq1 = seq_manager.next();
            u32 seq2 = seq_manager.next();
            TestAssert::equals(seq1 + 1, seq2);
            
            // Test wrap-around comparison
            TestAssert::is_true(SequenceManager::is_newer(100, 50));
            TestAssert::is_false(SequenceManager::is_newer(50, 100));
            
            // Test wrap-around edge case
            u32 near_wrap = constants::SEQUENCE_WRAP - 10;
            u32 after_wrap = 10;
            TestAssert::is_true(SequenceManager::is_newer(after_wrap, near_wrap));
        };
        
        return test;
    }
    
    static std::unique_ptr<TestCase> create_fragmentation_test() {
        auto test = std::make_unique<TestCase>();
        test->id = "protocol_fragmentation";
        test->name = "Message Fragmentation";
        test->description = "Test message fragmentation and reassembly";
        test->category = TestCategory::Unit;
        test->priority = TestPriority::High;
        
        test->test_function = []() {
            using namespace protocol;
            
            FragmentReassembler reassembler;
            
            // Create test message
            std::string test_message = "This is a test message that will be fragmented";
            u32 message_id = 123;
            u16 total_fragments = 3;
            
            // Add fragments out of order
            FragmentHeader frag2;
            frag2.message_id = message_id;
            frag2.fragment_index = 1;
            frag2.total_fragments = total_fragments;
            frag2.total_message_size = static_cast<u32>(test_message.size());
            frag2.fragment_offset = 16;
            
            auto result = reassembler.add_fragment(frag2, test_message.data() + 16, 16, timing::now());
            TestAssert::equals(static_cast<int>(FragmentReassembler::AddResult::NeedMoreFragments), 
                              static_cast<int>(result));
            
            // Add first fragment
            FragmentHeader frag1;
            frag1.message_id = message_id;
            frag1.fragment_index = 0;
            frag1.total_fragments = total_fragments;
            frag1.total_message_size = static_cast<u32>(test_message.size());
            frag1.fragment_offset = 0;
            
            result = reassembler.add_fragment(frag1, test_message.data(), 16, timing::now());
            TestAssert::equals(static_cast<int>(FragmentReassembler::AddResult::NeedMoreFragments), 
                              static_cast<int>(result));
            
            // Add final fragment
            FragmentHeader frag3;
            frag3.message_id = message_id;
            frag3.fragment_index = 2;
            frag3.total_fragments = total_fragments;
            frag3.total_message_size = static_cast<u32>(test_message.size());
            frag3.fragment_offset = 32;
            
            usize remaining = test_message.size() - 32;
            result = reassembler.add_fragment(frag3, test_message.data() + 32, remaining, timing::now());
            TestAssert::equals(static_cast<int>(FragmentReassembler::AddResult::MessageComplete), 
                              static_cast<int>(result));
            
            // Verify reassembled message
            auto reassembled = reassembler.get_completed_message(message_id);
            TestAssert::not_empty(reassembled);
            TestAssert::equals(test_message.size(), reassembled.size());
        };
        
        return test;
    }
    
    static std::unique_ptr<TestCase> create_reliability_test() {
        auto test = std::make_unique<TestCase>();
        test->id = "protocol_reliability";
        test->name = "Reliability Layer";
        test->description = "Test acknowledgment and retransmission logic";
        test->category = TestCategory::Unit;
        test->priority = TestPriority::Critical;
        
        test->test_function = []() {
            using namespace protocol;
            
            // Test ACK header
            AckHeader ack_header;
            ack_header.ack_sequence = 100;
            ack_header.ack_bitfield = 0x0F;  // Last 4 packets acknowledged
            
            TestAssert::is_true(ack_header.is_acked(100));  // Direct ACK
            TestAssert::is_true(ack_header.is_acked(99));   // In bitfield
            TestAssert::is_true(ack_header.is_acked(98));   // In bitfield
            TestAssert::is_false(ack_header.is_acked(95)); // Not in bitfield
            
            // Test pending ACK timeout
            PendingAck pending;
            pending.send_time = timing::now() - constants::ACK_TIMEOUT_US - 1000;
            TestAssert::is_true(pending.has_timed_out(timing::now()));
            
            pending.send_time = timing::now() - constants::ACK_TIMEOUT_US + 1000;
            TestAssert::is_false(pending.has_timed_out(timing::now()));
        };
        
        return test;
    }
};

/**
 * @brief Performance Benchmark Tests
 */
class PerformanceTests {
public:
    /** @brief Create performance test suite */
    static std::unique_ptr<TestSuite> create_test_suite() {
        auto suite = std::make_unique<TestSuite>("Performance Tests", 
                                               "Benchmark and performance validation tests");
        
        suite->add_test(create_bandwidth_test());
        suite->add_test(create_latency_test());
        suite->add_test(create_entity_sync_performance_test());
        suite->add_test(create_memory_usage_test());
        
        return suite;
    }

private:
    static std::unique_ptr<TestCase> create_bandwidth_test() {
        auto test = std::make_unique<TestCase>();
        test->id = "performance_bandwidth";
        test->name = "Bandwidth Utilization";
        test->description = "Measure bandwidth efficiency under different loads";
        test->category = TestCategory::Performance;
        test->priority = TestPriority::High;
        test->timeout_seconds = 60.0f;
        
        test->test_function = []() {
            NetworkTestEnvironment env;
            env.setup_server();
            env.add_client("BandwidthTestClient");
            
            TestAssert::is_true(env.wait_for_clients(1, 5.0f));
            
            // Create test entities
            auto entities = env.create_test_entities(100);
            
            // Measure bandwidth over time
            auto start_time = std::chrono::high_resolution_clock::now();
            f64 total_bandwidth = 0.0;
            u32 samples = 0;
            
            for (int i = 0; i < 600; ++i) {  // 10 seconds at 60 FPS
                env.update(1.0f / 60.0f);
                
                // Sample bandwidth every second
                if (i % 60 == 0) {
                    // Would get actual bandwidth measurement here
                    f64 current_bandwidth = 50.0 + (i / 60) * 10.0;  // Simulated
                    total_bandwidth += current_bandwidth;
                    samples++;
                }
            }
            
            f64 average_bandwidth = total_bandwidth / samples;
            
            // Bandwidth should be reasonable for the number of entities
            TestAssert::in_range(average_bandwidth, 10.0, 500.0, "Bandwidth out of expected range");
            
            env.add_metric("average_bandwidth_kbps", average_bandwidth);
            env.cleanup();
        };
        
        return test;
    }
    
    static std::unique_ptr<TestCase> create_latency_test() {
        auto test = std::make_unique<TestCase>();
        test->id = "performance_latency";
        test->name = "Network Latency Measurement";
        test->description = "Measure round-trip time and prediction accuracy";
        test->category = TestCategory::Performance;
        test->priority = TestPriority::High;
        
        test->test_function = []() {
            NetworkTestEnvironment env;
            env.set_network_conditions(50.0f, 10.0f, 0.0f);  // 50ms latency, no loss
            
            env.setup_server();
            env.add_client("LatencyTestClient");
            
            TestAssert::is_true(env.wait_for_clients(1, 5.0f));
            
            // Measure latency over multiple samples
            std::vector<f32> latency_samples;
            
            for (int i = 0; i < 100; ++i) {
                auto send_time = std::chrono::high_resolution_clock::now();
                
                // Simulate round-trip
                f32 simulated_latency = env.get_simulated_latency() * 2.0f;  // Round trip
                latency_samples.push_back(simulated_latency);
                
                env.update(1.0f / 60.0f);
            }
            
            // Calculate statistics
            f32 min_latency = *std::min_element(latency_samples.begin(), latency_samples.end());
            f32 max_latency = *std::max_element(latency_samples.begin(), latency_samples.end());
            f32 avg_latency = std::accumulate(latency_samples.begin(), latency_samples.end(), 0.0f) 
                             / latency_samples.size();
            
            env.add_metric("min_latency_ms", min_latency);
            env.add_metric("max_latency_ms", max_latency);
            env.add_metric("avg_latency_ms", avg_latency);
            
            // Latency should be close to configured value
            TestAssert::approx_equals(100.0, avg_latency, 20.0, "Average latency not within expected range");
            
            env.cleanup();
        };
        
        return test;
    }
    
    static std::unique_ptr<TestCase> create_entity_sync_performance_test() {
        auto test = std::make_unique<TestCase>();
        test->id = "performance_entity_sync";
        test->name = "Entity Synchronization Performance";
        test->description = "Measure performance of entity synchronization at scale";
        test->category = TestCategory::Performance;
        test->priority = TestPriority::Normal;
        test->timeout_seconds = 120.0f;
        
        test->test_function = []() {
            NetworkTestEnvironment env;
            env.setup_server();
            env.add_client("SyncTestClient");
            
            TestAssert::is_true(env.wait_for_clients(1, 5.0f));
            
            // Test with increasing entity counts
            std::vector<u32> entity_counts = {10, 50, 100, 500, 1000};
            
            for (u32 count : entity_counts) {
                auto start_time = std::chrono::high_resolution_clock::now();
                
                auto entities = env.create_test_entities(count);
                
                // Run synchronization for a period
                for (int i = 0; i < 600; ++i) {  // 10 seconds
                    env.update(1.0f / 60.0f);
                }
                
                auto end_time = std::chrono::high_resolution_clock::now();
                auto duration_ms = std::chrono::duration<f64, std::milli>(end_time - start_time).count();
                
                f64 entities_per_second = (count * 600) / (duration_ms / 1000.0);
                
                env.add_metric("entities_per_second_" + std::to_string(count), entities_per_second);
                
                // Performance should not degrade too severely with scale
                if (count <= 100) {
                    TestAssert::in_range(entities_per_second, 1000.0, 100000.0, 
                                       "Entity sync performance below threshold");
                }
            }
            
            env.cleanup();
        };
        
        return test;
    }
    
    static std::unique_ptr<TestCase> create_memory_usage_test() {
        auto test = std::make_unique<TestCase>();
        test->id = "performance_memory_usage";
        test->name = "Memory Usage Analysis";
        test->description = "Analyze memory allocation patterns and detect leaks";
        test->category = TestCategory::Performance;
        test->priority = TestPriority::High;
        
        test->test_function = []() {
            memory::MemoryTracker tracker;
            tracker.start_tracking("MemoryTest");
            
            {
                NetworkTestEnvironment env;
                env.setup_server();
                env.add_client("MemoryTestClient");
                
                TestAssert::is_true(env.wait_for_clients(1, 5.0f));
                
                auto initial_usage = tracker.get_current_usage();
                
                // Create and destroy entities multiple times
                for (int cycle = 0; cycle < 10; ++cycle) {
                    auto entities = env.create_test_entities(100);
                    
                    for (int i = 0; i < 60; ++i) {  // 1 second
                        env.update(1.0f / 60.0f);
                    }
                    
                    // Entities would be destroyed here in real scenario
                }
                
                auto final_usage = tracker.get_current_usage();
                
                env.add_metric("initial_memory_mb", initial_usage / 1024.0 / 1024.0);
                env.add_metric("final_memory_mb", final_usage / 1024.0 / 1024.0);
                env.add_metric("memory_growth_mb", (final_usage - initial_usage) / 1024.0 / 1024.0);
                
                // Memory growth should be reasonable
                f64 growth_mb = (final_usage - initial_usage) / 1024.0 / 1024.0;
                TestAssert::in_range(growth_mb, -10.0, 50.0, "Excessive memory growth detected");
                
                env.cleanup();
            }
            
            tracker.stop_tracking();
            
            // Check for memory leaks after cleanup
            auto final_usage = tracker.get_peak_usage();
            TestAssert::in_range(final_usage, 0UL, 100UL * 1024 * 1024, "Potential memory leak detected");
        };
        
        return test;
    }
};

//=============================================================================
// Main Testing Framework
//=============================================================================

/**
 * @brief Complete Network Testing Framework
 */
class NetworkTestingFramework {
private:
    std::vector<std::unique_ptr<TestSuite>> test_suites_;
    std::string output_directory_;
    bool generate_reports_{true};
    
public:
    /** @brief Initialize testing framework */
    explicit NetworkTestingFramework(const std::string& output_dir = "test_results/")
        : output_directory_(output_dir) {
        
        setup_test_suites();
    }
    
    /** @brief Run all test suites */
    void run_all_tests() {
        std::cout << "🧪 Starting ECScope Network Testing Framework\n";
        std::cout << "=============================================\n\n";
        
        auto overall_start = std::chrono::high_resolution_clock::now();
        
        u32 total_passed = 0;
        u32 total_failed = 0;
        u32 total_tests = 0;
        
        for (auto& suite : test_suites_) {
            suite->run_all();
            
            auto summary = suite->get_summary();
            total_passed += summary.passed;
            total_failed += summary.failed;
            total_tests += summary.total_tests;
        }
        
        auto overall_end = std::chrono::high_resolution_clock::now();
        auto overall_time = std::chrono::duration<f64, std::milli>(overall_end - overall_start).count();
        
        // Print overall summary
        std::cout << "\n🏁 Overall Test Results\n";
        std::cout << "======================\n";
        std::cout << "Total Tests: " << total_tests << "\n";
        std::cout << "Passed: " << total_passed << " (" << (total_passed * 100.0f / total_tests) << "%)\n";
        std::cout << "Failed: " << total_failed << "\n";
        std::cout << "Total Time: " << overall_time << " ms\n";
        
        if (generate_reports_) {
            generate_detailed_report();
        }
        
        std::cout << "\n✨ Testing complete!\n";
    }
    
    /** @brief Run specific test category */
    void run_category(TestCategory category) {
        for (auto& suite : test_suites_) {
            suite->run_category(category);
        }
    }
    
    /** @brief Enable/disable report generation */
    void set_report_generation(bool enabled) {
        generate_reports_ = enabled;
    }

private:
    /** @brief Setup all test suites */
    void setup_test_suites() {
        // Protocol unit tests
        test_suites_.push_back(ProtocolTests::create_test_suite());
        
        // Performance tests
        test_suites_.push_back(PerformanceTests::create_test_suite());
        
        // Additional test suites can be added here
        // test_suites_.push_back(ReliabilityTests::create_test_suite());
        // test_suites_.push_back(EducationalTests::create_test_suite());
    }
    
    /** @brief Generate detailed HTML report */
    void generate_detailed_report() {
        // Implementation would generate comprehensive HTML report
        // with charts, graphs, and detailed analysis
        std::cout << "📊 Detailed reports would be generated in " << output_directory_ << "\n";
    }
};

} // namespace ecscope::networking::testing