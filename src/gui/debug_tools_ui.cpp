#include "ecscope/gui/debug_tools_ui.hpp"
#include <algorithm>
#include <iomanip>
#include <sstream>
#include <cstring>
#include <fstream>
#include <regex>

#ifdef _WIN32
#include <windows.h>
#include <psapi.h>
#elif __linux__
#include <sys/resource.h>
#include <unistd.h>
#elif __APPLE__
#include <mach/mach.h>
#include <sys/types.h>
#include <sys/sysctl.h>
#endif

#ifdef ECSCOPE_HAS_IMGUI
#include <imgui.h>
#include <imgui_internal.h>
#endif

namespace ecscope {
namespace gui {

DebugToolsUI::DebugToolsUI()
    : show_window_(true)
    , show_profiler_(true)
    , show_memory_(true)
    , show_console_(true)
    , show_performance_monitor_(true)
    , show_call_stack_(false)
    , show_alerts_(true)
    , freeze_profiler_(false)
    , capture_screenshots_(false)
    , frame_delta_time_(0.016f) {
    
    splitter_sizes_[0] = 400.0f;
    splitter_sizes_[1] = 400.0f;
    splitter_sizes_[2] = 300.0f;
    
    export_path_ = "./debug_export/";
}

DebugToolsUI::~DebugToolsUI() {
    shutdown();
}

bool DebugToolsUI::initialize() {
#ifdef ECSCOPE_HAS_IMGUI
    profiler_ = std::make_unique<PerformanceProfiler>();
    profiler_->initialize();
    
    memory_profiler_ = std::make_unique<MemoryProfiler>();
    memory_profiler_->initialize();
    
    console_ = std::make_unique<DebugConsole>();
    console_->initialize();
    
    performance_monitor_ = std::make_unique<PerformanceMonitor>();
    performance_monitor_->initialize();
    
    call_stack_tracer_ = std::make_unique<CallStackTracer>();
    call_stack_tracer_->initialize();
    
    // Register default console commands
    console_->register_command("help", [this](const std::vector<std::string>& args) {
        console_->add_log_entry(DebugLevel::Info, "Console", "Available commands: help, clear, profiler, memory, export");
    });
    
    console_->register_command("clear", [this](const std::vector<std::string>& args) {
        console_->clear_logs();
    });
    
    console_->register_command("profiler", [this](const std::vector<std::string>& args) {
        if (args.size() > 1) {
            if (args[1] == "start") {
                profiler_->enable_profiling(ProfilerMode::CPU, true);
                console_->add_log_entry(DebugLevel::Info, "Profiler", "CPU profiling started");
            } else if (args[1] == "stop") {
                profiler_->enable_profiling(ProfilerMode::CPU, false);
                console_->add_log_entry(DebugLevel::Info, "Profiler", "CPU profiling stopped");
            }
        }
    });
    
    last_frame_time_ = std::chrono::high_resolution_clock::now();
    
    DebugToolsManager::instance().register_debug_tools_ui(this);
    return true;
#else
    return false;
#endif
}

void DebugToolsUI::render() {
#ifdef ECSCOPE_HAS_IMGUI
    if (!show_window_) return;

    ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar;
    
    if (ImGui::Begin("Debug Tools", &show_window_, window_flags)) {
        render_menu_bar();
        
        // Main layout with tabs
        if (ImGui::BeginTabBar("DebugToolsTabs")) {
            
            if (show_profiler_ && ImGui::BeginTabItem("Profiler")) {
                render_profiler_panel();
                ImGui::EndTabItem();
            }
            
            if (show_memory_ && ImGui::BeginTabItem("Memory")) {
                render_memory_panel();
                ImGui::EndTabItem();
            }
            
            if (show_console_ && ImGui::BeginTabItem("Console")) {
                render_console_panel();
                ImGui::EndTabItem();
            }
            
            if (show_performance_monitor_ && ImGui::BeginTabItem("Performance")) {
                render_performance_monitor_panel();
                ImGui::EndTabItem();
            }
            
            if (show_call_stack_ && ImGui::BeginTabItem("Call Stack")) {
                render_call_stack_panel();
                ImGui::EndTabItem();
            }
            
            if (show_alerts_ && ImGui::BeginTabItem("Alerts")) {
                render_alerts_panel();
                ImGui::EndTabItem();
            }
            
            ImGui::EndTabBar();
        }
    }
    ImGui::End();
#endif
}

void DebugToolsUI::render_menu_bar() {
#ifdef ECSCOPE_HAS_IMGUI
    if (ImGui::BeginMenuBar()) {
        if (ImGui::BeginMenu("View")) {
            ImGui::MenuItem("Profiler", nullptr, &show_profiler_);
            ImGui::MenuItem("Memory", nullptr, &show_memory_);
            ImGui::MenuItem("Console", nullptr, &show_console_);
            ImGui::MenuItem("Performance Monitor", nullptr, &show_performance_monitor_);
            ImGui::MenuItem("Call Stack", nullptr, &show_call_stack_);
            ImGui::MenuItem("Alerts", nullptr, &show_alerts_);
            ImGui::EndMenu();
        }
        
        if (ImGui::BeginMenu("Profiler")) {
            bool cpu_enabled = profiler_->is_profiling_enabled(ProfilerMode::CPU);
            bool memory_enabled = profiler_->is_profiling_enabled(ProfilerMode::Memory);
            bool gpu_enabled = profiler_->is_profiling_enabled(ProfilerMode::GPU);
            
            if (ImGui::MenuItem("CPU Profiling", nullptr, &cpu_enabled)) {
                profiler_->enable_profiling(ProfilerMode::CPU, cpu_enabled);
            }
            if (ImGui::MenuItem("Memory Profiling", nullptr, &memory_enabled)) {
                profiler_->enable_profiling(ProfilerMode::Memory, memory_enabled);
            }
            if (ImGui::MenuItem("GPU Profiling", nullptr, &gpu_enabled)) {
                profiler_->enable_profiling(ProfilerMode::GPU, gpu_enabled);
            }
            
            ImGui::Separator();
            ImGui::MenuItem("Freeze Profiler", nullptr, &freeze_profiler_);
            
            if (ImGui::MenuItem("Clear Profile Data")) {
                // Clear profiler data
            }
            ImGui::EndMenu();
        }
        
        if (ImGui::BeginMenu("Memory")) {
            if (ImGui::MenuItem("Perform Leak Detection")) {
                memory_profiler_->perform_leak_detection();
            }
            if (ImGui::MenuItem("Clear Memory Data")) {
                // Clear memory data
            }
            ImGui::EndMenu();
        }
        
        if (ImGui::BeginMenu("Export")) {
            if (ImGui::MenuItem("Export Profile Data")) {
                // Export profiler data
            }
            if (ImGui::MenuItem("Export Memory Report")) {
                // Export memory report
            }
            if (ImGui::MenuItem("Export Console Logs")) {
                console_->export_logs(export_path_ + "console_log.txt");
            }
            ImGui::EndMenu();
        }
        
        ImGui::EndMenuBar();
    }
#endif
}

void DebugToolsUI::render_profiler_panel() {
#ifdef ECSCOPE_HAS_IMGUI
    ImGui::Columns(2, "ProfilerColumns", true);
    
    // Frame time graph
    if (ImGui::CollapsingHeader("Frame Time", ImGuiTreeNodeFlags_DefaultOpen)) {
        render_frame_time_graph();
    }
    
    // CPU profile tree
    if (ImGui::CollapsingHeader("CPU Profile", ImGuiTreeNodeFlags_DefaultOpen)) {
        render_cpu_profile_tree();
    }
    
    ImGui::NextColumn();
    
    // Performance metrics
    if (ImGui::CollapsingHeader("Performance Metrics", ImGuiTreeNodeFlags_DefaultOpen)) {
        auto frame_history = profiler_->get_frame_history(60);
        
        if (!frame_history.empty()) {
            const auto& latest_frame = frame_history.back();
            
            ImGui::Text("Frame Time: %.2f ms", latest_frame.frame_time_ms);
            ImGui::Text("FPS: %.1f", 1000.0f / latest_frame.frame_time_ms);
            ImGui::Text("CPU Usage: %.1f%%", latest_frame.cpu_usage_percent);
            ImGui::Text("Memory Usage: %.2f MB", latest_frame.memory_usage_bytes / (1024.0f * 1024.0f));
            ImGui::Text("GPU Usage: %.1f%%", latest_frame.gpu_usage_percent);
            ImGui::Text("Draw Calls: %u", latest_frame.draw_calls);
            ImGui::Text("Triangles: %u", latest_frame.triangles);
            
            if (!latest_frame.custom_metrics.empty()) {
                ImGui::Separator();
                ImGui::Text("Custom Metrics:");
                for (const auto& [name, value] : latest_frame.custom_metrics) {
                    ImGui::Text("%s: %.2f", name.c_str(), value);
                }
            }
        }
    }
    
    // Profiler controls
    if (ImGui::CollapsingHeader("Controls", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Checkbox("Freeze Profiler", &freeze_profiler_);
        ImGui::Checkbox("Capture Screenshots", &capture_screenshots_);
        
        ImGui::Text("Export Path:");
        ImGui::InputText("##ExportPath", export_path_.data(), export_path_.capacity());
        
        if (ImGui::Button("Start All Profiling")) {
            profiler_->enable_profiling(ProfilerMode::CPU, true);
            profiler_->enable_profiling(ProfilerMode::Memory, true);
            profiler_->enable_profiling(ProfilerMode::GPU, true);
        }
        ImGui::SameLine();
        if (ImGui::Button("Stop All Profiling")) {
            profiler_->enable_profiling(ProfilerMode::CPU, false);
            profiler_->enable_profiling(ProfilerMode::Memory, false);
            profiler_->enable_profiling(ProfilerMode::GPU, false);
        }
    }
    
    ImGui::Columns(1);
#endif
}

void DebugToolsUI::render_memory_panel() {
#ifdef ECSCOPE_HAS_IMGUI
    ImGui::Columns(2, "MemoryColumns", true);
    
    // Memory usage graph
    if (ImGui::CollapsingHeader("Memory Usage", ImGuiTreeNodeFlags_DefaultOpen)) {
        render_memory_usage_graph();
    }
    
    // Memory allocations table
    if (ImGui::CollapsingHeader("Active Allocations", ImGuiTreeNodeFlags_DefaultOpen)) {
        render_memory_allocations_table();
    }
    
    ImGui::NextColumn();
    
    // Memory statistics
    if (ImGui::CollapsingHeader("Memory Statistics", ImGuiTreeNodeFlags_DefaultOpen)) {
        size_t total_allocated = memory_profiler_->get_total_allocated_memory();
        size_t peak_usage = memory_profiler_->get_peak_memory_usage();
        auto memory_by_category = memory_profiler_->get_memory_by_category();
        auto memory_leaks = memory_profiler_->get_memory_leaks();
        
        ImGui::Text("Total Allocated: %.2f MB", total_allocated / (1024.0f * 1024.0f));
        ImGui::Text("Peak Usage: %.2f MB", peak_usage / (1024.0f * 1024.0f));
        ImGui::Text("Memory Leaks: %zu", memory_leaks.size());
        
        if (!memory_leaks.empty()) {
            ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "WARNING: Memory leaks detected!");
        }
        
        ImGui::Separator();
        ImGui::Text("Memory by Category:");
        
        for (const auto& [category, bytes] : memory_by_category) {
            ImGui::Text("%s: %.2f KB", category.c_str(), bytes / 1024.0f);
        }
    }
    
    // Memory leak detection
    if (ImGui::CollapsingHeader("Leak Detection", ImGuiTreeNodeFlags_DefaultOpen)) {
        auto memory_leaks = memory_profiler_->get_memory_leaks();
        
        if (ImGui::Button("Detect Leaks")) {
            memory_profiler_->perform_leak_detection();
        }
        
        if (!memory_leaks.empty()) {
            ImGui::Text("Detected %zu memory leaks:", memory_leaks.size());
            
            if (ImGui::BeginTable("LeaksTable", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY)) {
                ImGui::TableSetupColumn("Address");
                ImGui::TableSetupColumn("Size");
                ImGui::TableSetupColumn("Category");
                ImGui::TableSetupColumn("Source");
                ImGui::TableHeadersRow();
                
                for (const auto& leak : memory_leaks) {
                    ImGui::TableNextRow();
                    ImGui::TableNextColumn();
                    ImGui::Text("%p", leak.address);
                    ImGui::TableNextColumn();
                    ImGui::Text("%zu bytes", leak.size);
                    ImGui::TableNextColumn();
                    ImGui::Text("%s", leak.category.c_str());
                    ImGui::TableNextColumn();
                    ImGui::Text("%s:%d", leak.source_file.c_str(), leak.source_line);
                }
                
                ImGui::EndTable();
            }
        } else {
            ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.3f, 1.0f), "No memory leaks detected");
        }
    }
    
