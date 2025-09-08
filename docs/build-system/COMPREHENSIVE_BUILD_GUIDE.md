# ECScope Build System Guide

**Complete guide to building, configuring, and deploying ECScope**

## Table of Contents

1. [Quick Start](#quick-start)
2. [Build Options Reference](#build-options-reference)
3. [Platform-Specific Guides](#platform-specific-guides)
4. [Development Builds](#development-builds)
5. [WebAssembly Builds](#webassembly-builds)
6. [Troubleshooting](#troubleshooting)
7. [Advanced Configuration](#advanced-configuration)

## Quick Start

### Minimal Console Build (No Dependencies)

```bash
git clone https://github.com/GonxKZ/entities-cpp2.git
cd entities-cpp2
mkdir build && cd build
cmake ..
cmake --build . --parallel
./ecscope_console
```

### Full Graphics Build (Requires SDL2)

```bash
# Install SDL2 first (see platform guides below)
cmake .. -DECSCOPE_ENABLE_GRAPHICS=ON
cmake --build . --parallel
./ecscope_ui
```

### Development Build (All Features)

```bash
cmake .. \
  -DCMAKE_BUILD_TYPE=Debug \
  -DECSCOPE_ENABLE_GRAPHICS=ON \
  -DECSCOPE_BUILD_TESTS=ON \
  -DECSCOPE_BUILD_EXAMPLES=ON \
  -DECSCOPE_BUILD_BENCHMARKS=ON
cmake --build . --parallel
ctest --parallel  # Run tests
```

## Build Options Reference

### Core Options

| Option | Default | Description |
|--------|---------|-------------|
| `ECSCOPE_BUILD_TESTS` | ON | Build comprehensive test suite |
| `ECSCOPE_BUILD_BENCHMARKS` | ON | Build performance benchmarks |
| `ECSCOPE_BUILD_EXAMPLES` | ON | Build educational examples |
| `ECSCOPE_ENABLE_INSTRUMENTATION` | ON | Enable tracing & memory hooks |

### System Features

| Option | Default | Description |
|--------|---------|-------------|
| `ECSCOPE_ENABLE_GRAPHICS` | ON | Enable 2D graphics (requires SDL2) |
| `ECSCOPE_ENABLE_PHYSICS` | ON | Enable 2D/3D physics system |
| `ECSCOPE_ENABLE_SCRIPTING` | OFF | Enable Python/Lua integration |
| `ECSCOPE_ENABLE_JOB_SYSTEM` | ON | Enable work-stealing job system |
| `ECSCOPE_ENABLE_MEMORY_ANALYSIS` | ON | Enable memory analysis tools |
| `ECSCOPE_ENABLE_PERFORMANCE_LAB` | ON | Enable performance laboratory |

### Performance Options

| Option | Default | Description |
|--------|---------|-------------|
| `ECSCOPE_ENABLE_SIMD` | ON | Enable SIMD optimizations (AVX2/SSE) |
| `ECSCOPE_ENABLE_LOCKFREE` | ON | Enable lock-free data structures |
| `ECSCOPE_ENABLE_NUMA` | OFF | Enable NUMA-aware allocators |
| `ECSCOPE_ENABLE_HARDWARE_DETECTION` | ON | Enable hardware capability detection |

### Development Options

| Option | Default | Description |
|--------|---------|-------------|
| `ECSCOPE_ENABLE_ASAN` | OFF | Enable AddressSanitizer |
| `ECSCOPE_ENABLE_TSAN` | OFF | Enable ThreadSanitizer |
| `ECSCOPE_ENABLE_UBSAN` | OFF | Enable UndefinedBehaviorSanitizer |
| `ECSCOPE_ENABLE_PROFILING` | OFF | Enable detailed profiling support |

### Build Configuration Examples

**Console-only build:**
```bash
cmake .. \
  -DECSCOPE_ENABLE_GRAPHICS=OFF \
  -DECSCOPE_BUILD_EXAMPLES=OFF
```

**Performance testing build:**
```bash
cmake .. \
  -DCMAKE_BUILD_TYPE=Release \
  -DECSCOPE_BUILD_BENCHMARKS=ON \
  -DECSCOPE_ENABLE_SIMD=ON
```

**Debug build with sanitizers:**
```bash
cmake .. \
  -DCMAKE_BUILD_TYPE=Debug \
  -DECSCOPE_ENABLE_ASAN=ON \
  -DECSCOPE_ENABLE_UBSAN=ON
```

**Educational build:**
```bash
cmake .. \
  -DECSCOPE_ENABLE_GRAPHICS=ON \
  -DECSCOPE_ENABLE_MEMORY_ANALYSIS=ON \
  -DECSCOPE_ENABLE_PERFORMANCE_LAB=ON \
  -DECSCOPE_BUILD_EXAMPLES=ON
```

## Platform-Specific Guides

### Ubuntu/Debian Linux

#### Install Dependencies

```bash
# Essential build tools
sudo apt update
sudo apt install build-essential cmake git

# Graphics support (optional)
sudo apt install libsdl2-dev libgl1-mesa-dev

# Additional development tools
sudo apt install gdb valgrind clang-format

# For scripting support (optional)
sudo apt install python3-dev liblua5.3-dev
```

#### Build Commands

```bash
# Clone and build
git clone https://github.com/GonxKZ/entities-cpp2.git
cd entities-cpp2
mkdir build && cd build

# Configure build
cmake .. -DECSCOPE_ENABLE_GRAPHICS=ON

# Build
make -j$(nproc)  # Use all CPU cores

# Install (optional)
sudo make install
```

### Fedora/CentOS/RHEL

#### Install Dependencies

```bash
# Essential build tools
sudo dnf install gcc-c++ cmake git

# Graphics support (optional)
sudo dnf install SDL2-devel mesa-libGL-devel

# For scripting support (optional)
sudo dnf install python3-devel lua-devel
```

### macOS

#### Install Dependencies

```bash
# Install Xcode command line tools
xcode-select --install

# Install Homebrew (if not already installed)
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"

# Install dependencies
brew install cmake sdl2

# For scripting support (optional)
brew install python lua
```

#### Build Commands

```bash
git clone https://github.com/GonxKZ/entities-cpp2.git
cd entities-cpp2
mkdir build && cd build

# Configure for macOS
cmake .. \
  -DECSCOPE_ENABLE_GRAPHICS=ON \
  -DCMAKE_CXX_COMPILER=clang++

# Build
make -j$(sysctl -n hw.ncpu)  # Use all CPU cores
```

### Windows

#### Option 1: Visual Studio

1. **Install Visual Studio 2022** with C++ development tools
2. **Install vcpkg** for package management:
   ```cmd
   git clone https://github.com/Microsoft/vcpkg.git
   cd vcpkg
   .\bootstrap-vcpkg.bat
   .\vcpkg integrate install
   ```

3. **Install dependencies:**
   ```cmd
   .\vcpkg install sdl2:x64-windows
   ```

4. **Build ECScope:**
   ```cmd
   git clone https://github.com/GonxKZ/entities-cpp2.git
   cd entities-cpp2
   mkdir build && cd build
   
   cmake .. ^
     -DCMAKE_TOOLCHAIN_FILE=C:\path\to\vcpkg\scripts\buildsystems\vcpkg.cmake ^
     -DECSCOPE_ENABLE_GRAPHICS=ON ^
     -A x64
   
   cmake --build . --config Release
   ```

#### Option 2: MinGW-w64 (MSYS2)

1. **Install MSYS2** from https://www.msys2.org/
2. **Install dependencies:**
   ```bash
   pacman -S mingw-w64-x86_64-gcc
   pacman -S mingw-w64-x86_64-cmake
   pacman -S mingw-w64-x86_64-SDL2
   ```

3. **Build:**
   ```bash
   mkdir build && cd build
   cmake .. -G "MinGW Makefiles" -DECSCOPE_ENABLE_GRAPHICS=ON
   mingw32-make -j4
   ```

## Development Builds

### Debug Configuration

```bash
cmake .. \
  -DCMAKE_BUILD_TYPE=Debug \
  -DECSCOPE_ENABLE_GRAPHICS=ON \
  -DECSCOPE_BUILD_TESTS=ON \
  -DECSCOPE_ENABLE_ASAN=ON \
  -DECSCOPE_ENABLE_UBSAN=ON
```

**Features:**
- Full debugging symbols (`-g3`)
- No optimization (`-O0`)
- Address sanitizer for memory error detection
- Undefined behavior sanitizer
- All assertions enabled

### Testing Build

```bash
cmake .. \
  -DCMAKE_BUILD_TYPE=RelWithDebInfo \
  -DECSCOPE_BUILD_TESTS=ON \
  -DECSCOPE_BUILD_BENCHMARKS=ON

# Build and run tests
cmake --build . --parallel
ctest --parallel --output-on-failure
```

### Profiling Build

```bash
cmake .. \
  -DCMAKE_BUILD_TYPE=Release \
  -DECSCOPE_ENABLE_PROFILING=ON \
  -DECSCOPE_BUILD_BENCHMARKS=ON

# Build and run benchmarks
cmake --build . --parallel
./benchmark_ecs_performance
./benchmark_memory_core
```

## WebAssembly Builds

ECScope supports WebAssembly compilation using Emscripten:

### Install Emscripten

```bash
# Clone and install Emscripten
git clone https://github.com/emscripten-core/emsdk.git
cd emsdk
./emsdk install latest
./emsdk activate latest
source ./emsdk_env.sh
```

### Build for Web

```bash
cd entities-cpp2
mkdir build-wasm && cd build-wasm

# Configure for WebAssembly
emcmake cmake .. \
  -DCMAKE_BUILD_TYPE=Release \
  -DECSCOPE_ENABLE_GRAPHICS=OFF \
  -DECSCOPE_BUILD_EXAMPLES=ON \
  -DCMAKE_CXX_FLAGS="-s USE_SDL=2"

# Build
emmake make -j4

# The build produces .wasm and .js files
ls *.wasm *.js
```

### Web Deployment

```bash
# Start local server for testing
python3 -m http.server 8000

# Open browser to http://localhost:8000
# Navigate to generated HTML files
```

### WebAssembly Configuration Options

```bash
# Basic web build
emcmake cmake .. -DECSCOPE_TARGET_WEB=ON

# With graphics (requires WebGL)
emcmake cmake .. \
  -DECSCOPE_TARGET_WEB=ON \
  -DECSCOPE_ENABLE_GRAPHICS=ON \
  -DCMAKE_CXX_FLAGS="-s USE_SDL=2 -s USE_WEBGL2=1"

# Optimized for size
emcmake cmake .. \
  -DECSCOPE_TARGET_WEB=ON \
  -DCMAKE_BUILD_TYPE=MinSizeRel \
  -DCMAKE_CXX_FLAGS="-Os -s WASM=1"
```

## Troubleshooting

### Common Build Issues

#### SDL2 Not Found

**Error:**
```
CMake Error: Could not find SDL2
```

**Solutions:**

*Linux:*
```bash
# Ubuntu/Debian
sudo apt install libsdl2-dev

# Fedora
sudo dnf install SDL2-devel
```

*macOS:*
```bash
brew install sdl2
```

*Windows:*
```cmd
# Using vcpkg
vcpkg install sdl2:x64-windows

# Or set SDL2_DIR manually
cmake .. -DSDL2_DIR=C:\path\to\SDL2
```

#### C++20 Compiler Error

**Error:**
```
error: 'concept' was not declared in this scope
```

**Solution:**
Ensure you have a C++20 compatible compiler:

```bash
# Check GCC version (need 10+)
gcc --version

# Check Clang version (need 12+)
clang --version

# Explicitly set compiler
cmake .. -DCMAKE_CXX_COMPILER=g++-11
```

#### ImGui Not Found

**Error:**
```
ImGui not found in external/imgui/
```

**Solution:**
```bash
# Initialize git submodules
git submodule update --init --recursive

# Or manually download ImGui
mkdir -p external
cd external
git clone https://github.com/ocornut/imgui.git
```

#### Linker Errors on Linux

**Error:**
```
undefined reference to pthread_create
```

**Solution:**
```bash
# Install threading library
sudo apt install libc6-dev

# Or explicitly link pthread
cmake .. -DCMAKE_EXE_LINKER_FLAGS="-pthread"
```

### Performance Issues

#### Slow Debug Builds

Debug builds are inherently slower. For development with better performance:

```bash
cmake .. -DCMAKE_BUILD_TYPE=RelWithDebInfo
```

#### Memory Usage During Compilation

If compilation runs out of memory:

```bash
# Limit parallel jobs
make -j2  # Use only 2 cores instead of all

# Or build specific targets
make ecscope_core
make ecscope_console
```

### Runtime Issues

#### Assertion Failures

```bash
# Disable assertions in release builds
cmake .. -DCMAKE_BUILD_TYPE=Release -DNDEBUG=ON
```

#### OpenGL Context Errors

Ensure you have proper graphics drivers installed:

```bash
# Linux - install Mesa drivers
sudo apt install mesa-utils
glxinfo | grep OpenGL  # Should show version 3.3+
```

## Advanced Configuration

### Cross-Compilation

#### ARM64 Linux

```bash
# Install cross-compiler
sudo apt install gcc-aarch64-linux-gnu g++-aarch64-linux-gnu

# Configure for ARM64
cmake .. \
  -DCMAKE_SYSTEM_NAME=Linux \
  -DCMAKE_SYSTEM_PROCESSOR=aarch64 \
  -DCMAKE_C_COMPILER=aarch64-linux-gnu-gcc \
  -DCMAKE_CXX_COMPILER=aarch64-linux-gnu-g++
```

#### Android (NDK)

```bash
export ANDROID_NDK=/path/to/android-ndk

cmake .. \
  -DCMAKE_TOOLCHAIN_FILE=$ANDROID_NDK/build/cmake/android.toolchain.cmake \
  -DANDROID_ABI=arm64-v8a \
  -DANDROID_PLATFORM=android-21
```

### Custom Installation

```bash
# Install to custom directory
cmake .. -DCMAKE_INSTALL_PREFIX=/opt/ecscope
make install

# Create package
cpack -G DEB     # Debian package
cpack -G RPM     # RedHat package  
cpack -G ZIP     # ZIP archive
```

### Integration with IDEs

#### Visual Studio Code

Create `.vscode/settings.json`:
```json
{
    "cmake.buildDirectory": "${workspaceFolder}/build",
    "cmake.configureArgs": [
        "-DECSCOPE_ENABLE_GRAPHICS=ON",
        "-DECSCOPE_BUILD_TESTS=ON"
    ]
}
```

#### CLion

CLion automatically detects CMake projects. Configure build profiles:
- Debug: `-DCMAKE_BUILD_TYPE=Debug -DECSCOPE_BUILD_TESTS=ON`
- Release: `-DCMAKE_BUILD_TYPE=Release -DECSCOPE_BUILD_BENCHMARKS=ON`

#### Qt Creator

Open `CMakeLists.txt` directly in Qt Creator and configure build variants.

### Continuous Integration

#### GitHub Actions Example

```yaml
name: Build and Test
on: [push, pull_request]

jobs:
  build:
    runs-on: ubuntu-latest
    steps:
    - uses: actions/checkout@v2
      with:
        submodules: recursive
    
    - name: Install dependencies
      run: |
        sudo apt update
        sudo apt install libsdl2-dev libgl1-mesa-dev
    
    - name: Configure
      run: |
        cmake -B build -DECSCOPE_ENABLE_GRAPHICS=ON -DECSCOPE_BUILD_TESTS=ON
    
    - name: Build
      run: cmake --build build --parallel
    
    - name: Test
      run: cd build && ctest --parallel --output-on-failure
```

This build system guide provides comprehensive coverage of ECScope's build configuration. The modular CMake design allows for flexible builds ranging from minimal console applications to full-featured educational environments with graphics, physics, and debugging tools.