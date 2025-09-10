#pragma once

#include <memory>
#include <vector>
#include <unordered_map>
#include <string>
#include <functional>
#include <chrono>
#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>

namespace ECScope::Debug {

// Forward declarations
class Profiler;
class Visualizer;
class Inspector;
class Console;
class DebugRenderer;
class PerformanceMonitor;

/**
 * @brief Core debug system that orchestrates all debugging tools
 * 
 * This is the main entry point for all debugging functionality in ECScope.
 * It manages profilers, visualizers, inspectors, and the debug console.
 */
class DebugSystem {
public:
    struct Config {
        bool enable_profiling = true;
        bool enable_visualization = true;
        bool enable_inspection = true;
        bool enable_console = true;
        bool enable_remote_debugging = false;
        bool minimal_performance_impact = true;
        
        // Profiling configuration
        int max_profiler_samples = 10000;
        float profiler_update_frequency = 60.0f;
        
        // Memory configuration
        size_t debug_memory_budget = 64 * 1024 * 1024; // 64MB
        
        // Network configuration
        uint16_t remote_debug_port = 7777;
        std::string remote_debug_address = "127.0.0.1";
    };

    explicit DebugSystem(const Config& config = {});
    ~DebugSystem();

    // Core lifecycle
    void Initialize();
    void Update(float deltaTime);
    void Render();
    void Shutdown();

    // Profiler management
    template<typename T, typename... Args>
    std::shared_ptr<T> CreateProfiler(const std::string& name, Args&&... args);
    
    std::shared_ptr<Profiler> GetProfiler(const std::string& name);
    void RemoveProfiler(const std::string& name);
    void ClearAllProfilers();

    // Visualizer management
    template<typename T, typename... Args>
    std::shared_ptr<T> CreateVisualizer(const std::string& name, Args&&... args);
    
    std::shared_ptr<Visualizer> GetVisualizer(const std::string& name);
    void RemoveVisualizer(const std::string& name);

    // Inspector management
    template<typename T, typename... Args>
    std::shared_ptr<T> CreateInspector(const std::string& name, Args&&... args);
    
    std::shared_ptr<Inspector> GetInspector(const std::string& name);
    void RemoveInspector(const std::string& name);

    // Console access
    Console& GetConsole() { return *m_console; }
    
    // Debug renderer access
    DebugRenderer& GetRenderer() { return *m_renderer; }
    
    // Performance monitoring
    PerformanceMonitor& GetPerformanceMonitor() { return *m_performance_monitor; }

    // Global debug state
    void SetEnabled(bool enabled) { m_enabled = enabled; }
    bool IsEnabled() const { return m_enabled; }
    
    void SetPaused(bool paused) { m_paused = paused; }
    bool IsPaused() const { return m_paused; }

    // Event system
    using EventCallback = std::function<void(const std::string& event, const void* data)>;
    void RegisterEventCallback(const std::string& event, EventCallback callback);
    void UnregisterEventCallback(const std::string& event);
    void EmitEvent(const std::string& event, const void* data = nullptr);

    // Statistics
    struct Stats {
        size_t total_profilers = 0;
        size_t active_profilers = 0;
        size_t total_visualizers = 0;
        size_t active_visualizers = 0;
        size_t total_inspectors = 0;
        size_t active_inspectors = 0;
        
        double update_time_ms = 0.0;
        double render_time_ms = 0.0;
        size_t memory_usage_bytes = 0;
        
        uint64_t total_events_processed = 0;
        uint64_t total_commands_executed = 0;
    };
    
    const Stats& GetStats() const { return m_stats; }
    void ResetStats();

    // Configuration
    const Config& GetConfig() const { return m_config; }
    void UpdateConfig(const Config& config);

private:
    Config m_config;
    bool m_enabled = true;
    bool m_paused = false;
    bool m_initialized = false;
    
    Stats m_stats;
    
    // Core components
    std::unique_ptr<Console> m_console;
    std::unique_ptr<DebugRenderer> m_renderer;
    std::unique_ptr<PerformanceMonitor> m_performance_monitor;
    
