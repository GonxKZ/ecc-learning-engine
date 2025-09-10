#include <ecscope/debug/debug_system.hpp>
#include <ecscope/debug/console.hpp>
#include <ecscope/debug/debug_renderer.hpp>
#include <algorithm>
#include <chrono>

namespace ECScope::Debug {

DebugSystem::DebugSystem(const Config& config) : m_config(config) {
}

DebugSystem::~DebugSystem() {
    if (m_initialized) {
        Shutdown();
    }
}

void DebugSystem::Initialize() {
    if (m_initialized) {
        return;
    }

    InitializeComponents();
    
    // Start background update thread if needed
    if (m_config.enable_profiling) {
        m_should_stop = false;
        m_update_thread = std::thread([this]() {
            const auto target_frame_time = std::chrono::milliseconds(
                static_cast<int>(1000.0f / m_config.profiler_update_frequency)
            );
            
            while (!m_should_stop) {
                auto frame_start = std::chrono::high_resolution_clock::now();
                
                const float deltaTime = 1.0f / m_config.profiler_update_frequency;
                UpdateInternal(deltaTime);
                
                auto frame_end = std::chrono::high_resolution_clock::now();
                auto frame_duration = frame_end - frame_start;
                
                if (frame_duration < target_frame_time) {
                    std::this_thread::sleep_for(target_frame_time - frame_duration);
                }
            }
        });
    }
    
    m_initialized = true;
    
    // Emit initialization event
    EmitEvent("debug_system_initialized", &m_config);
}

void DebugSystem::Update(float deltaTime) {
    if (!m_enabled || m_paused) {
        return;
    }

    auto start_time = std::chrono::high_resolution_clock::now();
    
    // Update components
    if (m_console) {
        m_console->Update(deltaTime);
    }
    
    if (m_performance_monitor) {
        m_performance_monitor->Update(deltaTime);
    }
    
    // Update profilers
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        for (auto& [name, profiler] : m_profilers) {
            if (profiler && profiler->IsEnabled()) {
                profiler->Update(deltaTime);
            }
        }
    }
    
    // Update visualizers
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        for (auto& [name, visualizer] : m_visualizers) {
            if (visualizer && visualizer->IsEnabled()) {
                visualizer->Update(deltaTime);
            }
        }
    }
    
    // Update inspectors
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        for (auto& [name, inspector] : m_inspectors) {
            if (inspector && inspector->IsEnabled()) {
                inspector->Update(deltaTime);
            }
        }
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
    m_stats.update_time_ms = duration.count() / 1000.0;
    
    UpdateStats();
}

void DebugSystem::Render() {
    if (!m_enabled || !m_initialized) {
        return;
    }

    auto start_time = std::chrono::high_resolution_clock::now();
    
    RenderInternal();
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
    m_stats.render_time_ms = duration.count() / 1000.0;
}

void DebugSystem::Shutdown() {
    if (!m_initialized) {
        return;
    }

    // Stop background thread
    m_should_stop = true;
    if (m_update_thread.joinable()) {
        m_update_thread.join();
    }
    
    ShutdownComponents();
    
    // Clear all registries
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_profilers.clear();
        m_visualizers.clear();
        m_inspectors.clear();
        m_event_callbacks.clear();
    }
    
    m_initialized = false;
    
    // Emit shutdown event
    EmitEvent("debug_system_shutdown", nullptr);
}

std::shared_ptr<Profiler> DebugSystem::GetProfiler(const std::string& name) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_profilers.find(name);
    return (it != m_profilers.end()) ? it->second : nullptr;
}

void DebugSystem::RemoveProfiler(const std::string& name) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_profilers.erase(name);
}

void DebugSystem::ClearAllProfilers() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_profilers.clear();
}

std::shared_ptr<Visualizer> DebugSystem::GetVisualizer(const std::string& name) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_visualizers.find(name);
    return (it != m_visualizers.end()) ? it->second : nullptr;
}

void DebugSystem::RemoveVisualizer(const std::string& name) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_visualizers.erase(name);
}

std::shared_ptr<Inspector> DebugSystem::GetInspector(const std::string& name) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_inspectors.find(name);
    return (it != m_inspectors.end()) ? it->second : nullptr;
}

void DebugSystem::RemoveInspector(const std::string& name) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_inspectors.erase(name);
}

void DebugSystem::RegisterEventCallback(const std::string& event, EventCallback callback) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_event_callbacks[event].push_back(std::move(callback));
}

