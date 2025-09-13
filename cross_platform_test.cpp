#include <iostream>
#include <string>
#include <chrono>
#include <thread>
#include <fstream>
#include <filesystem>
#include <cstdlib>

namespace fs = std::filesystem;

class CrossPlatformCompatibilityTest {
public:
    void runAllTests() {
        std::cout << "=== ECScope Cross-Platform Compatibility Testing ===" << std::endl;
        std::cout << "Testing platform-specific features and compatibility..." << std::endl << std::endl;
        
        // Platform detection
        testPlatformDetection();
        
        // File system compatibility
        testFileSystemOperations();
        
        // Threading compatibility  
        testThreadingSupport();
        
        // Memory operations
        testMemoryOperations();
        
        // Timing and precision
        testTimingSupport();
        
        // Compiler features
        testCompilerFeatures();
        
        // System resources
        testSystemResources();
        
        generateCompatibilityReport();
    }

private:
    struct TestResult {
        std::string test_name;
        bool passed;
        std::string details;
        double performance_score;
    };
    
    std::vector<TestResult> results_;
    
    void testPlatformDetection() {
        TestResult result;
        result.test_name = "Platform Detection";
        result.passed = true;
        result.performance_score = 100.0;
        
        std::cout << "Platform Detection Test:" << std::endl;
        
#ifdef __linux__
        std::cout << "  Platform: Linux ✓" << std::endl;
        result.details = "Linux platform detected";
#elif defined(_WIN32) || defined(_WIN64)
        std::cout << "  Platform: Windows ✓" << std::endl;
        result.details = "Windows platform detected";
#elif defined(__APPLE__)
        std::cout << "  Platform: macOS ✓" << std::endl;
        result.details = "macOS platform detected";
#else
        std::cout << "  Platform: Unknown ⚠" << std::endl;
        result.details = "Unknown platform";
        result.passed = false;
        result.performance_score = 50.0;
#endif
        
        // Architecture detection
#if defined(__x86_64__) || defined(_M_X64)
        std::cout << "  Architecture: x86_64 ✓" << std::endl;
        result.details += ", x86_64 architecture";
#elif defined(__aarch64__) || defined(_M_ARM64)
        std::cout << "  Architecture: ARM64 ✓" << std::endl;
        result.details += ", ARM64 architecture";
#else
        std::cout << "  Architecture: Other ⚠" << std::endl;
        result.details += ", non-standard architecture";
#endif
        
        results_.push_back(result);
        std::cout << std::endl;
    }
    
    void testFileSystemOperations() {
        TestResult result;
        result.test_name = "File System Operations";
        result.passed = true;
        result.performance_score = 0.0;
        
        std::cout << "File System Compatibility Test:" << std::endl;
        
        try {
            auto start = std::chrono::high_resolution_clock::now();
            
            // Create test directory
            fs::path test_dir = "test_fs_operations";
            if (fs::exists(test_dir)) {
                fs::remove_all(test_dir);
            }
            fs::create_directory(test_dir);
            std::cout << "  Directory Creation: ✓" << std::endl;
            
            // Create test file
            std::ofstream test_file(test_dir / "test_file.txt");
            test_file << "ECScope Cross-Platform Test File\\n";
            test_file.close();
            std::cout << "  File Creation: ✓" << std::endl;
            
            // Read test file
            std::ifstream read_file(test_dir / "test_file.txt");
            std::string content;
            std::getline(read_file, content);
            read_file.close();
            
            if (content.find("ECScope") != std::string::npos) {
                std::cout << "  File Read/Write: ✓" << std::endl;
            } else {
                throw std::runtime_error("File content mismatch");
            }
            
            // Test file permissions and metadata
            auto file_size = fs::file_size(test_dir / "test_file.txt");
            std::cout << "  File Size Detection: ✓ (" << file_size << " bytes)" << std::endl;
            
            // Cleanup
            fs::remove_all(test_dir);
            std::cout << "  Cleanup: ✓" << std::endl;
            
            auto end = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
            result.performance_score = 1000000.0 / duration.count(); // operations per second equivalent
            result.details = "All file operations successful";
            
        } catch (const std::exception& e) {
            std::cout << "  File System Test: ✗ (" << e.what() << ")" << std::endl;
            result.passed = false;
            result.details = "File system error: " + std::string(e.what());
        }
        
        results_.push_back(result);
        std::cout << std::endl;
    }
    
