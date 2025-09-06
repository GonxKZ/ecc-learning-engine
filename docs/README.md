# ECScope Documentation

Welcome to the comprehensive documentation for ECScope - Premier Educational ECS Engine.

## 📚 Documentation Structure

### 📖 Guides & Tutorials
- **[Getting Started](getting-started/GETTING_STARTED.md)** - Start your journey with ECScope
- **[Memory Management](guides/MEMORY_MANAGEMENT.md)** - Advanced memory management (Arena, Pool, PMR, NUMA)
- **[Memory Laboratory](guides/MEMORY_LAB.md)** - Interactive memory behavior analysis
- **[Physics Engine](guides/PHYSICS_2D.md)** - Rigid body, soft body, and fluid simulation
- **[2D Rendering](guides/RENDERING_2D.md)** - Modern OpenGL rendering pipeline
- **[Debugging Tools](guides/DEBUGGING_TOOLS.md)** - Visual debugging and profiling
- **[Troubleshooting](guides/TROUBLESHOOTING.md)** - Common issues and solutions
- **[Advanced Memory Features](guides/ADVANCED_MEMORY_FEATURES.md)** - NUMA, GC, and specialized pools
- **[Advanced Optimizations](guides/ADVANCED_OPTIMIZATIONS.md)** - Performance optimization techniques

### 🔧 Implementation Details
- **[ECS Integration](implementation/ECS_INTEGRATION_SUMMARY.md)** - Core ECS architecture and patterns
- **[Memory Systems](implementation/ADVANCED_MEMORY_SYSTEMS.md)** - Deep dive into memory management
- **[Visual Inspector](implementation/VISUAL_ECS_INSPECTOR_IMPLEMENTATION_SUMMARY.md)** - ECS debugging and visualization
- **[Visual Inspector Integration](implementation/VISUAL_ECS_INSPECTOR_INTEGRATION.md)** - Integration guide
- **[Performance Benchmarker](implementation/ECS_PERFORMANCE_BENCHMARKER_IMPLEMENTATION.md)** - Benchmarking and optimization
- **[Workflow Configuration](implementation/WORKFLOW_FIXES_SUMMARY.md)** - CI/CD setup and troubleshooting
- **[Error Analysis](implementation/all-errors.txt)** - GitHub Actions workflow error logs

### 🌐 Interactive Documentation
- **[Interactive Tutorials](interactive/README.md)** - Browser-based learning experience
- **[Live Code Examples](interactive/api_browser.html)** - Executable code samples
- **[Performance Dashboard](interactive/performance_dashboard.html)** - Real-time metrics visualization
- **[Code Playground](interactive/playground.html)** - Interactive C++ environment

### 🏗️ Architecture & System Design
- **[System Architecture](architecture/ARCHITECTURE.md)** - Overall system design and patterns
- **[ECS System Design](architecture/ECS_SYSTEM.md)** - Entity-Component-System architecture

### 📊 Performance & Benchmarks
- **[Performance Analysis](benchmarks/PERFORMANCE_ANALYSIS.md)** - Performance analysis and optimization
- **[Optimization Techniques](benchmarks/OPTIMIZATION_TECHNIQUES.md)** - Advanced optimization strategies

### 🔧 API Reference
- **[Complete API Reference](api/API_REFERENCE.md)** - Comprehensive API documentation

### 🛠️ Build System & Integration
- **[Build and Setup](build-system/BUILD_AND_SETUP.md)** - CMake configuration and platform setup
- **[Build System Fixed](guides/BUILD_SYSTEM_FIXED.md)** - Recent build system improvements
- **[ECS Memory Integration](integration/ECS_MEMORY_INTEGRATION.md)** - Memory system integration

### 📚 Tutorials & Examples
- **[Examples Guide](tutorials/EXAMPLES_GUIDE.md)** - Comprehensive examples and tutorials

### 🚀 Advanced Topics
- **[3D Physics Guide](advanced/3D_PHYSICS_GUIDE.md)** - Advanced 3D physics implementation
- **[Scripting Integration](advanced/SCRIPTING_INTEGRATION_GUIDE.md)** - Language integration guide

### 🛠️ Development Resources
- **[CMake Configurations](cmake-backups/README.md)** - Build system configurations and backups
- **[Documentation Builder](launch_docs.sh)** - Interactive documentation server

## 🚀 Quick Navigation

### For Beginners
1. Start with **[Getting Started](getting-started/GETTING_STARTED.md)**
2. Explore **[Interactive Tutorials](interactive/README.md)**
3. Try the **[Code Playground](interactive/playground.html)**

### For Advanced Users
1. Review **[Implementation Details](implementation/)**
2. Study **[Performance Analysis](benchmarks/PERFORMANCE_ANALYSIS.md)**
3. Examine **[API Reference](api/API_REFERENCE.md)**

### For Contributors
1. Check **[Build System Guide](build-system/BUILD_AND_SETUP.md)**
2. Review **[Workflow Configuration](implementation/WORKFLOW_FIXES_SUMMARY.md)**
3. Understand **[ECS Architecture](architecture/ECS_SYSTEM.md)**

