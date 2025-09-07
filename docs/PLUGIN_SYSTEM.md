# ECScope Plugin System Documentation

## Overview

ECScope features a comprehensive, production-ready plugin system that provides dynamic loading, hot-swapping, security sandboxing, and educational features. This system allows developers to extend ECScope's functionality through custom plugins while maintaining performance, security, and educational value.

## Table of Contents

- [Features](#features)
- [Architecture](#architecture)
- [Getting Started](#getting-started)
- [Plugin Development](#plugin-development)
- [Security & Sandboxing](#security--sandboxing)
- [Hot Reload System](#hot-reload-system)
- [ECS Integration](#ecs-integration)
- [Educational Features](#educational-features)
- [API Reference](#api-reference)
- [Examples](#examples)
- [Best Practices](#best-practices)
- [Troubleshooting](#troubleshooting)

## Features

### Core Plugin System
- **Dynamic Loading/Unloading**: Load plugins at runtime with cross-platform compatibility
- **Versioning & Dependencies**: Semantic versioning with automatic dependency resolution
- **Hot-Swapping**: Update plugins without engine restart while preserving state
- **Security Sandboxing**: Memory isolation, API access control, and resource limits
- **Performance Monitoring**: Real-time profiling and performance analysis

### Development Tools
- **Plugin SDK**: Complete development kit with templates and debugging tools
- **Testing Framework**: Comprehensive unit, integration, and performance testing
- **Documentation Generator**: Automatic documentation generation from code
- **Code Quality Analysis**: Static analysis and best practices enforcement

### Educational Features
- **Learning Tutorials**: Step-by-step plugin development guides
- **Debugging Visualization**: Real-time plugin state and performance visualization
- **Best Practices**: Built-in educational content and coding examples
- **Progressive Difficulty**: Beginner to advanced plugin examples

## Architecture

The plugin system consists of several interconnected components:

```
┌─────────────────────────────────────────────────────────────┐
│                     Plugin Manager                          │
│  ┌─────────────────┐ ┌─────────────────┐ ┌──────────────────┐│
│  │ Plugin Discovery│ │ Lifecycle Mgmt  │ │ Dependency Resolver││
│  └─────────────────┘ └─────────────────┘ └──────────────────┘│
└─────────────────────────────────────────────────────────────┘
┌─────────────────────────────────────────────────────────────┐
│                   Security Manager                          │
│  ┌─────────────────┐ ┌─────────────────┐ ┌──────────────────┐│
│  │ Sandboxing      │ │ Access Control  │ │ Resource Limits  ││
│  └─────────────────┘ └─────────────────┘ └──────────────────┘│
└─────────────────────────────────────────────────────────────┘
┌─────────────────────────────────────────────────────────────┐
│                   ECS Integration                           │
│  ┌─────────────────┐ ┌─────────────────┐ ┌──────────────────┐│
│  │ Component Bridge│ │ System Bridge   │ │ Event Bridge     ││
│  └─────────────────┘ └─────────────────┘ └──────────────────┘│
└─────────────────────────────────────────────────────────────┘
┌─────────────────────────────────────────────────────────────┐
│                   Dynamic Loader                           │
│  ┌─────────────────┐ ┌─────────────────┐ ┌──────────────────┐│
│  │ Cross-Platform  │ │ Symbol Resolver │ │ Hot Reload       ││
│  │ Library Loading │ │                 │ │ Support          ││
│  └─────────────────┘ └─────────────────┘ └──────────────────┘│
└─────────────────────────────────────────────────────────────┘
```

## Getting Started

### Building with Plugin Support

Enable the plugin system in CMake:

```bash
cmake -DECSCOPE_ENABLE_PLUGIN_SYSTEM=ON ..
make -j$(nproc)
```

### Basic Plugin Usage

```cpp
#include "plugin/plugin_manager.hpp"
#include "plugin/ecs_plugin_integration.hpp"

int main() {
    // Initialize ECS and Plugin System
    auto ecs_registry = std::make_unique<ecs::Registry>();
    auto plugin_manager = std::make_unique<PluginManager>();
    auto integration = std::make_unique<ECSPluginIntegrationManager>(
        *ecs_registry, *plugin_manager);
    
    // Initialize systems
    plugin_manager->initialize();
    integration->initialize();
    
    // Load plugins
    plugin_manager->discover_plugins();
    plugin_manager->load_all_plugins();
    
    // Main loop
    while (running) {
        integration->update(delta_time);
        plugin_manager->update_plugins(delta_time);
    }
    
    return 0;
}
```

## Plugin Development

### Creating a Basic Plugin

1. **Create Plugin Class**:

```cpp
#include "plugin/plugin_api.hpp"

class MyPlugin : public IPlugin {
    PluginMetadata metadata_;
    std::unique_ptr<PluginAPI> api_;

public:
    const PluginMetadata& get_metadata() const override {
        return metadata_;
    }
    
    bool initialize() override {
        // Register components, systems, etc.
        return true;
    }
    
    void shutdown() override {
        // Cleanup
    }
    
    void update(f64 delta_time) override {
        // Per-frame updates
    }
    
    void handle_event(const PluginEvent& event) override {
        // Handle events
    }
    
    // ... other required methods
};
```

2. **Export Plugin Functions**:

```cpp
extern "C" {
    PLUGIN_EXPORT IPlugin* create_plugin() {
        return new MyPlugin();
    }
    
    PLUGIN_EXPORT void destroy_plugin(IPlugin* plugin) {
        delete plugin;
    }
    
    PLUGIN_EXPORT const char* get_plugin_info() {
        return R"json({
            "name": "MyPlugin",
            "version": "1.0.0",
            "author": "Developer Name"
        })json";
    }
}
```

3. **Create Plugin Metadata** (myplugin.json):

```json
{
    "name": "MyPlugin",
    "display_name": "My Custom Plugin",
    "description": "Example custom plugin",
    "version": "1.0.0",
    "author": "Developer Name",
    "category": "Custom",
    "is_educational": false
}
```

### Using the Plugin SDK

The Plugin SDK provides tools to generate plugin scaffolding:

```cpp
#include "plugin/plugin_sdk.hpp"

sdk::PluginSDK sdk("./sdk");
sdk::TemplateConfig config;
config.type = sdk::PluginTemplateType::Component;
config.plugin_name = "MyCustomPlugin";
config.display_name = "My Custom Plugin";
config.author = "Your Name";

// Generate plugin project
sdk.create_plugin_project(config);

// Build plugin
sdk.build_plugin();

// Generate documentation
std::string docs = sdk.generate_documentation();
```

## Security & Sandboxing

### Security Configuration

```cpp
PluginSecurityContext security;
security.memory_limit = 64 * MB;
security.thread_limit = 2;
security.execution_timeout = std::chrono::milliseconds(1000);
security.enable_memory_protection = true;

// Grant specific permissions
security.grant_permission(PluginPermission::ECSAccess);
security.grant_permission(PluginPermission::FileSystemRead);

plugin_manager->set_plugin_security_context("MyPlugin", security);
```

### Security Features

- **Memory Isolation**: Each plugin gets its own memory arena
- **API Access Control**: Whitelist-based API function access
- **Resource Limits**: CPU time, memory usage, and thread limits
- **Execution Sandboxing**: Isolated execution environment
- **Signature Verification**: Optional code signing validation

## Hot Reload System

### Enabling Hot Reload

```cpp
// Enable for specific plugin
plugin_manager->enable_hot_reload("MyPlugin");

// Check for changes
auto changed = plugin_manager->check_for_plugin_changes();

// Perform hot reload
for (const auto& plugin_name : changed) {
    plugin_manager->hot_reload_plugin(plugin_name);
}
```

### State Preservation

The hot reload system automatically:
- Backs up plugin state before reload
- Preserves entity relationships
- Restores configuration settings
- Maintains component data integrity

## ECS Integration

### Registering Components

```cpp
// In plugin initialization
auto& ecs = api->get_ecs();

// Register custom component
ecs.register_component<MyComponent>("MyComponent", 
    "Custom component description", true);

// Create entities with plugin components
auto entity = ecs.create_entity<MyComponent>();
```

### Registering Systems

```cpp
// Register system function
ecs.register_system_function("MySystem", "MyPlugin",
    [](ecs::Registry& registry, f64 delta_time) {
        // System update logic
        registry.for_each<MyComponent>([](ecs::Entity entity, MyComponent& comp) {
            // Process component
        });
    });
```

### Event Handling

```cpp
// Subscribe to ECS events
api->get_events().subscribe<ComponentAddedEvent>([](const auto& event) {
    LOG_INFO("Component added to entity {}", event.entity);
});

// Publish custom events
MyCustomEvent event;
api->get_events().publish(event);
```

## Educational Features

### Learning Tutorials

The plugin system includes comprehensive tutorials:

```cpp
// Get available tutorials
auto tutorials = plugin_manager->get_available_tutorials();

// Generate specific tutorial
auto tutorial = tutorial_generator->create_beginner_tutorial();
std::string content = tutorial.generate_tutorial();
```

### Code Examples

Plugins can provide educational examples:

```cpp
// Add learning content
api->add_learning_note("Components should be data-focused");
api->add_code_example("Basic Component", R"cpp(
struct HealthComponent {
    float health = 100.0f;
    float max_health = 100.0f;
};
)cpp");
```

### Performance Profiling

```cpp
// Profile plugin operations
{
    auto timer = api->start_timer("expensive_operation");
    // ... expensive operation
} // Automatically recorded

// Get performance metrics
auto metrics = api->get_performance_metrics();
```

## API Reference

### Core Classes

- **`PluginManager`**: Main plugin management class
- **`IPlugin`**: Base plugin interface
- **`PluginAPI`**: Main API for plugin development
- **`PluginContainer`**: Individual plugin wrapper
- **`ECSPluginIntegrationManager`**: ECS integration coordinator

### Key Methods

```cpp
// PluginManager
bool load_plugin(const std::string& plugin_name);
bool unload_plugin(const std::string& plugin_name);
std::vector<PluginDiscoveryResult> discover_plugins();
bool hot_reload_plugin(const std::string& plugin_name);

// PluginAPI
template<typename T> bool register_service(const std::string& name, std::unique_ptr<T> service);
template<typename T> T* get_service(const std::string& name);
void log_info(const std::string& format, Args&&... args);
```

## Examples

### Component Plugin

See `examples/plugins/basic_component_plugin.cpp` for a complete example showing:
- Component registration
- ECS integration
- Educational features
- Performance monitoring

### System Plugin

See `examples/plugins/advanced_system_plugin.cpp` for:
- Multi-threaded system implementation
- AI state machines
- Performance optimization
- Complex system interactions

### Plugin System Demo

Run the complete demonstration:

```bash
./plugin_system_demo
```

This shows all plugin system features in action.

## Best Practices

### Plugin Design

1. **Keep It Simple**: Focus on single responsibility
2. **Data-Oriented**: Design components for cache efficiency
3. **Thread Safety**: Use proper synchronization
4. **Error Handling**: Robust error handling and recovery
5. **Documentation**: Comprehensive documentation and examples

### Performance

1. **Memory Management**: Use provided memory allocators
2. **Update Frequency**: Not all systems need per-frame updates
3. **Batch Operations**: Process multiple entities efficiently
4. **Profile Early**: Use built-in profiling tools

### Security

1. **Minimal Permissions**: Request only necessary permissions
2. **Input Validation**: Validate all external inputs
3. **Resource Limits**: Respect memory and CPU limits
4. **Error Propagation**: Don't crash the engine

### Educational

1. **Clear Documentation**: Explain concepts thoroughly
2. **Progressive Examples**: From simple to complex
3. **Learning Objectives**: Clear educational goals
4. **Interactive Demos**: Hands-on learning experiences

## Troubleshooting

### Common Issues

1. **Plugin Won't Load**
   - Check plugin metadata syntax
   - Verify required dependencies
   - Review permission requirements

2. **Hot Reload Fails**
   - Ensure plugin supports hot reload
   - Check for circular dependencies
   - Verify state backup integrity

3. **Performance Issues**
   - Use profiling tools to identify bottlenecks
   - Review memory allocation patterns
   - Check system update frequencies

4. **Security Violations**
   - Review security context settings
   - Check API permission requirements
   - Monitor resource usage

### Debugging

1. **Enable Debug Logging**:
```cpp
plugin_manager->set_config({{"verbose_logging", "true"}});
```

2. **Use Plugin Debugger**:
```cpp
auto& debugger = plugin_sdk->get_debugger();
debugger.set_breakpoint("MyFunction");
debugger.start_tracing();
```

3. **Monitor Performance**:
```cpp
auto stats = plugin_manager->get_statistics();
LOG_INFO("Plugin performance: {}", stats.performance_score);
```

## Support

For additional help:
- Check the examples in `examples/plugins/`
- Review the comprehensive demo in `examples/plugin_system_demo.cpp`
- Consult the API reference in header files
- Use the educational tutorials provided by the SDK

The ECScope plugin system is designed to be both powerful and educational, providing everything needed to create sophisticated, secure, and high-performance plugins for the ECScope engine.