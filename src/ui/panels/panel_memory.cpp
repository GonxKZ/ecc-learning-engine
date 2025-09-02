#include "panel_memory.hpp"
#include "ecs/registry.hpp"
#include "core/time.hpp"
#include "core/log.hpp"
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <cmath>

#ifdef ECSCOPE_HAS_GRAPHICS
#include "imgui.h"
#endif

namespace ecscope::ui {

MemoryObserverPanel::MemoryObserverPanel() 
    : Panel("Memory Observer", true) {
    
    // Initialize history with zeros
    memory_history_.fill(MemorySnapshot{});
}

void MemoryObserverPanel::render() {
#ifdef ECSCOPE_HAS_GRAPHICS
    if (!visible_) return;
    
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_None;
    if (ImGui::Begin(name_.c_str(), &visible_, window_flags)) {
        
        // Controls at top
        render_controls();
        ImGui::Separator();
        
        // Main content in tabs
        if (ImGui::BeginTabBar("MemoryTabs")) {
            
            // Current stats tab
            if (show_current_stats_ && ImGui::BeginTabItem("Current Stats")) {
                render_current_stats();
                ImGui::EndTabItem();
            }
            
            // Allocation graph tab
            if (show_allocation_graph_ && ImGui::BeginTabItem("Allocation Graph")) {
                render_allocation_graph();
                ImGui::EndTabItem();
            }
            
            // Allocator breakdown tab
            if (show_allocator_breakdown_ && ImGui::BeginTabItem("Allocator Breakdown")) {
                render_allocator_breakdown();
                ImGui::EndTabItem();
            }
            
            // Memory map tab (experimental)
            if (show_memory_map_ && ImGui::BeginTabItem("Memory Map")) {
                render_memory_map();
                ImGui::EndTabItem();
            }
            
            ImGui::EndTabBar();
        }
    }
    ImGui::End();
#endif
}

void MemoryObserverPanel::update(f64 delta_time) {
    last_update_time_ += delta_time;
    
    // Update memory snapshot at specified frequency
    f64 update_interval = 1.0 / static_cast<f64>(update_frequency_);
    if (last_update_time_ >= update_interval) {
        update_memory_snapshot();
        analyze_allocation_patterns();
        last_update_time_ = 0.0;
    }
}

void MemoryObserverPanel::record_allocation(usize size, const char* category) {
    (void)category; // TODO: Implement category tracking
    
    // This would be called by memory hooks
    memory_tracker::track_allocation(nullptr, size, category);
}

void MemoryObserverPanel::record_deallocation(usize size, const char* category) {
    (void)category; // TODO: Implement category tracking
    
    // This would be called by memory hooks  
    memory_tracker::track_deallocation(nullptr, size, category);
}

usize MemoryObserverPanel::current_memory_usage() const {
    if (history_count_ == 0) return 0;
    
    usize current_index = (history_head_ + HISTORY_SIZE - 1) % HISTORY_SIZE;
    return memory_history_[current_index].current_usage;
}

usize MemoryObserverPanel::peak_memory_usage() const {
    usize peak = 0;
    usize count = std::min(history_count_, HISTORY_SIZE);
    
    for (usize i = 0; i < count; ++i) {
        peak = std::max(peak, memory_history_[i].peak_usage);
    }
    
    return peak;
}

void MemoryObserverPanel::update_memory_snapshot() {
    MemorySnapshot snapshot = get_current_snapshot();
    
    // Add to circular buffer
    memory_history_[history_head_] = snapshot;
    history_head_ = (history_head_ + 1) % HISTORY_SIZE;
    if (history_count_ < HISTORY_SIZE) {
        history_count_++;
    }
}

void MemoryObserverPanel::render_current_stats() {
#ifdef ECSCOPE_HAS_GRAPHICS
    auto& registry = ecs::get_registry();
    
    // ECS Memory Stats
    ImGui::Text("ECS Memory Usage");
    ImGui::Separator();
    
    usize ecs_memory = registry.memory_usage();
    ImGui::Text("Registry: %s", format_memory_size(ecs_memory).c_str());
    ImGui::Text("Entities: %zu active / %zu total", 
                registry.active_entities(), 
                registry.total_entities_created());
    ImGui::Text("Archetypes: %zu", registry.archetype_count());
    
    // Memory efficiency metrics
    if (registry.active_entities() > 0) {
        f64 bytes_per_entity = static_cast<f64>(ecs_memory) / registry.active_entities();
        ImGui::Text("Avg per Entity: %.1f bytes", bytes_per_entity);
    }
    
    ImGui::Spacing();
    
    // System Memory Stats (estimated)
    ImGui::Text("System Memory (Estimated)");
    ImGui::Separator();
    
    usize current_usage = current_memory_usage();
    usize peak_usage = peak_memory_usage();
    
    ImGui::Text("Current Usage: %s", format_memory_size(current_usage).c_str());
    ImGui::Text("Peak Usage: %s", format_memory_size(peak_usage).c_str());
    
    if (peak_usage > 0) {
        f32 usage_fraction = static_cast<f32>(current_usage) / static_cast<f32>(peak_usage);
        ImGui::Text("Usage Ratio: %.1f%%", usage_fraction * 100.0f);
        
        // Progress bar
        imgui_utils::progress_bar_animated(usage_fraction, 
                                          format_memory_size(current_usage).c_str());
    }
    
    ImGui::Spacing();
    
    // Allocation Stats
    ImGui::Text("Allocation Statistics");
    ImGui::Separator();
    
    u32 alloc_count = memory_tracker::get_allocation_count();
    u32 free_count = memory_tracker::get_free_count();
    
    ImGui::Text("Allocations: %s", imgui_utils::format_number(alloc_count).c_str());
    ImGui::Text("Deallocations: %s", imgui_utils::format_number(free_count).c_str());
    
    if (alloc_count > 0) {
        ImGui::Text("Outstanding: %u", alloc_count - free_count);
        f32 free_ratio = static_cast<f32>(free_count) / static_cast<f32>(alloc_count);
        ImGui::Text("Free Ratio: %.1f%%", free_ratio * 100.0f);
    }
    
    ImGui::Text("Allocation Rate: %s/s", format_rate(average_allocation_rate_).c_str());
    ImGui::Text("Peak Rate: %s/s", format_rate(peak_allocation_rate_).c_str());
    
    // Memory health indicators
    ImGui::Spacing();
    ImGui::Text("Memory Health");
    ImGui::Separator();
    
    // Fragmentation score (0-100, lower is better)
    f32 frag_score = static_cast<f32>(fragmentation_score_) / 100.0f;
    ImVec4 frag_color = ImVec4(frag_score, 1.0f - frag_score, 0.0f, 1.0f);
    ImGui::TextColored(frag_color, "Fragmentation: %zu%%", fragmentation_score_);
    
    // Largest allocation
    ImGui::Text("Largest Allocation: %s", format_memory_size(largest_allocation_).c_str());
    
    // Memory leak detection
    bool has_leaks = memory_tracker::detect_leaks();
    if (has_leaks) {
        ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "  Memory leaks detected!");
        if (ImGui::Button("Dump Leak Report")) {
            memory_tracker::dump_allocations("memory_leaks.txt");
            LOG_INFO("Memory leak report saved to memory_leaks.txt");
        }
    } else {
        ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), " No leaks detected");
    }
