#!/bin/bash
set -euo pipefail

# ECScope Comprehensive Test Runner
# This script provides a unified interface for running all ECScope tests

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="$PROJECT_DIR/build"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Default values
TEST_TYPE="all"
BUILD_TYPE="Release"
PARALLEL_JOBS=$(nproc)
ENABLE_SANITIZERS=false
ENABLE_COVERAGE=false
GENERATE_REPORT=true
VERBOSE=false

# Function to print colored output
print_status() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

print_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Function to show usage
show_usage() {
    cat << EOF
ECScope Comprehensive Test Runner

Usage: $0 [OPTIONS]

OPTIONS:
    -t, --type TYPE          Test type to run (all|unit|integration|performance|educational)
    -b, --build-type TYPE    Build type (Debug|Release|RelWithDebInfo)
    -j, --jobs NUMBER        Number of parallel jobs for building and testing
    -s, --sanitizers         Enable sanitizers (AddressSanitizer, UBSan, etc.)
    -c, --coverage          Enable code coverage analysis
    -r, --no-report         Skip generating HTML report
    -v, --verbose           Verbose output
    -h, --help              Show this help message

EXAMPLES:
    $0                                      # Run all tests with default settings
    $0 -t unit -c                          # Run unit tests with coverage
    $0 -t performance -b Release           # Run performance tests in Release mode
    $0 -t all -s -v                       # Run all tests with sanitizers and verbose output

ENVIRONMENT VARIABLES:
    ECSCOPE_BUILD_DIR      Custom build directory (default: build)
    ECSCOPE_INSTALL_PREFIX Install prefix for ECScope
    GITHUB_ACTIONS         Set to 'true' when running in GitHub Actions

EOF
}

# Parse command line arguments
parse_args() {
    while [[ $# -gt 0 ]]; do
        case $1 in
            -t|--type)
                TEST_TYPE="$2"
                shift 2
                ;;
            -b|--build-type)
                BUILD_TYPE="$2"
                shift 2
                ;;
            -j|--jobs)
                PARALLEL_JOBS="$2"
                shift 2
                ;;
            -s|--sanitizers)
                ENABLE_SANITIZERS=true
                shift
                ;;
            -c|--coverage)
                ENABLE_COVERAGE=true
                shift
                ;;
            -r|--no-report)
                GENERATE_REPORT=false
                shift
                ;;
            -v|--verbose)
                VERBOSE=true
                shift
                ;;
            -h|--help)
                show_usage
                exit 0
                ;;
            *)
                print_error "Unknown option: $1"
                show_usage
                exit 1
                ;;
        esac
    done
}

# Function to validate prerequisites
check_prerequisites() {
    print_status "Checking prerequisites..."
    
    # Check for required tools
    local required_tools=("cmake" "ninja" "git")
    for tool in "${required_tools[@]}"; do
        if ! command -v "$tool" &> /dev/null; then
            print_error "Required tool '$tool' not found"
            exit 1
        fi
    done
    
    # Check for optional tools
    if [[ "$ENABLE_COVERAGE" == true ]]; then
        if ! command -v "lcov" &> /dev/null; then
            print_warning "lcov not found - coverage analysis will be disabled"
            ENABLE_COVERAGE=false
        fi
    fi
    
    # Check for Python (needed for report generation)
    if [[ "$GENERATE_REPORT" == true ]]; then
        if ! command -v "python3" &> /dev/null; then
            print_warning "Python3 not found - report generation will be disabled"
            GENERATE_REPORT=false
        else
            # Check for required Python packages
            python3 -c "import pandas, matplotlib, seaborn, jinja2" 2>/dev/null || {
                print_warning "Required Python packages not found - install with: pip install pandas matplotlib seaborn jinja2"
                GENERATE_REPORT=false
            }
        fi
    fi
    
    print_success "Prerequisites check completed"
}