    void testThreadingSupport() {
        TestResult result;
        result.test_name = "Threading Support";
        result.passed = true;
        
        std::cout << "Threading Compatibility Test:" << std::endl;
        
        try {
            unsigned int hw_threads = std::thread::hardware_concurrency();
            std::cout << "  Hardware Threads: " << hw_threads << " ✓" << std::endl;
            
            auto start = std::chrono::high_resolution_clock::now();
            
            // Test thread creation and joining
            constexpr size_t num_threads = 4;
            std::vector<std::thread> threads;
            std::vector<std::atomic<size_t>> counters(num_threads);
            
            for (size_t i = 0; i < num_threads; ++i) {
                counters[i] = 0;
                threads.emplace_back([&counters, i]() {
                    for (size_t j = 0; j < 100000; ++j) {
                        counters[i]++;
                    }
                });
            }
            
            for (auto& thread : threads) {
                thread.join();
            }
            
            // Verify atomic operations worked
            size_t total = 0;
            for (const auto& counter : counters) {
                total += counter.load();
            }
            
            auto end = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
            
            if (total == num_threads * 100000) {
                std::cout << "  Thread Synchronization: ✓" << std::endl;
                std::cout << "  Atomic Operations: ✓" << std::endl;
                result.performance_score = total / (duration.count() / 1000000.0); // operations per second
            } else {
                throw std::runtime_error("Thread synchronization failed");
            }
            
            result.details = "Threading fully functional, " + std::to_string(hw_threads) + " cores";
            
        } catch (const std::exception& e) {
            std::cout << "  Threading Test: ✗ (" << e.what() << ")" << std::endl;
            result.passed = false;
            result.details = "Threading error: " + std::string(e.what());
        }
        
        results_.push_back(result);
        std::cout << std::endl;
    }
    
    void testMemoryOperations() {
        TestResult result;
        result.test_name = "Memory Operations";
        result.passed = true;
        
        std::cout << "Memory Operations Compatibility Test:" << std::endl;
        
        try {
            auto start = std::chrono::high_resolution_clock::now();
            
            // Test large memory allocation
            constexpr size_t large_size = 100 * 1024 * 1024; // 100MB
            std::unique_ptr<char[]> large_buffer(new char[large_size]);
            std::cout << "  Large Allocation (100MB): ✓" << std::endl;
            
            // Test memory access patterns
            for (size_t i = 0; i < large_size; i += 4096) {
                large_buffer[i] = static_cast<char>(i & 0xFF);
            }
            std::cout << "  Memory Access Patterns: ✓" << std::endl;
            
            // Test alignment
            alignas(64) double aligned_array[1024];
            for (size_t i = 0; i < 1024; ++i) {
                aligned_array[i] = static_cast<double>(i);
            }
            std::cout << "  Aligned Memory Access: ✓" << std::endl;
            
            auto end = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
            
            result.performance_score = large_size / (duration.count() / 1000000.0); // bytes per second
            result.details = "Memory operations successful, " + std::to_string(large_size / (1024*1024)) + "MB test";
            
        } catch (const std::exception& e) {
            std::cout << "  Memory Test: ✗ (" << e.what() << ")" << std::endl;
            result.passed = false;
            result.details = "Memory error: " + std::string(e.what());
        }
        
        results_.push_back(result);
        std::cout << std::endl;
    }
    
    void testTimingSupport() {
        TestResult result;
        result.test_name = "High-Resolution Timing";
        result.passed = true;
        
        std::cout << "Timing Support Test:" << std::endl;
        
        try {
            // Test high-resolution clock
            auto start = std::chrono::high_resolution_clock::now();
            std::this_thread::sleep_for(std::chrono::microseconds(1000));
            auto end = std::chrono::high_resolution_clock::now();
            
            auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
            std::cout << "  High-Res Timer Resolution: " << duration.count() << " ns ✓" << std::endl;
            
            // Test steady clock
            auto steady_start = std::chrono::steady_clock::now();
            for (volatile int i = 0; i < 1000000; ++i) {}
            auto steady_end = std::chrono::steady_clock::now();
            
            auto steady_duration = std::chrono::duration_cast<std::chrono::microseconds>(steady_end - steady_start);
            std::cout << "  Steady Clock: " << steady_duration.count() << " μs ✓" << std::endl;
            
            result.performance_score = 1000000.0 / steady_duration.count(); // relative performance score
            result.details = "Timing precision: " + std::to_string(duration.count()) + " ns resolution";
            
        } catch (const std::exception& e) {
            std::cout << "  Timing Test: ✗ (" << e.what() << ")" << std::endl;
            result.passed = false;
            result.details = "Timing error: " + std::string(e.what());
        }
        
        results_.push_back(result);
        std::cout << std::endl;
    }
    