#endif
}

void MemoryObserverPanel::render_allocation_graph() {
#ifdef ECSCOPE_HAS_GRAPHICS
    if (history_count_ == 0) {
        ImGui::TextDisabled("No memory data available");
        return;
    }
    
    // Prepare data for plotting
    std::vector<f32> current_usage_data;
    std::vector<f32> peak_usage_data;
    std::vector<f32> allocation_rate_data;
    
    current_usage_data.reserve(history_count_);
    peak_usage_data.reserve(history_count_);
    allocation_rate_data.reserve(history_count_);
    
    usize start_index = history_count_ < HISTORY_SIZE ? 0 : history_head_;
    for (usize i = 0; i < history_count_; ++i) {
        usize index = (start_index + i) % HISTORY_SIZE;
        const auto& snapshot = memory_history_[index];
        
        current_usage_data.push_back(static_cast<f32>(snapshot.current_usage) / (1024.0f * 1024.0f));
        peak_usage_data.push_back(static_cast<f32>(snapshot.peak_usage) / (1024.0f * 1024.0f));
        
        // Calculate allocation rate for this snapshot
        if (i > 0) {
            usize prev_index = (start_index + i - 1) % HISTORY_SIZE;
            const auto& prev_snapshot = memory_history_[prev_index];
            f64 dt = snapshot.timestamp - prev_snapshot.timestamp;
            if (dt > 0.0) {
                f64 rate = static_cast<f64>(snapshot.total_allocated - prev_snapshot.total_allocated) / dt;
                allocation_rate_data.push_back(static_cast<f32>(rate) / (1024.0f * 1024.0f));
            } else {
                allocation_rate_data.push_back(0.0f);
            }
        } else {
            allocation_rate_data.push_back(0.0f);
        }
    }
    
    // Determine scale
    f32 scale_max = manual_scale_max_;
    if (auto_scale_ && !current_usage_data.empty()) {
        auto max_it = std::max_element(peak_usage_data.begin(), peak_usage_data.end());
        scale_max = *max_it * 1.2f; // 20% headroom
        scale_max = std::max(scale_max, 1.0f); // Minimum 1MB
    }
    
    // Memory usage graph
    ImGui::Text("Memory Usage (MB)");
    if (ImGui::BeginChild("UsageGraph", ImVec2(0, 200))) {
        if (!current_usage_data.empty()) {
            ImGui::PlotLines("Current Usage", current_usage_data.data(), 
                           static_cast<int>(current_usage_data.size()), 0, nullptr, 
                           0.0f, scale_max, ImVec2(0, 150));
            
            ImGui::SameLine();
            ImGui::BeginGroup();
            ImGui::TextColored(ImVec4(colors_.ecs_memory[0], colors_.ecs_memory[1], 
                                     colors_.ecs_memory[2], colors_.ecs_memory[3]), 
                              "  Current");
            ImGui::TextColored(ImVec4(colors_.system_memory[0], colors_.system_memory[1], 
                                     colors_.system_memory[2], colors_.system_memory[3]), 
                              "  Peak");
            ImGui::EndGroup();
        }
    }
    ImGui::EndChild();
    
    // Allocation rate graph
    ImGui::Text("Allocation Rate (MB/s)");
    if (ImGui::BeginChild("RateGraph", ImVec2(0, 150))) {
        if (!allocation_rate_data.empty()) {
            f32 rate_max = *std::max_element(allocation_rate_data.begin(), allocation_rate_data.end()) * 1.1f;
            ImGui::PlotLines("Allocation Rate", allocation_rate_data.data(), 
                           static_cast<int>(allocation_rate_data.size()), 0, nullptr, 
                           0.0f, rate_max, ImVec2(0, 120));
        }
    }
    ImGui::EndChild();
    
    // Graph controls
    ImGui::Separator();
    if (ImGui::Checkbox("Auto Scale", &auto_scale_)) {
        // Auto scale changed
    }
    
    if (!auto_scale_) {
        ImGui::SameLine();
        ImGui::SetNextItemWidth(100);
        ImGui::DragFloat("Max (MB)", &manual_scale_max_, 1.0f, 1.0f, 1000.0f);
    }
    
    ImGui::SameLine();
    if (ImGui::Button("Clear History")) {
        history_head_ = 0;
        history_count_ = 0;
        memory_history_.fill(MemorySnapshot{});
    }
#endif
}

