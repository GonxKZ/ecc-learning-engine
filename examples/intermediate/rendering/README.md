# ECScope 2D Rendering Tutorials

Welcome to the ECScope 2D Rendering tutorial series! These comprehensive tutorials will guide you through the fundamentals and advanced concepts of 2D graphics programming using ECScope's educational ECS framework.

## Tutorial Overview

### 🎯 Learning Objectives

By completing these tutorials, you will:
- Master the fundamentals of 2D graphics programming
- Understand modern rendering pipelines and optimization techniques
- Learn to create professional-quality 2D graphics applications
- Gain practical experience with Entity-Component-System architecture
- Develop skills in performance analysis and optimization

### 📚 Prerequisites

- Basic C++ knowledge (classes, pointers, STL containers)
- Understanding of basic mathematics (vectors, matrices, transformations)
- Familiarity with ECScope ECS basics (recommended: complete ECScope ECS tutorials first)
- Development environment with C++20 support and ECScope dependencies

## Tutorial Progression

### Tutorial 1: Basic Sprite Rendering
**File**: `01_basic_sprite_rendering.cpp`  
**Difficulty**: ⭐ Beginner  
**Duration**: 30-45 minutes

**What you'll learn:**
- Setting up a graphics window and rendering context
- Initializing ECScope's 2D renderer
- Creating cameras and understanding viewports
- Making entities with Transform and RenderableSprite components
- Implementing a basic game loop (update/render cycle)
- Understanding the rendering pipeline flow

**Key Concepts:**
- ECS architecture for graphics
- Component composition vs inheritance
- Separation of concerns in rendering
- Basic coordinate systems

**Prerequisites**: None (entry-level tutorial)

---

### Tutorial 2: Understanding Sprite Batching Performance
**File**: `02_batching_performance.cpp`  
**Difficulty**: ⭐⭐ Intermediate  
**Duration**: 45-60 minutes

**What you'll learn:**
- Why sprite batching is critical for performance
- How draw calls impact GPU performance
- Different batching strategies and their trade-offs
- Real-time performance analysis and metrics
- Visual debugging of batching efficiency
- Texture management for optimal batching

**Key Concepts:**
- GPU performance bottlenecks
- Draw call optimization
- Texture atlasing concepts
- Performance measurement and analysis
- Memory vs performance trade-offs

**Prerequisites**: Tutorial 1 completed

---

### Tutorial 3: Advanced Camera Systems
**File**: `03_advanced_cameras.cpp`  
**Difficulty**: ⭐⭐⭐ Advanced  
**Duration**: 60-90 minutes

**What you'll learn:**
- Multiple camera management and control
- Smooth camera following with interpolation
- Advanced camera movements (orbital, patrol, shake)
- Multi-viewport rendering (split-screen, picture-in-picture)
- Coordinate system transformations
- Camera-based culling and optimization

**Key Concepts:**
- 2D transformation matrices
- Interpolation and easing functions
- Viewport management
- Screen-space to world-space conversion
- Performance optimization with frustum culling

**Prerequisites**: Tutorials 1-2 completed, basic understanding of 2D transformations

---

## Getting Started

### 1. Environment Setup

Ensure you have ECScope properly built with graphics support:

```bash
# Build ECScope with graphics enabled
cmake -DECSCOPE_ENABLE_GRAPHICS=ON ..
make
```

**Required Dependencies:**
- SDL2 (for window management and input)
- OpenGL 3.3+ (for graphics rendering)
- C++20 compatible compiler (GCC 10+, Clang 10+, MSVC 2019+)

### 2. Building Tutorials

Each tutorial is a standalone executable. Build them individually:

```bash
# From the examples/rendering_tutorials directory
g++ -std=c++20 -I../../include -I../../src \
    01_basic_sprite_rendering.cpp \
    -lecscope -lSDL2 -lGL -lpthread \
    -o tutorial_01_basic_rendering
```

Or use the provided CMake integration (see main CMakeLists.txt).

### 3. Running Tutorials

```bash
./tutorial_01_basic_rendering
./tutorial_02_batching_performance --educational  # Enable detailed explanations
./tutorial_03_advanced_cameras
```

## Interactive Controls

Each tutorial includes interactive controls for hands-on learning:

### Universal Controls (All Tutorials)
- `ESC` or `Q` - Exit tutorial
- `R` - Reset to initial state
- `H` or `F1` - Show help/controls

### Tutorial-Specific Controls
See individual tutorial documentation or press `H` during execution.

## Educational Features

### 🎓 Progressive Learning
- Concepts build upon each other systematically
- Clear explanations with practical examples
- Real-time visual feedback and metrics
- Interactive parameter adjustment

### 📊 Performance Analysis
- Real-time performance metrics display
- Comparative analysis tools
- Bottleneck identification
- Optimization recommendations

### 🔬 Visual Debugging
- Debug rendering overlays
- Batch visualization modes
- Coordinate system displays
- Performance heat maps

