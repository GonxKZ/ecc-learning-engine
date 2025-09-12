# ECScope Comprehensive Rendering System UI

## Overview

The ECScope Rendering System UI is a professional-grade, real-time rendering pipeline control interface designed for the ECScope engine. It provides comprehensive tools for controlling, debugging, and optimizing all aspects of the modern deferred rendering pipeline.

## Features

### 🎨 Rendering Pipeline Control
- **Real-time parameter adjustment** for deferred rendering settings
- **Live preview** of changes without requiring restarts
- **G-buffer format configuration** with hardware optimization
- **Tiled/clustered deferred shading controls**
- **MSAA and quality settings** with automatic validation

### 🎭 PBR Material Editor
- **Interactive PBR material editing** with live preview
- **Material preset system** (Metal, Plastic, Wood, Ceramic, Glass, etc.)
- **Texture slot management** with hot-loading support
- **Material property visualization** with real-time feedback
- **Metallic/Roughness workflow** with industry-standard controls

### 🌟 Advanced Post-Processing Stack
- **HDR pipeline control** with multiple tone mapping operators
- **Bloom effect** with configurable threshold, intensity, and radius
- **Screen-Space Ambient Occlusion (SSAO)** with quality settings
- **Screen-Space Reflections (SSR)** with distance and quality controls
- **Temporal Anti-Aliasing (TAA)** with feedback and sharpening
- **Motion Blur** with configurable strength and sample count

### 🔆 Lighting System Control
- **Multi-light support** (Directional, Point, Spot, Area)
- **Shadow mapping controls** with cascade visualization
- **Light animation system** with configurable paths
- **Environment lighting** with IBL and ambient controls
- **Real-time light property editing** with immediate feedback

### 🔍 Visual Debugging Tools
- **G-buffer visualization** (Albedo, Normal, Depth, Material, Motion)
- **Render pass debugging** with step-through capability
- **Light complexity visualization** and heatmaps
- **Overdraw visualization** for performance optimization
- **Shadow cascade debugging** with color-coded visualization
- **Wireframe and bounds rendering** for debugging geometry

### 📊 Performance Profiling & Optimization
- **Real-time GPU profiling** with per-pass timing
- **Frame time analysis** with historical graphs
- **Memory usage monitoring** (GPU, Texture, Buffer)
- **Draw call analysis** with batching recommendations
- **Performance target tracking** (60fps, 30fps, etc.)
- **Bottleneck identification** and optimization suggestions

### 🎬 Scene Management Interface
- **3D viewport** with multiple camera control modes (Orbit, Fly, FPS, Inspect)
- **Scene hierarchy** with object selection and editing
- **Transform editing** with real-time manipulation
- **LOD system controls** and visualization
- **Object visibility** and shadow casting controls
- **Material assignment** per object

### ⚡ Shader Hot-Reload System
- **Automatic file monitoring** for shader changes
- **Hot-reload capability** without application restart
- **Shader error display** with detailed compilation messages
- **Reload status tracking** with visual indicators
- **Batch shader reloading** for workflow efficiency

### 🖥️ Professional Dashboard Integration
- **Modular panel system** with docking support
- **Workspace presets** for different development tasks
- **Feature registration** with the main dashboard
- **Configuration persistence** with preset management
- **Professional UI themes** (Dark, Light, High Contrast)

## Architecture

### Core Components

#### RenderingUI Class
The main class providing the comprehensive rendering interface:
- **Initialization & Lifecycle Management**
- **Real-time Configuration Updates**
- **Scene Management**
- **Performance Monitoring**
- **UI Panel Orchestration**

#### Configuration System
```cpp
struct LiveRenderingConfig {
    DeferredConfig deferred_config;      // Deferred rendering settings
    PostProcessConfig post_process;      // Post-processing pipeline
    ShadowConfig shadows;                // Shadow mapping settings
    EnvironmentConfig environment;       // Environment lighting
    QualityConfig quality;               // Quality and performance settings
};
```