    ImGui::Columns(1);
#endif
}

void DebugToolsUI::render_console_panel() {
#ifdef ECSCOPE_HAS_IMGUI
    console_->render();
#endif
}

void DebugToolsUI::render_performance_monitor_panel() {
#ifdef ECSCOPE_HAS_IMGUI
    performance_monitor_->render();
#endif
}

void DebugToolsUI::render_call_stack_panel() {
#ifdef ECSCOPE_HAS_IMGUI
    call_stack_tracer_->render();
#endif
}

void DebugToolsUI::render_alerts_panel() {
#ifdef ECSCOPE_HAS_IMGUI
    if (ImGui::CollapsingHeader("Performance Alerts", ImGuiTreeNodeFlags_DefaultOpen)) {
        auto alerts = profiler_->get_active_alerts();
        
        if (alerts.empty()) {
            ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.3f, 1.0f), "No active performance alerts");
        } else {
            ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "%zu active alerts:", alerts.size());
            
            if (ImGui::BeginTable("AlertsTable", 5, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY)) {
                ImGui::TableSetupColumn("Metric");
                ImGui::TableSetupColumn("Threshold");
                ImGui::TableSetupColumn("Current");
                ImGui::TableSetupColumn("Count");
                ImGui::TableSetupColumn("Description");
                ImGui::TableHeadersRow();
                
                for (const auto& alert : alerts) {
                    if (!alert.is_active) continue;
                    
                    ImGui::TableNextRow();
                    ImGui::TableNextColumn();
                    
                    const char* metric_names[] = {
                        "Frame Time", "CPU Usage", "Memory Usage", "GPU Usage",
                        "Draw Calls", "Triangles", "Network Latency", "Disk I/O", "Custom"
                    };
                    ImGui::Text("%s", metric_names[static_cast<int>(alert.metric)]);
                    
                    ImGui::TableNextColumn();
                    ImGui::Text("%.2f", alert.threshold);
                    
                    ImGui::TableNextColumn();
                    ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "%.2f", alert.current_value);
                    
                    ImGui::TableNextColumn();
                    ImGui::Text("%u", alert.trigger_count);
                    
                    ImGui::TableNextColumn();
                    ImGui::Text("%s", alert.description.c_str());
                }
                
                ImGui::EndTable();
            }
        }
    }