# Function to configure build
configure_build() {
    print_status "Configuring ECScope build..."
    
    # Use custom build directory if specified
    if [[ -n "${ECSCOPE_BUILD_DIR:-}" ]]; then
        BUILD_DIR="$ECSCOPE_BUILD_DIR"
    fi
    
    # Prepare CMake arguments
    local cmake_args=(
        "-B" "$BUILD_DIR"
        "-G" "Ninja"
        "-DCMAKE_BUILD_TYPE=$BUILD_TYPE"
        "-DECSCOPE_BUILD_TESTS=ON"
        "-DECSCOPE_BUILD_BENCHMARKS=ON"
        "-DECSCOPE_BUILD_EXAMPLES=ON"
        "-DECSCOPE_ENABLE_INSTRUMENTATION=ON"
        "-DECSCOPE_ENABLE_PHYSICS=ON"
        "-DECSCOPE_ENABLE_JOB_SYSTEM=ON"
        "-DECSCOPE_ENABLE_MEMORY_ANALYSIS=ON"
        "-DECSCOPE_ENABLE_PERFORMANCE_LAB=ON"
        "-DECSCOPE_ENABLE_HARDWARE_DETECTION=ON"
    )
    
    # Add sanitizer flags if enabled
    if [[ "$ENABLE_SANITIZERS" == true ]]; then
        cmake_args+=("-DECSCOPE_ENABLE_ASAN=ON")
        cmake_args+=("-DECSCOPE_ENABLE_UBSAN=ON")
        
        # Only enable ThreadSanitizer with Clang (conflicts with AddressSanitizer in GCC)
        if [[ "${CXX:-g++}" == *"clang"* ]]; then
            cmake_args+=("-DECSCOPE_ENABLE_TSAN=ON")
        fi
    fi
    
    # Add coverage flags if enabled
    if [[ "$ENABLE_COVERAGE" == true ]]; then
        cmake_args+=("-DECSCOPE_ENABLE_COVERAGE=ON")
    fi
    
    # Add install prefix if specified
    if [[ -n "${ECSCOPE_INSTALL_PREFIX:-}" ]]; then
        cmake_args+=("-DCMAKE_INSTALL_PREFIX=$ECSCOPE_INSTALL_PREFIX")
    fi
    
    # Configure
    if [[ "$VERBOSE" == true ]]; then
        cmake "${cmake_args[@]}"
    else
        cmake "${cmake_args[@]}" > /dev/null
    fi
    
    print_success "Build configured successfully"
}

# Function to build project
build_project() {
    print_status "Building ECScope..."
    
    local build_args=("--build" "$BUILD_DIR" "--parallel" "$PARALLEL_JOBS")
    
    if [[ "$VERBOSE" == true ]]; then
        cmake "${build_args[@]}"
    else
        cmake "${build_args[@]}" > /dev/null
    fi
    
    print_success "Build completed successfully"
}

# Function to run tests
run_tests() {
    print_status "Running ECScope tests (type: $TEST_TYPE)..."
    
    cd "$BUILD_DIR"
    
    # Determine test labels based on type
    local test_labels=""
    case "$TEST_TYPE" in
        "unit")
            test_labels="-L \"unit|fast\""
            ;;
        "integration")
            test_labels="-L \"integration\""
            ;;
        "performance")
            test_labels="-L \"performance\""
            ;;
        "educational")
            test_labels="-L \"educational\""
            ;;
        "all")
            test_labels=""  # Run all tests
            ;;
        *)
            print_error "Unknown test type: $TEST_TYPE"
            exit 1
            ;;
    esac
    
    # Set up test environment
    export GTEST_COLOR=1
    export GTEST_BRIEF=1
    
    # Configure sanitizer options if enabled
    if [[ "$ENABLE_SANITIZERS" == true ]]; then
        export ASAN_OPTIONS="detect_leaks=1:abort_on_error=1:check_initialization_order=1"
        export UBSAN_OPTIONS="halt_on_error=1:abort_on_error=1"
        export TSAN_OPTIONS="halt_on_error=1:abort_on_error=1"
    fi
    
    # Run tests
    local ctest_args=("--output-on-failure")
    
    if [[ "$TEST_TYPE" != "performance" ]]; then
        ctest_args+=("--parallel" "$PARALLEL_JOBS")
    else
        # Performance tests should run sequentially for accurate results
        ctest_args+=("--parallel" "1")
    fi
    
    if [[ -n "$test_labels" ]]; then
        ctest_args+=($test_labels)
    fi
    
    if [[ "$VERBOSE" == true ]]; then
        ctest_args+=("-V")
    fi
    
    # Generate XML output for report generation
    ctest_args+=("-T" "Test")
    
    print_status "Running: ctest ${ctest_args[*]}"
    
    if ctest "${ctest_args[@]}"; then
        print_success "All tests passed!"
    else
        local exit_code=$?
        print_error "Some tests failed (exit code: $exit_code)"
        
        # Still generate report even if tests failed
        if [[ "$GENERATE_REPORT" == true ]]; then
            generate_report
        fi
        
        exit $exit_code
    fi
    
    cd "$PROJECT_DIR"
}