### 💡 Educational Insights
- Contextual explanations during interaction
- Best practice recommendations
- Common pitfall warnings
- Professional development tips

## Common Issues and Solutions

### Graphics Not Working
```
Error: Graphics support not compiled
Solution: Rebuild ECScope with -DECSCOPE_ENABLE_GRAPHICS=ON
```

### Poor Performance
```
Issue: Low FPS or stuttering
Solutions:
- Run in Release mode (-O2 or -O3)
- Update graphics drivers
- Check GPU compatibility (OpenGL 3.3+ required)
- Reduce sprite count in performance tutorials
```

### SDL2 Not Found
```
Ubuntu/Debian: sudo apt-get install libsdl2-dev
CentOS/RHEL: sudo yum install SDL2-devel
macOS: brew install sdl2
Windows: Download SDL2 development libraries
```

## Advanced Usage

### Modifying Tutorials
The tutorials are designed to be educational and modifiable:

```cpp
// Example: Modify sprite count in Tutorial 2
void create_test_sprites() {
    sprite_count_ = 2000;  // Increase for stress testing
    // ... rest of function
}
```

### Adding Custom Features
Each tutorial provides extension points:

```cpp
// Example: Add custom camera movement in Tutorial 3
void update_custom_camera(f32 delta_time) {
    // Implement your camera logic here
    // This demonstrates extension points
}
```

### Performance Profiling
Use built-in performance tools:

```cpp
// Enable detailed performance tracking
auto config = renderer::Renderer2DConfig::educational_mode();
config.debug.collect_detailed_stats = true;
config.debug.enable_performance_overlay = true;
```

## Integration with ECScope Systems

### Memory System Integration
```cpp
// Monitor memory usage during rendering
auto memory_usage = renderer_->get_memory_usage();
std::cout << "GPU Memory: " << memory_usage.total << " bytes\n";
```

### Physics Integration
```cpp
// Render physics debug information
#ifdef ECSCOPE_HAS_PHYSICS
renderer_->set_debug_rendering_enabled(true);
physics_world.render_debug(*renderer_);
#endif
```

### UI Integration
```cpp
// Add rendering controls to UI
auto rendering_panel = std::make_unique<ui::panels::PanelRenderingDebug>();
ui_overlay_->add_panel("Rendering", std::move(rendering_panel));
```

## Contributing to Tutorials

### Adding New Tutorials
1. Follow the naming convention: `NN_descriptive_name.cpp`
2. Include comprehensive documentation and comments
3. Provide interactive controls and educational features
4. Test on multiple platforms and configurations
5. Update this README with tutorial information

### Improving Existing Tutorials
- Add more interactive features
- Enhance educational explanations
- Improve visual demonstrations
- Optimize performance
- Fix bugs or clarity issues

## Additional Resources

### ECScope Documentation
- [Core ECS Documentation](../../docs/ecs.md)
- [Rendering System Architecture](../../docs/rendering.md)
- [Memory Management Guide](../../docs/memory.md)

### External Resources
- [OpenGL Tutorial Series](https://learnopengl.com/)
- [2D Game Mathematics](https://www.redblobgames.com/)
- [Game Engine Architecture](http://gameenginebook.com/)

### Community and Support
- [ECScope GitHub Issues](https://github.com/ecscope/issues)
- [Educational Graphics Programming Discord](https://discord.gg/graphics-edu)
- [2D Rendering Best Practices Wiki](https://wiki.ecscope.dev/rendering2d)

---

## Tutorial Completion Checklist

Track your progress through the rendering tutorial series:

- [ ] **Tutorial 1**: Basic Sprite Rendering
  - [ ] Successfully create a graphics window
  - [ ] Render multiple colored sprites
  - [ ] Understand the basic render loop
  - [ ] Explain ECS rendering approach

- [ ] **Tutorial 2**: Sprite Batching Performance
  - [ ] Experience performance differences between batching modes
  - [ ] Analyze real-time performance metrics
  - [ ] Understand draw call optimization
  - [ ] Identify batching bottlenecks

- [ ] **Tutorial 3**: Advanced Camera Systems
  - [ ] Implement smooth camera following
  - [ ] Master different camera movement patterns
  - [ ] Understand coordinate transformations
  - [ ] Experience multi-camera rendering

### 🏆 Mastery Achievements

Complete these challenges to demonstrate mastery:

**Beginner Level:**
- Create a simple animated sprite scene
- Implement basic camera controls
- Achieve 60 FPS with 1000+ sprites

**Intermediate Level:**
- Build a texture atlas system
- Implement camera shake effects
- Create a performance profiling tool

**Advanced Level:**
- Design a custom batching strategy
- Implement multi-viewport rendering
- Build a 2D lighting system

---

**Happy Learning! 🚀**

Remember: The goal is not just to complete the tutorials, but to understand the underlying concepts and be able to apply them in your own projects. Take time to experiment with parameters, modify the code, and ask questions about how and why things work.

For questions, improvements, or additional tutorials, please contribute to the ECScope educational framework!