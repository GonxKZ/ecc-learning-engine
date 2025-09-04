# ECScope Advanced Build System Documentation

## Overview

ECScope features a comprehensive, production-quality CMake build system designed for educational use while providing maximum performance and flexibility. The build system supports cross-platform development, multiple optimization levels, and advanced features like SIMD optimizations, job systems, and hardware detection.

## Quick Start

### Basic Build Commands

```bash
# Educational build with full features (recommended for learning)
python build.py --type educational --features full

# High-performance optimized build
python build.py --type performance --features full --test

# Minimal console-only build
python build.py --type minimal --features core

# Debug build with sanitizers
python build.py --type debug --features core --test
```

### Traditional CMake Usage

```bash
mkdir build && cd build

# Educational configuration
cmake .. \
    -DCMAKE_BUILD_TYPE=RelWithDebInfo \
    -DECSCOPE_EDUCATIONAL_MODE=ON \
    -DECSCOPE_BUILD_EXAMPLES=ON \
    -DECSCOPE_BUILD_BENCHMARKS=ON \
    -DECSCOPE_ENABLE_GRAPHICS=ON

cmake --build . --parallel
ctest --output-on-failure
```

## Build System Architecture

### Modular Design

The build system is organized into specialized CMake modules:

- **`cmake/ECScope.cmake`**: Core build functions and utilities
- **`cmake/Dependencies.cmake`**: Modern dependency management
- **`cmake/Platform.cmake`**: Platform-specific optimizations
- **`cmake/Testing.cmake`**: Comprehensive testing framework
- **`cmake/Installation.cmake`**: Installation and packaging

### Key Features

1. **Modular Configuration**: Separate modules for different engine systems
2. **Platform Detection**: Automatic platform and compiler optimization
3. **Dependency Management**: Automatic detection and configuration using FetchContent
4. **Educational Build Modes**: Development, educational, and production configurations
5. **Advanced Optimization**: Profile-guided optimization, LTO, and hardware-specific builds
6. **Cross-Platform Support**: Windows, Linux, macOS, Android, iOS
7. **Container Support**: Docker and multi-stage builds
8. **Package Generation**: DEB, RPM, MSI, DMG, AppImage

## Build Configurations

### Build Types

| Type | Description | Use Case | Features |
|------|-------------|----------|----------|
| `debug` | Development with full debugging | Active development | Sanitizers, coverage, debug symbols |
| `release` | Optimized production build | Deployment | LTO, advanced optimizations, minimal size |
| `educational` | Balanced build for learning | Teaching/learning | Examples, benchmarks, instrumentation |
| `performance` | Maximum performance | Benchmarking | PGO, AVX-512, aggressive optimizations |
| `minimal` | Smallest possible build | Embedded systems | Core features only |

### Feature Presets

#### Full Features (`--features full`)
```cmake
ECSCOPE_ENABLE_GRAPHICS=ON           # 2D/3D graphics with SDL2/OpenGL/Vulkan
ECSCOPE_ENABLE_SIMD=ON              # SIMD optimizations (SSE, AVX, NEON)
ECSCOPE_ENABLE_JOB_SYSTEM=ON        # Work-stealing job system
ECSCOPE_ENABLE_3D_PHYSICS=ON        # Advanced 3D physics engine
ECSCOPE_ENABLE_2D_PHYSICS=ON        # 2D physics system
ECSCOPE_ENABLE_SCRIPTING=ON         # Python/Lua integration
ECSCOPE_ENABLE_HARDWARE_DETECTION=ON # CPU/GPU detection and optimization
ECSCOPE_BUILD_TESTS=ON              # Unit and integration tests
ECSCOPE_BUILD_BENCHMARKS=ON         # Performance benchmarks
ECSCOPE_BUILD_EXAMPLES=ON           # Educational examples
```

#### Core Features (`--features core`)
```cmake
ECSCOPE_ENABLE_GRAPHICS=OFF         # Console-only
ECSCOPE_ENABLE_SIMD=ON              # SIMD optimizations
ECSCOPE_ENABLE_JOB_SYSTEM=ON        # Multi-threading support
ECSCOPE_BUILD_TESTS=ON              # Essential tests
```

#### Graphics Focus (`--features graphics`)
```cmake
ECSCOPE_ENABLE_GRAPHICS=ON          # Graphics system
ECSCOPE_ENABLE_2D_PHYSICS=ON        # 2D physics for games
ECSCOPE_BUILD_EXAMPLES=ON           # Rendering examples
```

## Platform Support

### Supported Platforms

| Platform | Architecture | Compiler | Features |
|----------|-------------|----------|----------|
| Windows 10/11 | x64, x86 | MSVC, Clang, MinGW | Full support including DirectX |
| Linux | x64, ARM64 | GCC, Clang | Full support including Vulkan |
| macOS | x64, ARM64 (Apple Silicon) | Clang | Full support including Metal |
| Android | ARM64, x86_64 | Clang (NDK) | Core features, OpenGL ES |
| iOS | ARM64 | Clang | Core features, Metal |