#endif
}

void DebugToolsUI::render_frame_time_graph() {
#ifdef ECSCOPE_HAS_IMGUI
    auto frame_history = profiler_->get_frame_history(120);
    
    if (!frame_history.empty()) {
        std::vector<float> frame_times;
        frame_times.reserve(frame_history.size());
        
        for (const auto& frame : frame_history) {
            frame_times.push_back(frame.frame_time_ms);
        }
        
        float min_time = *std::min_element(frame_times.begin(), frame_times.end());
        float max_time = *std::max_element(frame_times.begin(), frame_times.end());
        
        ImGui::Text("Frame Time (ms): Min=%.2f, Max=%.2f", min_time, max_time);
        
        ImGui::PlotLines("##FrameTime", frame_times.data(), static_cast<int>(frame_times.size()),
                        0, nullptr, 0.0f, max_time * 1.2f, ImVec2(0, 100));
        
        // Draw target frame time lines
        ImDrawList* draw_list = ImGui::GetWindowDrawList();
        ImVec2 canvas_pos = ImGui::GetCursorScreenPos();
        ImVec2 canvas_size = ImGui::GetContentRegionAvail();
        
        // 60 FPS line (16.67ms)
        float target_60fps = 16.67f;
        if (target_60fps < max_time * 1.2f) {
            float y_pos = canvas_pos.y - 100 + (1.0f - target_60fps / (max_time * 1.2f)) * 100;
            draw_list->AddLine(ImVec2(canvas_pos.x, y_pos), ImVec2(canvas_pos.x + canvas_size.x, y_pos),
                             IM_COL32(0, 255, 0, 128), 1.0f);
        }
        
        // 30 FPS line (33.33ms)
        float target_30fps = 33.33f;
        if (target_30fps < max_time * 1.2f) {
            float y_pos = canvas_pos.y - 100 + (1.0f - target_30fps / (max_time * 1.2f)) * 100;
            draw_list->AddLine(ImVec2(canvas_pos.x, y_pos), ImVec2(canvas_pos.x + canvas_size.x, y_pos),
                             IM_COL32(255, 255, 0, 128), 1.0f);
        }
    } else {
        ImGui::Text("No frame data available");
    }
#endif
}

