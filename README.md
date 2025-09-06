# ECScope - Premier Educational ECS Engine

**World-class educational ECS engine in C++20 with complete physics simulation, modern 2D rendering, and advanced memory management**

> *The definitive laboratory for understanding Entity-Component-System architecture, data-oriented design, and modern game engine development*

[![C++20](https://img.shields.io/badge/C%2B%2B-20-blue.svg)](https://en.cppreference.com/w/cpp/20)
[![CMake](https://img.shields.io/badge/CMake-3.22%2B-green.svg)](https://cmake.org/)
[![License](https://img.shields.io/badge/License-MIT-green.svg)](LICENSE)
[![Build Status](https://img.shields.io/badge/Build-Passing-brightgreen.svg)](https://github.com/GonxKZ/ecc-learning-engine/actions)
[![Documentation](https://img.shields.io/badge/Docs-Complete-blue.svg)](docs/README.md)

## 🎯 What is ECScope?

ECScope is a **comprehensive educational ECS engine** that combines production-quality code with exceptional learning opportunities. It serves as both a fully functional game engine and an interactive learning platform for:

- **Entity-Component-System Architecture** with modern C++20 patterns
- **Advanced Memory Management** with custom allocators and real-time analysis
- **Data-Oriented Design** principles and optimization techniques
- **Game Engine Architecture** from foundation to advanced systems
- **Performance Engineering** with benchmarking and profiling tools

## 🚀 Quick Start

```bash
# Clone the repository
git clone https://github.com/GonxKZ/ecc-learning-engine.git
cd ecc-learning-engine

# Build the project
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . --parallel

# Run the main demo
./ecscope_demo

# Launch interactive documentation
cd .. && docs/launch_docs.sh
```

## 🎮 Core Features

### 🏗️ Advanced ECS Architecture
- **Sparse Sets & Archetypes** - Optimal component storage with cache-friendly access patterns
- **System Dependencies** - Automatic dependency resolution and parallel execution
- **Enhanced Queries** - Flexible, high-performance component filtering and iteration
- **Real-time Inspector** - Visual debugging and profiling of ECS operations

### 🧠 Memory Management Laboratory
- **Custom Allocators** - Arena, Pool, PMR, and NUMA-aware allocation strategies
- **Memory Debugging** - Leak detection, fragmentation analysis, and optimization tools
- **Garbage Collection** - Optional generational GC with educational visualization
- **Performance Analysis** - Real-time memory behavior tracking and insights

### ⚗️ Physics Engine
- **Rigid Body Simulation** - Complete 2D physics with collision detection and resolution
- **Soft Body Dynamics** - Mass-spring systems and finite element methods
- **Fluid Simulation** - SPH and PBF implementations with particle systems
- **Advanced Materials** - Temperature-dependent properties and damage modeling

### 🔊 3D Spatial Audio
- **HRTF Processing** - Realistic 3D audio positioning with head tracking
- **Environmental Audio** - Reverb, occlusion, and reflection simulation  
- **Real-time DSP** - Professional-grade audio processing with SIMD optimization
- **Educational Tools** - Interactive audio concepts and algorithm visualization

### 🌐 Networking & Synchronization
- **ECS Replication** - Efficient entity synchronization across networks
- **Client Prediction** - Lag compensation and rollback systems
- **Custom Protocols** - UDP reliability with educational packet analysis
- **Distributed Systems** - Educational networking concepts and visualization

### 🎨 Advanced Rendering & Tooling
- **Scene Editor** - Visual entity creation with drag-and-drop functionality
- **Asset Pipeline** - Multi-format importers with hot-reloading support
- **Shader System** - Visual shader editor with real-time compilation
- **Performance Profiler** - Comprehensive benchmarking and optimization tools

## 📚 Learning Path

ECScope provides a structured learning experience:

1. **[Getting Started](docs/guides/getting-started.md)** - Basic ECS concepts and first steps
2. **[Memory Management](docs/guides/memory-systems.md)** - Custom allocators and optimization
3. **[Physics Integration](docs/guides/physics-engine.md)** - Simulation and collision detection
4. **[Advanced Topics](docs/guides/advanced-features.md)** - Networking, audio, and tooling
5. **[Performance Engineering](docs/benchmarks/README.md)** - Profiling and optimization techniques

## 🛠️ Building & Requirements

### Prerequisites
- **C++20** compliant compiler (GCC 10+, Clang 12+, MSVC 19.29+)
- **CMake 3.22** or higher
- **Git** for submodule management

### Optional Dependencies
- **SDL2** - For windowing and input (enables graphics mode)
- **OpenGL 3.3+** - For hardware-accelerated rendering
- **Python 3.9+** - For analysis scripts and documentation generation

### Build Options
```bash
# Educational build with all features
cmake .. -DECSCOPE_ENABLE_GRAPHICS=ON -DECSCOPE_ENABLE_AUDIO=ON -DCMAKE_BUILD_TYPE=Debug

# Performance build for benchmarking
cmake .. -DCMAKE_BUILD_TYPE=Release -DECSCOPE_ENABLE_BENCHMARKS=ON

# Minimal build for CI/testing
cmake .. -DECSCOPE_MINIMAL_BUILD=ON
```

## 🧪 Testing & Validation

ECScope includes comprehensive testing:

```bash
# Run all tests
ctest --parallel

# Run specific test suites
./tests/ecs_tests
./tests/memory_tests
./tests/physics_tests

# Performance benchmarks
./benchmarks/ecs_benchmark
./benchmarks/memory_benchmark
```

## 📖 Documentation

- **[API Reference](docs/api/README.md)** - Complete API documentation
- **[Implementation Guides](docs/implementation/README.md)** - Deep-dive technical details
- **[Interactive Tutorials](docs/interactive/README.md)** - Browser-based learning
- **[Performance Analysis](docs/benchmarks/README.md)** - Optimization techniques

## 🤝 Contributing

ECScope welcomes contributions! See [CONTRIBUTING.md](CONTRIBUTING.md) for guidelines.

### Development Workflow
1. Fork the repository
2. Create a feature branch: `git checkout -b feature/amazing-feature`
3. Make your changes with tests
4. Run the full test suite: `ctest --parallel`
5. Submit a pull request

## 📄 License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## 🙏 Acknowledgments

- Built with modern C++20 and educational excellence in mind
- Inspired by production game engines and academic research
- Community-driven development with comprehensive documentation

---

**ECScope: Where learning meets production-quality code**