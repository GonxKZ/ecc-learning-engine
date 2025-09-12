# ECScope ECS Inspector

## Professional-Grade ECS Debugging and Development Tool

The ECScope ECS Inspector is a comprehensive debugging and development tool integrated with the main dashboard, providing real-time visualization, editing, and monitoring capabilities for the Entity-Component-System architecture.

## 🔥 Key Features

### 🎯 Entity Management Interface
- **Hierarchical entity tree view** with parent/child relationships
- **Real-time entity creation, deletion, and cloning**
- **Advanced search and filtering** by name, components, or tags
- **Bulk operations** for multiple entities
- **Multi-select support** with intuitive selection management

### 🔧 Component Visualization
- **Live component inspector** with real-time value updates
- **Type-safe component editing** with validation
- **Component addition/removal interface** with drag-and-drop support
- **Component templates and presets** for rapid development
- **Visual representation** of component data with custom editors

### 📊 System Monitoring
- **Real-time system execution profiling** with performance metrics
- **Interactive system dependency visualization** with graph layouts
- **Performance metrics per system** including timing and throughput
- **System enable/disable controls** for debugging
- **System execution order management** with dependency resolution

### 🧠 Advanced Features
- **Archetype visualization and analysis** with memory tracking
- **Memory usage tracking per archetype** with utilization charts
- **Query builder interface** for testing ECS queries
- **Entity lifetime tracking and debugging** with history
- **Component change history and rollback** with undo/redo functionality

### 🔗 Integration Requirements
- **Seamless integration** with existing ECS registry
- **Real-time updates** without performance impact
- **Thread-safe access** to ECS data
- **Dashboard integration** with workspace management
- **Extensible architecture** for custom component editors

## 🏗️ Architecture

### Core Components

#### `ECSInspector` Class
The main inspector class providing comprehensive debugging capabilities:

```cpp
class ECSInspector {
public:
    explicit ECSInspector(ecs::Registry* registry, const InspectorConfig& config);
    
    // Lifecycle
    bool initialize();
    void shutdown();
    void update(float delta_time);
    void render();
    
    // Dashboard integration
    void register_with_dashboard(Dashboard* dashboard);
    void render_as_dashboard_panel(const std::string& panel_name);
    
    // Entity management
    EntityID create_entity(const std::string& name = "", const std::string& tag = "");
    bool destroy_entities(const std::vector<EntityID>& entities);
    EntityID clone_entity(EntityID source, const std::string& name = "");
    
    // Component system
    template<typename T>
    void register_component_type(const std::string& name, const std::string& category = "General");
    
    // System monitoring
    void register_system(const SystemStats& system);
    void update_system_stats(const SystemID& system_id, 
                            std::chrono::duration<float, std::milli> execution_time, 
                            uint64_t entities_processed);
};
```

#### Advanced Widgets

**System Dependency Graph**
```cpp
class SystemDependencyGraph {
public:
    void update_graph(const SystemGraph& system_graph);
    void render_dependency_graph(const Rect& canvas_rect);
    void set_layout_algorithm(LayoutAlgorithm algorithm);
    std::optional<SystemID> get_selected_system() const;
};
```

**Performance Chart**
```cpp
class PerformanceChart {
public:
    void add_series(const std::string& name, const Vec4& color);
    void add_data_point(const std::string& series_name, float timestamp, float value);
    void render_chart(const Rect& chart_rect);
    ChartStats get_series_stats(const std::string& series_name) const;
};
```

**Memory Usage Widget**
```cpp
class MemoryUsageWidget {
public:
    void update_memory_info(const ecs::ECSMemoryStats& memory_stats);
    void render_memory_overview(const Rect& widget_rect);
    void render_memory_treemap(const Rect& widget_rect);
    void render_memory_timeline(const Rect& widget_rect);
};
```

## 🚀 Usage

### Basic Setup

```cpp
#include "ecscope/gui/ecs_inspector.hpp"
#include "ecscope/gui/dashboard.hpp"

// Create registry and inspector
ecs::Registry registry;
gui::Dashboard dashboard;
gui::ECSInspector inspector(&registry);

// Initialize systems
dashboard.initialize();
inspector.initialize();
inspector.register_with_dashboard(&dashboard);

// Register component types
inspector.register_component_type<Transform>("Transform", "Core");
inspector.register_component_type<Velocity>("Velocity", "Physics");
inspector.register_component_type<Health>("Health", "Gameplay");

// Main loop
while (running) {
    inspector.update(delta_time);
    dashboard.render();
    inspector.render();
}
```

### Component Registration

```cpp
// Register component metadata
ComponentMetadata transform_meta(std::type_index(typeid(Transform)), "Transform");
transform_meta.category = "Core";
transform_meta.size = sizeof(Transform);
transform_meta.is_editable = true;
inspector.register_component_metadata(transform_meta);

// Create custom component editor
auto transform_editor = std::make_unique<ComponentEditor>("Transform", sizeof(Transform));
transform_editor->register_property<float>("x", offsetof(Transform, x), "X Position");
transform_editor->register_property<float>("y", offsetof(Transform, y), "Y Position");
transform_editor->register_property<float>("z", offsetof(Transform, z), "Z Position");
```

### System Monitoring

