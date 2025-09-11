# ECScope Comprehensive Testing Framework

## Overview

This document describes the professional-grade testing framework built specifically for the ECScope game engine. The framework provides comprehensive testing capabilities including unit testing, integration testing, performance benchmarking, memory leak detection, stress testing, and continuous integration support.

## Table of Contents

1. [Framework Architecture](#framework-architecture)
2. [Test Categories](#test-categories)
3. [Getting Started](#getting-started)
4. [Writing Tests](#writing-tests)
5. [Running Tests](#running-tests)
6. [Performance Testing](#performance-testing)
7. [Memory Testing](#memory-testing)
8. [CI/CD Integration](#cicd-integration)
9. [Configuration Options](#configuration-options)
10. [Best Practices](#best-practices)

## Framework Architecture

### Core Components

- **Test Framework (`test_framework.hpp`)**: Core testing infrastructure with base classes, assertions, and fixtures
- **Test Runner (`test_runner.hpp`)**: Advanced test execution engine with parallel execution, filtering, and reporting
- **Test Discovery (`test_main.hpp`)**: Automatic test registration and discovery system
- **Specialized Testing Modules**:
  - `ecs_testing.hpp`: ECS-specific testing utilities
  - `rendering_testing.hpp`: Graphics and rendering validation
  - `physics_testing.hpp`: Physics simulation testing
  - `memory_testing.hpp`: Memory leak detection and profiling

### Key Features

- **Multi-paradigm Testing**: Unit, integration, performance, stress, and regression testing
- **Parallel Execution**: Thread-safe test execution with race condition detection
- **Memory Profiling**: Detailed memory leak detection with stack traces
- **Performance Monitoring**: Benchmarking with regression detection
- **Rich Reporting**: Console, XML, JSON, and HTML output formats
- **CI/CD Ready**: GitHub Actions integration with comprehensive workflows

## Test Categories

| Category | Description | Use Cases |
|----------|-------------|-----------|
| **Unit** | Test individual components in isolation | Component validation, algorithm correctness |
| **Integration** | Test system interactions | ECS system coordination, pipeline validation |
| **Performance** | Benchmark critical code paths | Frame rate validation, memory allocation profiling |
| **Memory** | Detect leaks and analyze usage | Memory safety, allocation patterns |
| **Stress** | Test under extreme conditions | Stability under load, resource exhaustion |
| **Regression** | Detect performance degradation | Continuous quality assurance |
| **Rendering** | Graphics and visual validation | Pixel-perfect comparison, shader compilation |
| **Physics** | Simulation accuracy testing | Determinism, conservation laws |
| **Multithreaded** | Concurrency and thread safety | Race condition detection, deadlock prevention |

## Getting Started

### Prerequisites

- CMake 3.22+
- C++20 compatible compiler (GCC 12+, Clang 15+, MSVC 2022)
- Platform-specific development tools (see CI configuration)

### Building the Testing Framework

```bash
# Configure with testing enabled
cmake -B build -DECSCOPE_BUILD_TESTS=ON \
                -DECSCOPE_ENABLE_TESTING_FRAMEWORK=ON \
                -DECSCOPE_ENABLE_MEMORY_TESTING=ON \
                -DECSCOPE_ENABLE_PERFORMANCE_TESTING=ON

# Build test executables
cmake --build build --parallel

# Run all tests
cd build && ctest --verbose
```

### Quick Test Run

```bash
# Run all tests with verbose output
./build/bin/ecscope_test_runner --verbose

# Run specific test categories
./build/bin/ecscope_test_runner --include-category=unit
./build/bin/ecscope_test_runner --include-category=performance
./build/bin/ecscope_test_runner --include-category=memory

# Generate HTML report
./build/bin/ecscope_test_runner --output-format=html --output-file=report.html
```

## Writing Tests

### Basic Unit Test

```cpp
#include <ecscope/testing/test_framework.hpp>

using namespace ecscope::testing;

class MyComponentTest : public TestCase {
public:
    MyComponentTest() : TestCase("My Component Test", TestCategory::UNIT) {}

    void run() override {
        // Test your component
        MyComponent component;
        component.initialize();
        
        ASSERT_TRUE(component.is_valid());
        ASSERT_EQ(component.get_value(), 42);
    }
};

// Register the test
REGISTER_TEST(MyComponentTest);
```

### Integration Test with Fixtures

```cpp
#include <ecscope/testing/ecs_testing.hpp>

class SystemIntegrationTest : public ECSTestFixture {
public:
    SystemIntegrationTest() : ECSTestFixture() {
        context_.name = "System Integration Test";
        context_.category = TestCategory::INTEGRATION;
    }

    void run() override {
        // Create test entities
        create_test_entities(100);
        
        // Test system interactions
        world_->update(1.0f / 60.0f);
        
        // Validate results
        ASSERT_TRUE(world_->is_valid());
    }
};

REGISTER_TEST(SystemIntegrationTest);
```

### Performance Benchmark

```cpp
#include <ecscope/testing/test_framework.hpp>

class EntityCreationBenchmark : public BenchmarkTest {
public:
    EntityCreationBenchmark() : BenchmarkTest("Entity Creation", 10000) {
        context_.category = TestCategory::PERFORMANCE;
    }

    void setup() override {
        world_ = std::make_unique<ecscope::World>();
    }

    void benchmark() override {
        world_->create_entity();
    }

    void teardown() override {
        world_.reset();
    }

private:
    std::unique_ptr<ecscope::World> world_;
};

REGISTER_TEST(EntityCreationBenchmark);
```

### Parameterized Test

```cpp
class ScalingTest : public ParameterizedTest<int> {
public:
    ScalingTest() : ParameterizedTest("Scaling Test", {100, 1000, 10000}) {
        context_.category = TestCategory::PERFORMANCE;
    }

    void run_with_parameter(const int& entity_count, size_t index) override {
        auto world = std::make_unique<ecscope::World>();
        
        auto start = std::chrono::high_resolution_clock::now();
        
        for (int i = 0; i < entity_count; ++i) {
            world->create_entity();
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        
        // Verify performance scales linearly
        double microseconds_per_entity = static_cast<double>(duration.count()) / entity_count;
        ASSERT_LT(microseconds_per_entity, 10.0);
    }
};

REGISTER_TEST(ScalingTest);
```

## Running Tests

### Command Line Options

```bash
# Help and information
ecscope_test_runner --help
ecscope_test_runner --list-tests

# Filtering tests
ecscope_test_runner --filter="Physics.*"
ecscope_test_runner --include-tag=fast
ecscope_test_runner --exclude-tag=slow
ecscope_test_runner --include-category=unit

# Execution control
ecscope_test_runner --no-parallel
ecscope_test_runner --shuffle
ecscope_test_runner --repeat=5
ecscope_test_runner --stop-on-failure

# Output and reporting
ecscope_test_runner --verbose
ecscope_test_runner --output-format=xml --output-file=results.xml
ecscope_test_runner --output-format=json --output-file=results.json
ecscope_test_runner --output-format=html --output-file=report.html

# Performance and memory
ecscope_test_runner --baseline-file=performance_baseline.json
ecscope_test_runner --no-memory-tracking
ecscope_test_runner --timeout=600
```

### CMake Integration

```bash
# Run tests through CMake/CTest
ctest --verbose
ctest -L unit                    # Run only unit tests
ctest -L "integration"           # Run integration tests
ctest --parallel 4               # Run tests in parallel

# Custom targets
make test_unit                   # Unit tests only
make test_integration           # Integration tests only
make test_performance           # Performance tests only
make test_all                   # All tests with HTML report
make coverage                   # Generate coverage report (if enabled)
```

## Performance Testing

### Benchmarking Framework

The framework includes a sophisticated benchmarking system:

- **Statistical Analysis**: Mean, standard deviation, min/max times
- **Regression Detection**: Automatic comparison with baselines
- **Scalability Testing**: Performance validation across different input sizes
- **Memory Profiling**: Allocation tracking during benchmarks

### Example Performance Test Output

```
[PERFORMANCE] Entity Creation Benchmark (10000 iterations)
  Mean time: 45.2ns per iteration
  Std deviation: 2.1ns
  Min time: 42.8ns
  Max time: 98.3ns
  Entities per second: 22,123,894
  Memory allocated: 1,250,000 bytes
  Peak memory: 1,312,500 bytes
```

### Performance Regression Detection

```cpp
// Automatic regression detection
runner.set_regression_baseline_file("performance_baselines.json");

// Tests will automatically compare against baseline
// and report regressions > 20% slower
```

## Memory Testing

### Memory Leak Detection

The framework provides comprehensive memory tracking:

- **Allocation Tracking**: Every malloc/new with stack traces
- **Leak Detection**: Automatic detection of unfreed memory
- **Fragmentation Analysis**: Memory layout optimization insights
- **Category Tracking**: Memory usage by subsystem

### Memory Test Example

```cpp
class MemoryLeakTest : public MemoryTestFixture {
public:
    MemoryLeakTest() : MemoryTestFixture() {
        context_.name = "Memory Leak Detection";
        context_.category = TestCategory::MEMORY;
    }

    void run() override {
        // Allocate memory with tracking
        allocate_and_track(1024, AllocationCategory::ECS_COMPONENT, "test_component");
        
        // Framework automatically detects leaks in teardown()
        cleanup_test_allocations();
    }
};
```

### Memory Report Output

```
Memory Usage Report
==================

Summary:
  Total Allocated: 15,728,640 bytes
  Total Deallocated: 15,728,640 bytes
  Current Usage: 0 bytes
  Peak Usage: 2,097,152 bytes
  Allocation Count: 1,250
  Deallocation Count: 1,250
  Leaked Allocations: 0
  Leaked Bytes: 0

Usage by Category:
  ECS Component: 8,388,608 bytes
  Rendering Buffer: 4,194,304 bytes
  Physics Body: 2,097,152 bytes
  Temporary: 1,048,576 bytes
```

## CI/CD Integration

### GitHub Actions Workflow

The framework includes a comprehensive CI/CD pipeline:

- **Multi-platform Testing**: Ubuntu, Windows, macOS
- **Compiler Matrix**: GCC, Clang, MSVC
- **Static Analysis**: cppcheck, clang-tidy, SonarCloud
- **Memory Testing**: Valgrind, AddressSanitizer
- **Performance Monitoring**: Automated regression detection
- **Security Testing**: CodeQL analysis
- **Documentation**: Automated doc generation and coverage

### Workflow Triggers

- **Push/PR**: Full test suite on main/develop branches
- **Nightly**: Comprehensive testing including stress tests
- **Release**: Full validation and package creation

### Test Results Integration

- **Codecov**: Code coverage reporting
- **SonarCloud**: Code quality metrics
- **GitHub Releases**: Automated package creation
- **Slack Notifications**: Failure alerts

## Configuration Options

### CMake Options

```cmake
# Core testing options
ECSCOPE_BUILD_TESTS=ON                    # Enable test building
ECSCOPE_ENABLE_TESTING_FRAMEWORK=ON      # Use comprehensive framework
ECSCOPE_ENABLE_MEMORY_TESTING=ON         # Memory leak detection
ECSCOPE_ENABLE_PERFORMANCE_TESTING=ON    # Performance benchmarks
ECSCOPE_ENABLE_STRESS_TESTING=ON         # Stress testing

# Advanced options
ECSCOPE_ENABLE_COVERAGE=OFF              # Code coverage analysis
ECSCOPE_ENABLE_SANITIZERS=OFF            # AddressSanitizer, etc.
ECSCOPE_ENABLE_FUZZING=OFF               # Fuzz testing
```

### Runtime Configuration

Create `test_config.json` for runtime configuration:

```json
{
  "parallel_execution": true,
  "max_threads": 8,
  "timeout_seconds": 300,
  "memory_tracking": true,
  "performance_tracking": true,
  "output_format": "html",
  "regression_threshold": 1.2,
  "categories": {
    "unit": { "timeout": 60 },
    "integration": { "timeout": 300 },
    "performance": { "timeout": 600 }
  }
}
```

## Best Practices

### Test Organization

1. **One Test Per Concept**: Each test should validate one specific behavior
2. **Descriptive Names**: Test names should clearly describe what is being tested
3. **Proper Categories**: Use appropriate test categories for organization
4. **Independent Tests**: Tests should not depend on each other's state

### Performance Testing

1. **Warm-up Runs**: Include warm-up iterations for consistent results
2. **Statistical Significance**: Run enough iterations for meaningful statistics
3. **Baseline Management**: Maintain performance baselines in version control
4. **Environment Consistency**: Run performance tests on consistent hardware

### Memory Testing

1. **Clean Fixtures**: Ensure test fixtures properly clean up resources
2. **Category Tagging**: Use memory allocation categories for better tracking
3. **Leak Tolerance**: Define acceptable leak thresholds for different test types
4. **Stack Trace Quality**: Compile with debug symbols for better stack traces

### Integration Testing

1. **Real Dependencies**: Use actual systems rather than mocks when possible
2. **Data-driven Tests**: Use external data files for comprehensive test cases
3. **Error Scenarios**: Test both success and failure paths
4. **Resource Management**: Properly initialize and cleanup shared resources

### Continuous Integration

1. **Fast Feedback**: Optimize test suite for quick developer feedback
2. **Parallel Execution**: Utilize parallel testing for faster CI runs
3. **Artifact Management**: Save test reports and logs for analysis
4. **Failure Analysis**: Provide clear failure messages and debugging information

## Troubleshooting

### Common Issues

**Tests fail with memory access violations**
- Enable AddressSanitizer: `-DECSCOPE_ENABLE_SANITIZERS=ON`
- Check for use-after-free or buffer overruns
- Verify proper test fixture cleanup

**Performance tests are inconsistent**
- Run on dedicated hardware or CI agents
- Increase iteration count for more stable results
- Check for background processes affecting performance

**Tests hang in CI**
- Verify timeout settings are appropriate
- Check for deadlocks in multithreaded tests
- Ensure proper cleanup in test fixtures

**Memory leaks reported incorrectly**
- Verify static initialization cleanup
- Check for global object destruction order
- Use memory leak suppression files if needed

### Debug Mode

Enable verbose debugging:

```bash
ecscope_test_runner --verbose --no-parallel --filter="MyTest"
```

### Profiling

Profile test performance:

```bash
# With perf (Linux)
perf record -g ./ecscope_test_runner --include-category=performance
perf report

# With Instruments (macOS)
instruments -t "Time Profiler" ./ecscope_test_runner

# With Visual Studio (Windows)
# Use the VS profiler integrated with the IDE
```

## Contributing

When adding new tests to the framework:

1. Follow the established patterns for your test type
2. Add appropriate documentation and examples
3. Ensure tests are deterministic and can run in parallel
4. Include both positive and negative test cases
5. Update this documentation for new features

## Support

For issues with the testing framework:

1. Check the troubleshooting section above
2. Review CI logs for detailed error information
3. Create issues in the project repository with:
   - Test command used
   - Full error output
   - System information (OS, compiler, etc.)
   - Minimal reproduction case

The testing framework is designed to be a comprehensive solution for game engine development, providing the quality assurance tools needed for professional game development projects.