# Function to generate coverage report
generate_coverage() {
    if [[ "$ENABLE_COVERAGE" != true ]]; then
        return 0
    fi
    
    print_status "Generating coverage report..."
    
    cd "$BUILD_DIR"
    
    # Capture coverage data
    lcov --capture --directory . --output-file coverage.info
    
    # Remove external dependencies and test files
    lcov --remove coverage.info '/usr/*' --output-file coverage.info
    lcov --remove coverage.info '*/_deps/*' --output-file coverage.info
    lcov --remove coverage.info '*/tests/*' --output-file coverage.info
    lcov --remove coverage.info '*/test_*' --output-file coverage.info
    
    # Generate HTML report
    genhtml coverage.info --output-directory coverage_html
    
    print_success "Coverage report generated in $BUILD_DIR/coverage_html/"
    
    cd "$PROJECT_DIR"
}

# Function to generate comprehensive report
generate_report() {
    if [[ "$GENERATE_REPORT" != true ]]; then
        return 0
    fi
    
    print_status "Generating comprehensive test report..."
    
    # Prepare report generation arguments
    local report_args=(
        "--input-dir" "$BUILD_DIR"
        "--output" "$BUILD_DIR/comprehensive_test_report.html"
    )
    
    if [[ "$TEST_TYPE" == "all" || "$TEST_TYPE" == "performance" ]]; then
        report_args+=("--include-performance")
    fi
    
    if [[ "$ENABLE_COVERAGE" == true ]]; then
        report_args+=("--include-coverage")
    fi
    
    if [[ "$TEST_TYPE" == "all" || "$TEST_TYPE" == "educational" ]]; then
        report_args+=("--include-educational")
    fi
    
    # Generate report
    if python3 "$SCRIPT_DIR/generate_comprehensive_report.py" "${report_args[@]}"; then
        print_success "Comprehensive report generated: $BUILD_DIR/comprehensive_test_report.html"
        
        # Open report in browser if not in CI
        if [[ "${GITHUB_ACTIONS:-false}" != "true" ]] && [[ "${CI:-false}" != "true" ]]; then
            if command -v xdg-open &> /dev/null; then
                xdg-open "$BUILD_DIR/comprehensive_test_report.html" &
            elif command -v open &> /dev/null; then
                open "$BUILD_DIR/comprehensive_test_report.html" &
            fi
        fi
    else
        print_warning "Failed to generate comprehensive report"
    fi
}

# Function to clean up
cleanup() {
    print_status "Performing cleanup..."
    
    # Reset any modified environment variables
    unset ASAN_OPTIONS UBSAN_OPTIONS TSAN_OPTIONS
    unset GTEST_COLOR GTEST_BRIEF
    
    print_success "Cleanup completed"
}

# Function to display summary
display_summary() {
    print_status "Test Summary:"
    echo "  Test Type: $TEST_TYPE"
    echo "  Build Type: $BUILD_TYPE" 
    echo "  Parallel Jobs: $PARALLEL_JOBS"
    echo "  Sanitizers: $ENABLE_SANITIZERS"
    echo "  Coverage: $ENABLE_COVERAGE"
    echo "  Generate Report: $GENERATE_REPORT"
    echo "  Build Directory: $BUILD_DIR"
    
    if [[ -f "$BUILD_DIR/test_summary.json" ]]; then
        echo ""
        echo "Detailed Results:"
        if command -v jq &> /dev/null; then
            jq -r '
                "  Unit Tests: " + (.unit_tests.passed | tostring) + "/" + (.unit_tests.total | tostring) + " passed",
                "  Integration Tests: " + (.integration_tests.passed | tostring) + "/" + (.integration_tests.total | tostring) + " passed", 
                "  Educational Tests: " + (.educational_tests.passed | tostring) + "/" + (.educational_tests.total | tostring) + " passed",
                "  Performance Status: " + .performance_tests.status,
                "  Coverage: " + (.coverage | tostring) + "%",
                "  Total Duration: " + (.total_duration | tostring) + "s"
            ' "$BUILD_DIR/test_summary.json"
        else
            cat "$BUILD_DIR/test_summary.json"
        fi
    fi
}

# Trap for cleanup on exit
trap cleanup EXIT

# Main execution
main() {
    echo "ECScope Comprehensive Test Runner"
    echo "================================="
    
    parse_args "$@"
    check_prerequisites
    configure_build
    build_project
    run_tests
    generate_coverage
    generate_report
    display_summary
    
    print_success "All operations completed successfully!"
}

# Run main function with all arguments
main "$@"