void DebugToolsUI::render_memory_usage_graph() {
#ifdef ECSCOPE_HAS_IMGUI
    auto memory_timeline = memory_profiler_->get_memory_timeline();
    
    if (!memory_timeline.empty()) {
        std::vector<float> memory_values;
        memory_values.reserve(memory_timeline.size());
        
        for (const auto& [timestamp, bytes] : memory_timeline) {
            memory_values.push_back(static_cast<float>(bytes) / (1024.0f * 1024.0f)); // Convert to MB
        }
        
        float max_memory = *std::max_element(memory_values.begin(), memory_values.end());
        
        ImGui::Text("Memory Usage (MB): Peak=%.2f", max_memory);
        
        ImGui::PlotLines("##MemoryUsage", memory_values.data(), static_cast<int>(memory_values.size()),
                        0, nullptr, 0.0f, max_memory * 1.1f, ImVec2(0, 100));
    } else {
        ImGui::Text("No memory data available");
    }
#endif
}

void DebugToolsUI::render_cpu_profile_tree() {
#ifdef ECSCOPE_HAS_IMGUI
    auto cpu_samples = profiler_->get_cpu_samples();
    
    if (!cpu_samples.empty()) {
        if (ImGui::BeginTable("CPUProfileTable", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY)) {
            ImGui::TableSetupColumn("Function");
            ImGui::TableSetupColumn("Total Time (ms)");
            ImGui::TableSetupColumn("Self Time (ms)");
            ImGui::TableSetupColumn("Call Count");
            ImGui::TableHeadersRow();
            
            for (const auto& sample : cpu_samples) {
                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::Text("%s", sample.function_name.c_str());
                
                ImGui::TableNextColumn();
                ImGui::Text("%.3f", sample.execution_time_ms);
                
                ImGui::TableNextColumn();
                ImGui::Text("%.3f", sample.self_time_ms);
                
                ImGui::TableNextColumn();
                ImGui::Text("%u", sample.call_count);
            }
            
            ImGui::EndTable();
        }
    } else {
        ImGui::Text("No CPU profile data available");
        ImGui::Text("Enable CPU profiling to see function timing data");
    }
#endif
}

void DebugToolsUI::render_memory_allocations_table() {
#ifdef ECSCOPE_HAS_IMGUI
    auto allocations = memory_profiler_->get_active_allocations();
    
    if (!allocations.empty()) {
        ImGui::Text("Active allocations: %zu", allocations.size());
        
        if (ImGui::BeginTable("AllocationsTable", 5, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | 
                             ImGuiTableFlags_ScrollY, ImVec2(0, 200))) {
            ImGui::TableSetupColumn("Address");
            ImGui::TableSetupColumn("Size");
            ImGui::TableSetupColumn("Category");
            ImGui::TableSetupColumn("Age");
            ImGui::TableSetupColumn("Source");
            ImGui::TableHeadersRow();
            
            auto now = std::chrono::steady_clock::now();
            
            for (const auto& alloc : allocations) {
                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::Text("%p", alloc.address);
                
                ImGui::TableNextColumn();
                ImGui::Text("%zu", alloc.size);
                
                ImGui::TableNextColumn();
                ImGui::Text("%s", alloc.category.c_str());
                
                ImGui::TableNextColumn();
                auto age = std::chrono::duration_cast<std::chrono::seconds>(now - alloc.allocation_time);
                ImGui::Text("%lds", age.count());
                
                ImGui::TableNextColumn();
                ImGui::Text("%s:%d", alloc.source_file.c_str(), alloc.source_line);
            }
            
            ImGui::EndTable();
        }
    } else {
        ImGui::Text("No active allocations tracked");
        ImGui::Text("Enable memory profiling to see allocation data");
    }
#endif
}

void DebugToolsUI::update(float delta_time) {
    frame_delta_time_ = delta_time;
    
    if (!freeze_profiler_) {
        update_profiler();
    }
    
    update_memory_tracker();
    check_performance_alerts();
    
    if (performance_monitor_) {
        performance_monitor_->update();
    }
}

void DebugToolsUI::update_profiler() {
    if (profiler_) {
        profiler_->begin_frame();
        
        // Record frame time
        auto current_time = std::chrono::high_resolution_clock::now();
        auto frame_duration = std::chrono::duration_cast<std::chrono::microseconds>(current_time - last_frame_time_);
        float frame_time_ms = frame_duration.count() / 1000.0f;
        last_frame_time_ = current_time;
        
        // Record custom metrics
        profiler_->record_custom_metric("Frame Delta", frame_delta_time_ * 1000.0f);
        
        profiler_->end_frame();
    }
}

void DebugToolsUI::update_memory_tracker() {
    if (memory_profiler_) {
        // Memory profiler updates happen automatically through allocation tracking
    }
}

