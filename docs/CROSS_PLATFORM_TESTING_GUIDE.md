# ECScope Cross-Platform Testing Guide

Comprehensive guide for testing ECScope UI system compatibility across Windows, Linux, and macOS platforms.

## Table of Contents

1. [Overview](#overview)
2. [Platform-Specific Considerations](#platform-specific-considerations)
3. [Build Instructions](#build-instructions)
4. [Testing Framework](#testing-framework)
5. [Automated Testing](#automated-testing)
6. [Troubleshooting](#troubleshooting)
7. [Performance Benchmarking](#performance-benchmarking)
8. [CI/CD Integration](#cicd-integration)

## Overview

ECScope is designed to work seamlessly across major desktop platforms. This guide covers comprehensive testing strategies to ensure compatibility and optimal performance on:

- **Windows 10/11** (MSVC, MinGW)
- **Linux** (Ubuntu, CentOS, Arch, etc.)
- **macOS** (10.15+, Intel and Apple Silicon)

### Key Components Tested

- Dear ImGui integration
- GLFW window management
- OpenGL rendering backend
- File system operations
- DPI scaling and high-DPI displays
- Threading and synchronization
- Platform-specific UI behaviors

## Platform-Specific Considerations

### Windows

#### Dependencies
```bash
# Using vcpkg (recommended)
vcpkg install glfw3:x64-windows opengl:x64-windows imgui:x64-windows

# Or using Conan
conan install . --build=missing -s os=Windows -s arch=x86_64
```

#### Build Requirements
- Visual Studio 2019/2022 (MSVC)
- Windows SDK 10.0.19041.0 or later
- CMake 3.22+

#### Windows-Specific Features
- DPI awareness (Per-Monitor V2)
- High contrast theme support
- Windows 11 rounded corners
- File dialog integration

#### Known Issues
- OpenGL context creation may fail on some Intel integrated graphics
- DPI scaling can cause font rendering issues on mixed-DPI setups

### Linux

#### Dependencies
```bash
# Ubuntu/Debian
sudo apt-get install build-essential cmake ninja-build pkg-config \
    libgl1-mesa-dev libglu1-mesa-dev \
    libxrandr-dev libxinerama-dev libxcursor-dev libxi-dev \
    libx11-dev libxext-dev libxfixes-dev \
    libasound2-dev

# Fedora/CentOS
sudo dnf install gcc-c++ cmake ninja-build pkg-config \
    mesa-libGL-devel mesa-libGLU-devel \
    libXrandr-devel libXinerama-devel libXcursor-devel libXi-devel \
    libX11-devel libXext-devel libXfixes-devel \
    alsa-lib-devel

# Arch Linux
sudo pacman -S base-devel cmake ninja pkg-config \
    mesa glu libxrandr libxinerama libxcursor libxi \
    libx11 libxext libxfixes alsa-lib
```

#### Build Requirements
- GCC 10+ or Clang 12+
- X11 or Wayland development libraries
- CMake 3.22+

#### Linux-Specific Features
- X11 and Wayland compatibility
- Multiple window manager support
- XDG Base Directory specification
- Desktop environment integration

#### Known Issues
- Wayland support may be limited depending on compositor
- Some window managers don't support fullscreen properly
- Font rendering varies between distributions

### macOS

#### Dependencies
```bash
# Using Homebrew
brew install cmake ninja pkg-config

# Using MacPorts
sudo port install cmake ninja pkgconfig
```

#### Build Requirements
- Xcode 12+ or Command Line Tools
- macOS 10.15 (Catalina) or later
- CMake 3.22+

#### macOS-Specific Features
- Retina display support
- Metal rendering backend (optional)
- Native menu bar integration
- App Bundle creation
- Notarization support

#### Known Issues
- Metal backend may not work on older Intel Macs
- Fullscreen behavior differs from other platforms
- App Bundle requires code signing for distribution

## Build Instructions

### Quick Start

```bash
# Clone repository
git clone https://github.com/your-org/ecscope.git
cd ecscope

# Run automated cross-platform build
python3 scripts/cross_platform/build_and_test.py
```

### Manual Build

#### Windows (Visual Studio)

```batch
# Configure
cmake -B build -G "Visual Studio 17 2022" -A x64 ^
    -DECSCOPE_BUILD_TESTS=ON ^
    -DECSCOPE_BUILD_CROSS_PLATFORM_TESTS=ON ^
    -DECSCOPE_BUILD_GUI=ON

# Build
cmake --build build --config Release --parallel

# Test
cd build
ctest --build-config Release --parallel --output-on-failure
```

#### Linux (Ninja)

```bash
# Configure
cmake -B build -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DECSCOPE_BUILD_TESTS=ON \
    -DECSCOPE_BUILD_CROSS_PLATFORM_TESTS=ON \
    -DECSCOPE_BUILD_GUI=ON

# Build
cmake --build build --parallel

# Test
cd build
ctest --parallel --output-on-failure
```

#### macOS (Xcode)

```bash
# Configure
cmake -B build -G Xcode \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_OSX_DEPLOYMENT_TARGET=10.15 \
    -DECSCOPE_BUILD_TESTS=ON \
    -DECSCOPE_BUILD_CROSS_PLATFORM_TESTS=ON \
    -DECSCOPE_BUILD_GUI=ON

# Build
cmake --build build --config Release --parallel

# Test
cd build
ctest --build-config Release --parallel --output-on-failure
```

### CMake Configuration Options

```cmake
# Core options
ECSCOPE_BUILD_TESTS=ON                    # Build test suite
ECSCOPE_BUILD_CROSS_PLATFORM_TESTS=ON     # Build cross-platform tests
ECSCOPE_BUILD_GUI=ON                      # Build GUI system
ECSCOPE_BUILD_EXAMPLES=ON                 # Build example applications

# Platform-specific options
ECSCOPE_ENABLE_DPI_AWARENESS=ON           # Windows DPI awareness
ECSCOPE_ENABLE_RETINA_SUPPORT=ON          # macOS Retina support
ECSCOPE_ENABLE_WAYLAND_SUPPORT=ON         # Linux Wayland support

# Advanced options
ECSCOPE_ENABLE_SANITIZERS=ON              # Enable AddressSanitizer (Debug)
ECSCOPE_ENABLE_COVERAGE=ON                # Enable code coverage
ECSCOPE_ENABLE_ADVANCED_OPTIMIZATIONS=ON  # Enable advanced optimizations
```

## Testing Framework

### Test Categories

1. **Platform Detection Tests**
   - Compiler identification
   - Operating system detection
   - Architecture detection

2. **GUI Compatibility Tests**
   - GLFW initialization and window creation
   - OpenGL context creation and capabilities
   - ImGui integration and rendering
   - DPI scaling and high-DPI support

3. **Platform-Specific Tests**
   - Windows: DPI awareness, window decorations
   - Linux: X11/Wayland compatibility, window managers
   - macOS: Retina support, menu bar integration

4. **File System Tests**
   - Path handling and separators
   - Special directories (Documents, temp, etc.)
   - Symbolic links and permissions

5. **Threading Tests**
   - Thread safety of GUI operations
   - Synchronization primitives
   - Atomic operations

6. **Performance Tests**
   - Initialization time
   - Frame rendering performance
   - Memory usage

### Running Individual Tests

```bash
# Run all cross-platform tests
ctest --label-regex "cross-platform"

# Run GUI compatibility tests
./bin/ecscope_gui_compatibility_test

# Run platform-specific tests
./bin/ecscope_platform_specific_test

# Run with specific output format
./bin/ecscope_gui_compatibility_test --reporter=junit --out=results.xml
```

### Test Configuration

Tests can be configured using environment variables:

```bash
# Enable verbose output
export ECSCOPE_TEST_VERBOSE=1

# Set test timeout (seconds)
export ECSCOPE_TEST_TIMEOUT=300

# Enable GPU testing (requires hardware acceleration)
export ECSCOPE_TEST_GPU=1

# Set display for headless testing (Linux)
export DISPLAY=:99
```

## Automated Testing

### Local Automation

```bash
# Run comprehensive cross-platform testing
python3 scripts/cross_platform/build_and_test.py \
    --source-dir . \
    --output-dir test_results

# Show available configurations
python3 scripts/cross_platform/build_and_test.py --config-only

# Generate report from existing results
python3 scripts/cross_platform/build_and_test.py \
    --report-only test_results/cross_platform_test_report.json
```

### Docker Testing

```bash
# Build test image
docker build -f docker/Dockerfile.test -t ecscope-test .

# Run tests in container
docker run --rm -v $(pwd):/workspace ecscope-test

# Run with GUI support (Linux X11)
docker run --rm -v $(pwd):/workspace \
    -e DISPLAY=$DISPLAY -v /tmp/.X11-unix:/tmp/.X11-unix \
    ecscope-test
```

### Virtual Machine Testing

For comprehensive platform testing, consider using virtual machines:

- **Windows**: Windows 10/11 VM with Visual Studio
- **Linux**: Ubuntu, CentOS, Arch Linux VMs
- **macOS**: macOS VM (on Apple hardware only)

## Troubleshooting

### Common Issues

#### Build Failures

**Problem**: CMake cannot find dependencies
```bash
# Solution: Install dependencies or set CMAKE_PREFIX_PATH
export CMAKE_PREFIX_PATH=/usr/local:/opt/homebrew
cmake -B build ...
```

**Problem**: Compiler version too old
```bash
# Ubuntu: Install newer GCC
sudo apt-get install gcc-11 g++-11
export CC=gcc-11 CXX=g++-11

# Update alternatives
sudo update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-11 100
sudo update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-11 100
```

#### Runtime Issues

**Problem**: OpenGL context creation fails
```bash
# Linux: Check OpenGL support
glxinfo | grep "OpenGL version"

# Install Mesa drivers
sudo apt-get install mesa-utils mesa-vulkan-drivers

# Update graphics drivers
sudo ubuntu-drivers autoinstall
```

**Problem**: GUI tests fail in headless environment
```bash
# Use Xvfb for virtual display
export DISPLAY=:99
Xvfb :99 -screen 0 1024x768x24 &
sleep 3
./bin/ecscope_gui_test
```

**Problem**: DPI scaling issues
- Windows: Check DPI awareness settings
- Linux: Set `GDK_SCALE` or `QT_SCALE_FACTOR`
- macOS: Test on actual Retina display

#### Performance Issues

**Problem**: Slow rendering performance
- Check GPU drivers are installed
- Verify hardware acceleration is enabled
- Monitor GPU usage during tests

**Problem**: High memory usage
- Run with memory profilers (Valgrind, AddressSanitizer)
- Check for memory leaks in debug builds
- Monitor texture and buffer usage

### Debug Builds

```bash
# Build with debug information and sanitizers
cmake -B build -DCMAKE_BUILD_TYPE=Debug \
    -DECSCOPE_ENABLE_SANITIZERS=ON \
    -DECSCOPE_ENABLE_DEBUG_LOGGING=ON

# Run with AddressSanitizer
export ASAN_OPTIONS=detect_leaks=1:abort_on_error=1
./bin/ecscope_gui_test

# Run with Valgrind (Linux)
valgrind --tool=memcheck --leak-check=full \
    ./bin/ecscope_gui_test
```

## Performance Benchmarking

### Running Benchmarks

```bash
# Build with benchmarks enabled
cmake -B build -DECSCOPE_BUILD_BENCHMARKS=ON
cmake --build build

# Run GUI performance benchmarks
./bin/ecscope_gui_benchmarks

# Run with specific output format
./bin/ecscope_gui_benchmarks --benchmark_format=json \
    --benchmark_out=benchmark_results.json

# Compare across platforms
python3 scripts/cross_platform/compare_performance.py \
    --baseline baseline_results.json \
    --current current_results.json
```

### Benchmark Categories

1. **Initialization Performance**
   - GLFW startup time
   - OpenGL context creation time
   - ImGui initialization time

2. **Rendering Performance**
   - Frame rate (FPS)
   - Draw call overhead
   - Vertex throughput

3. **Memory Performance**
   - Memory allocation patterns
   - Texture memory usage
   - Buffer management efficiency

4. **Platform-Specific Metrics**
   - DPI scaling performance
   - Window creation time
   - Event handling latency

### Performance Targets

| Metric | Target | Acceptable |
|--------|--------|------------|
| Initialization Time | < 500ms | < 1000ms |
| Frame Rate (1080p) | > 60 FPS | > 30 FPS |
| Memory Usage | < 100MB | < 200MB |
| Window Creation | < 100ms | < 250ms |

## CI/CD Integration

### GitHub Actions

The project includes comprehensive GitHub Actions workflows:

- **cross_platform_testing.yml**: Main cross-platform test suite
- **ci.yml**: Standard CI with build matrix

### Key Features

1. **Multi-Platform Matrix**
   - Windows (MSVC)
   - Linux (GCC, Clang)
   - macOS (Clang)

2. **Test Categories**
   - Build compatibility
   - GUI functionality
   - Performance benchmarks
   - Memory leak detection

3. **Artifact Collection**
   - Test results (JUnit XML)
   - Performance data (JSON)
   - Build logs
   - Coverage reports

### Triggering CI

```bash
# Push to trigger CI
git push origin feature-branch

# Create pull request for comprehensive testing
gh pr create --title "Cross-platform GUI improvements"

# Manual workflow dispatch
gh workflow run cross_platform_testing.yml
```

### CI Configuration

Environment variables for CI:

```yaml
env:
  CMAKE_BUILD_TYPE: Release
  ECSCOPE_BUILD_TESTS: ON
  ECSCOPE_BUILD_CROSS_PLATFORM_TESTS: ON
  ECSCOPE_BUILD_GUI: ON
  ECSCOPE_ENABLE_HEADLESS_TESTING: ON
```

## Best Practices

### Development Workflow

1. **Local Testing**
   - Test on your primary development platform
   - Run cross-platform tests before committing
   - Use virtual machines for other platforms

2. **Code Changes**
   - Consider platform differences in implementations
   - Use platform-specific preprocessor directives sparingly
   - Test DPI scaling and window management changes

3. **Performance**
   - Profile on all target platforms
   - Consider hardware differences
   - Test on integrated and discrete GPUs

### Code Guidelines

```cpp
// Good: Platform-agnostic code
void SetWindowTitle(const std::string& title) {
#ifdef ECSCOPE_HAS_GLFW
    if (window_) {
        glfwSetWindowTitle(window_, title.c_str());
    }
#endif
}

// Good: Use filesystem library for paths
std::filesystem::path GetConfigPath() {
#ifdef _WIN32
    // Windows: %APPDATA%/ECScope
    return GetKnownFolder(FOLDERID_RoamingAppData) / "ECScope";
#elif defined(__APPLE__)
    // macOS: ~/Library/Application Support/ECScope
    return GetHomeDirectory() / "Library/Application Support/ECScope";
#else
    // Linux: ~/.config/ECScope
    return GetXDGConfigHome() / "ECScope";
#endif
}

// Good: Handle DPI scaling
float GetDPIScale() {
#ifdef ECSCOPE_HAS_GLFW
    float x_scale, y_scale;
    glfwGetWindowContentScale(window_, &x_scale, &y_scale);
    return std::max(x_scale, y_scale);
#else
    return 1.0f;
#endif
}
```

### Testing Guidelines

1. **Write Platform-Agnostic Tests**
   - Use conditional compilation only when necessary
   - Test common functionality across all platforms
   - Validate platform-specific behavior separately

2. **Mock External Dependencies**
   - Abstract platform APIs behind interfaces
   - Use dependency injection for testability
   - Provide mock implementations for headless testing

3. **Handle Test Environment Differences**
   - Account for different display servers
   - Handle missing hardware acceleration gracefully
   - Use appropriate timeouts for different platforms

## Conclusion

This comprehensive testing strategy ensures ECScope works reliably across all supported platforms. The combination of automated testing, platform-specific considerations, and performance monitoring provides confidence in cross-platform compatibility.

For questions or issues, please:

1. Check the [Troubleshooting](#troubleshooting) section
2. Review existing GitHub issues
3. Create a new issue with platform details and test results
4. Join our Discord community for real-time support

---

**Next Steps:**
- [Contributing Guide](CONTRIBUTING.md)
- [Build System Documentation](BUILD_SYSTEM.md)
- [API Documentation](API_REFERENCE.md)