```cpp
// Register system for monitoring
SystemStats movement_stats;
movement_stats.system_id = "movement_system";
movement_stats.name = "Movement System";
movement_stats.category = "Physics";
inspector.register_system(movement_stats);

// Update system statistics during execution
auto start_time = std::chrono::high_resolution_clock::now();
movement_system.update(registry, delta_time);
auto end_time = std::chrono::high_resolution_clock::now();

inspector.update_system_stats("movement_system", 
                             std::chrono::duration<float, std::milli>(end_time - start_time),
                             entities_processed);
```

### Configuration Options

```cpp
InspectorConfig config = InspectorConfig::create_debugging_focused();
config.entity_refresh_rate = 16.0f;      // 60 FPS
config.component_refresh_rate = 33.0f;   // 30 FPS
config.enable_undo_redo = true;
config.enable_component_validation = true;
config.max_entities_displayed = 10000;

ECSInspector inspector(&registry, config);
```

## 📊 Performance Characteristics

### Update Frequencies
- **Entity tracking**: 60 FPS (16ms)
- **Component updates**: 30 FPS (33ms) 
- **System monitoring**: 10 FPS (100ms)
- **Memory analysis**: 2 FPS (500ms)

### Memory Usage
- **Base overhead**: ~500KB for inspector infrastructure
- **Per entity**: ~64 bytes of tracking data
- **Per component type**: ~256 bytes for metadata
- **History entries**: ~128 bytes per change record

### Performance Limits
- **Maximum entities tracked**: 10,000 (configurable)
- **History entries**: 1,000 (configurable)
- **Update time budget**: 5ms per frame (configurable)

## 🎨 Visual Features

### Entity Hierarchy
- Tree view with expand/collapse
- Color-coded entity types
- Real-time filtering and search
- Drag-and-drop reordering
- Multi-selection support

### Component Editors
- Type-specific property editors
- Real-time validation feedback
- Undo/redo with history
- Template application
- Batch editing operations

### System Monitoring
- Interactive dependency graphs
- Real-time performance charts
- Memory usage visualization
- Execution timeline
- Bottleneck identification

### Performance Visualization
- Frame time charts
- System execution profiles
- Memory usage tracking
- Cache hit rate analysis
- Allocation pattern visualization

## 🔧 Customization

### Custom Component Editors

```cpp
class CustomTransformEditor : public ComponentPropertyEditor {
public:
    bool render_property(const std::string& property_name, void* data, bool& changed) override {
        Transform* transform = static_cast<Transform*>(data);
        
        // Custom 3D position widget
        Vec3 position(transform->x, transform->y, transform->z);
        if (render_vec3_widget("Position", &position)) {
            transform->x = position.x;
            transform->y = position.y;
            transform->z = position.z;
            changed = true;
        }
        
        return true;
    }
};
```

### Custom Visualization Widgets

```cpp
class EntityRelationshipGraph {
public:
    void render_graph(const Rect& canvas_rect) {
        // Custom graph rendering for entity relationships
        for (auto [parent, children] : entity_relationships) {
            render_relationship_edge(parent, children);
        }
    }
};
```

## 🔍 Debugging Features

### Entity Lifecycle Tracking
- Creation/destruction events
- Component addition/removal history
- Entity state transitions
- Parent/child relationship changes

### Performance Profiling
- System execution timing
- Memory allocation tracking
- Cache performance analysis
- Bottleneck identification

### Query Analysis
- Query execution time profiling
- Result set analysis
- Query optimization suggestions
- Cache hit/miss statistics

### Memory Analysis
- Per-archetype memory usage
- Allocation pattern analysis
- Memory leak detection
- Fragmentation monitoring

## 📝 Integration Examples

### Complete Demo Application
See `examples/advanced/ecs_inspector_demo.cpp` for a comprehensive demonstration including:

- Multi-system game simulation
- Real-time component editing
- Performance monitoring
- Memory analysis
- Dashboard integration

### Build Configuration
```cmake
# Enable ECS Inspector
set(ECSCOPE_BUILD_ECS_INSPECTOR ON)

# Link libraries
target_link_libraries(your_application 
    ecscope_ecs_inspector
    ecscope_gui
)
```

## 🎯 Best Practices

### Performance Considerations
- Use performance-focused config for production builds
- Limit entity display count for large worlds
- Disable real-time updates when not actively debugging
- Configure appropriate refresh rates for different data types

### Memory Management
- Monitor memory usage in the metrics panel
- Clean up history periodically
- Use component templates to reduce redundancy
- Profile memory allocations regularly

### Debugging Workflow
1. **Start with system monitoring** to identify performance bottlenecks
2. **Use entity search** to find specific problematic entities
3. **Examine component values** in real-time during execution
4. **Analyze archetype distribution** to optimize data layout
5. **Profile memory usage** to prevent leaks and fragmentation

## 🔮 Future Enhancements

### Planned Features
- **Visual scripting** for component behavior
- **Remote debugging** over network
- **Automated testing** integration
- **Performance regression detection**
- **Export/import** of inspector configurations

### Extensibility Points
- Custom widget registration system
- Plugin architecture for third-party tools
- Scripting API for automation
- Custom visualization backends
- Integration with external profilers

---

The ECScope ECS Inspector represents a professional-grade debugging solution that transforms ECS development from a black-box experience into a transparent, controllable, and optimizable process. Its comprehensive feature set and seamless integration make it an indispensable tool for any serious ECS-based application development.