void DebugToolsUI::check_performance_alerts() {
    if (profiler_) {
        auto alerts = profiler_->get_active_alerts();
        for (const auto& alert : alerts) {
            if (alert.is_active && performance_alert_callback_) {
                performance_alert_callback_(alert);
            }
        }
    }
    
    if (memory_profiler_) {
        auto leaks = memory_profiler_->get_memory_leaks();
        if (!leaks.empty() && memory_leak_callback_) {
            memory_leak_callback_(leaks);
        }
    }
}

void DebugToolsUI::begin_profile_sample(const std::string& name) {
    if (profiler_) {
        profiler_->begin_sample(name);
    }
}

void DebugToolsUI::end_profile_sample(const std::string& name) {
    if (profiler_) {
        profiler_->end_sample(name);
    }
}

void DebugToolsUI::record_custom_metric(const std::string& name, float value) {
    if (profiler_) {
        profiler_->record_custom_metric(name, value);
    }
    if (performance_monitor_) {
        performance_monitor_->add_metric(name, PerformanceMetric::Custom, value);
    }
}

void DebugToolsUI::track_memory_allocation(void* address, size_t size, const std::string& category) {
    if (memory_profiler_) {
        memory_profiler_->track_allocation(address, size, category, "", 0, "");
    }
}

void DebugToolsUI::track_memory_deallocation(void* address) {
    if (memory_profiler_) {
        memory_profiler_->track_deallocation(address);
    }
}

void DebugToolsUI::log(DebugLevel level, const std::string& category, const std::string& message) {
    if (console_) {
        console_->add_log_entry(level, category, message);
    }
}

void DebugToolsUI::log_trace(const std::string& category, const std::string& message) {
    log(DebugLevel::Trace, category, message);
}

void DebugToolsUI::log_debug(const std::string& category, const std::string& message) {
    log(DebugLevel::Debug, category, message);
}

void DebugToolsUI::log_info(const std::string& category, const std::string& message) {
    log(DebugLevel::Info, category, message);
}

void DebugToolsUI::log_warning(const std::string& category, const std::string& message) {
    log(DebugLevel::Warning, category, message);
}

void DebugToolsUI::log_error(const std::string& category, const std::string& message) {
    log(DebugLevel::Error, category, message);
}

void DebugToolsUI::log_critical(const std::string& category, const std::string& message) {
    log(DebugLevel::Critical, category, message);
}

void DebugToolsUI::shutdown() {
    DebugToolsManager::instance().unregister_debug_tools_ui(this);
}

// Placeholder implementations for component classes
void PerformanceProfiler::initialize() {}
void PerformanceProfiler::shutdown() {}
void PerformanceProfiler::begin_frame() {}
void PerformanceProfiler::end_frame() {}
void PerformanceProfiler::begin_sample(const std::string&) {}
void PerformanceProfiler::end_sample(const std::string&) {}
void PerformanceProfiler::record_custom_metric(const std::string&, float) {}
void PerformanceProfiler::set_performance_threshold(PerformanceMetric, float) {}
std::vector<ProfileFrame> PerformanceProfiler::get_frame_history(size_t) const { return {}; }
std::vector<CPUProfileSample> PerformanceProfiler::get_cpu_samples() const { return {}; }
std::vector<PerformanceAlert> PerformanceProfiler::get_active_alerts() const { return {}; }
void PerformanceProfiler::enable_profiling(ProfilerMode mode, bool enable) { profiling_modes_[mode] = enable; }
bool PerformanceProfiler::is_profiling_enabled(ProfilerMode mode) const { 
    auto it = profiling_modes_.find(mode);
    return it != profiling_modes_.end() ? it->second : false;
}
void PerformanceProfiler::update_performance_alerts() {}
void PerformanceProfiler::cleanup_old_data() {}

void MemoryProfiler::initialize() { total_allocated_ = 0; peak_memory_ = 0; alert_threshold_ = 100 * 1024 * 1024; }
void MemoryProfiler::shutdown() {}
void MemoryProfiler::track_allocation(void* address, size_t size, const std::string& category, const std::string& file, int line, const std::string& function) {
    std::lock_guard<std::mutex> lock(memory_mutex_);
    MemoryBlock block;
    block.address = address;
    block.size = size;
    block.category = category;
    block.allocation_time = std::chrono::steady_clock::now();
    block.source_file = file;
    block.source_line = line;
    block.source_function = function;
    block.is_leaked = false;
    
    active_blocks_[address] = block;
    total_allocated_ += size;
    peak_memory_ = std::max(peak_memory_, total_allocated_);
    update_memory_timeline();
}

void MemoryProfiler::track_deallocation(void* address) {
    std::lock_guard<std::mutex> lock(memory_mutex_);
    auto it = active_blocks_.find(address);
    if (it != active_blocks_.end()) {
        total_allocated_ -= it->second.size;
        active_blocks_.erase(it);
        update_memory_timeline();
    }
}

std::vector<MemoryBlock> MemoryProfiler::get_active_allocations() const {
    std::lock_guard<std::mutex> lock(memory_mutex_);
    std::vector<MemoryBlock> result;
    result.reserve(active_blocks_.size());
    for (const auto& [addr, block] : active_blocks_) {
        result.push_back(block);
    }
    return result;
}

std::vector<MemoryBlock> MemoryProfiler::get_memory_leaks() const {
    std::lock_guard<std::mutex> lock(memory_mutex_);
    return leaked_blocks_;
}

size_t MemoryProfiler::get_total_allocated_memory() const {
    std::lock_guard<std::mutex> lock(memory_mutex_);
    return total_allocated_;
}