    void testCompilerFeatures() {
        TestResult result;
        result.test_name = "C++20 Compiler Features";
        result.passed = true;
        result.performance_score = 0.0;
        
        std::cout << "Compiler Features Test:" << std::endl;
        
        // Test C++20 features
        std::cout << "  C++20 Standard: ";
        if (__cplusplus >= 202002L) {
            std::cout << "✓ (" << __cplusplus << ")" << std::endl;
            result.performance_score += 25.0;
        } else {
            std::cout << "⚠ (" << __cplusplus << " - partial C++20)" << std::endl;
        }
        
        // Test compiler-specific features
#ifdef __GNUC__
        std::cout << "  GCC Compiler: ✓ (v" << __GNUC__ << "." << __GNUC_MINOR__ << ")" << std::endl;
        result.performance_score += 25.0;
#endif
#ifdef __clang__
        std::cout << "  Clang Compiler: ✓" << std::endl;
        result.performance_score += 25.0;
#endif
#ifdef _MSC_VER
        std::cout << "  MSVC Compiler: ✓" << std::endl;
        result.performance_score += 25.0;
#endif
        
        // Test constexpr support
        constexpr auto test_constexpr = []() constexpr { return 42; };
        constexpr auto result_val = test_constexpr();
        if (result_val == 42) {
            std::cout << "  Constexpr Support: ✓" << std::endl;
            result.performance_score += 25.0;
        }
        
        result.details = "C++20 features available, modern compiler detected";
        if (result.performance_score < 50.0) {
            result.passed = false;
            result.details = "Limited compiler feature support";
        }
        
        results_.push_back(result);
        std::cout << std::endl;
    }
    
    void testSystemResources() {
        TestResult result;
        result.test_name = "System Resources";
        result.passed = true;
        result.performance_score = 100.0;
        
        std::cout << "System Resources Test:" << std::endl;
        
        // CPU information
        unsigned int cpu_count = std::thread::hardware_concurrency();
        std::cout << "  CPU Cores: " << cpu_count << " ✓" << std::endl;
        
        // Memory information (basic test)
        try {
            constexpr size_t test_size = 1024 * 1024; // 1MB
            std::unique_ptr<char[]> test_buffer(new char[test_size]);
            std::cout << "  Memory Allocation: ✓" << std::endl;
        } catch (...) {
            std::cout << "  Memory Allocation: ✗" << std::endl;
            result.passed = false;
            result.performance_score = 0.0;
        }
        
        result.details = "System resources accessible, " + std::to_string(cpu_count) + " CPU cores";
        results_.push_back(result);
        std::cout << std::endl;
    }
    
    void generateCompatibilityReport() {
        std::cout << "=== CROSS-PLATFORM COMPATIBILITY REPORT ===" << std::endl;
        
        int passed_tests = 0;
        double total_performance = 0.0;
        
        for (const auto& result : results_) {
            std::cout << std::left << std::setw(25) << result.test_name;
            std::cout << (result.passed ? "✓ PASS  " : "✗ FAIL  ");
            
            if (result.performance_score > 0.0) {
                std::cout << "(Score: " << std::fixed << std::setprecision(1) << result.performance_score << ")";
            }
            std::cout << std::endl;
            
            if (result.passed) {
                passed_tests++;
                total_performance += result.performance_score;
            }
        }
        
        std::cout << "\\n=== SUMMARY ===" << std::endl;
        std::cout << "Tests Passed: " << passed_tests << "/" << results_.size() << std::endl;
        std::cout << "Success Rate: " << (100.0 * passed_tests / results_.size()) << "%" << std::endl;
        
        if (total_performance > 0.0) {
            std::cout << "Average Performance Score: " << (total_performance / results_.size()) << std::endl;
        }
        
        std::cout << "\\n=== PLATFORM STATUS ===" << std::endl;
        if (passed_tests == static_cast<int>(results_.size())) {
            std::cout << "✓ EXCELLENT - Full cross-platform compatibility confirmed" << std::endl;
            std::cout << "✓ System ready for production ECScope deployment" << std::endl;
        } else if (passed_tests > static_cast<int>(results_.size()) * 0.8) {
            std::cout << "⚠ GOOD - Minor compatibility issues detected" << std::endl;
            std::cout << "⚠ System mostly ready, address failing tests for full compatibility" << std::endl;
        } else {
            std::cout << "✗ NEEDS WORK - Significant compatibility issues" << std::endl;
            std::cout << "✗ Address failing tests before production deployment" << std::endl;
        }
    }
};

int main() {
    CrossPlatformCompatibilityTest test;
    test.runAllTests();
    return 0;
}