void DebugSystem::UnregisterEventCallback(const std::string& event) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_event_callbacks.erase(event);
}

void DebugSystem::EmitEvent(const std::string& event, const void* data) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_event_callbacks.find(event);
    if (it != m_event_callbacks.end()) {
        for (const auto& callback : it->second) {
            callback(event, data);
        }
    }
    m_stats.total_events_processed++;
}

void DebugSystem::ResetStats() {
    m_stats = Stats{};
}

void DebugSystem::UpdateConfig(const Config& config) {
    m_config = config;
    
    // Apply configuration changes
    if (m_console) {
        Console::ConsoleConfig console_config;
        console_config.enable_remote_access = config.enable_remote_debugging;
        console_config.remote_port = config.remote_debug_port;
        console_config.remote_bind_address = config.remote_debug_address;
        m_console->UpdateConfig(console_config);
    }
}

void DebugSystem::UpdateInternal(float deltaTime) {
    if (!m_enabled || m_paused) {
        return;
    }

    // This runs in the background thread for continuous profiling
    std::lock_guard<std::mutex> lock(m_mutex);
    
    for (auto& [name, profiler] : m_profilers) {
        if (profiler && profiler->IsEnabled()) {
            profiler->Update(deltaTime);
        }
    }
}

void DebugSystem::RenderInternal() {
    // Begin debug renderer frame
    if (m_renderer) {
        m_renderer->BeginFrame();
    }
    
    // Render visualizers
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        for (auto& [name, visualizer] : m_visualizers) {
            if (visualizer && visualizer->IsEnabled()) {
                visualizer->Render();
            }
        }
    }
    
    // Render inspectors
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        for (auto& [name, inspector] : m_inspectors) {
            if (inspector && inspector->IsEnabled()) {
                inspector->Render();
            }
        }
    }
    
    // Render console
    if (m_console) {
        m_console->Render();
    }
    
    // End debug renderer frame
    if (m_renderer) {
        m_renderer->EndFrame();
    }
}

void DebugSystem::UpdateStats() {
    // Count active components
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        
        m_stats.total_profilers = m_profilers.size();
        m_stats.active_profilers = std::count_if(m_profilers.begin(), m_profilers.end(),
            [](const auto& pair) { return pair.second && pair.second->IsEnabled(); });
        
        m_stats.total_visualizers = m_visualizers.size();
        m_stats.active_visualizers = std::count_if(m_visualizers.begin(), m_visualizers.end(),
            [](const auto& pair) { return pair.second && pair.second->IsEnabled(); });
        
        m_stats.total_inspectors = m_inspectors.size();
        m_stats.active_inspectors = std::count_if(m_inspectors.begin(), m_inspectors.end(),
            [](const auto& pair) { return pair.second && pair.second->IsEnabled(); });
    }
    
    // Estimate memory usage (simplified)
    m_stats.memory_usage_bytes = 
        sizeof(DebugSystem) +
        m_stats.total_profilers * 1024 +  // Estimate per profiler
        m_stats.total_visualizers * 2048 + // Estimate per visualizer
        m_stats.total_inspectors * 1024;   // Estimate per inspector
}

void DebugSystem::InitializeComponents() {
    // Initialize console
    if (m_config.enable_console) {
        Console::ConsoleConfig console_config;
        console_config.enable_remote_access = m_config.enable_remote_debugging;
        console_config.remote_port = m_config.remote_debug_port;
        console_config.remote_bind_address = m_config.remote_debug_address;
        
        m_console = std::make_unique<Console>(console_config);
        m_console->Initialize();
    }
    
    // Initialize debug renderer
    m_renderer = std::make_unique<DebugRenderer>();
    m_renderer->Initialize();
    
    // Initialize performance monitor
    m_performance_monitor = std::make_unique<PerformanceMonitor>();
    
    // Initialize existing components
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        
        for (auto& [name, profiler] : m_profilers) {
            if (profiler) {
                profiler->Initialize();
            }
        }
        
        for (auto& [name, visualizer] : m_visualizers) {
            if (visualizer) {
                visualizer->Initialize();
            }
        }
        
        for (auto& [name, inspector] : m_inspectors) {
            if (inspector) {
                inspector->Initialize();
            }
        }
    }
}

void DebugSystem::ShutdownComponents() {
    if (m_console) {
        m_console->Shutdown();
        m_console.reset();
    }
    
    if (m_renderer) {
        m_renderer->Shutdown();
        m_renderer.reset();
    }
    
    m_performance_monitor.reset();
}

} // namespace ECScope::Debug