    // Component registries
    std::unordered_map<std::string, std::shared_ptr<Profiler>> m_profilers;
    std::unordered_map<std::string, std::shared_ptr<Visualizer>> m_visualizers;
    std::unordered_map<std::string, std::shared_ptr<Inspector>> m_inspectors;
    
    // Event system
    std::unordered_map<std::string, std::vector<EventCallback>> m_event_callbacks;
    
    // Threading
    std::mutex m_mutex;
    std::thread m_update_thread;
    std::atomic<bool> m_should_stop{false};
    std::condition_variable m_update_cv;
    
    // Internal methods
    void UpdateInternal(float deltaTime);
    void RenderInternal();
    void UpdateStats();
    void InitializeComponents();
    void ShutdownComponents();
};

/**
 * @brief Base class for all profilers
 */
class Profiler {
public:
    virtual ~Profiler() = default;
    
    virtual void Initialize() {}
    virtual void Update(float deltaTime) = 0;
    virtual void Reset() = 0;
    virtual void SetEnabled(bool enabled) { m_enabled = enabled; }
    virtual bool IsEnabled() const { return m_enabled; }
    
    const std::string& GetName() const { return m_name; }
    
protected:
    explicit Profiler(std::string name) : m_name(std::move(name)) {}
    
    std::string m_name;
    bool m_enabled = true;
};

/**
 * @brief Base class for all visualizers
 */
class Visualizer {
public:
    virtual ~Visualizer() = default;
    
    virtual void Initialize() {}
    virtual void Update(float deltaTime) = 0;
    virtual void Render() = 0;
    virtual void SetEnabled(bool enabled) { m_enabled = enabled; }
    virtual bool IsEnabled() const { return m_enabled; }
    
    const std::string& GetName() const { return m_name; }
    
protected:
    explicit Visualizer(std::string name) : m_name(std::move(name)) {}
    
    std::string m_name;
    bool m_enabled = true;
};

/**
 * @brief Base class for all inspectors
 */
class Inspector {
public:
    virtual ~Inspector() = default;
    
    virtual void Initialize() {}
    virtual void Update(float deltaTime) = 0;
    virtual void Render() = 0;
    virtual void SetEnabled(bool enabled) { m_enabled = enabled; }
    virtual bool IsEnabled() const { return m_enabled; }
    
    const std::string& GetName() const { return m_name; }
    
protected:
    explicit Inspector(std::string name) : m_name(std::move(name)) {}
    
    std::string m_name;
    bool m_enabled = true;
};

// Template implementations
template<typename T, typename... Args>
std::shared_ptr<T> DebugSystem::CreateProfiler(const std::string& name, Args&&... args) {
    static_assert(std::is_base_of_v<Profiler, T>, "T must derive from Profiler");
    
    std::lock_guard<std::mutex> lock(m_mutex);
    
    auto profiler = std::make_shared<T>(name, std::forward<Args>(args)...);
    m_profilers[name] = profiler;
    
    if (m_initialized) {
        profiler->Initialize();
    }
    
    return profiler;
}

template<typename T, typename... Args>
std::shared_ptr<T> DebugSystem::CreateVisualizer(const std::string& name, Args&&... args) {
    static_assert(std::is_base_of_v<Visualizer, T>, "T must derive from Visualizer");
    
    std::lock_guard<std::mutex> lock(m_mutex);
    
    auto visualizer = std::make_shared<T>(name, std::forward<Args>(args)...);
    m_visualizers[name] = visualizer;
    
    if (m_initialized) {
        visualizer->Initialize();
    }
    
    return visualizer;
}

template<typename T, typename... Args>
std::shared_ptr<T> DebugSystem::CreateInspector(const std::string& name, Args&&... args) {
    static_assert(std::is_base_of_v<Inspector, T>, "T must derive from Inspector");
    
    std::lock_guard<std::mutex> lock(m_mutex);
    
    auto inspector = std::make_shared<T>(name, std::forward<Args>(args)...);
    m_inspectors[name] = inspector;
    
    if (m_initialized) {
        inspector->Initialize();
    }
    
    return inspector;
}

} // namespace ECScope::Debug