size_t MemoryProfiler::get_peak_memory_usage() const {
    std::lock_guard<std::mutex> lock(memory_mutex_);
    return peak_memory_;
}

std::unordered_map<std::string, size_t> MemoryProfiler::get_memory_by_category() const {
    std::lock_guard<std::mutex> lock(memory_mutex_);
    std::unordered_map<std::string, size_t> result;
    for (const auto& [addr, block] : active_blocks_) {
        result[block.category] += block.size;
    }
    return result;
}

std::vector<std::pair<std::chrono::steady_clock::time_point, size_t>> MemoryProfiler::get_memory_timeline() const {
    std::lock_guard<std::mutex> lock(memory_mutex_);
    return memory_timeline_;
}

void MemoryProfiler::set_memory_alert_threshold(size_t bytes) {
    std::lock_guard<std::mutex> lock(memory_mutex_);
    alert_threshold_ = bytes;
}

void MemoryProfiler::perform_leak_detection() {
    std::lock_guard<std::mutex> lock(memory_mutex_);
    leaked_blocks_.clear();
    
    auto now = std::chrono::steady_clock::now();
    for (const auto& [addr, block] : active_blocks_) {
        auto age = std::chrono::duration_cast<std::chrono::minutes>(now - block.allocation_time);
        if (age.count() > 5) { // Consider blocks older than 5 minutes as potential leaks
            MemoryBlock leak = block;
            leak.is_leaked = true;
            leaked_blocks_.push_back(leak);
        }
    }
}

void MemoryProfiler::update_memory_timeline() {
    auto now = std::chrono::steady_clock::now();
    memory_timeline_.emplace_back(now, total_allocated_);
    
    // Keep only last 100 entries
    if (memory_timeline_.size() > 100) {
        memory_timeline_.erase(memory_timeline_.begin());
    }
}

void DebugConsole::initialize() {
    min_log_level_ = DebugLevel::Debug;
    auto_scroll_ = true;
    max_log_entries_ = 10000;
    next_log_id_ = 1;
    command_history_pos_ = -1;
    std::memset(command_buffer_, 0, sizeof(command_buffer_));
}

void DebugConsole::shutdown() {}

void DebugConsole::render() {
#ifdef ECSCOPE_HAS_IMGUI
    ImGui::Columns(2, "ConsoleColumns", true);
    
    render_filters();
    
    ImGui::NextColumn();
    
    render_log_entries();
    render_command_input();
    
    ImGui::Columns(1);
#endif
}

void DebugConsole::render_log_entries() {
#ifdef ECSCOPE_HAS_IMGUI
    if (ImGui::BeginChild("LogEntries", ImVec2(0, -30), true)) {
        std::lock_guard<std::mutex> lock(console_mutex_);
        
        ImGuiListClipper clipper;
        clipper.Begin(static_cast<int>(log_entries_.size()));
        
        while (clipper.Step()) {
            for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++) {
                const auto& entry = log_entries_[i];
                
                if (!should_show_entry(entry)) continue;
                
                ImVec4 level_color = get_level_color(entry.level);
                
                // Timestamp
                auto time_t = std::chrono::system_clock::to_time_t(entry.timestamp);
                auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(entry.timestamp.time_since_epoch()) % 1000;
                
                std::stringstream ss;
                ss << std::put_time(std::localtime(&time_t), "%H:%M:%S");
                ss << "." << std::setfill('0') << std::setw(3) << ms.count();
                
                ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "[%s]", ss.str().c_str());
                ImGui::SameLine();
                
                // Level
                const char* level_names[] = {"TRACE", "DEBUG", "INFO", "WARN", "ERROR", "CRITICAL"};
                ImGui::TextColored(level_color, "[%s]", level_names[static_cast<int>(entry.level)]);
                ImGui::SameLine();
                
                // Category
                if (!entry.category.empty()) {
                    ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.8f, 1.0f), "[%s]", entry.category.c_str());
                    ImGui::SameLine();
                }
                
                // Message
                ImGui::Text("%s", entry.message.c_str());
                
                // Source info on hover
                if (!entry.file.empty() && ImGui::IsItemHovered()) {
                    ImGui::SetTooltip("%s:%d in %s()", entry.file.c_str(), entry.line, entry.function.c_str());
                }
            }
        }
        
        if (auto_scroll_ && ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) {
            ImGui::SetScrollHereY(1.0f);
        }
    }
    ImGui::EndChild();
#endif
}

void DebugConsole::render_command_input() {
#ifdef ECSCOPE_HAS_IMGUI
    if (ImGui::InputText("Command", command_buffer_, sizeof(command_buffer_), 
                        ImGuiInputTextFlags_EnterReturnsTrue)) {
        std::string command(command_buffer_);
        if (!command.empty()) {
            execute_command(command);
            command_history_.push_back(command);
            std::memset(command_buffer_, 0, sizeof(command_buffer_));
            ImGui::SetKeyboardFocusHere(-1);
        }
    }
    
    ImGui::SameLine();
    if (ImGui::Button("Execute")) {
        std::string command(command_buffer_);
        if (!command.empty()) {
            execute_command(command);
            command_history_.push_back(command);
            std::memset(command_buffer_, 0, sizeof(command_buffer_));
        }
    }
    
    ImGui::SameLine();
    if (ImGui::Button("Clear")) {
        clear_logs();
    }
#endif
}