#### Scene Management
```cpp
struct SceneObject {
    uint32_t id;
    std::string name;
    bool visible;
    MaterialProperties material;
    std::array<float, 16> transform;
    // LOD and rendering data
};

struct SceneLight {
    uint32_t id;
    std::string name;
    rendering::Light light_data;
    bool animated;
    // Animation and debug properties
};
```

#### Performance Monitoring
```cpp
struct RenderingPerformanceMetrics {
    float frame_time_ms;
    float gpu_time_ms;
    uint32_t draw_calls;
    uint64_t gpu_memory_used;
    // Detailed breakdown per pass
};
```

## Usage Examples

### Basic Initialization
```cpp
#include "ecscope/gui/rendering_ui.hpp"
#include "ecscope/rendering/renderer.hpp"
#include "ecscope/rendering/deferred_renderer.hpp"

// Create renderer
auto renderer = RendererFactory::create(RenderingAPI::Vulkan);

// Create deferred renderer
auto deferred_renderer = std::make_unique<DeferredRenderer>(renderer.get());
DeferredConfig config = optimize_g_buffer_format(renderer.get(), 1920, 1080);
deferred_renderer->initialize(config);

// Create rendering UI
auto rendering_ui = std::make_unique<RenderingUI>();
rendering_ui->initialize(renderer.get(), deferred_renderer.get());

// Main loop
while (running) {
    float delta_time = calculate_delta_time();
    
    rendering_ui->update(delta_time);
    rendering_ui->render();
}
```

### Scene Setup
```cpp
// Add objects to the scene
SceneObject cube;
cube.name = "Test Cube";
cube.material.albedo = {0.7f, 0.3f, 0.2f};
cube.material.metallic = 0.8f;
cube.material.roughness = 0.2f;
uint32_t cube_id = rendering_ui->add_scene_object(cube);

// Add lights
SceneLight main_light;
main_light.name = "Main Directional Light";
main_light.light_data.type = LightType::Directional;
main_light.light_data.intensity = 3.0f;
main_light.light_data.cast_shadows = true;
rendering_ui->add_scene_light(main_light);
```

### Configuration Management
```cpp
// Get current configuration
auto& config = rendering_ui->get_config();

// Modify settings
config.post_process.enable_bloom = true;
config.post_process.bloom_intensity = 1.2f;
config.shadows.shadow_resolution = 2048;

// Save configuration
rendering_ui->save_config("presets/high_quality.json");

// Load configuration
rendering_ui->load_config("presets/performance.json");
```

### Debug Visualization
```cpp
// Set debug visualization mode
rendering_ui->set_debug_mode(DebugVisualizationMode::GBufferAlbedo);

// Capture frame for analysis
rendering_ui->capture_frame();

// Get performance metrics
auto metrics = rendering_ui->get_metrics();
std::cout << "Frame time: " << metrics.frame_time_ms << "ms\n";
std::cout << "GPU memory: " << format_memory_size(metrics.gpu_memory_used) << "\n";
```

## Panel Overview

### 1. Pipeline Control Panel
- **Deferred rendering configuration**
- **G-buffer format selection**
- **Quality and performance settings**
- **Feature toggles** (SSR, TAA, Volumetrics)

### 2. Material Editor Panel
- **PBR material properties**
- **Texture slot management**
- **Material presets**
- **Live preview**

### 3. Lighting Control Panel
- **Light management**
- **Shadow configuration**
- **Environment settings**
- **Light animation**

### 4. Post-Processing Panel
- **HDR and tone mapping**
- **Bloom configuration**
- **SSAO settings**
- **SSR parameters**
- **TAA controls**

### 5. Debug Visualization Panel
- **G-buffer visualization**
- **Performance overlay**
- **Light debug rendering**
- **Debug mode selection**

### 6. Performance Profiler Panel
- **Frame timing graphs**
- **GPU profiling**
- **Memory usage charts**
- **Draw call analysis**

### 7. Scene Hierarchy Panel
- **Object tree view**
- **Transform editing**
- **Material assignment**
- **Visibility controls**

