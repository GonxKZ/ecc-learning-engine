# Visual ECS Inspector Integration Guide

## Overview

The Visual ECS Inspector provides comprehensive real-time visualization and analysis of your Entity-Component-System architecture. This guide shows how to integrate and use the inspector in your ECScope applications.

## Features

### 1. Real-time Archetype Visualizer
- Interactive graph showing entity archetypes and their relationships
- Component distribution visualization across archetypes  
- Dynamic updates as entities are created/destroyed/modified
- Force-directed layout with customizable positioning
- Visual indicators for "hot" archetypes with high activity

### 2. Visual System Profiler
- Real-time system execution timing and dependencies
- Bottleneck identification with visual heat maps
- System execution flow visualization
- Budget utilization tracking with over-budget warnings
- Interactive dependency graph with educational tooltips

### 3. Memory Usage Visualizer
- Real-time memory allocation patterns for components
- Visual representation of memory pools and their usage
- Cache performance visualization for different access patterns
- Integration with advanced memory debugging tools
- Memory pressure monitoring and fragmentation analysis

### 4. Interactive Entity Browser
- Hierarchical view of entities with their components
- Component value editing with real-time updates
- Entity relationships and dependency tracking
- Advanced search and filtering capabilities
- Archetype-based organization

### 5. Sparse Set Storage Analyzer
- Visual representation of dense and sparse arrays
- Cache locality analysis and performance metrics
- Access pattern tracking and optimization suggestions
- Memory efficiency calculations
- Educational content about sparse set internals

### 6. Performance Timeline
- Frame-by-frame performance tracking
- System execution timeline with bottleneck highlighting
- Memory usage trends over time
- Entity and archetype count evolution
- Exportable performance data

## Quick Start Integration

### 1. Basic Setup

```cpp
#include "ecscope/visual_ecs_inspector.hpp"
#include "ecscope/sparse_set_visualizer.hpp"
#include "ecscope/memory_tracker.hpp"

// Initialize memory tracking (recommended)
memory::TrackerConfig tracker_config = memory::TrackerConfig();
tracker_config.enable_tracking = true;
tracker_config.enable_access_tracking = true;
memory::MemoryTracker::initialize(tracker_config);

// Initialize sparse set analyzer
visualization::GlobalSparseSetAnalyzer::initialize();

// Create your ECS registry with memory tracking
ecs::AllocatorConfig ecs_config = ecs::AllocatorConfig::create_educational_focused();
auto registry = std::make_unique<ecs::Registry>(ecs_config, "My_Registry");
auto system_manager = std::make_unique<ecs::SystemManager>(registry.get());
```

### 2. Create and Configure Inspector

```cpp
// Create visual inspector
auto inspector = ui::create_visual_ecs_inspector();

// Configure what panels to show
inspector->show_archetype_graph(true);
inspector->show_system_profiler(true);
inspector->show_memory_visualizer(true);
inspector->show_entity_browser(true);
inspector->show_sparse_set_view(true);
inspector->show_performance_timeline(true);
inspector->show_educational_hints(true);

// Configure thresholds
inspector->set_hot_archetype_threshold(10.0);    // Entities/second
inspector->set_bottleneck_threshold(16.6);       // Milliseconds
inspector->set_update_frequency(10.0);           // Updates per second

// Add to UI overlay
ui::Overlay overlay;
overlay.add_panel(std::move(inspector));
```

### 3. Register Components for Sparse Set Analysis

```cpp
// Register component types for detailed analysis
auto& analyzer = visualization::GlobalSparseSetAnalyzer::instance();
analyzer.register_sparse_set("Transform", 1000);
analyzer.register_sparse_set("Velocity", 500);
analyzer.register_sparse_set("Renderable", 800);
analyzer.register_sparse_set("Health", 300);
```

### 4. Update Inspector in Main Loop

```cpp
void update_frame(f64 delta_time) {
    // Run your ECS systems
    system_manager->execute_frame(delta_time);
    
    // Update inspector with real data
    ui::visual_inspector_integration::update_from_registry(*inspector, *registry);
    ui::visual_inspector_integration::update_from_system_manager(*inspector, *system_manager);
    ui::visual_inspector_integration::update_from_memory_tracker(*inspector, 
        memory::MemoryTracker::instance());
    
    // Update sparse set analysis
    visualization::GlobalSparseSetAnalyzer::instance().analyze_all();
    
    // Update UI
    overlay.update(delta_time);
}
```

## Advanced Integration

### Custom System Integration

To get detailed system profiling, ensure your systems properly declare their dependencies:

```cpp
class MySystem : public ecs::UpdateSystem {
public:
    MySystem() : UpdateSystem("My System") {
        // Declare component access patterns
        reads<Transform>().writes<Velocity>();
        reads<Health>().writes<AI>();
        
        // Set performance budget
        set_time_budget(0.005); // 5ms budget
        
        // Declare system dependencies
        depends_on("Physics System");
        depends_on("Input System", false); // Soft dependency
    }
    
    void update(const ecs::SystemContext& context) override {
        // Your system logic here
        
        // Optional: Track component access for sparse set analysis
        context.registry().for_each<Transform, Velocity>([&](ecs::Entity entity, Transform& t, Velocity& v) {
            TRACK_SPARSE_SET_ACCESS("Transform", &t, sizeof(Transform), true,
                                   visualization::SparseSetAccessPattern::Sequential);
            TRACK_SPARSE_SET_ACCESS("Velocity", &v, sizeof(Velocity), true,
                                   visualization::SparseSetAccessPattern::Sequential);
            
            // Component logic...
        });
    }
};
```