void DebugConsole::render_filters() {
#ifdef ECSCOPE_HAS_IMGUI
    if (ImGui::CollapsingHeader("Filters", ImGuiTreeNodeFlags_DefaultOpen)) {
        // Log level filter
        const char* level_names[] = {"Trace", "Debug", "Info", "Warning", "Error", "Critical"};
        int current_level = static_cast<int>(min_log_level_);
        if (ImGui::Combo("Min Level", &current_level, level_names, IM_ARRAYSIZE(level_names))) {
            min_log_level_ = static_cast<DebugLevel>(current_level);
        }
        
        ImGui::Checkbox("Auto Scroll", &auto_scroll_);
        
        ImGui::Separator();
        ImGui::Text("Categories:");
        
        std::lock_guard<std::mutex> lock(console_mutex_);
        std::set<std::string> unique_categories;
        for (const auto& entry : log_entries_) {
            if (!entry.category.empty()) {
                unique_categories.insert(entry.category);
            }
        }
        
        for (const auto& category : unique_categories) {
            bool enabled = category_filters_.find(category) == category_filters_.end() || category_filters_[category];
            if (ImGui::Checkbox(category.c_str(), &enabled)) {
                category_filters_[category] = enabled;
            }
        }
    }
#endif
}

void DebugConsole::add_log_entry(DebugLevel level, const std::string& category, const std::string& message,
                                const std::string& file, int line, const std::string& function) {
    std::lock_guard<std::mutex> lock(console_mutex_);
    
    DebugLogEntry entry;
    entry.id = next_log_id_++;
    entry.timestamp = std::chrono::system_clock::now();
    entry.level = level;
    entry.category = category;
    entry.message = message;
    entry.file = file;
    entry.line = line;
    entry.function = function;
    entry.thread_id = std::this_thread::get_id();
    
    log_entries_.push_back(entry);
    
    // Remove old entries if we exceed the limit
    if (log_entries_.size() > max_log_entries_) {
        log_entries_.erase(log_entries_.begin(), log_entries_.begin() + (log_entries_.size() - max_log_entries_));
    }
}

void DebugConsole::clear_logs() {
    std::lock_guard<std::mutex> lock(console_mutex_);
    log_entries_.clear();
}

void DebugConsole::export_logs(const std::string& filename) const {
    std::lock_guard<std::mutex> lock(console_mutex_);
    
    std::ofstream file(filename);
    if (!file.is_open()) return;
    
    const char* level_names[] = {"TRACE", "DEBUG", "INFO", "WARN", "ERROR", "CRITICAL"};
    
    for (const auto& entry : log_entries_) {
        auto time_t = std::chrono::system_clock::to_time_t(entry.timestamp);
        file << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
        file << " [" << level_names[static_cast<int>(entry.level)] << "]";
        if (!entry.category.empty()) {
            file << " [" << entry.category << "]";
        }
        file << " " << entry.message;
        if (!entry.file.empty()) {
            file << " (" << entry.file << ":" << entry.line << ")";
        }
        file << "\n";
    }
}

void DebugConsole::execute_command(const std::string& command) {
    auto args = parse_command(command);
    if (args.empty()) return;
    
    std::string cmd_name = args[0];
    auto it = commands_.find(cmd_name);
    if (it != commands_.end()) {
        it->second(args);
    } else {
        add_log_entry(DebugLevel::Error, "Console", "Unknown command: " + cmd_name);
    }
}

void DebugConsole::register_command(const std::string& name, std::function<void(const std::vector<std::string>&)> handler) {
    commands_[name] = handler;
}

bool DebugConsole::should_show_entry(const DebugLogEntry& entry) const {
    if (entry.level < min_log_level_) return false;
    
    if (!entry.category.empty()) {
        auto it = category_filters_.find(entry.category);
        if (it != category_filters_.end() && !it->second) {
            return false;
        }
    }
    
    return true;
}

ImVec4 DebugConsole::get_level_color(DebugLevel level) const {
    switch (level) {
        case DebugLevel::Trace: return ImVec4(0.6f, 0.6f, 0.6f, 1.0f);
        case DebugLevel::Debug: return ImVec4(0.8f, 0.8f, 0.8f, 1.0f);
        case DebugLevel::Info: return ImVec4(0.9f, 0.9f, 0.9f, 1.0f);
        case DebugLevel::Warning: return ImVec4(1.0f, 1.0f, 0.3f, 1.0f);
        case DebugLevel::Error: return ImVec4(1.0f, 0.3f, 0.3f, 1.0f);
        case DebugLevel::Critical: return ImVec4(1.0f, 0.1f, 0.1f, 1.0f);
        default: return ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
    }
}

std::vector<std::string> DebugConsole::parse_command(const std::string& command_line) const {
    std::vector<std::string> args;
    std::istringstream iss(command_line);
    std::string arg;
    
    while (iss >> arg) {
        args.push_back(arg);
    }
    
    return args;
}