void MemoryObserverPanel::render_allocator_breakdown() {
#ifdef ECSCOPE_HAS_GRAPHICS
    // Get category breakdown from memory tracker
    auto categories = memory_tracker::get_category_breakdown();
    
    if (categories.empty()) {
        ImGui::TextDisabled("No allocator data available");
        ImGui::Text("Enable memory tracking for detailed breakdown");
        return;
    }
    
    // Calculate total for percentages
    usize total_memory = 0;
    for (const auto& [category, size] : categories) {
        total_memory += size;
    }
    
    // Render as table
    if (ImGui::BeginTable("AllocatorTable", 3, 
                         ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
        
        ImGui::TableSetupColumn("Category", ImGuiTableColumnFlags_WidthFixed, 150.0f);
        ImGui::TableSetupColumn("Size", ImGuiTableColumnFlags_WidthFixed, 100.0f);
        ImGui::TableSetupColumn("Percentage", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableHeadersRow();
        
        for (const auto& [category, size] : categories) {
            ImGui::TableNextRow();
            
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("%s", category.c_str());
            
            ImGui::TableSetColumnIndex(1);
            ImGui::Text("%s", format_memory_size(size).c_str());
            
            ImGui::TableSetColumnIndex(2);
            if (total_memory > 0) {
                f32 percentage = static_cast<f32>(size) / static_cast<f32>(total_memory);
                ImGui::ProgressBar(percentage, ImVec2(-1, 0), 
                                  (std::to_string(static_cast<int>(percentage * 100)) + "%").c_str());
            }
        }
        
        ImGui::EndTable();
    }
    
    // Pie chart visualization
    ImGui::Spacing();
    ImGui::Text("Memory Distribution");
    
    if (ImGui::BeginChild("PieChart", ImVec2(0, 300))) {
        // Simple pie chart using ImGui drawing primitives
        ImDrawList* draw_list = ImGui::GetWindowDrawList();
        ImVec2 canvas_p0 = ImGui::GetCursorScreenPos();
        ImVec2 canvas_sz = ImGui::GetContentRegionAvail();
        
        if (canvas_sz.x > 100 && canvas_sz.y > 100) {
            ImVec2 center = ImVec2(canvas_p0.x + canvas_sz.x * 0.5f, canvas_p0.y + canvas_sz.y * 0.5f);
            f32 radius = std::min(canvas_sz.x, canvas_sz.y) * 0.4f;
            
            f32 current_angle = -3.14159f * 0.5f; // Start at top
            const ImU32 colors[] = {
                IM_COL32(255, 100, 100, 255), // Red
                IM_COL32(100, 255, 100, 255), // Green  
                IM_COL32(100, 100, 255, 255), // Blue
                IM_COL32(255, 255, 100, 255), // Yellow
                IM_COL32(255, 100, 255, 255), // Magenta
                IM_COL32(100, 255, 255, 255), // Cyan
            };
            
            for (usize i = 0; i < categories.size() && i < 6; ++i) {
                const auto& [category, size] = categories[i];
                
                if (total_memory > 0) {
                    f32 percentage = static_cast<f32>(size) / static_cast<f32>(total_memory);
                    f32 angle_span = percentage * 2.0f * 3.14159f;
                    
                    // Draw pie slice
                    draw_list->PathArcTo(center, radius, current_angle, current_angle + angle_span, 32);
                    draw_list->PathLineTo(center);
                    draw_list->PathFillConvex(colors[i]);
                    
                    current_angle += angle_span;
                }
            }
            
            // Draw legend
            ImVec2 legend_pos = ImVec2(canvas_p0.x + 10, canvas_p0.y + 10);
            for (usize i = 0; i < categories.size() && i < 6; ++i) {
                const auto& [category, size] = categories[i];
                
                draw_list->AddRectFilled(
                    ImVec2(legend_pos.x, legend_pos.y + i * 20),
                    ImVec2(legend_pos.x + 15, legend_pos.y + i * 20 + 15),
                    colors[i]
                );
                
                // Label
                ImGui::SetCursorScreenPos(ImVec2(legend_pos.x + 20, legend_pos.y + i * 20));
                ImGui::Text("%s (%s)", category.c_str(), format_memory_size(size).c_str());
            }
        }
        
        ImGui::Dummy(canvas_sz);
    }
    ImGui::EndChild();
#endif
}

void MemoryObserverPanel::render_memory_map() {
#ifdef ECSCOPE_HAS_GRAPHICS
    ImGui::TextDisabled("Memory Map View (Experimental)");
    ImGui::Text("This feature would show a visual representation");
    ImGui::Text("of memory layout, heap fragmentation,");
    ImGui::Text("and allocation patterns in real-time.");
    
    ImGui::Spacing();
    ImGui::Text("Planned features:");
    ImGui::BulletText("Heap visualization");
    ImGui::BulletText("Fragmentation analysis");
    ImGui::BulletText("Hot/cold memory regions");
    ImGui::BulletText("Allocation lifetime tracking");
    ImGui::BulletText("Cache-friendly pattern detection");
#endif
}

void MemoryObserverPanel::render_controls() {
#ifdef ECSCOPE_HAS_GRAPHICS
    // Update frequency control
    ImGui::SetNextItemWidth(100);
    if (ImGui::DragFloat("Update Hz", &update_frequency_, 0.1f, 1.0f, 60.0f, "%.1f")) {
        update_frequency_ = std::clamp(update_frequency_, 1.0f, 60.0f);
    }
    
    // View toggles
    ImGui::SameLine();
    if (ImGui::Button("Views")) {
        ImGui::OpenPopup("ViewSettings");
    }
    
    if (ImGui::BeginPopup("ViewSettings")) {
        ImGui::Checkbox("Current Stats", &show_current_stats_);
        ImGui::Checkbox("Allocation Graph", &show_allocation_graph_);
        ImGui::Checkbox("Allocator Breakdown", &show_allocator_breakdown_);
        ImGui::Checkbox("Memory Map", &show_memory_map_);
        ImGui::EndPopup();
    }
    
    // Actions
    ImGui::SameLine();
    if (ImGui::Button("Force Update")) {
        update_memory_snapshot();
        analyze_allocation_patterns();
    }
    
    ImGui::SameLine();
    if (ImGui::Button("Export Data")) {
        // TODO: Implement data export
        LOG_INFO("Memory data export not yet implemented");
    }
#endif
}

void MemoryObserverPanel::analyze_allocation_patterns() {
    if (history_count_ < 2) return;
    
    // Calculate allocation rate over recent history
    constexpr usize RATE_WINDOW = 10; // Last 10 samples
    usize samples = std::min(history_count_, RATE_WINDOW);
    
    f64 total_rate = 0.0;
    f64 max_rate = 0.0;
    
    for (usize i = 1; i < samples; ++i) {
        usize curr_idx = (history_head_ + HISTORY_SIZE - i) % HISTORY_SIZE;
        usize prev_idx = (history_head_ + HISTORY_SIZE - i - 1) % HISTORY_SIZE;
        
        const auto& curr = memory_history_[curr_idx];
        const auto& prev = memory_history_[prev_idx];
        
        f64 dt = curr.timestamp - prev.timestamp;
        if (dt > 0.0) {
            f64 rate = static_cast<f64>(curr.total_allocated - prev.total_allocated) / dt;
            total_rate += rate;
            max_rate = std::max(max_rate, rate);
        }
    }
    
    average_allocation_rate_ = samples > 1 ? total_rate / (samples - 1) : 0.0;
    peak_allocation_rate_ = std::max(peak_allocation_rate_, max_rate);
    
    // Simple fragmentation estimation (placeholder)
    // In reality, this would analyze heap structure
    fragmentation_score_ = static_cast<usize>(std::min(50.0, average_allocation_rate_ / 1000.0));
}

MemoryObserverPanel::MemorySnapshot MemoryObserverPanel::get_current_snapshot() const {
    MemorySnapshot snapshot;
    snapshot.timestamp = core::get_time_seconds();
    
    // Get memory statistics from various sources
    auto& registry = ecs::get_registry();
    
    // ECS memory is easy to track
    usize ecs_memory = registry.memory_usage();
    
    // System memory would need platform-specific code
    // For now, use ECS memory as a placeholder
    snapshot.current_usage = ecs_memory;
    snapshot.peak_usage = std::max(ecs_memory, peak_memory_usage());
    
    // Get allocation counts from memory tracker
    snapshot.allocation_count = memory_tracker::get_allocation_count();
    snapshot.free_count = memory_tracker::get_free_count();
    snapshot.total_allocated = snapshot.allocation_count; // Simplified
    snapshot.total_freed = snapshot.free_count;           // Simplified
    
    return snapshot;
}

std::string MemoryObserverPanel::format_memory_size(usize bytes) const {
    return imgui_utils::format_bytes(bytes);
}

std::string MemoryObserverPanel::format_rate(f64 bytes_per_sec) const {
    return imgui_utils::format_bytes(static_cast<usize>(bytes_per_sec)) + "/s";
}

float* MemoryObserverPanel::get_history_buffer(usize& out_count) const {
    // This would return a buffer for ImGui plotting
    // Implementation depends on specific use case
    out_count = history_count_;
    return nullptr; // Placeholder
}

// Memory tracker implementation (simplified)
namespace memory_tracker {
    
    static struct {
        bool initialized{false};
        usize current_usage{0};
        usize peak_usage{0};
        u32 allocation_count{0};
        u32 free_count{0};
        std::vector<std::pair<std::string, usize>> categories;
    } g_tracker;
    
    void initialize() {
        g_tracker = {};
        g_tracker.initialized = true;
        LOG_INFO("Memory tracker initialized");
    }
    
    void shutdown() {
        g_tracker.initialized = false;
        LOG_INFO("Memory tracker shutdown");
    }
    
    void track_allocation(void* ptr, usize size, const char* category) {
        (void)ptr; // Not needed for basic tracking
        
        if (!g_tracker.initialized) return;
        
        g_tracker.current_usage += size;
        g_tracker.peak_usage = std::max(g_tracker.peak_usage, g_tracker.current_usage);
        g_tracker.allocation_count++;
        
        // Update category tracking
        std::string cat = category ? category : "Unknown";
        auto it = std::find_if(g_tracker.categories.begin(), g_tracker.categories.end(),
                              [&cat](const auto& pair) { return pair.first == cat; });
        
        if (it != g_tracker.categories.end()) {
            it->second += size;
        } else {
            g_tracker.categories.emplace_back(cat, size);
        }
    }
    
    void track_deallocation(void* ptr, usize size, const char* category) {
        (void)ptr;
        (void)category; // Category tracking for deallocations is more complex
        
        if (!g_tracker.initialized) return;
        
        g_tracker.current_usage = (size > g_tracker.current_usage) ? 0 : g_tracker.current_usage - size;
        g_tracker.free_count++;
    }
    
    usize get_current_usage() {
        return g_tracker.current_usage;
    }
    
    usize get_peak_usage() {
        return g_tracker.peak_usage;
    }
    
    u32 get_allocation_count() {
        return g_tracker.allocation_count;
    }
    
    u32 get_free_count() {
        return g_tracker.free_count;
    }
    
    std::vector<std::pair<std::string, usize>> get_category_breakdown() {
        return g_tracker.categories;
    }
    
    bool detect_leaks() {
        return g_tracker.allocation_count != g_tracker.free_count;
    }
    
    void dump_allocations(const std::string& filename) {
        LOG_INFO("Memory dump saved to: " + filename + " (not implemented)");
    }
    
} // namespace memory_tracker

} // namespace ecscope::ui