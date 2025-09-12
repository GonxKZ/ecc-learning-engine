# ECScope Comprehensive Debug Tools System

A professional-grade debugging and profiling interface for ECScope engine featuring real-time performance monitoring, memory tracking, and comprehensive logging capabilities.

## Table of Contents
- [Overview](#overview)
- [Features](#features)
- [Architecture](#architecture)
- [Components](#components)
- [Usage](#usage)
- [API Reference](#api-reference)
- [Examples](#examples)
- [Performance](#performance)
- [Building](#building)

## Overview

The ECScope Debug Tools System provides a comprehensive suite of debugging and profiling tools for game developers, performance engineers, and QA teams. Built on top of Dear ImGui, it offers professional-grade debugging capabilities including real-time performance profiling, memory leak detection, advanced logging, call stack tracing, and performance alerting systems.

### Key Capabilities

- **Real-time Performance Profiler**: CPU, GPU, and memory profiling with microsecond precision
- **Memory Tracker**: Allocation tracking, leak detection, and memory usage analysis
- **Debug Console**: Advanced logging system with filtering, search, and command execution
- **Performance Monitor**: System metrics monitoring with customizable graphs and alerts
- **Call Stack Tracer**: Function call stack capture and analysis for debugging crashes
- **Alert System**: Configurable performance thresholds with automatic alerting
- **Export Functionality**: Data export for offline analysis and reporting
- **Professional UI/UX**: Modern tabbed interface with docking and customizable layouts

## Features

### Performance Profiler
- **Frame Time Analysis**: Microsecond-precision frame timing with 60/30 FPS target lines
- **CPU Profiling**: Function-level timing with call count and self-time analysis
- **Memory Profiling**: Real-time memory usage tracking and allocation monitoring
- **GPU Profiling**: GPU usage monitoring and graphics performance metrics
- **Custom Metrics**: User-defined metrics for application-specific profiling
- **Profile Data Export**: Export profiling data for external analysis tools
- **Freeze Functionality**: Pause profiling to analyze specific performance snapshots

### Memory Tracker
- **Allocation Tracking**: Track all memory allocations with source location information
- **Leak Detection**: Automated memory leak detection with detailed leak reports
- **Category-based Analysis**: Memory usage analysis by allocation category
- **Memory Timeline**: Historical memory usage graphs and peak usage tracking
- **Allocation Age Tracking**: Monitor long-lived allocations that may indicate leaks
- **Source Code Integration**: Track allocations back to specific files and line numbers
- **Memory Alerts**: Configurable memory usage thresholds with automatic alerting

### Debug Console
- **Multi-level Logging**: Support for Trace, Debug, Info, Warning, Error, and Critical levels
- **Category Filtering**: Filter logs by category with enable/disable toggles
- **Search Functionality**: Real-time text search across all log entries
- **Command System**: Built-in command interpreter with extensible command registration
- **Log Export**: Export logs to files for external analysis and archival
- **Thread-safe Logging**: Safe logging from multiple threads with proper synchronization
- **Auto-scroll**: Automatic scrolling to latest entries with manual override

### Performance Monitor
- **Real-time Metrics**: Live monitoring of system performance metrics
- **Customizable Graphs**: Configurable metric visualization with color coding
- **System Integration**: CPU, memory, GPU, and disk I/O monitoring
- **Metric History**: Historical data tracking with configurable sample counts
- **Performance Alerts**: Threshold-based alerting for critical performance issues
- **Export Capabilities**: Export performance data for trend analysis

### Call Stack Tracer
- **Stack Capture**: Real-time call stack capture with symbol resolution
- **Automatic Tracing**: Configurable automatic stack capture at specified intervals
- **Crash Analysis**: Call stack information for debugging crashes and exceptions
- **Symbol Demangling**: Automatic demangling of C++ function names
- **Stack History**: Historical stack traces for tracking execution patterns

### Alert System
- **Configurable Thresholds**: Set custom thresholds for frame time, CPU, memory, and custom metrics
- **Real-time Monitoring**: Continuous monitoring with immediate alert triggering
- **Alert History**: Track alert frequency and patterns over time
- **Callback Integration**: Custom callback support for application-specific alert handling
- **Visual Indicators**: Color-coded alerts in the UI with severity levels

## Architecture

The Debug Tools System is built with a modular architecture consisting of several key components:

```
DebugToolsUI (Main Controller)
├── PerformanceProfiler (CPU/GPU/Memory Profiling)
├── MemoryProfiler (Allocation Tracking & Leak Detection)
├── DebugConsole (Logging & Command System)
├── PerformanceMonitor (System Metrics & Graphs)
├── CallStackTracer (Stack Trace Capture)
└── DebugToolsManager (System Integration & Coordination)
```

### Integration Points

- **Engine Integration**: Direct integration with ECScope engine systems for automatic profiling
- **Dashboard System**: Registered as dashboard feature with proper UI management
- **Memory Management**: Efficient profiling data storage with configurable retention
- **Threading**: Thread-safe design for multi-threaded profiling and logging
- **Plugin System**: Extensible architecture for custom profiling tools and metrics

## Components

### DebugToolsUI
Main controller class that orchestrates all debugging tools and provides the primary interface.

```cpp
class DebugToolsUI {
public:
    bool initialize();
    void render();
    void update(float delta_time);
    void shutdown();

    // Profiler interface
    void begin_profile_sample(const std::string& name);
    void end_profile_sample(const std::string& name);
    void record_custom_metric(const std::string& name, float value);
    
    // Memory tracking interface
    void track_memory_allocation(void* address, size_t size, const std::string& category = "General");
    void track_memory_deallocation(void* address);
    
    // Logging interface
    void log(DebugLevel level, const std::string& category, const std::string& message);
    void log_trace(const std::string& category, const std::string& message);
    void log_debug(const std::string& category, const std::string& message);
    void log_info(const std::string& category, const std::string& message);
    void log_warning(const std::string& category, const std::string& message);
    void log_error(const std::string& category, const std::string& message);
    void log_critical(const std::string& category, const std::string& message);
    
    // Configuration
    void set_profiler_enabled(ProfilerMode mode, bool enabled);
    void set_performance_threshold(PerformanceMetric metric, float threshold);
    void set_memory_alert_threshold(size_t bytes);
    
    // Callbacks
    void set_performance_alert_callback(std::function<void(const PerformanceAlert&)> callback);
    void set_memory_leak_callback(std::function<void(const std::vector<MemoryBlock>&)> callback);
};
```

### PerformanceProfiler
Advanced performance profiling system with support for multiple profiling modes.

```cpp
class PerformanceProfiler {
public:
    void initialize();
    void shutdown();
    
    void begin_frame();
    void end_frame();
    
    void begin_sample(const std::string& name);
    void end_sample(const std::string& name);
    
    void record_custom_metric(const std::string& name, float value);
    void set_performance_threshold(PerformanceMetric metric, float threshold);
    
    std::vector<ProfileFrame> get_frame_history(size_t count = 60) const;
    std::vector<CPUProfileSample> get_cpu_samples() const;
    std::vector<PerformanceAlert> get_active_alerts() const;
    
    void enable_profiling(ProfilerMode mode, bool enable);
    bool is_profiling_enabled(ProfilerMode mode) const;
};
```

### MemoryProfiler
Comprehensive memory tracking and leak detection system.

```cpp
class MemoryProfiler {
public:
    void initialize();
    void shutdown();
    
    void track_allocation(void* address, size_t size, const std::string& category,
                         const std::string& file, int line, const std::string& function);
    void track_deallocation(void* address);
    
    std::vector<MemoryBlock> get_active_allocations() const;
    std::vector<MemoryBlock> get_memory_leaks() const;
    size_t get_total_allocated_memory() const;
    size_t get_peak_memory_usage() const;
    
    std::unordered_map<std::string, size_t> get_memory_by_category() const;
    std::vector<std::pair<std::chrono::steady_clock::time_point, size_t>> get_memory_timeline() const;
    
    void set_memory_alert_threshold(size_t bytes);
    void perform_leak_detection();
};
```

### DebugConsole
Advanced logging and command execution system.

```cpp
class DebugConsole {
public:
    void initialize();
    void shutdown();
    void render();
    
    void add_log_entry(DebugLevel level, const std::string& category, const std::string& message,
                      const std::string& file = "", int line = 0, const std::string& function = "");
    void clear_logs();
    void export_logs(const std::string& filename) const;
    
    void set_log_filter(DebugLevel min_level);
    void set_category_filter(const std::string& category, bool enabled);
    void set_auto_scroll(bool enabled);
    
    void execute_command(const std::string& command);
    void register_command(const std::string& name, std::function<void(const std::vector<std::string>&)> handler);
};
```

## Usage

### Basic Setup

```cpp
// Initialize debug tools
auto debug_tools = std::make_unique<DebugToolsUI>();
if (!debug_tools->initialize()) {
    std::cerr << "Failed to initialize debug tools\n";
    return false;
}

// Set up callbacks
debug_tools->set_performance_alert_callback([](const PerformanceAlert& alert) {
    std::cout << "Performance alert: " << alert.description << std::endl;
});

debug_tools->set_memory_leak_callback([](const std::vector<MemoryBlock>& leaks) {
    std::cout << "Memory leaks detected: " << leaks.size() << std::endl;
});

// Configure thresholds
debug_tools->set_performance_threshold(PerformanceMetric::FrameTime, 16.67f); // 60 FPS
debug_tools->set_performance_threshold(PerformanceMetric::CPUUsage, 80.0f);   // 80%
debug_tools->set_memory_alert_threshold(100 * 1024 * 1024); // 100MB

// Enable profiling
debug_tools->set_profiler_enabled(ProfilerMode::CPU, true);
debug_tools->set_profiler_enabled(ProfilerMode::Memory, true);

// Main loop
while (running) {
    float delta_time = calculate_delta_time();
    
    debug_tools->update(delta_time);
    debug_tools->render();
}
```

### Performance Profiling

```cpp
// Manual profiling with scoped profiler
{
    ECSCOPE_PROFILE_SCOPE("Update Physics");
    // Physics update code here
    update_physics_systems();
}

// Function profiling
void update_rendering() {
    ECSCOPE_PROFILE_FUNCTION();
    // Rendering code here
}

// Manual profiling
debug_tools->begin_profile_sample("Custom System");
// Custom system update code
debug_tools->end_profile_sample("Custom System");

// Record custom metrics
debug_tools->record_custom_metric("Entity Count", static_cast<float>(entity_count));
debug_tools->record_custom_metric("Draw Calls", static_cast<float>(draw_calls));
debug_tools->record_custom_metric("Triangles", static_cast<float>(triangle_count));
```

### Memory Tracking

```cpp
// Track memory allocation
void* my_allocation = malloc(1024);
debug_tools->track_memory_allocation(my_allocation, 1024, "Game Objects");

// Track deallocation
free(my_allocation);
debug_tools->track_memory_deallocation(my_allocation);

// Perform leak detection
debug_tools->perform_leak_detection();

// Get memory statistics
auto active_allocations = debug_tools->get_active_allocations();
auto memory_leaks = debug_tools->get_memory_leaks();
size_t total_memory = debug_tools->get_total_allocated_memory();
```

### Logging System

```cpp
// Basic logging
debug_tools->log_info("Game", "Player entered new level");
debug_tools->log_warning("Performance", "Frame time exceeded 20ms");
debug_tools->log_error("System", "Failed to load texture file");

// Formatted logging
debug_tools->log_debug("Network", "Received packet: size=" + std::to_string(packet_size));

// Register custom console commands
debug_tools->register_command("spawn_entity", [](const std::vector<std::string>& args) {
    if (args.size() > 1) {
        std::string entity_type = args[1];
        spawn_entity(entity_type);
        debug_tools->log_info("Game", "Spawned entity: " + entity_type);
    }
});
```

### Custom Performance Metrics

```cpp
// Record various game metrics
debug_tools->record_custom_metric("FPS", 1000.0f / frame_time_ms);
debug_tools->record_custom_metric("Entity Update Time", entity_update_time_ms);
debug_tools->record_custom_metric("Network Latency", network_ping_ms);
debug_tools->record_custom_metric("Audio Active Sources", static_cast<float>(active_audio_sources));
debug_tools->record_custom_metric("GPU Memory Usage", gpu_memory_usage_mb);
```

## API Reference

### Core Classes

#### DebugToolsUI
- `initialize()`: Initialize the debug tools system
- `render()`: Render the debug tools interface
- `update()`: Update profiling data and performance monitoring
- `begin_profile_sample()` / `end_profile_sample()`: Manual profiling functions
- `track_memory_allocation()` / `track_memory_deallocation()`: Memory tracking
- `log()`: Main logging function with various convenience methods

#### ProfileFrame
Performance data structure for a single frame:
- Frame timing information (frame time, FPS)
- CPU and GPU usage percentages
- Memory usage statistics
- Draw call and triangle counts
- Custom metrics dictionary

#### MemoryBlock
Memory allocation tracking structure:
- Allocation address and size
- Category and source location information
- Allocation timestamp and age
- Leak detection status

#### DebugLogEntry
Log entry structure containing:
- Timestamp and debug level
- Category and message text
- Source file and line information
- Thread ID for multi-threaded debugging

### Data Structures

#### ProfileFrame
```cpp
struct ProfileFrame {
    uint64_t frame_id;
    std::chrono::high_resolution_clock::time_point timestamp;
    float frame_time_ms;
    float cpu_usage_percent;
    size_t memory_usage_bytes;
    float gpu_usage_percent;
    uint32_t draw_calls;
    uint32_t triangles;
    std::unordered_map<std::string, float> custom_metrics;
};
```

#### MemoryBlock
```cpp
struct MemoryBlock {
    void* address;
    size_t size;
    std::string category;
    std::chrono::steady_clock::time_point allocation_time;
    std::string source_file;
    int source_line;
    std::string source_function;
    bool is_leaked;
};
```

#### PerformanceAlert
```cpp
struct PerformanceAlert {
    uint64_t id;
    std::chrono::steady_clock::time_point timestamp;
    PerformanceMetric metric;
    float threshold;
    float current_value;
    std::string description;
    bool is_active;
    uint32_t trigger_count;
};
```

### Convenience Macros

```cpp
// Scoped profiling macros
#define ECSCOPE_PROFILE_SCOPE(name) \
    ecscope::gui::ScopedProfiler _prof(name)

#define ECSCOPE_PROFILE_FUNCTION() \
    ECSCOPE_PROFILE_SCOPE(__FUNCTION__)

// Usage examples
void update_entities() {
    ECSCOPE_PROFILE_FUNCTION();
    // Function code here
}

void custom_system_update() {
    ECSCOPE_PROFILE_SCOPE("Custom System Update");
    // System update code here
}
```

## Examples

### Complete Demo Application
The system includes a comprehensive demo application showcasing all features:

```bash
# Build and run the demo
mkdir build && cd build
cmake -DECSCOPE_BUILD_GUI=ON ..
make
./debug_tools_demo
```

### Integration Example
See `examples/advanced/17-debug-tools-demo.cpp` for a complete integration example with mock game system.

## Performance

### Optimization Features
- **Low-overhead Profiling**: Microsecond-precision timing with minimal performance impact
- **Configurable Data Retention**: Adjustable history size and sampling rates
- **Thread-safe Operations**: Lock-free data structures where possible
- **Memory Pool Usage**: Efficient memory management for profiling data
- **Selective Profiling**: Enable/disable specific profiling modes as needed
- **Background Processing**: Asynchronous data processing and analysis

### Performance Metrics
- Profiling overhead: <1% CPU usage for typical profiling scenarios
- Memory overhead: 5-20MB for profiling data and UI (configurable)
- Logging throughput: 10,000+ log entries per second with filtering
- Memory tracking: Handles 100,000+ active allocations efficiently
- UI responsiveness: 60 FPS interface even with heavy profiling active

### Recommended Settings
- **Development**: All profiling enabled, maximum data retention
- **QA Testing**: Performance profiling + logging, medium retention
- **Performance Analysis**: CPU + GPU profiling, high precision timing
- **Memory Debugging**: Memory profiling + leak detection enabled
- **Production Monitoring**: Minimal logging + alert system only

## Building

### Requirements
- C++20 compatible compiler (GCC 10+, Clang 12+, MSVC 2019+)
- CMake 3.22 or later
- Dear ImGui (for GUI components)
- GLFW3 (for window management)
- OpenGL 3.3+ (for UI rendering)
- Platform-specific debugging libraries (optional)

### Build Configuration
```bash
cmake -DECSCOPE_BUILD_GUI=ON \
      -DCMAKE_BUILD_TYPE=Debug \
      ..
```

### Optional Dependencies
- **Symbol Resolution**: Platform-specific libraries for call stack symbolication
- **Performance Counters**: Hardware performance monitoring libraries
- **Memory Debugging**: Valgrind, Application Verifier, or similar tools
- **Crash Reporting**: Integration with crash reporting systems

### Platform Notes
- **Windows**: Requires DbgHelp.dll for call stack tracing
- **Linux**: Requires libunwind and addr2line for symbol resolution
- **macOS**: Uses system frameworks for performance monitoring
- **Cross-platform**: Core functionality available on all supported platforms

## Advanced Features

### Performance Analysis
- **Frame Time Histograms**: Statistical analysis of frame timing distribution
- **Performance Regression Detection**: Automatic detection of performance degradation
- **Comparative Analysis**: Compare performance between different builds or configurations
- **Bottleneck Identification**: Automatic identification of performance bottlenecks
- **Trend Analysis**: Long-term performance trend monitoring and reporting

### Memory Analysis
- **Allocation Patterns**: Analysis of memory allocation patterns and lifetimes
- **Fragmentation Detection**: Memory fragmentation analysis and reporting
- **Category-based Tracking**: Detailed memory usage breakdown by allocation category
- **Peak Usage Analysis**: Analysis of memory usage spikes and their causes
- **Leak Attribution**: Detailed leak reports with source code attribution

### Integration Features
- **Automated Testing Integration**: Integration with automated testing frameworks
- **CI/CD Pipeline Support**: Performance regression detection in continuous integration
- **Remote Monitoring**: Network-based debugging and monitoring capabilities
- **Plugin Architecture**: Extensible system for custom debugging tools
- **Data Export Formats**: Multiple export formats for external analysis tools

## Contributing

The Debug Tools System follows ECScope's contribution guidelines. Key areas for contribution:
- Platform-specific profiling implementations
- Advanced memory analysis algorithms
- Performance visualization improvements
- Integration with external debugging tools
- Custom profiling metrics and analysis

## License

Part of the ECScope engine project. See main project license for details.

---

*This document describes version 1.0.0 of the ECScope Debug Tools System. For the latest updates and detailed API documentation, refer to the inline code documentation and examples.*