## 📱 Interactive Features

ECScope documentation includes several interactive components:

- **Live Code Execution** - Run C++ code directly in your browser
- **Real-time Performance Monitoring** - Watch ECS operations in action
- **Interactive Visualizations** - Understand memory layouts and algorithms
- **Progressive Learning Paths** - Adaptive tutorials based on your progress

## 🔍 Search & Navigation

Use the search functionality in the interactive documentation to quickly find:
- API functions and classes
- Implementation details
- Performance characteristics
- Code examples
- Troubleshooting guides

## 📖 How to Use This Documentation

1. **Linear Learning**: Follow the guides in order for a comprehensive education
2. **Reference Mode**: Jump to specific API or implementation details
3. **Interactive Mode**: Use the browser-based tools for hands-on learning
4. **Problem Solving**: Check implementation details and error logs for troubleshooting

## 🛠️ Building the Documentation

To build and serve the interactive documentation locally:

```bash
# From the docs directory
./launch_docs.sh

# Or manually
python3 -m http.server 8080
```

Then open http://localhost:8080 in your browser.

## 🤝 Contributing to Documentation

Documentation improvements are always welcome! Please:

1. Keep explanations clear and concise
2. Include code examples where helpful
3. Update interactive components when adding new features
4. Maintain cross-references between related topics

---

**ECScope Documentation: Your complete guide to mastering ECS architecture and game engine development**

### Architecture
- **[System Overview](architecture/overview.md)** - Complete system architecture and design principles
- **[ECS System](architecture/ecs-system.md)** - Entity-Component-System implementation details
- **[Memory Management](architecture/memory-management.md)** - Custom allocators and memory optimization
- **[Physics Engine](architecture/physics-engine.md)** - 2D physics system architecture
- **[Rendering Pipeline](architecture/rendering-pipeline.md)** - Modern 2D rendering system

### Tutorials
- **[Examples Guide](tutorials/examples-guide.md)** - Comprehensive walkthrough of all examples
- **[Beginner Tutorials](tutorials/beginner/)** - Start here for basic concepts
- **[Intermediate Tutorials](tutorials/intermediate/)** - Advanced system integration
- **[Advanced Tutorials](tutorials/advanced/)** - Performance optimization and research

### Advanced Topics
- **[Memory Features](advanced/memory-features.md)** - Advanced memory management techniques
- **[Optimizations](advanced/optimizations.md)** - Performance optimization strategies
- **[Job System](advanced/job-system.md)** - Parallel processing and concurrency
- **[Lock-free Memory](advanced/lock-free-memory.md)** - Lock-free data structures

### Integration Guides
- **[ECS Integration](integration/ecs-integration.md)** - System integration patterns
- **[Scripting Integration](integration/scripting-integration.md)** - Python and Lua integration

### Build System
- **[Documentation](build-system/documentation.md)** - Detailed build system documentation
- **[Summary](build-system/summary.md)** - Build configuration overview

### API Reference
- **[Overview](api-reference/overview.md)** - Complete API documentation

### Additional Resources
- **[Physics Math](physics_math.md)** - Mathematical foundations
- **[Troubleshooting](TROUBLESHOOTING.md)** - Common issues and solutions
- **[Debugging Tools](DEBUGGING_TOOLS.md)** - Development and debugging utilities

## Learning Paths

### For Beginners (1-2 weeks)
1. Read the [Overview](getting-started/overview.md)
2. Follow the [Installation](getting-started/installation.md) guide
3. Work through [Beginner Tutorials](tutorials/beginner/)
4. Explore the [Memory Laboratory](tutorials/intermediate/memory-laboratory.md)

### For Intermediate Users (2-4 weeks)
1. Study the [Architecture](architecture/) documentation
2. Complete [Intermediate Tutorials](tutorials/intermediate/)
3. Experiment with [Performance Analysis](tutorials/advanced/performance-analysis.md)
4. Explore [Advanced Topics](advanced/)

### For Advanced Users and Researchers
1. Deep dive into [Advanced Tutorials](tutorials/advanced/)
2. Study [Optimization Techniques](tutorials/advanced/optimization-techniques.md)
3. Explore the complete [API Reference](api-reference/)
4. Contribute to the codebase and research

## Quick Navigation

- **Need help getting started?** → [Getting Started Overview](getting-started/overview.md)
- **Want to understand the architecture?** → [System Overview](architecture/overview.md)
- **Ready for hands-on learning?** → [Examples Guide](tutorials/examples-guide.md)
- **Looking for specific API details?** → [API Reference](api-reference/overview.md)
- **Having issues?** → [Troubleshooting](TROUBLESHOOTING.md)

## Educational Philosophy

ECScope is designed as a "memory lab in motion" - making invisible memory patterns visible through:
- Real-time performance analysis
- Interactive memory experiments
- Progressive complexity learning
- Production-quality implementation
- Comprehensive documentation

Start your journey with the [Getting Started Overview](getting-started/overview.md) to understand ECScope's unique educational approach.