### Hardware Optimizations

- **x86/x64**: SSE, AVX, AVX2, AVX-512 with runtime detection
- **ARM64**: NEON optimizations with Apple Silicon support
- **GPU**: OpenGL, Vulkan, DirectX 11/12, Metal detection
- **NUMA**: Linux NUMA topology awareness
- **Cache**: CPU cache-aware data structures

## Advanced Features

### SIMD Optimizations

```cmake
# Automatic SIMD detection and optimization
ECSCOPE_ENABLE_SIMD=ON              # Enable SIMD support
ECSCOPE_ENABLE_AVX512=ON            # Enable AVX-512 if available
ECSCOPE_ENABLE_ARM_NEON=ON          # Enable ARM NEON if available
```

The build system automatically detects available SIMD instructions and enables appropriate optimizations:

- **Runtime Detection**: CPU features detected at build time
- **Multiple Variants**: Code paths for different instruction sets
- **Educational Examples**: SIMD performance demonstrations

### Job System Integration

```cmake
ECSCOPE_ENABLE_JOB_SYSTEM=ON        # Work-stealing job system
ECSCOPE_ENABLE_NUMA=ON              # NUMA-aware scheduling (Linux)
```

Features:
- Work-stealing task scheduler
- NUMA topology detection
- Thread affinity optimization
- Lock-free data structures

### Memory Management

```cmake
ECSCOPE_ENABLE_CUSTOM_ALLOCATORS=ON # Custom memory allocators
ECSCOPE_ENABLE_MEMORY_DEBUGGING=ON  # Advanced memory debugging
ECSCOPE_ENABLE_MEMORY_PROFILING=ON  # Memory usage profiling
```

Advanced memory features:
- Pool allocators with NUMA awareness
- Cache-aligned data structures
- Memory bandwidth analysis
- Allocation pattern tracking

### Hardware Detection

```cmake
ECSCOPE_ENABLE_HARDWARE_DETECTION=ON # CPU/GPU detection
ECSCOPE_ENABLE_THERMAL_MANAGEMENT=ON # Thermal monitoring
ECSCOPE_ENABLE_CPU_TOPOLOGY=ON       # CPU topology detection
```

Capabilities:
- CPU feature detection (SSE, AVX, etc.)
- GPU capability enumeration
- Thermal and power monitoring
- Performance scaling based on hardware

## Cross-Compilation

### Supported Targets

```bash
# ARM64 Linux cross-compilation
python build.py --type release --cross linux-arm64

# Android ARM64
python build.py --type minimal --cross android-arm64

# Windows from Linux (MinGW)
python build.py --type release --cross windows-x64
```

### Toolchain Configuration

Toolchain files are provided for common cross-compilation scenarios:

- `cmake/toolchains/aarch64-linux-gnu.cmake` - ARM64 Linux
- `cmake/toolchains/android-arm64.cmake` - Android ARM64
- `cmake/toolchains/mingw-w64.cmake` - Windows cross-compilation

## Docker Support

### Container Variants

```bash
# Development environment with full tools
python build.py --docker development

# Minimal runtime container
python build.py --docker runtime

# Testing and CI environment
python build.py --docker testing

# Alpine Linux minimal build
python build.py --docker alpine
```

### Multi-stage Builds

The Docker configuration supports multi-stage builds:

1. **Builder Stage**: Full development environment with all dependencies
2. **Runtime Stage**: Minimal container with only runtime dependencies
3. **Development Stage**: Enhanced development environment with debugging tools
4. **Testing Stage**: CI/CD environment with testing and analysis tools

## Testing Framework

### Test Categories

```bash
# Run all tests
ctest

# Unit tests only
ctest -L unit

# Integration tests
ctest -L integration

# Performance tests
ctest -L performance

# Stress tests
ctest -L stress
```

### Test Configuration

- **Unit Tests**: Fast, isolated component tests
- **Integration Tests**: Multi-component system tests
- **Performance Tests**: Benchmarking and performance validation
- **Stress Tests**: High-load and endurance testing
- **Memory Tests**: Memory leak and corruption detection

### Coverage and Analysis

```bash
# Generate coverage report
make coverage

# Run with sanitizers
cmake .. -DECSCOPE_ENABLE_SANITIZERS=ON
```

Tools integrated:
- AddressSanitizer, ThreadSanitizer, UBSanitizer
- Code coverage with lcov/gcov
- Static analysis with clang-tidy
- Memory profiling with Valgrind

## Package Management

### Installation

```bash
# Install to system
cmake --build build --target install

# Custom prefix
cmake .. -DCMAKE_INSTALL_PREFIX=/usr/local/ecscope
```

### Package Generation