// Placeholder implementations for other component classes
void PerformanceMonitor::initialize() { realtime_monitoring_ = true; update_frequency_ = 10.0f; last_update_time_ = 0.0f; max_samples_ = 100; }
void PerformanceMonitor::shutdown() {}
void PerformanceMonitor::render() {
#ifdef ECSCOPE_HAS_IMGUI
    render_metric_graphs();
    render_metric_table();
    render_metric_controls();
#endif
}
void PerformanceMonitor::update() { update_system_metrics(); }
void PerformanceMonitor::add_metric(const std::string& name, PerformanceMetric type, float value) {
    auto& metric = metrics_[name];
    metric.type = type;
    metric.values.push_back(value);
    if (metric.values.size() > max_samples_) {
        metric.values.erase(metric.values.begin());
    }
    if (metric.min_range == 0.0f && metric.max_range == 0.0f) {
        metric.min_range = 0.0f;
        metric.max_range = 100.0f;
    }
}
void PerformanceMonitor::set_metric_range(const std::string& name, float min_val, float max_val) {
    metrics_[name].min_range = min_val;
    metrics_[name].max_range = max_val;
}
void PerformanceMonitor::set_metric_color(const std::string& name, ImU32 color) {
    metrics_[name].color = color;
}
void PerformanceMonitor::render_metric_graphs() {
#ifdef ECSCOPE_HAS_IMGUI
    for (const auto& [name, metric] : metrics_) {
        if (!metric.values.empty()) {
            ImGui::PlotLines(name.c_str(), metric.values.data(), static_cast<int>(metric.values.size()),
                           0, nullptr, metric.min_range, metric.max_range, ImVec2(0, 60));
        }
    }
#endif
}
void PerformanceMonitor::render_metric_table() {
#ifdef ECSCOPE_HAS_IMGUI
    if (ImGui::BeginTable("MetricsTable", 3, ImGuiTableFlags_Borders)) {
        ImGui::TableSetupColumn("Metric");
        ImGui::TableSetupColumn("Current");
        ImGui::TableSetupColumn("Range");
        ImGui::TableHeadersRow();
        
        for (const auto& [name, metric] : metrics_) {
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::Text("%s", name.c_str());
            ImGui::TableNextColumn();
            if (!metric.values.empty()) {
                ImGui::Text("%.2f", metric.values.back());
            }
            ImGui::TableNextColumn();
            ImGui::Text("%.1f - %.1f", metric.min_range, metric.max_range);
        }
        
        ImGui::EndTable();
    }
#endif
}
void PerformanceMonitor::render_metric_controls() {
#ifdef ECSCOPE_HAS_IMGUI
    ImGui::Checkbox("Real-time Monitoring", &realtime_monitoring_);
    ImGui::SliderFloat("Update Frequency", &update_frequency_, 1.0f, 60.0f, "%.1f Hz");
#endif
}
void PerformanceMonitor::update_system_metrics() {}
float PerformanceMonitor::get_cpu_usage() { return 0.0f; }
size_t PerformanceMonitor::get_memory_usage() { return 0; }
float PerformanceMonitor::get_gpu_usage() { return 0.0f; }

void CallStackTracer::initialize() { auto_capture_enabled_ = false; capture_interval_ms_ = 100; }
void CallStackTracer::shutdown() {}
void CallStackTracer::render() {
#ifdef ECSCOPE_HAS_IMGUI
    ImGui::Checkbox("Auto Capture", &auto_capture_enabled_);
    if (ImGui::Button("Capture Now")) {
        capture_call_stack();
    }
    
    if (!call_stack_history_.empty()) {
        ImGui::Text("Call Stack History:");
        for (size_t i = 0; i < call_stack_history_.size(); ++i) {
            if (ImGui::TreeNode(("Stack " + std::to_string(i)).c_str())) {
                for (const auto& frame : call_stack_history_[i]) {
                    ImGui::Text("%s", frame.c_str());
                }
                ImGui::TreePop();
            }
        }
    }
#endif
}
void CallStackTracer::capture_call_stack() { /* Implementation would use platform-specific stack walking */ }
void CallStackTracer::enable_automatic_capture(bool enable, uint32_t interval_ms) {
    auto_capture_enabled_ = enable;
    capture_interval_ms_ = interval_ms;
}
std::vector<std::string> CallStackTracer::get_current_call_stack() const { return current_call_stack_; }
std::vector<std::vector<std::string>> CallStackTracer::get_call_stack_history(size_t count) const { return call_stack_history_; }
void CallStackTracer::perform_stack_walk() {}
std::string CallStackTracer::demangle_symbol(const std::string& symbol) const { return symbol; }

// DebugToolsManager implementation
void DebugToolsManager::initialize() {}

void DebugToolsManager::shutdown() {
    std::lock_guard<std::mutex> lock(ui_mutex_);
    registered_uis_.clear();
}

void DebugToolsManager::register_debug_tools_ui(DebugToolsUI* ui) {
    std::lock_guard<std::mutex> lock(ui_mutex_);
    registered_uis_.push_back(ui);
}

void DebugToolsManager::unregister_debug_tools_ui(DebugToolsUI* ui) {
    std::lock_guard<std::mutex> lock(ui_mutex_);
    registered_uis_.erase(
        std::remove(registered_uis_.begin(), registered_uis_.end(), ui),
        registered_uis_.end());
}

void DebugToolsManager::notify_performance_sample(const ProfileFrame& frame) {
    std::lock_guard<std::mutex> lock(ui_mutex_);
    for (auto* ui : registered_uis_) {
        // Notify UIs about performance sample
    }
}

void DebugToolsManager::notify_memory_allocation(const MemoryBlock& block) {
    std::lock_guard<std::mutex> lock(ui_mutex_);
    for (auto* ui : registered_uis_) {
        // Notify UIs about memory allocation
    }
}

void DebugToolsManager::notify_log_entry(const DebugLogEntry& entry) {
    std::lock_guard<std::mutex> lock(ui_mutex_);
    for (auto* ui : registered_uis_) {
        // Notify UIs about log entry
    }
}

void DebugToolsManager::update(float delta_time) {
    // Update manager state
}

// ScopedProfiler implementation
ScopedProfiler::ScopedProfiler(const std::string& name) 
    : name_(name) 
    , start_time_(std::chrono::high_resolution_clock::now()) {
    DebugToolsManager::instance(); // Ensure manager is initialized
}

ScopedProfiler::~ScopedProfiler() {
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time_);
    // Would record the profiling sample here
}

}} // namespace ecscope::gui