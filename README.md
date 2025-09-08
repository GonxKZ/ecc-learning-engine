# ECScope - Educational ECS Engine

**A modern C++20 Entity-Component-System engine designed for learning and experimentation**

[![C++20](https://img.shields.io/badge/C%2B%2B-20-blue.svg)](https://en.cppreference.com/w/cpp/20)
[![CMake](https://img.shields.io/badge/CMake-3.22%2B-green.svg)](https://cmake.org/)
[![License](https://img.shields.io/badge/License-MIT-green.svg)](LICENSE)
[![Build Status](https://img.shields.io/badge/Build-Passing-brightgreen.svg)](https://github.com/GonxKZ/entities-cpp2/actions)

## What is ECScope?

ECScope is an educational ECS (Entity-Component-System) engine that demonstrates modern C++20 patterns and game engine architecture. It's designed to be:

- **Educational First**: Clear, well-commented code with extensive documentation
- **Practical**: Real 2D physics, rendering, and memory management systems
- **Modern**: Leverages C++20 features like concepts, ranges, and modules
- **Performance-Aware**: Includes profiling tools and optimization techniques
- **Modular**: Clean separation between core systems and optional features

## Quick Start

```bash
# Clone the repository
git clone https://github.com/GonxKZ/entities-cpp2.git
cd entities-cpp2

# Build (console version - no dependencies needed)
mkdir build && cd build
cmake ..
cmake --build . --parallel

# Run the main demo
./ecscope_console

# Or with full graphics support (requires SDL2)
cmake .. -DECSCOPE_ENABLE_GRAPHICS=ON
cmake --build . --parallel
./ecscope_ui
```

## Core Features

### 🏗 ECS Architecture
- **Modern ECS Design**: Component storage with archetype optimization
- **Type-Safe Components**: Template-based component registration
- **Flexible Queries**: Iterator-based entity filtering and access
- **System Management**: Automatic dependency resolution and scheduling

### 🧠 Memory Management
- **Custom Allocators**: Arena, pool, and PMR allocators for different use cases
- **Memory Tracking**: Comprehensive allocation monitoring and leak detection
- **Educational Tools**: Real-time memory usage visualization and analysis
- **Performance Analysis**: Allocator benchmarking and optimization guidance

### ⚛ 2D Physics Engine
- **Collision Detection**: Broad and narrow phase collision detection
- **Physics Simulation**: Rigid body dynamics with constraint solving
- **Debug Visualization**: Real-time physics debug rendering
- **Educational Examples**: Step-by-step physics concept demonstrations

### 🎨 2D Rendering
- **OpenGL 3.3+ Rendering**: Modern shader-based rendering pipeline
- **Batch Rendering**: Efficient sprite batching for performance
- **Shader System**: Simple shader compilation and management
- **Debug Drawing**: Physics visualization and development tools

### 🔍 Development Tools
- **ImGui Integration**: Interactive debug panels and UI system
- **Performance Profiler**: Real-time performance monitoring
- **Memory Inspector**: Detailed memory usage analysis
- **ECS Debugger**: Entity and component inspection tools

## System Requirements

### Minimum Requirements
- **C++20** compliant compiler (GCC 10+, Clang 12+, MSVC 19.29+)
- **CMake 3.22** or higher
- **4GB RAM** (for compilation)

### Optional Dependencies
- **SDL2** - For windowing and input (enables graphics mode)
- **OpenGL 3.3+** - For hardware-accelerated rendering
- **ImGui** - Included as submodule (for debug UI)

## Build Options

```bash
# Console-only build (minimal dependencies)
cmake ..

# Full graphics build
cmake .. -DECSCOPE_ENABLE_GRAPHICS=ON

# Development build with all debugging tools
cmake .. -DECSCOPE_ENABLE_GRAPHICS=ON -DCMAKE_BUILD_TYPE=Debug

# Performance build for benchmarking
cmake .. -DCMAKE_BUILD_TYPE=Release -DECSCOPE_BUILD_BENCHMARKS=ON

# Available options:
# -DECSCOPE_ENABLE_GRAPHICS=ON/OFF     - Enable 2D rendering
# -DECSCOPE_ENABLE_PHYSICS=ON/OFF      - Enable physics engine
# -DECSCOPE_BUILD_EXAMPLES=ON/OFF      - Build example programs  
# -DECSCOPE_BUILD_TESTS=ON/OFF         - Build unit tests
# -DECSCOPE_BUILD_BENCHMARKS=ON/OFF    - Build performance benchmarks
# -DECSCOPE_ENABLE_SIMD=ON/OFF         - Enable SIMD optimizations
```

## Learning Path

### Beginner Level
1. **[Memory Basics](examples/beginner/01-memory-basics.cpp)** - Understanding custom allocators
2. **[Basic Physics](examples/beginner/02-basic-physics.cpp)** - Simple collision detection
3. **[Basic Rendering](examples/beginner/03-basic-rendering.cpp)** - Drawing sprites

### Intermediate Level
4. **[Physics Integration](examples/intermediate/04-physics-integration.cpp)** - Combining physics and rendering
5. **[Performance Analysis](examples/intermediate/05-performance-analysis.cpp)** - Profiling and optimization
6. **[Job System Basics](examples/intermediate/job-system-basics.cpp)** - Parallel processing

### Advanced Level
7. **[Custom Allocators](examples/advanced/07-custom-allocators.cpp)** - Building custom memory systems
8. **[Performance Laboratory](examples/advanced/09-performance-laboratory.cpp)** - Advanced optimization techniques

## Documentation

- **[Getting Started](docs/getting-started/GETTING_STARTED.md)** - Complete setup and learning guide
- **[API Reference](docs/api/API_REFERENCE.md)** - Complete API documentation
- **[Architecture Overview](docs/architecture/ARCHITECTURE.md)** - System design and patterns
- **[Build System Guide](docs/build-system/BUILD_AND_SETUP.md)** - Advanced build configuration
- **[Memory Management Guide](docs/guides/MEMORY_MANAGEMENT.md)** - Custom allocators and optimization
- **[Physics Guide](docs/guides/PHYSICS_2D.md)** - Physics system explanation
- **[Troubleshooting](docs/guides/TROUBLESHOOTING.md)** - Common issues and solutions

## Key Components

### Core Libraries
- **ecscope_core** - ECS registry, components, systems
- **ecscope_memory** - Custom allocators and memory tracking
- **ecscope_physics** - 2D physics simulation (optional)
- **ecscope_graphics** - 2D rendering pipeline (optional)
- **ecscope_ui** - ImGui-based debug interface (optional)

### Applications
- **ecscope_console** - Console-based application for core functionality
- **ecscope_ui** - Full graphical interface with debug tools
- **ecscope_minimal** - Ultra-lightweight example

### WebAssembly Support
- **ecscope_wasm** - Browser-based version for online demonstrations
- **web/** - HTML/JavaScript interface for web deployment

## Performance Characteristics

- **Entity Creation**: ~10M entities/second (with custom allocators)
- **Component Access**: Cache-friendly archetype storage
- **Memory Overhead**: ~8 bytes per entity (excluding components)
- **Rendering**: 100K+ sprites at 60 FPS (with batching)
- **Physics**: 5K+ rigid bodies in real-time

## Contributing

We welcome contributions! Please see [CONTRIBUTING.md](CONTRIBUTING.md) for guidelines.

### Development Workflow
1. Fork the repository
2. Create a feature branch: `git checkout -b feature/amazing-feature`  
3. Make your changes with tests
4. Run the test suite: `ctest --parallel`
5. Submit a pull request

## Educational Use

ECScope is specifically designed for:
- **Computer Science Courses**: Game engine architecture, memory management
- **Game Development Classes**: ECS patterns, physics simulation
- **Performance Engineering**: Allocation strategies, cache optimization
- **Self-Study**: Well-commented code with extensive examples

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## Acknowledgments

- Modern C++20 features and best practices
- Educational focus with production-quality code
- Community-driven development with comprehensive documentation

---

**ECScope: Learn by building, understand by experimenting**