# ECScope Build System and Setup Guide

**Complete guide for building, configuring, and deploying ECScope across different platforms and use cases**

## Table of Contents

1. [System Requirements](#system-requirements)
2. [Quick Start Setup](#quick-start-setup)
3. [Detailed Build Configuration](#detailed-build-configuration)
4. [Platform-Specific Instructions](#platform-specific-instructions)
5. [Development Environment Setup](#development-environment-setup)
6. [Troubleshooting](#troubleshooting)
7. [Advanced Configuration](#advanced-configuration)
8. [Performance Optimization](#performance-optimization)

## System Requirements

### Minimum Requirements

**Compiler Support**:
- **GCC**: 10.0+ (C++20 support)
- **Clang**: 12.0+ (C++20 support)  
- **MSVC**: 2022+ (Visual Studio 17.0+)
- **Apple Clang**: 13.0+ (Xcode 13+)

**Build Tools**:
- **CMake**: 3.22 or higher
- **Make/Ninja**: For build execution
- **Git**: For dependency management

**Libraries** (Console Mode):
- Standard C++20 library
- POSIX threads (Linux/macOS)

**Libraries** (Graphics Mode):
- **SDL2**: 2.0.20+ (window management and input)
- **OpenGL**: 3.3+ drivers
- **ImGui**: Included in external/ directory

### Recommended Development Environment

**Hardware Recommendations**:
- **CPU**: Multi-core processor (Intel i5/AMD Ryzen 5 or better)
- **RAM**: 8GB+ (16GB recommended for large simulations)
- **GPU**: OpenGL 3.3+ compatible (discrete GPU recommended for performance analysis)
- **Storage**: SSD for faster compilation

**Software Recommendations**:
- **IDE**: CLion, Visual Studio, VSCode with C++ extensions
- **Debugger**: GDB, LLDB, or Visual Studio Debugger
- **Profiler**: perf, Intel VTune, or built-in ECScope profiling tools

## Quick Start Setup

### Linux (Ubuntu/Debian)

```bash
# Install dependencies
sudo apt update
sudo apt install build-essential cmake git
sudo apt install libsdl2-dev  # For graphics support (optional)

# Clone and build
git clone https://github.com/your-repo/ecscope.git
cd ecscope
mkdir build && cd build

# Console-only build (minimal dependencies)
cmake ..
make -j$(nproc)

# Test console build
./ecscope_performance_lab_console

# Graphics build (requires SDL2)
cmake -DECSCOPE_ENABLE_GRAPHICS=ON ..
make -j$(nproc)

# Test graphics build
./ecscope_ui
```

### macOS

```bash
# Install dependencies via Homebrew
brew install cmake sdl2

# Clone and build
git clone https://github.com/your-repo/ecscope.git
cd ecscope
mkdir build && cd build

# Build with graphics support
cmake -DECSCOPE_ENABLE_GRAPHICS=ON ..
make -j$(sysctl -n hw.ncpu)

# Test
./ecscope_ui
```

### Windows (Visual Studio)

```powershell
# Install dependencies via vcpkg
vcpkg install sdl2:x64-windows

# Clone repository
git clone https://github.com/your-repo/ecscope.git
cd ecscope
mkdir build
cd build

# Configure with vcpkg toolchain
cmake -DCMAKE_TOOLCHAIN_FILE=C:\vcpkg\scripts\buildsystems\vcpkg.cmake -DECSCOPE_ENABLE_GRAPHICS=ON ..

# Build
cmake --build . --config Release

# Test
.\Release\ecscope_ui.exe
```

## Detailed Build Configuration

### CMake Configuration Options

```cmake
# Core options
option(ECSCOPE_BUILD_TESTS "Build unit tests" ON)
option(ECSCOPE_ENABLE_INSTRUMENTATION "Enable tracing & memory hooks" ON)
option(ECSCOPE_ENABLE_GRAPHICS "Enable graphical UI (requires SDL2, ImGui)" OFF)
option(ECSCOPE_BUILD_EXAMPLES "Build rendering examples and tutorials" ON)

# Performance options
option(ECSCOPE_ENABLE_SIMD "Enable SIMD optimizations" ON)
option(ECSCOPE_ENABLE_FAST_MATH "Enable fast math optimizations" OFF)
option(ECSCOPE_ENABLE_LTO "Enable Link Time Optimization" OFF)

# Educational options
option(ECSCOPE_DETAILED_LOGGING "Enable detailed educational logging" ON)
option(ECSCOPE_MEMORY_DEBUGGING "Enable comprehensive memory debugging" ON)
option(ECSCOPE_PERFORMANCE_ANALYSIS "Enable real-time performance analysis" ON)

# Advanced options
option(ECSCOPE_SANITIZERS "Enable AddressSanitizer and UBSan" OFF)
option(ECSCOPE_STATIC_ANALYSIS "Enable static analysis warnings" OFF)
option(ECSCOPE_BENCHMARKING "Build benchmarking suite" ON)
```

### Build Configurations

**Educational Development Configuration**:
```bash
cmake \
  -DCMAKE_BUILD_TYPE=Debug \
  -DECSCOPE_ENABLE_GRAPHICS=ON \
  -DECSCOPE_BUILD_EXAMPLES=ON \
  -DECSCOPE_DETAILED_LOGGING=ON \
  -DECSCOPE_MEMORY_DEBUGGING=ON \
  -DECSCOPE_PERFORMANCE_ANALYSIS=ON \
  -DECSCOPE_SANITIZERS=ON \
  ..
```

**Performance Benchmarking Configuration**:
```bash
cmake \
  -DCMAKE_BUILD_TYPE=Release \
  -DECSCOPE_ENABLE_GRAPHICS=ON \
  -DECSCOPE_BUILD_EXAMPLES=ON \
  -DECSCOPE_DETAILED_LOGGING=OFF \
  -DECSCOPE_MEMORY_DEBUGGING=OFF \
  -DECSCOPE_ENABLE_LTO=ON \
  -DECSCOPE_ENABLE_SIMD=ON \
  -DECSCOPE_ENABLE_FAST_MATH=ON \
  ..
```

**Research/Production Configuration**:
```bash
cmake \
  -DCMAKE_BUILD_TYPE=RelWithDebInfo \
  -DECSCOPE_ENABLE_GRAPHICS=ON \
  -DECSCOPE_BUILD_EXAMPLES=OFF \
  -DECSCOPE_INSTRUMENTATION=OFF \
  -DECSCOPE_ENABLE_LTO=ON \
  -DECSCOPE_ENABLE_SIMD=ON \
  ..
```

### Compiler-Specific Optimizations

**GCC Optimizations**:
```cmake
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(ecscope PRIVATE
        # Performance optimizations
        -O3
        -march=native          # Optimize for current CPU
        -mtune=native
        -flto                  # Link-time optimization
        
        # Educational profiling
        -g                     # Debug symbols
        -fno-omit-frame-pointer # Better profiling
        
        # Warnings
        -Wall -Wextra -Wpedantic
        -Wno-unused-parameter  # Common in template code
        
        # Modern C++ features
        -fcoroutines           # C++20 coroutines
        -fconcepts             # C++20 concepts
    )
endif()
```

**Clang Optimizations**:
```cmake
if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    target_compile_options(ecscope PRIVATE
        # Performance optimizations
        -O3
        -march=native
        -flto=thin             # Thin LTO for faster builds
        
        # Educational features
        -g
        -fno-omit-frame-pointer
        
        # Enhanced warnings
        -Weverything
        -Wno-c++98-compat
        -Wno-padded            # Don't warn about padding
        -Wno-global-constructors
        
        # Sanitizers (debug builds)
        $<$<CONFIG:Debug>:-fsanitize=address>
        $<$<CONFIG:Debug>:-fsanitize=undefined>
    )
endif()
```

**MSVC Optimizations**:
```cmake
if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    target_compile_options(ecscope PRIVATE
        # Performance optimizations
        /O2                    # Optimize for speed
        /GL                    # Whole program optimization
        /arch:AVX2             # Use AVX2 instructions
        
        # Educational features
        /Zi                    # Debug information
        /DEBUG                 # Linker debug info
        
        # Warnings
        /W4                    # High warning level
        /wd4251                # DLL interface warnings
        /wd4996                # Deprecated function warnings
        
        # Modern C++
        /std:c++20
        /Zc:__cplusplus        # Correct __cplusplus macro
    )
    
    target_link_options(ecscope PRIVATE
        $<$<CONFIG:Release>:/LTCG>  # Link-time code generation
    )
endif()
```

## Platform-Specific Instructions

### Linux Distribution-Specific Setup

**Ubuntu/Debian**:
```bash
# Install build dependencies
sudo apt install build-essential cmake git pkg-config

# Install graphics dependencies
sudo apt install libsdl2-dev libgl1-mesa-dev

# Install optional development tools
sudo apt install gdb valgrind perf linux-tools-generic

# For educational tracing
sudo apt install trace-cmd kernelshark
```

**Fedora/CentOS/RHEL**:
```bash
# Install build dependencies
sudo dnf install gcc-c++ cmake git pkgconfig

# Install graphics dependencies
sudo dnf install SDL2-devel mesa-libGL-devel

# Development tools
sudo dnf install gdb valgrind perf
```

**Arch Linux**:
```bash
# Install dependencies
sudo pacman -S base-devel cmake git sdl2 mesa

# Development tools
sudo pacman -S gdb valgrind perf
```

### macOS Setup

**Using Homebrew**:
```bash
# Install Xcode command line tools
xcode-select --install

# Install dependencies
brew install cmake sdl2

# Optional development tools
brew install lldb # Already included with Xcode
```

**Manual SDL2 Installation**:
```bash
# Download SDL2 development libraries
curl -O https://www.libsdl.org/release/SDL2-2.26.0.dmg

# Mount and copy frameworks to /Library/Frameworks/
# Or use local installation in project directory
```

### Windows Setup

**Using vcpkg (Recommended)**:
```powershell
# Install vcpkg
git clone https://github.com/Microsoft/vcpkg.git
cd vcpkg
.\bootstrap-vcpkg.bat

# Install dependencies
.\vcpkg install sdl2:x64-windows
.\vcpkg install opengl:x64-windows

# Integrate with Visual Studio
.\vcpkg integrate install
```

**Using NuGet (Visual Studio)**:
```xml
<!-- In packages.config -->
<?xml version="1.0" encoding="utf-8"?>
<packages>
  <package id="sdl2" version="2.26.0" targetFramework="native" />
  <package id="sdl2.redist" version="2.26.0" targetFramework="native" />
</packages>
```

**Manual Installation**:
```powershell
# Download SDL2 development libraries
# https://www.libsdl.org/download-2.0.php

# Extract to C:\SDL2 or project/external/SDL2
# Update CMakeLists.txt paths accordingly
```

## Development Environment Setup

### VSCode Configuration

**`.vscode/tasks.json`**:
```json
{
    "version": "2.0.0",
    "tasks": [
        {
            "label": "ECScope: Configure Debug",
            "type": "shell",
            "command": "cmake",
            "args": [
                "-DCMAKE_BUILD_TYPE=Debug",
                "-DECSCOPE_ENABLE_GRAPHICS=ON",
                "-DECSCOPE_BUILD_EXAMPLES=ON",
                "-DECSCOPE_SANITIZERS=ON",
                ".."
            ],
            "options": {
                "cwd": "${workspaceFolder}/build"
            },
            "group": "build"
        },
        {
            "label": "ECScope: Build Debug",
            "type": "shell",
            "command": "make",
            "args": ["-j8"],
            "options": {
                "cwd": "${workspaceFolder}/build"
            },
            "group": {
                "kind": "build",
                "isDefault": true
            },
            "dependsOn": "ECScope: Configure Debug"
        },
        {
            "label": "ECScope: Build Release",
            "type": "shell",
            "command": "cmake",
            "args": [
                "--build", ".", 
                "--config", "Release",
                "--parallel", "8"
            ],
            "options": {
                "cwd": "${workspaceFolder}/build"
            },
            "group": "build"
        }
    ]
}
```

**`.vscode/launch.json`**:
```json
{
    "version": "0.2.0",
    "configurations": [
        {
            "name": "ECScope Console Debug",
            "type": "cppdbg",
            "request": "launch",
            "program": "${workspaceFolder}/build/ecscope_performance_lab_console",
            "args": [],
            "stopAtEntry": false,
            "cwd": "${workspaceFolder}",
            "environment": [],
            "externalConsole": false,
            "MIMode": "gdb",
            "setupCommands": [
                {
                    "description": "Enable pretty-printing for gdb",
                    "text": "-enable-pretty-printing",
                    "ignoreFailures": true
                }
            ],
            "preLaunchTask": "ECScope: Build Debug"
        },
        {
            "name": "ECScope UI Debug",
            "type": "cppdbg",
            "request": "launch",
            "program": "${workspaceFolder}/build/ecscope_ui",
            "args": [],
            "stopAtEntry": false,
            "cwd": "${workspaceFolder}",
            "environment": [],
            "externalConsole": false,
            "MIMode": "gdb",
            "preLaunchTask": "ECScope: Build Debug"
        },
        {
            "name": "ECScope Physics Demo",
            "type": "cppdbg",
            "request": "launch",
            "program": "${workspaceFolder}/build/ecscope_tutorial_01",
            "args": [],
            "stopAtEntry": false,
            "cwd": "${workspaceFolder}",
            "environment": [],
            "externalConsole": false,
            "MIMode": "gdb",
            "preLaunchTask": "ECScope: Build Debug"
        }
    ]
}
```

**`.vscode/c_cpp_properties.json`**:
```json
{
    "configurations": [
        {
            "name": "ECScope",
            "includePath": [
                "${workspaceFolder}/include",
                "${workspaceFolder}/src",
                "${workspaceFolder}/external/imgui",
                "/usr/include/SDL2"
            ],
            "defines": [
                "ECSCOPE_ENABLE_INSTRUMENTATION=1",
                "ECSCOPE_HAS_GRAPHICS=1"
            ],
            "compilerPath": "/usr/bin/g++",
            "cStandard": "c17",
            "cppStandard": "c++20",
            "intelliSenseMode": "linux-gcc-x64",
            "configurationProvider": "ms-vscode.cmake-tools"
        }
    ],
    "version": 4
}
```

### CLion Configuration

**CMakePresets.json**:
```json
{
    "version": 3,
    "cmakeMinimumRequired": {
        "major": 3,
        "minor": 22,
        "patch": 0
    },
    "configurePresets": [
        {
            "name": "debug-education",
            "displayName": "Educational Debug Build",
            "description": "Debug build with all educational features enabled",
            "binaryDir": "${sourceDir}/build/debug",
            "cacheVariables": {
                "CMAKE_BUILD_TYPE": "Debug",
                "ECSCOPE_ENABLE_GRAPHICS": "ON",
                "ECSCOPE_BUILD_EXAMPLES": "ON",
                "ECSCOPE_DETAILED_LOGGING": "ON",
                "ECSCOPE_MEMORY_DEBUGGING": "ON",
                "ECSCOPE_PERFORMANCE_ANALYSIS": "ON"
            }
        },
        {
            "name": "release-performance",
            "displayName": "Performance Release Build",
            "description": "Optimized build for performance benchmarking",
            "binaryDir": "${sourceDir}/build/release",
            "cacheVariables": {
                "CMAKE_BUILD_TYPE": "Release",
                "ECSCOPE_ENABLE_GRAPHICS": "ON",
                "ECSCOPE_BUILD_EXAMPLES": "ON",
                "ECSCOPE_DETAILED_LOGGING": "OFF",
                "ECSCOPE_ENABLE_LTO": "ON",
                "ECSCOPE_ENABLE_SIMD": "ON"
            }
        },
        {
            "name": "console-only",
            "displayName": "Console Only Build",
            "description": "Minimal build without graphics dependencies",
            "binaryDir": "${sourceDir}/build/console",
            "cacheVariables": {
                "CMAKE_BUILD_TYPE": "Release",
                "ECSCOPE_ENABLE_GRAPHICS": "OFF",
                "ECSCOPE_BUILD_EXAMPLES": "OFF"
            }
        }
    ],
    "buildPresets": [
        {
            "name": "debug-education",
            "configurePreset": "debug-education"
        },
        {
            "name": "release-performance",
            "configurePreset": "release-performance"
        },
        {
            "name": "console-only",
            "configurePreset": "console-only"
        }
    ]
}
```

### Visual Studio Configuration

**CMakeSettings.json**:
```json
{
    "configurations": [
        {
            "name": "x64-Debug-Education",
            "generator": "Ninja",
            "configurationType": "Debug",
            "buildRoot": "${projectDir}\\build\\debug",
            "installRoot": "${projectDir}\\install\\debug",
            "cmakeCommandArgs": "",
            "buildCommandArgs": "",
            "ctestCommandArgs": "",
            "variables": [
                {
                    "name": "ECSCOPE_ENABLE_GRAPHICS",
                    "value": "ON",
                    "type": "BOOL"
                },
                {
                    "name": "ECSCOPE_BUILD_EXAMPLES",
                    "value": "ON",
                    "type": "BOOL"
                },
                {
                    "name": "ECSCOPE_DETAILED_LOGGING",
                    "value": "ON",
                    "type": "BOOL"
                }
            ]
        },
        {
            "name": "x64-Release-Performance",
            "generator": "Ninja",
            "configurationType": "Release",
            "buildRoot": "${projectDir}\\build\\release",
            "installRoot": "${projectDir}\\install\\release",
            "variables": [
                {
                    "name": "ECSCOPE_ENABLE_GRAPHICS",
                    "value": "ON",
                    "type": "BOOL"
                },
                {
                    "name": "ECSCOPE_ENABLE_LTO",
                    "value": "ON",
                    "type": "BOOL"
                },
                {
                    "name": "ECSCOPE_DETAILED_LOGGING",
                    "value": "OFF",
                    "type": "BOOL"
                }
            ]
        }
    ]
}
```

## Platform-Specific Instructions

### Linux Development Setup

**Installing Graphics Drivers**:
```bash
# NVIDIA drivers (proprietary)
sudo apt install nvidia-driver-470 nvidia-utils-470

# AMD drivers (open source)
sudo apt install mesa-vulkan-drivers libvulkan1

# Intel graphics (usually pre-installed)
sudo apt install intel-media-va-driver

# Verify OpenGL support
glxinfo | grep "OpenGL version"
# Should show 3.3 or higher
```

**Performance Monitoring Setup**:
```bash
# Install performance tools
sudo apt install linux-tools-generic htop

# Enable perf for non-root users
echo 'kernel.perf_event_paranoid = 1' | sudo tee -a /etc/sysctl.conf

# Install additional profiling tools
sudo apt install valgrind kcachegrind

# For memory debugging
sudo apt install libc6-dbg
```

### macOS Development Setup

**Xcode Integration**:
```bash
# Generate Xcode project
cmake -G Xcode -DECSCOPE_ENABLE_GRAPHICS=ON ..

# Open in Xcode
open ECScope.xcodeproj
```

**Performance Tools Setup**:
```bash
# Install profiling tools
brew install gperftools

# Instruments integration (built into Xcode)
# Use Time Profiler and Allocations instruments
```

### Windows Development Setup

**Visual Studio Integration**:
```cmake
# In CMakeLists.txt - Windows-specific configuration
if(WIN32)
    # Ensure proper Windows SDK
    set(CMAKE_SYSTEM_VERSION "10.0")
    
    # Configure for different Visual Studio versions
    if(MSVC_VERSION GREATER_EQUAL 1930) # VS 2022
        target_compile_features(ecscope PUBLIC cxx_std_20)
    endif()
    
    # Windows-specific libraries
    target_link_libraries(ecscope PRIVATE
        opengl32    # OpenGL
        winmm       # Multimedia timer
    )
    
    # Copy SDL2 DLLs to output directory
    if(ECSCOPE_HAS_GRAPHICS AND TARGET SDL2::SDL2)
        add_custom_command(TARGET ecscope_ui POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:SDL2::SDL2>
            $<TARGET_FILE_DIR:ecscope_ui>
        )
    endif()
endif()
```

**Windows Performance Tools**:
- **Visual Studio Diagnostics**: Built-in profiler and memory analyzer
- **Intel VTune**: Advanced CPU profiling (free community edition)
- **PerfView**: Microsoft's ETW-based profiler
- **Application Verifier**: Memory debugging tool

## Troubleshooting

### Common Build Issues

**CMake Configuration Fails**:
```bash
# Clear CMake cache
rm -rf build/
mkdir build && cd build

# Verbose configuration to see detailed errors
cmake -DCMAKE_VERBOSE_MAKEFILE=ON ..

# Check for missing dependencies
cmake --debug-output ..
```

**C++20 Support Issues**:
```cmake
# Verify compiler C++20 support
cmake -DCMAKE_CXX_COMPILER=g++-11 ..  # Use specific compiler version

# Check compiler version
g++ --version
clang++ --version
```

**SDL2 Not Found**:
```bash
# Linux: Install development packages
sudo apt install libsdl2-dev

# macOS: Use Homebrew
brew install sdl2

# Windows: Use vcpkg
vcpkg install sdl2:x64-windows

# Manual installation
export SDL2_DIR=/path/to/sdl2
cmake -DSDL2_DIR=$SDL2_DIR ..
```

**OpenGL Issues**:
```bash
# Check OpenGL version
glxinfo | grep "OpenGL version"  # Linux
system_profiler SPDisplaysDataType | grep "OpenGL"  # macOS

# Update graphics drivers
# NVIDIA: Download from nvidia.com
# AMD: Download from amd.com
# Intel: Usually auto-updated via Windows Update
```

### Runtime Issues

**Application Crashes on Startup**:
```bash
# Run with debugger
gdb ./ecscope_ui
(gdb) run
(gdb) bt  # Print backtrace when crashed

# Check for missing shared libraries
ldd ./ecscope_ui  # Linux
otool -L ./ecscope_ui  # macOS
```

**Graphics Not Working**:
```bash
# Verify OpenGL context creation
export MESA_DEBUG=1  # Linux - enable debug output
./ecscope_ui

# Check SDL2 video driver
export SDL_VIDEODRIVER=x11  # Force X11 on Linux
export SDL_VIDEODRIVER=wayland  # Try Wayland on Linux
```

**Performance Issues**:
```bash
# Build with optimizations
cmake -DCMAKE_BUILD_TYPE=Release ..

# Disable debug features
cmake -DECSCOPE_DETAILED_LOGGING=OFF -DECSCOPE_MEMORY_DEBUGGING=OFF ..

# Check for debug symbols in release build
objdump -h ./ecscope_ui | grep debug  # Should be minimal in release
```

### Memory Debugging

**AddressSanitizer Setup**:
```bash
# Build with ASan
cmake -DECSCOPE_SANITIZERS=ON -DCMAKE_BUILD_TYPE=Debug ..
make

# Run with ASan
./ecscope_ui
# ASan will report memory errors with detailed stack traces
```

**Valgrind Analysis**:
```bash
# Memory leak detection
valgrind --leak-check=full --show-leak-kinds=all ./ecscope_ui

# Cache analysis
valgrind --tool=cachegrind ./ecscope_performance_lab_console
kcachegrind cachegrind.out.*

# Memory error detection
valgrind --tool=memcheck --track-origins=yes ./ecscope_ui
```

## Advanced Configuration

### Custom Allocator Configuration

```cpp
// Custom allocator configuration for specific use cases
class CustomBuildConfig {
public:
    static AllocatorConfig create_research_config() {
        AllocatorConfig config{};
        config.enable_archetype_arena = true;
        config.enable_entity_pool = true;
        config.enable_pmr_containers = true;
        config.enable_memory_tracking = true;
        config.enable_detailed_instrumentation = true;
        
        // Research-specific settings
        config.arena_size_mb = 64;  // Large arena for experiments
        config.pool_block_sizes = {16, 32, 64, 128, 256, 512}; // Multiple pool sizes
        config.enable_allocation_guards = true; // Detect overruns
        config.enable_leak_detection = true;
        
        return config;
    }
    
    static AllocatorConfig create_production_config() {
        AllocatorConfig config{};
        config.enable_archetype_arena = true;
        config.enable_entity_pool = true;
        config.enable_pmr_containers = true;
        config.enable_memory_tracking = false;  // Disable for performance
        config.enable_detailed_instrumentation = false;
        
        // Production-optimized settings
        config.arena_size_mb = 16;  // Conservative memory usage
        config.pool_block_sizes = {32, 64, 128}; // Common sizes only
        config.enable_allocation_guards = false;
        config.enable_leak_detection = false;
        
        return config;
    }
    
    static AllocatorConfig create_educational_config() {
        AllocatorConfig config{};
        config.enable_archetype_arena = true;
        config.enable_entity_pool = true;
        config.enable_pmr_containers = true;
        config.enable_memory_tracking = true;
        config.enable_detailed_instrumentation = true;
        
        // Educational-focused settings
        config.arena_size_mb = 8;   // Small for clear patterns
        config.pool_block_sizes = {16, 32, 64}; // Simple progression
        config.enable_allocation_guards = true;
        config.enable_leak_detection = true;
        config.enable_access_tracking = true; // Track every memory access
        config.enable_cache_analysis = true;  // Detailed cache behavior
        
        return config;
    }
};
```

### Cross-Platform Build Scripts

**`build_scripts/build_all_platforms.sh`**:
```bash
#!/bin/bash
set -e

BUILD_DIR="builds"
PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

echo "ECScope Multi-Platform Build Script"
echo "Project root: $PROJECT_ROOT"

# Clean previous builds
rm -rf "$BUILD_DIR"
mkdir -p "$BUILD_DIR"

# Function to build for specific configuration
build_config() {
    local config_name=$1
    local cmake_args=$2
    local build_dir="$BUILD_DIR/$config_name"
    
    echo "Building configuration: $config_name"
    mkdir -p "$build_dir"
    cd "$build_dir"
    
    cmake $cmake_args "$PROJECT_ROOT"
    make -j$(nproc)
    
    # Run tests if available
    if [ -f "./ecscope_tests" ]; then
        echo "Running tests for $config_name"
        ./ecscope_tests
    fi
    
    cd "$PROJECT_ROOT"
    echo "Completed: $config_name"
}

# Build different configurations
build_config "console-release" "-DCMAKE_BUILD_TYPE=Release -DECSCOPE_ENABLE_GRAPHICS=OFF"
build_config "console-debug" "-DCMAKE_BUILD_TYPE=Debug -DECSCOPE_ENABLE_GRAPHICS=OFF -DECSCOPE_SANITIZERS=ON"
build_config "graphics-release" "-DCMAKE_BUILD_TYPE=Release -DECSCOPE_ENABLE_GRAPHICS=ON"
build_config "graphics-debug" "-DCMAKE_BUILD_TYPE=Debug -DECSCOPE_ENABLE_GRAPHICS=ON -DECSCOPE_SANITIZERS=ON"

echo "All builds completed successfully!"
echo "Builds available in: $BUILD_DIR/"
```

**`build_scripts/setup_dev_environment.sh`**:
```bash
#!/bin/bash

echo "Setting up ECScope development environment..."

# Create necessary directories
mkdir -p build logs profiling_data

# Setup git hooks
cp build_scripts/git_hooks/* .git/hooks/
chmod +x .git/hooks/*

# Create development configuration
cat > dev_config.cmake << EOF
# Development-specific CMake configuration
set(ECSCOPE_DEV_MODE ON)
set(ECSCOPE_ENABLE_GRAPHICS ON)
set(ECSCOPE_BUILD_EXAMPLES ON)
set(ECSCOPE_DETAILED_LOGGING ON)
set(ECSCOPE_MEMORY_DEBUGGING ON)
set(ECSCOPE_PERFORMANCE_ANALYSIS ON)
set(ECSCOPE_SANITIZERS ON)
EOF

# Create VS Code workspace settings
mkdir -p .vscode
cat > .vscode/settings.json << EOF
{
    "cmake.configureSettings": {
        "ECSCOPE_ENABLE_GRAPHICS": "ON",
        "ECSCOPE_BUILD_EXAMPLES": "ON",
        "ECSCOPE_DETAILED_LOGGING": "ON"
    },
    "cmake.buildDirectory": "\${workspaceFolder}/build",
    "C_Cpp.default.configurationProvider": "ms-vscode.cmake-tools",
    "files.associations": {
        "*.hpp": "cpp",
        "*.inl": "cpp"
    }
}
EOF

echo "Development environment setup complete!"
echo "Next steps:"
echo "1. cd build"
echo "2. cmake -C ../dev_config.cmake .."
echo "3. make -j\$(nproc)"
echo "4. ./ecscope_ui"
```

### Docker Development Environment

**`Dockerfile.dev`**:
```dockerfile
FROM ubuntu:22.04

# Install dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    libsdl2-dev \
    libgl1-mesa-dev \
    pkg-config \
    gdb \
    valgrind \
    perf \
    && rm -rf /var/lib/apt/lists/*

# Create development user
RUN useradd -m -s /bin/bash developer
USER developer
WORKDIR /home/developer

# Setup development environment
COPY --chown=developer:developer . /home/developer/ecscope
WORKDIR /home/developer/ecscope

# Build ECScope
RUN mkdir build && cd build && \
    cmake -DECSCOPE_ENABLE_GRAPHICS=OFF .. && \
    make -j$(nproc)

# Default command
CMD ["./build/ecscope_performance_lab_console"]
```

**`docker-compose.yml`**:
```yaml
version: '3.8'
services:
  ecscope-dev:
    build:
      context: .
      dockerfile: Dockerfile.dev
    volumes:
      - .:/home/developer/ecscope
      - ./build:/home/developer/ecscope/build
    working_dir: /home/developer/ecscope
    environment:
      - DISPLAY=${DISPLAY}  # For X11 forwarding (Linux)
    network_mode: host
    
  ecscope-build:
    build:
      context: .
      dockerfile: Dockerfile.dev
    volumes:
      - .:/home/developer/ecscope
    command: >
      bash -c "
        cd build &&
        cmake -DECSCOPE_ENABLE_GRAPHICS=ON .. &&
        make -j\$$(nproc) &&
        ctest --output-on-failure
      "
```

### Continuous Integration Setup

**`.github/workflows/build.yml`**:
```yaml
name: ECScope Build and Test

on:
  push:
    branches: [ main, develop ]
  pull_request:
    branches: [ main ]

jobs:
  build-linux:
    runs-on: ubuntu-latest
    strategy:
      matrix:
        config:
          - { name: "Console Debug", cmake_args: "-DCMAKE_BUILD_TYPE=Debug -DECSCOPE_ENABLE_GRAPHICS=OFF" }
          - { name: "Console Release", cmake_args: "-DCMAKE_BUILD_TYPE=Release -DECSCOPE_ENABLE_GRAPHICS=OFF" }
          - { name: "Graphics Debug", cmake_args: "-DCMAKE_BUILD_TYPE=Debug -DECSCOPE_ENABLE_GRAPHICS=ON" }
          - { name: "Graphics Release", cmake_args: "-DCMAKE_BUILD_TYPE=Release -DECSCOPE_ENABLE_GRAPHICS=ON" }
        
    steps:
    - uses: actions/checkout@v3
    
    - name: Install Dependencies
      run: |
        sudo apt-get update
        sudo apt-get install -y build-essential cmake libsdl2-dev libgl1-mesa-dev
        
    - name: Configure CMake
      run: |
        mkdir build
        cd build
        cmake ${{ matrix.config.cmake_args }} ..
        
    - name: Build
      run: |
        cd build
        make -j$(nproc)
        
    - name: Test
      run: |
        cd build
        ctest --output-on-failure
        
    - name: Run Performance Tests
      if: matrix.config.name == 'Console Release'
      run: |
        cd build
        ./ecscope_performance_lab_console --benchmark --output=performance_results.json
        
    - name: Upload Performance Results
      if: matrix.config.name == 'Console Release'
      uses: actions/upload-artifact@v3
      with:
        name: performance-results
        path: build/performance_results.json

  build-windows:
    runs-on: windows-latest
    
    steps:
    - uses: actions/checkout@v3
    
    - name: Setup vcpkg
      run: |
        git clone https://github.com/Microsoft/vcpkg.git
        .\vcpkg\bootstrap-vcpkg.bat
        .\vcpkg\vcpkg install sdl2:x64-windows
        
    - name: Configure CMake
      run: |
        mkdir build
        cd build
        cmake -DCMAKE_TOOLCHAIN_FILE=..\vcpkg\scripts\buildsystems\vcpkg.cmake -DECSCOPE_ENABLE_GRAPHICS=ON ..
        
    - name: Build
      run: |
        cd build
        cmake --build . --config Release --parallel 4
        
    - name: Test
      run: |
        cd build
        ctest --output-on-failure --config Release

  build-macos:
    runs-on: macos-latest
    
    steps:
    - uses: actions/checkout@v3
    
    - name: Install Dependencies
      run: |
        brew install cmake sdl2
        
    - name: Configure and Build
      run: |
        mkdir build && cd build
        cmake -DCMAKE_BUILD_TYPE=Release -DECSCOPE_ENABLE_GRAPHICS=ON ..
        make -j$(sysctl -n hw.ncpu)
        
    - name: Test
      run: |
        cd build
        ctest --output-on-failure
```

## Performance Optimization

### Build-Time Optimizations

**Parallel Compilation**:
```bash
# Use all available cores
make -j$(nproc)  # Linux
make -j$(sysctl -n hw.ncpu)  # macOS
cmake --build . --parallel %NUMBER_OF_PROCESSORS%  # Windows
```

**Precompiled Headers**:
```cmake
# Add precompiled headers for faster compilation
target_precompile_headers(ecscope PRIVATE
    <iostream>
    <vector>
    <memory>
    <string>
    <unordered_map>
    <algorithm>
    <chrono>
    include/core/types.hpp
    include/ecs/component.hpp
)
```

**Unity Builds**:
```cmake
# Enable unity builds for faster compilation
set_target_properties(ecscope PROPERTIES
    UNITY_BUILD ON
    UNITY_BUILD_BATCH_SIZE 8
)
```

### Runtime Performance Configuration

**Profile-Guided Optimization (GCC)**:
```bash
# Step 1: Build with profiling
cmake -DCMAKE_CXX_FLAGS="-fprofile-generate" ..
make

# Step 2: Run typical workload
./ecscope_performance_lab_console --profile-workload

# Step 3: Rebuild with optimization
cmake -DCMAKE_CXX_FLAGS="-fprofile-use" ..
make
```

**Link-Time Optimization**:
```cmake
# Enable LTO for maximum performance
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    set_target_properties(ecscope PROPERTIES
        INTERPROCEDURAL_OPTIMIZATION ON
    )
endif()
```

### Educational Build Configurations

**Memory Teaching Configuration**:
```cmake
# Optimized for teaching memory concepts
set(ECSCOPE_MEMORY_TEACHING_MODE ON)
target_compile_definitions(ecscope PRIVATE
    ECSCOPE_DETAILED_ALLOCATION_TRACKING=1
    ECSCOPE_CACHE_LINE_ANALYSIS=1
    ECSCOPE_MEMORY_ACCESS_VISUALIZATION=1
)
```

**Performance Teaching Configuration**:
```cmake
# Optimized for teaching performance concepts
set(ECSCOPE_PERFORMANCE_TEACHING_MODE ON)
target_compile_definitions(ecscope PRIVATE
    ECSCOPE_DETAILED_TIMING=1
    ECSCOPE_SYSTEM_PROFILING=1
    ECSCOPE_BOTTLENECK_ANALYSIS=1
)
```

---

**ECScope Build System: Flexible configuration for every learning scenario, optimized compilation for every platform, comprehensive tooling for every developer.**

*The build system enables seamless development across platforms while maintaining educational focus and performance excellence.*