### Memory Tracking Integration

For detailed memory analysis, integrate with the memory tracker in your allocators:

```cpp
// Example custom allocator integration
template<typename T>
T* my_allocate(usize count) {
    void* ptr = std::malloc(sizeof(T) * count);
    
    // Track allocation for visualization
    memory::tracker::track_alloc(ptr, sizeof(T) * count, sizeof(T) * count, 
                                alignof(T), memory::AllocationCategory::ECS_Components,
                                memory::AllocatorType::System_Malloc, "MyAllocator", 1);
    
    return static_cast<T*>(ptr);
}

void my_deallocate(void* ptr) {
    memory::tracker::track_dealloc(ptr, memory::AllocatorType::System_Malloc, 
                                  "MyAllocator", 1);
    std::free(ptr);
}
```

### Export and Analysis

The inspector provides comprehensive data export capabilities:

```cpp
// Export analysis data
inspector->export_archetype_data("archetype_analysis.json");
inspector->export_system_performance("system_performance.csv");
inspector->export_memory_analysis("memory_analysis.json");
inspector->export_performance_timeline("performance_timeline.csv");

// Export sparse set analysis
visualization::GlobalSparseSetAnalyzer::instance().export_analysis_report("sparse_sets.md");
```

## Educational Features

The inspector includes extensive educational content:

### Tooltips and Explanations
- Hover over any UI element for contextual explanations
- Press F1 for comprehensive help
- Educational overlays explain ECS concepts in real-time

### Key Concepts Demonstrated
- **Archetypes**: Groups of entities with identical component signatures
- **Sparse Sets**: O(1) component storage with cache-friendly iteration
- **Memory Locality**: How data layout affects performance
- **System Dependencies**: Execution ordering and parallelization
- **Cache Behavior**: Impact of access patterns on performance

### Performance Insights
- Bottleneck identification and optimization suggestions
- Memory usage patterns and efficiency analysis
- Cache hit/miss ratio visualization
- Component access pattern analysis

## Configuration Options

### Display Configuration
```cpp
// Toggle panels
inspector->show_archetype_graph(true);
inspector->show_system_profiler(true);
inspector->show_memory_visualizer(true);
inspector->show_entity_browser(true);
inspector->show_sparse_set_view(true);
inspector->show_performance_timeline(true);

// Configure thresholds
inspector->set_hot_archetype_threshold(15.0);     // Entities created per second
inspector->set_bottleneck_threshold(8.3);         // Milliseconds (120 FPS)
inspector->set_update_frequency(20.0);            // 20 Hz updates
```

### Analysis Configuration
```cpp
// Configure sparse set analyzer
auto& analyzer = visualization::GlobalSparseSetAnalyzer::instance();
analyzer.set_access_tracking(true);
analyzer.set_cache_analysis(true);
analyzer.set_analysis_frequency(5.0); // 5 Hz

// Configure memory tracker
memory::TrackerConfig config;
config.enable_tracking = true;
config.enable_call_stacks = false; // Expensive but detailed
config.enable_access_tracking = true;
config.enable_heat_mapping = true;
config.enable_leak_detection = true;
config.max_tracked_allocations = 10000;
config.sampling_rate = 1.0; // Track all allocations
memory::MemoryTracker::instance().set_config(config);
```

## Performance Considerations

### Update Frequency
- Default update frequency is 10 Hz (every 100ms)
- Higher frequencies provide more real-time data but consume more CPU
- Lower frequencies are suitable for long-running analysis

### Memory Tracking Overhead
- Full tracking: ~5% performance impact
- Minimal tracking: ~1% performance impact
- Can be disabled entirely in release builds

### Sparse Set Analysis
- Automatic analysis runs at configurable frequency (default 5 Hz)
- Access pattern tracking is lightweight
- Cache simulation is approximated for performance

### UI Performance
- Inspector uses efficient ImGui immediate mode rendering
- Large datasets are automatically sampled for visualization
- Heavy analysis is performed on background threads where possible

## Troubleshooting

### Common Issues

**Inspector shows no data:**
- Ensure update integration functions are called every frame
- Check that memory tracking is properly initialized
- Verify sparse set registration for component types

**Poor performance:**
- Reduce update frequency
- Disable expensive features like call stack capture
- Limit maximum tracked allocations

**Memory usage growing:**
- Enable automatic cleanup in memory tracker
- Regularly call `compact_memory()` on analyzers
- Check for memory leaks in tracked allocations

### Debug Output
The inspector logs important events:
```cpp
// Enable debug logging
core::log::set_level(core::log::Level::Debug);

// Check logs for integration status
LOG_DEBUG("Visual ECS Inspector integration status");
```

## Examples

See `examples/visual_ecs_inspector_demo.cpp` for a complete working example demonstrating all inspector features with a realistic ECS application.

## Summary

The Visual ECS Inspector provides unprecedented insight into ECS performance and architecture. By integrating it into your ECScope applications, you gain:

- Real-time visualization of entity relationships and memory usage
- Performance profiling with bottleneck identification  
- Educational insights into ECS concepts and optimization
- Comprehensive analysis and export capabilities
- Interactive tools for debugging and optimization

The inspector is designed to be both educational and practical, making it an invaluable tool for learning ECS concepts and optimizing real-world applications.