### 8. 3D Viewport Panel
- **Interactive 3D view**
- **Camera controls**
- **Navigation modes**
- **Real-time rendering**

### 9. Shader Editor Panel
- **Hot-reload controls**
- **Shader status**
- **Error display**
- **File monitoring**

## Integration Points

### Dashboard Integration
The rendering UI integrates seamlessly with the ECScope Dashboard system:
```cpp
// Register with dashboard
dashboard->initialize(renderer.get());
rendering_ui->initialize(renderer.get(), deferred_renderer.get(), dashboard.get());

// Features are automatically registered
rendering_ui->render(); // Renders within dashboard docking system
```

### Renderer Integration
Works with both Vulkan and OpenGL backends:
```cpp
// Automatic API detection and optimization
auto renderer = RendererFactory::create(RenderingAPI::Auto);
auto capabilities = renderer->get_capabilities();

// Hardware-specific optimizations
DeferredConfig config = optimize_g_buffer_format(renderer.get(), width, height);
```

## Performance Considerations

### Real-time Updates
- **Thread-safe parameter updates** prevent rendering pipeline stalls
- **Dirty flag system** minimizes unnecessary GPU state changes
- **Batched configuration updates** reduce API overhead

### Memory Management
- **GPU memory monitoring** with automatic alerts
- **Texture streaming visualization** for optimization
- **Resource leak detection** in debug builds

### Optimization Features
- **Automatic quality scaling** based on performance targets
- **LOD system integration** with distance-based switching
- **Culling visualization** for performance optimization

## Build Requirements

### Dependencies
- **ImGui** for immediate mode GUI rendering
- **ECScope Core** logging and utility systems
- **ECScope Rendering** backend integration
- **C++17** or later for modern language features

### Compilation Flags
```cmake
target_compile_definitions(rendering_ui PRIVATE
    ECSCOPE_HAS_IMGUI=1
    ECSCOPE_ENABLE_PROFILING=1
)
```

## Configuration Files

### Preset Format (JSON)
```json
{
    "version": "1.0",
    "deferred_config": {
        "width": 1920,
        "height": 1080,
        "msaa_samples": 4,
        "enable_screen_space_reflections": true,
        "enable_temporal_effects": true
    },
    "post_process": {
        "enable_hdr": true,
        "exposure": 1.0,
        "enable_bloom": true,
        "bloom_intensity": 0.8
    },
    "shadows": {
        "enable_shadows": true,
        "cascade_count": 4,
        "shadow_resolution": 2048
    }
}
```

## Future Enhancements

### Planned Features
- **Ray tracing controls** for RT-capable hardware
- **Variable rate shading** configuration
- **Mesh shader pipeline** support
- **GPU-driven rendering** controls
- **Neural network upscaling** integration

### Extensibility
- **Plugin system** for custom panels
- **Custom material editors** for specialized workflows
- **Scripting integration** for automated testing
- **Network synchronization** for collaborative editing

## Troubleshooting

### Common Issues

#### Performance Problems
- **Enable GPU profiling** to identify bottlenecks
- **Check memory usage** for excessive allocations
- **Review draw call count** for batching opportunities

#### Rendering Artifacts
- **Use G-buffer visualization** to debug material issues
- **Check shadow cascade distances** for shadow problems
- **Verify normal map orientation** in material editor

#### Configuration Issues
- **Validate hardware compatibility** with current settings
- **Reset to defaults** if configuration becomes corrupted
- **Check log output** for detailed error messages

## Examples and Demos

See the comprehensive demo at:
- `/examples/advanced/comprehensive_rendering_ui_demo.cpp`

This demo showcases:
- **Full pipeline setup** with optimized settings
- **Material showcase** with multiple PBR presets
- **Advanced lighting** with animated elements
- **Performance benchmarking** with automated testing
- **Debug visualization** cycling through all modes

## License

Part of the ECScope engine framework.
Copyright (c) 2024 ECScope Development Team.

---

For more information, see the ECScope engine documentation and API reference.