```bash
# Generate packages for current platform
python build.py --type release --package

# Specific package types
python build.py --type release --package --package-types DEB RPM
```

Supported package formats:
- **Linux**: DEB, RPM, TGZ, AppImage
- **Windows**: WIX (MSI), ZIP, NSIS
- **macOS**: DMG, PKG, TGZ
- **Source**: TGZ, ZIP

### Find Package Support

ECScope provides comprehensive CMake package configuration:

```cmake
find_package(ECScope REQUIRED COMPONENTS Graphics JobSystem)

# Check specific features
ecscope_check_feature(GRAPHICS)
ecscope_check_feature(SIMD)

target_link_libraries(my_target PRIVATE ECScope::ecscope)
```

## Performance Optimization

### Build-Time Optimizations

```cmake
ECSCOPE_ENABLE_LTO=ON               # Link-time optimization
ECSCOPE_ENABLE_PGO=ON               # Profile-guided optimization
ECSCOPE_ENABLE_UNITY_BUILD=ON       # Unity/jumbo builds
ECSCOPE_ENABLE_PCH=ON               # Precompiled headers
ECSCOPE_ENABLE_CCACHE=ON            # Compiler cache
```

### Runtime Optimizations

- **Hardware-Specific**: CPU and GPU optimizations
- **Cache-Friendly**: Data structure layout optimization
- **SIMD Utilization**: Vectorized operations
- **Memory Locality**: NUMA-aware allocation
- **Thermal Management**: Dynamic performance scaling

## Educational Features

### Examples and Tutorials

Built examples showcase different aspects of the engine:

- **Performance Laboratory**: Memory behavior analysis
- **SIMD Demonstrations**: Vectorization performance
- **Job System Examples**: Parallel programming patterns
- **Physics Simulations**: 2D/3D physics demonstrations
- **Rendering Tutorials**: Graphics programming concepts

### Build Analysis

```bash
# Benchmark build times
python build.py --benchmark-builds

# Analyze dependencies
cmake --build build --target dependency-graph
```

### Memory Observatory

ECScope includes comprehensive memory analysis tools:

- Allocation pattern tracking
- Cache miss analysis
- Memory bandwidth measurement
- NUMA topology visualization
- Pool allocator efficiency metrics

## Troubleshooting

### Common Issues

1. **Missing Dependencies**
   ```bash
   # Install development packages (Ubuntu)
   sudo apt install build-essential cmake ninja-build libsdl2-dev libgl1-mesa-dev
   ```

2. **SIMD Compilation Errors**
   ```bash
   # Disable specific SIMD if unsupported
   cmake .. -DECSCOPE_ENABLE_AVX512=OFF
   ```

3. **Cross-Compilation Issues**
   ```bash
   # Use Docker for consistent cross-compilation
   python build.py --docker development
   ```

### Debug Configuration

```bash
# Debug build with full diagnostics
python build.py --type debug --features core --test
```

### Verbose Output

```bash
# CMake verbose output
cmake .. -DCMAKE_VERBOSE_MAKEFILE=ON

# Ninja verbose output
ninja -v
```

## Integration with IDEs

### Visual Studio Code

```json
{
    "cmake.buildDirectory": "${workspaceFolder}/build-educational",
    "cmake.configureArgs": [
        "-DECSCOPE_EDUCATIONAL_MODE=ON",
        "-DECSCOPE_BUILD_EXAMPLES=ON"
    ]
}
```

### Visual Studio

Open the project folder directly - CMake integration is automatic.

### CLion

CLion automatically detects CMake projects and provides full IntelliSense support.

## Contributing

### Build System Development

1. **Adding New Features**: Extend `cmake/ECScope.cmake`
2. **Platform Support**: Add toolchain files and platform detection
3. **Testing**: Add test cases in `tests/` directory
4. **Documentation**: Update this documentation

### Best Practices

- Use the helper functions provided in `cmake/ECScope.cmake`
- Follow the modular structure for new components
- Add feature flags for optional functionality
- Include educational examples for new features
- Ensure cross-platform compatibility

## Performance Metrics

Expected build performance (typical developer machine):

| Configuration | Cold Build | Incremental | Unity Build | With PCH |
|---------------|------------|-------------|-------------|----------|
| Debug | 45s | 8s | 25s | 20s |
| Release | 120s | 12s | 60s | 45s |
| Educational | 90s | 10s | 50s | 35s |

Memory usage during build: ~2GB peak for full build with all features.

## Future Enhancements

Planned build system improvements:

- [ ] Distributed builds with distcc/icecc
- [ ] WebAssembly compilation target
- [ ] GPU-accelerated builds where possible
- [ ] More granular feature selection
- [ ] Improved educational build analytics
- [ ] Integration with more package managers (vcpkg, Conan)

---

This build system represents a production-quality foundation for ECScope that scales from educational use to professional game development while maintaining excellent performance characteristics and comprehensive platform support.