# ECScope Examples - Progressive Learning Path

This directory contains a comprehensive collection of educational examples demonstrating ECScope's capabilities, organized by complexity level to provide a structured learning experience.

## Learning Path Overview

The examples are organized into four categories:

### 🟢 Beginner Examples (`beginner/`)
**Perfect for**: First-time ECS users, memory management beginners
**Time**: 1-2 weeks
**Prerequisites**: Basic C++ knowledge

- **[01-memory-basics.cpp](beginner/01-memory-basics.cpp)** - Understanding custom allocators and memory tracking
- **[02-basic-physics.cpp](beginner/02-basic-physics.cpp)** - Simple physics components and basic collision
- **[03-basic-rendering.cpp](beginner/03-basic-rendering.cpp)** - Sprite rendering and basic graphics concepts

### 🟡 Intermediate Examples (`intermediate/`)
**Perfect for**: Understanding system integration and optimization
**Time**: 2-4 weeks
**Prerequisites**: Completed beginner examples

- **[04-physics-integration.cpp](intermediate/04-physics-integration.cpp)** - Complete physics system with ECS integration
- **[05-performance-analysis.cpp](intermediate/05-performance-analysis.cpp)** - Memory performance validation and analysis
- **[06-rendering-benchmarks.cpp](intermediate/06-rendering-benchmarks.cpp)** - GPU optimization and batching performance

### 🔴 Advanced Examples (`advanced/`)
**Perfect for**: Performance optimization and research
**Time**: 4+ weeks
**Prerequisites**: Strong understanding of intermediate concepts

- **[07-custom-allocators.cpp](advanced/07-custom-allocators.cpp)** - Advanced memory management techniques
- **[08-job-system-integration.cpp](advanced/08-job-system-integration.cpp)** - Parallel processing and concurrency
- **[09-performance-laboratory.cpp](advanced/09-performance-laboratory.cpp)** - Interactive performance experiments

### 📚 Reference Implementations (`reference/`)
**Complete, production-ready examples showcasing full system capabilities**

- **[optimization-showcase.cpp](reference/optimization-showcase.cpp)** - Comprehensive optimization demonstrations
- **[hardware-analysis.cpp](reference/hardware-analysis.cpp)** - Hardware-specific analysis and educational tools
- **[rendering-tutorials/](reference/rendering-tutorials/)** - Progressive rendering learning examples

## Quick Start Guide

### 30-Second Start
```bash
# Build and run the memory basics example
cd build
make
./ecscope_memory_basics
```

### Complete Educational Setup
```bash
# Build all examples
cmake -DECSCOPE_BUILD_EXAMPLES=ON ..
make

# Available example executables:
ls ecscope_example_* | sort
```

## Example Categories Explained

### Beginner Examples Focus
- **Core Concepts**: Understanding ECS architecture and components
- **Memory Basics**: Simple allocator usage and tracking
- **Visual Learning**: Immediate feedback and clear results
- **Foundation Building**: Essential patterns used throughout ECScope

### Intermediate Examples Focus
- **System Integration**: How different systems work together
- **Performance Awareness**: Understanding optimization impact
- **Real-world Patterns**: Production-ready implementation techniques
- **Analysis Tools**: Using ECScope's built-in profiling capabilities

### Advanced Examples Focus
- **Optimization Techniques**: Advanced memory and CPU optimization
- **Research Applications**: Extending ECScope for research projects
- **Production Readiness**: Scalable, high-performance implementations
- **Custom Development**: Building your own systems and components

### Reference Examples Purpose
- **Complete Implementations**: Full-featured, production-quality code
- **Best Practices**: Demonstrating optimal patterns and techniques
- **Educational Tools**: Interactive learning and experimentation
- **Research Platform**: Foundation for advanced research projects

## Building and Running Examples

### Individual Examples
```bash
# Build specific example category
make ecscope_beginner_examples
make ecscope_intermediate_examples
make ecscope_advanced_examples
```

### All Examples
```bash
# Build complete example suite
make examples
```

### With Graphics Support
```bash
# Enable full graphics capabilities
cmake -DECSCOPE_ENABLE_GRAPHICS=ON -DECSCOPE_BUILD_EXAMPLES=ON ..
make
```

## Learning Recommendations

### Week 1: Foundation Building
- Start with `01-memory-basics.cpp`
- Understand allocator concepts
- Learn ECS entity/component basics
- Explore memory tracking visualization

### Week 2: System Understanding
- Progress to `02-basic-physics.cpp`
- Learn component relationships
- Understand system execution order
- Experiment with physics parameters

### Week 3: Rendering and Graphics
- Work through `03-basic-rendering.cpp`
- Learn sprite rendering concepts
- Understand GPU optimization basics
- Explore camera and viewport systems

### Week 4+: Integration and Optimization
- Move to intermediate examples
- Focus on system integration patterns
- Learn performance analysis techniques
- Begin optimization experiments

## Educational Features

### Interactive Learning
- **Real-time Modification**: Change parameters and see immediate results
- **Visual Debugging**: Built-in debug visualization for all systems
- **Performance Monitoring**: Live memory and CPU usage analysis
- **Step-by-step Analysis**: Watch algorithms execute in detail

### Documentation Integration
- **Inline Documentation**: Comprehensive comments explaining every concept
- **Reference Links**: Direct connections to relevant documentation
- **Theory Connections**: Links between code implementation and computer science theory
- **Further Reading**: Suggestions for deeper exploration

### Experimental Platform
- **Parameter Tweaking**: Safe experimentation with system parameters
- **Performance Comparison**: Built-in benchmarking and comparison tools
- **Custom Extensions**: Framework for implementing your own examples
- **Research Foundation**: Platform for conducting performance research

## Getting Help

### Documentation
- **[Examples Guide](../docs/tutorials/examples-guide.md)** - Detailed walkthrough of each example
- **[Architecture Overview](../docs/architecture/overview.md)** - Understanding ECScope's design
- **[API Reference](../docs/api-reference/overview.md)** - Complete API documentation

### Common Issues
- **Build Problems**: Check the [Installation Guide](../docs/getting-started/installation.md)
- **Runtime Issues**: See [Troubleshooting](../docs/TROUBLESHOOTING.md)
- **Performance Questions**: Read [Performance Analysis](../docs/tutorials/advanced/performance-analysis.md)

### Next Steps
- **Custom Examples**: Use ECScope as a foundation for your own projects
- **Research Projects**: Extend ECScope for academic or industrial research
- **Contributions**: Help improve the educational experience for others

---

**Ready to start learning?** Begin with [01-memory-basics.cpp](beginner/01-memory-basics.cpp) and embark on your journey into advanced game engine architecture!